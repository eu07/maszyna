/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "renderer.h"
#include "globals.h"

opengl_renderer GfxRenderer;

void
opengl_renderer::Init() {

    // create dynamic light pool
    for( int idx = 0; idx < Global::DynamicLightCount; ++idx ) {

        opengl_light light;
        light.id = GL_LIGHT1 + idx;

        light.position[ 3 ] = 1.0f;
        ::glLightf( light.id, GL_SPOT_CUTOFF, 20.0f );
        ::glLightf( light.id, GL_SPOT_EXPONENT, 10.0f );
        ::glLightf( light.id, GL_CONSTANT_ATTENUATION, 0.0f );
        ::glLightf( light.id, GL_LINEAR_ATTENUATION, 0.035f );

        m_lights.emplace_back( light );
    }
}

void
opengl_renderer::Update_Lights( light_array const &Lights ) {

    int const count = std::min( m_lights.size(), Lights.data.size() );
    if( count == 0 ) { return; }

    auto renderlight = m_lights.begin();

    for( auto const &scenelight : Lights.data ) {

        if( renderlight == m_lights.end() ) {
            // we ran out of lights to assign
            break;
        }
        if( scenelight.intensity == 0.0f ) {
            // all lights past this one are bound to be off
            break;
        }
        if( ( Global::pCameraPosition - scenelight.position ).Length() > 1000.0f ) {
            // we don't care about lights past arbitrary limit of 1 km.
            // but there could still be weaker lights which are closer, so keep looking
            continue;
        }
        // if the light passed tests so far, it's good enough
        renderlight->set_position( scenelight.position );
        renderlight->direction = scenelight.direction;

        renderlight->diffuse[ 0 ] = scenelight.color.x;
        renderlight->diffuse[ 1 ] = scenelight.color.y;
        renderlight->diffuse[ 2 ] = scenelight.color.z;
        renderlight->ambient[ 0 ] = scenelight.color.x * scenelight.intensity;
        renderlight->ambient[ 1 ] = scenelight.color.y * scenelight.intensity;
        renderlight->ambient[ 2 ] = scenelight.color.z * scenelight.intensity;

        ::glLightf( renderlight->id, GL_LINEAR_ATTENUATION, 0.3f / std::pow( scenelight.count, 2 ) );
        ::glEnable( renderlight->id );

        renderlight->apply_intensity();
        renderlight->apply_angle();

        ++renderlight;
    }

    while( renderlight != m_lights.end() ) {
        // if we went through all scene lights and there's still opengl lights remaining, kill these
        ::glDisable( renderlight->id );
        ++renderlight;
    }
}

void
opengl_renderer::Disable_Lights() {

    for( int idx = 0; idx < m_lights.size() + 1; ++idx ) {

        ::glDisable( GL_LIGHT0 + idx );
    }
}
//---------------------------------------------------------------------------
