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
#include "dynobj.h"
#include "driver.h"

void
light_array::insert( TDynamicObject const *Owner ) {

    // we're only storing lights for locos, which have two sets of lights, front and rear
    // for a more generic role this function would have to be tweaked to add vehicle type-specific light combinations
    data.emplace_back( Owner, 0 );
    data.emplace_back( Owner, 1 );
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
        if( light.index == 0 ) {
            // front light set
            light.position = light.owner->GetPosition() + ( light.owner->VectorFront() * light.owner->GetLength() * 0.45 );
            light.direction = light.owner->VectorFront();
        }
        else {
            // rear light set
            light.position = light.owner->GetPosition() - ( light.owner->VectorFront() * light.owner->GetLength() * 0.45 );
            light.direction = light.owner->VectorFront();
            light.direction.x = -light.direction.x;
            light.direction.z = -light.direction.z;
        }
        // determine intensity of this light set
        if( true == light.owner->MoverParameters->Battery ) {
            // with battery on, the intensity depends on the state of activated switches
            auto const &lightbits = light.owner->iLights[ light.index ];
            light.count = 0 +
                ( ( lightbits & 1 ) ? 1 : 0 ) +
                ( ( lightbits & 4 ) ? 1 : 0 ) +
                ( ( lightbits & 16 ) ? 1 : 0 );

            if( light.count > 0 ) {
                // TODO: intensity can be affected further by dim switch or other factors
                light.intensity = std::max( 0.0f, std::log( (float)light.count  + 1.0f ) );
                light.intensity *= ( light.owner->DimHeadlights ? 0.6f : 1.0f );
            }
            else {
                light.intensity = 0.0f;
            }

            // crude catch for unmanned modules which share the light state with the controlled unit.
            // why don't they get their own light bits btw ._.
            // TODO, TBD: have separate light bits for each vehicle, so this override isn't necessary
            if( ( light.owner->Controller == AIdriver )
             && ( light.owner->Mechanik == nullptr ) ) {
                
                light.intensity = 0.0f;
                light.count = 0;
            }
        }
        else {
            // with battery off the lights are off
            light.intensity = 0.0f;
            light.count = 0;
        }
    }
}
