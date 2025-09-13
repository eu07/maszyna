#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "view_data.hlsli"
#include "material_common.hlsli"

#define CASCADE_COUNT 8

cbuffer ShadowProjections : register(b11) {
  float4x4 g_CascadeProjections[CASCADE_COUNT];
  float4 g_CascadePlanes[(CASCADE_COUNT + 3) / 4];
}

SamplerComparisonState g_ShadowSampler : register(s11);
Texture2DArray<float> g_ShadowMap : register(t11);

float3 PosToShadow(in float4x4 cascade, in float3 pos) {
  float4 shadow_view = mul(cascade, float4(pos, 1.));
  shadow_view /= shadow_view.w;
  shadow_view.xy = shadow_view.xy * float2(.5, -.5) + .5;
  return shadow_view.xyz;
}

bool CalculateShadowView(in float4x4 cascade, in float plane, in MaterialData data, out float3 shadow_view, out float3 shadow_view_ddx, out float3 shadow_view_ddy) {
#ifdef DEFERRED_LIGHTING_PASS
  if(dot(data.m_Position, data.m_Position) > plane) return false;
  shadow_view = PosToShadow(cascade, data.m_Position);
  shadow_view_ddx = PosToShadow(cascade, data.m_PositionDDX) - shadow_view;
  shadow_view_ddy = PosToShadow(cascade, data.m_PositionDDY) - shadow_view;
  return true;
#else
  if(dot(data.m_Position, data.m_Position) > plane) return false;
  shadow_view = PosToShadow(cascade, data.m_Position);
  //if(any(shadow_view <= 0. || shadow_view >= 1.)) return false;
  //shadow_view_ddx = PosToShadow(cascade, data.m_Position + ddx(data.m_Position)) - shadow_view;
  //shadow_view_ddy = PosToShadow(cascade, data.m_Position + ddy(data.m_Position)) - shadow_view;
  return true;
#endif
}

float GetShadow(in float3 view, in MaterialData data) {
  float shadow = 1.;
  uint3 shadowmap_size;
  g_ShadowMap.GetDimensions(shadowmap_size.x, shadowmap_size.y, shadowmap_size.z);
  for(int i = 0; i < shadowmap_size.z; ++i) {
    float3 shadowView, vShadowTexDDX, vShadowTexDDY;
    if(CalculateShadowView(mul(g_CascadeProjections[i], g_InverseModelView), g_CascadePlanes[i / 4][i % 4], data, shadowView, vShadowTexDDX, vShadowTexDDY)) {
      shadow = 0.;
#ifdef DEFERRED_LIGHTING_PASS
      float2x2 screenToShadow = float2x2(vShadowTexDDX.xy, vShadowTexDDY.xy);
      float fDeterminant = determinant(screenToShadow);
      float fInvDeterminant = 1.0f / fDeterminant;

      float2x2 shadowToScreen = float2x2 (
      screenToShadow._22 * fInvDeterminant,
      screenToShadow._12 * -fInvDeterminant,
      screenToShadow._21 * -fInvDeterminant,
      screenToShadow._11 * fInvDeterminant );
      float2 vRightTexelDepthRatio = mul( float2(1./shadowmap_size.x, 0.),
          shadowToScreen );
          float2 vUpTexelDepthRatio = mul( float2(0., 1./shadowmap_size.y),
          shadowToScreen ); 
      float fUpTexelDepthDelta =
          vUpTexelDepthRatio.x * vShadowTexDDX.z
          + vUpTexelDepthRatio.y * vShadowTexDDY.z;
      float fRightTexelDepthDelta =
          vRightTexelDepthRatio.x * vShadowTexDDX.z
          + vRightTexelDepthRatio.y * vShadowTexDDY.z;
#endif
      [unroll]
      for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
#ifdef DEFERRED_LIGHTING_PASS
          shadow += g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, float3(shadowView.xy, float(i)), shadowView.z + x * fRightTexelDepthDelta + y * fUpTexelDepthDelta, int2(x, y)) / 9.;
#else
          shadow += g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, float3(shadowView.xy, float(i)), shadowView.z, int2(x, y)) / 9.;
#endif
        }
      }
      break;
    }
  }
  return shadow;
}

#endif
