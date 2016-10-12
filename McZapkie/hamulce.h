#pragma once
#ifndef INCLUDED_HAMULCE_H
#define INCLUDED_HAMULCE_H
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
duzo wersji ¿eliwa
KE
Tarcze od 152A
Magnetyki (implementacja w mover.pas)
Matrosow 394
H14K1 (zasadniczy), H1405 (pomocniczy), St113 (ep)
Knorr/West EP - ¿eby by³
*/

#include "friction.h" // Pascal unit
#include <SysUtils.hpp> // Pascal unit
#include <mctools.hpp> // Pascal unit
#include <SysInit.hpp> // Pascal unit
#include <System.hpp> // Pascal unit



	static int const LocalBrakePosNo = 10;         /*ilosc nastaw hamulca recznego lub pomocniczego*/
	static int const MainBrakeMaxPos = 10;          /*max. ilosc nastaw hamulca zasadniczego*/

	/*nastawy hamulca*/
	static int const bdelay_G = 1;    //G
	static int const bdelay_P = 2;    //P
	static int const bdelay_R = 4;    //R
	static int const bdelay_M = 8;    //Mg
	static int const bdelay_GR = 128; //G-R


	/*stan hamulca*/
	static int const b_off = 0;   //luzowanie
	static int const b_hld = 1;   //trzymanie
	static int const b_on = 2;   //napelnianie
	static int const b_rfl = 4;   //uzupelnianie
	static int const b_rls = 8;   //odluzniacz
	static int const b_ep = 16;   //elektropneumatyczny
	static int const b_asb = 32;   //elektropneumatyczny   
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
	static int const bp_P10Bg = 2; //¿eliwo fosforowe P10
	static int const bp_P10Bgu = 1;
	static int const bp_LLBg = 4; //komp. b.n.t.
	static int const bp_LLBgu = 3;
	static int const bp_LBg = 6; //komp. n.t.
	static int const bp_LBgu = 5;
	static int const bp_KBg = 8; //komp. w.t.
	static int const bp_KBgu = 7;
	static int const bp_D1 = 9; //tarcze
	static int const bp_D2 = 10;
	static int const bp_FR513 = 11; //Frenoplast FR513
	static int const bp_Cosid = 12; //jakistam kompozyt :D
	static int const bp_PKPBg = 13; //¿eliwo PKP
	static int const bp_PKPBgu = 14;
	static int const bp_MHS = 128; //magnetyczny hamulec szynowy
	static int const bp_P10yBg = 15; //¿eliwo fosforowe P10
	static int const bp_P10yBgu = 16;
	static int const bp_FR510 = 17; //Frenoplast FR510

	static int const sf_Acc = 1;  //przyspieszacz
	static int const sf_BR = 2;  //przekladnia
	static int const sf_CylB = 4;  //cylinder - napelnianie
	static int const sf_CylU = 8;  //cylinder - oproznianie
	static int const sf_rel = 16; //odluzniacz
	static int const sf_ep = 32; //zawory ep

	static int const bh_MIN = 0;  //minimalna pozycja
	static int const bh_MAX = 1;  //maksymalna pozycja
	static int const bh_FS = 2;  //napelnianie uderzeniowe //jesli nie ma, to jazda
	static int const bh_RP = 3;  //jazda
	static int const bh_NP = 4;  //odciecie - podwojna trakcja
	static int const bh_MB = 5;  //odciecie - utrzymanie stopnia hamowania/pierwszy 1 stopien hamowania
	static int const bh_FB = 6;  //pelne
	static int const bh_EB = 7;  //nagle
	static int const bh_EPR = 8;  //ep - luzowanie  //pelny luz dla ep k¹towego
	static int const bh_EPN = 9;  //ep - utrzymanie //jesli rowne luzowaniu, wtedy sterowanie przyciskiem
	static int const bh_EPB = 10;  //ep - hamowanie  //pelne hamowanie dla ep k¹towego


	static double const SpgD = 0.7917;
	static double const SpO = 0.5067;  //przekroj przewodu 1" w l/m
				 //wyj: jednostka dosyc dziwna, ale wszystkie obliczenia
				 //i pojemnosci sa podane w litrach (rozsadne wielkosci)
				 //zas dlugosc pojazdow jest podana w metrach
				 //a predkosc przeplywu w m/s                           //3.5
																		//7//1.5
 //   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (14, 5.4), (9, 5.0), (6, 4.6), (9, 4.5), (9, 4.0), (9, 3.5), (9, 2.8), (34, 2.8));
 //   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (7, 5.0), (2.0, 5.0), (4.5, 4.6), (4.5, 4.2), (4.5, 3.8), (4.5, 3.4), (4.5, 2.8), (8, 2.8));
	static double const BPT[ /*?*//*-2..6*/ (6) - (-2) + 1][2] = { (0 , 5.0) , (7 , 5.0) , (2.0 , 5.0) , (4.5 , 4.6) , (4.5 , 4.2) , (4.5 , 3.8) , (4.5 , 3.4) , (4.5 , 2.8) , (8 , 2.8) };
	static double const BPT_394[ /*?*//*-1..5*/ (5) - (-1) + 1][2] = { (13 , 10.0) , (5 , 5.0) , (0 , -1) , (5 , -1) , (5 , 0.0) , (5 , 0.0) , (18 , 0.0) };
	//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (12, 5.4), (9, 5.0), (9, 4.6), (9, 4.2), (9, 3.8), (9, 3.4), (9, 2.8), (34, 2.8));
	//      BPT: array[-2..6] of array [0..1] of real= ((0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0));
	static int const i_bcpno = 6;
	static double const pi = 3.141592653589793;

	//klasa obejmujaca pojedyncze zbiorniki


	class TReservoir
	{
	protected:
		double Cap;
		double Vol;
		double dVol;

	public:
		TReservoir();
		void CreateCap(double Capacity);
		void CreatePress(double Press);
		virtual double pa();
		virtual double P();
		void Flow(double dv);
		void Act();

	};


	typedef TReservoir *PReservoir;


	class TBrakeCyl : public TReservoir

	{
	public:
		virtual double pa()/*override*/;
		virtual double P()/*override*/;

	};


	//klasa obejmujaca uklad hamulca zespolonego pojazdu


	class TBrake
	{
	protected:
		TReservoir *BrakeCyl;      //silownik
		TReservoir *BrakeRes;      //ZP
		TReservoir *ValveRes;      //komora wstepna
		int BCN;                 //ilosc silownikow
		double BCM;                 //przekladnia hamulcowa
		double BCA;                 //laczny przekroj silownikow
		int BrakeDelays;         //dostepne opoznienia
		int BrakeDelayFlag;      //aktualna nastawa
		TFricMat *FM;              //material cierny
		double MaxBP;               //najwyzsze cisnienie
		int BA;                  //osie hamowane
		int NBpA;                //klocki na os
		double SizeBR;              //rozmiar^2 ZP (w stosunku do 14")
		double SizeBC;              //rozmiar^2 CH (w stosunku do 14")
		bool DCV;              //podwojny zawor zwrotny
		double ASBP;                //cisnienie hamulca pp

		int BrakeStatus; //flaga stanu
		int SoundFlag;

	public:
		TBrake(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa);
		//maksymalne cisnienie, promien, skok roboczy, pojemnosc ZP;
		//ilosc cylindrow, opoznienia hamulca, material klockow, osie hamowane, klocki na os;

		virtual double GetFC(double Vel, double N);         //wspolczynnik tarcia - hamulec wie lepiej
		virtual double GetPF(double PP, double dt, double Vel);      //przeplyw miedzy komora wstepna i PG
		double GetBCF();                           //sila tlokowa z tloka
		virtual double GetHPFlow(double HP, double dt);  //przeplyw - 8 bar
		virtual double GetBCP();  //cisnienie cylindrow hamulcowych
		double GetBRP(); //cisnienie zbiornika pomocniczego
		double GetVRP(); //cisnienie komory wstepnej rozdzielacza
		virtual double GetCRP();  //cisnienie zbiornika sterujacego
		virtual void Init(double PP, double HPP, double LPP, double BP, int BDF);  //inicjalizacja hamulca
		bool SetBDF(int nBDF); //nastawiacz GPRM
		void Releaser(int state); //odluzniacz
		virtual void SetEPS(double nEPS); //hamulec EP
		void ASB(int state); //hamulec przeciwposlizgowy
		int GetStatus(); //flaga statusu, moze sie przydac do odglosow
		void SetASBP(double Press); //ustalenie cisnienia pp
		virtual void ForceEmptiness();
		int GetSoundFlag();
		//        procedure

	};



	class TWest : public TBrake

	{
	private:
		double LBP;           //cisnienie hamulca pomocniczego
		double dVP;           //pobor powietrza wysokiego cisnienia
		double EPS;           //stan elektropneumatyka
		double TareM; double LoadM;  //masa proznego i pelnego
		double TareBP;        //cisnienie dla proznego
		double LoadC;         //wspolczynnik przystawki wazacej

	public:
		void SetLBP(double P);   //cisnienie z hamulca pomocniczego
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		double GetHPFlow(double HP, double dt)/*override*/;
		void PLC(double mass);  //wspolczynnik cisnienia przystawki wazacej
		void SetEPS(double nEPS)/*override*/;  //stan hamulca EP
		void SetLP(double TM, double LM, double TBP);  //parametry przystawki wazacej

		inline TWest(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TESt : public TBrake

	{
	protected:
		TReservoir *CntrlRes;      //zbiornik steruj¹cy
		double BVM;                 //przelozenie PG-CH

	public:
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void EStParams(double i_crc);                 //parametry charakterystyczne dla ESt
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		double GetCRP()/*override*/;
		void CheckState(double BCP, double & dV1); //glowny przyrzad rozrzadczy
		void CheckReleaser(double dt); //odluzniacz
		double CVs(double BP);      //napelniacz sterujacego
		double BVs(double BCP);     //napelniacz pomocniczego

		inline TESt(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TESt3 : public TESt

	{
	private:
		double CylFlowSpeed[2][2];

	public:
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG

		inline TESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TESt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TESt3AL2 : public TESt3

	{
	private:
		double TareM; double LoadM;  //masa proznego i pelnego
		double TareBP;  //cisnienie dla proznego
		double LoadC;

	public:
		TReservoir *ImplsRes;      //komora impulsowa
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void PLC(double mass);  //wspolczynnik cisnienia przystawki wazacej
		void SetLP(double TM, double LM, double TBP);  //parametry przystawki wazacej
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;

		inline TESt3AL2(double i_mbp, double i_bcr, double i_bcd, double i_brc
			, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TESt3(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TESt4R : public TESt

	{
	private:
		bool RapidStatus;
	protected:
		TReservoir *ImplsRes;      //komora impulsowa
		double RapidTemp;           //akrualne, zmienne przelozenie

	public:
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;

		inline TESt4R(double i_mbp, double i_bcr, double i_bcd, double i_brc
			, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TESt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TLSt : public TESt4R

	{
	private:
		double CylFlowSpeed[2][2];
	protected:
		double LBP;       //cisnienie hamulca pomocniczego
		double RM;        //przelozenie rapida
		double EDFlag; //luzowanie hamulca z powodu zalaczonego ED

	public:
		void SetLBP(double P);   //cisnienie z hamulca pomocniczego
		void SetRM(double RMR);   //ustalenie przelozenia rapida
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		double GetHPFlow(double HP, double dt)/*override*/;  //przeplyw - 8 bar
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		virtual double GetEDBCP();    //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
		void SetED(double EDstate); //stan hamulca ED do luzowania

		inline TLSt(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TESt4R(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TEStED : public TLSt  //zawor z EP09 - Est4 z oddzielnym przekladnikiem, kontrola rapidu i takie tam

	{
	private:
		double Nozzles[11]; //dysze
		bool Zamykajacy;       //pamiec zaworka zamykajacego
		bool Przys_blok;       //blokada przyspieszacza
		TReservoir *Miedzypoj;     //pojemnosc posrednia (urojona) do napelniania ZP i ZS
		double TareM; double LoadM;  //masa proznego i pelnego
		double TareBP;  //cisnienie dla proznego
		double LoadC;

	public:
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		double GetEDBCP()/*override*/;    //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED
		void PLC(double mass);  //wspolczynnik cisnienia przystawki wazacej
		void SetLP(double TM, double LM, double TBP);  //parametry przystawki wazacej        

		inline TEStED(double i_mbp, double i_bcr, double i_bcd, double i_brc
			, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TLSt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TEStEP2 : public TLSt

	{
	private:
		double TareM; double LoadM;  //masa proznego i pelnego
		double TareBP;  //cisnienie dla proznego
		double LoadC;
		double EPS;

	public:
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;   //inicjalizacja
		void PLC(double mass);  //wspolczynnik cisnienia przystawki wazacej
		void SetEPS(double nEPS)/*override*/;  //stan hamulca EP
		void SetLP(double TM, double LM, double TBP);  //parametry przystawki wazacej

		inline TEStEP2(double i_mbp, double i_bcr, double i_bcd, double i_brc
			, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TLSt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TCV1 : public TBrake

	{
	private:
		double BVM;                 //przelozenie PG-CH
	protected:
		TReservoir *CntrlRes;      //zbiornik steruj¹cy

	public:
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		double GetCRP()/*override*/;
		void CheckState(double BCP, double & dV1);
		double CVs(double BP);
		double BVs(double BCP);

		inline TCV1(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TCV1R : public TCV1

	{
	private:
		TReservoir *ImplsRes;      //komora impulsowa
		bool RapidStatus;

	public:
		//        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
		//        procedure Init(PP, HPP, LPP, BP: real; BDF: int); override;

		inline TCV1R(double i_mbp, double i_bcr, double i_bcd, double i_brc,
			int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TCV1(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TCV1L_TR : public TCV1

	{
	private:
		TReservoir *ImplsRes;      //komora impulsowa
		double LBP;     //cisnienie hamulca pomocniczego

	public:
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		void SetLBP(double P);   //cisnienie z hamulca pomocniczego
		double GetHPFlow(double HP, double dt)/*override*/;  //przeplyw - 8 bar

		inline TCV1L_TR(double i_mbp, double i_bcr, double i_bcd, double i_brc
			, int i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TCV1(i_mbp, i_bcr, i_bcd, i_brc, i_bcn
				, i_BD, i_mat, i_ba, i_nbpa) { }
	};



	class TKE : public TBrake //Knorr Einheitsbauart — jeden do wszystkiego

	{
	private:
		bool RapidStatus;
		TReservoir *ImplsRes;      //komora impulsowa
		TReservoir *CntrlRes;      //zbiornik steruj¹cy
		TReservoir *Brak2Res;      //zbiornik pomocniczy 2        
		double BVM;                 //przelozenie PG-CH
		double TareM; double LoadM;        //masa proznego i pelnego
		double TareBP;              //cisnienie dla proznego
		double LoadC;               //wspolczynnik zaladowania
		double RM;                  //przelozenie rapida
		double LBP;                 //cisnienie hamulca pomocniczego

	public:
		void SetRM(double RMR);   //ustalenie przelozenia rapida
		double GetPF(double PP, double dt, double Vel)/*override*/;      //przeplyw miedzy komora wstepna i PG
		void Init(double PP, double HPP, double LPP, double BP, int BDF)/*override*/;
		double GetHPFlow(double HP, double dt)/*override*/;  //przeplyw - 8 bar
		double GetCRP()/*override*/;
		void CheckState(double BCP, double & dV1);
		void CheckReleaser(double dt); //odluzniacz
		double CVs(double BP);      //napelniacz sterujacego
		double BVs(double BCP);     //napelniacz pomocniczego
		void PLC(double mass);  //wspolczynnik cisnienia przystawki wazacej
		void SetLP(double TM, double LM, double TBP);  //parametry przystawki wazacej
		void SetLBP(double P);   //cisnienie z hamulca pomocniczego

		inline TKE(double i_mbp, double i_bcr, double i_bcd, double i_brc, int
			i_bcn, int i_BD, int i_mat, int i_ba, int i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn,
				i_BD, i_mat, i_ba, i_nbpa) { }
	};





	//klasa obejmujaca krany


	class TDriverHandle
	{
	private:
		//        BCP: integer;

	public:
		bool Time;
		bool TimeEP;
		double Sounds[5];       //wielkosci przeplywow dla dzwiekow              
		virtual double GetPF(double i_bcp, double PP, double HP, double dt, double ep);
		virtual void Init(double Press);
		virtual double GetCP();
		virtual void SetReductor(double nAdj);
		virtual double GetSound(int i);
		virtual double GetPos(int i);

	};



	class TFV4a : public TDriverHandle

	{
	private:
		double CP; double TP; double RP;      //zbiornik steruj¹cy, czasowy, redukcyjny

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;

		inline TFV4a(void) : TDriverHandle() { }

	};



	class TFV4aM : public TDriverHandle

	{
	private:
		double CP; double TP; double RP;      //zbiornik steruj¹cy, czasowy, redukcyjny
		double XP;              //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
		double RedAdj;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
//        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
		bool Fala;
		double LPP_RP(double pos);
		bool EQ(double pos, double i_pos);
	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetSound(int i)/*override*/;
		double GetPos(int i)/*override*/;

		inline TFV4aM(void) : TDriverHandle() { }
	};



	class TMHZ_EN57 : public TDriverHandle

	{
	private:
		double CP; double TP; double RP;      //zbiornik steruj¹cy, czasowy, redukcyjny
		double RedAdj;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
		bool Fala;
		double LPP_RP(double pos);
		bool EQ(double pos, double i_pos);

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetSound(int i)/*override*/;
		double GetPos(int i)/*override*/;
		double GetCP()/*override*/;
		double GetEP(double pos);

		inline TMHZ_EN57(void) : TDriverHandle() { }
	};



	/*    FBS2= class(TTDriverHandle)
		  private
			CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
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
				  CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
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


	class TM394 : public TDriverHandle

	{
	private:
		double CP;      //zbiornik steruj¹cy, czasowy, redukcyjny
		double RedAdj;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetCP()/*override*/;
		double GetPos(int i)/*override*/;

		inline TM394(void) : TDriverHandle() { }
	};



	class TH14K1 : public TDriverHandle

	{
	protected:
		double CP;      //zbiornik steruj¹cy, czasowy, redukcyjny
		double RedAdj;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		void SetReductor(double nAdj)/*override*/;
		double GetCP()/*override*/;
		double GetPos(int i)/*override*/;

		inline TH14K1(void) : TDriverHandle() { }
	};



	class TSt113 : public TH14K1

	{
	private:
		double EPS;

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		double GetCP()/*override*/;
		double GetPos(int i)/*override*/;
		void Init(double Press)/*override*/;

		inline TSt113(void) : TH14K1() { }
	};



	class Ttest : public TDriverHandle

	{
	private:
		double CP;

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;

		inline Ttest(void) : TDriverHandle() { }
	};



	class TFD1 : public TDriverHandle

	{
	private:
		double MaxBP;  //najwyzsze cisnienie
		double BP;     //aktualne cisnienie

	public:
		double Speed;  //szybkosc dzialania
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		double GetCP()/*override*/;
		void SetSpeed(double nSpeed);
		//        procedure Init(press: real; MaxBP: real); overload;

		inline TFD1(void) : TDriverHandle() { }
	};



	class TH1405 : public TDriverHandle

	{
	private:
		double MaxBP;  //najwyzsze cisnienie
		double BP;     //aktualne cisnienie

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		void Init(double Press)/*override*/;
		double GetCP()/*override*/;
		//        procedure Init(press: real; MaxBP: real); overload;

		inline TH1405(void) : TDriverHandle() { }
	};




	class TFVel6 : public TDriverHandle

	{
	private:
		double EPS;

	public:
		double GetPF(double i_bcp, double PP, double HP, double dt, double ep)/*override*/;
		double GetCP()/*override*/;
		double GetPos(int i)/*override*/;
		double GetSound(int i)/*override*/;
		void Init(double Press)/*override*/;

		inline TFVel6(void) : TDriverHandle() { }
	};


	extern double PF(double P1, double P2, double S, double DP = 0.25);
	extern double PF1(double P1, double P2, double S);

	extern double PFVa(double PH, double PL, double S, double LIM, double DP = 0.1); //zawor napelniajacy z PH do PL, PL do LIM
	extern double PFVd(double PH, double PL, double S, double LIM, double DP = 0.1); //zawor wypuszczajacy z PH do PL, PH do LIM

	extern long lround(double value); //zastepuje funkcje nieobecna w C++99

#if !defined(NO_IMPLICIT_NAMESPACE_USE)
#endif
#endif//INCLUDED_HAMULCE_H
//END
