__import ShaderCommon;
__import DefaultVS;

VertexOut main(VertexIn vIn)
{
    VertexOut vOut;

    // Filled out in geometry shader
    vOut.posH = float4(0.0f, 0.0f, 0.0f, 0.0f);
    vOut.prevPosH = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4x4 worldMat = getWorldMat(vIn);
    float4 posW = mul(vIn.pos, worldMat);
    vOut.posW = posW.xyz;

#ifdef HAS_TEXCRD
    vOut.texC = vIn.texC;
#else
    vOut.texC = 0;
#endif

#ifdef HAS_COLORS
    vOut.colorV = vIn.color;
#else
    vOut.colorV = 0;
#endif

#ifdef HAS_NORMAL
    vOut.normalW = mul(vIn.normal, getWorldInvTransposeMat(vIn)).xyz;
#else
    vOut.normalW = 0;
#endif

#ifdef HAS_BITANGENT
    vOut.bitangentW = mul(vIn.bitangent, (float3x3)getWorldMat(vIn)).xyz;
#else
    vOut.bitangentW = 0;
#endif

#ifdef HAS_LIGHTMAP_UV
    vOut.lightmapC = vIn.lightmapC;
#else
    vOut.lightmapC = 0;
#endif

    return vOut;
}
