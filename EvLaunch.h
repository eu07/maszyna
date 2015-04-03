//---------------------------------------------------------------------------

#ifndef EvLaunchH
#define EvLaunchH

#include "Classes.h"

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
    AnsiString asEvent1Name;
    AnsiString asEvent2Name;
    AnsiString asMemCellName;
    TEvent *Event1;
    TEvent *Event2;
    TMemCell *MemCell;
    int iCheckMask;
    __fastcall TEventLauncher();
    __fastcall ~TEventLauncher();
    void __fastcall Init();
    bool __fastcall Load(cParser *parser);
    bool __fastcall Render();
    bool __fastcall IsGlobal();
};

//---------------------------------------------------------------------------
#endif
