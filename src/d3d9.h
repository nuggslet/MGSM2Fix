#pragma once

#include "m2fixbase.h"

#include <d3d9.h>
#include <d3dcompiler.h>

class D3D9 : public M2FixBase
{
public:
	static auto & GetInstance(D3D9 *instance = nullptr)
	{
		static D3D9 _instance_;
		static D3D9 *_instance = nullptr;
		if (instance) _instance = instance;
		if (!instance && !_instance) {
			_instance = &_instance_;
		}
		return *_instance;
	}

	static void LoadInstance(D3D9 *instance = nullptr) {
		GetInstance(instance).Load();
	}

	virtual void Load() override;

	static inline pD3DCompile D3DCompile = nullptr;

protected:
	class Direct3D {
	public:
		static IDirect3D9 * WINAPI Create9(UINT SDKVersion);
	};

	class Device {
	public:
		static HRESULT WINAPI CreateDevice(
			IDirect3D9             *pD3D9,
			UINT                   Adapter,
			D3DDEVTYPE             DeviceType,
			HWND                   hFocusWindow,
			DWORD                  BehaviorFlags,
			D3DPRESENT_PARAMETERS  *pPresentationParameters,
			IDirect3DDevice9       **ppReturnedDeviceInterface
		);

		static HRESULT WINAPI Present(
			IDirect3DDevice9 *pDevice,
			const RECT       *pSourceRect,
			const RECT       *pDestRect,
			HWND             hDestWindowOverride,
			const RGNDATA    *pDirtyRegion
		);

		static HRESULT WINAPI Reset(
			IDirect3DDevice9      *pDevice,
			D3DPRESENT_PARAMETERS *pPresentationParameters
		);
	};

	virtual IDirect3D9 * WINAPI Create9(
		IDirect3D9 * (WINAPI *pFunction)(UINT SDKVersion),
		UINT SDKVersion
	);

	virtual HRESULT WINAPI CreateDevice(
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
	);

	virtual HRESULT WINAPI Present(
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
	);

	virtual HRESULT WINAPI Reset(
		HRESULT (WINAPI *pFunction)(
			IDirect3DDevice9      *pDevice,
			D3DPRESENT_PARAMETERS *pPresentationParameters
		),
		IDirect3DDevice9      *pDevice,
		D3DPRESENT_PARAMETERS *pPresentationParameters
	);

	static void HookDevice(IDirect3DDevice9 *pDevice);

public:
	static inline IDirect3DDevice9 *Device = nullptr;
};
