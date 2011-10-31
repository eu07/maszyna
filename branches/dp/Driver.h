//---------------------------------------------------------------------------

#ifndef DriverH
#define DriverH

#include "Classes.h"
#include <mover.hpp>	// Pascal unit
#include "dumb3d.h"
using namespace Math3D;

enum TOrders
{//rozkazy dla AI
 Wait_for_orders=0,       //czekanie na dostarczenie nast�pnych rozkaz�w
 //operacje tymczasowe
 Prepare_engine=1,        //w��czenie silnika
 Release_engine=2,        //wy��czenie silnika
 Change_direction=4,      //zmiana kierunku (bez skanowania sygnalizacji)
 Connect=8,               //pod��czanie wagon�w (bez skanowania sygnalizacji)
 Disconnect=0x10,         //od��czanie wagon�w (bez skanowania sygnalizacji)
 //jazda
 Shunt=0x20,              //tryb manewrowy
 Obey_train=0x40,         //tryb poci�gowy
 Jump_to_first_order=0x60 //zap�lenie do pierwszej pozycji (po co?)
};

enum TMovementStatus
{//flagi bitowe ruchu
 moveStopCloser=1, //podjecha� blisko W4 (nie podje�d�a� na pocz�tku ani po zmianie czo�a)
 moveStopPoint=2, //stawa� na W4 (wy��czone podczas zmiany czo�a)
 moveAvaken=4, //po w��czeniu silnika pojazd nie przemie�ci� si�
 movePress=8, //dociskanie przy od��czeniu (zamiast zmiennej Prepare2press)
 moveBackwardLook=16 //skanowanie tor�w w przeciwn� stron� w celu zmiany kierunku
};

enum TStopReason
{//pow�d zatrzymania, dodawany do SetVelocity 0
 stopNone,  //nie ma powodu - powinien jecha�
 stopSleep, //nie zosta� odpalony, to nie pojedzie
 stopSem,   //semafor zamkni�ty
 stopTime,  //czekanie na godzin� odjazdu
 stopEnd,   //brak dalszej cz�ci toru
 stopDir,   //trzeba stan��, by zmieni� kierunek jazdy
 stopJoin,  //stoi w celu po��czenia wagon�w
 stopBlock, //przeszkoda na drodze ruchu
 stopComm,  //otrzymano tak� komend� (niewiadomego pochodzenia)
 stopOut,   //komenda wyjazdu poza stacj� (raczej nie powinna zatrzymywa�!)
 stopRadio, //komunikat przekazany radiem (Radiostop)
 stopExt,   //komenda z zewn�trz
 stopError  //z powodu b��du w obliczeniu drogi hamowania
};
/*
struct TProximityTablePos
{
	double Dist;
	double Vel;
	double Acc;
	Byte Flag;
} ;
*/

//-- var, const, procedure ---------------------------------------------------
static const bool Aggressive=true;
static const bool Easyman=false;
static const bool AIdriver=true;
static const bool Humandriver=false;
static const int maxorders=0x20; //ilo�� rozkaz�w w tabelce
static const int maxdriverfails=0x4; //ile b��d�w mo�e zrobi� AI zanim zmieni nastawienie
extern bool WriteLogFlag; //logowanie parametr�w fizycznych

class TController
{
 double fShuntVelocity; //maksymalna pr�dko�� manewrowania, zale�y m.in. od sk�adu
 double fLength; //d�ugo�� sk�adu (dla ogranicze� i stawania przed semaforami)
 int iVehicles; //ilo�� pojazd�w w sk�adzie
 bool EngineActive; //ABu: Czy silnik byl juz zalaczony
 vector3 vMechLoc; //pozycja pojazdu do liczenia odleg�o�ci od semafora (?)
 bool Psyche;
 int iDrivigFlags; //flagi bitowe ruchu
public:
 double ReactionTime; //czas reakcji Ra: czego?
private:
 bool Ready; //ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow jest ustawiane w dynobj->cpp
 double LastUpdatedTime; //czas od ostatniego logu
 double ElapsedTime; //czas od poczatku logu
 double deltalog; //przyrost czasu
 double LastReactionTime;
 bool HelpMeFlag; //wystawiane True jesli cos niedobrego sie dzieje
public:
 bool AIControllFlag; //rzeczywisty/wirtualny maszynista
 bool OnStationFlag; //Czy jest na peronie
private:
 TDynamicObject *pVehicle; //pojazd w kt�rym siedzi steruj�cy
 TDynamicObject *pVehicles[2]; //skrajne pojazdy w sk�adzie
 Mover::TMoverParameters *Controlling; //jakim pojazdem steruje
 Mtable::TTrainParameters *TrainParams; //do jakiego pociagu nalezy
 int TrainNumber; //numer rozkladowy tego pociagu
 //AnsiString OrderCommand; //komenda pobierana z pojazdu
 //double OrderValue; //argument komendy
 double AccPreferred; //preferowane przyspieszenie
public:
 double AccDesired; //chwilowe przyspieszenie
 double VelDesired; //predkosc
private:
 double VelforDriver; //predkosc dla manewrow
 double VelActual; //predkosc do kt�rej d�zy
public:
 double VelNext; //predkosc przy nastepnym obiekcie
private:
 double fProximityDist; //odleglosc podawana w SetProximityVelocity()
public:
 double ActualProximityDist; //ustawia nowa predkosc do ktorej ma dazyc oraz predkosc przy nastepnym obiekcie
private:
 //TProximityTablePos ProximityTable[256];
 //double ReducedTable[256];
 //Byte ProximityTableIndex;
 //Byte LPTA;
 //Byte LPTI;
 vector3 vCommandLocation; //polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda
 TOrders OrderList[maxorders]; //lista rozkaz�w
 int OrderPos,OrderTop; //rozkaz aktualny oraz wolne miejsce do wstawiania nowych
 TextFile LogFile; //zapis parametrow fizycznych
 TextFile AILogFile; //log AI
 bool ScanMe; //flaga potrzeby skanowania toru dla DynObj.cpp
 bool MaxVelFlag;
 bool MinVelFlag;
 int iDirection; //kierunek jazdy wzgl�dem pojazdu, w kt�rym siedzi AI (1=prz�d,-1=ty�)
 int iDirectionOrder; //�adany kierunek jazdy (s�u�y do zmiany kierunku)
 int iVehicleCount; //ilo�� pojazd�w do od��czenia albo zabrania ze sk�adu (-1=wszystkie)
 bool Prepare2press; //dociskanie w celu od��czenia
 int iDriverFailCount; //licznik b��d�w AI
 bool Need_TryAgain;
 bool Need_BrakeRelease;
public:
 double fMinProximityDist; //minimalna oleg�o�� do przeszkody, jak� nale�y zachowa�
private:
 double fMaxProximityDist; //akceptowalna odleg�o�� stani�cia przed przeszkod�
 TStopReason eStopReason; //pow�d zatrzymania przy ustawieniu zerowej pr�dko�ci
 void __fastcall SetDriverPsyche();
 bool __fastcall PrepareEngine();
 bool __fastcall ReleaseEngine();
 bool __fastcall IncBrake();
 bool __fastcall DecBrake();
 bool __fastcall IncSpeed();
 bool __fastcall DecSpeed();
 void __fastcall RecognizeCommand(); //odczytuje komende przekazana lokomotywie
 void __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const Mover::TLocation &NewLocation,TStopReason reason=stopComm);
public:
 void __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const vector3 *NewLocation,TStopReason reason=stopComm);
 bool __fastcall UpdateSituation(double dt); //uruchamiac przynajmniej raz na sekunde
 //procedury dotyczace rozkazow dla maszynisty
 void __fastcall SetVelocity(double NewVel,double NewVelNext,TStopReason r=stopNone); //uaktualnia informacje o predkosci
 bool __fastcall SetProximityVelocity(double NewDist,double NewVelNext); //uaktualnia informacje o predkosci przy nastepnym semaforze
private:
 bool __fastcall AddReducedVelocity(double Distance, double Velocity, Byte Flag);
public:
 void __fastcall JumpToNextOrder();
 void __fastcall JumpToFirstOrder();
 void __fastcall OrderPush(TOrders NewOrder);
 void __fastcall OrderNext(TOrders NewOrder);
 TOrders __fastcall OrderCurrentGet();
 TOrders __fastcall OrderNextGet();
private:
 bool __fastcall CheckVehicles();
 void __fastcall CloseLog();
 void __fastcall OrderCheck();
public:
 void __fastcall OrdersInit(double fVel);
 void __fastcall OrdersDump();
 __fastcall TController
 (bool AI,
  TDynamicObject *NewControll,
  Mtable::TTrainParameters *NewTrainParams,
  bool InitPsyche
 );
 AnsiString __fastcall OrderCurrent();
 void __fastcall WaitingSet(double Seconds);
private:
 AnsiString VehicleName;
 double VelMargin;
 double fWarningDuration; //ile czasu jeszcze tr�bi�
 double fStopTime; //czas postoju przed dalsz� jazd� (np. na przystanku)
 double WaitingTime; //zliczany czas oczekiwania do samoistnego ruszenia
 double WaitingExpireTime; //maksymlany czas oczekiwania do samoistnego ruszenia
 AnsiString __fastcall Order2Str(TOrders Order);
 int __fastcall OrderDirectionChange(int newdir,Mover::TMoverParameters *Vehicle);
 void __fastcall Lights(int head,int rear);
 //Ra: poni�sze przenie�� do modu�u AI:
 TEvent* eSignSkip; //mini�ty sygna� zezwalaj�cy na jazd�, pomijany przy szukaniu
 AnsiString asNextStop; //nazwa nast�pnego punktu zatrzymania wg rozk�adu
public:
 TEvent* eSignLast; //ostatnio znaleziony sygna�, o ile nie mini�ty
private:
 TEvent* __fastcall CheckTrackEvent(double fDirection,TTrack *Track);
 TTrack* __fastcall TraceRoute(double &fDistance,double &fDirection,TTrack *Track,TEvent*&Event);
 void __fastcall SetProximityVelocity(double dist,double vel,const vector3 *pos);
 void __fastcall DirectionSet(bool forward);
public:
 //inline __fastcall TController() { };
 AnsiString __fastcall StopReasonText();
 __fastcall ~TController();
 void __fastcall ScanEventTrack();
 bool __fastcall CheckEvent(TEvent *e,bool prox);
 AnsiString __fastcall NextStop();
 void __fastcall TakeControl(bool yes);
};

#endif
