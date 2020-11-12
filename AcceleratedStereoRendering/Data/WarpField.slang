cbuffer PerImageCB
{
    float gNearZ;
    float gFarZ;
    //float2 gViewportDims;
    float ipd;
};

Texture2D gDepthTex;
Texture2D gLeftTex;
//SamplerState gLinearSampler;

float linearDepth(float depth)
{
    return gNearZ / (gFarZ - depth * (gFarZ - gNearZ)) * gFarZ;
}


float4 main(float2 texC : TEXCOORD, float4 pos : SV_Position) : SV_Target0
{
    uint2 pixelPos = (uint2) pos.xy;
    float depthXs = gDepthTex[pixelPos].r;
    //float2 Vxs;
    //float u = ipd;
    //float v = 0;
    //Vxs.x = -u * gCamera.projMat[0][0] / linearDepth(depthXs);
    //Vxs.y = -v * gCamera.projMat[1][1] / linearDepth(depthXs);
    //float2 offsetTexC = texC + Vxs;
    //float4 color = float4(gLeftTex.SampleLevel(gLinearSampler,offsetTexC, 0).rgb, 1);
    //return color;
    return float4(depthXs,depthXs,depthXs,1);
}
