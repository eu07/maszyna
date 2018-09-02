/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <unordered_map>

namespace locale {

enum string {
    driver_aid_header,
    driver_aid_throttle,
    driver_aid_speedlimit,
    driver_aid_nextlimit,
    driver_aid_grade,
    driver_aid_brakes,
    driver_aid_pressures,
    driver_aid_alerter,
    driver_aid_shp,
    driver_aid_loadinginprogress,
    driver_aid_vehicleahead,

    driver_timetable_header,
    driver_timetable_time,
    driver_timetable_notimetable,

    driver_transcripts_header,

    driver_pause_header,
    driver_pause_resume,
    driver_pause_quit,

    debug_vehicle_nameloadstatuscouplers,
    debug_vehicle_owned,
    debug_vehicle_none,
    debug_vehicle_devicespower,
    debug_vehicle_radio,
    debug_vehicle_oilpressure,
    debug_vehicle_controllersenginerevolutions,
    debug_vehicle_shuntmode,
    debug_vehicle_temperatures,
    debug_vehicle_brakespressures,
    debug_vehicle_pantograph,
    debug_vehicle_forcesaccelerationvelocityposition
};

void
    init();
std::string
    label_cab_control( std::string const &Label );

extern std::vector<std::string> strings;

extern std::unordered_map<std::string, std::string> m_cabcontrols;

}

//---------------------------------------------------------------------------

