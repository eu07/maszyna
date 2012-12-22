//---------------------------------------------------------------------------

#ifndef ClassesH
#define ClassesH
//---------------------------------------------------------------------------
//Ra: zestaw klas do robienia wska�nik�w, aby uporz�dkowa� nag��wki
//---------------------------------------------------------------------------
class TTrack; //odcinek trajektorii
class TEvent;
class TTrain; //pojazd sterowany
class TDynamicObject; //pojazd w scenerii
class TGroundNode; //statyczny obiekt scenerii
class TAnimModel; //opakowanie egzemplarz modelu
class TAnimContainer; //fragment opakowania egzemplarza modelu
//class TModel3d; //siatka modelu wsp�lna dla egzemplarzy
class TSubModel; //fragment modelu (tu do wy�wietlania terenu)
class TMemCell; //kom�rka pami�ci
class cParser;
class TRealSound; //d�wi�k ze wsp�rz�dnymi XYZ
class TEventLauncher;
class TTraction; //drut
class TTractionPowerSource; //zasilanie drut�w

class TMoverParameters;
namespace _mover
{
class TLocation;
class TRotation;
};

namespace Mtable
{
class TTrainParameters; //rozk�ad jazdy
};

class TController; //obiekt steruj�cy poci�giem (AI)
class TNames; //obiekt sortuj�cy nazwy

typedef enum
{//binarne odpowiedniki komend w kom�rce pami�ci
 cm_Unknown, //ci�g nierozpoznany (nie jest komend�)
 cm_SetVelocity,
 cm_ShuntVelocity,
 cm_SetProximityVelocity,
 cm_ChangeDirection,
 cm_PassengerStopPoint,
 cm_OutsideStation,
 cm_Command //komenda pobierana z kom�rki
} TCommandType;

#endif
