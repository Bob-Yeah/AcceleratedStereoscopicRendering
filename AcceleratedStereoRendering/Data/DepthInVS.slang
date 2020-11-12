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
    return vOut;
}
