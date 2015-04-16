/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef WorldH
#define WorldH

#include "Usefull.h"
#include "Classes.h"
#include "Texture.h"
#include "Camera.h"
#include "Ground.h"
#include "MdlMngr.h"
#include "Globals.h"
#include "sky.h"
//#include <winuser.h>

class TWorld
{
    void InOutKey();
    void FollowView(bool wycisz = true);
    void DistantView();

  public:
    bool Init(HWND NhWnd, HDC hDC);
    HWND hWnd;
    GLvoid glPrint(const char *fmt);
    void OnKeyDown(int cKey);
    void OnKeyUp(int cKey);
    // void UpdateWindow();
    void OnMouseMove(double x, double y);
    void OnCommandGet(DaneRozkaz *pRozkaz);
    bool Update();
    void TrainDelete(TDynamicObject *d = NULL);
    TWorld();
    ~TWorld();
    // double Aspect;
  private:
    AnsiString OutText1; // teksty na ekranie
    AnsiString OutText2;
    AnsiString OutText3;
    AnsiString OutText4;
    void ShowHints();
    bool Render();
    TCamera Camera;
    TGround Ground;
    TTrain *Train;
    TDynamicObject *pDynamicNearest;
    bool Paused;
    GLuint base; // numer DL dla znaków w napisach
    GLuint light; // numer tekstury dla smugi
    TSky Clouds;
    TEvent *KeyEvents[10]; // eventy wyzwalane z klawiaury
    TMoverParameters *mvControlled; // wskaŸnik na cz³on silnikowy, do wyœwietlania jego parametrów
    int iCheckFPS; // kiedy znów sprawdziæ FPS, ¿eby wy³¹czaæ optymalizacji od razu do zera
    double fTime50Hz; // bufor czasu dla komunikacji z PoKeys
    double fTimeBuffer; // bufor czasu aktualizacji dla sta³ego kroku fizyki
    double fMaxDt; //[s] krok czasowy fizyki (0.01 dla normalnych warunków)
    int iPause; // wykrywanie zmian w zapauzowaniu
  public:
    void ModifyTGA(const AnsiString &dir = "");
    void CreateE3D(const AnsiString &dir = "", bool dyn = false);
    void CabChange(TDynamicObject *old, TDynamicObject *now);
};
//---------------------------------------------------------------------------
#endif
