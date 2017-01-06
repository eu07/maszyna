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
Brakes. Oerlikon ESt.
Copyright (C) 2007-2014 Maciej Cierniak
*/

#include "hamulce.h"
#include "Mover.h"
#include <cmath>
#include <typeinfo>

//---FUNKCJE OGOLNE---

static double const DPL = 0.25;
double TFV4aM::pos_table[11] = {-2, 6, -1, 0, -2, 1, 4, 6, 0, 0, 0};
double TMHZ_EN57::pos_table[11] = {-2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0};
double TM394::pos_table[11] = {-1, 5, -1, 0, 1, 2, 4, 5, 0, 0, 0};
double TH14K1::BPT_K[6][2] = {{10, 0}, {4, 1}, {0, 1}, {4, 0}, {4, -1}, {15, -1}};
double TH14K1::pos_table[11] = {-1, 4, -1, 0, 1, 2, 3, 4, 0, 0, 0};
double TSt113::BPT_K[6][2] = {{10, 0}, {4, 1}, {0, 1}, {4, 0}, {4, -1}, {15, -1}};
double TSt113::BEP_K[7] = {0, -1, 1, 0, 0, 0, 0};
double TSt113::pos_table[11] = {-1, 5, -1, 0, 2, 3, 4, 5, 0, 0, 1};
double TFVel6::pos_table[11] = {-1, 6, -1, 0, 6, 4, 4.7, 5, -1, 0, 1};

double PR(double P1, double P2)
{
    double PH = Max0R(P1, P2) + 0.1;
    double PL = P1 + P2 - PH + 0.2;
    return (P2 - P1) / (1.13 * PH - PL);
}

double PF_old(double P1, double P2, double S)
{
    double PH = Max0R(P1, P2) + 1;
    double PL = P1 + P2 - PH + 2;
    if (PH - PL < 0.0001)
        return 0;
    else if ((PH - PL) < 0.05)
        return 20 * (PH - PL) * (PH + 1) * 222 * S * (P2 - P1) / (1.13 * PH - PL);
    else
        return (PH + 1) * 222 * S * (P2 - P1) / (1.13 * PH - PL);
}

double PF(double P1, double P2, double S, double DP)
{
    double PH = Max0R(P1, P2) + 1; // wyzsze cisnienie absolutne
    double PL = P1 + P2 - PH + 2; // nizsze cisnienie absolutne
    double sg = PL / PH; // bezwymiarowy stosunek cisnien
    double FM = PH * 197 * S * Sign(P2 - P1); // najwyzszy mozliwy przeplyw, wraz z kierunkiem
    if ((sg > 0.5)) // jesli ponizej stosunku krytycznego
        if ((PH - PL) < DP) // niewielka roznica cisnien
            return (1 - sg) / DPL * FM * 2 * sqrt((DP) * (PH - DP));
        //      return 1/DPL*(PH-PL)*fm*2*SQRT((sg)*(1-sg));
        else
            return FM * 2 * sqrt((sg) * (1 - sg));
    else // powyzej stosunku krytycznego
        return FM;
}

double PF1(double P1, double P2, double S)
{
    static double const DPS = 0.001;

    double PH = Max0R(P1, P2) + 1; // wyzsze cisnienie absolutne
    double PL = P1 + P2 - PH + 2; // nizsze cisnienie absolutne
    double sg = PL / PH; // bezwymiarowy stosunek cisnien
    double FM = PH * 197 * S * Sign(P2 - P1); // najwyzszy mozliwy przeplyw, wraz z kierunkiem
    if ((sg > 0.5)) // jesli ponizej stosunku krytycznego
        if ((sg < DPS)) // niewielka roznica cisnien
            return (1 - sg) / DPS * FM * 2 * sqrt((DPS) * (1 - DPS));
        else
            return FM * 2 * sqrt((sg) * (1 - sg));
    else // powyzej stosunku krytycznego
        return FM;
}

long lround(double value)
{
    return floorl(value + 0.5);
}

double PFVa(double PH, double PL, double S, double LIM,
            double DP) // zawor napelniajacy z PH do PL, PL do LIM
{
    if (LIM > PL)
    {
        LIM = LIM + 1;
        PH = PH + 1; // wyzsze cisnienie absolutne
        PL = PL + 1; // nizsze cisnienie absolutne
        double sg = PL / PH; // bezwymiarowy stosunek cisnien
        double FM = PH * 197 * S; // najwyzszy mozliwy przeplyw, wraz z kierunkiem
        if ((LIM - PL) < DP)
            FM = FM * (LIM - PL) / DP; // jesli jestesmy przy nastawieniu, to zawor sie przymyka
        if ((sg > 0.5)) // jesli ponizej stosunku krytycznego
            if ((PH - PL) < DPL) // niewielka roznica cisnien
                return (PH - PL) / DPL * FM * 2 * sqrt((sg) * (1 - sg));
            else
                return FM * 2 * sqrt((sg) * (1 - sg));
        else // powyzej stosunku krytycznego
            return FM;
    }
    else
        return 0;
}

double PFVd(double PH, double PL, double S, double LIM,
            double DP) // zawor wypuszczajacy z PH do PL, PH do LIM
{
    if (LIM < PH)
    {
        LIM = LIM + 1;
        PH = PH + 1; // wyzsze cisnienie absolutne
        PL = PL + 1; // nizsze cisnienie absolutne
        double sg = PL / PH; // bezwymiarowy stosunek cisnien
        double FM = PH * 197 * S; // najwyzszy mozliwy przeplyw, wraz z kierunkiem
        if ((PH - LIM) < 0.1)
            FM = FM * (PH - LIM) / DP; // jesli jestesmy przy nastawieniu, to zawor sie przymyka
        if ((sg > 0.5)) // jesli ponizej stosunku krytycznego
            if ((PH - PL) < DPL) // niewielka roznica cisnien
                return (PH - PL) / DPL * FM * 2 * sqrt((sg) * (1 - sg));
            else
                return FM * 2 * sqrt((sg) * (1 - sg));
        else // powyzej stosunku krytycznego
            return FM;
    }
    else
        return 0;
}

//---ZBIORNIKI---

double TReservoir::pa()
{
    return 0.1 * Vol / Cap;
}

double TReservoir::P()
{
    return Vol / Cap;
}

void TReservoir::Flow(double dv)
{
    dVol = dVol + dv;
}

TReservoir::TReservoir()
{
    // inherited:: Create;
    Cap = 1;
    Vol = 0;
    dVol = 0;
}

void TReservoir::Act()
{
    Vol = Vol + dVol;
    dVol = 0;
}

void TReservoir::CreateCap(double Capacity)
{
    Cap = Capacity;
}

void TReservoir::CreatePress(double Press)
{
    Vol = Cap * Press;
    dVol = 0;
}

//---SILOWNIK---
double TBrakeCyl::pa()
// var VtoC: real;
{
    //  VtoC:=Vol/Cap;
    return P() * 0.1;
}

/* NOWSZA WERSJA - maksymalne ciœnienie to ok. 4,75 bar, co powoduje
//                 problemy przy rapidzie w lokomotywach, gdyz jest3
//                 osiagany wierzcholek paraboli
function TBrakeCyl.P:real;
var VtoC: real;
begin
  VtoC:=Vol/Cap;
  if VtoC<0.06 then P:=VtoC/4
  else if VtoC>0.88 then P:=0.5+(VtoC-0.88)*1.043-0.064*(VtoC-0.88)*(VtoC-0.88)
  else P:=0.15+0.35/0.82*(VtoC-0.06);
end; */

//(* STARA WERSJA
double TBrakeCyl::P()
{
    static double const VS = 0.005;
    static double const pS = 0.05;
    static double const VD = 1.10;
    static double const cD = 1;
    static double const pD = VD - cD;

    double VtoC = Vol / Cap; // stosunek cisnienia do objetosci
    //  P:=VtoC;
    if (VtoC < VS)
        return VtoC * pS / VS; // objetosc szkodliwa
    else if (VtoC > VD)
        return VtoC - cD; // caly silownik;
    else
        return pS + (VtoC - VS) / (VD - VS) * (pD - pS); // wysuwanie tloka
} //*)

//---HAMULEC---
/*
constructor TBrake.Create(i_mbp, i_bcr, i_bcd, i_brc: real; i_bcn, i_BD, i_mat, i_ba, i_nbpa: int);
begin
  inherited Create;
  MaxBP:=i_mbp;
  BCN:=i_bcn;
  BCA:=i_bcn*i_bcr*i_bcr*pi;
  BA:=i_ba;
  NBpA:=i_nbpa;
  BrakeDelays:=i_BD;

//tworzenie zbiornikow
  BrakeCyl.CreateCap(i_bcd*BCA*1000);
  BrakeRes.CreateCap(i_brc);
  ValveRes.CreateCap(0.2);

//  FM.Free;
//materialy cierne
  case i_mat of
  bp_P10Bg:   FM:=TP10Bg.Create;
  bp_P10Bgu:  FM:=TP10Bgu.Create;
  else //domyslnie
  FM:=TP10.Create;
  end;


end  ; */

TBrake::TBrake(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD,
               int i_mat, int i_ba, int i_nbpa)
{
    // inherited:: Create;
    MaxBP = i_mbp;
    BCN = i_bcn;
    BCM = 1;
    BCA = i_bcn * i_bcr * i_bcr * pi;
    BA = i_ba;
    NBpA = i_nbpa;
    BrakeDelays = i_BD;
    BrakeDelayFlag = bdelay_P;
    DCV = false;
    ASBP = 0.0;
    BrakeStatus = b_hld;
    SoundFlag = 0;
    // 210.88
    //  SizeBR:=i_bcn*i_bcr*i_bcr*i_bcd*40.17*MaxBP/(5-MaxBP);  //objetosc ZP w stosunku do cylindra
    //  14" i cisnienia 4.2 atm
    SizeBR = i_brc * 0.0128;
    SizeBC = i_bcn * i_bcr * i_bcr * i_bcd * 210.88 * MaxBP /
             4.2; // objetosc CH w stosunku do cylindra 14" i cisnienia 4.2 atm

    //  BrakeCyl:=TReservoir.Create;
    BrakeCyl = new TBrakeCyl();
    BrakeRes = new TReservoir();
    ValveRes = new TReservoir();

    // tworzenie zbiornikow
    BrakeCyl->CreateCap(i_bcd * BCA * 1000);
    BrakeRes->CreateCap(i_brc);
    ValveRes->CreateCap(0.25);

    //  FM.Free;
    // materialy cierne
    i_mat = i_mat & (255 - bp_MHS);
    switch (i_mat)
    {
    case bp_P10Bg:
        FM = new TP10Bg();
        break;
    case bp_P10Bgu:
        FM = new TP10Bgu();
        break;
    case bp_FR513:
        FM = new TFR513();
        break;
    case bp_FR510:
        FM = new TFR510();
        break;
    case bp_Cosid:
        FM = new TCosid();
        break;
    case bp_P10yBg:
        FM = new TP10yBg();
        break;
    case bp_P10yBgu:
        FM = new TP10yBgu();
        break;
    case bp_D1:
        FM = new TDisk1();
        break;
    case bp_D2:
        FM = new TDisk2();
        break;
    default: // domyslnie
        FM = new TP10();
    }
}

// inicjalizacja hamulca (stan poczatkowy)
void TBrake::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    BrakeDelayFlag = BDF;
}

// pobranie wspolczynnika tarcia materialu
double TBrake::GetFC(double Vel, double N)
{
    double result;
    return FM->GetFC(N, Vel);
}

// cisnienie cylindra hamulcowego
double TBrake::GetBCP()
{
    return BrakeCyl->P();
}

// ciœnienie steruj¹ce hamowaniem elektro-dynamicznym
double TBrake::GetEDBCP()
{
    return 0;
}

// cisnienie zbiornika pomocniczego
double TBrake::GetBRP()
{
    return BrakeRes->P();
}

// cisnienie komory wstepnej
double TBrake::GetVRP()
{
    return ValveRes->P();
}

// cisnienie zbiornika sterujacego
double TBrake::GetCRP()
{
    return 0;
}

// przeplyw z przewodu glowneg
double TBrake::GetPF(double PP, double dt, double Vel)
{
    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    return 0;
}

// przeplyw z przewodu zasilajacego
double TBrake::GetHPFlow(double HP, double dt)
{
    return 0;
}

double TBrake::GetBCF()
{
    return BCA * 100 * BrakeCyl->P();
}

bool TBrake::SetBDF(int nBDF)
{
    bool result;
    if (((nBDF & BrakeDelays) == nBDF) && (nBDF != BrakeDelayFlag))
    {
        BrakeDelayFlag = nBDF;
        return true;
    }
    else
        return false;
}

void TBrake::Releaser(int state)
{
    BrakeStatus = (BrakeStatus & 247) || state * b_rls;
}

void TBrake::SetEPS(double nEPS)
{
}

void TBrake::ASB(int state)
{ // 255-b_asb(32)
    BrakeStatus = (BrakeStatus & 223) || state * b_asb;
}

int TBrake::GetStatus()
{
    return BrakeStatus;
}

int TBrake::GetSoundFlag()
{
    int result = SoundFlag;
    SoundFlag = 0;
    return result;
}

void TBrake::SetASBP(double Press)
{
    ASBP = Press;
}

void TBrake::ForceEmptiness()
{
    ValveRes->CreatePress(0);
    BrakeRes->CreatePress(0);
    ValveRes->Act();
    BrakeRes->Act();
}

//---WESTINGHOUSE---

void TWest::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TBrake::Init(PP, HPP, LPP, BP, BDF);
    ValveRes->CreatePress(PP);
    BrakeCyl->CreatePress(BP);
    BrakeRes->CreatePress(PP / 2 + HPP / 2);
    // BrakeStatus:=3*int(BP>0.1);
}

double TWest::GetPF(double PP, double dt, double Vel)
{
    double dv;
    double dV1;
    double VVP;
    double BVP;
    double CVP;
    double BCP;
    double temp;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    CVP = BrakeCyl->P();
    BCP = BrakeCyl->P();

    if ((BrakeStatus & 1) == 1)
        if ((VVP + 0.03 < BVP))
            BrakeStatus |= 2;
        else if ((VVP > BVP + 0.1))
            BrakeStatus &= 252;
        else if ((VVP > BVP))
            BrakeStatus &= 253;
        else
            ;
    else if ((VVP + 0.25 < BVP))
        BrakeStatus |= 3;

    if (((BrakeStatus & b_hld) == b_off) && (!DCV))
        dv = PF(0, CVP, 0.0068 * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);

    if ((BCP > LBP + 0.01) && (DCV))
        dv = PF(0, CVP, 0.1 * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);

    // hamulec EP
    temp = BVP * int(EPS > 0);
    dv = PF(temp, LBP, 0.0015) * dt * EPS * EPS * int(LBP * EPS < MaxBP * LoadC);
    LBP = LBP - dv;
    dv = 0;

    // przeplyw ZP <-> silowniki
    if (((BrakeStatus & b_on) == b_on) && ((TareBP < 0.1) || (BCP < MaxBP * LoadC)))
        if ((BVP > LBP))
        {
            DCV = false;
            dv = PF(BVP, CVP, 0.017 * SizeBC) * dt;
        }
        else
            dv = 0;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);
    if ((DCV))
        dVP = PF(LBP, BCP, 0.01 * SizeBC) * dt;
    else
        dVP = 0;
    BrakeCyl->Flow(-dVP);
    if ((dVP > 0))
        dVP = 0;
    // przeplyw ZP <-> rozdzielacz
    if (((BrakeStatus & b_hld) == b_off))
        dv = PF(BVP, VVP, 0.0011 * SizeBR) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dv * 0.95;
    ValveRes->Flow(-0.05 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01 * SizeBR) * dt;
    ValveRes->Flow(-dv);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    return dv - dV1;
}

double TWest::GetHPFlow(double HP, double dt)
{
    return dVP;
}

void TWest::SetLBP(double P)
{
    LBP = P;
    if (P > BrakeCyl->P())
        //   begin
        DCV = true;
    //   end
    //  else
    //    LBP:=P;
}

void TWest::SetEPS(double nEPS)
{
    double BCP;

    BCP = BrakeCyl->P();
    if (nEPS > 0)
        DCV = true;
    else if (nEPS == 0)
    {
        if ((EPS != 0))
        {
            if ((LBP > 0.4))
                LBP = BrakeCyl->P();
            if ((LBP < 0.15))
                LBP = 0;
        }
    }
    EPS = nEPS;
}

void TWest::PLC(double mass)
{
    LoadC = 1 +
            int(mass < LoadM) *
                ((TareBP + (MaxBP - TareBP) * (mass - TareM) / (LoadM - TareM)) / MaxBP - 1);
}

void TWest::SetLP(double TM, double LM, double TBP)
{
    TareM = TM;
    LoadM = LM;
    TareBP = TBP;
}

//---OERLIKON EST4---
void TESt::CheckReleaser(double dt)
{
    double VVP;
    double BVP;
    double CVP;

    VVP = Min0R(ValveRes->P(), BrakeRes->P() + 0.05);
    CVP = CntrlRes->P() - 0.0;

    // odluzniacz
    if ((BrakeStatus & b_rls) == b_rls)
        if ((CVP - VVP < 0))
            BrakeStatus &= 247;
        else
        {
            CntrlRes->Flow(+PF(CVP, 0, 0.1) * dt);
        }
}

void TESt::CheckState(double BCP, double &dV1)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    //  if (BVP<VVP) then
    //    VVP:=(BVP+VVP)/2;
    CVP = CntrlRes->P() - 0.0;

    // sprawdzanie stanu
    if (((BrakeStatus & 1) == 1) && (BCP > 0.25))
        if ((VVP + 0.003 + BCP / BVM < CVP))
            BrakeStatus |= 2; // hamowanie stopniowe
        else if ((VVP - 0.003 + (BCP - 0.1) / BVM > CVP))
            BrakeStatus &= 252; // luzowanie
        else if ((VVP + BCP / BVM > CVP))
            BrakeStatus &= 253; // zatrzymanie napelaniania
        else
            ;
    else if ((VVP + 0.10 < CVP) && (BCP < 0.25)) // poczatek hamowania
    {
        if ((BrakeStatus & 1) == 0)
        {
            ValveRes->CreatePress(0.02 * VVP);
            SoundFlag |= sf_Acc;
            ValveRes->Act();
        }
        BrakeStatus |= 3;

        //     ValveRes.CreatePress(0);
        //     dV1:=1;
    }
    else if ((VVP + (BCP - 0.1) / BVM < CVP) && ((CVP - VVP) * BVM > 0.25) &&
             (BCP > 0.25)) // zatrzymanie luzowanie
        BrakeStatus |= 1;

    if ((BrakeStatus & 1) == 0)
        SoundFlag |= sf_CylU;
}

double TESt::CVs(double BP)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    CVP = CntrlRes->P();
    VVP = ValveRes->P();

    // przeplyw ZS <-> PG
    if ((VVP < CVP - 0.12) || (BVP < CVP - 0.3) || (BP > 0.4))
        return 0;
    else if ((VVP > CVP + 0.4))
        if ((BVP > CVP + 0.2))
            return 0.23;
        else
            return 0.05;
    else if ((BVP > CVP - 0.1))
        return 1;
    else
        return 0.3;
}

double TESt::BVs(double BCP)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    CVP = CntrlRes->P();
    VVP = ValveRes->P();

    // przeplyw ZP <-> rozdzielacz
    if ((BVP < CVP - 0.3))
        return 0.6;
    else if ((BCP < 0.5))
        if ((VVP > CVP + 0.4))
            return 0.1;
        else
            return 0.3;
    else
        return 0;
}

double TESt::GetPF(double PP, double dt, double Vel)
{
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = BrakeCyl->P();
    CVP = CntrlRes->P() - 0.0;

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(BCP, dV1);
    CheckReleaser(dt);

    CVP = CntrlRes->P();
    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    temp = CVs(BCP);
    dv = PF(CVP, VVP, 0.0015 * temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, 0.0058 * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);

    // przeplyw ZP <-> silowniki
    if ((BrakeStatus & b_on) == b_on)
        dv = PF(BVP, BCP, 0.016 * SizeBC) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);

    // przeplyw ZP <-> rozdzielacz
    temp = BVs(BCP);
    //  if(BrakeStatus and b_hld)=b_off then
    if ((VVP - 0.05 > BVP))
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01) * dt;
    ValveRes->Flow(-dv);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return dv - dV1;
}

void TESt::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TBrake::Init(PP, HPP, LPP, BP, BDF);
    ValveRes->CreatePress(PP);
    BrakeCyl->CreatePress(BP);
    BrakeRes->CreatePress(PP);
    CntrlRes->CreateCap(15);
    CntrlRes->CreatePress(HPP);
    BrakeStatus = 0;

    BVM = 1.0 / (HPP - LPP) * MaxBP;

    BrakeDelayFlag = BDF;
}

void TESt::EStParams(double i_crc)
{
}

double TESt::GetCRP()
{
    return CntrlRes->P();
}

//---EP2---

void TEStEP2::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TLSt::Init(PP, HPP, LPP, BP, BDF);
    ImplsRes->CreateCap(1);
    ImplsRes->CreatePress(BP);

    BrakeRes->CreatePress(PP);

    BrakeDelayFlag = bdelay_P;
    BrakeDelays = bdelay_P;
}

double TEStEP2::GetPF(double PP, double dt, double Vel)
{
    double result;
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = ImplsRes->P();
    CVP = CntrlRes->P(); // 110115 - konsultacje warszawa1

    dv = 0;
    dV1 = 0;

    // odluzniacz
    CheckReleaser(dt);

    // sprawdzanie stanu
    if (((BrakeStatus & 1) == 1) && (BCP > 0.25))
        if ((VVP + 0.003 + BCP / BVM < CVP - 0.12))
            BrakeStatus |= 2; // hamowanie stopniowe;
        else if ((VVP - 0.003 + BCP / BVM > CVP - 0.12))
            BrakeStatus &= 252; // luzowanie;
        else if ((VVP + BCP / BVM > CVP - 0.12))
            BrakeStatus &= 253; // zatrzymanie napelaniania;
        else
            ;
    else if ((VVP + 0.10 < CVP - 0.12) && (BCP < 0.25)) // poczatek hamowania
    {
        //if ((BrakeStatus & 1) == 0)
        //{
        //    //       ValveRes.CreatePress(0.5*VVP);  //110115 - konsultacje warszawa1
        //    //       SoundFlag:=SoundFlag or sf_Acc;
        //    //       ValveRes.Act;
        //}
        BrakeStatus |= 3;
    }
    else if ((VVP + BCP / BVM < CVP - 0.12) && (BCP > 0.25)) // zatrzymanie luzowanie
        BrakeStatus |= 1;

    // przeplyw ZS <-> PG
    if ((BVP < CVP - 0.2) || (BrakeStatus > 0) || (BCP > 0.25))
        temp = 0;
    else if ((VVP > CVP + 0.4))
        temp = 0.1;
    else
        temp = 0.5;

    dv = PF(CVP, VVP, 0.0015 * temp / 1.8) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // hamulec EP
    temp = BVP * int(EPS > 0);
    dv = PF(temp, LBP, 0.00053 + 0.00060 * int(EPS < 0)) * dt * EPS * EPS *
         int(LBP * EPS < MaxBP * LoadC);
    LBP = LBP - dv;

    // luzowanie KI
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, 0.00083) * dt;
    else
        dv = 0;
    ImplsRes->Flow(-dv);
    // przeplyw ZP <-> KI
    if (((BrakeStatus & b_on) == b_on) && (BCP < MaxBP * LoadC))
        dv = PF(BVP, BCP, 0.0006) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    ImplsRes->Flow(-dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01 * SizeBR) * dt;
    ValveRes->Flow(-dv);

    result = dv - dV1;

    temp = Max0R(BCP, LBP);

    if ((ImplsRes->P() > LBP + 0.01))
        LBP = 0;

    // luzowanie CH
    if ((BrakeCyl->P() > temp + 0.005) || (Max0R(ImplsRes->P(), 8 * LBP) < 0.05))
        dv = PF(0, BrakeCyl->P(), 0.25 * SizeBC * (0.01 + (BrakeCyl->P() - temp))) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);
    // przeplyw ZP <-> CH
    if ((BrakeCyl->P() < temp - 0.005) && (Max0R(ImplsRes->P(), 8 * LBP) > 0.10) &&
        (Max0R(BCP, LBP) < MaxBP * LoadC))
        dv = PF(BVP, BrakeCyl->P(), 0.35 * SizeBC * (0.01 - (BrakeCyl->P() - temp))) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);

    ImplsRes->Act();
    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return result;
}

void TEStEP2::PLC(double mass)
{
    LoadC = 1 +
            int(mass < LoadM) *
                ((TareBP + (MaxBP - TareBP) * (mass - TareM) / (LoadM - TareM)) / MaxBP - 1);
}

void TEStEP2::SetEPS(double nEPS)
{
    EPS = nEPS;
    if ((EPS > 0) && (LBP + 0.01 < BrakeCyl->P()))
        LBP = BrakeCyl->P();
}

void TEStEP2::SetLP(double TM, double LM, double TBP)
{
    TareM = TM;
    LoadM = LM;
    TareBP = TBP;
}

//---EST3--

double TESt3::GetPF(double PP, double dt, double Vel)
{
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = BrakeCyl->P();
    CVP = CntrlRes->P() - 0.0;

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(BCP, dV1);
    CheckReleaser(dt);

    CVP = CntrlRes->P();
    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    temp = CVs(BCP);
    dv = PF(CVP, VVP, 0.0015 * temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, 0.0042 * (1.37 - int(BrakeDelayFlag == bdelay_G)) * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);
    // przeplyw ZP <-> silowniki
    if ((BrakeStatus & b_on) == b_on)
        dv = PF(BVP, BCP,
                0.017 * (1 + int((BCP < 0.58) && (BrakeDelayFlag == bdelay_G))) *
                    (1.13 - int((BCP > 0.6) && (BrakeDelayFlag == bdelay_G))) * SizeBC) *
             dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);
    // przeplyw ZP <-> rozdzielacz
    temp = BVs(BCP);
    if ((VVP - 0.05 > BVP))
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01) * dt;
    ValveRes->Flow(-dv);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return dv - dV1;
}

//---EST4-RAPID---

double TESt4R::GetPF(double PP, double dt, double Vel)
{
    double result;
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = ImplsRes->P();
    CVP = CntrlRes->P();

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(BCP, dV1);
    CheckReleaser(dt);

    CVP = CntrlRes->P();
    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    temp = CVs(BCP);
    dv = PF(CVP, VVP, 0.0015 * temp / 1.8) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie KI
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, 0.00037 * 1.14 * 15 / 19) * dt;
    else
        dv = 0;
    ImplsRes->Flow(-dv);
    // przeplyw ZP <-> KI
    if ((BrakeStatus & b_on) == b_on)
        dv = PF(BVP, BCP, 0.0014) * dt;
    else
        dv = 0;
    //  BrakeRes->Flow(dV);
    ImplsRes->Flow(-dv);
    // przeplyw ZP <-> rozdzielacz
    temp = BVs(BCP);
    if ((BVP < VVP - 0.05)) // or((PP<CVP)and(CVP<PP-0.1)
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01 * SizeBR) * dt;
    ValveRes->Flow(-dv);

    result = dv - dV1;

    RapidStatus = (BrakeDelayFlag == bdelay_R) && (((Vel > 55) && (RapidStatus)) || (Vel > 70));

    RapidTemp = RapidTemp + (0.9 * int(RapidStatus) - RapidTemp) * dt / 2;
    temp = 1.9 - RapidTemp;
    if (((BrakeStatus & b_asb) == b_asb))
        temp = 1000;
    // luzowanie CH
    if ((BrakeCyl->P() * temp > ImplsRes->P() + 0.005) || (ImplsRes->P() < 0.25))
        if (((BrakeStatus & b_asb) == b_asb))
            dv = PFVd(BrakeCyl->P(), 0, 0.115 * SizeBC * 4, ImplsRes->P() / temp) * dt;
        else
            dv = PFVd(BrakeCyl->P(), 0, 0.115 * SizeBC, ImplsRes->P() / temp) * dt;
    //   dV:=PF(0,BrakeCyl.P,0.115*sizeBC/2)*dt
    //   dV:=PFVd(BrakeCyl.P,0,0.015*sizeBC/2,ImplsRes.P/temp)*dt
    else
        dv = 0;
    BrakeCyl->Flow(-dv);
    // przeplyw ZP <-> CH
    if ((BrakeCyl->P() * temp < ImplsRes->P() - 0.005) && (ImplsRes->P() > 0.3))
        //   dV:=PFVa(BVP,BrakeCyl.P,0.020*sizeBC,ImplsRes.P/temp)*dt
        dv = PFVa(BVP, BrakeCyl->P(), 0.60 * SizeBC, ImplsRes->P() / temp) * dt;
    else
        dv = 0;
    BrakeRes->Flow(-dv);
    BrakeCyl->Flow(+dv);

    ImplsRes->Act();
    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return result;
}

void TESt4R::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TESt::Init(PP, HPP, LPP, BP, BDF);
    ImplsRes->CreateCap(1);
    ImplsRes->CreatePress(BP);

    BrakeDelayFlag = bdelay_R;
}

//---EST3/AL2---

double TESt3AL2::GetPF(double PP, double dt, double Vel)
{
    double result;
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = ImplsRes->P();
    CVP = CntrlRes->P() - 0.0;

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(BCP, dV1);
    CheckReleaser(dt);

    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    temp = CVs(BCP);
    dv = PF(CVP, VVP, 0.0015 * temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie KI
    if ((BrakeStatus && b_hld) == b_off)
        dv = PF(0, BCP, 0.00017 * (1.37 - int(BrakeDelayFlag == bdelay_G))) * dt;
    else
        dv = 0;
    ImplsRes->Flow(-dv);
    // przeplyw ZP <-> KI
    if (((BrakeStatus & b_on) == b_on) && (BCP < MaxBP))
        dv = PF(BVP, BCP,
                0.0008 * (1 + int((BCP < 0.58) && (BrakeDelayFlag == bdelay_G))) *
                    (1.13 - int((BCP > 0.6) && (BrakeDelayFlag == bdelay_G)))) *
             dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    ImplsRes->Flow(-dv);
    // przeplyw ZP <-> rozdzielacz
    temp = BVs(BCP);
    if ((VVP - 0.05 > BVP))
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01) * dt;
    ValveRes->Flow(-dv);
    result = dv - dV1;

    // luzowanie CH
    if ((BrakeCyl->P() > ImplsRes->P() * LoadC + 0.005) || (ImplsRes->P() < 0.15))
        dv = PF(0, BrakeCyl->P(), 0.015 * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);

    // przeplyw ZP <-> CH
    if ((BrakeCyl->P() < ImplsRes->P() * LoadC - 0.005) && (ImplsRes->P() > 0.15))
        dv = PF(BVP, BrakeCyl->P(), 0.020 * SizeBC) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);

    ImplsRes->Act();
    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return result;
}

void TESt3AL2::PLC(double mass)
{
    LoadC = 1 +
            int(mass < LoadM) *
                ((TareBP + (MaxBP - TareBP) * (mass - TareM) / (LoadM - TareM)) / MaxBP - 1);
}

void TESt3AL2::SetLP(double TM, double LM, double TBP)
{
    TareM = TM;
    LoadM = LM;
    TareBP = TBP;
}

void TESt3AL2::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TESt::Init(PP, HPP, LPP, BP, BDF);
    ImplsRes->CreateCap(1);
    ImplsRes->CreatePress(BP);
}

//---LSt---

double TLSt::GetPF(double PP, double dt, double Vel)
{
    double result;
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    // ValveRes.CreatePress(LBP);
    // LBP:=0;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = ImplsRes->P();
    CVP = CntrlRes->P();

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    if ((BrakeStatus & b_rls) == b_rls)
        if ((CVP < 0))
            BrakeStatus &= 247;
        else
        { // 008
            dv = PF1(CVP, BCP, 0.024) * dt;
            CntrlRes->Flow(+dv);
            //     dV1:=+dV; //minus potem jest
            //     ImplsRes->Flow(-dV1);
        }

    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    if (((CVP - BCP) * BVM > 0.5))
        temp = 0;
    else if ((VVP > CVP + 0.4))
        temp = 0.5;
    else
        temp = 0.5;

    dv = PF1(CVP, VVP, 0.0015 * temp / 1.8 / 2) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie KI  {G}
    //   if VVP>BCP then
    //    dV:=PF(VVP,BCP,0.00004)*dt
    //   else if (CVP-BCP)<1.5 then
    //    dV:=PF(VVP,BCP,0.00020*(1.33-int((CVP-BCP)*BVM>0.65)))*dt
    //  else dV:=0;      0.00025 P
    /*P*/
    if (VVP > BCP)
        dv = PF(VVP, BCP,
                0.00043 * (1.5 - int(((CVP - BCP) * BVM > 1) && (BrakeDelayFlag == bdelay_G))),
                0.1) *
             dt;
    else if ((CVP - BCP) < 1.5)
        dv = PF(VVP, BCP,
                0.001472 * (1.36 - int(((CVP - BCP) * BVM > 1) && (BrakeDelayFlag == bdelay_G))),
                0.1) *
             dt;
    else
        dv = 0;

    ImplsRes->Flow(-dv);
    ValveRes->Flow(+dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01, 0.1) * dt;
    ValveRes->Flow(-dv);

    result = dv - dV1;

    //  if Vel>55 then temp:=0.72 else
    //    temp:=1;{R}
    // cisnienie PP
    RapidTemp =
        RapidTemp + (RM * int((Vel > 55) && (BrakeDelayFlag == bdelay_R)) - RapidTemp) * dt / 2;
    temp = 1 - RapidTemp;
    if (EDFlag > 0.2)
        temp = 10000;

    // powtarzacz — podwojny zawor zwrotny
    temp = Max0R(((CVP - BCP) * BVM + ASBP * int((BrakeStatus & b_asb) == b_asb)) / temp, LBP);
    // luzowanie CH
    if ((BrakeCyl->P() > temp + 0.005) || (temp < 0.28))
        //   dV:=PF(0,BrakeCyl->P(),0.0015*3*sizeBC)*dt
        //   dV:=PF(0,BrakeCyl->P(),0.005*3*sizeBC)*dt
        dv = PFVd(BrakeCyl->P(), 0, 0.005 * 7 * SizeBC, temp) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);
    // przeplyw ZP <-> CH
    if ((BrakeCyl->P() < temp - 0.005) && (temp > 0.29))
        //   dV:=PF(BVP,BrakeCyl->P(),0.002*3*sizeBC*2)*dt
        dv = -PFVa(BVP, BrakeCyl->P(), 0.002 * 7 * SizeBC * 2, temp) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);

    ImplsRes->Act();
    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    //  LBP:=ValveRes->P();
    //  ValveRes.CreatePress(ImplsRes->P());
    return result;
}

void TLSt::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TESt4R::Init(PP, HPP, LPP, BP, BDF);
    ValveRes->CreateCap(1);
    ImplsRes->CreateCap(8);
    ImplsRes->CreatePress(PP);
    BrakeRes->CreatePress(8);
    ValveRes->CreatePress(PP);

    EDFlag = 0;

    BrakeDelayFlag = BDF;
}

void TLSt::SetLBP(double P)
{
    LBP = P;
}

double TLSt::GetEDBCP()
{
    double CVP;
    double BCP;

    CVP = CntrlRes->P();
    BCP = ImplsRes->P();
    return (CVP - BCP) * BVM;
}

void TLSt::SetED(double EDstate)
{
    EDFlag = EDstate;
}

void TLSt::SetRM(double RMR)
{
    RM = 1 - RMR;
}

double TLSt::GetHPFlow(double HP, double dt)
{
    double dv;

    dv = Min0R(PF(HP, BrakeRes->P(), 0.01 * dt), 0);
    BrakeRes->Flow(-dv);
    return dv;
}

//---EStED---

double TEStED::GetPF(double PP, double dt, double Vel)
{
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;
    double MPP;
    double nastG;
    int i;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = ImplsRes->P();
    CVP = CntrlRes->P() - 0.0;
    MPP = Miedzypoj->P();
    dV1 = 0;

    nastG = (BrakeDelayFlag & bdelay_G);

    // sprawdzanie stanu
    if ((BCP < 0.25) && (VVP + 0.08 > CVP))
        Przys_blok = false;

    // sprawdzanie stanu
    if ((VVP + 0.002 + BCP / BVM < CVP - 0.05) && (Przys_blok))
        BrakeStatus |= 3; // hamowanie stopniowe;
    else if ((VVP - 0.002 + (BCP - 0.1) / BVM > CVP - 0.05))
        BrakeStatus &= 252; // luzowanie;
    else if ((VVP + BCP / BVM > CVP - 0.05))
        BrakeStatus &= 253; // zatrzymanie napelaniania;
    else if ((VVP + (BCP - 0.1) / BVM < CVP - 0.05) && (BCP > 0.25)) // zatrzymanie luzowania
        BrakeStatus |= 1;

    if ((VVP + 0.10 < CVP) && (BCP < 0.25)) // poczatek hamowania
        if ((!Przys_blok))
        {
            ValveRes->CreatePress(0.75 * VVP);
            SoundFlag |=  sf_Acc;
            ValveRes->Act();
            Przys_blok = true;
        }

    if ((BCP > 0.5))
        Zamykajacy = true;
    else if ((VVP - 0.6 < MPP))
        Zamykajacy = false;

    if ((BrakeStatus & b_rls) == b_rls)
    {
        dv = PF(CVP, BCP, 0.024) * dt;
        CntrlRes->Flow(+dv);
    }

    // luzowanie
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, Nozzles[3] * nastG + (1 - nastG) * Nozzles[1]) * dt;
    else
        dv = 0;
    ImplsRes->Flow(-dv);
    if (((BrakeStatus & b_on) == b_on) && (BCP < MaxBP))
        dv =
            PF(BVP, BCP, Nozzles[2] * (nastG + 2 * int(BCP < 0.8)) + Nozzles[0] * (1 - nastG)) * dt;
    else
        dv = 0;
    ImplsRes->Flow(-dv);
    BrakeRes->Flow(dv);

    // przeplyw testowy miedzypojemnosci
    if ((MPP < CVP - 0.3))
        temp = Nozzles[4];
    else if ((BCP < 0.5))
        if ((Zamykajacy))
            temp = Nozzles[8]; // 1.25;
        else
            temp = Nozzles[7];
    else
        temp = 0;
    dv = PF(MPP, VVP, temp);

    if ((MPP < CVP - 0.17))
        temp = 0;
    else if ((MPP > CVP - 0.08))
        temp = Nozzles[5];
    else
        temp = Nozzles[6];
    dv = dv + PF(MPP, CVP, temp);

    if ((MPP - 0.05 > BVP))
        dv = dv + PF(MPP - 0.05, BVP, Nozzles[10] * nastG + (1 - nastG) * Nozzles[9]);
    if (MPP > VVP)
        dv = dv + PF(MPP, VVP, 0.02);
    Miedzypoj->Flow(dv * dt * 0.15);

    RapidTemp =
        RapidTemp + (RM * int((Vel > 55) && (BrakeDelayFlag == bdelay_R)) - RapidTemp) * dt / 2;
    temp = Max0R(1 - RapidTemp, 0.001);
    //  if EDFlag then temp:=1000;
    //  temp:=temp/(1-);

    // powtarzacz — podwojny zawor zwrotny
    temp = Max0R(LoadC * BCP / temp * Min0R(Max0R(1 - EDFlag, 0), 1), LBP);

    if ((BrakeCyl->P() > temp))
        dv = -PFVd(BrakeCyl->P(), 0, 0.02 * SizeBC, temp) * dt;
    else if ((BrakeCyl->P() < temp))
        dv = PFVa(BVP, BrakeCyl->P(), 0.02 * SizeBC, temp) * dt;
    else
        dv = 0;

    BrakeCyl->Flow(dv);
    if (dv > 0)
        BrakeRes->Flow(-dv);

    // przeplyw ZS <-> PG
    if ((MPP < CVP - 0.17))
        temp = 0;
    else if ((MPP > CVP - 0.08))
        temp = Nozzles[5];
    else
        temp = Nozzles[6];
    dv = PF(CVP, MPP, temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.02 * dv);
    dV1 = dV1 + 0.98 * dv;

    // przeplyw ZP <-> MPJ
    if ((MPP - 0.05 > BVP))
        dv = PF(BVP, MPP - 0.05, Nozzles[10] * nastG + (1 - nastG) * Nozzles[9]) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.98;
    ValveRes->Flow(-0.02 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.005) * dt; // 0.01
    ValveRes->Flow(-dv);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    Miedzypoj->Act();
    ImplsRes->Act();
    return dv - dV1;
}

void TEStED::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TLSt::Init(PP, HPP, LPP, BP, BDF);
    int i;

    ValveRes->CreatePress(PP);
    BrakeCyl->CreatePress(BP);

    //  CntrlRes:=TReservoir.Create;
    //  CntrlRes.CreateCap(15);
    //  CntrlRes.CreatePress(1*HPP);

    BrakeStatus = int(BP > 1);
    Miedzypoj->CreateCap(5);
    Miedzypoj->CreatePress(PP);

    ImplsRes->CreateCap(1);
    ImplsRes->CreatePress(BP);

    BVM = 1.0 / (HPP - 0.05 - LPP) * MaxBP;

    BrakeDelayFlag = BDF;
    Zamykajacy = false;
    EDFlag = 0;

    Nozzles[0] = 1.250 / 1.7;
    Nozzles[1] = 0.907;
    Nozzles[2] = 0.510 / 1.7;
    Nozzles[3] = 0.524 / 1.17;
    Nozzles[4] = 7.4;
    Nozzles[7] = 5.3;
    Nozzles[8] = 2.5;
    Nozzles[9] = 7.28;
    Nozzles[10] = 2.96;
    Nozzles[5] = 1.1;
    Nozzles[6] = 0.9;

    {
        for (i = 0; i < 11; ++i)
        {
            Nozzles[i] = Nozzles[i] * Nozzles[i] * 3.14159 / 4000;
        }
    }
}

double TEStED::GetEDBCP()
{
    return ImplsRes->P() * LoadC;
}

void TEStED::PLC(double mass)
{
    LoadC = 1 +
            int(mass < LoadM) *
                ((TareBP + (MaxBP - TareBP) * (mass - TareM) / (LoadM - TareM)) / MaxBP - 1);
}

void TEStED::SetLP(double TM, double LM, double TBP)
{
    TareM = TM;
    LoadM = LM;
    TareBP = TBP;
}

//---DAKO CV1---

void TCV1::CheckState(double BCP, double &dV1)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = Min0R(ValveRes->P(), BVP + 0.05);
    CVP = CntrlRes->P();

    // odluzniacz
    if (((BrakeStatus & b_rls) == b_rls) && (CVP - VVP < 0))
        BrakeStatus &=  247;

    // sprawdzanie stanu
    if ((BrakeStatus & b_hld) == b_hld)
        if ((VVP + 0.003 + BCP / BVM < CVP))
            BrakeStatus |= 2; // hamowanie stopniowe;
        else if ((VVP - 0.003 + BCP * 1.0 / BVM > CVP))
            BrakeStatus &= 252; // luzowanie;
        else if ((VVP + BCP * 1.0 / BVM > CVP))
            BrakeStatus &= 253; // zatrzymanie napelaniania;
        else
            ;
    else if ((VVP + 0.10 < CVP) && (BCP < 0.1)) // poczatek hamowania
    {
        BrakeStatus |= 3;
        dV1 = 1.25;
    }
    else if ((VVP + BCP / BVM < CVP) && (BCP > 0.25)) // zatrzymanie luzowanie
        BrakeStatus |= 1;
}

double TCV1::CVs(double BP)
{
    // przeplyw ZS <-> PG
    if ((BP > 0.05))
        return 0;
    else
        return 0.23;
}

double TCV1::BVs(double BCP)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    CVP = CntrlRes->P();
    VVP = ValveRes->P();

    // przeplyw ZP <-> rozdzielacz
    if ((BVP < CVP - 0.1))
        return 1;
    else if ((BCP > 0.05))
        return 0;
    else
        return 0.2 * (1.5 - int(BVP > VVP));
}

double TCV1::GetPF(double PP, double dt, double Vel)
{
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = Min0R(ValveRes->P(), BVP + 0.05);
    BCP = BrakeCyl->P();
    CVP = CntrlRes->P();

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(BCP, dV1);

    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    temp = CVs(BCP);
    dv = PF(CVP, VVP, 0.0015 * temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, 0.0042 * (1.37 - int(BrakeDelayFlag == bdelay_G)) * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);

    // przeplyw ZP <-> silowniki
    if ((BrakeStatus & b_on) == b_on)
        dv = PF(BVP, BCP,
                0.017 * (1 + int((BCP < 0.58) && (BrakeDelayFlag == bdelay_G))) *
                    (1.13 - int((BCP > 0.6) && (BrakeDelayFlag == bdelay_G))) * SizeBC) *
             dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);

    // przeplyw ZP <-> rozdzielacz
    temp = BVs(BCP);
    if ((VVP + 0.05 > BVP))
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01) * dt;
    ValveRes->Flow(-dv);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return dv - dV1;
}

void TCV1::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TBrake::Init(PP, HPP, LPP, BP, BDF);
    ValveRes->CreatePress(PP);
    BrakeCyl->CreatePress(BP);
    BrakeRes->CreatePress(PP);
    CntrlRes->CreateCap(15);
    CntrlRes->CreatePress(HPP);
    BrakeStatus = 0;

    BVM = 1.0 / (HPP - LPP) * MaxBP;

    BrakeDelayFlag = BDF;
}

double TCV1::GetCRP()
{
    return CntrlRes->P();
}

//---CV1-L-TR---

void TCV1L_TR::SetLBP(double P)
{
    LBP = P;
}

double TCV1L_TR::GetHPFlow(double HP, double dt)
{
    double dv;

    dv = PF(HP, BrakeRes->P(), 0.01) * dt;
    dv = Min0R(0, dv);
    BrakeRes->Flow(-dv);
    return dv;
}

void TCV1L_TR::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TCV1::Init(PP, HPP, LPP, BP, BDF);
    ImplsRes->CreateCap(2.5);
    ImplsRes->CreatePress(BP);
}

double TCV1L_TR::GetPF(double PP, double dt, double Vel)
{
    double result;
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = Min0R(ValveRes->P(), BVP + 0.05);
    BCP = ImplsRes->P();
    CVP = CntrlRes->P();

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(BCP, dV1);

    VVP = ValveRes->P();
    // przeplyw ZS <-> PG
    temp = CVs(BCP);
    dv = PF(CVP, VVP, 0.0015 * temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie KI
    if ((BrakeStatus & b_hld) == b_off)
        dv = PF(0, BCP, 0.000425 * (1.37 - int(BrakeDelayFlag == bdelay_G))) * dt;
    else
        dv = 0;
    ImplsRes->Flow(-dv);
    // przeplyw ZP <-> KI
    if (((BrakeStatus & b_on) == b_on) && (BCP < MaxBP))
        dv = PF(BVP, BCP,
                0.002 * (1 + int((BCP < 0.58) && (BrakeDelayFlag == bdelay_G))) *
                    (1.13 - int((BCP > 0.6) && (BrakeDelayFlag == bdelay_G)))) *
             dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    ImplsRes->Flow(-dv);

    // przeplyw ZP <-> rozdzielacz
    temp = BVs(BCP);
    if ((VVP + 0.05 > BVP))
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01) * dt;
    result = dv - dV1;

    temp = Max0R(BCP, LBP);

    // luzowanie CH
    if ((BrakeCyl->P() > temp + 0.005) || (Max0R(ImplsRes->P(), 8 * LBP) < 0.25))
        dv = PF(0, BrakeCyl->P(), 0.015 * SizeBC) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);

    // przeplyw ZP <-> CH
    if ((BrakeCyl->P() < temp - 0.005) && (Max0R(ImplsRes->P(), 8 * LBP) > 0.3) &&
        (Max0R(BCP, LBP) < MaxBP))
        dv = PF(BVP, BrakeCyl->P(), 0.020 * SizeBC) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    BrakeCyl->Flow(-dv);

    ImplsRes->Act();
    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    return result;
}

//--- KNORR KE ---
void TKE::CheckReleaser(double dt)
{
    double VVP;
    double CVP;

    VVP = ValveRes->P();
    CVP = CntrlRes->P();

    // odluzniacz
    if ((BrakeStatus && b_rls) == b_rls)
        if ((CVP - VVP < 0))
            BrakeStatus &= 247;
        else
            CntrlRes->Flow(+PF(CVP, 0, 0.1) * dt);
}

void TKE::CheckState(double BCP, double &dV1)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    CVP = CntrlRes->P();

    // sprawdzanie stanu
    if ((BrakeStatus && 1) == 1)
        if ((VVP + 0.003 + BCP / BVM < CVP))
            BrakeStatus |= 2; // hamowanie stopniowe;
        else if ((VVP - 0.003 + BCP / BVM > CVP))
            BrakeStatus &= 252; // luzowanie;
        else if ((VVP + BCP / BVM > CVP))
            BrakeStatus &= 253; // zatrzymanie napelaniania;
        else
            ;
    else if ((VVP + 0.10 < CVP) && (BCP < 0.1)) // poczatek hamowania
    {
        BrakeStatus |= 3;
        ValveRes->CreatePress(0.8 * VVP); // przyspieszacz
    }
    else if ((VVP + BCP / BVM < CVP) && ((CVP - VVP) * BVM > 0.25)) // zatrzymanie luzowanie
        BrakeStatus |= 1;
}

double TKE::CVs(double BP)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    CVP = CntrlRes->P();
    VVP = ValveRes->P();

    // przeplyw ZS <-> PG
    if ((BP > 0.2))
        return 0;
    else if ((VVP > CVP + 0.4))
        return 0.05;
    else
        return 0.23;
}

double TKE::BVs(double BCP)
{
    double VVP;
    double BVP;
    double CVP;

    BVP = BrakeRes->P();
    CVP = CntrlRes->P();
    VVP = ValveRes->P();

    // przeplyw ZP <-> rozdzielacz
    if ((BVP > VVP))
        return 0;
    else if ((BVP < CVP - 0.3))
        return 0.6;
    else
        return 0.13;
}

double TKE::GetPF(double PP, double dt, double Vel)
{
    double dv;
    double dV1;
    double temp;
    double VVP;
    double BVP;
    double BCP;
    double IMP;
    double CVP;

    BVP = BrakeRes->P();
    VVP = ValveRes->P();
    BCP = BrakeCyl->P();
    IMP = ImplsRes->P();
    CVP = CntrlRes->P();

    dv = 0;
    dV1 = 0;

    // sprawdzanie stanu
    CheckState(IMP, dV1);
    CheckReleaser(dt);

    // przeplyw ZS <-> PG
    temp = CVs(IMP);
    dv = PF(CVP, VVP, 0.0015 * temp) * dt;
    CntrlRes->Flow(+dv);
    ValveRes->Flow(-0.04 * dv);
    dV1 = dV1 - 0.96 * dv;

    // luzowanie
    if ((BrakeStatus & b_hld) == b_off)
    {
        if (((BrakeDelayFlag & bdelay_G) == 0))
            temp = 0.283 + 0.139;
        else
            temp = 0.139;
        dv = PF(0, IMP, 0.001 * temp) * dt;
    }
    else
        dv = 0;
    ImplsRes->Flow(-dv);

    // przeplyw ZP <-> silowniki
    if (((BrakeStatus & b_on) == b_on) && (IMP < MaxBP))
    {
        temp = 0.113;
        if (((BrakeDelayFlag & bdelay_G) == 0))
            temp = temp + 0.636;
        if ((BCP < 0.5))
            temp = temp + 0.785;
        dv = PF(BVP, IMP, 0.001 * temp) * dt;
    }
    else
        dv = 0;
    BrakeRes->Flow(dv);
    ImplsRes->Flow(-dv);

    // rapid
    if (!((typeid(*FM) == typeid(TDisk1)) ||
          (typeid(*FM) == typeid(TDisk2)))) // jesli zeliwo to schodz
        RapidStatus = ((BrakeDelayFlag & bdelay_R) == bdelay_R) &&
                      (((Vel > 50) && (RapidStatus)) || (Vel > 70));
    else // jesli tarczowki, to zostan
        RapidStatus = ((BrakeDelayFlag & bdelay_R) == bdelay_R);

    //  temp:=1.9-0.9*int(RapidStatus);

    if ((RM * RM > 0.1)) // jesli jest rapid
        if ((RM > 0)) // jesli dodatni (naddatek);
            temp = 1 - RM * int(RapidStatus);
        else
            temp = 1 - RM * (1 - int(RapidStatus));
    else
        temp = 1;
    temp = temp / LoadC;
    // luzowanie CH
    //  temp:=Max0R(BCP,LBP);
    IMP = Max0R(IMP / temp, Max0R(LBP, ASBP * int((BrakeStatus & b_asb) == b_asb)));

    // luzowanie CH
    if ((BCP > IMP + 0.005) || (Max0R(ImplsRes->P(), 8 * LBP) < 0.25))
        dv = PFVd(BCP, 0, 0.05, IMP) * dt;
    else
        dv = 0;
    BrakeCyl->Flow(-dv);
    if ((BCP < IMP - 0.005) && (Max0R(ImplsRes->P(), 8 * LBP) > 0.3))
        dv = PFVa(BVP, BCP, 0.05, IMP) * dt;
    else
        dv = 0;
    BrakeRes->Flow(-dv);
    BrakeCyl->Flow(+dv);

    // przeplyw ZP <-> rozdzielacz
    temp = BVs(IMP);
    //  if(BrakeStatus and b_hld)=b_off then
    if ((IMP < 0.25) || (VVP + 0.05 > BVP))
        dv = PF(BVP, VVP, 0.02 * SizeBR * temp / 1.87) * dt;
    else
        dv = 0;
    BrakeRes->Flow(dv);
    dV1 = dV1 + dv * 0.96;
    ValveRes->Flow(-0.04 * dv);
    // przeplyw PG <-> rozdzielacz
    dv = PF(PP, VVP, 0.01) * dt;
    ValveRes->Flow(-dv);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    ImplsRes->Act();
    return dv - dV1;
}

void TKE::Init(double PP, double HPP, double LPP, double BP, int BDF)
{
    TBrake::Init(PP, HPP, LPP, BP, BDF);
    ValveRes->CreatePress(PP);
    BrakeCyl->CreatePress(BP);
    BrakeRes->CreatePress(PP);

    CntrlRes->CreateCap(5);
    CntrlRes->CreatePress(HPP);

    ImplsRes->CreateCap(1);
    ImplsRes->CreatePress(BP);

    BrakeStatus = 0;

    BVM = 1.0 / (HPP - LPP) * MaxBP;

    BrakeDelayFlag = BDF;
}

double TKE::GetCRP()
{
    return CntrlRes->P();
}

double TKE::GetHPFlow(double HP, double dt)
{
    double dv;

    dv = PF(HP, BrakeRes->P(), 0.01) * dt;
    dv = Min0R(0, dv);
    BrakeRes->Flow(-dv);
    return dv;
}

void TKE::PLC(double mass)
{
    LoadC = 1 +
            int(mass < LoadM) *
                ((TareBP + (MaxBP - TareBP) * (mass - TareM) / (LoadM - TareM)) / MaxBP - 1);
}

void TKE::SetLP(double TM, double LM, double TBP)
{
    TareM = TM;
    LoadM = LM;
    TareBP = TBP;
}

void TKE::SetRM(double RMR)
{
    RM = 1.0 - RMR;
}

void TKE::SetLBP(double P)
{
    LBP = P;
}

//---KRANY---

double TDriverHandle::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    return 0;
}

void TDriverHandle::Init(double Press)
{
    Time = false;
    TimeEP = false;
}

void TDriverHandle::SetReductor(double nAdj)
{
}

double TDriverHandle::GetCP()
{
    return 0;
}

double TDriverHandle::GetSound(int i)
{
    return 0;
}

double TDriverHandle::GetPos(int i)
{
    return 0;
}

double TDriverHandle::GetEP(double pos)
{
    return 0;
}
//---FV4a---

double TFV4a::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 100;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;

    ep = PP; // SPKS!!
    LimPP = Min0R(BPT[lround(i_bcp) + 2][1], HP);
    ActFlowSpeed = BPT[lround(i_bcp) + 2][0];

    if ((i_bcp == i_bcpno))
        LimPP = 2.9;

    CP = CP + 20 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt / 1;
    RP = RP + 20 * Min0R(abs(ep - RP), 0.05) * PR(RP, ep) * dt / 2.5;

    LimPP = CP;
    dpPipe = Min0R(HP, LimPP);

    dpMainValve = PF(dpPipe, PP, ActFlowSpeed / LBDelay) * dt;
    if ((CP > RP + 0.05))
        dpMainValve = PF(Min0R(CP + 0.1, HP), PP, 1.1 * ActFlowSpeed / LBDelay) * dt;
    if ((CP < RP - 0.05))
        dpMainValve = PF(CP - 0.1, PP, 1.1 * ActFlowSpeed / LBDelay) * dt;

    if (lround(i_bcp) == -1)
    {
        CP = CP + 5 * Min0R(abs(LimPP - CP), 0.2) * PR(CP, LimPP) * dt / 2;
        if ((CP < RP + 0.03))
            if ((TP < 5))
                TP = TP + dt;
        //            if(cp+0.03<5.4)then
        if ((RP + 0.03 < 5.4) || (CP + 0.03 < 5.4)) // fala
            dpMainValve = PF(Min0R(HP, 17.1), PP, ActFlowSpeed / LBDelay) * dt;
        //              dpMainValve:=20*Min0R(abs(ep-7.1),0.05)*PF(HP,pp,ActFlowSpeed/LBDelay)*dt;
        else
        {
            RP = 5.45;
            if ((CP < PP - 0.01)) //: /34*9
                dpMainValve = PF(dpPipe, PP, ActFlowSpeed / 34 * 9 / LBDelay) * dt;
            else
                dpMainValve = PF(dpPipe, PP, ActFlowSpeed / LBDelay) * dt;
        }
    }

    if ((lround(i_bcp) == 0))
    {
        if ((TP > 0.1))
        {
            CP = 5 + (TP - 0.1) * 0.08;
            TP = TP - dt / 12 / 2;
        }
        if ((CP > RP + 0.1) && (CP <= 5))
            dpMainValve = PF(Min0R(CP + 0.25, HP), PP, 2 * ActFlowSpeed / LBDelay) * dt;
        else if (CP > 5)
            dpMainValve = PF(Min0R(CP, HP), PP, 2 * ActFlowSpeed / LBDelay) * dt;
        else
            dpMainValve = PF(dpPipe, PP, ActFlowSpeed / LBDelay) * dt;
    }

    if ((lround(i_bcp) == i_bcpno))
    {
        dpMainValve = PF(0, PP, ActFlowSpeed / LBDelay) * dt;
    }

    return dpMainValve;
}

void TFV4a::Init(double Press)
{
    CP = Press;
    TP = 0;
    RP = Press;
    Time = false;
    TimeEP = false;
}

//---FV4a/M--- nowonapisany kran bez poprawki IC

double TFV4aM::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 100;
    static double const xpM = 0.3; // mnoznik membrany komory pod

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;
    double DP;
    double pom;
    int i;

    ep = PP / 2 * 1.5 + ep / 2 * 0.5; // SPKS!!
    //          ep:=pp;
    //          ep:=cp/3+pp/3+ep/3;
    //          ep:=cp;

    for (i = 0; i < 5; ++i)
        Sounds[i] = 0;
    DP = 0;

    i_bcp = Max0R(Min0R(i_bcp, 5.999), -1.999); // na wszelki wypadek, zeby nie wyszlo poza zakres

    if ((TP > 0))
    { // jesli czasowy jest niepusty
        //            dp:=0.07; //od cisnienia 5 do 0 w 60 sekund ((5-0)*dt/75)
        DP = 0.045; // 2.5 w 55 sekund (5,35->5,15 w PG)
        TP = TP - DP * dt;
        Sounds[s_fv4a_t] = DP;
    }
    else //.08
    {
        TP = 0;
    }

    if ((XP > 0)) // jesli komora pod niepusta jest niepusty
    {
        DP = 2.5;
        Sounds[s_fv4a_x] = DP * XP;
        XP = XP - dt * DP * 2; // od cisnienia 5 do 0 w 10 sekund ((5-0)*dt/10)
    }
    else //.75
        XP = 0; // jak pusty, to pusty

    LimPP = Min0R(LPP_RP(i_bcp) + TP * 0.08 + RedAdj, HP); // pozycja + czasowy lub zasilanie
    ActFlowSpeed = BPT[lround(i_bcp) + 2][0];

    if ((EQ(i_bcp, -1)))
        pom = Min0R(HP, 5.4 + RedAdj);
    else
        pom = Min0R(CP, HP);

    if ((pom > RP + 0.25))
        Fala = true;
    if ((Fala))
        if ((pom > RP + 0.3))
            //              if(ep>rp+0.11)then
            XP = XP - 20 * PR(pom, XP) * dt;
        //              else
        //                xp:=xp-16*(ep-(ep+0.01))/(0.1)*PR(ep,xp)*dt;
        else
            Fala = false;

    if ((LimPP > CP)) // podwyzszanie szybkie
        CP = CP + 5 * 60 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy;
    else
        CP = CP + 13 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy

    LimPP = pom; // cp
    dpPipe = Min0R(HP, LimPP + XP * xpM);

    if (dpPipe > PP)
        dpMainValve = -PFVa(HP, PP, ActFlowSpeed / LBDelay, dpPipe, 0.4);
    else
        dpMainValve = PFVd(PP, 0, ActFlowSpeed / LBDelay, dpPipe, 0.4);

    if (EQ(i_bcp, -1))
    {
        if ((TP < 5))
            TP = TP + dt; // 5/10
        if ((TP < 1))
            TP = TP - 0.5 * dt; // 5/10
        // dpMainValve:=dpMainValve*2;
        //+1*PF(dpPipe,pp,ActFlowSpeed/LBDelay)//coby
        // nie przeszkadzal przy ladowaniu z zaworu obok
    }

    if (EQ(i_bcp, 0))
    {
        if ((TP > 2))
            dpMainValve = dpMainValve * 1.5; //+0.5*PF(dpPipe,pp,ActFlowSpeed/LBDelay)//coby nie
        // przeszkadzal przy ladowaniu z zaworu obok
    }

    ep = dpPipe;
    if ((EQ(i_bcp, 0) || (RP > ep)))
        RP = RP + PF(RP, ep, 0.0007) * dt; // powolne wzrastanie, ale szybsze na jezdzie;
    else
        RP = RP + PF(RP, ep, 0.000093 / 2 * 2) * dt; // powolne wzrastanie i to bardzo
    // jednak trzeba wydluzyc, bo
    // obecnie zle dziala
    if ((RP < ep) &&
        (RP <
         BPT[lround(i_bcpno) + 2][1])) // jesli jestesmy ponizej cisnienia w sterujacym (2.9 bar)
        RP = RP + PF(RP, CP, 0.005) * dt; // przypisz cisnienie w PG - wydluzanie napelniania o czas
    // potrzebny do napelnienia PG

    if ((EQ(i_bcp, i_bcpno)) || (EQ(i_bcp, -2)))
    {
        DP = PF(0, PP, ActFlowSpeed / LBDelay);
        dpMainValve = DP;
        Sounds[s_fv4a_e] = DP;
        Sounds[s_fv4a_u] = 0;
        Sounds[s_fv4a_b] = 0;
        Sounds[s_fv4a_x] = 0;
    }
    else
    {
        if (dpMainValve > 0)
            Sounds[s_fv4a_b] = dpMainValve;
        else
            Sounds[s_fv4a_u] = -dpMainValve;
    }

    return dpMainValve * dt;
}

void TFV4aM::Init(double Press)
{
    CP = Press;
    TP = 0.0;
    RP = Press;
    XP = 0.0;
    Time = false;
    TimeEP = false;
    RedAdj = 0.0;
    Fala = false;
}

void TFV4aM::SetReductor(double nAdj)
{
    RedAdj = nAdj;
}

double TFV4aM::GetSound(int i)
{
    if (i > 4)
        return 0;
    else
        return Sounds[i];
}

double TFV4aM::GetPos(int i)
{
    return pos_table[i];
}

double TFV4aM::LPP_RP(double pos) // cisnienie z zaokraglonej pozycji;
{
    int i_pos;

    i_pos = lround(pos - 0.5) + 2; // zaokraglone w dol
    double i, j, k, l;
    i = BPT[i_pos][1];
    j = BPT[i_pos + 1][1];
    k = pos + 2 - i_pos;
    l = i + (j - i) * k;
    double r = BPT[i_pos][1] +
               (BPT[i_pos + 1][1] - BPT[i_pos][1]) * (pos + 2 - i_pos); // interpolacja liniowa
    return r;
}
bool TFV4aM::EQ(double pos, double i_pos)
{
    return (pos <= i_pos + 0.5) && (pos > i_pos - 0.5);
}

//---FV4a/M--- nowonapisany kran bez poprawki IC

double TMHZ_EN57::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 100;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;
    double DP;
    double pom;
    int i;

    {
        long i_end = 5;
        for (i = 0; i < i_end; ++i)
            Sounds[i] = 0;
    }
    DP = 0;

    i_bcp = Max0R(Min0R(i_bcp, 9.999), -0.999); // na wszelki wypadek, zeby nie wyszlo poza zakres

    if ((TP > 0))
    {
        DP = 0.045;
        if (EQ(i_bcp, 0))
            TP = TP - DP * dt;
        Sounds[s_fv4a_t] = DP;
    }
    else
    {
        TP = 0;
    }

    LimPP = Min0R(LPP_RP(i_bcp) + TP * 0.08 + RedAdj, HP); // pozycja + czasowy lub zasilanie
    ActFlowSpeed = 4;

    if ((EQ(i_bcp, -1)))
        pom = Min0R(HP, 5.4 + RedAdj);
    else
        pom = Min0R(CP, HP);

    if ((LimPP > CP)) // podwyzszanie szybkie
        CP = CP + 60 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy;
    else
        CP = CP + 13 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy

    LimPP = pom; // cp
    if (EQ(i_bcp, -1))
        dpPipe = HP;
    else
        dpPipe = Min0R(HP, LimPP);

    if (dpPipe > PP)
        dpMainValve = -PFVa(HP, PP, ActFlowSpeed / LBDelay, dpPipe, 0.4);
    else
        dpMainValve = PFVd(PP, 0, ActFlowSpeed / LBDelay, dpPipe, 0.4);

    if (EQ(i_bcp, -1))
    {
        if ((TP < 5))
            TP = TP + dt; // 5/10
        if ((TP < 1))
            TP = TP - 0.5 * dt; // 5/10
    }

    if ((EQ(i_bcp, 10)) || (EQ(i_bcp, -2)))
    {
        DP = PF(0, PP, 2 * ActFlowSpeed / LBDelay);
        dpMainValve = DP;
        Sounds[s_fv4a_e] = DP;
        Sounds[s_fv4a_u] = 0;
        Sounds[s_fv4a_b] = 0;
        Sounds[s_fv4a_x] = 0;
    }
    else
    {
        if (dpMainValve > 0)
            Sounds[s_fv4a_b] = dpMainValve;
        else
            Sounds[s_fv4a_u] = -dpMainValve;
    }

    if ((i_bcp < 1.5))
        RP = Max0R(0, 0.125 * i_bcp);
    else
        RP = Min0R(1, 0.125 * i_bcp - 0.125);

    return dpMainValve * dt;
}

void TMHZ_EN57::Init(double Press)
{
    CP = Press;
    TP = 0.0;
    RP = 0.0;
    Time = false;
    TimeEP = false;
    RedAdj = 0.0;
    Fala = false;
}

void TMHZ_EN57::SetReductor(double nAdj)
{
    RedAdj = nAdj;
}

double TMHZ_EN57::GetSound(int i)
{
    if (i > 4)
        return 0;
    else
        return Sounds[i];
}

double TMHZ_EN57::GetPos(int i)
{
    return pos_table[i];
}

double TMHZ_EN57::GetCP()
{
    return RP;
}

double TMHZ_EN57::GetEP(double pos)
{
    if (pos < 9.5)
        return Min0R(Max0R(0, 0.125 * pos), 1);
    else
        return 0;
}

double TMHZ_EN57::LPP_RP(double pos) // cisnienie z zaokraglonej pozycji;
{
    if (pos > 8.5)
        return 5.0 - 0.15 * pos - 0.35;
    else if (pos > 0.5)
        return 5.0 - 0.15 * pos - 0.1;
    else
        return 5.0;
}

bool TMHZ_EN57::EQ(double pos, double i_pos)
{
    return (pos <= i_pos + 0.5) && (pos > i_pos - 0.5);
}

//---M394--- Matrosow

double TM394::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 65;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;
    int BCP;

    BCP = lround(i_bcp);
    if (BCP < -1)
        BCP = 1;

    LimPP = Min0R(BPT_394[BCP + 1][1], HP);
    ActFlowSpeed = BPT_394[BCP + 1][0];
    if ((BCP == 1) || (BCP == i_bcpno))
        LimPP = PP;
    if ((BCP == 0))
        LimPP = LimPP + RedAdj;
    if ((BCP != 2))
        if (CP < LimPP)
            CP = CP + 4 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy
        //      cp:=cp+6*(2+int(bcp<0))*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt //zbiornik
        //      sterujacy;
        else if (BCP == 0)
            CP = CP - 0.2 * dt / 100;
        else
            CP = CP +
                 4 * (1 + int(BCP != 3) + int(BCP > 4)) * Min0R(abs(LimPP - CP), 0.05) *
                     PR(CP, LimPP) * dt; // zbiornik sterujacy

    LimPP = CP;
    dpPipe = Min0R(HP, LimPP);

    //  if(dpPipe>pp)then //napelnianie
    //    dpMainValve:=PF(dpPipe,pp,ActFlowSpeed/LBDelay)*dt
    //  else //spuszczanie
    dpMainValve = PF(dpPipe, PP, ActFlowSpeed / LBDelay) * dt;

    if (BCP == -1)
        dpMainValve = PF(HP, PP, ActFlowSpeed / LBDelay) * dt;

    if (BCP == i_bcpno)
        dpMainValve = PF(0, PP, ActFlowSpeed / LBDelay) * dt;

    return dpMainValve;
}

void TM394::Init(double Press)
{
    CP = Press;
    RedAdj = 0;
    Time = true;
    TimeEP = false;
}

void TM394::SetReductor(double nAdj)
{
    RedAdj = nAdj;
}

double TM394::GetCP()
{
    return CP;
}

double TM394::GetPos(int i)
{
    return pos_table[i];
}

//---H14K1-- Knorr

double TH14K1::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 100; // szybkosc + zasilanie sterujacego
    //   static double const BPT_K[/*?*/ /*-1..4*/ (4) - (-1) + 1][2] =
    //{ (10, 0), (4, 1), (0, 1), (4, 0), (4, -1), (15, -1) };
    static double const NomPress = 5.0;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;
    int BCP;

    BCP = lround(i_bcp);
    if (i_bcp < -1)
        BCP = 1;
    LimPP = BPT_K[BCP + 1][1];
    if (LimPP < 0)
        LimPP = 0.5 * PP;
    else if (LimPP > 0)
        LimPP = PP;
    else
        LimPP = CP;
    ActFlowSpeed = BPT_K[BCP + 1][0];

    CP = CP + 6 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy

    dpMainValve = 0;

    if (BCP == -1)
        dpMainValve = PF(HP, PP, ActFlowSpeed / LBDelay) * dt;
    if ((BCP == 0))
        dpMainValve = -PFVa(HP, PP, ActFlowSpeed / LBDelay, NomPress + RedAdj) * dt;
    if ((BCP > 1) && (PP > CP))
        dpMainValve = PFVd(PP, 0, ActFlowSpeed / LBDelay, CP) * dt;
    if (BCP == i_bcpno)
        dpMainValve = PF(0, PP, ActFlowSpeed / LBDelay) * dt;

    return dpMainValve;
}

void TH14K1::Init(double Press)
{
    CP = Press;
    RedAdj = 0;
    Time = true;
    TimeEP = true;
}

void TH14K1::SetReductor(double nAdj)
{
    RedAdj = nAdj;
}

double TH14K1::GetCP()
{
    return CP;
}

double TH14K1::GetPos(int i)
{
    return pos_table[i];
}

//---St113-- Knorr EP

double TSt113::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 100; // szybkosc + zasilanie sterujacego
    static double const NomPress = 5.0;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;
    int BCP;

    BCP = lround(i_bcp);

    EPS = BEP_K[BCP];

    if (BCP > 0)
        BCP = BCP - 1;

    if (BCP < -1)
        BCP = 1;
    LimPP = BPT_K[BCP + 1][1];
    if (LimPP < 0)
        LimPP = 0.5 * PP;
    else if (LimPP > 0)
        LimPP = PP;
    else
        LimPP = CP;
    ActFlowSpeed = BPT_K[BCP + 1][0];

    CP = CP + 6 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt; // zbiornik sterujacy

    dpMainValve = 0;

    if (BCP == -1)
        dpMainValve = PF(HP, PP, ActFlowSpeed / LBDelay) * dt;
    if ((BCP == 0))
        dpMainValve = -PFVa(HP, PP, ActFlowSpeed / LBDelay, NomPress + RedAdj) * dt;
    if ((BCP > 1) && (PP > CP))
        dpMainValve = PFVd(PP, 0, ActFlowSpeed / LBDelay, CP) * dt;
    if (BCP == i_bcpno)
        dpMainValve = PF(0, PP, ActFlowSpeed / LBDelay) * dt;

    return dpMainValve;
}

double TSt113::GetCP()
{
    return EPS;
}

double TSt113::GetPos(int i)
{
    return pos_table[i];
}

void TSt113::Init(double Press)
{
    Time = true;
    TimeEP = true;
    EPS = 0.0;
}

//--- test ---

double Ttest::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    double result;
    static int const LBDelay = 100;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;

    LimPP = BPT[lround(i_bcp) + 2][1];
    ActFlowSpeed = BPT[lround(i_bcp) + 2][0];

    if ((i_bcp == i_bcpno))
        LimPP = 0.0;

    if ((i_bcp == -1))
        LimPP = 7;

    CP = CP + 20 * Min0R(abs(LimPP - CP), 0.05) * PR(CP, LimPP) * dt / 1;

    LimPP = CP;
    dpPipe = Min0R(HP, LimPP);

    dpMainValve = PF(dpPipe, PP, ActFlowSpeed / LBDelay) * dt;

    if ((lround(i_bcp) == i_bcpno))
    {
        dpMainValve = PF(0, PP, ActFlowSpeed / LBDelay) * dt;
    }

    return dpMainValve;
}

void Ttest::Init(double Press)
{
    CP = Press;
}

//---FD1---

double TFD1::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    double DP;
    double temp;

    //  MaxBP:=4;
    //  temp:=Min0R(i_bcp*MaxBP,Min0R(5.0,HP));
    temp = Min0R(i_bcp * MaxBP, HP); // 0011
    DP = 10 * Min0R(abs(temp - BP), 0.1) * PF(temp, BP, 0.0006 * (2 + int(temp > BP))) * dt * Speed;
    BP = BP - DP;
    return -DP;
}

void TFD1::Init(double Press)
{
    BP = 0;
    MaxBP = Press;
    Time = false;
    TimeEP = false;
    Speed = 1;
}

double TFD1::GetCP()
{
    return BP;
}

void TFD1::SetSpeed(double nSpeed)
{
    Speed = nSpeed;
}

//---KNORR---

double TH1405::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    double DP;
    double temp;
    double A;

    PP = Min0R(PP, MaxBP);
    if (i_bcp > 0.5)
    {
        temp = Min0R(MaxBP, HP);
        A = 2 * (i_bcp - 0.5) * 0.0011;
        BP = Max0R(BP, PP);
    }
    else
    {
        temp = 0;
        A = 0.2 * (0.5 - i_bcp) * 0.0033;
        BP = Min0R(BP, PP);
    }
    DP = PF(temp, BP, A) * dt;
    BP = BP - DP;
    return -DP;
}

void TH1405::Init(double Press)
{
    BP = 0;
    MaxBP = Press;
    Time = true;
    TimeEP = false;
}

double TH1405::GetCP()
{
    return BP;
}

//---FVel6---

double TFVel6::GetPF(double i_bcp, double PP, double HP, double dt, double ep)
{
    static int const LBDelay = 100;

    double LimPP;
    double dpPipe;
    double dpMainValve;
    double ActFlowSpeed;

    LimPP = Min0R(5 * int(i_bcp < 3.5), HP);
    if ((i_bcp >= 3.5) && ((i_bcp < 4.3) || (i_bcp > 5.5)))
        ActFlowSpeed = 0;
    else if ((i_bcp > 4.3) && (i_bcp < 4.8))
        ActFlowSpeed = 4 * (i_bcp - 4.3); // konsultacje wawa1 - bylo 8;
    else if ((i_bcp < 4))
        ActFlowSpeed = 2;
    else
        ActFlowSpeed = 4;
    dpMainValve = PF(LimPP, PP, ActFlowSpeed / LBDelay) * dt;

    Sounds[s_fv4a_e] = 0;
    Sounds[s_fv4a_u] = 0;
    Sounds[s_fv4a_b] = 0;
    if ((i_bcp < 3.5))
        Sounds[s_fv4a_u] = -dpMainValve;
    else if ((i_bcp < 4.8))
        Sounds[s_fv4a_b] = dpMainValve;
    else if ((i_bcp < 5.5))
        Sounds[s_fv4a_e] = dpMainValve;

    if ((i_bcp < -0.5))
        EPS = -1;
    else if ((i_bcp > 0.5) && (i_bcp < 4.7))
        EPS = 1;
    else
        EPS = 0;
    //    EPS:=i_bcp*int(i_bcp<2)
    return dpMainValve;
}

double TFVel6::GetCP()
{
    return EPS;
}

double TFVel6::GetPos(int i)
{
    return pos_table[i];
}

double TFVel6::GetSound(int i)
{
    if (i > 2)
        return 0;
    else
        return Sounds[i];
}

void TFVel6::Init(double Press)
{
    Time = true;
    TimeEP = true;
    EPS = 0.0;
}

// END
