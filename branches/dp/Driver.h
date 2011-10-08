//---------------------------------------------------------------------------

#ifndef DriverH
#define DriverH

// Ra: poprawiony r�cznie plik nag��wkowy

#include "Classes.h"
#include <mover.hpp>	// Pascal unit

//namespace Ai_driver {
//-- type declarations -------------------------------------------------------
enum TOrders
{//rozkazy dla AI
 Wait_for_orders,    //czekanie na dostarczenie nast�pnych rozkaz�w
 Prepare_engine,     //w��czenie silnika
 Shunt,              //tryb manewrowy
 Change_direction,   //zmiana kierunku
 Obey_train,         //tryb poci�gowy
 Release_engine,     //wy��czenie silnika
 Jump_to_first_order //zap�lenie do pierwszej pozycji
};

enum TStopReason
{//pow�d zatrzymania
 stopNone, //nie ma powodu - powinien jecha�
 stopSleep, //nie zosta� odpalony, to nie pojedzie
 stopSem, //semafor zamkni�ty
 stopTime, //czekanie na godzin� odjazdu
 stopEnd, //brak dalszej cz�ci toru
 stopDir, //trzeba stan��, by zmieni� kierunek jazdy
 stopBlock, //przeszkoda na drodze ruchu
 stopComm, //otrzymano tak� komend� (niewiadomego pochodzenia)
 stopOut, //komenda wyjazdu poza stacj� (raczej nie powinna zatrzymywa�!)
 stopRadio, //komunikat przekazany radiem (Radiostop)
 stopError //z powodu b��du w obliczeniu drogi hamowania
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
static const int rvTrack = 0x1;
static const int rvSignal = 0x2;
static const int rvVechicle = 0x3;
static const int rvShunt = 0x4;
static const int rvPassStop = 0x5;

class TController
{
public:
 bool EngineActive; //ABu: Czy silnik byl juz zalaczony
 Mover::TLocation MechLoc;
 Mover::TRotation MechRot;
 bool Psyche;
 double ReactionTime; //czas reakcji Ra: czego?
 bool Ready; //ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow jest ustawiane w dynobj->cpp
 double LastUpdatedTime; //czas od ostatniego logu
 double ElapsedTime; //czas od poczatku logu
 double deltalog; //przyrost czasu
 double LastReactionTime;
 bool HelpMeFlag; //wystawiane True jesli cos niedobrego sie dzieje
 //PlayerIP: IPAddress;  //adres gracza
 bool AIControllFlag; //rzeczywisty/wirtualny maszynista
 bool OnStationFlag; //Czy jest na peronie
 TDynamicObject *Vehicle; //pojazd w kt�rym siedzi steruj�cy
 TDynamicObject *Vehicles[2]; //skrajne pojazdy w sk�adzie
 Mover::TMoverParameters *Controlling; //jakim pojazdem steruje
 Mtable::TTrainParameters *TrainSet; //do jakiego pociagu nalezy
 int TrainNumber; //numer rozkladowy tego pociagu
 AnsiString OrderCommand; //komenda pobierana z pojazdu
 double OrderValue; //argument komendy
 double AccPreferred; //preferowane przyspieszenie
 double AccDesired; //chwilowe przyspieszenie
 double VelDesired; //predkosc
 double VelforDriver; //predkosc dla manewrow
 double VelActual; //predkosc do kt�rej d�zy
 double VelNext; //predkosc przy nastepnym obiekcie
 double ProximityDist; //odleglosc od obiektu, ujemna jesli nieznana
 double ActualProximityDist; //ustawia nowa predkosc do ktorej ma dazyc oraz predkosc przy nastepnym obiekcie
 //TProximityTablePos ProximityTable[256];
 //double ReducedTable[256];
 //Byte ProximityTableIndex;
 //Byte LPTA;
 //Byte LPTI;
 Mover::TLocation CommandLocation; //polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda
 TOrders OrderList[maxorders]; //lista rozkaz�w
 Byte OrderPos; //aktualny rozkaz
 TextFile LogFile; //zapis parametrow fizycznych
 TextFile AILogFile; //log AI
 bool ScanMe; //flaga potrzeby skanowania toru dla DynObj.cpp
 bool MaxVelFlag;
 bool MinVelFlag;
 int ChangeDir;
 int ChangeDirOrder;
 int VehicleCount;
 bool Prepare2press;
 int DriverFailCount;
 bool Need_TryAgain;
 bool Need_BrakeRelease;
 double MinProximityDist;
 double MaxProximityDist;
 bool bCheckSKP;
 TStopReason eStopReason;
 void __fastcall SetDriverPsyche();
 bool __fastcall PrepareEngine();
 bool __fastcall ReleaseEngine();
 bool __fastcall IncBrake();
 bool __fastcall DecBrake();
 bool __fastcall IncSpeed();
 bool __fastcall DecSpeed();
 void __fastcall RecognizeCommand(); //odczytuje komende przekazana lokomotywie
 void __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const Mover::TLocation &NewLocation,TStopReason reason=stopComm);
 bool __fastcall UpdateSituation(double dt); //uruchamiac przynajmniej raz na sekunde
 //procedury dotyczace rozkazow dla maszynisty
 void __fastcall SetVelocity(double NewVel,double NewVelNext,TStopReason r=stopNone); //uaktualnia informacje o predkosci
 bool __fastcall SetProximityVelocity(double NewDist,double NewVelNext); //uaktualnia informacje o predkosci przy nastepnym semaforze
 bool __fastcall AddReducedVelocity(double Distance, double Velocity, Byte Flag);
 void __fastcall JumpToNextOrder();
 void __fastcall JumpToFirstOrder();
 void __fastcall ChangeOrder(TOrders NewOrder);
 TOrders __fastcall GetCurrentOrder();
 bool __fastcall CheckSKP();
 void __fastcall ResetSKP();
 void __fastcall CloseLog();
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
 double WarningDuration;
 double WaitingTime; //zliczany czas oczekiwania do samoistnego ruszenia
 double WaitingExpireTime; //maksymlany czas oczekiwania do samoistnego ruszenia
 int __fastcall OrderDirectionChange(int newdir,Mover::TMoverParameters *Vehicle);
public:
 //inline __fastcall TController() { };
 AnsiString __fastcall StopReasonText();
 inline __fastcall virtual ~TController() {};
};



//}	/* namespace Ai_driver */
//#if !defined(NO_IMPLICIT_NAMESPACE_USE)
//using namespace Ai_driver;
//#endif

#endif
