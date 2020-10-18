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

    // if the xOn model is missing, activate plain On instead
    inline void TurnxOnWithOnAsFallback()
    {
        if (ModelxOn != nullptr) {
            On = false;
            xOn = true;
            Update();
        }
        else {
            TurnOn();
        }
    };
    // if the xOn model is missing, activate plain Off instead
    inline void TurnxOnWithOffAsFallback()
    {
        if (ModelxOn != nullptr) {
            On = false;
            xOn = true;
            Update();
        }
        else {
            TurnOff();
        }
    };
    inline bool Active() const
    {
        return ( ( ModelOn != nullptr ) || ( ModelxOn != nullptr ) );
    };
};

