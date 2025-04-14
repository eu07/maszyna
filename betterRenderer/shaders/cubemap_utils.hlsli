#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define ONE_OVER_PI 0.31830988618379067153776752674503
#define TWO_OVER_PI 0.63661977236758134307553505349006
#define ONE_OVER_TWO_PI 0.15915494309189533576888376337251

float3 CalcNormal(in uint3 PixCoord);

float2 EquirectFromNormal(in float3 Normal);

float2 PixCoordToFloat(in uint2 Coord, in uint2 Size);

float2 PixCoordToFloat(in uint2 Coord, in uint2 Size) {
  return ((float2)Coord + .5.xx) / (float2)Size;
}

float3 CalcNormal(in uint3 PixCoord) {
  static const float3x3 FaceTransform[6] = {
    // +X
    float3x3(  0.,  0., -1.,
               0., -1.,  0., 
               1.,  0.,  0. ),
    // -X
    float3x3(  0.,  0.,  1.,
               0., -1.,  0., 
              -1.,  0.,  0. ),
    // +Y
    float3x3(  1.,  0.,  0.,
               0.,  0.,  1., 
               0.,  1.,  0. ),
    // -Y
    float3x3(  1.,  0.,  0.,
               0.,  0., -1., 
               0., -1.,  0. ),
    // +Z
    float3x3(  1.,  0.,  0.,
               0., -1.,  0., 
               0.,  0.,  1. ),
    // -Z
    float3x3( -1.,  0.,  0.,
               0., -1.,  0., 
               0.,  0., -1. )
  };
  uint2 FaceSize;
  uint Elements;
  g_OutCubemap.GetDimensions(FaceSize.x, FaceSize.y, Elements);
  return normalize(mul(float3(PixCoordToFloat(PixCoord.xy, FaceSize) * 2. - 1., 1.), FaceTransform[PixCoord.z]));
}

float2 EquirectFromNormal(in float3 Normal) {
  return float2(atan2(Normal.x, Normal.z) * ONE_OVER_TWO_PI, -asin(Normal.y) * ONE_OVER_PI) + .5.xx;
}