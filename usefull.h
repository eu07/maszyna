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
#define global_texture_path "textures/"

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
interpolate( Type_ const &First, Type_ const &Second, float const Factor ) {

    return ( First * ( 1.0f - Factor ) ) + ( Second * Factor );
}

template <typename Type_>
Type_
interpolate( Type_ const &First, Type_ const &Second, double const Factor ) {

    return ( First * ( 1.0 - Factor ) ) + ( Second * Factor );
}

// tests whether provided points form a degenerate triangle
template <typename VecType_>
bool
degenerate( VecType_ const &Vertex1, VecType_ const &Vertex2, VecType_ const &Vertex3 ) {

    //  degenerate( A, B, C, minarea ) = ( ( B - A ).cross( C - A ) ).lengthSquared() < ( 4.0f * minarea * minarea );
    return ( glm::length2( glm::cross( Vertex2 - Vertex1, Vertex3 - Vertex1 ) ) == 0.0 );
}

// calculates bounding box for provided set of points
template <class Iterator_, class VecType_>
void
bounding_box( VecType_ &Mincorner, VecType_ &Maxcorner, Iterator_ First, Iterator_ Last ) {

    Mincorner = VecType_( std::numeric_limits<typename VecType_::value_type>::max() );
    Maxcorner = VecType_( std::numeric_limits<typename VecType_::value_type>::lowest() );

    std::for_each(
        First, Last,
        [&]( typename Iterator_::value_type &point ) {
            Mincorner = glm::min( Mincorner, VecType_{ point } );
            Maxcorner = glm::max( Maxcorner, VecType_{ point } ); } );
}

//---------------------------------------------------------------------------
