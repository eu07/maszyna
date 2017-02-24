#pragma once

#include "float3d.h"

inline
float3
XYZtoRGB( float3 const &XYZ ) {

    // M^-1 for Adobe RGB from http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
//  float const mi[ 3 ][ 3 ] = { 2.041369, -0.969266, 0.0134474, -0.5649464, 1.8760108, -0.1183897, -0.3446944, 0.041556, 1.0154096 };
    // m^-1 for sRGB:
    float const mi[ 3 ][ 3 ] = { 3.240479, -0.969256, 0.055648, -1.53715, 1.875991, -0.204043, -0.49853, 0.041556, 1.057311 };

    return float3{
        XYZ.x*mi[ 0 ][ 0 ] + XYZ.y*mi[ 1 ][ 0 ] + XYZ.z*mi[ 2 ][ 0 ],
        XYZ.x*mi[ 0 ][ 1 ] + XYZ.y*mi[ 1 ][ 1 ] + XYZ.z*mi[ 2 ][ 1 ],
        XYZ.x*mi[ 0 ][ 2 ] + XYZ.y*mi[ 1 ][ 2 ] + XYZ.z*mi[ 2 ][ 2 ] };
}

inline
float3
RGBtoHSV( float3 const &RGB ) {

    float3 hsv;

    float const max = std::max( std::max( RGB.x, RGB.y ), RGB.z );
    float const min = std::min( std::min( RGB.x, RGB.y ), RGB.z );
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
    if( RGB.x >= max )                           // > is bogus, just keeps compilor happy
        hsv.x = ( RGB.y - RGB.z ) / delta;        // between yellow & magenta
    else
        if( RGB.y >= max )
            hsv.x = 2.0 + ( RGB.y - RGB.x ) / delta;  // between cyan & yellow
        else
            hsv.x = 4.0 + ( RGB.x - RGB.y ) / delta;  // between magenta & cyan

    hsv.x *= 60.0;                              // degrees

    if( hsv.x < 0.0 )
        hsv.x += 360.0;

    return hsv;
}

inline
float3
HSVtoRGB( float3 const &HSV ) {

    float3 rgb;

    if( HSV.y <= 0.0 ) {       // < is bogus, just shuts up warnings
        rgb.x = HSV.z;
        rgb.y = HSV.z;
        rgb.z = HSV.z;
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
            rgb.x = HSV.z;
            rgb.y = t;
            rgb.z = p;
            break;
        case 1:
            rgb.x = q;
            rgb.y = HSV.z;
            rgb.z = p;
            break;
        case 2:
            rgb.x = p;
            rgb.y = HSV.z;
            rgb.z = t;
            break;

        case 3:
            rgb.x = p;
            rgb.y = q;
            rgb.z = HSV.z;
            break;
        case 4:
            rgb.x = t;
            rgb.y = p;
            rgb.z = HSV.z;
            break;
        case 5:
        default:
            rgb.x = HSV.z;
            rgb.y = p;
            rgb.z = q;
            break;
    }
    return rgb;
}
