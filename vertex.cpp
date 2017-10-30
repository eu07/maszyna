/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "vertex.h"

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
