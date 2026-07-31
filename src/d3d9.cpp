#include "m2fix.h"

#include "d3d9.h"
#include "color_correction.hpp"

IDirect3D9 * WINAPI D3D9::Direct3D::Create9(UINT SDKVersion)
{
    return D3D9::GetInstance().Create9(
        D3D9::Direct3D::Create9,
        SDKVersion
    );
}

IDirect3D9 * WINAPI D3D9::Create9(
    IDirect3D9 * (WINAPI *pFunction)(UINT SDKVersion),
    UINT SDKVersion
) {
    IDirect3D9 *pD3D9 = M2Hook::GetInstance().Invoke<IDirect3D9 *>(
        pFunction,
        SDKVersion
    );

    if (!pD3D9) return pD3D9;

    // IDirect3D9::CreateDevice is always vtable slot 16
    void **d3d9Vtable = *reinterpret_cast<void ***>(pD3D9);
    M2Hook::GetInstance().Hook(
        d3d9Vtable[16],
        D3D9::Device::CreateDevice, "[D3D9] IDirect3D9::CreateDevice"
    );

    return pD3D9;
}

HRESULT WINAPI D3D9::Device::CreateDevice(
    IDirect3D9             *pD3D9,
    UINT                   Adapter,
    D3DDEVTYPE             DeviceType,
    HWND                   hFocusWindow,
    DWORD                  BehaviorFlags,
    D3DPRESENT_PARAMETERS  *pPresentationParameters,
    IDirect3DDevice9       **ppReturnedDeviceInterface
) {
    return D3D9::GetInstance().CreateDevice(
        D3D9::Device::CreateDevice,
        pD3D9,
        Adapter,
        DeviceType,
        hFocusWindow,
        BehaviorFlags,
        pPresentationParameters,
        ppReturnedDeviceInterface
    );
}

HRESULT WINAPI D3D9::CreateDevice(
    HRESULT (WINAPI *pFunction)(
        IDirect3D9             *pD3D9,
        UINT                   Adapter,
        D3DDEVTYPE             DeviceType,
        HWND                   hFocusWindow,
        DWORD                  BehaviorFlags,
        D3DPRESENT_PARAMETERS  *pPresentationParameters,
        IDirect3DDevice9       **ppReturnedDeviceInterface
    ),
    IDirect3D9             *pD3D9,
    UINT                   Adapter,
    D3DDEVTYPE             DeviceType,
    HWND                   hFocusWindow,
    DWORD                  BehaviorFlags,
    D3DPRESENT_PARAMETERS  *pPresentationParameters,
    IDirect3DDevice9       **ppReturnedDeviceInterface
) {
    HRESULT res = M2Hook::GetInstance().Invoke<HRESULT>(
        pFunction,
        pD3D9,
        Adapter,
        DeviceType,
        hFocusWindow,
        BehaviorFlags,
        pPresentationParameters,
        ppReturnedDeviceInterface
    );

    if (FAILED(res) || Device || !ppReturnedDeviceInterface || !*ppReturnedDeviceInterface) {
        return res;
    }

    HookDevice(*ppReturnedDeviceInterface);

    return res;
}

void D3D9::HookDevice(IDirect3DDevice9 *pDevice)
{
    Device = pDevice;

    spdlog::info("[D3D9] IDirect3D9::CreateDevice() -> {}", fmt::ptr(pDevice));

    // IDirect3DDevice9::Reset is always vtable slot 16, Present is always slot 17.
    void **deviceVtable = *reinterpret_cast<void ***>(pDevice);
    M2Hook::GetInstance().Hook(
        deviceVtable[16],
        D3D9::Device::Reset, "[D3D9] IDirect3DDevice9::Reset"
    );
    M2Hook::GetInstance().Hook(
        deviceVtable[17],
        D3D9::Device::Present, "[D3D9] IDirect3DDevice9::Present"
    );

    ColorCorrection::Init();
}

HRESULT WINAPI D3D9::Device::Present(
    IDirect3DDevice9 *pDevice,
    const RECT       *pSourceRect,
    const RECT       *pDestRect,
    HWND             hDestWindowOverride,
    const RGNDATA    *pDirtyRegion
) {
    return D3D9::GetInstance().Present(
        D3D9::Device::Present,
        pDevice,
        pSourceRect,
        pDestRect,
        hDestWindowOverride,
        pDirtyRegion
    );
}

HRESULT WINAPI D3D9::Present(
    HRESULT (WINAPI *pFunction)(
        IDirect3DDevice9 *pDevice,
        const RECT       *pSourceRect,
        const RECT       *pDestRect,
        HWND             hDestWindowOverride,
        const RGNDATA    *pDirtyRegion
    ),
    IDirect3DDevice9 *pDevice,
    const RECT       *pSourceRect,
    const RECT       *pDestRect,
    HWND             hDestWindowOverride,
    const RGNDATA    *pDirtyRegion
) {
    ColorCorrection::DrawD3D9(pDevice);

    return M2Hook::GetInstance().Invoke<HRESULT>(
        pFunction,
        pDevice,
        pSourceRect,
        pDestRect,
        hDestWindowOverride,
        pDirtyRegion
    );
}

HRESULT WINAPI D3D9::Device::Reset(
    IDirect3DDevice9      *pDevice,
    D3DPRESENT_PARAMETERS *pPresentationParameters
) {
    return D3D9::GetInstance().Reset(
        D3D9::Device::Reset,
        pDevice,
        pPresentationParameters
    );
}

HRESULT WINAPI D3D9::Reset(
    HRESULT (WINAPI *pFunction)(
        IDirect3DDevice9      *pDevice,
        D3DPRESENT_PARAMETERS *pPresentationParameters
    ),
    IDirect3DDevice9      *pDevice,
    D3DPRESENT_PARAMETERS *pPresentationParameters
) {


    ColorCorrection::ReleaseD3D9();

    return M2Hook::GetInstance().Invoke<HRESULT>(
        pFunction,
        pDevice,
        pPresentationParameters
    );
}

void D3D9::Load()
{
    HMODULE D3DCompiler_47 = LoadLibraryA("D3DCompiler_47.dll");
    if (!D3DCompiler_47) {
        spdlog::warn("[D3D9] Failed to load D3DCompiler_47.dll.");
        return;
    }

    D3DCompile = reinterpret_cast<pD3DCompile>(
        GetProcAddress(D3DCompiler_47, "D3DCompile")
    );
    if (!D3DCompile) {
        spdlog::warn("[D3D9] D3DCompile lookup failed.");
        return;
    }

    HMODULE d3d9 = LoadLibraryA("d3d9.dll");
    if (!d3d9) {
        spdlog::warn("[D3D9] Failed to load d3d9.dll.");
        return;
    }

    typedef IDirect3D9 * (WINAPI *PFN_DIRECT3D_CREATE9)(UINT SDKVersion);

    auto direct3DCreate9 = reinterpret_cast<PFN_DIRECT3D_CREATE9>(
        GetProcAddress(d3d9, "Direct3DCreate9")
    );
    if (!direct3DCreate9) {
        spdlog::info("[D3D9] Direct3DCreate9 lookup failed.");
        return;
    }

    M2Hook::GetInstance().Hook(
        direct3DCreate9,
        D3D9::Direct3D::Create9, "[D3D9] Direct3DCreate9"
    );
}
