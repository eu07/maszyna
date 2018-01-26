/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "classes.h"
#include "scenenode.h"
#include "names.h"

class TTractionPowerSource : public editor::basic_node {

  private:
    double NominalVoltage = 0.0;
    double VoltageFrequency = 0.0;
    double InternalRes = 0.2;
    double MaxOutputCurrent = 0.0;
    double FastFuseTimeOut = 1.0;
    int FastFuseRepetition = 3;
    double SlowFuseTimeOut = 60.0;
    bool Recuperation = false;

    double TotalCurrent = 0.0;
    double TotalAdmitance = 1e-10; // 10Mom - jakaś tam upływność
    double TotalPreviousAdmitance = 1e-10; // zero jest szkodliwe
    double OutputVoltage = 0.0;
    bool FastFuse = false;
    bool SlowFuse = false;
    double FuseTimer = 0.0;
    int FuseCounter = 0;

public:
    // zmienne publiczne
    TTractionPowerSource *psNode[ 2 ] = { nullptr, nullptr }; // zasilanie na końcach dla sekcji
    bool bSection = false; // czy jest sekcją

    TTractionPowerSource( scene::node_data const &Nodedata );

    void Init(double const u, double const i);
    bool Load(cParser *parser);
    bool Update(double dt);
    double CurrentGet(double res);
    void VoltageSet(double const v) {
        NominalVoltage = v; };
    void PowerSet(TTractionPowerSource *ps);
};



// collection of generators for power grid present in the scene
class powergridsource_table : public basic_table<TTractionPowerSource> {

public:
    // legacy method, calculates changes in simulation state over specified time
    void
        update( double const Deltatime );
};

//---------------------------------------------------------------------------
