Texture2D sourceTex : register(t0);
SamplerState samLinear : register(s0);

float4 main(float2 uv : TEXCOORD) : SV_Target
{
    return sourceTex.Sample(samLinear, uv);
}
