#include "sky_common.hlsli"

#include "sky_inscattering.hlsli"

cbuffer DispatchConstants : register(b0)
{
    float4x4 g_InverseView;
    float4x4 g_InverseProjection;
    float3 g_SunDir;
    float g_Altitude;
    float3 g_MoonDir;
    float g_MaxDepth;
}

RWTexture2DArray<float4> g_AerialLut : register(u0);
RWTexture2D<float4> g_Sky : register(u1);
Texture2D<float4> g_TransmittanceLut : register(t13);
SamplerState g_TransmittanceLutSampler : register(s13);

[numthreads(8, 8, 1)] void CS_AerialLUT(uint3 PixCoord : SV_DispatchThreadID)
{
    uint3 texture_size;
    g_AerialLut.GetDimensions(texture_size.x, texture_size.y, texture_size.z);

    float moon_phase = acos(dot(g_SunDir, g_MoonDir)) * ONE_OVER_PI;

    uint2 pix_coord = PixCoord.xy;
    float start_depth = 0.;
    float4 L = 0.;
    float4 M = 0.;
    float4 transmittance = 1.;
    float4 transmittance_m = 1.;

    float4 position_ndc = float4(((pix_coord + 0.5) / texture_size.xy - .5) * float2(2., -2.), 1., 1.);
    position_ndc = mul(g_InverseProjection, position_ndc);
    position_ndc /= position_ndc.w;

    float lat = lerp(PI_OVER_TWO, -PI_OVER_TWO, pix_coord.y / (texture_size.y - 1.));
    float lon = lerp(0., TWO_PI, pix_coord.x / float(texture_size.x));
    float3 ray_dir;
    ray_dir.y = sin(lat);
    ray_dir.x = cos(lon) * cos(lat);
    ray_dir.z = sin(lon) * cos(lat);

    float3 sun_dir;
    sun_dir.y = g_SunDir.y;
    sun_dir.x = sqrt(1 - sun_dir.y * sun_dir.y);
    sun_dir.z = 0.;
    // float3 ray_dir = mul((float3x3)g_InverseView, normalize(position_ndc.xyz));

    float3 ray_origin = float3(0.0, EARTH_RADIUS + max(g_Altitude, 1.), 0.0);

    float t_d = g_MaxDepth;

    float atmos_dist = ray_sphere_intersection(ray_origin, ray_dir, ATMOSPHERE_RADIUS);
    float ground_dist = ray_sphere_intersection(ray_origin, ray_dir, EARTH_RADIUS);
    // We are inside the atmosphere
    if (ground_dist < 0.0)
    {
        // No ground collision, use the distance to the outer atmosphere
        t_d = min(t_d, atmos_dist);
    }
    else
    {
        // We have a collision with the ground, use the distance to it
        t_d = min(t_d, ground_dist);
    }

    float cos_theta = dot(-ray_dir, g_SunDir);
    float cos_theta_moon = dot(-ray_dir, g_MoonDir);
    float molecular_phase = molecular_phase_function(cos_theta);
    float aerosol_phase = aerosol_phase_function(cos_theta);
    float molecular_phase_moon = molecular_phase_function(cos_theta_moon);
    float aerosol_phase_moon = aerosol_phase_function(cos_theta_moon);

    for (uint i = 0; i < texture_size.z; ++i)
    {
        float t = (i + 1.) / (float)texture_size.z;
        float end_depth = min(t_d, t * t * g_MaxDepth);
        compute_inscattering(g_TransmittanceLut, g_TransmittanceLutSampler, molecular_phase, aerosol_phase, 5, ray_origin, ray_dir, start_depth, end_depth, g_SunDir, L, transmittance);
        compute_inscattering(g_TransmittanceLut, g_TransmittanceLutSampler, molecular_phase_moon, aerosol_phase_moon, 5, ray_origin, ray_dir, start_depth, end_depth, g_MoonDir, M, transmittance_m);
        g_AerialLut[uint3(pix_coord, i)] = float4(linear_srgb_from_spectral_samples(L + .07 * moon_phase * M) * exp2(EXPOSURE), dot(transmittance, .25));
        start_depth = end_depth;
    }
}

[numthreads(8, 8, 1)] void CS_Sky(uint3 PixCoord : SV_DispatchThreadID)
{

    uint2 texture_size;
    g_Sky.GetDimensions(texture_size.x, texture_size.y);

    float moon_phase = acos(dot(g_SunDir, g_MoonDir)) * ONE_OVER_PI;

    uint2 pix_coord = PixCoord.xy;

    float lat = lerp(PI_OVER_TWO, -.125 * PI_OVER_TWO, pix_coord.y / (texture_size.y - 1.));
    float lon = lerp(0., TWO_PI, pix_coord.x / float(texture_size.x-1.));
    float3 ray_dir;
    ray_dir.y = sin(lat);
    ray_dir.x = cos(lon) * cos(lat);
    ray_dir.z = sin(lon) * cos(lat);

    float3 sun_dir;
    sun_dir.y = g_SunDir.y;
    sun_dir.x = sqrt(1 - sun_dir.y * sun_dir.y);
    sun_dir.z = 0.;

    float4 position_ndc = float4(((pix_coord + 0.5) / texture_size.xy - .5) * float2(2., -2.), 1., 1.);
    position_ndc = mul(g_InverseProjection, position_ndc);
    position_ndc /= position_ndc.w;
    // float3 ray_dir = mul((float3x3)g_InverseView, normalize(position_ndc.xyz));

    g_Sky[pix_coord] = get_inscattering(g_TransmittanceLut, g_TransmittanceLutSampler, 32, g_Altitude, ray_dir, g_MaxDepth, 1.#INF, g_SunDir);
    g_Sky[pix_coord] += .07 * moon_phase * get_inscattering(g_TransmittanceLut, g_TransmittanceLutSampler, 32, g_Altitude, ray_dir, g_MaxDepth, 1.#INF, g_MoonDir);
}
