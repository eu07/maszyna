#ifndef SKY_HLSLI
#define SKY_HLSLI

#include "math.hlsli"

// Public Domain under http://unlicense.org, see link for details.

// A Nishita93-style single scattering atmosphere approximation.
// This can serve as a (not quite drop-in) replacement for e.g.
//     https://github.com/wwwtyro/glsl-atmosphere
// Inherently provides both the sky model and the aerial perspective
// in the same code.
// Works for both surface and space views.

// Consider a spherical planet of radius R, centered at origin.
// Assuming, for the purposes of scattering computations, a 2-component
// (Rayleigh and Mie particles) atmosphere with position-dependent
// relative (i.e. dimensionless) densities Wr(r), Wm(r), the optical
// depth on the (segment of) ray ro+t*rd is:
//     D(ro,rd,l,h) = ∫ Ber*Wr(ro+t*rd)+Bem*Wm(ro+t*rd) dt on [l;h]
// where Ber and Bem are volume-extinction coefficients (in units of m^-1):
//     Ber=Bsr+Bar
//     Bem=Bsm+Bam
//     Bar, Bam - Rayleigh and Mie volume-absorption coefficients, respectively
//     Bsr, Bsm - Rayleigh and Mie volume-scattering coefficients, respectively
// at W=1.
// NOTE: the volume-scattering, etc., coefficients are typically
// denoted β in literature, and are written as B here to keep the
// identifiers in ASCII. Similarly, the densities are typically denoted ρ.
// The integral is typically taken either on [0;+∞), or until the ray
// hits an obstacle (e.g. the planet itself).
// From this, transmittance is calculated as usual via Beer-Lambert law:
//     T=exp(-D)
// Consider a light source (effectively infinitely far away) in the
// direction ld (so that the light direction is -ld). In the
// single-scattering approximation we only consider the following scenario:
// photons from the light source arrive (attenuated by the atmosphere) from
// the direction ld at some point on view ray ro+t*rd, experience scattering
// and arrive (again attenuated by the atmosphere) at viewer. The radiance,
// in W*sr^-1*m^-2, that the viewer observes in direction rd, then is:
//     L = E * ∫ S(ro+t*rd)*(Pr*Bsr*Wr(ro+t*rd)+Pm*Bsm*Wm(ro+t*rd))*exp(-(D(ro,rd,0,t)+D(ro+t*rd,ld,0,+∞))) dt
// where
//     E      - irradiance from the light source, in W*m^-2
//     Pr, Pm - Rayleigh and Mie phase functions, dimensionless
//     S(r)   - shadow/obstruction (0 if a ray from r towards ld intersects an obstacle, 1 otherwise)
// The irradiance is assumed constant in vicinity of the planet,
// and the light is only coming from ld (otherwise we would need to
// integrate the incoming radiance from all possible ld; here
// we basically assume that Lin(dir)=E*delta(dir-ld)).
// The phase functions depend on rd and ld, and describe the directional
// dependence of scattering.
// If the light is not monochromatic, the quantities L, E, and B* are actually
// spectral: they depend on the wavelength (or frequency, depending on how
// you style the spectrum). The units of L and E also change accordingly (e.g.
// E in W*sr^-1*m^-3 and E in W*m^-3 for spectrum-as-a-function-of-wavelength),
// but B* is still in m^-1.
// The entire double integral (since D(...) inside are themselves integrals)
// for the single-scattering approximation, in TeX:
//     L_\lambda=E_\lambda \int_0^\infty S(\vec{r}_o+t \vec{r}_d) \left( P_r(\vec{r}_d,\vec{l}_d) B_{s r \lambda} W_r(\vec{r}_o+t \vec{r}_d) + P_m(\vec{r}_d,\vec{l}_d) B_{s m \lambda} W_m(\vec{r}_o+t \vec{r}_d) \right) \exp \left( -\left( \int_0^t \left( (B_{s r \lambda}+B_{a r \lambda}) W_r(\vec{r}_o+s \vec{r}_d)+ (B_{s m \lambda}+B_{a m \lambda}) W_m(\vec{r}_o+s \vec{r}_d) \right) \, ds + \int_0^\infty \left((B_{s r \lambda}+B_{a r \lambda}) W_r(\vec{r}_o+t \vec{r}_d+s \vec{l}_d)+ (B_{s m \lambda}+B_{a m \lambda}) W_m(\vec{r}_o+t \vec{r}_d+s \vec{l}_d) \right) \, ds \right) \right)
// NOTE: this can be also expressed as a differential equation. In this
// formulation, the computation is similar to alpha-blending infinitely many
// [t;dt] elements. This blending corresponds to exp(-D(ro,rd,0,t)) part in the
// integral, which is, therefore, absent from the differential equation.
//
// Computing this integral analytically is challenging, even for simple setups,
// so it is typically tackled numerically.
// The inner integral, D(...), however, admits fairly reasonable analytical
// approximations (whether they are faster than numerical methods is a separate
// question - we can get away with surprisingly few samples for
// numerical integration). A single-component version is sufficient to explore,
// since the multi-component version (Rayleigh and Mie) is a simple sum (this
// is NOT the case for the entire integral L, which is part of the reason
// for difficulty of computing it).
// Consider density exponentially decreasing with the altitude (a simple
// version of barometric formula)
//     W=exp(-h/H)=exp(-(|r|-R)/H)
// where H is the scale height (generally, Rayleigh and Mie particles have
// different scale heights).
// This, basically, reduces the integral D(...) to the Chapman function, for
// which several approximations exist.
// We can approximate the density as a quadratic (gaussian) exponent:
//     W≈exp(-(r^2-R^2)/(2*R*H))=exp(-h/H-h^2/(2*R*H))
// In this case, the single-component integral is:
//     D = ∫ B*W(ro+t*rd) dt = B*sqrt(pi*R*H/2)*exp((R^2+dot(ro,rd)^2-dot(ro,ro))/(2*R*H))*erf((t+dot(ro,rd))/sqrt(2*R*H))
// This, actually, corresponds an existing approximation of the Chapman
// function. It has the advantage of being relatively simple and fully analytic.
// For Earth (R/H≈700) the error is reported ~0.1%.
// The expression for D above is useful both as a building block for the full
// radiance integral L, and on its own: for computing the optical depth, e.g.
// for aerial perspective and irradiance (both magnitude and color!) on a surface.
// Once we have this, the rest is just numerical intergration
// to compute L. We use an effective atmosphere radius (tuning parameter, currently
// const*(Hr+Hm), which balances errors from large integration step vs discarding
// larger part of atmosphere; more samples incentivize larger effective height) to only
// integrate on a finite segment where the atmospheric density is considered high
// enough. We then potentially split the integration segment by the planet's shadow,
// to only integrate on the lit parts of the ray (an alternative, to integrate on the
// entire segment, but skip the obstructed samples, seems less attractive, especially
// if the number of samples is low), since that are the only parts that contribute
// the light in our model. The integration itself is done using rectangle rule with
// a specifically chosen offset. This was found to provide better results than
// the commonly used midpoint rule (i.e. offset=1/2), while being similarly as simple.
// Notably, same as midpoint rule, there are no samples on segment endpoints,
// so the (sub-)segments don't share samples, which simplifies the computation.
// The results may look passable with as few as 4 samples total. The default in this
// shader is 8.

// Literature:
//     https://en.wikipedia.org/wiki/Optical_depth
//     https://en.wikipedia.org/wiki/Rayleigh_scattering
//     https://en.wikipedia.org/wiki/Mie_scattering
//     https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law
//     https://en.wikipedia.org/wiki/Sunlight
//     https://en.wikipedia.org/wiki/Barometric_formula
//     https://en.wikipedia.org/wiki/Scale_height
//     https://en.wikipedia.org/wiki/Chapman_function
//     https://pbr-book.org/4ed/Volume_Scattering
//     https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky/simulating-colors-of-the-sky.html
//     http://www.thetenthplanet.de/archives/4519
//     https://zero-radiance.github.io/post/analytic-media/
//     Tomoyuki Nishita, Takao Sirai, Katsumi Tadamura, and Eihachiro Nakamae. 1993. Display of the earth taking into account atmospheric scattering. In Proceedings of the 20th annual conference on Computer graphics and interactive techniques (SIGGRAPH '93). Association for Computing Machinery, New York, NY, USA, 175–182. https://doi.org/10.1145/166117.166140
//         http://nishitalab.org/user/nis/cdrom/sig93_nis.pdf
//     Eric Bruneton. 2016. A qualitative and quantitative evaluation of 8 clear sky models. IEEE transactions on visualization and computer graphics 23, 12 (2016), 2641--2655.
//         https://arxiv.org/pdf/1612.04336.pdf
//     Joseph T. Kider, Daniel Knowlton, Jeremy Newlin, Yining Karl Li, and Donald P. Greenberg. 2014. A framework for the experimental comparison of solar and skydome illumination. ACM Trans. Graph. 33, 6, Article 180 (November 2014), 12 pages. https://doi.org/10.1145/2661229.2661259
//         https://spectralskylight.github.io/Kider2014.pdf
//     Lukas Hosek and Alexander Wilkie. 2012. An analytic model for full spectral sky-dome radiance. ACM Trans. Graph. 31, 4, Article 95 (July 2012), 9 pages. https://doi.org/10.1145/2185520.2185591
//         https://cgg.mff.cuni.cz/projects/SkylightModelling/HosekWilkie_SkylightModel_SIGGRAPH2012_Preprint.pdf
//     A. J. Preetham, Peter Shirley, and Brian Smits. 1999. A practical analytic model for daylight. In Proceedings of the 26th annual conference on Computer graphics and interactive techniques (SIGGRAPH '99). ACM Press/Addison-Wesley Publishing Co., USA, 91–100. https://doi.org/10.1145/311535.311545
//         https://courses.cs.duke.edu/fall01/cps124/resources/p91-preetham.pdf
//     Naty hoffman & Arcot J. Preetham, Photorealistic Real-Time Outdoor Light Scattering, GDC 2002
//         https://renderwonk.com/publications/gdm-2002/GDM_August_2002.pdf
//         https://drivers.amd.com/developer/gdc/GDC02_HoffmanPreetham.pdf
//     https://github.com/ebruneton/clear-sky-models/tree/master
//     https://github.com/spectralskylight
//     https://github.com/wwwtyro/glsl-atmosphere

//==============================================================================
// Mathematical constants.

static const float INF = 1e17;
static const float pi = 3.14159265358979;

//==============================================================================
// Physical constants.

// Astrophysical Constants
// Source: https://pdg.lbl.gov/2023/reviews/rpp2022-rev-astrophysical-constants.pdf

static const float au = 149597870700.0; // Astronomical unit, in m.
static const float Rs = 6.957e8; // Nominal Solar equatorial radius, in m.
static const float Sa = 6.79431e-5; // Solar solid angle from 1 a.u., in sr (=2*pi*(1-sqrt(au^2-rs^2)/au))
static const float Re = 6.3781e6; // Nominal Earth equatorial radius, in m.

static const float Esc = 128e3; // Solar illuminance constant, in lux (https://en.wikipedia.org/wiki/Sunlight).
// NOTE: you want to compute such constants yourself (perhaps you are dealing with
// other planet and/or star), you would need the light source's spectrum (spectral
// radiance, L(λ) in W*sr^-1*m^-2*nm^-1, e.g. AM0 for Sun, see
// https://www.shadertoy.com/view/4XjGDK), its solid angle S as seen from viewer's
// position, and, if you want to work with photometric (perceptual) quantities,
// the lumious efficiency function (see http://www.cvrl.org/cie.htm and
// e.g. https://www.shadertoy.com/view/msXyDH).
// Then
//     Ee =     S*∫     L(λ)*dλ  - irradiance (radiometric quantity, in W/m^2)
//     Ev = 863*S*∫V(λ)*L(λ)*dλ  - illuminance (photometric quantity, in lux)
// and the full color (using color matching functions,
// http://www.cvrl.org/cie.htm), also in lux, is:
//     X = 863*S*∫x(λ)*dλ
//     Y = 863*S*∫y(λ)*dλ
//     Z = 863*S*∫z(λ)*dλ
// NOTE: V(λ)=y(λ). The relative, dimensionless color would be (X,Y,Z)/Ev.
// If you don't need photometric quantities (e.g. you are doing spectral
// render), then you just integrate L(λ) as is. E.g. for the AM0 solar spectrum
// this yields Ee equal to the solar constant Gsc=1361 W/m^2. The spectral
// radiance can be approximated as blackbody spectrum (see e.g.
// https://www.shadertoy.com/view/4XjGDK).

static const float Lsun = Esc / Sa; // Solar luminance, in cd/m^2.

//==============================================================================
// Colorspace.
// Following http://www.thetenthplanet.de/archives/4519, we adopt
// a chromatic adapation space (which we will unimaginatively call
// CAS) with the aim to make the scattering computations *independent* per
// channel, see the post for details.
// The channels in this space ~correspond to point sampling
// the spectrum at 615 nm, 535 nm, and 445 nm, respectively.

static const float3x3 CAS2RGB = float3x3(
     1.6218, -0.4493, 0.0325,
    -0.0374, 1.0598, -0.0742,
    -0.0283, -0.1119, 1.0491);
    
static const float3x3 RGB2CAS = float3x3(
     6.2267e-1, 2.6392e-1, -6.2375e-4,
     2.3324e-2, 9.6056e-1, 6.7215e-2,
     1.9285e-2, 1.0958e-1, 9.6035e-1);

static const float3 CASwavelength = float3(615, 535, 445);

// Sun color in CAS, normalized to unit luminance.
static const float3 CASsun = float3(0.9420, 1.0269, 1.0241);

// Rayleigh volume-scattering coefficient for
// CAS wavelengths, in m^-1, as per
// https://www.shadertoy.com/view/43j3zm.
static const float3 CASrayleigh = float3(7.2865e-6, 1.2863e-5, 2.7408e-5);

//==============================================================================

float2 quadratic_solve(float a, float b, float c)
{
    float d = b * b - a * c;
#if 1
    return d > 0.0 ? (-b + sqrt(d) * float2(-1, +1)) / a : float2(+INF, -INF);
#else
    // Expected to be more accurate.
    if(!(d>0.0)) return float2(+INF,-INF);
    float q=-b+(b<0.0?sqrt(d):-sqrt(d)),l=c/q,h=q/a; // NOT sign(b), in case b=0.
    return float2(min(l,h),max(l,h));
#endif
}

//==============================================================================
// Approximations for erf and erfcx.

#if 0
// From https://www.shadertoy.com/view/ml3yWj
// Eabs ~ 2.8e-8
// Erel ~ 2.8e-8
float erf(float x)
{
    x=clamp(x,-4.0,+4.0);
    float x2=x*x;
    return tanh(x*(1.12837919+x2*(0.275732946+x2*(0.0408672727+x2*0.00200393011)))/(1.0+x2*(0.153282651+x2*(0.0224402472+x2*0.000285807058))));
}

// Erel=2.89760318e-08
float erfcx(float x)
{
    float q=1.0+sqrt(pi)*abs(x);
    float t=abs(x)>1e9?1.0:abs(x)/(1.0+abs(x));
    float p=(1.0+t*(-1.83923738+t*(0.716286914+t*(0.798943258+t*(-0.777020726+t*0.196551435)))))/(1.0+t*(-2.48331366+t*(2.67169669+t*(-1.47041333+t*(0.417875249+t*-0.0403214433)))));
    float y=p/q;
    return x<0.0?2.0*exp(x*x)-y:y;
}
#else
// See https://www.shadertoy.com/view/4XSGzW
// Eabs<0.0037.
float erf(float x)
{
    x = clamp(x, -3.0, +3.0);
    return (1.13072 * x) / (1.0 + (x * x) * (0.357055 + (x * x) * -0.01014));
}

// Eabs<0.00150
// Erel<0.01100
float erfcx(float x)
{
    const float a = 0.4956;
    float y = (1.0 + a * abs(x)) / (1.0 + abs(x) * ((2.0 / sqrt(pi) + a) + sqrt(pi) * a * abs(x)));
    return x >= 0.0 ? y : 2.0 * exp(x * x) - y;
}
#endif

//==============================================================================
// Gaussian integrals.
// Source: https://www.shadertoy.com/view/4XSGzW.

// Compute exp(x)*(erf(z)-erf(y)).
float gauss_segment(float x, float y, float z)
{
    return y * z < 0.0 ?
        exp(x) * (erf(z) - erf(y)) :
        sign(y + z) * (exp(x - y * y) * erfcx(abs(y)) - exp(x - z * z) * erfcx(abs(z)));
}

// ∫ ρ(R,H,k,r(t)) dt on [l;h]
// where
//     r(t) = ro+t*rd
//     ρ    = k*exp(-(r^2-R^2)/(2*R*H)) = k*exp(-h/H-h^2/(2*H*R))
//     h    = |r|-R
float density_integral(float R, float H, float k, float3 ro, float3 rd, float l, float h)
{
    float A = 0.5 / (R * H);
    float B = dot(ro, rd) / (R * H);
    float C = 0.5 * (dot(ro, ro) - R * R) / (R * H);
    float W = 0.25 * B * B / A - C;
    return 0.5 * k * sqrt(pi / A) * gauss_segment(W, sqrt(A) * l + 0.5 * B / sqrt(A), sqrt(A) * h + 0.5 * B / sqrt(A));
}

//==============================================================================
// Phase functions.
// We follow the approach from
//     Johannes Jendersie and Eugene d'Eon. 2023. An Approximate Mie Scattering Function for Fog and Cloud Rendering. In ACM SIGGRAPH 2023 Talks (SIGGRAPH '23). Association for Computing Machinery, New York, NY, USA, Article 47, 1–2. https://doi.org/10.1145/3587421.3595409
//     https://research.nvidia.com/labs/rtr/approximate-mie/publications/approximate-mie.pdf
// and approximate the Mie phase function as a weighted sum of Henyey-Greenstein and Draine
// phase functions.
// NOTE: this reduces to pure Henyey-Greenstein for phase_mie(float4(g,0,0,0),x).

// Draine phase function.
//     B.T. Draine. 2003. Scattering by interstellar dust grains. I. Optical and ultraviolet. The Astrophysical Journal 598, 2 (2003), 1017. https://doi.org/10.1086/379118
// NOTE: this reduces to Henyey-Greenstein for a=0,
// to Rayleigh for g=0, a=1 and to Cornette-Shanks for a=1.
float phase_draine(float a, float g, float x)
{
    float d = 1.0 + g * g - 2.0 * g * x;
    return 1.0 / (4.0 * pi) * (1.0 - g * g) / (1.0 + a * (1.0 + 2.0 * g * g) / 3.0) * // <-- constant factor.
        (1.0 + a * x * x) / (d * sqrt(d));
}

float phase_rayleigh(float x)
{
    return phase_draine(1.0, 0.0, x);
}

// Parametrization by droplet size.
// Input: droplet size, in m.
// Valid range: 5e-6<d<50e-6.
float4 phase_params_mie(float d)
{
    d *= 1e6; // Convert to micrometres.
    return float4(
        exp(-0.0990567 / (d - 1.67154)), // gHG
        exp(-2.20679 / (d + 3.91029) - 0.428934), // gD
        exp(3.62489 - 8.29288 / (d + 5.52825)), // alpha
        exp(-0.599085 / (d - 0.641583) - 0.665888) // wD
    );
}

float phase_mie(float4 M, float x)
{
    return lerp(phase_draine(0.0, M.x, x), phase_draine(M.z, M.y, x), M.w);
}

//==============================================================================
// Convert turbidity (used e.g. by Preetham and Hosek sky models) to Mie 
// volume-scattering coefficient.
// The turbidity is defined as the ratio of vertical optical depths:
//     T=(t_mie+t_rayleigh)/t_rayleigh
// at wavelength 550 nm (since, at least, t_rayleigh is wavelength-dependent).
// From this we get t_mie=(T-1)*t_rayleigh.
// Assuming volume-absorption coefficients are negligible, we get an
// expression for Mie volume-scattering coefficient in terms of
// Rayleigh volume-scattering coefficient (at 550 nm), scale heights
// and turbidity.

// To quote "An analytic model for full spectral sky-dome radiance"
// by Hosek and Wilkie:
// "This allows the user to easily define sky appearance without worrying
// too much about the intricacies of meteorology: T = 2 yields a very clear,
// Arctic-like sky, T = 3 a clear sky in a temperate climate, T = 6 a sky
// on a warm, moist day, T = 10 a slightly hazy day, and values of T above
// 50 represent dense fog."
// NOTE: this description doesn't appear to match the visuals: the
// sky at T>=2 is considerably hazier than the description suggests.
// The reason is not clear, though it may be partially explained by
// Hosek model using a simple Henyey-Greenstein phase function for Mie
// scattering.

// Returns Mie volume-scattering coefficient (scalar, assumed
// wavelength-independent), in m^-1.
float turbidity2mie(float B550, float Hr, float Hm, float T)
{
    // NOTE: the vertical optical depth is just B*H.
    return (T - 1.0) * B550 * Hr / Hm;
}

// Using the reference value of Rayleigh volume-scattering
// coefficient at 550 nm.
// See https://www.shadertoy.com/view/43j3zm.
float turbidity2mie(float Hr, float Hm, float T)
{
    return turbidity2mie(1.149e-5, Hr, Hm, T);
}

//==============================================================================
// Atmospheric scattering.

// The number of samples is 2*NUM_STEPS.
static const int NUM_STEPS = 4;

// Returns color (C, in [0]), and alpha (A, in [1]), of the computed atmospheric scattering,
// which then can be used in the typical premultiplied alpha blending:
//     Cresult=C+(1-A)*Cbackground
//     Aresult=A+(1-A)*Abackground
// except alpha is per-component.
// NOTE: the units are for the reference. You can use other
// set of units, as long as it is consistent (everything in
// parsecs, etc.) - the internals of this function are not
// tied to SI.
float2x3 atmosphere(
    float3 ro, // Ray origin.
    float3 rd, // Ray direction.
    float3 ld, // Direction toward light source.
    float E, // Illuminance from light source, in lux.
    float R, // Planet radius, in m.
    float3 Bsr, float3 Bar, // Rayleigh volume-scattering and volume-absorption coefficients, in m^-1.
    float3 Bsm, float3 Bam, // Mie volume-scattering and volume-absorption coefficients, in m^-1.
    float Hr, float Hm, // Rayleigh and Mie scale height, in m.
    float4 M, // Mie phase function parameters.
    float2 s // Initial integration segment.
    )
{
    ld = normalize(ld);
    rd = normalize(rd);
    float Ra = R + 7.5 * (Hr + Hm); // Effective radius of atmosphere.
    float2 sp = quadratic_solve(1.0, dot(ro, rd), dot(ro, ro) - R * R); // Segment inside planet.
    float2 sa = quadratic_solve(1.0, dot(ro, rd), dot(ro, ro) - Ra * Ra); // Segment inside atmosphere.
    if (sp.x < sp.y && sp.y > s.x)
        s.y = min(s.y, sp.x); // Clamp segment by planet.
    float3 Ber = Bsr + Bar;
    float3 Bem = Bsm + Bam;
    // Compute alpha. Needs to happen while the segment is still only
    // clamped by planet.
    float3 alpha = 1.0 - exp(-(
            Ber * density_integral(R, Hr, 1.0, ro, rd, s.x, s.y) +
            Bem * density_integral(R, Hm, 1.0, ro, rd, s.x, s.y)));
    s = float2(max(s.x, sa.x), min(s.y, sa.y)); // Clamp segment by atmosphere.
    float3 po = ro - dot(ro, ld) * ld;
    float3 pd = rd - dot(rd, ld) * ld;
    float2 ss = quadratic_solve(dot(pd, pd), dot(po, pd), dot(po, po) - R * R); // Segment inside planet's shadow.
    if (dot(rd, ld) > 0.0)
        ss.y = min(ss.y, -dot(ro, ld) / dot(rd, ld)); // Shadow is only half
    else
        ss.x = max(ss.x, -dot(ro, ld) / dot(rd, ld)); // of the cylinder.
    float2 sl = s, sh = s.y;
    if (ss.x < ss.y)
        sl = float2(s.x, min(s.y, ss.x)); // Possibly split segment
    if (ss.x < ss.y)
        sh = float2(max(s.x, ss.y), s.y); // into two by shadow.
    // If there's only one real segment, split it in half, so there
    // are always 2.
    if (!(sl.x < sl.y))
    {
        sl = float2(sh.x, 0.5 * (sh.x + sh.y));
        sh = float2(0.5 * (sh.x + sh.y), sh.y);
    }
    else if (!(sh.x < sh.y))
    {
        sh = float2(0.5 * (sl.x + sl.y), sl.y);
        sl = float2(sl.x, 0.5 * (sl.x + sl.y));
    }
    float3 Cr = 0;
    float3 Cm = 0;
    float Pr = phase_rayleigh(dot(rd, ld));
    float Pm = phase_mie(M, dot(rd, ld));
    // Integrate both segments.
    //s = sl;
    for (int k = 0; k < 1; ++k)
    {
        if (!(s.x < s.y))
            continue;
        // Find the step size, and estimate the optimized offset.
        float dt = (s.y - s.x) / float(NUM_STEPS);
        float3 B = Ber + Bem;
        float X = max(max(B.x, B.y), B.z) * dt;
        // The offset is chosen to provide exact answer
        // for integrating exp(-(X/dt)*t) on [0;dt].
        float offset = (X < 0.25 ? 0.5 - X / 24.0 : log(X / (1.0 - exp(-X))) / X);
        if (k == 1)
            offset = 1.0 - offset;
        for (int i = 0; i < NUM_STEPS; ++i)
        {
            float t = s.x + (float(i) + offset) * dt;
            float3 r = ro + rd * t;
            float h = (dot(r, r) - R * R) / (2.0 * R);
            float Dr = density_integral(R, Hr, 1.0, ro, rd, 0.0, t); // + density_integral(R, Hr, 1.0, r, ld, 0.0, INF);
            float Dm = density_integral(R, Hm, 1.0, ro, rd, 0.0, t);// + density_integral(R, Hm, 1.0, r, ld, 0.0, INF);
            float3 a = exp(-(Ber * Dr + Bem * Dm));
            Cr += exp(-h / Hr) * a * dt;
            Cm += exp(-h / Hm) * a * dt;
        }
        s = sh;
    }
    return float2x3(E * (Pr * Bsr * Cr + Pm * Bsm * Cm), alpha);
}

//==============================================================================
// Main image.

//void mainImage(out float4 fragColor, in float2 fragCoord)
//{
//    float F = 1.5;
//    float R = Re;
//    float3 ro = float3(0, R + 80.0, 5.0);
//    float3 rd = normalize(float3((2.0 * fragCoord - iResolution.xy) / iResolution.y, -F));
//    float3 md = normalize(float3((2.0 * iMouse.xy - iResolution.xy) / iResolution.y, -F));
//    float3 ld = normalize(float3(1, sin(0.5 * iTime) + 0.875, -2));
//    if (length(iMouse.xy) > 16.0)
//        ld = md;
//    float Hr = 7.994e3, Hm = 1.2e3;
//    if (texture(iChannel0, float2(49.5, 0.5) / float2(256, 3)).x > 0.0)
//        ro.z = sqrt(R * Hr);
//    if (texture(iChannel0, float2(50.5, 0.5) / float2(256, 3)).x > 0.0)
//        ro.z = R / 16.0;
//    if (texture(iChannel0, float2(51.5, 0.5) / float2(256, 3)).x > 0.0)
//        ro.z = R / 8.0;
//    if (texture(iChannel0, float2(52.5, 0.5) / float2(256, 3)).x > 0.0)
//        ro.z = 4.0 * R;
//    float T = 1.375; // Turbidity.
//    float D = 17e-6; // Droplet size, in m.
//    float3 Bsr = CASrayleigh, Bar = float3(0);
//    float3 Bsm = float3(turbidity2mie(Hr, Hm, T)), Bam = float3(0); //  NOTE: glsl-atmosphere uses Bsm=21e-6
//    float4 M = phase_params_mie(D);
//    // From https://en.wikipedia.org/wiki/Orders_of_magnitude_(illuminance)#Luminance
//    //     5e3 - Typical photographic scene in full sunlight
//    //     7e3 - Average clear sky
//    float Lref = 7e3; // Scale luminance, in cd/m^2.
//    // NOTE: we work entirely in CAS, and only convert to RGB right
//    // before the tonemapping.
//    float3 col = float3(1e-7); // Airglow.
//    float2 s = quadratic_solve(1.0, dot(ro, rd), dot(ro, ro) - R * R);
//    // Render Sun.
//    if (true)
//        col += Lsun * CASsun * smoothstep(1.0 - (0.5 * Rs * Rs / (au * au)), 1.0, dot(rd, ld));
//    else
//        col += Lsun * CASsun * exp(-(1.0 - dot(rd, ld)) / (2.0 * Rs * Rs / (au * au)));
//    // Render planet.
//    if (s.x < s.y && s.y > 0.0)
//    {
//        float3 a = exp(-(Bsr + Bar) * density_integral(R, Hr, 1.0, ro, rd, 0.0, s.x) + (Bsm + Bam) * density_integral(R, Hm, 1.0, ro, rd, 0.0, s.x));
//        col = Esc / pi * CASsun * a * (1.0 / 128.0 + max(dot(ld, normalize(ro + s.x * rd)), 0.0)) * (RGB2CAS * float3(0.125, 0.25, 0.0625));
//    }
//    float2x3 m = atmosphere(ro, normalize(rd), normalize(ld), Esc, R, Bsr, Bar, Bsm, Bam, Hr, Hm, M, float2(0.0, INF));
//    col = CASsun * m[0] + col * (1.0 - m[1]);
//    col = CAS2RGB * col;
//    // Apply exposure.
//    col = 1.0 - exp(-col / Lref);
//    col = lerp(12.92 * col, 1.055 * pow(col, float3(1.0 / 2.4)) - 0.055, step(0.0031308, col)); // sRGB
//    
//    float3x3 t = float3x3(0., 0., 0., 0., 0., 0., 0.1, 0.5, 0.3);
//    
//    fragColor = float4(col, 1.0);
//    //fragColor.rgb = t * float3(0., 0., 1.);
//}

void CalcAtmosphere(inout float3 color, in float3 viewDir, in float3 sunDir, in float altitude, in float far, in float3 sunColor)
{
    float R = Re;
    float3 ro = float3(0, R + 80.0, 5.0);
    float3 rd = normalize(viewDir);
    float3 ld = normalize(sunDir);
    float Hr = 7.994e3, Hm = 1.2e3;
    float T = 1.375; // Turbidity.
    float D = 17e-6; // Droplet size, in m.
    float3 Bsr = CASrayleigh, Bar = 0;
    float3 Bsm = turbidity2mie(Hr, Hm, T), Bam = 0; //  NOTE: glsl-atmosphere uses Bsm=21e-6
    float4 M = phase_params_mie(D);
    // From https://en.wikipedia.org/wiki/Orders_of_magnitude_(illuminance)#Luminance
    //     5e3 - Typical photographic scene in full sunlight
    //     7e3 - Average clear sky
    float Lref = 7e3; // Scale luminance, in cd/m^2.
    // NOTE: we work entirely in CAS, and only convert to RGB right
    // before the tonemapping.
    float3 col = 1e-7; // Airglow.
    float2 s = quadratic_solve(1.0, dot(ro, rd), dot(ro, ro) - R * R);
    // Render Sun.
    //if (true)
    //    col += Lsun * CASsun * smoothstep(1.0 - (0.5 * Rs * Rs / (au * au)), 1.0, dot(rd, ld));
    //else
    //    col += Lsun * CASsun * exp(-(1.0 - dot(rd, ld)) / (2.0 * Rs * Rs / (au * au)));
    // Render planet.
    if (far < INF)
    {
        col = mul(RGB2CAS, color);
    }
    else
    {
        far = INF;
    }
    //if (s.x < s.y && s.y > 0.0)
    //{
    //    float3 a = exp(-(Bsr + Bar) * density_integral(R, Hr, 1.0, ro, rd, 0.0, s.x) + (Bsm + Bam) * density_integral(R, Hm, 1.0, ro, rd, 0.0, s.x));
    //    col = Esc / pi * CASsun * a * (1.0 / 128.0 + max(dot(ld, normalize(ro + s.x * rd)), 0.0)) * (mul(RGB2CAS, float3(0.125, 0.25, 0.0625)));
    //}
    float2x3 m = atmosphere(ro, normalize(rd), normalize(ld), 4., R, Bsr, Bar, Bsm, Bam, Hr, Hm, M, float2(0.0, far));
    col = sunColor * m[0] + col * (1.0 - m[1]);
    col = mul(CAS2RGB, col);
    // Apply exposure.
    //col = 1.0 - exp(-col / Lref);
    color = col;
    //col = lerp(12.92 * col, 1.055 * pow(col, float3(1.0 / 2.4)) - 0.055, step(0.0031308, col)); // sRGB
    //
    //float3x3 t = float3x3(0., 0., 0., 0., 0., 0., 0.1, 0.5, 0.3);
    //Atmosphere atmosphere;
    //atmosphere.earthRadius = 6360.e3;
    //atmosphere.atmosphereRadius = 6420.e3;
    //atmosphere.Hr = 7994.;
    //atmosphere.Hm = 1200.;
    //atmosphere.betaR = float3(5.8e-6, 13.5e-6, 33.1e-6);
    //atmosphere.betaR = float3(7.2865e-6, 1.2863e-5, 2.7408e-5);
    ////atmosphere.betaR = float3(8.658882115850087e-6, 1.5119838839850446e-5, 3.15880085046994e-5);
    //atmosphere.betaM = (float3) 21.e-6;
    ////atmosphere.irradiance = float3(1.4662835232339013, 1.7557753193321481, 1.7149614362178376);
    //
    //atmosphere.sunDirection = sunDir;
    //float3 light = ComputeIncidentLight(atmosphere, float3(0., atmosphere.earthRadius + 2500. + altitude, 0.), viewDir, 0., far);
    ////light = light.x * float3(0.933600, 0.358317, 0.000000) + light.y * float3(0.413697, 0.910408, 0.003489) + light.z * float3(0.178639, 0.025820, 0.983576);
    ////light = light.x * float3(0.899398, 0.437131, 0.000009) + light.y * float3(0.256233, 0.966471, 0.016686) + light.z * float3(0.175352, 0.029461, 0.984065);
    ////light = max(0., light.x * float3(3.240970, -0.969244, 0.055630) + light.y * float3(-1.537383, 1.875968, -0.203977) + light.z * float3(-0.498611, 0.041555, 1.056972));
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
    CalcAtmosphere(color, viewDir, sunDir, altitude, INF, 1.);

}

#endif
