/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "sky.h"
#include "Globals.h"
#include "MdlMngr.h"

//---------------------------------------------------------------------------
//GLfloat lightPos[4] = {0.0f, 0.0f, 0.0f, 1.0f};

void TSky::Init() {

    if( ( Global::asSky != "1" )
     && ( Global::asSky != "0" ) ) {

        mdCloud = TModelsManager::GetModel( Global::asSky );
    }
};

//---------------------------------------------------------------------------
