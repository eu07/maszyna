//---------------------------------------------------------------------------

#ifndef TrainH
#define TrainH

#include "Track.h"
#include "TrkFoll.h"
#include "Model3d.h"
#include "Spring.h"
#include "Gauge.h"
#include "Button.h"
#include "DynObj.h"
#include "mtable.hpp"

#include "Sound.h"
#include "AdvSound.h"
#include "RealSound.h"
#include "FadeSound.h"


// typedef enum {st_Off, st_Starting, st_On, st_ShuttingDown} T4State;

const int maxcab=2;

// const double fCzuwakTime= 90.0f;
const double fCzuwakBlink=0.15;
const float fConverterPrzekaznik=1.5; //hunter-261211: do przekaznika nadmiarowego przetwornicy
                         //0.33f
// const double fBuzzerTime= 5.0f;
const float fHaslerTime=1.2;

// const double fStycznTime= 0.5f;
// const double fDblClickTime= 0.2f;

class TCab
{
public:
 __fastcall TCab();
 __fastcall ~TCab();
 void __fastcall Init(double Initx1,double Inity1,double Initz1,double Initx2,double Inity2,double Initz2,bool InitEnabled,bool InitOccupied);
 void __fastcall Load(TQueryParserComp *Parser);
 vector3 CabPos1;
 vector3 CabPos2;
 bool bEnabled;
 bool bOccupied;
 double dimm_r, dimm_g, dimm_b;                 //McZapkie-120503: tlumienie swiatla
 double intlit_r, intlit_g, intlit_b;           //McZapkie-120503: oswietlenie kabiny
 double intlitlow_r, intlitlow_g, intlitlow_b;  //McZapkie-120503: przyciemnione oswietlenie kabiny
private:
 //bool bChangePossible;
 TGauge *ggList; //Ra 2014-08: lista animacji macierzowych (ga�ek) w kabinie
 int iGaugesMax,iGauges; //ile miejsca w tablicy i ile jest w u�yciu
 TButton *btList; //Ra 2014-08: lista animacji dwustanowych (lampek) w kabinie
 int iButtonsMax,iButtons; //ile miejsca w tablicy i ile jest w u�yciu
public:
 TGauge* __fastcall Gauge(int n=-1); //pobranie adresu obiektu
 TButton* __fastcall Button(int n=-1); //pobranie adresu obiektu
 void __fastcall Update();
};


class TTrain
{
public:
    bool CabChange(int iDirection);
    bool ActiveUniversal4;
    bool ShowNextCurrent; //pokaz przd w podlaczonej lokomotywie (ET41)
    bool InitializeCab(int NewCabNo, AnsiString asFileName);
    __fastcall TTrain();
    __fastcall ~TTrain();
//    bool __fastcall Init(TTrack *Track);
//McZapkie-010302
    bool __fastcall Init(TDynamicObject *NewDynamicObject,bool e3d=false);
    void __fastcall OnKeyDown(int cKey);
    void __fastcall OnKeyUp(int cKey);

//    bool __fastcall SHP() { fShpTimer= 0; };

    inline vector3 __fastcall GetDirection() { return DynamicObject->VectorFront(); };
    inline vector3 __fastcall GetUp() { return DynamicObject->VectorUp(); };
    void __fastcall UpdateMechPosition(double dt);
    bool __fastcall Update();
    void __fastcall MechStop();
//    virtual bool __fastcall RenderAlpha();
//McZapkie-310302: ladowanie parametrow z pliku
    bool __fastcall LoadMMediaFile(AnsiString asFileName);

private: //�eby go nic z zewn�trz nie przestawia�o
 TDynamicObject *DynamicObject; //przestawia zmiana pojazdu [F5]
private: //�eby go nic z zewn�trz nie przestawia�o
 TMoverParameters *mvControlled; //cz�on, w kt�rym sterujemy silnikiem
 TMoverParameters *mvOccupied; //cz�on, w kt�rym sterujemy hamulcem
 TMoverParameters *mvSecond; //drugi cz�on (ET40, ET41, ET42, ukrotnienia)
 TMoverParameters *mvThird; //trzeci cz�on (SN61)
public: //reszta mo�e by� publiczna

    //AnsiString asMessage;

//McZapkie: definicje wska�nik�w
//Ra 2014-08: cz�sciowo przeniesione do tablicy w TCab
    TGauge ggZbS;
    TGauge ggClockSInd;
    TGauge ggClockMInd;
    TGauge ggClockHInd;
    //TGauge ggHVoltage;
    TGauge ggLVoltage;
    //TGauge ggEnrot1m;
    //TGauge ggEnrot2m;
    //TGauge ggEnrot3m;
    //TGauge ggEngageRatio;
    TGauge ggMainGearStatus;

    TGauge ggEngineVoltage;
    TGauge ggI1B; //drugi cz�on w postaci jawnej
    TGauge ggI2B;
    TGauge ggI3B;
    TGauge ggItotalB;

//McZapkie: definicje regulatorow
    TGauge ggMainCtrl;
    TGauge ggMainCtrlAct;
    TGauge ggScndCtrl;
    TGauge ggScndCtrlButton;
    TGauge ggDirKey;
    TGauge ggBrakeCtrl;
    TGauge ggLocalBrake;
    TGauge ggManualBrake;
    TGauge ggBrakeProfileCtrl; //nastawiacz GPR - przelacznik obrotowy
    TGauge ggBrakeProfileG;    //nastawiacz GP - hebelek towarowy
    TGauge ggBrakeProfileR;    //nastawiacz PR - hamowanie dwustopniowe

    TGauge ggMaxCurrentCtrl;

    TGauge ggMainOffButton;
    TGauge ggMainOnButton;
    TGauge ggMainButton; //EZT
    TGauge ggSecurityResetButton;
    TGauge ggReleaserButton;
    TGauge ggAntiSlipButton;
    TGauge ggFuseButton;
    TGauge ggConverterFuseButton; //hunter-261211: przycisk odblokowania nadmiarowego przetwornic i ogrzewania
    TGauge ggStLinOffButton;
    TGauge ggRadioButton;
    TGauge ggUpperLightButton;
    TGauge ggLeftLightButton;
    TGauge ggRightLightButton;
    TGauge ggLeftEndLightButton;
    TGauge ggRightEndLightButton;

    //hunter-230112: przelacznik swiatel tylnich
    TGauge ggRearUpperLightButton;
    TGauge ggRearLeftLightButton;
    TGauge ggRearRightLightButton;
    TGauge ggRearLeftEndLightButton;
    TGauge ggRearRightEndLightButton;

    TGauge ggIgnitionKey;

    TGauge ggCompressorButton;
    TGauge ggConverterButton;
    TGauge ggConverterOffButton;

//ABu 090305 - syrena i prad nastepnego czlonu
    TGauge ggHornButton;
    TGauge ggNextCurrentButton;
//ABu 090305 - uniwersalne przyciski
    TGauge ggUniversal1Button;
    TGauge ggUniversal2Button;
    TGauge ggUniversal3Button;
    TGauge ggUniversal4Button;

    TGauge ggCabLightButton; //hunter-091012: przelacznik oswietlania kabiny
    TGauge ggCabLightDimButton; //hunter-091012: przelacznik przyciemnienia oswietlenia kabiny

//NBMX wrzesien 2003 - obsluga drzwi
    TGauge ggDoorLeftButton;
    TGauge ggDoorRightButton;
    TGauge ggDepartureSignalButton;

//Winger 160204 - obsluga pantografow - ZROBIC
    TGauge ggPantFrontButton;
    TGauge ggPantRearButton;
    TGauge ggPantFrontButtonOff; //EZT
    TGauge ggPantAllDownButton;
//Winger 020304 - wlacznik ogrzewania
    TGauge ggTrainHeatingButton;
    TGauge ggSignallingButton;
    TGauge ggDoorSignallingButton;
//    TModel3d *mdKabina; McZapkie-030303: to do dynobj
    //TGauge ggDistCounter; //Ra 2014-07: licznik kilometr�w
    //TGauge ggVelocityDgt; //i od razu pr�dko�ciomierz

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
//yB: drugie lampki dla EP05 i ET42
      TButton btLampkaOporyB;
      TButton btLampkaStycznB;
      TButton btLampkaWylSzybkiB;
      TButton btLampkaNadmPrzetwB;
      TButton btLampkaPrzetwB;      
 //KURS90 lampki jazdy bezoporowej dla EU04
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
    int LampkaUniversal3_typ; //ABu 030405 - swiecenie uzaleznione od: 0-nic, 1-obw.gl, 2-przetw.
    bool LampkaUniversal3_st;
    TButton btLampkaWentZaluzje;     //ET22
    TButton btLampkaOgrzewanieSkladu;
    TButton btLampkaSHP;
    TButton btLampkaCzuwaka;  //McZapkie-141102
    TButton btLampkaRezerwa;
//youBy - jakies dodatkowe lampki
    TButton btLampkaNapNastHam;
    TButton btLampkaSprezarka;
    TButton btLampkaSprezarkaB;
    TButton btLampkaBocznik1;
    TButton btLampkaBocznik2;
    TButton btLampkaBocznik3;
    TButton btLampkaBocznik4;
    TButton btLampkaRadiotelefon;
    TButton btLampkaHamienie;
    TButton btLampkaJazda; //Ra: nie u�ywane
//KURS90
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
    TButton btLampkaForward; //Ra: lampki w prz�d i w ty� dla komputerowych kabin
    TButton btLampkaBackward;

    TButton btCabLight; //hunter-171012: lampa oswietlajaca kabine
    //Ra 2013-12: wirtualne "lampki" do odbijania na haslerze w PoKeys
    TButton btHaslerBrakes; //ci�nienie w cylindrach
    TButton btHaslerCurrent; //pr�d na silnikach

    vector3 pPosition;
    vector3 pMechOffset; //driverNpos
    vector3 vMechMovement;
    vector3 pMechPosition;
    vector3 pMechShake;
    vector3 vMechVelocity;
//McZapkie: do poruszania sie po kabinie
    double fMechCroach;
//McZapkie: opis kabiny - obszar poruszania sie mechanika oraz zajetosc
    TCab Cabine[maxcab+1];  //przedzial maszynowy, kabina 1 (A), kabina 2 (B)
    int iCabn;
    TSpring MechSpring;
    double fMechSpringX;  //McZapkie-250303: parametry bujania
    double fMechSpringY;
    double fMechSpringZ;
    double fMechMaxSpring;
    double fMechRoll;
    double fMechPitch;

    PSound dsbNastawnikJazdy;
    PSound dsbNastawnikBocz; //hunter-081211
    PSound dsbRelay;
    PSound dsbPneumaticRelay;
    PSound dsbSwitch;
    PSound dsbPneumaticSwitch;
    PSound dsbReverserKey; //hunter-121211

    PSound dsbCouplerAttach; //Ra: w kabinie????
    PSound dsbCouplerDetach; //Ra: w kabinie???

    PSound dsbDieselIgnition; //Ra: w kabinie???

    PSound dsbDoorClose; //Ra: w kabinie???
    PSound dsbDoorOpen; //Ra: w kabinie???

//Winger 010304
    PSound dsbPantUp;
    PSound dsbPantDown;

    PSound dsbWejscie_na_bezoporow;
    PSound dsbWejscie_na_drugi_uklad; //hunter-081211: poprawka literowki


//    PSound dsbHiss1;
  //  PSound dsbHiss2;

//McZapkie-280302
    TRealSound rsBrake;
    TRealSound rsSlippery;
    TRealSound rsHiss;  //upuszczanie
    TRealSound rsHissU; //napelnianie
    TRealSound rsHissE; //nagle
    TRealSound rsHissX; //fala
    TRealSound rsHissT; //czasowy
    TRealSound rsSBHiss;
    TRealSound rsRunningNoise;
    TRealSound rsEngageSlippery;
    TRealSound rsFadeSound;

    PSound dsbHasler;
    PSound dsbBuzzer;
    PSound dsbSlipAlarm; //Bombardier 011010: alarm przy poslizgu dla 181/182
    //TFadeSound sConverter;  //przetwornica
    //TFadeSound sSmallCompressor;  //przetwornica

    int iCabLightFlag; //McZapkie:120503: oswietlenie kabiny (0: wyl, 1: przyciemnione, 2: pelne)
    bool bCabLight; //hunter-091012: czy swiatlo jest zapalone?
    bool bCabLightDim; //hunter-091012: czy przyciemnienie kabiny jest zapalone?

    vector3 pMechSittingPosition; //ABu 180404
 vector3 __fastcall MirrorPosition(bool lewe);
private:
    //PSound dsbBuzzer;
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
    float fConverterTimer;  //hunter-261211: dla przekaznika
    float fMainRelayTimer;  //hunter-141211: zalaczanie WSa z opoznieniem
    float fCzuwakTestTimer;     //hunter-091012: do testu czuwaka

    int CAflag; //hunter-131211: dla osobnego zbijania CA i SHP

    double fPoslizgTimer;
//    double fShpTimer;
//    double fDblClickTimer;
    //ABu: Przeniesione do public. - Wiem, ze to nieladnie...
    //bool CabChange(int iDirection);
    //bool InitializeCab(int NewCabNo, AnsiString asFileName);
    TTrack *tor;
    int keybrakecount;
//McZapkie-240302 - przyda sie do tachometru
    float fTachoVelocity;
    float fTachoVelocityJump; //ze skakaniem
    float fTachoTimer;
    float fTachoCount;
    float fHVoltage; //napi�cie dla dynamicznych ga�ek
    float fHCurrent[4]; //pr�dy: suma i amperomierze 1,2,3
    float fEngine[4]; //obroty te� trzeba pobra�
//McZapkie: do syczenia
    float fPPress,fNPress;
    float fSPPress,fSNPress;
 int iSekunda; //Ra: sekunda aktualizacji pr�dko�ci
 int iRadioChannel; //numer aktualnego kana�u radiowego
public:
 int __fastcall RadioChannel() {return iRadioChannel;};
 inline TDynamicObject* __fastcall Dynamic() {return DynamicObject;};
 inline TMoverParameters* __fastcall Controlled() {return mvControlled;};
 void __fastcall DynamicSet(TDynamicObject *d);
 void __fastcall Silence();
};
//---------------------------------------------------------------------------
#endif
