#ifndef LIGHTING_FUNCTIONS_HLSLI
#define LIGHTING_FUNCTIONS_HLSLI

struct SurfaceData {
  float3 m_Albedo;
  float m_Alpha;
  float m_Metalness;
  float m_Roughness;
  float m_DiffuseOcclusion;
  float m_SpecularOcclusion;
  float3 m_View;
  float3 m_Normal;
  float3 m_Reflect;
  float m_NdotV;
  float3 m_SpecularF0;
  float3 m_SpecularF;
  float m_HorizonFading;
};

struct DirectionalLight {
  float3 m_LightVector;
  float3 m_Color;
};

void ApplyDirectionalLight(inout float3 lit, in DirectionalLight light, in SurfaceData material, in float shadow);

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness);
float FresnelSchlickRoughness(float cosTheta, float F0, float roughness);
float DistributionGGX(float3 N, float3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);
float ComputeSpecOcclusion(float NdotV, float AO, float roughness);
float HorizonFading(float NdotL, float horizonFade);

void ApplyDirectionalLight(inout float3 lit, in DirectionalLight light, in SurfaceData material, in float shadow) {
  float3 H = normalize(material.m_View + light.m_LightVector);
  float3 radiance = light.m_Color;
  
  float NdotL = max(dot(material.m_Normal, light.m_LightVector), 0.);   

  // Cook-Torrance BRDF
  float NDF = DistributionGGX(material.m_Normal, H, material.m_Roughness);   
  float G   = GeometrySmith(material.m_Normal, material.m_View, light.m_LightVector, material.m_Roughness);      
     
  float3 numerator    = NDF * G * material.m_SpecularF; 
  float denominator = 4. * material.m_NdotV * NdotL + 1e-4; // + 0.0001 to prevent divide by zero
  float3 specular = numerator / denominator;
  
  // kS is equal to Fresnel
  float3 kS = material.m_SpecularF;
  // for energy conservation, the diffuse and specular light can't
  // be above 1.0 (unless the surface emits light); to preserve this
  // relationship the diffuse component (kD) should equal 1.0 - kS.
  float3 kD = 1. - kS;
  // multiply kD by the inverse metalness such that only non-metals 
  // have diffuse lighting, or a linear blend if partly metal (pure metals
  // have no diffuse light).
  kD = lerp(kD, 0., material.m_Metalness);	  


  // add to outgoing radiance Lo
  //lit += shadow * (kD * material.m_Albedo * ONE_OVER_PI * material.m_DiffuseOcclusion * material.m_Alpha + specular * material.m_SpecularOcclusion * HorizonFading(NdotL, material.m_HorizonFading)) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
  lit += shadow * (kD * material.m_Albedo * ONE_OVER_PI * material.m_Alpha + specular * HorizonFading(NdotL, material.m_HorizonFading)) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}

float FresnelSchlickRoughness(float cosTheta, float F0, float roughness)
{
    return F0 + (max(1. - roughness, F0) - F0) * pow(saturate(1. - cosTheta), 5.);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max((float3)(1. - roughness), F0) - F0) * pow(saturate(1. - cosTheta), 5.);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.) + 1.);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.);
    float k = (r*r) / 8.;

    float num   = NdotV;
    float denom = NdotV * (1. - k) + k;
	
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.);
    float NdotL = max(dot(N, L), 0.);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float ComputeSpecOcclusion(float NdotV, float AO, float roughness)
{
  return saturate(pow(NdotV + AO, exp2(-16. * roughness - 1.)) - 1. + AO);
}

float HorizonFading(float NdotL, float horizonFade) 
{ 
  float horiz = saturate(1.0 + horizonFade * NdotL); 
  return horiz * horiz; 
}

#endif