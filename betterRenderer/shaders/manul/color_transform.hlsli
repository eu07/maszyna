#ifndef COLOR_TRANSFORM_HLSLI
#define COLOR_TRANSFORM_HLSLI

float3 ACEScg_to_XYZ(in float3 c);
float3 REC709_to_XYZ(in float3 c);
float3 AdobeRGB_to_XYZ(in float3 c);
float3 XYZ_to_REC2020(in float3 c);
float3 XYZ_to_REC709(in float3 c);
float3 ACEScg_to_REC709(in float3 c);

float3 ACEScg_to_XYZ(in float3 c){
    return
        c.r * float3(0.66245418, 0.27222872, -0.00557465) + 
        c.g * float3(0.13400421, 0.67408177, 0.00406073) + 
        c.b * float3(0.15618769, 0.05368952, 1.0103391);
}

float3 REC709_to_XYZ(in float3 c) {
    return
        c.r * float3(0.4123908, 0.21263901, 0.01933082) + 
        c.g * float3(0.35758434, 0.71516868, 0.11919478) + 
        c.b * float3(0.18048079, 0.07219232, 0.95053215);
}

float3 AdobeRGB_to_XYZ(in float3 c) {
    return
        c.r * float3(0.57667, 0.29734, 0.02703) + 
        c.g * float3(0.18556, 0.62736, 0.07069) + 
        c.b * float3(0.18823, 0.07529, 0.99134);
}

float3 XYZ_to_REC2020(in float3 c) {
    return
        c.r * float3(1.71665119, -0.66668435, 0.01763986) + 
        c.g * float3(-0.35567078, 1.61648124, -0.04277061) + 
        c.b * float3(-0.25336628, 0.01576855, 0.94210312);
}

float3 XYZ_to_REC709(in float3 c) {
    return
        c.r * float3(3.24096994, -0.96924364, 0.05563008) + 
        c.g * float3(-1.53738318, 1.8759675, -0.20397696) + 
        c.b * float3(-0.49861076, 0.04155506, 1.05697151);
}

float3 ACEScg_to_REC709(in float3 c) {
    return
        c.r * float3(1.70507964, -0.12970053, -0.02416634) + 
        c.g * float3(-0.62423346, 1.13846855, -0.12461416) + 
        c.b * float3(-0.08084618, -0.00876802, 1.1487805);
}

float REC709_to_Luma(in float3 c) {
    return dot(c, float3(.2126, .7152, .0722));
}

float XYZ_to_Luma(in float3 c) {
    return REC709_to_Luma(XYZ_to_REC709(c));
}

#endif