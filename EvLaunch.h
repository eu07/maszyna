/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef EvLaunchH
#define EvLaunchH

#include "Classes.h"
#include <string>

using namespace std;

class TEventLauncher
{
  private:
    int iKey;
    double DeltaTime;
    double UpdatedTime;
    double fVal1;
    double fVal2;
    char *szText;
    int iHour, iMinute; // minuta uruchomienia
  public:
    double dRadius;
    string asEvent1Name;
    string asEvent2Name;
    string asMemCellName;
    TEvent *Event1;
    TEvent *Event2;
    TMemCell *MemCell;
    int iCheckMask;
    TEventLauncher();
    ~TEventLauncher();
    void Init();
    bool Load(cParser *parser);
    bool Render();
    bool IsGlobal();
};

//---------------------------------------------------------------------------
#endif
