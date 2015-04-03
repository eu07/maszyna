//---------------------------------------------------------------------------

#ifndef LogsH
#define LogsH
#include <system.hpp>

void WriteConsoleOnly(const char *str, double value);
void WriteConsoleOnly(const char *str);
void WriteLog(const char *str, double value);
void WriteLog(const char *str);
void Error(const AnsiString &asMessage, bool box = true);
void ErrorLog(const AnsiString &asMessage);
void WriteLog(const AnsiString &str);
//---------------------------------------------------------------------------
#endif
