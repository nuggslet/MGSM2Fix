#include "m2fix.h"
#include "psx.h"

#include "d3d11.h"

#include <d3dcompiler.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "resource.h"

ID3D11Device        *D3D11::Device = nullptr;
ID3D11DeviceContext *D3D11::DeferredContext = nullptr;

ID3D11VertexShader *D3D11::upscalerVertexShader = nullptr;
ID3D11PixelShader  *D3D11::upscalerPixelShader = nullptr;
ID3DBlob           *D3D11::upscalerVertexBlob = nullptr;
ID3DBlob           *D3D11::upscalerPixelBlob = nullptr;

std::map<ID3D11Texture2D *,          ID3D11ShaderResourceView *> D3D11::srvVram = {};
std::map<ID3D11ShaderResourceView *, ID3D11ShaderResourceView *> D3D11::srvVramRemastered = {};
std::map<ID3D11ShaderResourceView *, ID3D11RenderTargetView *>   D3D11::rtvVramRemastered = {};

std::vector<ID3D11Texture2D *>  D3D11::texVram = {};
std::vector<ID3D11Texture2D *>  D3D11::texVramRemastered = {};
D3D11_TEXTURE2D_DESC            D3D11::descVramRemastered = {};

bool D3D11::upscalerDisabled = true;
bool D3D11::overlayDisabled = true;

HRESULT WINAPI D3D11::CreateTexture2D(
    ID3D11Device           *pDevice,
    D3D11_TEXTURE2D_DESC   *pDesc,
    D3D11_SUBRESOURCE_DATA *pInitialData,
    ID3D11Texture2D        **ppTexture2D
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreateTexture2D,
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
            CreateTexture2D,
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
            CreateTexture2D,
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

HRESULT WINAPI D3D11::CreateVertexShader(
    ID3D11Device       *pDevice,
    void               *pShaderBytecode,
    SIZE_T             BytecodeLength,
    ID3D11ClassLinkage *pClassLinkage,
    ID3D11VertexShader **ppVertexShader
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreateVertexShader,
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

HRESULT WINAPI D3D11::CreatePixelShader(
    ID3D11Device       *pDevice,
    void               *pShaderBytecode,
    SIZE_T             BytecodeLength,
    ID3D11ClassLinkage *pClassLinkage,
    ID3D11PixelShader  **ppPixelShader
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreatePixelShader,
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
    return D3D11::UpdateSubresource(
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
    return D3D11::UpdateSubresource(
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

    return M2Hook::GetInstance().VirtualInvoke<void>(
        pFunction,
        pContext,
        pDstResource,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch
    );
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
    return D3D11::CopySubresourceRegion(
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
    return D3D11::CopySubresourceRegion(
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

HRESULT WINAPI D3D11::CreateRenderTargetView(
    ID3D11Device                  *pDevice,
    ID3D11Resource                *pResource,
    D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D11RenderTargetView        **ppRTView
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreateRenderTargetView,
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

HRESULT WINAPI D3D11::CreateShaderResourceView(
    ID3D11Device                    *pDevice,
    ID3D11Resource                  *pResource,
    D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D11ShaderResourceView        **ppSRView
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreateShaderResourceView,
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
            CreateShaderResourceView,
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

HRESULT WINAPI D3D11::CreateSamplerState(
    ID3D11Device       *pDevice,
    D3D11_SAMPLER_DESC *pSamplerDesc,
    ID3D11SamplerState **ppSamplerState
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreateSamplerState,
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
    return D3D11::VSSetShaderResources(
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
    return D3D11::VSSetShaderResources(
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
    return D3D11::PSSetShaderResources(
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
    return D3D11::PSSetShaderResources(
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
    return D3D11::VSSetSamplers(
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
    return D3D11::VSSetSamplers(
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
    return D3D11::PSSetSamplers(
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
    return D3D11::PSSetSamplers(
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
    return D3D11::RSSetViewports(
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
    return D3D11::RSSetViewports(
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
    return D3D11::RSSetScissorRects(
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
    return D3D11::RSSetScissorRects(
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
    return D3D11::ClearRenderTargetView(
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
    return D3D11::ClearRenderTargetView(
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
    return D3D11::Draw(
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
    return D3D11::Draw(
        D3D11::Deferred::Draw,
        pContext,
        VertexCount,
        StartVertexLocation
    );
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
    return D3D11::VSSetShader(
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
    return D3D11::VSSetShader(
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
    return D3D11::PSSetShader(
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
    return D3D11::PSSetShader(
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
    return D3D11::IASetPrimitiveTopology(
        D3D11::Immediate::IASetPrimitiveTopology,
        pContext,
        Topology
    );
}

void WINAPI D3D11::Deferred::IASetPrimitiveTopology(
    ID3D11DeviceContext      *pContext,
    D3D11_PRIMITIVE_TOPOLOGY Topology
) {
    return D3D11::IASetPrimitiveTopology(
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
    return D3D11::IASetInputLayout(
        D3D11::Immediate::IASetInputLayout,
        pContext,
        pInputLayout
    );
}

void WINAPI D3D11::Deferred::IASetInputLayout(
    ID3D11DeviceContext *pContext,
    ID3D11InputLayout   *pInputLayout
) {
    return D3D11::IASetInputLayout(
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
    return D3D11::IASetIndexBuffer(
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
    return D3D11::IASetIndexBuffer(
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
    return D3D11::IASetVertexBuffers(
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
    return D3D11::IASetVertexBuffers(
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
    return D3D11::OMSetRenderTargets(
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
    return D3D11::OMSetRenderTargets(
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

        HMODULE D3DCompiler_47 = LoadLibraryA("D3DCompiler_47.dll");
        if (!D3DCompiler_47) {
            spdlog::warn("[D3D11] Failed to load D3DCompiler_47.dll.");
            return;
        }

        auto D3DCompiler_47_D3DCompile = reinterpret_cast<pD3DCompile>(
            GetProcAddress(D3DCompiler_47, "D3DCompile")
        );
        if (!D3DCompiler_47_D3DCompile) {
            spdlog::warn("[D3D11] D3DCompile lookup failed.");
            return;
        }

        auto upscalerVertexSource = reinterpret_cast<const char *>(
                M2Hook::GetInstance(".").ModuleResource(IDR_HLSL1, "HLSL")
        );
        ID3DBlob *upscalerVertexError = nullptr;
        res = D3DCompiler_47_D3DCompile(
            upscalerVertexSource,
            strlen(upscalerVertexSource),
            std::format("{}::VertexUpscaler", M2Fix::FixName()).c_str(),
            nullptr,
            nullptr,
            "main",
            "vs_4_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
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
        res = D3DCompiler_47_D3DCompile(
            upscalerPixelSource,
            strlen(upscalerPixelSource),
            std::format("{}::PixelUpscaler", M2Fix::FixName()).c_str(),
            nullptr,
            nullptr,
            "main",
            "ps_4_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
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
        ID3D11ShaderResourceView *nullSRV = nullptr;
        ID3D11RenderTargetView *dstRTV    = rtvVramRemastered[srcSRV];

        if (M2Fix::GameInstance().GWBlank()) {
            ID3D11RenderTargetView *originalRTV = nullptr;
            ID3D11DepthStencilView *originalDSV = nullptr;
            pContext->OMGetRenderTargets(1, &originalRTV, &originalDSV);
            pContext->OMSetRenderTargets(1, &dstRTV, nullptr);

            FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            pContext->ClearRenderTargetView(dstRTV, ColorRGBA);

            pContext->OMSetRenderTargets(1, &originalRTV, originalDSV);
            if (originalRTV) originalRTV->Release();
            if (originalDSV) originalDSV->Release();

            continue;
        }

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        sampDesc.MinLOD         = 0.0f;
        sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;

        ID3D11SamplerState *sampler = nullptr;
        Device->CreateSamplerState(&sampDesc, &sampler);

        ID3D11RenderTargetView *originalRTV = nullptr;
        ID3D11DepthStencilView *originalDSV = nullptr;
        pContext->OMGetRenderTargets(1, &originalRTV, &originalDSV);
        pContext->OMSetRenderTargets(1, &dstRTV, nullptr);

        UINT numViewports = 1;
        D3D11_VIEWPORT originalViewport = {};
        pContext->RSGetViewports(&numViewports, &originalViewport);
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width    = static_cast<float>(descVramRemastered.Width);
        viewport.Height   = static_cast<float>(descVramRemastered.Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        pContext->RSSetViewports(1, &viewport);

        UINT numRects = 1;
        D3D11_RECT originalScissor = {};
        pContext->RSGetScissorRects(&numRects, &originalScissor);
        D3D11_RECT scissor = {};
        scissor.left   = 0;
        scissor.top    = 0;
        scissor.right  = descVramRemastered.Width;
        scissor.bottom = descVramRemastered.Height;
        pContext->RSSetScissorRects(1, &scissor);

        FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        pContext->ClearRenderTargetView(dstRTV, ColorRGBA);

        ID3D11InputLayout *originalInputLayout = nullptr;
        pContext->IAGetInputLayout(&originalInputLayout);
        pContext->IASetInputLayout(nullptr);

        D3D11_PRIMITIVE_TOPOLOGY originalPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        pContext->IAGetPrimitiveTopology(&originalPrimitiveTopology);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11Buffer *originalVertexBuffer = nullptr;
        UINT originalVertexStride = 0, originalVertexOffset = 0;
        pContext->IAGetVertexBuffers(0, 1, &originalVertexBuffer, &originalVertexStride, &originalVertexOffset);
        pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

        ID3D11Buffer *originalIndexBuffer = nullptr;
        DXGI_FORMAT originalIndexFormat = DXGI_FORMAT_UNKNOWN;
        UINT originalIndexOffset = 0;
        pContext->IAGetIndexBuffer(&originalIndexBuffer, &originalIndexFormat, &originalIndexOffset);
        pContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

        ID3D11VertexShader *originalVertexShader = nullptr;
        pContext->VSGetShader(&originalVertexShader, nullptr, nullptr);
        pContext->VSSetShader(upscalerVertexShader, nullptr, 0);

        ID3D11PixelShader *originalPixelShader = nullptr;
        pContext->PSGetShader(&originalPixelShader, nullptr, nullptr);
        pContext->PSSetShader(upscalerPixelShader, nullptr, 0);

        ID3D11ShaderResourceView *originalSRV = nullptr;
        pContext->PSGetShaderResources(0, 1, &originalSRV);
        upscalerDisabled = true; {
            pContext->PSSetShaderResources(0, 1, &srcSRV);
        } upscalerDisabled = false;

        ID3D11SamplerState *originalSampler = nullptr;
        pContext->PSGetSamplers(0, 1, &originalSampler);
        pContext->PSSetSamplers(0, 1, &sampler);

        upscalerDisabled = true; {
            pContext->Draw(3, 0);
        } upscalerDisabled = false;

        // At this point we are just fixing the pipeline after interfering with it.
        pContext->OMSetRenderTargets(1, &originalRTV, originalDSV);
        if (originalRTV) originalRTV->Release();
        if (originalDSV) originalDSV->Release();

        pContext->RSSetViewports(1, &originalViewport);
        pContext->RSSetScissorRects(1, &originalScissor);

        pContext->IASetInputLayout(originalInputLayout);
        if (originalInputLayout) originalInputLayout->Release();
        pContext->IASetPrimitiveTopology(originalPrimitiveTopology);
        pContext->IASetVertexBuffers(0, 0, &originalVertexBuffer, &originalVertexStride, &originalVertexOffset);
        if (originalVertexBuffer) originalVertexBuffer->Release();
        pContext->IASetIndexBuffer(originalIndexBuffer, originalIndexFormat, originalIndexOffset);
        if (originalIndexBuffer) originalIndexBuffer->Release();

        pContext->VSSetShader(originalVertexShader, nullptr, 0);
        if (originalVertexShader) originalVertexShader->Release();
        pContext->PSSetShader(originalPixelShader, nullptr, 0);
        if (originalPixelShader) originalPixelShader->Release();
        pContext->PSSetSamplers(0, 1, &originalSampler);
        if (originalSampler) originalSampler->Release();

        upscalerDisabled = true; {
            pContext->PSSetShaderResources(0, 1, &originalSRV);
        } upscalerDisabled = false;
        if (originalSRV) originalSRV->Release();
    }
}

void WINAPI D3D11::Immediate::ClearDepthStencilView(
    ID3D11DeviceContext    *pContext,
    ID3D11DepthStencilView *pDepthStencilView,
    UINT                   ClearFlags,
    FLOAT                  Depth,
    UINT8                  Stencil
) {
    return D3D11::ClearDepthStencilView(
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
    return D3D11::ClearDepthStencilView(
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
    return D3D11::ExecuteCommandList(
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
    return D3D11::ExecuteCommandList(
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
    return D3D11::DrawIndexed(
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
    return D3D11::DrawIndexed(
        D3D11::Deferred::DrawIndexed,
        pContext,
        IndexCount,
        StartIndexLocation,
        BaseVertexLocation
    );
}

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

    ImGui::Begin("MGSM2Fix", nullptr,
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

    Overlay(pContext);
}

HRESULT WINAPI D3D11::Immediate::FinishCommandList(
    ID3D11DeviceContext *pContext,
    BOOL                RestoreDeferredContextState,
    ID3D11CommandList   **ppCommandList
) {
    return D3D11::FinishCommandList(
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
    return D3D11::FinishCommandList(
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

HRESULT WINAPI D3D11::CreateDeferredContext(
    ID3D11Device        *pDevice,
    UINT                ContextFlags,
    ID3D11DeviceContext **ppDeferredContext
) {
    HRESULT res = M2Hook::GetInstance().VirtualInvoke<HRESULT>(
        CreateDeferredContext,
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

    VIRTUAL_HOOK(ClearDepthStencilView);
    VIRTUAL_HOOK(CopySubresourceRegion);
    VIRTUAL_HOOK(PSSetShaderResources);
    VIRTUAL_HOOK(DrawIndexed);

    if (M2Config::iRendererLevel >= 2) {
        VIRTUAL_HOOK(UpdateSubresource);
        VIRTUAL_HOOK(ClearRenderTargetView);
        VIRTUAL_HOOK(IASetPrimitiveTopology);
        VIRTUAL_HOOK(IASetInputLayout);
        VIRTUAL_HOOK(IASetIndexBuffer);
        VIRTUAL_HOOK(VSSetShader);
        VIRTUAL_HOOK(VSSetSamplers);
        VIRTUAL_HOOK(VSSetShaderResources);
        VIRTUAL_HOOK(PSSetShader);
        VIRTUAL_HOOK(PSSetSamplers);
        VIRTUAL_HOOK(RSSetViewports);
        VIRTUAL_HOOK(RSSetScissorRects);
        VIRTUAL_HOOK(OMSetRenderTargets);
        VIRTUAL_HOOK(Draw);
        VIRTUAL_HOOK(ExecuteCommandList);
        VIRTUAL_HOOK(FinishCommandList);
    }
    #undef VIRTUAL_HOOK

    return res;
}

HRESULT WINAPI D3D11::CreateDevice(
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
        CreateDevice,
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
            ##name, "[D3D11] ID3D11Device::" #name \
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

    M2Hook::GetInstance().VirtualTableHook(
        pImmediateContext, "[D3D11] [Immediate] ID3D11DeviceContext"
    );

#define VIRTUAL_HOOK(name) { \
        M2Hook::GetInstance().VirtualHook( \
            pImmediateContext, &ID3D11DeviceContext::name, \
            D3D11::Immediate::##name, "[D3D11] [Immediate] ID3D11DeviceContext::" #name \
        ); \
    }

    VIRTUAL_HOOK(ClearDepthStencilView);
    VIRTUAL_HOOK(CopySubresourceRegion);
    VIRTUAL_HOOK(PSSetShaderResources);
    VIRTUAL_HOOK(DrawIndexed);

    if (M2Config::iRendererLevel >= 2) {
        VIRTUAL_HOOK(UpdateSubresource);
        VIRTUAL_HOOK(ClearRenderTargetView);
        VIRTUAL_HOOK(IASetPrimitiveTopology);
        VIRTUAL_HOOK(IASetInputLayout);
        VIRTUAL_HOOK(IASetIndexBuffer);
        VIRTUAL_HOOK(VSSetShader);
        VIRTUAL_HOOK(VSSetSamplers);
        VIRTUAL_HOOK(VSSetShaderResources);
        VIRTUAL_HOOK(PSSetShader);
        VIRTUAL_HOOK(PSSetSamplers);
        VIRTUAL_HOOK(RSSetViewports);
        VIRTUAL_HOOK(RSSetScissorRects);
        VIRTUAL_HOOK(OMSetRenderTargets);
        VIRTUAL_HOOK(Draw);
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

        pDevice->CreatePixelShader(
            upscalerPixelBlob->GetBufferPointer(),
            upscalerPixelBlob->GetBufferSize(),
            nullptr, &upscalerPixelShader
        );
    }

    return res;
}

void D3D11::Load()
{
    Upscale(nullptr);
    Overlay(nullptr);

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
        CreateDevice, "[D3D11] D3D11CreateDevice"
    );
}
