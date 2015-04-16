/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef TractionPowerH
#define TractionPowerH
#include "parser.h" //Tolaris-010603

class TGroundNode;

class TTractionPowerSource
{
  private:
    double NominalVoltage;
    double VoltageFrequency;
    double InternalRes;
    double MaxOutputCurrent;
    double FastFuseTimeOut;
    int FastFuseRepetition;
    double SlowFuseTimeOut;
    bool Recuperation;

    double TotalCurrent;
    double TotalAdmitance;
    double TotalPreviousAdmitance;
    double OutputVoltage;
    bool FastFuse;
    bool SlowFuse;
    double FuseTimer;
    int FuseCounter;

  protected:
  public: // zmienne publiczne
    TTractionPowerSource *psNode[2]; // zasilanie na koñcach dla sekcji
    bool bSection; // czy jest sekcj¹
    TGroundNode *gMyNode; // Ra 2015-03: znowu prowizorka, aby mieæ nazwê do logowania
  public:
    // AnsiString asName;
    TTractionPowerSource();
    ~TTractionPowerSource();
    void Init(double u, double i);
    bool Load(cParser *parser);
    bool Render();
    bool Update(double dt);
    double CurrentGet(double res);
    void VoltageSet(double v) { NominalVoltage = v; };
    void PowerSet(TTractionPowerSource *ps);
};

//---------------------------------------------------------------------------
#endif
