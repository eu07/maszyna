//---------------------------------------------------------------------------

#ifndef DriverH
#define DriverH

#include "Classes.h"
#include "dumb3d.h"
#include <fstream>
using namespace Math3D;

enum TOrders
{//rozkazy dla AI
 Wait_for_orders=0,       //czekanie na dostarczenie nast�pnych rozkaz�w
 //operacje tymczasowe
 Prepare_engine=1,        //w��czenie silnika
 Release_engine=2,        //wy��czenie silnika
 Change_direction=4,      //zmiana kierunku (bez skanowania sygnalizacji)
 Connect=8,               //pod��czanie wagon�w (z cz�ciowym skanowaniem sygnalizacji)
 Disconnect=0x10,         //od��czanie wagon�w (bez skanowania sygnalizacji)
 //jazda
 Shunt=0x20,              //tryb manewrowy
 Obey_train=0x40,         //tryb poci�gowy
 Jump_to_first_order=0x60 //zap�lenie do pierwszej pozycji (po co?)
};

enum TMovementStatus
{//flagi bitowe ruchu (iDrivigFlags)
 moveStopCloser=1, //podjecha� blisko W4 (nie podje�d�a� na pocz�tku ani po zmianie czo�a)
 moveStopPoint=2, //stawa� na W4 (wy��czone podczas zmiany czo�a)
 moveAvaken=4, //po w��czeniu silnika pojazd nie przemie�ci� si�
 movePress=8, //dociskanie przy od��czeniu (zamiast zmiennej Prepare2press)
 moveConnect=0x10, //jest blisko innego pojazdu i mo�na pr�bowa� pod��czy�
 movePrimary=0x20, //ma priorytet w sk�adzie (master)
 //moveStopThen=0x40, //nie podje�d�a� do semafora, je�li droga nie jest wolna
 moveStopHere=0x80, //nie podje�d�a� do semafora, je�li droga nie jest wolna
 moveStartHorn=0x100, //podaj sygna� po podaniu wolnej drogi
 moveStartHornDone=0x200 //podano sygna� po podaniu wolnej drogi
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

class TSpeedPos
{//pozycja tabeli pr�dko�ci dla AI
public:
 double fDist; //aktualna odleg�o�� (ujemna gdy mini�te)
 double fVelNext; //pr�dko�� obowi�zuj�ca od tego miejsca
 //double fAcc;
 int iFlags;
 //1=istotny,2=tor,4=odwrotnie,8-zwrotnica (mo�e si� zmieni�),16-stan zwrotnicy,32-mini�ty,64=koniec,128=�uk
 //0x100=event,0x200=manewrowa,0x400=przystanek,0x800=SBL,0x1000=wys�ana komenda
 //0x10000=zatkanie
 vector3 vPos; //wsp�rz�dne XYZ do liczenia odleg�o�ci
 struct
 {
  TTrack *tTrack; //wska�nik na tor o zmiennej pr�dko�ci (zwrotnica, obrotnica)
  TEvent *eEvent; //po��czenie z eventem albo kom�rk� pami�ci
 };
 void __fastcall CommandCheck();
public:
 void __fastcall Clear();
 bool __fastcall Update(vector3 *p,vector3 *dir,double &len);
 bool __fastcall Set(TEvent *e,double d);
 void __fastcall Set(TTrack *t,double d,int f);
};

//----------------------------------------------------------------------------
static const bool Aggressive=true;
static const bool Easyman=false;
static const bool AIdriver=true;
static const bool Humandriver=false;
static const int maxorders=32; //ilo�� rozkaz�w w tabelce
static const int maxdriverfails=4; //ile b��d�w mo�e zrobi� AI zanim zmieni nastawienie
extern bool WriteLogFlag; //logowanie parametr�w fizycznych
//----------------------------------------------------------------------------

class TController
{
private: //obs�uga tabelki pr�dko�ci (musi mie� mo�liwo�� odhaczania stacji w rozk�adzie)
 TSpeedPos *sSpeedTable; //najbli�sze zmiany pr�dko�ci
 int iSpeedTableSize; //wielko�� tabelki
 int iFirst; //aktualna pozycja w tabeli (modulo iSpeedTableSize)
 int iLast; //ostatnia wype�niona pozycja w tabeli <iFirst (modulo iSpeedTableSize)
 int iTableDirection; //kierunek zape�nienia tabelki wzgl�dem pojazdu z AI
 double fLastVel; //pr�dko�� na poprzednio sprawdzonym torze
 TTrack *tLast; //ostatni analizowany tor
 //TTrack tSignLast; //tor z ostatnio znalezionym eventem
 bool __fastcall TableCheckEvent(TEvent *e);
 bool __fastcall TableAddNew();
 bool __fastcall TableNotFound(TEvent *e);
 void __fastcall TableClear();
 TEvent* __fastcall TableCheckTrackEvent(double fDirection,TTrack *Track);
 void __fastcall TableTraceRoute(double fDistance,TDynamicObject *pVehicle=NULL);
 void __fastcall TableCheck(double fDistance);
 TCommandType __fastcall TableUpdate(double &fVelDes,double &fDist,double &fNext,double &fAcc);
 void __fastcall TablePurger();
private: //parametry aktualnego sk�adu
 double fLength; //d�ugo�� sk�adu (do wyci�gania z ogranicze�)
 double fMass; //ca�kowita masa do liczenia stycznej sk�adowej grawitacji
 double fAccGravity; //przyspieszenie sk�adowej stycznej grawitacji
public:
 TEvent *eSignNext; //sygna� zmieniaj�cy pr�dko��, do pokazania na [F2]
 AnsiString asNextStop; //nazwa nast�pnego punktu zatrzymania wg rozk�adu
private: //parametry sterowania pojazdem (stan, hamowanie)
 double fShuntVelocity; //maksymalna pr�dko�� manewrowania, zale�y m.in. od sk�adu
 int iVehicles; //ilo�� pojazd�w w sk�adzie
 bool EngineActive; //ABu: Czy silnik byl juz zalaczony
 //vector3 vMechLoc; //pozycja pojazdu do liczenia odleg�o�ci od semafora (?)
 bool Psyche;
 int iDrivigFlags; //flagi bitowe ruchu
 double fDriverBraking; //po pomno�eniu przez v^2 [km/h] daje ~drog� hamowania [m]
 double fDriverDist; //dopuszczalna odleg�o�� podjechania do przeszkody
 double fVelMax; //maksymalna pr�dko�� sk�adu (sprawdzany ka�dy pojazd)
 double fBrakeDist; //przybli�ona droga hamowania
public:
 double ReactionTime; //czas reakcji Ra: czego? �wiadomo�ci AI
 double fBrakeTime;  //wpisana warto�� jest zmniejszana do 0, gdy ujemna nale�y zmieni� nastaw� hamulca
private:
 double fReady; //poziom odhamowania wagon�w
 bool Ready; //ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow
 double LastUpdatedTime; //czas od ostatniego logu
 double ElapsedTime; //czas od poczatku logu
 double deltalog; //przyrost czasu
 double LastReactionTime;
 bool HelpMeFlag; //wystawiane True jesli cos niedobrego sie dzieje
public:
 bool AIControllFlag; //rzeczywisty/wirtualny maszynista
 //bool OnStationFlag; //Czy jest na peronie
private:
 TDynamicObject *pVehicle; //pojazd w kt�rym siedzi steruj�cy
 TDynamicObject *pVehicles[2]; //skrajne pojazdy w sk�adzie (niekoniecznie bezpo�rednio sterowane)
 TMoverParameters *Controlling; //jakim pojazdem steruje
 Mtable::TTrainParameters *TrainParams; //rozk�ad jazdy zawsze jest, nawet je�li pusty
 //int TrainNumber; //numer rozkladowy tego pociagu
 //AnsiString OrderCommand; //komenda pobierana z pojazdu
 //double OrderValue; //argument komendy
 int iRadioChannel; //numer aktualnego kana�u radiowego
public:
 Mtable::TTrainParameters* __fastcall Timetable() {return TrainParams;};
 double AccPreferred; //preferowane przyspieszenie (wg psychiki kieruj�cego, zmniejszana przy wykryciu kolizji)
 double AccDesired; //przyspieszenie, jakie ma utrzymywa� (<0:nie przyspieszaj,<-0.1:hamuj)
 double VelDesired; //predko��, z jak� ma jecha�, <=VelActual
private:
 double VelforDriver; //pr�dko��, u�ywana przy zmianie kierunku (ograniczenie przy nieznajmo�ci szlaku?)
 double VelActual; //predko�� zadawana przez funkcj� SetVelocity() (semaforem, ograniczeniem albo komend�)
public:
 double VelNext; //pr�dko��, jaka ma by� po przejechaniu d�ugo�ci ProximityDist
private:
 //double fProximityDist; //odleglosc podawana w SetProximityVelocity(); >0:przelicza� do punktu, <0:podana warto��
public:
 double ActualProximityDist; //odleg�o�� brana pod uwag� przy wyliczaniu pr�dko�ci i przyspieszenia
private:
 vector3 vCommandLocation; //polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda
 TOrders OrderList[maxorders]; //lista rozkaz�w
 int OrderPos,OrderTop; //rozkaz aktualny oraz wolne miejsce do wstawiania nowych
 std::ofstream LogFile; //zapis parametrow fizycznych
 std::ofstream AILogFile; //log AI
 //bool ScanMe; //flaga potrzeby skanowania toru dla DynObj.cpp
 bool MaxVelFlag;
 bool MinVelFlag;
 int iDirection; //kierunek jazdy wzgl�dem sprz�g�w pojazdu, w kt�rym siedzi AI (1=prz�d,-1=ty�)
 int iDirectionOrder; //�adany kierunek jazdy (s�u�y do zmiany kierunku)
 int iVehicleCount; //ilo�� pojazd�w do od��czenia albo zabrania ze sk�adu (-1=wszystkie)
 int iCoupler; //sprz�g, kt�ry nale�y u�y� przy ��czeniu
 bool Prepare2press; //dociskanie w celu od��czenia
 int iDriverFailCount; //licznik b��d�w AI
 bool Need_TryAgain; //true, je�li druga pozycja w elektryku nie za�apa�a
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
 void __fastcall SpeedSet();
 void __fastcall RecognizeCommand(); //odczytuje komende przekazana lokomotywie
 void __fastcall Activation(); //umieszczenie obsady w odpowiednim cz�onie
public:
 void __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const _mover::TLocation &NewLocation,TStopReason reason=stopComm);
 bool __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const vector3 *NewLocation,TStopReason reason=stopComm);
 bool __fastcall UpdateSituation(double dt); //uruchamiac przynajmniej raz na sekunde
 //procedury dotyczace rozkazow dla maszynisty
 void __fastcall SetVelocity(double NewVel,double NewVelNext,TStopReason r=stopNone); //uaktualnia informacje o predkosci
 bool __fastcall SetProximityVelocity(double NewDist,double NewVelNext); //uaktualnia informacje o predkosci przy nastepnym semaforze
private:
 bool __fastcall AddReducedVelocity(double Distance, double Velocity, Byte Flag);
 void __fastcall AutoRewident();
public:
 void __fastcall JumpToNextOrder();
 void __fastcall JumpToFirstOrder();
 void __fastcall OrderPush(TOrders NewOrder);
 void __fastcall OrderNext(TOrders NewOrder);
 TOrders __fastcall OrderCurrentGet();
 TOrders __fastcall OrderNextGet();
 bool __fastcall CheckVehicles();
private:
 void __fastcall CloseLog();
 void __fastcall OrderCheck();
public:
 void __fastcall OrdersInit(double fVel);
 void __fastcall OrdersClear();
 void __fastcall OrdersDump();
 __fastcall TController
 (bool AI,
  TDynamicObject *NewControll,
  bool InitPsyche,
  bool primary=true //czy ma aktywnie prowadzi�?
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
 void __fastcall DirectionForward(bool forward);
 int __fastcall OrderDirectionChange(int newdir,TMoverParameters *Vehicle);
 void __fastcall Lights(int head,int rear);
 double __fastcall Distance(vector3 &p1,vector3 &n,vector3 &p2);
private: //Ra: stare funkcje skanuj�ce, u�ywane do szukania sygnalizatora z ty�u
 TEvent* eSignLast; //ostatnio znaleziony sygna�, o ile nie mini�ty
 bool __fastcall CheckEvent(TEvent *e);
 TEvent* __fastcall CheckTrackEvent(double fDirection,TTrack *Track);
 TEvent* __fastcall CheckTrackEventBackward(double fDirection,TTrack *Track);
 TTrack* __fastcall BackwardTraceRoute(double &fDistance,double &fDirection,TTrack *Track,TEvent*&Event);
 void __fastcall SetProximityVelocity(double dist,double vel,const vector3 *pos);
 bool __fastcall BackwardScan();
public:
 AnsiString __fastcall StopReasonText();
 __fastcall ~TController();
 AnsiString __fastcall NextStop();
 void __fastcall TakeControl(bool yes);
 AnsiString __fastcall Relation();
 AnsiString __fastcall TrainName();
 double __fastcall StationCount();
 double __fastcall StationIndex();
};

#endif
