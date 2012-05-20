//---------------------------------------------------------------------------

#ifndef GaugeH
#define GaugeH

//#include "Classes.h"
#include "QueryParserComp.hpp"
//class Queryparsercomp::TQueryParserComp;
class TSubModel;
class TModel3d;
      
typedef enum { gt_Unknown, gt_Rotate, gt_Move } TGaugeType;

class TGauge
{
private:
 TGaugeType eType;
 double fFriction;
 double fDesiredValue;
 double fValue;
 double fOffset;
 double fScale;
 double fStepSize;
 int iChannel; //kana³ analogowej komunikacji zwrotnej
public:
  __fastcall TGauge();
  __fastcall ~TGauge();
  void __fastcall Clear();
  void __fastcall Init(TSubModel *NewSubModel, TGaugeType eNewTyp, double fNewScale=1, double fNewOffset=0, double fNewFriction=0, double fNewValue=0);
  void __fastcall Load(TQueryParserComp *Parser,TModel3d *md1,TModel3d *md2=NULL);
  void __fastcall PermIncValue(double fNewDesired);
  void __fastcall IncValue(double fNewDesired);
  void __fastcall DecValue(double fNewDesired);
  void __fastcall UpdateValue(double fNewDesired);
  void __fastcall PutValue(double fNewDesired);
  float GetValue(){return fValue;};
  void __fastcall Update();
  void __fastcall Render();
  void __fastcall Output(int i=-1) {iChannel=i;}; //ustawienie kana³u analogowego komunikacji zwrotnej
  TSubModel *SubModel; //McZapkie-310302: zeby mozna bylo sprawdzac czy zainicjowany poprawnie
};

//---------------------------------------------------------------------------
#endif

