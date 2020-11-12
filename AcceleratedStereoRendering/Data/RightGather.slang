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
    float4x4 gLeftEyeViewProj;
};

struct GatherOut
{
    float4 color            : SV_TARGET0;
    float4 posW             : SV_TARGET1;
    float4 normW            : SV_TARGET2;
    float4 diffuseOpacity   : SV_TARGET3;
    float4 specRough        : SV_TARGET4;
    //float depthCoLeft        : SV_DEPTH;
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
        ShadingData sd;
        if(gStereoTarget == LeftEye)
            sd = prepareShadingData(vsOut, gMaterial, gCamera.posWLeft);
        else
            sd = prepareShadingData(vsOut, gMaterial, gCamera.posWRight);
        //gOut.depthCoLeft = z; // depth fail
        gOut.color = 0;
        gOut.posW           = float4(sd.posW, 1.f);
        gOut.normW          = float4(sd.N, 0.f);
        gOut.diffuseOpacity = float4(sd.diffuse, sd.opacity);
        gOut.specRough      = float4(sd.specular, sd.linearRoughness);
    }
    else{
        //gOut.depthCoLeft = z;  // depth pass
        gOut.color = float4(gLeftEyeTex.SampleLevel(gLinearSampler, texCLeftF, 0).rgb, 1);
        gOut.posW           = 0;
        gOut.normW          = 0;
        gOut.diffuseOpacity = 0;
        gOut.specRough      = 0;
   }
   return gOut;
}

//GatherOut main(float2 texC: TEXCOORD, float4 pos : SV_Position)
//{
//    GatherOut gOut;
//
//    float z = clamp(gRightEyeDepthTex.SampleLevel(gLinearSampler, texC.xy, 0).r, 0.000001, 0.99999);
//
//    float4 posWH = mul(float4(texC.x*2-1, 1-texC.y*2, z, 1), gInvRightEyeProj);
//    posWH /= posWH.w;
//    float4 posW = mul(posWH, gInvRightEyeView);
//    float4 posH = mul(posW, gLeftEyeViewProj);
//    float2 texCLeft = posH.xy / posH.w;
//    float2 texCLeftF;
//    texCLeftF.x = (texCLeft.x + 1)/2;
//    texCLeftF.y = (1 - texCLeft.y)/2;
//
//    float leftDepth =  gLeftEyeDepthTex.SampleLevel(gLinearSampler,texCLeftF, 0).r;
//    float percentage = gLeftEyeDepthTex.SampleCmpLevelZero(gDepthCompSampler,texCLeftF, z);
//
//    if (leftDepth < z - 0.001 || abs(texCLeft.x)>1)
//    {
//        gOut.depthCoLeft = 1;
//        //gOut.stencilCoLeft = 0;
//        gOut.color = float4(0,0,0,1);
//    }
//    else{
//        gOut.depthCoLeft = leftDepth - 0.0001;
//        //gOut.stencilCoLeft = 1;
//        gOut.color = float4(gLeftEyeTex.SampleLevel(gLinearSampler, texCLeftF, 0).rgb, 1);
//    }
//    return gOut;
//}
