//---------------------------------------------------------------------------

#ifndef DriverH
#define DriverH

#include "Classes.h"
#include "dumb3d.h"
using namespace Math3D;

enum TOrders
{//rozkazy dla AI
 Wait_for_orders=0,       //czekanie na dostarczenie nastêpnych rozkazów
 //operacje tymczasowe
 Prepare_engine=1,        //w³¹czenie silnika
 Release_engine=2,        //wy³¹czenie silnika
 Change_direction=4,      //zmiana kierunku (bez skanowania sygnalizacji)
 Connect=8,               //pod³¹czanie wagonów (bez skanowania sygnalizacji)
 Disconnect=0x10,         //od³¹czanie wagonów (bez skanowania sygnalizacji)
 //jazda
 Shunt=0x20,              //tryb manewrowy
 Obey_train=0x40,         //tryb poci¹gowy
 Jump_to_first_order=0x60 //zapêlenie do pierwszej pozycji (po co?)
};

enum TMovementStatus
{//flagi bitowe ruchu (iDrivigFlags)
 moveStopCloser=1, //podjechaæ blisko W4 (nie podje¿d¿aæ na pocz¹tku ani po zmianie czo³a)
 moveStopPoint=2, //stawaæ na W4 (wy³¹czone podczas zmiany czo³a)
 moveAvaken=4, //po w³¹czeniu silnika pojazd nie przemieœci³ siê
 movePress=8, //dociskanie przy od³¹czeniu (zamiast zmiennej Prepare2press)
 moveBackwardLook=16, //skanowanie torów w przeciwn¹ stronê w celu zmiany kierunku
 moveConnect=0x20, //jest blisko innego pojazdu i mo¿na próbowaæ pod³¹czyæ
 movePrimary=0x40, //ma priorytet w sk³adzie
 moveStopHere=0x80 //nie podje¿d¿aæ do semafora, jeœli droga nie jest wolna
};

enum TStopReason
{//powód zatrzymania, dodawany do SetVelocity 0
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
 stopExt,   //komenda z zewn¹trz
 stopError  //z powodu b³êdu w obliczeniu drogi hamowania
};

class TSpeedPos
{//pozycja tabeli prêdkoœci dla AI
public:
 double fDist; //aktualna odleg³oœæ (ujemna gdy miniête)
 double fVelNext; //prêdkoœæ obowi¹zuj¹ca od tego miejsca
 //double fAcc;
 int iFlags;
 //1=istotny,2=tor,4=odwrotnie,8-zwrotnica (mo¿e siê zmieniæ),16-stan zwrotnicy,32-miniêty,64=koniec,128=³uk
 //0x100=event,0x200=manewrowa,0x400=przystanek,0x800=SBL
 //0x10000=zatkanie
 vector3 vPos; //wspó³rzêdne XYZ do liczenia odleg³oœci
 struct
 {
  TTrack *tTrack; //wskaŸnik na tor o zmiennej prêdkoœci (zwrotnica, obrotnica)
  TEvent *eEvent; //po³¹czenie z eventem albo komórk¹ pamiêci
 };
 void __fastcall CommandCheck();
public:
 void __fastcall Clear();
 bool __fastcall Update(vector3 *p,vector3 *dir,double &len);
 bool __fastcall Set(TEvent *e,double d);
 void __fastcall Set(TTrack *t,double d,int f);
};

class TSpeedTable
{//tabela prêdkoœci dla AI wraz z obs³ug¹
};

//----------------------------------------------------------------------------
static const bool Aggressive=true;
static const bool Easyman=false;
static const bool AIdriver=true;
static const bool Humandriver=false;
static const int maxorders=32; //iloœæ rozkazów w tabelce
static const int maxdriverfails=4; //ile b³êdów mo¿e zrobiæ AI zanim zmieni nastawienie
extern bool WriteLogFlag; //logowanie parametrów fizycznych
//----------------------------------------------------------------------------

class TController
{
private: //obs³uga tabelki prêdkoœci (musi mieæ mo¿liwoœæ odhaczania stacji w rozk³adzie)
 TSpeedPos *sSpeedTable; //najbli¿sze zmiany prêdkoœci
 int iSpeedTableSize; //wielkoœæ tabelki
 int iFirst; //aktualna pozycja w tabeli (modulo iSpeedTableSize)
 int iLast; //ostatnia wype³niona pozycja w tabeli <iFirst (modulo iSpeedTableSize)
 int iTableDirection; //kierunek zape³nienia tabelki wzglêdem pojazdu z AI
 double fLastVel; //prêdkoœæ na poprzednio sprawdzonym torze
 TTrack *tLast; //ostatni analizowany tor
 //float fLastDir; //kierunek na ostatnim torze
 TEvent *eSignSkip; //sygna³ do pominiêcia (przejechany)
 //TTrack tSignLast; //tor z ostatnio znalezionym eventem
 //TOrders oMode; //aktualny tryb pracy
 double fLength; //d³ugoœæ sk³adu (dla ograniczeñ)
 bool __fastcall TableCheckEvent(TEvent *e,bool prox);
 bool __fastcall TableAddNew();
 bool __fastcall TableNotFound(TEvent *e);
 void __fastcall TableClear();
 TEvent* __fastcall TableCheckTrackEvent(double fDirection,TTrack *Track);
 void __fastcall TableTraceRoute(double fDistance,int iDir,TDynamicObject *pVehicle=NULL);
 void __fastcall TableCheck(double fDistance,int iDir);
 TCommandType __fastcall TableUpdate(double &fVelDes,double &fDist,double &fNext,double &fAcc);
 void __fastcall TablePurger();
public:
 TEvent *eLastCommand; //ostatnio wys³ana komenda
 AnsiString asNextStop; //nazwa najbli¿szego przystanku
private: //parametry sterowania pojazdem (stan, hamowanie)
 double fShuntVelocity; //maksymalna prêdkoœæ manewrowania, zale¿y m.in. od sk³adu
 int iVehicles; //iloœæ pojazdów w sk³adzie
 bool EngineActive; //ABu: Czy silnik byl juz zalaczony
 //vector3 vMechLoc; //pozycja pojazdu do liczenia odleg³oœci od semafora (?)
 bool Psyche;
 int iDrivigFlags; //flagi bitowe ruchu
 double fDriverBraking; //po pomno¿eniu przez v^2 [km/h] daje ~drogê hamowania [m]
 double fDriverDist; //dopuszczalna odleg³oœæ podjechania do przeszkody
 double fVelMax; //maksymalna prêdkoœæ sk³adu (sprawdzany ka¿dy pojazd)
 double fBrakeDist; //przybli¿ona droga hamowania
public:
 double ReactionTime; //czas reakcji Ra: czego? œwiadomoœci AI
 double fBrakeTime;  //wpisana wartoœæ jest zmniejszana do 0, gdy ujemna nale¿y zmieniæ nastawê hamulca
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
 TDynamicObject *pVehicle; //pojazd w którym siedzi steruj¹cy
 TDynamicObject *pVehicles[2]; //skrajne pojazdy w sk³adzie (niekoniecznie bezpoœrednio sterowane)
 TMoverParameters *Controlling; //jakim pojazdem steruje
 Mtable::TTrainParameters *TrainParams; //rozk³ad jazdy; do jakiego pociagu nalezy
 //int TrainNumber; //numer rozkladowy tego pociagu
 //AnsiString OrderCommand; //komenda pobierana z pojazdu
 //double OrderValue; //argument komendy
public:
 double AccPreferred; //preferowane przyspieszenie (wg psychiki kieruj¹cego, zmniejszana przy wykryciu kolizji)
 double AccDesired; //przyspieszenie, jakie ma utrzymywaæ (<0:nie przyspieszaj,<-0.1:hamuj)
 double VelDesired; //predkoœæ, z jak¹ ma jechaæ, <=VelActual
private:
 double VelforDriver; //prêdkoœæ, u¿ywana przy zmianie kierunku (ograniczenie przy nieznajmoœci szlaku?)
 double VelActual; //predkoœæ zadawana przez funkcjê SetVelocity() (semaforem, ograniczeniem albo komend¹)
public:
 double VelNext; //prêdkoœæ, jaka ma byæ po przejechaniu d³ugoœci ProximityDist
private:
 double fProximityDist; //odleglosc podawana w SetProximityVelocity(); >0:przeliczaæ do punktu, <0:podana wartoœæ
public:
 double ActualProximityDist; //odleg³oœæ brana pod uwagê przy wyliczaniu prêdkoœci i przyspieszenia
private:
 vector3 vCommandLocation; //polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda
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
 int iCoupler; //sprzêg, który nale¿y u¿yæ przy ³¹czeniu
 bool Prepare2press; //dociskanie w celu od³¹czenia
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
 void __fastcall Activation(); //umieszczenie obsady w odpowiednim cz³onie
public:
 void __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const _mover::TLocation &NewLocation,TStopReason reason=stopComm);
 bool __fastcall PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const vector3 *NewLocation,TStopReason reason=stopComm);
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
  Mtable::TTrainParameters *NewTrainParams,
  bool InitPsyche
 );
 AnsiString __fastcall OrderCurrent();
 void __fastcall WaitingSet(double Seconds);
private:
 AnsiString VehicleName;
 double VelMargin;
 double fWarningDuration; //ile czasu jeszcze tr¹biæ
 double fStopTime; //czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
 double WaitingTime; //zliczany czas oczekiwania do samoistnego ruszenia
 double WaitingExpireTime; //maksymlany czas oczekiwania do samoistnego ruszenia
 AnsiString __fastcall Order2Str(TOrders Order);
 int __fastcall OrderDirectionChange(int newdir,TMoverParameters *Vehicle);
 void __fastcall Lights(int head,int rear);
 double __fastcall Distance(vector3 &p1,vector3 &n,vector3 &p2);
 //Ra: poni¿sze przenieœæ do modu³u AI:
 //TEvent* eSignSkip; //miniêty sygna³ zezwalaj¹cy na jazdê, pomijany przy szukaniu
 //AnsiString asNextStop; //nazwa nastêpnego punktu zatrzymania wg rozk³adu
 TEvent* eSignLast; //ostatnio znaleziony sygna³, o ile nie miniêty
 TEvent* __fastcall CheckTrackEvent(double fDirection,TTrack *Track);
 TTrack* __fastcall TraceRoute(double &fDistance,double &fDirection,TTrack *Track,TEvent*&Event);
 void __fastcall SetProximityVelocity(double dist,double vel,const vector3 *pos);
 void __fastcall DirectionForward(bool forward);
public:
 //inline __fastcall TController() { };
 AnsiString __fastcall StopReasonText();
 __fastcall ~TController();
 void __fastcall ScanEventTrack();
 bool __fastcall CheckEvent(TEvent *e,bool prox);
 AnsiString __fastcall NextStop();
 void __fastcall TakeControl(bool yes);
 AnsiString __fastcall Relation();
 AnsiString __fastcall TrainName();
 double __fastcall StationCount();
 double __fastcall StationIndex();
};

#endif
