/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

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
