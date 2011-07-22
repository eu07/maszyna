// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'friction.pas' rev: 5.00

#ifndef frictionHPP
#define frictionHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Friction
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TFricMat;
class PASCALIMPLEMENTATION TFricMat : public System::TObject 
{
	typedef System::TObject inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFricMat(void) : System::TObject() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFricMat(void) { }
	#pragma option pop
	
};


class DELPHICLASS TP10Bg;
class PASCALIMPLEMENTATION TP10Bg : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TP10Bg(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TP10Bg(void) { }
	#pragma option pop
	
};


class DELPHICLASS TP10Bgu;
class PASCALIMPLEMENTATION TP10Bgu : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TP10Bgu(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TP10Bgu(void) { }
	#pragma option pop
	
};


class DELPHICLASS TP10;
class PASCALIMPLEMENTATION TP10 : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TP10(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TP10(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
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
static const Byte bp_MHS = 0x80;

}	/* namespace Friction */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Friction;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// friction
