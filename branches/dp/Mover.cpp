//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include "Mover.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: tu nale�y przenosi� funcje z mover.pas, kt�re nie s� z niego wywo�ywane.
// Je�li jakie� zmienne nie s� u�ywane w mover.pas, te� mo�na je przenosi�.
// Przeniesienie wszystkiego na raz zrobi�o by zbyt wielki chaos do ogarni�cia.

const dEpsilon=0.001;

__fastcall TMoverParameters::TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
 int LoadInitial,AnsiString LoadTypeInitial,int Cab)
 : T_MoverParameters(VelInitial,TypeNameInit,NameInit,LoadInitial,LoadTypeInitial,Cab)
{//g��wny konstruktor
 DimHalf.x=0.5*Dim.W; //po�owa szeroko�ci
 DimHalf.y=0.5*Dim.L; //po�owa d�ugo�ci
 DimHalf.z=0.5*Dim.H; //po�owa wysoko�ci
};


double __fastcall TMoverParameters::Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2)
{//zwraca odleg�o�� pomi�dzy pojazdami (Loc1) i (Loc2) z uwzgl�dnieneim ich d�ugo�ci (kule!)
 return hypot(Loc2.X-Loc1.X,Loc1.Y-Loc2.Y)-0.5*(Dim2.L+Dim1.L);
};

double __fastcall TMoverParameters::Distance(const vector3 &s1, const vector3 &s2, const vector3 &d1, const vector3 &d2)
{//obliczenie odleg�o�ci prostopad�o�cian�w o �rodkach (s1) i (s2) i wymiarach (d1) i (d2)
};

double __fastcall TMoverParameters::CouplerDist(Byte Coupler)
{//obliczenie odleg�o�ci pomi�dzy sprz�gami (kula!)
 return Couplers[Coupler].CoupleDist=Distance(Loc,Couplers[Coupler].Connected->Loc,Dim,Couplers[Coupler].Connected->Dim); //odleg�o�� pomi�dzy sprz�gami (kula!)
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType)
{//��czenie do swojego sprz�gu (ConnectNo) pojazdu (ConnectTo) stron� (ConnectToNr)
 //Ra: zwykle wykonywane dwukrotnie, dla ka�dego pojazdu oddzielnie
 //Ra: trzeba by odr�ni� wym�g doci�ni�cia od uszkodzenia sprz�gu przy podczepianiu AI do sk�adu
 if (ConnectTo) //je�li nie pusty
 {
  if (ConnectToNr!=2) Couplers[ConnectNo].ConnectedNr=ConnectToNr; //2=nic nie pod��czone
  TCouplerType ct=ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplerType; //typ sprz�gu pod��czanego pojazdu
  Couplers[ConnectNo].Connected=ConnectTo; //tak podpi�� zawsze mo�na, najwy�ej b�dzie wirtualny
  CouplerDist(ConnectNo); //Couplers[ConnectNo].CoupleDist=Distance(Loc,ConnectTo->Loc,Dim,ConnectTo->Dim); //odleg�o�� pomi�dzy sprz�gami
  if ((CouplingType&ctrain_coupler)? //czy miewiertualny?
   ((Couplers[ConnectNo].CoupleDist<=dEpsilon)&&(Couplers[ConnectNo].CouplerType!=NoCoupler)&&(Couplers[ConnectNo].CouplerType==ct)):true)
  {//stykaja sie zderzaki i kompatybilne typy sprzegow, chyba �e wirtualny
   if (Couplers[ConnectNo].CouplingFlag==ctrain_virtual) //je�li wcze�niej nie by�o po��czone
   {//ustalenie z kt�rej strony rysowa� sprz�g
    Couplers[ConnectNo].Render=true; //tego rysowa�
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].Render=false; //a tego nie
   };
   Couplers[ConnectNo].CouplingFlag=CouplingType;
   if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie ��czymy zwrotnie!
   {//je�li ��czenie sprz�giem niewirtualnym, ustawiamy po��czenie zwrotne
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=CouplingType;
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].Connected=this;
    Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist=Couplers[ConnectNo].CoupleDist;
    return true;
   }
   else
    return false; //pod��czenie nie uda�o si� - jest wirtualne
  }
 }
 return false; //brak pod��czanego pojazdu, zbyt du�a odleg�o��, niezgodny typ sprz�gu, brak sprz�gu, brak haka
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType)
{//��czenie do (ConnectNo) pojazdu (ConnectTo) stron� (ConnectToNr)
 return Attach(ConnectNo,ConnectToNr,(TMoverParameters*)ConnectTo,CouplingType);
};

bool __fastcall TMoverParameters::DettachDistance(Byte ConnectNo)
{//Ra: sprawdzenie, czy odleg�o�� jest dobra do roz��czania
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to roz��czanie jest OK
 if ((Couplers[ConnectNo].CouplingFlag&ctrain_coupler)==0) return true; //hak nie po��czony - roz��czanie jest OK
 if (TestFlag(DamageFlag,dtrain_coupling)) return true; //hak urwany - roz��czanie jest OK
 //ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
 //if (CouplerType==Articulated) return false; //sprz�g nie do rozpi�cia - mo�e by� tylko urwany
 //Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
 CouplerDist(ConnectNo);
 return (Couplers[ConnectNo].CoupleDist<0.0)||(Couplers[ConnectNo].CoupleDist>0.2); //mo�na roz��cza�, je�li doci�ni�ty
};

bool __fastcall TMoverParameters::Dettach(Byte ConnectNo)
{//rozlaczanie
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to odczepiono
 //with Couplers[ConnectNo] do
 if (DettachDistance(ConnectNo))
 {//gdy scisniete zderzaki, chyba ze zerwany sprzeg albo tylko wirtualny
  //Connected:=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych
  Couplers[ConnectNo].CouplingFlag=0; //pozostaje sprz�g wirtualny
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=0; //pozostaje sprz�g wirtualny
  return true;
 }
 //od��czamy w�e i reszt�, pozostaje sprz�g fizyczny, kt�ry wymaga doci�ni�cia
 Couplers[ConnectNo].CouplingFlag&=ctrain_coupler;
 Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=Couplers[ConnectNo].CouplingFlag;
 return false; //jeszcze nie roz��czony
};

void __fastcall TMoverParameters::SetCoupleDist()
{//przeliczenie odleg�o�ci sprz�g�w
 if (Couplers[0].Connected)
 {
  CouplerDist(0);
  if (CategoryFlag&2)
  {//Ra: dla samochod�w zderzanie kul to za ma�o
  }
 }
 if (Couplers[1].Connected)
 {
  CouplerDist(1);
  if (CategoryFlag&2)
  {//Ra: dla samochod�w zderzanie kul to za ma�o
  }
 }
};

