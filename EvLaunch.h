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

#include <string>
#include "Classes.h"

class TEventLauncher
{
  private:
    int iKey;
    double DeltaTime;
    double UpdatedTime;
    double fVal1;
    double fVal2;
#ifdef EU07_USE_OLD_TEVENTLAUNCHER_TEXT_ARRAY
    char * szText;
#else
    std::string szText;
#endif
    int iHour, iMinute; // minuta uruchomienia
  public:
    double dRadius;
    std::string asEvent1Name;
    std::string asEvent2Name;
    std::string asMemCellName;
    TEvent *Event1;
    TEvent *Event2;
    TMemCell *MemCell;
    int iCheckMask;
    TEventLauncher();
#ifdef EU07_USE_OLD_TEVENTLAUNCHER_TEXT_ARRAY
    ~TEventLauncher();
#endif
    void Init();
    bool Load(cParser *parser);
    bool Render();
    bool IsGlobal();
};

//---------------------------------------------------------------------------
#endif
