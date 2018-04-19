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

std::string
    label_cab_control( std::string const &Label );

static std::unordered_map<std::string, std::string> m_cabcontrols = {
    { "mainctrl:", "master controller" },
    { "scndctrl:", "second controller" },
    { "dirkey:" , "reverser" },
    { "brakectrl:", "train brake" },
    { "localbrake:", "independent brake" },
    { "manualbrake:", "manual brake" },
    { "alarmchain:", "emergency brake" },
    { "brakeprofile_sw:", "brake acting speed" },
    { "brakeprofileg_sw:", "brake acting speed: cargo" },
    { "brakeprofiler_sw:", "brake acting speed: rapid" },
    { "maxcurrent_sw:", "motor overload relay threshold" },
    { "waterpump_sw:", "water pump" },
    { "waterpumpbreaker_sw:", "water pump breaker" },
    { "waterheater_sw:", "water heater" },
    { "waterheaterbreaker_sw:", "water heater breaker" },
    { "watercircuitslink_sw:", "water circuits link" },
    { "fuelpump_sw:", "fuel pump" },
    { "oilpump_sw:", "oil pump" },
    { "main_off_bt:", "line breaker" },
    { "main_on_bt:", "line breaker" },
    { "security_reset_bt:", "alerter" },
    { "releaser_bt:", "independent brake releaser" },
    { "sand_bt:", "sandbox" },
    { "antislip_bt:", "wheelspin brake" },
    { "horn_bt:", "horn" },
    { "hornlow_bt:", "low tone horn" },
    { "hornhigh_bt:", "high tone horn" },
    { "fuse_bt:", "motor overload relay reset" },
    { "converterfuse_bt:", "converter overload relay reset" },
    { "stlinoff_bt:", "motor connectors" },
    { "door_left_sw:", "left door" },
    { "door_right_sw:", "right door" },
    { "departure_signal_bt:", "departure signal" },
    { "upperlight_sw:", "upper headlight" },
    { "leftlight_sw:", "left headlight" },
    { "rightlight_sw:", "right headlight" },
    { "dimheadlights_sw:", "headlights dimmer" },
    { "leftend_sw:", "left marker light" },
    { "rightend_sw:", "right marker light" },
    { "lights_sw:", "light pattern" },
    { "rearupperlight_sw:", "rear upper headlight" },
    { "rearleftlight_sw:", "rear left headlight" },
    { "rearrightlight_sw:", "rear right headlight" },
    { "rearleftend_sw:", "rear left marker light" },
    { "rearrightend_sw:",  "rear right marker light" },
    { "compressor_sw:", "compressor" },
    { "compressorlocal_sw:", "local compressor" },
    { "converter_sw:", "converter" },
    { "converterlocal_sw:", "local converter" },
    { "converteroff_sw:", "converter" },
    { "main_sw:", "line breaker" },
    { "radio_sw:", "radio" },
    { "radiochannel_sw:", "radio channel" },
    { "radiochannelprev_sw:", "radio channel" },
    { "radiochannelnext_sw:", "radio channel" },
    { "radiotest_sw:", "radiostop test" },
    { "radiostop_sw:", "radiostop" },
    { "pantfront_sw:", "pantograph A" },
    { "pantrear_sw:", "pantograph B" },
    { "pantfrontoff_sw:", "pantograph A" },
    { "pantrearoff_sw:", "pantograph B" },
    { "pantalloff_sw:", "all pantographs" },
    { "pantselected_sw:", "selected pantograph" },
    { "pantselectedoff_sw:", "selected pantograph" },
    { "pantcompressor_sw:", "pantograph compressor" },
    { "pantcompressorvalve_sw:", "pantograph 3 way valve" },
    { "trainheating_sw:", "heating" },
    { "signalling_sw:", "braking indicator" },
    { "door_signalling_sw:", "door locking" },
    { "nextcurrent_sw:", "current indicator source" },
    { "instrumentlight_sw:", "instrument light" },
    { "cablight_sw:", "interior light" },
    { "cablightdim_sw:", "interior light dimmer" },
    { "battery_sw:", "battery" },
    { "universal0:", "generic" },
    { "universal1:", "generic" },
    { "universal2:", "generic" },
    { "universal3:", "generic" },
    { "universal4:", "generic" },
    { "universal5:", "generic" },
    { "universal6:", "generic" },
    { "universal7:", "generic" },
    { "universal8:", "generic" },
    { "universal9:", "generic" }
};

}

//---------------------------------------------------------------------------

