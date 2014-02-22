//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "sky.h"
#include "Globals.h"

//---------------------------------------------------------------------------
GLfloat  lightPos[4] = { 0.0f,  0.0f, 0.0f, 1.0f };

__fastcall TSky::~TSky()
{
};

__fastcall TSky::TSky()
{
};


void __fastcall TSky::Init()
{
  WriteLog(Global::asSky.c_str());
  WriteLog("init");
  AnsiString asModel;
  asModel=Global::asSky;
  if ((asModel!="1") && (asModel!="0"))
//   {
    mdCloud=TModelsManager::GetModel(asModel.c_str());
//   }
};


void __fastcall TSky::Render()
{
 if (mdCloud)
 {//jeœli jest model nieba
  glPushMatrix();
  //glDisable(GL_DEPTH_TEST);
  glTranslatef(Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z);
  glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
  if (Global::bUseVBO)
  {//renderowanie z VBO
   mdCloud->RaRender(100,0);
   mdCloud->RaRenderAlpha(100,0);
  }
  else
  {//renderowanie z Display List
   mdCloud->Render(100,0);
   mdCloud->RenderAlpha(100,0);
  }
  //glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT);
  //glEnable(GL_LIGHTING);
  glPopMatrix();
  glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);
 }
};



//---------------------------------------------------------------------------

#pragma package(smart_init)
