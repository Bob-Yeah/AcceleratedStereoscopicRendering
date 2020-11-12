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
    float4x4 gRightEyeViewProj;
    float2 gViewportDims;
};

struct GatherOut
{
    float4 color            : SV_TARGET0;
    float depthCoLeft        : SV_DEPTH;
};

SamplerState gLinearSampler;
SamplerComparisonState gDepthCompSampler;
Texture2D gLeftEyeTex;
Texture2D gLeftEyeDepthTex;

GatherOut main(VertexOut vsOut, float4 pos : SV_Position)
{
    GatherOut gOut;
    float4 rightPosH = mul(float4(vsOut.posW.xyz,1), gRightEyeViewProj);
    float z = rightPosH.z / rightPosH.w;  // 0 - 1, 按理来说是right eye正常depth map的数值，在近平面为0， 远平面为1
    float2 xy = rightPosH.xy / rightPosH.w;
    
    float4 posLH = mul(float4(vsOut.posW.xyz,1), gLeftEyeViewProj);
    float2 texCLeft = posLH.xy / posLH.w;
    float2 texCLeftF;
    texCLeftF.x = (texCLeft.x + 1)/2;
    texCLeftF.y = (1 - texCLeft.y)/2;
    float leftDepth =  gLeftEyeDepthTex.SampleLevel(gLinearSampler,texCLeftF, 0).r;

    //ShadingData sd;
    //if(gStereoTarget == LeftEye)
    //    sd = prepareShadingData(vsOut, gMaterial, gCamera.posWLeft);
    //else
    //    sd = prepareShadingData(vsOut, gMaterial, gCamera.posWRight);

    if (leftDepth + 0.00001 < z || abs(texCLeft.x)>1 || abs(texCLeft.y)>1)
    {
        gOut.depthCoLeft = 1.0; // depth fail
        gOut.color = float4(0,0,0,1);
    }
    else{
        gOut.depthCoLeft = z - 0.00001;  // depth pass
        gOut.color = float4(gLeftEyeTex.SampleLevel(gLinearSampler, texCLeftF, 0).rgb, 1);
   }
   //gOut.posW           = float4(sd.posW, 1.f);
   //gOut.normW          = float4(sd.N, 0.f);
   //gOut.diffuseOpacity = float4(sd.diffuse, sd.opacity);
   //gOut.specRough      = float4(sd.specular, sd.linearRoughness);
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
