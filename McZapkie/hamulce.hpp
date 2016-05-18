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
	
protected:
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


typedef TReservoir* *PReservoir;

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
	
protected:
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
	bool DCV;
	double ASBP;
	Byte BrakeStatus;
	Byte SoundFlag;
	
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
	virtual void __fastcall SetEPS(double nEPS);
	void __fastcall ASB(Byte state);
	Byte __fastcall GetStatus(void);
	void __fastcall SetASBP(double press);
	virtual void __fastcall ForceEmptiness(void);
	Byte __fastcall GetSoundFlag(void);
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TBrake(void) { }
	#pragma option pop
	
};


class DELPHICLASS TWest;
class PASCALIMPLEMENTATION TWest : public TBrake 
{
	typedef TBrake inherited;
	
private:
	double LBP;
	double dVP;
	double EPS;
	double TareM;
	double LoadM;
	double TareBP;
	double LoadC;
	
public:
	void __fastcall SetLBP(double P);
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetHPFlow(double HP, double dt);
	void __fastcall PLC(double mass);
	virtual void __fastcall SetEPS(double nEPS);
	void __fastcall SetLP(double TM, double LM, double TBP);
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
	double RapidTemp;
	
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
	double EDFlag;
	
public:
	void __fastcall SetLBP(double P);
	void __fastcall SetRM(double RMR);
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual double __fastcall GetHPFlow(double HP, double dt);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetEDBCP(void);
	void __fastcall SetED(double EDstate);
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


class DELPHICLASS TEStED;
class PASCALIMPLEMENTATION TEStED : public TLSt 
{
	typedef TLSt inherited;
	
private:
	double Nozzles[11];
	bool Zamykajacy;
	bool Przys_blok;
	TReservoir* Miedzypoj;
	double TareM;
	double LoadM;
	double TareBP;
	double LoadC;
	
public:
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual double __fastcall GetEDBCP(void);
	void __fastcall PLC(double mass);
	void __fastcall SetLP(double TM, double LM, double TBP);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TEStED(double i_mbp, double i_bcr, double i_bcd, double i_brc
		, Byte i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TLSt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
		, i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TEStED(void) { }
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
	virtual void __fastcall SetEPS(double nEPS);
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


class DELPHICLASS TKE;
class PASCALIMPLEMENTATION TKE : public TBrake 
{
	typedef TBrake inherited;
	
private:
	bool RapidStatus;
	TReservoir* ImplsRes;
	TReservoir* CntrlRes;
	TReservoir* Brak2Res;
	double BVM;
	double TareM;
	double LoadM;
	double TareBP;
	double LoadC;
	double RM;
	double LBP;
	
public:
	void __fastcall SetRM(double RMR);
	virtual double __fastcall GetPF(double PP, double dt, double Vel);
	virtual void __fastcall Init(double PP, double HPP, double LPP, double BP, Byte BDF);
	virtual double __fastcall GetHPFlow(double HP, double dt);
	virtual double __fastcall GetCRP(void);
	void __fastcall CheckState(double BCP, double &dV1);
	void __fastcall CheckReleaser(double dt);
	double __fastcall CVs(double bp);
	double __fastcall BVs(double BCP);
	void __fastcall PLC(double mass);
	void __fastcall SetLP(double TM, double LM, double TBP);
	void __fastcall SetLBP(double P);
public:
	#pragma option push -w-inl
	/* TBrake.Create */ inline __fastcall TKE(double i_mbp, double i_bcr, double i_bcd, double i_brc, Byte 
		i_bcn, Byte i_BD, Byte i_mat, Byte i_ba, Byte i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, 
		i_BD, i_mat, i_ba, i_nbpa) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TKE(void) { }
	#pragma option pop
	
};


class DELPHICLASS THandle;
class PASCALIMPLEMENTATION THandle : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	bool Time;
	bool TimeEP;
	double Sounds[5];
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual double __fastcall GetCP(void);
	virtual void __fastcall SetReductor(double nAdj);
	virtual double __fastcall GetSound(Byte i);
	virtual double __fastcall GetPos(Byte i);
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
	double RedAdj;
	bool Fala;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual void __fastcall SetReductor(double nAdj);
	virtual double __fastcall GetSound(Byte i);
	virtual double __fastcall GetPos(Byte i);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFV4aM(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFV4aM(void) { }
	#pragma option pop
	
};


class DELPHICLASS TMHZ_EN57;
class PASCALIMPLEMENTATION TMHZ_EN57 : public THandle 
{
	typedef THandle inherited;
	
private:
	double CP;
	double TP;
	double RP;
	double RedAdj;
	bool Fala;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual void __fastcall SetReductor(double nAdj);
	virtual double __fastcall GetSound(Byte i);
	virtual double __fastcall GetPos(Byte i);
	virtual double __fastcall GetCP(void);
	double __fastcall GetEP(double pos);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TMHZ_EN57(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TMHZ_EN57(void) { }
	#pragma option pop
	
};


class DELPHICLASS TM394;
class PASCALIMPLEMENTATION TM394 : public THandle 
{
	typedef THandle inherited;
	
private:
	double CP;
	double RedAdj;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual void __fastcall SetReductor(double nAdj);
	virtual double __fastcall GetCP(void);
	virtual double __fastcall GetPos(Byte i);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TM394(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TM394(void) { }
	#pragma option pop
	
};


class DELPHICLASS TH14K1;
class PASCALIMPLEMENTATION TH14K1 : public THandle 
{
	typedef THandle inherited;
	
private:
	double CP;
	double RedAdj;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual void __fastcall SetReductor(double nAdj);
	virtual double __fastcall GetCP(void);
	virtual double __fastcall GetPos(Byte i);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TH14K1(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TH14K1(void) { }
	#pragma option pop
	
};


class DELPHICLASS TSt113;
class PASCALIMPLEMENTATION TSt113 : public TH14K1 
{
	typedef TH14K1 inherited;
	
private:
	double EPS;
	
public:
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual double __fastcall GetCP(void);
	virtual double __fastcall GetPos(Byte i);
	virtual void __fastcall Init(double press);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TSt113(void) : TH14K1() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TSt113(void) { }
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
	double Speed;
	virtual double __fastcall GetPF(double i_bcp, double pp, double hp, double dt, double ep);
	virtual void __fastcall Init(double press);
	virtual double __fastcall GetCP(void);
	void __fastcall SetSpeed(double nSpeed);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFD1(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFD1(void) { }
	#pragma option pop
	
};


class DELPHICLASS TH1405;
class PASCALIMPLEMENTATION TH1405 : public THandle 
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
	/* TObject.Create */ inline __fastcall TH1405(void) : THandle() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TH1405(void) { }
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
	virtual double __fastcall GetPos(Byte i);
	virtual double __fastcall GetSound(Byte i);
	virtual void __fastcall Init(double press);
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
static const Shortint b_asb = 0x20;
static const Byte b_dmg = 0x80;
static const Shortint df_on = 0x1;
static const Shortint df_off = 0x2;
static const Shortint df_br = 0x4;
static const Shortint df_vv = 0x8;
static const Shortint df_bc = 0x10;
static const Shortint df_cv = 0x20;
static const Shortint df_PP = 0x40;
static const Byte df_RR = 0x80;
static const Shortint s_fv4a_b = 0x0;
static const Shortint s_fv4a_u = 0x1;
static const Shortint s_fv4a_e = 0x2;
static const Shortint s_fv4a_x = 0x3;
static const Shortint s_fv4a_t = 0x4;
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
static const Shortint bp_FR510 = 0x11;
static const Shortint sf_Acc = 0x1;
static const Shortint sf_BR = 0x2;
static const Shortint sf_CylB = 0x4;
static const Shortint sf_CylU = 0x8;
static const Shortint sf_rel = 0x10;
static const Shortint sf_ep = 0x20;
static const Shortint bh_MIN = 0x0;
static const Shortint bh_MAX = 0x1;
static const Shortint bh_FS = 0x2;
static const Shortint bh_RP = 0x3;
static const Shortint bh_NP = 0x4;
static const Shortint bh_MB = 0x5;
static const Shortint bh_FB = 0x6;
static const Shortint bh_EB = 0x7;
static const Shortint bh_EPR = 0x8;
static const Shortint bh_EPN = 0x9;
static const Shortint bh_EPB = 0xa;
#define SpgD  (7.917000E-01)
#define SpO  (5.067000E-01)
extern PACKAGE double BPT[9][2];
extern PACKAGE double BPT_394[7][2];
static const Shortint i_bcpno = 0x6;
extern PACKAGE double __fastcall PF(double P1, double P2, double S, double DP);
extern PACKAGE double __fastcall PF1(double P1, double P2, double S);
extern PACKAGE double __fastcall PFVa(double PH, double PL, double S, double LIM, double DP);
extern PACKAGE double __fastcall PFVd(double PH, double PL, double S, double LIM, double DP);

}	/* namespace Hamulce */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Hamulce;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// hamulce
