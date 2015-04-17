/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

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
