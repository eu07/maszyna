Texture2D<float> g_DepthTexture : register(t0);
RWTexture2D<float> g_Output : register(u0);
sampler g_SamplerLinearClamp : register(s0);

// From https://www.shadertoy.com/view/3tB3z3 - except we're using R2 here
#define XE_HILBERT_LEVEL    6U
#define XE_HILBERT_WIDTH    ( (1U << XE_HILBERT_LEVEL) )
#define XE_HILBERT_AREA     ( XE_HILBERT_WIDTH * XE_HILBERT_WIDTH )
inline uint HilbertIndex( uint posX, uint posY )
{   
    uint index = 0U;
    for( uint curLevel = XE_HILBERT_WIDTH/2U; curLevel > 0U; curLevel /= 2U )
    {
        uint regionX = ( posX & curLevel ) > 0U;
        uint regionY = ( posY & curLevel ) > 0U;
        index += curLevel * curLevel * ( (3U * regionX) ^ regionY);
        if( regionY == 0U )
        {
            if( regionX == 1U )
            {
                posX = uint( (XE_HILBERT_WIDTH - 1U) ) - posX;
                posY = uint( (XE_HILBERT_WIDTH - 1U) ) - posY;
            }

            uint temp = posX;
            posX = posY;
            posY = temp;
        }
    }
    return index;
}

// Engine-specific screen & temporal noise loader
float2 SpatioTemporalNoise( uint2 pixCoord, uint temporalIndex )    // without TAA, temporalIndex is always 0
{
    float2 noise;
#if 1   // Hilbert curve driving R2 (see https://www.shadertoy.com/view/3tB3z3)
    #ifdef XE_GTAO_HILBERT_LUT_AVAILABLE // load from lookup texture...
        uint index = g_srcHilbertLUT.Load( uint3( pixCoord % 64, 0 ) ).x;
    #else // ...or generate in-place?
        uint index = HilbertIndex( pixCoord.x, pixCoord.y );
    #endif
    index += 288*(temporalIndex%64); // why 288? tried out a few and that's the best so far (with XE_HILBERT_LEVEL 6U) - but there's probably better :)
    // R2 sequence - see http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    return float2( frac( 0.5 + index * float2(0.75487766624669276005, 0.5698402909980532659114) ) );
#else   // Pseudo-random (fastest but looks bad - not a good choice)
    uint baseHash = Hash32( pixCoord.x + (pixCoord.y << 15) );
    baseHash = Hash32Combine( baseHash, temporalIndex );
    return float2( Hash32ToFloat( baseHash ), Hash32ToFloat( Hash32( baseHash ) ) );
#endif
}

cbuffer DrawConstants : register(b0) {
  float4x4 g_Projection;
  float4x4 g_InverseProjection;
  float3 g_LightDirView;
  float g_NumSamples;
  float g_SampleRange;
  float g_Thickness;
  uint g_FrameIndex;
}

uint2 NdcToPixel(in float2 size, in float4 ndc);
float2 NdcToUv(in float4 ndc);
float4 PixelToNdc(in float2 size, in uint2 pixel, in float depth);
float4 UvToNdc(in float2 uv, in float depth);
uint2 ViewToPixel(in float2 size, in float3 view);
float2 ViewToUv(in float3 view);
float GetPixelDepth(in float2 size, in uint2 pixel);
float GetUvDepth(in float2 uv);
float3 PixelToViewWithDepth(in float2 size, in uint2 pixel);
float3 UvToViewWithDepth(in float2 uv);

[numthreads(8, 8, 1)]
void main(uint3 PixCoord : SV_DispatchThreadID) {
  float2 size;
  g_DepthTexture.GetDimensions(size.x, size.y);
  uint2 pixel = PixCoord.xy;
  
  float occlusion = 0;
  g_Output[pixel] = 1.;
  
  float2 noise = SpatioTemporalNoise(PixCoord.xy, g_FrameIndex);
  float3 viewPosition = PixelToViewWithDepth(size, pixel);
  float3 sample = viewPosition;
  float3 step = g_LightDirView * g_SampleRange / g_NumSamples;
  sample += noise.x * step;
  for(int i = 0; i < g_NumSamples; ++i) {
    sample += step;
    float2 sample_uv = ViewToUv(sample);
    if(all(and(sample_uv > 0, sample_uv < 1.))) {
      float depthDelta = sample.z * .998 - GetUvDepth(sample_uv);
      if(depthDelta < 0. && depthDelta > -g_Thickness) {
        occlusion = 1.;
        break;
      }
    }
  }
  g_Output[pixel] = 1. - occlusion;
}

uint2 NdcToPixel(in float2 size, in float4 ndc) {
  return (uint2)floor(NdcToUv(ndc) * size);
}

float2 NdcToUv(in float4 ndc) {
  return ndc.xy * float2(.5, -.5) + .5;
}

float4 PixelToNdc(in float2 size, in uint2 pixel, in float ndc_depth) {
  return float4(float2(2., -2.) * ((((float2)pixel + .5) / size) - .5), ndc_depth, 1.);
}

float4 UvToNdc(in float2 uv, in float ndc_depth) {
  return float4(float2(2., -2.) * (uv - .5), ndc_depth, 1.);
}

uint2 ViewToPixel(in float2 size, in float3 view) {
  float4 ndc = mul(g_Projection, float4(view, 1.));
  ndc /= ndc.w;
  return NdcToPixel(size, ndc);
}

float2 ViewToUv(in float3 view) {
  float4 ndc = mul(g_Projection, float4(view, 1.));
  ndc /= ndc.w;
  return NdcToUv(ndc);
}

float GetPixelDepth(in float2 size, in uint2 pixel) {
  return PixelToViewWithDepth(size, pixel).z;
}

float GetUvDepth(in float2 uv) {
  return UvToViewWithDepth(uv).z;
}

float3 PixelToViewWithDepth(in float2 size, in uint2 pixel) {
  float4 ndc = PixelToNdc(size, pixel, g_DepthTexture[pixel]);
  ndc = mul(g_InverseProjection, ndc);
  return ndc.xyz / ndc.w;
}

float3 UvToViewWithDepth(in float2 uv) {
  float4 ndc = UvToNdc(uv, g_DepthTexture.SampleLevel(g_SamplerLinearClamp, uv, 0.));
  ndc = mul(g_InverseProjection, ndc);
  return ndc.xyz / ndc.w;
}

