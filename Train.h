/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef TrainH
#define TrainH

//#include "Track.h"
//#include "TrkFoll.h"
#include "Button.h"
#include "DynObj.h"
#include "Gauge.h"
#include "Model3d.h"
#include "Spring.h"
#include "mtable.h"

#include "AdvSound.h"
#include "FadeSound.h"
#include "PyInt.h"
#include "RealSound.h"
#include "Sound.h"
#include <string>

// typedef enum {st_Off, st_Starting, st_On, st_ShuttingDown} T4State;

const int maxcab = 2;

// const double fCzuwakTime= 90.0f;
const double fCzuwakBlink = 0.15;
const float fConverterPrzekaznik = 1.5f; // hunter-261211: do przekaznika nadmiarowego przetwornicy
// 0.33f
// const double fBuzzerTime= 5.0f;
const float fHaslerTime = 1.2f;

// const double fStycznTime= 0.5f;
// const double fDblClickTime= 0.2f;

class TCab
{
  public:
    TCab();
    ~TCab();
    void Init(double Initx1, double Inity1, double Initz1, double Initx2, double Inity2,
              double Initz2, bool InitEnabled, bool InitOccupied);
    void Load(cParser &Parser);
    vector3 CabPos1;
    vector3 CabPos2;
    bool bEnabled;
    bool bOccupied;
    double dimm_r, dimm_g, dimm_b; // McZapkie-120503: tlumienie swiatla
    double intlit_r, intlit_g, intlit_b; // McZapkie-120503: oswietlenie kabiny
    double intlitlow_r, intlitlow_g,
        intlitlow_b; // McZapkie-120503: przyciemnione oswietlenie kabiny
  private:
    // bool bChangePossible;
    TGauge *ggList; // Ra 2014-08: lista animacji macierzowych (gałek) w kabinie
    int iGaugesMax, iGauges; // ile miejsca w tablicy i ile jest w użyciu
    TButton *btList; // Ra 2014-08: lista animacji dwustanowych (lampek) w kabinie
    int iButtonsMax, iButtons; // ile miejsca w tablicy i ile jest w użyciu
  public:
    TGauge *Gauge(int n = -1); // pobranie adresu obiektu
    TButton *Button(int n = -1); // pobranie adresu obiektu
    void Update();
};

class TTrain
{
  public:
    bool CabChange(int iDirection);
    bool ActiveUniversal4;
    bool ShowNextCurrent; // pokaz przd w podlaczonej lokomotywie (ET41)
    bool InitializeCab(int NewCabNo, std::string const &asFileName);
    TTrain();
    ~TTrain();
    //    bool Init(TTrack *Track);
    // McZapkie-010302
    bool Init(TDynamicObject *NewDynamicObject, bool e3d = false);
    void OnKeyDown(int cKey);
    void OnKeyUp(int cKey);

    //    bool SHP() { fShpTimer= 0; };

    inline vector3 GetDirection()
    {
        return DynamicObject->VectorFront();
    };
    inline vector3 GetUp()
    {
        return DynamicObject->VectorUp();
    };
    void UpdateMechPosition(double dt);
    bool Update( double const Deltatime );
    void MechStop();
    void SetLights();
    //    virtual bool RenderAlpha();
    // McZapkie-310302: ladowanie parametrow z pliku
    bool LoadMMediaFile(std::string const &asFileName);
    PyObject *GetTrainState();

  private:
    // clears state of all cabin controls
    void clear_cab_controls();
    // initializes a gauge matching provided label. returns: true if the label was found, false
    // otherwise
    bool initialize_gauge(cParser &Parser, std::string const &Label, int const Cabindex);
    // initializes a button matching provided label. returns: true if the label was found, false
    // otherwise
    bool initialize_button(cParser &Parser, std::string const &Label, int const Cabindex);

  private: //żeby go nic z zewnątrz nie przestawiało
    TDynamicObject *DynamicObject; // przestawia zmiana pojazdu [F5]
  private: //żeby go nic z zewnątrz nie przestawiało
    TMoverParameters *mvControlled; // człon, w którym sterujemy silnikiem
    TMoverParameters *mvOccupied; // człon, w którym sterujemy hamulcem
    TMoverParameters *mvSecond; // drugi człon (ET40, ET41, ET42, ukrotnienia)
    TMoverParameters *mvThird; // trzeci człon (SN61)
  public: // reszta może by?publiczna
    // AnsiString asMessage;

    // McZapkie: definicje wskaźników
    // Ra 2014-08: częsciowo przeniesione do tablicy w TCab
    TGauge ggZbS;
    TGauge ggClockSInd;
    TGauge ggClockMInd;
    TGauge ggClockHInd;
    // TGauge ggHVoltage;
    TGauge ggLVoltage;
    // TGauge ggEnrot1m;
    // TGauge ggEnrot2m;
    // TGauge ggEnrot3m;
    // TGauge ggEngageRatio;
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
    TGauge ggConverterFuseButton; // hunter-261211: przycisk odblokowania
    // nadmiarowego przetwornic i ogrzewania
    TGauge ggStLinOffButton;
    TGauge ggRadioButton;
    TGauge ggUpperLightButton;
    TGauge ggLeftLightButton;
    TGauge ggRightLightButton;
    TGauge ggLeftEndLightButton;
    TGauge ggRightEndLightButton;
    TGauge ggLightsButton; // przelacznik reflektorow (wszystkich)

    // hunter-230112: przelacznik swiatel tylnich
    TGauge ggRearUpperLightButton;
    TGauge ggRearLeftLightButton;
    TGauge ggRearRightLightButton;
    TGauge ggRearLeftEndLightButton;
    TGauge ggRearRightEndLightButton;

    TGauge ggIgnitionKey;

    TGauge ggCompressorButton;
    TGauge ggConverterButton;
    TGauge ggConverterOffButton;

    // ABu 090305 - syrena i prad nastepnego czlonu
    TGauge ggHornButton;
    TGauge ggNextCurrentButton;
    // ABu 090305 - uniwersalne przyciski
    TGauge ggUniversal1Button;
    TGauge ggUniversal2Button;
    TGauge ggUniversal3Button;
    TGauge ggUniversal4Button;

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
    TGauge ggPantAllDownButton;
    // Winger 020304 - wlacznik ogrzewania
    TGauge ggTrainHeatingButton;
    TGauge ggSignallingButton;
    TGauge ggDoorSignallingButton;
    //    TModel3d *mdKabina; McZapkie-030303: to do dynobj
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
    //    TButton btLampkaUnknown;
    TButton btLampkaOpory;
    TButton btLampkaWysRozr;
    TButton btLampkaUniversal3;
    int LampkaUniversal3_typ; // ABu 030405 - swiecenie uzaleznione od: 0-nic,
    // 1-obw.gl, 2-przetw.
    bool LampkaUniversal3_st;
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
    //    TButton bt;
    //
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
    double fMechCroach;
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

    PSound dsbNastawnikJazdy;
    PSound dsbNastawnikBocz; // hunter-081211
    PSound dsbRelay;
    PSound dsbPneumaticRelay;
    PSound dsbSwitch;
    PSound dsbPneumaticSwitch;
    PSound dsbReverserKey; // hunter-121211

    PSound dsbCouplerAttach; // Ra: w kabinie????
    PSound dsbCouplerDetach; // Ra: w kabinie???

    PSound dsbDieselIgnition; // Ra: w kabinie???

    PSound dsbDoorClose; // Ra: w kabinie???
    PSound dsbDoorOpen; // Ra: w kabinie???

    // Winger 010304
    PSound dsbPantUp;
    PSound dsbPantDown;

    PSound dsbWejscie_na_bezoporow;
    PSound dsbWejscie_na_drugi_uklad; // hunter-081211: poprawka literowki

    //    PSound dsbHiss1;
    //  PSound dsbHiss2;

    // McZapkie-280302
    TRealSound rsBrake;
    TRealSound rsSlippery;
    TRealSound rsHiss; // upuszczanie
    TRealSound rsHissU; // napelnianie
    TRealSound rsHissE; // nagle
    TRealSound rsHissX; // fala
    TRealSound rsHissT; // czasowy
    TRealSound rsSBHiss;
    TRealSound rsRunningNoise;
    TRealSound rsEngageSlippery;
    TRealSound rsFadeSound;

    PSound dsbHasler;
    PSound dsbBuzzer;
    PSound dsbSlipAlarm; // Bombardier 011010: alarm przy poslizgu dla 181/182
    // TFadeSound sConverter;  //przetwornica
    // TFadeSound sSmallCompressor;  //przetwornica

    int iCabLightFlag; // McZapkie:120503: oswietlenie kabiny (0: wyl, 1:
    // przyciemnione, 2: pelne)
    bool bCabLight; // hunter-091012: czy swiatlo jest zapalone?
    bool bCabLightDim; // hunter-091012: czy przyciemnienie kabiny jest zapalone?

    vector3 pMechSittingPosition; // ABu 180404
    vector3 MirrorPosition(bool lewe);

  private:
    // PSound dsbBuzzer;
    PSound dsbCouplerStretch;
    PSound dsbEN57_CouplerStretch;
    PSound dsbBufferClamp;
    //    TSubModel *smCzuwakShpOn;
    //    TSubModel *smCzuwakOn;
    //    TSubModel *smShpOn;
    //    TSubModel *smCzuwakShpOff;
    //    double fCzuwakTimer;
    double fBlinkTimer;
    float fHaslerTimer;
    float fConverterTimer; // hunter-261211: dla przekaznika
    float fMainRelayTimer; // hunter-141211: zalaczanie WSa z opoznieniem
    float fCzuwakTestTimer; // hunter-091012: do testu czuwaka
    float fLightsTimer; // yB 150617: timer do swiatel

    int CAflag; // hunter-131211: dla osobnego zbijania CA i SHP

    double fPoslizgTimer;
    //    double fShpTimer;
    //    double fDblClickTimer;
    // ABu: Przeniesione do public. - Wiem, ze to nieladnie...
    // bool CabChange(int iDirection);
    // bool InitializeCab(int NewCabNo, AnsiString asFileName);
    TTrack *tor;
    int keybrakecount;
    // McZapkie-240302 - przyda sie do tachometru
    float fTachoVelocity;
    float fTachoVelocityJump; // ze skakaniem
    float fTachoTimer;
    float fTachoCount;
    float fHVoltage; // napi?cie dla dynamicznych ga?ek
    float fHCurrent[4]; // pr?dy: suma i amperomierze 1,2,3
    float fEngine[4]; // obroty te? trzeba pobra?
    int iCarNo, iPowerNo, iUnitNo; // liczba pojazdow, czlonow napednych i jednostek spiętych ze
                                   // sobą
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
    float fEIMParams[9][10]; // parametry dla silnikow asynchronicznych
    int RadioChannel()
    {
        return iRadioChannel;
    };
    inline TDynamicObject *Dynamic()
    {
        return DynamicObject;
    };
    inline TMoverParameters *Controlled()
    {
        return mvControlled;
    };
    void DynamicSet(TDynamicObject *d);
    void Silence();
};
//---------------------------------------------------------------------------
#endif
