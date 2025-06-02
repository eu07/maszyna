#ifndef SKY_INSCATTERING_HLSLI
#define SKY_INSCATTERING_HLSLI

/*
 * Buffer B: Sky texture
 *
 * "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 * by Sébastien Hillaire (2020).
 *
 * We render the sky to a texture instead of raymarching on the entire screen.
 * This is not very useful in Shadertoy, but very useful for someone looking
 * to implement this on a real application.
 *
 * It is important to note that quality decreases significantly when rendering
 * space views. To avoid this, the compute_inscattering() function can be used
 * directly when rendering to a fullscreen quad.
 */

#include "sky_common.hlsli"
 
void compute_inscattering(Texture2D transmittance_lut, SamplerState lut_sampler, in float molecular_phase, in float aerosol_phase, int steps, float3 ray_origin, float3 ray_dir, float t_min, float t_max, float3 sun_dir, inout float4 L_inscattering, inout float4 transmittance)
{
    float dt = (t_max - t_min) / float(steps);

    for (int i = 0; i < steps; ++i) {
        float t = t_min + (float(i) + 0.5) * dt;
        float3 x_t = ray_origin + ray_dir * t;

        float distance_to_earth_center = length(x_t);
        float3 zenith_dir = x_t / distance_to_earth_center;
        float altitude = distance_to_earth_center - EARTH_RADIUS;
        float normalized_altitude = altitude / ATMOSPHERE_THICKNESS;

        float sample_cos_theta = dot(zenith_dir, sun_dir);

        float4 aerosol_absorption, aerosol_scattering;
        float4 molecular_absorption, molecular_scattering;
        float4 fog_scattering;
        float4 extinction;
        get_atmosphere_collision_coefficients(
            altitude,
            aerosol_absorption, aerosol_scattering,
            molecular_absorption, molecular_scattering,
            fog_scattering,
            extinction);

        float4 transmittance_to_sun = transmittance_from_lut(
            transmittance_lut, lut_sampler, sample_cos_theta, normalized_altitude);

        float4 ms = get_multiple_scattering(
            transmittance_lut, lut_sampler, sample_cos_theta, normalized_altitude,
            distance_to_earth_center);

        float4 S = sun_spectral_irradiance *
            (molecular_scattering * (molecular_phase * transmittance_to_sun + ms) +
             (aerosol_scattering + fog_scattering)   * (aerosol_phase   * transmittance_to_sun + ms));

        float4 step_transmittance = exp(-dt * extinction);

        // Energy-conserving analytical integration
        // "Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite"
        // by Sébastien Hillaire
        float4 S_int = (S - S * step_transmittance) / max(extinction, 1e-7);
        L_inscattering += transmittance * S_int;
        transmittance *= step_transmittance;
    }
}

float4 get_inscattering(Texture2D transmittance_lut, SamplerState lut_sampler, int steps, float altitude, float3 ray_dir, float t_min, float t_max, float3 sun_dir) {
    float cos_theta = dot(-ray_dir, sun_dir);
    float molecular_phase = molecular_phase_function(cos_theta);
    float aerosol_phase = aerosol_phase_function(cos_theta);

    float3 ray_origin = float3(0.0, EARTH_RADIUS + max(altitude, 1.), 0.0);
    float atmos_dist  = ray_sphere_intersection(ray_origin, ray_dir, ATMOSPHERE_RADIUS);
    float ground_dist = ray_sphere_intersection(ray_origin, ray_dir, EARTH_RADIUS);
    // We are inside the atmosphere
    if (ground_dist < 0.0) {
        // No ground collision, use the distance to the outer atmosphere
        t_max = min(t_max, atmos_dist);
    } else {
        // We have a collision with the ground, use the distance to it
        t_max = min(t_max, ground_dist);
    }

    if(t_min >= t_max) return float4(0., 0., 0., 1.);

    float4 L = 0.;
    float4 transmittance = 1.;
    compute_inscattering(transmittance_lut, lut_sampler, molecular_phase, aerosol_phase, 32, ray_origin, ray_dir, t_min, t_max, sun_dir, L, transmittance);
    return float4(linear_srgb_from_spectral_samples(L) * exp2(EXPOSURE), dot(transmittance, .25));
}

#endif