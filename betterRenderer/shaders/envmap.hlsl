
RWTexture2DArray<float4> g_OutCubemap : register(u0);
RWTexture2D<float2> g_OutBRDF : register(u1);

Texture2D<float3> g_SourceEquirect : register(t0);
TextureCube<float3> g_SourceCube : register(t1);
Texture1D<float2> g_SampleKernel : register(t2);
SamplerState g_SamplerLinearClampV : register(s0);
SamplerState g_SamplerLinearClamp : register(s1);

#include "cubemap_utils.hlsli"

float DistributionGGX(float NdotH, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float roughness, float NdotV, float NdotL);
float3 ImportanceSampleGGX(in float2 Xi,in float Roughness , in float3 N);
void ImportanceSampleCosDir(in float2 u, in float3 N, out float3 L, out float NdotL, out float pdf);

float2 GetSample(uint sample, uint sampleCount);

struct FilterParameters {
  float m_PreExposureMul;
  float m_Roughness;
  float m_SampleCount;
  float m_SourceWidth;
  float m_MipBias;
  uint3 m_Offset;
};

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<FilterParameters> g_FilterParams;

#else

cbuffer g_Const : register(b0) { FilterParameters g_FilterParams; }

#endif

#include "manul/sky.hlsli"

[numthreads(32, 32, 1)]
void CSSampleEquirectangular(uint3 PixCoord : SV_DispatchThreadID) {
  //Sky(g_OutCubemap[PixCoord + g_FilterParams.m_Offset], CalcNormal(PixCoord + g_FilterParams.m_Offset), normalize(float3(0., .1, 1.)), 0.);
  g_OutCubemap[PixCoord + g_FilterParams.m_Offset].rgb = clamp(g_SourceEquirect.SampleLevel(g_SamplerLinearClampV, EquirectFromNormal(CalcNormal(PixCoord)), 0.) * g_FilterParams.m_PreExposureMul, 0., 65535.);
}

[numthreads(32, 32, 1)]
void CSDiffuseIBL(uint3 PixCoord : SV_DispatchThreadID) {
  float3 N = CalcNormal(PixCoord + g_FilterParams.m_Offset);
  
  float sampleCount = g_FilterParams.m_SampleCount;
  float width = g_FilterParams.m_SourceWidth;
  float mipBias = g_FilterParams.m_MipBias;

  float3 accBrdf = 0;
  for ( uint i =0; i < sampleCount ; ++ i ) {
    float2 eta = GetSample (i , sampleCount ) ;
    float3 L ;
    float NdotL ;
    float pdf ;
    ImportanceSampleCosDir ( eta , N , L , NdotL , pdf ) ;
    if ( NdotL >0) {
      float omegaS = 1. / ( sampleCount * pdf );
      float omegaP = 4. * PI / (6. * width * width );
      float mipLevel = max(0., mipBias + .5 * log2 ( omegaS / omegaP ));
      accBrdf += g_SourceCube . SampleLevel ( g_SamplerLinearClamp , L, mipLevel ) ;
    }
  }
  g_OutCubemap[PixCoord + g_FilterParams.m_Offset].rgb = accBrdf * (1. / sampleCount);
}

[numthreads(32, 32, 1)]
void CSSpecularIBL(uint3 PixCoord : SV_DispatchThreadID) {
  float3 N = CalcNormal(PixCoord + g_FilterParams.m_Offset);
  
  float roughness = g_FilterParams.m_Roughness;
  float width = g_FilterParams.m_SourceWidth;
  float sampleCount = g_FilterParams.m_SampleCount;
  float mipBias = g_FilterParams.m_MipBias;

  float3 accBrdf = 0;
  float accWeight = 0;
  for ( uint i =0; i < sampleCount ; ++ i ) {
    float2 eta = GetSample (i , sampleCount ) ;
    float pdf ;
    float3 H = ImportanceSampleGGX(eta, roughness, N);
    float3 L = normalize(2 * dot( N, H ) * H - N);
    float NdotL = dot(N, L);
    float NdotH = dot(N, H);
    if(NdotL > 0) {    
      float D   = DistributionGGX(NdotH, roughness);
      float pdf = D / 4.; 
      float omegaS = 1. / ( sampleCount * pdf );
      float omegaP = 4. * PI / (6. * width * width );
      float mipLevel = roughness == 0. ? mipBias : max(0., mipBias + .5 * log2 ( omegaS / omegaP ));
      accBrdf += g_SourceCube . SampleLevel ( g_SamplerLinearClamp , L, mipLevel ) * NdotL;
      accWeight += NdotL;
    }
  }
  g_OutCubemap[PixCoord + g_FilterParams.m_Offset].rgb = accBrdf * (1. / accWeight);
}

[numthreads(32, 32, 1)]
void CSIntegrateBRDF(uint3 PixCoord : SV_DispatchThreadID) {
  uint2 FaceSize;
  g_OutBRDF.GetDimensions(FaceSize.x, FaceSize.y);
  float NoV = ((float)PixCoord.x + .5) / (float)FaceSize.x;
  float Roughness = ((float)PixCoord.y + .5) / (float)FaceSize.y;
  float3 N = float3(0., 0., 1.);
  
  float3 V;
  V.x = sqrt( 1. - NoV * NoV ); // sin
  V.y = 0.;
  V.z = NoV; // cos
  float A = 0.;
  float B = 0.;
  float sampleCount = 1024.;
  for( uint i = 0; i < sampleCount; i++ )
  {
    float2 Xi = GetSample (i , sampleCount );
    float3 H = ImportanceSampleGGX(Xi, Roughness, N);
    float3 L = normalize(2. * dot( V, H ) * H - V);
    float NoL = saturate( L.z );
    float NoH = saturate( H.z );
    float VoH = saturate( dot( V, H ) );
    if( NoL > 0. )
    {
      float G = GeometrySmith( Roughness, NoV, NoL );
      float G_Vis = G * VoH / (NoH * NoV);
      float Fc = pow( 1. - VoH, 5. );
      A += (1. - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }
  g_OutBRDF[PixCoord.xy] = float2( A, B ) / sampleCount;
}

[numthreads(32, 32, 1)]
void CSGenerateCubeMip(uint3 PixCoord : SV_DispatchThreadID) {
  g_OutCubemap[PixCoord + g_FilterParams.m_Offset].rgb = g_SourceCube.SampleLevel(g_SamplerLinearClamp, CalcNormal(PixCoord + g_FilterParams.m_Offset), 0.);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float roughness, float NdotV, float NdotL)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;

    float nom   = a2;
    float denom = (NdotH*NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

void ImportanceSampleCosDir (in float2 Xi, in float3 N, out float3 L, out float NdotL, out float pdf) {
  // Local referencial
  float3 upVector = abs (N.z) < .999 ? float3 (0., 0., 1.) : float3 (1., 0., 0.) ;
  float3 tangentX = normalize ( cross ( upVector , N ) ) ;
  float3 tangentY = cross ( N , tangentX ) ;
  
  float r = sqrt ( Xi.x ) ;
  float phi = Xi.y * TWO_PI;
  
  L = float3 ( r * cos ( phi ) , r * sin ( phi ) , sqrt ( max (0. ,1. - Xi.x ) ) ) ;
  L = normalize ( tangentX * L . y + tangentY * L . x + N * L . z ) ;
  
  NdotL = dot (L , N ) ;
  pdf = NdotL * ONE_OVER_PI ;
}

float3 ImportanceSampleGGX(in float2 Xi,in float Roughness , in float3 N)
{
  double a = Roughness * Roughness;
  float Phi = TWO_PI * Xi.x;
  float CosTheta = sqrt( (float)((1. - Xi.y) / ( 1. + (a*a - 1.) * Xi.y ) ));
  float SinTheta = sqrt( 1. - CosTheta * CosTheta );
  float3 H = float3( SinTheta * cos( Phi ), SinTheta * sin( Phi ), CosTheta);
  float3 UpVector = abs(N.z) < .999 ? float3(0.,0.,1.) : float3(1.,0.,0.);
  float3 TangentX = normalize( cross( UpVector , N ) );
  float3 TangentY = cross( N, TangentX );
  // Tangent to world space
  return normalize( TangentX * H.x + TangentY * H.y + N * H.z );
}

float RadicalInverse_VdC(uint bits) 
{
    return float(reversebits(bits)) * 2.3283064365386963e-10; // / 0x100000000
}

float2 GetSample(uint i, uint N) {
  return float2((float)i/(float)N, RadicalInverse_VdC(i));
}