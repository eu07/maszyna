/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef DriverH
#define DriverH

#include "Classes.h"
#include "dumb3d.h"
#include <fstream>
using namespace Math3D;

enum TOrders
{ // rozkazy dla AI
    Wait_for_orders = 0, // czekanie na dostarczenie nastêpnych rozkazów
    // operacje tymczasowe
    Prepare_engine = 1, // w³¹czenie silnika
    Release_engine = 2, // wy³¹czenie silnika
    Change_direction = 4, // zmiana kierunku (bez skanowania sygnalizacji)
    Connect = 8, // pod³¹czanie wagonów (z czêœciowym skanowaniem sygnalizacji)
    Disconnect = 0x10, // od³¹czanie wagonów (bez skanowania sygnalizacji)
    // jazda
    Shunt = 0x20, // tryb manewrowy
    Obey_train = 0x40, // tryb poci¹gowy
    Jump_to_first_order = 0x60 // zapêlenie do pierwszej pozycji (po co?)
};

enum TMovementStatus
{ // flagi bitowe ruchu (iDrivigFlags)
    moveStopCloser = 1, // podjechaæ blisko W4 (nie podje¿d¿aæ na pocz¹tku ani po zmianie czo³a)
    moveStopPoint = 2, // stawaæ na W4 (wy³¹czone podczas zmiany czo³a)
    moveActive = 4, // pojazd jest za³¹czony i skanuje
    movePress = 8, // dociskanie przy od³¹czeniu (zamiast zmiennej Prepare2press)
    moveConnect = 0x10, // jest blisko innego pojazdu i mo¿na próbowaæ pod³¹czyæ
    movePrimary = 0x20, // ma priorytet w sk³adzie (master)
    moveLate = 0x40, // flaga spóŸnienia, w³¹czy bardziej
    moveStopHere = 0x80, // nie podje¿d¿aæ do semafora, jeœli droga nie jest wolna
    moveStartHorn = 0x100, // podawaj sygna³ po podaniu wolnej drogi
    moveStartHornNow = 0x200, // podaj sygna³ po odhamowaniu
    moveStartHornDone = 0x400, // podano sygna³ po podaniu wolnej drogi
    moveOerlikons = 0x800, // sk³ad wy³¹cznie z zaworami? Oerlikona
    moveIncSpeed = 0x1000, // za³¹czenie jazdy (np. dla EZT)
    moveTrackEnd = 0x2000, // dalsza jazda do przodu trwale ograniczona (W5, koniec toru)
    moveSwitchFound = 0x4000, // na drodze skanowania do przodu jest rozjazd
    moveGuardSignal = 0x8000, // sygna³ od kierownika (min¹³ czas postoju)
    moveVisibility = 0x10000, // jazda na widocznoœæ po przejechaniu S1 na SBL
    moveDoorOpened = 0x20000, // drzwi zosta³y otwarte - doliczyæ czas na zamkniêcie
    movePushPull = 0x40000, // zmiana czo³a przez zmianê kabiny - nie odczepiaæ przy zmianie kierunku
    moveSemaphorFound = 0x80000, // na drodze skanowania zosta³ znaleziony semafor
    moveSemaphorWasElapsed = 0x100000, // miniêty zosta³ semafor
    moveTrainInsideStation = 0x200000, // poci¹g miêdzy semaforem a rozjazdami lub nastêpnym semaforem
    moveSpeedLimitFound = 0x400000 // poci¹g w ograniczeniu z podan¹ jego d³ugoœci¹
};

enum TStopReason
{ // powód zatrzymania, dodawany do SetVelocity 0 - w zasadzie do usuniêcia
    stopNone, // nie ma powodu - powinien jechaæ
    stopSleep, // nie zosta³ odpalony, to nie pojedzie
    stopSem, // semafor zamkniêty
    stopTime, // czekanie na godzinê odjazdu
    stopEnd, // brak dalszej czêœci toru
    stopDir, // trzeba stan¹æ, by zmieniæ kierunek jazdy
    stopJoin, // stoi w celu po³¹czenia wagonów
    stopBlock, // przeszkoda na drodze ruchu
    stopComm, // otrzymano tak¹ komendê (niewiadomego pochodzenia)
    stopOut, // komenda wyjazdu poza stacjê (raczej nie powinna zatrzymywaæ!)
    stopRadio, // komunikat przekazany radiem (Radiostop)
    stopExt, // komenda z zewn¹trz
    stopError // z powodu b³êdu w obliczeniu drogi hamowania
};

enum TAction
{ // przechowanie aktualnego stanu AI od poprzedniego przeb³ysku œwiadomoœci
    actUnknown, // stan nieznany (domyœlny na pocz¹tku)
    actPantUp, // podnieœ pantograf (info dla u¿ytkownika)
    actConv, // za³¹cz przetwornicê (info dla u¿ytkownika)
    actCompr, // za³¹cz sprê¿arkê (info dla u¿ytkownika)
    actSleep, //œpi (wygaszony)
    actDrive, // jazda
    actGo, // ruszanie z miejsca
    actSlow, // przyhamowanie przed ograniczeniem
    sctStop, // hamowanie w celu precyzyjnego zatrzymania
    actIdle, // luzowanie sk³adu przed odjazdem
    actRelease, // luzowanie sk³adu po zmniejszeniu prêdkoœci
    actConnect, // dojazd w celu podczepienia
    actWait, // czekanie na przystanku
    actReady, // zg³oszona gotowoœæ do odjazdu od kierownika
    actEmergency, // hamowanie awaryjne
    actGoUphill, // ruszanie pod górê
    actTest, // hamowanie kontrolne (podczas jazdy)
    actTrial // próba hamulca (na postoju)
};

enum TSpeedPosFlag
{ // wartoœci dla iFlag w TSpeedPos
    spEnabled = 0x1, // pozycja brana pod uwagê
    spTrack = 0x2, // to jest tor
    spReverse = 0x4, // odwrotnie
    spSwitch = 0x8, // to zwrotnica
    spSwitchStatus = 0x10, // stan zwrotnicy
    spElapsed = 0x20, // pozycja miniêta przez pojazd
    spEnd = 0x40, // koniec
    spCurve = 0x80, // ³uk
    spEvent = 0x100, // event
    spShuntSemaphor = 0x200, // tarcza manewrowa
    spPassengerStopPoint = 0x400, // przystanek osobowy (wskaŸnik W4)
    spStopOnSBL = 0x800, // zatrzymanie na SBL
    spCommandSent = 0x1000, // komenda wys³ana
    spOutsideStation = 0x2000, // wskaŸnik koñca manewrów
    spSemaphor = 0x4000, // semafor poci¹gowy
    spRoadVel = 0x8000, // zadanie prêdkoœci drogowej
    spSectionVel = 0x20000, // odcinek z ograniczeniem
    spProximityVelocity = 0x40000, // odcinek z ograniczeniem i podan¹ jego d³ugoœcia
    spEndOfTable = 0x10000 // zatkanie tabelki
};

class TSpeedPos
{ // pozycja tabeli prêdkoœci dla AI
  public:
    double fDist; // aktualna odleg³oœæ (ujemna gdy miniête)
    double fVelNext; // prêdkoœæ obowi¹zuj¹ca od tego miejsca
    double fSectionVelocityDist; //d³ugoœæ ograniczenia prêdkoœci
    // double fAcc;
    int iFlags; //flagi typu wpisu do tabelki
    // 1=istotny,2=tor,4=odwrotnie,8-zwrotnica (mo¿e siê zmieniæ),16-stan
    // zwrotnicy,32-miniêty,64=koniec,128=³uk
    // 0x100=event,0x200=manewrowa,0x400=przystanek,0x800=SBL,0x1000=wys³ana komenda,0x2000=W5
    // 0x4000=semafor,0x10000=zatkanie
    vector3 vPos; // wspó³rzêdne XYZ do liczenia odleg³oœci
    struct
    {
        TTrack *trTrack; // wskaŸnik na tor o zmiennej prêdkoœci (zwrotnica, obrotnica)
        TEvent *evEvent; // po³¹czenie z eventem albo komórk¹ pamiêci
    };
    void CommandCheck();

  public:
    void Clear();
    bool Update(vector3 *p, vector3 *dir, double &len);
    bool Set(TEvent *e, double d, TOrders order = Wait_for_orders);
    void Set(TTrack *t, double d, int f);
    AnsiString TableText();
	AnsiString GetName();
	bool IsProperSemaphor(TOrders order = Wait_for_orders);
};

//----------------------------------------------------------------------------
static const bool Aggressive = true;
static const bool Easyman = false;
static const bool AIdriver = true;
static const bool Humandriver = false;
static const int maxorders = 32; // iloœæ rozkazów w tabelce
static const int maxdriverfails = 4; // ile b³êdów mo¿e zrobiæ AI zanim zmieni nastawienie
extern bool WriteLogFlag; // logowanie parametrów fizycznych
//----------------------------------------------------------------------------

class TController
{
  private: // obs³uga tabelki prêdkoœci (musi mieæ mo¿liwoœæ odhaczania stacji w rozk³adzie)
    TSpeedPos *sSpeedTable; // najbli¿sze zmiany prêdkoœci
    int iSpeedTableSize; // wielkoœæ tabelki
    int iFirst; // aktualna pozycja w tabeli (modulo iSpeedTableSize)
    int iLast; // ostatnia wype³niona pozycja w tabeli <iFirst (modulo iSpeedTableSize)
    int iTableDirection; // kierunek zape³nienia tabelki wzglêdem pojazdu z AI
    double fLastVel; // prêdkoœæ na poprzednio sprawdzonym torze
    TTrack *tLast; // ostatni analizowany tor
    TEvent *eSignSkip; // mo¿na pomin¹æ ten SBL po zatrzymaniu
	TSpeedPos *sSemNext; // nastêpny semafor na drodze zale¿ny od trybu jazdy
	TSpeedPos *sSemNextStop; // nastêpny semafor na drodze zale¿ny od trybu jazdy i na stój
  private: // parametry aktualnego sk³adu
    double fLength; // d³ugoœæ sk³adu (do wyci¹gania z ograniczeñ)
    double fMass; // ca³kowita masa do liczenia stycznej sk³adowej grawitacji
    double fAccGravity; // przyspieszenie sk³adowej stycznej grawitacji
  public:
    TEvent *eSignNext; // sygna³ zmieniaj¹cy prêdkoœæ, do pokazania na [F2]
    AnsiString asNextStop; // nazwa nastêpnego punktu zatrzymania wg rozk³adu
    int iStationStart; // numer pierwszej stacji pokazywanej na podgl¹dzie rozk³adu
  private: // parametry sterowania pojazdem (stan, hamowanie)
    double fShuntVelocity; // maksymalna prêdkoœæ manewrowania, zale¿y m.in. od sk³adu
    int iVehicles; // iloœæ pojazdów w sk³adzie
    int iEngineActive; // ABu: Czy silnik byl juz zalaczony; Ra: postêp w za³¹czaniu
    // vector3 vMechLoc; //pozycja pojazdu do liczenia odleg³oœci od semafora (?)
    bool Psyche;
    int iDrivigFlags; // flagi bitowe ruchu
    double fDriverBraking; // po pomno¿eniu przez v^2 [km/h] daje ~drogê hamowania [m]
    double fDriverDist; // dopuszczalna odleg³oœæ podjechania do przeszkody
    double fVelMax; // maksymalna prêdkoœæ sk³adu (sprawdzany ka¿dy pojazd)
    double fBrakeDist; // przybli¿ona droga hamowania
    double fAccThreshold; // próg opóŸnienia dla zadzia³ania hamulca
  public:
    double fLastStopExpDist; // odleg³oœæ wygasania ostateniego przystanku
    double ReactionTime; // czas reakcji Ra: czego i na co? œwiadomoœci AI
    double fBrakeTime; // wpisana wartoœæ jest zmniejszana do 0, gdy ujemna nale¿y zmieniæ nastawê
    // hamulca
  private:
    double fReady; // poziom odhamowania wagonów
    bool Ready; // ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow
    double LastUpdatedTime; // czas od ostatniego logu
    double ElapsedTime; // czas od poczatku logu
    double deltalog; // przyrost czasu
    double LastReactionTime;
    double fActionTime; // czas u¿ywany przy regulacji prêdkoœci i zamykaniu drzwi
    TAction eAction; // aktualny stan
    bool HelpMeFlag; // wystawiane True jesli cos niedobrego sie dzieje
  public:
    inline TAction GetAction()
    {
        return eAction;
    }
    bool AIControllFlag; // rzeczywisty/wirtualny maszynista
    int iRouteWanted; // oczekiwany kierunek jazdy (0-stop,1-lewo,2-prawo,3-prosto) np. odpala
    // migacz lub czeka na stan zwrotnicy
  private:
    TDynamicObject *pVehicle; // pojazd w którym siedzi steruj¹cy
    TDynamicObject *
        pVehicles[2]; // skrajne pojazdy w sk³adzie (niekoniecznie bezpoœrednio sterowane)
    TMoverParameters *mvControlling; // jakim pojazdem steruje (mo¿e silnikowym w EZT)
    TMoverParameters *mvOccupied; // jakim pojazdem hamuje
    Mtable::TTrainParameters *TrainParams; // rozk³ad jazdy zawsze jest, nawet jeœli pusty
    // int TrainNumber; //numer rozkladowy tego pociagu
    // AnsiString OrderCommand; //komenda pobierana z pojazdu
    // double OrderValue; //argument komendy
    int iRadioChannel; // numer aktualnego kana³u radiowego
    TTextSound *tsGuardSignal; // komunikat od kierownika
    int iGuardRadio; // numer kana³u radiowego kierownika (0, gdy nie u¿ywa radia)
  public:
    double AccPreferred; // preferowane przyspieszenie (wg psychiki kieruj¹cego, zmniejszana przy
    // wykryciu kolizji)
    double AccDesired; // przyspieszenie, jakie ma utrzymywaæ (<0:nie przyspieszaj,<-0.1:hamuj)
    double VelDesired; // predkoœæ, z jak¹ ma jechaæ, wynikaj¹ca z analizy tableki; <=VelSignal
    double fAccDesiredAv; // uœrednione przyspieszenie z kolejnych przeb³ysków œwiadomoœci, ¿eby
    // ograniczyæ migotanie
  public:
    double VelforDriver; // prêdkoœæ, u¿ywana przy zmianie kierunku (ograniczenie przy nieznajmoœci
    // szlaku?)
    double VelSignal; // ograniczenie prêdkoœci z kompilacji znaków i sygna³ów
    double VelLimit; // predkoœæ zadawana przez event jednokierunkowego ograniczenia prêdkoœci
  public:
    double VelSignalLast; // prêdkoœæ zadana na ostatnim semaforze
    double VelLimitLast; // prêdkoœæ zadana przez ograniczenie
    double VelRoad; // aktualna prêdkoœæ drogowa (ze znaku W27)
    // (PutValues albo komend¹)
  public:
    double VelNext; // prêdkoœæ, jaka ma byæ po przejechaniu d³ugoœci ProximityDist
  private:
     double fProximityDist; //odleglosc podawana w SetProximityVelocity(); >0:przeliczaæ do
    // punktu, <0:podana wartoœæ
	 double FirstSemaphorDist; // odleg³oœæ do pierwszego znalezionego semafora
  public:
    double
        ActualProximityDist; // odleg³oœæ brana pod uwagê przy wyliczaniu prêdkoœci i przyspieszenia
  private:
    vector3 vCommandLocation; // polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego
    // odnosi sie komenda
    TOrders OrderList[maxorders]; // lista rozkazów
    int OrderPos, OrderTop; // rozkaz aktualny oraz wolne miejsce do wstawiania nowych
    std::ofstream LogFile; // zapis parametrow fizycznych
    std::ofstream AILogFile; // log AI
    bool MaxVelFlag;
    bool MinVelFlag;
    int iDirection; // kierunek jazdy wzglêdem sprzêgów pojazdu, w którym siedzi AI (1=przód,-1=ty³)
    int iDirectionOrder; //¿adany kierunek jazdy (s³u¿y do zmiany kierunku)
    int iVehicleCount; // iloœæ pojazdów do od³¹czenia albo zabrania ze sk³adu (-1=wszystkie)
    int iCoupler; // maska sprzêgu, jak¹ nale¿y u¿yæ przy ³¹czeniu (po osi¹gniêciu trybu Connect), 0
    // gdy jazda bez ³¹czenia
    int iDriverFailCount; // licznik b³êdów AI
    bool Need_TryAgain; // true, jeœli druga pozycja w elektryku nie za³apa³a
    bool Need_BrakeRelease;

  public:
    double fMinProximityDist; // minimalna oleg³oœæ do przeszkody, jak¹ nale¿y zachowaæ
    double fOverhead1; // informacja o napiêciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
    double fOverhead2; // informacja o sposobie jazdy (-1=normalnie, 0=bez pr¹du, >0=z opuszczonym i
    // ograniczeniem prêdkoœci)
    int iOverheadZero; // suma bitowa jezdy bezpr¹dowej, bity ustawiane przez pojazdy z
    // podniesionymi pantografami
    int iOverheadDown; // suma bitowa opuszczenia pantografów, bity ustawiane przez pojazdy z
    // podniesionymi pantografami
    double fVoltage; // uœrednione napiêcie sieci: przy spadku poni¿ej wartoœci minimalnej opóŸniæ
    // rozruch o losowy czas
  private:
    double fMaxProximityDist; // akceptowalna odleg³oœæ staniêcia przed przeszkod¹
    TStopReason eStopReason; // powód zatrzymania przy ustawieniu zerowej prêdkoœci
    AnsiString VehicleName;
    double fVelPlus; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
    double fVelMinus; // margines obni¿enia prêdkoœci, powoduj¹cy za³¹czenie napêdu
    double fWarningDuration; // ile czasu jeszcze tr¹biæ
    double fStopTime; // czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
    double WaitingTime; // zliczany czas oczekiwania do samoistnego ruszenia
    double WaitingExpireTime; // maksymlany czas oczekiwania do samoistnego ruszenia
    // TEvent* eSignLast; //ostatnio znaleziony sygna³, o ile nie miniêty
  private: //---//---//---//---// koniec zmiennych, poni¿ej metody //---//---//---//---//
    void SetDriverPsyche();
    bool PrepareEngine();
    bool ReleaseEngine();
    bool IncBrake();
    bool DecBrake();
    bool IncSpeed();
    bool DecSpeed(bool force = false);
    void SpeedSet();
    void Doors(bool what);
    void RecognizeCommand(); // odczytuje komende przekazana lokomotywie
    void Activation(); // umieszczenie obsady w odpowiednim cz³onie
    void ControllingSet(); // znajduje cz³on do sterowania
    void AutoRewident(); // ustawia hamulce w sk³adzie
  public:
    Mtable::TTrainParameters *__fastcall Timetable()
    {
        return TrainParams;
    };
    void PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                    const _mover::TLocation &NewLocation, TStopReason reason = stopComm);
    bool PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                    const vector3 *NewLocation, TStopReason reason = stopComm);
    bool UpdateSituation(double dt); // uruchamiac przynajmniej raz na sekundê
    // procedury dotyczace rozkazow dla maszynisty
    void SetVelocity(double NewVel, double NewVelNext,
                     TStopReason r = stopNone); // uaktualnia informacje o prêdkoœci
    bool SetProximityVelocity(
        double NewDist,
        double NewVelNext); // uaktualnia informacje o prêdkoœci przy nastepnym semaforze
  public:
    void JumpToNextOrder();
    void JumpToFirstOrder();
    void OrderPush(TOrders NewOrder);
    void OrderNext(TOrders NewOrder);
    inline TOrders OrderCurrentGet();
    inline TOrders OrderNextGet();
    bool CheckVehicles(TOrders user = Wait_for_orders);

  private:
    void CloseLog();
    void OrderCheck();

  public:
    void OrdersInit(double fVel);
    void OrdersClear();
    void OrdersDump();
    TController(bool AI, TDynamicObject *NewControll, bool InitPsyche,
                bool primary = true // czy ma aktywnie prowadziæ?
                );
    AnsiString OrderCurrent();
    void WaitingSet(double Seconds);

  private:
    AnsiString Order2Str(TOrders Order);
    void DirectionForward(bool forward);
    int OrderDirectionChange(int newdir, TMoverParameters *Vehicle);
    void Lights(int head, int rear);
    double Distance(vector3 &p1, vector3 &n, vector3 &p2);

  private: // Ra: metody obs³uguj¹ce skanowanie toru
    TEvent *__fastcall CheckTrackEvent(double fDirection, TTrack *Track);
    bool TableCheckEvent(TEvent *e);
    bool TableAddNew();
    bool TableNotFound(TEvent *e);
    void TableClear();
    TEvent *__fastcall TableCheckTrackEvent(double fDirection, TTrack *Track);
    void TableTraceRoute(double fDistance, TDynamicObject *pVehicle = NULL);
    void TableCheck(double fDistance);
    TCommandType TableUpdate(double &fVelDes, double &fDist, double &fNext, double &fAcc);
    void TablePurger();

  private: // Ra: stare funkcje skanuj¹ce, u¿ywane do szukania sygnalizatora z ty³u
    bool BackwardTrackBusy(TTrack *Track);
    TEvent *__fastcall CheckTrackEventBackward(double fDirection, TTrack *Track);
    TTrack *__fastcall BackwardTraceRoute(double &fDistance, double &fDirection, TTrack *Track,
                                          TEvent *&Event);
    void SetProximityVelocity(double dist, double vel, const vector3 *pos);
    TCommandType BackwardScan();

  public:
    void PhysicsLog();
    AnsiString StopReasonText();
    ~TController();
    AnsiString NextStop();
    void TakeControl(bool yes);
    AnsiString Relation();
    AnsiString TrainName();
    int StationCount();
    int StationIndex();
    bool IsStop();
    bool Primary()
    {
        return this ? bool(iDrivigFlags & movePrimary) : false;
    };
    int inline DrivigFlags()
    {
        return iDrivigFlags;
    };
    void MoveTo(TDynamicObject *to);
    void DirectionInitial();
    AnsiString TableText(int i);
    int CrossRoute(TTrack *tr);
    void RouteSwitch(int d);
    AnsiString OwnerName();
};

#endif
