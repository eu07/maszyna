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
using namespace Math3D;

const double Steel2Steel_friction = 0.15;      //tarcie statyczne
const double g = 9.81;                     //przyspieszenie ziemskie
const double SandSpeed = 0.1;              //ile kg/s}
const double RVentSpeed = 0.4;             //rozpedzanie sie wentylatora obr/s^2}
const double RVentMinI = 50.0;             //przy jakim pradzie sie wylaczaja}
const double Pirazy2 = 6.2831853071794f;
#define PI 3.1415926535897f

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
static int const sound_none = 0;
static int const sound_loud = 1;
static int const sound_couplerstretch = 2;
static int const sound_bufferclamp = 4;
static int const sound_bufferbump = 8;
static int const sound_relay = 16;
static int const sound_manyrelay = 32;
static int const sound_brakeacc = 64;

static bool  PhysicActivationFlag = false;

//szczególne typy pojazdów (inna obsługa) dla zmiennej TrainType
//zamienione na flagi bitowe, aby szybko wybierać grupę (np. EZT+SZT)
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
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
};
/*rotacja*/
struct TRotation
{
	double Rx = 0.0;
	double Ry = 0.0;
	double Rz = 0.0;
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
enum TBrakeSystem { Individual, Pneumatic, ElectroPneumatic };
/*podtypy hamulcow zespolonych*/
enum TBrakeSubSystem { ss_None, ss_W, ss_K, ss_KK, ss_Hik, ss_ESt, ss_KE, ss_LSt, ss_MT, ss_Dako };
enum TBrakeValve { NoValve, W, W_Lu_VI, W_Lu_L, W_Lu_XR, K, Kg, Kp, Kss, Kkg, Kkp, Kks, Hikg1, Hikss, Hikp1, KE, SW, EStED, NESt3, ESt3, LSt, ESt4, ESt3AL2, EP1, EP2, M483, CV1_L_TR, CV1, CV1_R, Other };
enum TBrakeHandle { NoHandle, West, FV4a, M394, M254, FVel1, FVel6, D2, Knorr, FD1, BS2, testH, St113, MHZ_P, MHZ_T, MHZ_EN57 };
/*typy hamulcow indywidualnych*/
enum TLocalBrake { NoBrake, ManualBrake, PneumaticBrake, HydraulicBrake };
/*dla osob/towar: opoznienie hamowania/odhamowania*/
typedef double TBrakeDelayTable[4];

struct TBrakePressure
{
	double PipePressureVal = 0.0;
	double BrakePressureVal = 0.0;
	double FlowSpeedVal = 0.0;
	TBrakeSystem BrakeType = Pneumatic;
};

typedef std::map<int,TBrakePressure> TBrakePressureTable;

/*typy napedow*/
enum TEngineTypes { None, Dumb, WheelsDriven, ElectricSeriesMotor, ElectricInductionMotor, DieselEngine, SteamEngine, DieselElectric };
/*postac dostarczanej energii*/
enum TPowerType { NoPower, BioPower, MechPower, ElectricPower, SteamPower };
/*rodzaj paliwa*/
enum TFuelType { Undefined, Coal, Oil };
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
	double OVP;       //czy jest przekaznik nadnapieciowy
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
enum TPowerSource { NotDefined, InternalSource, Transducer, Generator, Accumulator, CurrentCollector, PowerCable, Heater };


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
			TEngineTypes GeneratorEngine;

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
        SourceType = NotDefined;
        PowerType = NoPower;
        RPowerCable.PowerTrans = NoPower;
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
	int SystemType; /*0: brak, 1: czuwak aktywny, 2: SHP/sygnalizacja kabinowa*/
	double AwareDelay; // czas powtarzania czuwaka
	double AwareMinSpeed; // minimalna prędkość załączenia czuwaka, normalnie 10% Vmax
	double SoundSignalDelay;
	double EmergencyBrakeDelay;
	int Status; /*0: wylaczony, 1: wlaczony, 2: czuwak, 4: shp, 8: alarm, 16: hamowanie awaryjne*/
	double SystemTimer;
	double SystemSoundCATimer;
	double SystemSoundSHPTimer;
	double SystemBrakeCATimer;
	double SystemBrakeSHPTimer;
	double SystemBrakeCATestTimer; // hunter-091012
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

enum TCouplerType { NoCoupler, Articulated, Bare, Chain, Screw, Automatic };

struct TCoupling {
	/*parametry*/
    double SpringKB = 1.0;   /*stala sprezystosci zderzaka/sprzegu, %tlumiennosci */
    double SpringKC = 1.0;
    double beta = 0.0;
    double DmaxB = 0.1; /*tolerancja scisku/rozciagania, sila rozerwania*/
    double FmaxB = 1000.0;
    double DmaxC = 0.1;
    double FmaxC = 1000.0;
	TCouplerType CouplerType = NoCoupler;     /*typ sprzegu*/
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
};

class TMoverParameters
{ // Ra: wrapper na kod pascalowy, przejmujący jego funkcje  Q: 20160824 - juz nie wrapper a klasa bazowa :)
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
	TEngineTypes EngineType = None;               /*typ napedu*/
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
    double LightPower = 0.0; /*moc pobierana na ogrzewanie/oswietlenie*/
	double BatteryVoltage = 0.0;        /*Winger - baterie w elektrykach*/
	bool Battery = false; /*Czy sa zalavzone baterie*/
	bool EpFuse = true; /*Czy sa zalavzone baterie*/
	bool Signalling = false;         /*Czy jest zalaczona sygnalizacja hamowania ostatniego wagonu*/
	bool DoorSignalling = false;         /*Czy jest zalaczona sygnalizacja blokady drzwi*/
	bool Radio = true;         /*Czy jest zalaczony radiotelefon*/
	double NominalBatteryVoltage = 0.0;        /*Winger - baterie w elektrykach*/
	TDimension Dim;          /*wymiary*/
	double Cx = 0.0;                 /*wsp. op. aerodyn.*/
	double Floor = 0.96;              //poziom podłogi dla ładunków
	double WheelDiameter = 1.0;     /*srednica kol napednych*/
	double WheelDiameterL = 0.9;    //Ra: srednica kol tocznych przednich
	double WheelDiameterT = 0.9;    //Ra: srednica kol tocznych tylnych
	double TrackW = 1.435;             /*nominalna szerokosc toru [m]*/
	double AxleInertialMoment = 0.0; /*moment bezwladnosci zestawu kolowego*/
	std::string AxleArangement;  /*uklad osi np. Bo'Bo' albo 1'C*/
	int NPoweredAxles = 0;     /*ilosc osi napednych liczona z powyzszego*/
	int NAxles = 0;            /*ilosc wszystkich osi j.w.*/
	int BearingType = 1;        /*lozyska: 0 - slizgowe, 1 - toczne*/
	double ADist = 0.0; double BDist = 0.0;       /*odlegosc osi oraz czopow skretu*/
									  /*hamulce:*/
	int NBpA = 0;              /*ilosc el. ciernych na os: 0 1 2 lub 4*/
	int SandCapacity = 0;    /*zasobnik piasku [kg]*/
	TBrakeSystem BrakeSystem = Individual;/*rodzaj hamulca zespolonego*/
	TBrakeSubSystem BrakeSubsystem = ss_None ;
	TBrakeValve BrakeValve = NoValve;
	TBrakeHandle BrakeHandle = NoHandle;
	TBrakeHandle BrakeLocHandle = NoHandle;
	double MBPM = 1.0; /*masa najwiekszego cisnienia*/

	std::shared_ptr<TBrake> Hamulec;
	std::shared_ptr<TDriverHandle> Handle;
	std::shared_ptr<TDriverHandle> LocHandle;
	std::shared_ptr<TReservoir> Pipe;
	std::shared_ptr<TReservoir> Pipe2;

	TLocalBrake LocalBrake = NoBrake;  /*rodzaj hamulca indywidualnego*/
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
	int BrakeValveSize = 0;
	std::string BrakeValveParams;
	double Spg = 0.0;
	double MinCompressor = 0.0;
    double MaxCompressor = 0.0;
    double CompressorSpeed = 0.0;
	/*cisnienie wlaczania, zalaczania sprezarki, wydajnosc sprezarki*/
	TBrakeDelayTable BrakeDelay; /*opoznienie hamowania/odhamowania t/o*/
	int BrakeCtrlPosNo = 0;     /*ilosc pozycji hamulca*/
							/*nastawniki:*/
	int MainCtrlPosNo = 0;     /*ilosc pozycji nastawnika*/
	int ScndCtrlPosNo = 0;
	int LightsPosNo = 0; // NOTE: values higher than 0 seem to break the current code for light switches
    int LightsDefPos = 1;
	bool LightsWrap = false;
	int Lights[2][17]; // pozycje świateł, przód - tył, 1 .. 16
	bool ScndInMain = false;     /*zaleznosc bocznika od nastawnika*/
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
	int ImaxLo = 0; int ImaxHi = 0; /*maksymalny prad niskiego i wysokiego rozruchu*/
	double nmax = 0.0;             /*maksymalna dop. ilosc obrotow /s*/
	double InitialCtrlDelay = 0.0; double CtrlDelay = 0.0;        /* -//-  -//- miedzy kolejnymi poz.*/
	double CtrlDownDelay = 0.0;    /* -//-  -//- przy schodzeniu z poz.*/ /*hunter-101012*/
	int FastSerialCircuit = 0;/*0 - po kolei zamyka styczniki az do osiagniecia szeregowej, 1 - natychmiastowe wejscie na szeregowa*/ /*hunter-111012*/
	int AutoRelayType = 0;    /*0 -brak, 1 - jest, 2 - opcja*/
	bool CoupledCtrl = false;   /*czy mainctrl i scndctrl sa sprzezone*/
						//CouplerNr: TCouplerNr;  {ABu: nr sprzegu podlaczonego w drugim obiekcie}
	bool IsCoupled = false;     /*czy jest sprzezony ale jedzie z tylu*/
	int DynamicBrakeType = 0; /*patrz dbrake_**/
	int RVentType = 0;        /*0 - brak, 1 - jest, 2 - automatycznie wlaczany*/
	double RVentnmax = 1.0;      /*maks. obroty wentylatorow oporow rozruchowych*/
	double RVentCutOff = 0.0;      /*rezystancja wylaczania wentylatorow dla RVentType=2*/
	int CompressorPower = 1; /*0: bezp. z obwodow silnika, 1: z przetwornicy, reczne, 2: w przetwornicy, stale, 5: z silnikowego*/
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
	/*-dla wagonow*/
    double MaxLoad = 0.0;           /*masa w T lub ilosc w sztukach - ladownosc*/
	std::string LoadAccepted; std::string LoadQuantity; /*co moze byc zaladowane, jednostki miary*/
	double OverLoadFactor = 0.0;       /*ile razy moze byc przekroczona ladownosc*/
	double LoadSpeed = 0.0; double UnLoadSpeed = 0.0;/*szybkosc na- i rozladunku jednostki/s*/
	int DoorOpenCtrl = 0; int DoorCloseCtrl = 0; /*0: przez pasazera, 1: przez maszyniste, 2: samoczynne (zamykanie)*/
	double DoorStayOpen = 0.0;               /*jak dlugo otwarte w przypadku DoorCloseCtrl=2*/
	bool DoorClosureWarning = false;      /*czy jest ostrzeganie przed zamknieciem*/
	double DoorOpenSpeed = 1.0; double DoorCloseSpeed = 1.0;      /*predkosc otwierania i zamykania w j.u. */
	double DoorMaxShiftL = 0.5; double DoorMaxShiftR = 0.5; double DoorMaxPlugShift = 0.1;/*szerokosc otwarcia lub kat*/
	int DoorOpenMethod = 2;             /*sposob otwarcia - 1: przesuwne, 2: obrotowe, 3: trójelementowe*/
	double PlatformSpeed = 0.25;   /*szybkosc stopnia*/
	double PlatformMaxShift = 0.5; /*wysuniecie stopnia*/
	int PlatformOpenMethod = 1; /*sposob animacji stopnia*/
	bool ScndS = false; /*Czy jest bocznikowanie na szeregowej*/
    /*--sekcja zmiennych*/
    /*--opis konkretnego egzemplarza taboru*/
	TLocation Loc; //pozycja pojazdów do wyznaczenia odległości pomiędzy sprzęgami
	TRotation Rot;
	std::string Name;                       /*nazwa wlasna*/
	TCoupling Couplers[2];  //urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami
	double HVCouplers[2][2]; //przewod WN
	int ScanCounter = 0;   /*pomocnicze do skanowania sprzegow*/
	bool EventFlag = false;                 /*!o true jesli cos nietypowego sie wydarzy*/
	int SoundFlag = 0;                    /*!o patrz stale sound_ */
	double DistCounter = 0.0;                  /*! licznik kilometrow */
	double V = 0.0;    //predkosc w [m/s] względem sprzęgów (dodania gdy jedzie w stronę 0)
	double Vel = 0.0;  //moduł prędkości w [km/h], używany przez AI
	double AccS = 0.0; //efektywne przyspieszenie styczne w [m/s^2] (wszystkie siły)
	double AccN = 0.0; //przyspieszenie normalne w [m/s^2]
	double AccV = 0.0;
	double nrot = 0.0;
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
	bool ConverterFlag = false ;              /*!  czy wlaczona przetwornica NBMX*/
	bool ConverterAllow = false;             /*zezwolenie na prace przetwornicy NBMX*/
	int BrakeCtrlPos = -2;               /*nastawa hamulca zespolonego*/
	double BrakeCtrlPosR = 0.0;                 /*nastawa hamulca zespolonego - plynna dla FV4a*/
	double BrakeCtrlPos2 = 0.0;                 /*nastawa hamulca zespolonego - kapturek dla FV4a*/
	int LocalBrakePos = 0;                 /*nastawa hamulca indywidualnego*/
	int ManualBrakePos = 0;                 /*nastawa hamulca recznego*/
	double LocalBrakePosA = 0.0;
	int BrakeStatus = b_off; /*0 - odham, 1 - ham., 2 - uszk., 4 - odluzniacz, 8 - antyposlizg, 16 - uzyte EP, 32 - pozycja R, 64 - powrot z R*/
	bool EmergencyBrakeFlag = false;        /*hamowanie nagle*/
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
							  //WarningSignal: byte;     {0: nie trabi, 1,2: trabi}
	bool DepartureSignal = false; /*sygnal odjazdu*/
	bool InsideConsist = false;
	/*-zmienne dla lokomotywy elektrycznej*/
	TTractionParam RunningTraction;/*parametry sieci trakcyjnej najblizej lokomotywy*/
	double enrot = 0.0;
    double Im = 0.0;
    double Itot = 0.0;
    double IHeating = 0.0;
    double ITraction = 0.0;
    double TotalCurrent = 0.0;
    double Mm = 0.0;
    double Mw = 0.0;
    double Fw = 0.0;
    double Ft = 0.0;
	/*ilosc obrotow, prad silnika i calkowity, momenty, sily napedne*/
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
	bool ResistorsFlag = false;  /*!o jazda rezystorowa*/
	double RventRot = 0.0;          /*!s obroty wentylatorow rozruchowych*/
	bool UnBrake = false;       /*w EZT - nacisniete odhamowywanie*/
	double PantPress = 0.0; /*Cisnienie w zbiornikach pantografow*/
	bool s_CAtestebrake = false; //hunter-091012: zmienna dla testu ca

    /*-zmienne dla lokomotywy spalinowej z przekladnia mechaniczna*/
	double dizel_fill = 0.0; /*napelnienie*/
	double dizel_engagestate = 0.0; /*sprzeglo skrzyni biegow: 0 - luz, 1 - wlaczone, 0.5 - wlaczone 50% (z poslizgiem)*/
	double dizel_engage = 0.0; /*sprzeglo skrzyni biegow: aktualny docisk*/
	double dizel_automaticgearstatus = 0.0; /*0 - bez zmiany, -1 zmiana na nizszy +1 zmiana na wyzszy*/
	bool dizel_enginestart = false;      /*czy trwa rozruch silnika*/
	double dizel_engagedeltaomega = 0.0;    /*roznica predkosci katowych tarcz sprzegla*/

    /*- zmienne dla lokomotyw z silnikami indukcyjnymi -*/
	double eimv[21];

	/*-zmienne dla drezyny*/
	double PulseForce = 0.0;        /*przylozona sila*/
	double PulseForceTimer = 0.0;
	int PulseForceCount = 0;

	/*dla drezyny, silnika spalinowego i parowego*/
	double eAngle = 1.5;

	/*-dla wagonow*/
    double Load = 0.0;      /*masa w T lub ilosc w sztukach - zaladowane*/
	std::string LoadType;   /*co jest zaladowane*/
	int LoadStatus = 0; //+1=trwa rozladunek,+2=trwa zaladunek,+4=zakończono,0=zaktualizowany model
	double LastLoadChangeTime = 0.0; //raz (roz)ładowania

	bool DoorBlocked = false;    //Czy jest blokada drzwi
	bool DoorLeftOpened = false;  //stan drzwi
	bool DoorRightOpened = false;
	bool PantFrontUp = false;  //stan patykow 'Winger 160204
	bool PantRearUp = false;
	bool PantFrontSP = true;  //dzwiek patykow 'Winger 010304
    bool PantRearSP = true;
	int PantFrontStart = 0;  //stan patykow 'Winger 160204
	int PantRearStart = 0;
	double PantFrontVolt = 0.0;   //pantograf pod napieciem? 'Winger 160404
	double PantRearVolt = 0.0;
	std::string PantSwitchType;
	std::string ConvSwitchType;

	bool Heating = false; //ogrzewanie 'Winger 020304
	int DoubleTr = 1; //trakcja ukrotniona - przedni pojazd 'Winger 160304

	bool PhysicActivation = true;

	/*ABu: stale dla wyznaczania sil (i nie tylko) po optymalizacji*/
	double FrictConst1 = 0.0;
	double FrictConst2s = 0.0;
	double FrictConst2d= 0.0;
	double TotalMassxg = 0.0; /*TotalMass*g*/

	vector3 vCoulpler[2]; // powtórzenie współrzędnych sprzęgów z DynObj :/
	vector3 DimHalf; // połowy rozmiarów do obliczeń geometrycznych
					 // int WarningSignal; //0: nie trabi, 1,2: trabi syreną o podanym numerze
	int WarningSignal = 0; // tymczasowo 8bit, ze względu na funkcje w MTools
	double fBrakeCtrlPos = -2.0; // płynna nastawa hamulca zespolonego
	bool bPantKurek3 = true; // kurek trójdrogowy (pantografu): true=połączenie z ZG, false=połączenie z małą sprężarką // domyślnie zbiornik pantografu połączony jest ze zbiornikiem głównym
	int iProblem = 0; // flagi problemów z taborem, aby AI nie musiało porównywać; 0=może jechać
	int iLights[2]; // bity zapalonych świateł tutaj, żeby dało się liczyć pobór prądu
private:
	double CouplerDist(int Coupler);

public:
	TMoverParameters(double VelInitial, std::string TypeNameInit, std::string NameInit, int LoadInitial, std::string LoadTypeInitial, int Cab);
	// obsługa sprzęgów
	double Distance(const TLocation &Loc1, const TLocation &Loc2, const TDimension &Dim1, const TDimension &Dim2);
/*	double Distance(const vector3 &Loc1, const vector3 &Loc2, const vector3 &Dim1, const vector3 &Dim2);
*/	//bool AttachA(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo, int CouplingType, bool Forced = false);
	bool Attach(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo, int CouplingType, bool Forced = false);
	int DettachStatus(int ConnectNo);
	bool Dettach(int ConnectNo);
	void SetCoupleDist();
	bool DirectionForward();
	void BrakeLevelSet(double b);
	bool BrakeLevelAdd(double b);
	bool IncBrakeLevel(); // wersja na użytek AI
	bool DecBrakeLevel();
	bool ChangeCab(int direction);
	bool CurrentSwitch(int direction);
	void UpdateBatteryVoltage(double dt);
	double ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track, TTractionParam &ElectricTraction, const TLocation &NewLoc, TRotation &NewRot); //oblicza przesuniecie pojazdu
	double FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track, const TLocation &NewLoc, TRotation &NewRot); //oblicza przesuniecie pojazdu - wersja zoptymalizowana
	double ShowEngineRotation(int VehN);

	// Q *******************************************************************************************
	double GetTrainsetVoltage(void);
	bool Physic_ReActivation(void);
	double LocalBrakeRatio(void);
	double ManualBrakeRatio(void);
	double PipeRatio(void);/*ile napelniac*/
	double RealPipeRatio(void);/*jak szybko*/
	double BrakeVP(void);

	/*! przesylanie komend sterujacych*/
	bool SendCtrlBroadcast(std::string CtrlCommand, double ctrlvalue);
	bool SendCtrlToNext(std::string CtrlCommand, double ctrlvalue, double dir);
	bool SetInternalCommand(std::string NewCommand, double NewValue1, double NewValue2);
	double GetExternalCommand(std::string &Command);
	bool RunCommand(std::string Command, double CValue1, double CValue2);
	bool RunInternalCommand(void);
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


	bool AddPulseForce(int Multipler);/*dla drezyny*/

	bool SandDoseOn(void);/*wlacza/wylacza sypanie piasku*/

						  /*! zbijanie czuwaka/SHP*/
	void SSReset(void);
	bool SecuritySystemReset(void);
	void SecuritySystemCheck(double dt);

	bool BatterySwitch(bool State);
	bool EpFuseSwitch(bool State);

	/*! stopnie hamowania - hamulec zasadniczy*/
	bool IncBrakeLevelOld(void);
	bool DecBrakeLevelOld(void);
	bool IncLocalBrakeLevel(int CtrlSpeed);
	bool DecLocalBrakeLevel(int CtrlSpeed);
	/*! ABu 010205: - skrajne polozenia ham. pomocniczego*/
	bool IncLocalBrakeLevelFAST(void);
	bool DecLocalBrakeLevelFAST(void);
	bool IncManualBrakeLevel(int CtrlSpeed);
	bool DecManualBrakeLevel(int CtrlSpeed);
	bool DynamicBrakeSwitch(bool Switch);
	bool EmergencyBrakeSwitch(bool Switch);
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
	double Adhesive(double staticfriction);
	double TractionForce(double dt);
	double FrictionForce(double R, int TDamage);
	double BrakeForce(const TTrackParam &Track);
	double CouplerForce(int CouplerN, double dt);
	void CollisionDetect(int CouplerN, double dt);
	/*obrot kol uwzgledniajacy poslizg*/
	double ComputeRotatingWheel(double WForce, double dt, double n);

	/*--funkcje dla lokomotyw*/
	bool DirectionBackward(void);/*! kierunek ruchu*/
	bool MainSwitch(bool State);/*! wylacznik glowny*/
	bool ConverterSwitch(bool State);/*! wl/wyl przetwornicy*/
	bool CompressorSwitch(bool State);/*! wl/wyl sprezarki*/

									  /*-funkcje typowe dla lokomotywy elektrycznej*/
	void ConverterCheck(); // przetwornica
	bool FuseOn(void); //bezpiecznik nadamiary
	bool FuseFlagCheck(void); // sprawdzanie flagi nadmiarowego
	void FuseOff(void); // wylaczenie nadmiarowego
    double ShowCurrent( int AmpN ); //pokazuje bezwgl. wartosc pradu na wybranym amperomierzu
	double ShowCurrentP(int AmpN);  //pokazuje bezwgl. wartosc pradu w wybranym pojezdzie                                                             //Q 20160722

								 /*!o pokazuje bezwgl. wartosc obrotow na obrotomierzu jednego z 3 pojazdow*/
								 /*function ShowEngineRotation(VehN:int): integer; //Ra 2014-06: przeniesione do C++*/
								 /*funkcje uzalezniajace sile pociagowa od predkosci: v2n, n2r, current, momentum*/
	double v2n(void);
	double current(double n, double U);
	double Momentum(double I);
	double MomentumF(double I, double Iw, int SCP);

	bool CutOffEngine(void); //odlaczenie udszkodzonych silnikow
							 /*funkcje automatycznego rozruchu np EN57*/
	bool MaxCurrentSwitch(bool State); //przelacznik pradu wysokiego rozruchu
	bool MinCurrentSwitch(bool State); //przelacznik pradu automatycznego rozruchu
	bool AutoRelaySwitch(bool State); //przelacznik automatycznego rozruchu
	bool AutoRelayCheck(void);//symulacja automatycznego rozruchu

	bool ResistorsFlagCheck(void); //sprawdzenie kontrolki oporow rozruchowych NBMX
	bool PantFront(bool State); //obsluga pantografou przedniego
	bool PantRear(bool State); //obsluga pantografu tylnego

							   /*-funkcje typowe dla lokomotywy spalinowej z przekladnia mechaniczna*/
	bool dizel_EngageSwitch(double state);
	bool dizel_EngageChange(double dt);
	bool dizel_AutoGearCheck(void);
	double dizel_fillcheck(int mcp);
	double dizel_Momentum(double dizel_fill, double n, double dt);
	bool dizel_Update(double dt);

	/* funckje dla wagonow*/
	bool LoadingDone(double LSpeed, std::string LoadInit);
	bool DoorLeft(bool State); //obsluga drzwi lewych
	bool DoorRight(bool State); //obsluga drzwi prawych
	bool DoorBlockedFlag(void); //sprawdzenie blokady drzwi

								/* funkcje dla samochodow*/
	bool ChangeOffsetH(double DeltaOffset);

	/*funkcje ladujace pliki opisujace pojazd*/
	bool LoadFIZ(std::string chkpath);                                                               //Q 20160717    bool LoadChkFile(std::string chkpath);
    bool CheckLocomotiveParameters( bool ReadyFlag, int Dir );
    std::string EngineDescription( int what );
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
    TEngineTypes LoadFIZ_EngineDecode( std::string const &Engine );
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

extern double Distance(TLocation Loc1, TLocation Loc2, TDimension Dim1, TDimension Dim2);

template <typename _Type>
bool
extract_value( _Type &Variable, std::string const &Key, std::string const &Input, std::string const &Default ) {

    auto value = extract_value( Key, Input );
    if( false == value.empty() ) {
        // set the specified variable to retrieved value
        std::stringstream converter;
        converter << value;
        converter >> Variable;
        return true; // located the variable
    }
    else {
        // set the variable to provided default value
        if( false == Default.empty() ) {
            std::stringstream converter;
            converter << Default;
            converter >> Variable;
        }
        return false; // supplied the default
	}
}

inline
std::string
extract_value( std::string const &Key, std::string const &Input ) {

    std::string value;
    auto lookup = Input.find( Key + "=" );
    if( lookup != std::string::npos ) {
        value = Input.substr( Input.find_first_not_of( ' ', lookup + Key.size() + 1 ) );
        lookup = value.find( ' ' );
        if( lookup != std::string::npos ) {
            // trim everything past the value
            value.erase( lookup );
        }
    }
    return value;
}