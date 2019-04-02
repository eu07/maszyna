/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
MaSzyna EU07 locomotive simulator
Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/
/*
Authors:
MarcinW, McZapkie, Shaxbee, ABu, nbmx, youBy, Ra, winger, mamut, Q424,
Stele, firleju, szociu, hunter, ZiomalCl, OLI_EU and others
*/

#include "stdafx.h"

#include "application.h"
#include "Logs.h"
#include <cstdlib>

#ifdef _MSC_VER 
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup") 
#endif 


int main( int argc, char *argv[] )
{
    try
	{
        auto result { Application.init( argc, argv ) };
        if( result == 0 ) {
            result = Application.run();
        }
        Application.exit();
    }
    catch( std::bad_alloc const &Error )
	{
        ErrorLog( "Critical error, memory allocation failure: " + std::string( Error.what() ) );
    }
	std::_Exit(0); // skip destructors, there are ordering errors which causes segfaults
}
