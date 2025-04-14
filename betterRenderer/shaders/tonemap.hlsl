#include "manul/color_transform.hlsli"

#define MA_TONEMAP_SRGB_REC709 0
#define MA_TONEMAP_LINEAR_REC2020 1

struct TonemapConstants {
  int m_TonemapFunction;
  float m_SceneExposure;
  float m_SceneNits;
  float m_SceneGamma;
};

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<TonemapConstants> g_TonemapConstants;

#else

cbuffer g_Const : register(b0) { TonemapConstants g_TonemapConstants; }

#endif

struct VertexOutput {
  float2 m_Position : Position;
  float4 m_PositionSV : SV_Position;
};

struct PixelOutput {
  float4 m_Output : SV_Target0;
};

sampler source_sampler : register(s0);
Texture2D source : register(t0);
Texture2D<float3> noise : register(t1);

float3 ACESFilm(float3 x)
{
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return (x*(a*x+b))/(x*(c*x+d)+e);
}

float luminance(float3 v)
{
    return dot(v, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 reinhard_jodie(float3 v)
{
    float l = luminance(v);
    float3 tv = v / (1.0f + v);
    return lerp(v / (1.0f + l), tv, tv);
}

float3 reinhard_extended_luminance(float3 v, float max_white_l)
{
    float l_old = luminance(v);
    float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0f + l_old);
    return v * l_new / l_old;
}

float3 ApplySRGBCurve( float3 x )
{
    // Approximately pow(x, 1.0 / 2.2)
    return select(x < 0.0031308, 12.92 * x, 1.055 * pow(x, 1.0 / 2.4) - 0.055);
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

float3 filmicF(float3 x)
{
	float A = 0.22f;
	float B = 0.30f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.01f;
	float F = 0.30f;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
}

float3 filmic(float3 x)
{
	return filmicF(x) / filmicF(11.2);
}

static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    //color = saturate(color);

    return color;
}

PixelOutput main(VertexOutput ps_in) {
  PixelOutput result;
  //result.m_Output.rgb = pow(ACESFilm(source.Sample(source_sampler, ps_in.m_Position).rgb), 1./2.2);
  result.m_Output.rgb = source[(uint2)ps_in.m_PositionSV.xy].rgb;
  result.m_Output.rgb = pow(result.m_Output.rgb*g_TonemapConstants.m_SceneExposure, g_TonemapConstants.m_SceneGamma);
  switch(g_TonemapConstants.m_TonemapFunction) {
    case MA_TONEMAP_SRGB_REC709:
    {
      uint2 noise_size;
      noise.GetDimensions(noise_size.x, noise_size.y);
      float3 noise_sample = noise[ps_in.m_PositionSV.xy % noise_size];

      result.m_Output.rgb = XYZ_to_REC709(result.m_Output.rgb);
      result.m_Output.rgb = saturate(reinhard_jodie(result.m_Output.rgb));
      //result.m_Output.rgb = saturate(XYZ_to_REC2020(result.m_Output.rgb));
      ApplySRGBCurve(result.m_Output.rgb);

      result.m_Output.rgb = floor(result.m_Output.rgb * 256. + noise_sample) / 255.;
    }
    break;
    case MA_TONEMAP_LINEAR_REC2020:
    {
      const float st2084max = 10000.;
      const float hdrScalar = g_TonemapConstants.m_SceneNits / st2084max;
      //result.m_Output.rgb = XYZ_to_REC2020(result.m_Output.rgb);
      //result.m_Output.rgb = ACESFilm(result.m_Output.rgb);
      //const float3  LuminanceWeights = float3(0.299,0.587,0.114);
      //float luminance = dot(LuminanceWeights, result.m_Output.rgb);
      //result.m_Output.rgb = lerp(luminance.xxx, result.m_Output.rgb, 2.2);
      result.m_Output.rgb = XYZ_to_REC709(result.m_Output.rgb);
      result.m_Output.rgb = ACESFilm(result.m_Output.rgb);
      result.m_Output.rgb = Rec709ToRec2020(result.m_Output.rgb);
      result.m_Output.rgb = saturate(result.m_Output.rgb);
      result.m_Output.rgb *= hdrScalar;
      result.m_Output.rgb = LinearToST2084(result.m_Output.rgb);
    }
    break;
  }
  result.m_Output.a = 1.;
  return result;
}
