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
    data.emplace_back( Owner, end::front );
    data.emplace_back( Owner, end::rear );
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
        if( light.index == end::front ) {
            // front light set
            light.position = light.owner->GetPosition() + ( light.owner->VectorFront() * ( std::max( 0.0, light.owner->GetLength() * 0.5 - 2.0 ) ) );// +( light.owner->VectorUp() * 0.25 );
            light.direction = glm::make_vec3( light.owner->VectorFront().getArray() );
        }
        else {
            // rear light set
            light.position = light.owner->GetPosition() - ( light.owner->VectorFront() * ( std::max( 0.0, light.owner->GetLength() * 0.5 - 2.0 ) ) );// +( light.owner->VectorUp() * 0.25 );
            light.direction = glm::make_vec3( light.owner->VectorFront().getArray() );
            light.direction.x = -light.direction.x;
            light.direction.z = -light.direction.z;
        }
        // determine intensity of this light set
        if( ( true == light.owner->MoverParameters->Power24vIsAvailable )
         || ( true == light.owner->MoverParameters->Power110vIsAvailable ) ) {
            // with power on, the intensity depends on the state of activated switches
            // first we cross-check the list of enabled lights with the lights installed in the vehicle...
            auto const lights { light.owner->MoverParameters->iLights[ light.index ] & light.owner->LightList( static_cast<end>( light.index ) ) };
            // ...then check their individual state
            light.count = 0
                + ( ( lights & light::headlight_left  ) ? 1 : 0 )
                + ( ( lights & light::headlight_right ) ? 1 : 0 )
                + ( ( lights & light::headlight_upper ) ? 1 : 0 )
                + ( ( lights & light::highbeamlight_left ) ? 1: 0)
                + ( ( lights & light::highbeamlight_right ) ? 1 : 0);

            if( light.count > 0 ) {
				light.intensity = std::max(0.0f, std::log((float)light.count + 1.0f));
				if (light.owner->DimHeadlights && !light.owner->HighBeamLights) // tylko przyciemnione
					light.intensity *= light.owner->MoverParameters->dimMultiplier;
				else if (!light.owner->DimHeadlights && !light.owner->HighBeamLights) // normalne
					light.intensity *= light.owner->MoverParameters->normMultiplier;
				else if (light.owner->DimHeadlights && light.owner->HighBeamLights) // przyciemnione dlugie
					light.intensity *= light.owner->MoverParameters->highDimMultiplier;
				else if (!light.owner->DimHeadlights && light.owner->HighBeamLights) // dlugie zwykle
					light.intensity *= light.owner->MoverParameters->highMultiplier;

                // TBD, TODO: intensity can be affected further by other factors
                light.state = {
                    ( ( lights & light::headlight_left | light::highbeamlight_left ) ? 1.f : 0.f ),
                    ( ( lights & light::headlight_upper ) ? 1.f : 0.f ), 
                    ( ( lights & light::headlight_right | light::highbeamlight_right) ? 1.f : 0.f ) };

                light.color = {
                    light.owner->MoverParameters->refR / 255,
                    light.owner->MoverParameters->refG / 255,
                    light.owner->MoverParameters->refB / 255
                };

				if (light.owner->DimHeadlights && !light.owner->HighBeamLights) // tylko przyciemnione
					light.state *= light.owner->MoverParameters->dimMultiplier;
				else if (!light.owner->DimHeadlights && !light.owner->HighBeamLights)
					light.state *= light.owner->MoverParameters->normMultiplier;
				else if (light.owner->DimHeadlights && light.owner->HighBeamLights) // przyciemnione dlugie
					light.state *= light.owner->MoverParameters->highDimMultiplier;
				else if (!light.owner->DimHeadlights && light.owner->HighBeamLights) // dlugie zwykle
					light.state *= light.owner->MoverParameters->highMultiplier;
                
            }
            else {
                light.intensity = 0.0f;
                light.state = glm::vec3{ 0.f };
            }
        }
        else {
            // with battery off the lights are off
            light.intensity = 0.0f;
            light.count = 0;
        }
    }
}
