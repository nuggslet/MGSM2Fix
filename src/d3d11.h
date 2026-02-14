#pragma once

#include "m2fixbase.h"

#include <d3d11.h>
#include <d3dcompiler.h>

class D3D11 : public M2FixBase
{
public:
	static auto & GetInstance(D3D11 *instance = nullptr)
	{
		static D3D11 _instance_;
		static D3D11 *_instance = nullptr;
		if (instance) _instance = instance;
		if (!instance && !_instance) {
			_instance = &_instance_;
		}
		return *_instance;
	}

	static void LoadInstance(D3D11 *instance = nullptr) {
		GetInstance(instance).Load();
	}

	virtual void Load() override;

	static inline pD3DCompile D3DCompile = nullptr;

protected:
	static void Upscale(ID3D11DeviceContext *pContext);
#if defined(M2FIX_USE_IMGUI)
	static void Overlay(ID3D11DeviceContext *pContext);
#endif

	typedef struct
	{
		D3D11_PRIMITIVE_TOPOLOGY                PrimitiveTopology;
		ID3D11InputLayout                       *InputLayout;
		ID3D11Buffer                            *IndexBuffer;
		DXGI_FORMAT                             IndexBufferFormat;
		UINT                                    IndexBufferOffset;
		std::vector<ID3D11RenderTargetView *>   RenderTargetViews;
		ID3D11DepthStencilView                  *DepthStencilView;
		std::vector<ID3D11Buffer *>             VertexBuffers;
		std::vector<UINT>                       VertexStrides;
		std::vector<UINT>                       VertexOffsets;
		std::vector<D3D11_VIEWPORT>             Viewports;
		std::vector<D3D11_RECT>                 ScissorRects;
		ID3D11VertexShader                      *VertexShader;
		std::vector<ID3D11SamplerState *>       VertexSamplers;
		std::vector<ID3D11ShaderResourceView *> VertexShaderResources;
		std::vector<ID3D11Buffer *>             VertexConstantBuffers;
		ID3D11PixelShader                       *PixelShader;
		std::vector<ID3D11SamplerState *>       PixelSamplers;
		std::vector<ID3D11ShaderResourceView *> PixelShaderResources;
		std::vector<ID3D11Buffer *>             PixelConstantBuffers;
		ID3D11BlendState                        *BlendState;
		FLOAT                                   BlendFactor[4];
		UINT                                    BlendSampleMask;
		ID3D11RasterizerState                   *RasterizerState;
		ID3D11DepthStencilState                 *DepthStencilState;
		UINT                                    DepthStencilRef;
		UINT VertexCount;
		UINT StartVertexLocation;
		std::map<ID3D11RenderTargetView *, bool>     ClearRenderTargetViews;
		std::map<ID3D11RenderTargetView *, FLOAT[4]> ColorRenderTargetViews;
		std::map<ID3D11Buffer *, std::vector<unsigned char>> VertexConstantArchives;
		std::map<ID3D11Buffer *, std::vector<unsigned char>> PixelConstantArchives;
	} State;

	static void Queue(ID3D11DeviceContext *pContext, std::deque<State> & states, UINT VertexCount = 0, UINT StartVertexLocation = 0);
	static void Serve(ID3D11DeviceContext *pContext, std::deque<State> & states);

#if defined(M2FIX_USE_IMGUI)
	static BOOL WINAPI ShowWindow(
		HWND hWnd,
		int  nCmdShow
	);
#endif

	class Immediate {
	public:
		static void WINAPI UpdateSubresource(
			ID3D11DeviceContext *pContext,
			ID3D11Resource      *pDstResource,
			UINT                DstSubresource,
			D3D11_BOX           *pDstBox,
			void                *pSrcData,
			UINT                SrcRowPitch,
			UINT                SrcDepthPitch
		);

		static void WINAPI CopySubresourceRegion(
			ID3D11DeviceContext *pContext,
			ID3D11Resource      *pDstResource,
			UINT                DstSubresource,
			UINT                DstX,
			UINT                DstY,
			UINT                DstZ,
			ID3D11Resource      *pSrcResource,
			UINT                SrcSubresource,
			D3D11_BOX           *pSrcBox
		);

		static void WINAPI VSSetShaderResources(
			ID3D11DeviceContext      *pContext,
			UINT                     StartSlot,
			UINT                     NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews
		);

		static void WINAPI PSSetShaderResources(
			ID3D11DeviceContext      *pContext,
			UINT                     StartSlot,
			UINT                     NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews
		);

		static void WINAPI VSSetSamplers(
			ID3D11DeviceContext *pContext,
			UINT                StartSlot,
			UINT                NumSamplers,
			ID3D11SamplerState  **ppSamplers
		);

		static void WINAPI PSSetSamplers(
			ID3D11DeviceContext *pContext,
			UINT                StartSlot,
			UINT                NumSamplers,
			ID3D11SamplerState  **ppSamplers
		);

		static void WINAPI RSSetViewports(
			ID3D11DeviceContext *pContext,
			UINT                NumViewports,
			D3D11_VIEWPORT      *pViewports
		);

		static void WINAPI RSSetScissorRects(
			ID3D11DeviceContext *pContext,
			UINT                NumRects,
			D3D11_RECT          *pRects
		);

		static void WINAPI ClearRenderTargetView(
			ID3D11DeviceContext    *pContext,
			ID3D11RenderTargetView *pRenderTargetView,
			FLOAT                  ColorRGBA[4]
		);

		static void WINAPI Draw(
			ID3D11DeviceContext *pContext,
			UINT                VertexCount,
			UINT                StartVertexLocation
		);

		static void WINAPI VSSetShader(
			ID3D11DeviceContext *pContext,
			ID3D11VertexShader  *pVertexShader,
			ID3D11ClassInstance **ppClassInstances,
			UINT                NumClassInstances
		);

		static void WINAPI PSSetShader(
			ID3D11DeviceContext *pContext,
			ID3D11PixelShader   *pPixelShader,
			ID3D11ClassInstance **ppClassInstances,
			UINT                NumClassInstances
		);

		static void WINAPI IASetPrimitiveTopology(
			ID3D11DeviceContext      *pContext,
			D3D11_PRIMITIVE_TOPOLOGY Topology
		);

		static void WINAPI IASetInputLayout(
			ID3D11DeviceContext *pContext,
			ID3D11InputLayout   *pInputLayout
		);

		static void WINAPI IASetIndexBuffer(
			ID3D11DeviceContext *pContext,
			ID3D11Buffer        *pIndexBuffer,
			DXGI_FORMAT         Format,
			UINT                Offset
		);

		static void WINAPI IASetVertexBuffers(
			ID3D11DeviceContext *pContext,
			UINT                StartSlot,
			UINT                NumBuffers,
			ID3D11Buffer        **ppVertexBuffers,
			UINT                *pStrides,
			UINT                *pOffsets
		);

		static void WINAPI OMSetRenderTargets(
			ID3D11DeviceContext    *pContext,
			UINT                   NumViews,
			ID3D11RenderTargetView **ppRenderTargetViews,
			ID3D11DepthStencilView *pDepthStencilView
		);

		static void WINAPI ClearDepthStencilView(
			ID3D11DeviceContext    *pContext,
			ID3D11DepthStencilView *pDepthStencilView,
			UINT                   ClearFlags,
			FLOAT                  Depth,
			UINT8                  Stencil
		);

		static void WINAPI ExecuteCommandList(
			ID3D11DeviceContext *pContext,
			ID3D11CommandList   *pCommandList,
			BOOL                RestoreContextState
		);

		static HRESULT WINAPI FinishCommandList(
			ID3D11DeviceContext *pContext,
			BOOL                RestoreDeferredContextState,
			ID3D11CommandList   **ppCommandList
		);

		static void WINAPI DrawIndexed(
			ID3D11DeviceContext *pContext,
			UINT                IndexCount,
			UINT                StartIndexLocation,
			INT                 BaseVertexLocation
		);
	};

	class Deferred {
	public:
		static void WINAPI UpdateSubresource(
			ID3D11DeviceContext *pContext,
			ID3D11Resource      *pDstResource,
			UINT                DstSubresource,
			D3D11_BOX           *pDstBox,
			void                *pSrcData,
			UINT                SrcRowPitch,
			UINT                SrcDepthPitch
		);

		static void WINAPI CopySubresourceRegion(
			ID3D11DeviceContext *pContext,
			ID3D11Resource      *pDstResource,
			UINT                DstSubresource,
			UINT                DstX,
			UINT                DstY,
			UINT                DstZ,
			ID3D11Resource      *pSrcResource,
			UINT                SrcSubresource,
			D3D11_BOX           *pSrcBox
		);

		static void WINAPI VSSetShaderResources(
			ID3D11DeviceContext      *pContext,
			UINT                     StartSlot,
			UINT                     NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews
		);

		static void WINAPI PSSetShaderResources(
			ID3D11DeviceContext      *pContext,
			UINT                     StartSlot,
			UINT                     NumViews,
			ID3D11ShaderResourceView **ppShaderResourceViews
		);

		static void WINAPI VSSetSamplers(
			ID3D11DeviceContext *pContext,
			UINT                StartSlot,
			UINT                NumSamplers,
			ID3D11SamplerState  **ppSamplers
		);

		static void WINAPI PSSetSamplers(
			ID3D11DeviceContext *pContext,
			UINT                StartSlot,
			UINT                NumSamplers,
			ID3D11SamplerState  **ppSamplers
		);

		static void WINAPI RSSetViewports(
			ID3D11DeviceContext *pContext,
			UINT                NumViewports,
			D3D11_VIEWPORT      *pViewports
		);

		static void WINAPI RSSetScissorRects(
			ID3D11DeviceContext *pContext,
			UINT                NumRects,
			D3D11_RECT          *pRects
		);

		static void WINAPI ClearRenderTargetView(
			ID3D11DeviceContext    *pContext,
			ID3D11RenderTargetView *pRenderTargetView,
			FLOAT                  ColorRGBA[4]
		);

		static void WINAPI Draw(
			ID3D11DeviceContext *pContext,
			UINT                VertexCount,
			UINT                StartVertexLocation
		);

		static void WINAPI VSSetShader(
			ID3D11DeviceContext *pContext,
			ID3D11VertexShader  *pVertexShader,
			ID3D11ClassInstance **ppClassInstances,
			UINT                NumClassInstances
		);

		static void WINAPI PSSetShader(
			ID3D11DeviceContext *pContext,
			ID3D11PixelShader   *pPixelShader,
			ID3D11ClassInstance **ppClassInstances,
			UINT                NumClassInstances
		);

		static void WINAPI IASetPrimitiveTopology(
			ID3D11DeviceContext      *pContext,
			D3D11_PRIMITIVE_TOPOLOGY Topology
		);

		static void WINAPI IASetInputLayout(
			ID3D11DeviceContext *pContext,
			ID3D11InputLayout   *pInputLayout
		);

		static void WINAPI IASetIndexBuffer(
			ID3D11DeviceContext *pContext,
			ID3D11Buffer        *pIndexBuffer,
			DXGI_FORMAT         Format,
			UINT                Offset
		);

		static void WINAPI IASetVertexBuffers(
			ID3D11DeviceContext *pContext,
			UINT                StartSlot,
			UINT                NumBuffers,
			ID3D11Buffer        **ppVertexBuffers,
			UINT                *pStrides,
			UINT                *pOffsets
		);

		static void WINAPI OMSetRenderTargets(
			ID3D11DeviceContext    *pContext,
			UINT                   NumViews,
			ID3D11RenderTargetView **ppRenderTargetViews,
			ID3D11DepthStencilView *pDepthStencilView
		);

		static void WINAPI ClearDepthStencilView(
			ID3D11DeviceContext    *pContext,
			ID3D11DepthStencilView *pDepthStencilView,
			UINT                   ClearFlags,
			FLOAT                  Depth,
			UINT8                  Stencil
		);

		static void WINAPI ExecuteCommandList(
			ID3D11DeviceContext *pContext,
			ID3D11CommandList   *pCommandList,
			BOOL                RestoreContextState
		);

		static HRESULT WINAPI FinishCommandList(
			ID3D11DeviceContext *pContext,
			BOOL                RestoreDeferredContextState,
			ID3D11CommandList   **ppCommandList
		);

		static void WINAPI DrawIndexed(
			ID3D11DeviceContext *pContext,
			UINT                IndexCount,
			UINT                StartIndexLocation,
			INT                 BaseVertexLocation
		);
	};

	class Device {
	public:
		static HRESULT WINAPI CreateTexture2D(
			ID3D11Device           *pDevice,
			D3D11_TEXTURE2D_DESC   *pDesc,
			D3D11_SUBRESOURCE_DATA *pInitialData,
			ID3D11Texture2D        **ppTexture2D
		);

		static HRESULT WINAPI CreateVertexShader(
			ID3D11Device       *pDevice,
			void               *pShaderBytecode,
			SIZE_T             BytecodeLength,
			ID3D11ClassLinkage *pClassLinkage,
			ID3D11VertexShader **ppVertexShader
		);

		static HRESULT WINAPI CreatePixelShader(
			ID3D11Device       *pDevice,
			void               *pShaderBytecode,
			SIZE_T             BytecodeLength,
			ID3D11ClassLinkage *pClassLinkage,
			ID3D11PixelShader  **ppPixelShader
		);

		static HRESULT WINAPI CreateRenderTargetView(
			ID3D11Device                  *pDevice,
			ID3D11Resource                *pResource,
			D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
			ID3D11RenderTargetView        **ppRTView
		);

		static HRESULT WINAPI CreateShaderResourceView(
			ID3D11Device                    *pDevice,
			ID3D11Resource                  *pResource,
			D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
			ID3D11ShaderResourceView        **ppSRView
		);

		static HRESULT WINAPI CreateSamplerState(
			ID3D11Device       *pDevice,
			D3D11_SAMPLER_DESC *pSamplerDesc,
			ID3D11SamplerState **ppSamplerState
		);

		static HRESULT WINAPI CreateDeferredContext(
			ID3D11Device        *pDevice,
			UINT                ContextFlags,
			ID3D11DeviceContext **ppDeferredContext
		);

		static HRESULT WINAPI CreateDevice(
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
		);
	};

	virtual void WINAPI UpdateSubresource(
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
	);

	virtual void WINAPI CopySubresourceRegion(
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
	);

	virtual void WINAPI VSSetShaderResources(
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
	);

	virtual void WINAPI PSSetShaderResources(
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
	);

	virtual void WINAPI VSSetSamplers(
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
	);

	virtual void WINAPI PSSetSamplers(
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
	);

	virtual void WINAPI RSSetViewports(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext *pContext,
			UINT                NumViewports,
			D3D11_VIEWPORT      *pViewports
		),
		ID3D11DeviceContext *pContext,
		UINT                NumViewports,
		D3D11_VIEWPORT      *pViewports
	);

	virtual void WINAPI RSSetScissorRects(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext *pContext,
			UINT                NumRects,
			D3D11_RECT          *pRects
		),
		ID3D11DeviceContext *pContext,
		UINT                NumRects,
		D3D11_RECT          *pRects
	);

	virtual void WINAPI ClearRenderTargetView(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext    *pContext,
			ID3D11RenderTargetView *pRenderTargetView,
			FLOAT                  ColorRGBA[4]
		),
		ID3D11DeviceContext    *pContext,
		ID3D11RenderTargetView *pRenderTargetView,
		FLOAT                  ColorRGBA[4]
	);

	virtual void WINAPI Draw(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext *pContext,
			UINT                VertexCount,
			UINT                StartVertexLocation
		),
		ID3D11DeviceContext *pContext,
		UINT                VertexCount,
		UINT                StartVertexLocation
	);

	virtual void WINAPI VSSetShader(
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
	);

	virtual void WINAPI PSSetShader(
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
	);

	virtual void WINAPI IASetPrimitiveTopology(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext      *pContext,
			D3D11_PRIMITIVE_TOPOLOGY Topology
		),
		ID3D11DeviceContext      *pContext,
		D3D11_PRIMITIVE_TOPOLOGY Topology
	);

	virtual void WINAPI IASetInputLayout(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext *pContext,
			ID3D11InputLayout   *pInputLayout
		),
		ID3D11DeviceContext *pContext,
		ID3D11InputLayout   *pInputLayout
	);

	virtual void WINAPI IASetIndexBuffer(
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
	);

	virtual void WINAPI IASetVertexBuffers(
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
	);

	virtual void WINAPI OMSetRenderTargets(
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
	);

	virtual void WINAPI ClearDepthStencilView(
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
	);

	virtual void WINAPI ExecuteCommandList(
		void (WINAPI *pFunction)(
			ID3D11DeviceContext *pContext,
			ID3D11CommandList   *pCommandList,
			BOOL                RestoreContextState
		),
		ID3D11DeviceContext *pContext,
		ID3D11CommandList   *pCommandList,
		BOOL                RestoreContextState
	);

	virtual HRESULT WINAPI FinishCommandList(
		HRESULT (WINAPI *pFunction)(
			ID3D11DeviceContext *pContext,
			BOOL                RestoreDeferredContextState,
			ID3D11CommandList   **ppCommandList
		),
		ID3D11DeviceContext *pContext,
		BOOL                RestoreDeferredContextState,
		ID3D11CommandList   **ppCommandList
	);

	virtual void WINAPI DrawIndexed(
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
	);

	virtual HRESULT WINAPI CreateTexture2D(
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
	);

	virtual HRESULT WINAPI CreateVertexShader(
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
	);

	virtual HRESULT WINAPI CreatePixelShader(
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
	);

	virtual HRESULT WINAPI CreateRenderTargetView(
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
	);

	virtual HRESULT WINAPI CreateShaderResourceView(
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
	);

	virtual HRESULT WINAPI CreateSamplerState(
		HRESULT (WINAPI *pFunction)(
			ID3D11Device       *pDevice,
			D3D11_SAMPLER_DESC *pSamplerDesc,
			ID3D11SamplerState **ppSamplerState
		),
		ID3D11Device       *pDevice,
		D3D11_SAMPLER_DESC *pSamplerDesc,
		ID3D11SamplerState **ppSamplerState
	);

	virtual HRESULT WINAPI CreateDeferredContext(
		HRESULT (WINAPI *pFunction)(
			ID3D11Device        *pDevice,
			UINT                ContextFlags,
			ID3D11DeviceContext **ppDeferredContext
		),
		ID3D11Device        *pDevice,
		UINT                ContextFlags,
		ID3D11DeviceContext **ppDeferredContext
	);

	virtual HRESULT WINAPI CreateDevice(
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
	);

public:
	static inline ID3D11Device        *Device           = nullptr;
	static inline ID3D11DeviceContext *ImmediateContext = nullptr;
	static inline ID3D11DeviceContext *DeferredContext  = nullptr;

	static inline ID3D11VertexShader *upscalerVertexShader = nullptr;
	static inline ID3D11PixelShader  *upscalerPixelShader  = nullptr;
	static inline ID3DBlob           *upscalerVertexBlob   = nullptr;
	static inline ID3DBlob           *upscalerPixelBlob    = nullptr;
	static inline ID3D11SamplerState *upscalerSampler      = nullptr;

	static inline std::vector<ID3D11Texture2D *> texVram = {};
	static inline std::vector<ID3D11Texture2D *> texVramRemastered = {};
	static inline std::map<ID3D11Texture2D *,          ID3D11ShaderResourceView *> srvVram = {};
	static inline std::map<ID3D11ShaderResourceView *, ID3D11ShaderResourceView *> srvVramRemastered = {};
	static inline std::map<ID3D11ShaderResourceView *, ID3D11RenderTargetView *>   rtvVramRemastered = {};
	static inline D3D11_TEXTURE2D_DESC descVramRemastered = {};

	static inline bool upscalerDisabled = true;
	static inline bool overlayDisabled  = true;

	static inline std::map<ID3D11Buffer *, std::vector<unsigned char>> Buffers = {};

	static inline std::deque<State> upscalerDraws = {};
};
