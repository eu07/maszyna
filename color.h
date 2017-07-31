#pragma once

#include "float3d.h"

inline
glm::vec3
XYZtoRGB( glm::vec3 const &XYZ ) {

    // M^-1 for Adobe RGB from http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
    float const mi[ 3 ][ 3 ] = { 2.041369f, -0.969266f, 0.0134474f, -0.5649464f, 1.8760108f, -0.1183897f, -0.3446944f, 0.041556f, 1.0154096f };
    // m^-1 for sRGB:
//    float const mi[ 3 ][ 3 ] = { 3.240479f, -0.969256f, 0.055648f, -1.53715f, 1.875991f, -0.204043f, -0.49853f, 0.041556f, 1.057311f };

    return glm::vec3{
        XYZ.x*mi[ 0 ][ 0 ] + XYZ.y*mi[ 1 ][ 0 ] + XYZ.z*mi[ 2 ][ 0 ],
        XYZ.x*mi[ 0 ][ 1 ] + XYZ.y*mi[ 1 ][ 1 ] + XYZ.z*mi[ 2 ][ 1 ],
        XYZ.x*mi[ 0 ][ 2 ] + XYZ.y*mi[ 1 ][ 2 ] + XYZ.z*mi[ 2 ][ 2 ] };
}

inline
glm::vec3
RGBtoHSV( glm::vec3 const &RGB ) {

    glm::vec3 hsv;

    float const max = std::max( std::max( RGB.r, RGB.g ), RGB.b );
    float const min = std::min( std::min( RGB.r, RGB.g ), RGB.b );
    float const delta = max - min;

    hsv.z = max;                                // v
    if( delta < 0.00001 ) {
        hsv.y = 0;
        hsv.x = 0; // undefined, maybe nan?
        return hsv;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        hsv.y = ( delta / max );                  // s
    }
    else {
        // if max is 0, then r = g = b = 0              
        // s = 0, v is undefined
        hsv.y = 0.0;
        hsv.x = NAN;                            // its now undefined
        return hsv;
    }
    if( RGB.r >= max )                           // > is bogus, just keeps compilor happy
        hsv.x = ( RGB.g - RGB.b ) / delta;        // between yellow & magenta
    else
        if( RGB.g >= max )
            hsv.x = 2.0 + ( RGB.g - RGB.r ) / delta;  // between cyan & yellow
        else
            hsv.x = 4.0 + ( RGB.r - RGB.g ) / delta;  // between magenta & cyan

    hsv.x *= 60.0;                              // degrees

    if( hsv.x < 0.0 )
        hsv.x += 360.0;

    return hsv;
}

inline
glm::vec3
HSVtoRGB( glm::vec3 const &HSV ) {

    glm::vec3 rgb;

    if( HSV.y <= 0.0 ) {       // < is bogus, just shuts up warnings
        rgb.r = HSV.z;
        rgb.g = HSV.z;
        rgb.b = HSV.z;
        return rgb;
    }
    float hh = HSV.x;
    if( hh >= 360.0 ) hh = 0.0;
    hh /= 60.0;
    int const i = (int)hh;
    float const ff = hh - i;
    float const p = HSV.z * ( 1.0 - HSV.y );
    float const q = HSV.z * ( 1.0 - ( HSV.y * ff ) );
    float const t = HSV.z * ( 1.0 - ( HSV.y * ( 1.0 - ff ) ) );

    switch( i ) {
        case 0:
            rgb.r = HSV.z;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = HSV.z;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = HSV.z;
            rgb.b = t;
            break;

        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = HSV.z;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = HSV.z;
            break;
        case 5:
        default:
            rgb.r = HSV.z;
            rgb.g = p;
            rgb.b = q;
            break;
    }
    return rgb;
}
