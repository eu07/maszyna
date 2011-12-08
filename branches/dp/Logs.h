//---------------------------------------------------------------------------

#ifndef LogsH
#define LogsH

#include    "classes.hpp"

void __fastcall WriteConsoleOnly(char *str, double value);
void __fastcall WriteConsoleOnly(char *str);
void __fastcall WriteLog(AnsiString str, double value);
void __fastcall WriteLog(AnsiString str);

void __fastcall QueuedLog(AnsiString qstr);              // QUEUEDZIU 090806
void __fastcall QueuedDPLog(AnsiString qstr);            // QUEUEDZIU 090806
void __fastcall QueuedSwitchList(AnsiString qstr);       // QUEUEDZIU 090806
void __fastcall QueuedWarnings(AnsiString qstr);         // QUEUEDZIU 090806
void __fastcall QueuedTexturesList(AnsiString qstr);     // QUEUEDZIU 131210

__fastcall DEBUG(AnsiString PAR, int FID, int NUM);

//---------------------------------------------------------------------------
#endif
