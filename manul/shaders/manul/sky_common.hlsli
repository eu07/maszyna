#ifndef SKY_COMMON_HLSLI
#define SKY_COMMON_HLSLI

#include "color_transform.hlsli"
#include "math.hlsli"

cbuffer SkyConstants : register(b13){
    float4 g_AerosolAbsorptionCrossSection;
    float4 g_AerosolScatteringCrossSection;
    float4 g_GroundAlbedo;
    float g_AerosolHeightScale;
    float g_AerosolTurbidity;
    float g_AerosolBaseDensity;
    float g_AerosolBackgroundDividedByBaseDensity;
    float g_OzoneMean;
    float g_FogDensity;
    float g_FogHeightOffset;
    float g_FogHeightScale;
}

// Configurable parameters

// #define ANIMATE_SUN 1
// // 0=equirectangular, 1=fisheye, 2=projection
// #define CAMERA_TYPE 2
// // 0=Background, 1=Desert Dust, 2=Maritime Clean, 3=Maritime Mineral,
// // 4=Polar Antarctic, 5=Polar Artic, 6=Remote Continental, 7=Rural, 8=Urban
//#define AEROSOL_TYPE 8

// static const float SUN_ELEVATION_DEGREES = 0.0;    // 0=horizon, 90=zenith
// static const float EYE_ALTITUDE          = 0.5;    // km
//static const int   MONTH                 = 0;      // 0-11, January to December
//static const float AEROSOL_TURBIDITY     = 1.0;
//static const float4  GROUND_ALBEDO         = 0.3;
// // Ray marching steps. More steps mean better accuracy but worse performance
// static const int TRANSMITTANCE_STEPS     = 32;
// static const int IN_SCATTERING_STEPS     = 32;
// // Camera settings
static const float EXPOSURE              = -4.0;
// // For the "projection" type camera
// static const float CAMERA_FOV   =  90.0;
// static const float CAMERA_YAW   =  15.0;
// static const float CAMERA_PITCH = -12.0;
// static const float CAMERA_ROLL  =   0.0;

// // Debug
// #define ENABLE_SPECTRAL 1
// #define ENABLE_MULTIPLE_SCATTERING 1
// #define ENABLE_AEROSOLS 1
// #define SHOW_RELATIVE_LUMINANCE 0
// #define TONEMAPPING_TECHNIQUE 0 // 0=ACES, 1=simple

//-----------------------------------------------------------------------------
// Constants

// All parameters that depend on wavelength (float4) are sampled at
// 630, 560, 490, 430 nanometers

//static const float PI = 3.14159265358979323846;
static const float INV_PI = 0.31830988618379067154;
static const float INV_4PI = 0.25 * INV_PI;
static const float PHASE_ISOTROPIC = INV_4PI;
static const float RAYLEIGH_PHASE_SCALE = (3.0 / 16.0) * INV_PI;
static const float g = 0.8;
static const float gg = g*g;

static const float EARTH_RADIUS = 6371.0e3; // km
static const float ATMOSPHERE_THICKNESS = 100.0e3; // km
static const float ATMOSPHERE_RADIUS = EARTH_RADIUS + ATMOSPHERE_THICKNESS;
// static const float EYE_DISTANCE_TO_EARTH_CENTER = EARTH_RADIUS + EYE_ALTITUDE;
// static const float SUN_ZENITH_COS_ANGLE = cos(radians(90.0 - SUN_ELEVATION_DEGREES));
// static const float3 SUN_DIR = float3(-sqrt(1.0 - SUN_ZENITH_COS_ANGLE*SUN_ZENITH_COS_ANGLE), 0.0, SUN_ZENITH_COS_ANGLE);

// Extraterrestial Solar Irradiance Spectra, units W * m^-2 * nm^-1
// https://www.nrel.gov/grid/solar-resource/spectra.html
static const float4 sun_spectral_irradiance = float4(1.679, 1.828, 1.986, 1.307);
// Rayleigh scattering coefficient at sea level, units km^-1
// "Rayleigh-scattering calculations for the terrestrial atmosphere"
// by Anthony Bucholtz (1995).
static const float4 molecular_scattering_coefficient_base = float4(6.605e-6, 1.067e-5, 1.842e-5, 3.156e-5);
// Fog scattering/extinction cross section, units m^2 / molecules
// Mie theory results for IOR of 1.333. Particle size is a log normal
// distribution of mean diameter=15 and std deviation=0.4
static const float4 fog_scattering_cross_section = float4(5.015e-10, 4.987e-10, 4.966e-10, 4.949e-10);
// Ozone absorption cross section, units m^2 / molecules
// "High spectral resolution ozone absorption cross-sections"
// by V. Gorshelev et al. (2014).
static const float4 ozone_absorption_cross_section = float4(3.472e-25, 3.914e-25, 1.349e-25, 11.03e-27);

// Mean ozone concentration in Dobson for each month of the year.
// static const float ozone_mean_monthly_dobson[] = {
//     347.0, // January
//     370.0, // February
//     381.0, // March
//     384.0, // April
//     372.0, // May
//     352.0, // June
//     333.0, // July
//     317.0, // August
//     298.0, // September
//     285.0, // October
//     290.0, // November
//     315.0  // December
// };

/*
 * Every aerosol type expects 5 parameters:
 * - Scattering cross section
 * - Absorption cross section
 * - Base density (km^-3)
 * - Background density (km^-3)
 * - Height scaling parameter
 * These parameters can be sent as uniforms.
 *
 * This model for aerosols and their corresponding parameters come from
 * "A Physically-Based Spatio-Temporal Sky Model"
 * by Guimera et al. (2018).
 */
// #if   AEROSOL_TYPE == 0 // Background
// static const float4 aerosol_absorption_cross_section = float4(4.5517e-19, 5.9269e-19, 6.9143e-19, 8.5228e-19);
// static const float4 aerosol_scattering_cross_section = float4(1.8921e-26, 1.6951e-26, 1.7436e-26, 2.1158e-26);
// static const float aerosol_base_density = 2.584e14;
// static const float aerosol_background_density = 2e3;
// #elif AEROSOL_TYPE == 1 // Desert Dust
// static const float4 aerosol_absorption_cross_section = float4(4.6758e-16, 4.4654e-16, 4.1989e-16, 4.1493e-16);
// static const float4 aerosol_scattering_cross_section = float4(2.9144e-16, 3.1463e-16, 3.3902e-16, 3.4298e-16);
// static const float aerosol_base_density = 1.8662e15;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 2.0;
// #elif AEROSOL_TYPE == 2 // Maritime Clean
// static const float4 aerosol_absorption_cross_section = float4(6.3312e-19, 7.5567e-19, 9.2627e-19, 1.0391e-18);
// static const float4 aerosol_scattering_cross_section = float4(4.6539e-26, 2.721e-26, 4.1104e-26, 5.6249e-26);
// static const float aerosol_base_density = 2.0266e14;
// static const float aerosol_background_density = 2e6;
// static const float aerosol_height_scale = 0.9;
// #elif AEROSOL_TYPE == 3 // Maritime Mineral
// static const float4 aerosol_absorption_cross_section = float4(6.9365e-19, 7.5951e-19, 8.2423e-19, 8.9101e-19);
// static const float4 aerosol_scattering_cross_section = float4(2.3699e-19, 2.2439e-19, 2.2126e-19, 2.021e-19);
// static const float aerosol_base_density = 2.0266e14;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 2.0;
// #elif AEROSOL_TYPE == 4 // Polar Antarctic
// static const float4 aerosol_absorption_cross_section = float4(1.3399e-16, 1.3178e-16, 1.2909e-16, 1.3006e-16);
// static const float4 aerosol_scattering_cross_section = float4(1.5506e-19, 1.809e-19, 2.3069e-19, 2.5804e-19);
// static const float aerosol_base_density = 2.3864e13;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 30.0;
// #elif AEROSOL_TYPE == 5 // Polar Arctic
// static const float4 aerosol_absorption_cross_section = float4(1.0364e-16, 1.0609e-16, 1.0193e-16, 1.0092e-16);
// static const float4 aerosol_scattering_cross_section = float4(2.1609e-17, 2.2759e-17, 2.5089e-17, 2.6323e-17);
// static const float aerosol_base_density = 2.3864e13;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 30.0;
// #elif AEROSOL_TYPE == 6 // Remote Continental
// static const float4 aerosol_absorption_cross_section = float4(4.5307e-18, 5.0662e-18, 4.4877e-18, 3.7917e-18);
// static const float4 aerosol_scattering_cross_section = float4(1.8764e-18, 1.746e-18, 1.6902e-18, 1.479e-18);
// static const float aerosol_base_density = 6.103e15;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 0.73;
// #elif AEROSOL_TYPE == 7 // Rural
// static const float4 aerosol_absorption_cross_section = float4(5.0393e-23, 8.0765e-23, 1.3823e-22, 2.3383e-22);
// static const float4 aerosol_scattering_cross_section = float4(2.6004e-22, 2.4844e-22, 2.8362e-22, 2.7494e-22);
// static const float aerosol_base_density = 8.544e15;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 0.73;
// #elif AEROSOL_TYPE == 8 // Urban
// static const float4 aerosol_absorption_cross_section = float4(2.8722e-24, 4.6168e-24, 7.9706e-24, 1.3578e-23);
// static const float4 aerosol_scattering_cross_section = float4(1.5908e-22, 1.7711e-22, 2.0942e-22, 2.4033e-22);
// static const float aerosol_base_density = 1.3681e17;
// static const float aerosol_background_density = 2e3;
// static const float aerosol_height_scale = 0.73;
// #endif
//static const float aerosol_background_divided_by_base_density = aerosol_background_density / aerosol_base_density;
// static const float fog_density = 29811535.0;
// static const float fog_scale_height = .01;
// static const float fog_height_offset = 0.325;

//-----------------------------------------------------------------------------

/*
 * Helper function to obtain the transmittance to the top of the atmosphere
 * from Buffer A.
 */
float4 transmittance_from_lut(Texture2D lut, SamplerState lut_sampler, float cos_theta, float normalized_altitude)
{
    float u = saturate(cos_theta * 0.5 + 0.5);
    float v = saturate(normalized_altitude);
    return lut.SampleLevel(lut_sampler, float2(u, v), 0.);
}

/*
 * Returns the distance between ro and the first intersection with the sphere
 * or -1.0 if there is no intersection. The sphere's origin is (0,0,0).
 * -1.0 is also returned if the ray is pointing away from the sphere.
 */
float ray_sphere_intersection(float3 ro, float3 rd, float radius)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius*radius;
    if (c > 0.0 && b > 0.0) return -1.0;
    float d = b*b - c;
    if (d < 0.0) return -1.0;
    if (d > b*b) return (-b+sqrt(d));
    return (-b-sqrt(d));
}

/*
 * Rayleigh phase function.
 */
float molecular_phase_function(float cos_theta)
{
    return RAYLEIGH_PHASE_SCALE * (1.0 + cos_theta*cos_theta);
}

/*
 * Henyey-Greenstrein phase function.
 */
float aerosol_phase_function(float cos_theta)
{
    float den = 1.0 + gg + 2.0 * g * cos_theta;
    return INV_4PI * (1.0 - gg) / (den * sqrt(den));
}

float4 get_multiple_scattering(Texture2D transmittance_lut, SamplerState lut_sampler, float cos_theta, float normalized_height, float d)
{
    // Solid angle subtended by the planet from a point at d distance
    // from the planet center.
    float omega = 2.0 * PI * (1.0 - sqrt(max(0., d*d - EARTH_RADIUS*EARTH_RADIUS)) / d);

    float4 T_to_ground = transmittance_from_lut(transmittance_lut, lut_sampler, cos_theta, 0.0);

    float4 T_ground_to_sample =
        transmittance_from_lut(transmittance_lut, lut_sampler, 1.0, 0.0) /
        transmittance_from_lut(transmittance_lut, lut_sampler, 1.0, normalized_height);

    // 2nd order scattering from the ground
    float4 L_ground = PHASE_ISOTROPIC * omega * (g_GroundAlbedo / PI) * T_to_ground * T_ground_to_sample * cos_theta;

    // Fit of Earth's multiple scattering coming from other points in the atmosphere
    float4 L_ms = 0.02 * float4(0.217, 0.347, 0.594, 1.0) * (1.0 / (1.0 + 5.0 * exp(-17.92 * cos_theta)));

    return L_ms + L_ground;
}

/*
 * Return the molecular volume scattering coefficient (km^-1) for a given altitude
 * in kilometers.
 */
float4 get_molecular_scattering_coefficient(float h)
{
    return molecular_scattering_coefficient_base * exp(-0.07771971 * pow(h, 1.16364243));
}

/*
 * Return the molecular volume absorption coefficient (km^-1) for a given altitude
 * in kilometers.
 */
float4 get_molecular_absorption_coefficient(float h)
{
    h += 1e-4; // Avoid division by 0
    float t = log(h) - 3.22261;
    float density = 3.78547397e17 * (1.0 / h) * exp(-t * t * 5.55555555);
    return ozone_absorption_cross_section * g_OzoneMean * density;
}

/*
 * Return the fog volume scattering coefficient (m^-1) for a given altitude in
 * kilometers.
 *
 * Fog (or mist, depending on density) is a special kind of aerosol consisting
 * of water droplets or ice crystals. Visibility is mostly dependent on fog.
 */
float4 get_fog_scattering_coefficient(float h)
{
    if (g_FogDensity > 0.0) {
        return fog_scattering_cross_section * g_FogDensity
            * min(1.0, exp((-h + g_FogHeightOffset) / g_FogHeightScale));
    } else {
        return 0.0;
    }
}

float get_aerosol_density(float h)
{
    return g_AerosolBaseDensity * (exp(-h / g_AerosolHeightScale)
        + g_AerosolBackgroundDividedByBaseDensity);
}

/*
 * Get the collision coefficients (scattering and absorption) of the
 * atmospheric medium for a given point at an altitude h.
 */
void get_atmosphere_collision_coefficients(in float h,
                                           out float4 aerosol_absorption,
                                           out float4 aerosol_scattering,
                                           out float4 molecular_absorption,
                                           out float4 molecular_scattering,
                                           out float4 fog_scattering,
                                           out float4 extinction)
{
    h = max(h, 1.e-3); // In case height is negative
    h *= 1.e-3;
    float aerosol_density = get_aerosol_density(h);
    aerosol_absorption = g_AerosolAbsorptionCrossSection * aerosol_density * g_AerosolTurbidity;
    aerosol_scattering = g_AerosolScatteringCrossSection * aerosol_density * g_AerosolTurbidity;
    molecular_absorption = get_molecular_absorption_coefficient(h);
    molecular_scattering = get_molecular_scattering_coefficient(h);
    fog_scattering = get_fog_scattering_coefficient(h);
    extinction = aerosol_absorption + aerosol_scattering + molecular_absorption + molecular_scattering + fog_scattering;
}

//-----------------------------------------------------------------------------
// Spectral rendering stuff

static const float4x3 M = float4x3(
    137.672389239975, -8.632904716299537, -1.7181567391931372,
    32.549094028629234, 91.29801417199785, -12.005406444382531,
    -38.91428392614275, 34.31665471469816, 29.89044807197628,
    8.572844237945445, -11.103384660054624, 117.47585277566478
);

float3 linear_srgb_from_spectral_samples(float4 L)
{
    return REC709_to_XYZ(mul(L, M));
}


#endif