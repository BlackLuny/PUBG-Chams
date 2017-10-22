#pragma once

#include <stdafx.hpp>

namespace SteveBase::Manager {
    enum DepthState : int {
        Enabled,
        Disabled,
        ReadNoWrite,
        NoReadNoWrite,
        StateCount
    };

    class HookManager {
    public:
        static void Init();

        static HRESULT GenerateShader(ID3D11Device* pD3DDevice, ID3D11PixelShader** pShader, float r, float g, float b);
    };
}