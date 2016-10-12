#pragma once
#ifndef INCLUDED_MCTOOLS_H
#define INCLUDED_MCTOOLS_H
 

/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*rozne takie duperele do operacji na stringach w paszczalu, pewnie w delfi sa lepsze*/
/*konwersja zmiennych na stringi, funkcje matematyczne, logiczne, lancuchowe, I/O etc*/

#include <string>
#include <istream>


/*Ra: te sta³e nie s¹ u¿ywane...
        _FileName = ['a'..'z','A'..'Z',':','\','.','*','?','0'..'9','_','-'];
        _RealNum  = ['0'..'9','-','+','.','E','e'];
        _Integer  = ['0'..'9','-']; //Ra: to siê gryzie z STLport w Builder 6
        _Plus_Int = ['0'..'9'];
        _All      = [' '..'þ'];
        _Delimiter= [',',';']+_EOL;
        _Delimiter_Space=_Delimiter+[' '];
*/
static char _EOL[2] = { (char)13, (char)10 };
        //static char const _SPACE = " ";
static char  _Spacesigns[4] = { (char)" ",(char)9, (char)13, (char)10};
        static int const CutLeft = -1;
		static int const CutRight = 1;
		static int const CutBoth = 0;  /*Cut_Space*/
		static double const pi = 3.141592653589793;

        int ConversionError = 0;
        int LineCount = 0;
        bool DebugModeFlag = false;
        bool FreeFlyModeFlag = false;


        typedef unsigned long/*?*//*set of: char */ TableChar;  /*MCTUTIL*/

/*konwersje*/

/*funkcje matematyczne*/
int Max0(int x1, int x2);
int Min0(int x1, int x2);

double Max0R(double x1, double x2);
double Min0R(double x1, double x2);

int Sign(double x);

/*funkcje logiczne*/
bool TestFlag(int Flag,  int Value);
bool SetFlag( int & Flag,  int Value);
bool iSetFlag( int & Flag,  int Value);

bool FuzzyLogic(double Test, double Threshold, double Probability);
/*jesli Test>Threshold to losowanie*/
bool FuzzyLogicAI(double Test, double Threshold, double Probability);
/*to samo ale zawsze niezaleznie od DebugFlag*/

/*operacje na stringach*/
std::string ReadWord( std::ifstream & infile); /*czyta slowo z wiersza pliku tekstowego*/
std::string Ups(std::string s);
std::string Cut_Space(std::string s,  int Just);
std::string ExtractKeyWord(std::string InS,  std::string KeyWord);   /*wyciaga slowo kluczowe i lancuch do pierwszej spacji*/
std::string DUE(std::string s);  /*Delete Until Equal sign*/
std::string DWE(std::string s);  /*Delete While Equal sign*/
std::string Ld2Sp(std::string s); /*Low dash to Space sign*/
std::string Tab2Sp(std::string s); /*Tab to Space sign*/

/*procedury, zmienne i funkcje graficzne*/
void ComputeArc(double X0, double Y0, double Xn, double Yn, double R, double L, double dL,   double & phi, double & Xout, double & Yout);
/*wylicza polozenie Xout Yout i orientacje phi punktu na elemencie dL luku*/
void ComputeALine(double X0, double Y0, double Xn, double Yn, double L, double R,   double & Xout, double & Yout);



 double Xmin; double Ymin; double Xmax; double Ymax;
 double Xaspect; double Yaspect;
 double Hstep; double Vstep;
 int Vsize; int Hsize;

double Xhor( double h);
       /* Converts horizontal screen coordinate into real X-coordinate. */

double Yver( double v);
       /* Converts vertical screen coordinate into real Y-coordinate. */

long Horiz(double x);

long Vert(double Y);

void ClearPendingExceptions();

/*------------------------------------------------*/


#endif//INCLUDED_MCTOOLS_H
//END
