#ifndef SKY_HLSLI
#define SKY_HLSLI

#include "math.hlsli"

#define SKY_INF 1.#INF

static const float3x3 CAS2RGB = float3x3(
     1.6218, -0.4493, 0.0325,
    -0.0374, 1.0598, -0.0742,
    -0.0283, -0.1119, 1.0491);
    
static const float3x3 RGB2CAS = float3x3(
     6.2267e-1, 2.6392e-1, -6.2375e-4,
     2.3324e-2, 9.6056e-1, 6.7215e-2,
     1.9285e-2, 1.0958e-1, 9.6035e-1);

struct Atmosphere
{
    float3 sunDirection;
    float atmosphereRadius;
    float earthRadius;
    float Hr;
    float Hm;
    float3 betaR;
    float3 betaM;
    float3 irradiance;
};

void swap(inout float a, inout float b)
{
    float temp = a;
    a = b;
    b = temp;
}

bool solveQuadratic(in float a, in float b, in float c, out float x1, out float x2)
{
    if (b == 0)
    {
        // Handle special case where the the two vector ray.dir and V are perpendicular
        // with V = ray.orig - sphere.centre
        if (a == 0)
            return false;
        x1 = 0;
        x2 = sqrt(-c / a);
        return true;
    }
    float discr = b * b - 4 * a * c;

    if (discr < 0)
        return false;

    float q = (b < 0.f) ? -0.5f * (b - sqrt(discr)) : -0.5f * (b + sqrt(discr));
    x1 = q / a;
    x2 = c / q;

    return true;
}

bool solveQuadratic(in float a, in float b, in float c)
{
    if (b == 0)
    {
        // Handle special case where the the two vector ray.dir and V are perpendicular
        // with V = ray.orig - sphere.centre
        return a != 0;
    }

    return b * b - 4 * a * c >= 0;
}

bool RaySphereIntersect(in float3 orig, in float3 dir, in float radius, out float t0, out float t1)
{
    // They ray dir is normalized so A = 1 
    float A = dot(dir, dir);
    float B = 2 * dot(dir, orig);
    float C = dot(orig, orig) - radius * radius;

    if (!solveQuadratic(A, B, C, t0, t1))
        return false;

    if (t0 > t1)
        swap(t0, t1);

    return true;
}

bool RaySphereIntersect(in float3 orig, in float3 dir, in float radius)
{
    // They ray dir is normalized so A = 1 
    float A = dot(dir, dir);
    float B = 2 * dot(dir, orig);
    float C = dot(orig, orig) - radius * radius;

    return solveQuadratic(A, B, C);
}

float2x3 ComputeIncidentLight(in Atmosphere atmosphere, in float3 orig, in float3 dir, float tmin, float tmax)
{
    float t0, t1;
    if (!RaySphereIntersect(orig, dir, atmosphere.atmosphereRadius, t0, t1) || t1 < 0)
        return 0;
    if (t0 > tmin && t0 > 0)
        tmin = t0;
    if (t1 < tmax)
        tmax = t1;
    uint numSamples = 16;
    uint numSamplesLight = 8;
    float segmentLength = (tmax - tmin) / numSamples;
    float tCurrent = tmin;
    float3 sumR = 0.;
    float3 sumM = 0.; //mie and rayleigh contribution 
    float opticalDepthR = 0, opticalDepthM = 0;
    float mu = dot(dir, atmosphere.sunDirection); //mu in the paper which is the cosine of the angle between the sun direction and the ray direction 
    float phaseR = 3.f / (16.f * PI) * (1 + mu * mu);
    float g = 0.76f;
    float phaseM = 3.f / (8.f * PI) * ((1.f - g * g) * (1.f + mu * mu)) / ((2.f + g * g) * pow(1.f + g * g - 2.f * g * mu, 1.5f));
    for (uint i = 0; i < numSamples; ++i)
    {
        float3 samplePosition = orig + (tCurrent + segmentLength * 0.5f) * dir;
        float height = length(samplePosition) - atmosphere.earthRadius;
        // compute optical depth for light
        float hr = exp(-height / atmosphere.Hr) * segmentLength;
        float hm = exp(-height / atmosphere.Hm) * segmentLength;
        opticalDepthR += hr;
        opticalDepthM += hm;
        // light optical depth
        float t0Light, t1Light;
        RaySphereIntersect(samplePosition, atmosphere.sunDirection, atmosphere.atmosphereRadius, t0Light, t1Light);
        float segmentLengthLight = t1Light / numSamplesLight, tCurrentLight = 0;
        float opticalDepthLightR = 0, opticalDepthLightM = 0;
        uint j;
        for (j = 0; j < numSamplesLight; ++j)
        {
            float3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5f) * atmosphere.sunDirection;
            float heightLight = length(samplePositionLight) - atmosphere.earthRadius;
            if (heightLight < 0)
                break;
            opticalDepthLightR += exp(-heightLight / atmosphere.Hr) * segmentLengthLight;
            opticalDepthLightM += exp(-heightLight / atmosphere.Hm) * segmentLengthLight;
            tCurrentLight += segmentLengthLight;
        }
        if (j == numSamplesLight)
        {
            float3 tau = atmosphere.betaR * (opticalDepthR + opticalDepthLightR) + atmosphere.betaM * 1.1f * (opticalDepthM + opticalDepthLightM);
            float3 attenuation = exp(-tau);
            sumR += attenuation * hr;
            sumM += attenuation * hm;
        }
        tCurrent += segmentLength;
    }
 
    // We use a magic number here for the intensity of the sun (20). We will make it more
    // scientific in a future revision of this lesson/code
    float3 tau = atmosphere.betaR * opticalDepthR + atmosphere.betaM * 1.1f * opticalDepthM;
    return float2x3((sumR * atmosphere.betaR * phaseR + sumM * atmosphere.betaM * phaseM) * 20, exp(-tau));
}

//#define MIE_G 0.76
//#define SQR_G (MIE_G * MIE_G)
//#define RAYLEIGH_SCALE 8e3 /* Rayleigh scale height (m). */
//#define MIE_SCALE 1.2e3 /* Mie scale height (m). */

//float phase_rayleigh(float mu)
//{
    //return 3.0f / (16.0f * PI) * (1.0f + mu * mu);
//}

//float phase_mie(float mu)
//{
    //return (3.0f * (1.0f - SQR_G) * (1.0f + mu * mu)) /
         //(8.0f * PI * (2.0f + SQR_G) * pow((1.0f + SQR_G - 2.0f * MIE_G * mu), 1.5));
//}

//float density_rayleigh(float height)
//{
    //return exp(-height / RAYLEIGH_SCALE);
//}

//float density_mie(float height)
//{
    //return exp(-height / MIE_SCALE);
//}

//float density_ozone(float height)
//{
    //float den = 0.0f;
    //if (height >= 10000.0f && height < 25000.0f)
    //{
        //den = 1.0f / 15000.0f * height - 2.0f / 3.0f;
    //}
    //else if (height >= 25000 && height < 40000)
    //{
        //den = -(1.0f / 15000.0f * height - 8.0f / 3.0f);
    //}
    //return den;
//}

///* Parameters for optical depth quadrature.
 //* See the comment in ray_optical_depth for more detail.
 //* Computed using sympy and following Python code:
 //* # from sympy.integrals.quadrature import gauss_laguerre
 //* # from sympy import exp
 //* # x, w = gauss_laguerre(8, 50)
 //* # xend = 25
 //* # print([(xi / xend).evalf(10) for xi in x])
 //* # print([(wi * exp(xi) / xend).evalf(10) for xi, wi in zip(x, w)])
 //*/
//static const int quadrature_steps = 8;
//static const float quadrature_nodes[] =
//{
    //0.006811185292f,
                                         //0.03614807107f,
                                         //0.09004346519f,
                                         //0.1706680068f,
                                         //0.2818362161f,
                                         //0.4303406404f,
                                         //0.6296271457f,
                                         //0.9145252695f
//};
//static const float quadrature_weights[] =
//{
    //0.01750893642f,
                                           //0.04135477391f,
                                           //0.06678839063f,
                                           //0.09507698807f,
                                           //0.1283416365f,
                                           //0.1707430204f,
                                           //0.2327233347f,
                                           //0.3562490486f
//};

//float3 ray_optical_depth(in Atmosphere atmosphere, float3 ray_origin)
//{
  ///* This function computes the optical depth along a ray.
   //* Instead of using classic ray marching, the code is based on Gauss-Laguerre quadrature,
   //* which is designed to compute the integral of f(x)*exp(-x) from 0 to infinity.
   //* This works well here, since the optical depth along the ray tends to decrease exponentially.
   //* By setting f(x) = g(x) exp(x), the exponentials cancel out and we get the integral of g(x).
   //* The nodes and weights used here are the standard n=6 Gauss-Laguerre values, except that
   //* the exp(x) scaling factor is already included in the weights.
   //* The parametrization along the ray is scaled so that the last quadrature node is still within
   //* the atmosphere. */
    //float t0, t1;
    //float3 ray_dir = atmosphere.sunDirection;
    //RaySphereIntersect(ray_origin, ray_dir, atmosphere.atmosphereRadius, t0, t1);
    //float3 ray_end = ray_dir * t1;
    //float ray_length = distance(ray_origin, ray_end);

    //float3 segment = ray_length * ray_dir;

  ///* instead of tracking the transmission spectrum across all wavelengths directly,
   //* we use the fact that the density always has the same spectrum for each type of
   //* scattering, so we split the density into a constant spectrum and a factor and
   //* only track the factors */
    //float3 optical_depth = float3(0.0f, 0.0f, 0.0f);

    //for (int i = 0; i < quadrature_steps; i++)
    //{
        //float3 P = ray_origin + quadrature_nodes[i] * segment;

    ///* height above sea level */
        //float height = length(P) - atmosphere.earthRadius;

        //float3 density =float3(
        //density_rayleigh(height), density_mie(height), density_ozone(height));
        //optical_depth += density * quadrature_weights[i];
    //}

    //return optical_depth * ray_length;
//}

//float3 ComputeIncidentLight(in Atmosphere atmosphere, in float3 orig, in float3 dir, float tmin, float tmax)
//{
    //float t0, t1;
    //if (!RaySphereIntersect(orig, dir, atmosphere.atmosphereRadius, t0, t1) || t1 < 0)
        //return 0;
    //if (t0 > tmin && t0 > 0)
        //tmin = t0;
    //if (t1 < tmax)
        //tmax = t1;
    //uint numSamples = 16;
    //uint numSamplesLight = 8;
    //float segmentLength = (tmax - tmin) / numSamples;
    //float tCurrent = tmin;
    //float mu = dot(dir, atmosphere.sunDirection); //mu in the paper which is the cosine of the angle between the sun direction and the ray direction 
    //float3 opticalDepth = float3(0., 0., 0.);
    //float3 densityScale = float3(1., 1., 1.);
    //float3 phaseFunction = float3(phase_rayleigh(mu), phase_mie(mu), 0.0f);
    //float3 spectrum = float3(0., 0., 0.);
    //for (uint i = 0; i < numSamples; ++i)
    //{
        //float3 samplePosition = orig + (tCurrent + segmentLength * 0.5f) * dir;
        //float height = length(samplePosition) - atmosphere.earthRadius;
        //float3 density = densityScale * float3(density_rayleigh(height),
                                                 //density_mie(height),
                                                 //density_ozone(height));
        //opticalDepth += segmentLength * density;
        
        ////if (!RaySphereIntersect(samplePosition, atmosphere.sunDirection, atmosphere.earthRadius))
        ////{
            //float3 light_optical_depth = densityScale * ray_optical_depth(atmosphere, samplePosition);
            //float3 total_optical_depth = opticalDepth + light_optical_depth;
            
            //for (int wl = 0; wl < 3; ++wl)
            //{
                //float3 extinction_density = total_optical_depth * float3(atmosphere.betaR[wl],
                                                                      //1.11f * atmosphere.betaM[wl],
                                                                      //0. /*ozone_coeff[wl]*/);
                //float attenuation = exp(-dot(extinction_density, float3(1., 1., 1.)));

                //float3 scattering_density = density * float3(atmosphere.betaR[wl], atmosphere.betaM[wl], 0.0f);

        ///* the total inscattered radiance from one segment is:
         //* Tr(A<->B) * Tr(B<->C) * sigma_s * phase * L * segment_length
         //*
         //* These terms are:
         //* Tr(A<->B): Transmission from start to scattering position (tracked in optical_depth)
         //* Tr(B<->C): Transmission from scattering position to light (computed in
         //* ray_optical_depth) sigma_s: Scattering density phase: Phase function of the scattering
         //* type (Rayleigh or Mie) L: Radiance coming from the light source segment_length: The
         //* length of the segment
         //*
         //* The code here is just that, with a bit of additional optimization to not store full
         //* spectra for the optical depth
         //*/
                //spectrum[wl] += attenuation * dot(phaseFunction * scattering_density, float3(1., 1., 1.)) *
                          //atmosphere.irradiance[wl] * 7. * segmentLength;
            //}

        ////}

        //// compute optical depth for light
        ////float hr = exp(-height / atmosphere.Hr) * segmentLength;
        ////float hm = exp(-height / atmosphere.Hm) * segmentLength;
        ////opticalDepthR += hr;
        ////opticalDepthM += hm;
        ////// light optical depth
        ////float t0Light, t1Light;
        ////RaySphereIntersect(samplePosition, atmosphere.sunDirection, atmosphere.atmosphereRadius, t0Light, t1Light);
        ////float segmentLengthLight = t1Light / numSamplesLight, tCurrentLight = 0;
        ////float opticalDepthLightR = 0, opticalDepthLightM = 0;
        ////uint j;
        ////for (j = 0; j < numSamplesLight; ++j)
        ////{
        ////    float3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5f) * atmosphere.sunDirection;
        ////    float heightLight = length(samplePositionLight) - atmosphere.earthRadius;
        ////    if (heightLight < 0)
        ////        break;
        ////    opticalDepthLightR += exp(-heightLight / atmosphere.Hr) * segmentLengthLight;
        ////    opticalDepthLightM += exp(-heightLight / atmosphere.Hm) * segmentLengthLight;
        ////    tCurrentLight += segmentLengthLight;
        ////}
        ////if (j == numSamplesLight)
        ////{
        ////    float3 tau = atmosphere.betaR * (opticalDepthR + opticalDepthLightR) + atmosphere.betaM * 1.1f * (opticalDepthM + opticalDepthLightM);
        ////    float3 attenuation = float3(exp(-tau.x), exp(-tau.y), exp(-tau.z));
        ////    sumR += attenuation * hr;
        ////    sumM += attenuation * hm;
        ////}
        //tCurrent += segmentLength;
    //}
    //return spectrum;
    //// We use a magic number here for the intensity of the sun (20). We will make it more
    //// scientific in a future revision of this lesson/code
    ////return (sumR * atmosphere.betaR * phaseR + sumM * atmosphere.betaM * phaseM) * 20;
//}

void CalcAtmosphere(inout float3 color, in float3 viewDir, in float3 sunDir, in float altitude, in float far, in float3 sunColor)
{
    //float3x3 RGB2CAS = float3x3(1., 0., 0., 0., 1., 0., 0., 0., 1.);
    //float3x3 CAS2RGB = float3x3(1., 0., 0., 0., 1., 0., 0., 0., 1.);
    Atmosphere atmosphere;
    atmosphere.earthRadius = 6360.e3;
    atmosphere.atmosphereRadius = 6420.e3;
    atmosphere.Hr = 7994.;
    atmosphere.Hm = 1200.;
    atmosphere.betaR = float3(5.8e-6, 13.5e-6, 33.1e-6);
    atmosphere.betaR = float3(7.2865e-6, 1.2863e-5, 2.7408e-5);
    //atmosphere.betaR = float3(8.658882115850087e-6, 1.5119838839850446e-5, 3.15880085046994e-5);
    atmosphere.betaM = (float3) 21.e-6;
    //atmosphere.irradiance = float3(1.4662835232339013, 1.7557753193321481, 1.7149614362178376);
    
    if (isinf(far))
    {
        color = 1.e-7;
    }
    else
    {
        color = mul(RGB2CAS, color);
    }
    
    atmosphere.sunDirection = sunDir;
    float2x3 atm = ComputeIncidentLight(atmosphere, float3(0., atmosphere.earthRadius + altitude, 0.), viewDir, 0., far);
    color = atm[0] + color * atm[1];
    
    color = mul(CAS2RGB, color);
    //light = light.x * float3(0.933600, 0.358317, 0.000000) + light.y * float3(0.413697, 0.910408, 0.003489) + light.z * float3(0.178639, 0.025820, 0.983576);
    //light = light.x * float3(0.899398, 0.437131, 0.000009) + light.y * float3(0.256233, 0.966471, 0.016686) + light.z * float3(0.175352, 0.029461, 0.984065);
    //light = max(0., light.x * float3(3.240970, -0.969244, 0.055630) + light.y * float3(-1.537383, 1.875968, -0.203977) + light.z * float3(-0.498611, 0.041555, 1.056972));
    //return max(0., mul(float3x3(
    // 1.6218, -0.4493, 0.0325,
    //-0.0374, 1.0598, -0.0742,
    //-0.0283, -0.1119, 1.0491), light));
    //float3x3 transform = float3x3(1.6218, -.4493, .0325, -.0374, 1.0598, -.0742, -.0283, -.0119, 1.0491);
    //return mul(light, transform);
    //return max(0., light.x * float3(2.512071, -0.057897, -0.043816) + light.y * float3(-0.622965, 1.469301, -0.155101) + light.z * float3(0.063069, -0.143869, 2.035035));
    //return light;
    //return (light.x * float3(1., 0., 0.) + light.y * float3(0.64, 1., 0.) + light.z * float3(0., 0., 1.)) / 1.64;
    //return (light.x * float3(0.12544465, 0., 0.) + light.y * float3(0., 1.4467391, 0.)
    //+ light.z * float3(0.22230228, 0., 1.86127603));
    //return float3(dot(light, float3(0.64,
    //0.33,
    //0.03)), dot(light, float3(0.30,
    //0.60,
    //0.10)),
    //dot(light, float3(0.15,
    //0.06,
    //0.79)));

}

void Sky(inout float3 color, in float3 viewDir, in float3 sunDir, in float altitude)
{
    CalcAtmosphere(color, viewDir, sunDir, altitude, 1.#INF, 1.);
}

#endif
