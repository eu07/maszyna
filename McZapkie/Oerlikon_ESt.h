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

#include <memory>
#include "hamulce.h" // Pascal unit
#include "friction.h" // Pascal unit
#include "mctools.h" // Pascal unit

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
    std::shared_ptr<TReservoir> BrakeRes;
    std::shared_ptr<TReservoir> Next;

	TPrzekladnik() : TReservoir() {};
    virtual void Update(double dt);
	virtual void SetPoslizg(bool flag) {};
	virtual void SetP(double P) {};
	virtual void SetMult(double m) {};
	virtual void SetLBP(double P) {};
	virtual void SetRapidParams(double mult, double size) {};
	virtual void SetRapidStatus(bool rs) {};
};

class TRura : public TPrzekladnik // nieprzekladnik, rura laczaca

{
  private:
  public:
	  TRura() : TPrzekladnik() {};
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
	inline TPrzeciwposlizg() : TRura()
	{
		Poslizg = false;
	}
};

// przekladnik dwustopniowy
class TRapid : public TPrzekladnik { 

  private:
    bool RapidStatus = false; // status rapidu
    double RapidMult = 0.0; // przelozenie (w dol)
    //        Komora2: real;
    double DN = 0.0;
    double DL = 0.0; // srednice dysz napelniania i luzowania

  public:
    void SetRapidParams(double mult, double size);
    void SetRapidStatus(bool rs);
    void Update(double dt) /*override*/;
	inline TRapid() :
		TPrzekladnik()
	{}
};

// AL2
class TPrzekCiagly : public TPrzekladnik {

  private:
    double mult = 0.0;

  public:
    void SetMult(double m);
    void Update(double dt) /*override*/;
	inline TPrzekCiagly() :
		TPrzekladnik()
	{}
};

// podwojny zawor zwrotny
class TPrzek_PZZ : public TPrzekladnik {

  private:
    double LBP = 0.0;

  public:
    void SetLBP(double P);
    void Update(double dt) /*override*/;
	inline TPrzek_PZZ() :
		TPrzekladnik()
	{}
};

class TPrzekZalamany : public TPrzekladnik // Knicksventil

{
  private:
  public:
	  TPrzekZalamany() :
		  TPrzekladnik()
	  {}
};

// przy napelnianiu - rura, przy hamowaniu - upust
class TPrzekED : public TRura  {

  private:
    double MaxP = 0.0;

  public:
    void SetP(double P);
    void Update(double dt) /*override*/;
	inline TPrzekED() :
		TRura()
	{}
};

class TNESt3 : public TBrake {

  private:
	std::shared_ptr<TReservoir> CntrlRes; // zbiornik sterujacy
	std::shared_ptr<TReservoir> Miedzypoj; // pojemnosc posrednia (urojona) do napelniania ZP i ZS
	std::shared_ptr<TPrzekladnik> Przekladniki[ 4 ];
	double Nozzles[ dMAX ]; // dysze
    double BVM = 0.0; // przelozenie PG-CH
    //        ValveFlag: byte;           //polozenie roznych zaworkow
    bool Zamykajacy = true; // pamiec zaworka zamykajacego
    //        Przys_wlot: boolean;       //wlot do komory przyspieszacza
    bool Przys_blok = true; // blokada przyspieszacza
    bool RapidStatus = true;
    bool RapidStaly = true;
    double LoadC = 0.0;
	double TareM = 0.0; // masa proznego
    double LoadM = 0.0; // masa pelnego
    double TareBP = 0.0; // cisnienie dla proznego
    double HBG300 = 0.0; // zawor ograniczajacy cisnienie
    double Podskok = 0.0; // podskok preznosci poczatkowej
    //        HPBR: real;               //zasilanie ZP z wysokiego cisnienia
    bool autom = true; // odluzniacz samoczynny
    double LBP = 0.0; // cisnienie hamulca pomocniczego

  public:
	inline TNESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
           TBrake(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
	{}
    void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF ) /*override*/;
    virtual double GetPF( double const PP, double const dt, double const Vel ) /*override*/; // przeplyw miedzy komora wstepna i PG
    void EStParams(double i_crc); // parametry charakterystyczne dla ESt
    virtual double GetCRP() /*override*/;
    void CheckState(double const BCP, double &dV1); // glowny przyrzad rozrzadczy
    void CheckReleaser(double const dt); // odluzniacz
    double CVs(double const BP); // napelniacz sterujacego
    double BVs(double const BCP); // napelniacz pomocniczego
    void SetSize( int const size, std::string const &params ); // ustawianie dysz (rozmiaru ZR), przekladniki
    void PLC(double const mass); // wspolczynnik cisnienia przystawki wazacej
    void SetLP(double const TM, double const LM, double const TBP); // parametry przystawki wazacej
    virtual void ForceEmptiness() /*override*/; // wymuszenie bycia pustym
    void SetLBP(double const P); // cisnienie z hamulca pomocniczego
};

extern double d2A( double const d );

#endif // INCLUDED_OERLIKON_EST_H
// END
