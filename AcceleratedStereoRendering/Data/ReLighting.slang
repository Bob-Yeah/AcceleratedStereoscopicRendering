//import ShaderCommon;
//import Shading;
//import Utils;
//
//#define LeftEye         0
//#define RightEye        1
//
//#define NUM_LIGHTSOURCES 1
//
//cbuffer PerImageCB
//{
//    uint gStereoTarget;
//    float4x4 gLightViewProj;
//    float gBias;
//    uint gKernelSize;
//};
//
//SamplerComparisonState gPCFCompSampler;
//
//ITexture2D gPos;
//ITexture2D gNorm;
//ITexture2D gDiffuseMatl;
//ITexture2D gSpecMatl;

Texture2D gShadowMap;


float4 main(float2 texC : TEXCOORD, float4 pos : SV_Position) : SV_Target0
{
    uint2 pixelPos = (uint2) pos.xy;

    float3 posW = gShadowMap[pixelPos].rgb;

    return float4(posW, 1);
}
