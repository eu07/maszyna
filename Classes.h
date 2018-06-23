/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

//---------------------------------------------------------------------------
// Ra: zestaw klas do robienia wskaźników, aby uporządkować nagłówki
//---------------------------------------------------------------------------
struct material_data;

class TTrack; // odcinek trajektorii
class TEvent;
class TTrain; // pojazd sterowany
class TDynamicObject; // pojazd w scenerii
class TAnimModel; // opakowanie egzemplarz modelu
class TAnimContainer; // fragment opakowania egzemplarza modelu
class TModel3d; //siatka modelu wspólna dla egzemplarzy
class TSubModel; // fragment modelu (tu do wyświetlania terenu)
class TMemCell; // komórka pamięci
class cParser;
class sound_source;
class TEventLauncher;
class TTraction; // drut
class TTractionPowerSource; // zasilanie drutów
class TWorld;
class TCamera;
class simulation_time;
class TMoverParameters;
class TController; // obiekt sterujący pociągiem (AI)

namespace scene
{
	struct node_data;
}

namespace Mtable
{
	class TTrainParameters; // rozkład jazdy
	class TMtableTime; // czas dla danego posterunku
};

enum TCommandType { // binarne odpowiedniki komend w komórce pamięci
	CM_UNKNOWN, // ciąg nierozpoznany (nie jest komendą)
	CM_READY, // W4 zezwala na odjazd, ale semafor może zatrzymać
	CM_SET_VELOCITY, // prędkość pociągowa zadawana na semaforze
	CM_ROAD_VELOCITY, // prędkość drogowa
	CM_SECTION_VELOCITY, //ograniczenie prędkości na odcinku
	CM_SHUNT_VELOCITY, // prędkość manewrowa na semaforze
	CM_SET_PROXIMITY_VELOCITY, // informacja wstępna o ograniczeniu
	CM_CHANGE_DIRECTION,
	CM_PASSENGER_STOP_POINT,
	CM_OUTSIDE_STATION,
	CM_SHUNT,
	CM_COMMAND // komenda pobierana z komórki
};
