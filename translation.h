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
    debug_vehicle_forcesaccelerationvelocityposition,

    ui_expand,
    ui_loading,
    ui_loading_scenery,
    ui_quit_simulation_q,
    ui_yes,
    ui_no,
    ui_general,
    ui_debug_mode,
    ui_quit,
    ui_tools,
    ui_logging_to_log,
    ui_screenshot,
    ui_windows,
    ui_log,
    ui_map,
    ui_mode_windows,

    cab_mainctrl,
    cab_scndctrl,
    cab_shuntmodepower,
    cab_dirkey,
    cab_brakectrl,
    cab_localbrake,
    cab_manualbrake,
    cab_alarmchain,
    cab_brakeprofile_sw,
    cab_brakeprofileg_sw,
    cab_brakeprofiler_sw,
    cab_brakeopmode_sw,
    cab_maxcurrent_sw,
    cab_waterpump_sw,
    cab_waterpumpbreaker_sw,
    cab_waterheater_sw,
    cab_waterheaterbreaker_sw,
    cab_watercircuitslink_sw,
    cab_fuelpump_sw,
    cab_oilpump_sw,
    cab_motorblowersfront_sw,
    cab_motorblowersrear_sw,
    cab_motorblowersalloff_sw,
    cab_main_off_bt,
    cab_main_on_bt,
    cab_security_reset_bt,
    cab_releaser_bt,
    cab_sand_bt,
    cab_antislip_bt,
    cab_horn_bt,
    cab_hornlow_bt,
    cab_hornhigh_bt,
    cab_whistle_bt,
    cab_fuse_bt,
    cab_converterfuse_bt,
    cab_stlinoff_bt,
    cab_door_left_sw,
    cab_door_right_sw,
    cab_doorlefton_sw,
    cab_doorrighton_sw,
    cab_doorleftoff_sw,
    cab_doorrightoff_sw,
    cab_dooralloff_sw,
    cab_departure_signal_bt,
    cab_upperlight_sw,
    cab_leftlight_sw,
    cab_rightlight_sw,
    cab_dimheadlights_sw,
    cab_leftend_sw,
    cab_rightend_sw,
    cab_lights_sw,
    cab_rearupperlight_sw,
    cab_rearleftlight_sw,
    cab_rearrightlight_sw,
    cab_rearleftend_sw,
    cab_rearrightend_sw,
    cab_compressor_sw,
    cab_compressorlocal_sw,
    cab_converter_sw,
    cab_converterlocal_sw,
    cab_converteroff_sw,
    cab_main_sw,
    cab_radio_sw,
    cab_radiochannel_sw,
    cab_radiochannelprev_sw,
    cab_radiochannelnext_sw,
    cab_radiotest_sw,
    cab_radiostop_sw,
    cab_pantfront_sw,
    cab_pantrear_sw,
    cab_pantfrontoff_sw,
    cab_pantrearoff_sw,
    cab_pantalloff_sw,
    cab_pantselected_sw,
    cab_pantselectedoff_sw,
    cab_pantcompressor_sw,
    cab_pantcompressorvalve_sw,
    cab_trainheating_sw,
    cab_signalling_sw,
    cab_door_signalling_sw,
    cab_nextcurrent_sw,
    cab_instrumentlight_sw,
    cab_dashboardlight_sw,
    cab_timetablelight_sw,
    cab_cablight_sw,
    cab_cablightdim_sw,
    cab_battery_sw,
    cab_universal0,
    cab_universal1,
    cab_universal2,
    cab_universal3,
    cab_universal4,
    cab_universal5,
    cab_universal6,
    cab_universal7,
    cab_universal8,
    cab_universal9,

    string_count
};

void
    init();
std::string
    label_cab_control( std::string const &Label );

extern std::vector<std::string> strings;

extern std::unordered_map<std::string, std::string> m_cabcontrols;

}

//---------------------------------------------------------------------------

