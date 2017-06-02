/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "stdafx.h"

#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define DegToRad(a) ((M_PI / 180.0) * (a)) //(a) w nawiasie, bo może być dodawaniem
#define RadToDeg(r) ((180.0 / M_PI) * (r))

#define asModelsPath std::string("models\\")
#define asSceneryPath std::string("scenery\\")
#define szSceneryPath "scenery\\"
#define szTexturePath "textures\\"
#define szSoundPath "sounds\\"

#define MAKE_ID4(a,b,c,d) (((std::uint32_t)(d)<<24)|((std::uint32_t)(c)<<16)|((std::uint32_t)(b)<<8)|(std::uint32_t)(a))

template <typename _Type>
void SafeDelete( _Type &Pointer ) {
    delete Pointer;
    Pointer = nullptr;
}

template <typename _Type>
void SafeDeleteArray( _Type &Pointer ) {
    delete[] Pointer;
    Pointer = nullptr;
}

template <typename _Type>
_Type
clamp( _Type const Value, _Type const Min, _Type const Max ) {

    _Type value = Value;
    if( value < Min ) { value = Min; }
    if( value > Max ) { value = Max; }
    return value;
}

// keeps the provided value in specified range 0-Range, as if the range was circular buffer
template <typename _Type>
_Type
clamp_circular( _Type Value, _Type const Range = static_cast<_Type>(360) ) {

    Value -= Range * (int)( Value / Range ); // clamp the range to 0-360
    if( Value < 0.0 ) Value += Range;

    return Value;
}

template <typename _Type>
_Type
interpolate( _Type const First, _Type const Second, float const Factor ) {

    return ( First * ( 1.0f - Factor ) ) + ( Second * Factor );
}

//---------------------------------------------------------------------------
