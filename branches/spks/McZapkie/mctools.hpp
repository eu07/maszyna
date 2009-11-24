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
#define _FileName (System::Set<char, 0, 255> () << '\x2a' << '\x2d' << '\x2e' << '\x30' << '\x31' << '\x32' \
	<< '\x33' << '\x34' << '\x35' << '\x36' << '\x37' << '\x38' << '\x39' << '\x3a' << '\x3f' << '\x41' \
	<< '\x42' << '\x43' << '\x44' << '\x45' << '\x46' << '\x47' << '\x4a' << '\x4d' << '\x4e' << '\x50' \
	<< '\x51' << '\x52' << '\x53' << '\x54' << '\x55' << '\x56' << '\x57' << '\x58' << '\x59' << '\x5a' \
	<< '\x5f' << '\x61' << '\x62' << '\x63' << '\x64' << '\x65' << '\x66' << '\x67' << '\x6a' << '\x6d' \
	<< '\x6e' << '\x70' << '\x71' << '\x72' << '\x73' << '\x74' << '\x75' << '\x76' << '\x77' << '\x78' \
	<< '\x79' << '\x7a' << '\x7f' )
#define _RealNum (System::Set<char, 0, 255> () << '\x2b' << '\x2d' << '\x2e' << '\x30' << '\x31' << '\x32' \
	<< '\x33' << '\x34' << '\x35' << '\x36' << '\x37' << '\x38' << '\x39' << '\x45' << '\x4b' << '\x4d' \
	<< '\x4e' << '\x50' << '\x51' << '\x52' << '\x53' << '\x54' << '\x55' << '\x56' << '\x57' << '\x58' \
	<< '\x59' << '\x65' )
#define _Integer (System::Set<char, 0, 255> () << '\x2d' << '\x30' << '\x31' << '\x32' << '\x33' << '\x34' \
	<< '\x35' << '\x36' << '\x37' << '\x38' << '\x39' )
#define _Plus_Int (System::Set<char, 0, 255> () << '\x30' << '\x31' << '\x32' << '\x33' << '\x34' << '\x35' \
	<< '\x36' << '\x37' << '\x38' << '\x39' )
#define _All (System::Set<char, 0, 255> () << '\x20' << '\x21' << '\x22' << '\x23' << '\x24' << '\x25' \
	<< '\x26' << '\x27' << '\x28' << '\x29' << '\x2a' << '\x2b' << '\x2c' << '\x2d' << '\x2e' << '\x2f' \
	<< '\x30' << '\x31' << '\x32' << '\x33' << '\x34' << '\x35' << '\x36' << '\x37' << '\x38' << '\x39' \
	<< '\x3a' << '\x3b' << '\x3c' << '\x3d' << '\x3e' << '\x3f' << '\x40' << '\x41' << '\x42' << '\x43' \
	<< '\x44' << '\x45' << '\x46' << '\x47' << '\x48' << '\x49' << '\x4a' << '\x4b' << '\x4c' << '\x4d' \
	<< '\x4e' << '\x4f' << '\x50' << '\x51' << '\x52' << '\x53' << '\x54' << '\x55' << '\x56' << '\x57' \
	<< '\x58' << '\x59' << '\x5a' << '\x5b' << '\x5c' << '\x5d' << '\x5e' << '\x5f' << '\x60' << '\x61' \
	<< '\x62' << '\x63' << '\x64' << '\x65' << '\x66' << '\x67' << '\x68' << '\x69' << '\x6a' << '\x6b' \
	<< '\x6c' << '\x6d' << '\x6e' << '\x6f' << '\x70' << '\x71' << '\x72' << '\x73' << '\x74' << '\x75' \
	<< '\x76' << '\x77' << '\x78' << '\x79' << '\x7a' << '\x7b' << '\x7c' << '\x7d' << '\x7e' << '\x7f' \
	<< '\x80' << '\x81' << '\x82' << '\x83' << '\x84' << '\x85' << '\x86' << '\x87' << '\x88' << '\x89' \
	<< '\x8a' << '\x8b' << '\x8c' << '\x8d' << '\x8e' << '\x8f' << '\x90' << '\x91' << '\x92' << '\x93' \
	<< '\x94' << '\x95' << '\x96' << '\x97' << '\x98' << '\x99' << '\x9a' << '\x9b' << '\x9c' << '\x9d' \
	<< '\x9e' << '\x9f' << '\xa0' << '\xa1' << '\xa2' << '\xa3' << '\xa4' << '\xa5' << '\xa6' << '\xa7' \
	<< '\xa8' << '\xa9' << '\xaa' << '\xab' << '\xac' << '\xad' << '\xae' << '\xaf' << '\xb0' << '\xb1' \
	<< '\xb2' << '\xb3' << '\xb4' << '\xb5' << '\xb6' << '\xb7' << '\xb8' << '\xb9' << '\xba' << '\xbb' \
	<< '\xbc' << '\xbd' << '\xbe' << '\xbf' << '\xc0' << '\xc1' << '\xc2' << '\xc3' << '\xc4' << '\xc5' \
	<< '\xc6' << '\xc7' << '\xc8' << '\xc9' << '\xca' << '\xcb' << '\xcc' << '\xcd' << '\xce' << '\xcf' \
	<< '\xd0' << '\xd1' << '\xd2' << '\xd3' << '\xd4' << '\xd5' << '\xd6' << '\xd7' << '\xd8' << '\xd9' \
	<< '\xda' << '\xdb' << '\xdc' << '\xdd' << '\xde' << '\xdf' << '\xe0' << '\xe1' << '\xe2' << '\xe3' \
	<< '\xe4' << '\xe5' << '\xe6' << '\xe7' << '\xe8' << '\xe9' << '\xea' << '\xeb' << '\xec' << '\xed' \
	<< '\xee' << '\xef' << '\xf0' << '\xf1' << '\xf2' << '\xf3' << '\xf4' << '\xf5' << '\xf6' << '\xf7' \
	<< '\xf8' << '\xf9' << '\xfa' << '\xfb' << '\xfc' << '\xfd' << '\xfe' << '\xff' )
#define _EOL (System::Set<char, 0, 255> () << '\xa' << '\xd' )
static const char _SPACE = '\x20';
#define _Delimiter (System::Set<char, 0, 255> () << '\xa' << '\xd' << '\x2a' << '\x2d' << '\x4a' << '\x4d' \
	<< '\x6a' << '\x6d' << '\x8a' << '\x8d' << '\xaa' << '\xad' << '\xca' << '\xcd' << '\xea' << '\xed' \
	)
#define _Delimiter_Space (System::Set<char, 0, 255> () << '\xa' << '\xd' << '\x2a' << '\x2d' << '\x4a' \
	<< '\x4d' << '\x6a' << '\x6d' << '\x8a' << '\x8d' << '\xaa' << '\xad' << '\xca' << '\xcd' << '\xea' \
	<< '\xed' )
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

}	/* namespace Mctools */
#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Mctools;
#endif
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mctools
