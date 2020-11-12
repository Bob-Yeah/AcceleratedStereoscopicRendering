import ShaderCommon;
import DefaultVS;

// Stereo switch flag
#define LeftEye   0
#define RightEye  1

cbuffer PerImageCB
{
    uint gStereoTarget;
};

VertexOut main(VertexIn vIn)
{
    VertexOut vOut;
    float4x4 worldMat = getWorldMat(vIn);
    float4 posW = mul(vIn.pos, worldMat);
    vOut.posW = posW.xyz;
    if(gStereoTarget == LeftEye)
        vOut.posH = mul(posW, gCamera.viewProjMat);
    else
        vOut.posH = mul(posW, gCamera.rightEyeViewProjMat);

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
    vOut.bitangentW = mul(vIn.bitangent, (float3x3)getWorldMat(vIn));
#else
    vOut.bitangentW = 0;
#endif

#ifdef HAS_LIGHTMAP_UV
    vOut.lightmapC = vIn.lightmapC;
#else
    vOut.lightmapC = 0;
#endif

#ifdef HAS_PREV_POSITION
    float4 prevPos = vIn.prevPos;
#else
    float4 prevPos = vIn.pos;
#endif
    float4 prevPosW = mul(prevPos, gPrevWorldMat[vIn.instanceID]);
    if (gStereoTarget == LeftEye)
        vOut.prevPosH = mul(prevPosW, gCamera.prevViewProjMat);
    else
        vOut.prevPosH = mul(prevPosW, gCamera.rightEyePrevViewProjMat);

    return vOut;
}
