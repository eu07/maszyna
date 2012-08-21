// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'friction.pas' rev: 5.00

#ifndef frictionHPP
#define frictionHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
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


class DELPHICLASS TP10yBg;
class PASCALIMPLEMENTATION TP10yBg : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TP10yBg(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TP10yBg(void) { }
	#pragma option pop
	
};


class DELPHICLASS TP10yBgu;
class PASCALIMPLEMENTATION TP10yBgu : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TP10yBgu(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TP10yBgu(void) { }
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


class DELPHICLASS TFR513;
class PASCALIMPLEMENTATION TFR513 : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFR513(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFR513(void) { }
	#pragma option pop
	
};


class DELPHICLASS TFR510;
class PASCALIMPLEMENTATION TFR510 : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TFR510(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TFR510(void) { }
	#pragma option pop
	
};


class DELPHICLASS TCosid;
class PASCALIMPLEMENTATION TCosid : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TCosid(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TCosid(void) { }
	#pragma option pop
	
};


class DELPHICLASS TDisk1;
class PASCALIMPLEMENTATION TDisk1 : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TDisk1(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TDisk1(void) { }
	#pragma option pop
	
};


class DELPHICLASS TDisk2;
class PASCALIMPLEMENTATION TDisk2 : public TFricMat 
{
	typedef TFricMat inherited;
	
public:
	virtual double __fastcall GetFC(double N, double Vel);
public:
	#pragma option push -w-inl
	/* TObject.Create */ inline __fastcall TDisk2(void) : TFricMat() { }
	#pragma option pop
	#pragma option push -w-inl
	/* TObject.Destroy */ inline __fastcall virtual ~TDisk2(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Friction */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Friction;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// friction
