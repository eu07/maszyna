/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "station.h"

#include "dynobj.h"
#include "mtable.h"

// exchanges load with consist attached to specified vehicle, operating on specified schedule
double
basic_station::update_load( TDynamicObject *First, Mtable::TTrainParameters &Schedule ) {

    auto const firststop { Schedule.StationIndex == 1 };
    auto const laststop { Schedule.StationIndex == Schedule.StationCount };
    // HACK: determine whether current station is a (small) stop from the presence of "po" at the name end
    auto const stationname { Schedule.TimeTable[ Schedule.StationIndex ].StationName };
    auto const stationequipment { Schedule.TimeTable[ Schedule.StationIndex ].StationWare };
    auto const trainstop { (
        ( ( stationname.size() >= 2 ) && ( stationname.substr( stationname.size() - 2 ) == "po" ) )
     || ( stationequipment.find( "po" ) != std::string::npos ) ) };
    // train stops exchange smaller groups than regular stations
    // NOTE: this is crude and unaccurate, but for now will do
    auto const stationsizemodifier { ( trainstop ? 1.0 : 2.0 ) };
    // go through all vehicles and update their load
    // NOTE: for the time being we limit ourselves to passenger-carrying cars only
    auto exchangetime { 0.0 };

    auto *vehicle { First };
    while( vehicle != nullptr ) {

        auto &parameters { *vehicle->MoverParameters };

        if( ( true == parameters.LoadType.empty() )
         && ( parameters.LoadAccepted.find( "passengers" ) != std::string::npos ) ) {
            // set the type for empty cars
            parameters.LoadType = "passengers";
        }

        if( parameters.LoadType == "passengers" ) {
            // NOTE: for the time being we're doing simple, random load change calculation
            // TODO: exchange driven by station parameters and time of the day
            auto loadcount = static_cast<int>(
                laststop ?
                    0 :
                    Random( parameters.MaxLoad * 0.15 * stationsizemodifier ) );
            if( true == firststop ) {
                // slightly larger group at the initial station
                loadcount *= 2;
            }
            auto unloadcount = static_cast<int>(
                firststop ? 0 :
                laststop ? parameters.Load :
                Random( parameters.MaxLoad * 0.10 * stationsizemodifier ) );

            parameters.Load = clamp<int>( parameters.Load + loadcount - unloadcount, 0, parameters.MaxLoad );
            vehicle->LoadUpdate();
            vehicle->update_load_visibility();

            exchangetime = std::max( exchangetime, loadcount / parameters.LoadSpeed + unloadcount / parameters.UnLoadSpeed );
        }
        vehicle = vehicle->Next();
    }

    return exchangetime;
}
