/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "opengllight.h"
#include "utilities.h"

void
opengl_light::apply_intensity( float const Factor ) {

    if( Factor == 1.f ) {

        ::glLightfv( id, GL_AMBIENT, glm::value_ptr( ambient ) );
        ::glLightfv( id, GL_DIFFUSE, glm::value_ptr( diffuse ) );
        ::glLightfv( id, GL_SPECULAR, glm::value_ptr( specular ) );
    }
    else {
        auto const factor{ clamp( Factor, 0.05f, 1.f ) };
        // temporary light scaling mechanics (ultimately this work will be left to the shaders
        glm::vec4 scaledambient( ambient.r * factor, ambient.g * factor, ambient.b * factor, ambient.a );
        glm::vec4 scaleddiffuse( diffuse.r * factor, diffuse.g * factor, diffuse.b * factor, diffuse.a );
        glm::vec4 scaledspecular( specular.r * factor, specular.g * factor, specular.b * factor, specular.a );
        glLightfv( id, GL_AMBIENT, glm::value_ptr( scaledambient ) );
        glLightfv( id, GL_DIFFUSE, glm::value_ptr( scaleddiffuse ) );
        glLightfv( id, GL_SPECULAR, glm::value_ptr( scaledspecular ) );
    }
}

void
opengl_light::apply_angle() {

    ::glLightfv( id, GL_POSITION, glm::value_ptr( glm::vec4{ position, ( is_directional ? 0.f : 1.f ) } ) );
    if( false == is_directional ) {
        ::glLightfv( id, GL_SPOT_DIRECTION, glm::value_ptr( direction ) );
    }
}

//---------------------------------------------------------------------------
