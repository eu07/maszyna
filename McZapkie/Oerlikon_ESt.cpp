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

#include "stdafx.h"
#include "Oerlikon_ESt.h"
#include "utilities.h"

double d2A( double const d )
{
    return (d * d) * 0.7854 / 1000.0;
}

// ------ RURA ------

double TRura::P(void)
{
    return Next->P();
}

void TRura::Update(double dt)
{
    Next->Flow(dVol);
    dVol = 0;
}

// ------ PRZECIWPOSLIG ------

void TPrzeciwposlizg::SetPoslizg(bool flag)
{
    Poslizg = flag;
}

void TPrzeciwposlizg::Update(double dt)
{
    if (true == Poslizg)
    {
        BrakeRes->Flow(dVol);
        Next->Flow(PF(Next->P(), 0, d2A(10)) * dt);
    }
    else
        Next->Flow(dVol);
    dVol = 0;
}

// ------ PRZEKLADNIK ------

void TPrzekladnik::Update(double dt)
{
    double dV;

    double BCP{ Next->P() };
    double BVP{ BrakeRes->P() };

    if ( BCP > P() )
        dV = -PFVd(BCP, 0, d2A(10), P()) * dt;
    else {
        if( BCP < P() )
            dV = PFVa( BVP, BCP, d2A( 10 ), P() ) * dt;
        else
            dV = 0;
    }

    Next->Flow(dV);
    if (dV > 0)
        BrakeRes->Flow(-dV);
}

// ------ PRZEKLADNIK RAPID ------

void TRapid::SetRapidParams(double mult, double size)
{
    RapidMult = mult;
    RapidStatus = false;
    if (size > 0.1) // dopasowywanie srednicy przekladnika
    {
        DN = d2A(size * 0.4);
        DL = d2A(size * 0.4);
    }
    else
    {
        DN = d2A(5.0);
        DL = d2A(5.0);
    }
}

void TRapid::SetRapidStatus(bool rs)
{
    RapidStatus = rs;
}

void TRapid::Update(double dt)
{
    double BVP{ BrakeRes->P() };
    double BCP{ Next->P() };
    double ActMult;
    double dV;

    if ( true == RapidStatus )
    {
        ActMult = RapidMult;
    }
    else
    {
        ActMult = 1.0;
    }

    if ((BCP * RapidMult) > (P() * ActMult))
        dV = -PFVd(BCP, 0, DL, P() * ActMult / RapidMult) * dt;
    else if ((BCP * RapidMult) < (P() * ActMult))
        dV = PFVa(BVP, BCP, DN, P() * ActMult / RapidMult) * dt;
    else
        dV = 0.0;

    Next->Flow(dV);
    if (dV > 0.0)
        BrakeRes->Flow(-dV);
}

// ------ PRZEKŁADNIK CIĄGŁY ------

void TPrzekCiagly::SetMult(double m)
{
    Mult = m;
}

void TPrzekCiagly::Update(double dt)
{
    double const BVP{ BrakeRes->P() };
    double const BCP{ Next->P() };
    double dV;

    if ( BCP > (P() * Mult))
        dV = -PFVd(BCP, 0, d2A(8.0), P() * Mult) * dt;
    else if (BCP < (P() * Mult))
        dV = PFVa(BVP, BCP, d2A(8.0), P() * Mult) * dt;
    else
        dV = 0.0;

    Next->Flow(dV);
    if (dV > 0.0)
        BrakeRes->Flow(-dV);
}

// ------ PRZEKŁADNIK CIĄGŁY ------

void TPrzek_PZZ::SetLBP(double P)
{
    LBP = P;
}

void TPrzek_PZZ::Update(double dt)
{

    double const BVP{ BrakeRes->P() };
    double const BCP{ Next->P() };
    double const Pgr{ std::max( LBP, P() ) };
    double dV;

    if (BCP > Pgr)
        dV = -PFVd(BCP, 0, d2A(8.0), Pgr) * dt;
    else if (BCP < Pgr)
        dV = PFVa(BVP, BCP, d2A(8.0), Pgr) * dt;
    else
        dV = 0.0;

    Next->Flow(dV);
    if (dV > 0.0)
        BrakeRes->Flow(-dV);
}

// ------ PRZECIWPOSLIG ------

void TPrzekED::SetP(double P)
{
    MaxP = P;
}

void TPrzekED::Update(double dt)
{
    if (Next->P() > MaxP)
    {
        BrakeRes->Flow(dVol);
        Next->Flow(PFVd(Next->P(), 0, d2A(10.0) * dt, MaxP));
    }
    else
        Next->Flow(dVol);
    dVol = 0.0;
}

// ------ OERLIKON EST NA BOGATO ------

double TNESt3::GetPF( double const PP, double const dt, double const Vel ) // przeplyw miedzy komora wstepna i PG
{
    double const BVP{ BrakeRes->P() };
    double const VVP{ ValveRes->P() };
    //  BCP:=BrakeCyl.P;
    double const BCP{ Przekladniki[ 1 ]->P() };
    double const CVP{ CntrlRes->P() };
    double const MPP{ Miedzypoj->P() };
    double dV{ 0.0 };
    double dV1{ 0.0 };

    double const nastG = static_cast<double>(BrakeDelayFlag & bdelay_G);

    // sprawdzanie stanu
    CheckState(BCP, dV1);
    CheckReleaser(dt);

    // luzowanie
    if ((BrakeStatus & b_hld) == b_off)
        dV =
            PF(0.0, BCP, Nozzles[dTO] * nastG + (1.0 - nastG) * Nozzles[dOO])
            * dt * (0.1 + 4.9 * std::min(0.2, BCP - ((CVP - 0.05 - VVP) * BVM + 0.1)));
    else
        dV = 0.0;
    //  BrakeCyl.Flow(-dV);
    Przekladniki[1]->Flow(-dV);
    if ( ((BrakeStatus & b_on) == b_on) && (Przekladniki[1]->P() * HBG300 < MaxBP) )
        dV =
            PF( BVP, BCP, Nozzles[dTN] * (nastG + 2.0 * (BCP < Podskok ? 1.0 : 0.0)) + Nozzles[dON] * (1.0 - nastG) )
            * dt * (0.1 + 4.9 * std::min( 0.2, (CVP - 0.05 - VVP) * BVM - BCP ) );
    else
        dV = 0;
    //  BrakeCyl.Flow(-dV);
    Przekladniki[1]->Flow(-dV);
    BrakeRes->Flow(dV);

    for (int i = 1; i < 4; ++i)
    {
        Przekladniki[i]->Update(dt);
        if (typeid(*Przekladniki[i]) == typeid(TRapid))
        {
            RapidStatus = ( ( ( BrakeDelayFlag & bdelay_R ) == bdelay_R )
                         && ( (   std::abs( Vel ) > 70.0 )
                           || ( ( std::abs( Vel ) > 50.0 ) && ( RapidStatus ) )
                           || ( RapidStaly ) ) );
            Przekladniki[i]->SetRapidStatus(RapidStatus);
        }
        else if( typeid( *Przekladniki[i] ) == typeid( TPrzeciwposlizg ) )
            Przekladniki[i]->SetPoslizg((BrakeStatus & b_asb) == b_asb);
        else if( typeid( *Przekladniki[ i ] ) == typeid( TPrzekED ) ) {
            if( Vel < -15.0 )
                Przekladniki[ i ]->SetP( 0.0 );
            else
                Przekladniki[ i ]->SetP( MaxBP * 3.0 );
        }
        else if( typeid( *Przekladniki[i] ) == typeid( TPrzekCiagly ) )
            Przekladniki[i]->SetMult(LoadC);
        else if( typeid( *Przekladniki[i] ) == typeid( TPrzek_PZZ ) )
            Przekladniki[i]->SetLBP(LBP);
    }

    // przeplyw testowy miedzypojemnosci
    dV = PF(MPP, VVP, BVs(BCP)) + PF(MPP, CVP, CVs(BCP));
    if ((MPP - 0.05) > BVP)
        dV += PF(MPP - 0.05, BVP, Nozzles[dPT] * nastG + (1.0 - nastG) * Nozzles[dPO]);
    if (MPP > VVP)
        dV += PF(MPP, VVP, d2A(5.0));
    Miedzypoj->Flow(dV * dt * 0.15);

    // przeplyw ZS <-> PG
    double const temp = CVs(BCP);
    dV = PF(CVP, MPP, temp) * dt;
    CntrlRes->Flow(dV);
    ValveRes->Flow(-0.02 * dV);
    dV1 += 0.98 * dV;

    // przeplyw ZP <-> MPJ
    if ((MPP - 0.05) > BVP)
        dV = PF(BVP, MPP - 0.05, Nozzles[dPT] * nastG + (1.0 - nastG) * Nozzles[dPO]) * dt;
    else
        dV = 0.0;
    BrakeRes->Flow(dV);
    dV1 += dV * 0.98;
    ValveRes->Flow(-0.02 * dV);
    // przeplyw PG <-> rozdzielacz
    dV = PF(PP, VVP, 0.005) * dt; // 0.01
    ValveRes->Flow(-dV);

    ValveRes->Act();
    BrakeCyl->Act();
    BrakeRes->Act();
    CntrlRes->Act();
    Miedzypoj->Act();
    Przekladniki[1]->Act();
    Przekladniki[2]->Act();
    Przekladniki[3]->Act();
    return dV - dV1;
}

void TNESt3::EStParams( double const i_crc ) // parametry charakterystyczne dla ESt
{
}

void TNESt3::Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )
{
    TBrake::Init(PP, HPP, LPP, BP, BDF);
    ValveRes->CreatePress(PP);
    BrakeCyl->CreatePress(BP);
    BrakeRes->CreatePress(PP);
    CntrlRes = std::make_shared<TReservoir>();
    CntrlRes->CreateCap(15.0);
    CntrlRes->CreatePress(HPP);
    BrakeStatus = (BP > 1.0 ? 1 : 0);
    Miedzypoj = std::make_shared<TReservoir>();
    Miedzypoj->CreateCap(5.0);
    Miedzypoj->CreatePress(PP);

    BVM = 1.0 / (HPP - 0.05 - LPP) * MaxBP;

    BrakeDelayFlag = BDF;

    Zamykajacy = false;

    if ( (typeid(*FM) == typeid(TDisk1)) || (typeid(*FM) == typeid(TDisk2)) ) // jesli zeliwo to schodz
        RapidStaly = true;
    else
        RapidStaly = false;
}

double TNESt3::GetCRP()
{
    return CntrlRes->P();
    //  return Przekladniki[1].P;
    //  return Miedzypoj.P;
}

void TNESt3::CheckState(double const BCP, double &dV1) // glowny przyrzad rozrzadczy
{
    double const BVP{ BrakeRes->P() }; //-> tu ma byc komora rozprezna
    double const VVP{ ValveRes->P() };
    double const CVP{ CntrlRes->P() };
    double const MPP{ Miedzypoj->P() };

    if ((BCP < 0.25) && (VVP + 0.08 > CVP))
        Przys_blok = false;

    // sprawdzanie stanu
    // if ((BrakeStatus and 1)=1)and(BCP>0.25)then
    if (((VVP + 0.01 + BCP / BVM) < (CVP - 0.05)) && (Przys_blok))
        BrakeStatus |= ( b_on | b_hld ); // hamowanie stopniowe;
    else if ((VVP - 0.01 + (BCP - 0.1) / BVM) > (CVP - 0.05))
        BrakeStatus &= ~( b_on | b_hld ); // luzowanie;
    else if ((VVP + BCP / BVM) > (CVP - 0.05))
        BrakeStatus &= ~b_on; // zatrzymanie napelaniania;
    else if (((VVP + (BCP - 0.1) / BVM) < (CVP - 0.05)) && (BCP > 0.25)) // zatrzymanie luzowania
        BrakeStatus |= b_hld;

    if( ( BrakeStatus & b_hld ) == 0 )
        SoundFlag |= sf_CylU;

    if (((VVP + 0.10) < CVP) && (BCP < 0.25)) // poczatek hamowania
        if (false == Przys_blok)
        {
            ValveRes->CreatePress(0.1 * VVP);
            SoundFlag |= sf_Acc;
            ValveRes->Act();
            Przys_blok = true;
        }

    if (BCP > 0.5)
        Zamykajacy = true;
    else if ((VVP - 0.6) < MPP)
        Zamykajacy = false;
}

void TNESt3::CheckReleaser(double const dt) // odluzniacz
{
    double const VVP{ ValveRes->P() };
    double const CVP{ CntrlRes->P() };

    // odluzniacz automatyczny
    if ((BrakeStatus & b_rls) == b_rls)
    {
        CntrlRes->Flow(PF(CVP, 0, 0.02) * dt);
        if ((CVP < (VVP + 0.3)) || (false == autom))
            BrakeStatus &= ~b_rls;
    }
}

double TNESt3::CVs(double const BP) // napelniacz sterujacego
{
    double const CVP{ CntrlRes->P() };
    double const MPP{ Miedzypoj->P() };

    // przeplyw ZS <-> PG
    if (MPP < (CVP - 0.17))
        return 0.0;
    else if (MPP > (CVP - 0.08))
        return Nozzles[dSd];
    else
        return Nozzles[dSm];
}

double TNESt3::BVs(double const BCP) // napelniacz pomocniczego
{
    double const CVP{ CntrlRes->P() };
    double const MPP{ Miedzypoj->P() };

    // przeplyw ZP <-> rozdzielacz
    if (MPP < (CVP - 0.3))
        return Nozzles[dP];
    else if( BCP < 0.5 ) {
        if( true == Zamykajacy )
            return Nozzles[ dPm ]; // 1.25
        else
            return Nozzles[ dPd ];
    }
    else
        return 0.0;
}

void TNESt3::PLC(double const mass)
{
    LoadC = 1.0 +
        ( mass < LoadM ?
            ( (TareBP + (MaxBP - TareBP) * (mass - TareM) / (LoadM - TareM) ) / MaxBP - 1.0 ) :
            0.0 );
}

void TNESt3::ForceEmptiness()
{
    ValveRes->CreatePress(0);
    BrakeRes->CreatePress(0);
    Miedzypoj->CreatePress(0);
    CntrlRes->CreatePress(0);

    ValveRes->Act();
    BrakeRes->Act();
    Miedzypoj->Act();
    CntrlRes->Act();
}

void TNESt3::SetLP(double const TM, double const LM, double const TBP)
{
    TareM = TM;
    LoadM = LM;
    TareBP = TBP;
}

void TNESt3::SetLBP(double const P)
{
    LBP = P;
}

void TNESt3::SetSize( int const size, std::string const &params ) // ustawianie dysz (rozmiaru ZR)
{
    static double const dNO1l = 1.250;
    static double const dNT1l = 0.510;
    static double const dOO1l = 0.907;
    static double const dOT1l = 0.524;

    if (contains( params, "ESt3" ) )
    {
        Podskok = 0.7;
        Przekladniki[1] = std::make_shared<TRura>();
        Przekladniki[3] = std::make_shared<TRura>();
    }
    else
    {
        Podskok = -1.0;
        Przekladniki[1] = std::make_shared<TRapid>();
        if (contains( params, "-s216" ) )
            Przekladniki[1]->SetRapidParams(2, 16);
        else
            Przekladniki[1]->SetRapidParams(2, 0);
        Przekladniki[3] = std::make_shared<TPrzeciwposlizg>();
        if (contains( params,"-ED") )
        {
            Przekladniki[1]->SetRapidParams(2, 18);
            Przekladniki[3] = std::make_shared<TPrzekED>();
        }
    }

    if (contains(params,"AL2") )
        Przekladniki[2] = std::make_shared<TPrzekCiagly>();
    else if (contains(params,"PZZ") )
        Przekladniki[2] = std::make_shared<TPrzek_PZZ>();
    else
        Przekladniki[2] = std::make_shared<TRura>();

	if( ( contains( params, "3d" ) )
	 || ( contains( params, "4d" ) ) ) {
        autom = false;
	}
    else
        autom = true;
    if ((contains( params,"HBG300")))
        HBG300 = 1.0;
    else
        HBG300 = 0.0;

    switch (size)
    {
    case 16:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = 5.0; // 5.0;
        Nozzles[dOO] = 3.4; // 3.4;
        Nozzles[dTN] = 2.0;
        Nozzles[dTO] = 1.75;
        Nozzles[dP] = 3.8;
        Nozzles[dPd] = 2.70;
        Nozzles[dPm] = 1.25;
        Nozzles[dPO] = Nozzles[dON];
        Nozzles[dPT] = Nozzles[dTN];
        break;
    }
    case 14:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = 4.3;
        Nozzles[dOO] = 2.85;
        Nozzles[dTN] = 1.83;
        Nozzles[dTO] = 1.57;
        Nozzles[dP] = 3.4;
        Nozzles[dPd] = 2.20;
        Nozzles[dPm] = 1.10;
        Nozzles[dPO] = Nozzles[dON];
        Nozzles[dPT] = Nozzles[dTN];
        break;
    }
    case 12:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = 3.7;
        Nozzles[dOO] = 2.50;
        Nozzles[dTN] = 1.65;
        Nozzles[dTO] = 1.39;
        Nozzles[dP] = 2.65;
        Nozzles[dPd] = 1.80;
        Nozzles[dPm] = 0.85;
        Nozzles[dPO] = Nozzles[dON];
        Nozzles[dPT] = Nozzles[dTN];
        break;
    }
    case 10:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = 3.1;
        Nozzles[dOO] = 2.0;
        Nozzles[dTN] = 1.35;
        Nozzles[dTO] = 1.13;
        Nozzles[dP] = 1.6;
        Nozzles[dPd] = 1.55;
        Nozzles[dPm] = 0.7;
        Nozzles[dPO] = Nozzles[dON];
        Nozzles[dPT] = Nozzles[dTN];
        break;
    }
    case 200:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = dNO1l;
        Nozzles[dOO] = dOO1l / 1.15;
        Nozzles[dTN] = dNT1l;
        Nozzles[dTO] = dOT1l;
        Nozzles[dP] = 7.4;
        Nozzles[dPd] = 5.3;
        Nozzles[dPm] = 2.5;
        Nozzles[dPO] = 7.28;
        Nozzles[dPT] = 2.96;
        break;
    }
    case 375:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = dNO1l;
        Nozzles[dOO] = dOO1l / 1.15;
        Nozzles[dTN] = dNT1l;
        Nozzles[dTO] = dOT1l;
        Nozzles[dP] = 13.0;
        Nozzles[dPd] = 9.6;
        Nozzles[dPm] = 4.4;
        Nozzles[dPO] = 9.92;
        Nozzles[dPT] = 3.99;
        break;
    }
    case 150:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = dNO1l;
        Nozzles[dOO] = dOO1l;
        Nozzles[dTN] = dNT1l;
        Nozzles[dTO] = dOT1l;
        Nozzles[dP] = 5.8;
        Nozzles[dPd] = 4.1;
        Nozzles[dPm] = 1.9;
        Nozzles[dPO] = 6.33;
        Nozzles[dPT] = 2.58;
        break;
    }
    case 100:
    { // dON,dOO,dTN,dTO,dP,dS
        Nozzles[dON] = dNO1l;
        Nozzles[dOO] = dOO1l;
        Nozzles[dTN] = dNT1l;
        Nozzles[dTO] = dOT1l;
        Nozzles[dP] = 4.2;
        Nozzles[dPd] = 2.9;
        Nozzles[dPm] = 1.4;
        Nozzles[dPO] = 5.19;
        Nozzles[dPT] = 2.14;
        break;
    }
    default:
    {
        Nozzles[dON] = 0;
        Nozzles[dOO] = 0;
        Nozzles[dTN] = 0;
        Nozzles[dTO] = 0;
        Nozzles[dP] = 0;
        Nozzles[dPd] = 0;
        Nozzles[dPm] = 0;
    }
    }

    Nozzles[dSd] = 1.1;
    Nozzles[dSm] = 0.9;

    // przeliczanie z mm^2 na l/m
    for (int i = 0; i < dMAX; ++i)
    {
        Nozzles[i] = d2A(Nozzles[i]); //(/1000^2*pi/4*1000)
    }
    for (int i = 1; i < 4; ++i)
    {
        Przekladniki[i]->BrakeRes = BrakeRes;
        Przekladniki[i]->CreateCap(i);
        Przekladniki[i]->CreatePress(BrakeCyl->P());
        if (i < 3)
            Przekladniki[i]->Next = Przekladniki[i + 1];
        else
            Przekladniki[i]->Next = BrakeCyl;
    }
}

// END
