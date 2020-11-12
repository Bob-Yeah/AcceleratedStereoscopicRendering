Texture2D gDepthTex;
SamplerState gLinearSampler;

float4 main(float2 texC: TEXCOORD, float4 pos : SV_Position) : SV_TARGET0
{
    float left =  gDepthTex.SampleLevel(gLinearSampler,texC, 0).r;
   
    return float4(left,left,left,1);
}
