//---------------------------------------------------------------------------

#include "Mover.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: tu nale�y przenosi� funcje z mover.pas, kt�re nie s� z niego wywo�ywane.
// Je�li jakie� zmienne nie s� u�ywane w mover.pas, te� mo�na je przenosi�.
// Przeniesienie wszystkiego na raz zrobi�o by zbyt wielki chaos do ogarni�cia.

const dEpsilon=0.01; //1cm (zale�y od typu sprz�gu...)

__fastcall TMoverParameters::TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
 int LoadInitial,AnsiString LoadTypeInitial,int Cab)
 : T_MoverParameters(VelInitial,TypeNameInit,NameInit,LoadInitial,LoadTypeInitial,Cab)
{//g��wny konstruktor
 DimHalf.x=0.5*Dim.W; //po�owa szeroko�ci, OX jest w bok?
 DimHalf.y=0.5*Dim.L; //po�owa d�ugo�ci, OY jest do przodu?
 DimHalf.z=0.5*Dim.H; //po�owa wysoko�ci, OZ jest w g�r�?
 BrakeLevelSet(-2); //Pascal ustawia na 0, przestawimy na odci�cie (CHK jest jeszcze nie wczytane!)
};


double __fastcall TMoverParameters::Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2)
{//zwraca odleg�o�� pomi�dzy pojazdami (Loc1) i (Loc2) z uwzgl�dnieneim ich d�ugo�ci (kule!)
 return hypot(Loc2.X-Loc1.X,Loc1.Y-Loc2.Y)-0.5*(Dim2.L+Dim1.L);
};

double __fastcall TMoverParameters::Distance(const vector3 &s1, const vector3 &s2, const vector3 &d1, const vector3 &d2)
{//obliczenie odleg�o�ci prostopad�o�cian�w o �rodkach (s1) i (s2) i wymiarach (d1) i (d2)
 //return 0.0; //b�dzie zg�asza� warning - funkcja do usuni�cia, chyba �e si� przyda...
};

double __fastcall TMoverParameters::CouplerDist(Byte Coupler)
{//obliczenie odleg�o�ci pomi�dzy sprz�gami (kula!)
 return Couplers[Coupler].CoupleDist=Distance(Loc,Couplers[Coupler].Connected->Loc,Dim,Couplers[Coupler].Connected->Dim); //odleg�o�� pomi�dzy sprz�gami (kula!)
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType,bool Forced)
{//��czenie do swojego sprz�gu (ConnectNo) pojazdu (ConnectTo) stron� (ConnectToNr)
 //Ra: zwykle wykonywane dwukrotnie, dla ka�dego pojazdu oddzielnie
 //Ra: trzeba by odr�ni� wym�g doci�ni�cia od uszkodzenia sprz�gu przy podczepianiu AI do sk�adu
 if (ConnectTo) //je�li nie pusty
 {
  if (ConnectToNr!=2) Couplers[ConnectNo].ConnectedNr=ConnectToNr; //2=nic nie pod��czone
  TCouplerType ct=ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplerType; //typ sprz�gu pod��czanego pojazdu
  Couplers[ConnectNo].Connected=ConnectTo; //tak podpi�� (do siebie) zawsze mo�na, najwy�ej b�dzie wirtualny
  CouplerDist(ConnectNo); //przeliczenie odleg�o�ci pomi�dzy sprz�gami
  if (CouplingType==ctrain_virtual) return false; //wirtualny wi�cej nic nie robi
  if (Forced?true:((Couplers[ConnectNo].CoupleDist<=dEpsilon)&&(Couplers[ConnectNo].CouplerType!=NoCoupler)&&(Couplers[ConnectNo].CouplerType==ct)))
  {//stykaja sie zderzaki i kompatybilne typy sprzegow, chyba �e ��czenie na starcie
   if (Couplers[ConnectNo].CouplingFlag==ctrain_virtual) //je�li wcze�niej nie by�o po��czone
   {//ustalenie z kt�rej strony rysowa� sprz�g
    Couplers[ConnectNo].Render=true; //tego rysowa�
    ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Render=false; //a tego nie
   };
   Couplers[ConnectNo].CouplingFlag=CouplingType; //ustawienie typu sprz�gu
   //if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie ��czymy zwrotnie!
   //{//je�li ��czenie sprz�giem niewirtualnym, ustawiamy po��czenie zwrotne
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=CouplingType;
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Connected=this;
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist=Couplers[ConnectNo].CoupleDist;
   return true;
   //}
   //pod��czenie nie uda�o si� - jest wirtualne
  }
 }
 return false; //brak pod��czanego pojazdu, zbyt du�a odleg�o��, niezgodny typ sprz�gu, brak sprz�gu, brak haka
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType,bool Forced)
{//��czenie do (ConnectNo) pojazdu (ConnectTo) stron� (ConnectToNr)
 return Attach(ConnectNo,ConnectToNr,(TMoverParameters*)ConnectTo,CouplingType,Forced);
};

int __fastcall TMoverParameters::DettachStatus(Byte ConnectNo)
{//Ra: sprawdzenie, czy odleg�o�� jest dobra do roz��czania
 //powinny by� 3 informacje: =0 sprz�g ju� roz��czony, <0 da si� roz��czy�. >0 nie da si� roz��czy�
 if (!Couplers[ConnectNo].Connected)
  return 0; //nie ma nic, to roz��czanie jest OK
 if ((Couplers[ConnectNo].CouplingFlag&ctrain_coupler)==0)
  return -Couplers[ConnectNo].CouplingFlag; //hak nie po��czony - roz��czanie jest OK
 if (TestFlag(DamageFlag,dtrain_coupling))
  return -Couplers[ConnectNo].CouplingFlag; //hak urwany - roz��czanie jest OK
 //ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
 //if (CouplerType==Articulated) return false; //sprz�g nie do rozpi�cia - mo�e by� tylko urwany
 //Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
 CouplerDist(ConnectNo);
 if (Couplers[ConnectNo].CouplerType==Screw?Couplers[ConnectNo].CoupleDist<0.0:true)
  return -Couplers[ConnectNo].CouplingFlag; //mo�na roz��cza�, je�li doci�ni�ty
 return (Couplers[ConnectNo].CoupleDist>0.2)?-Couplers[ConnectNo].CouplingFlag:Couplers[ConnectNo].CouplingFlag;
};

bool __fastcall TMoverParameters::Dettach(Byte ConnectNo)
{//rozlaczanie
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to odczepiono
 //with Couplers[ConnectNo] do
 int i=DettachStatus(ConnectNo); //stan sprz�gu
 if (i<0)
 {//gdy scisniete zderzaki, chyba ze zerwany sprzeg (wirtualnego nie odpinamy z drugiej strony)
  //Couplers[ConnectNo].Connected=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=0; //pozostaje sprz�g wirtualny
  Couplers[ConnectNo].CouplingFlag=0; //pozostaje sprz�g wirtualny
  return true;
 }
 else if (i>0)
 {//od��czamy w�e i reszt�, pozostaje sprz�g fizyczny, kt�ry wymaga doci�ni�cia (z wirtualnym nic)
  Couplers[ConnectNo].CouplingFlag&=ctrain_coupler;
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=Couplers[ConnectNo].CouplingFlag;
 }
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

bool __fastcall TMoverParameters::DirectionForward()
{
 if ((MainCtrlPosNo>0)&&(ActiveDir<1)&&(MainCtrlPos==0))
 {
  ++ActiveDir;
  DirAbsolute=ActiveDir*CabNo;
  SendCtrlToNext("Direction",ActiveDir,CabNo);
  return true;
 }
 else if ((ActiveDir==1)&&(MainCtrlPos==0)&&(TrainType==dt_EZT))
  return MinCurrentSwitch(true); //"wysoki rozruch" EN57
 return false;
};

// Nastawianie hamulc�w

void __fastcall TMoverParameters::BrakeLevelSet(double b)
{//ustawienie pozycji hamulca na warto�� (b) w zakresie od -2 do BrakeCtrlPosNo
 //jedyny dopuszczalny spos�b przestawienia hamulca zasadniczego
 if (fBrakeCtrlPos==b) return; //nie przelicza�, jak nie ma zmiany
 fBrakeCtrlPos=b;
 if (fBrakeCtrlPos<-2.0)
  fBrakeCtrlPos=-2.0; //odci�cie
 else
  if (fBrakeCtrlPos>double(BrakeCtrlPosNo))
   fBrakeCtrlPos=BrakeCtrlPosNo;
 int x=floor(fBrakeCtrlPos); //je�li odwo�ujemy si� do BrakeCtrlPos w po�rednich, to musi by� obci�te a nie zaokr�gone
 while ((x>BrakeCtrlPos)&&(BrakeCtrlPos<BrakeCtrlPosNo)) //je�li zwi�kszy�o si� o 1
  if (!T_MoverParameters::IncBrakeLevelOld()) break; //wyj�cie awaryjne
 while ((x<BrakeCtrlPos)&&(BrakeCtrlPos>=-1)) //je�li zmniejszy�o si� o 1
  if (!T_MoverParameters::DecBrakeLevelOld()) break;
 BrakePressureActual=BrakePressureTable[BrakeCtrlPos+2]; //skopiowanie pozycji
 if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje u�amki
  if (fBrakeCtrlPos>0.0)
  {//warto�ci po�rednie wyliczamy tylko dla hamowania
   double u=fBrakeCtrlPos-double(x); //u�amek ponad warto�� ca�kowit�
   if (u>0.0)
   {//wyliczamy warto�ci wa�one
    BrakePressureActual.PipePressureVal+=-u*BrakePressureActual.PipePressureVal+u*BrakePressureTable[BrakeCtrlPos+1+2].PipePressureVal;
    //BrakePressureActual.BrakePressureVal+=-u*BrakePressureActual.BrakePressureVal+u*BrakePressureTable[BrakeCtrlPos+1].BrakePressureVal;  //to chyba nie b�dzie tak dzia�a�, zw�aszcza w EN57
    BrakePressureActual.FlowSpeedVal+=-u*BrakePressureActual.FlowSpeedVal+u*BrakePressureTable[BrakeCtrlPos+1+2].FlowSpeedVal;
   }
  }
};

bool __fastcall TMoverParameters::BrakeLevelAdd(double b)
{//dodanie warto�ci (b) do pozycji hamulca (w tym ujemnej)
 //zwraca false, gdy po dodaniu by�o by poza zakresem
 BrakeLevelSet(fBrakeCtrlPos+b);
 return b>0.0?(fBrakeCtrlPos<BrakeCtrlPosNo):(BrakeCtrlPos>-1.0); //true, je�li mo�na kontynuowa�
};

bool __fastcall TMoverParameters::IncBrakeLevel()
{//nowa wersja na u�ytek AI, false gdy osi�gni�to pozycj� BrakeCtrlPosNo
 return BrakeLevelAdd(1.0);
};

bool __fastcall TMoverParameters::DecBrakeLevel()
{//nowa wersja na u�ytek AI, false gdy osi�gni�to pozycj� -1
 return BrakeLevelAdd(-1.0);
};

bool __fastcall TMoverParameters::ChangeCab(int direction)
{//zmiana kabiny i resetowanie ustawien
//var //b:byte;
//    c:boolean;
 if (abs(ActiveCab+direction)<2)
 {
//  if (ActiveCab+direction=0) then LastCab:=ActiveCab;
   ActiveCab=ActiveCab+direction;
   //ChangeCab=true;
   if ((BrakeSystem==Pneumatic)&&(BrakeCtrlPosNo>0))
   {
    BrakeLevelSet(-2); //BrakeCtrlPos=-2;
    LimPipePress=PipePress;
    ActFlowSpeed=0;
   }
   else
    //if (TrainType=dt_EZT) and (BrakeCtrlPosNo>0) then
    //  BrakeCtrlPos:=5; //z Megapacka
    //else
    BrakeLevelSet(0); //BrakeCtrlPos=0;
//   if not TestFlag(BrakeStatus,b_dmg) then
//    BrakeStatus:=b_off; //z Megapacka
   MainCtrlPos=0;
   ScndCtrlPos=0;
   if ((EngineType!=DieselEngine)&&(EngineType!=DieselElectric))
   {
    Mains=false;
    CompressorAllow=false;
    ConverterAllow=false;
   }
//   if (ActiveCab<>LastCab) and (ActiveCab<>0) then
//    begin
//     c:=PantFrontUp;
//     PantFrontUp:=PantRearUp;
//     PantRearUp:=c;
//    end; //yB: poszlo do wylacznika rozrzadu
  ActiveDir=0;
  DirAbsolute=0;
  return true;
 }
 return false;
};

bool __fastcall TMoverParameters::CurrentSwitch(int direction)
{//rozruch wysoki (true) albo niski (false)
 //Ra: przenios�em z Train.cpp, nie wiem czy ma to sens
 if (MaxCurrentSwitch(direction))
 {if (TrainType!=dt_EZT)
   return (MinCurrentSwitch(direction));
 }
 return false;
};
