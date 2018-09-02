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

#include "globals.h"

namespace locale {

void
init() {
    // TODO: import localized strings from localization files
    std::unordered_map<std::string, std::vector<std::string>> stringmap;

    stringmap.insert(
    { "en",
      {
        "Driving Aid",
        "Throttle:  %2d+%d %c%s",
        " Speed: %d km/h (limit %d km/h%s)%s",
        ", new limit: %d km/h in %.1f km",
        " Grade: %.1f%%",
        "Brakes:  %4.1f+%-2.0f%c%s",
        " Pressure: %.2f kPa (train pipe: %.2f kPa)",
        "!ALERTER! ",
        "!SHP!",
        " Loading/unloading in progress (%d s left)",
        " Another vehicle ahead (distance: %.1f m)",

        "Timetable",
        "Time: %d:%02d:%02d",
        "(no timetable)",

        "Transcripts",

        "Simulation Paused",
        "Resume",
        "Quit",

        "Name: %s%s\nLoad: %d %s\nStatus: %s%s\nCouplers:\n front: %s\n rear:  %s",
        ", owned by: ",
        "none",
        "Devices: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%s\nPower transfers: %.0f@%.0f%s%s%s%.0f@%.0f",
        " radio: ",
        " oil pressure: ",
        "Controllers:\n master: %d(%d), secondary: %s\nEngine output: %.1f, current: %.0f\nRevolutions:\n engine: %.0f, motors: %.0f, ventilators: %.0f, fans: %.0f+%.0f",
        " (shunt mode)",
        "\nTemperatures:\n engine: %.2f, oil: %.2f, water: %.2f%c%.2f",
        "Brakes:\n train: %.2f, independent: %.2f, delay: %s, load flag: %d\nBrake cylinder pressures:\n train: %.2f, independent: %.2f, status: 0x%.2x\nPipe pressures:\n brake: %.2f (hat: %.2f), main: %.2f, control: %.2f\nTank pressures:\n auxiliary: %.2f, main: %.2f, control: %.2f",
        " pantograph: %.2f%cMT",
        "Forces:\n tractive: %.1f, brake: %.1f, friction: %.2f%s\nAcceleration:\n tangential: %.2f, normal: %.2f (path radius: %s)\nVelocity: %.2f, distance traveled: %.2f\nPosition: [%.2f, %.2f, %.2f]"
      }
    } );

    stringmap.insert(
    { "pl",
      {
        "Pomocnik",
        "Nastawnik: %2d+%d %c%s",
        " Predkosc: %d km/h (limit %d km/h%s)%s",
        ", nowy limit: %d km/h za %.1f km",
        " Pochylenie: %.1f%%",
        "Hamulce: %4.1f+%-2.0f%c%s",
        " Cisnienie: %.2f kPa (przewod glowny: %.2f kPa)",
        "!CZUWAK!  ",
        "!SHP!",
        " Wsiadanie/wysiadanie pasazerow (do zakonczenia %d s)",
        " Inny pojazd na drodze (odleglosc: %.1f m)",

        "Rozklad jazdy",
        "Godzina: %d:%02d:%02d",
        "(brak rozkladu)",

        "Transkrypcje",

        "Symulacja wstrzymana",
        "Wznow",
        "Zakoncz",

        "Nazwa: %s%s\nLadunek: %d %s\nStatus: %s%s\nSprzegi:\n przedni: %s\n tylny:   %s",
        ", wlasciciel: ",
        "wolny",
        "Urzadzenia: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%s\nTransfer pradow: %.0f@%.0f%s%s%s%.0f@%.0f",
        " radio: ",
        " cisn.oleju: ",
        "Nastawniki:\n glowny: %d(%d), dodatkowy: %s\nMoc silnika: %.1f, prad silnika: %.0f\nObroty:\n silnik: %.0f, motory: %.0f, went.silnika: %.0f, went.chlodnicy: %.0f+%.0f",
        " (tryb manewrowy)",
        "\nTemperatury:\n silnik: %.2f, olej: %.2f, woda: %.2f%c%.2f",
        "Hamulce:\n zespolony: %.2f, pomocniczy: %.2f, nastawa: %s, ladunek: %d\nCisnienie w cylindrach:\n zespolony: %.2f, pomocniczy: %.2f, status: 0x%.2x\nCisnienia w przewodach:\n glowny: %.2f (kapturek: %.2f), zasilajacy: %.2f, kontrolny: %.2f\nCisnienia w zbiornikach:\n pomocniczy: %.2f, glowny: %.2f, sterujacy: %.2f",
        " pantograf: %.2f%cZG",
        "Sily:\n napedna: %.1f, hamowania: %.1f, tarcie: %.2f%s\nPrzyspieszenia:\n styczne: %.2f, normalne: %.2f (promien: %s)\nPredkosc: %.2f, pokonana odleglosc: %.2f\nPozycja: [%.2f, %.2f, %.2f]"
      }
    } );

    auto const lookup = stringmap.find( Global.asLang );

    locale::strings = (
        lookup != stringmap.end() ?
            lookup->second :
            stringmap.find( "en" )->second );
}

std::string
label_cab_control( std::string const &Label ) {

    auto const lookup = m_cabcontrols.find( Label );
    return (
        lookup != m_cabcontrols.end() ?
            lookup->second :
            "" );
}

std::vector<std::string> strings;

std::unordered_map<std::string, std::string> m_cabcontrols = {
    { "mainctrl:", "master controller" },
    { "scndctrl:", "second controller" },
    { "shuntmodepower:", "shunt mode power" },
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
    { "whistle_bt:", "whistle" },
    { "fuse_bt:", "motor overload relay reset" },
    { "converterfuse_bt:", "converter overload relay reset" },
    { "stlinoff_bt:", "motor connectors" },
    { "door_left_sw:", "left door" },
    { "door_right_sw:", "right door" },
    { "doorlefton_sw:", "left door" },
    { "doorrighton_sw:", "right door" },
    { "doorleftoff_sw:", "left door" },
    { "doorrightoff_sw:", "right door" },
    { "dooralloff_sw:", "all doors" },
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

} // namespace locale

//---------------------------------------------------------------------------
