#include "stdafx.h"

#include "color_correction.hpp"

#include "m2config.h"
#include "sqemutask.h"

namespace
{
    // By Afevis. Original Vibrance + Curves are based off CeeJayDK's SweetFX shaders
    const char* kShaderD3D11 = R"(
    Texture2D    backBuffer : register(t0);
    SamplerState smpl       : register(s0);

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TexCoord)
    {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    }

    float4 PS(float4 pos : SV_Position, float2 uv : TexCoord) : SV_Target
    {
        float3 color = backBuffer.Sample(smpl, uv).rgb;

        static const float3 coefLuma = float3(0.212656, 0.715158, 0.072186);

        // Vibrance
        const float kVibrance = 0.15;
        float luma             = dot(coefLuma, color);
        float max_color        = max(color.r, max(color.g, color.b));
        float min_color        = min(color.r, min(color.g, color.b));
        float color_saturation = max_color - min_color;

        color = lerp(luma, color, 1.0 + (kVibrance * (1.0 - (sign(kVibrance) * color_saturation))));

        // Gamma Curve
        const float kContrast = 0.225;
        luma = dot(coefLuma, color);                    // recompute post-vibrance
        float3 chroma = color - luma;
        float s = luma;
        s = s * s * s * (s * (s * 6.0 - 15.0) + 10.0);  // Perlin's smootherstep
        luma  = lerp(luma, s, kContrast);               // blend by contrast
        color = luma + chroma;

        //AVS -> COLOR INVERSION TEST, UNCOMMENT TO CHECK IF THE SHADER IS WORKING <3
        //color = 1.0 - color;

        return float4(color, 1.0);
    }
    )";

    ComPtr<ID3DBlob>                 vsBlob;
    ComPtr<ID3DBlob>                 psBlob;
    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11SamplerState>       smpl;
    ComPtr<ID3D11Texture2D>          tempTex;
    ComPtr<ID3D11ShaderResourceView> tempSRV;
    ComPtr<ID3D11RasterizerState>    rs;
    ComPtr<ID3D11DepthStencilState>  dss;
    D3D11_TEXTURE2D_DESC             tempDesc = {};

    // Same as above, just ported to D3D9
    const char* kShaderD3D9 = R"(
    sampler2D backBuffer : register(s0);

    float4 PS(float2 uv : TEXCOORD0) : COLOR0
    {
        float3 color = tex2D(backBuffer, uv).rgb;

        static const float3 coefLuma = float3(0.212656, 0.715158, 0.072186);

        // Vibrance
        const float kVibrance = 0.15;
        float luma             = dot(coefLuma, color);
        float max_color        = max(color.r, max(color.g, color.b));
        float min_color        = min(color.r, min(color.g, color.b));
        float color_saturation = max_color - min_color;

        color = lerp(luma, color, 1.0 + (kVibrance * (1.0 - (sign(kVibrance) * color_saturation))));

        // Gamma Curve
        const float kContrast = 0.225;
        luma = dot(coefLuma, color);                    // recompute post-vibrance
        float3 chroma = color - luma;
        float s = luma;
        s = s * s * s * (s * (s * 6.0 - 15.0) + 10.0);  // Perlin's smootherstep
        luma  = lerp(luma, s, kContrast);               // blend by contrast
        color = luma + chroma;

        return float4(color, 1.0);
    }
    )";

    struct VertexD3D9
    {
        float x, y, z, rhw;
        float u, v;
    };

    ComPtr<ID3DBlob>              psBlobD3D9;
    ComPtr<IDirect3DPixelShader9> psD3D9;
    ComPtr<IDirect3DTexture9>     tempTexD3D9;
    D3DSURFACE_DESC                tempDescD3D9 = {};
}



void ColorCorrection::Setup()
{
    bEnabled = M2Config::bColorCorrectionEnabled;

    if (!bEnabled)
    {
        spdlog::info("[Gamma Correction] Disabled in config, skipping shader compilation.");
        return;
    }

    spdlog::info("[Gamma Correction] Compiling shaders...");

    if (!D3D11::D3DCompile)
    {
        spdlog::error("[Gamma Correction] Failed to get D3D11 D3DCompile");
    }
    else
    {
        ComPtr<ID3DBlob> err;

        HRESULT hr = D3D11::D3DCompile(kShaderD3D11, strlen(kShaderD3D11), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            spdlog::error("[Gamma Correction] VS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
        }
        else
        {
            err.Reset();

            hr = D3D11::D3DCompile(kShaderD3D11, strlen(kShaderD3D11), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
            if (FAILED(hr))
            {
                spdlog::error("[Gamma Correction] PS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
                vsBlob.Reset();
            }
            else
            {
                spdlog::info("[Gamma Correction] Shaders compiled successfully");
            }
        }
    }

    if (!D3D9::D3DCompile)
    {
        spdlog::error("[Gamma Correction D3D9] Failed to get D3D9 D3DCompile");
    }
    else
    {
        ComPtr<ID3DBlob> err;

        HRESULT hr = D3D9::D3DCompile(kShaderD3D9, strlen(kShaderD3D9), nullptr, nullptr, nullptr, "PS", "ps_3_0", 0, 0, psBlobD3D9.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            spdlog::error("[Gamma Correction D3D9] PS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
        }
        else
        {
            spdlog::info("[Gamma Correction D3D9] Shader compiled successfully");
        }
    }
}

// Called via D3D11::CreateDevice()/CreateDeviceAndSwapChain() or D3D9::HookDevice() depending on what the game actually uses. No-op when called from the unused backend.
void ColorCorrection::Init()
{
    if (!bEnabled)
    {
        return;
    }

    if (!bShaderLoaded)
    {
        if (ID3D11Device* dev = D3D11::Device; !dev)
        {
            // Game is likely using d3d9, no-op
        }
        else if (!vsBlob || !psBlob)
        {
            spdlog::error("[Gamma Correction] Shader bytecode missing");
        }
        else
        {
            dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf());
            dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf());

            D3D11_SAMPLER_DESC sd = {};
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            dev->CreateSamplerState(&sd, smpl.GetAddressOf());

            D3D11_RASTERIZER_DESC rd = {};
            rd.FillMode = D3D11_FILL_SOLID;
            rd.CullMode = D3D11_CULL_NONE;
            dev->CreateRasterizerState(&rd, rs.GetAddressOf());

            D3D11_DEPTH_STENCIL_DESC dsd = {};
            dsd.DepthEnable = FALSE;
            dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            dev->CreateDepthStencilState(&dsd, dss.GetAddressOf());

            vsBlob.Reset();
            psBlob.Reset();

            bShaderLoaded = true;
            spdlog::info("[Gamma Correction] Initialized");
        }
    }

    if (!bShaderLoadedD3D9)
    {
        if (IDirect3DDevice9* dev9 = D3D9::Device; !dev9)
        {
            // Game is likely using d3d11, no-op
        }
        else if (!psBlobD3D9)
        {
            spdlog::error("[Gamma Correction D3D9] Shader bytecode missing");
        }
        else
        {
            HRESULT hr = dev9->CreatePixelShader(static_cast<const DWORD*>(psBlobD3D9->GetBufferPointer()), psD3D9.GetAddressOf());
            if (FAILED(hr))
            {
                spdlog::error("[Gamma Correction D3D9] CreatePixelShader failed: {:#x}", static_cast<unsigned int>(hr));
            }
            else
            {
                psBlobD3D9.Reset();

                bShaderLoadedD3D9 = true;
                spdlog::info("[Gamma Correction D3D9] Initialized");
            }
        }
    }
}

// Save & restore the renderer's state before running our shader pass, otherwise the pause menu overlay will break.
void ColorCorrection::Draw(IDXGISwapChain* swap)
{
    if (!bShaderLoaded)
    {
        return;
    }

    //spdlog::info("[Gamma Correction] Drawing...");
    if (SQEmuTask_IsPaused())
    {
        //spdlog::info("paused");
        return;
    }

    auto* ctx = D3D11::ImmediateContext;
    auto* dev = D3D11::Device;

    ComPtr<ID3D11Texture2D> backbuf;
    swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuf.GetAddressOf());

    D3D11_TEXTURE2D_DESC bbDesc;
    backbuf->GetDesc(&bbDesc);

    // Recreate the read-copy if the backbuffer dimensions changed
    if (!tempTex || tempDesc.Width != bbDesc.Width || tempDesc.Height != bbDesc.Height)
    {
        tempTex.Reset();
        tempSRV.Reset();

        D3D11_TEXTURE2D_DESC td = bbDesc;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.CPUAccessFlags = 0;
        td.MiscFlags = 0;
        dev->CreateTexture2D(&td, nullptr, tempTex.GetAddressOf());
        dev->CreateShaderResourceView(tempTex.Get(), nullptr, tempSRV.GetAddressOf());
        tempDesc = bbDesc;
    }

    ctx->CopyResource(tempTex.Get(), backbuf.Get());

    ComPtr<ID3D11RenderTargetView> rtv;
    dev->CreateRenderTargetView(backbuf.Get(), nullptr, rtv.GetAddressOf());

    D3D11_VIEWPORT vp = { 0.f, 0.f, (float)bbDesc.Width, (float)bbDesc.Height, 0.f, 1.f };

    // save state
    ID3D11RenderTargetView* oldRTV[8] = {};
    ID3D11DepthStencilView* oldDSV = nullptr;
    ID3D11BlendState* oldBlend = nullptr;
    ID3D11DepthStencilState* oldDSS = nullptr;
    ID3D11RasterizerState* oldRS = nullptr;
    ID3D11VertexShader* oldVS = nullptr;
    ID3D11PixelShader* oldPS = nullptr;
    ID3D11InputLayout* oldIL = nullptr;
    ID3D11Buffer* oldVB[1] = {};
    ID3D11ShaderResourceView* oldSRV[1] = {};
    ID3D11SamplerState* oldSmpl[1] = {};
    D3D11_PRIMITIVE_TOPOLOGY oldTopology;
    D3D11_VIEWPORT           oldVP[1] = {};
    UINT oldBlendMask = 0, oldStencilRef = 0, oldStride = 0, oldOffset = 0, numVP = 1;
    float oldBlendFactor[4] = {};

    ctx->OMGetRenderTargets(8, oldRTV, &oldDSV);
    ctx->OMGetBlendState(&oldBlend, oldBlendFactor, &oldBlendMask);
    ctx->OMGetDepthStencilState(&oldDSS, &oldStencilRef);
    ctx->RSGetState(&oldRS);
    ctx->RSGetViewports(&numVP, oldVP);
    ctx->VSGetShader(&oldVS, nullptr, nullptr);
    ctx->PSGetShader(&oldPS, nullptr, nullptr);
    ctx->IAGetInputLayout(&oldIL);
    ctx->IAGetPrimitiveTopology(&oldTopology);
    ctx->IAGetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);
    ctx->PSGetShaderResources(0, 1, oldSRV);
    ctx->PSGetSamplers(0, 1, oldSmpl);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);
    ctx->PSSetShaderResources(0, 1, tempSRV.GetAddressOf());
    ctx->PSSetSamplers(0, 1, smpl.GetAddressOf());
    ctx->RSSetState(rs.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(dss.Get(), 0);
    ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    ctx->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

    ctx->Draw(3, 0);

    // restore state
    ctx->OMSetRenderTargets(8, oldRTV, oldDSV);
    ctx->OMSetBlendState(oldBlend, oldBlendFactor, oldBlendMask);
    ctx->OMSetDepthStencilState(oldDSS, oldStencilRef);
    ctx->RSSetState(oldRS);
    ctx->RSSetViewports(numVP, oldVP);
    ctx->VSSetShader(oldVS, nullptr, 0);
    ctx->PSSetShader(oldPS, nullptr, 0);
    ctx->IASetInputLayout(oldIL);
    ctx->IASetPrimitiveTopology(oldTopology);
    ctx->IASetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);
    ctx->PSSetShaderResources(0, 1, oldSRV);
    ctx->PSSetSamplers(0, 1, oldSmpl);

    for (auto* r : oldRTV) if (r) r->Release();
    if (oldDSV)     oldDSV->Release();
    if (oldBlend)   oldBlend->Release();
    if (oldDSS)     oldDSS->Release();
    if (oldRS)      oldRS->Release();
    if (oldVS)      oldVS->Release();
    if (oldPS)      oldPS->Release();
    if (oldIL)      oldIL->Release();
    if (oldVB[0])   oldVB[0]->Release();
    if (oldSRV[0])  oldSRV[0]->Release();
    if (oldSmpl[0]) oldSmpl[0]->Release();

    //spdlog::info("[Gamma Correction] did the neediful");
}

void ColorCorrection::ReleaseD3D9()
{
    tempTexD3D9.Reset();
    tempDescD3D9 = {};
}

void ColorCorrection::DrawD3D9(IDirect3DDevice9* device)
{
    if (!bShaderLoadedD3D9)
    {
        return;
    }
    //spdlog::info("drawing");

    if (SQEmuTask_IsPaused())
    {
        //spdlog::info("[Gamma Correction D3D9] paused");
        return;
    }

    ComPtr<IDirect3DSurface9> backSurface;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backSurface.GetAddressOf())))
        return;

    D3DSURFACE_DESC bbDesc;
    backSurface->GetDesc(&bbDesc);

    // Recreate the read-copy if the backbuffer dimensions changed
    if (!tempTexD3D9 || tempDescD3D9.Width != bbDesc.Width || tempDescD3D9.Height != bbDesc.Height)
    {
        tempTexD3D9.Reset();

        HRESULT hr = device->CreateTexture(bbDesc.Width, bbDesc.Height, 1, D3DUSAGE_RENDERTARGET, bbDesc.Format, D3DPOOL_DEFAULT, tempTexD3D9.GetAddressOf(), nullptr);
        if (FAILED(hr))
        {
            spdlog::error("[Gamma Correction D3D9] CreateTexture failed: {:#x}", static_cast<unsigned int>(hr));
            return;
        }

        tempDescD3D9 = bbDesc;
    }

    ComPtr<IDirect3DSurface9> tempSurface;
    if (FAILED(tempTexD3D9->GetSurfaceLevel(0, tempSurface.GetAddressOf())))
        return;

    if (FAILED(device->StretchRect(backSurface.Get(), nullptr, tempSurface.Get(), nullptr, D3DTEXF_NONE)))
        return;

    // save state
    ComPtr<IDirect3DStateBlock9> stateBlock;
    device->CreateStateBlock(D3DSBT_ALL, stateBlock.GetAddressOf());

    device->SetRenderTarget(0, backSurface.Get());
    device->SetPixelShader(psD3D9.Get());
    device->SetVertexShader(nullptr);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    device->SetTexture(0, tempTexD3D9.Get());
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    // offset by -0.5 to land exactly on the backbuffer's texels.
    const float w = static_cast<float>(bbDesc.Width);
    const float h = static_cast<float>(bbDesc.Height);
    VertexD3D9 verts[4] = {
        { -0.5f,    -0.5f,    0.f, 1.f, 0.f, 0.f },
        { w - 0.5f, -0.5f,    0.f, 1.f, 1.f, 0.f },
        { w - 0.5f, h - 0.5f, 0.f, 1.f, 1.f, 1.f },
        { -0.5f,    h - 0.5f, 0.f, 1.f, 0.f, 1.f },
    };

    device->BeginScene();
    device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(VertexD3D9));
    device->EndScene();

    // restore state
    if (stateBlock) stateBlock->Apply();

    //spdlog::info("[Gamma Correction D3D9] did the neediful");
}
