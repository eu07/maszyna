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
#include <friction.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Oerlikon_est
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TPrzekladnik;
class PASCALIMPLEMENTATION TPrzekladnik : public Hamulce::TReservoir 
{
	typedef Hamulce::TReservoir inherited;
	
public:
	Hamulce::TReservoir* *BrakeRes;
	Hamulce::TReservoir* *Next;
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TPrzekladnik(void) : Hamulce::TReservoir() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPrzekladnik(void) { }
	#pragma option pop
	
};


class DELPHICLASS TRura;
class PASCALIMPLEMENTATION TRura : public TPrzekladnik 
{
	typedef TPrzekladnik inherited;
	
public:
	virtual double __fastcall P(void);
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TRura(void) : TPrzekladnik() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TRura(void) { }
	#pragma option pop
	
};


class DELPHICLASS TPrzeciwposlizg;
class PASCALIMPLEMENTATION TPrzeciwposlizg : public TRura 
{
	typedef TRura inherited;
	
private:
	bool Poslizg;
	
public:
	void __fastcall SetPoslizg(bool flag);
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TPrzeciwposlizg(void) : TRura() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPrzeciwposlizg(void) { }
	#pragma option pop
	
};


class DELPHICLASS TRapid;
class PASCALIMPLEMENTATION TRapid : public TPrzekladnik 
{
	typedef TPrzekladnik inherited;
	
private:
	bool RapidStatus;
	double RapidMult;
	double DN;
	double DL;
	
public:
	void __fastcall SetRapidParams(double mult, double size);
	void __fastcall SetRapidStatus(bool rs);
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TRapid(void) : TPrzekladnik() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TRapid(void) { }
	#pragma option pop
	
};


class DELPHICLASS TPrzekCiagly;
class PASCALIMPLEMENTATION TPrzekCiagly : public TPrzekladnik 
{
	typedef TPrzekladnik inherited;
	
private:
	double Mult;
	
public:
	void __fastcall SetMult(double m);
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TPrzekCiagly(void) : TPrzekladnik() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPrzekCiagly(void) { }
	#pragma option pop
	
};


class DELPHICLASS TPrzek_PZZ;
class PASCALIMPLEMENTATION TPrzek_PZZ : public TPrzekladnik 
{
	typedef TPrzekladnik inherited;
	
private:
	double LBP;
	
public:
	void __fastcall SetLBP(double P);
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TPrzek_PZZ(void) : TPrzekladnik() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPrzek_PZZ(void) { }
	#pragma option pop
	
};


class DELPHICLASS TPrzekZalamany;
class PASCALIMPLEMENTATION TPrzekZalamany : public TPrzekladnik 
{
	typedef TPrzekladnik inherited;
	
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TPrzekZalamany(void) : TPrzekladnik() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPrzekZalamany(void) { }
	#pragma option pop
	
};


class DELPHICLASS TPrzekED;
class PASCALIMPLEMENTATION TPrzekED : public TRura 
{
	typedef TRura inherited;
	
private:
	double MaxP;
	
public:
	void __fastcall SetP(double P);
	virtual void __fastcall Update(double dt);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TPrzekED(void) : TRura() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TPrzekED(void) { }
	#pragma option pop
	
};


class DELPHICLASS TNESt3;
class PASCALIMPLEMENTATION TNESt3 : public Hamulce::TBrake 
{
	typedef Hamulce::TBrake inherited;
	
private:
	double Nozzles[11];
	Hamulce::TReservoir* CntrlRes;
	double BVM;
	bool Zamykajacy;
	bool Przys_blok;
	Hamulce::TReservoir* Miedzypoj;
	TPrzekladnik* Przekladniki[3];
	bool RapidStatus;
	bool RapidStaly;
	double LoadC;
	double TareM;
	double LoadM;
	double TareBP;
	double HBG300;
	double Podskok;
	bool autom;
	double LBP;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	void __fastcall EStParams(double i_crc);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetCRP(void);
	void __fastcall CheckState(double BCP, double &dV1);
	void __fastcall CheckReleaser(double dt);
	double __fastcall CVs(double bp);
	double __fastcall BVs(double BCP);
	void __fastcall SetSize(int size, AnsiString params);
	void __fastcall PLC(double mass);
	void __fastcall SetLP(double TM, double LM, double TBP);
	virtual void __fastcall ForceEmptiness(void);
	void __fastcall SetLBP(double P);
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
static const Shortint dMAX = 0xa;
static const Shortint dON = 0x0;
static const Shortint dOO = 0x1;
static const Shortint dTN = 0x2;
static const Shortint dTO = 0x3;
static const Shortint dP = 0x4;
static const Shortint dSd = 0x5;
static const Shortint dSm = 0x6;
static const Shortint dPd = 0x7;
static const Shortint dPm = 0x8;
static const Shortint dPO = 0x9;
static const Shortint dPT = 0xa;
static const Shortint p_none = 0x0;
static const Shortint p_rapid = 0x1;
static const Shortint p_pp = 0x2;
static const Shortint p_al2 = 0x3;
static const Shortint p_ppz = 0x4;
static const Shortint P_ed = 0x5;
extern PACKAGE double __fastcall d2A(double d);

}	/* namespace Oerlikon_est */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Oerlikon_est;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// Oerlikon_ESt
