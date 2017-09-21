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
#include "DynObj.h"
#include "Button.h"
#include "Gauge.h"
#include "Spring.h"
#include "PyInt.h"
#include "command.h"

// typedef enum {st_Off, st_Starting, st_On, st_ShuttingDown} T4State;

const int maxcab = 2;

const double fCzuwakBlink = 0.15;
const float fConverterPrzekaznik = 1.5f; // hunter-261211: do przekaznika nadmiarowego przetwornicy
// 0.33f
// const double fBuzzerTime= 5.0f;
const float fHaslerTime = 1.2f;

class TCab
{
  public:
    TCab();
    ~TCab();
/*
    void Init(double Initx1, double Inity1, double Initz1, double Initx2, double Inity2, double Initz2, bool InitEnabled, bool InitOccupied);
*/
    void Load(cParser &Parser);
    vector3 CabPos1;
    vector3 CabPos2;
    bool bEnabled;
    bool bOccupied;
    double dimm_r, dimm_g, dimm_b; // McZapkie-120503: tlumienie swiatla
    double intlit_r, intlit_g, intlit_b; // McZapkie-120503: oswietlenie kabiny
    double intlitlow_r, intlitlow_g, intlitlow_b; // McZapkie-120503: przyciemnione oswietlenie kabiny
  private:
/*
    TGauge *ggList; // Ra 2014-08: lista animacji macierzowych (gałek) w kabinie
    int iGaugesMax, iGauges; // ile miejsca w tablicy i ile jest w użyciu
    TButton *btList; // Ra 2014-08: lista animacji dwustanowych (lampek) w kabinie
    int iButtonsMax, iButtons; // ile miejsca w tablicy i ile jest w użyciu
*/
      std::vector<TGauge> ggList;
      std::vector<TButton> btList;
  public:
    TGauge &Gauge(int n = -1); // pobranie adresu obiektu
    TButton &Button(int n = -1); // pobranie adresu obiektu
    void Update();
};

class control_mapper {
    typedef std::unordered_map< TSubModel const *, std::string> submodelstring_map;
    submodelstring_map m_controlnames;
public:
    void
        clear() { m_controlnames.clear(); }
    void
        insert( TGauge const &Gauge, std::string const &Label );
    std::string
        find( TSubModel const *Control ) const;
};

class TTrain
{
  public:
    bool CabChange(int iDirection);
    bool ShowNextCurrent; // pokaz przd w podlaczonej lokomotywie (ET41)
    bool InitializeCab(int NewCabNo, std::string const &asFileName);
    TTrain();
    ~TTrain();
    // McZapkie-010302
    bool Init(TDynamicObject *NewDynamicObject, bool e3d = false);
    void OnKeyDown(int cKey);

    inline vector3 GetDirection() { return DynamicObject->VectorFront(); };
    inline vector3 GetUp() { return DynamicObject->VectorUp(); };
    inline std::string GetLabel( TSubModel const *Control ) const { return m_controlmapper.find( Control ); }
    void UpdateMechPosition(double dt);
    vector3 GetWorldMechPosition();
    bool Update( double const Deltatime );
    bool m_updated = false;
    void MechStop();
    void SetLights();
    // McZapkie-310302: ladowanie parametrow z pliku
    bool LoadMMediaFile(std::string const &asFileName);
    PyObject *GetTrainState();

  private:
// types
    typedef void( *command_handler )( TTrain *Train, command_data const &Command );
    typedef std::unordered_map<user_command, command_handler> commandhandler_map;
    // clears state of all cabin controls
    void clear_cab_controls();
    // sets cabin controls based on current state of the vehicle
    // NOTE: we can get rid of this function once we have per-cab persistent state
    void set_cab_controls();
    // initializes a gauge matching provided label. returns: true if the label was found, false otherwise
    bool initialize_gauge(cParser &Parser, std::string const &Label, int const Cabindex);
    // initializes a button matching provided label. returns: true if the label was found, false otherwise
    bool initialize_button(cParser &Parser, std::string const &Label, int const Cabindex);
    // plays specified sound, or fallback sound if the primary sound isn't presend
    // NOTE: temporary routine until sound system is sorted out and paired with switches
    void play_sound( sound* Sound, float gain = 1.0f);
    void play_sound( sound* Sound, sound* Fallbacksound, float gain = 1.0f );
    // helper, returns true for EMU with oerlikon brake
    bool is_eztoer() const;
    // command handlers
    // NOTE: we're currently using universal handlers and static handler map but it may be beneficial to have these implemented on individual class instance basis
    // TBD, TODO: consider this approach if we ever want to have customized consist behaviour to received commands, based on the consist/vehicle type or whatever
    static void OnCommand_mastercontrollerincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerincreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerdecreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerincreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerdecreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_notchingrelaytoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_mucurrentindicatorothersourceactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakeincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakeincreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakedecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakedecreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakebailoff( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakedecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakecharging( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakerelease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakefirstservice( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeservice( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakefullservice( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeemergency( TTrain *Train, command_data const &Command );
    static void OnCommand_manualbrakeincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_manualbrakedecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_alarmchaintoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_wheelspinbrakeactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_sandboxactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_epbrakecontroltoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeactingspeedincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeactingspeeddecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_mubrakingindicatortoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_alerteracknowledge( TTrain *Train, command_data const &Command );
    static void OnCommand_batterytoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographcompressorvalvetoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographcompressoractivate( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographtogglefront( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographtogglerear( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographlowerall( TTrain *Train, command_data const &Command );
    static void OnCommand_linebreakertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_convertertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_convertertogglelocal( TTrain *Train, command_data const &Command );
    static void OnCommand_converteroverloadrelayreset( TTrain *Train, command_data const &Command );
    static void OnCommand_compressortoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_compressortogglelocal( TTrain *Train, command_data const &Command );
    static void OnCommand_motorconnectorsopen( TTrain *Train, command_data const &Command );
    static void OnCommand_motordisconnect( TTrain *Train, command_data const &Command );
    static void OnCommand_motoroverloadrelaythresholdtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_motoroverloadrelayreset( TTrain *Train, command_data const &Command );
    static void OnCommand_heatingtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttoggleleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttoggleright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttoggleupper( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertoggleleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertoggleright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttogglerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttogglerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttogglerearupper( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertogglerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertogglerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightsdimtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlighttoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlightdimtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_instrumentlighttoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_doorlocktoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_doortoggleleft( TTrain *Train, command_data const &Command );
    static void OnCommand_doortoggleright( TTrain *Train, command_data const &Command );
    static void OnCommand_departureannounce( TTrain *Train, command_data const &Command );
    static void OnCommand_hornlowactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_hornhighactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_radiotoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_radiostoptest( TTrain *Train, command_data const &Command );
    static void OnCommand_generictoggle( TTrain *Train, command_data const &Command );

// members
    TDynamicObject *DynamicObject; // przestawia zmiana pojazdu [F5]
    TMoverParameters *mvControlled; // człon, w którym sterujemy silnikiem
    TMoverParameters *mvOccupied; // człon, w którym sterujemy hamulcem
    TMoverParameters *mvSecond; // drugi człon (ET40, ET41, ET42, ukrotnienia)
    TMoverParameters *mvThird; // trzeci człon (SN61)
    // helper variable, to prevent immediate switch between closing and opening line breaker circuit
    int m_linebreakerstate { 0 }; // -1: freshly open, 0: open, 1: closed, 2: freshly closed (and yes this is awful way to go about it)
    static const commandhandler_map m_commandhandlers;
    control_mapper m_controlmapper;

public: // reszta może by?publiczna

    // McZapkie: definicje wskaźników
    // Ra 2014-08: częsciowo przeniesione do tablicy w TCab
    TGauge ggZbS;
    TGauge ggClockSInd;
    TGauge ggClockMInd;
    TGauge ggClockHInd;
    TGauge ggLVoltage;
    TGauge ggMainGearStatus;

    TGauge ggEngineVoltage;
    TGauge ggI1B; // drugi człon w postaci jawnej
    TGauge ggI2B;
    TGauge ggI3B;
    TGauge ggItotalB;

    // McZapkie: definicje regulatorow
    TGauge ggMainCtrl;
    TGauge ggMainCtrlAct;
    TGauge ggScndCtrl;
    TGauge ggScndCtrlButton;
    TGauge ggDirKey;
    TGauge ggBrakeCtrl;
    TGauge ggLocalBrake;
    TGauge ggManualBrake;
    TGauge ggAlarmChain;
    TGauge ggBrakeProfileCtrl; // nastawiacz GPR - przelacznik obrotowy
    TGauge ggBrakeProfileG; // nastawiacz GP - hebelek towarowy
    TGauge ggBrakeProfileR; // nastawiacz PR - hamowanie dwustopniowe

    TGauge ggMaxCurrentCtrl;

    TGauge ggMainOffButton;
    TGauge ggMainOnButton;
    TGauge ggMainButton; // EZT
    TGauge ggSecurityResetButton;
    TGauge ggReleaserButton;
    TGauge ggSandButton; // guzik piasecznicy
    TGauge ggAntiSlipButton;
    TGauge ggFuseButton;
    TGauge ggConverterFuseButton; // hunter-261211: przycisk odblokowania nadmiarowego przetwornic i ogrzewania
    TGauge ggStLinOffButton;
    TGauge ggRadioButton;
    TGauge ggUpperLightButton;
    TGauge ggLeftLightButton;
    TGauge ggRightLightButton;
    TGauge ggLeftEndLightButton;
    TGauge ggRightEndLightButton;
    TGauge ggLightsButton; // przelacznik reflektorow (wszystkich)
    TGauge ggDimHeadlightsButton; // headlights dimming switch

    // hunter-230112: przelacznik swiatel tylnich
    TGauge ggRearUpperLightButton;
    TGauge ggRearLeftLightButton;
    TGauge ggRearRightLightButton;
    TGauge ggRearLeftEndLightButton;
    TGauge ggRearRightEndLightButton;

    TGauge ggIgnitionKey;

    TGauge ggCompressorButton;
    TGauge ggCompressorLocalButton; // controls only compressor of its own unit (et42-specific)
    TGauge ggConverterButton;
    TGauge ggConverterLocalButton; // controls only converter of its own unit (et42-specific)
    TGauge ggConverterOffButton;

    // ABu 090305 - syrena i prad nastepnego czlonu
    TGauge ggHornButton;
    TGauge ggHornLowButton;
    TGauge ggHornHighButton;
    TGauge ggNextCurrentButton;

    std::array<TGauge, 10> ggUniversals; // NOTE: temporary arrangement until we have dynamically built control table

    TGauge ggInstrumentLightButton;
    TGauge ggCabLightButton; // hunter-091012: przelacznik oswietlania kabiny
    TGauge ggCabLightDimButton; // hunter-091012: przelacznik przyciemnienia
    TGauge ggBatteryButton; // Stele 161228 hebelek baterii
    // oswietlenia kabiny

    // NBMX wrzesien 2003 - obsluga drzwi
    TGauge ggDoorLeftButton;
    TGauge ggDoorRightButton;
    TGauge ggDepartureSignalButton;

    // Winger 160204 - obsluga pantografow - ZROBIC
    TGauge ggPantFrontButton;
    TGauge ggPantRearButton;
    TGauge ggPantFrontButtonOff; // EZT
    TGauge ggPantRearButtonOff;
    TGauge ggPantAllDownButton;
    TGauge ggPantSelectedButton;
    TGauge ggPantSelectedDownButton;
    TGauge ggPantCompressorButton;
    TGauge ggPantCompressorValve;
    // Winger 020304 - wlacznik ogrzewania
    TGauge ggTrainHeatingButton;
    TGauge ggSignallingButton;
    TGauge ggDoorSignallingButton;
    // TGauge ggDistCounter; //Ra 2014-07: licznik kilometrów
    // TGauge ggVelocityDgt; //i od razu prędkościomierz

    TButton btLampkaPoslizg;
    TButton btLampkaStyczn;
    TButton btLampkaNadmPrzetw;
    TButton btLampkaPrzetw;
    TButton btLampkaPrzekRozn;
    TButton btLampkaPrzekRoznPom;
    TButton btLampkaNadmSil;
    TButton btLampkaWylSzybki;
    TButton btLampkaNadmWent;
    TButton btLampkaNadmSpr;
    // yB: drugie lampki dla EP05 i ET42
    TButton btLampkaOporyB;
    TButton btLampkaStycznB;
    TButton btLampkaWylSzybkiB;
    TButton btLampkaNadmPrzetwB;
    TButton btLampkaPrzetwB;
    // KURS90 lampki jazdy bezoporowej dla EU04
    TButton btLampkaBezoporowaB;
    TButton btLampkaBezoporowa;
    TButton btLampkaUkrotnienie;
    TButton btLampkaHamPosp;
    TButton btLampkaRadio;
    TButton btLampkaHamowanie1zes;
    TButton btLampkaHamowanie2zes;
    TButton btLampkaOpory;
    TButton btLampkaWysRozr;
    TButton btInstrumentLight;
    int InstrumentLightType; // ABu 030405 - swiecenie uzaleznione od: 0-nic, 1-obw.gl, 2-przetw.
    bool InstrumentLightActive;
    TButton btLampkaWentZaluzje; // ET22
    TButton btLampkaOgrzewanieSkladu;
    TButton btLampkaSHP;
    TButton btLampkaCzuwaka; // McZapkie-141102
    TButton btLampkaRezerwa;
    // youBy - jakies dodatkowe lampki
    TButton btLampkaNapNastHam;
    TButton btLampkaSprezarka;
    TButton btLampkaSprezarkaB;
    TButton btLampkaBocznik1;
    TButton btLampkaBocznik2;
    TButton btLampkaBocznik3;
    TButton btLampkaBocznik4;
    TButton btLampkaRadiotelefon;
    TButton btLampkaHamienie;
    TButton btLampkaED; // Stele 161228 hamowanie elektrodynamiczne
    TButton btLampkaJazda; // Ra: nie używane
    // KURS90
    TButton btLampkaBoczniki;
    TButton btLampkaMaxSila;
    TButton btLampkaPrzekrMaxSila;
    TButton btLampkaDoorLeft;
    TButton btLampkaDoorRight;
    TButton btLampkaDepartureSignal;
    TButton btLampkaBlokadaDrzwi;
    TButton btLampkaHamulecReczny;
    TButton btLampkaForward; // Ra: lampki w przód i w ty?dla komputerowych kabin
    TButton btLampkaBackward;

    TButton btCabLight; // hunter-171012: lampa oswietlajaca kabine
    // Ra 2013-12: wirtualne "lampki" do odbijania na haslerze w PoKeys
    TButton btHaslerBrakes; // ciśnienie w cylindrach
    TButton btHaslerCurrent; // prąd na silnikach

    vector3 pPosition;
    vector3 pMechOffset; // driverNpos
    vector3 vMechMovement;
    vector3 pMechPosition;
    vector3 pMechShake;
    vector3 vMechVelocity;
    // McZapkie: do poruszania sie po kabinie
    // McZapkie: opis kabiny - obszar poruszania sie mechanika oraz zajetosc
    TCab Cabine[maxcab + 1]; // przedzial maszynowy, kabina 1 (A), kabina 2 (B)
    int iCabn;
    TSpring MechSpring;
    double fMechSpringX; // McZapkie-250303: parametry bujania
    double fMechSpringY;
    double fMechSpringZ;
    double fMechMaxSpring;
    double fMechRoll;
    double fMechPitch;

    sound* dsbNastawnikJazdy = nullptr;
    sound* dsbNastawnikBocz = nullptr; // hunter-081211
    sound* dsbRelay = nullptr;
    sound* dsbPneumaticRelay = nullptr;
    sound* dsbSwitch = nullptr;
    sound* dsbPneumaticSwitch = nullptr;
    sound* dsbReverserKey = nullptr; // hunter-121211

    sound* dsbCouplerAttach = nullptr; // Ra: w kabinie????
    sound* dsbCouplerDetach = nullptr; // Ra: w kabinie???

    sound* dsbDieselIgnition = nullptr; // Ra: w kabinie???

    sound* dsbDoorClose = nullptr; // Ra: w kabinie???
    sound* dsbDoorOpen = nullptr; // Ra: w kabinie???

    // Winger 010304
    sound* dsbPantUp = nullptr;
    sound* dsbPantDown = nullptr;

    sound* dsbWejscie_na_bezoporow = nullptr;
    sound* dsbWejscie_na_drugi_uklad = nullptr; // hunter-081211: poprawka literowki

    //    sound* dsbHiss1;
    //  sound* dsbHiss2;

    // McZapkie-280302
    sound* rsBrake = nullptr;
    sound* rsSlippery = nullptr;
    sound* rsHiss = nullptr; // upuszczanie
    sound* rsHissU = nullptr; // napelnianie
    sound* rsHissE = nullptr; // nagle
    sound* rsHissX = nullptr; // fala
    sound* rsHissT = nullptr; // czasowy
    sound* rsSBHiss = nullptr;
    sound* rsRunningNoise = nullptr;
    sound* rsEngageSlippery = nullptr;
    sound* rsFadeSound = nullptr;

    sound* dsbHasler = nullptr;
    sound* dsbBuzzer = nullptr;
    sound* dsbSlipAlarm = nullptr; // Bombardier 011010: alarm przy poslizgu dla 181/182

    int iCabLightFlag; // McZapkie:120503: oswietlenie kabiny (0: wyl, 1: przyciemnione, 2: pelne)
    bool bCabLight; // hunter-091012: czy swiatlo jest zapalone?
    bool bCabLightDim; // hunter-091012: czy przyciemnienie kabiny jest zapalone?

    vector3 pMechSittingPosition; // ABu 180404
    vector3 MirrorPosition(bool lewe);

  private:
    sound* dsbCouplerStretch = nullptr;
    sound* dsbEN57_CouplerStretch = nullptr;
    sound* dsbBufferClamp = nullptr;

    double fBlinkTimer;
    float fHaslerTimer;
    float fConverterTimer; // hunter-261211: dla przekaznika
    float fMainRelayTimer; // hunter-141211: zalaczanie WSa z opoznieniem
    float fCzuwakTestTimer; // hunter-091012: do testu czuwaka
    float fLightsTimer; // yB 150617: timer do swiatel

    int CAflag; // hunter-131211: dla osobnego zbijania CA i SHP

    double fPoslizgTimer;
    TTrack *tor;
    int keybrakecount;
    // McZapkie-240302 - przyda sie do tachometru
    float fTachoVelocity{ 0.0f };
    float fTachoVelocityJump{ 0.0f }; // ze skakaniem
    float fTachoTimer{ 0.0f };
    float fTachoCount{ 0.0f };
    float fHVoltage{ 0.0f }; // napi?cie dla dynamicznych ga?ek
    float fHCurrent[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f }; // pr?dy: suma i amperomierze 1,2,3
    float fEngine[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f }; // obroty te? trzeba pobra?
    int iCarNo, iPowerNo, iUnitNo; // liczba pojazdow, czlonow napednych i jednostek spiętych ze sobą
    bool bDoors[20][3]; // drzwi dla wszystkich czlonow
    int iUnits[20]; // numer jednostki
    int iDoorNo[20]; // liczba drzwi
    char cCode[20]; // kod pojazdu
    std::string asCarName[20]; // nazwa czlonu
    bool bMains[8]; // WSy
    float fCntVol[8]; // napiecie NN
    bool bPants[8][2]; // podniesienie pantografow
    bool bFuse[8]; // nadmiarowe
    bool bBatt[8]; // baterie
    bool bConv[8]; // przetwornice
    bool bComp[8][2]; // sprezarki
    bool bHeat[8]; // grzanie
    // McZapkie: do syczenia
    float fPPress, fNPress;
    float fSPPress, fSNPress;
    int iSekunda; // Ra: sekunda aktualizacji pr?dko?ci
    int iRadioChannel; // numer aktualnego kana?u radiowego
    TPythonScreens pyScreens;

  public:
    float fPress[20][3]; // cisnienia dla wszystkich czlonow
    static std::vector<std::string> const fPress_labels;
    float fEIMParams[9][10]; // parametry dla silnikow asynchronicznych
    int RadioChannel() { return iRadioChannel; };
    inline TDynamicObject *Dynamic() { return DynamicObject; };
    inline TMoverParameters *Controlled() { return mvControlled; };
    void DynamicSet(TDynamicObject *d);
    void Silence();

	float get_tacho();
	float get_tank_pressure();
	float get_pipe_pressure();
	float get_brake_pressure();
	float get_hv_voltage();
	std::array<float, 3> get_current();
	bool get_alarm();
	int get_drive_direction();

    void set_mainctrl(int);
    void set_scndctrl(int);
    void set_trainbrake(float);
    void set_localbrake(float);
};
//---------------------------------------------------------------------------
