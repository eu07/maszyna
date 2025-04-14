
#include "color_transform.hlsli"

/* ---------------------------------------------------------------------------------------------- */
/*                                        Resource bindings                                       */
/* ---------------------------------------------------------------------------------------------- */

Texture2D<float4> g_ColorImage : register(t0);

globallycoherent RWTexture2D<float2> g_AutoExposureTexture : register(u0);

globallycoherent RWStructuredBuffer<uint> g_SpdAtomicCounter : register(u2);
globallycoherent RWTexture2D<float> g_ImgMip6 : register(u3);

/* --------------------------------------- Push constants --------------------------------------- */

struct PushConstantsCompLuminance {
  uint m_num_mips;
  uint m_num_workgroups;
  float m_delta_time;
  float m_min_luminance_ev;
  float m_max_luminance_ev;
};

#ifdef SPIRV
[[vk::push_constant]] ConstantBuffer<PushConstantsCompLuminance> g_PushConstants;
#else
cbuffer g_Const : register(b1) { PushConstantsCompLuminance g_PushConstants; }
#endif

/* ---------------------------------------------------------------------------------------------- */
/*                                       Function interface                                       */
/* ---------------------------------------------------------------------------------------------- */

uint GetNumMips() {
  return g_PushConstants.m_num_mips;
}

uint GetNumWorkGroups() {
  return g_PushConstants.m_num_workgroups;
}

float GetDeltaTime() {
  return g_PushConstants.m_delta_time;
}

float ComputeAutoExposureFromLavg(float l_avg) {
    l_avg = exp(l_avg);

    const float s = 100.; //ISO arithmetic speed
    const float k = 12.5;
    float exposure_iso100 = log2((l_avg * s) / k);

    const float q = .65;
    float l_max = (78. / (q * s)) * pow(2., exposure_iso100);

    return 1. / l_max;
}

void OutputLuminance(float4 out_value) {
  float prev = g_AutoExposureTexture[uint2(0, 0)].y;
  float result = out_value.r;

  result = clamp(
    result,
    log(.125 * exp2(g_PushConstants.m_min_luminance_ev)),
    log(.125 * exp2(g_PushConstants.m_max_luminance_ev)));

  if (prev < 1.e8) // Compare Lavg, so small or negative values
  {
      result = prev + (result - prev) * (1. - exp(-GetDeltaTime()));
  }
  g_AutoExposureTexture[uint2(0, 0)] = float2(ComputeAutoExposureFromLavg(result), result);
}

/* ---------------------------------------------------------------------------------------------- */
/*                                       SPD implementation                                       */
/* ---------------------------------------------------------------------------------------------- */

#define A_GPU
#define A_HLSL
#include "amd/ffx_a.h"

/* ---------------------------------------- Shared memory --------------------------------------- */

groupshared AU1 g_SpdCounter;
groupshared AF4 g_SpdIntermediate[16][16];

/* ------------------------------------ Image load-store ops ------------------------------------ */

AF4 SpdLoadSourceImage(ASU2 p, AU1 slice){
  return AF4(log(max(1.e-6, XYZ_to_Luma(g_ColorImage[p].rgb))), 0., 0., 0.);
}

AF4 SpdLoad(ASU2 p, AU1 slice) {
  return float4(g_ImgMip6[p], 0., 0., 0.);
}

void SpdStore(ASU2 p, AF4 value, AU1 mip, AU1 slice) {
  if(mip == 5) {
    g_ImgMip6[p].r = value;
  }
  else if(mip == GetNumMips() - 1 && all(p == ASU2(0, 0))) {
    OutputLuminance(value);
  }
}

/* -------------------------------- Shared memory load-store ops -------------------------------- */

AF4 SpdLoadIntermediate(ASU1 x, ASU1 y) {
  return g_SpdIntermediate[x][y];
}

void SpdStoreIntermediate(ASU1 x, ASU1 y, AF4 value) {
  g_SpdIntermediate[x][y] = value;
}

/* ------------------------------------- Atomic counter ops ------------------------------------- */

void SpdIncreaseAtomicCounter(AU1 slice) {
  InterlockedAdd(g_SpdAtomicCounter[0], 1, g_SpdCounter);
}

AU1 SpdGetAtomicCounter() {
  return g_SpdCounter;
}

void SpdResetAtomicCounter(AU1 slice) {
  g_SpdAtomicCounter[0] = 0;
}

/* ------------------------------------- Reduction function ------------------------------------- */

AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3) {
  return (v0 + v1 + v2 + v3) * .25;
}

/* ----------------------------------------- SPD wrapper ---------------------------------------- */

#include "amd/ffx_spd.h"

[numthreads(256,1,1)]
void CS_ComputeAvgLuminance(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex) {
  SpdDownsample( 
    WorkGroupId.xy,
    LocalThreadIndex,
    GetNumMips(), 
    GetNumWorkGroups(),
    0);
};
