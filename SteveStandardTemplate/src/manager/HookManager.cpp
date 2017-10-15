#include <stdafx.hpp>

#include <manager/HookManager.hpp>
#include <utility/SystemUtility.hpp>
#include <vendor/minhook/include/MinHook.h>

namespace SteveBase::Manager {

    const string shader_format = text(R"(
        struct VS_OUT
        {
            float4 Position : SV_Position;
            float4 Color : COLOR0;
        };

        float4 main( VS_OUT input ) : SV_Target
        {
            float4 fake;
            fake.a = 1.f;
            fake.r = %f;
            fake.g = %f;
            fake.b = %f;
            return fake;
        }
    )");

    // hook
    D3D11PresentFn phookD3D11Present = nullptr;
    D3D11DrawIndexedFn phookD3D11DrawIndexed = nullptr;
    D3D11CreateQueryFn phookD3D11CreateQuery = nullptr;

    // object
    ID3D11Device *pDevice = nullptr;
    ID3D11DeviceContext *pContext = nullptr;

    // vtable
    DWORD_PTR* pSwapChainVtable = nullptr;
    DWORD_PTR* pContextVTable = nullptr;
    DWORD_PTR* pDeviceVTable = nullptr;

    // rendertarget
    ID3D11Texture2D* RenderTargetTexture;
    ID3D11RenderTargetView* RenderTargetView = nullptr;

    // shader
    ID3D11PixelShader* psRed = nullptr;
    ID3D11PixelShader* psOrange = nullptr;

    ID3D11DepthStencilState* myDepthStencilStates[eDepthState::_DEPTH_COUNT];

    void SetDepthStencilState(const eDepthState aState) {
        pContext->OMSetDepthStencilState(myDepthStencilStates[aState], 1);
    }

    const int MultisampleCount = 1; // Set to 1 to disable multisampling

    bool hack_switch = false;
    bool bOnce = false;

    LRESULT CALLBACK DXGIMsgProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, const UINT SyncInterval, const UINT Flags) {
        if (!bOnce) {
            bOnce = true;

            //get device and context
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pDevice))) {
                pSwapChain->GetDevice(__uuidof(pDevice), (void**)&pDevice);
                pDevice->GetImmediateContext(&pContext);
            }

            //create depthstencilstate
            D3D11_DEPTH_STENCIL_DESC  stencilDesc;
            stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
            stencilDesc.StencilEnable = true;
            stencilDesc.StencilReadMask = 0xFF;
            stencilDesc.StencilWriteMask = 0xFF;
            stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
            stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
            stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

            stencilDesc.DepthEnable = true;
            stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::ENABLED)]);

            stencilDesc.DepthEnable = false;
            stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::DISABLED)]);

            stencilDesc.DepthEnable = false;
            stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            stencilDesc.StencilEnable = false;
            stencilDesc.StencilReadMask = UINT8(0xFF);
            stencilDesc.StencilWriteMask = 0x0;
            pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::NO_READ_NO_WRITE)]);

            stencilDesc.DepthEnable = true;
            stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; //
            stencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
            stencilDesc.StencilEnable = false;
            stencilDesc.StencilReadMask = UINT8(0xFF);
            stencilDesc.StencilWriteMask = 0x0;

            stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
            stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
            stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

            stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
            stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
            stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
            stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
            pDevice->CreateDepthStencilState(&stencilDesc, &myDepthStencilStates[static_cast<int>(eDepthState::READ_NO_WRITE)]);

            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&RenderTargetTexture))) {
                pDevice->CreateRenderTargetView(RenderTargetTexture, NULL, &RenderTargetView);
                RenderTargetTexture->Release();
            }

        }

        if (GetAsyncKeyState(VK_INSERT) & 0x0001) hack_switch = !hack_switch;

        if (!psRed) HookManager::GenerateShader(pDevice, &psRed, 1.0f, 0.0f, 0.0f);

        if (!psOrange) HookManager::GenerateShader(pDevice, &psOrange, 1.f, 0.3f, 0);

        if (hack_switch) {
            //call before you draw
            pContext->OMSetRenderTargets(1, &RenderTargetView, NULL);
        }

        return phookD3D11Present(pSwapChain, SyncInterval, Flags);
    }

    //vertex
    ID3D11Buffer *veBuffer;
    UINT Stride = 0;
    UINT veBufferOffset = 0;
    D3D11_BUFFER_DESC vedesc;

    //index
    ID3D11Buffer *inBuffer;
    DXGI_FORMAT inFormat;
    UINT        inOffset;
    D3D11_BUFFER_DESC indesc;

    void __stdcall hookD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        if (hack_switch) {
            //get stride & vdesc.ByteWidth
            pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
            if (veBuffer) veBuffer->GetDesc(&vedesc);
            if (veBuffer != nullptr) { veBuffer->Release(); veBuffer = NULL; }

            //get indesc.ByteWidth
            pContext->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
            if (inBuffer) inBuffer->GetDesc(&indesc);
            if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }

            if (Stride == 36) // PLAYERS
            {
                SetDepthStencilState(DISABLED);
                pContext->PSSetShader(psRed, NULL, NULL);
                phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
                pContext->PSSetShader(psOrange, NULL, NULL);
                //if (pssrStartSlot == 1) //if black screen, find correct pssrStartSlot
                SetDepthStencilState(READ_NO_WRITE);
            }
        }

        return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    void __stdcall hookD3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery) {
        //Disable Occlusion which prevents rendering player models through certain objects (used by wallhack to see models through walls at all distances, REDUCES FPS)
        if (hack_switch && pQueryDesc->Query == D3D11_QUERY_OCCLUSION) {
            D3D11_QUERY_DESC oqueryDesc;
            (&oqueryDesc)->MiscFlags = pQueryDesc->MiscFlags;
            (&oqueryDesc)->Query = D3D11_QUERY_TIMESTAMP;

            return phookD3D11CreateQuery(pDevice, &oqueryDesc, ppQuery);
        }

        return phookD3D11CreateQuery(pDevice, pQueryDesc, ppQuery);
    }

    void HookManager::Init() {
        SystemUtility::SafeGetModuleHandle(text("dxgi.dll"));
        Sleep(100);

        IDXGISwapChain* pSwapChain;

        WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(nullptr), nullptr, nullptr, nullptr, nullptr, "DX", nullptr };
        RegisterClassExA(&wc);
        const auto hWnd = CreateWindowA(text("DX").c_str(), NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

        D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
        D3D_FEATURE_LEVEL obtainedLevel;

        DXGI_SWAP_CHAIN_DESC scd;
        ZeroMemory(&scd, sizeof(scd));
        scd.BufferCount = 1;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

        scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        scd.OutputWindow = hWnd;
        scd.SampleDesc.Count = MultisampleCount;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        scd.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

        // LibOVR 0.4.3 requires that the width and height for the backbuffer is set even if
        // you use windowed mode, despite being optional according to the D3D11 documentation.
        scd.BufferDesc.Width = 1;
        scd.BufferDesc.Height = 1;
        scd.BufferDesc.RefreshRate.Numerator = 0;
        scd.BufferDesc.RefreshRate.Denominator = 1;

        UINT createFlags = 0;
#ifdef _DEBUG
        // This flag gives you some quite wonderful debug text. Not wonderful for performance, though!
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        if (FAILED(D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createFlags,
            requestedLevels,
            sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
            D3D11_SDK_VERSION,
            &scd,
            &pSwapChain,
            &pDevice,
            &obtainedLevel,
            &pContext))) {
            MessageBox(hWnd, text("Failed to create directX device and swapchain!").c_str(), text("Error").c_str(), MB_ICONERROR);
            
        }


        pSwapChainVtable = (DWORD_PTR*)pSwapChain;
        pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

        LoggerDebug("pSwapChainVtable: 0x{0:X}", (uint64_t) pSwapChainVtable);

        pContextVTable = (DWORD_PTR*)pContext;
        pContextVTable = (DWORD_PTR*)pContextVTable[0];

        LoggerDebug("pContextVTable: 0x{0:X}", (uint64_t) pContextVTable);

        pDeviceVTable = (DWORD_PTR*)pDevice;
        pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];

        LoggerDebug("pDeviceVTable: 0x{0:X}", (uint64_t) pDeviceVTable);

        // save old function
        phookD3D11Present = (D3D11PresentFn)pSwapChainVtable[8];
        phookD3D11DrawIndexed = (D3D11DrawIndexedFn)pContextVTable[12];
        phookD3D11CreateQuery = (D3D11CreateQueryFn)pDeviceVTable[24];

        // change the shit here, or you will be banned
        if (MH_Initialize() == MH_OK) {
            if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[8], hookD3D11Present, reinterpret_cast<void**>(&phookD3D11Present)) != MH_OK) { return; }
            if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return; }
            
            if (MH_CreateHook((DWORD_PTR*)pContextVTable[12], hookD3D11DrawIndexed, reinterpret_cast<void**>(&phookD3D11DrawIndexed)) != MH_OK) { return; }
            if (MH_EnableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return; }

            if (MH_CreateHook((DWORD_PTR*)pDeviceVTable[24], hookD3D11CreateQuery, reinterpret_cast<void**>(&phookD3D11CreateQuery)) != MH_OK) { return; }
            if (MH_EnableHook((DWORD_PTR*)pDeviceVTable[24]) != MH_OK) { return; }
            
            DWORD dwOld;
            VirtualProtect(phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);
        }
    }

    HRESULT HookManager::GenerateShader(ID3D11Device *pD3DDevice, ID3D11PixelShader **pShader, float r, float g, float b) {
        ID3D10Blob* pBlob;
        char szPixelShader[1000];
        sprintf_s(szPixelShader, shader_format.c_str(), r, g, b);

        ID3DBlob* d3dErrorMsgBlob;

        auto hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", nullptr, nullptr, "main", "ps_4_0", NULL, NULL, &pBlob, &d3dErrorMsgBlob);

        if (FAILED(hr)) {
            return hr;
        }

        hr = pD3DDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, pShader);

        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }
}
