struct VS_OUT {
    float2 uv  : TEXCOORD0;
    float4 pos : SV_POSITION;
};

VS_OUT main(uint vertexId : SV_VertexID)
{
    float2 pos[3] = {
        float2(-1.0f, -1.0f),
        float2(-1.0f,  3.0f),
        float2( 3.0f, -1.0f)
    };

    float2 uv[3] = {
        float2(0.0f, 0.0f),
        float2(0.0f, 2.0f),
        float2(2.0f, 0.0f)
    };

    VS_OUT output;
    output.uv  = uv[vertexId];
    output.uv  = float2(output.uv.x, 1.0f - output.uv.y);
    output.pos = float4(pos[vertexId], 0.0f, 1.0f);
    return output;
}
