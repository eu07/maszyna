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
  AnsiString asModel1, asModel2;
  asModel1 = Global::asSky;
  asModel2 = Global::asSkyH; // "skystars.t3d";

  mdCloud1=TModelsManager::GetModel(asModel1.c_str());
  mdCloud2=TModelsManager::GetModel(asModel2.c_str());
  Global::bRenderSky = true;
};


void __fastcall TSky::RenderA()
{
//glDisable(GL_LIGHT2);
glDisable(GL_FOG);

 if (mdCloud1)
 {//jeœli jest model nieba

  //glDisable(GL_DEPTH_TEST);
//-  glTranslatef(Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z);
//-  glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
#ifdef USE_VBO
  if (Global::bUseVBO)
  {//renderowanie z VBO
   glPushMatrix();
   mdCloud1->RaRender(100,0);
   mdCloud1->RaRenderAlpha(100,0);
  }
  else
#endif
  {//renderowanie z Display List
//  glColor4f(0.4, 0.2, 0.2, 0.5);
//  glClearColor (0.4, 0.2, 0.23, 1.0);
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//  glEnable(GL_BLEND);
//-  glEnable(GL_ALPHA_TEST);

//-    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_LIGHTING);

//glClearColor (0.5, 0.2, 0.13, 1.0);
glPushMatrix();
   glTranslatef(Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z);
//   glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
   glScalef(5, 5, 5);
//   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//   glEnable(GL_BLEND);
   if (mdCloud2) mdCloud2->Render(100,0);
   if (mdCloud2) mdCloud2->RenderAlpha(100,0);       // STARS

//   glScalef(1, 1, 1);
// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//   glBlendFunc(GL_ONE, GL_ONE_MINUS_DST_COLOR);                                    // NIE JEST TAK JAK BYM CHCLAL BO TROCHE ZOLTAWE ALE JEST OK :)
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);


   if (mdCloud1) mdCloud1->Render(100,0);
   if (mdCloud1) mdCloud1->RenderAlpha(100,0);     // CLOUDS
   glPopMatrix();
  }
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glDisable(GL_BLEND);
  glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);
 }
 glEnable(GL_FOG);
//   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER,0.04);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);                        // COBY ZPOWROTEM WLACZYC POPRAWNE RENDEROWANIE ALPHA, PO INFORMACJACH
//    glDepthFunc(GL_LEQUAL);
};



//---------------------------------------------------------------------------

#pragma package(smart_init)
