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
#include "mctools.h"
#include <cmath>
#include <ifstream>

/*================================================*/

std::string Ups(std::string s)
{
    int jatka;
    std::string swy;

    swy = "";
    {
        long jatka_end = s.length() + 1;
        for (jatka = 0; jatka < jatka_end; jatka++)
            swy = swy + UpCase(s[jatka]);
    }
    return swy;
} /*=Ups=*/

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
    double result;
    if (x1 > x2)
        Max0R = x1;
    else
        Max0R = x2;
    return result;
}

double Min0R(double x1, double x2)
{
    double result;
    if (x1 < x2)
        Min0R = x1;
    else
        Min0R = x2;
    return result;
}

int Sign(double x)
{
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    return 0;
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
        if ((Flag & Value) == 0)
        {
            Flag |= Value;
            return true; // true, gdy by³o wczeœniej 0 i zosta³o ustawione
        }
    if (Value < 0)
        if ((Flag & abs(Value)) == abs(Value))
        {
            Flag |= Value; // Value jest ujemne, czyli zerowanie flagi
            return true; // true, gdy by³o wczeœniej 1 i zosta³o wyzerowane
        }
	return false;
}

bool FuzzyLogic(double Test, double Threshold, double Probability)
{
    if ((Test > Threshold) && (!DebugModeFlag))
        return
            (Random < Probability * Threshold * 1.0 / Test) /*im wiekszy Test tym wieksza szansa*/;
    else
        return false;
}

bool FuzzyLogicAI(double Test, double Threshold, double Probability)
{
    if ((Test > Threshold))
        return
            (Random < Probability * Threshold * 1.0 / Test) /*im wiekszy Test tym wieksza szansa*/;
    else
        return false;
}

std::string ReadWord(std::ifstream &infile)
{
    std::string result;
    std::string s;
    char c;
    bool nextword;

    s = "";
    nextword = false;
    while ((!eof(infile)) && (!nextword))
    {
        read(infile, c);
        if (c in _Spacesigns)
            if (s != "")
                nextword = true;
        if (!(c in _Spacesigns))
            s = s + c;
    }
    ReadWord = s;
    return result;
}

std::string Cut_Space(std::string s, int Just)
{
    std::string result;
    unsigned char ii;

    switch (Just)
    {
    case CutLeft:
    {
        ii = 0;
        while ((ii < length(s)) && (s[ii + 1] == _SPACE))
            ++ii;
        s = Copy(s, ii + 1, length(s) - ii);
    }
    case CutRight:
    {
        ii = length(s);
        while ((ii > 0) && (s[ii] == _SPACE))
            --ii;
        s = Copy(s, 1, ii);
    }
    case CutBoth:
    {
        s = Cut_Space(s, CutLeft);
        s = Cut_Space(s, CutRight);
    }
    }
    Cut_Space = s;
    return result;
}
/*Cut_Space*/

std::string ExtractKeyWord(std::string InS, std::string KeyWord)
{
    std::string result;
    std::string s;
    unsigned char kwp;

    InS = InS + " ";
    kwp = Pos(KeyWord, InS);
    if (kwp > 0)
    {
        s = Copy(InS, kwp, length(InS));
        s = Copy(s, 1, Pos(" ", s) - 1);
    }
    else
        s = "";
    ExtractKeyWord = s;
    return result;
}

std::string DUE(std::string s) /*Delete Until Equal sign*/
{
    std::string result;
    DUE = Copy(s, Pos("=", s) + 1, length(s));
    return result;
}

std::string DWE(std::string s) /*Delete While Equal sign*/
{
    std::string result;
    unsigned char ep;

    ep = Pos("=", s);
    if (ep > 0)
        DWE = Copy(s, 1, ep - 1);
    else
        DWE = s;
    return result;
}

std::string Ld2Sp(std::string s) /*Low dash to Space sign*/
{
    std::string result;
    unsigned char b;
    std::string s2;

    s2 = "";
    {
        long b_end = length(s) + 1;
        for (b = 1; b < b_end; ++b)
            if (s[b] == "_")
                s2 = s2 + " ";
            else
                s2 = s2 + s[b];
    }
    Ld2Sp = s2;
    return result;
}

std::string Tab2Sp(std::string s) /*Tab to Space sign*/
{
    std::string result;
    unsigned char b;
    std::string s2;

    s2 = "";
    {
        long b_end = length(s) + 1;
        for (b = 1; b < b_end; ++b)
            if (s[b] == (char)9)
                s2 = s2 + " ";
            else
                s2 = s2 + s[b];
    }
    Tab2Sp = s2;
    return result;
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
            gamma = Pi * 1.0 / 2;
        else
            gamma = 3 * Pi * 1.0 / 2;
        alfa = L * 1.0 / R;
        phi = gamma - (alfa + Pi * Round(R * 1.0 / AbsR)) * 1.0 / 2;
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
        gamma = Pi * 1.0 / 2;
    else
        gamma = 3 * Pi * 1.0 / 2;
    if (R != 0)
        alfa = L * 1.0 / R;
    Xout = X0 + L * cos(alfa * 1.0 / 2 - gamma);
    Yout = Y0 + L * sin(alfa * 1.0 / 2 - gamma);
}

/*graficzne:*/

double Xhor(double h)
{
    double result;
    Xhor = h * Hstep + Xmin;
    return result;
}

double Yver(double v)
{
    double result;
    Yver = (Vsize - v) * Vstep + Ymin;
    return result;
}

long Horiz(double x)
{
    long result;
    x = (x - Xmin) * 1.0 / Hstep;
    if (x > -MaxInt)
        if (x < MaxInt)
            Horiz = Round(x);
        else
            Horiz = MaxInt;
    else
        Horiz = -MaxInt;
    return result;
}

long Vert(double Y)
{
    long result;
    Y = (Y - Ymin) * 1.0 / Vstep;
    if (Y > -MaxInt)
        if (Y < MaxInt)
            Vert = Vsize - Round(Y);
        else
            Vert = MaxInt;
    else
        Vert = -MaxInt;
    return result;
}

void ClearPendingExceptions()
// resetuje b³êdy FPU, wymagane dla Trunc()
{
    ; /*?*/ /* ASM
       FNCLEX
 ASM END */
}

// END
