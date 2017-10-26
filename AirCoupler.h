/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Model3d.h"
#include "parser.h"

class TAirCoupler
{
private:
    // TButtonType eType; //?
    TSubModel *ModelOn, *ModelOff, *ModelxOn;
    bool On;
    bool xOn;
    void Update();

public:
    TAirCoupler();
    ~TAirCoupler();

    void Clear();
    void Init(std::string const &asName, TModel3d *Model);
    void Load(cParser *Parser, TModel3d *Model);
    //  inline bool Active() { if ((ModelOn)||(ModelOff)) return true; return false;};
    int GetStatus();
    inline void TurnOn()
    {
        On = true;
        xOn = false;
        Update();
    };
    inline void TurnOff()
    {
        On = false;
        xOn = false;
        Update();
    };
    inline void TurnxOn()
    {
        On = false;
        xOn = true;
        Update();
    };
};
