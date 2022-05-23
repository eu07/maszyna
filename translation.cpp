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
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/

#include "stdafx.h"
#include "translation.h"
#include "Logs.h"
#include "Globals.h"
#include "MOVER.h"

void locale::init()
{
	std::fstream stream("lang/" + Global.asLang + ".po", std::ios_base::in | std::ios_base::binary);

	if (!stream.is_open()) {
		WriteLog("translation: cannot open lang file: lang/" + Global.asLang + ".po");
		return;
	}

	crashreport_add_info("translation", Global.asLang);

	while (parse_translation(stream));

	WriteLog("translation: " + std::to_string(lang_mapping.size()) + " strings loaded");
}

const std::string& locale::lookup_s(const std::string &msg, bool constant)
{
	if (constant) {
		auto it = pointer_cache.find(&msg);
		if (it != pointer_cache.end())
			return *((const std::string*)(it->second));
	}

	auto it = lang_mapping.find(msg);
	if (it != lang_mapping.end()) {
		if (constant)
			pointer_cache.emplace(&msg, &(it->second));
		return it->second;
	}

	if (constant)
		pointer_cache.emplace(&msg, &msg);
	return msg;
}

const char* locale::lookup_c(const char *msg, bool constant)
{
	if (constant) {
		auto it = pointer_cache.find(msg);
		if (it != pointer_cache.end())
			return (const char*)(it->second);
	}

	auto it = lang_mapping.find(std::string(msg));
	if (it != lang_mapping.end()) {
		if (constant)
			pointer_cache.emplace(msg, it->second.c_str());
		return it->second.c_str();
	}

	if (constant)
		pointer_cache.emplace(msg, msg);
	return msg;
}

bool locale::parse_translation(std::istream &stream)
{
	std::string line;

	std::string msgid;
	std::string msgstr;
	std::string msgctxt;
	char last = 'x';

	while (std::getline(stream, line)) {
		if (line.size() > 0 && line[0] == '#')
			continue;

		if (string_starts_with(line, "msgid"))
			last = 'i';
		else if (string_starts_with(line, "msgstr"))
			last = 's';
		else if (string_starts_with(line, "msgctxt"))
			last = 'c';

		if (line.size() > 1 && last != 'x') {
			if (last == 'i')
				msgid += parse_c_literal(line);
			else if (last == 's')
				msgstr += parse_c_literal(line);
			else if (last == 'c')
				msgctxt += parse_c_literal(line);
		}
		else {
			if (!msgid.empty() && !msgstr.empty()) {
				// TODO: context support is incomplete
				if (!msgctxt.empty())
					lang_mapping.emplace(msgctxt + "\x1d" + msgid, msgstr);
				else
					lang_mapping.emplace(msgid, msgstr);
			}
			return true;
		}
	}

	return false;
}

std::string locale::parse_c_literal(const std::string &str)
{
	std::istringstream stream(str);
	std::string out;

	bool active = false;
	bool escape = false;

	char c;
	while ((c = stream.get()) != std::char_traits<char>::eof()) {
		if (!escape && c == '"')
			active = !active;
		else if (active && !escape && c == '\\')
			escape = true;
		else if (active && escape) {
			if (c == 'r')
				out += '\r';
			else if (c == 'n')
				out += '\n';
			else if (c == 't')
				out += '\t';
			else if (c == '\\')
				out += '\\';
			else if (c == '?')
				out += '?';
			else if (c == '\'')
				out += '\'';
			else if (c == '"')
				out += '"';
			else if (c == 'x') {
				char n1 = stream.get() - 48;
				char n2 = stream.get() - 48;
				if (n1 > 9)
					n1 -= 7;
				if (n1 > 16)
					n1 -= 32;
				if (n2 > 9)
					n2 -= 7;
				if (n2 > 16)
					n2 -= 32;
				out += ((n1 << 4) | n2);
			}
			escape = false;
		}
		else if (active)
			out += c;
	}

	return out;
}

std::string locale::label_cab_control(std::string const &Label)
{
    if( Label.empty() ) { return ""; }

	static std::unordered_map<std::string, std::string> cabcontrols_labels = {
	    { "mainctrl:", STRN("master controller") },
	    { "jointctrl:", STRN("master controller") },
	    { "scndctrl:", STRN("second controller") },
	    { "shuntmodepower:", STRN("shunt mode power") },
	    { "tempomat_sw:", STRN("cruise control") },
	    { "tempomatoff_sw:", STRN("cruise control") },
	    { "speedinc_bt:", STRN("cruise control (speed)") },
	    { "speeddec_bt:", STRN("cruise control (speed)") },
	    { "speedctrlpowerinc_bt:", STRN("cruise control (power)") },
	    { "speedctrlpowerdec_bt:", STRN("cruise control (power)") },
	    { "speedbutton0:", STRN("cruise control (speed)") },
	    { "speedbutton1:", STRN("cruise control (speed)") },
	    { "speedbutton2:", STRN("cruise control (speed)") },
	    { "speedbutton3:", STRN("cruise control (speed)") },
	    { "speedbutton4:", STRN("cruise control (speed)") },
	    { "speedbutton5:", STRN("cruise control (speed)") },
	    { "speedbutton6:", STRN("cruise control (speed)") },
	    { "speedbutton7:", STRN("cruise control (speed)") },
	    { "speedbutton8:", STRN("cruise control (speed)") },
	    { "speedbutton9:", STRN("cruise control (speed)") },
	    { "distancecounter_sw:", STRN("distance counter") },
	    { "dirkey:", STRN("reverser") },
	    { "dirbackward_bt:", STRN("reverser") },
	    { "dirforward_bt:", STRN("reverser") },
	    { "dirneutral_bt:", STRN("reverser") },
	    { "brakectrl:", STRN("train brake") },
	    { "localbrake:", STRN("independent brake") },
	    { "manualbrake:", STRN("manual brake") },
	    { "alarmchain:", STRN("emergency brake") },
	    { "brakeprofile_sw:", STRN("brake acting speed") },
	    { "brakeprofileg_sw:", STRN("brake acting speed: cargo") },
	    { "brakeprofiler_sw:", STRN("brake acting speed: rapid") },
	    { "brakeopmode_sw", STRN("brake operation mode") },
	    { "maxcurrent_sw:", STRN("motor overload relay threshold") },
	    { "waterpump_sw:", STRN("water pump") },
	    { "waterpumpbreaker_sw:", STRN("water pump breaker") },
	    { "waterheater_sw:", STRN("water heater") },
	    { "waterheaterbreaker_sw:", STRN("water heater breaker") },
	    { "watercircuitslink_sw:", STRN("water circuits link") },
	    { "fuelpump_sw:", STRN("fuel pump") },
	    { "oilpump_sw:", STRN("oil pump") },
	    { "motorblowersfront_sw:", STRN("motor blowers A") },
	    { "motorblowersrear_sw:", STRN("motor blowers B") },
	    { "motorblowersalloff_sw:", STRN("all motor blowers") },
	    { "coolingfans_sw:", STRN("cooling fans") },
	    { "main_off_bt:", STRN("line breaker") },
	    { "main_on_bt:", STRN("line breaker") },
	    { "security_reset_bt:", STRN("alerter") },
	    { "shp_reset_bt:", STRN("shp") },
	    { "releaser_bt:", STRN("independent brake releaser") },
	    { "sand_bt:", STRN("sandbox") },
	    { "antislip_bt:", STRN("wheelspin brake") },
	    { "horn_bt:", STRN("horn") },
	    { "hornlow_bt:", STRN("low tone horn") },
	    { "hornhigh_bt:", STRN("high tone horn") },
	    { "whistle_bt:", STRN("whistle") },
	    { "fuse_bt:", STRN("motor overload relay reset") },
	    { "converterfuse_bt:", STRN("converter overload relay reset") },
	    { "stlinoff_bt:", STRN("motor connectors") },
	    { "doorleftpermit_sw:", STRN("left door (permit)") },
	    { "doorrightpermit_sw:", STRN("right door (permit)") },
	    { "doorpermitpreset_sw:", STRN("door (permit)") },
	    { "door_left_sw:", STRN("left door") },
	    { "door_right_sw:", STRN("right door") },
	    { "doorlefton_sw:", STRN("left door (open)") },
	    { "doorrighton_sw:", STRN("right door (open)") },
	    { "doorleftoff_sw:", STRN("left door (close)") },
	    { "doorrightoff_sw:", STRN("right door (close)") },
	    { "doorallon_sw:", STRN("all doors (open)") },
	    { "dooralloff_sw:", STRN("all doors (close)") },
	    { "doorstep_sw:", STRN("doorstep") },
	    { "doormode_sw", STRN("door control mode") },
	    { "departure_signal_bt:", STRN("departure signal") },
	    { "upperlight_sw:", STRN("upper headlight") },
	    { "leftlight_sw:", STRN("left headlight") },
	    { "rightlight_sw:", STRN("right headlight") },
	    { "dimheadlights_sw:", STRN("headlights dimmer") },
	    { "leftend_sw:", STRN("left marker light") },
	    { "rightend_sw:", STRN("right marker light") },
	    { "lights_sw:", STRN("light pattern") },
	    { "rearupperlight_sw:", STRN("rear upper headlight") },
	    { "rearleftlight_sw:", STRN("rear left headlight") },
	    { "rearrightlight_sw:", STRN("rear right headlight") },
	    { "rearleftend_sw:", STRN("rear left marker light") },
	    { "rearrightend_sw:", STRN("rear right marker light") },
	    { "compressor_sw:", STRN("compressor") },
	    { "compressorlocal_sw:", STRN("local compressor") },
	    { "converter_sw:", STRN("converter") },
	    { "converterlocal_sw:", STRN("local converter") },
	    { "converteroff_sw:", STRN("converter") },
	    { "main_sw:", STRN("line breaker") },
	    { "radio_sw:", STRN("radio") },
	    { "radiochannel_sw:", STRN("radio channel") },
	    { "radiochannelprev_sw:", STRN("radio channel") },
	    { "radiochannelnext_sw:", STRN("radio channel") },
	    { "radiotest_sw:", STRN("radiostop test") },
	    { "radiostop_sw:", STRN("radiostop") },
	    { "radiocall3_sw:", STRN("radio call 3") },
	    { "radiovolume_sw:", STRN("radio volume") },
	    { "radiovolumenext_sw:", STRN("radio volume") },
	    { "radiovolumeprev_sw:", STRN("radio volume") },
	    { "pantfront_sw:", STRN("pantograph A") },
	    { "pantrear_sw:", STRN("pantograph B") },
	    { "pantfrontoff_sw:", STRN("pantograph A") },
	    { "pantrearoff_sw:", STRN("pantograph B") },
	    { "pantalloff_sw:", STRN("all pantographs") },
	    { "pantselected_sw:", STRN("selected pantograph") },
	    { "pantselectedoff_sw:", STRN("selected pantograph") },
	    { "pantselect_sw:", STRN("selected pantograph") },
	    { "pantvalves_sw:", STRN("selected pantograph") },
	    { "pantcompressor_sw:", STRN("pantograph compressor") },
	    { "pantcompressorvalve_sw:", STRN("pantograph 3 way valve") },
	    { "trainheating_sw:", STRN("heating") },
	    { "signalling_sw:", STRN("braking indicator") },
	    { "door_signalling_sw:", STRN("door locking") },
	    { "nextcurrent_sw:", STRN("current indicator source") },
	    { "instrumentlight_sw:", STRN("instrument light") },
	    { "dashboardlight_sw:", STRN("dashboard light") },
	    { "timetablelight_sw:", STRN("timetable light") },
	    { "cablight_sw:", STRN("interior light") },
	    { "cablightdim_sw:", STRN("interior light dimmer") },
	    { "compartmentlights_sw:", STRN("compartment lights") },
	    { "compartmentlightsoff_sw:", STRN("compartment lights") },
	    { "compartmentlightson_sw:", STRN("compartment lights") },
	    { "couplingdisconnect_sw:", STRN("coupling") },
	    { "battery_sw:", STRN("battery") },
	    { "batteryon_sw:", STRN("battery") },
	    { "batteryoff_sw:", STRN("battery") },
	    { "epbrake_bt:", STRN("ep brake") },
	    { "relayreset1_bt:", STRN("relay reset") },
	    { "relayreset2_bt:", STRN("relay reset") },
	    { "relayreset3_bt:", STRN("relay reset") },
	    { "springbrakeoff_bt:", STRN("spring brake") },
	    { "springbrakeon_bt:", STRN("spring brake") },
	    { "springbraketoggle_bt:", STRN("spring brake") },
	    { "universalbrake1_bt:", STRN("train brake") },
	    { "universalbrake2_bt:", STRN("train brake") },
	    { "universalbrake3_bt:", STRN("train brake") },
	    { "universal0:", STRN("interactive part") },
	    { "universal1:", STRN("interactive part") },
	    { "universal2:", STRN("interactive part") },
	    { "universal3:", STRN("interactive part") },
	    { "universal4:", STRN("interactive part") },
	    { "universal5:", STRN("interactive part") },
	    { "universal6:", STRN("interactive part") },
	    { "universal7:", STRN("interactive part") },
	    { "universal8:", STRN("interactive part") },
	    { "universal9:", STRN("interactive part") }
	};

	auto const it = cabcontrols_labels.find( Label );
    return (
	    it != cabcontrols_labels.end() ?
	        lookup_s(it->second) :
            "" );
}

const std::string& locale::coupling_name(int c)
{
	static std::unordered_map<coupling, std::string> coupling_names =
	{
	    { coupling::faux, STRN("faux") },
	    { coupling::coupler, STRN("coupler") },
	    { coupling::brakehose, STRN("brake hose") },
	    { coupling::control, STRN("control") },
	    { coupling::highvoltage, STRN("high voltage") },
	    { coupling::gangway, STRN("gangway") },
	    { coupling::mainhose, STRN("main hose") },
	    { coupling::heating, STRN("heating") },
	    { coupling::permanent, STRN("permanent") },
        { coupling::power24v, STRN("power 24V") },
        { coupling::power110v, STRN("power 110V") },
        { coupling::power3x400v, STRN("power 3x400V") },
	};

	static std::string unknown(STRN("unknown"));

	auto it = coupling_names.find(static_cast<coupling>(c));
	if (it != coupling_names.end())
		return lookup_s(it->second);
	else
		return lookup_s(unknown);
}

locale Translations;
