//---------------------------------------------------------------------------

#ifndef MoverH
#define MoverH
//---------------------------------------------------------------------------
#include "mover.hpp"
// Ra: Niestety "_mover.hpp" si� nieprawid�owo generuje - przek�ada sobie TCoupling na sam koniec.
// Przy wszelkich poprawkach w "_mover.pas" trzeba skopiowa� r�cznie "_mover.hpp" do "mover.hpp" i
// poprawi� b��dy! Tak a� do wydzielnia TCoupling z Pascala do C++...
// Docelowo obs�ug� sprz�g�w (��czenie, roz��czanie, obliczanie odleg�o�ci, przesy� komend)
// trzeba przenie�� na poziom DynObj.cpp.
// Obs�ug� silninik�w te� trzeba wydzieli� do osobnego modu�u, bo ka�dy osobno mo�e mie� po�lizg.
#include "dumb3d.h"
using namespace Math3D;

class TMoverParameters : public T_MoverParameters
{//Ra: wrapper na kod pascalowy, przejmuj�cy jego funkcje
public:
 vector3 vCoulpler[2]; //powt�rzenie wsp�rz�dnych sprz�g�w z DynObj :/
 vector3 DimHalf; //po�owy rozmiar�w do oblicze� geometrycznych
 int WarningSignal; //0: nie trabi, 1,2: trabi syren� o podanym numerze
private:
 double __fastcall CouplerDist(Byte Coupler);
public:
 __fastcall TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
  int LoadInitial,AnsiString LoadTypeInitial,int Cab);
 //obs�uga sprz�g�w
 double __fastcall Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2);
 double __fastcall Distance(const vector3 &Loc1,const vector3 &Loc2,const vector3 &Dim1,const vector3 &Dim2);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType,bool Forced=false);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType,bool Forced=false);
 int __fastcall DettachStatus(Byte ConnectNo);
 bool __fastcall Dettach(Byte ConnectNo);
 void __fastcall SetCoupleDist();
};

#endif
