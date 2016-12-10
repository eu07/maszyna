#pragma once
#ifndef INCLUDED_OERLIKON_EST_H
#define INCLUDED_OERLIKON_EST_H
/*fizyka hamulcow Oerlikon ESt dla symulatora*/

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

#include "hamulce.h" // Pascal unit
#include "friction.h" // Pascal unit
#include "mctools.h" // Pascal unit
#include <string>

/*
(C) youBy
Co jest:
- glowny przyrzad rozrzadczy
- napelniacz zbiornika pomocniczego
- napelniacz zbiornika sterujacego
- zawor podskoku
- nibyprzyspieszacz
- tylko 16",14",12",10"
- nieprzekladnik rura
- przekladnik 1:1
- przekladniki AL2
- przeciwposlizgi
- rapid REL2
- HGB300
- inne srednice
Co brakuje:
- dobry przyspieszacz
- mozliwosc zasilania z wysokiego cisnienia ESt4
- ep: EP1 i EP2
- samoczynne ep
- PZZ dla dodatkowego
*/

static int const dMAX = 11; // dysze
static int const dON = 0; // osobowy napelnianie (+ZP)
static int const dOO = 1; // osobowy oproznianie
static int const dTN = 2; // towarowy napelnianie (+ZP)
static int const dTO = 3; // towarowy oproznianie
static int const dP = 4; // zbiornik pomocniczy
static int const dSd = 5; // zbiornik sterujacy
static int const dSm = 6; // zbiornik sterujacy
static int const dPd = 7; // duzy przelot zamykajcego
static int const dPm = 8; // maly przelot zamykajacego
static int const dPO = 9; // zasilanie pomocniczego O
static int const dPT = 10; // zasilanie pomocniczego T

// przekladniki
static int const p_none = 0;
static int const p_rapid = 1;
static int const p_pp = 2;
static int const p_al2 = 3;
static int const p_ppz = 4;
static int const P_ed = 5;

class TPrzekladnik : public TReservoir // przekladnik (powtarzacz)

{
  private:
  public:
    TReservoir *BrakeRes;
    TReservoir *Next;
    virtual void Update(double dt);
	void SetPoslizg(bool flag) {};
	void SetP(double P) {};
	void SetMult(double m) {};
	void SetLBP(double P) {};
	void SetRapidParams(double mult, double size) {};
	void SetRapidStatus(bool rs) {};
};

class TRura : public TPrzekladnik // nieprzekladnik, rura laczaca

{
  private:
  public:
    virtual double P(void) /*override*/;
    virtual void Update(double dt) /*override*/;
};

class TPrzeciwposlizg : public TRura // przy napelnianiu - rura, przy poslizgu - upust

{
  private:
    bool Poslizg;

  public:
    void SetPoslizg(bool flag);
    void Update(double dt) /*override*/;
	inline TPrzeciwposlizg()
	{
		Poslizg = false;
	}
};

class TRapid : public TPrzekladnik // przekladnik dwustopniowy

{
  private:
    bool RapidStatus; // status rapidu
    double RapidMult; // przelozenie (w dol)
    //        Komora2: real;
    double DN;
    double DL; // srednice dysz napelniania i luzowania

  public:
    void SetRapidParams(double mult, double size);
    void SetRapidStatus(bool rs);
    void Update(double dt) /*override*/;
	inline TRapid()
	{
		RapidStatus = false;
		RapidMult, DN, DL = 0.0;
	}
};

class TPrzekCiagly : public TPrzekladnik // AL2

{
  private:
    double mult;

  public:
    void SetMult(double m);
    void Update(double dt) /*override*/;
	inline TPrzekCiagly()
	{
		mult = 0.0;
	}
};

class TPrzek_PZZ : public TPrzekladnik // podwojny zawor zwrotny

{
  private:
    double LBP;

  public:
    void SetLBP(double P);
    void Update(double dt) /*override*/;
	inline TPrzek_PZZ()
	{
		LBP = 0.0;
	}
};

class TPrzekZalamany : public TPrzekladnik // Knicksventil

{
  private:
  public:
};

class TPrzekED : public TRura // przy napelnianiu - rura, przy hamowaniu - upust

{
  private:
    double MaxP;

  public:
    void SetP(double P);
    void Update(double dt) /*override*/;
	inline TPrzekED()
	{
		MaxP = 0.0;
	}
};

class TNESt3 : public TBrake

{
  private:
    double Nozzles[dMAX]; // dysze
    TReservoir *CntrlRes; // zbiornik steruj¹cy
    double BVM; // przelozenie PG-CH
    //        ValveFlag: byte;           //polozenie roznych zaworkow
    bool Zamykajacy; // pamiec zaworka zamykajacego
    //        Przys_wlot: boolean;       //wlot do komory przyspieszacza
    bool Przys_blok; // blokada przyspieszacza
    TReservoir *Miedzypoj; // pojemnosc posrednia (urojona) do napelniania ZP i ZS
    TPrzekladnik *Przekladniki[4];
    bool RapidStatus;
    bool RapidStaly;
    double LoadC;
    double TareM;
    double LoadM; // masa proznego i pelnego
    double TareBP; // cisnienie dla proznego
    double HBG300; // zawor ograniczajacy cisnienie
    double Podskok; // podskok preznosci poczatkowej
    //        HPBR: real;               //zasilanie ZP z wysokiego cisnienia
    bool autom; // odluzniacz samoczynny
    double LBP; // cisnienie hamulca pomocniczego

    void Init(double PP, double HPP, double LPP, double BP, int BDF) /*override*/;

  public:
	inline TNESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc,
		int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa,
		double PP, double HPP, double LPP, double BP, int BDF) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
			, i_BD, i_mat, i_ba, i_nbpa, PP, HPP, LPP, BP, BDF)
	{
		Init(PP, HPP, LPP, BP, BDF);
	}
    virtual double GetPF(double PP, double dt,
                 double Vel) /*override*/; // przeplyw miedzy komora wstepna i PG
    void EStParams(double i_crc); // parametry charakterystyczne dla ESt
    virtual double GetCRP() /*override*/;
    void CheckState(double BCP, double &dV1); // glowny przyrzad rozrzadczy
    void CheckReleaser(double dt); // odluzniacz
    double CVs(double BP); // napelniacz sterujacego
    double BVs(double BCP); // napelniacz pomocniczego
    void SetSize(int size, std::string params); // ustawianie dysz (rozmiaru ZR), przekladniki
    void PLC(double mass); // wspolczynnik cisnienia przystawki wazacej
    void SetLP(double TM, double LM, double TBP); // parametry przystawki wazacej
    virtual void ForceEmptiness() /*override*/; // wymuszenie bycia pustym
    void SetLBP(double P); // cisnienie z hamulca pomocniczego
};

extern double d2A(double d);

#endif // INCLUDED_OERLIKON_EST_H
// END
