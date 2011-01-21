// Borland C++ Builder
// Copyright (c) 1995, 1999 by Borland International
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mctools.pas' rev: 5.00

#ifndef mctoolsHPP
#define mctoolsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mctools
{
//-- type declarations -------------------------------------------------------
typedef Set<char, 0, 255>  TableChar;

//-- var, const, procedure ---------------------------------------------------
#define _EOL (System::Set<char, 0, 255> () << '\xa' << '\xd' )
static const char _SPACE = '\x20';
#define _Spacesigns (System::Set<char, 0, 255> () << '\x9' << '\xa' << '\xd' << '\x29' << '\x2a' << '\x2d' \
	<< '\x49' << '\x4a' << '\x4d' << '\x69' << '\x6a' << '\x6d' << '\x89' << '\x8a' << '\x8d' << '\xa9' \
	<< '\xaa' << '\xad' << '\xc9' << '\xca' << '\xcd' << '\xe9' << '\xea' << '\xed' )
static const Shortint CutLeft = 0xffffffff;
static const Shortint CutRight = 0x1;
static const Shortint CutBoth = 0x0;
extern PACKAGE int ConversionError;
extern PACKAGE int LineCount;
extern PACKAGE bool DebugModeFlag;
extern PACKAGE bool FreeFlyModeFlag;
extern PACKAGE double Xmin;
extern PACKAGE double Ymin;
extern PACKAGE double Xmax;
extern PACKAGE double Ymax;
extern PACKAGE double Xaspect;
extern PACKAGE double Yaspect;
extern PACKAGE double Hstep;
extern PACKAGE double Vstep;
extern PACKAGE int Vsize;
extern PACKAGE int Hsize;
extern PACKAGE AnsiString __fastcall Ups(AnsiString s);
extern PACKAGE AnsiString __fastcall b2s(Byte b);
extern PACKAGE AnsiString __fastcall i2s(int i);
extern PACKAGE AnsiString __fastcall l2s(int l);
extern PACKAGE AnsiString __fastcall r2s(double r, Byte SWidth);
extern PACKAGE Byte __fastcall s2b(AnsiString s);
extern PACKAGE int __fastcall s2i(AnsiString s);
extern PACKAGE int __fastcall s2l(AnsiString s);
extern PACKAGE double __fastcall s2r(AnsiString s);
extern PACKAGE Byte __fastcall s2bE(AnsiString s);
extern PACKAGE int __fastcall s2iE(AnsiString s);
extern PACKAGE int __fastcall s2lE(AnsiString s);
extern PACKAGE double __fastcall s2rE(AnsiString s);
extern PACKAGE Byte __fastcall Max0(Byte x1, Byte x2);
extern PACKAGE Byte __fastcall Min0(Byte x1, Byte x2);
extern PACKAGE double __fastcall Max0R(double x1, double x2);
extern PACKAGE double __fastcall Min0R(double x1, double x2);
extern PACKAGE int __fastcall Sign(double x);
extern PACKAGE bool __fastcall TestFlag(int Flag, int Value);
extern PACKAGE bool __fastcall SetFlag(Byte &Flag, int Value);
extern PACKAGE bool __fastcall iSetFlag(int &Flag, int Value);
extern PACKAGE bool __fastcall FuzzyLogic(double Test, double Threshold, double Probability);
extern PACKAGE bool __fastcall FuzzyLogicAI(double Test, double Threshold, double Probability);
extern PACKAGE AnsiString __fastcall ReadWord(TextFile &infile);
extern PACKAGE AnsiString __fastcall Cut_Space(AnsiString s, int Just);
extern PACKAGE AnsiString __fastcall ExtractKeyWord(AnsiString InS, AnsiString KeyWord);
extern PACKAGE AnsiString __fastcall DUE(AnsiString S);
extern PACKAGE AnsiString __fastcall DWE(AnsiString S);
extern PACKAGE AnsiString __fastcall Ld2Sp(AnsiString S);
extern PACKAGE AnsiString __fastcall Tab2Sp(AnsiString S);
extern PACKAGE void __fastcall ComputeArc(double X0, double Y0, double Xn, double Yn, double R, double 
	L, double dL, double &phi, double &Xout, double &Yout);
extern PACKAGE void __fastcall ComputeALine(double X0, double Y0, double Xn, double Yn, double L, double 
	R, double &Xout, double &Yout);
extern PACKAGE double __fastcall Xhor(double h);
extern PACKAGE double __fastcall Yver(double v);
extern PACKAGE int __fastcall Horiz(double X);
extern PACKAGE int __fastcall Vert(double Y);
extern PACKAGE void __fastcall ClearPendingExceptions(void);

}	/* namespace Mctools */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Mctools;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mctools
