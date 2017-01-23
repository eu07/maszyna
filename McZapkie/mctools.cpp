/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
MaSzyna EU07 - SPKS
Brakes.
Copyright (C) 2007-2014 Maciej Cierniak
*/
#include "stdafx.h"
#include "mctools.h"

/*================================================*/

int ConversionError = 0;
int LineCount = 0;
bool DebugModeFlag = false;
bool FreeFlyModeFlag = false;

//std::string Ups(std::string s)
//{
//    int jatka;
//    std::string swy;
//
//    swy = "";
//    {
//        long jatka_end = s.length() + 1;
//        for (jatka = 0; jatka < jatka_end; jatka++)
//            swy = swy + UpCase(s[jatka]);
//    }
//    return swy;
//} /*=Ups=*/

int Max0(int x1, int x2)
{
    if (x1 > x2)
        return x1;
    else
        return x2;
}

int Min0(int x1, int x2)
{
    if (x1 < x2)
        return x1;
    else
        return x2;
}

double Max0R(double x1, double x2)
{
    if (x1 > x2)
        return x1;
    else
        return x2;
}

double Min0R(double x1, double x2)
{
    if (x1 < x2)
        return x1;
    else
        return x2;
}

// shitty replacement for Borland timestamp function
// TODO: replace with something sensible
std::string Now() {

	std::time_t timenow = std::time( nullptr );
	std::tm tm = *std::localtime( &timenow );
	std::stringstream  converter;
	std::string output;
	converter << std::put_time( &tm, "%c" );
	converter >> output;
	return output;

/*	char buffer[ 256 ];
	sprintf( buffer,
		"%d-%02d-%02d %02d:%02d:%02d.%03d",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds );
*/
}

bool TestFlag(int Flag, int Value)
{
    if ((Flag & Value) == Value)
        return true;
    else
        return false;
}

bool SetFlag(int &Flag, int Value)
{
	return iSetFlag(Flag, Value);
}

bool iSetFlag(int &Flag, int Value)
{
    if (Value > 0)
    {
        if ((Flag & Value) == 0)
        {
            Flag |= Value;
            return true; // true, gdy by³o wczeœniej 0 i zosta³o ustawione
        }
    }
    else if (Value < 0)
    {
        Value = abs(Value);
        if ((Flag & Value) == Value)
        {
            Flag &= ~Value; // Value jest ujemne, czyli zerowanie flagi
            return true; // true, gdy by³o wczeœniej 1 i zosta³o wyzerowane
        }
    }
	return false;
}

bool UnSetFlag(int &Flag, int Value)
{
	Flag &= ~Value;
	return true;
}

bool FuzzyLogic(double Test, double Threshold, double Probability)
{
    if ((Test > Threshold) && (!DebugModeFlag))
        return
            (Random() < Probability * Threshold * 1.0 / Test) /*im wiekszy Test tym wieksza szansa*/;
    else
        return false;
}

bool FuzzyLogicAI(double Test, double Threshold, double Probability)
{
    if ((Test > Threshold))
        return
            (Random() < Probability * Threshold * 1.0 / Test) /*im wiekszy Test tym wieksza szansa*/;
    else
        return false;
}

std::string ReadWord(std::ifstream& infile)
{
    std::string s = "";
    char c;
	bool nextword = false;

    while ((!infile.eof()) && (!nextword))
    {
        infile.get(c);
        if (_spacesigns.find(c) != std::string::npos)
            if (s != "")
                nextword = true;
        if (_spacesigns.find(c) == std::string::npos)
            s += c;
    }
    return s;
}

std::string TrimSpace(std::string &s, int Just)
{
    /*int ii;

    switch (Just)
    {
    case CutLeft:
    {
        ii = 0;
        while ((ii < s.length()) && (s[ii + 1] == (char)" "))
            ++ii;
        s = s.substr(ii + 1, s.length() - ii);
    }
    case CutRight:
    {
        ii = s.length();
        while ((ii > 0) && (s[ii] == (char)" "))
            --ii;
        s = s.substr(0, ii);
    }
    case CutBoth:
    {
        s = TrimSpace(s, CutLeft);
        s = TrimSpace(s, CutRight);
    }
    }
    return s;*/

    if (s.empty())
        return "";
    size_t first = s.find_first_not_of(' ');
    if (first == std::string::npos)
        return "";
    size_t last = s.find_last_not_of(' ');
    return s.substr(first, (last - first + 1));
}

char* TrimAndReduceSpaces(const char* s)
{ // redukuje spacje pomiedzy znakami do jednej
	char* tmp = nullptr;
	if (s)
	{

		tmp = _strdup(s);
		char* from = tmp + strspn(tmp, " ");
		char* to = tmp;

		do if ((*to = *from++) == ' ')
			from += strspn(from, " ");
		while (*to++);

		while (*--to == ' ')
			*to = '\0';
	}
	return tmp;
}

std::string ExtractKeyWord(std::string InS, std::string KeyWord)
{
    std::string s;
    InS = InS + " ";
    std::size_t kwp = InS.find(KeyWord);
    if (kwp != std::string::npos)
    {
		s = InS.substr(kwp, InS.length());
        //s = Copy(InS, kwp, length(InS));
		s = s.substr(0, s.find_first_of(" "));
        //s = Copy(s, 1, Pos(" ", s) - 1);
    }
    else
        s = "";
    return s;
}

std::string DUE(std::string s) /*Delete Before Equal sign*/
{
    //DUE = Copy(s, Pos("=", s) + 1, length(s));
	return s.substr(s.find("=") + 1, s.length());
}

std::string DWE(std::string s) /*Delete After Equal sign*/
{
    size_t ep = s.find("=");
	if (ep != std::string::npos)
		//DWE = Copy(s, 1, ep - 1);
		return s.substr(0, ep);
    else
        return s;
}

std::string Ld2Sp(std::string const &s) /*Low dash to Space sign*/
{
    std::string s2( s );
	char tmp[] = { '_', ' ' };
	for (int b = 0; b < s2.length(); ++b)
		if (s2[b] == tmp[0])
			s2[b] = tmp[1];
	//{
	//	if (s[b] == tmp[0])
	//		s2 = s2 + " ";
	//	else
	//		s2 = s2 + s[b];
	//}
	return s2;
}

std::string Tab2Sp(std::string const &s) /*Tab to Space sign*/
{
	std::string s2 = "";
	char tmp = (char)9;
	for (std::size_t b = 0; b < s.length(); ++b)
	//{
	//	if (s[b] == tmp[0])
	//		s[b] = tmp[1];
	//}
	//return s;
	{
		if (s[b] == tmp)
			s2 = s2 + " ";
		else
			s2 = s2 + s[b];
	}
	return s2;
}

std::string ExchangeCharInString( std::string const &Source, char const &From, char const &To )
{
	std::string replacement; replacement.reserve( Source.size() );
	std::for_each(Source.cbegin(), Source.cend(), [&](char const idx) {
		if( idx != From )    { replacement += idx; }
		else {
			if( To != NULL ) { replacement += To; } }
	} );

	return replacement;
/*
	int const length = Source.size();
	std::string replacement; replacement.reserve( length );
	for( int idx = 0; idx < length; ++idx ) {
		if( Source[ idx ] != From ) {
			replacement += Source[ idx ];
		}
		else {
			if( To != NULL ) {
				replacement += To;
			}
		}
	}

	return replacement;
*/
}

std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems)
{ // dzieli tekst na wektor tekstow

    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> Split(const std::string &s, char delim)
{ // dzieli tekst na wektor tekstow
    std::vector<std::string> elems;
    Split(s, delim, elems);
    return elems;
}

std::vector<std::string> Split(const std::string &s)
{ // dzieli tekst na wektor tekstow po bia³ych znakach
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (ss >> item)
	{
		elems.push_back(item);
	}
	return elems;
}

std::string to_string(int _Val)
{
	std::ostringstream o;
	o << _Val;
	return o.str();
};

std::string to_string(unsigned int _Val)
{
	std::ostringstream o;
	o << _Val;
	return o.str();
};

std::string to_string(double _Val)
{
	std::ostringstream o;
	o << _Val;
	return o.str();
};

std::string to_string(int _Val, int precision)
{
	std::ostringstream o;
	o << std::fixed << std::setprecision(precision);
	o << _Val;
	return o.str();
};

std::string to_string(double _Val, int precision)
{
	std::ostringstream o;
	o << std::fixed << std::setprecision(precision);
	o << _Val;
	return o.str();
};

std::string to_string(int _Val, int precision, int width)
{
	std::ostringstream o;
	o.width(width);
	o << std::fixed << std::setprecision(precision);
	o << _Val;
	return o.str();
};

std::string to_string(double const Value, int const Precision, int const Width)
{
	std::ostringstream converter;
	converter << std::setw( Width ) << std::fixed << std::setprecision(Precision) << Value;
	return converter.str();
};

std::string to_hex_str( int const Value, int const Width )
{
	std::ostringstream converter;
	converter << "0x" << std::uppercase << std::setfill( '0' ) << std::setw( Width ) << std::hex << Value;
	return converter.str();
};

int stol_def(const std::string &str, const int &DefaultValue)
{
    // NOTE: there's good chance this function is bugged, it produced wrong result on at least one occasion
    // TODO: rewrite into something more reliable

	// this function was developed iteratively on Codereview.stackexchange
	// with the assistance of @Corbin
	std::size_t len = str.size();
	while (std::isspace(str[len - 1]))
		len--;
	if (len == 0)
		return DefaultValue;
	errno = 0;
	char *s = new char[len + 1];
	std::strncpy(s, str.c_str(), len);
	char *p;
	int result = strtol(s, &p, 0);
	delete[] s;
	if( ( *p != '\0' ) || ( errno != 0 ) )
	{
		return DefaultValue;
	}
	return result;
}

std::string ToLower(std::string const &text)
{
	std::string lowercase( text );
	std::transform(text.begin(), text.end(), lowercase.begin(), ::tolower);
	return lowercase;
}

std::string ToUpper(std::string const &text)
{
	std::string uppercase( text );
	std::transform(text.begin(), text.end(), uppercase.begin(), ::toupper);
	return uppercase;
}

void ComputeArc(double X0, double Y0, double Xn, double Yn, double R, double L, double dL,
                double &phi, double &Xout, double &Yout)
/*wylicza polozenie Xout Yout i orientacje phi punktu na elemencie dL luku*/
{
    double dX;
    double dY;
    double Xc;
    double Yc;
    double gamma;
    double alfa;
    double AbsR;

    if ((R != 0) && (L != 0))
    {
        AbsR = abs(R);
        dX = Xn - X0;
        dY = Yn - Y0;
        if (dX != 0)
            gamma = atan(dY * 1.0 / dX);
        else if (dY > 0)
            gamma = M_PI * 1.0 / 2;
        else
            gamma = 3 * M_PI * 1.0 / 2;
        alfa = L * 1.0 / R;
        phi = gamma - (alfa + M_PI * Round(R * 1.0 / AbsR)) * 1.0 / 2;
        Xc = X0 - AbsR * cos(phi);
        Yc = Y0 - AbsR * sin(phi);
        phi = phi + alfa * dL * 1.0 / L;
        Xout = AbsR * cos(phi) + Xc;
        Yout = AbsR * sin(phi) + Yc;
    }
}

void ComputeALine(double X0, double Y0, double Xn, double Yn, double L, double R, double &Xout,
                  double &Yout)
{
    double dX;
    double dY;
    double gamma;
    double alfa;
    /*   pX,pY : real;*/

    alfa = 0; // ABu: bo nie bylo zainicjowane
    dX = Xn - X0;
    dY = Yn - Y0;
    if (dX != 0)
        gamma = atan(dY * 1.0 / dX);
    else if (dY > 0)
        gamma = M_PI * 1.0 / 2;
    else
        gamma = 3 * M_PI * 1.0 / 2;
    if (R != 0)
        alfa = L * 1.0 / R;
    Xout = X0 + L * cos(alfa * 1.0 / 2 - gamma);
    Yout = Y0 + L * sin(alfa * 1.0 / 2 - gamma);
}

bool FileExists( std::string const &Filename ) {

	std::ifstream file( Filename );
	if( file.is_open() == false ) { return false; }
	else                          { return true; }
}

/*
//graficzne:

double Xhor(double h)
{
    return h * Hstep + Xmin;
}

double Yver(double v)
{
    return (Vsize - v) * Vstep + Ymin;
}

long Horiz(double x)
{
    x = (x - Xmin) * 1.0 / Hstep;
    if (x > -INT_MAX)
        if (x < INT_MAX)
            return Round(x);
        else
            return INT_MAX;
    else
        return -INT_MAX;
}

long Vert(double Y)
{
    Y = (Y - Ymin) * 1.0 / Vstep;
    if (Y > -INT_MAX)
        if (Y < INT_MAX)
            return Vsize - Round(Y);
        else
            return INT_MAX;
    else
        return -INT_MAX;
}
*/

// NOTE: this now does nothing.
void ClearPendingExceptions()
// resetuje b³êdy FPU, wymagane dla Trunc()
{
    ; /*?*/ /* ASM
       FNCLEX
 ASM END */
}

// END
