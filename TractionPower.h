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
    TGroundNode const *gMyNode = nullptr; // wskaźnik na węzeł rodzica

  protected:
  public: // zmienne publiczne
    TTractionPowerSource *psNode[2]; // zasilanie na końcach dla sekcji
    bool bSection = false; // czy jest sekcją
  public:
    // AnsiString asName;
    TTractionPowerSource(TGroundNode const *node);
    ~TTractionPowerSource();
    void Init(double const u, double const i);
    bool Load(cParser *parser);
    bool Render();
    bool Update(double dt);
    double CurrentGet(double res);
    void VoltageSet(double const v)
    {
        NominalVoltage = v;
    };
    void PowerSet(TTractionPowerSource *ps);
};

//---------------------------------------------------------------------------
#endif
