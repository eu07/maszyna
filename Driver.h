/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include "Classes.h"
#include "MOVER.h"
#include "sound.h"
#include "DynObj.h"

enum TOrders
{ // rozkazy dla AI
    Wait_for_orders = 0, // czekanie na dostarczenie następnych rozkazów
    // operacje tymczasowe
    Prepare_engine = 1, // włączenie silnika
    Release_engine = 2, // wyłączenie silnika
    Change_direction = 4, // zmiana kierunku (bez skanowania sygnalizacji)
    Connect = 8, // podłączanie wagonów (z częściowym skanowaniem sygnalizacji)
    Disconnect = 0x10, // odłączanie wagonów (bez skanowania sygnalizacji)
    // jazda
    Shunt = 0x20, // tryb manewrowy
    Obey_train = 0x40, // tryb pociągowy
    Jump_to_first_order = 0x60 // zapęlenie do pierwszej pozycji (po co?)
};

enum TMovementStatus
{ // flagi bitowe ruchu (iDrivigFlags)
    moveStopCloser = 1, // podjechać blisko W4 (nie podjeżdżać na początku ani po zmianie czoła)
    moveStopPoint = 2, // stawać na W4 (wyłączone podczas zmiany czoła)
    moveActive = 4, // pojazd jest załączony i skanuje
    movePress = 8, // dociskanie przy odłączeniu (zamiast zmiennej Prepare2press)
    moveConnect = 0x10, // jest blisko innego pojazdu i można próbować podłączyć
    movePrimary = 0x20, // ma priorytet w składzie (master)
    moveLate = 0x40, // flaga spóźnienia, włączy bardziej
    moveStopHere = 0x80, // nie podjeżdżać do semafora, jeśli droga nie jest wolna
    moveStartHorn = 0x100, // podawaj sygnał po podaniu wolnej drogi
    moveStartHornNow = 0x200, // podaj sygnał po odhamowaniu
    moveStartHornDone = 0x400, // podano sygnał po podaniu wolnej drogi
    moveOerlikons = 0x800, // skład wyłącznie z zaworami? Oerlikona
    moveIncSpeed = 0x1000, // załączenie jazdy (np. dla EZT)
    moveTrackEnd = 0x2000, // dalsza jazda do przodu trwale ograniczona (W5, koniec toru)
    moveSwitchFound = 0x4000, // na drodze skanowania do przodu jest rozjazd
    moveGuardSignal = 0x8000, // sygnał od kierownika (minął czas postoju)
    moveVisibility = 0x10000, // jazda na widoczność po przejechaniu S1 na SBL
    moveDoorOpened = 0x20000, // drzwi zostały otwarte - doliczyć czas na zamknięcie
    movePushPull = 0x40000, // zmiana czoła przez zmianę kabiny - nie odczepiać przy zmianie kierunku
    moveSemaphorFound = 0x80000, // na drodze skanowania został znaleziony semafor
    moveStopPointFound = 0x100000 // stop point detected ahead
/*
    moveSemaphorWasElapsed = 0x100000, // minięty został semafor
    moveTrainInsideStation = 0x200000, // pociąg między semaforem a rozjazdami lub następnym semaforem
    moveSpeedLimitFound = 0x400000 // pociąg w ograniczeniu z podaną jego długością
*/
};

enum TStopReason
{ // powód zatrzymania, dodawany do SetVelocity 0 - w zasadzie do usunięcia
    stopNone, // nie ma powodu - powinien jechać
    stopSleep, // nie został odpalony, to nie pojedzie
    stopSem, // semafor zamknięty
    stopTime, // czekanie na godzinę odjazdu
    stopEnd, // brak dalszej części toru
    stopDir, // trzeba stanąć, by zmienić kierunek jazdy
    stopJoin, // stoi w celu połączenia wagonów
    stopBlock, // przeszkoda na drodze ruchu
    stopComm, // otrzymano taką komendę (niewiadomego pochodzenia)
    stopOut, // komenda wyjazdu poza stację (raczej nie powinna zatrzymywać!)
    stopRadio, // komunikat przekazany radiem (Radiostop)
    stopExt, // komenda z zewnątrz
    stopError // z powodu błędu w obliczeniu drogi hamowania
};

enum class TAction : int
{ // przechowanie aktualnego stanu AI od poprzedniego przebłysku świadomości
    actUnknown, // stan nieznany (domyślny na początku)
    actPantUp, // podnieś pantograf (info dla użytkownika)
    actConv, // załącz przetwornicę (info dla użytkownika)
    actCompr, // załącz sprężarkę (info dla użytkownika)
    actSleep, //śpi (wygaszony)
    actDrive, // jazda
    actGo, // ruszanie z miejsca
    actSlow, // przyhamowanie przed ograniczeniem
    sctStop, // hamowanie w celu precyzyjnego zatrzymania
    actIdle, // luzowanie składu przed odjazdem
    actRelease, // luzowanie składu po zmniejszeniu prędkości
    actConnect, // dojazd w celu podczepienia
    actWait, // czekanie na przystanku
    actReady, // zgłoszona gotowość do odjazdu od kierownika
    actEmergency, // hamowanie awaryjne
    actGoUphill, // ruszanie pod górę
    actTest, // hamowanie kontrolne (podczas jazdy)
    actTrial // próba hamulca (na postoju)
};

enum TSpeedPosFlag
{ // wartości dla iFlag w TSpeedPos
    spNone = 0x0,
    spEnabled = 0x1, // pozycja brana pod uwagę
    spTrack = 0x2, // to jest tor
    spReverse = 0x4, // odwrotnie
    spSwitch = 0x8, // to zwrotnica
    spSwitchStatus = 0x10, // stan zwrotnicy
    spElapsed = 0x20, // pozycja minięta przez pojazd
    spEnd = 0x40, // koniec
    spCurve = 0x80, // łuk
    spEvent = 0x100, // event
    spShuntSemaphor = 0x200, // tarcza manewrowa
    spPassengerStopPoint = 0x400, // przystanek osobowy (wskaźnik W4)
    spStopOnSBL = 0x800, // zatrzymanie na SBL
    spCommandSent = 0x1000, // komenda wysłana
    spOutsideStation = 0x2000, // wskaźnik końca manewrów
    spSemaphor = 0x4000, // semafor pociągowy
    spRoadVel = 0x8000, // zadanie prędkości drogowej
    spSectionVel = 0x20000, // odcinek z ograniczeniem
    spProximityVelocity = 0x40000 // odcinek z ograniczeniem i podaną jego długościa
//    spDontApplySpeedLimit = 0x10000 // this point won't apply its speed limit. potentially set by the scanning vehicle
};

class TSpeedPos
{ // pozycja tabeli prędkości dla AI
  public:
    double fDist{ 0.0 }; // aktualna odległość (ujemna gdy minięte)
    double fVelNext{ -1.0 }; // prędkość obowiązująca od tego miejsca
    double fSectionVelocityDist{ 0.0 }; // długość ograniczenia prędkości
    int iFlags{ spNone }; // flagi typu wpisu do tabelki
	bool bMoved{ false }; // czy przesunięty (dotyczy punktu zatrzymania w peronie)
    Math3D::vector3 vPos; // współrzędne XYZ do liczenia odległości
    struct
    {
        TTrack *trTrack{ nullptr }; // wskaźnik na tor o zmiennej prędkości (zwrotnica, obrotnica)
        basic_event *evEvent{ nullptr }; // połączenie z eventem albo komórką pamięci
    };
    void CommandCheck();

  public:
    TSpeedPos(TTrack *track, double dist, int flag);
    TSpeedPos(basic_event *event, double dist, double length, TOrders order);
    TSpeedPos() = default;
    void Clear();
    bool Update();
    // aktualizuje odległość we wpisie
    inline
    void
        UpdateDistance( double dist ) {
            fDist -= dist; }
    bool Set(basic_event *e, double d, double length, TOrders order = Wait_for_orders);
    void Set(TTrack *t, double d, int f);
    std::string TableText() const;
    std::string GetName() const;
    bool IsProperSemaphor(TOrders order = Wait_for_orders);
};

//----------------------------------------------------------------------------
static const bool Aggressive = true;
static const bool Easyman = false;
static const bool AIdriver = true;
static const bool Humandriver = false;
static const int maxorders = 64; // ilość rozkazów w tabelce
static const int maxdriverfails = 4; // ile błędów może zrobić AI zanim zmieni nastawienie
extern bool WriteLogFlag; // logowanie parametrów fizycznych
static const int BrakeAccTableSize = 20;
//----------------------------------------------------------------------------

class TController {
    // TBD: few authorized inspectors, or bunch of public getters?
    friend class TTrain;
    friend class drivingaid_panel;
    friend class timetable_panel;
    friend class debug_panel;
    friend class whois_event;

public:
    TController( bool AI, TDynamicObject *NewControll, bool InitPsyche, bool primary = true );
    ~TController();

// ai operations logic
// methods
public:
    void UpdateSituation(double dt); // uruchamiac przynajmniej raz na sekundę
    void MoveTo(TDynamicObject *to);
    void TakeControl(bool yes);
    inline
    bool Primary() const {
        return ( ( iDrivigFlags & movePrimary ) != 0 ); };
    inline
    TMoverParameters const *Controlling() const {
        return mvControlling; }
    void DirectionInitial();
    inline
    int Direction() const {
        return iDirection; }
    inline 
    TAction GetAction() {
        return eAction; }
private:
    void Activation(); // umieszczenie obsady w odpowiednim członie
    void ControllingSet(); // znajduje człon do sterowania
    void SetDriverPsyche();
    bool IncBrake();
    bool DecBrake();
    bool IncSpeed();
    bool DecSpeed(bool force = false);
    void SpeedSet();
	void SpeedCntrl(double DesiredSpeed);
	double ESMVelocity(bool Main);
    bool UpdateHeating();
    // uaktualnia informacje o prędkości
    void SetVelocity(double NewVel, double NewVelNext, TStopReason r = stopNone);
    int CheckDirection();
    void WaitingSet(double Seconds);
    void DirectionForward(bool forward);
    int OrderDirectionChange(int newdir, TMoverParameters *Vehicle);
    void Lights(int head, int rear);
    std::string StopReasonText() const;
    double BrakeAccFactor() const;
    // modifies brake distance for low target speeds, to ease braking rate in such situations
    float
        braking_distance_multiplier( float const Targetvelocity ) const;
    inline
    int DrivigFlags() const {
        return iDrivigFlags; };
// members
public:
    bool AIControllFlag = false; // rzeczywisty/wirtualny maszynista
    int iOverheadZero = 0; // suma bitowa jezdy bezprądowej, bity ustawiane przez pojazdy z podniesionymi pantografami
    int iOverheadDown = 0; // suma bitowa opuszczenia pantografów, bity ustawiane przez pojazdy z podniesionymi pantografami
private:
    bool Psyche = false;
    int HelperState = 0; //stan pomocnika maszynisty
    TDynamicObject *pVehicle = nullptr; // pojazd w którym siedzi sterujący
    TMoverParameters *mvControlling = nullptr; // jakim pojazdem steruje (może silnikowym w EZT)
    TMoverParameters *mvOccupied = nullptr; // jakim pojazdem hamuje
    std::string VehicleName;
    std::array<int, 2> m_lighthints { -1 }; // suggested light patterns
    double AccPreferred = 0.0; // preferowane przyspieszenie (wg psychiki kierującego, zmniejszana przy wykryciu kolizji)
    double AccDesired = AccPreferred; // przyspieszenie, jakie ma utrzymywać (<0:nie przyspieszaj,<-0.1:hamuj)
    double VelDesired = 0.0; // predkość, z jaką ma jechać, wynikająca z analizy tableki; <=VelSignal
    double fAccDesiredAv = 0.0; // uśrednione przyspieszenie z kolejnych przebłysków świadomości, żeby ograniczyć migotanie
    double VelforDriver = -1.0; // prędkość, używana przy zmianie kierunku (ograniczenie przy nieznajmości szlaku?)
    double VelSignal = 0.0; // ograniczenie prędkości z kompilacji znaków i sygnałów // normalnie na początku ma stać, no chyba że jedzie
    double VelLimit = -1.0; // predkość zadawana przez event jednokierunkowego ograniczenia prędkości // -1: brak ograniczenia prędkości
    double VelSignalLast = -1.0; // prędkość zadana na ostatnim semaforze // ostatni semafor też bez ograniczenia
    double VelSignalNext = 0.0; // prędkość zadana na następnym semaforze
    double VelLimitLast = -1.0; // prędkość zadana przez ograniczenie // ostatnie ograniczenie bez ograniczenia
    double VelRoad = -1.0; // aktualna prędkość drogowa (ze znaku W27) (PutValues albo komendą) // prędkość drogowa bez ograniczenia
    double VelNext = 120.0; // prędkość, jaka ma być po przejechaniu długości ProximityDist
    double VelRestricted = -1.0; // speed of travel after passing a permissive signal at stop
    double FirstSemaphorDist = 10000.0; // odległość do pierwszego znalezionego semafora
    double ActualProximityDist = 1.0; // odległość brana pod uwagę przy wyliczaniu prędkości i przyspieszenia
    int iDirection = 0; // kierunek jazdy względem sprzęgów pojazdu, w którym siedzi AI (1=przód,-1=tył)
    int iDirectionOrder = 0; //żadany kierunek jazdy (służy do zmiany kierunku)
    int iVehicleCount = -2; // wartość neutralna // ilość pojazdów do odłączenia albo zabrania ze składu (-1=wszystkie)
    int iCoupler = 0; // maska sprzęgu, jaką należy użyć przy łączeniu (po osiągnięciu trybu Connect), 0 gdy jazda bez łączenia
    int iDriverFailCount = 0; // licznik błędów AI
    bool Need_TryAgain = false; // true, jeśli druga pozycja w elektryku nie załapała
    bool Need_BrakeRelease = true;
    bool IsAtPassengerStop{ false }; // true if the consist is within acceptable range of w4 post
    double fMinProximityDist = 30.0; // stawanie między 30 a 60 m przed przeszkodą // minimalna oległość do przeszkody, jaką należy zachować
    double fOverhead1 = 3000.0; // informacja o napięciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
    double fOverhead2 = -1.0; // informacja o sposobie jazdy (-1=normalnie, 0=bez prądu, >0=z opuszczonym i ograniczeniem prędkości)
    double fVoltage = 0.0; // uśrednione napięcie sieci: przy spadku poniżej wartości minimalnej opóźnić rozruch o losowy czas
    double fMaxProximityDist = 50.0; // stawanie między 30 a 60 m przed przeszkodą // akceptowalna odległość stanięcia przed przeszkodą
    TStopReason eStopReason = stopSleep; // powód zatrzymania przy ustawieniu zerowej prędkości // na początku śpi
    double fVelPlus = 0.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
    double fVelMinus = 0.0; // margines obniżenia prędkości, powodujący załączenie napędu
    double fWarningDuration = 0.0; // ile czasu jeszcze trąbić
    double WaitingTime = 0.0; // zliczany czas oczekiwania do samoistnego ruszenia
    double WaitingExpireTime = 31.0; // tyle ma czekać, zanim się ruszy // maksymlany czas oczekiwania do samoistnego ruszenia
    double IdleTime {}; // keeps track of time spent at a stop
    double fStopTime = 0.0; // czas postoju przed dalszą jazdą (np. na przystanku)
    double fShuntVelocity = 40.0; // maksymalna prędkość manewrowania, zależy m.in. od składu // domyślna prędkość manewrowa
    int iDrivigFlags = // flagi bitowe ruchu
        moveStopPoint | // podjedź do W4 możliwie blisko
        moveStopHere | // nie podjeżdżaj do semafora, jeśli droga nie jest wolna
        moveStartHorn; // podaj sygnał po podaniu wolnej drogi
    double fDriverBraking = 0.0; // po pomnożeniu przez v^2 [km/h] daje ~drogę hamowania [m]
    double fDriverDist = 0.0; // dopuszczalna odległość podjechania do przeszkody
    double fVelMax = -1.0; // maksymalna prędkość składu (sprawdzany każdy pojazd)
    double fBrakeDist = 0.0; // przybliżona droga hamowania
	double fBrakeReaction = 1.0; //opóźnienie zadziałania hamulca - czas w s / (km/h)
	double fNominalAccThreshold = 0.0; // nominalny próg opóźnienia dla zadziałania hamulca
    double fAccThreshold = 0.0; // aktualny próg opóźnienia dla zadziałania hamulca
	double AbsAccS_pub = 0.0; // próg opóźnienia dla zadziałania hamulca
    // dla fBrake_aX:
    // indeks [0] - wartości odpowiednie dla aktualnej prędkości
    // a potem jest 20 wartości dla różnych prędkości zmieniających się co 5 % Vmax pojazdu obsadzonego
	double fBrake_a0[BrakeAccTableSize+1] = { 0.0 }; // opóźnienia hamowania przy ustawieniu zaworu maszynisty w pozycji 1.0
	double fBrake_a1[BrakeAccTableSize+1] = { 0.0 }; // przyrost opóźnienia hamowania po przestawieniu zaworu maszynisty o 0,25 pozycji
    double BrakingInitialLevel{ 1.0 };
    double BrakingLevelIncrease{ 0.25 };
    double ReactionTime = 0.0; // czas reakcji Ra: czego i na co? świadomości AI
    double fBrakeTime = 0.0; // wpisana wartość jest zmniejszana do 0, gdy ujemna należy zmienić nastawę hamulca
    double BrakeChargingCooldown {}; // prevents the ai from trying to charge the train brake too frequently
    double LastReactionTime = 0.0;
    double fActionTime = 0.0; // czas używany przy regulacji prędkości i zamykaniu drzwi
    double m_radiocontroltime{ 0.0 }; // timer used to control speed of radio operations
    TAction eAction { TAction::actUnknown }; // aktualny stan

// orders
// methods
public:
    void PutCommand(std::string NewCommand, double NewValue1, double NewValue2, const TLocation &NewLocation, TStopReason reason = stopComm);
    bool PutCommand( std::string NewCommand, double NewValue1, double NewValue2, glm::dvec3 const *NewLocation, TStopReason reason = stopComm );
private:
    void RecognizeCommand(); // odczytuje komende przekazana lokomotywie
    void JumpToNextOrder();
    void JumpToFirstOrder();
    void OrderPush(TOrders NewOrder);
    void OrderNext(TOrders NewOrder);
    inline TOrders OrderCurrentGet();
    inline TOrders OrderNextGet();
    void OrderCheck();
    void OrdersInit(double fVel);
    void OrdersClear();
    void OrdersDump();
    std::string OrderCurrent() const;
    std::string Order2Str(TOrders Order) const;
// members
    Math3D::vector3 vCommandLocation; // polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda // NOTE: not used
    TOrders OrderList[ maxorders ]; // lista rozkazów
    int OrderPos = 0,
        OrderTop = 0; // rozkaz aktualny oraz wolne miejsce do wstawiania nowych

// scantable
// methods
public:
    int CrossRoute(TTrack *tr);
    inline void MoveDistanceAdd( double distance ) {
        dMoveLen += distance * iDirection; } //jak jedzie do tyłu to trzeba uwzględniać, że distance jest ujemna
private:
    // Ra: metody obsługujące skanowanie toru
    std::vector<basic_event *> CheckTrackEvent( TTrack *Track, double const fDirection ) const;
    bool TableAddNew();
    bool TableNotFound( basic_event const *Event ) const;
    void TableTraceRoute( double fDistance, TDynamicObject *pVehicle );
    void TableCheck( double fDistance );
    TCommandType TableUpdate( double &fVelDes, double &fDist, double &fNext, double &fAcc );
    // returns most recently calculated distance to potential obstacle ahead
    double TrackBlock() const;
    void TablePurger();
    void TableSort();
    inline double MoveDistanceGet() const {
        return dMoveLen;
    }
    inline void MoveDistanceReset() {
        dMoveLen = 0.0;
    }
    std::size_t TableSize() const { return sSpeedTable.size(); }
    void TableClear();
    int TableDirection() { return iTableDirection; }
    // Ra: stare funkcje skanujące, używane do szukania sygnalizatora z tyłu
    bool BackwardTrackBusy( TTrack *Track );
    basic_event *CheckTrackEventBackward( double fDirection, TTrack *Track );
    TTrack *BackwardTraceRoute( double &fDistance, double &fDirection, TTrack *Track, basic_event *&Event );
    void SetProximityVelocity( double dist, double vel, glm::dvec3 const *pos );
    TCommandType BackwardScan();
    std::string TableText(std::size_t const Index) const;
/*
    void RouteSwitch(int d);
*/
// members
    int iLast{ 0 }; // ostatnia wypełniona pozycja w tabeli <iFirst (modulo iSpeedTableSize)
    int iTableDirection{ 0 }; // kierunek zapełnienia tabelki względem pojazdu z AI
    std::vector<TSpeedPos> sSpeedTable;
    double fLastVel = 0.0; // prędkość na poprzednio sprawdzonym torze
    TTrack *tLast = nullptr; // ostatni analizowany tor
    basic_event *eSignSkip = nullptr; // można pominąć ten SBL po zatrzymaniu
    std::size_t SemNextIndex{ std::size_t(-1) };
    std::size_t SemNextStopIndex{ std::size_t( -1 ) };
    double dMoveLen = 0.0; // odległość przejechana od ostatniego sprawdzenia tabelki
    basic_event *eSignNext = nullptr; // sygnał zmieniający prędkość, do pokazania na [F2]

// timetable
// methods
public:
    std::string TrainName() const;
private:
    std::string Relation() const;
    Mtable::TTrainParameters const * TrainTimetable() const;
    int StationIndex() const;
    int StationCount() const;
    bool IsStop() const;
    std::string NextStop() const;
    // tests whether the train is delayed and sets accordingly a driving flag
    void UpdateDelayFlag();
// members
    Mtable::TTrainParameters *TrainParams = nullptr; // rozkład jazdy zawsze jest, nawet jeśli pusty
    std::string asNextStop; // nazwa następnego punktu zatrzymania wg rozkładu
    int iStationStart = 0; // numer pierwszej stacji pokazywanej na podglądzie rozkładu
    double fLastStopExpDist = -1.0; // odległość wygasania ostateniego przystanku
    int iRadioChannel = 1; // numer aktualnego kanału radiowego
    int iGuardRadio = 0; // numer kanału radiowego kierownika (0, gdy nie używa radia)
    sound_source tsGuardSignal { sound_placement::internal };

// consist
// methods
public:
private:
    bool CheckVehicles(TOrders user = Wait_for_orders);
    bool PrepareEngine();
    bool ReleaseEngine();
    void Doors(bool const Open, int const Side = 0);
    // returns true if any vehicle in the consist has an open door
    bool doors_open() const;
    void AutoRewident(); // ustawia hamulce w składzie
// members
    double fLength = 0.0; // długość składu (do wyciągania z ograniczeń)
    double fMass = 0.0; // całkowita masa do liczenia stycznej składowej grawitacji
    double fAccGravity = 0.0; // przyspieszenie składowej stycznej grawitacji
    int iVehicles = 0; // ilość pojazdów w składzie
    int iEngineActive = 0; // ABu: Czy silnik byl juz zalaczony; Ra: postęp w załączaniu
    bool IsCargoTrain{ false };
    bool IsHeavyCargoTrain{ false };
    bool IsLineBreakerClosed{ false }; // state of line breaker in all powered vehicles under control
    double fReady = 0.0; // poziom odhamowania wagonów
    bool Ready = false; // ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow
    TDynamicObject *pVehicles[ 2 ]; // skrajne pojazdy w składzie (niekoniecznie bezpośrednio sterowane)

// logs
// methods
    void PhysicsLog();
    void CloseLog();
// members
    std::ofstream LogFile; // zapis parametrow fizycznych
    double LastUpdatedTime = 0.0; // czas od ostatniego logu
    double ElapsedTime = 0.0; // czas od poczatku logu

// getters
public:
    TDynamicObject const *Vehicle() const {
        return pVehicle; }
    TDynamicObject *Vehicle( side const Side ) const {
        return pVehicles[ Side ]; }
private:
    std::string OwnerName() const;

// leftovers
/*
    int iRouteWanted = 3; // oczekiwany kierunek jazdy (0-stop,1-lewo,2-prawo,3-prosto) np. odpala migacz lub czeka na stan zwrotnicy
*/

};
