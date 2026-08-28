#pragma once

#include "d3d11.h"
#include "d3d9.h"

namespace ColorCorrection
{
    void Setup();
    void Init();
    void Draw(IDXGISwapChain* swap);

    void DrawD3D9(IDirect3DDevice9* device);
    void ReleaseD3D9();

    inline bool bEnabled = false;
    inline bool bShaderLoaded = false;
    inline bool bShaderLoadedD3D9 = false;
}
