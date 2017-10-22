#include <stdafx.hpp>

#include <manager/HookManager.hpp>
#include <utility/SystemUtility.hpp>
#include <manager/PatternManager.hpp>
#include <utility/Logger.hpp>

// #include <vendor/FW1FontWrapper/FW1FontWrapper.h>
#include <vendor/imgui/imgui.h>
#include <vendor/imgui/examples/directx11_example/imgui_impl_dx11.h>

namespace SteveBase::Manager {
    using namespace Utility;


    // hook
    Func<void(const void *, const void *, const bool, const bool)> origRHIEndDrawingViewportFn;
    FuncStdCall<void(ID3D11DeviceContext*, UINT, UINT, INT)> origDrawIndexedFn;
    FuncStdCall<void(ID3D11Device*, const D3D11_QUERY_DESC *, ID3D11Query **)> origCreateQueryFn;
    FuncStdCall<HRESULT(IDXGISwapChain*, UINT, UINT)> origPresentFn;

    // object
    IDXGISwapChain* g_swapChain;
    ID3D11Device *g_device;
    ID3D11DeviceContext *g_context;

    // shader
    ID3D11PixelShader* g_redShader;
    ID3D11PixelShader* g_orangeShader;

    // rendertarget
    ID3D11Texture2D* g_renderTargetTexture;
    ID3D11RenderTargetView* g_renderTargetView;

    ID3D11DepthStencilState* depthStencilStates[DepthState::StateCount];

    const int gameObjectStride = 36; // what the fuck is this?
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa473780(v=vs.85).aspx

    const int multisampleCount = 1; // set to 1 to disable multisampling


#if 0
    LRESULT CALLBACK DxgiMsgProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam) $(DefWindowProc(hwnd, uMsg, wParam, lParam));

    BOOL CALLBACK windowCallback(HWND hWnd, long lParam) {
        char buff[255], buff2[255];

        if (IsWindowVisible(hWnd)) {
            GetWindowText(hWnd, buff, 254);
            GetClassName(hWnd, buff2, 254);
            LoggerDebug("hWnd name: {}, {}", buff, buff2);

        }

        return TRUE;

    };
#endif

    HWND window;
    bool d3dHookInitialized = false;

    // i know the structure is fucked up
    // but i liked it :)
    struct {
        struct {
            bool enabled = false;
        } chams;

        struct {
            bool enabled = false;
            struct {
                int value = 0;
            } stride;

            struct {
                bool enabled = false;
                int value = 0;
            } veByteWidth;

            struct {
                bool enabled = false;
                int value = 0;
            } inByteWidth;
        } modelLogger;
    } variable;

    bool menuSwitch = false; // using VK_INSERT right now

    // self: rcx, viewportRHI: rdx, present: r8, lockToVSync: r9
    void RHIEndDrawingViewportHook(const void *self, const void *viewportRHI, const bool present, const bool lockToVSync) {
        g_swapChain = *(IDXGISwapChain**)((uintptr_t)viewportRHI + 96);

        if (!d3dHookInitialized && SUCCEEDED(g_swapChain->GetDevice(__uuidof(ID3D11Device), (void **)&g_device))) {
            g_swapChain->GetDevice(__uuidof(g_device), (void**)&g_device);
            g_device->GetImmediateContext(&g_context);

            // base config
            D3D11_DEPTH_STENCIL_DESC desc;
            {
                desc.DepthFunc = D3D11_COMPARISON_LESS;
                desc.StencilEnable = true;
                desc.StencilReadMask = 0xFF;
                desc.StencilWriteMask = 0xFF;
                desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
                desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
                desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
                desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
                desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            }

            // Enabled
            auto &enabled = desc;
            {
                enabled.DepthEnable = true;
            }
            g_device->CreateDepthStencilState(&enabled, &depthStencilStates[static_cast<int>(DepthState::Enabled)]);


            // Disabled
            auto &disabled = enabled;
            {
                disabled.DepthEnable = false;
            }
            g_device->CreateDepthStencilState(&disabled, &depthStencilStates[static_cast<int>(DepthState::Disabled)]);

            // NoReadNoWrite
            auto &noReadNoWrite = disabled;
            {
                noReadNoWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
                noReadNoWrite.StencilEnable = false;
                noReadNoWrite.StencilReadMask = UINT8(0xFF);
                noReadNoWrite.StencilWriteMask = 0x0;
            }
            g_device->CreateDepthStencilState(&noReadNoWrite, &depthStencilStates[static_cast<int>(DepthState::NoReadNoWrite)]);


            // ReadNoWrite
            auto &readNoWrite = noReadNoWrite;
            {
                readNoWrite.DepthEnable = true;
                readNoWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                readNoWrite.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
                {
                    auto &frontFace = readNoWrite.FrontFace;
                    auto &backFace = readNoWrite.BackFace;

                    frontFace.StencilFailOp = backFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
                    frontFace.StencilDepthFailOp = backFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;

                    frontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                    frontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

                    backFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
                    backFace.StencilFunc = D3D11_COMPARISON_NEVER;
                }
            }
            g_device->CreateDepthStencilState(&readNoWrite, &depthStencilStates[static_cast<int>(DepthState::ReadNoWrite)]);


            // red shader
            if (!g_redShader) {
                HookManager::GenerateShader(g_device, &g_redShader, 1.0f, 0.0f, 0.0f);
            }

            // orange shader
            if (!g_orangeShader) {
                HookManager::GenerateShader(g_device, &g_orangeShader, 1.f, 0.3f, 0);
            }

            ImGui_ImplDX11_Init(window, g_device, g_context);
            ImGui_ImplDX11_CreateDeviceObjects();

            if (SUCCEEDED(g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&g_renderTargetTexture))) {
                g_device->CreateRenderTargetView(g_renderTargetTexture, nullptr, &g_renderTargetView);
                g_renderTargetTexture->Release();
            }


            d3dHookInitialized = true;
        }

        g_context->OMSetRenderTargets(1, &g_renderTargetView, nullptr);

        ImGui_ImplDX11_NewFrame();

        // if you like, you can even stub OSHGui here
        // included in my vendor collection
        if (menuSwitch) {
            ImGui::Begin(textonce("Model Logger"), nullptr, {300, 200});
            {
                ImGui::Checkbox(textonce("Chams Enabled?"), &variable.chams.enabled);
                ImGui::Checkbox(textonce("Enabled?"), &variable.modelLogger.enabled);
                if (variable.modelLogger.enabled) {
                    if (variable.modelLogger.enabled) {
                        ImGui::InputInt(textonce("Stride: "), &variable.modelLogger.stride.value);
                        
                    }

                    ImGui::Checkbox(textonce("In-Byte Width?"), &variable.modelLogger.inByteWidth.enabled);
                    if (variable.modelLogger.inByteWidth.enabled) {
                        ImGui::InputInt(textonce("inByte"), &variable.modelLogger.inByteWidth.value);
                        ImGui::SliderInt(textonce("inByte: "), &variable.modelLogger.inByteWidth.value, 0, 10000000);
                    }

                    ImGui::Checkbox(textonce("Ve-Byte Width?"), &variable.modelLogger.veByteWidth.enabled);
                    if (variable.modelLogger.veByteWidth.enabled) {
                        ImGui::InputInt(textonce("veByte"), &variable.modelLogger.veByteWidth.value);
                        ImGui::SliderInt(textonce("veByte: "), &variable.modelLogger.veByteWidth.value, 0, 10000000);
                    }
                }
            }
            ImGui::End();
        }

        ImGui::Render();

        // LoggerDebug("RHIEndDrawingViewport");
        // handover control
        origRHIEndDrawingViewportFn(self, viewportRHI, present, lockToVSync);
    }

    ID3D11Buffer *veBuffer;
    UINT stride;
    D3D11_BUFFER_DESC vedesc;

    ID3D11Buffer *inBuffer;
    DXGI_FORMAT inFormat;
    UINT        inOffset;
    D3D11_BUFFER_DESC indesc;

    void __stdcall DrawIndexedHook(ID3D11DeviceContext* context, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
        auto applyChams = [&]() {
            context->OMSetDepthStencilState(depthStencilStates[Disabled], 1);
            {
                context->PSSetShader(g_redShader, nullptr, 0);
                origDrawIndexedFn(context, IndexCount, StartIndexLocation, BaseVertexLocation);
                context->PSSetShader(g_orangeShader, nullptr, 0);
            }
            context->OMSetDepthStencilState(depthStencilStates[ReadNoWrite], 1);
        };

        context->IAGetVertexBuffers(0, 1, &veBuffer, &stride, nullptr);
        if (veBuffer != nullptr) {
            veBuffer->GetDesc(&vedesc);
            veBuffer->Release();
        }

        //get indesc.ByteWidth
        context->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
        if (inBuffer != nullptr) {
            inBuffer->GetDesc(&indesc);
            inBuffer->Release();
        }

        if (variable.modelLogger.enabled) {
            
            // well, i dont know how to filter the conditions here
            // fucking de morgan's law
            if (
                (stride != variable.modelLogger.stride.value) ||
                (variable.modelLogger.inByteWidth.enabled && variable.modelLogger.inByteWidth.value * stride != indesc.ByteWidth) ||
                (variable.modelLogger.veByteWidth.enabled && variable.modelLogger.veByteWidth.value * stride != vedesc.ByteWidth)
            ) {
                ;
            } else {
                // LoggerDebug("Applying chams for Stride: {}, BW: {}", stride, vedesc.ByteWidth);
                applyChams();
            }
            

        }
    
        if (variable.chams.enabled) {
            if (stride == gameObjectStride) {
                const bool notBrokenGlass = vedesc.ByteWidth <= 170000;

                const bool buggy =
                    vedesc.ByteWidth == (122766 * gameObjectStride)
                    || vedesc.ByteWidth == (21363 * gameObjectStride)
                    || vedesc.ByteWidth == (6343 * gameObjectStride);

                const bool dacia =
                    vedesc.ByteWidth == (30049 * gameObjectStride)
                    || vedesc.ByteWidth == (17292 * gameObjectStride)
                    || vedesc.ByteWidth == (6481 * gameObjectStride);

                const bool uaz =
                    vedesc.ByteWidth == (46533 * gameObjectStride) || vedesc.ByteWidth == (28729 * gameObjectStride) || vedesc.ByteWidth == (9815 * gameObjectStride)  /* UAZ */
                    || vedesc.ByteWidth == (48197 * gameObjectStride) || vedesc.ByteWidth == (31616 * gameObjectStride) || vedesc.ByteWidth == (10102 * gameObjectStride)  /* UAZ B */
                    || vedesc.ByteWidth == (46717 * gameObjectStride) || vedesc.ByteWidth == (26879 * gameObjectStride) || vedesc.ByteWidth == (9906 * gameObjectStride); /* UAZ C */

                const bool destroyedDoor =
                    vedesc.ByteWidth == (72170 * gameObjectStride)  /* Destroyed Door A */
                    || vedesc.ByteWidth == (39156 * gameObjectStride)  /* Destroyed Door A2 */
                    || vedesc.ByteWidth == (44708 * gameObjectStride); /* Destroyed Door F */

                const bool destroyedWood =
                    vedesc.ByteWidth == (28286 * gameObjectStride)  /* Destroyed Wood E11 */
                    || vedesc.ByteWidth == (24923 * gameObjectStride); /* Destroyed Wood E12 */

                const bool destroyedFence =
                    vedesc.ByteWidth == (23026 * gameObjectStride)  /* Destroyed Fence Short A */
                    || vedesc.ByteWidth == (19196 * gameObjectStride)  /* Destroyed Fence Short B */
                    || vedesc.ByteWidth == (15332 * gameObjectStride)  /* Destroyed Fence Short C */
                    || vedesc.ByteWidth == (23035 * gameObjectStride)  /* Destroyed Fence A */
                    || vedesc.ByteWidth == (25962 * gameObjectStride)  /* Destroyed Fence B */
                    || vedesc.ByteWidth == (19931 * gameObjectStride)  /* Destroyed Fence C */
                    || vedesc.ByteWidth == (19222 * gameObjectStride); /* Destroyed Fence D */

                const bool campHouseWall =
                    vedesc.ByteWidth == (11506 * gameObjectStride)  /* CampHouse Wall Back 3 */
                    || vedesc.ByteWidth == (17751 * gameObjectStride)  /* CampHouse Wall Back 2 */
                    || vedesc.ByteWidth == (13637 * gameObjectStride)  /* CampHouse Wall Back 1 */
                    || vedesc.ByteWidth == (6056 * gameObjectStride)  /* CampHouse Wall Right 1 */
                    || vedesc.ByteWidth == (12765 * gameObjectStride)  /* CampHouse Wall Right 2 */
                    || vedesc.ByteWidth == (11164 * gameObjectStride)  /* CampHouse Wall Left 2 */
                    || vedesc.ByteWidth == (9413 * gameObjectStride)  /* CampHouse Wall Left 1 */
                    || vedesc.ByteWidth == (9596 * gameObjectStride)  /* CampHouse Wall Door 1 */
                    || vedesc.ByteWidth == (6793 * gameObjectStride); /* CampHouse Wall Door 2 */

                const bool weapons =
                    vedesc.ByteWidth == (8658 * gameObjectStride)  /* M16 */
                    || vedesc.ByteWidth == (8351 * gameObjectStride)  /* AK */
                    || vedesc.ByteWidth == (9974 * gameObjectStride)  /* SCAR-L */
                    || vedesc.ByteWidth == (12166 * gameObjectStride)  /* HK416 */
                    || vedesc.ByteWidth == (6518 * gameObjectStride)  /* UZI */
                    || vedesc.ByteWidth == (12021 * gameObjectStride)  /* SKS */
                    || vedesc.ByteWidth == (5470 * gameObjectStride)  /* S686 */
                    || vedesc.ByteWidth == (36723 * gameObjectStride)  /* Kar98K */
                    || vedesc.ByteWidth == (6147 * gameObjectStride); /* AWM */

                const bool vehicle = buggy || dacia || uaz;
                const bool destroyed = destroyedDoor || destroyedWood || destroyedFence;

                if (!(notBrokenGlass || destroyed || campHouseWall)) {
                    applyChams();
                }

            }
        }
    
        // LoggerDebug("DrawIndexed");
        // handover control
        origDrawIndexedFn(context, IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    void __stdcall CreateQueryHook(ID3D11Device* device, D3D11_QUERY_DESC *queryDesc, ID3D11Query **ppQuery) {
        if (variable.chams.enabled) {
            if (queryDesc->Query == D3D11_QUERY_OCCLUSION) {
                // disable Occlusion which prevents rendering player models through certain objects (used by wallhack to see models through walls at all distances, REDUCES FPS)
                queryDesc->Query = D3D11_QUERY_TIMESTAMP;
            }
        }

        // LoggerDebug("CreateQuery");
        // handover control
        origCreateQueryFn(device, queryDesc, ppQuery);
    }

    WNDPROC oldWindowProcedure;

    LRESULT CALLBACK OnWindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (d3dHookInitialized) {
            if (GetAsyncKeyState(VK_INSERT) & 0x0001) {
                menuSwitch = !menuSwitch;
            }

            switch (msg) {
                case WM_SIZE:
                    if (g_device != nullptr && wParam != SIZE_MINIMIZED) {
                        ImGui_ImplDX11_InvalidateDeviceObjects();
                        // g_swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                        ImGui_ImplDX11_CreateDeviceObjects();
                    }
                    return 0;
                default: break;
            }

            ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam);
        }

        // LoggerDebug("Window");

        return CallWindowProc(oldWindowProcedure, hWnd, msg, wParam, lParam);
    }

    constexpr auto requestedLevels = Misc::make_array(D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1);

    void HookManager::Init() {
        SystemUtility::SafeGetModuleHandle(textonce("dxgi.dll"));
        do Sleep(10), window = FindWindow(textonce("UnrealWindow"), textonce("PLAYERUNKNOWN'S BATTLEGROUNDS ")); while (window == nullptr);

        UINT createFlags = 0;

#if 0
        // This flag gives you some quite wonderful debug text. Not wonderful for performance, though!
        // createFlags |= D3D11_CREATE_DEVICE_DEBUG;

        WNDCLASSEXA wc = { sizeof(WNDCLASSEX) };
        {
            wc.style = CS_CLASSDC;
            wc.lpfnWndProc = DxgiMsgProc;
            wc.cbClsExtra = wc.cbWndExtra = 0L;
            wc.hInstance = GetModuleHandleA(nullptr);
            wc.hIcon = nullptr;
            wc.hCursor = nullptr;
            wc.hbrBackground = nullptr;
            wc.lpszMenuName = nullptr;
            wc.lpszClassName = _strdup(text("DX").c_str());
            wc.hIconSm = nullptr;
        }

        RegisterClassExA(&wc);
        const auto hWnd = CreateWindowA(
            text("DX").c_str(),
            nullptr,
            WS_OVERLAPPEDWINDOW,
            100, 100,
            300, 300,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );
#endif

#if 0
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = multisampleCount;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Windowed = (GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) == 0U;

        auto &bufferDesc = swapChainDesc.BufferDesc;
        {
            bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

            // LibOVR 0.4.3 requires that the width and height for the backbuffer is set even if
            // you use windowed mode, despite being optional according to the D3D11 documentation.
            bufferDesc.Width = 1;
            bufferDesc.Height = 1;
            bufferDesc.RefreshRate.Numerator = 0;
            bufferDesc.RefreshRate.Denominator = 1;
        }
#endif

        // D3D_FEATURE_LEVEL obtainedLevel;
        auto ret = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createFlags,
            requestedLevels.data(),
            (UINT)requestedLevels.size(),
            D3D11_SDK_VERSION,
            &g_device,
            nullptr,
            &g_context
        );

        if (FAILED(ret)) {
#if DEBUG == 1
            MessageBox(nullptr, textonce("Failed to create D3D pointers!"), textonce("Error"), MB_ICONERROR);
#endif
            exit(0);
        }

        oldWindowProcedure = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (DWORD_PTR)OnWindowProcedure);

        // CAVEAT: DON'T USE DWORD, DWORD IS 32-BIT BOUND
        // DWORD_PTR HOWEVER ISN'T
        // LoggerDebug("swapChain: 0x{0:X}", (uint64_t)swapChain);
        LoggerDebug("context: 0x{0:X}", (uint64_t)g_context);
        LoggerDebug("device: 0x{0:X}", (uint64_t)g_device);

        auto contextProxy = (Misc::ProxiedClass*) g_context;
        auto deviceProxy = (Misc::ProxiedClass*) g_device;

        LoggerDebug("context vtbl: 0x{0:X}", (uint64_t)contextProxy->GetVMT());
        LoggerDebug("device vtbl: 0x{0:X}", (uint64_t)deviceProxy->GetVMT());

        // change the shit here, or you will be banned


        /*
        DWORD old;

        auto vtable = contextProxy->GetVMT();
        VirtualProtect(vtable, 1024, PAGE_EXECUTE_READWRITE, &old);
        origDrawIndexedFn = (decltype(origDrawIndexedFn))vtable[12];
        vtable[12] = (Misc::ProxiedClass::vfunc_t) DrawIndexedHook;
        VirtualProtect(vtable, 1024, old, nullptr);


        VirtualProtect(&deviceProxy->GetVMT()[24], sizeof LPVOID, PAGE_EXECUTE_READWRITE, &old);
        origCreateQueryFn = deviceProxy->GetVFunc<decltype(origCreateQueryFn)>(24);
        deviceProxy->GetVMT()[24] = (Misc::ProxiedClass::vfunc_t) CreateQueryHook;
        VirtualProtect(&deviceProxy->GetVMT()[24], sizeof LPVOID, old, nullptr);
        */

        if (MH_Initialize() == MH_OK) {
            auto fn = (LPVOID)GetPattern("TslGame.exe")("RHIEndDrawingViewport");
#if DEBUG == 1
#define CREATE_HOOK(dest, from, outOrig) \
    do {\
        if (MH_OK != MH_CreateHook((dest), (from), (LPVOID *) (outOrig))) { \
            std::array<char, 512> buf = {}; \
                auto fmt = textonce("Failed to create hook for {0} on 0x{1:X}"); \
                sprintf(buf.data(), fmt, textonce(#from), (uintptr_t)dest); \
                MessageBox(nullptr, buf.data(), text("Error").c_str(), MB_ICONERROR); \
                LoggerError("Failed to create hook for {0} on 0x{1:X}", #from, (uintptr_t)dest); \
                exit(0); \
        } \
    if (MH_OK != MH_EnableHook((dest))) { \
            std::array<char, 512> buf = {}; \
            auto fmt = textonce("Failed to enable hook for {0} on 0x{1:X}"); \
            sprintf(buf.data(), fmt, textonce(#from), (uintptr_t)dest); \
            MessageBox(nullptr, buf.data(), text("Error").c_str(), MB_ICONERROR); \
            LoggerError("Failed to enable hook for {0} on 0x{1:X}", #from, (uintptr_t)dest); \
            exit(0); \
    } \
    } while (0)
#else
#define CREATE_HOOK(dest, from, outOrig) \
    do {\
        if (MH_OK != MH_CreateHook((dest), (from), (LPVOID *) (outOrig))) exit(0); \
        if (MH_OK != MH_EnableHook((dest))) exit(0); \
    } while (0)

#endif

            CREATE_HOOK(fn, RHIEndDrawingViewportHook, &origRHIEndDrawingViewportFn);

            CREATE_HOOK(contextProxy->GetVFunc<LPVOID>(12), DrawIndexedHook, &origDrawIndexedFn);
            CREATE_HOOK(deviceProxy->GetVFunc<LPVOID>(24), CreateQueryHook, &origCreateQueryFn);
        }

    }

    HRESULT HookManager::GenerateShader(ID3D11Device *pD3DDevice, ID3D11PixelShader **pShader, float r, float g, float b) {
        // shader
        const auto shader_format = textonce(R"(
            struct VS_OUT {
                float4 Position : SV_Position;
                float4 Color : COLOR0;
            };

            float4 main( VS_OUT input ) : SV_Target {
                float4 fake;
                fake.a = 1.f;
                fake.r = %f;
                fake.g = %f;
                fake.b = %f;
                return fake;
            }
        )");

        ID3D10Blob* pBlob;
        ID3DBlob* d3dErrorMsgBlob;

        std::array<char, 512> buf = {};
        sprintf(buf.data(), shader_format, r, g, b);

        auto ret = D3DCompile(
            buf.data(),
            buf.size(),
            textonce("shader"),
            nullptr,
            nullptr,
            textonce("main"),
            textonce("ps_4_0"),
            0, 0,
            &pBlob,
            &d3dErrorMsgBlob
        );

        if (FAILED(ret)) {
            return ret;
        }

        ret = pD3DDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, pShader);

        if (FAILED(ret)) {
            return ret;
        }

        return S_OK;
    }
}
