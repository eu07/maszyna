//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "Mover.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: tu nale¿y przenosiæ funcje z mover.pas, które nie s¹ z niego wywo³ywane.
// Jeœli jakieœ zmienne nie s¹ u¿ywane w mover.pas, te¿ mo¿na je przenosiæ.
// Przeniesienie wszystkiego na raz zrobi³o by zbyt wielki chaos do ogarniêcia.

const dEpsilon=0.001;

__fastcall TMoverParameters::TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
 int LoadInitial,AnsiString LoadTypeInitial,int Cab)
 : T_MoverParameters(VelInitial,TypeNameInit,NameInit,LoadInitial,LoadTypeInitial,Cab)
{//g³ówny konstruktor
 DimHalf.x=0.5*Dim.W; //po³owa szerokoœci
 DimHalf.y=0.5*Dim.L; //po³owa d³ugoœci
 DimHalf.z=0.5*Dim.H; //po³owa wysokoœci
};


double __fastcall TMoverParameters::Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2)
{//zwraca odleg³oœæ pomiêdzy pojazdami (Loc1) i (Loc2) z uwzglêdnieneim ich d³ugoœci (kule!)
 return hypot(Loc2.X-Loc1.X,Loc1.Y-Loc2.Y)-0.5*(Dim2.L+Dim1.L);
};

double __fastcall TMoverParameters::Distance(const vector3 &s1, const vector3 &s2, const vector3 &d1, const vector3 &d2)
{//obliczenie odleg³oœci prostopad³oœcianów o œrodkach (s1) i (s2) i wymiarach (d1) i (d2)
};

double __fastcall TMoverParameters::CouplerDist(Byte Coupler)
{//obliczenie odleg³oœci pomiêdzy sprzêgami (kula!)
 return Couplers[Coupler].CoupleDist=Distance(Loc,Couplers[Coupler].Connected->Loc,Dim,Couplers[Coupler].Connected->Dim); //odleg³oœæ pomiêdzy sprzêgami (kula!)
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType)
{//³¹czenie do swojego sprzêgu (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
 //Ra: zwykle wykonywane dwukrotnie, dla ka¿dego pojazdu oddzielnie
 //Ra: trzeba by odró¿niæ wymóg dociœniêcia od uszkodzenia sprzêgu przy podczepianiu AI do sk³adu
 if (ConnectTo) //jeœli nie pusty
 {
  if (ConnectToNr!=2) Couplers[ConnectNo].ConnectedNr=ConnectToNr; //2=nic nie pod³¹czone
  TCouplerType ct=ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplerType; //typ sprzêgu pod³¹czanego pojazdu
  Couplers[ConnectNo].Connected=ConnectTo; //tak podpi¹æ zawsze mo¿na, najwy¿ej bêdzie wirtualny
  CouplerDist(ConnectNo); //Couplers[ConnectNo].CoupleDist=Distance(Loc,ConnectTo->Loc,Dim,ConnectTo->Dim); //odleg³oœæ pomiêdzy sprzêgami
  if ((CouplingType&ctrain_coupler)? //czy miewiertualny?
   ((Couplers[ConnectNo].CoupleDist<=dEpsilon)&&(Couplers[ConnectNo].CouplerType!=NoCoupler)&&(Couplers[ConnectNo].CouplerType==ct)):true)
  {//stykaja sie zderzaki i kompatybilne typy sprzegow, chyba ¿e wirtualny
   if (Couplers[ConnectNo].CouplingFlag==ctrain_virtual) //jeœli wczeœniej nie by³o po³¹czone
   {//ustalenie z której strony rysowaæ sprzêg
    Couplers[ConnectNo].Render=true; //tego rysowaæ
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].Render=false; //a tego nie
   };
   Couplers[ConnectNo].CouplingFlag=CouplingType;
   if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie ³¹czymy zwrotnie!
   {//jeœli ³¹czenie sprzêgiem niewirtualnym, ustawiamy po³¹czenie zwrotne
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=CouplingType;
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].Connected=this;
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist=Couplers[ConnectNo].CoupleDist;
    return true;
   }
   else
    return false; //pod³¹czenie nie uda³o siê - jest wirtualne
  }
 }
 return false; //brak pod³¹czanego pojazdu, zbyt du¿a odleg³oœæ, niezgodny typ sprzêgu, brak sprzêgu, brak haka
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType)
{//³¹czenie do (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
 return Attach(ConnectNo,ConnectToNr,(TMoverParameters*)ConnectTo,CouplingType);
};

bool __fastcall TMoverParameters::DettachDistance(Byte ConnectNo)
{//Ra: sprawdzenie, czy odleg³oœæ jest dobra do roz³¹czania
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to roz³¹czanie jest OK
 if ((Couplers[ConnectNo].CouplingFlag&ctrain_coupler)==0) return true; //hak nie po³¹czony - roz³¹czanie jest OK
 if (TestFlag(DamageFlag,dtrain_coupling)) return true; //hak urwany - roz³¹czanie jest OK
 //ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
 //if (CouplerType==Articulated) return false; //sprzêg nie do rozpiêcia - mo¿e byæ tylko urwany
 //Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
 CouplerDist(ConnectNo);
 return (Couplers[ConnectNo].CoupleDist<0.0)||(Couplers[ConnectNo].CoupleDist>0.2); //mo¿na roz³¹czaæ, jeœli dociœniêty
};

bool __fastcall TMoverParameters::Dettach(Byte ConnectNo)
{//rozlaczanie
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to odczepiono
 //with Couplers[ConnectNo] do
 if (DettachDistance(ConnectNo))
 {//gdy scisniete zderzaki, chyba ze zerwany sprzeg albo tylko wirtualny
  //Connected:=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych
  Couplers[ConnectNo].CouplingFlag=0; //pozostaje sprzêg wirtualny
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=0; //pozostaje sprzêg wirtualny
  return true;
 }
 //od³¹czamy wê¿e i resztê, pozostaje sprzêg fizyczny, który wymaga dociœniêcia
 Couplers[ConnectNo].CouplingFlag&=ctrain_coupler;
 Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=Couplers[ConnectNo].CouplingFlag;
 return false; //jeszcze nie roz³¹czony
};

void __fastcall TMoverParameters::SetCoupleDist()
{//przeliczenie odleg³oœci sprzêgów
 if (Couplers[0].Connected)
 {
  CouplerDist(0);
  if (CategoryFlag&2)
  {//Ra: dla samochodów zderzanie kul to za ma³o
  }
 }
 if (Couplers[1].Connected)
 {
  CouplerDist(1);
  if (CategoryFlag&2)
  {//Ra: dla samochodów zderzanie kul to za ma³o
  }
 }
};

