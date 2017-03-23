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
class TTrack; // odcinek trajektorii
class TEvent;
class TTrain; // pojazd sterowany
class TDynamicObject; // pojazd w scenerii
class TGroundNode; // statyczny obiekt scenerii
class TAnimModel; // opakowanie egzemplarz modelu
class TAnimContainer; // fragment opakowania egzemplarza modelu
class TModel3d; //siatka modelu wspólna dla egzemplarzy
class TSubModel; // fragment modelu (tu do wyświetlania terenu)
class TMemCell; // komórka pamięci
class cParser;
class TRealSound; // dźwięk ze współrzędnymi XYZ
class TTextSound; // dźwięk ze stenogramem
class TEventLauncher;
class TTraction; // drut
class TTractionPowerSource; // zasilanie drutów

namespace Mtable
{
class TTrainParameters; // rozkład jazdy
class TMtableTime; // czas dla danego posterunku
};

class TController; // obiekt sterujący pociągiem (AI)

typedef enum
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
} TCommandType;

#endif
