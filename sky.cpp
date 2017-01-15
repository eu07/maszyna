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
#include "Logs.h"
#include "Globals.h"
#include "MdlMngr.h"

//---------------------------------------------------------------------------
GLfloat lightPos[4] = {0.0f, 0.0f, 0.0f, 1.0f};

TSky::~TSky(){};

TSky::TSky(){};

void TSky::Init()
{
    WriteLog(Global::asSky.c_str());
    WriteLog("init");
    if ((Global::asSky != "1") && (Global::asSky != "0"))
        //   {
        mdCloud = TModelsManager::GetModel(Global::asSky.c_str());
    //   }
};

void TSky::Render()
{
    if (mdCloud)
    { // jeœli jest model nieba
        glPushMatrix();
        // glDisable(GL_DEPTH_TEST);
        glTranslatef(Global::pCameraPosition.x, Global::pCameraPosition.y,
                     Global::pCameraPosition.z);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        if (Global::bUseVBO)
        { // renderowanie z VBO
            mdCloud->RaRender(100, 0);
            mdCloud->RaRenderAlpha(100, 0);
        }
        else
        { // renderowanie z Display List
            mdCloud->Render(100, 0);
            mdCloud->RenderAlpha(100, 0);
        }
        // glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        // glEnable(GL_LIGHTING);
        glPopMatrix();
        glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos);
    }
};

//---------------------------------------------------------------------------
