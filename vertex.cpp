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
#include "vertex.h"
#include "sn_utils.h"

void
world_vertex::serialize( std::ostream &s ) const {

    sn_utils::ls_float64( s, position.x );
    sn_utils::ls_float64( s, position.y );
    sn_utils::ls_float64( s, position.z );

    sn_utils::ls_float32( s, normal.x );
    sn_utils::ls_float32( s, normal.y );
    sn_utils::ls_float32( s, normal.z );

    sn_utils::ls_float32( s, texture.x );
    sn_utils::ls_float32( s, texture.y );
}

void
world_vertex::deserialize( std::istream &s ) {

    position.x = sn_utils::ld_float64( s );
    position.y = sn_utils::ld_float64( s );
    position.z = sn_utils::ld_float64( s );

    normal.x = sn_utils::ld_float32( s );
    normal.y = sn_utils::ld_float32( s );
    normal.z = sn_utils::ld_float32( s );

    texture.x = sn_utils::ld_float32( s );
    texture.y = sn_utils::ld_float32( s );
}

template <>
world_vertex &
world_vertex::operator+=( world_vertex const &Right ) {

    position += Right.position;
    normal += Right.normal;
    texture += Right.texture;
    return *this;
}

template <>
world_vertex &
world_vertex::operator*=( world_vertex const &Right ) {

    position *= Right.position;
    normal *= Right.normal;
    texture *= Right.texture;
    return *this;
}

//---------------------------------------------------------------------------
