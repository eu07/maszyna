// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'Oerlikon_ESt.pas' rev: 5.00

#ifndef Oerlikon_EStHPP
#define Oerlikon_EStHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <hamulce.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Oerlikon_est
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TNESt3;
class PASCALIMPLEMENTATION TNESt3 : public Hamulce::TBrake 
{
	typedef Hamulce::TBrake inherited;
	
private:
	double Nozzles[6];
	Hamulce::TReservoir* CntrlRes;
	double BVM;
	Byte ValveFlag;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	void __fastcall EStParams(double i_crc);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetCRP(void);
	void __fastcall CheckState(double BCP, double &dV1);
	void __fastcall CheckReleaser(double dt);
	double __fastcall CVs(double bp);
	double __fastcall BVs(double BCP);
	void __fastcall SetSize(int size);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TNESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc
		, Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : Hamulce::TBrake(i_mbp, i_bcr, i_bcd
		, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TNESt3(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint dMAX = 0x5;
static const Shortint dON = 0x0;
static const Shortint dOO = 0x1;
static const Shortint dTN = 0x2;
static const Shortint dTO = 0x3;
static const Shortint dP = 0x4;
static const Shortint dS = 0x5;

}	/* namespace Oerlikon_est */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Oerlikon_est;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// Oerlikon_ESt
