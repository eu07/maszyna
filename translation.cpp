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

	/*
    // TODO: import localized strings from localization files
    std::unordered_map<std::string, std::vector<std::string>> stringmap;

    stringmap.insert(
    { "en",
      {
        "Driving Aid",
        "Throttle:  %2d+%d %c%s",
        " Speed: %d km/h (limit %d km/h%s)%s",
        ", new limit: %d km/h in %.1f km",
        " Grade: %.1f%%%%",
        "Brakes:  %4.1f+%-2.0f%c%s",
        " Pressure: %.2f kPa (train pipe: %.2f kPa)",
        "!ALERTER! ",
        "!SHP!",
        " Loading/unloading in progress (%d s left)",
        " Another vehicle ahead (distance: %.1f m)",

        "Scenario",
        "Assignment",
        "Current task:",
        "Wait for orders",
        "Start the engine",
        "Shut down the engine",
        "Change direction",
        "Couple to consist ahead",
        "Uncouple %s",
        "the engine",
        "the engine plus the next vehicle",
        "the engine plus %d next vehicles",
        "the engine plus %d next vehicles",
        "Shunt according to signals",
        "Loose shunt according to signals",
        "Drive according to signals and timetable",
        "Bank consist ahead",

        "%-*.*s    Time: %d:%02d:%02d", // HACK: some manual padding to account for longer 'time' equivalent in polish version
        "Timetable",
        "(no timetable)",
        "Consist weight: %d t (specified) %d t (actual)\nConsist length: %d m",

        "Transcripts",

        "Simulation Paused",
        "Resume",
        "Quit",

        "Name: %s%s\nLoad: %.0f %s\nStatus: %s%s\nCouplers:\n front: %s\n rear:  %s",
        ", owned by: ",
        "none",
        "Devices: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%s\nPower transfers: %.0f@%.0f%s%s%s%.0f@%.0f",
        " radio: ",
        " oil pressure: ",
        "Controllers:\n master: %d(%d), secondary: %s\nEngine output: %.1f, current: %.0f\nRevolutions:\n engine: %.0f, motors: %.0f\n engine fans: %.0f, motor fans: %.0f+%.0f, cooling fans: %.0f+%.0f",
        " (shunt mode)",
        "\nTemperatures:\n engine: %.2f, oil: %.2f, water: %.2f%c%.2f",
	    "Brakes:\n train: %.2f, independent: %.2f, mode: %d, delay: %s, load flag: %d\nBrake cylinder pressures:\n train: %.2f, independent: %.2f, status: 0x%.2x\nPipe pressures:\n brake: %.2f (hat: %.2f), main: %.2f, control: %.2f\nTank pressures:\n auxiliary: %.2f, main: %.2f, control: %.2f",
        " pantograph: %.2f%cMT",
        "Forces:\n tractive: %.1f, brake: %.1f, friction: %.2f%s\nAcceleration:\n tangential: %.2f, normal: %.2f (path radius: %s)\nVelocity: %.2f, distance traveled: %.2f\nPosition: [%.2f, %.2f, %.2f]",

        "expand",
        "Loading",
        "Loading scenery",
        "Quit simulation?",
        "Yes",
        "No",
        "General",
        "Debug mode",
        "Quit",
        "Tools",
        "Logging to log.txt",
        "Screenshot",
        "Windows",
        "Log",
        "Map",
        "Mode windows",

	    "Time and environment",
	    "Time",
	    "Day in year",
	    "Visibility",
	    "Overcast and precipitation",
	    "Temperature",
	    "Apply",

	    "Straight |",
	    "Divert /",
	    "Open |",
	    "Close -",
	    "Insert obstacle:",
	    "Delete obstacle",

	    "Vehicle list",

	    "Vehicle parameters",
	    "Radiostop",
	    "Reset position",
	    "Stop and repair",
	    "Refill main tank",
	    "Rupture main pipe",
	    "Move +500m",
	    "Move -500m",

	    "Camera preview",

        "master controller",
        "master controller",
        "second controller",
        "shunt mode power",
        "tempomat",
        "reverser",
        "train brake",
        "independent brake",
        "manual brake",
        "emergency brake",
        "brake acting speed",
        "brake acting speed: cargo",
        "brake acting speed: rapid",
	    "brake operation mode",
        "motor overload relay threshold",
        "water pump",
        "water pump breaker",
        "water heater",
        "water heater breaker",
        "water circuits link",
        "fuel pump",
        "oil pump",
        "motor blowers A",
        "motor blowers B",
        "all motor blowers",
        "cooling fans",
        "line breaker",
        "line breaker",
        "alerter",
        "independent brake releaser",
        "sandbox",
        "wheelspin brake",
        "horn",
        "low tone horn",
        "high tone horn",
        "whistle",
        "motor overload relay reset",
        "converter overload relay reset",
        "motor connectors",
        "left door (permit)",
        "right door (permit)",
        "door (permit)",
        "left door",
        "right door",
        "left door (open)",
        "right door (open)",
        "left door (close)",
        "right door (close)",
        "all doors (open)",
        "all doors (close)",
        "doorstep",
        "door control mode",
        "departure signal",
        "upper headlight",
        "left headlight",
        "right headlight",
        "headlights dimmer",
        "left marker light",
        "right marker light",
        "light pattern",
        "rear upper headlight",
        "rear left headlight",
        "rear right headlight",
        "rear left marker light",
        "rear right marker light",
        "compressor",
        "local compressor",
        "converter",
        "local converter",
        "converter",
        "line breaker",
        "radio",
        "radio channel",
        "radio channel",
        "radio channel",
        "radiostop test",
        "radiostop",
        "pantograph A",
        "pantograph B",
        "pantograph A",
        "pantograph B",
        "all pantographs",
        "selected pantograph",
        "selected pantograph",
        "pantograph compressor",
        "pantograph 3 way valve",
        "heating",
        "braking indicator",
        "door locking",
        "current indicator source",
        "instrument light",
        "dashboard light",
        "timetable light",
        "interior light",
        "interior light dimmer",
        "battery",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part",
        "interactive part"
      }
    } );

    stringmap.insert(
    { "pl",
      {
        u8"Pomocnik",
        u8"Nastawnik: %2d+%d %c%s",
        u8" Prędkość: %d km/h (limit %d km/h%s)%s",
        u8", nowy limit: %d km/h za %.1f km",
        u8" Nachylenie: %.1f%%%%",
        u8"Hamulce: %4.1f+%-2.0f%c%s",
        u8" Ciśnienie: %.2f kPa (przewód główny: %.2f kPa)",
        u8"!CZUWAK!  ",
        u8"!SHP!",
        u8" Wsiadanie/wysiadanie pasażerów (do zakończenia %d s)",
        u8" Inny pojazd na drodze (odległość: %.1f m)",

        u8"Scenariusz",
        u8"Zlecenie",
        u8"Bieżace zadanie:",
        u8"Oczekiwać dalszych poleceń",
        u8"Przygotować pojazd do jazdy",
        u8"Wyłączyc pojazd",
        u8"Zmienić kierunek jazdy",
        u8"Sprząc się ze składem z przodu",
        u8"Odpiąć %s",
        u8"pojazd prowadzący",
        u8"pojazd prowadzący plus kolejny",
        u8"pojazd prowadzący plus %d kolejne",
        u8"pojazd prowadzący plus %d kolejnych",
        u8"Prowadzić manewry według sygnałów",
        u8"Prowadzić manewry przetaczania odrzutem",
        u8"Prowadzić skład według sygnałów i rozkładu",
        u8"Popychać skład z przodu",

        u8"%-*.*s Godzina: %d:%02d:%02d",
        u8"Rozkład jazdy",
        u8"(brak rozkładu)",
        u8"Brutto: %d t (rozkładowe) %d t (rzeczywiste)\nDługość pociągu: %d m",

        u8"Transkrypcje",

        u8"Symulacja wstrzymana",
        u8"Wznów",
        u8"Zakończ",

        u8"Nazwa: %s%s\nŁadunek: %.0f %s\nStatus: %s%s\nSprzęgi:\n przedni: %s\n tylny:   %s",
        u8", wlaściciel: ",
        u8"wolny",
        u8"Urządzenia: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%s\nPrzepływ prądu: %.0f@%.0f%s%s%s%.0f@%.0f",
        u8" radio: ",
        u8" cisń. oleju: ",
        u8"Nastawniki:\n glówny: %d(%d), dodatkowy: %s\nMoc silnika: %.1f, prąd silnika: %.0f\nObroty:\n silnik: %.0f, motory: %.0f, went.silnika: %.0f, went.chłodnicy: %.0f+%.0f",
        u8" (tryb manewrowy)",
        u8"\nTemperatury:\n silnik: %.2f, olej: %.2f, woda: %.2f%c%.2f",
	    u8"Hamulce:\n zespolony: %.2f, pomocniczy: %.2f, tryb: %d, nastawa: %s, ładunek: %d\nCiśnienie w cylindrach:\n zespolony: %.2f, pomocniczy: %.2f, status: 0x%.2x\nCiśnienia w przewodach:\n główny: %.2f (kapturek: %.2f), zasilający: %.2f, kontrolny: %.2f\nCiśnienia w zbiornikach:\n pomocniczy: %.2f, główny: %.2f, sterujący: %.2f",
        u8" pantograf: %.2f%cZG",
        u8"Siły:\n napędna: %.1f, hamowania: %.1f, tarcie: %.2f%s\nPrzyśpieszenia:\n styczne: %.2f, normalne: %.2f (promień: %s)\nPrędkość: %.2f, pokonana odleglość: %.2f\nPozycja: [%.2f, %.2f, %.2f]",

        u8"rozszerz",
        u8"Ładowanie",
        u8"Wczytywanie scenerii",
        u8"Zakończyć symulację?",
        u8"Tak",
        u8"Nie",
        u8"Ogólne",
        u8"Tryb debug",
        u8"Wyjdź",
        u8"Narzędzia",
        u8"Logowanie do log.txt",
        u8"Zrzut ekranu",
        u8"Okna",
        u8"Log",
        u8"Mapa",
        u8"Okna trybu",

	    u8"Czas i środowisko",
	    u8"Czas",
	    u8"Dzień w roku",
	    u8"Widoczność",
	    u8"Zachmurzenie i opady",
	    u8"Temperatura",
	    u8"Zastosuj",

	    u8"Prosto |",
	    u8"W bok /",
	    u8"Otwórz |",
	    u8"Zamknij -",
	    u8"Wstaw przeszkodę:",
	    u8"Usuń przeszkodę",

	    u8"Lista pojazdów",

	    u8"Parametry pojazdu",
	    u8"Radiostop",
	    u8"Zresetuj pozycję",
	    u8"Zatrzymaj i napraw",
	    u8"Napełnij ZG",
	    u8"Zerwij PG",
	    u8"Przesuń +500m",
	    u8"Przesuń -500m",

	    u8"Podgląd kamery",

        u8"nastawnik jazdy",
        u8"nastawnik jazdy",
        u8"nastawnik dodatkowy",
        u8"sterowanie analogowe",
        u8"tempomat",
        u8"nastawnik kierunku",
        u8"hamulec zespolony",
        u8"hamulec pomocniczy",
        u8"hamulec ręczny",
        u8"hamulec bezpieczeństwa",
        u8"nastawa hamulca",
        u8"nastawa hamulca: towarowy",
        u8"nastawa hamulca: pośpieszny",
	    u8"tryb pracy hamulca",
        u8"zakres prądu rozruchu",
        u8"pompa wody",
        u8"wyłącznik samoczynny pompy wody",
        u8"podgrzewacz wody",
        u8"wyłącznik samoczynny podgrzewacza wody",
        u8"zawór połaczenia obiegow wody",
        u8"pompa paliwa",
        u8"pompa oleju",
        u8"wentylatory silników trakcyjnych A",
        u8"wentylatory silników trakcyjnych B",
        u8"wszystkie wentylatory silników trakcyjnych",
        u8"wentylatory oporów rozruchowych",
        u8"wyłącznik szybki",
        u8"wyłącznik szybki",
        u8"czuwak",
        u8"odluźniacz",
        u8"piasecznica",
        u8"hamulec przeciwpoślizgowy",
        u8"syrena",
        u8"syrena (ton niski)",
        u8"syrena (ton wysoki)",
        u8"gwizdawka",
        u8"przekaźnik nadmiarowy silników trakcyjnych",
        u8"przekaźnik nadmiarowy przetwornicy",
        u8"styczniki liniowe",
        u8"drzwi lewe (zezwól)",
        u8"drzwi prawe (zezwól)",
        u8"drzwi (zezwól)",
        u8"drzwi lewe",
        u8"drzwi prawe",
        u8"drzwi lewe (otwórz)",
        u8"drzwi prawe (otwórz)",
        u8"drzwi lewe (zamknij)",
        u8"drzwi prawe (zamknij)",
        u8"drzwi (otwórz)",
        u8"drzwi (zamknij)",
        u8"stopień drzwi",
        u8"tryb sterowania drzwiami",
        u8"sygnal odjazdu",
        u8"reflektor gorny",
        u8"reflektor lewy",
        u8"reflektor prawy",
        u8"przyciemnienie reflektórow",
        u8"sygnal lewy",
        u8"sygnal prawy",
        u8"programator świateł",
        u8"tylny reflektor górny",
        u8"tylny reflektor lewy",
        u8"tylny reflektor prawy",
        u8"tylny sygnal lewy",
        u8"tylny sygnal prawy",
        u8"sprężarka",
        u8"sprężarka lokalna",
        u8"przetwornica",
        u8"przetwornica lokalna",
        u8"przetwornica",
        u8"wyłącznik szybki",
        u8"radio",
        u8"kanał radia",
        u8"kanał radia",
        u8"kanał radia",
        u8"test radiostopu",
        u8"radiostop",
        u8"pantograf A",
        u8"pantograf B",
        u8"pantograf A",
        u8"pantograf B",
        u8"wszystkie pantografy",
        u8"wybrany pantograf",
        u8"wybrany pantograf",
        u8"sprężarka pantografow",
        u8"kurek trójdrogowy pantografow",
        u8"ogrzewanie pociągu",
        u8"sygnalizacja hamowania",
        u8"blokada drzwi",
        u8"prądy drugiego członu",
        u8"oświetlenie przyrządow",
        u8"oświetlenie pulpitu",
        u8"oświetlenie rozkładu jazdy",
        u8"oświetlenie kabiny",
        u8"przyciemnienie oświetlenia kabiny",
        u8"bateria",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy",
        u8"element ruchomy"
      }
    } );

    auto lookup { stringmap.find( Global.asLang ) };
    if( lookup == stringmap.end() ) {
        lookup = stringmap.find( "en" );
    }

    locale::strings = lookup->second;

    // prepare cab controls translation table
    {
        std::vector<std::string> cabcontrols = {
            "mainctrl:",
            "jointctrl:",
            "scndctrl:",
            "shuntmodepower:",
            "tempomat_sw:",
            "dirkey:",
            "brakectrl:",
            "localbrake:",
            "manualbrake:",
            "alarmchain:",
            "brakeprofile_sw:",
            "brakeprofileg_sw:",
            "brakeprofiler_sw:",
		    "brakeopmode_sw",
            "maxcurrent_sw:",
            "waterpump_sw:",
            "waterpumpbreaker_sw:",
            "waterheater_sw:",
            "waterheaterbreaker_sw:",
            "watercircuitslink_sw:",
            "fuelpump_sw:",
            "oilpump_sw:",
            "motorblowersfront_sw:",
            "motorblowersrear_sw:",
            "motorblowersalloff_sw:",
            "coolingfans_sw:",
            "main_off_bt:",
            "main_on_bt:",
            "security_reset_bt:",
            "releaser_bt:",
            "sand_bt:",
            "antislip_bt:",
            "horn_bt:",
            "hornlow_bt:",
            "hornhigh_bt:",
            "whistle_bt:",
            "fuse_bt:",
            "converterfuse_bt:",
            "stlinoff_bt:",
            "doorleftpermit_sw:",
            "doorrightpermit_sw:",
            "doorpermitpreset_sw:",
            "door_left_sw:",
            "door_right_sw:",
            "doorlefton_sw:",
            "doorrighton_sw:",
            "doorleftoff_sw:",
            "doorrightoff_sw:",
            "doorallon_sw:",
            "dooralloff_sw:",
            "doorstep_sw:",
            "doormode_sw",
            "departure_signal_bt:",
            "upperlight_sw:",
            "leftlight_sw:",
            "rightlight_sw:",
            "dimheadlights_sw:",
            "leftend_sw:",
            "rightend_sw:",
            "lights_sw:",
            "rearupperlight_sw:",
            "rearleftlight_sw:",
            "rearrightlight_sw:",
            "rearleftend_sw:",
            "rearrightend_sw:",
            "compressor_sw:",
            "compressorlocal_sw:",
            "converter_sw:",
            "converterlocal_sw:",
            "converteroff_sw:",
            "main_sw:",
            "radio_sw:",
            "radiochannel_sw:",
            "radiochannelprev_sw:",
            "radiochannelnext_sw:",
            "radiotest_sw:",
            "radiostop_sw:",
            "pantfront_sw:",
            "pantrear_sw:",
            "pantfrontoff_sw:",
            "pantrearoff_sw:",
            "pantalloff_sw:",
            "pantselected_sw:",
            "pantselectedoff_sw:",
            "pantcompressor_sw:",
            "pantcompressorvalve_sw:",
            "trainheating_sw:",
            "signalling_sw:",
            "door_signalling_sw:",
            "nextcurrent_sw:",
            "instrumentlight_sw:",
            "dashboardlight_sw:",
            "timetablelight_sw:",
            "cablight_sw:",
            "cablightdim_sw:",
            "battery_sw:",
            "universal0:",
            "universal1:",
            "universal2:",
            "universal3:",
            "universal4:",
            "universal5:",
            "universal6:",
            "universal7:",
            "universal8:",
            "universal9:"
        };

        std::size_t stringidx { string::cab_mainctrl };
        for( auto const &cabcontrol : cabcontrols ) {
            m_cabcontrols.insert( { cabcontrol, strings[ stringidx++ ] } );
        }
    }
	*/

void locale::init()
{
	std::fstream stream("lang/" + Global.asLang + ".po", std::ios_base::in | std::ios_base::binary);

	if (!stream.is_open()) {
		WriteLog("translation: cannot open lang file: " + Global.asLang);
		return;
	}

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
	    { "mainctrl:", "master controller" },
	    { "jointctrl:", "master controller" },
	    { "scndctrl:", "second controller" },
	    { "shuntmodepower:", "shunt mode power" },
	    { "tempomat_sw:", "tempomat" },
	    { "dirkey:", "reverser" },
	    { "brakectrl:", "train brake" },
	    { "localbrake:", "independent brake" },
	    { "manualbrake:", "manual brake" },
	    { "alarmchain:", "emergency brake" },
	    { "brakeprofile_sw:", "brake acting speed" },
	    { "brakeprofileg_sw:", "brake acting speed: cargo" },
	    { "brakeprofiler_sw:", "brake acting speed: rapid" },
	    { "brakeopmode_sw", "brake operation mode" },
	    { "maxcurrent_sw:", "motor overload relay threshold" },
	    { "waterpump_sw:", "water pump" },
	    { "waterpumpbreaker_sw:", "water pump breaker" },
	    { "waterheater_sw:", "water heater" },
	    { "waterheaterbreaker_sw:", "water heater breaker" },
	    { "watercircuitslink_sw:", "water circuits link" },
	    { "fuelpump_sw:", "fuel pump" },
	    { "oilpump_sw:", "oil pump" },
	    { "motorblowersfront_sw:", "motor blowers A" },
	    { "motorblowersrear_sw:", "motor blowers B" },
	    { "motorblowersalloff_sw:", "all motor blowers" },
	    { "coolingfans_sw:", "cooling fans" },
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
	    { "doorleftpermit_sw:", "left door (permit)" },
	    { "doorrightpermit_sw:", "right door (permit)" },
	    { "doorpermitpreset_sw:", "door (permit)" },
	    { "door_left_sw:", "left door" },
	    { "door_right_sw:", "right door" },
	    { "doorlefton_sw:", "left door (open)" },
	    { "doorrighton_sw:", "right door (open)" },
	    { "doorleftoff_sw:", "left door (close)" },
	    { "doorrightoff_sw:", "right door (close)" },
	    { "doorallon_sw:", "all doors (open)" },
	    { "dooralloff_sw:", "all doors (close)" },
	    { "doorstep_sw:", "doorstep" },
	    { "doormode_sw", "door control mode" },
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
	    { "rearrightend_sw:", "rear right marker light" },
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
	    { "dashboardlight_sw:", "dashboard light" },
	    { "timetablelight_sw:", "timetable light" },
	    { "cablight_sw:", "interior light" },
	    { "cablightdim_sw:", "interior light dimmer" },
	    { "battery_sw:", "battery" },
	    { "universal0:", "interactive part" },
	    { "universal1:", "interactive part" },
	    { "universal2:", "interactive part" },
	    { "universal3:", "interactive part" },
	    { "universal4:", "interactive part" },
	    { "universal5:", "interactive part" },
	    { "universal6:", "interactive part" },
	    { "universal7:", "interactive part" },
	    { "universal8:", "interactive part" },
	    { "universal9:", "interactive part" }
	};

	auto const it = cabcontrols_labels.find( Label );
    return (
	    it != cabcontrols_labels.end() ?
	        lookup_s(it->second) :
            "" );
}

locale Translations;
