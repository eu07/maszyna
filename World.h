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
//#include <winuser.h>

class TWorld
{
 void __fastcall InOutKey();
 void __fastcall FollowView();
 void __fastcall DistantView();
public:
 bool __fastcall Init(HWND NhWnd, HDC hDC);
 HWND hWnd;
 GLvoid __fastcall glPrint(const char *fmt);
 void __fastcall OnKeyDown(int cKey);
 void __fastcall OnKeyUp(int cKey);
 //void __fastcall UpdateWindow();
 void __fastcall OnMouseMove(double x, double y);
 void __fastcall OnCommandGet(DaneRozkaz *pRozkaz);
 bool __fastcall Update();
 void __fastcall TrainDelete(TDynamicObject *d=NULL);
 __fastcall TWorld();
 __fastcall ~TWorld();
 //double Aspect;
private:
 AnsiString OutText1; //teksty na ekranie
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
 GLuint	base; //numer DL dla znaków w napisach
 GLuint light; //numer tekstury dla smugi
 TSky Clouds;
 TEvent *KeyEvents[10]; //eventy wyzwalane z klawiaury
 TMoverParameters *mvControlled; //wskaŸnik na cz³on silnikowy, do wyœwietlania jego parametrów
 int iCheckFPS; //kiedy znów sprawdziæ FPS, ¿eby wy³¹czaæ optymalizacji od razu do zera
 double fTimeBuffer; //bufor czasu aktualizacji dla sta³ego kroku fizyki
 double fMaxDt; //[s] krok czasowy fizyki (0.01 dla normalnych warunków)
public:
 void __fastcall ModifyTGA(const AnsiString &dir="");
 void __fastcall CreateE3D(const AnsiString &dir="",bool dyn=false);
 void __fastcall CabChange(TDynamicObject *old,TDynamicObject *now);
};
//---------------------------------------------------------------------------
#endif
