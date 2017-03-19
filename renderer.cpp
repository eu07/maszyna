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
#include "World.h"

opengl_renderer GfxRenderer;
extern TWorld World;

void
opengl_renderer::Init() {

}

void
opengl_renderer::Update_Lights( light_array const &Lights ) {

    size_t const count = std::min( (size_t)Global::DynamicLightCount, Lights.data.size() );
    if( count == 0 ) { return; }

	size_t renderlight = 0;

    for( auto const &scenelight : Lights.data ) {

        if( renderlight == Global::DynamicLightCount ) {
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

		auto const luminance = Global::fLuminance; // TODO: adjust this based on location, e.g. for tunnels
		float3 position(scenelight.position.x, scenelight.position.y, scenelight.position.z);
		float3 direction(scenelight.direction.x, scenelight.direction.y, scenelight.direction.z);
		float3 color(scenelight.color.x,
		             scenelight.color.y,
		             scenelight.color.z);

		World.shader.set_light((GLuint)renderlight + 1, gl_program_light::SPOT, position, direction, 0.906f, 0.866f, color, 0.007f, 0.0002f);

        ++renderlight;
    }

	World.shader.set_light_count((GLuint)renderlight + 1);
}

void
opengl_renderer::Disable_Lights() {

	World.shader.set_light_count(0);
}
//---------------------------------------------------------------------------
