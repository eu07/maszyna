//---------------------------------------------------------------------------

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
#include <winuser.h>

class TWorld
{
 void __fastcall InOutKey();
 void __fastcall FollowView();
 void __fastcall DistantView();
public:
    bool __fastcall Init(HWND NhWnd, HDC hDC);
    HWND hWnd;
    GLvoid __fastcall glPrint(const char *fmt);
    void __fastcall OnKeyPress(int cKey);
//    void __fastcall UpdateWindow();
    void __fastcall OnMouseMove(double x, double y);
    void __fastcall OnCommandGet(DaneRozkaz *pRozkaz);
    bool __fastcall Update();
    void __fastcall TrainDelete(TDynamicObject *d=NULL);
    __fastcall TWorld();
    __fastcall ~TWorld();

    double Aspect;

private:
    byte lastmm; //ABu: zeby bylo wiadomo, czy zmienil sie czas
    AnsiString OutText1;
    AnsiString OutText2;
    AnsiString OutText3;
    AnsiString OutText4;
    void ShowHints();
    bool __fastcall Render();
    TCamera Camera;
    TGround Ground;
    TTrain *Train;
    TDynamicObject *pDynamicNearest;
    bool Paused;

    GLuint	base;
    GLuint      light; //swiatlo obadamy ;]

    TSky Clouds;
    TEvent *KeyEvents[10];
    int iCheckFPS; //kiedy znów sprawdziæ FPS, ¿eby wy³¹czaæ optymalizacji od razu do zera
public:
 void __fastcall ModifyTGA(const AnsiString &dir="");
 void __fastcall CreateE3D(const AnsiString &dir="",bool dyn=false);
};
//---------------------------------------------------------------------------
#endif
