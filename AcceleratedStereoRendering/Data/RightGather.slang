import Shading;
import ShaderCommon;
import DefaultVS;
#define LeftEye      0
#define RightEye     1

cbuffer PerImageCB
{
    uint gStereoTarget;
};

cbuffer PerFrameCB
{
    float4x4 gInvRightEyeView;
    float4x4 gInvRightEyeProj;
    float4x4 gLeftEyeViewProj;
};

struct GatherOut
{
    float4 color            : SV_TARGET0;
    float depthCoLeft        : SV_DEPTH;
};

SamplerState gLinearSampler;
Texture2D gLeftEyeTex;
Texture2D gLeftEyeDepthTex;

GatherOut main(VertexOut vsOut)
{
    GatherOut gOut;
    float z = vsOut.posH.z;

    float4 posLH = mul(float4(vsOut.posW.xyz,1), gLeftEyeViewProj);
    float2 texCLeft = posLH.xy / posLH.w;
    float2 texCLeftF;
    texCLeftF.x = (texCLeft.x + 1)/2;
    texCLeftF.y = (1 - texCLeft.y)/2;
    float leftDepth =  gLeftEyeDepthTex.SampleLevel(gLinearSampler,texCLeftF, 0).r;

    if (leftDepth + 0.00001 < z || abs(texCLeft.x)>1 || abs(texCLeft.y)>1)
    {
        gOut.depthCoLeft = 1.0; // depth fail
        gOut.color = float4(0,0,0,1);
    }
    else{
        gOut.depthCoLeft = z - 0.00001;  // depth pass
        gOut.color = float4(gLeftEyeTex.SampleLevel(gLinearSampler, texCLeftF, 0).rgb, 1);
   }
   return gOut;
}
