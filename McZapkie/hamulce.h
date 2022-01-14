/*fizyka hamulcow dla symulatora*/

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


/*
(C) youBy
Co brakuje:
moze jeszcze jakis SW
*/
/*
Zrobione:
ESt3, ESt3AL2, ESt4R, LSt, FV4a, FD1, EP2, prosty westinghouse
duzo wersji żeliwa
KE
Tarcze od 152A
Magnetyki (implementacja w mover.pas)
Matrosow 394
H14K1 (zasadniczy), H1405 (pomocniczy), St113 (ep)
Knorr/West EP - żeby był
*/

#pragma once

#include "friction.h" // Pascal unit

static int const LocalBrakePosNo = 10;         /*ilosc nastaw hamulca recznego lub pomocniczego*/
static int const MainBrakeMaxPos = 10;          /*max. ilosc nastaw hamulca zasadniczego*/

/*nastawy hamulca*/
static int const bdelay_G = 1;    //G
static int const bdelay_P = 2;    //P
static int const bdelay_R = 4;    //R
static int const bdelay_M = 8;    //Mg


/*stan hamulca*/
static int const b_off = 0;   //luzowanie
static int const b_hld = 1;   //trzymanie
static int const b_on = 2;   //napelnianie
static int const b_rfl = 4;   //uzupelnianie
static int const b_rls = 8;   //odluzniacz
static int const b_ep = 16;   //elektropneumatyczny
static int const b_asb = 32;   //przeciwposlizg-wstrzymanie
static int const b_asb_unbrake = 64; //przeciwposlizg-luzowanie
static int const b_dmg = 128;   //wylaczony z dzialania

/*uszkodzenia hamulca*/
static int const df_on = 1;  //napelnianie
static int const df_off = 2;  //luzowanie
static int const df_br = 4;  //wyplyw z ZP
static int const df_vv = 8;  //wyplyw z komory wstepnej
static int const df_bc = 16;  //wyplyw z silownika
static int const df_cv = 32;  //wyplyw z ZS
static int const df_PP = 64;  //zawsze niski stopien
static int const df_RR = 128;  //zawsze wysoki stopien

/*indeksy dzwiekow FV4a*/
static int const s_fv4a_b = 0; //hamowanie
static int const s_fv4a_u = 1; //luzowanie
static int const s_fv4a_e = 2; //hamowanie nagle
static int const s_fv4a_x = 3; //wyplyw sterujacego fala
static int const s_fv4a_t = 4; //wyplyw z czasowego

/*pary cierne*/
static int const bp_P10 = 0;
static int const bp_P10Bg = 2; //żeliwo fosforowe P10
static int const bp_P10Bgu = 1;
static int const bp_LLBg = 4; //komp. b.n.t.
static int const bp_LLBgu = 3;
static int const bp_LBg = 6; //komp. n.t.
static int const bp_LBgu = 5;
static int const bp_KBg = 8; //komp. w.t.
static int const bp_KBgu = 7;
static int const bp_D1 = 9; //tarcze
static int const bp_D2 = 10;
static int const bp_FR513 = 11; // Frenoplast FR513
static int const bp_Cosid = 12; // jakistam kompozyt :D
static int const bp_PKPBg = 13; //żeliwo PKP
static int const bp_PKPBgu = 14;
static int const bp_MHS = 128; // magnetyczny hamulec szynowy
static int const bp_P10yBg = 15; //żeliwo fosforowe P10
static int const bp_P10yBgu = 16;
static int const bp_FR510 = 17; //Frenoplast FR510

static int const sf_Acc = 1;  //przyspieszacz
static int const sf_BR = 2;  //przekladnia
static int const sf_CylB = 4;  //cylinder - napelnianie
static int const sf_CylU = 8;  //cylinder - oproznianie
static int const sf_rel = 16; //odluzniacz
static int const sf_ep = 32; //zawory ep

static int const bh_MIN = 0; // minimalna pozycja
static int const bh_MAX = 1; // maksymalna pozycja
static int const bh_FS = 2; // napelnianie uderzeniowe //jesli nie ma, to jazda
static int const bh_RP = 3; // jazda
static int const bh_NP = 4; // odciecie - podwojna trakcja
static int const bh_MB = 5; // odciecie - utrzymanie stopnia hamowania/pierwszy 1 stopien hamowania
static int const bh_FB = 6; // pelne
static int const bh_EB = 7; // nagle
static int const bh_EPR = 8; // ep - luzowanie  //pelny luz dla ep kątowego
static int const bh_EPN = 9; // ep - utrzymanie //jesli rowne luzowaniu, wtedy sterowanie przyciskiem
static int const bh_EPB = 10; // ep - hamowanie  //pelne hamowanie dla ep kątowego


static double const SpgD = 0.7917;
static double const SpO = 0.5067;  //przekroj przewodu 1" w l/m
				//wyj: jednostka dosyc dziwna, ale wszystkie obliczenia
				//i pojemnosci sa podane w litrach (rozsadne wielkosci)
				//zas dlugosc pojazdow jest podana w metrach
				//a predkosc przeplywu w m/s                           //3.5
																	//7//1.5
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (14, 5.4), (9, 5.0), (6, 4.6), (9, 4.5), (9, 4.0), (9, 3.5), (9, 2.8), (34, 2.8));
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (7, 5.0), (2.0, 5.0), (4.5, 4.6), (4.5, 4.2), (4.5, 3.8), (4.5, 3.4), (4.5, 2.8), (8, 2.8));
static double const BPT[9][2] = { {0 , 5.0} , {7 , 5.0} , {2.0 , 5.0} , {4.5 , 4.6} , {4.5 , 4.2} , {4.5 , 3.8} , {4.5 , 3.4} , {4.5 , 2.8} , {8 , 2.8} };
static double const BPT_394[7][2] = { {13 , 10.0} , {5 , 5.0} , {0 , -1} , {5 , -1} , {5 , 0.0} , {5 , 0.0} , {18 , 0.0} };
//double *BPT = zero_based_BPT[2]; //tablica pozycji hamulca dla zakresu -2..6
//double *BPT_394 = zero_based_BPT_394[1]; //tablica pozycji hamulca dla zakresu -1..5
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (12, 5.4), (9, 5.0), (9, 4.6), (9, 4.2), (9, 3.8), (9, 3.4), (9, 2.8), (34, 2.8));
//      BPT: array[-2..6] of array [0..1] of real= ((0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0));
// static double const pi = 3.141592653589793; //definicja w mctools

enum TUniversalBrake // możliwe działania uniwersalnego przycisku hamulca
{ // kolejne flagi
	ub_Release = 0x01, // odluźniacz - ZR
	ub_UnlockPipe = 0x02, // odblok PG / mostkowanie hamulca bezpieczeństwa - POJAZD
	ub_HighPressure = 0x04, // impuls wysokiego ciśnienia - ZM
	ub_Overload = 0x08, // przycisk asymilacji / kontrolowanego przeładowania - ZM
	ub_AntiSlipBrake = 0x10, // przycisk przyhamowania przeciwposlizgowego - ZR
	ub_Ostatni = 0x80000000 // ostatnia flaga bitowa
};

//klasa obejmujaca pojedyncze zbiorniki
class TReservoir {

protected:
    double Cap{ 1.0 };
    double Vol{ 0.0 };
    double dVol{ 0.0 };

  public:
    void CreateCap(double Capacity);
    void CreatePress(double Press);
    virtual double pa();
    virtual double P();
    void Flow(double dv);
    void Act();

		TReservoir() = default;
};

typedef TReservoir *PReservoir;


class TBrakeCyl : public TReservoir {

  public:
		virtual double pa()/*override*/;
		virtual double P()/*override*/;
		TBrakeCyl() : TReservoir() {};
};

//klasa obejmujaca uklad hamulca zespolonego pojazdu
class TBrake {

  protected:
		std::shared_ptr<TReservoir> BrakeCyl;      //silownik
		std::shared_ptr<TReservoir> BrakeRes;      //ZP
		std::shared_ptr<TReservoir> ValveRes;      //komora wstepna
		int BCN = 0; //ilosc silownikow
		double BCM = 0.0; //przekladnia hamulcowa
		double BCA = 0.0; //laczny przekroj silownikow
		int BrakeDelays = 0; //dostepne opoznienia
		int BrakeDelayFlag = 0; //aktualna nastawa
		std::shared_ptr<TFricMat> FM; //material cierny
		double MaxBP = 0.0; //najwyzsze cisnienie
		int BA = 0; //osie hamowane
		int NBpA = 0; //klocki na os
		double SizeBR = 0.0; //rozmiar^2 ZP (w stosunku do 14")
		double SizeBC = 0.0; //rozmiar^2 CH (w stosunku do 14")
		bool DCV = false; //podwojny zawor zwrotny
		double ASBP = 0.0; //cisnienie hamulca pp
        double RV = 0.0; // rapid activation vehicle velocity threshold

		int UniversalFlag = 0; //flaga wcisnietych przyciskow uniwersalnych
        int BrakeStatus{ b_off }; //flaga stanu
		int SoundFlag = 0;

  public:
		TBrake(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa);
		//maksymalne cisnienie, promien, skok roboczy, pojemnosc ZP, ilosc cylindrow, opoznienia hamulca, material klockow, osie hamowane, klocki na os;
        virtual void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF );  //inicjalizacja hamulca

        double GetFC( double const Vel, double const N );         //wspolczynnik tarcia - hamulec wie lepiej
        virtual double GetPF( double const PP, double const dt, double const Vel );      //przeplyw miedzy komora wstepna i PG
		double GetBCF();                           //sila tlokowa z tloka
        virtual double GetHPFlow( double const HP, double const dt );  //przeplyw - 8 bar
		double GetBCP();  //cisnienie cylindrow hamulcowych
		virtual double GetEDBCP();    //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
		double GetBRP(); //cisnienie zbiornika pomocniczego
		double GetVRP(); //cisnienie komory wstepnej rozdzielacza
		virtual double GetCRP();  //cisnienie zbiornika sterujacego
        bool SetBDF( int const nBDF ); //nastawiacz GPRM
        void Releaser( int const state ); //odluzniacz
        bool Releaser() const;
        virtual void SetEPS( double const nEPS ); //hamulec EP
        virtual void SetRM( double const RMR ) {};   //ustalenie przelozenia rapida
        virtual void SetRV( double const RVR ) { RV = RVR; };   //ustalenie przelozenia rapida
		virtual void SetLP(double const TM, double const LM, double const TBP) {};  //parametry przystawki wazacej
		virtual void SetLBP(double const P) {};   //cisnienie z hamulca pomocniczego
		virtual void PLC(double const mass) {};  //wspolczynnik cisnienia przystawki wazacej
		void ASB(int state); //hamulec przeciwposlizgowy
		int GetStatus(); //flaga statusu, moze sie przydac do odglosow
        void SetASBP( double const Press ); //ustalenie cisnienia pp
    virtual void ForceEmptiness();
    // removes specified amount of air from the reservoirs
    virtual void ForceLeak( double const Amount );
    int GetSoundFlag();
    int GetBrakeStatus() const { return BrakeStatus; }
    void SetBrakeStatus( int const Status ) { BrakeStatus = Status; }
    virtual void SetED( double const EDstate ) {}; //stan hamulca ED do luzowania
	virtual void SetUniversalFlag(int flag) { UniversalFlag = flag; } //przycisk uniwersalny
};

class TWest : public TBrake {

  private:
		double LBP = 0.0;     //cisnienie hamulca pomocniczego
		double dVP = 0.0;     //pobor powietrza wysokiego cisnienia
		double EPS = 0.0;     //stan elektropneumatyka
		double TareM = 0.0;   //masa proznego
		double LoadM = 0.0;   //i pelnego
		double TareBP = 0.0;  //cisnienie dla proznego
		double LoadC = 0.0;   //wspolczynnik przystawki wazacej

  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
		void SetLBP(double const P);   //cisnienie z hamulca pomocniczego
        double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
        double GetHPFlow( double const HP, double const dt )/*override*/;
		void PLC(double const mass);  //wspolczynnik cisnienia przystawki wazacej
        void SetEPS( double const nEPS )/*override*/;  //stan hamulca EP
		void SetLP(double const TM, double const LM, double const TBP);  //parametry przystawki wazacej

		inline TWest(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
              TBrake(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
		{}
};

class TESt : public TBrake {

  private:

  protected:
		std::shared_ptr<TReservoir> CntrlRes;      // zbiornik sterujący
		double BVM = 0.0;                 // przelozenie PG-CH

  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
        double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
		void EStParams(double i_crc);                 //parametry charakterystyczne dla ESt
		double GetCRP()/*override*/;
		void CheckState(double BCP, double & dV1); //glowny przyrzad rozrzadczy
		void CheckReleaser(double dt); //odluzniacz
		double CVs(double BP);      //napelniacz sterujacego
		double BVs(double BCP);     //napelniacz pomocniczego
        void ForceEmptiness() /*override*/; // wymuszenie bycia pustym

		inline TESt(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
             TBrake(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
			CntrlRes = std::make_shared<TReservoir>();
    }
};

class TESt3 : public TESt {

  private:
		//double CylFlowSpeed[2][2]; //zmienna nie uzywana

  public:
      double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG

		inline TESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
                TESt(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
		{}
};

class TESt3AL2 : public TESt3 {

  private:
		std::shared_ptr<TReservoir> ImplsRes;      //komora impulsowa
		double TareM = 0.0; //masa proznego
		double LoadM = 0.0; //i pelnego
		double TareBP = 0.0;  //cisnienie dla proznego
		double LoadC = 0.0;

  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
      double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
		void PLC(double const mass);  //wspolczynnik cisnienia przystawki wazacej
		void SetLP(double const TM, double const LM, double const TBP);  //parametry przystawki wazacej

		inline TESt3AL2(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
                  TESt3(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
			ImplsRes = std::make_shared<TReservoir>();
    }
};

class TESt4R : public TESt {

  private:
		bool RapidStatus = false;

  protected:
		std::shared_ptr<TReservoir> ImplsRes;      //komora impulsowa
		double RapidTemp = 0.0;           //aktualne, zmienne przelozenie

  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
      double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG

		inline TESt4R(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
                 TESt(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
			ImplsRes = std::make_shared<TReservoir>();
    }
};

class TLSt : public TESt4R {

  private:
    // double CylFlowSpeed[2][2]; // zmienna nie używana

  protected:
		double LBP = 0.0;       //cisnienie hamulca pomocniczego
		double RM = 0.0;        //przelozenie rapida
		double EDFlag = 0.0; //luzowanie hamulca z powodu zalaczonego ED

  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
		void SetLBP(double const P);   //cisnienie z hamulca pomocniczego
        void SetRM( double const RMR );   //ustalenie przelozenia rapida
        double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
        double GetHPFlow( double const HP, double const dt )/*override*/;  //przeplyw - 8 bar
		virtual double GetEDBCP();    //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
        virtual void SetED( double const EDstate ); //stan hamulca ED do luzowania

		inline TLSt(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
             TESt4R(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
		{}
};

class TEStED : public TLSt {  //zawor z EP09 - Est4 z oddzielnym przekladnikiem, kontrola rapidu i takie tam

  private:
		std::shared_ptr<TReservoir> Miedzypoj;     //pojemnosc posrednia (urojona) do napelniania ZP i ZS
		double Nozzles[ 11 ]; //dysze
		bool Zamykajacy = false;       //pamiec zaworka zamykajacego
		bool Przys_blok = false;       //blokada przyspieszacza
		double TareM = 0.0;  //masa proznego
		double LoadM = 0.0;  //i pelnego
		double TareBP = 0.0;  //cisnienie dla proznego
		double LoadC = 0.0;


  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
      double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
		double GetEDBCP()/*override*/;    //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED
		void PLC(double const mass);  //wspolczynnik cisnienia przystawki wazacej
        void SetLP( double const TM, double const LM, double const TBP );  //parametry przystawki wazacej        

		inline TEStED(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
                 TLSt(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
			Miedzypoj = std::make_shared<TReservoir>();
    }
};

class TEStEP2 : public TLSt {

protected:
    double TareM = 0.0;  //masa proznego
    double LoadM = 0.0;  //masa pelnego
    double TareBP = 0.0;  //cisnienie dla proznego
    double LoadC = 0.0;
    double EPS = 0.0;

public:
    void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;   //inicjalizacja
    double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
    void PLC( double const mass );  //wspolczynnik cisnienia przystawki wazacej
    void SetEPS( double const nEPS )/*override*/;  //stan hamulca EP
    void SetLP( double const TM, double const LM, double const TBP );  //parametry przystawki wazacej
	void virtual EPCalc(double dt);

		inline TEStEP2(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
               TLSt(          i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
		{}
};

class TEStEP1 : public TEStEP2 {

public:
	void EPCalc(double dt);
    void SetEPS( double const nEPS ) override;  //stan hamulca EP

	inline TEStEP1(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
		TEStEP2(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa)
	{}
};

class TCV1 : public TBrake {

private:
    double BVM = 0.0; //przelozenie PG-CH

protected:
    std::shared_ptr<TReservoir> CntrlRes; // zbiornik sterujący

public:
    void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
    double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
    double GetCRP()/*override*/;
    void CheckState( double const BCP, double &dV1 );
    double CVs( double const BP );
    double BVs( double const BCP );
    void ForceEmptiness() /*override*/; // wymuszenie bycia pustym

		inline TCV1(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
             TBrake(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
			CntrlRes = std::make_shared<TReservoir>();
    }
};

	//class TCV1R : public TCV1

//{
	//private:
//	TReservoir *ImplsRes;      //komora impulsowa
//	bool RapidStatus;

	//public:
	//	//        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
//	//        procedure Init(PP, HPP, LPP, BP: real; BDF: int); override;

//	inline TCV1R(double i_mbp, double i_bcr, double i_bcd, double i_brc,
//		int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa,
	//		double PP, double HPP, double LPP, double BP, int BDF) : TCV1(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
//			, i_BD, i_mat, i_ba, i_nbpa, PP, HPP, LPP, BP, BDF) { }
//};

class TCV1L_TR : public TCV1 {

private:
    std::shared_ptr<TReservoir> ImplsRes;      //komora impulsowa
    double LBP = 0.0;     //cisnienie hamulca pomocniczego

public:
    void Init( double const PP, double const  HPP, double const LPP, double const BP, int const BDF )/*override*/;
    double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
    void SetLBP( double const P );   //cisnienie z hamulca pomocniczego
    double GetHPFlow( double const HP, double const dt )/*override*/;  //przeplyw - 8 bar

    inline TCV1L_TR(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
               TCV1(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
        ImplsRes = std::make_shared<TReservoir>();
    }
};

class TKE : public TBrake { //Knorr Einheitsbauart — jeden do wszystkiego

  private:
		std::shared_ptr<TReservoir> ImplsRes;      //komora impulsowa
		std::shared_ptr<TReservoir> CntrlRes;      // zbiornik sterujący
		std::shared_ptr<TReservoir> Brak2Res;      //zbiornik pomocniczy 2        
		bool RapidStatus = false;
		double BVM = 0.0; //przelozenie PG-CH
		double TareM = 0.0; //masa proznego
		double LoadM = 0.0; //masa pelnego
		double TareBP = 0.0; //cisnienie dla proznego
		double LoadC = 0.0; //wspolczynnik zaladowania
		double RM = 0.0; //przelozenie rapida
		double LBP = 0.0; //cisnienie hamulca pomocniczego

  public:
      void Init( double const PP, double const HPP, double const LPP, double const BP, int const BDF )/*override*/;
      void SetRM( double const RMR );   //ustalenie przelozenia rapida
      double GetPF( double const PP, double const dt, double const Vel )/*override*/;      //przeplyw miedzy komora wstepna i PG
        double GetHPFlow( double const HP, double const dt )/*override*/;  //przeplyw - 8 bar
		double GetCRP()/*override*/;
        void CheckState( double const BCP, double &dV1 );
        void CheckReleaser( double const dt ); //odluzniacz
        double CVs( double const BP );      //napelniacz sterujacego
        double BVs( double const BCP );     //napelniacz pomocniczego
        void PLC( double const mass );  //wspolczynnik cisnienia przystawki wazacej
        void SetLP( double const TM, double const LM, double const TBP );  //parametry przystawki wazacej
        void SetLBP( double const P );   //cisnienie z hamulca pomocniczego
        void ForceEmptiness() /*override*/; // wymuszenie bycia pustym

		inline TKE(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) :
            TBrake(       i_mbp,        i_bcr,        i_bcd,        i_brc,     i_bcn,     i_BD,     i_mat,     i_ba,     i_nbpa)
    {
			ImplsRes = std::make_shared<TReservoir>();
			CntrlRes = std::make_shared<TReservoir>();
			Brak2Res = std::make_shared<TReservoir>();
    }
};

//klasa obejmujaca krany
class TDriverHandle {

  protected:
    //        BCP: integer;
	  bool AutoOvrld = false; //czy jest asymilacja automatyczna na pozycji -1
	  bool ManualOvrld = false; //czy jest asymilacja reczna przyciskiem
	  bool ManualOvrldActive = false; //czy jest wcisniety przycisk asymilacji
	  int UniversalFlag = 0; //flaga wcisnietych przyciskow uniwersalnych
      int i_bcpno = 6;
  public:
		bool Time = false;
		bool TimeEP = false;
        double Sounds[ 5 ]; //wielkosci przeplywow dla dzwiekow              

    virtual double GetPF(double i_bcp, double PP, double HP, double dt, double ep);
    virtual void Init(double Press);
    virtual double GetCP();
    virtual double GetEP();
    virtual double GetRP();
    virtual void SetReductor(double nAdj); //korekcja pozycji reduktora cisnienia
    virtual double GetSound(int i); //pobranie glosnosci wybranego dzwieku
    virtual double GetPos(int i); //pobranie numeru pozycji o zadanym kodzie (funkcji)
    virtual double GetEP(double pos); //pobranie sily hamulca ep
	virtual void SetParams(bool AO, bool MO, double, double, double OMP, double OPD) {}; //ustawianie jakichs parametrow dla zaworu
	virtual void OvrldButton(bool Active);  //przycisk recznego przeladowania/asymilacji
	virtual void SetUniversalFlag(int flag); //przycisk uniwersalny
    inline TDriverHandle() { memset( Sounds, 0, sizeof( Sounds ) ); }
};

class TFV4a : public TDriverHandle {

  private:
		double CP = 0.0; //zbiornik sterujący
		double TP = 0.0; //zbiornik czasowy
		double RP = 0.0; //zbiornik redukcyjny

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;

		inline TFV4a() :
			TDriverHandle()
		{}
};

class TFV4aM : public TDriverHandle {

  private:
		double CP = 0.0; //zbiornik sterujący
		double TP = 0.0; //zbiornik czasowy
		double RP = 0.0; //zbiornik redukcyjny
		double XP = 0.0; //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
		double RedAdj = 0.0; //dostosowanie reduktora cisnienia (krecenie kapturkiem)
    //        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
		bool Fala = false;
		static double const pos_table[11]; // = { -2, 6, -1, 0, -2, 1, 4, 6, 0, 0, 0 };

    double LPP_RP(double pos);
    bool EQ(double pos, double i_pos);

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetSound(int i)/*override*/;
		double GetPos(int i)/*override*/;
		double GetCP();
		double GetRP();
		inline TFV4aM() :
			TDriverHandle()
        {}
};

class TMHZ_EN57 : public TDriverHandle {

  private:
		double CP = 0.0; //zbiornik sterujący
		double TP = 0.0; //zbiornik czasowy
		double RP = 0.0; //zbiornik redukcyjny
		double RedAdj = 0.0; //dostosowanie reduktora cisnienia (krecenie kapturkiem)
		bool Fala = false;
		double UnbrakeOverPressure = 0.0;
		double OverloadMaxPressure = 1.0; //maksymalne zwiekszenie cisnienia przy asymilacji
		double OverloadPressureDecrease = 0.045; //predkosc spadku cisnienia przy asymilacji
		static double const pos_table[11]; //= { -2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0 };

    double LPP_RP(double pos);
    bool EQ(double pos, double i_pos);

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetSound(int i)/*override*/;
		double GetPos(int i)/*override*/;
		double GetCP()/*override*/;
		double GetRP()/*override*/;
		double GetEP(double pos);
		void SetParams(bool AO, bool MO, double OverP, double, double OMP, double OPD);
		inline TMHZ_EN57(void) :
			TDriverHandle()
		{}
};

class TMHZ_K5P : public TDriverHandle {

private:
	double CP = 0.0; //zbiornik sterujący
	double TP = 0.0; //zbiornik czasowy
	double RP = 0.0; //zbiornik redukcyjny
	double RedAdj = 0.0; //dostosowanie reduktora cisnienia (krecenie kapturkiem)
	bool Fala = false; //czy jest napelnianie uderzeniowe
	double UnbrakeOverPressure = 0.0;
	double OverloadMaxPressure = 1.0; //maksymalne zwiekszenie cisnienia przy asymilacji
	double OverloadPressureDecrease = 0.002; //predkosc spadku cisnienia przy asymilacji
	double FillingStrokeFactor = 1.0; //mnożnik otwarcia zaworu przy uderzeniowym (bez fali)
	static double const pos_table[11]; //= { -2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0 };

	bool EQ(double pos, double i_pos);

public:
	double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
	void Init(double Press)/*override*/;
	void SetReductor(double nAdj)/*override*/;
	double GetSound(int i)/*override*/;
	double GetPos(int i)/*override*/;
	double GetCP()/*override*/;
	double GetRP()/*override*/;
	void SetParams(bool AO, bool MO, double, double, double OMP, double OPD); /*ovveride*/

	inline TMHZ_K5P(void) :
		TDriverHandle()
	{}
};

class TMHZ_6P : public TDriverHandle {

private:
	double CP = 0.0; //zbiornik sterujący
	double TP = 0.0; //zbiornik czasowy
	double RP = 0.0; //zbiornik redukcyjny
	double RedAdj = 0.0; //dostosowanie reduktora cisnienia (krecenie kapturkiem)
	bool Fala = false; //czy jest napelnianie uderzeniowe
	double UnbrakeOverPressure = 0.0; //wartosc napelniania uderzeniowego
	double OverloadMaxPressure = 1.0; //maksymalne zwiekszenie cisnienia przy asymilacji
	double OverloadPressureDecrease = 0.002; //predkosc spadku cisnienia przy asymilacji
	double FillingStrokeFactor = 1.0; //mnożnik otwarcia zaworu przy uderzeniowym (bez fali)
	static double const pos_table[11]; //= { -2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0 };

	bool EQ(double pos, double i_pos);

public:
	double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
	void Init(double Press)/*override*/;
	void SetReductor(double nAdj)/*override*/;
	double GetSound(int i)/*override*/;
	double GetPos(int i)/*override*/;
	double GetCP()/*override*/;
	double GetRP()/*override*/;
	void SetParams(bool AO, bool MO, double, double, double OMP, double OPD); /*ovveride*/

	inline TMHZ_6P(void) :
		TDriverHandle()
	{}
};

/*    FBS2= class(TTDriverHandle)
          private
			CP, TP, RP: real;      //zbiornik sterujący, czasowy, redukcyjny
			XP: real;              //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
                RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
//        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
                Fala: boolean;
          public
                function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
                procedure Init(press: real); override;
                procedure SetReductor(nAdj: real); override;
                function GetSound(i: int): real; override;
                function GetPos(i: int): real; override;
          end;                    */

/*    TD2= class(TTDriverHandle)
              private
				  CP, TP, RP: real;      //zbiornik sterujący, czasowy, redukcyjny
				  XP: real;              //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
                RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
//        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
                Fala: boolean;
              public
                function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
                procedure Init(press: real); override;
                procedure SetReductor(nAdj: real); override;
                function GetSound(i: int): real; override;
                function GetPos(i: int): real; override;
              end;*/

class TM394 : public TDriverHandle {

  private:
		double CP = 0.0; //zbiornik sterujący, czasowy, redukcyjny
		double RedAdj = 0.0; //dostosowanie reduktora cisnienia (krecenie kapturkiem)
		static double const pos_table[11]; // = { -1, 5, -1, 0, 1, 2, 4, 5, 0, 0, 0 };

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetCP()/*override*/;
		double GetRP()/*override*/;
		double GetPos(int i)/*override*/;

		inline TM394(void) :
			TDriverHandle()
		{
            i_bcpno = 5; }
};

class TH14K1 : public TDriverHandle {

  private:
		static double const BPT_K[/*?*/ /*-1..4*/ (4) - (-1) + 1][2];
		static double const pos_table[11]; // = {-1, 4, -1, 0, 1, 2, 3, 4, 0, 0, 0};

  protected:
		double CP = 0.0; //zbiornik sterujący, czasowy, redukcyjny
		double RedAdj = 0.0; //dostosowanie reduktora cisnienia (krecenie kapturkiem)

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetCP()/*override*/;
		double GetRP()/*override*/;
		double GetPos(int i)/*override*/;

		inline TH14K1(void) :
			TDriverHandle()
		{
            i_bcpno = 4; }
};

class TSt113 : public TH14K1 {

  private:
		double EPS = 0.0;
		static double const BPT_K[/*?*/ /*-1..4*/ (4) - (-1) + 1][2];
		static double const BEP_K[/*?*/ /*-1..5*/ (5) - (-1) + 1];
		static double const pos_table[11]; // = {-1, 5, -1, 0, 2, 3, 4, 5, 0, 0, 1};
		double CP = 0;
  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		double GetCP()/*override*/;
		double GetRP()/*override*/;
		double GetEP()/*override*/;
		double GetPos(int i)/*override*/;
		void Init(double Press)/*override*/;

		inline TSt113(void) :
			TH14K1()
		{}
};

class Ttest : public TDriverHandle {

  private:
		double CP = 0.0;

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;

		inline Ttest(void) :
			TDriverHandle()
		{}
};

class TFD1 : public TDriverHandle {

  private:
		double MaxBP = 0.0; //najwyzsze cisnienie
		double BP = 0.0; //aktualne cisnienie

  public:
		double Speed = 0.0;  //szybkosc dzialania

		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		double GetCP()/*override*/;
    void SetSpeed(double nSpeed);
    //        procedure Init(press: real; MaxBP: real); overload;

		inline TFD1(void) :
			TDriverHandle()
		{}
};

class TH1405 : public TDriverHandle {

  private:
		double MaxBP = 0.0; //najwyzsze cisnienie
		double BP = 0.0; //aktualne cisnienie

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		double GetCP()/*override*/;
    //        procedure Init(press: real; MaxBP: real); overload;

		inline TH1405(void) :
			TDriverHandle()
		{}
};

class TFVel6 : public TDriverHandle {

  private:
		double EPS = 0.0;
		static double const pos_table[ 11 ]; // = {-1, 6, -1, 0, 6, 4, 4.7, 5, -1, 0, 1};
		double CP = 0.0;

  public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		double GetCP()/*override*/;
		double GetRP()/*override*/;
		double GetEP()/*override*/;
		double GetPos(int i)/*override*/;
		double GetSound(int i)/*override*/;
		void Init(double Press)/*override*/;

		inline TFVel6(void) :
			TDriverHandle()
		{}
};

class TFVE408 : public TDriverHandle {

private:
	double EPS = 0.0;
	static double const pos_table[11]; // = {-1, 6, -1, 0, 6, 4, 4.7, 5, -1, 0, 1};
	double CP = 0.0;

public:
	double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
	double GetCP()/*override*/;
	double GetEP()/*override*/;
	double GetRP()/*override*/;
	double GetPos(int i)/*override*/;
	double GetSound(int i)/*override*/;
	void Init(double Press)/*override*/;

	inline TFVE408(void) :
		TDriverHandle()
	{}
};


extern double PF( double const P1, double const P2, double const S, double const DP = 0.25 );
extern double PF1( double const P1, double const P2, double const S );

extern double PFVa( double PH, double PL, double const S, double LIM, double const DP = 0.1 ); //zawor napelniajacy z PH do PL, PL do LIM
extern double PFVd( double PH, double PL, double const S, double LIM, double const DP = 0.1 ); //zawor wypuszczajacy z PH do PL, PH do LIM
