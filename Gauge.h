//---------------------------------------------------------------------------

#ifndef GaugeH
#define GaugeH

#include "Model3d.h"

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

public:
  __fastcall TGauge();
  __fastcall ~TGauge();
  __fastcall Clear();
  bool __fastcall Init(TSubModel *NewSubModel, TGaugeType eNewTyp, double fNewScale=1, double fNewOffset=0, double fNewFriction=0, double fNewValue=0);
  bool __fastcall Load(TQueryParserComp *Parser, TModel3d *mdKabina);
  bool __fastcall PermIncValue(double fNewDesired);
  bool __fastcall IncValue(double fNewDesired);
  bool __fastcall DecValue(double fNewDesired);
  bool __fastcall UpdateValue(double fNewDesired);
  bool __fastcall PutValue(double fNewDesired);
  float GetValue(){return fValue;};
  bool __fastcall Update();
  bool __fastcall Render();
  TSubModel *SubModel; //McZapkie-310302: zeby mozna bylo sprawdzac czy zainicjowany poprawnie  
};

//---------------------------------------------------------------------------
#endif
