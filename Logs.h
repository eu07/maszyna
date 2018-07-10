/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "uilayer.h"

enum class logtype : unsigned int {

    generic = ( 1 << 0 ),
    file = ( 1 << 1 ),
    model = ( 1 << 2 ),
    texture = ( 1 << 3 ),
	lua = ( 1 << 4 )
};

void WriteLog( const char *str, logtype const Type = logtype::generic );
void Error( const std::string &asMessage, bool box = false );
void Error( const char* &asMessage, bool box = false );
void ErrorLog( const std::string &str, logtype const Type = logtype::generic );
void WriteLog( const std::string &str, logtype const Type = logtype::generic );
void CommLog( const char *str );
void CommLog( const std::string &str );
