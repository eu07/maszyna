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
#include <fstream>
#include <time.h>
#include <sys/stat.h>
#include <vector>
#include <sstream>

using namespace std;

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
static string _spacesigns = " " + (char)9  + (char)13  + (char)10;
static int const CutLeft = -1;
static int const CutRight = 1;
static int const CutBoth = 0;  /*Cut_Space*/
static double const pi = 3.141592653589793;

extern int ConversionError;
extern int LineCount;
extern bool DebugModeFlag;
extern bool FreeFlyModeFlag;


typedef unsigned long/*?*//*set of: char */ TableChar;  /*MCTUTIL*/

/*konwersje*/

/*funkcje matematyczne*/
int Max0(int x1, int x2);
int Min0(int x1, int x2);

double Max0R(double x1, double x2);
double Min0R(double x1, double x2);

int Sign(double x);
inline long Round(float f)
{
	return (long)(f + 0.5);
	//return lround(f);
}

inline int Random()
{
	srand(time(NULL));
	return rand();
}

inline double Random(double a, double b)
{
	srand(time(NULL));
	return a + rand() / (float)RAND_MAX * (b - a);
}

inline double Random(double b)
{
	return Random(0.0, b);
}

inline double BorlandTime()
{
	std::tm epoch;
	epoch.tm_sec = 0;
	epoch.tm_min = 0;
	epoch.tm_hour = 0;
	epoch.tm_mday = 1;
	epoch.tm_mon = 0;
	epoch.tm_year = 0;
	time_t basetime = mktime(&epoch);
	time_t raw_t = time(NULL);
	return (difftime(raw_t, basetime) / 24) + 2;
}

/*funkcje logiczne*/
extern bool TestFlag(int Flag,  int Value);
extern bool SetFlag( int & Flag,  int Value);
extern bool iSetFlag( int & Flag,  int Value);
extern bool UnSetFlag(int &Flag, int Value);

bool FuzzyLogic(double Test, double Threshold, double Probability);
/*jesli Test>Threshold to losowanie*/
bool FuzzyLogicAI(double Test, double Threshold, double Probability);
/*to samo ale zawsze niezaleznie od DebugFlag*/

/*operacje na stringach*/
std::string ReadWord( std::ifstream& infile); /*czyta slowo z wiersza pliku tekstowego*/
//std::string Ups(std::string s);
std::string TrimSpace(std::string &s,  int Just = CutBoth);
char* TrimAndReduceSpaces(const char* s);
std::string ExtractKeyWord(std::string InS,  std::string KeyWord);   /*wyciaga slowo kluczowe i lancuch do pierwszej spacji*/
std::string DUE(std::string s);  /*Delete Until Equal sign*/
std::string DWE(std::string s);  /*Delete While Equal sign*/
std::string Ld2Sp(std::string s); /*Low dash to Space sign*/
std::string Tab2Sp(std::string s); /*Tab to Space sign*/
std::string ExchangeCharInString(string s,  const char &aim, const char &target); // zamienia jeden znak na drugi
std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> Split(const std::string &s, char delim);
std::vector<std::string> Split(const std::string &s);

std::string to_string(int _Val);
std::string to_string(unsigned int _Val);
std::string to_string(int _Val, int precision);
std::string to_string(int _Val, int precision, int width);
std::string to_string(double _Val);
std::string to_string(double _Val, int precision);
std::string to_string(double _Val, int precision, int width);
std::string to_hex_str(double _Val, int precision = 0, int width = 0);
inline std::string to_string(bool _Val)
{
	return to_string((int)_Val);
}

int stol_def(const std::string & str, const int & DefaultValue);

std::string ToLower(std::string text);
std::string ToUpper(std::string text);

/*procedury, zmienne i funkcje graficzne*/
void ComputeArc(double X0, double Y0, double Xn, double Yn, double R, double L, double dL,   double & phi, double & Xout, double & Yout);
/*wylicza polozenie Xout Yout i orientacje phi punktu na elemencie dL luku*/
void ComputeALine(double X0, double Y0, double Xn, double Yn, double L, double R,   double & Xout, double & Yout);

inline bool fileExists(const std::string &name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

extern double Xmin;
extern double Ymin;
extern double Xmax;
extern double Ymax;
extern double Xaspect;
extern double Yaspect;
extern double Hstep;
extern double Vstep;
extern int Vsize;
extern int Hsize;

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
