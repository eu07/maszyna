//---------------------------------------------------------------------------

#ifndef MoverH
#define MoverH
//---------------------------------------------------------------------------
#include "Mover.hpp"
// Ra: Niestety "_mover.hpp" siê nieprawid³owo generuje - przek³ada sobie TCoupling na sam koniec.
// Przy wszelkich poprawkach w "_mover.pas" trzeba skopiowaæ rêcznie "_mover.hpp" do "mover.hpp" i
// poprawiæ b³êdy! Tak a¿ do wydzielnia TCoupling z Pascala do C++...
// Docelowo obs³ugê sprzêgów (³¹czenie, roz³¹czanie, obliczanie odleg³oœci, przesy³ komend)
// trzeba przenieœæ na poziom DynObj.cpp.
// Obs³ugê silniników te¿ trzeba wydzieliæ do osobnego modu³u, bo ka¿dy osobno mo¿e mieæ poœlizg.
#include "dumb3d.h"
using namespace Math3D;

enum TProblem //lista problemów taboru, które uniemo¿liwiaj¹ jazdê
{//flagi bitowe
 pr_Hamuje=1, //pojazd ma za³¹czony hamulec lub zatarte osie
 pr_Pantografy=2, //pojazd wymaga napompowania pantografów
 pr_Ostatni=0x80000000 //ostatnia flaga bitowa
};

class TMoverParameters : public T_MoverParameters
{//Ra: wrapper na kod pascalowy, przejmuj¹cy jego funkcje
public:
 vector3 vCoulpler[2]; //powtórzenie wspó³rzêdnych sprzêgów z DynObj :/
 vector3 DimHalf; //po³owy rozmiarów do obliczeñ geometrycznych
 //int WarningSignal; //0: nie trabi, 1,2: trabi syren¹ o podanym numerze
 unsigned char WarningSignal; //tymczasowo 8bit, ze wzglêdu na funkcje w MTools
 double fBrakeCtrlPos; //p³ynna nastawa hamulca zespolonego
 bool bPantKurek3; //kurek trójdrogowy (pantografu): true=po³¹czenie z ZG, false=po³¹czenie z ma³¹ sprê¿ark¹
 int iProblem; //flagi problemów z taborem, aby AI nie musia³o porównywaæ; 0=mo¿e jechaæ
 int iLights[2]; //bity zapalonych œwiate³ tutaj, ¿eby da³o siê liczyæ pobór pr¹du 
private:
 double __fastcall CouplerDist(Byte Coupler);
public:
 __fastcall TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
  int LoadInitial,AnsiString LoadTypeInitial,int Cab);
 //obs³uga sprzêgów
 double __fastcall Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2);
 double __fastcall Distance(const vector3 &Loc1,const vector3 &Loc2,const vector3 &Dim1,const vector3 &Dim2);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType,bool Forced=false);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType,bool Forced=false);
 int __fastcall DettachStatus(Byte ConnectNo);
 bool __fastcall Dettach(Byte ConnectNo);
 void __fastcall SetCoupleDist();
 bool __fastcall DirectionForward();
 void __fastcall BrakeLevelSet(double b);
 bool __fastcall BrakeLevelAdd(double b);
 bool __fastcall IncBrakeLevel(); //wersja na u¿ytek AI
 bool __fastcall DecBrakeLevel();
 bool __fastcall ChangeCab(int direction);
 bool __fastcall CurrentSwitch(int direction);
 void __fastcall UpdateBatteryVoltage(double dt);
 double __fastcall ComputeMovement
 (double dt,double dt1,
  const TTrackShape &Shape,TTrackParam &Track,TTractionParam &ElectricTraction,
  const TLocation &NewLoc,TRotation &NewRot
 );
 double __fastcall FastComputeMovement
 (double dt,
  const TTrackShape &Shape,TTrackParam &Track,
  const	TLocation &NewLoc,TRotation &NewRot
 );
 double __fastcall ShowEngineRotation(int VehN);
 //double __fastcall GetTrainsetVoltage(void);
 //bool __fastcall Physic_ReActivation(void);
 //double __fastcall LocalBrakeRatio(void);
 //double __fastcall ManualBrakeRatio(void);
 //double __fastcall PipeRatio(void);
 //double __fastcall RealPipeRatio(void);
 //double __fastcall BrakeVP(void);
 //bool __fastcall DynamicBrakeSwitch(bool Switch);
 //bool __fastcall SendCtrlBroadcast(AnsiString CtrlCommand, double ctrlvalue);
 //bool __fastcall SendCtrlToNext(AnsiString CtrlCommand, double ctrlvalue, double dir);
 //bool __fastcall CabActivisation(void);
 //bool __fastcall CabDeactivisation(void);
 //bool __fastcall IncMainCtrl(int CtrlSpeed);
 //bool __fastcall DecMainCtrl(int CtrlSpeed);
 //bool __fastcall IncScndCtrl(int CtrlSpeed);
 //bool __fastcall DecScndCtrl(int CtrlSpeed);
 //bool __fastcall AddPulseForce(int Multipler);
 //bool __fastcall SandDoseOn(void);
 //bool __fastcall SecuritySystemReset(void);
 //void __fastcall SecuritySystemCheck(double dt);
 //bool __fastcall BatterySwitch(bool State);
 //bool __fastcall EpFuseSwitch(bool State);
 //bool __fastcall IncBrakeLevelOld(void);
 //bool __fastcall DecBrakeLevelOld(void);
 //bool __fastcall IncLocalBrakeLevel(Byte CtrlSpeed);
 //bool __fastcall DecLocalBrakeLevel(Byte CtrlSpeed);
 //bool __fastcall IncLocalBrakeLevelFAST(void);
 //bool __fastcall DecLocalBrakeLevelFAST(void);
 //bool __fastcall IncManualBrakeLevel(Byte CtrlSpeed);
 //bool __fastcall DecManualBrakeLevel(Byte CtrlSpeed);
 //bool __fastcall EmergencyBrakeSwitch(bool Switch);
 //bool __fastcall AntiSlippingBrake(void);
 //bool __fastcall BrakeReleaser(Byte state);
 //bool __fastcall SwitchEPBrake(Byte state);
 //bool __fastcall AntiSlippingButton(void);
 //bool __fastcall IncBrakePress(double &brake, double PressLimit, double dp);
 //bool __fastcall DecBrakePress(double &brake, double PressLimit, double dp);
 //bool __fastcall BrakeDelaySwitch(Byte BDS);
 //bool __fastcall IncBrakeMult(void);
 //bool __fastcall DecBrakeMult(void);
 //void __fastcall UpdateBrakePressure(double dt);
 //void __fastcall UpdatePipePressure(double dt);
 //void __fastcall CompressorCheck(double dt);
 void __fastcall UpdatePantVolume(double dt);
 //void __fastcall UpdateScndPipePressure(double dt);
 //void __fastcall UpdateBatteryVoltage(double dt);
 //double __fastcall GetDVc(double dt);
 //void __fastcall ComputeConstans(void);
 //double __fastcall ComputeMass(void);
 //double __fastcall Adhesive(double staticfriction);
 //double __fastcall TractionForce(double dt);
 //double __fastcall FrictionForce(double R, Byte TDamage);
 //double __fastcall BrakeForce(const TTrackParam &Track);
 //double __fastcall CouplerForce(Byte CouplerN, double dt);
 //void __fastcall CollisionDetect(Byte CouplerN, double dt);
 //double __fastcall ComputeRotatingWheel(double WForce, double dt, double n);
 //bool __fastcall SetInternalCommand(AnsiString NewCommand, double NewValue1, double NewValue2);
 //double __fastcall GetExternalCommand(AnsiString &Command);
 //bool __fastcall RunCommand(AnsiString command, double CValue1, double CValue2);
 //bool __fastcall RunInternalCommand(void);
 //void __fastcall PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2, const TLocation
 //	&NewLocation);
 //bool __fastcall DirectionBackward(void);
 //bool __fastcall MainSwitch(bool State);
 //bool __fastcall ConverterSwitch(bool State);
 //bool __fastcall CompressorSwitch(bool State);
 void __fastcall ConverterCheck();
 //bool __fastcall FuseOn(void);
 //bool __fastcall FuseFlagCheck(void);
 //void __fastcall FuseOff(void);
 //int __fastcall ShowCurrent(Byte AmpN);
 //double __fastcall v2n(void);
 //double __fastcall current(double n, double U);
 //double __fastcall Momentum(double I);
 //double __fastcall MomentumF(double I, double Iw, Byte SCP);
 //bool __fastcall CutOffEngine(void);
 //bool __fastcall MaxCurrentSwitch(bool State);
 //bool __fastcall ResistorsFlagCheck(void);
 //bool __fastcall MinCurrentSwitch(bool State);
 //bool __fastcall AutoRelaySwitch(bool State);
 //bool __fastcall AutoRelayCheck(void);
 //bool __fastcall dizel_EngageSwitch(double state);
 //bool __fastcall dizel_EngageChange(double dt);
 //bool __fastcall dizel_AutoGearCheck(void);
 //double __fastcall dizel_fillcheck(Byte mcp);
 //double __fastcall dizel_Momentum(double dizel_fill, double n, double dt);
 //bool __fastcall dizel_Update(double dt);
 //bool __fastcall LoadingDone(double LSpeed, AnsiString LoadInit);
 //void __fastcall ComputeTotalForce(double dt, double dt1, bool FullVer);
 //double __fastcall ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track
 //	, TTractionParam &ElectricTraction, const TLocation &NewLoc, TRotation &NewRot);
 //double __fastcall FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track, const
 //	TLocation &NewLoc, TRotation &NewRot);
 //bool __fastcall ChangeOffsetH(double DeltaOffset);
 //__fastcall T_MoverParameters(double VelInitial, AnsiString TypeNameInit, AnsiString NameInit, int LoadInitial
 //	, AnsiString LoadTypeInitial, int Cab);
 //bool __fastcall LoadChkFile(AnsiString chkpath);
 //bool __fastcall CheckLocomotiveParameters(bool ReadyFlag, int Dir);
 //AnsiString __fastcall EngineDescription(int what);
 //bool __fastcall DoorLeft(bool State);
 //bool __fastcall DoorRight(bool State);
 //bool __fastcall DoorBlockedFlag(void);
 //bool __fastcall PantFront(bool State);
 //bool __fastcall PantRear(bool State);
 //
};

#endif
