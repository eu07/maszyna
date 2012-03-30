// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) '_mover.pas' rev: 5.00

#ifndef _moverHPP
#define _moverHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace _mover
{
//-- type declarations -------------------------------------------------------
struct TLocation
{
	double X;
	double Y;
	double Z;
} ;

struct TRotation
{
	double Rx;
	double Ry;
	double Rz;
} ;

struct TDimension
{
	double W;
	double L;
	double H;
} ;

struct TCommand
{
	AnsiString Command;
	double Value1;
	double Value2;
	TLocation Location;
} ;

struct TTrackShape
{
	double R;
	double Len;
	double dHtrack;
	double dHrail;
} ;

struct TTrackParam
{
	double Width;
	double friction;
	Byte CategoryFlag;
	Byte QualityFlag;
	Byte DamageFlag;
	double Velmax;
} ;

struct TTractionParam
{
	double TractionVoltage;
	double TractionFreq;
	double TractionMaxCurrent;
	double TractionResistivity;
} ;

#pragma option push -b-
enum TBrakeSystem { Individual, Pneumatic, ElectroPneumatic };
#pragma option pop

#pragma option push -b-
enum TBrakeSubsystem { Standard, WeLu, Knorr, KE, Hik, Kk, Oerlikon };
#pragma option pop

#pragma option push -b-
enum TLocalBrake { NoBrake, ManualBrake, PneumaticBrake, HydraulicBrake };
#pragma option pop

typedef double TBrakeDelayTable[4];

struct TBrakePressure
{
	double PipePressureVal;
	double BrakePressureVal;
	double FlowSpeedVal;
	TBrakeSystem BrakeType;
} ;

typedef TBrakePressure TBrakePressureTable[13];

#pragma option push -b-
enum TEngineTypes { None, Dumb, WheelsDriven, ElectricSeriesMotor, DieselEngine, SteamEngine, DieselElectric 
	};
#pragma option pop

#pragma option push -b-
enum TPowerType { NoPower, BioPower, MechPower, ElectricPower, SteamPower };
#pragma option pop

#pragma option push -b-
enum TFuelType { Undefined, Coal, Oil };
#pragma option pop

struct TGrateType
{
	TFuelType FuelType;
	double GrateSurface;
	double FuelTransportSpeed;
	double IgnitionTemperature;
	double MaxTemperature;
} ;

struct TBoilerType
{
	double BoilerVolume;
	double BoilerHeatSurface;
	double SuperHeaterSurface;
	double MaxWaterVolume;
	double MinWaterVolume;
	double MaxPressure;
} ;

struct TCurrentCollector
{
	double MinH;
	double MaxH;
	double CSW;
} ;

#pragma option push -b-
enum TPowerSource { NotDefined, InternalSource, Transducer, Generator, Accumulator, CurrentCollector, 
	PowerCable, Heater };
#pragma option pop

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
			TGrateType Grate;
			TBoilerType Boiler;
			
		};
		struct 
		{
			TPowerType PowerTrans;
			double SteamPressure;
			
		};
		struct 
		{
			Byte CollectorsNo;
			TCurrentCollector CollectorParameters;
			
		};
		struct 
		{
			double MaxCapacity;
			TPowerSource RechargeSource;
			
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
} ;

struct TScheme
{
	Byte Relay;
	double R;
	Byte Bn;
	Byte Mn;
	bool AutoSwitch;
	Byte ScndAct;
} ;

typedef TScheme TSchemeTable[65];

struct TDEScheme
{
	double RPM;
	double GenPower;
	double Umax;
	double Imax;
} ;

typedef TDEScheme TDESchemeTable[33];

struct TShuntScheme
{
	double Umin;
	double Umax;
	double Pmin;
	double Pmax;
} ;

typedef TShuntScheme TShuntSchemeTable[33];

struct TMPTRelay
{
	double Iup;
	double Idown;
} ;

typedef TMPTRelay TMPTRelayTable[8];

struct TMotorParameters
{
	double mfi;
	double mIsat;
	double fi;
	double Isat;
	bool AutoSwitch;
} ;

struct TSecuritySystem
{
	Byte SystemType;
	double AwareDelay;
	double SoundSignalDelay;
	double EmergencyBrakeDelay;
	Byte Status;
	double SystemTimer;
	double SystemSoundTimer;
	double SystemBrakeTimer;
	int VelocityAllowed;
	int NextVelocityAllowed;
	bool RadioStop;
} ;

struct TTransmision
{
	Byte NToothM;
	Byte NToothW;
	double Ratio;
} ;

#pragma option push -b-
enum TCouplerType { NoCoupler, Articulated, Bare, Chain, Screw, Automatic };
#pragma option pop

class DELPHICLASS T_MoverParameters;
struct TCoupling;
class PASCALIMPLEMENTATION T_MoverParameters : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	double dMoveLen;
	AnsiString filename;
	Byte CategoryFlag;
	AnsiString TypeName;
	int TrainType;
	TEngineTypes EngineType;
	TPowerParameters EnginePowerSource;
	TPowerParameters SystemPowerSource;
	TPowerParameters HeatingPowerSource;
	TPowerParameters AlterHeatPowerSource;
	TPowerParameters LightPowerSource;
	TPowerParameters AlterLightPowerSource;
	double Vmax;
	double Mass;
	double Power;
	double Mred;
	double TotalMass;
	double HeatingPower;
	double LightPower;
	int BatteryVoltage;
	TDimension Dim;
	double Cx;
	double WheelDiameter;
	double WheelDiameterL;
	double WheelDiameterT;
	double TrackW;
	double AxleInertialMoment;
	AnsiString AxleArangement;
	Byte NPoweredAxles;
	Byte NAxles;
	Byte BearingType;
	double ADist;
	double BDist;
	Byte NBpA;
	int SandCapacity;
	TBrakeSystem BrakeSystem;
	TBrakeSubsystem BrakeSubsystem;
	double MBPM;
	TLocalBrake LocalBrake;
	TBrakePressure BrakePressureTable[13];
	Byte ASBType;
	Byte TurboTest;
	double MaxBrakeForce;
	double MaxBrakePress;
	double P2FTrans;
	double TrackBrakeForce;
	Byte BrakeMethod;
	double HighPipePress;
	double LowPipePress;
	double DeltaPipePress;
	double CntrlPipePress;
	double BrakeVolume;
	double BrakeVVolume;
	double VeselVolume;
	int BrakeCylNo;
	double BrakeCylRadius;
	double BrakeCylDist;
	double BrakeCylMult[4];
	Byte BCMFlag;
	double MinCompressor;
	double MaxCompressor;
	double CompressorSpeed;
	double BrakeDelay[4];
	Byte BrakeCtrlPosNo;
	Byte MainCtrlPosNo;
	Byte ScndCtrlPosNo;
	bool ScndInMain;
	TSecuritySystem SecuritySystem;
	TScheme RList[65];
	int RlistSize;
	TMotorParameters MotorParam[9];
	TTransmision Transmision;
	double NominalVoltage;
	double WindingRes;
	double CircuitRes;
	int IminLo;
	int IminHi;
	int ImaxLo;
	int ImaxHi;
	double nmax;
	double InitialCtrlDelay;
	double CtrlDelay;
	Byte AutoRelayType;
	bool CoupledCtrl;
	bool IsCoupled;
	Byte DynamicBrakeType;
	Byte RVentType;
	double RVentnmax;
	double RVentCutOff;
	int CompressorPower;
	int SmallCompressorPower;
	double dizel_Mmax;
	double dizel_nMmax;
	double dizel_Mnmax;
	double dizel_nmax;
	double dizel_nominalfill;
	double dizel_Mstand;
	double dizel_nmax_cutoff;
	double dizel_nmin;
	double dizel_minVelfullengage;
	double dizel_AIM;
	double dizel_engageDia;
	double dizel_engageMaxForce;
	double dizel_engagefriction;
	double AnPos;
	bool AnalogCtrl;
	bool AnMainCtrl;
	bool ShuntModeAllow;
	bool ShuntMode;
	bool Flat;
	double Vhyp;
	TDEScheme DElist[33];
	double Vadd;
	TMPTRelay MPTRelay[8];
	Byte RelayType;
	TShuntScheme SST[33];
	double PowerCorRatio;
	double Ftmax;
	int MaxLoad;
	AnsiString LoadAccepted;
	AnsiString LoadQuantity;
	double OverLoadFactor;
	double LoadSpeed;
	double UnLoadSpeed;
	Byte DoorOpenCtrl;
	Byte DoorCloseCtrl;
	double DoorStayOpen;
	bool DoorClosureWarning;
	double DoorOpenSpeed;
	double DoorCloseSpeed;
	double DoorMaxShiftL;
	double DoorMaxShiftR;
	Byte DoorOpenMethod;
	TLocation Loc;
	TRotation Rot;
	AnsiString Name;
	TCoupling Couplers[2];
	int ScanCounter;
	bool EventFlag;
	Byte SoundFlag;
	double DistCounter;
	double V;
	double Vel;
	double AccS;
	double AccN;
	double AccV;
	double nrot;
	double EnginePower;
	double dL;
	double Fb;
	double Ff;
	double FTrain;
	double FStand;
	double FTotal;
	double UnitBrakeForce;
	bool SlippingWheels;
	bool SandDose;
	double Sand;
	double BrakeSlippingTimer;
	double dpBrake;
	double dpPipe;
	double dpMainValve;
	double dpLocalValve;
	double ScndPipePress;
	double BrakePress;
	double LocBrakePress;
	double PipeBrakePress;
	double PipePress;
	double PPP;
	double Volume;
	double CompressedVolume;
	double PantVolume;
	double Compressor;
	bool CompressorFlag;
	bool PantCompFlag;
	bool CompressorAllow;
	bool ConverterFlag;
	bool ConverterAllow;
	int BrakeCtrlPos;
	Byte LocalBrakePos;
	Byte BrakeStatus;
	bool EmergencyBrakeFlag;
	Byte BrakeDelayFlag;
	Byte BrakeDelays;
	bool DynamicBrakeFlag;
	double LimPipePress;
	double ActFlowSpeed;
	Byte DamageFlag;
	Byte DerailReason;
	TCommand CommandIn;
	AnsiString CommandOut;
	AnsiString CommandLast;
	double ValueOut;
	TTrackShape RunningShape;
	TTrackParam RunningTrack;
	double OffsetTrackH;
	double OffsetTrackV;
	bool Mains;
	Byte MainCtrlPos;
	Byte ScndCtrlPos;
	int ActiveDir;
	int CabNo;
	int DirAbsolute;
	int ActiveCab;
	int LastCab;
	double LastSwitchingTime;
	Byte WarningSignal;
	bool DepartureSignal;
	bool InsideConsist;
	TTractionParam RunningTraction;
	double enrot;
	double Im;
	double Itot;
	double Mm;
	double Mw;
	double Fw;
	double Ft;
	int Imin;
	int Imax;
	double Voltage;
	Byte MainCtrlActualPos;
	Byte ScndCtrlActualPos;
	bool DelayCtrlFlag;
	double LastRelayTime;
	bool AutoRelayFlag;
	bool FuseFlag;
	bool ConvOvldFlag;
	bool StLinFlag;
	bool ResistorsFlag;
	double RventRot;
	bool UnBrake;
	double dizel_fill;
	double dizel_engagestate;
	double dizel_engage;
	double dizel_automaticgearstatus;
	bool dizel_enginestart;
	double dizel_engagedeltaomega;
	double PulseForce;
	double PulseForceTimer;
	int PulseForceCount;
	double eAngle;
	int Load;
	AnsiString LoadType;
	Byte LoadStatus;
	double LastLoadChangeTime;
	bool DoorLeftOpened;
	bool DoorRightOpened;
	bool PantFrontUp;
	bool PantRearUp;
	bool PantFrontSP;
	bool PantRearSP;
	int PantFrontStart;
	int PantRearStart;
	bool PantFrontVolt;
	bool PantRearVolt;
	AnsiString PantSwitchType;
	AnsiString ConvSwitchType;
	bool Heating;
	int DoubleTr;
	bool PhysicActivation;
	double FrictConst1;
	double FrictConst2s;
	double FrictConst2d;
	double TotalMassxg;
	bool __fastcall GetTrainsetVoltage(void);
	bool __fastcall Physic_ReActivation(void);
	void __fastcall PantCheck(void);
	double __fastcall LocalBrakeRatio(void);
	double __fastcall PipeRatio(void);
	double __fastcall RealPipeRatio(void);
	double __fastcall BrakeVP(void);
	bool __fastcall DynamicBrakeSwitch(bool Switch);
	bool __fastcall SendCtrlBroadcast(AnsiString CtrlCommand, double ctrlvalue);
	bool __fastcall SendCtrlToNext(AnsiString CtrlCommand, double ctrlvalue, double dir);
	bool __fastcall CabActivisation(void);
	bool __fastcall CabDeactivisation(void);
	bool __fastcall IncMainCtrl(int CtrlSpeed);
	bool __fastcall DecMainCtrl(int CtrlSpeed);
	bool __fastcall IncScndCtrl(int CtrlSpeed);
	bool __fastcall DecScndCtrl(int CtrlSpeed);
	bool __fastcall AddPulseForce(int Multipler);
	bool __fastcall SandDoseOn(void);
	bool __fastcall SecuritySystemReset(void);
	void __fastcall SecuritySystemCheck(double dt);
	bool __fastcall IncBrakeLevel(void);
	bool __fastcall DecBrakeLevel(void);
	bool __fastcall IncLocalBrakeLevel(Byte CtrlSpeed);
	bool __fastcall DecLocalBrakeLevel(Byte CtrlSpeed);
	bool __fastcall IncLocalBrakeLevelFAST(void);
	bool __fastcall DecLocalBrakeLevelFAST(void);
	bool __fastcall EmergencyBrakeSwitch(bool Switch);
	bool __fastcall AntiSlippingBrake(void);
	bool __fastcall BrakeReleaser(void);
	bool __fastcall SwitchEPBrake(Byte state);
	bool __fastcall AntiSlippingButton(void);
	bool __fastcall IncBrakePress(double &brake, double PressLimit, double dp);
	bool __fastcall DecBrakePress(double &brake, double PressLimit, double dp);
	bool __fastcall BrakeDelaySwitch(Byte BDS);
	bool __fastcall IncBrakeMult(void);
	bool __fastcall DecBrakeMult(void);
	void __fastcall UpdateBrakePressure(double dt);
	void __fastcall UpdatePipePressure(double dt);
	void __fastcall CompressorCheck(double dt);
	void __fastcall UpdatePantVolume(double dt);
	void __fastcall UpdateScndPipePressure(double dt);
	void __fastcall ComputeConstans(void);
	double __fastcall ComputeMass(void);
	double __fastcall Adhesive(double staticfriction);
	double __fastcall TractionForce(double dt);
	double __fastcall FrictionForce(double R, Byte TDamage);
	double __fastcall BrakeForce(const TTrackParam &Track);
	double __fastcall CouplerForce(Byte CouplerN, double dt);
	void __fastcall CollisionDetect(Byte CouplerN, double dt);
	double __fastcall ComputeRotatingWheel(double WForce, double dt, double n);
	bool __fastcall SetInternalCommand(AnsiString NewCommand, double NewValue1, double NewValue2);
	double __fastcall GetExternalCommand(AnsiString &Command);
	bool __fastcall RunCommand(AnsiString command, double CValue1, double CValue2);
	bool __fastcall RunInternalCommand(void);
	void __fastcall PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2, const TLocation 
		&NewLocation);
	bool __fastcall DirectionForward(void);
	bool __fastcall DirectionBackward(void);
	bool __fastcall MainSwitch(bool State);
	bool __fastcall ChangeCab(int direction);
	bool __fastcall ConverterSwitch(bool State);
	bool __fastcall CompressorSwitch(bool State);
	void __fastcall ConverterCheck(void);
	bool __fastcall FuseOn(void);
	bool __fastcall FuseFlagCheck(void);
	void __fastcall FuseOff(void);
	int __fastcall ShowCurrent(Byte AmpN);
	int __fastcall ShowEngineRotation(Byte VehN);
	double __fastcall v2n(void);
	double __fastcall current(double n, double U);
	double __fastcall Momentum(double I);
	bool __fastcall CutOffEngine(void);
	bool __fastcall MaxCurrentSwitch(bool State);
	bool __fastcall ResistorsFlagCheck(void);
	bool __fastcall MinCurrentSwitch(bool State);
	bool __fastcall AutoRelaySwitch(bool State);
	bool __fastcall AutoRelayCheck(void);
	bool __fastcall dizel_EngageSwitch(double state);
	bool __fastcall dizel_EngageChange(double dt);
	bool __fastcall dizel_AutoGearCheck(void);
	double __fastcall dizel_fillcheck(Byte mcp);
	double __fastcall dizel_Momentum(double dizel_fill, double n, double dt);
	bool __fastcall dizel_Update(double dt);
	bool __fastcall LoadingDone(double LSpeed, AnsiString LoadInit);
	void __fastcall ComputeTotalForce(double dt, double dt1, bool FullVer);
	double __fastcall ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track
		, TTractionParam &ElectricTraction, const TLocation &NewLoc, TRotation &NewRot);
	double __fastcall FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track, const 
		TLocation &NewLoc, TRotation &NewRot);
	bool __fastcall ChangeOffsetH(double DeltaOffset);
	__fastcall T_MoverParameters(double VelInitial, AnsiString TypeNameInit, AnsiString NameInit, int LoadInitial
		, AnsiString LoadTypeInitial, int Cab);
	bool __fastcall LoadChkFile(AnsiString chkpath);
	bool __fastcall CheckLocomotiveParameters(bool ReadyFlag, int Dir);
	AnsiString __fastcall EngineDescription(int what);
	bool __fastcall DoorLeft(bool State);
	bool __fastcall DoorRight(bool State);
	bool __fastcall PantFront(bool State);
	bool __fastcall PantRear(bool State);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall T_MoverParameters(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~T_MoverParameters(void) { }
	#pragma option pop
	
};


struct TCoupling
{
	double SpringKB;
	double SpringKC;
	double beta;
	double DmaxB;
	double FmaxB;
	double DmaxC;
	double FmaxC;
	TCouplerType CouplerType;
	Byte CouplingFlag;
	Byte AllowedFlag;
	bool Render;
	double CoupleDist;
	T_MoverParameters* Connected;
	Byte ConnectedNr;
	double CForce;
	double Dist;
	bool CheckCollision;
} ;

//-- var, const, procedure ---------------------------------------------------
static const bool Go = true;
static const bool Hold = false;
static const Shortint ResArraySize = 0x40;
static const Shortint MotorParametersArraySize = 0x8;
static const Shortint maxcc = 0x4;
static const Shortint LocalBrakePosNo = 0xa;
static const Shortint MainBrakeMaxPos = 0xa;
static const Shortint dtrack_railwear = 0x2;
static const Shortint dtrack_freerail = 0x4;
static const Shortint dtrack_thinrail = 0x8;
static const Shortint dtrack_railbend = 0x10;
static const Shortint dtrack_plants = 0x20;
static const Shortint dtrack_nomove = 0x40;
static const Byte dtrack_norail = 0x80;
static const Shortint dtrain_thinwheel = 0x1;
static const Shortint dtrain_loadshift = 0x1;
static const Shortint dtrain_wheelwear = 0x2;
static const Shortint dtrain_bearing = 0x4;
static const Shortint dtrain_coupling = 0x8;
static const Shortint dtrain_ventilator = 0x10;
static const Shortint dtrain_loaddamage = 0x10;
static const Shortint dtrain_engine = 0x20;
static const Shortint dtrain_loaddestroyed = 0x20;
static const Shortint dtrain_axle = 0x40;
static const Byte dtrain_out = 0x80;
#define p_elengproblem  (1.000000E-02)
#define p_elengdamage  (1.000000E-01)
#define p_coupldmg  (2.000000E-02)
#define p_derail  (1.000000E-03)
#define p_accn  (1.000000E-01)
#define p_slippdmg  (1.000000E-03)
static const Shortint ctrain_virtual = 0x0;
static const Shortint ctrain_coupler = 0x1;
static const Shortint ctrain_pneumatic = 0x2;
static const Shortint ctrain_controll = 0x4;
static const Shortint ctrain_power = 0x8;
static const Shortint ctrain_passenger = 0x10;
static const Shortint ctrain_scndpneumatic = 0x20;
static const Shortint ctrain_heating = 0x40;
static const Shortint dbrake_none = 0x0;
static const Shortint dbrake_passive = 0x1;
static const Shortint dbrake_switch = 0x2;
static const Shortint dbrake_reversal = 0x4;
static const Shortint dbrake_automatic = 0x8;
static const Shortint bdelay_P = 0x0;
static const Shortint bdelay_G = 0x1;
static const Shortint bdelay_R = 0x2;
static const Shortint bdelay_E = 0x4;
static const Shortint b_off = 0x0;
static const Shortint b_on = 0x1;
static const Shortint b_dmg = 0x2;
static const Shortint b_release = 0x4;
static const Shortint b_antislip = 0x8;
static const Shortint b_epused = 0x10;
static const Shortint b_Rused = 0x20;
static const Shortint b_Ractive = 0x40;
static const Shortint bp_classic = 0x0;
static const Shortint bp_diameter = 0x1;
static const Shortint bp_magnetic = 0x2;
static const Shortint s_waiting = 0x1;
static const Shortint s_aware = 0x2;
static const Shortint s_active = 0x4;
static const Shortint s_alarm = 0x8;
static const Shortint s_ebrake = 0x10;
static const Shortint sound_none = 0x0;
static const Shortint sound_loud = 0x1;
static const Shortint sound_couplerstretch = 0x2;
static const Shortint sound_bufferclamp = 0x4;
static const Shortint sound_bufferbump = 0x8;
static const Shortint sound_relay = 0x10;
static const Shortint sound_manyrelay = 0x20;
static const Shortint sound_brakeacc = 0x40;
#define Spg  (5.067000E-01)
extern PACKAGE bool PhysicActivationFlag;
static const Shortint dt_Default = 0x0;
static const Shortint dt_EZT = 0x1;
static const Shortint dt_ET41 = 0x2;
static const Shortint dt_ET42 = 0x4;
static const Shortint dt_PseudoDiesel = 0x8;
static const Shortint dt_ET22 = 0x10;
static const Shortint dt_SN61 = 0x20;
static const Shortint dt_181 = 0x40;
extern PACKAGE double __fastcall Distance(const TLocation &Loc1, const TLocation &Loc2, const TDimension 
	&Dim1, const TDimension &Dim2);

}	/* namespace _mover */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace _mover;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// _mover
