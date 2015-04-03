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
    TMachajka();
    virtual ~TMachajka();
    virtual bool Init(TDynamicObject *NewDynamicObject);
    virtual void OnKeyPress(int cKey);
    virtual bool Update(double dt);
    virtual bool UpdateMechPosition();
    virtual bool Render();
};

//---------------------------------------------------------------------------
#endif
