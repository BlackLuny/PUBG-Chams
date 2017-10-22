#include "Direct3D11Renderer.hpp"
#include "Direct3D11Texture.hpp"
#include "Direct3D11GeometryBuffer.hpp"
#include "Direct3D11RenderTarget.hpp"
#include "Direct3D11ViewportTarget.hpp"
#include "Direct3D11TextureTarget.hpp"
#include "../../Misc/Exceptions.hpp"

#include <d3dx11effect.h>

#include <vendor/XorStr/XorStr.h>

auto shaderSource = text(R"(
    matrix WorldMatrix;
    matrix ProjectionMatrix;
    Texture2D Texture;
    bool UseTexture;

    struct VSSceneIn
    {
	    float3 pos : POSITION;
	    float4 colour : COLOR;
	    float2 tex : TEXCOORD;
    };

    struct PSSceneIn
    {
	    float4 pos : SV_Position;
	    float4 colour : COLOR;
	    float2 tex : TEXCOORD;
    };
    DepthStencilState DisableDepth
    {
	    DepthEnable = FALSE;
	    DepthWriteMask = ZERO;
    };
    BlendState Normal
    {
	    AlphaToCoverageEnable = false;
	    BlendEnable[0] = true;
	    SrcBlendAlpha = INV_DEST_ALPHA;
	    DestBlendAlpha = ONE;
	    SrcBlend = SRC_ALPHA;
	    DestBlend = INV_SRC_ALPHA;
    };
    SamplerState LinearSampler
    {
	    Filter = MIN_MAG_MIP_LINEAR;
	    AddressU = Clamp;
	    AddressV = Clamp;
    };
    RasterizerState ClipRasterstate
    {
	    DepthClipEnable = false;
	    FillMode = Solid;
	    CullMode = None;
	    ScissorEnable = true;
    };
    RasterizerState NoclipRasterstate
    {
	    DepthClipEnable = false;
	    FillMode = Solid;
	    CullMode = None;
	    ScissorEnable = false;
    };
    // Vertex shader
    PSSceneIn VS(VSSceneIn input)
    {
	    PSSceneIn output;
	    output.pos = mul(float4(input.pos, 1), WorldMatrix);
	    output.pos = mul(output.pos, ProjectionMatrix);
	    output.tex = input.tex;
	    output.colour.rgba = input.colour.bgra;
	    return output;
    }
    // Pixel shader
    float4 PS(PSSceneIn input) : SV_Target
    {
	    float4 colour;
	    if (UseTexture)
		    colour = Texture.Sample(LinearSampler, input.tex) * input.colour;
	    else
		    colour = input.colour;
	    return colour;
    }
    // Techniques
    technique10 ClippedRendering
    {
	    pass P0
	    {
		    SetVertexShader(CompileShader(vs_4_0, VS()));
		    SetGeometryShader(NULL);
		    SetPixelShader(CompileShader(ps_4_0, PS()));
		    SetDepthStencilState(DisableDepth, 0);
		    SetBlendState(Normal, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		    SetRasterizerState(ClipRasterstate);
	    }
    }
    technique10 UnclippedRendering
    {
	    pass P0
	    {
		    SetVertexShader(CompileShader(vs_4_0, VS()));
		    SetGeometryShader(NULL);
		    SetPixelShader(CompileShader(ps_4_0, PS()));
		    SetDepthStencilState(DisableDepth, 0);
		    SetBlendState(Normal, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		    SetRasterizerState(NoclipRasterstate);
	    }
    }
)");

namespace OSHGui
{
	namespace Drawing
	{
		//---------------------------------------------------------------------------
		//Constructor
		//---------------------------------------------------------------------------
		Direct3D11Renderer::Direct3D11Renderer(ID3D11Device *_device, ID3D11DeviceContext *_context)
			: device(_device, _context),
			  displaySize(GetViewportSize()),
			  displayDPI(96, 96),
			  defaultTarget(std::make_shared<Direct3D11ViewportTarget>(*this)),
			  stateBlock(_context),
			  clippedTechnique(nullptr),
			  unclippedTechnique(nullptr),
			  inputLayout(nullptr),
			  worldMatrixVariable(nullptr),
			  projectionMatrixVariable(nullptr),
			  textureVariable(nullptr),
			  useTextureVariable(nullptr)
		{
			ID3D10Blob *errors = nullptr;
			ID3D10Blob *blob = nullptr;
			if (FAILED(D3DX11CompileFromMemory(shaderSource, shaderSource.size(), nullptr, nullptr, nullptr, nullptr, textonce("fx_5_0"), NULL, NULL, nullptr, &blob, &errors, nullptr)))
			{
				std::string msg(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
				errors->Release();
				
				throw Misc::Exception(msg);
			}

			if (FAILED(D3DX11CreateEffectFromMemory(blob->GetBufferPointer(), blob->GetBufferSize(), 0, device.Device, &effect)))
			{
				throw Misc::Exception();
			}

			if (blob)
			{
				blob->Release();
			}

			clippedTechnique = effect->GetTechniqueByName("ClippedRendering");
			unclippedTechnique = effect->GetTechniqueByName("UnclippedRendering");

			worldMatrixVariable = effect->GetVariableByName("WorldMatrix")->AsMatrix();
			projectionMatrixVariable = effect->GetVariableByName("ProjectionMatrix")->AsMatrix();
			textureVariable = effect->GetVariableByName("Texture")->AsShaderResource();
			useTextureVariable = effect->GetVariableByName("UseTexture")->AsScalar();

			const D3D11_INPUT_ELEMENT_DESC vertexLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			D3DX11_PASS_DESC passDescription;
			if (FAILED(clippedTechnique->GetPassByIndex(0)->GetDesc(&passDescription)))
			{
				throw Misc::Exception();
			}

			auto count = sizeof(vertexLayout) / sizeof(vertexLayout[0]);
			if (FAILED(device.Device->CreateInputLayout(vertexLayout, count, passDescription.pIAInputSignature, passDescription.IAInputSignatureSize, &inputLayout)))
			{
				throw Misc::Exception();
			}
		}
		//---------------------------------------------------------------------------
		Direct3D11Renderer::~Direct3D11Renderer()
		{
			if (effect)
			{
				effect->Release();
			}

			if (inputLayout)
			{
				inputLayout->Release();
			}
		}
		//---------------------------------------------------------------------------
		//Getter/Setter
		//---------------------------------------------------------------------------
		RenderTargetPtr& Direct3D11Renderer::GetDefaultRenderTarget()
		{
			return defaultTarget;
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::SetDisplaySize(const SizeF &size)
		{
			if (size != displaySize)
			{
				displaySize = size;

				auto area = defaultTarget->GetArea();
				area.SetSize(size);
				defaultTarget->SetArea(area);
			}
		}
		//---------------------------------------------------------------------------
		const SizeF& Direct3D11Renderer::GetDisplaySize() const
		{
			return displaySize;
		}
		//---------------------------------------------------------------------------
		const PointF& Direct3D11Renderer::GetDisplayDPI() const
		{
			return displayDPI;
		}
		//---------------------------------------------------------------------------
		uint32_t Direct3D11Renderer::GetMaximumTextureSize() const
		{
			return 8192;
		}
		//---------------------------------------------------------------------------
		SizeF Direct3D11Renderer::GetViewportSize()
		{
			D3D11_VIEWPORT vp;
			uint32_t count = 1;

			device.Context->RSGetViewports(&count, &vp);

			if (count != 1)
			{
				throw Misc::Exception();
			}

			return SizeF(vp.Width, vp.Height);
		}
		//---------------------------------------------------------------------------
		Direct3D11Renderer::IDevice11& Direct3D11Renderer::GetDevice()
		{
			return device;
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::BindTechniquePass(const bool clipped)
		{
			if (clipped)
			{
				clippedTechnique->GetPassByIndex(0)->Apply(0, device.Context);
			}
			else
			{
				unclippedTechnique->GetPassByIndex(0)->Apply(0, device.Context);
			}
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::SetCurrentTextureShaderResource(ID3D11ShaderResourceView *srv)
		{
			textureVariable->SetResource(srv);
			useTextureVariable->SetBool(srv != nullptr);
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::SetProjectionMatrix(D3DXMATRIX &matrix)
		{
			projectionMatrixVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::SetWorldMatrix(D3DXMATRIX& matrix)
		{
			worldMatrixVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
		}
		//---------------------------------------------------------------------------
		//Runtime-Functions
		//---------------------------------------------------------------------------
		GeometryBufferPtr Direct3D11Renderer::CreateGeometryBuffer()
		{
			return std::make_shared<Direct3D11GeometryBuffer>(*this);
		}
		//---------------------------------------------------------------------------
		TextureTargetPtr Direct3D11Renderer::CreateTextureTarget()
		{
			return std::make_shared<Direct3D11TextureTarget>(*this);
		}
		//---------------------------------------------------------------------------
		TexturePtr Direct3D11Renderer::CreateTexture()
		{
			return std::shared_ptr<Direct3D11Texture>(new Direct3D11Texture(device));
		}
		//---------------------------------------------------------------------------
		TexturePtr Direct3D11Renderer::CreateTexture(const Misc::AnsiString &filename)
		{
			return std::shared_ptr<Direct3D11Texture>(new Direct3D11Texture(device, filename));
		}
		//---------------------------------------------------------------------------
		TexturePtr Direct3D11Renderer::CreateTexture(const SizeF &size)
		{
			return std::shared_ptr<Direct3D11Texture>(new Direct3D11Texture(device, size));
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::BeginRendering()
		{
			stateBlock.Capture();

			device.Context->IASetInputLayout(inputLayout);
			device.Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
		//---------------------------------------------------------------------------
		void Direct3D11Renderer::EndRendering()
		{
			stateBlock.Apply();

			stateBlock.Release();
		}
		//---------------------------------------------------------------------------
	}
}