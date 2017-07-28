/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "frustum.h"

std::vector<glm::vec4> ndcfrustumshapepoints = {
    { -1, -1, -1, 1 }, { 1, -1, -1, 1 }, { 1, 1, -1, 1 }, { -1, 1, -1, 1 }, // z-near
    { -1, -1,  1, 1 }, { 1, -1,  1, 1 }, { 1, 1,  1, 1 }, { -1, 1,  1, 1 } }; // z-far

void
cFrustum::calculate() {

    auto const &projection = OpenGLMatrices.data( GL_PROJECTION );
    auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );

    calculate( projection, modelview );
}

void
cFrustum::calculate( glm::mat4 const &Projection, glm::mat4 const &Modelview ) {

    float const *proj = &Projection[ 0 ][ 0 ];
    float const *modl = &Modelview[ 0 ][ 0 ];
    float clip[ 16 ];

    // multiply the matrices to retrieve clipping planes
    clip[ 0 ] = modl[ 0 ] * proj[ 0 ] + modl[ 1 ] * proj[ 4 ] + modl[ 2 ] * proj[ 8 ] + modl[ 3 ] * proj[ 12 ];
    clip[ 1 ] = modl[ 0 ] * proj[ 1 ] + modl[ 1 ] * proj[ 5 ] + modl[ 2 ] * proj[ 9 ] + modl[ 3 ] * proj[ 13 ];
    clip[ 2 ] = modl[ 0 ] * proj[ 2 ] + modl[ 1 ] * proj[ 6 ] + modl[ 2 ] * proj[ 10 ] + modl[ 3 ] * proj[ 14 ];
    clip[ 3 ] = modl[ 0 ] * proj[ 3 ] + modl[ 1 ] * proj[ 7 ] + modl[ 2 ] * proj[ 11 ] + modl[ 3 ] * proj[ 15 ];

    clip[ 4 ] = modl[ 4 ] * proj[ 0 ] + modl[ 5 ] * proj[ 4 ] + modl[ 6 ] * proj[ 8 ] + modl[ 7 ] * proj[ 12 ];
    clip[ 5 ] = modl[ 4 ] * proj[ 1 ] + modl[ 5 ] * proj[ 5 ] + modl[ 6 ] * proj[ 9 ] + modl[ 7 ] * proj[ 13 ];
    clip[ 6 ] = modl[ 4 ] * proj[ 2 ] + modl[ 5 ] * proj[ 6 ] + modl[ 6 ] * proj[ 10 ] + modl[ 7 ] * proj[ 14 ];
    clip[ 7 ] = modl[ 4 ] * proj[ 3 ] + modl[ 5 ] * proj[ 7 ] + modl[ 6 ] * proj[ 11 ] + modl[ 7 ] * proj[ 15 ];

    clip[ 8 ] = modl[ 8 ] * proj[ 0 ] + modl[ 9 ] * proj[ 4 ] + modl[ 10 ] * proj[ 8 ] + modl[ 11 ] * proj[ 12 ];
    clip[ 9 ] = modl[ 8 ] * proj[ 1 ] + modl[ 9 ] * proj[ 5 ] + modl[ 10 ] * proj[ 9 ] + modl[ 11 ] * proj[ 13 ];
    clip[ 10 ] = modl[ 8 ] * proj[ 2 ] + modl[ 9 ] * proj[ 6 ] + modl[ 10 ] * proj[ 10 ] + modl[ 11 ] * proj[ 14 ];
    clip[ 11 ] = modl[ 8 ] * proj[ 3 ] + modl[ 9 ] * proj[ 7 ] + modl[ 10 ] * proj[ 11 ] + modl[ 11 ] * proj[ 15 ];

    clip[ 12 ] = modl[ 12 ] * proj[ 0 ] + modl[ 13 ] * proj[ 4 ] + modl[ 14 ] * proj[ 8 ] + modl[ 15 ] * proj[ 12 ];
    clip[ 13 ] = modl[ 12 ] * proj[ 1 ] + modl[ 13 ] * proj[ 5 ] + modl[ 14 ] * proj[ 9 ] + modl[ 15 ] * proj[ 13 ];
    clip[ 14 ] = modl[ 12 ] * proj[ 2 ] + modl[ 13 ] * proj[ 6 ] + modl[ 14 ] * proj[ 10 ] + modl[ 15 ] * proj[ 14 ];
    clip[ 15 ] = modl[ 12 ] * proj[ 3 ] + modl[ 13 ] * proj[ 7 ] + modl[ 14 ] * proj[ 11 ] + modl[ 15 ] * proj[ 15 ];

    // get the sides of the frustum.
    m_frustum[ side_RIGHT ][ plane_A ] = clip[ 3 ] - clip[ 0 ];
    m_frustum[ side_RIGHT ][ plane_B ] = clip[ 7 ] - clip[ 4 ];
    m_frustum[ side_RIGHT ][ plane_C ] = clip[ 11 ] - clip[ 8 ];
    m_frustum[ side_RIGHT ][ plane_D ] = clip[ 15 ] - clip[ 12 ];
    normalize_plane( side_RIGHT );

    m_frustum[ side_LEFT ][ plane_A ] = clip[ 3 ] + clip[ 0 ];
    m_frustum[ side_LEFT ][ plane_B ] = clip[ 7 ] + clip[ 4 ];
    m_frustum[ side_LEFT ][ plane_C ] = clip[ 11 ] + clip[ 8 ];
    m_frustum[ side_LEFT ][ plane_D ] = clip[ 15 ] + clip[ 12 ];
    normalize_plane( side_LEFT );

    m_frustum[ side_BOTTOM ][ plane_A ] = clip[ 3 ] + clip[ 1 ];
    m_frustum[ side_BOTTOM ][ plane_B ] = clip[ 7 ] + clip[ 5 ];
    m_frustum[ side_BOTTOM ][ plane_C ] = clip[ 11 ] + clip[ 9 ];
    m_frustum[ side_BOTTOM ][ plane_D ] = clip[ 15 ] + clip[ 13 ];
    normalize_plane( side_BOTTOM );

    m_frustum[ side_TOP ][ plane_A ] = clip[ 3 ] - clip[ 1 ];
    m_frustum[ side_TOP ][ plane_B ] = clip[ 7 ] - clip[ 5 ];
    m_frustum[ side_TOP ][ plane_C ] = clip[ 11 ] - clip[ 9 ];
    m_frustum[ side_TOP ][ plane_D ] = clip[ 15 ] - clip[ 13 ];
    normalize_plane( side_TOP );

    m_frustum[ side_BACK ][ plane_A ] = clip[ 3 ] - clip[ 2 ];
    m_frustum[ side_BACK ][ plane_B ] = clip[ 7 ] - clip[ 6 ];
    m_frustum[ side_BACK ][ plane_C ] = clip[ 11 ] - clip[ 10 ];
    m_frustum[ side_BACK ][ plane_D ] = clip[ 15 ] - clip[ 14 ];
    normalize_plane( side_BACK );

    m_frustum[ side_FRONT ][ plane_A ] = clip[ 3 ] + clip[ 2 ];
    m_frustum[ side_FRONT ][ plane_B ] = clip[ 7 ] + clip[ 6 ];
    m_frustum[ side_FRONT ][ plane_C ] = clip[ 11 ] + clip[ 10 ];
    m_frustum[ side_FRONT ][ plane_D ] = clip[ 15 ] + clip[ 14 ];
    normalize_plane( side_FRONT );

    m_inversetransformation = glm::inverse( Projection * Modelview );

    // calculate frustum corners
    m_frustumpoints = ndcfrustumshapepoints;
    transform_to_world(
        std::begin( m_frustumpoints ),
        std::end( m_frustumpoints ) );
}

bool
cFrustum::point_inside( float const X, float const Y, float const Z ) const {

    // cycle through the sides of the frustum, checking if the point is behind them
    for( int idx = 0; idx < 6; ++idx ) {
        if( m_frustum[ idx ][ plane_A ] * X
            + m_frustum[ idx ][ plane_B ] * Y
            + m_frustum[ idx ][ plane_C ] * Z
            + m_frustum[ idx ][ plane_D ] <= 0 )
            return false;
    }

    // the point is in front of each frustum plane, i.e. inside of the frustum
    return true;
}

float
cFrustum::sphere_inside( float const X, float const Y, float const Z, float const Radius ) const {

    float distance;
    // go through all the sides of the frustum. bail out as soon as possible
    for( int idx = 0; idx < 6; ++idx ) {
        distance =
            m_frustum[ idx ][ plane_A ] * X
            + m_frustum[ idx ][ plane_B ] * Y
            + m_frustum[ idx ][ plane_C ] * Z
            + m_frustum[ idx ][ plane_D ];
        if( distance <= -Radius )
            return 0.0f;
    }
    return distance + Radius;
}

bool
cFrustum::cube_inside( float const X, float const Y, float const Z, float const Size ) const {

    for( int idx = 0; idx < 6; ++idx ) {
        if( m_frustum[ idx ][ plane_A ] * ( X - Size )
            + m_frustum[ idx ][ plane_B ] * ( Y - Size )
            + m_frustum[ idx ][ plane_C ] * ( Z - Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X + Size )
            + m_frustum[ idx ][ plane_B ] * ( Y - Size )
            + m_frustum[ idx ][ plane_C ] * ( Z - Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X - Size )
            + m_frustum[ idx ][ plane_B ] * ( Y + Size )
            + m_frustum[ idx ][ plane_C ] * ( Z - Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X + Size )
            + m_frustum[ idx ][ plane_B ] * ( Y + Size )
            + m_frustum[ idx ][ plane_C ] * ( Z - Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X - Size )
            + m_frustum[ idx ][ plane_B ] * ( Y - Size )
            + m_frustum[ idx ][ plane_C ] * ( Z + Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X + Size )
            + m_frustum[ idx ][ plane_B ] * ( Y - Size )
            + m_frustum[ idx ][ plane_C ] * ( Z + Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X - Size )
            + m_frustum[ idx ][ plane_B ] * ( Y + Size )
            + m_frustum[ idx ][ plane_C ] * ( Z + Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;
        if( m_frustum[ idx ][ plane_A ] * ( X + Size )
            + m_frustum[ idx ][ plane_B ] * ( Y + Size )
            + m_frustum[ idx ][ plane_C ] * ( Z + Size )
            + m_frustum[ idx ][ plane_D ] > 0 )
            continue;

        return false;
    }

    return true;
}

std::vector<std::size_t> const frustumshapepoinstorder{ 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

// debug helper, draws shape of frustum in world space
void
cFrustum::draw( glm::vec3 const &Offset ) const {

    ::glBegin( GL_LINES );
    for( auto const pointindex : frustumshapepoinstorder ) {
        ::glVertex3fv( glm::value_ptr( glm::vec3{ m_frustumpoints[ pointindex ] } - Offset ) );
    }
    ::glEnd();
}

void cFrustum::normalize_plane( cFrustum::side const Side ) {

    float magnitude =
        std::sqrt(
            m_frustum[ Side ][ plane_A ] * m_frustum[ Side ][ plane_A ]
            + m_frustum[ Side ][ plane_B ] * m_frustum[ Side ][ plane_B ]
            + m_frustum[ Side ][ plane_C ] * m_frustum[ Side ][ plane_C ] );

    m_frustum[ Side ][ plane_A ] /= magnitude;
    m_frustum[ Side ][ plane_B ] /= magnitude;
    m_frustum[ Side ][ plane_C ] /= magnitude;
    m_frustum[ Side ][ plane_D ] /= magnitude;
}
