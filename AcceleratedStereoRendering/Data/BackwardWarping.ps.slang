cbuffer PerImageCB
{
    float gDelta;
    int gIterNum;
};

SamplerState gLinearSampler;
Texture2D gLeftEyeTex;
Texture2D gVs;

float4 main(float2 texC : TEXCOORD, float4 pos : SV_Position) : SV_Target0
{
    //#1 Xs1 <- Xw + delta , G(Xs) = Xw - V(Xs)
    float2 Xs = texC;
    Xs.x = clamp(Xs.x + gDelta,0,1);
    float2 VXs;
    float2 GXs;
    float loss;

    for(uint i = 0; i<gIterNum;i++){
        VXs = gVs.SampleLevel(gLinearSampler,Xs, 0).rg;
        GXs = texC - VXs;
        loss = length(Xs-GXs);
        Xs = GXs;
    }

    if (loss > 0.001){
        return float4(0,0,0,0);
    }
    else{
        float4 color = float4(gLeftEyeTex.SampleLevel(gLinearSampler, Xs, 0).rgb, 1);
        return color;
    }
    
}
