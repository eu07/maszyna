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
Friction coefficient.
Copyright (C) 2007-2013 Maciej Cierniak
*/

#include "stdafx.h"
#include "friction.h"

double TFricMat::GetFC(double N, double Vel)
{
    return 1;
}

double TP10Bg::GetFC(double N, double Vel)
{
    //  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100))*(1.0032-0.0007*N-0.0001*N*N);
    //  GetFC:=47/(2*Vel+100)*(2.145-0.0538*N+0.00074*N*N-0.00000536*N*N*N)/2.145;
    //    GetFC:=46/(2*Vel+100)*(11.33-0.105*N)/(11.33+0.179*N);
    //  GetFC:=49/(2*Vel+100)*(13.08-0.083*N)/(12.94+0.285*N);
    // if Vel<20 then Vel:=20;
    //  Vel:= Vel-20;
    //  GetFC:=0.52*((1*Vel+100)/(5.0*Vel+100))*(13.08-0.083*N)/(12.94+0.285*N);
    //    GetFC:=Min0R(0.67*(1*(277-2.66*Vel)/(100+2.1*Vel)+0.23*(-686+8.27*Vel)/(100+1.16*Vel))*(13.08-0.083*N)/(12.94+0.285*N),0.4);
    return exp(-0.022 * N) * (0.19 - 0.095 * exp(-Vel * 1.0 / 25.7)) +
            0.384 * exp(-Vel * 1.0 / 25.7) - 0.028;
}

double TP10Bgu::GetFC(double N, double Vel)
{
    //  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100));
    //  GetFC:=47/(2*Vel+100)*(2.137-0.0514*N+0.000832*N*N-0.00000604*N*N*N)/2.137;
    //  GetFC:=0.49*100/(2*Vel+100)*(11.33-0.013*N)/(11.33+0.280*N);
    //  GetFC:=0.52*((Vel+100)/(5.0*Vel+100))*(11.33-0.013*N)/(11.33+0.280*N);
    // if Vel<20 then Vel:=20;
    //  Vel:= Vel-20;
    //  GetFC:=0.52*((0.0*Vel+120)/(5*Vel+100))*(11.33-0.013*N)/(11.33+0.280*N);
    //  GetFC:=0.49*100/(3*Vel+100)*(11.33-0.013*N)/(11.33+0.280*N);
    //    GetFC:=Min0R(0.67*(1*(277-2.66*Vel)/(100+2.1*Vel)+0.23*(-686+8.27*Vel)/(100+1.16*Vel))*(11.33-0.013*N)/(11.33+0.280*N),0.4);
    return exp(-0.017 * N) * (0.18 - 0.09 * exp(-Vel * 1.0 / 25.7)) +
            0.381 * exp(-Vel * 1.0 / 25.7) - 0.022; // 0.05*exp(-0.2*N);
}

double TP10yBg::GetFC(double N, double Vel)
{
    double A;
    double C;
    double u0;
    double V0;

    A = 2.135 * exp(-0.03726 * N) - 0.5;
    C = 0.353 - A * 0.029;
    u0 = 0.41 - C;
    V0 = 25.7 + 20 * A;
    return (u0 + C * exp(-Vel * 1.0 / V0));
}

double TP10yBgu::GetFC(double N, double Vel)
{
    double A;
    double C;
    double u0;
    double V0;

    A = 1.68 * exp(-0.02735 * N) - 0.5;
    C = 0.353 - A * 0.044;
    u0 = 0.41 - C;
    V0 = 25.7 + 21 * A;
    return (u0 + C * exp(-Vel * 1.0 / V0));
}

double TP10::GetFC(double N, double Vel)
{
    return
        0.60 * ((1.6 * N + 100) * 1.0 / (8.0 * N + 100)) * ((Vel + 100) * 1.0 / (5 * Vel + 100));
    //  GetFC:=43/(2*Vel+100)*(2.145-0.0538*N+0.00074*N*N-0.00000536*N*N*N)/2.145
}

double TFR513::GetFC(double N, double Vel)
{
    return 0.3 - Vel * 0.00081;
    //  GetFC:=43/(2*Vel+100)*(2.145-0.0538*N+0.00074*N*N-0.00000536*N*N*N)/2.145
}

double TCosid::GetFC(double N, double Vel)
{
    return 0.27;
}

double TDisk1::GetFC(double N, double Vel)
{
    return 0.2375 + 0.000885 * N - 0.000345 * N * N;
}

double TDisk2::GetFC(double N, double Vel)
{
    return 0.27;
}

double TFR510::GetFC(double N, double Vel)
{
    return 0.15;
}

//END
