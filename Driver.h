//---------------------------------------------------------------------------

#ifndef DriverH
#define DriverH

// Ra: poprawiony rêcznie plik nag³ówkowy

#include "Classes.h"
#include <mover.hpp>	// Pascal unit

//namespace Ai_driver {
//-- type declarations -------------------------------------------------------
enum TOrders
{//rozkazy dla AI
 Wait_for_orders=0,       //czekanie na dostarczenie nastêpnych rozkazów
 //operacje tymczasowe
 Prepare_engine=1,        //w³¹czenie silnika
 Release_engine=2,        //wy³¹czenie silnika
 Change_direction=4,      //zmiana kierunku
 Connect=8,               //pod³¹czanie wagonów
 Disconnect=0x10,         //od³¹czanie wagonów
 //jazda
 Shunt=0x10,              //tryb manewrowy
 Obey_train=0x20,         //tryb poci¹gowy
 Jump_to_first_order=0x30 //zapêlenie do pierwszej pozycji (po co?)
};

enum TStopReason
{//powód zatrzymania
 stopNone,  //nie ma powodu - powinien jechaæ
 stopSleep, //nie zosta³ odpalony, to nie pojedzie
 stopSem,   //semafor zamkniêty
 stopTime,  //czekanie na godzinê odjazdu
 stopEnd,   //brak dalszej czêœci toru
 stopDir,   //trzeba stan¹æ, by zmieniæ kierunek jazdy
 stopJoin,  //stoi w celu po³¹czenia wagonów
 stopBlock, //przeszkoda na drodze ruchu
 stopComm,  //otrzymano tak¹ komendê (niewiadomego pochodzenia)
 stopOut,   //komenda wyjazdu poza stacjê (raczej nie powinna zatrzymywaæ!)
 stopRadio, //komunikat przekazany radiem (Radiostop)
 stopError  //z powodu b³êdu w obliczeniu drogi hamowania
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
static const bool Aggressive = true;
static const bool Easyman = false;
static const bool AIdriver = true;
static const bool Humandriver = false;
static const int maxorders = 0x10;
static const int maxdriverfails = 0x4;
extern bool WriteLogFlag;
/*
static const int rvTrack = 0x1;
static const int rvSignal = 0x2;
static const int rvVechicle = 0x3;
static const int rvShunt = 0x4;
static const int rvPassStop = 0x5;
*/

class TController
{
 double fShuntVelocity; //maksymalna prêdkoœæ manewrowania, zale¿y m.in. od sk³adu
 double fLength; //d³ugoœæ sk³adu (dla ograniczeñ i stawania przed semaforami)
 bool bCheckVehicles; //nale¿y sprawdziæ pojazdy i ustawiæ œwiat³a
 bool EngineActive; //ABu: Czy silnik byl juz zalaczony
 Mover::TLocation MechLoc;
 Mover::TRotation MechRot;
 bool Psyche;
public:
 double ReactionTime; //czas reakcji Ra: czego?
 bool Ready; //ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow jest ustawiane w dynobj->cpp
private:
 double LastUpdatedTime; //czas od ostatniego logu
 double ElapsedTime; //czas od poczatku logu
 double deltalog; //przyrost czasu
 double LastReactionTime;
 bool HelpMeFlag; //wystawiane True jesli cos niedobrego sie dzieje
public:
 bool AIControllFlag; //rzeczywisty/wirtualny maszynista
 bool OnStationFlag; //Czy jest na peronie
private:
 TDynamicObject *pVehicle; //pojazd w którym siedzi steruj¹cy
 TDynamicObject *Vehicles[2]; //skrajne pojazdy w sk³adzie
 Mover::TMoverParameters *Controlling; //jakim pojazdem steruje
 Mtable::TTrainParameters *TrainSet; //do jakiego pociagu nalezy
 int TrainNumber; //numer rozkladowy tego pociagu
 AnsiString OrderCommand; //komenda pobierana z pojazdu
 double OrderValue; //argument komendy
 double AccPreferred; //preferowane przyspieszenie
public:
 double AccDesired; //chwilowe przyspieszenie
 double VelDesired; //predkosc
private:
 double VelforDriver; //predkosc dla manewrow
 double VelActual; //predkosc do której d¹zy
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
 Mover::TLocation CommandLocation; //polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda
 TOrders OrderList[maxorders]; //lista rozkazów
 int OrderPos,OrderTop; //rozkaz aktualny oraz wolne miejsce do wstawiania nowych
 TextFile LogFile; //zapis parametrow fizycznych
 TextFile AILogFile; //log AI
 bool ScanMe; //flaga potrzeby skanowania toru dla DynObj.cpp
 bool MaxVelFlag;
 bool MinVelFlag;
 int iDirection; //kierunek jazdy wzglêdem pojazdu, w którym siedzi AI (1=przód,-1=ty³)
 int iDirectionOrder; //¿adany kierunek jazdy (s³u¿y do zmiany kierunku)
 int iVehicleCount; //iloœæ pojazdów do od³¹czenia albo zabrania ze sk³adu (-1=wszystkie)
 bool Prepare2press; //
 int iDriverFailCount; //licznik b³êdów AI
 bool Need_TryAgain;
 bool Need_BrakeRelease;
public:
 double fMinProximityDist; //minimalna oleg³oœæ do przeszkody, jak¹ nale¿y zachowaæ
private:
 double fMaxProximityDist; //akceptowalna odleg³oœæ staniêcia przed przeszkod¹
 TStopReason eStopReason; //powód zatrzymania przy ustawieniu zerowej prêdkoœci
 void __fastcall SetDriverPsyche();
 bool __fastcall PrepareEngine();
 bool __fastcall ReleaseEngine();
 bool __fastcall IncBrake();
 bool __fastcall DecBrake();
 bool __fastcall IncSpeed();
 bool __fastcall DecSpeed();
 void __fastcall RecognizeCommand(); //odczytuje komende przekazana lokomotywie
public:
 void __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const Mover::TLocation &NewLocation,TStopReason reason=stopComm);
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
 bool __fastcall CheckSKP();
 void __fastcall ResetSKP();
private:
 void __fastcall CloseLog();
public:
 __fastcall TController
 (const Mover::TLocation &LocInitial,
  const Mover::TRotation &RotInitial,
  bool AI,
  TDynamicObject *NewControll,
  Mtable::TTrainParameters *NewTrainSet,
  bool InitPsyche
 );
 AnsiString __fastcall OrderCurrent();
 void __fastcall WaitingSet(double Seconds);
private:
 AnsiString VehicleName;
 double VelMargin;
 double fWarningDuration; //ile czasu jeszcze tr¹biæ
 double WaitingTime; //zliczany czas oczekiwania do samoistnego ruszenia
 double WaitingExpireTime; //maksymlany czas oczekiwania do samoistnego ruszenia
 AnsiString __fastcall Order2Str(TOrders Order);
 int __fastcall OrderDirectionChange(int newdir,Mover::TMoverParameters *Vehicle);
public:
 //inline __fastcall TController() { };
 AnsiString __fastcall StopReasonText();
 __fastcall ~TController();
};

#endif
