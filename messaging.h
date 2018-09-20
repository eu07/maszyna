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
#include "winheaders.h"

class TDynamicObject;

namespace multiplayer {

struct DaneRozkaz { // struktura komunikacji z EU07.EXE
    int iSygn; // sygnatura 'EU07'
    int iComm; // rozkaz/status (kod ramki)
    union {
        float fPar[ 62 ];
        int iPar[ 62 ];
        char cString[ 248 ]; // upakowane stringi
    };
};

struct DaneRozkaz2 {              // struktura komunikacji z EU07.EXE
    int iSygn; // sygnatura 'EU07'
    int iComm; // rozkaz/status (kod ramki)
    union {
        float fPar[ 496 ];
        int iPar[ 496 ];
        char cString[ 1984 ]; // upakowane stringi
    };
};

void Navigate( std::string const &ClassName, UINT Msg, WPARAM wParam, LPARAM lParam );

void OnCommandGet( multiplayer::DaneRozkaz *pRozkaz );

void WyslijEvent( const std::string &e, const std::string &d );
void WyslijString( const std::string &t, int n );
void WyslijWolny( const std::string &t );
void WyslijNamiary( TDynamicObject const *Vehicle );
void WyslijParam( int nr, int fl );
void WyslijUszkodzenia( const std::string &t, char fl );
void WyslijObsadzone(); // -> skladanie wielu pojazdow    

} // multiplayer

//---------------------------------------------------------------------------
