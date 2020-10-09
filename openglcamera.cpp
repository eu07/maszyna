/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "openglcamera.h"

#include "DynObj.h"

void
opengl_camera::update_frustum( glm::mat4 const &Projection, glm::mat4 const &Modelview ) {

    m_frustum.calculate( Projection, Modelview );
    // cache inverse tranformation matrix
    // NOTE: transformation is done only to camera-centric space
    m_inversetransformation = glm::inverse( Projection * glm::mat4{ glm::mat3{ Modelview } } );
    // calculate frustum corners
    m_frustumpoints = ndcfrustumshapepoints;
    transform_to_world(
        std::begin( m_frustumpoints ),
        std::end( m_frustumpoints ) );
}

// returns true if specified object is within camera frustum, false otherwise
bool
opengl_camera::visible( scene::bounding_area const &Area ) const {

    return ( m_frustum.sphere_inside( Area.center, Area.radius ) > 0.f );
}

bool
opengl_camera::visible( TDynamicObject const *Dynamic ) const {

    // sphere test is faster than AABB, so we'll use it here
    // we're giving vehicles some extra padding, to allow for things like shared bogeys extending past the main body
    return ( m_frustum.sphere_inside( Dynamic->GetPosition(), Dynamic->radius() * 1.25 ) > 0.0f );
}

// debug helper, draws shape of frustum in world space
void
opengl_camera::draw( glm::vec3 const &Offset ) const {

    ::glBegin( GL_LINES );
    for( auto const pointindex : frustumshapepoinstorder ) {
        ::glVertex3fv( glm::value_ptr( glm::vec3{ m_frustumpoints[ pointindex ] } - Offset ) );
    }
    ::glEnd();
}

//---------------------------------------------------------------------------
