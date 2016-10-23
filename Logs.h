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
#include <string>

void WriteConsoleOnly(const char *str, double value);
void WriteConsoleOnly(const char *str, bool newline = true);
void WriteLog(const char *str, double value);
void WriteLog(const char *str, bool newline = true);
void Error(const AnsiString &asMessage, bool box = true);
void ErrorLog(const std::string &str, bool newline = true);
//void ErrorLog(const AnsiString &asMessage);
void WriteLog(const AnsiString &str, bool newline = true);
void WriteLog(const std::string &str, bool newline = true);
void CommLog(const char *str);
void CommLog(const std::string);
void CommLog(const AnsiString &str);
//---------------------------------------------------------------------------
#endif
