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

cbuffer PerImageCB
{
    float2 gDepthRange;
    uint gDebugMode;
    uint gStereoOn;
    float gZ0;
};

#define Pos         1
#define Normals     2
#define DiffMat     3
#define SpecMat     4
#define LinearRough 5

float linearizeDepth(float z)
{
    float zNear = gDepthRange.x;
    float zFar = gDepthRange.y;
    float depth = z;
    return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

float3 shade(float3 posW, float3 normalW, float linearRoughness, float4 albedo, float3 spec, float z)
{
    // Discard empty pixels
    if (albedo.a <= 0)
    {
        discard;
    }

    float z_lin = linearizeDepth(z);
    float3 result;

    if (gDebugMode == Pos)
        result = posW;
    else if (gDebugMode == Normals)
        result = 0.5 * normalW + 0.5f;
    else if (gDebugMode == DiffMat)
        result = albedo.rgb;
    else if (gDebugMode == SpecMat)
        result = spec;
    else if (gDebugMode == LinearRough)
        result = float3(linearRoughness);
    else
    {
        result = float3(z_lin);
    }

    if (gStereoOn == 1 && z_lin > gZ0 / gDepthRange.y && z_lin < (gZ0 + 0.1f) / gDepthRange.y)
        result = float3(1);

    return result;
}

ITexture2D gPos;
ITexture2D gNorm;
ITexture2D gDiffuseMatl;
ITexture2D gSpecMatl;
ITexture2D depthBuf;

float4 main(float2 texC : TEXCOORD, float4 pos : SV_Position) : SV_Target0
{
    uint2 pixelPos = (uint2) pos.xy;

    float3 posW = gPos[pixelPos].rgb;
    float3 normalW = gNorm[pixelPos].rgb;
    float4 albedo = gDiffuseMatl[pixelPos];
    float4 spec = gSpecMatl[pixelPos];
    float z = depthBuf[pixelPos].r;

    float3 color = shade(posW, normalW, spec.a, albedo, spec.rgb, z);

    return float4(color, 1);
}
