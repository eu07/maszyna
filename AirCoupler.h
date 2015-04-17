/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef AirCouplerH
#define AirCouplerH

#include "Model3d.h"
#include "QueryParserComp.hpp"

class TAirCoupler
{
  private:
    //    TButtonType eType;
    TSubModel *pModelOn, *pModelOff, *pModelxOn;
    bool bOn;
    bool bxOn;
    void Update();

  public:
    TAirCoupler();
    ~TAirCoupler();
    void Clear();
    inline void TurnOn()
    {
        bOn = true;
        bxOn = false;
        Update();
    };
    inline void TurnOff()
    {
        bOn = false;
        bxOn = false;
        Update();
    };
    inline void TurnxOn()
    {
        bOn = false;
        bxOn = true;
        Update();
    };
    //  inline bool Active() { if ((pModelOn)||(pModelOff)) return true; return false;};
    int GetStatus();
    void Init(AnsiString asName, TModel3d *pModel);
    void Load(TQueryParserComp *Parser, TModel3d *pModel);
    //  bool Render();
};

//---------------------------------------------------------------------------
#endif
