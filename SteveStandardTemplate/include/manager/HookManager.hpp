#pragma once

#include <stdafx.hpp>

namespace SteveBase::Manager {

    using D3D11PresentFn = FuncStdCall<HRESULT(IDXGISwapChain*, UINT, UINT)>;
    using D3D11DrawIndexedFn = FuncStdCall<void(ID3D11DeviceContext*, UINT, UINT, INT)>;
    using D3D11CreateQueryFn = FuncStdCall<void(ID3D11Device*, const D3D11_QUERY_DESC *, ID3D11Query **)>;
    using D3D11EndSceneFn = FuncStdCall<void(ID3D11Device*, const D3D11_QUERY_DESC *, ID3D11Query **)>;

    enum eDepthState : int {
        ENABLED,
        DISABLED,
        READ_NO_WRITE,
        NO_READ_NO_WRITE,
        _DEPTH_COUNT
    };

    class HookManager {
    public:
        static void Init();

        static HRESULT GenerateShader(ID3D11Device* pD3DDevice, ID3D11PixelShader** pShader, float r, float g, float b);
    };
}