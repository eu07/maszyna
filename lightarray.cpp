/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "stdafx.h"
#include "lightarray.h"

#include "DynObj.h"

void
light_array::insert( TDynamicObject const *Owner ) {

    // we're only storing lights for locos, which have two sets of lights, front and rear
    // for a more generic role this function would have to be tweaked to add vehicle type-specific light combinations
    data.emplace_back( Owner, side::front );
    data.emplace_back( Owner, side::rear );
}

void
light_array::remove( TDynamicObject const *Owner ) {

    data.erase(
        std::remove_if(
            data.begin(),
            data.end(),
            [=]( light_record const &light ){ return light.owner == Owner; } ),
        data.end() );
}

// updates records in the collection
void
light_array::update() {

    for( auto &light : data ) {
        // update light parameters to match current data of the owner
        if( light.index == side::front ) {
            // front light set
            light.position = light.owner->GetPosition() + ( light.owner->VectorFront() * light.owner->GetLength() * 0.4 );
            light.direction = glm::make_vec3( light.owner->VectorFront().getArray() );
        }
        else {
            // rear light set
            light.position = light.owner->GetPosition() - ( light.owner->VectorFront() * light.owner->GetLength() * 0.4 );
            light.direction = glm::make_vec3( light.owner->VectorFront().getArray() );
            light.direction.x = -light.direction.x;
            light.direction.z = -light.direction.z;
        }
        // determine intensity of this light set
        if( ( true == light.owner->MoverParameters->Battery )
         || ( true == light.owner->MoverParameters->ConverterFlag ) ) {
            // with power on, the intensity depends on the state of activated switches
            // first we cross-check the list of enabled lights with the lights installed in the vehicle...
            auto const lights { light.owner->iLights[ light.index ] & light.owner->LightList( static_cast<side>( light.index ) ) };
            // ...then check their individual state
            light.count = 0
                + ( ( lights & light::headlight_left  ) ? 1 : 0 )
                + ( ( lights & light::headlight_right ) ? 1 : 0 )
                + ( ( lights & light::headlight_upper ) ? 1 : 0 );

            if( light.count > 0 ) {
                light.intensity = std::max( 0.0f, std::log( (float)light.count  + 1.0f ) );
                light.intensity *= ( light.owner->DimHeadlights ? 0.6f : 1.0f );
                // TBD, TODO: intensity can be affected further by other factors
            }
            else {
                light.intensity = 0.0f;
            }
        }
        else {
            // with battery off the lights are off
            light.intensity = 0.0f;
            light.count = 0;
        }
    }
}
