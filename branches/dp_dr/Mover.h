//---------------------------------------------------------------------------

#ifndef MoverH
#define MoverH
//---------------------------------------------------------------------------
#include "mover.hpp"
// Ra: Niestety "_mover.hpp" si� nieprawid�owo generuje - przek�ad sobie TCoupling na sam koniec.
// Przy wszelki poprawkach w "_mover.pas" trzeba skopiowa� r�cznie "_mover.hpp" do "mover.hpp" i poprawi� b��dy!
// Tak a� do wydzielnia TCoupling z Pascala do C++...
// Docelowo obs�ug� sprz�g�w (��czenie, roz��czanie, obliczanie odleg�o�ci, przesy� komend)
//   trzeba przenie�� na poziom DynObj.cpp.
// Obs�ug� silninik�w te� trzeba wydzieli� do osobnego modu�u, bo ka�dy osobno mo�e mie� po�lizg.
#include "dumb3d.h"

class TMoverParameters : public T_MoverParameters
{//Ra: wrapper na kod pascalowy, przejmuj�cy jego funkcje
public:
 Math3D::vector3 vCoulpler[2]; //powt�rzenie wsp�rz�dnych sprz�g�w z DynObj :/
private:
 double __fastcall CouplerDist(Byte Coupler);
public:
 __fastcall TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
  int LoadInitial,AnsiString LoadTypeInitial,int Cab);
 //obs�uga sprz�g�w
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType);
 bool __fastcall DettachDistance(Byte ConnectNo);
 bool __fastcall Dettach(Byte ConnectNo);
 void __fastcall SetCoupleDist();
};

#endif
