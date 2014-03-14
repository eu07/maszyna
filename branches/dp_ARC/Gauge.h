//---------------------------------------------------------------------------

#ifndef GaugeH
#define GaugeH

//#include "Classes.h"
#include "QueryParserComp.hpp"
//class Queryparsercomp::TQueryParserComp;
class TSubModel;
class TModel3d;
      
typedef enum
{//typ ruchu
 gt_Unknown, //na razie nie znany
 gt_Rotate,  //obrót
 gt_Move,    //przesuniêcie równoleg³e
 gt_Wiper    //obrót trzech kolejnych submodeli o ten sam k¹t (np. wycieraczka, drzwi harmonijkowe)
} TGaugeType;

class TGauge
{//animowany wskaŸnik, mog¹cy przyjmowaæ wiele stanów poœrednich
private:
 TGaugeType eType; //typ ruchu
 double fFriction; //hamowanie przy zli¿aniu siê do zadanej wartoœci
 double fDesiredValue; //wartoœæ docelowa
 double fValue; //wartoœæ obecna
 double fOffset; //wartoœæ pocz¹tkowa ("0")
 double fScale; //wartoœæ koñcowa ("1")
 double fStepSize; //nie u¿ywane
 int iChannel; //kana³ analogowej komunikacji zwrotnej
public:
 __fastcall TGauge();
 __fastcall ~TGauge();
 void __fastcall Clear();
 void __fastcall Init(TSubModel *NewSubModel,TGaugeType eNewTyp,double fNewScale=1,double fNewOffset=0,double fNewFriction=0,double fNewValue=0);
 void __fastcall Load(TQueryParserComp *Parser,TModel3d *md1,TModel3d *md2=NULL,double mul=1.0);
 void __fastcall PermIncValue(double fNewDesired);
 void __fastcall IncValue(double fNewDesired);
 void __fastcall DecValue(double fNewDesired);
 void __fastcall UpdateValue(double fNewDesired);
 void __fastcall PutValue(double fNewDesired);
 float GetValue() {return fValue;};
 void __fastcall Update();
 void __fastcall Render();
 void __fastcall Output(int i=-1) {iChannel=i;}; //ustawienie kana³u analogowego komunikacji zwrotnej
 TSubModel *SubModel; //McZapkie-310302: zeby mozna bylo sprawdzac czy zainicjowany poprawnie
};

//---------------------------------------------------------------------------
#endif

