/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"

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
    // if the xOn model is missing, activate plain On instead
    inline void TurnxOnWithOnAsFallback()
    {
        if( pModelxOn != nullptr ) {
            bOn = false;
            bxOn = true;
            Update();
        }
        else {
            TurnOn();
        }
    };
    // if the xOn model is missing, activate plain Off instead
    inline void TurnxOnWithOffAsFallback()
    {
        if( pModelxOn != nullptr ) {
            bOn = false;
            bxOn = true;
            Update();
        }
        else {
            TurnOff();
        }
    };
    inline bool Active() const
    {
        return ( ( pModelOn != nullptr ) || ( pModelxOn != nullptr ) );
    };
    int GetStatus();
    void Init(std::string const &asName, TModel3d *pModel);
    void Load(cParser *Parser, TModel3d *pModel);
    //  bool Render();
};

//---------------------------------------------------------------------------
