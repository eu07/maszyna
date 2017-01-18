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
static int const ctrain_virtual = 0;        //gdy pojazdy na tym samym torze siê widz¹ wzajemnie
static int const ctrain_coupler = 1;        //sprzeg fizyczny
static int const ctrain_pneumatic = 2;      //przewody hamulcowe
static int const ctrain_controll = 4;       //przewody steruj¹ce (ukrotnienie)
static int const ctrain_power = 8;          //przewody zasilaj¹ce (WN)
static int const ctrain_passenger = 16;     //mostek przejœciowy
static int const ctrain_scndpneumatic = 32; //przewody 8 atm (¿ó³te; zasilanie powietrzem)
static int const ctrain_heating = 64;       //przewody ogrzewania WN
static int const ctrain_depot = 128;        //nie roz³¹czalny podczas zwyk³ych manewrów (miêdzycz³onowy), we wpisie wartoœæ ujemna

											/*typ hamulca elektrodynamicznego*/
static int const dbrake_none = 0;
static int const dbrake_passive = 1;
static int const dbrake_switch = 2;
static int const dbrake_reversal = 4;
static int const dbrake_automatic = 8;

/*status czuwaka/SHP*/
//hunter-091012: rozdzielenie alarmow, dodanie testu czuwaka
static int const s_waiting = 1; //dzia³a
static int const s_aware = 2;   //czuwak miga
static int const s_active = 4;  //SHP œwieci
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

//szczególne typy pojazdów (inna obs³uga) dla zmiennej TrainType
//zamienione na flagi bitowe, aby szybko wybieraæ grupê (np. EZT+SZT)
static int const dt_Default = 0;
static int const dt_EZT = 1;
static int const dt_ET41 = 2;
static int const dt_ET42 = 4;
static int const dt_PseudoDiesel = 8;
static int const dt_ET22 = 0x10; //u¿ywane od Megapacka
static int const dt_SN61 = 0x20; //nie u¿ywane w warunkach, ale ustawiane z CHK
static int const dt_EP05 = 0x40;
static int const dt_ET40 = 0x80;
static int const dt_181 = 0x100;

//sta³e dla asynchronów
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

enum TProblem // lista problemów taboru, które uniemo¿liwiaj¹ jazdê
{ // flagi bitowe
	pr_Hamuje = 1, // pojazd ma za³¹czony hamulec lub zatarte osie
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
	double Width;
	double friction;
	int CategoryFlag;
	int QualityFlag;
	int DamageFlag;
	double Velmax; /*dla uzytku maszynisty w ai_driver*/
    inline TTrackParam() {
        Width, friction, Velmax = 0.0;
        CategoryFlag, QualityFlag, DamageFlag = 0;
    }
};

struct TTractionParam
{
	double TractionVoltage; /*napiecie*/
	double TractionFreq; /*czestotliwosc*/
	double TractionMaxCurrent; /*obciazalnosc*/
	double TractionResistivity; /*rezystancja styku*/
    inline TTractionParam() {
        TractionVoltage, TractionFreq = 0.0;
        TractionMaxCurrent, TractionResistivity = 0.0;
    }
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
	double PipePressureVal;
	double BrakePressureVal;
	double FlowSpeedVal;
	TBrakeSystem BrakeType;
    inline TBrakePressure() {
        BrakeType = Pneumatic;
        PipePressureVal, BrakePressureVal, FlowSpeedVal = 0.0;
    }
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
	long CollectorsNo; //musi byæ tu, bo inaczej siê kopie
	double MinH; double MaxH; //zakres ruchu pantografu, nigdzie nie u¿ywany
	double CSW;       //szerokoœæ czêœci roboczej (styku) œlizgacza
	double MinV; double MaxV; //minimalne i maksymalne akceptowane napiêcie
	double OVP;       //czy jest przekaznik nadnapieciowy
	double InsetV;    //minimalne napiêcie wymagane do za³¹czenia
	double MinPress;  //minimalne ciœnienie do za³¹czenia WS
	double MaxPress;  //maksymalne ciœnienie za reduktorem
    //inline TCurrentCollector() {
    //    CollectorsNo = 0;
    //    MinH, MaxH, CSW, MinV, MaxV = 0.0;
    //    OVP, InsetV, MinPress, MaxPress = 0.0;
    //}
};
/*typy Ÿróde³ mocy*/
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

/*parametry Ÿróde³ mocy*/
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
	int Relay; /*numer pozycji rozruchu samoczynnego*/
	double R; /*opornik rozruchowy*/ /*dla dizla napelnienie*/
	int Bn;
	int Mn; /*ilosc galezi i silnikow w galezi*/ /*dla dizla Mn: czy luz czy nie*/
	bool AutoSwitch; /*czy dana pozycja nastawniana jest recznie czy autom.*/
	int ScndAct; /*jesli ma bocznik w nastawniku, to ktory bocznik na ktorej pozycji*/
    inline TScheme()
    {
        Relay = 0;
        R = 0.0;
        Bn = 0.0;
        Mn = 0.0;
        AutoSwitch = false;
        ScndAct = 0;
    }
};
typedef TScheme TSchemeTable[ResArraySize + 1]; /*tablica rezystorow rozr.*/
struct TDEScheme
{
	double RPM; /*obroty diesla*/
	double GenPower; /*moc maksymalna*/
	double Umax; /*napiecie maksymalne*/
	double Imax; /*prad maksymalny*/
};
typedef TDEScheme TDESchemeTable[33]; /*tablica rezystorow rozr.*/
struct TShuntScheme
{
	double Umin;
	double Umax;
	double Pmin;
	double Pmax;
};
typedef TShuntScheme TShuntSchemeTable[33];
struct TMPTRelay
{/*lista przekaznikow bocznikowania*/
	double Iup;
	double Idown;
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
	double AwareMinSpeed; // minimalna prêdkoœæ za³¹czenia czuwaka, normalnie 10% Vmax
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
	int NToothM;
	int NToothW;
	double Ratio;
    TTransmision() {
        NToothM, NToothW = 0;
        Ratio = 1.0;
    }
};

enum TCouplerType { NoCoupler, Articulated, Bare, Chain, Screw, Automatic };


class TMoverParameters;                          // wyforwardowanie klasy coby  typ byl widoczny w ponizszej strukturze
struct TCoupling {
	/*parametry*/
	double SpringKB; double SpringKC; double beta;   /*stala sprezystosci zderzaka/sprzegu, %tlumiennosci */
	double DmaxB; double FmaxB; double DmaxC; double FmaxC; /*tolerancja scisku/rozciagania, sila rozerwania*/
	TCouplerType CouplerType;     /*typ sprzegu*/
								  /*zmienne*/
	int CouplingFlag; /*0 - wirtualnie, 1 - sprzegi, 2 - pneumatycznie, 4 - sterowanie, 8 - kabel mocy*/
	int AllowedFlag;       //Ra: znaczenie jak wy¿ej, maska dostêpnych
	bool Render;             /*ABu: czy rysowac jak zaczepiony sprzeg*/
	double CoupleDist;            /*ABu: optymalizacja - liczenie odleglosci raz na klatkê, bez iteracji*/
	TMoverParameters* Connected; /*co jest podlaczone*/
	int ConnectedNr;           //Ra: od której strony pod³¹czony do (Connected): 0=przód, 1=ty³
	double CForce;                /*sila z jaka dzialal*/
	double Dist;                  /*strzalka ugiecia zderzaków*/
	bool CheckCollision;     /*czy sprawdzac sile czy pedy*/
};

class TMoverParameters
{ // Ra: wrapper na kod pascalowy, przejmuj¹cy jego funkcje  Q: 20160824 - juz nie wrapper a klasa bazowa :)
public:

	double dMoveLen;
	std::string filename;
	/*---opis lokomotywy, wagonu itp*/
	/*--opis serii--*/
	int CategoryFlag;       /*1 - pociag, 2 - samochod, 4 - statek, 8 - samolot*/
							/*--sekcja stalych typowych parametrow*/
	std::string TypeName;         /*nazwa serii/typu*/
								  //TrainType: string;       {typ: EZT/elektrowoz - Winger 040304}
	int TrainType; /*Ra: powinno byæ szybciej ni¿ string*/
	TEngineTypes EngineType;               /*typ napedu*/
	TPowerParameters EnginePowerSource;    /*zrodlo mocy dla silnikow*/
	TPowerParameters SystemPowerSource;    /*zrodlo mocy dla systemow sterowania/przetwornic/sprezarek*/
	TPowerParameters HeatingPowerSource;   /*zrodlo mocy dla ogrzewania*/
	TPowerParameters AlterHeatPowerSource; /*alternatywne zrodlo mocy dla ogrzewania*/
	TPowerParameters LightPowerSource;     /*zrodlo mocy dla oswietlenia*/
	TPowerParameters AlterLightPowerSource;/*alternatywne mocy dla oswietlenia*/
	double Vmax; double Mass; double Power;    /*max. predkosc kontrukcyjna, masa wlasna, moc*/
	double Mred;    /*Ra: zredukowane masy wiruj¹ce; potrzebne do obliczeñ hamowania*/
	double TotalMass; /*wyliczane przez ComputeMass*/
	double HeatingPower; double LightPower; /*moc pobierana na ogrzewanie/oswietlenie*/
	double BatteryVoltage;        /*Winger - baterie w elektrykach*/
	bool Battery; /*Czy sa zalavzone baterie*/
	bool EpFuse; /*Czy sa zalavzone baterie*/
	bool Signalling;         /*Czy jest zalaczona sygnalizacja hamowania ostatniego wagonu*/
	bool DoorSignalling;         /*Czy jest zalaczona sygnalizacja blokady drzwi*/
	bool Radio;         /*Czy jest zalaczony radiotelefon*/
	double NominalBatteryVoltage;        /*Winger - baterie w elektrykach*/
	TDimension Dim;          /*wymiary*/
	double Cx;                 /*wsp. op. aerodyn.*/
	double Floor;              //poziom pod³ogi dla ³adunków
	double WheelDiameter;     /*srednica kol napednych*/
	double WheelDiameterL;    //Ra: srednica kol tocznych przednich
	double WheelDiameterT;    //Ra: srednica kol tocznych tylnych
	double TrackW;             /*nominalna szerokosc toru [m]*/
	double AxleInertialMoment; /*moment bezwladnosci zestawu kolowego*/
	std::string AxleArangement;  /*uklad osi np. Bo'Bo' albo 1'C*/
	int NPoweredAxles;     /*ilosc osi napednych liczona z powyzszego*/
	int NAxles;            /*ilosc wszystkich osi j.w.*/
	int BearingType;        /*lozyska: 0 - slizgowe, 1 - toczne*/
	double ADist; double BDist;       /*odlegosc osi oraz czopow skretu*/
									  /*hamulce:*/
	int NBpA;              /*ilosc el. ciernych na os: 0 1 2 lub 4*/
	int SandCapacity;    /*zasobnik piasku [kg]*/
	TBrakeSystem BrakeSystem;/*rodzaj hamulca zespolonego*/
	TBrakeSubSystem BrakeSubsystem;
	TBrakeValve BrakeValve;
	TBrakeHandle BrakeHandle;
	TBrakeHandle BrakeLocHandle;
	double MBPM; /*masa najwiekszego cisnienia*/

	std::shared_ptr<TBrake> Hamulec;
	std::shared_ptr<TDriverHandle> Handle;
	std::shared_ptr<TDriverHandle> LocHandle;
	std::shared_ptr<TReservoir> Pipe;
	std::shared_ptr<TReservoir> Pipe2;

	TLocalBrake LocalBrake;  /*rodzaj hamulca indywidualnego*/
	TBrakePressureTable BrakePressureTable; /*wyszczegolnienie cisnien w rurze*/
	TBrakePressure BrakePressureActual; //wartoœci wa¿one dla aktualnej pozycji kranu
	int ASBType;            /*0: brak hamulca przeciwposlizgowego, 1: reczny, 2: automat*/
	int TurboTest;
	double MaxBrakeForce;      /*maksymalna sila nacisku hamulca*/
	double MaxBrakePress[5]; //pomocniczy, proz, sred, lad, pp
	double P2FTrans;
	double TrackBrakeForce;    /*sila nacisku hamulca szynowego*/
	int BrakeMethod;        /*flaga rodzaju hamulca*/
							/*max. cisnienie w cyl. ham., stala proporcjonalnosci p-K*/
	double HighPipePress; double LowPipePress; double DeltaPipePress;
	/*max. i min. robocze cisnienie w przewodzie glownym oraz roznica miedzy nimi*/
	double CntrlPipePress; //ciœnienie z zbiorniku steruj¹cym
	double BrakeVolume; double BrakeVVolume; double VeselVolume;
	/*pojemnosc powietrza w ukladzie hamulcowym, w ukladzie glownej sprezarki [m^3] */
	int BrakeCylNo; /*ilosc cylindrow ham.*/
	double BrakeCylRadius; double BrakeCylDist;
	double BrakeCylMult[3];
	int LoadFlag;
	/*promien cylindra, skok cylindra, przekladnia hamulcowa*/
	double BrakeCylSpring; /*suma nacisku sprezyn powrotnych, kN*/
	double BrakeSlckAdj; /*opor nastawiacza skoku tloka, kN*/
	double BrakeRigEff; /*sprawnosc przekladni dzwigniowej*/
	double RapidMult; /*przelozenie rapidu*/
	int BrakeValveSize;
	std::string BrakeValveParams;
	double Spg;
	double MinCompressor; double MaxCompressor; double CompressorSpeed;
	/*cisnienie wlaczania, zalaczania sprezarki, wydajnosc sprezarki*/
	TBrakeDelayTable BrakeDelay; /*opoznienie hamowania/odhamowania t/o*/
	int BrakeCtrlPosNo;     /*ilosc pozycji hamulca*/
							/*nastawniki:*/
	int MainCtrlPosNo;     /*ilosc pozycji nastawnika*/
	int ScndCtrlPosNo;
	int LightsPosNo; int LightsDefPos;
	bool LightsWrap;
	int Lights[2][17]; // pozycje œwiate³, przód - ty³, 1 .. 16
	bool ScndInMain;     /*zaleznosc bocznika od nastawnika*/
	bool MBrake;     /*Czy jest hamulec reczny*/
	double StopBrakeDecc;
	TSecuritySystem SecuritySystem;

	/*-sekcja parametrow dla lokomotywy elektrycznej*/
	TSchemeTable RList;     /*lista rezystorow rozruchowych i polaczen silnikow, dla dizla: napelnienia*/
	int RlistSize;
	TMotorParameters MotorParam[MotorParametersArraySize + 1];
	/*rozne parametry silnika przy bocznikowaniach*/
	/*dla lokomotywy spalinowej z przekladnia mechaniczna: przelozenia biegow*/
	TTransmision Transmision;
	// record   {liczba zebow przekladni}
	//  NToothM, NToothW : byte;
	//  Ratio: real;  {NToothW/NToothM}
	// end;
	double NominalVoltage;   /*nominalne napiecie silnika*/
	double WindingRes;
	double u; //wspolczynnik tarcia yB wywalic!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	double CircuitRes;      /*rezystancje silnika i obwodu*/
	int IminLo; int IminHi; /*prady przelacznika automatycznego rozruchu, uzywane tez przez ai_driver*/
	int ImaxLo; int ImaxHi; /*maksymalny prad niskiego i wysokiego rozruchu*/
	double nmax;             /*maksymalna dop. ilosc obrotow /s*/
	double InitialCtrlDelay; double CtrlDelay;        /* -//-  -//- miedzy kolejnymi poz.*/
	double CtrlDownDelay;    /* -//-  -//- przy schodzeniu z poz.*/ /*hunter-101012*/
	int FastSerialCircuit;/*0 - po kolei zamyka styczniki az do osiagniecia szeregowej, 1 - natychmiastowe wejscie na szeregowa*/ /*hunter-111012*/
	int AutoRelayType;    /*0 -brak, 1 - jest, 2 - opcja*/
	bool CoupledCtrl;   /*czy mainctrl i scndctrl sa sprzezone*/
						//CouplerNr: TCouplerNr;  {ABu: nr sprzegu podlaczonego w drugim obiekcie}
	bool IsCoupled;     /*czy jest sprzezony ale jedzie z tylu*/
	int DynamicBrakeType; /*patrz dbrake_**/
	int RVentType;        /*0 - brak, 1 - jest, 2 - automatycznie wlaczany*/
	double RVentnmax;      /*maks. obroty wentylatorow oporow rozruchowych*/
	double RVentCutOff;      /*rezystancja wylaczania wentylatorow dla RVentType=2*/
	int CompressorPower; /*0: bezp. z obwodow silnika, 1: z przetwornicy, reczne, 2: w przetwornicy, stale, 5: z silnikowego*/
	int SmallCompressorPower; /*Winger ZROBIC*/
	bool Trafo;      /*pojazd wyposa¿ony w transformator*/

					 /*-sekcja parametrow dla lokomotywy spalinowej z przekladnia mechaniczna*/
	double dizel_Mmax; double dizel_nMmax; double dizel_Mnmax; double dizel_nmax; double dizel_nominalfill;
	/*parametry aproksymacji silnika spalinowego*/
	double dizel_Mstand; /*moment oporow ruchu silnika dla enrot=0*/
						 /*                dizel_auto_min, dizel_auto_max: real; {predkosc obrotowa przelaczania automatycznej skrzyni biegow*/
	double dizel_nmax_cutoff; /*predkosc obrotowa zadzialania ogranicznika predkosci*/
	double dizel_nmin; /*najmniejsza dopuszczalna predkosc obrotowa*/
	double dizel_minVelfullengage; /*najmniejsza predkosc przy jezdzie ze sprzeglem bez poslizgu*/
	double dizel_AIM; /*moment bezwladnosci walu itp*/
	double dizel_engageDia; double dizel_engageMaxForce; double dizel_engagefriction; /*parametry sprzegla*/

																					  /*- dla lokomotyw spalinowo-elektrycznych -*/
	double AnPos; // pozycja sterowania dokladnego (analogowego)
	bool AnalogCtrl; //
	bool AnMainCtrl; //
	bool ShuntModeAllow;
	bool ShuntMode;

	bool Flat;
	double Vhyp;
	TDESchemeTable DElist;
	double Vadd;
	TMPTRelayTable MPTRelay;
	int RelayType;
	TShuntSchemeTable SST;
	double PowerCorRatio; //Wspolczynnik korekcyjny


						  /*- dla uproszczonego modelu silnika (dumb) oraz dla drezyny*/
	double Ftmax;

	/*- dla lokomotyw z silnikami indukcyjnymi -*/
	double eimc[26];

	/*-dla wagonow*/
	long MaxLoad;           /*masa w T lub ilosc w sztukach - ladownosc*/
	std::string LoadAccepted; std::string LoadQuantity; /*co moze byc zaladowane, jednostki miary*/
	double OverLoadFactor;       /*ile razy moze byc przekroczona ladownosc*/
	double LoadSpeed; double UnLoadSpeed;/*szybkosc na- i rozladunku jednostki/s*/
	int DoorOpenCtrl; int DoorCloseCtrl; /*0: przez pasazera, 1: przez maszyniste, 2: samoczynne (zamykanie)*/
	double DoorStayOpen;               /*jak dlugo otwarte w przypadku DoorCloseCtrl=2*/
	bool DoorClosureWarning;      /*czy jest ostrzeganie przed zamknieciem*/
	double DoorOpenSpeed; double DoorCloseSpeed;      /*predkosc otwierania i zamykania w j.u. */
	double DoorMaxShiftL; double DoorMaxShiftR; double DoorMaxPlugShift;/*szerokosc otwarcia lub kat*/
	int DoorOpenMethod;             /*sposob otwarcia - 1: przesuwne, 2: obrotowe, 3: trójelementowe*/
	double PlatformSpeed;   /*szybkosc stopnia*/
	double PlatformMaxShift; /*wysuniecie stopnia*/
	int PlatformOpenMethod; /*sposob animacji stopnia*/
	bool ScndS; /*Czy jest bocznikowanie na szeregowej*/

				/*--sekcja zmiennych*/
				/*--opis konkretnego egzemplarza taboru*/
	TLocation Loc; //pozycja pojazdów do wyznaczenia odleg³oœci pomiêdzy sprzêgami
	TRotation Rot;
	std::string Name;                       /*nazwa wlasna*/
	TCoupling Couplers[2];  //urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami
	double HVCouplers[2][2]; //przewod WN
	int ScanCounter;   /*pomocnicze do skanowania sprzegow*/
	bool EventFlag;                 /*!o true jesli cos nietypowego sie wydarzy*/
	int SoundFlag;                    /*!o patrz stale sound_ */
	double DistCounter;                  /*! licznik kilometrow */
	double V;    //predkosc w [m/s] wzglêdem sprzêgów (dodania gdy jedzie w stronê 0)
	double Vel;  //modu³ prêdkoœci w [km/h], u¿ywany przez AI
	double AccS; //efektywne przyspieszenie styczne w [m/s^2] (wszystkie si³y)
	double AccN; //przyspieszenie normalne w [m/s^2]
	double AccV;
	double nrot;
	/*! rotacja kol [obr/s]*/
	double EnginePower;                  /*! chwilowa moc silnikow*/
	double dL; double Fb; double Ff;                  /*przesuniecie, sila hamowania i tarcia*/
	double FTrain; double FStand;                /*! sila pociagowa i oporow ruchu*/
	double FTotal;                       /*! calkowita sila dzialajaca na pojazd*/
	double UnitBrakeForce;               /*!s si³a hamowania przypadaj¹ca na jeden element*/
	double Ntotal;               /*!s si³a nacisku klockow*/
	bool SlippingWheels; bool SandDose;   /*! poslizg kol, sypanie piasku*/
	double Sand;                         /*ilosc piasku*/
	double BrakeSlippingTimer;            /*pomocnicza zmienna do wylaczania przeciwposlizgu*/
	double dpBrake; double dpPipe; double dpMainValve; double dpLocalValve;
	/*! przyrosty cisnienia w kroku czasowym*/
	double ScndPipePress;                /*cisnienie w przewodzie zasilajacym*/
	double BrakePress;                    /*!o cisnienie w cylindrach hamulcowych*/
	double LocBrakePress;                 /*!o cisnienie w cylindrach hamulcowych z pomocniczego*/
	double PipeBrakePress;                /*!o cisnienie w cylindrach hamulcowych z przewodu*/
	double PipePress;                    /*!o cisnienie w przewodzie glownym*/
	double EqvtPipePress;                /*!o cisnienie w przewodzie glownym skladu*/
	double Volume;                       /*objetosc spr. powietrza w zbiorniku hamulca*/
	double CompressedVolume;              /*objetosc spr. powietrza w ukl. zasilania*/
	double PantVolume;                    /*objetosc spr. powietrza w ukl. pantografu*/
	double Compressor;                   /*! cisnienie w ukladzie zasilajacym*/
	bool CompressorFlag;             /*!o czy wlaczona sprezarka*/
	bool PantCompFlag;             /*!o czy wlaczona sprezarka pantografow*/
	bool CompressorAllow;            /*! zezwolenie na uruchomienie sprezarki  NBMX*/
	bool ConverterFlag;              /*!  czy wlaczona przetwornica NBMX*/
	bool ConverterAllow;             /*zezwolenie na prace przetwornicy NBMX*/
	int BrakeCtrlPos;               /*nastawa hamulca zespolonego*/
	double BrakeCtrlPosR;                 /*nastawa hamulca zespolonego - plynna dla FV4a*/
	double BrakeCtrlPos2;                 /*nastawa hamulca zespolonego - kapturek dla FV4a*/
	int LocalBrakePos;                 /*nastawa hamulca indywidualnego*/
	int ManualBrakePos;                 /*nastawa hamulca recznego*/
	double LocalBrakePosA;
	int BrakeStatus; /*0 - odham, 1 - ham., 2 - uszk., 4 - odluzniacz, 8 - antyposlizg, 16 - uzyte EP, 32 - pozycja R, 64 - powrot z R*/
	bool EmergencyBrakeFlag;        /*hamowanie nagle*/
	int BrakeDelayFlag;               /*nastawa opoznienia ham. osob/towar/posp/exp 0/1/2/4*/
	int BrakeDelays;                   /*nastawy mozliwe do uzyskania*/
	int BrakeOpModeFlag;               /*nastawa trybu pracy PS/PN/EP/MED 1/2/4/8*/
	int BrakeOpModes;                   /*nastawy mozliwe do uzyskania*/
	bool DynamicBrakeFlag;          /*czy wlaczony hamulec elektrodymiczny*/
									//                NapUdWsp: integer;
	double LimPipePress;                 /*stabilizator cisnienia*/
	double ActFlowSpeed;                 /*szybkosc stabilizatora*/


	int DamageFlag;  //kombinacja bitowa stalych dtrain_* }
	int EngDmgFlag;  //kombinacja bitowa stalych usterek}
	int DerailReason; //przyczyna wykolejenia

					  //EndSignalsFlag: byte;  {ABu 060205: zmiany - koncowki: 1/16 - swiatla prz/tyl, 2/31 - blachy prz/tyl}
					  //HeadSignalsFlag: byte; {ABu 060205: zmiany - swiatla: 1/2/4 - przod, 16/32/63 - tyl}
	TCommand CommandIn;
	/*komenda przekazywana przez PutCommand*/
	/*i wykonywana przez RunInternalCommand*/
	std::string CommandOut;        /*komenda przekazywana przez ExternalCommand*/
	std::string CommandLast; //Ra: ostatnio wykonana komenda do podgl¹du
	double ValueOut;            /*argument komendy która ma byc przekazana na zewnatrz*/

	TTrackShape RunningShape;/*geometria toru po ktorym jedzie pojazd*/
	TTrackParam RunningTrack;/*parametry toru po ktorym jedzie pojazd*/
	double OffsetTrackH; double OffsetTrackV;  /*przesuniecie poz. i pion. w/m osi toru*/

											   /*-zmienne dla lokomotyw*/
	bool Mains;    /*polozenie glownego wylacznika*/
	int MainCtrlPos; /*polozenie glownego nastawnika*/
	int ScndCtrlPos; /*polozenie dodatkowego nastawnika*/
	int LightsPos;
	int ActiveDir; //czy lok. jest wlaczona i w ktorym kierunku:
				   //wzglêdem wybranej kabiny: -1 - do tylu, +1 - do przodu, 0 - wylaczona
	int CabNo; //numer kabiny, z której jest sterowanie: 1 lub -1; w przeciwnym razie brak sterowania - rozrzad
	int DirAbsolute; //zadany kierunek jazdy wzglêdem sprzêgów (1=w strone 0,-1=w stronê 1)
	int ActiveCab; //numer kabiny, w ktorej jest obsada (zwykle jedna na sk³ad)
	double LastSwitchingTime; /*czas ostatniego przelaczania czegos*/
							  //WarningSignal: byte;     {0: nie trabi, 1,2: trabi}
	bool DepartureSignal; /*sygnal odjazdu*/
	bool InsideConsist;
	/*-zmienne dla lokomotywy elektrycznej*/
	TTractionParam RunningTraction;/*parametry sieci trakcyjnej najblizej lokomotywy*/
	double enrot; double Im; double Itot; double IHeating; double ITraction; double TotalCurrent; double Mm; double Mw; double Fw; double Ft;
	/*ilosc obrotow, prad silnika i calkowity, momenty, sily napedne*/
	//Ra: Im jest ujemny, jeœli lok jedzie w stronê sprzêgu 1
	//a ujemne powinien byæ przy odwróconej polaryzacji sieci...
	//w wielu miejscach jest u¿ywane abs(Im)
	int Imin; int Imax;      /*prad przelaczania automatycznego rozruchu, prad bezpiecznika*/
	double Voltage;           /*aktualne napiecie sieci zasilajacej*/
	int MainCtrlActualPos; /*wskaznik RList*/
	int ScndCtrlActualPos; /*wskaznik MotorParam*/
	bool DelayCtrlFlag;  //czy czekanie na 1. pozycji na za³¹czenie?
	double LastRelayTime;     /*czas ostatniego przelaczania stycznikow*/
	bool AutoRelayFlag;  /*mozna zmieniac jesli AutoRelayType=2*/
	bool FuseFlag;       /*!o bezpiecznik nadmiarowy*/
	bool ConvOvldFlag;              /*!  nadmiarowy przetwornicy i ogrzewania*/
	bool StLinFlag;       /*!o styczniki liniowe*/
	bool ResistorsFlag;  /*!o jazda rezystorowa*/
	double RventRot;          /*!s obroty wentylatorow rozruchowych*/
	bool UnBrake;       /*w EZT - nacisniete odhamowywanie*/
	double PantPress; /*Cisnienie w zbiornikach pantografow*/
	bool s_CAtestebrake; //hunter-091012: zmienna dla testu ca


						 /*-zmienne dla lokomotywy spalinowej z przekladnia mechaniczna*/
	double dizel_fill; /*napelnienie*/
	double dizel_engagestate; /*sprzeglo skrzyni biegow: 0 - luz, 1 - wlaczone, 0.5 - wlaczone 50% (z poslizgiem)*/
	double dizel_engage; /*sprzeglo skrzyni biegow: aktualny docisk*/
	double dizel_automaticgearstatus; /*0 - bez zmiany, -1 zmiana na nizszy +1 zmiana na wyzszy*/
	bool dizel_enginestart;      /*czy trwa rozruch silnika*/
	double dizel_engagedeltaomega;    /*roznica predkosci katowych tarcz sprzegla*/

									  /*- zmienne dla lokomotyw z silnikami indukcyjnymi -*/
	double eimv[21];

	/*-zmienne dla drezyny*/
	double PulseForce;        /*przylozona sila*/
	double PulseForceTimer;
	int PulseForceCount;

	/*dla drezyny, silnika spalinowego i parowego*/
	double eAngle;

	/*-dla wagonow*/
	long Load;      /*masa w T lub ilosc w sztukach - zaladowane*/
	std::string LoadType;   /*co jest zaladowane*/
	int LoadStatus; //+1=trwa rozladunek,+2=trwa zaladunek,+4=zakoñczono,0=zaktualizowany model
	double LastLoadChangeTime; //raz (roz)³adowania

	bool DoorBlocked;    //Czy jest blokada drzwi
	bool DoorLeftOpened;  //stan drzwi
	bool DoorRightOpened;
	bool PantFrontUp;  //stan patykow 'Winger 160204
	bool PantRearUp;
	bool PantFrontSP;  //dzwiek patykow 'Winger 010304
	bool PantRearSP;
	int PantFrontStart;  //stan patykow 'Winger 160204
	int PantRearStart;
	double PantFrontVolt;   //pantograf pod napieciem? 'Winger 160404
	double PantRearVolt;
	std::string PantSwitchType;
	std::string ConvSwitchType;

	bool Heating; //ogrzewanie 'Winger 020304
	int DoubleTr; //trakcja ukrotniona - przedni pojazd 'Winger 160304

	bool PhysicActivation;

	/*ABu: stale dla wyznaczania sil (i nie tylko) po optymalizacji*/
	double FrictConst1;
	double FrictConst2s;
	double FrictConst2d;
	double TotalMassxg; /*TotalMass*g*/

	vector3 vCoulpler[2]; // powtórzenie wspó³rzêdnych sprzêgów z DynObj :/
	vector3 DimHalf; // po³owy rozmiarów do obliczeñ geometrycznych
					 // int WarningSignal; //0: nie trabi, 1,2: trabi syren¹ o podanym numerze
	int WarningSignal; // tymczasowo 8bit, ze wzglêdu na funkcje w MTools
	double fBrakeCtrlPos; // p³ynna nastawa hamulca zespolonego
	bool bPantKurek3; // kurek trójdrogowy (pantografu): true=po³¹czenie z ZG, false=po³¹czenie z ma³¹ sprê¿ark¹
	int iProblem; // flagi problemów z taborem, aby AI nie musia³o porównywaæ; 0=mo¿e jechaæ
	int iLights[2]; // bity zapalonych œwiate³ tutaj, ¿eby da³o siê liczyæ pobór pr¹du
private:
	double CouplerDist(int Coupler);

public:
	TMoverParameters(double VelInitial, std::string TypeNameInit, std::string NameInit, int LoadInitial, std::string LoadTypeInitial, int Cab);
	// obs³uga sprzêgów
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
	bool IncBrakeLevel(); // wersja na u¿ytek AI
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
	int ShowCurrent(int AmpN); //pokazuje bezwgl. wartosc pradu na wybranym amperomierzu
	int ShowCurrentP(int AmpN);  //pokazuje bezwgl. wartosc pradu w wybranym pojezdzie                                                             //Q 20160722

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
	bool readMPT(int ln, std::string line);                                                         //Q 20160717
	bool readRLIST(int ln, std::string line);                                                       //Q 20160718
	bool readBPT(int ln, std::string line);                                                         //Q 20160721
	void BrakeValveDecode(std::string s);                                                            //Q 20160719
	void BrakeSubsystemDecode();                                                                    //Q 20160719
	void PowerParamDecode(std::string lines, std::string prefix, TPowerParameters &PowerParamDecode); //Q 20160719
	TPowerSource PowerSourceDecode(std::string s);                                                   //Q 20160719
	TPowerType PowerDecode(std::string s);                                                           //Q 20160719
	TEngineTypes EngineDecode(std::string s);                                                        //Q 20160721
	bool CheckLocomotiveParameters(bool ReadyFlag, int Dir);
	std::string EngineDescription(int what);
};

extern double Distance(TLocation Loc1, TLocation Loc2, TDimension Dim1, TDimension Dim2);
