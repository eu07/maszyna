//---------------------------------------------------------------------------

#ifndef MoverH
#define MoverH
//---------------------------------------------------------------------------
#include "Mover.hpp"
// Ra: Niestety "_mover.hpp" siê nieprawid³owo generuje - przek³ada sobie TCoupling na sam koniec.
// Przy wszelkich poprawkach w "_mover.pas" trzeba skopiowaæ rêcznie "_mover.hpp" do "mover.hpp" i
// poprawiæ b³êdy! Tak a¿ do wydzielnia TCoupling z Pascala do C++...
// Docelowo obs³ugê sprzêgów (³¹czenie, roz³¹czanie, obliczanie odleg³oœci, przesy³ komend)
// trzeba przenieœæ na poziom DynObj.cpp.
// Obs³ugê silniników te¿ trzeba wydzieliæ do osobnego modu³u, bo ka¿dy osobno mo¿e mieæ poœlizg.
#include "dumb3d.h"
using namespace Math3D;

enum TProblem //lista problemów taboru, które uniemo¿liwiaj¹ jazdê
{//flagi bitowe
 pr_Hamuje=1, //pojazd ma za³¹czony hamulec lub zatarte osie
 pr_Pantografy=2, //pojazd wymaga napompowania pantografów
 pr_Ostatni=0x80000000 //ostatnia flaga bitowa
};

class TMoverParameters : public T_MoverParameters
{//Ra: wrapper na kod pascalowy, przejmuj¹cy jego funkcje
public:
 vector3 vCoulpler[2]; //powtórzenie wspó³rzêdnych sprzêgów z DynObj :/
 vector3 DimHalf; //po³owy rozmiarów do obliczeñ geometrycznych
 //int WarningSignal; //0: nie trabi, 1,2: trabi syren¹ o podanym numerze
 unsigned char WarningSignal; //tymczasowo 8bit, ze wzglêdu na funkcje w MTools
 double fBrakeCtrlPos; //p³ynna nastawa hamulca zespolonego
 bool bPantKurek3; //kurek trójdrogowy (pantografu): true=po³¹czenie z ZG, false=po³¹czenie z ma³¹ sprê¿ark¹
 int iProblem; //flagi problemów z taborem, aby AI nie musia³o porównywaæ; 0=mo¿e jechaæ
 int iLights[2]; //bity zapalonych œwiate³ tutaj, ¿eby da³o siê liczyæ pobór pr¹du 
private:
 double __fastcall CouplerDist(Byte Coupler);
public:
 __fastcall TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
  int LoadInitial,AnsiString LoadTypeInitial,int Cab);
 //obs³uga sprzêgów
 double __fastcall Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2);
 double __fastcall Distance(const vector3 &Loc1,const vector3 &Loc2,const vector3 &Dim1,const vector3 &Dim2);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType,bool Forced=false);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType,bool Forced=false);
 int __fastcall DettachStatus(Byte ConnectNo);
 bool __fastcall Dettach(Byte ConnectNo);
 void __fastcall SetCoupleDist();
 bool __fastcall DirectionForward();
 void __fastcall BrakeLevelSet(double b);
 bool __fastcall BrakeLevelAdd(double b);
 bool __fastcall IncBrakeLevel(); //wersja na u¿ytek AI
 bool __fastcall DecBrakeLevel();
 bool __fastcall ChangeCab(int direction);
 bool __fastcall CurrentSwitch(int direction);
 void __fastcall UpdatePantVolume(double dt);
 void __fastcall UpdateBatteryVoltage(double dt);
 double __fastcall ComputeMovement
 (double dt,double dt1,
  const TTrackShape &Shape,TTrackParam &Track,TTractionParam &ElectricTraction,
  const TLocation &NewLoc,TRotation &NewRot
 );
 double __fastcall FastComputeMovement
 (double dt,
  const TTrackShape &Shape,TTrackParam &Track,
  const	TLocation &NewLoc,TRotation &NewRot
 );
};

#endif
