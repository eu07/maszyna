// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mtable.pas' rev: 5.00

#ifndef mtableHPP
#define mtableHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mtable
{
//-- type declarations -------------------------------------------------------
struct TMTableLine
{
	double km;
	double vmax;
	AnsiString StationName;
	AnsiString StationWare;
	Byte TrackNo;
	int Ah;
	int Am;
	int Dh;
	int Dm;
	double tm;
	int WaitTime;
} ;

typedef TMTableLine TMTable[101];

class DELPHICLASS TTrainParameters;
typedef TTrainParameters* *PTrainParameters;

class PASCALIMPLEMENTATION TTrainParameters : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	AnsiString TrainName;
	double TTVmax;
	AnsiString Relation1;
	AnsiString Relation2;
	double BrakeRatio;
	AnsiString LocSeries;
	double LocLoad;
	TMTableLine TimeTable[101];
	int StationCount;
	int StationIndex;
	AnsiString NextStationName;
	double LastStationLatency;
	int Direction;
	double __fastcall CheckTrainLatency(void);
	AnsiString __fastcall ShowRelation();
	double __fastcall WatchMTable(double DistCounter);
	AnsiString __fastcall NextStop();
	bool __fastcall IsStop(void);
	bool __fastcall IsTimeToGo(double hh, double mm);
	bool __fastcall UpdateMTable(double hh, double mm, AnsiString NewName);
	__fastcall TTrainParameters(AnsiString NewTrainName);
	void __fastcall NewName(AnsiString NewTrainName);
	bool __fastcall LoadTTfile(AnsiString scnpath, int iPlus, double vMax);
	bool __fastcall DirectionChange(void);
	void __fastcall StationIndexInc(void);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TTrainParameters(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TTrainParameters(void) { }
	#pragma option pop
	
};


class DELPHICLASS TMTableTime;
class PASCALIMPLEMENTATION TMTableTime : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	double GameTime;
	int dd;
	int hh;
	int mm;
	int srh;
	int srm;
	int ssh;
	int ssm;
	double mr;
	void __fastcall UpdateMTableTime(double deltaT);
	__fastcall TMTableTime(int InitH, int InitM, int InitSRH, int InitSRM, int InitSSH, int InitSSM);
public:
		
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TMTableTime(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TMTableTime(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint MaxTTableSize = 0x64;
static const char hrsd = '\x2e';
extern PACKAGE TMTableTime* GlobalTime;

}	/* namespace Mtable */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Mtable;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mtable
