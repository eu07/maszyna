/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef ClassesH
#define ClassesH
//---------------------------------------------------------------------------
// Ra: zestaw klas do robienia wskaźników, aby uporządkować nagłówki
//---------------------------------------------------------------------------
class opengl_renderer;
class TTrack; // odcinek trajektorii
class basic_event;
class TTrain; // pojazd sterowany
class TDynamicObject; // pojazd w scenerii
struct material_data;
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
class TCamera;
class scenario_time;
class TMoverParameters;
class ui_layer;
class editor_ui;
class itemproperties_panel;
class event_manager;
class memory_table;
class powergridsource_table;
class instance_table;
class vehicle_table;
struct light_array;
class sound_table;
class shape_node;
class lines_node;

namespace scene {
struct node_data;
class basic_node;
}

namespace Mtable
{
class TTrainParameters; // rozkład jazdy
class TMtableTime; // czas dla danego posterunku
};

class TController; // obiekt sterujący pociągiem (AI)

enum class TCommandType
{ // binarne odpowiedniki komend w komórce pamięci
    cm_Unknown, // ciąg nierozpoznany (nie jest komendą)
    cm_Ready, // W4 zezwala na odjazd, ale semafor może zatrzymać
    cm_SetVelocity, // prędkość pociągowa zadawana na semaforze
    cm_RoadVelocity, // prędkość drogowa
    cm_SectionVelocity, //ograniczenie prędkości na odcinku
    cm_ShuntVelocity, // prędkość manewrowa na semaforze
    cm_SetProximityVelocity, // informacja wstępna o ograniczeniu
    cm_ChangeDirection,
    cm_PassengerStopPoint,
    cm_OutsideStation,
    cm_Shunt,
    cm_Command // komenda pobierana z komórki
};

#endif
