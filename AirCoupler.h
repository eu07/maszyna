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

class AirCoupler
{
private:
    TSubModel *ModelOn, *ModelOff, *ModelxOn;
    bool On;
    bool xOn;
    void Update();

public:
    AirCoupler();
    ~AirCoupler();
    ///Reset members.
    void Clear();
    ///Looks for submodels.
    void Init(std::string const &asName, TModel3d *Model);
    ///Loads info about coupler.
    void Load(cParser *Parser, TModel3d *Model);
    int GetStatus();
    inline void TurnOn() ///Turns on straight coupler.
    {
        On = true;
        xOn = false;
        Update();
    };
    inline void TurnOff() ///Turns on disconnected coupler.
    {
        On = false;
        xOn = false;
        Update();
    };
    inline void TurnxOn() ///Turns on slanted coupler.
    {
        On = false;
        xOn = true;
        Update();
    };
};
