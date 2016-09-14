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
 
#include <hamulce.hpp>	// Pascal unit
#include <friction.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

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


 static double/*?*/ const dMAX = 10;  //dysze
 static double/*?*/ const dON = 0;  //osobowy napelnianie (+ZP)
 static double/*?*/ const dOO = 1;  //osobowy oproznianie
 static double/*?*/ const dTN = 2;  //towarowy napelnianie (+ZP)
 static double/*?*/ const dTO = 3;  //towarowy oproznianie
 static double/*?*/ const dP = 4;  //zbiornik pomocniczy
 static double/*?*/ const dSd = 5;  //zbiornik sterujacy
 static double/*?*/ const dSm = 6;  //zbiornik sterujacy
 static double/*?*/ const dPd = 7;  //duzy przelot zamykajcego
 static double/*?*/ const dPm = 8;  //maly przelot zamykajacego
 static double/*?*/ const dPO = 9;  //zasilanie pomocniczego O
 static double/*?*/ const dPT = 10;  //zasilanie pomocniczego T

//przekladniki
  static double/*?*/ const p_none = 0;
  static double/*?*/ const p_rapid = 1;
  static double/*?*/ const p_pp = 2;
  static double/*?*/ const p_al2 = 3;
  static double/*?*/ const p_ppz = 4;
  static double/*?*/ const P_ed = 5;




    
struct/*class*/ TPrzekladnik: public TReservoir //przekladnik (powtarzacz)
      
{
private:
      
public:
        PReservoir BrakeRes;
        PReservoir Next;
        virtual void Update(double dt); 
      
};


    
struct/*class*/ TRura: public TPrzekladnik //nieprzekladnik, rura laczaca
      
{
private:
      
public:
        double P()/*override*/; 
        void Update(double dt)/*override*/; 
      
};


    
struct/*class*/ TPrzeciwposlizg: public TRura //przy napelnianiu - rura, przy poslizgu - upust
      
{
private:
        bool Poslizg;
      
public:
        void SetPoslizg(bool flag);
        void Update(double dt)/*override*/; 
      
};


    
struct/*class*/ TRapid: public TPrzekladnik //przekladnik dwustopniowy
      
{
private:
        bool RapidStatus; //status rapidu
        double RapidMult;      //przelozenie (w dol)
//        Komora2: real;
        double DN; double DL;          //srednice dysz napelniania i luzowania
      
public:
        void SetRapidParams(double mult,  double size);
        void SetRapidStatus(bool rs);
        void Update(double dt)/*override*/; 
      
};


    
struct/*class*/ TPrzekCiagly: public TPrzekladnik //AL2
      
{
private:
        double mult;
      
public:
        void SetMult(double m);
        void Update(double dt)/*override*/; 
      
};


    
struct/*class*/ TPrzek_PZZ: public TPrzekladnik //podwojny zawor zwrotny
      
{
private:
        double LBP;
      
public:
        void SetLBP(double P);
        void Update(double dt)/*override*/; 
      
};


    
struct/*class*/ TPrzekZalamany: public TPrzekladnik //Knicksventil
      
{
private:
      
public:
      
};


    
struct/*class*/ TPrzekED: public TRura //przy napelnianiu - rura, przy hamowaniu - upust
      
{
private:
        double MaxP;
      
public:
        void SetP(double P);
        void Update(double dt)/*override*/; 
      
};


    
struct/*class*/ TNESt3: public TBrake
      
{
private:
        double Nozzles[dMAX+1]; //dysze
        TReservoir CntrlRes;      //zbiornik steruj¹cy
        double BVM;                 //przelozenie PG-CH
//        ValveFlag: byte;           //polozenie roznych zaworkow
        bool Zamykajacy;       //pamiec zaworka zamykajacego
//        Przys_wlot: boolean;       //wlot do komory przyspieszacza
        bool Przys_blok;       //blokada przyspieszacza
        TReservoir Miedzypoj;     //pojemnosc posrednia (urojona) do napelniania ZP i ZS
        TPrzekladnik Przekladniki[ /*?*//*1..3*/ (3)-(1)+1 ];
        bool RapidStatus;
        bool RapidStaly;
        double LoadC;
        double TareM; double LoadM;       //masa proznego i pelnego
        double TareBP;             //cisnienie dla proznego
        double HBG300;             //zawor ograniczajacy cisnienie
        double Podskok;            //podskok preznosci poczatkowej
//        HPBR: real;               //zasilanie ZP z wysokiego cisnienia
        bool autom;           //odluzniacz samoczynny
        double LBP;                //cisnienie hamulca pomocniczego        
      
public:
        double GetPF(double PP, double dt, double Vel)/*override*/;           //przeplyw miedzy komora wstepna i PG
        void EStParams(double i_crc);                           //parametry charakterystyczne dla ESt
        void Init(double PP, double HPP, double LPP, double BP,  unsigned char BDF)/*override*/; 
        double GetCRP()/*override*/; 
        void CheckState(double BCP,   double & dV1);             //glowny przyrzad rozrzadczy
        void CheckReleaser(double dt);                          //odluzniacz
        double CVs(double BP);                               //napelniacz sterujacego
        double BVs(double BCP);                              //napelniacz pomocniczego
        void SetSize(int size,  std::string params);           //ustawianie dysz (rozmiaru ZR), przekladniki
        void PLC(double mass);                                  //wspolczynnik cisnienia przystawki wazacej
        void SetLP(double TM, double LM, double TBP);                         //parametry przystawki wazacej
        void ForceEmptiness()/*override*/;                        //wymuszenie bycia pustym
        void SetLBP(double P);                                  //cisnienie z hamulca pomocniczego
      
};


double d2A(double d);



#endif//INCLUDED_OERLIKON_EST_H
//END
