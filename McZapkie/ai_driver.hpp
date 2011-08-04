// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ai_driver.pas' rev: 5.00

#ifndef ai_driverHPP
#define ai_driverHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <mctools.hpp>	// Pascal unit
#include <mover.hpp>	// Pascal unit
#include <mtable.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Ai_driver
{
//-- type declarations -------------------------------------------------------
#pragma option push -b-
enum TOrders { Wait_for_orders, Prepare_engine, Shunt, Change_direction, Obey_train, Release_engine, 
	Jump_to_first_order };
#pragma option pop

class DELPHICLASS TController;
class PASCALIMPLEMENTATION TController : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	bool EngineActive;
	Mover::TLocation MechLoc;
	Mover::TRotation MechRot;
	bool Psyche;
	double ReactionTime;
	bool Ready;
	double LastUpdatedTime;
	double ElapsedTime;
	double deltalog;
	double LastReactionTime;
	bool HelpMeFlag;
	bool AIControllFlag;
	bool OnStationFlag;
	Mover::TMoverParameters* *Controlling;
	Mtable::TTrainParameters* *TrainSet;
	int TrainNumber;
	AnsiString OrderCommand;
	double OrderValue;
	double AccPreferred;
	double AccDesired;
	double VelDesired;
	double VelforDriver;
	double VelActual;
	double VelNext;
	double ProximityDist;
	double ActualProximityDist;
	Mover::TLocation CommandLocation;
	TOrders OrderList[17];
	Byte OrderPos;
	TextFile LogFile;
	TextFile AILogFile;
	bool ScanMe;
	bool MaxVelFlag;
	bool MinVelFlag;
	int ChangeDir;
	int ChangeDirOrder;
	int VehicleCount;
	bool Prepare2press;
	int DriverFailCount;
	bool Need_TryAgain;
	bool Need_BrakeRelease;
	double MinProximityDist;
	double MaxProximityDist;
	bool bCheckSKP;
	void __fastcall SetDriverPsyche(void);
	bool __fastcall PrepareEngine(void);
	bool __fastcall ReleaseEngine(void);
	bool __fastcall IncBrake(void);
	bool __fastcall DecBrake(void);
	bool __fastcall IncSpeed(void);
	bool __fastcall DecSpeed(void);
	void __fastcall RecognizeCommand(void);
	void __fastcall PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2, const Mover::TLocation 
		&NewLocation);
	bool __fastcall UpdateSituation(double dt);
	void __fastcall SetVelocity(double NewVel, double NewVelNext);
	bool __fastcall SetProximityVelocity(double NewDist, double NewVelNext);
	void __fastcall JumpToNextOrder(void);
	void __fastcall JumpToFirstOrder(void);
	void __fastcall ChangeOrder(TOrders NewOrder);
	TOrders __fastcall GetCurrentOrder(void);
	bool __fastcall CheckSKP(void);
	void __fastcall ResetSKP(void);
	void __fastcall CloseLog(void);
	__fastcall TController(const Mover::TLocation &LocInitial, const Mover::TRotation &RotInitial, bool 
		AI, Mover::PMoverParameters NewControll, Mtable::PTrainParameters NewTrainSet, bool InitPsyche);
	AnsiString __fastcall OrderCurrent();
	
private:
	AnsiString VehicleName;
	double VelMargin;
	double WarningDuration;
	double WaitingTime;
	double WaitingExpireTime;
	int __fastcall OrderDirectionChange(int newdir, Mover::PMoverParameters Vehicle);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TController(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TController(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const bool Aggressive = true;
static const bool Easyman = false;
static const bool AIdriver = true;
static const bool Humandriver = false;
static const Shortint maxorders = 0x10;
static const Shortint maxdriverfails = 0x4;
extern PACKAGE bool WriteLogFlag;

}	/* namespace Ai_driver */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Ai_driver;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ai_driver
