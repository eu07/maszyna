/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "dictionary.h"

#include "simulation.h"
#include "utilities.h"
#include "DynObj.h"
#include "Driver.h"
#include "mtable.h"

dictionary_source::dictionary_source( std::string const &Input ) {

    auto const keyvaluepairs { Split( Input, '&' ) };

    for( auto const &keyvaluepair : keyvaluepairs ) {

        auto const valuepos { keyvaluepair.find( '=' ) };
        if( keyvaluepair[ 0 ] != '$' ) {
            // regular key, value pairs
            insert(
                ToLower( keyvaluepair.substr( 0, valuepos ) ),
                keyvaluepair.substr( valuepos + 1 ) );
        }
        else {
            // special case, $key indicates request to include certain dataset clarified by value
            auto const key { ToLower( keyvaluepair.substr( 0, valuepos ) ) };
            auto const value { keyvaluepair.substr( valuepos + 1 ) };

            if( key == "$timetable" ) {
                // timetable pulled from (preferably) the owner/direct controller of specified vehicle
                auto const *vehicle { simulation::Vehicles.find( value ) };
                auto const *controller { (
                    vehicle == nullptr ? nullptr :
                    vehicle->ctOwner == nullptr ? vehicle->Mechanik :
                    vehicle->ctOwner ) };
                if( controller != nullptr ) {
                    controller->TrainTimetable().serialize( this );
                }
            }
        }
    }
}
