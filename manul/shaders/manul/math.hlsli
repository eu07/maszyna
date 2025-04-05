#ifndef MATH_HLSLI
#define MATH_HLSLI

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define PI_OVER_TWO 1.5707963267948966192313216916398
#define PI_OVER_THREE 1.0471975511965977461542144610932
#define PI_OVER_SIX 0.52359877559829887307710723054658
#define ONE_OVER_PI 0.31830988618379067153776752674503
#define TWO_OVER_PI 0.63661977236758134307553505349006
#define ONE_OVER_TWO_PI 0.15915494309189533576888376337251

float3 UnpackNormalXY(in float2 normal) {
  float3 result;
  result.xy = 2. * (normal - .5);
  result.z = sqrt(saturate(1. - dot(result.xy, result.xy)));
  return result;
}

float3 UnpackNormalXYZ(in float3 normal) {
  return normalize(normal - .5);
}

float InvLerp(float a, float b, float v) {
    return (v - a) / (b - a);
}

#endif
