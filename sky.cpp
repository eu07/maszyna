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

#ifdef EU07_USE_OLD_RENDERCODE
void TSky::Render( glm::vec3 const &Tint )
{
    if (mdCloud)
    { // jeśli jest model nieba
        // setup
        ::glEnable( GL_LIGHTING );
        GfxRenderer.Disable_Lights();
        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(Tint) );
        // render
        GfxRenderer.Render( mdCloud, nullptr, 100.0 );
        GfxRenderer.Render_Alpha( mdCloud, nullptr, 100.0 );
        // post-render cleanup
        GLfloat noambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, noambient );
        ::glEnable( GL_LIGHT0 ); // other lights will be enabled during lights update
        ::glDisable( GL_LIGHTING );
    }
};
#endif

//---------------------------------------------------------------------------
