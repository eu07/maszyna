//---------------------------------------------------------------------------

#ifndef MoverH
#define MoverH
//---------------------------------------------------------------------------
#include "mover.hpp"
// Ra: Niestety "_mover.hpp" siê nieprawid³owo generuje - przek³ad sobie TCoupling na sam koniec.
// Przy wszelki poprawkach w "_mover.pas" trzeba skopiowaæ rêcznie "_mover.hpp" do "mover.hpp" i poprawiæ b³êdy!
// Tak a¿ do wydzielnia TCoupling z Pascala do C++...
// Docelowo obs³ugê sprzêgów (³¹czenie, roz³¹czanie, obliczanie odleg³oœci, przesy³ komend)
//   trzeba przenieœæ na poziom DynObj.cpp.
// Obs³ugê silniników te¿ trzeba wydzieliæ do osobnego modu³u, bo ka¿dy osobno mo¿e mieæ poœlizg.
#include "dumb3d.h"

class TMoverParameters : public T_MoverParameters
{//Ra: wrapper na kod pascalowy, przejmuj¹cy jego funkcje
public:
 Math3D::vector3 vCoulpler[2]; //powtórzenie wspó³rzêdnych sprzêgów z DynObj :/
private:
 double __fastcall CouplerDist(Byte Coupler);
public:
 __fastcall TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
  int LoadInitial,AnsiString LoadTypeInitial,int Cab);
 //obs³uga sprzêgów
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType);
 bool __fastcall Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType);
 bool __fastcall DettachDistance(Byte ConnectNo);
 bool __fastcall Dettach(Byte ConnectNo);
 void __fastcall SetCoupleDist();
};

#endif
