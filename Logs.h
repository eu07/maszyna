/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <string>

enum logtype : unsigned int {

    generic = 0x1,
    file = 0x2,
    model = 0x4
};

void WriteLog( const char *str, logtype const Type = logtype::generic );
void Error( const std::string &asMessage, bool box = false );
void Error( const char* &asMessage, bool box = false );
void ErrorLog( const std::string &str, logtype const Type = logtype::generic );
void WriteLog( const std::string &str, logtype const Type = logtype::generic );
void CommLog( const char *str );
void CommLog( const std::string &str );
