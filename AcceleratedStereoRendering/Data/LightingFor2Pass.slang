/*
  Copyrighted(c) 2020, TH Köln.All rights reserved. Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met :
 
  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
  * Neither the name of TH Köln nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER
  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Authors: Niko Wissmann
 */

import ShaderCommon;
import Shading;
import Utils;

#define LeftEye         0
#define RightEye        1

#define NUM_LIGHTSOURCES 1

cbuffer PerImageCB
{
    uint gStereoTarget;
    float4x4 gLightViewProj;
    float gBias;
    uint gKernelSize;
};

SamplerComparisonState gPCFCompSampler;

ITexture2D gPos;
ITexture2D gNorm;
ITexture2D gDiffuseMatl;
ITexture2D gSpecMatl;

Texture2D gShadowMap;

// PCF Shadow Factor
float getShadowFactor(float3 posW)
{
    float4 pixelLight = mul(float4(posW, 1), gLightViewProj);
    pixelLight /= pixelLight.w;

    float2 texC = float2(pixelLight.x, -pixelLight.y) * 0.5 + 0.5;
    pixelLight.z -= gBias;

    float w, h;
    gShadowMap.GetDimensions(w, h);
    float xOffset = 1.0 / w;
    float yOffset = 1.0 / h;

    float kernelSize = gKernelSize - 1;
    float halfKernelSize = kernelSize / 2.0;

    float factor = 0.0;
    for (float y = -halfKernelSize; y <= halfKernelSize; y += 1.0)
    {
        for (float x = -halfKernelSize; x <= halfKernelSize; x += 1.0)
        {
            float2 offsetTexC = texC + float2(x * xOffset, y * yOffset);
            factor += gShadowMap.SampleCmpLevelZero(gPCFCompSampler, offsetTexC, pixelLight.z);
        }
    }

    return saturate(factor / (kernelSize * kernelSize));
}

float3 shade(float3 camPosW, uint2 pixelPos, float3 posW, float3 normalW, float4 albedo, float4 specular)
{
    if (albedo.a <= 0)
    {
        discard;
    }

    ShadingData sd = reconstructShadingData(camPosW, posW, normalW, albedo, specular);
    float4 shadingColor = float4(0, 0, 0, 1);

    for (uint l = 0; l < gLightsCount; l++)
    {
        float shadowFactor = 1;
        if (l == 0)
        {
            shadowFactor = getShadowFactor(posW);
        }
#ifdef _SHOWBRDF
        shadingColor.rgb = float3(evalMaterial(sd, gLights[l], shadowFactor).specularBrdf);
#else
        shadingColor.rgb += evalMaterial(sd, gLights[l], shadowFactor).color.rgb;
#endif
    }

#ifndef _SHOWBRDF // if not
    shadingColor.rgb += evalMaterial(sd, gLightProbe).color.rgb;
#endif

    return shadingColor.rgb;
}

float4 main(float2 texC : TEXCOORD, float4 pos : SV_Position) : SV_Target0
{
    uint2 pixelPos = (uint2) pos.xy;

    float3 posW = gPos[pixelPos].rgb;
    float3 normalW = gNorm[pixelPos].rgb;
    float4 albedo = gDiffuseMatl[pixelPos];
    float4 specular = gSpecMatl[pixelPos];

    float3 outputColor;
    if(gStereoTarget == LeftEye)
        outputColor = shade(gCamera.posWLeft, pixelPos, posW, normalW, albedo, specular);
    else
        outputColor = shade(gCamera.posWRight, pixelPos, posW, normalW, albedo, specular);

    return float4(outputColor, 1);
}
