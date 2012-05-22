//---------------------------------------------------------------------------

#ifndef LogsH
#define LogsH
#include <system.hpp> 

void __fastcall WriteConsoleOnly(const char *str, double value);
void __fastcall WriteConsoleOnly(const char *str);
void __fastcall WriteLog(const char *str, double value);
void __fastcall WriteLog(const char *str);
void __fastcall Error(const AnsiString &asMessage,bool box=true);
void __fastcall WriteLog(const AnsiString &str);
//---------------------------------------------------------------------------
#endif
