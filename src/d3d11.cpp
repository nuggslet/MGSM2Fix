#include "m2fix.h"

#include "d3d11.h"

#if defined(M2FIX_USE_IMGUI)
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#endif

#include "resource.h"

HRESULT WINAPI D3D11::Device::CreateTexture2D(
    ID3D11Device           *pDevice,
    D3D11_TEXTURE2D_DESC   *pDesc,
    D3D11_SUBRESOURCE_DATA *pInitialData,
    ID3D11Texture2D        **ppTexture2D
) {
    return D3D11::GetInstance().CreateTexture2D(
        D3D11::Device::CreateTexture2D,
        pDevice,
        pDesc,
        pInitialData,
        ppTexture2D
    );
}

HRESULT WINAPI D3D11::CreateTexture2D(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device           *pDevice,
        D3D11_TEXTURE2D_DESC   *pDesc,
        D3D11_SUBRESOURCE_DATA *pInitialData,
        ID3D11Texture2D        **ppTexture2D
    ),
    ID3D11Device           *pDevice,
    D3D11_TEXTURE2D_DESC   *pDesc,
    D3D11_SUBRESOURCE_DATA *pInitialData,
    ID3D11Texture2D        **ppTexture2D
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        pDesc,
        pInitialData,
        ppTexture2D
    );

    int gw_width  = 0, gw_height  = 0;
    int fb_width  = 0, fb_height  = 0;
    int img_width = 0, img_height = 0;
    M2Fix::GameInstance().GWRenderGeometry(
        gw_width,  gw_height,
        fb_width,  fb_height,
        img_width, img_height
    );

    ID3D11Texture2D *pTexture2D = *ppTexture2D;

    if (pDesc->Height == M2Config::iInternalHeight &&
        pDesc->Format == DXGI_FORMAT_R8G8B8A8_UNORM &&
        pDesc->Usage  == D3D11_USAGE_DEFAULT &&
        M2Config::iInternalHeight < fb_height &&
        !upscalerDisabled)
    {
        texVram.push_back(pTexture2D);
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateTexture2D({}, {}, {}) -> {}",
                pDesc->Width,
                pDesc->Height,
                static_cast<UINT>(pDesc->Format),
                fmt::ptr(pTexture2D)
            );
        }

        pDesc->Width  = fb_width;
        pDesc->Height = fb_height;
        pDesc->BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
            pFunction,
            pDevice,
            pDesc,
            pInitialData,
            &pTexture2D
        );
        texVramRemastered.push_back(pTexture2D);
        descVramRemastered = *pDesc;
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateTexture2D({}, {}, {}) -> {}",
                pDesc->Width,
                pDesc->Height,
                static_cast<UINT>(pDesc->Format),
                fmt::ptr(pTexture2D)
            );
        }
    }
    else if (pDesc->Width  == gw_width &&
             pDesc->Height == gw_height &&
             pDesc->Format == DXGI_FORMAT_R8G8B8A8_UNORM &&
             pDesc->Usage  == D3D11_USAGE_DEFAULT &&
             M2Config::iInternalHeight > fb_height &&
             !upscalerDisabled)
    {
        texVram.push_back(pTexture2D);
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateTexture2D({}, {}, {}) -> {}",
                pDesc->Width,
                pDesc->Height,
                static_cast<UINT>(pDesc->Format),
                fmt::ptr(pTexture2D)
            );
        }

        pDesc->Width  = (M2Config::iInternalHeight * pDesc->Width)  / fb_height;
        pDesc->Height = (M2Config::iInternalHeight * pDesc->Height) / fb_height;
        pDesc->BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
            pFunction,
            pDevice,
            pDesc,
            pInitialData,
            &pTexture2D
        );
        texVramRemastered.push_back(pTexture2D);
        descVramRemastered = *pDesc;
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateTexture2D({}, {}, {}) -> {}",
                pDesc->Width,
                pDesc->Height,
                static_cast<UINT>(pDesc->Format),
                fmt::ptr(pTexture2D)
            );
        }
    }
    else {
        if (M2Config::iRendererLevel >= 3) {
            spdlog::info("[D3D11] ID3D11Device::CreateTexture2D({}, {}, {}) -> {}",
                pDesc->Width,
                pDesc->Height,
                static_cast<UINT>(pDesc->Format),
                fmt::ptr(pTexture2D)
            );
        }
    }

    return res;
}

HRESULT WINAPI D3D11::Device::CreateVertexShader(
    ID3D11Device       *pDevice,
    void               *pShaderBytecode,
    SIZE_T             BytecodeLength,
    ID3D11ClassLinkage *pClassLinkage,
    ID3D11VertexShader **ppVertexShader
) {
    return D3D11::GetInstance().CreateVertexShader(
        D3D11::Device::CreateVertexShader,
        pDevice,
        pShaderBytecode,
        BytecodeLength,
        pClassLinkage,
        ppVertexShader
    );
}

HRESULT WINAPI D3D11::CreateVertexShader(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device       *pDevice,
        void               *pShaderBytecode,
        SIZE_T             BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11VertexShader **ppVertexShader
    ),
    ID3D11Device       *pDevice,
    void               *pShaderBytecode,
    SIZE_T             BytecodeLength,
    ID3D11ClassLinkage *pClassLinkage,
    ID3D11VertexShader **ppVertexShader
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        pShaderBytecode,
        BytecodeLength,
        pClassLinkage,
        ppVertexShader
    );

    ID3D11VertexShader *pVertexShader = *ppVertexShader;
    if (M2Config::iRendererLevel >= 3) {
        spdlog::info("[D3D11] ID3D11Device::CreateVertexShader({}, {}) -> {}",
            fmt::ptr(pShaderBytecode),
            BytecodeLength,
            fmt::ptr(pVertexShader)
        );
    }

    return res;
}

HRESULT WINAPI D3D11::Device::CreatePixelShader(
    ID3D11Device       *pDevice,
    void               *pShaderBytecode,
    SIZE_T             BytecodeLength,
    ID3D11ClassLinkage *pClassLinkage,
    ID3D11PixelShader  **ppPixelShader
) {
    return D3D11::GetInstance().CreatePixelShader(
        D3D11::Device::CreatePixelShader,
        pDevice,
        pShaderBytecode,
        BytecodeLength,
        pClassLinkage,
        ppPixelShader
    );
}

HRESULT WINAPI D3D11::CreatePixelShader(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device       *pDevice,
        void               *pShaderBytecode,
        SIZE_T             BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11PixelShader  **ppPixelShader
    ),
    ID3D11Device       *pDevice,
    void               *pShaderBytecode,
    SIZE_T             BytecodeLength,
    ID3D11ClassLinkage *pClassLinkage,
    ID3D11PixelShader  **ppPixelShader
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        pShaderBytecode,
        BytecodeLength,
        pClassLinkage,
        ppPixelShader
    );

    ID3D11PixelShader *pPixelShader = *ppPixelShader;
    if (M2Config::iRendererLevel >= 3) {
        spdlog::info("[D3D11] ID3D11Device::CreatePixelShader({}, {}) -> {}",
            fmt::ptr(pShaderBytecode),
            BytecodeLength,
            fmt::ptr(pPixelShader)
        );
    }

    return res;
}

void WINAPI D3D11::Immediate::UpdateSubresource(
    ID3D11DeviceContext *pContext,
    ID3D11Resource      *pDstResource,
    UINT                DstSubresource,
    D3D11_BOX           *pDstBox,
    void                *pSrcData,
    UINT                SrcRowPitch,
    UINT                SrcDepthPitch
) {
    return D3D11::GetInstance().UpdateSubresource(
        D3D11::Immediate::UpdateSubresource,
        pContext,
        pDstResource,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch
    );
}

void WINAPI D3D11::Deferred::UpdateSubresource(
    ID3D11DeviceContext *pContext,
    ID3D11Resource      *pDstResource,
    UINT                DstSubresource,
    D3D11_BOX           *pDstBox,
    void                *pSrcData,
    UINT                SrcRowPitch,
    UINT                SrcDepthPitch
) {
    return D3D11::GetInstance().UpdateSubresource(
        D3D11::Deferred::UpdateSubresource,
        pContext,
        pDstResource,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch
    );
}

void WINAPI D3D11::UpdateSubresource(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11Resource      *pDstResource,
        UINT                DstSubresource,
        D3D11_BOX           *pDstBox,
        void                *pSrcData,
        UINT                SrcRowPitch,
        UINT                SrcDepthPitch
    ),
    ID3D11DeviceContext *pContext,
    ID3D11Resource      *pDstResource,
    UINT                DstSubresource,
    D3D11_BOX           *pDstBox,
    void                *pSrcData,
    UINT                SrcRowPitch,
    UINT                SrcDepthPitch
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::UpdateSubresource({}, {})",
            fmt::ptr(pDstResource),
            fmt::ptr(pSrcData)
        );
    }

    M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pDstResource,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch
    );

    D3D11_RESOURCE_DIMENSION Type = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pDstResource->GetType(&Type);

    if (Type == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
        ID3D11Buffer *pDstBuffer = static_cast<ID3D11Buffer *>(pDstResource);
        if (pDstBuffer) {
            D3D11_BUFFER_DESC desc = {};
            pDstBuffer->GetDesc(&desc);
            unsigned char *_pSrcData = static_cast<unsigned char *>(pSrcData);
            Buffers[pDstBuffer] = { _pSrcData, _pSrcData + desc.ByteWidth };
        }
    }
}

void WINAPI D3D11::Immediate::CopySubresourceRegion(
    ID3D11DeviceContext *pContext,
    ID3D11Resource      *pDstResource,
    UINT                DstSubresource,
    UINT                DstX,
    UINT                DstY,
    UINT                DstZ,
    ID3D11Resource      *pSrcResource,
    UINT                SrcSubresource,
    D3D11_BOX           *pSrcBox
) {
    return D3D11::GetInstance().CopySubresourceRegion(
        D3D11::Immediate::CopySubresourceRegion,
        pContext,
        pDstResource,
        DstSubresource,
        DstX,
        DstY,
        DstZ,
        pSrcResource,
        SrcSubresource,
        pSrcBox
    );
}

void WINAPI D3D11::Deferred::CopySubresourceRegion(
    ID3D11DeviceContext *pContext,
    ID3D11Resource      *pDstResource,
    UINT                DstSubresource,
    UINT                DstX,
    UINT                DstY,
    UINT                DstZ,
    ID3D11Resource      *pSrcResource,
    UINT                SrcSubresource,
    D3D11_BOX           *pSrcBox
) {
    return D3D11::GetInstance().CopySubresourceRegion(
        D3D11::Deferred::CopySubresourceRegion,
        pContext,
        pDstResource,
        DstSubresource,
        DstX,
        DstY,
        DstZ,
        pSrcResource,
        SrcSubresource,
        pSrcBox
    );
}

void WINAPI D3D11::CopySubresourceRegion(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11Resource      *pDstResource,
        UINT                DstSubresource,
        UINT                DstX,
        UINT                DstY,
        UINT                DstZ,
        ID3D11Resource      *pSrcResource,
        UINT                SrcSubresource,
        D3D11_BOX           *pSrcBox
    ),
    ID3D11DeviceContext *pContext,
    ID3D11Resource      *pDstResource,
    UINT                DstSubresource,
    UINT                DstX,
    UINT                DstY,
    UINT                DstZ,
    ID3D11Resource      *pSrcResource,
    UINT                SrcSubresource,
    D3D11_BOX           *pSrcBox
) {
    int gw_width  = 0, gw_height  = 0;
    int fb_width  = 0, fb_height  = 0;
    int img_width = 0, img_height = 0;
    M2Fix::GameInstance().GWRenderGeometry(
        gw_width,  gw_height,
        fb_width,  fb_height,
        img_width, img_height
    );

    auto src = std::find(texVram.begin(), texVram.end(), pSrcResource);
    auto dst = std::find(texVram.begin(), texVram.end(), pDstResource);
    if (src != texVram.end() && M2Config::iInternalHeight < fb_height && !upscalerDisabled) {
        pSrcResource = texVramRemastered[src - texVram.begin()];
        if (M2Config::iRendererLevel >= 2) {
            spdlog::info("[D3D11] ID3D11DeviceContext::CopySubresourceRegion({}, {}, {}, {}, {}) # {}",
                fmt::ptr(pDstResource),
                fmt::ptr(pSrcResource),
                DstX,
                pSrcBox->right,
                pSrcBox->bottom,
                src - texVram.begin()
            );
        }
    }
    else if (dst != texVram.end() && M2Config::iInternalHeight > fb_height && !upscalerDisabled) {
        pDstResource = texVramRemastered[dst - texVram.begin()];
        UINT width  = (descVramRemastered.Width  * img_width)  / gw_width;
        UINT height = (descVramRemastered.Height * img_height) / gw_height;
        if (M2Config::iRendererLevel >= 2) {
            spdlog::info("[D3D11] ID3D11DeviceContext::CopySubresourceRegion({}, {}, {}, {}, {}) # {} -> {} {} {}",
                fmt::ptr(pDstResource),
                fmt::ptr(pSrcResource),
                DstX,
                pSrcBox->right,
                pSrcBox->bottom,
                dst - texVram.begin(),
                DstX ? width : 0,
                width,
                height
            );
        }
        DstX = DstX ? width : 0;
        pSrcBox->right  = width;
        pSrcBox->bottom = height;
    } else {
        if (M2Config::iRendererLevel >= 3) {
            spdlog::info("[D3D11] ID3D11DeviceContext::CopySubresourceRegion({}, {}, {}, {}, {})",
                fmt::ptr(pDstResource),
                fmt::ptr(pSrcResource),
                DstX,
                pSrcBox->right,
                pSrcBox->bottom
            );
        }
    }

    if (M2Config::iInternalHeight < fb_height) {
        Upscale(pContext);
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pDstResource,
        DstSubresource,
        DstX,
        DstY,
        DstZ,
        pSrcResource,
        SrcSubresource,
        pSrcBox
    );
}

HRESULT WINAPI D3D11::Device::CreateRenderTargetView(
    ID3D11Device                  *pDevice,
    ID3D11Resource                *pResource,
    D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D11RenderTargetView        **ppRTView
) {
    return D3D11::GetInstance().CreateRenderTargetView(
        D3D11::Device::CreateRenderTargetView,
        pDevice,
        pResource,
        pDesc,
        ppRTView
    );
}

HRESULT WINAPI D3D11::CreateRenderTargetView(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device                  *pDevice,
        ID3D11Resource                *pResource,
        D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
        ID3D11RenderTargetView        **ppRTView
    ),
    ID3D11Device                  *pDevice,
    ID3D11Resource                *pResource,
    D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D11RenderTargetView        **ppRTView
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        pResource,
        pDesc,
        ppRTView
    );

    ID3D11RenderTargetView *pRTView = *ppRTView;
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11Device::CreateRenderTargetView({}, {}) -> {}",
            fmt::ptr(pResource),
            fmt::ptr(pDesc),
            fmt::ptr(pRTView)
        );
    }

    return res;
}

HRESULT WINAPI D3D11::Device::CreateShaderResourceView(
    ID3D11Device                    *pDevice,
    ID3D11Resource                  *pResource,
    D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D11ShaderResourceView        **ppSRView
) {
    return D3D11::GetInstance().CreateShaderResourceView(
        D3D11::Device::CreateShaderResourceView,
        pDevice,
        pResource,
        pDesc,
        ppSRView
    );
}

HRESULT WINAPI D3D11::CreateShaderResourceView(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device                    *pDevice,
        ID3D11Resource                  *pResource,
        D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
        ID3D11ShaderResourceView        **ppSRView
    ),
    ID3D11Device                    *pDevice,
    ID3D11Resource                  *pResource,
    D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D11ShaderResourceView        **ppSRView
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        pResource,
        pDesc,
        ppSRView
    );

    auto it = std::find(texVram.begin(), texVram.end(), pResource);
    if (it != texVram.end() && !upscalerDisabled) {
        ID3D11ShaderResourceView *pSRView = *ppSRView;

        srvVram[texVram[it - texVram.begin()]] = pSRView;
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateShaderResourceView({}, {}) -> {}",
                fmt::ptr(pResource),
                fmt::ptr(pDesc),
                fmt::ptr(pSRView)
            );
        }

        pResource = texVramRemastered[it - texVram.begin()];

        ID3D11RenderTargetView *pRTView = nullptr;
        res = Device->CreateRenderTargetView(pResource, nullptr, &pRTView);

        rtvVramRemastered[pSRView] = pRTView;
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateShaderResourceView({}) -> {}",
                fmt::ptr(pResource),
                fmt::ptr(pRTView)
            );
        }

        ID3D11ShaderResourceView *_pSRView = pSRView;
        res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
            pFunction,
            pDevice,
            pResource,
            nullptr,
            &pSRView
        );

        _pSRView->AddRef();

        srvVramRemastered[_pSRView] = pSRView;
        if (M2Config::iRendererLevel >= 1) {
            spdlog::info("[D3D11] ID3D11Device::CreateShaderResourceView({}, {}) -> {}",
                fmt::ptr(pResource),
                fmt::ptr(pDesc),
                fmt::ptr(pSRView)
            );
        }
    }  else {
        ID3D11ShaderResourceView *pSRView = *ppSRView;
        if (M2Config::iRendererLevel >= 3) {
            spdlog::info("[D3D11] ID3D11Device::CreateShaderResourceView({}, {}) -> {}",
                fmt::ptr(pResource),
                fmt::ptr(pDesc),
                fmt::ptr(pSRView)
            );
        }
    }

    return res;
}

HRESULT WINAPI D3D11::Device::CreateSamplerState(
    ID3D11Device       *pDevice,
    D3D11_SAMPLER_DESC *pSamplerDesc,
    ID3D11SamplerState **ppSamplerState
) {
    return D3D11::GetInstance().CreateSamplerState(
        D3D11::Device::CreateSamplerState,
        pDevice,
        pSamplerDesc,
        ppSamplerState
    );
}

HRESULT WINAPI D3D11::CreateSamplerState(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device       *pDevice,
        D3D11_SAMPLER_DESC *pSamplerDesc,
        ID3D11SamplerState **ppSamplerState
    ),
    ID3D11Device       *pDevice,
    D3D11_SAMPLER_DESC *pSamplerDesc,
    ID3D11SamplerState **ppSamplerState
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        pSamplerDesc,
        ppSamplerState
    );

    ID3D11SamplerState *pSamplerState = *ppSamplerState;
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11Device::CreateSamplerState({}) -> {}",
            fmt::ptr(pSamplerDesc),
            fmt::ptr(pSamplerState)
        );
    }

    return res;
}

void WINAPI D3D11::Immediate::VSSetShaderResources(
    ID3D11DeviceContext      *pContext,
    UINT                     StartSlot,
    UINT                     NumViews,
    ID3D11ShaderResourceView **ppShaderResourceViews
) {
    return D3D11::GetInstance().VSSetShaderResources(
        D3D11::Immediate::VSSetShaderResources,
        pContext,
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
}

void WINAPI D3D11::Deferred::VSSetShaderResources(
    ID3D11DeviceContext      *pContext,
    UINT                     StartSlot,
    UINT                     NumViews,
    ID3D11ShaderResourceView **ppShaderResourceViews
) {
    return D3D11::GetInstance().VSSetShaderResources(
        D3D11::Deferred::VSSetShaderResources,
        pContext,
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
}

void WINAPI D3D11::VSSetShaderResources(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext      *pContext,
        UINT                     StartSlot,
        UINT                     NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews
    ),
    ID3D11DeviceContext      *pContext,
    UINT                     StartSlot,
    UINT                     NumViews,
    ID3D11ShaderResourceView **ppShaderResourceViews
) {
    if (M2Config::iRendererLevel >= 3) {
        if (NumViews > 4) {
            spdlog::info("[D3D11] ID3D11DeviceContext::VSSetShaderResources({}, {})",
                NumViews,
                fmt::ptr(ppShaderResourceViews)
            );
        } else {
            for (UINT View = 0; View < NumViews; ++View) {
                spdlog::info("[D3D11] ID3D11DeviceContext::VSSetShaderResources({}->{}, {})",
                    NumViews,
                    View,
                    fmt::ptr(ppShaderResourceViews[View])
                );
            }
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
}

void WINAPI D3D11::Immediate::PSSetShaderResources(
    ID3D11DeviceContext      *pContext,
    UINT                     StartSlot,
    UINT                     NumViews,
    ID3D11ShaderResourceView **ppShaderResourceViews
) {
    return D3D11::GetInstance().PSSetShaderResources(
        D3D11::Immediate::PSSetShaderResources,
        pContext,
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
}

void WINAPI D3D11::Deferred::PSSetShaderResources(
    ID3D11DeviceContext      *pContext,
    UINT                     StartSlot,
    UINT                     NumViews,
    ID3D11ShaderResourceView **ppShaderResourceViews
) {
    return D3D11::GetInstance().PSSetShaderResources(
        D3D11::Deferred::PSSetShaderResources,
        pContext,
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
}

void WINAPI D3D11::PSSetShaderResources(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext      *pContext,
        UINT                     StartSlot,
        UINT                     NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews
    ),
    ID3D11DeviceContext      *pContext,
    UINT                     StartSlot,
    UINT                     NumViews,
    ID3D11ShaderResourceView **ppShaderResourceViews
) {
    for (UINT View = 0; View < NumViews; ++View) {
        if (srvVramRemastered.count(ppShaderResourceViews[View]) && !upscalerDisabled) {
            ppShaderResourceViews[View] = srvVramRemastered[ppShaderResourceViews[View]];
        }
    }

    if (M2Config::iRendererLevel >= 3) {
        if (NumViews > 4) {
            spdlog::info("[D3D11] ID3D11DeviceContext::PSSetShaderResources({}, {})",
                NumViews,
                fmt::ptr(ppShaderResourceViews)
            );
        } else {
            for (UINT View = 0; View < NumViews; ++View) {
                spdlog::info("[D3D11] ID3D11DeviceContext::PSSetShaderResources({}->{}, {})",
                    NumViews,
                    View,
                    fmt::ptr(ppShaderResourceViews[View])
                );
            }
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
}

void WINAPI D3D11::Immediate::VSSetSamplers(
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumSamplers,
    ID3D11SamplerState  **ppSamplers
) {
    return D3D11::GetInstance().VSSetSamplers(
        D3D11::Immediate::VSSetSamplers,
        pContext,
        StartSlot,
        NumSamplers,
        ppSamplers
    );
}

void WINAPI D3D11::Deferred::VSSetSamplers(
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumSamplers,
    ID3D11SamplerState  **ppSamplers
) {
    return D3D11::GetInstance().VSSetSamplers(
        D3D11::Deferred::VSSetSamplers,
        pContext,
        StartSlot,
        NumSamplers,
        ppSamplers
    );
}

void WINAPI D3D11::VSSetSamplers(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                StartSlot,
        UINT                NumSamplers,
        ID3D11SamplerState  **ppSamplers
    ),
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumSamplers,
    ID3D11SamplerState  **ppSamplers
) {
    if (M2Config::iRendererLevel >= 2) {
        for (UINT Sampler = 0; Sampler < NumSamplers; ++Sampler) {
            spdlog::info("[D3D11] ID3D11DeviceContext::VSSetSamplers({}->{}, {})",
                NumSamplers,
                Sampler,
                fmt::ptr(ppSamplers[Sampler])
            );
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        StartSlot,
        NumSamplers,
        ppSamplers
    );
}

void WINAPI D3D11::Immediate::PSSetSamplers(
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumSamplers,
    ID3D11SamplerState  **ppSamplers
) {
    return D3D11::GetInstance().PSSetSamplers(
        D3D11::Immediate::PSSetSamplers,
        pContext,
        StartSlot,
        NumSamplers,
        ppSamplers
    );
}

void WINAPI D3D11::Deferred::PSSetSamplers(
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumSamplers,
    ID3D11SamplerState  **ppSamplers
) {
    return D3D11::GetInstance().PSSetSamplers(
        D3D11::Deferred::PSSetSamplers,
        pContext,
        StartSlot,
        NumSamplers,
        ppSamplers
    );
}

void WINAPI D3D11::PSSetSamplers(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                StartSlot,
        UINT                NumSamplers,
        ID3D11SamplerState  **ppSamplers
    ),
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumSamplers,
    ID3D11SamplerState  **ppSamplers
) {
    if (M2Config::iRendererLevel >= 2) {
        for (UINT Sampler = 0; Sampler < NumSamplers; ++Sampler) {
            spdlog::info("[D3D11] ID3D11DeviceContext::PSSetSamplers({}->{}, {})",
                NumSamplers,
                Sampler,
                fmt::ptr(ppSamplers[Sampler])
            );
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        StartSlot,
        NumSamplers,
        ppSamplers
    );
}

void WINAPI D3D11::Immediate::RSSetViewports(
    ID3D11DeviceContext *pContext,
    UINT                NumViewports,
    D3D11_VIEWPORT      *pViewports
) {
    return D3D11::GetInstance().RSSetViewports(
        D3D11::Immediate::RSSetViewports,
        pContext,
        NumViewports,
        pViewports
    );
}

void WINAPI D3D11::Deferred::RSSetViewports(
    ID3D11DeviceContext *pContext,
    UINT                NumViewports,
    D3D11_VIEWPORT      *pViewports
) {
    return D3D11::GetInstance().RSSetViewports(
        D3D11::Deferred::RSSetViewports,
        pContext,
        NumViewports,
        pViewports
    );
}

void WINAPI D3D11::RSSetViewports(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                NumViewports,
        D3D11_VIEWPORT      *pViewports
    ),
    ID3D11DeviceContext *pContext,
    UINT                NumViewports,
    D3D11_VIEWPORT      *pViewports
) {
    if (M2Config::iRendererLevel >= 2) {
        for (UINT Viewport = 0; Viewport < NumViewports; ++Viewport) {
            spdlog::info("[D3D11] ID3D11DeviceContext::RSSetViewports({}->{}, {} {} {} {})",
                NumViewports,
                Viewport,
                pViewports[Viewport].TopLeftX,
                pViewports[Viewport].TopLeftY,
                pViewports[Viewport].Width,
                pViewports[Viewport].Height
            );
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        NumViewports,
        pViewports
    );
}

void WINAPI D3D11::Immediate::RSSetScissorRects(
    ID3D11DeviceContext *pContext,
    UINT                NumRects,
    D3D11_RECT          *pRects
) {
    return D3D11::GetInstance().RSSetScissorRects(
        D3D11::Immediate::RSSetScissorRects,
        pContext,
        NumRects,
        pRects
    );
}

void WINAPI D3D11::Deferred::RSSetScissorRects(
    ID3D11DeviceContext *pContext,
    UINT                NumRects,
    D3D11_RECT          *pRects
) {
    return D3D11::GetInstance().RSSetScissorRects(
        D3D11::Deferred::RSSetScissorRects,
        pContext,
        NumRects,
        pRects
    );
}

void WINAPI D3D11::RSSetScissorRects(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                NumRects,
        D3D11_RECT          *pRects
    ),
    ID3D11DeviceContext *pContext,
    UINT                NumRects,
    D3D11_RECT          *pRects
) {
    if (M2Config::iRendererLevel >= 2) {
        for (UINT Rect = 0; Rect < NumRects; ++Rect) {
            spdlog::info("[D3D11] ID3D11DeviceContext::RSSetScissorRects({}->{}, {} {} {} {})",
                NumRects,
                Rect,
                pRects[Rect].left,
                pRects[Rect].top,
                pRects[Rect].right,
                pRects[Rect].bottom
            );
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        NumRects,
        pRects
    );
}

void WINAPI D3D11::Immediate::ClearRenderTargetView(
    ID3D11DeviceContext    *pContext,
    ID3D11RenderTargetView *pRenderTargetView,
    FLOAT                  ColorRGBA[4]
) {
    return D3D11::GetInstance().ClearRenderTargetView(
        D3D11::Immediate::ClearRenderTargetView,
        pContext,
        pRenderTargetView,
        ColorRGBA
    );
}

void WINAPI D3D11::Deferred::ClearRenderTargetView(
    ID3D11DeviceContext    *pContext,
    ID3D11RenderTargetView *pRenderTargetView,
    FLOAT                  ColorRGBA[4]
) {
    return D3D11::GetInstance().ClearRenderTargetView(
        D3D11::Deferred::ClearRenderTargetView,
        pContext,
        pRenderTargetView,
        ColorRGBA
    );
}

void WINAPI D3D11::ClearRenderTargetView(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext    *pContext,
        ID3D11RenderTargetView *pRenderTargetView,
        FLOAT                  ColorRGBA[4]
    ),
    ID3D11DeviceContext    *pContext,
    ID3D11RenderTargetView *pRenderTargetView,
    FLOAT                  ColorRGBA[4]
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::ClearRenderTargetView({}, {}, {}, {}, {})",
            fmt::ptr(pRenderTargetView),
            ColorRGBA[0],
            ColorRGBA[1],
            ColorRGBA[2],
            ColorRGBA[3]
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pRenderTargetView,
        ColorRGBA
    );
}

void WINAPI D3D11::Immediate::Draw(
    ID3D11DeviceContext *pContext,
    UINT                VertexCount,
    UINT                StartVertexLocation
) {
    return D3D11::GetInstance().Draw(
        D3D11::Immediate::Draw,
        pContext,
        VertexCount,
        StartVertexLocation
    );
}

void WINAPI D3D11::Deferred::Draw(
    ID3D11DeviceContext *pContext,
    UINT                VertexCount,
    UINT                StartVertexLocation
) {
    return D3D11::GetInstance().Draw(
        D3D11::Deferred::Draw,
        pContext,
        VertexCount,
        StartVertexLocation
    );
}

void D3D11::Queue(ID3D11DeviceContext *pContext, std::deque<State> & states, UINT VertexCount, UINT StartVertexLocation)
{
    State state = {};
    UINT num = 0;
    pContext->IAGetPrimitiveTopology(&state.PrimitiveTopology);
    pContext->IAGetIndexBuffer(&state.IndexBuffer, &state.IndexBufferFormat, &state.IndexBufferOffset);
    pContext->IAGetInputLayout(&state.InputLayout);

    state.VertexBuffers.resize(16);
    state.VertexStrides.resize(16);
    state.VertexOffsets.resize(16);
    pContext->IAGetVertexBuffers(
        0,
        16,
        state.VertexBuffers.data(),
        state.VertexStrides.data(),
        state.VertexOffsets.data()
    );

    pContext->RSGetViewports(&num, nullptr);
    state.Viewports.resize(num);
    pContext->RSGetViewports(&num, state.Viewports.data());
    pContext->RSGetScissorRects(&num, nullptr);
    state.ScissorRects.resize(num);
    pContext->RSGetScissorRects(&num, state.ScissorRects.data());

    pContext->VSGetShader(&state.VertexShader, nullptr, nullptr);
    pContext->PSGetShader(&state.PixelShader,  nullptr, nullptr);

    state.VertexSamplers.resize(16);
    state.PixelSamplers.resize(16);
    pContext->VSGetSamplers(0, 16, state.VertexSamplers.data());
    pContext->PSGetSamplers(0, 16, state.PixelSamplers.data());

    state.VertexShaderResources.resize(16);
    state.PixelShaderResources.resize(16);
    pContext->VSGetShaderResources(0, 16, state.VertexShaderResources.data());
    pContext->PSGetShaderResources(0, 16, state.PixelShaderResources.data());

    state.VertexConstantBuffers.resize(4);
    state.PixelConstantBuffers.resize(4);
    pContext->VSGetConstantBuffers(0, 4, state.VertexConstantBuffers.data());
    pContext->PSGetConstantBuffers(0, 4, state.PixelConstantBuffers.data());

    for (auto * VertexConstantBuffer : state.VertexConstantBuffers) {
        if (!VertexConstantBuffer) continue;
        if (!Buffers.contains(VertexConstantBuffer)) continue;
        state.VertexConstantArchives[VertexConstantBuffer] = Buffers[VertexConstantBuffer];
    }

    for (auto * PixelConstantBuffer : state.PixelConstantBuffers) {
        if (!PixelConstantBuffer) continue;
        if (!Buffers.contains(PixelConstantBuffer)) continue;
        state.PixelConstantArchives[PixelConstantBuffer] = Buffers[PixelConstantBuffer];
    }

    pContext->RSGetState(&state.RasterizerState);

    pContext->OMGetDepthStencilState(&state.DepthStencilState, &state.DepthStencilRef);
    pContext->OMGetBlendState(&state.BlendState, state.BlendFactor, &state.BlendSampleMask);

    state.RenderTargetViews.resize(4);
    pContext->OMGetRenderTargets(4, state.RenderTargetViews.data(), &state.DepthStencilView);

    state.VertexCount = VertexCount;
    state.StartVertexLocation = StartVertexLocation;

    states.push_back(state);
}

void D3D11::Serve(ID3D11DeviceContext *pContext, std::deque<State> & states)
{
    while (!states.empty())
    {
        auto state = states.front();

        pContext->OMSetDepthStencilState(state.DepthStencilState, state.DepthStencilRef);
        pContext->OMSetBlendState(state.BlendState, state.BlendFactor, state.BlendSampleMask);
        if (!state.RenderTargetViews.empty()) {
            pContext->OMSetRenderTargets(state.RenderTargetViews.size(), state.RenderTargetViews.data(), state.DepthStencilView);
            for (auto * RenderTargetView : state.RenderTargetViews) {
                if (!state.ClearRenderTargetViews[RenderTargetView]) continue;
                pContext->ClearRenderTargetView(RenderTargetView, state.ColorRenderTargetViews[RenderTargetView]);
            }
        }

        pContext->RSSetState(state.RasterizerState);
        pContext->RSSetViewports(state.Viewports.size(), state.Viewports.data());
        pContext->RSSetScissorRects(state.ScissorRects.size(), state.ScissorRects.data());

        pContext->IASetInputLayout(state.InputLayout);
        pContext->IASetPrimitiveTopology(state.PrimitiveTopology);
        pContext->IASetVertexBuffers(0, state.VertexBuffers.size(), state.VertexBuffers.data(), state.VertexStrides.data(), state.VertexOffsets.data());
        pContext->IASetIndexBuffer(state.IndexBuffer, state.IndexBufferFormat, state.IndexBufferOffset);

        for (auto * VertexConstantBuffer : state.VertexConstantBuffers) {
            if (!VertexConstantBuffer) continue;
            auto & Buffer = state.VertexConstantArchives[VertexConstantBuffer];
            if (Buffer.empty()) continue;
            pContext->UpdateSubresource(VertexConstantBuffer, 0, nullptr, Buffer.data(), 0, 0);
        }

        for (auto * PixelConstantBuffer : state.PixelConstantBuffers) {
            if (!PixelConstantBuffer) continue;
            auto & Buffer = state.PixelConstantArchives[PixelConstantBuffer];
            if (Buffer.empty()) continue;
            pContext->UpdateSubresource(PixelConstantBuffer, 0, nullptr, Buffer.data(), 0, 0);
        }

        pContext->VSSetShader(state.VertexShader, nullptr, 0);
        pContext->VSSetConstantBuffers(0, state.VertexConstantBuffers.size(), state.VertexConstantBuffers.data());
        pContext->VSSetSamplers(0, state.VertexSamplers.size(), state.VertexSamplers.data());
        pContext->VSSetShaderResources(0, state.VertexShaderResources.size(), state.VertexShaderResources.data());

        pContext->PSSetShader(state.PixelShader, nullptr, 0);
        pContext->PSSetConstantBuffers(0, state.PixelConstantBuffers.size(), state.PixelConstantBuffers.data());
        pContext->PSSetSamplers(0, state.PixelSamplers.size(), state.PixelSamplers.data());
        pContext->PSSetShaderResources(0, state.PixelShaderResources.size(), state.PixelShaderResources.data());

        if (state.VertexCount != 0) {
            pContext->Draw(state.VertexCount, state.StartVertexLocation);
        }

        if (state.IndexBuffer) state.IndexBuffer->Release();
        for (auto * VertexBuffer : state.VertexBuffers) {
            if (VertexBuffer) VertexBuffer->Release();
        }
        if (state.InputLayout) state.InputLayout->Release();
        if (state.VertexShader) state.VertexShader->Release();
        if (state.PixelShader) state.PixelShader->Release();
        for (auto * VertexSampler : state.VertexSamplers) {
            if (VertexSampler) VertexSampler->Release();
        }
        for (auto * PixelSampler : state.PixelSamplers) {
            if (PixelSampler) PixelSampler->Release();
        }
        for (auto * VertexShaderResource : state.VertexShaderResources) {
            if (VertexShaderResource) VertexShaderResource->Release();
        }
        for (auto * PixelShaderResource : state.PixelShaderResources) {
            if (PixelShaderResource) PixelShaderResource->Release();
        }
        for (auto * VertexConstantBuffer : state.VertexConstantBuffers) {
            if (VertexConstantBuffer) VertexConstantBuffer->Release();
        }
        for (auto * PixelConstantBuffer : state.PixelConstantBuffers) {
            if (PixelConstantBuffer) PixelConstantBuffer->Release();
        }
        if (state.RasterizerState) state.RasterizerState->Release();
        if (state.DepthStencilState) state.DepthStencilState->Release();
        if (state.BlendState) state.BlendState->Release();
        for (auto * RenderTargetView : state.RenderTargetViews) {
            if (RenderTargetView) RenderTargetView->Release();
        }
        if (state.DepthStencilView) state.DepthStencilView->Release();

        states.pop_front();
    }
}

void WINAPI D3D11::Draw(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                VertexCount,
        UINT                StartVertexLocation
    ),
    ID3D11DeviceContext *pContext,
    UINT                VertexCount,
    UINT                StartVertexLocation
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::Draw({}, {})",
            VertexCount,
            StartVertexLocation
        );
    }

    M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        VertexCount,
        StartVertexLocation
    );
}

void WINAPI D3D11::Immediate::VSSetShader(
    ID3D11DeviceContext *pContext,
    ID3D11VertexShader  *pVertexShader,
    ID3D11ClassInstance **ppClassInstances,
    UINT                NumClassInstances
) {
    return D3D11::GetInstance().VSSetShader(
        D3D11::Immediate::VSSetShader,
        pContext,
        pVertexShader,
        ppClassInstances,
        NumClassInstances
    );
}

void WINAPI D3D11::Deferred::VSSetShader(
    ID3D11DeviceContext *pContext,
    ID3D11VertexShader  *pVertexShader,
    ID3D11ClassInstance **ppClassInstances,
    UINT                NumClassInstances
) {
    return D3D11::GetInstance().VSSetShader(
        D3D11::Deferred::VSSetShader,
        pContext,
        pVertexShader,
        ppClassInstances,
        NumClassInstances
    );
}

void WINAPI D3D11::VSSetShader(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11VertexShader  *pVertexShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT                NumClassInstances
    ),
    ID3D11DeviceContext *pContext,
    ID3D11VertexShader  *pVertexShader,
    ID3D11ClassInstance **ppClassInstances,
    UINT                NumClassInstances
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::VSSetShader({})",
            fmt::ptr(pVertexShader)
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pVertexShader,
        ppClassInstances,
        NumClassInstances
    );
}

void WINAPI D3D11::Immediate::PSSetShader(
    ID3D11DeviceContext *pContext,
    ID3D11PixelShader   *pPixelShader,
    ID3D11ClassInstance **ppClassInstances,
    UINT                NumClassInstances
) {
    return D3D11::GetInstance().PSSetShader(
        D3D11::Immediate::PSSetShader,
        pContext,
        pPixelShader,
        ppClassInstances,
        NumClassInstances
    );
}

void WINAPI D3D11::Deferred::PSSetShader(
    ID3D11DeviceContext *pContext,
    ID3D11PixelShader   *pPixelShader,
    ID3D11ClassInstance **ppClassInstances,
    UINT                NumClassInstances
) {
    return D3D11::GetInstance().PSSetShader(
        D3D11::Deferred::PSSetShader,
        pContext,
        pPixelShader,
        ppClassInstances,
        NumClassInstances
    );
}

void WINAPI D3D11::PSSetShader(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11PixelShader   *pPixelShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT                NumClassInstances
    ),
    ID3D11DeviceContext *pContext,
    ID3D11PixelShader   *pPixelShader,
    ID3D11ClassInstance **ppClassInstances,
    UINT                NumClassInstances
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::PSSetShader({})",
            fmt::ptr(pPixelShader)
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pPixelShader,
        ppClassInstances,
        NumClassInstances
    );
}

void WINAPI D3D11::Immediate::IASetPrimitiveTopology(
    ID3D11DeviceContext      *pContext,
    D3D11_PRIMITIVE_TOPOLOGY Topology
) {
    return D3D11::GetInstance().IASetPrimitiveTopology(
        D3D11::Immediate::IASetPrimitiveTopology,
        pContext,
        Topology
    );
}

void WINAPI D3D11::Deferred::IASetPrimitiveTopology(
    ID3D11DeviceContext      *pContext,
    D3D11_PRIMITIVE_TOPOLOGY Topology
) {
    return D3D11::GetInstance().IASetPrimitiveTopology(
        D3D11::Deferred::IASetPrimitiveTopology,
        pContext,
        Topology
    );
}

void WINAPI D3D11::IASetPrimitiveTopology(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext      *pContext,
        D3D11_PRIMITIVE_TOPOLOGY Topology
    ),
    ID3D11DeviceContext      *pContext,
    D3D11_PRIMITIVE_TOPOLOGY Topology
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::IASetPrimitiveTopology({})",
            static_cast<int>(Topology)
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        Topology
    );
}

void WINAPI D3D11::Immediate::IASetInputLayout(
    ID3D11DeviceContext *pContext,
    ID3D11InputLayout   *pInputLayout
) {
    return D3D11::GetInstance().IASetInputLayout(
        D3D11::Immediate::IASetInputLayout,
        pContext,
        pInputLayout
    );
}

void WINAPI D3D11::Deferred::IASetInputLayout(
    ID3D11DeviceContext *pContext,
    ID3D11InputLayout   *pInputLayout
) {
    return D3D11::GetInstance().IASetInputLayout(
        D3D11::Deferred::IASetInputLayout,
        pContext,
        pInputLayout
    );
}

void WINAPI D3D11::IASetInputLayout(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11InputLayout   *pInputLayout
    ),
    ID3D11DeviceContext *pContext,
    ID3D11InputLayout   *pInputLayout
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::IASetInputLayout({})",
            fmt::ptr(pInputLayout)
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pInputLayout
    );
}

void WINAPI D3D11::Immediate::IASetIndexBuffer(
    ID3D11DeviceContext *pContext,
    ID3D11Buffer        *pIndexBuffer,
    DXGI_FORMAT         Format,
    UINT                Offset
) {
    return D3D11::GetInstance().IASetIndexBuffer(
        D3D11::Immediate::IASetIndexBuffer,
        pContext,
        pIndexBuffer,
        Format,
        Offset
    );
}

void WINAPI D3D11::Deferred::IASetIndexBuffer(
    ID3D11DeviceContext *pContext,
    ID3D11Buffer        *pIndexBuffer,
    DXGI_FORMAT         Format,
    UINT                Offset
) {
    return D3D11::GetInstance().IASetIndexBuffer(
        D3D11::Deferred::IASetIndexBuffer,
        pContext,
        pIndexBuffer,
        Format,
        Offset
    );
}

void WINAPI D3D11::IASetIndexBuffer(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11Buffer        *pIndexBuffer,
        DXGI_FORMAT         Format,
        UINT                Offset
    ),
    ID3D11DeviceContext *pContext,
    ID3D11Buffer        *pIndexBuffer,
    DXGI_FORMAT         Format,
    UINT                Offset
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::IASetIndexBuffer({})",
            fmt::ptr(pIndexBuffer),
            static_cast<int>(Format),
            Offset
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pIndexBuffer,
        Format,
        Offset
    );
}

void WINAPI D3D11::Immediate::IASetVertexBuffers(
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumBuffers,
    ID3D11Buffer        **ppVertexBuffers,
    UINT                *pStrides,
    UINT                *pOffsets
) {
    return D3D11::GetInstance().IASetVertexBuffers(
        D3D11::Immediate::IASetVertexBuffers,
        pContext,
        StartSlot,
        NumBuffers,
        ppVertexBuffers,
        pStrides,
        pOffsets
    );
}

void WINAPI D3D11::Deferred::IASetVertexBuffers(
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumBuffers,
    ID3D11Buffer        **ppVertexBuffers,
    UINT                *pStrides,
    UINT                *pOffsets
) {
    return D3D11::GetInstance().IASetVertexBuffers(
        D3D11::Deferred::IASetVertexBuffers,
        pContext,
        StartSlot,
        NumBuffers,
        ppVertexBuffers,
        pStrides,
        pOffsets
    );
}

void WINAPI D3D11::IASetVertexBuffers(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                StartSlot,
        UINT                NumBuffers,
        ID3D11Buffer        **ppVertexBuffers,
        UINT                *pStrides,
        UINT                *pOffsets
    ),
    ID3D11DeviceContext *pContext,
    UINT                StartSlot,
    UINT                NumBuffers,
    ID3D11Buffer        **ppVertexBuffers,
    UINT                *pStrides,
    UINT                *pOffsets
) {
    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::IASetVertexBuffers({}, {}, {}, {}, {})",
            StartSlot,
            NumBuffers,
            fmt::ptr(ppVertexBuffers),
            fmt::ptr(pStrides),
            fmt::ptr(pOffsets)
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        StartSlot,
        NumBuffers,
        ppVertexBuffers,
        pStrides,
        pOffsets
    );
}

void WINAPI D3D11::Immediate::OMSetRenderTargets(
    ID3D11DeviceContext    *pContext,
    UINT                   NumViews,
    ID3D11RenderTargetView **ppRenderTargetViews,
    ID3D11DepthStencilView *pDepthStencilView
) {
    return D3D11::GetInstance().OMSetRenderTargets(
        D3D11::Immediate::OMSetRenderTargets,
        pContext,
        NumViews,
        ppRenderTargetViews,
        pDepthStencilView
    );
}

void WINAPI D3D11::Deferred::OMSetRenderTargets(
    ID3D11DeviceContext    *pContext,
    UINT                   NumViews,
    ID3D11RenderTargetView **ppRenderTargetViews,
    ID3D11DepthStencilView *pDepthStencilView
) {
    return D3D11::GetInstance().OMSetRenderTargets(
        D3D11::Deferred::OMSetRenderTargets,
        pContext,
        NumViews,
        ppRenderTargetViews,
        pDepthStencilView
    );
}

void WINAPI D3D11::OMSetRenderTargets(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext    *pContext,
        UINT                   NumViews,
        ID3D11RenderTargetView **ppRenderTargetViews,
        ID3D11DepthStencilView *pDepthStencilView
    ),
    ID3D11DeviceContext    *pContext,
    UINT                   NumViews,
    ID3D11RenderTargetView **ppRenderTargetViews,
    ID3D11DepthStencilView *pDepthStencilView
) {
    if (M2Config::iRendererLevel >= 2) {
        for (UINT View = 0; View < NumViews; ++View) {
            spdlog::info("[D3D11] ID3D11DeviceContext::OMSetRenderTargets({}->{}, {}, {})",
                NumViews,
                View,
                fmt::ptr(ppRenderTargetViews[View]),
                fmt::ptr(pDepthStencilView)
            );
        }
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        NumViews,
        ppRenderTargetViews,
        pDepthStencilView
    );
}

void D3D11::Upscale(ID3D11DeviceContext *pContext)
{
    if (!pContext && M2Config::bInternalEnabled) {
        upscalerDisabled = false;

        HRESULT res = S_OK;

        auto upscalerVertexSource = reinterpret_cast<const char *>(
                M2Hook::GetInstance(".").ModuleResource(IDR_HLSL1, "HLSL")
        );
        ID3DBlob *upscalerVertexError = nullptr;
        res = D3D11::D3DCompile(
            upscalerVertexSource,
            strlen(upscalerVertexSource),
            std::format("{}::VertexUpscaler", M2Fix::FixName()).c_str(),
            nullptr,
            nullptr,
            "main",
            "vs_4_0",
            M2Config::bDebuggerEnabled ? (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION) : 0,
            0,
            &upscalerVertexBlob,
            &upscalerVertexError
        );
        if (res != S_OK) {
            char *upscalerVertexString =
                reinterpret_cast<char *>(upscalerVertexError->GetBufferPointer());
            upscalerVertexString[strcspn(upscalerVertexString, "\r\n")] = 0;
            spdlog::warn("[D3D11] {}::VertexUpscaler shader compilation failed: {} {}.",
                M2Fix::FixName(),
                res,
                upscalerVertexString
            );
            return;
        } else {
            spdlog::info("[D3D11] {}::VertexUpscaler shader compilation succeeded.", M2Fix::FixName());
        }

        auto upscalerPixelSource = reinterpret_cast<const char *>(
            M2Hook::GetInstance(".").ModuleResource(IDR_HLSL2, "HLSL")
        );
        ID3DBlob *upscalerPixelError = nullptr;
        res = D3D11::D3DCompile(
            upscalerPixelSource,
            strlen(upscalerPixelSource),
            std::format("{}::PixelUpscaler", M2Fix::FixName()).c_str(),
            nullptr,
            nullptr,
            "main",
            "ps_4_0",
            M2Config::bDebuggerEnabled ? (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION) : 0,
            0,
            &upscalerPixelBlob,
            &upscalerPixelError
        );
        if (res != S_OK) {
            char *upscalerPixelString =
                reinterpret_cast<char *>(upscalerPixelError->GetBufferPointer());
            upscalerPixelString[strcspn(upscalerPixelString, "\r\n")] = 0;
            spdlog::warn("[D3D11] {}::PixelUpscaler shader compilation failed: {} {}.",
                M2Fix::FixName(),
                res,
                upscalerPixelString
            );
            return;
        } else {
            spdlog::info("[D3D11] {}::PixelUpscaler shader compilation succeeded.", M2Fix::FixName());
        }

        return;
    }

    if (upscalerDisabled) return;

    for (auto [texVram, srcSRV] : srvVram) {
        ID3D11RenderTargetView *dstRTV = rtvVramRemastered[srcSRV];

        State state = {};
        Queue(pContext, upscalerDraws);

        if (!M2Fix::GameInstance().GWBlank()) {
            D3D11_VIEWPORT viewport = {};
            viewport.Width    = static_cast<FLOAT>(descVramRemastered.Width);
            viewport.Height   = static_cast<FLOAT>(descVramRemastered.Height);
            viewport.MaxDepth = 1.0f;
            state.Viewports.push_back(viewport);

            D3D11_RECT scissor = {};
            scissor.right  = static_cast<LONG>(descVramRemastered.Width);
            scissor.bottom = static_cast<LONG>(descVramRemastered.Height);
            state.ScissorRects.push_back(scissor);

            upscalerVertexShader->AddRef();
            state.VertexShader = upscalerVertexShader;

            upscalerPixelShader->AddRef();
            state.PixelShader = upscalerPixelShader;

            upscalerSampler->AddRef();
            state.PixelSamplers.push_back(upscalerSampler);

            srcSRV->AddRef();
            state.PixelShaderResources.push_back(srcSRV);

            state.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            state.BlendSampleMask = UINT32_MAX;
            state.VertexCount = 3;
        }

        dstRTV->AddRef();
        state.RenderTargetViews.push_back(dstRTV);
        state.ClearRenderTargetViews[dstRTV] = true;

        upscalerDraws.push_front(state);

        upscalerDisabled = true; {
            Serve(pContext, upscalerDraws);
        } upscalerDisabled = false;
    }
}

void WINAPI D3D11::Immediate::ClearDepthStencilView(
    ID3D11DeviceContext    *pContext,
    ID3D11DepthStencilView *pDepthStencilView,
    UINT                   ClearFlags,
    FLOAT                  Depth,
    UINT8                  Stencil
) {
    return D3D11::GetInstance().ClearDepthStencilView(
        D3D11::Immediate::ClearDepthStencilView,
        pContext,
        pDepthStencilView,
        ClearFlags,
        Depth,
        Stencil
    );
}

void WINAPI D3D11::Deferred::ClearDepthStencilView(
    ID3D11DeviceContext    *pContext,
    ID3D11DepthStencilView *pDepthStencilView,
    UINT                   ClearFlags,
    FLOAT                  Depth,
    UINT8                  Stencil
) {
    return D3D11::GetInstance().ClearDepthStencilView(
        D3D11::Deferred::ClearDepthStencilView,
        pContext,
        pDepthStencilView,
        ClearFlags,
        Depth,
        Stencil
    );
}

void WINAPI D3D11::ClearDepthStencilView(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext    *pContext,
        ID3D11DepthStencilView *pDepthStencilView,
        UINT                   ClearFlags,
        FLOAT                  Depth,
        UINT8                  Stencil
    ),
    ID3D11DeviceContext    *pContext,
    ID3D11DepthStencilView *pDepthStencilView,
    UINT                   ClearFlags,
    FLOAT                  Depth,
    UINT8                  Stencil
) {
    int gw_width  = 0, gw_height  = 0;
    int fb_width  = 0, fb_height  = 0;
    int img_width = 0, img_height = 0;
    M2Fix::GameInstance().GWRenderGeometry(
        gw_width,  gw_height,
        fb_width,  fb_height,
        img_width, img_height
    );

    if (M2Config::iInternalHeight > fb_height) {
        Upscale(pContext);
    }

    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::ClearDepthStencilView({}, {}, {}, {})",
            fmt::ptr(pDepthStencilView),
            ClearFlags,
            Depth,
            Stencil
        );
    }

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pDepthStencilView,
        ClearFlags,
        Depth,
        Stencil
    );
}

void WINAPI D3D11::Immediate::ExecuteCommandList(
    ID3D11DeviceContext *pContext,
    ID3D11CommandList   *pCommandList,
    BOOL                RestoreContextState
) {
    return D3D11::GetInstance().ExecuteCommandList(
        D3D11::Immediate::ExecuteCommandList,
        pContext,
        pCommandList,
        RestoreContextState
    );
}

void WINAPI D3D11::Deferred::ExecuteCommandList(
    ID3D11DeviceContext *pContext,
    ID3D11CommandList   *pCommandList,
    BOOL                RestoreContextState
) {
    return D3D11::GetInstance().ExecuteCommandList(
        D3D11::Deferred::ExecuteCommandList,
        pContext,
        pCommandList,
        RestoreContextState
    );
}

void WINAPI D3D11::ExecuteCommandList(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        ID3D11CommandList   *pCommandList,
        BOOL                RestoreContextState
    ),
    ID3D11DeviceContext *pContext,
    ID3D11CommandList   *pCommandList,
    BOOL                RestoreContextState
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pContext,
        pCommandList,
        RestoreContextState
    );

    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::ExecuteCommandList({}, {})",
            fmt::ptr(pCommandList),
            RestoreContextState
        );
    }
}

void WINAPI D3D11::Immediate::DrawIndexed(
    ID3D11DeviceContext *pContext,
    UINT                IndexCount,
    UINT                StartIndexLocation,
    INT                 BaseVertexLocation
) {
    return D3D11::GetInstance().DrawIndexed(
        D3D11::Immediate::DrawIndexed,
        pContext,
        IndexCount,
        StartIndexLocation,
        BaseVertexLocation
    );
}

void WINAPI D3D11::Deferred::DrawIndexed(
    ID3D11DeviceContext *pContext,
    UINT                IndexCount,
    UINT                StartIndexLocation,
    INT                 BaseVertexLocation
) {
    return D3D11::GetInstance().DrawIndexed(
        D3D11::Deferred::DrawIndexed,
        pContext,
        IndexCount,
        StartIndexLocation,
        BaseVertexLocation
    );
}

#if defined(M2FIX_USE_IMGUI)
BOOL WINAPI D3D11::ShowWindow(
    HWND hWnd,
    int  nCmdShow
) {
    ImGui_ImplWin32_Init(hWnd);

    return M2Hook::GetInstance().Invoke<BOOL>(
        ShowWindow,
        hWnd,
        nCmdShow
    );
}
#endif

#if defined(M2FIX_USE_IMGUI)
void D3D11::Overlay(ID3D11DeviceContext *pContext)
{
    if (!pContext && M2Config::bConsole) {
        overlayDisabled = false;

        ImGuiIO & io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;

        ImGui::CreateContext();
        ImGui::StyleColorsLight();

        HMODULE USER32 = LoadLibraryA("USER32.dll");
        if (!USER32) {
            spdlog::warn("[D3D11] Failed to load USER32.dll.");
            overlayDisabled = true;
            return;
        }

        auto USER32_ShowWindow = GetProcAddress(USER32, "ShowWindow");
        if (!USER32_ShowWindow) {
            spdlog::warn("[D3D11] ShowWindow lookup failed.");
            overlayDisabled = true;
            return;
        }
        bool ret = M2Hook::GetInstance().Hook(
            USER32_ShowWindow,
            ShowWindow,
            "[D3D11] ShowWindow"
        );

        if (!ret) {
            overlayDisabled = true;
        }

        return;
    }

    if (overlayDisabled) return;
     overlayDisabled = true;

    bool upscalerEnabled = !upscalerDisabled;
    upscalerDisabled = true;

    struct ImGui_ImplDX11_Data
    {
        ID3D11Device             *pd3dDevice;
        ID3D11DeviceContext      *pd3dDeviceContext;
        IDXGIFactory             *pFactory;
        ID3D11Buffer             *pVB;
        ID3D11Buffer             *pIB;
        ID3D11VertexShader       *pVertexShader;
        ID3D11InputLayout        *pInputLayout;
        ID3D11Buffer             *pVertexConstantBuffer;
        ID3D11PixelShader        *pPixelShader;
        ID3D11SamplerState       *pFontSampler;
        ID3D11ShaderResourceView *pFontTextureView;
        ID3D11RasterizerState    *pRasterizerState;
        ID3D11BlendState         *pBlendState;
        ID3D11DepthStencilState  *pDepthStencilState;
        int                      VertexBufferSize;
        int                      IndexBufferSize;
    };

    ImGuiIO & io = ImGui::GetIO();
    ImGui_ImplDX11_Data *bd = reinterpret_cast<ImGui_ImplDX11_Data *>(
        io.BackendRendererUserData
    );
    if (!bd || bd->pd3dDeviceContext != pContext) {
        if (bd) ImGui_ImplDX11_Shutdown();
        ImGui_ImplDX11_Init(Device, pContext);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin(
        M2Fix::GetInstance().FixName().c_str(),
        nullptr,
        ImGuiWindowFlags_NoTitleBar      |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoScrollbar     |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs
    );

    ImGui::SetWindowPos(ImVec2(8, 8));
    ImGui::SetWindowSize(ImVec2(105, 30));
    auto wm = fmt::format("{} v{}",
        M2Fix::GetInstance().FixName(),
        M2Fix::GetInstance().FixVersion()
    );

    ImGui::Text(wm.c_str());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    upscalerDisabled = !upscalerEnabled;
    overlayDisabled = false;
}
#endif

void WINAPI D3D11::DrawIndexed(
    void (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        UINT                IndexCount,
        UINT                StartIndexLocation,
        INT                 BaseVertexLocation
    ),
    ID3D11DeviceContext *pContext,
    UINT                IndexCount,
    UINT                StartIndexLocation,
    INT                 BaseVertexLocation
) {
    M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        IndexCount,
        StartIndexLocation,
        BaseVertexLocation
    );

    if (M2Config::iRendererLevel >= 3) {
        spdlog::info("[D3D11] ID3D11DeviceContext::DrawIndexed({}, {}, {})",
            IndexCount,
            StartIndexLocation,
            BaseVertexLocation
        );
    }

#if defined(M2FIX_USE_IMGUI)
    Overlay(pContext);
#endif
}

HRESULT WINAPI D3D11::Immediate::FinishCommandList(
    ID3D11DeviceContext *pContext,
    BOOL                RestoreDeferredContextState,
    ID3D11CommandList   **ppCommandList
) {
    return D3D11::GetInstance().FinishCommandList(
        D3D11::Immediate::FinishCommandList,
        pContext,
        RestoreDeferredContextState,
        ppCommandList
    );
}

HRESULT WINAPI D3D11::Deferred::FinishCommandList(
    ID3D11DeviceContext *pContext,
    BOOL                RestoreDeferredContextState,
    ID3D11CommandList   **ppCommandList
) {
    return D3D11::GetInstance().FinishCommandList(
        D3D11::Deferred::FinishCommandList,
        pContext,
        RestoreDeferredContextState,
        ppCommandList
    );
}

HRESULT WINAPI D3D11::FinishCommandList(
    HRESULT (WINAPI *pFunction)(
        ID3D11DeviceContext *pContext,
        BOOL                RestoreDeferredContextState,
        ID3D11CommandList   **ppCommandList
    ),
    ID3D11DeviceContext *pContext,
    BOOL                RestoreDeferredContextState,
    ID3D11CommandList   **ppCommandList
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pContext,
        RestoreDeferredContextState,
        ppCommandList
    );

    if (M2Config::iRendererLevel >= 2) {
        spdlog::info("[D3D11] ID3D11DeviceContext::FinishCommandList({}, {})",
            RestoreDeferredContextState,
            fmt::ptr(ppCommandList)
        );
    }

    return res;
}

HRESULT WINAPI D3D11::Device::CreateDeferredContext(
    ID3D11Device        *pDevice,
    UINT                ContextFlags,
    ID3D11DeviceContext **ppDeferredContext
) {
    return D3D11::GetInstance().CreateDeferredContext(
        D3D11::Device::CreateDeferredContext,
        pDevice,
        ContextFlags,
        ppDeferredContext
    );
}

HRESULT WINAPI D3D11::CreateDeferredContext(
    HRESULT (WINAPI *pFunction)(
        ID3D11Device        *pDevice,
        UINT                ContextFlags,
        ID3D11DeviceContext **ppDeferredContext
    ),
    ID3D11Device        *pDevice,
    UINT                ContextFlags,
    ID3D11DeviceContext **ppDeferredContext
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        pFunction,
        pDevice,
        ContextFlags,
        ppDeferredContext
    );

    ID3D11DeviceContext *pDeferredContext = *ppDeferredContext;
    if (DeferredContext) return res;
    DeferredContext = pDeferredContext;

    if (M2Config::iRendererLevel >= 1) {
        spdlog::info("[D3D11] ID3D11Device::CreateDeferredContext() -> {}",
            fmt::ptr(pDeferredContext)
        );
    }

    M2Hook::GetInstance().VirtualTableHook(
        pDeferredContext, "[D3D11] [Deferred] ID3D11DeviceContext"
    );

    #define VIRTUAL_HOOK(name) { \
        M2Hook::GetInstance().VirtualHook( \
            pDeferredContext, &ID3D11DeviceContext::name, \
            D3D11::Deferred::##name, "[D3D11] [Deferred] ID3D11DeviceContext::" #name \
        ); \
    }

    VIRTUAL_HOOK(ClearRenderTargetView);
    VIRTUAL_HOOK(ClearDepthStencilView);
    VIRTUAL_HOOK(CopySubresourceRegion);
    VIRTUAL_HOOK(PSSetShaderResources);
    VIRTUAL_HOOK(PSSetSamplers);
    VIRTUAL_HOOK(IASetPrimitiveTopology);
    VIRTUAL_HOOK(IASetInputLayout);
    VIRTUAL_HOOK(IASetVertexBuffers);
    VIRTUAL_HOOK(Draw);
    VIRTUAL_HOOK(DrawIndexed);
    VIRTUAL_HOOK(UpdateSubresource);
    VIRTUAL_HOOK(IASetIndexBuffer);
    VIRTUAL_HOOK(VSSetShader);
    VIRTUAL_HOOK(VSSetSamplers);
    VIRTUAL_HOOK(VSSetShaderResources);
    VIRTUAL_HOOK(PSSetShader);
    VIRTUAL_HOOK(RSSetViewports);
    VIRTUAL_HOOK(RSSetScissorRects);
    VIRTUAL_HOOK(OMSetRenderTargets);

    if (M2Config::iRendererLevel >= 2) {
        VIRTUAL_HOOK(ExecuteCommandList);
        VIRTUAL_HOOK(FinishCommandList);
    }
    #undef VIRTUAL_HOOK

    return res;
}

HRESULT WINAPI D3D11::Device::CreateDevice(
    IDXGIAdapter        *pAdapter,
    D3D_DRIVER_TYPE     DriverType,
    HMODULE             Software,
    UINT                Flags,
    D3D_FEATURE_LEVEL   *pFeatureLevels,
    UINT                FeatureLevels,
    UINT                SDKVersion,
    ID3D11Device        **ppDevice,
    D3D_FEATURE_LEVEL   *pFeatureLevel,
    ID3D11DeviceContext **ppImmediateContext
) {
    return D3D11::GetInstance().CreateDevice(
        D3D11::Device::CreateDevice,
        pAdapter,
        DriverType,
        Software,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        SDKVersion,
        ppDevice,
        pFeatureLevel,
        ppImmediateContext
    );
}

HRESULT WINAPI D3D11::CreateDevice(
    HRESULT (WINAPI *pFunction)(
        IDXGIAdapter        *pAdapter,
        D3D_DRIVER_TYPE     DriverType,
        HMODULE             Software,
        UINT                Flags,
        D3D_FEATURE_LEVEL   *pFeatureLevels,
        UINT                FeatureLevels,
        UINT                SDKVersion,
        ID3D11Device        **ppDevice,
        D3D_FEATURE_LEVEL   *pFeatureLevel,
        ID3D11DeviceContext **ppImmediateContext
    ),
    IDXGIAdapter        *pAdapter,
    D3D_DRIVER_TYPE     DriverType,
    HMODULE             Software,
    UINT                Flags,
    D3D_FEATURE_LEVEL   *pFeatureLevels,
    UINT                FeatureLevels,
    UINT                SDKVersion,
    ID3D11Device        **ppDevice,
    D3D_FEATURE_LEVEL   *pFeatureLevel,
    ID3D11DeviceContext **ppImmediateContext
) {
    //Flags |= D3D11_CREATE_DEVICE_DEBUG;

    HRESULT res = M2Hook::GetInstance().Invoke<HRESULT>(
        pFunction,
        pAdapter,
        DriverType,
        Software,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        SDKVersion,
        ppDevice,
        pFeatureLevel,
        ppImmediateContext
    );

    ID3D11Device *pDevice = *ppDevice;
    if (Device) return res;
    Device = pDevice;

    if (M2Config::iRendererLevel >= 1) {
        spdlog::info("[D3D11] D3D11CreateDevice({}, {}, {}) -> {}",
            static_cast<int>(DriverType),
            static_cast<int>(Flags),
            static_cast<int>(SDKVersion),
            fmt::ptr(pDevice)
        );
    }

    M2Hook::GetInstance().VirtualTableHook(
        pDevice, "[D3D11] ID3D11Device"
    );

    #define VIRTUAL_HOOK(name) { \
        M2Hook::GetInstance().VirtualHook( \
            pDevice, &ID3D11Device::name, \
            D3D11::Device::##name, "[D3D11] ID3D11Device::" #name \
        ); \
    }

    VIRTUAL_HOOK(CreateDeferredContext);
    VIRTUAL_HOOK(CreateTexture2D);
    VIRTUAL_HOOK(CreateShaderResourceView);

    if (M2Config::iRendererLevel >= 2) {
        VIRTUAL_HOOK(CreateRenderTargetView);
        VIRTUAL_HOOK(CreateSamplerState);
    }

    if (M2Config::iRendererLevel >= 3) {
        VIRTUAL_HOOK(CreateVertexShader);
        VIRTUAL_HOOK(CreatePixelShader);
    }
    #undef VIRTUAL_HOOK

    ID3D11DeviceContext *pImmediateContext = *ppImmediateContext;
    ImmediateContext = pImmediateContext;

    M2Hook::GetInstance().VirtualTableHook(
        pImmediateContext, "[D3D11] [Immediate] ID3D11DeviceContext"
    );

	#define VIRTUAL_HOOK(name) { \
        M2Hook::GetInstance().VirtualHook( \
            pImmediateContext, &ID3D11DeviceContext::name, \
            D3D11::Immediate::##name, "[D3D11] [Immediate] ID3D11DeviceContext::" #name \
        ); \
    }

    VIRTUAL_HOOK(ClearRenderTargetView);
    VIRTUAL_HOOK(ClearDepthStencilView);
    VIRTUAL_HOOK(CopySubresourceRegion);
    VIRTUAL_HOOK(PSSetShaderResources);
    VIRTUAL_HOOK(PSSetSamplers);
    VIRTUAL_HOOK(IASetPrimitiveTopology);
    VIRTUAL_HOOK(IASetInputLayout);
    VIRTUAL_HOOK(IASetVertexBuffers);
    VIRTUAL_HOOK(Draw);
    VIRTUAL_HOOK(DrawIndexed);
    VIRTUAL_HOOK(UpdateSubresource);
    VIRTUAL_HOOK(IASetIndexBuffer);
    VIRTUAL_HOOK(VSSetShader);
    VIRTUAL_HOOK(VSSetSamplers);
    VIRTUAL_HOOK(VSSetShaderResources);
    VIRTUAL_HOOK(PSSetShader);
    VIRTUAL_HOOK(RSSetViewports);
    VIRTUAL_HOOK(RSSetScissorRects);
    VIRTUAL_HOOK(OMSetRenderTargets);

    if (M2Config::iRendererLevel >= 2) {
        VIRTUAL_HOOK(ExecuteCommandList);
        VIRTUAL_HOOK(FinishCommandList);
    }
	#undef VIRTUAL_HOOK

    if (!upscalerDisabled) {
        pDevice->CreateVertexShader(
            upscalerVertexBlob->GetBufferPointer(),
            upscalerVertexBlob->GetBufferSize(),
            nullptr, &upscalerVertexShader
        );
        upscalerVertexShader->AddRef();

        pDevice->CreatePixelShader(
            upscalerPixelBlob->GetBufferPointer(),
            upscalerPixelBlob->GetBufferSize(),
            nullptr, &upscalerPixelShader
        );
        upscalerPixelShader->AddRef();

        D3D11_SAMPLER_DESC upscalerSamplerDesc = {};
        upscalerSamplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
        upscalerSamplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
        upscalerSamplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
        upscalerSamplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
        upscalerSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        upscalerSamplerDesc.MinLOD         = 0.0f;
        upscalerSamplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;
        pDevice->CreateSamplerState(&upscalerSamplerDesc, &upscalerSampler);
        upscalerSampler->AddRef();
    }

    return res;
}

void D3D11::Load()
{
    HMODULE D3DCompiler_47 = LoadLibraryA("D3DCompiler_47.dll");
    if (!D3DCompiler_47) {
        spdlog::warn("[D3D11] Failed to load D3DCompiler_47.dll.");
        return;
    }

    D3DCompile = reinterpret_cast<pD3DCompile>(
        GetProcAddress(D3DCompiler_47, "D3DCompile")
        );
    if (!D3DCompile) {
        spdlog::warn("[D3D11] D3DCompile lookup failed.");
        return;
    }

    Upscale(nullptr);

#if defined(M2FIX_USE_IMGUI)
    Overlay(nullptr);
#endif

    HMODULE d3d11 = LoadLibraryA("d3d11.dll");
    if (!d3d11) {
        spdlog::warn("[D3D11] Failed to load d3d11.dll.");
        return;
    }

    auto d3d11_CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(
        GetProcAddress(d3d11, "D3D11CreateDevice")
    );
    if (!d3d11_CreateDevice) {
        spdlog::info("[D3D11] D3D11CreateDevice lookup failed.");
        return;
    }
    M2Hook::GetInstance().Hook(
        d3d11_CreateDevice,
        D3D11::Device::CreateDevice, "[D3D11] D3D11CreateDevice"
    );
}

// Forces the games to run on the user's dedicated GPU in the event that they have
// a secondary integrated GPU that isn't properly configured in their nvidia/amd display drivers.
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
