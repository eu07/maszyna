/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#define MA_TONEMAP_SRGB_REC709 0
#define MA_TONEMAP_LINEAR_REC2020 1

struct Constants
{
    float2 invDisplaySize;
    float outputNits;
    int m_TonemapFunction;
};

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<Constants> g_Const;

#else

cbuffer g_Const : register(b0) { Constants g_Const; }

#endif

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

float3 LinearToSRGB(float3 color)
{
    // Approximately pow(color, 1.0 / 2.2)
    return select(color < 0.0031308, 12.92 * color, 1.055 * pow(abs(color), 1.0 / 2.4) - 0.055);
}

float3 SRGBToLinear(float3 color)
{
    // Approximately pow(color, 2.2)
    return select(color < 0.04045, color / 12.92, pow(abs(color + 0.055) / 1.055, 2.4));
}

float3 Rec709ToRec2020(float3 color)
{
    static const float3x3 conversion =
    {
        0.627402, 0.329292, 0.043306,
        0.069095, 0.919544, 0.011360,
        0.016394, 0.088028, 0.895578
    };
    return mul(conversion, color);
}

float3 LinearToST2084(float3 color)
{
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;
    float3 cp = pow(abs(color), m1);
    return pow((c1 + c2 * cp) / (1 + c3 * cp), m2);
}

sampler sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 main(PS_INPUT input) : SV_Target
{
    float4 tex_col = texture0.Sample(sampler0, input.uv);
    float4 input_col = input.col;
    switch(g_Const.m_TonemapFunction) {
        case MA_TONEMAP_LINEAR_REC2020:
        {
            input_col.rgb = SRGBToLinear(input_col.rgb);
            float4 out_col = input_col * tex_col;
            const float st2084max = 10000.;
            const float hdrScalar = g_Const.outputNits / st2084max;
            //out_col.rgb = Rec709ToRec2020(out_col.rgb);
            out_col.rgb *= hdrScalar;
            out_col.rgb = LinearToST2084(out_col.rgb);
            return out_col;
        }
        default:
        {
            tex_col.rgb = LinearToSRGB(tex_col.rgb);
            return input_col * tex_col;
        }
    }
}
