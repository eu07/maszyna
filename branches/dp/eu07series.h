//---------------------------------------------------------------------------

#ifndef eu07seriesH
#define eu07seriesH

#include "Train.h"

class TEu07:public TTrain
{
public:

    __fastcall TEu07();
    virtual __fastcall ~TEu07();
    virtual bool __fastcall Init(TDynamicObject *NewDynamicObject);
    virtual void __fastcall OnKeyPress(int cKey);
    virtual bool __fastcall Update(double dt);
    virtual bool __fastcall UpdateMechPosition();
    virtual bool __fastcall Render();
};


//---------------------------------------------------------------------------
#endif
