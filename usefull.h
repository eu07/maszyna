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

template <typename Type_>
void SafeDelete( Type_ &Pointer ) {
    delete Pointer;
    Pointer = nullptr;
}

template <typename Type_>
void SafeDeleteArray( Type_ &Pointer ) {
    delete[] Pointer;
    Pointer = nullptr;
}

template <typename Type_>
Type_
clamp( Type_ const Value, Type_ const Min, Type_ const Max ) {

    Type_ value = Value;
    if( value < Min ) { value = Min; }
    if( value > Max ) { value = Max; }
    return value;
}

// keeps the provided value in specified range 0-Range, as if the range was circular buffer
template <typename Type_>
Type_
clamp_circular( Type_ Value, Type_ const Range = static_cast<Type_>(360) ) {

    Value -= Range * (int)( Value / Range ); // clamp the range to 0-360
    if( Value < 0.0 ) Value += Range;

    return Value;
}

template <typename Type_>
Type_
interpolate( Type_ const First, Type_ const Second, float const Factor ) {

    return ( First * ( 1.0f - Factor ) ) + ( Second * Factor );
}

//---------------------------------------------------------------------------
