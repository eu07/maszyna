#ifndef SKY_HLSLI
#define SKY_HLSLI

#include "math.hlsli"

#include "sky_common.hlsli"

Texture2D g_Sky : register(t13);
Texture2DArray<float4> g_AerialPerspectiveLut : register(t14);
SamplerState g_SkySampler : register(s13);

#define SKY_INF 1. #INF

float3 CalcSphereNormal(in float3 viewDir, in float3 sphereDir, in float cosAngularSize);

void CalcSun(inout float3 color, in float3 viewDir, in float3 sunDir, in float altitude)
{
    float3 ray_origin = float3(0.0, EARTH_RADIUS + max(altitude, 1.), 0.0);

    float ground_dist = ray_sphere_intersection(ray_origin, viewDir, EARTH_RADIUS);
    if (ground_dist < 0.0)
    {
        if (dot(viewDir, sunDir) > 0.99998869014)
        {
            color = linear_srgb_from_spectral_samples(sun_spectral_irradiance);
        }
    }
}

void CalcMoon(inout float3 color, in float3 viewDir, in float3 moonDir, in float3 sunDir, in float altitude)
{
    float3 ray_origin = float3(0.0, EARTH_RADIUS + max(altitude, 1.), 0.0);

    float ground_dist = ray_sphere_intersection(ray_origin, viewDir, EARTH_RADIUS);
    if (ground_dist < 0.0)
    {
        float3 normal = CalcSphereNormal(viewDir, moonDir, 0.99998869014);
        if (dot(normal, normal) > 0.)
        {
            color = .07 * max(dot(normal, sunDir), 0.) * linear_srgb_from_spectral_samples(sun_spectral_irradiance);
        }
    }
}

float3 CalcSphereNormal(in float3 viewDir, in float3 sphereDir, in float cosAngularSize)
{
    float sqrSinAngularSize = 1. - cosAngularSize * cosAngularSize;
    float tca = dot(sphereDir, viewDir);
    float d2 = dot(sphereDir, sphereDir) - tca * tca;
    if (d2 > sqrSinAngularSize)
        return (float3)(0.);
    float thc = sqrt(sqrSinAngularSize - d2);
    float t = tca - thc;

    if (t < 0.)
        return (float3)(0.);

    float3 p = viewDir * t;
    return normalize(p - sphereDir);
}

float2 CalcEquirectangularCoords(in float3 viewDir, in float3 sunDir, in float2 size)
{
    float2 uv_scale = float2(1., 1. - 1. / size.y);

    // float lon = acos(clamp(dot(normalize(sunDir.xz), normalize(viewDir.xz)), -1., 1.));
    float lon = atan2(-viewDir.z, -viewDir.x);
    float lat = asin(viewDir.y);

    return float2(InvLerp(-PI, PI, lon), InvLerp(PI_OVER_TWO, -PI_OVER_TWO, lat)) * uv_scale;
}

float2 CalcEquirectangularCoordsBottomClipped(in float3 viewDir, in float3 sunDir, in float2 size)
{
    float2 uv_scale = float2(1., 1. - 1. / size.y);

    // float lon = acos(clamp(dot(normalize(sunDir.xz), normalize(viewDir.xz)), -1., 1.));
    float lon = atan2(-viewDir.z, -viewDir.x);
    float lat = asin(viewDir.y);

    return float2(InvLerp(-PI, PI, lon), InvLerp(PI_OVER_TWO, -.125 * PI_OVER_TWO, lat)) * uv_scale;
}

void CalcAtmosphere(inout float3 color, in float3 viewDir, in float3 sunDir)
{
    uint2 size;
    g_Sky.GetDimensions(size.x, size.y);

    float4 sky = g_Sky.SampleLevel(g_SkySampler, CalcEquirectangularCoordsBottomClipped(viewDir, sunDir, size), 0.);
    color = color * sky.a + sky.rgb;
}

void ApplyAerialPerspective(inout float3 color, in float alpha, in float3 viewDir, in float3 sunDir, in float depth)
{
    uint3 size;
    g_AerialPerspectiveLut.GetDimensions(size.x, size.y, size.z);
    float slice = sqrt(depth) * size.z;
    float slice_factor = frac(slice);
    slice -= 1.;
    float2 uv = CalcEquirectangularCoords(viewDir, sunDir, size.xy);
    float4 sample_near = slice > 0. ? g_AerialPerspectiveLut.SampleLevel(g_SkySampler, float3(uv, floor(slice)), 0.) : float4(0., 0., 0., 1.);
    float4 sample_far = g_AerialPerspectiveLut.SampleLevel(g_SkySampler, float3(uv, ceil(slice)), 0.);
    float4 aerial = lerp(sample_near, sample_far, slice_factor);
    color = color * aerial.a + aerial.rgb * alpha;
}

#endif
