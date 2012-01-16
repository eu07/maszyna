// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'hamulce.pas' rev: 5.00

#ifndef hamulceHPP
#define hamulceHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <friction.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Hamulce
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TReservoir;
class PASCALIMPLEMENTATION TReservoir : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	double Cap;
	double Vol;
	double dVol;
	
public:
	__fastcall TReservoir(void);
	void __fastcall CreateCap(double Capacity);
	void __fastcall CreatePress(double Press);
	virtual double __fastcall pa(void);
	virtual double __fastcall P(void);
	void __fastcall Flow(double dv);
	void __fastcall Act(void);
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TReservoir(void) { }
	#pragma option pop
	
};


class DELPHICLASS TBrakeCyl;
class PASCALIMPLEMENTATION TBrakeCyl : public TReservoir 
{
	typedef TReservoir inherited;
	
public:
	virtual double __fastcall pa(void);
	virtual double __fastcall P(void);
public:
	#pragma option push -w-inl
	/* TReservoir.Create */ inline __fastcall TBrakeCyl(void) : TReservoir() { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TBrakeCyl(void) { }
	#pragma option pop
	
};


class DELPHICLASS TBrake;
class PASCALIMPLEMENTATION TBrake : public System::TObject 
{
	typedef System::TObject inherited;
	
private:
	TReservoir* BrakeCyl;
	TReservoir* BrakeRes;
	TReservoir* ValveRes;
	Byte BCN;
	double BCM;
	double BCA;
	Byte BrakeDelays;
	Byte BrakeDelayFlag;
	Friction::TFricMat* FM;
	double MaxBP;
	Byte BA;
	Byte NBpA;
	double SizeBR;
	double SizeBC;
	Byte BrakeStatus;
	
public:
	__fastcall TBrake(double i_mbp, double i_bcr, double i_bcd, double i_brc, Byte i_bcn, Byte i_BD, Byte 
		i_mat, Byte i_ba, Byte i_nbpa);
	virtual double __fastcall GetFC(double Vel, double N);
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	double __fastcall GetBCF(void);
	virtual double __fastcall GetHPFlow(double HP, double dt);
	virtual double __fastcall GetBCP(void);
	double __fastcall GetBRP(void);
	double __fastcall GetVRP(void);
	virtual double __fastcall GetCRP(void);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	bool __fastcall SetBDF(Byte nBDF);
	void __fastcall Releaser(Byte state);
	Byte __fastcall GetStatus(void);
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TBrake(void) { }
	#pragma option pop
	
};


class DELPHICLASS TWest;
class PASCALIMPLEMENTATION TWest : public TBrake 
{
	typedef TBrake inherited;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TWest(double i_mbp, double i_bcr, double i_bcd, double i_brc, 
		Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TWest(void) { }
	#pragma option pop
	
};


class DELPHICLASS TESt;
class PASCALIMPLEMENTATION TESt : public TBrake 
{
	typedef TBrake inherited;
	
private:
	TReservoir* CntrlRes;
	double BVM;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	void __fastcall EStParams(double i_crc);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetCRP(void);
	void __fastcall CheckState(double BCP, double &dV1);
	void __fastcall CheckReleaser(double dt);
	double __fastcall CVs(double bp);
	double __fastcall BVs(double BCP);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TESt(double i_mbp, double i_bcr, double i_bcd, double i_brc, 
		Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TESt(void) { }
	#pragma option pop
	
};


class DELPHICLASS TESt3;
class PASCALIMPLEMENTATION TESt3 : public TESt 
{
	typedef TESt inherited;
	
private:
	double CylFlowSpeed[2][2];
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc, 
		Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TESt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TESt3(void) { }
	#pragma option pop
	
};


class DELPHICLASS TESt3AL2;
class PASCALIMPLEMENTATION TESt3AL2 : public TESt3 
{
	typedef TESt3 inherited;
	
private:
	double TareM;
	double LoadM;
	double TareBP;
	double LoadC;
	
public:
	TReservoir* ImplsRes;
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	void __fastcall PLC(double mass);
	void __fastcall SetLP(double TM, double LM, double TBP);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TESt3AL2(double i_mbp, double i_bcr, double i_bcd, double i_brc
		, Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TESt3(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TESt3AL2(void) { }
	#pragma option pop
	
};


class DELPHICLASS TESt4R;
class PASCALIMPLEMENTATION TESt4R : public TESt 
{
	typedef TESt inherited;
	
private:
	bool RapidStatus;
	
public:
	TReservoir* ImplsRes;
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TESt4R(double i_mbp, double i_bcr, double i_bcd, double i_brc
		, Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TESt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TESt4R(void) { }
	#pragma option pop
	
};


class DELPHICLASS TLSt;
class PASCALIMPLEMENTATION TLSt : public TESt4R 
{
	typedef TESt4R inherited;
	
private:
	double CylFlowSpeed[2][2];
	double LBP;
	double RM;
	bool EDFlag;
	
public:
	void __fastcall SetLBP(double P);
	void __fastcall SetRM(double RMR);
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual double __fastcall GetHPFlow(double HP, double dt);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	double __fastcall GetEDBCP(void);
	void __fastcall SetED(bool EDstate);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TLSt(double i_mbp, double i_bcr, double i_bcd, double i_brc, 
		Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TESt4R(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TLSt(void) { }
	#pragma option pop
	
};


class DELPHICLASS TEStEP2;
class PASCALIMPLEMENTATION TEStEP2 : public TLSt 
{
	typedef TLSt inherited;
	
private:
	double TareM;
	double LoadM;
	double TareBP;
	double LoadC;
	double EPS;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	void __fastcall PLC(double mass);
	void __fastcall SetEPS(double nEPS);
	void __fastcall SetLP(double TM, double LM, double TBP);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TEStEP2(double i_mbp, double i_bcr, double i_bcd, double i_brc
		, Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TLSt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TEStEP2(void) { }
	#pragma option pop
	
};


class DELPHICLASS TCV1;
class PASCALIMPLEMENTATION TCV1 : public TBrake 
{
	typedef TBrake inherited;
	
private:
	TReservoir* CntrlRes;
	double BVM;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetCRP(void);
	void __fastcall CheckState(double BCP, double &dV1);
	double __fastcall CVs(double bp);
	double __fastcall BVs(double BCP);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TCV1(double i_mbp, double i_bcr, double i_bcd, double i_brc, 
		Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TCV1(void) { }
	#pragma option pop
	
};


class DELPHICLASS TCV1R;
class PASCALIMPLEMENTATION TCV1R : public TCV1 
{
	typedef TCV1 inherited;
	
private:
	TReservoir* ImplsRes;
	bool RapidStatus;
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TCV1R(double i_mbp, double i_bcr, double i_bcd, double i_brc, 
		Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TCV1(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TCV1R(void) { }
	#pragma option pop
	
};


class DELPHICLASS TCV1L_TR;
class PASCALIMPLEMENTATION TCV1L_TR : public TCV1 
{
	typedef TCV1 inherited;
	
private:
	TReservoir* ImplsRes;
	double LBP;
	
public:
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	void __fastcall SetLBP(double P);
	virtual double __fastcall GetHPFlow(double HP, double dt);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TCV1L_TR(double i_mbp, double i_bcr, double i_bcd, double i_brc
		, Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TCV1(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TCV1L_TR(void) { }
	#pragma option pop
	
};


class DELPHICLASS THandle;
class PASCALIMPLEMENTATION THandle : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual double __fastcall GetCP(void);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall THandle(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~THandle(void) { }
	#pragma option pop
	
};


class DELPHICLASS TFV4a;
class PASCALIMPLEMENTATION TFV4a : public THandle 
{
	typedef THandle inherited;
	
private:
	double CP;
	double TP;
	double RP;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFV4a(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFV4a(void) { }
	#pragma option pop
	
};


class DELPHICLASS TFV4aM;
class PASCALIMPLEMENTATION TFV4aM : public THandle 
{
	typedef THandle inherited;
	
private:
	double CP;
	double TP;
	double RP;
	double XP;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFV4aM(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFV4aM(void) { }
	#pragma option pop
	
};


class DELPHICLASS Ttest;
class PASCALIMPLEMENTATION Ttest : public THandle 
{
	typedef THandle inherited;
	
private:
	double CP;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall Ttest(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~Ttest(void) { }
	#pragma option pop
	
};


class DELPHICLASS TFD1;
class PASCALIMPLEMENTATION TFD1 : public THandle 
{
	typedef THandle inherited;
	
private:
	double MaxBP;
	double BP;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual double __fastcall GetCP(void);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFD1(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFD1(void) { }
	#pragma option pop
	
};


class DELPHICLASS TFVel6;
class PASCALIMPLEMENTATION TFVel6 : public THandle 
{
	typedef THandle inherited;
	
private:
	double EPS;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual double __fastcall GetCP(void);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFVel6(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFVel6(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
static const Shortint LocalBrakePosNo = 0xa;
static const Shortint MainBrakeMaxPos = 0xa;
static const Shortint bdelay_G = 0x1;
static const Shortint bdelay_P = 0x2;
static const Shortint bdelay_R = 0x4;
static const Shortint bdelay_M = 0x8;
static const Byte bdelay_GR = 0x80;
static const Shortint b_off = 0x0;
static const Shortint b_hld = 0x1;
static const Shortint b_on = 0x2;
static const Shortint b_rfl = 0x4;
static const Shortint b_rls = 0x8;
static const Shortint b_ep = 0x10;
static const Byte b_dmg = 0x80;
static const Shortint df_on = 0x1;
static const Shortint df_off = 0x2;
static const Shortint df_br = 0x4;
static const Shortint df_vv = 0x8;
static const Shortint df_bc = 0x10;
static const Shortint df_cv = 0x20;
static const Shortint df_PP = 0x40;
static const Byte df_RR = 0x80;
static const Shortint bp_P10 = 0x0;
static const Shortint bp_P10Bg = 0x2;
static const Shortint bp_P10Bgu = 0x1;
static const Shortint bp_LLBg = 0x4;
static const Shortint bp_LLBgu = 0x3;
static const Shortint bp_LBg = 0x6;
static const Shortint bp_LBgu = 0x5;
static const Shortint bp_KBg = 0x8;
static const Shortint bp_KBgu = 0x7;
static const Shortint bp_D1 = 0x9;
static const Shortint bp_D2 = 0xa;
static const Shortint bp_FR513 = 0xb;
static const Shortint bp_Cosid = 0xc;
static const Shortint bp_PKPBg = 0xd;
static const Shortint bp_PKPBgu = 0xe;
static const Byte bp_MHS = 0x80;
static const Shortint bp_P10yBg = 0xf;
static const Shortint bp_P10yBgu = 0x10;
#define Spg  (7.917000E-01)
#define SpO  (5.067000E-01)
extern PACKAGE double BPT[9][2];
static const Shortint i_bcpno = 0x6;
extern PACKAGE double __fastcall PF(double P1, double P2, double S);

}	/* namespace Hamulce */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Hamulce;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// hamulce
