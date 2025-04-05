RWTexture2D<float4> g_ColorImage : register(u0);
Texture2D<float2> g_AutoExposureTexture : register(t0);

[numthreads(8, 8, 1)]
void CS_ApplyAutoExposure(uint3 PixCoord : SV_DispatchThreadID) {
  g_ColorImage[PixCoord.xy] = max(1.e-6, g_ColorImage[PixCoord.xy] * pow(2., g_AutoExposureTexture[uint2(0, 0)].x));
}
