//---------------------------------------------------------------------------

#ifndef MachajkaH
#define MachajkaH

#include "Train.h"

class TMachajka : public TTrain
{
  public:
    TSubModel *wajcha;
    //    double v;
    //    double m;
    //    double a;
    //    double f;
    //    bool enter;
    //    bool space;
    //    double wa,wv;
    __fastcall TMachajka();
    virtual __fastcall ~TMachajka();
    virtual bool __fastcall Init(TDynamicObject *NewDynamicObject);
    virtual void __fastcall OnKeyPress(int cKey);
    virtual bool __fastcall Update(double dt);
    virtual bool __fastcall UpdateMechPosition();
    virtual bool __fastcall Render();
};

//---------------------------------------------------------------------------
#endif
