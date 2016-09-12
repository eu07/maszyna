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
// Ra: zestaw klas do robienia wskaŸników, aby uporz¹dkowaæ nag³ówki
//---------------------------------------------------------------------------
class TTrack; // odcinek trajektorii
class TEvent;
class TTrain; // pojazd sterowany
class TDynamicObject; // pojazd w scenerii
class TGroundNode; // statyczny obiekt scenerii
class TAnimModel; // opakowanie egzemplarz modelu
class TAnimContainer; // fragment opakowania egzemplarza modelu
// class TModel3d; //siatka modelu wspólna dla egzemplarzy
class TSubModel; // fragment modelu (tu do wyœwietlania terenu)
class TMemCell; // komórka pamiêci
class cParser;
class TRealSound; // dŸwiêk ze wspó³rzêdnymi XYZ
class TTextSound; // dŸwiêk ze stenogramem
class TEventLauncher;
class TTraction; // drut
class TTractionPowerSource; // zasilanie drutów

class TMoverParameters;
namespace _mover
{
class TLocation;
class TRotation;
};

namespace Mtable
{
class TTrainParameters; // rozk³ad jazdy
class TMtableTime; // czas dla danego posterunku
};

class TController; // obiekt steruj¹cy poci¹giem (AI)
class TNames; // obiekt sortuj¹cy nazwy

typedef enum
{ // binarne odpowiedniki komend w komórce pamiêci
    cm_Unknown, // ci¹g nierozpoznany (nie jest komend¹)
    cm_Ready, // W4 zezwala na odjazd, ale semafor mo¿e zatrzymaæ
    cm_SetVelocity, // prêdkoœæ poci¹gowa zadawana na semaforze
    cm_RoadVelocity, // prêdkoœæ drogowa
    cm_SectionVelocity, //ograniczenie prêdkoœci na odcinku
    cm_ShuntVelocity, // prêdkoœæ manewrowa na semaforze
    cm_SetProximityVelocity, // informacja wstêpna o ograniczeniu
    cm_ChangeDirection,
    cm_PassengerStopPoint,
    cm_OutsideStation,
    cm_Shunt,
    cm_Command // komenda pobierana z komórki
} TCommandType;

#endif
