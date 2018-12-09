/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once
//---------------------------------------------------------------------------
//Q: 20160805 - odlaczenie pliku fizyki .pas od kompilacji
#include <map>
#include "hamulce.h"
/*
MaSzyna EU07 locomotive simulator
Copyright (C) 2001-2004  Maciej Czapkiewicz and others

*/


/*
(C) McZapkie v.2004.02
Co brakuje:
6. brak pantografow itp  'Nie do konca, juz czesc zrobiona - Winger
7. brak efektu grzania oporow rozruchowych, silnika, osi
9. ulepszyc sprzeg sztywny oraz automatyczny
10. klopoty z EN57
...
n. Inne lokomotywy oprocz elektrowozu pradu stalego
*/
/*
Zrobione:
1. model szeregowego silnika elektrycznego sterowanego reostatem
2. fizyka ruchu - sily oporu, nachylenia, przyspieszenia styczne i normalne,
przyczepnosc/poslizg przy hamowaniu i rozruchu
3. docisk klockow hamulcowych -hamulec reczny oraz pomocniczy hamulec pneumatyczny
4. ubytki cisnienia wskutek hamowania, kompresor - ladowanie zbiornika
5. niektore uszkodzenia i awarie - logika rozmyta
a) silnik elektryczny - za duzy prad, napiecie, obroty
b) sprzegi - zerwanie gdy zbyt duze sily
6. flagi niektorych dzwiekow
7. hamulec zasadniczy - elektropneumatyczny (impulsowanie), pneumatyczny - zmiany cisnienia
zwiekszenie nacisku przy duzych predkosciach w hamulcach Oerlikona
8. sprzegi - sila, tlumiennosc
9. lok. el. - wylacznik glowny, odlacznik uszkodzonych silnikow
10. parametry trakcji
11. opoznienia w zalaczaniu stycznikow
12. trakcja wielokrotna
13. informacja na temat zrodla mocy dla silnikow trakcyjnych, ogrzewania, oswietlenia
14. dumb - uproszczony model lokomotywy (tylko dla AI)
15. ladunki - ilosc, typ, szybkosc na- i rozladunku, funkcje przetestowac!
16. ulepszony model fizyczny silnika elektrycznego (hamowanie silnikiem, ujemne napiecia)
17. poprawione hamulce - parametry cylindrow itp
18. ulepszone hamulce - napelnianie uderzeniowe, stopnie hamowania
19. poprawione hamulce - wyszczegolnenie cisnienia dla stopni hamowania
20. poprawione hamulce elektropneumatyczne
21. poprawione hamowanie przeciwposlizgowe i odluzniacz
22. dodany model mechanicznego napedu recznego (drezyna)
23. poprawiona szybkosc hamowania pomocniczego i przeciwposlizgowego, odlaczanie silnikow, odlaczanie bocznikow
24. wprowadzone systemy zabezpieczenia: SHP, Czuwak, sygnalizacja kabinowa (nie dzialaja w DebugMode).
25. poprawiona predkosc propagacji fali cisnienia (normalizacja przez kwadrat dlugosci) //to jest zdeka liniowe
26. uwzgledniona masa ladunku, przesuniecie w/m toru ruchu
27. lampy/sygnaly przednie/tylne
28. wprowadzona pozycja odciecia hamulca (yB: tja, ale oerlikona....)
29. rozne poprawki: slizganie, wykolejanie itp
30. model silnika spalinowego z przekladnia mechaniczna
31. otwieranie drzwi, sprezarka, przetwornica
32. wylaczanie obliczen dla nieruchomych wagonow
33. Zbudowany model rozdzielacza powietrza roznych systemow
34. Poprawiona pozycja napelniania uderzeniowego i hamulec EP
35. Dodane baterie akumulatorow (KURS90)
36. Hamowanie elektrodynamiczne w ET42 i poprawione w EP09
37. jednokierunkowosc walu kulakowego w EZT (nie do konca)
38. wal kulakowy w ET40
39. poprawiona blokada nastawnika w ET40
...
*/

#include "dumb3d.h"

extern int ConversionError;

const double Steel2Steel_friction = 0.15;      //tarcie statyczne
const double g = 9.81;                     //przyspieszenie ziemskie
const double SandSpeed = 0.1;              //ile kg/s}
const double Pirazy2 = 6.2831853071794f;

//-- var, const, procedure ---------------------------------------------------
static bool const Go = true;
static bool const Hold = false;                 /*dla CheckLocomotiveParameters*/
static int const ResArraySize = 64;            /*dla silnikow elektrycznych*/
static int const MotorParametersArraySize = 10;
static int const maxcc = 4;                    /*max. ilosc odbierakow pradu*/
//static int const LocalBrakePosNo = 10;         /*ilosc nastaw hamulca pomocniczego*/
//static int const MainBrakeMaxPos = 10;          /*max. ilosc nastaw hamulca zasadniczego*/
static int const ManualBrakePosNo = 20;        /*ilosc nastaw hamulca recznego*/
static int const LightsSwitchPosNo = 16;

/*uszkodzenia toru*/
static int const dtrack_railwear = 2;
static int const dtrack_freerail = 4;
static int const dtrack_thinrail = 8;
static int const dtrack_railbend = 16;
static int const dtrack_plants = 32;
static int const dtrack_nomove = 64;
static int const dtrack_norail = 128;

/*uszkodzenia taboru*/
static int const dtrain_thinwheel = 1;     /*dla lokomotyw*/
static int const dtrain_loadshift = 1;     /*dla wagonow*/
static int const dtrain_wheelwear = 2;
static int const dtrain_bearing = 4;
static int const dtrain_coupling = 8;
static int const dtrain_ventilator = 16;   /*dla lokomotywy el.*/
static int const dtrain_loaddamage = 16;   /*dla wagonow*/
static int const dtrain_engine = 32;       /*dla lokomotyw*/
static int const dtrain_loaddestroyed = 32;/*dla wagonow*/
static int const dtrain_axle = 64;
static int const dtrain_out = 128;         /*wykolejenie*/

										   /*wagi prawdopodobienstwa dla funkcji FuzzyLogic*/
#define p_elengproblem  (1.000000E-02)
#define p_elengdamage  (1.000000E-01)
#define p_coupldmg  (2.000000E-02)
#define p_derail  (1.000000E-03)
#define p_accn  (1.000000E-01)
#define p_slippdmg  (1.000000E-03)

										   /*typ sprzegu*/
static int const ctrain_virtual = 0;        //gdy pojazdy na tym samym torze się widzą wzajemnie
static int const ctrain_coupler = 1;        //sprzeg fizyczny
static int const ctrain_pneumatic = 2;      //przewody hamulcowe
static int const ctrain_controll = 4;       //przewody sterujące (ukrotnienie)
static int const ctrain_power = 8;          //przewody zasilające (WN)
static int const ctrain_passenger = 16;     //mostek przejściowy
static int const ctrain_scndpneumatic = 32; //przewody 8 atm (żółte; zasilanie powietrzem)
static int const ctrain_heating = 64;       //przewody ogrzewania WN
static int const ctrain_depot = 128;        //nie rozłączalny podczas zwykłych manewrów (międzyczłonowy), we wpisie wartość ujemna
// vehicle sides; exclusive
enum side {
    front = 0,
    rear
};
// possible coupling types; can be combined
enum coupling {
    faux = 0x0,
    coupler = 0x1,
    brakehose = 0x2,
    control = 0x4,
    highvoltage = 0x8,
    gangway = 0x10,
    mainhose = 0x20,
    heating = 0x40,
    permanent = 0x80,
    uic = 0x100
};
// possible effect ranges for control commands; exclusive
enum class range_t {
    local,
    unit,
    consist
};
// start method for devices; exclusive
enum class start_t {
    manual,
    automatic,
    manualwithautofallback,
    converter,
    battery
};
// recognized vehicle light locations and types; can be combined
enum light {

    headlight_left = 0x01,
    redmarker_left = 0x02,
    headlight_upper = 0x04,
    headlight_right = 0x10,
    redmarker_right = 0x20,
    rearendsignals = 0x40
};

// door operation methods; exclusive
enum control_t {
    passenger, // local, opened/closed for duration of loading
    driver, // remote, operated by the driver
    autonomous, // local, closed when vehicle moves and/or after timeout
    conductor, // remote, operated by the conductor
    mixed // primary manual but answers also to remote control
};
											/*typ hamulca elektrodynamicznego*/
static int const dbrake_none = 0;
static int const dbrake_passive = 1;
static int const dbrake_switch = 2;
static int const dbrake_reversal = 4;
static int const dbrake_automatic = 8;

/*status czuwaka/SHP*/
//hunter-091012: rozdzielenie alarmow, dodanie testu czuwaka
static int const s_waiting = 1; //działa
static int const s_aware = 2;   //czuwak miga
static int const s_active = 4;  //SHP świeci
static int const s_CAalarm = 8;    //buczy
static int const s_SHPalarm = 16;  //buczy
static int const s_CAebrake = 32;  //hamuje
static int const s_SHPebrake = 64; //hamuje
static int const s_CAtest = 128;

/*dzwieki*/
enum sound {
    none,
    loud = 1 << 0,
    couplerstretch = 1 << 1,
    bufferclash = 1 << 2,
    relay = 1 << 3,
    parallel = 1 << 4,
    shuntfield = 1 << 5,
    pneumatic = 1 << 6,
    detachall = 1 << 7,
    attachcoupler = 1 << 8,
    attachbrakehose = 1 << 9,
    attachmainhose = 1 << 10,
    attachcontrol = 1 << 11,
    attachgangway = 1 << 12,
    attachheating = 1 << 13
};

//szczególne typy pojazdów (inna obsługa) dla zmiennej TrainType
//zamienione na flagi bitowe, aby szybko wybierać grupę (np. EZT+SZT)
// TODO: convert to enums, they're used as specific checks anyway
static int const dt_Default = 0;
static int const dt_EZT = 1;
static int const dt_ET41 = 2;
static int const dt_ET42 = 4;
static int const dt_PseudoDiesel = 8;
static int const dt_ET22 = 0x10; //używane od Megapacka
static int const dt_SN61 = 0x20; //nie używane w warunkach, ale ustawiane z CHK
static int const dt_EP05 = 0x40;
static int const dt_ET40 = 0x80;
static int const dt_181 = 0x100;
static int const dt_DMU = 0x200;

//stałe dla asynchronów
static int const eimc_s_dfic = 0;
static int const eimc_s_dfmax = 1;
static int const eimc_s_p = 2;
static int const eimc_s_cfu = 3;
static int const eimc_s_cim = 4;
static int const eimc_s_icif = 5;
static int const eimc_f_Uzmax = 7;
static int const eimc_f_Uzh = 8;
static int const eimc_f_DU = 9;
static int const eimc_f_I0 = 10;
static int const eimc_f_cfu = 11;
static int const eimc_p_F0 = 13;
static int const eimc_p_a1 = 14;
static int const eimc_p_Pmax = 15;
static int const eimc_p_Fh = 16;
static int const eimc_p_Ph = 17;
static int const eimc_p_Vh0 = 18;
static int const eimc_p_Vh1 = 19;
static int const eimc_p_Imax = 20;
static int const eimc_p_abed = 21;
static int const eimc_p_eped = 22;

//zmienne dla asynchronów
static int const eimv_FMAXMAX = 0;
static int const eimv_Fmax = 1;
static int const eimv_ks = 2;
static int const eimv_df = 3;
static int const eimv_fp = 4;
static int const eimv_U = 5;
static int const eimv_pole = 6;
static int const eimv_Ic = 7;
static int const eimv_If = 8;
static int const eimv_M = 9;
static int const eimv_Fr = 10;
static int const eimv_Ipoj = 11;
static int const eimv_Pm = 12;
static int const eimv_Pe = 13;
static int const eimv_eta = 14;
static int const eimv_fkr = 15;
static int const eimv_Uzsmax = 16;
static int const eimv_Pmax = 17;
static int const eimv_Fzad = 18;
static int const eimv_Imax = 19;
static int const eimv_Fful = 20;

static int const bom_PS = 1;
static int const bom_PN = 2;
static int const bom_EP = 4;
static int const bom_MED = 8;

enum TProblem // lista problemów taboru, które uniemożliwiają jazdę
{ // flagi bitowe
	pr_Hamuje = 1, // pojazd ma załączony hamulec lub zatarte osie
	pr_Pantografy = 2, // pojazd wymaga napompowania pantografów
	pr_Ostatni = 0x80000000 // ostatnia flaga bitowa
};

/*ogolne*/
/*lokacja*/
struct TLocation
{
	double X;
	double Y;
	double Z;
};
/*rotacja*/
struct TRotation
{
	double Rx;
	double Ry;
	double Rz;
};
/*wymiary*/
struct TDimension
{
	double W = 0.0;
	double L = 0.0;
	double H = 0.0;
};

struct TCommand
{
	std::string Command; /*komenda*/
	double Value1 = 0.0; /*argumenty komendy*/
	double Value2 = 0.0;
    int Coupling { coupling::control }; // coupler flag used to determine command propagation
    TLocation Location;
};

/*tory*/
struct TTrackShape
{/*ksztalt odcinka*/
	double R = 0.0; // promien
	double Len = 0.0; // dlugosc
	double dHtrack = 0.0; // nachylenie
	double dHrail = 0.0; // przechylka
};

struct TTrackParam
{/*parametry odcinka - szerokosc, tarcie statyczne, kategoria,  obciazalnosc w t/os, uszkodzenia*/
	double Width = 0.0;
	double friction = 0.0;
	int CategoryFlag = 0;
	int QualityFlag = 0;
	int DamageFlag = 0;
	double Velmax; /*dla uzytku maszynisty w ai_driver*/
};

struct TTractionParam
{
	double TractionVoltage = 0.0; /*napiecie*/
	double TractionFreq = 0.0; /*czestotliwosc*/
	double TractionMaxCurrent = 0.0; /*obciazalnosc*/
	double TractionResistivity = 0.0; /*rezystancja styku*/
};
/*powyzsze parametry zwiazane sa z torem po ktorym aktualnie pojazd jedzie*/

/*typy hamulcow zespolonych*/
enum class TBrakeSystem { Individual, Pneumatic, ElectroPneumatic };
/*podtypy hamulcow zespolonych*/
enum class TBrakeSubSystem { ss_None, ss_W, ss_K, ss_KK, ss_Hik, ss_ESt, ss_KE, ss_LSt, ss_MT, ss_Dako };
enum class TBrakeValve { NoValve, W, W_Lu_VI, W_Lu_L, W_Lu_XR, K, Kg, Kp, Kss, Kkg, Kkp, Kks, Hikg1, Hikss, Hikp1, KE, SW, EStED, NESt3, ESt3, LSt, ESt4, ESt3AL2, EP1, EP2, M483, CV1_L_TR, CV1, CV1_R, Other };
enum class TBrakeHandle { NoHandle, West, FV4a, M394, M254, FVel1, FVel6, D2, Knorr, FD1, BS2, testH, St113, MHZ_P, MHZ_T, MHZ_EN57, MHZ_K5P, MHZ_K8P };
/*typy hamulcow indywidualnych*/
enum class TLocalBrake { NoBrake, ManualBrake, PneumaticBrake, HydraulicBrake };
/*dla osob/towar: opoznienie hamowania/odhamowania*/
typedef double TBrakeDelayTable[4];

struct TBrakePressure
{
	double PipePressureVal = 0.0;
	double BrakePressureVal = 0.0;
	double FlowSpeedVal = 0.0;
	TBrakeSystem BrakeType = TBrakeSystem::Pneumatic;
};

typedef std::map<int,TBrakePressure> TBrakePressureTable;

/*typy napedow*/
enum class TEngineType { None, Dumb, WheelsDriven, ElectricSeriesMotor, ElectricInductionMotor, DieselEngine, SteamEngine, DieselElectric };
/*postac dostarczanej energii*/
enum class TPowerType { NoPower, BioPower, MechPower, ElectricPower, SteamPower };
/*rodzaj paliwa*/
enum class TFuelType { Undefined, Coal, Oil };
/*rodzaj rusztu*/
struct TGrateType {
	TFuelType FuelType;
	double GrateSurface;
	double FuelTransportSpeed;
	double IgnitionTemperature;
	double MaxTemperature;
    //inline TGrateType() {
    //    FuelType = Undefined;
    //    GrateSurface = 0.0;
    //    FuelTransportSpeed = 0.0;
    //    IgnitionTemperature = 0.0;
    //    MaxTemperature = 0.0;
    //}
};
/*rodzaj kotla*/
struct TBoilerType {
	double BoilerVolume;
	double BoilerHeatSurface;
	double SuperHeaterSurface;
	double MaxWaterVolume; double MinWaterVolume;
	double MaxPressure;
    //inline TBoilerType() {
    //    BoilerVolume = 0.0;
    //    BoilerHeatSurface = 0.0;
    //    SuperHeaterSurface = 0.0;
    //    MaxWaterVolume = 0.0;
    //    MinWaterVolume = 0.0;
    //    MaxPressure = 0.0;
    //}
};
/*rodzaj odbieraka pradu*/
struct TCurrentCollector {
    long CollectorsNo; //musi być tu, bo inaczej się kopie
    double MinH; double MaxH; //zakres ruchu pantografu, nigdzie nie używany
    double CSW;       //szerokość części roboczej (styku) ślizgacza
    double MinV; double MaxV; //minimalne i maksymalne akceptowane napięcie
    bool OVP;       //czy jest przekaznik nadnapieciowy
    double InsetV;    //minimalne napięcie wymagane do załączenia
    double MinPress;  //minimalne ciśnienie do załączenia WS
    double MaxPress;  //maksymalne ciśnienie za reduktorem
    //inline TCurrentCollector() {
    //    CollectorsNo = 0;
    //    MinH, MaxH, CSW, MinV, MaxV = 0.0;
    //    OVP, InsetV, MinPress, MaxPress = 0.0;
    //}
};
/*typy źródeł mocy*/
enum class TPowerSource { NotDefined, InternalSource, Transducer, Generator, Accumulator, CurrentCollector, PowerCable, Heater };


struct _mover__1
{
	double MaxCapacity;
	TPowerSource RechargeSource;
    //inline _mover__1() {
    //    MaxCapacity = 0.0;
    //    RechargeSource = NotDefined;
    //}
};

struct _mover__2
{
	TPowerType PowerTrans;
	double SteamPressure;
    //inline _mover__2() {
    //    SteamPressure = 0.0;
    //    PowerTrans = NoPower;
    //}
};

struct _mover__3
{
	TGrateType Grate;
	TBoilerType Boiler;
};

/*parametry źródeł mocy*/
struct TPowerParameters
{
	double MaxVoltage;
	double MaxCurrent;
	double IntR;
	TPowerSource SourceType;
	union
	{
		struct
		{
			_mover__3 RHeater;
		};
		struct
		{
			_mover__2 RPowerCable;
		};
		struct
		{
			TCurrentCollector CollectorParameters;
		};
		struct
		{
			_mover__1 RAccumulator;
		};
		struct
		{
			TEngineType GeneratorEngine;
		};
		struct
		{
			double InputVoltage;
		};
		struct
		{
			TPowerType PowerType;
		};

	};
    inline TPowerParameters()
    {
        MaxVoltage = 0.0;
        MaxCurrent = 0.0;
        IntR = 0.001;
        SourceType = TPowerSource::NotDefined;
        PowerType = TPowerType::NoPower;
        RPowerCable.PowerTrans = TPowerType::NoPower;
    }
};

/*dla lokomotyw elektrycznych:*/
struct TScheme
{
	int Relay = 0; /*numer pozycji rozruchu samoczynnego*/
	double R = 0.0; /*opornik rozruchowy*/ /*dla dizla napelnienie*/
	int Bn = 0;
	int Mn = 0; /*ilosc galezi i silnikow w galezi*/ /*dla dizla Mn: czy luz czy nie*/
	bool AutoSwitch = false; /*czy dana pozycja nastawniana jest recznie czy autom.*/
	int ScndAct = 0; /*jesli ma bocznik w nastawniku, to ktory bocznik na ktorej pozycji*/
};
typedef TScheme TSchemeTable[ResArraySize + 1]; /*tablica rezystorow rozr.*/
struct TDEScheme
{
	double RPM = 0.0; /*obroty diesla*/
	double GenPower = 0.0; /*moc maksymalna*/
	double Umax = 0.0; /*napiecie maksymalne*/
	double Imax = 0.0; /*prad maksymalny*/
};
typedef TDEScheme TDESchemeTable[33]; /*tablica rezystorow rozr.*/
struct TShuntScheme
{
	double Umin = 0.0;
	double Umax = 0.0;
	double Pmin = 0.0;
	double Pmax = 0.0;
};
typedef TShuntScheme TShuntSchemeTable[33];
struct TMPTRelay
{/*lista przekaznikow bocznikowania*/
	double Iup = 0.0;
	double Idown = 0.0;
};
typedef TMPTRelay TMPTRelayTable[8];

struct TMotorParameters
{
	double mfi;
	double mIsat;
	double mfi0; // aproksymacja M(I) silnika} {dla dizla mIsat=przekladnia biegu
	double fi;
	double Isat;
	double fi0; // aproksymacja E(n)=fi*n}    {dla dizla fi, mfi: predkosci przelozenia biegu <->
	bool AutoSwitch;
    TMotorParameters() {
        mfi = 0.0;
        mIsat = 0.0;
        mfi0 = 0.0;
        fi = 0.0;
        Isat = 0.0;
        fi0 = 0.0;
        AutoSwitch = false;
    }
};

struct TSecuritySystem
{
    int SystemType { 0 }; /*0: brak, 1: czuwak aktywny, 2: SHP/sygnalizacja kabinowa*/
    double AwareDelay { -1.0 }; // czas powtarzania czuwaka
	double AwareMinSpeed; // minimalna prędkość załączenia czuwaka, normalnie 10% Vmax
    double SoundSignalDelay { -1.0 };
    double EmergencyBrakeDelay { -1.0 };
    int Status { 0 }; /*0: wylaczony, 1: wlaczony, 2: czuwak, 4: shp, 8: alarm, 16: hamowanie awaryjne*/
    double SystemTimer { 0.0 };
    double SystemSoundCATimer { 0.0 };
    double SystemSoundSHPTimer { 0.0 };
    double SystemBrakeCATimer { 0.0 };
    double SystemBrakeSHPTimer { 0.0 };
    double SystemBrakeCATestTimer { 0.0 }; // hunter-091012
	int VelocityAllowed;
	int NextVelocityAllowed; /*predkosc pokazywana przez sygnalizacje kabinowa*/
	bool RadioStop; // czy jest RadioStop
};

struct TTransmision
{//liczba zebow przekladni
	int NToothM = 0;
	int NToothW = 0;
	double Ratio = 1.0;
};

enum class TCouplerType { NoCoupler, Articulated, Bare, Chain, Screw, Automatic };

struct power_coupling {
    double current{ 0.0 };
    double voltage{ 0.0 };
    bool local{ false }; // whether the power comes from external or onboard source
};

struct TCoupling {
	/*parametry*/
    double SpringKB = 1.0;   /*stala sprezystosci zderzaka/sprzegu, %tlumiennosci */
    double SpringKC = 1.0;
    double beta = 0.0;
    double DmaxB = 0.1; /*tolerancja scisku/rozciagania, sila rozerwania*/
    double FmaxB = 1000.0;
    double DmaxC = 0.1;
    double FmaxC = 1000.0;
	TCouplerType CouplerType = TCouplerType::NoCoupler;     /*typ sprzegu*/
								  /*zmienne*/
	int CouplingFlag = 0; /*0 - wirtualnie, 1 - sprzegi, 2 - pneumatycznie, 4 - sterowanie, 8 - kabel mocy*/
	int AllowedFlag = 3;       //Ra: znaczenie jak wyżej, maska dostępnych
	bool Render = false;             /*ABu: czy rysowac jak zaczepiony sprzeg*/
	double CoupleDist = 0.0;            /*ABu: optymalizacja - liczenie odleglosci raz na klatkę, bez iteracji*/
	class TMoverParameters *Connected = nullptr; /*co jest podlaczone*/
    int ConnectedNr = 0;           //Ra: od której strony podłączony do (Connected): 0=przód, 1=tył
	double CForce = 0.0;                /*sila z jaka dzialal*/
	double Dist = 0.0;                  /*strzalka ugiecia zderzaków*/
	bool CheckCollision = false;     /*czy sprawdzac sile czy pedy*/

    power_coupling power_high;
    power_coupling power_low; // TODO: implement this

    int sounds { 0 }; // sounds emitted by the coupling devices
};

class TMoverParameters
{ // Ra: wrapper na kod pascalowy, przejmujący jego funkcje  Q: 20160824 - juz nie wrapper a klasa bazowa :)
private:
// types

    // basic approximation of a generic device
    // TBD: inheritance or composition?
    struct basic_device {
        // config
        start_t start_type { start_t::manual };
        // ld inputs
        bool is_enabled { false }; // device is allowed/requested to operate
        bool is_disabled { false }; // device is requested to stop
        // TODO: add remaining inputs; start conditions and potential breakers
        // ld outputs
        bool is_active { false }; // device is working
    };

    struct cooling_fan : public basic_device {
        // config
        float speed { 0.f }; // cooling fan rpm; either fraction of parent rpm, or absolute value if negative
        // ld outputs
        float revolutions { 0.f }; // current fan rpm
    };

    // basic approximation of a fuel pump
    struct fuel_pump : public basic_device {
        // TODO: fuel consumption, optional automatic engine start after activation
    };

    // basic approximation of an oil pump
    struct oil_pump : public basic_device {
        // config
        float pressure_minimum { 0.f }; // lowest acceptable working pressure
        float pressure_maximum { 0.65f }; // oil pressure at maximum engine revolutions
        // ld inputs
        float resource_amount { 1.f }; // amount of affected resource, compared to nominal value
        // internal data
        float pressure_target { 0.f };
        // ld outputs
        float pressure { 0.f }; // current pressure
    };

    // basic approximation of a water pump
    struct water_pump : public basic_device {
        // ld inputs
        // TODO: move to breaker list in the basic device once implemented
        bool breaker { true }; // device is allowed to operate
    };

    struct water_heater {
        // config
        struct heater_config_t {
            float temp_min { -1 }; // lowest accepted temperature
            float temp_max { -1 }; // highest accepted temperature
        } config;
        // ld inputs
        bool breaker { true }; // device is allowed to operate
        bool is_enabled { false }; // device is requested to operate
        // ld outputs
        bool is_active { false }; // device is working
        bool is_damaged { false }; // device is damaged
    };

    struct heat_data {
        // input, state of relevant devices
        bool cooling { false }; // TODO: user controlled device, implement
    //    bool okienko { true }; // window in the engine compartment
        // system configuration
        bool auxiliary_water_circuit { false }; // cooling system has an extra water circuit
        double fan_speed { 0.075 }; // cooling fan rpm; either fraction of engine rpm, or absolute value if negative
        // heat exchange factors
        double kw { 0.35 };
        double kv { 0.6 };
        double kfe { 1.0 };
        double kfs { 80.0 };
        double kfo { 25.0 };
        double kfo2 { 25.0 };
        // system parts
        struct fluid_circuit_t {

            struct circuit_config_t {
                float temp_min { -1 }; // lowest accepted temperature
                float temp_max { -1 }; // highest accepted temperature
                float temp_cooling { -1 }; // active cooling activation point
                float temp_flow { -1 }; // fluid flow activation point
                bool shutters { false }; // the radiator has shutters to assist the cooling
            } config;
            bool is_cold { false }; // fluid is too cold
            bool is_warm { false }; // fluid is too hot
            bool is_hot { false }; // fluid temperature crossed cooling threshold
            bool is_flowing { false }; // fluid is being pushed through the circuit
        }   water,
            water_aux,
            oil;
        // output, state of affected devices
        bool PA { false }; // malfunction flag
        float rpmw { 0.0 }; // current main circuit fan revolutions
        float rpmwz { 0.0 }; // desired main circuit fan revolutions
        bool zaluzje1 { false };
        float rpmw2 { 0.0 }; // current auxiliary circuit fan revolutions
        float rpmwz2 { 0.0 }; // desired auxiliary circuit fan revolutions
        bool zaluzje2 { false };
        // output, temperatures
        float Te { 15.0 }; // ambient temperature TODO: get it from environment data
        // NOTE: by default the engine is initialized in warm, startup-ready state
        float Ts { 50.0 }; // engine temperature
        float To { 45.0 }; // oil temperature
        float Tsr { 50.0 }; // main circuit radiator temperature (?)
        float Twy { 50.0 }; // main circuit water temperature
        float Tsr2 { 40.0 }; // secondary circuit radiator temperature (?)
        float Twy2 { 40.0 }; // secondary circuit water temperature
        float temperatura1 { 50.0 };
        float temperatura2 { 40.0 };
    };

public:

	double dMoveLen = 0.0;
	std::string filename;
	/*---opis lokomotywy, wagonu itp*/
	/*--opis serii--*/
	int CategoryFlag = 1;       /*1 - pociag, 2 - samochod, 4 - statek, 8 - samolot*/
							/*--sekcja stalych typowych parametrow*/
	std::string TypeName;         /*nazwa serii/typu*/
								  //TrainType: string;       {typ: EZT/elektrowoz - Winger 040304}
	int TrainType = 0; /*Ra: powinno być szybciej niż string*/
	TEngineType EngineType = TEngineType::None;               /*typ napedu*/
	TPowerParameters EnginePowerSource;    /*zrodlo mocy dla silnikow*/
	TPowerParameters SystemPowerSource;    /*zrodlo mocy dla systemow sterowania/przetwornic/sprezarek*/
	TPowerParameters HeatingPowerSource;   /*zrodlo mocy dla ogrzewania*/
	TPowerParameters AlterHeatPowerSource; /*alternatywne zrodlo mocy dla ogrzewania*/
	TPowerParameters LightPowerSource;     /*zrodlo mocy dla oswietlenia*/
	TPowerParameters AlterLightPowerSource;/*alternatywne mocy dla oswietlenia*/
    double Vmax = -1.0;
    double Mass = 0.0;
    double Power = 0.0;    /*max. predkosc kontrukcyjna, masa wlasna, moc*/
	double Mred = 0.0;    /*Ra: zredukowane masy wirujące; potrzebne do obliczeń hamowania*/
	double TotalMass = 0.0; /*wyliczane przez ComputeMass*/
	double HeatingPower = 0.0;
    double EngineHeatingRPM { 0.0 }; // guaranteed engine revolutions with heating enabled
    double LightPower = 0.0; /*moc pobierana na ogrzewanie/oswietlenie*/
	double BatteryVoltage = 0.0;        /*Winger - baterie w elektrykach*/
	bool Battery = false; /*Czy sa zalavzone baterie*/
	bool EpFuse = true; /*Czy sa zalavzone baterie*/
	bool Signalling = false;         /*Czy jest zalaczona sygnalizacja hamowania ostatniego wagonu*/
	bool Radio = true;         /*Czy jest zalaczony radiotelefon*/
	float NominalBatteryVoltage = 0.f;        /*Winger - baterie w elektrykach*/
	TDimension Dim;          /*wymiary*/
	double Cx = 0.0;                 /*wsp. op. aerodyn.*/
	float Floor = 0.96f;              //poziom podłogi dla ładunków
	float WheelDiameter = 1.f;     /*srednica kol napednych*/
	float WheelDiameterL = 0.9f;    //Ra: srednica kol tocznych przednich
	float WheelDiameterT = 0.9f;    //Ra: srednica kol tocznych tylnych
	float TrackW = 1.435f;             /*nominalna szerokosc toru [m]*/
	double AxleInertialMoment = 0.0; /*moment bezwladnosci zestawu kolowego*/
	std::string AxleArangement;  /*uklad osi np. Bo'Bo' albo 1'C*/
	int NPoweredAxles = 0;     /*ilosc osi napednych liczona z powyzszego*/
	int NAxles = 0;            /*ilosc wszystkich osi j.w.*/
	int BearingType = 1;        /*lozyska: 0 - slizgowe, 1 - toczne*/
	double ADist = 0.0; double BDist = 0.0;       /*odlegosc osi oraz czopow skretu*/
									  /*hamulce:*/
	int NBpA = 0;              /*ilosc el. ciernych na os: 0 1 2 lub 4*/
	int SandCapacity = 0;    /*zasobnik piasku [kg]*/
	TBrakeSystem BrakeSystem = TBrakeSystem::Individual;/*rodzaj hamulca zespolonego*/
	TBrakeSubSystem BrakeSubsystem = TBrakeSubSystem::ss_None ;
	TBrakeValve BrakeValve = TBrakeValve::NoValve;
	TBrakeHandle BrakeHandle = TBrakeHandle::NoHandle;
	TBrakeHandle BrakeLocHandle = TBrakeHandle::NoHandle;
	double MBPM = 1.0; /*masa najwiekszego cisnienia*/

	std::shared_ptr<TBrake> Hamulec;
	std::shared_ptr<TDriverHandle> Handle;
	std::shared_ptr<TDriverHandle> LocHandle;
	std::shared_ptr<TReservoir> Pipe;
	std::shared_ptr<TReservoir> Pipe2;

	TLocalBrake LocalBrake = TLocalBrake::NoBrake;  /*rodzaj hamulca indywidualnego*/
	TBrakePressureTable BrakePressureTable; /*wyszczegolnienie cisnien w rurze*/
	TBrakePressure BrakePressureActual; //wartości ważone dla aktualnej pozycji kranu
	int ASBType = 0;            /*0: brak hamulca przeciwposlizgowego, 1: reczny, 2: automat*/
	int TurboTest = 0;
	double MaxBrakeForce = 0.0;      /*maksymalna sila nacisku hamulca*/
	double MaxBrakePress[5]; //pomocniczy, proz, sred, lad, pp
	double P2FTrans = 0.0;
	double TrackBrakeForce = 0.0;    /*sila nacisku hamulca szynowego*/
	int BrakeMethod = 0;        /*flaga rodzaju hamulca*/
							/*max. cisnienie w cyl. ham., stala proporcjonalnosci p-K*/
	double HighPipePress = 0.0;
    double LowPipePress = 0.0;
    double DeltaPipePress = 0.0;
	/*max. i min. robocze cisnienie w przewodzie glownym oraz roznica miedzy nimi*/
	double CntrlPipePress = 0.0; //ciśnienie z zbiorniku sterującym
	double BrakeVolume = 0.0;
    double BrakeVVolume = 0.0;
    double VeselVolume = 0.0;
	/*pojemnosc powietrza w ukladzie hamulcowym, w ukladzie glownej sprezarki [m^3] */
	int BrakeCylNo = 0; /*ilosc cylindrow ham.*/
	double BrakeCylRadius = 0.0;
    double BrakeCylDist = 0.0;
	double BrakeCylMult[3];
	int LoadFlag = 0;
	/*promien cylindra, skok cylindra, przekladnia hamulcowa*/
	double BrakeCylSpring = 0.0; /*suma nacisku sprezyn powrotnych, kN*/
	double BrakeSlckAdj = 0.0; /*opor nastawiacza skoku tloka, kN*/
	double BrakeRigEff = 0.0; /*sprawnosc przekladni dzwigniowej*/
	double RapidMult = 1.0; /*przelozenie rapidu*/
	double RapidVel = 55.0; /*szybkosc przelaczania rapidu*/
	int BrakeValveSize = 0;
	std::string BrakeValveParams;
	double Spg = 0.0;
	double MinCompressor = 0.0;
    double MaxCompressor = 0.0;
    double CompressorSpeed = 0.0;
	/*cisnienie wlaczania, zalaczania sprezarki, wydajnosc sprezarki*/
	TBrakeDelayTable BrakeDelay; /*opoznienie hamowania/odhamowania t/o*/
    double AirLeakRate{ 0.01 }; // base rate of air leak from brake system components ( 0.001 = 1 l/sec )
	int BrakeCtrlPosNo = 0;     /*ilosc pozycji hamulca*/
							/*nastawniki:*/
	int MainCtrlPosNo = 0;     /*ilosc pozycji nastawnika*/
	int ScndCtrlPosNo = 0;
	int LightsPosNo = 0;
    int LightsDefPos = 1;
	bool LightsWrap = false;
	int Lights[2][17]; // pozycje świateł, przód - tył, 1 .. 16
    int ScndInMain{ 0 };     /*zaleznosc bocznika od nastawnika*/
	bool MBrake = false;     /*Czy jest hamulec reczny*/
	double StopBrakeDecc = 0.0;
	TSecuritySystem SecuritySystem;

	/*-sekcja parametrow dla lokomotywy elektrycznej*/
	TSchemeTable RList;     /*lista rezystorow rozruchowych i polaczen silnikow, dla dizla: napelnienia*/
	int RlistSize = 0;
	TMotorParameters MotorParam[MotorParametersArraySize + 1];
	/*rozne parametry silnika przy bocznikowaniach*/
	/*dla lokomotywy spalinowej z przekladnia mechaniczna: przelozenia biegow*/
	TTransmision Transmision;
	// record   {liczba zebow przekladni}
	//  NToothM, NToothW : byte;
	//  Ratio: real;  {NToothW/NToothM}
	// end;
	double NominalVoltage = 0.0;   /*nominalne napiecie silnika*/
	double WindingRes = 0.0;
	double u = 0.0; //wspolczynnik tarcia yB wywalic!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	double CircuitRes = 0.0;      /*rezystancje silnika i obwodu*/
	int IminLo = 0; int IminHi = 0; /*prady przelacznika automatycznego rozruchu, uzywane tez przez ai_driver*/
	int ImaxLo = 0; // maksymalny prad niskiego rozruchu
    int ImaxHi = 0; // maksymalny prad wysokiego rozruchu
	double nmax = 0.0;             /*maksymalna dop. ilosc obrotow /s*/
	double InitialCtrlDelay = 0.0; double CtrlDelay = 0.0;        /* -//-  -//- miedzy kolejnymi poz.*/
	double CtrlDownDelay = 0.0;    /* -//-  -//- przy schodzeniu z poz.*/ /*hunter-101012*/
	int FastSerialCircuit = 0;/*0 - po kolei zamyka styczniki az do osiagniecia szeregowej, 1 - natychmiastowe wejscie na szeregowa*/ /*hunter-111012*/
	int AutoRelayType = 0;    /*0 -brak, 1 - jest, 2 - opcja*/
	bool CoupledCtrl = false;   /*czy mainctrl i scndctrl sa sprzezone*/
						//CouplerNr: TCouplerNr;  {ABu: nr sprzegu podlaczonego w drugim obiekcie}
	bool IsCoupled = false;     /*czy jest sprzezony ale jedzie z tylu*/
	int DynamicBrakeType = 0; /*patrz dbrake_**/
	int DynamicBrakeAmpmeters = 2; /*liczba amperomierzy przy hamowaniu ED*/
	double DynamicBrakeRes = 5.8; /*rezystancja oporników przy hamowaniu ED*/
	double TUHEX_Sum = 750; /*nastawa sterownika hamowania ED*/
	double TUHEX_Diff = 10; /*histereza działania sterownika hamowania ED*/
	double TUHEX_MinIw = 60; /*minimalny prąd wzbudzenia przy hamowaniu ED - wynika z chk silnika*/
	double TUHEX_MaxIw = 400; /*maksymalny prąd wzbudzenia przy hamowaniu ED - ograniczenie max siły*/

	int RVentType = 0;        /*0 - brak, 1 - jest, 2 - automatycznie wlaczany*/
	double RVentnmax = 1.0;      /*maks. obroty wentylatorow oporow rozruchowych*/
	double RVentCutOff = 0.0;      /*rezystancja wylaczania wentylatorow dla RVentType=2*/
    double RVentSpeed { 0.5 }; //rozpedzanie sie wentylatora obr/s^2}
    double RVentMinI { 50.0 }; //przy jakim pradzie sie wylaczaja}
	int CompressorPower = 1; // 0: main circuit, 1: z przetwornicy, reczne, 2: w przetwornicy, stale, 3: diesel engine, 4: converter of unit in front, 5: converter of unit behind
	int SmallCompressorPower = 0; /*Winger ZROBIC*/
	bool Trafo = false;      /*pojazd wyposażony w transformator*/

					 /*-sekcja parametrow dla lokomotywy spalinowej z przekladnia mechaniczna*/
	double dizel_Mmax = 1.0;
    double dizel_nMmax = 1.0;
    double dizel_Mnmax = 2.0;
    double dizel_nmax = 2.0;
    double dizel_nominalfill = 0.0;
	/*parametry aproksymacji silnika spalinowego*/
	double dizel_Mstand = 0.0; /*moment oporow ruchu silnika dla enrot=0*/
						 /*                dizel_auto_min, dizel_auto_max: real; {predkosc obrotowa przelaczania automatycznej skrzyni biegow*/
	double dizel_nmax_cutoff = 0.0; /*predkosc obrotowa zadzialania ogranicznika predkosci*/
	double dizel_nmin = 0.0; /*najmniejsza dopuszczalna predkosc obrotowa*/
	double dizel_minVelfullengage = 0.0; /*najmniejsza predkosc przy jezdzie ze sprzeglem bez poslizgu*/
	double dizel_AIM = 1.0; /*moment bezwladnosci walu itp*/
	double dizel_engageDia = 0.5; double dizel_engageMaxForce = 6000.0; double dizel_engagefriction = 0.5; /*parametry sprzegla*/
	double engagedownspeed = 0.9;
	double engageupspeed = 0.5;
	/*parametry przetwornika momentu*/
	bool hydro_TC = false; /*obecnosc hydraulicznego przetwornika momentu*/
	double hydro_TC_TMMax = 2.0; /*maksymalne wzmocnienie momentu*/
	double hydro_TC_CouplingPoint = 0.85; /*wzgledna predkosc zasprzeglenia*/
	double hydro_TC_LockupTorque = 3000.0; /*moment graniczny sprzegla blokujacego*/
	double hydro_TC_LockupRate = 1.0; /*szybkosc zalaczania sprzegla blokujacego*/
	double hydro_TC_UnlockRate = 1.0; /*szybkosc rozlaczania sprzegla blokujacego*/
	double hydro_TC_FillRateInc = 1.0; /*szybkosc napelniania sprzegla*/
	double hydro_TC_FillRateDec = 1.0; /*szybkosc oprozniania sprzegla*/
	double hydro_TC_TorqueInIn = 4.5; /*stala momentu proporcjonalnego do kwadratu obrotow wejsciowych*/
	double hydro_TC_TorqueInOut = 0.0; /*stala momentu proporcjonalnego do roznica obrotow wejsciowych i wyjsciowych*/
	double hydro_TC_TorqueOutOut = 0.0; /*stala momentu proporcjonalnego do kwadratu obrotow wyjsciowych*/
	double hydro_TC_LockupSpeed = 1.0; /*prog predkosci zalaczania sprzegla blokujacego*/
	double hydro_TC_UnlockSpeed = 1.0; /*prog predkosci rozlaczania sprzegla blokujacego*/
	/*parametry retardera*/
	bool hydro_R = false; /*obecnosc retardera*/
	int hydro_R_Placement = 0; /*umiejscowienie retardera: 0 - za skrzynia biegow, 1 - miedzy przetwornikiem a biegami, 2 - przed skrzynia biegow */
	double hydro_R_TorqueInIn = 1.0; /*stala momentu proporcjonalnego do kwadratu obrotow wejsciowych*/
	double hydro_R_MaxTorque = 1.0; /*maksymalny moment retardera*/
	double hydro_R_MaxPower = 1.0; /*maksymalna moc retardera - ogranicza moment*/
	double hydro_R_FillRateInc = 1.0; /*szybkosc napelniania sprzegla*/
	double hydro_R_FillRateDec = 1.0; /*szybkosc oprozniania sprzegla*/
	double hydro_R_MinVel = 1.0; /*minimalna predkosc, przy ktorej retarder dziala*/
    /*- dla lokomotyw spalinowo-elektrycznych -*/
	double AnPos = 0.0; // pozycja sterowania dokladnego (analogowego)
	bool AnalogCtrl = false; //
	bool AnMainCtrl = false; //
	bool ShuntModeAllow = false;
    bool ShuntMode = false;

	bool Flat = false;
	double Vhyp = 1.0;
	TDESchemeTable DElist;
	double Vadd = 1.0;
	TMPTRelayTable MPTRelay;
	int RelayType = 0;
	TShuntSchemeTable SST;
	double PowerCorRatio = 1.0; //Wspolczynnik korekcyjny
    /*- dla uproszczonego modelu silnika (dumb) oraz dla drezyny*/
	double Ftmax = 0.0;
	/*- dla lokomotyw z silnikami indukcyjnymi -*/
	double eimc[26];
	bool EIMCLogForce = false; // 
    static std::vector<std::string> const eimc_labels;
    double InverterFrequency { 0.0 }; // current frequency of power inverters
	/* -dla pojazdów z blendingiem EP/ED (MED) */
	double MED_Vmax = 0; // predkosc maksymalna dla obliczen chwilowej sily hamowania EP w MED
	double MED_Vmin = 0; // predkosc minimalna dla obliczen chwilowej sily hamowania EP w MED
	double MED_Vref = 0; // predkosc referencyjna dla obliczen dostepnej sily hamowania EP w MED
	double MED_amax = 9.81; // maksymalne opoznienie hamowania sluzbowego MED
	bool MED_EPVC = 0; // czy korekcja sily hamowania EP, gdy nie ma dostepnego ED
	double MED_EPVC_Time = 7; // czas korekcji sily hamowania EP, gdy nie ma dostepnego ED
	bool MED_Ncor = 0; // czy korekcja sily hamowania z uwzglednieniem nacisku
	/*-dla wagonow*/
    struct load_attributes {
        std::string name; // name of the cargo
        float offset_min { 0.f }; // offset applied to cargo model when load amount is 0

        load_attributes() = default;
        load_attributes( std::string const &Name, float const Offsetmin ) :
            name( Name ), offset_min( Offsetmin )
        {}
    };
    std::vector<load_attributes> LoadAttributes;
    float MaxLoad = 0.f;           /*masa w T lub ilosc w sztukach - ladownosc*/
    double OverLoadFactor = 0.0;       /*ile razy moze byc przekroczona ladownosc*/
    float LoadSpeed = 0.f; float UnLoadSpeed = 0.f;/*szybkosc na- i rozladunku jednostki/s*/
	int DoorOpenCtrl = 0; int DoorCloseCtrl = 0; /*0: przez pasazera, 1: przez maszyniste, 2: samoczynne (zamykanie)*/
	double DoorStayOpen = 0.0;               /*jak dlugo otwarte w przypadku DoorCloseCtrl=2*/
	bool DoorClosureWarning = false;      /*czy jest ostrzeganie przed zamknieciem*/
    bool DoorClosureWarningAuto = false; // departure signal plays automatically while door closing button is held down
	double DoorOpenSpeed = 1.0; double DoorCloseSpeed = 1.0;      /*predkosc otwierania i zamykania w j.u. */
	double DoorMaxShiftL = 0.5; double DoorMaxShiftR = 0.5; double DoorMaxPlugShift = 0.1;/*szerokosc otwarcia lub kat*/
	int DoorOpenMethod = 2;             /*sposob otwarcia - 1: przesuwne, 2: obrotowe, 3: trójelementowe*/
    float DoorCloseDelay { 0.f }; // delay (in seconds) before the door begin closing, once conditions to close are met
	double PlatformSpeed = 0.5;   /*szybkosc stopnia*/
    double PlatformMaxShift { 0.0 }; /*wysuniecie stopnia*/
    int PlatformOpenMethod { 2 }; /*sposob animacji stopnia*/
    double MirrorMaxShift { 90.0 };
	bool ScndS = false; /*Czy jest bocznikowanie na szeregowej*/
	double SpeedCtrlDelay = 2; /*opoznienie dzialania tempomatu z wybieralna predkoscia*/
    /*--sekcja zmiennych*/
    /*--opis konkretnego egzemplarza taboru*/
    TLocation Loc { 0.0, 0.0, 0.0 }; //pozycja pojazdów do wyznaczenia odległości pomiędzy sprzęgami
    TRotation Rot { 0.0, 0.0, 0.0 };
	std::string Name;                       /*nazwa wlasna*/
	TCoupling Couplers[2];  //urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami
#ifdef EU07_USE_OLD_HVCOUPLERS
    double HVCouplers[ 2 ][ 2 ]; //przewod WN
    enum hvcoupler {
        current = 0,
        voltage
    };
#endif
	bool EventFlag = false;                 /*!o true jesli cos nietypowego sie wydarzy*/
	int SoundFlag = 0;                    /*!o patrz stale sound_ */
	double DistCounter = 0.0;                  /*! licznik kilometrow */
	double V = 0.0;    //predkosc w [m/s] względem sprzęgów (dodania gdy jedzie w stronę 0)
	double Vel = 0.0;  //moduł prędkości w [km/h], używany przez AI
	double AccS = 0.0; //efektywne przyspieszenie styczne w [m/s^2] (wszystkie siły)
    double AccSVBased {}; // tangential acceleration calculated from velocity change
	double AccN = 0.0; // przyspieszenie normalne w [m/s^2]
	double AccVert = 0.0; // vertical acceleration
	double nrot = 0.0;
	double WheelFlat = 0.0;
    bool TruckHunting { true }; // enable/disable truck hunting calculation
	/*! rotacja kol [obr/s]*/
	double EnginePower = 0.0;                  /*! chwilowa moc silnikow*/
	double dL = 0.0; double Fb = 0.0; double Ff = 0.0;                  /*przesuniecie, sila hamowania i tarcia*/
	double FTrain = 0.0; double FStand = 0.0;                /*! sila pociagowa i oporow ruchu*/
	double FTotal = 0.0;                       /*! calkowita sila dzialajaca na pojazd*/
	double UnitBrakeForce = 0.0;               /*!s siła hamowania przypadająca na jeden element*/
	double Ntotal = 0.0;               /*!s siła nacisku klockow*/
	bool SlippingWheels = false; bool SandDose = false;   /*! poslizg kol, sypanie piasku*/
	double Sand = 0.0;                         /*ilosc piasku*/
	double BrakeSlippingTimer = 0.0;            /*pomocnicza zmienna do wylaczania przeciwposlizgu*/
	double dpBrake = 0.0; double dpPipe = 0.0; double dpMainValve = 0.0; double dpLocalValve = 0.0;
	/*! przyrosty cisnienia w kroku czasowym*/
	double ScndPipePress = 0.0;                /*cisnienie w przewodzie zasilajacym*/
	double BrakePress = 0.0;                    /*!o cisnienie w cylindrach hamulcowych*/
	double LocBrakePress = 0.0;                 /*!o cisnienie w cylindrach hamulcowych z pomocniczego*/
	double PipeBrakePress = 0.0;                /*!o cisnienie w cylindrach hamulcowych z przewodu*/
	double PipePress = 0.0;                    /*!o cisnienie w przewodzie glownym*/
	double EqvtPipePress = 0.0;                /*!o cisnienie w przewodzie glownym skladu*/
	double Volume = 0.0;                       /*objetosc spr. powietrza w zbiorniku hamulca*/
	double CompressedVolume = 0.0;              /*objetosc spr. powietrza w ukl. zasilania*/
	double PantVolume = 0.48;                    /*objetosc spr. powietrza w ukl. pantografu*/ // aby podniesione pantografy opadły w krótkim czasie przy wyłączonej sprężarce
	double Compressor = 0.0;                   /*! cisnienie w ukladzie zasilajacym*/
	bool CompressorFlag = false;             /*!o czy wlaczona sprezarka*/
	bool PantCompFlag = false;             /*!o czy wlaczona sprezarka pantografow*/
	bool CompressorAllow = false;            /*! zezwolenie na uruchomienie sprezarki  NBMX*/
    bool CompressorAllowLocal{ true }; // local device state override (most units don't have this fitted so it's set to true not to intefere)
    bool CompressorGovernorLock{ false }; // indicates whether compressor pressure switch was activated due to reaching cut-out pressure
    start_t CompressorStart{ start_t::manual }; // whether the compressor is started manually, or another way
    // TODO converter parameters, for when we start cleaning up mover parameters
    start_t ConverterStart{ start_t::manual }; // whether converter is started manually, or by other means
    float ConverterStartDelay{ 0.0f }; // delay (in seconds) before the converter is started, once its activation conditions are met
    double ConverterStartDelayTimer{ 0.0 }; // helper, for tracking whether converter start delay passed
	bool ConverterAllow = false;             /*zezwolenie na prace przetwornicy NBMX*/
    bool ConverterAllowLocal{ true }; // local device state override (most units don't have this fitted so it's set to true not to intefere)
    bool ConverterFlag = false;              /*!  czy wlaczona przetwornica NBMX*/
    fuel_pump FuelPump;
    oil_pump OilPump;
    water_pump WaterPump;
    water_heater WaterHeater;
    bool WaterCircuitsLink { false }; // optional connection between water circuits
    heat_data dizel_heat;
    std::array<cooling_fan, 2> MotorBlowers;

    int BrakeCtrlPos = -2;               /*nastawa hamulca zespolonego*/
	double BrakeCtrlPosR = 0.0;                 /*nastawa hamulca zespolonego - plynna dla FV4a*/
	double BrakeCtrlPos2 = 0.0;                 /*nastawa hamulca zespolonego - kapturek dla FV4a*/
	int ManualBrakePos = 0;                 /*nastawa hamulca recznego*/
	double LocalBrakePosA = 0.0;   /*nastawa hamulca pomocniczego*/
	double LocalBrakePosAEIM = 0.0;  /*pozycja hamulca pomocniczego ep dla asynchronicznych ezt*/
/*
	int BrakeStatus = b_off; //0 - odham, 1 - ham., 2 - uszk., 4 - odluzniacz, 8 - antyposlizg, 16 - uzyte EP, 32 - pozycja R, 64 - powrot z R
*/
    bool AlarmChainFlag = false;    // manual emergency brake
	bool RadioStopFlag = false;        /*hamowanie nagle*/
	int BrakeDelayFlag = 0;               /*nastawa opoznienia ham. osob/towar/posp/exp 0/1/2/4*/
	int BrakeDelays = 0;                   /*nastawy mozliwe do uzyskania*/
	int BrakeOpModeFlag = 0;               /*nastawa trybu pracy PS/PN/EP/MED 1/2/4/8*/
	int BrakeOpModes = 0;                   /*nastawy mozliwe do uzyskania*/
	bool DynamicBrakeFlag = false;          /*czy wlaczony hamulec elektrodymiczny*/
									//                NapUdWsp: integer;
	double LimPipePress = 0.0;                 /*stabilizator cisnienia*/
	double ActFlowSpeed = 0.0;                 /*szybkosc stabilizatora*/


	int DamageFlag = 0;  //kombinacja bitowa stalych dtrain_* }
	int EngDmgFlag = 0;  //kombinacja bitowa stalych usterek}
	int DerailReason = 0; //przyczyna wykolejenia

					  //EndSignalsFlag: byte;  {ABu 060205: zmiany - koncowki: 1/16 - swiatla prz/tyl, 2/31 - blachy prz/tyl}
					  //HeadSignalsFlag: byte; {ABu 060205: zmiany - swiatla: 1/2/4 - przod, 16/32/63 - tyl}
	TCommand CommandIn;
	/*komenda przekazywana przez PutCommand*/
	/*i wykonywana przez RunInternalCommand*/
	std::string CommandOut;        /*komenda przekazywana przez ExternalCommand*/
	std::string CommandLast; //Ra: ostatnio wykonana komenda do podglądu
	double ValueOut = 0.0;            /*argument komendy która ma byc przekazana na zewnatrz*/

	TTrackShape RunningShape;/*geometria toru po ktorym jedzie pojazd*/
	TTrackParam RunningTrack;/*parametry toru po ktorym jedzie pojazd*/
	double OffsetTrackH = 0.0; double OffsetTrackV = 0.0;  /*przesuniecie poz. i pion. w/m osi toru*/

    /*-zmienne dla lokomotyw*/
	bool Mains = false;    /*polozenie glownego wylacznika*/
	int MainCtrlPos = 0; /*polozenie glownego nastawnika*/
	int ScndCtrlPos = 0; /*polozenie dodatkowego nastawnika*/
	int LightsPos = 0;
	int ActiveDir = 0; //czy lok. jest wlaczona i w ktorym kierunku:
				   //względem wybranej kabiny: -1 - do tylu, +1 - do przodu, 0 - wylaczona
	int CabNo = 0; //numer kabiny, z której jest sterowanie: 1 lub -1; w przeciwnym razie brak sterowania - rozrzad
	int DirAbsolute = 0; //zadany kierunek jazdy względem sprzęgów (1=w strone 0,-1=w stronę 1)
	int ActiveCab = 0; //numer kabiny, w ktorej jest obsada (zwykle jedna na skład)
	double LastSwitchingTime = 0.0; /*czas ostatniego przelaczania czegos*/
    int WarningSignal = 0; // 0: nie trabi, 1,2,4: trabi
	bool DepartureSignal = false; /*sygnal odjazdu*/
	bool InsideConsist = false;
	/*-zmienne dla lokomotywy elektrycznej*/
	TTractionParam RunningTraction;/*parametry sieci trakcyjnej najblizej lokomotywy*/
	double enrot = 0.0; // ilosc obrotow silnika
    double Im = 0.0; // prad silnika
/*
    // currently not used
    double IHeating = 0.0;
    double ITraction = 0.0;
*/
    double Itot = 0.0; // prad calkowity
    double TotalCurrent = 0.0;
    // momenty
    double Mm = 0.0;
    double Mw = 0.0;
    // sily napedne
    double Fw = 0.0;
    double Ft = 0.0;
	//Ra: Im jest ujemny, jeśli lok jedzie w stronę sprzęgu 1
	//a ujemne powinien być przy odwróconej polaryzacji sieci...
	//w wielu miejscach jest używane abs(Im)
	int Imin = 0; int Imax = 0;      /*prad przelaczania automatycznego rozruchu, prad bezpiecznika*/
	double Voltage = 0.0;           /*aktualne napiecie sieci zasilajacej*/
	int MainCtrlActualPos = 0; /*wskaznik RList*/
	int ScndCtrlActualPos = 0; /*wskaznik MotorParam*/
	bool DelayCtrlFlag = false;  //czy czekanie na 1. pozycji na załączenie?
	double LastRelayTime = 0.0;     /*czas ostatniego przelaczania stycznikow*/
	bool AutoRelayFlag = false;  /*mozna zmieniac jesli AutoRelayType=2*/
	bool FuseFlag = false;       /*!o bezpiecznik nadmiarowy*/
	bool ConvOvldFlag = false;              /*!  nadmiarowy przetwornicy i ogrzewania*/
	bool StLinFlag = false;       /*!o styczniki liniowe*/
    bool StLinSwitchOff{ false }; // state of the button forcing motor connectors open
	bool ResistorsFlag = false;  /*!o jazda rezystorowa*/
	double RventRot = 0.0;          /*!s obroty wentylatorow rozruchowych*/
    bool UnBrake = false;       /*w EZT - nacisniete odhamowywanie*/
	double PantPress = 0.0; /*Cisnienie w zbiornikach pantografow*/
    bool PantPressSwitchActive{ false }; // state of the pantograph pressure switch. gets primed at defined pressure level in pantograph air system
    bool PantPressLockActive{ false }; // pwr system state flag. fires when pressure switch activates by pantograph pressure dropping below defined level
    bool NoVoltRelay{ true }; // switches off if the power level drops below threshold
    bool OvervoltageRelay{ true }; // switches off if the power level goes above threshold
    bool s_CAtestebrake = false; //hunter-091012: zmienna dla testu ca

    /*-zmienne dla lokomotywy spalinowej z przekladnia mechaniczna*/
	double dizel_fill = 0.0; /*napelnienie*/
	double dizel_engagestate = 0.0; /*sprzeglo skrzyni biegow: 0 - luz, 1 - wlaczone, 0.5 - wlaczone 50% (z poslizgiem)*/
	double dizel_engage = 0.0; /*sprzeglo skrzyni biegow: aktualny docisk*/
	double dizel_automaticgearstatus = 0.0; /*0 - bez zmiany, -1 zmiana na nizszy +1 zmiana na wyzszy*/
    bool dizel_startup { false }; // engine startup procedure request indicator
    bool dizel_ignition { false }; // engine ignition request indicator
    bool dizel_spinup { false }; // engine spin up to idle speed flag
	double dizel_engagedeltaomega = 0.0;    /*roznica predkosci katowych tarcz sprzegla*/
	double dizel_n_old = 0.0; /*poredkosc na potrzeby obliczen sprzegiel*/
	double dizel_Torque = 0.0; /*poredkosc na potrzeby obliczen sprzegiel*/

	/* - zmienne dla przetowrnika momentu */
	double hydro_TC_Fill = 0.0; /*napelnienie*/
	bool hydro_TC_Lockup = false; /*zapiecie sprzegla*/
	double hydro_TC_TorqueIn = 0.0; /*moment*/
	double hydro_TC_TorqueOut = 0.0; /*moment*/
	double hydro_TC_TMRatio = 1.0; /*aktualne przelozenie momentu*/
	double hydro_TC_Request = 0.0; /*zadanie sily*/
	double hydro_TC_nIn = 0.0; /*predkosc obrotowa walu wejsciowego*/
	double hydro_TC_nOut = 0.0; /*predkosc obrotowa walu wyjsciowego*/

	/* - zmienne dla przetowrnika momentu */
	double hydro_R_Fill = 0.0; /*napelnienie*/
	double hydro_R_Torque = 0.0; /*moment*/
	double hydro_R_Request = 0.0; /*zadanie sily hamowania*/
	double hydro_R_n = 0.0; /*predkosc obrotowa retardera*/

    /*- zmienne dla lokomotyw z silnikami indukcyjnymi -*/
	double eimv[21];
    static std::vector<std::string> const eimv_labels;
	double SpeedCtrlTimer = 0; /*zegar dzialania tempomatu z wybieralna predkoscia*/
	double NewSpeed = 0; /*nowa predkosc do zadania*/
	double MED_EPVC_CurrentTime = 0; /*aktualny czas licznika czasu korekcji siły EP*/

	/*-zmienne dla drezyny*/
	double PulseForce = 0.0;        /*przylozona sila*/
	double PulseForceTimer = 0.0;
	int PulseForceCount = 0;

	/*dla drezyny, silnika spalinowego i parowego*/
	double eAngle = M_PI * 0.5;

	/*-dla wagonow*/
    float LoadAmount = 0.f;      /*masa w T lub ilosc w sztukach - zaladowane*/
    load_attributes LoadType;
    std::string LoadQuantity; // jednostki miary
    int LoadStatus = 0; //+1=trwa rozladunek,+2=trwa zaladunek,+4=zakończono,0=zaktualizowany model
	double LastLoadChangeTime = 0.0; //raz (roz)ładowania

	bool DoorBlocked = false;    //Czy jest blokada drzwi
    bool DoorLockEnabled { true };
	bool DoorLeftOpened = false;  //stan drzwi
    double DoorLeftOpenTimer { -1.0 }; // left door closing timer for automatic door type
	bool DoorRightOpened = false;
    double DoorRightOpenTimer{ -1.0 }; // right door closing timer for automatic door type
    bool PantFrontUp = false;  //stan patykow 'Winger 160204
	bool PantRearUp = false;
	bool PantFrontSP = true;  //dzwiek patykow 'Winger 010304
    bool PantRearSP = true;
	int PantFrontStart = 0;  //stan patykow 'Winger 160204
	int PantRearStart = 0;
	double PantFrontVolt = 0.0;   //pantograf pod napieciem? 'Winger 160404
	double PantRearVolt = 0.0;
    // TODO: move these switch types where they belong, cabin definition
	std::string PantSwitchType;
	std::string ConvSwitchType;
    std::string StLinSwitchType;

	bool Heating = false; //ogrzewanie 'Winger 020304
	int DoubleTr = 1; //trakcja ukrotniona - przedni pojazd 'Winger 160304

	bool PhysicActivation = true;

	/*ABu: stale dla wyznaczania sil (i nie tylko) po optymalizacji*/
	double FrictConst1 = 0.0;
	double FrictConst2s = 0.0;
	double FrictConst2d= 0.0;
	double TotalMassxg = 0.0; /*TotalMass*g*/

	Math3D::vector3 vCoulpler[2]; // powtórzenie współrzędnych sprzęgów z DynObj :/
	double fBrakeCtrlPos = -2.0; // płynna nastawa hamulca zespolonego
	bool bPantKurek3 = true; // kurek trójdrogowy (pantografu): true=połączenie z ZG, false=połączenie z małą sprężarką // domyślnie zbiornik pantografu połączony jest ze zbiornikiem głównym
	int iProblem = 0; // flagi problemów z taborem, aby AI nie musiało porównywać; 0=może jechać
	int iLights[2]; // bity zapalonych świateł tutaj, żeby dało się liczyć pobór prądu
private:
	double CouplerDist(int Coupler);

public:
	TMoverParameters(double VelInitial, std::string TypeNameInit, std::string NameInit, int Cab);
	// obsługa sprzęgów
	double Distance(const TLocation &Loc1, const TLocation &Loc2, const TDimension &Dim1, const TDimension &Dim2);
/*	double Distance(const vector3 &Loc1, const vector3 &Loc2, const vector3 &Dim1, const vector3 &Dim2);
*/	//bool AttachA(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo, int CouplingType, bool Forced = false);
	bool Attach(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo, int CouplingType, bool Forced = false, bool Audible = true);
	int DettachStatus(int ConnectNo);
	bool Dettach(int ConnectNo);
	void SetCoupleDist();
	bool DirectionForward();
	void BrakeLevelSet(double b);
	bool BrakeLevelAdd(double b);
	bool IncBrakeLevel(); // wersja na użytek AI
	bool DecBrakeLevel();
	bool ChangeCab(int direction);
	bool CurrentSwitch(bool const State);
	void UpdateBatteryVoltage(double dt);
	double ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track, TTractionParam &ElectricTraction, const TLocation &NewLoc, TRotation &NewRot); //oblicza przesuniecie pojazdu
	double FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track, const TLocation &NewLoc, TRotation &NewRot); //oblicza przesuniecie pojazdu - wersja zoptymalizowana
    void compute_movement_( double const Deltatime );
	double ShowEngineRotation(int VehN);

	// Q *******************************************************************************************
	double GetTrainsetVoltage(void);
	bool Physic_ReActivation(void);
	double LocalBrakeRatio(void);
	double ManualBrakeRatio(void);
	double PipeRatio(void);/*ile napelniac*/
	double RealPipeRatio(void);/*jak szybko*/
	double BrakeVP(void) const;

	/*! przesylanie komend sterujacych*/
	bool SendCtrlBroadcast(std::string CtrlCommand, double ctrlvalue);
	bool SendCtrlToNext(std::string const CtrlCommand, double const ctrlvalue, double const dir, int const Couplertype = ctrain_controll);
    bool SetInternalCommand( std::string NewCommand, double NewValue1, double NewValue2, int const Couplertype = ctrain_controll );
	double GetExternalCommand(std::string &Command);
    bool RunCommand( std::string Command, double CValue1, double CValue2, int const Couplertype = ctrain_controll );
    bool RunInternalCommand();
	void PutCommand(std::string NewCommand, double NewValue1, double NewValue2, const TLocation &NewLocation);
	bool CabActivisation(void);
	bool CabDeactivisation(void);

	/*! funkcje zwiekszajace/zmniejszajace nastawniki*/
	/*! glowny nastawnik:*/
	bool IncMainCtrl(int CtrlSpeed);
	bool DecMainCtrl(int CtrlSpeed);
	/*! pomocniczy nastawnik:*/
	bool IncScndCtrl(int CtrlSpeed);
	bool DecScndCtrl(int CtrlSpeed);
	int GetVirtualScndPos();

	bool AddPulseForce(int Multipler);/*dla drezyny*/

    bool Sandbox( bool const State, range_t const Notify = range_t::consist );/*wlacza/wylacza sypanie piasku*/

						  /*! zbijanie czuwaka/SHP*/
	void SSReset(void);
	bool SecuritySystemReset(void);
	void SecuritySystemCheck(double dt);

	bool BatterySwitch(bool State);
	bool EpFuseSwitch(bool State);

	/*! stopnie hamowania - hamulec zasadniczy*/
	bool IncBrakeLevelOld(void);
	bool DecBrakeLevelOld(void);
	bool IncLocalBrakeLevel(float const CtrlSpeed);
	bool DecLocalBrakeLevel(float const CtrlSpeed);
	/*! ABu 010205: - skrajne polozenia ham. pomocniczego*/
	bool IncManualBrakeLevel(int CtrlSpeed);
	bool DecManualBrakeLevel(int CtrlSpeed);
	bool DynamicBrakeSwitch(bool Switch);
	bool RadiostopSwitch(bool Switch);
    bool AlarmChainSwitch( bool const State );
	bool AntiSlippingBrake(void);
	bool BrakeReleaser(int state);
	bool SwitchEPBrake(int state);
	bool AntiSlippingButton(void); /*! reczny wlacznik urzadzen antyposlizgowych*/

								   /*funkcje dla ukladow pneumatycznych*/
	bool IncBrakePress(double &brake, double PressLimit, double dp);
	bool DecBrakePress(double &brake, double PressLimit, double dp);
	bool BrakeDelaySwitch(int BDS);/*! przelaczanie nastawy opoznienia*/
    bool IncBrakeMult(void);/*przelaczanie prozny/ladowny*/
	bool DecBrakeMult(void);
	/*pomocnicze funkcje dla ukladow pneumatycznych*/
	void UpdateBrakePressure(double dt);
	void UpdatePipePressure(double dt);
	void CompressorCheck(double dt);/*wlacza, wylacza kompresor, laduje zbiornik*/
	void UpdatePantVolume(double dt);             //Ra
	void UpdateScndPipePressure(double dt);
	double GetDVc(double dt);

	/*funkcje obliczajace sily*/
	void ComputeConstans(void);//ABu: wczesniejsze wyznaczenie stalych dla liczenia sil
	double ComputeMass(void);
	void ComputeTotalForce(double dt, double dt1, bool FullVer);
	double Adhesive(double staticfriction) const;
	double TractionForce(double dt);
	double FrictionForce(double R, int TDamage);
	double BrakeForceR(double ratio, double velocity);
	double BrakeForceP(double press, double velocity);
	double BrakeForce(const TTrackParam &Track);
	double CouplerForce(int CouplerN, double dt);
	void CollisionDetect(int CouplerN, double dt);
	/*obrot kol uwzgledniajacy poslizg*/
	double ComputeRotatingWheel(double WForce, double dt, double n);

	/*--funkcje dla lokomotyw*/
	bool DirectionBackward(void);/*! kierunek ruchu*/
    bool WaterPumpBreakerSwitch( bool State, range_t const Notify = range_t::consist ); // water pump breaker state toggle
    bool WaterPumpSwitch( bool State, range_t const Notify = range_t::consist ); // water pump state toggle
    bool WaterPumpSwitchOff( bool State, range_t const Notify = range_t::consist ); // water pump state toggle
    bool WaterHeaterBreakerSwitch( bool State, range_t const Notify = range_t::consist ); // water heater breaker state toggle
    bool WaterHeaterSwitch( bool State, range_t const Notify = range_t::consist ); // water heater state toggle
    bool WaterCircuitsLinkSwitch( bool State, range_t const Notify = range_t::consist ); // water circuits link state toggle
    bool FuelPumpSwitch( bool State, range_t const Notify = range_t::consist ); // fuel pump state toggle
    bool FuelPumpSwitchOff( bool State, range_t const Notify = range_t::consist ); // fuel pump state toggle
    bool OilPumpSwitch( bool State, range_t const Notify = range_t::consist ); // oil pump state toggle
    bool OilPumpSwitchOff( bool State, range_t const Notify = range_t::consist ); // oil pump state toggle
    bool MotorBlowersSwitch( bool State, side const Side, range_t const Notify = range_t::consist ); // traction motor fan state toggle
    bool MotorBlowersSwitchOff( bool State, side const Side, range_t const Notify = range_t::consist ); // traction motor fan state toggle
    bool MainSwitch( bool const State, range_t const Notify = range_t::consist );/*! wylacznik glowny*/
    bool ConverterSwitch( bool State, range_t const Notify = range_t::consist );/*! wl/wyl przetwornicy*/
    bool CompressorSwitch( bool State, range_t const Notify = range_t::consist );/*! wl/wyl sprezarki*/

									  /*-funkcje typowe dla lokomotywy elektrycznej*/
	void ConverterCheck( double const Timestep ); // przetwornica
    void WaterPumpCheck( double const Timestep );
    void WaterHeaterCheck( double const Timestep );
    void FuelPumpCheck( double const Timestep );
    void OilPumpCheck( double const Timestep );
    void MotorBlowersCheck( double const Timestep );
    bool FuseOn(void); //bezpiecznik nadamiary
	bool FuseFlagCheck(void) const; // sprawdzanie flagi nadmiarowego
	void FuseOff(void); // wylaczenie nadmiarowego
    double ShowCurrent( int AmpN ) const; //pokazuje bezwgl. wartosc pradu na wybranym amperomierzu
	double ShowCurrentP(int AmpN) const;  //pokazuje bezwgl. wartosc pradu w wybranym pojezdzie                                                             //Q 20160722

								 /*!o pokazuje bezwgl. wartosc obrotow na obrotomierzu jednego z 3 pojazdow*/
								 /*function ShowEngineRotation(VehN:int): integer; //Ra 2014-06: przeniesione do C++*/
								 /*funkcje uzalezniajace sile pociagowa od predkosci: v2n, n2r, current, momentum*/
	double v2n(void);
	double Current(double n, double U);
	double Momentum(double I);
	double MomentumF(double I, double Iw, int SCP);

	bool CutOffEngine(void); //odlaczenie udszkodzonych silnikow
							 /*funkcje automatycznego rozruchu np EN57*/
	bool MaxCurrentSwitch(bool State); //przelacznik pradu wysokiego rozruchu
	bool MinCurrentSwitch(bool State); //przelacznik pradu automatycznego rozruchu
	bool AutoRelaySwitch(bool State); //przelacznik automatycznego rozruchu
	bool AutoRelayCheck();//symulacja automatycznego rozruchu
    bool MotorConnectorsCheck();

	bool ResistorsFlagCheck(void) const; //sprawdzenie kontrolki oporow rozruchowych NBMX
    bool PantFront( bool const State, range_t const Notify = range_t::consist ); //obsluga pantografou przedniego
    bool PantRear( bool const State, range_t const Notify = range_t::consist ); //obsluga pantografu tylnego

							   /*-funkcje typowe dla lokomotywy spalinowej z przekladnia mechaniczna*/
	bool dizel_EngageSwitch(double state);
	bool dizel_EngageChange(double dt);
	bool dizel_AutoGearCheck(void);
	double dizel_fillcheck(int mcp);
	double dizel_Momentum(double dizel_fill, double n, double dt);
    void dizel_HeatSet( float const Value );
    void dizel_Heat( double const dt );
    bool dizel_StartupCheck();
    bool dizel_Update(double dt);

	/* funckje dla wagonow*/
    bool AssignLoad( std::string const &Name, float const Amount = 0.f );
	bool LoadingDone(double LSpeed, std::string const &Loadname);
	bool DoorLeft(bool State, range_t const Notify = range_t::consist ); //obsluga drzwi lewych
	bool DoorRight(bool State, range_t const Notify = range_t::consist ); //obsluga drzwi prawych
	bool DoorBlockedFlag(void); //sprawdzenie blokady drzwi
    bool signal_departure( bool const State, range_t const Notify = range_t::consist ); // toggles departure warning
    void update_autonomous_doors( double const Deltatime ); // automatic door controller update

    /* funkcje dla samochodow*/
	bool ChangeOffsetH(double DeltaOffset);

	/*funkcje ladujace pliki opisujace pojazd*/
	bool LoadFIZ(std::string chkpath);                                                               //Q 20160717    bool LoadChkFile(std::string chkpath);
    bool CheckLocomotiveParameters( bool ReadyFlag, int Dir );
    std::string EngineDescription( int what ) const;
private:
    void LoadFIZ_Param( std::string const &line );
    void LoadFIZ_Load( std::string const &line );
    void LoadFIZ_Dimensions( std::string const &line );
    void LoadFIZ_Wheels( std::string const &line );
    void LoadFIZ_Brake( std::string const &line );
    void LoadFIZ_Doors( std::string const &line );
    void LoadFIZ_BuffCoupl( std::string const &line, int const Index );
    void LoadFIZ_TurboPos( std::string const &line );
    void LoadFIZ_Cntrl( std::string const &line );
	void LoadFIZ_Blending(std::string const &line);
    void LoadFIZ_Light( std::string const &line );
    void LoadFIZ_Security( std::string const &line );
    void LoadFIZ_Clima( std::string const &line );
    void LoadFIZ_Power( std::string const &Line );
    void LoadFIZ_Engine( std::string const &Input );
    void LoadFIZ_Switches( std::string const &Input );
    void LoadFIZ_MotorParamTable( std::string const &Input );
    void LoadFIZ_Circuit( std::string const &Input );
    void LoadFIZ_RList( std::string const &Input );
    void LoadFIZ_DList( std::string const &Input );
    void LoadFIZ_FFList( std::string const &Input );
    void LoadFIZ_LightsList( std::string const &Input );
    void LoadFIZ_PowerParamsDecode( TPowerParameters &Powerparameters, std::string const Prefix, std::string const &Input );
    TPowerType LoadFIZ_PowerDecode( std::string const &Power );
    TPowerSource LoadFIZ_SourceDecode( std::string const &Source );
    TEngineType LoadFIZ_EngineDecode( std::string const &Engine );
    bool readMPT0( std::string const &line );
    bool readMPT( std::string const &line );                                             //Q 20160717
    bool readMPTElectricSeries( std::string const &line );
    bool readMPTDieselElectric( std::string const &line );
    bool readMPTDieselEngine( std::string const &line );
	bool readBPT(/*int const ln,*/ std::string const &line);                                             //Q 20160721
    bool readRList( std::string const &Input );
    bool readDList( std::string const &line );
    bool readFFList( std::string const &line );
    bool readWWList( std::string const &line );
    bool readLightsList( std::string const &Input );
    void BrakeValveDecode( std::string const &s );                                                            //Q 20160719
	void BrakeSubsystemDecode();                                                                     //Q 20160719
};

//double Distance(TLocation Loc1, TLocation Loc2, TDimension Dim1, TDimension Dim2);
