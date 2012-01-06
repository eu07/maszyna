//---------------------------------------------------------------------------

#ifndef WorldH
#define WorldH


#include "Usefull.h"
#include "Classes.h"
#include "Texture.h"
#include "Camera.h"
#include "Ground.h"
#include "MdlMngr.h"
#include "Train.h"
#include "Globals.h"
#include "sky.h"
#include "sun.h"
#include "dynobj.h"
#include <winuser.h>

class TczastkiS
{
	private:
		float	
				cz,cs;		 //czas zycia, czas smierci czastki
							 //							   `						
	public:
		float	xp,xk,xa,   
                        yp,yk,ya,
                        zp,zk,za,
                        rgba[4],     //kolor + alfa
                        speed, rot;
				
		TczastkiS() {};
		~TczastkiS() {};
		void odswierz();
};



class TWorld
{
public:
    HWND hWnd;
    HWND xhWnd;
    HDC xhDC;

    bool __fastcall Init(HWND NhWnd, HDC hDC);
    __fastcall MOUSEHIT(int ID);
    __fastcall MOUSEMOV(int ID, int x, int y);

    GLvoid __fastcall glPrint(const char *fmt);
    void __fastcall OnKeyPress(int cKey);
//    void __fastcall UpdateWindow();
    void __fastcall OnMouseMove(double x, double y);
    void __fastcall OnMouseWheel(int zDelta);
    void __fastcall OnMouseLpush(double x, double y);
    void __fastcall OnMouseRpush(double x, double y);
    void __fastcall OnMouseMpush(double x, double y);
    void __fastcall OnCmd(AnsiString cmd, AnsiString PAR1, AnsiString PAR2, AnsiString PAR3, AnsiString PAR4, AnsiString PAR5);
    void __fastcall OnCommandGet(DaneRozkaz *pRozkaz);
    void __fastcall setdirecttable(AnsiString DIRECTION);
    bool __fastcall Update();

    bool __fastcall RenderFILTER(double alpha);
    bool __fastcall RenderEXITQUERY(double alpha);
    bool __fastcall RenderConsole(double speed, double dt);
    bool __fastcall RenderConsoleText();
    bool __fastcall RenderIRCEU07Text();
    bool __fastcall RenderDOC();
    bool __fastcall RenderMenu();
    bool __fastcall RenderMenuCheckBox(int w, int h, int x, int y, int ident, bool check, bool selected, AnsiString label);
    bool __fastcall RenderMenuInputBox(int w, int h, int x, int y, int ident, bool selected, AnsiString label);
    bool __fastcall RenderMenuButton(int w, int h, int x, int y, int ident, bool selected, AnsiString label);
    bool __fastcall RenderMenuPanel1(int w, int h, int x, int y, int ident, bool selected, AnsiString label, AnsiString backg);
    bool __fastcall RenderMenuListBox(int w, int h, int x, int y, int ident, bool selected, int items, AnsiString label);
    bool __fastcall RenderMenuListBoxItem(int w, int h, int x, int y, int ident, int selid, bool selected, int item, AnsiString label);
    bool __fastcall RenderLoader(int node, AnsiString text);
    bool __fastcall RenderFadeOff(int percent);
    bool __fastcall RenderINFOX(int node);
    bool __fastcall RenderInformation(int type);
    bool __fastcall RenderTUTORIAL(int type);
    bool __fastcall RenderFPS();
    bool __fastcall renderpanview(float trans, int frameheight, int pans);
    bool __fastcall renderfadeoff(float trans);
    bool __fastcall renderhitcolor(int r, int g, int b, int a);
    bool __fastcall rendercompass(float trans, int size, double angle);
    bool __fastcall rendertext(int x, int y, double scale, AnsiString astext);
    bool __fastcall renderpointerx(double sizepx, int sw, int sh);
    bool __fastcall menuinitctrls();
    bool __fastcall RenderCab();
    bool __fastcall RenderX();
    bool __fastcall RenderLokLights();
    bool __fastcall LOADLOADERCONFIG();
    void LosujPozycje(int i, vector3 pos);
    void UstawPoczatek(vector3 pos);
    void RuchObiektow(vector3 pos);
    __fastcall RysujBlyski(int tid, int type);
    __fastcall draw_snow(int tid, int type, vector3 pos);
        
    __fastcall TWorld();
    __fastcall ~TWorld();

    double Aspect;

// CONSOLE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  Queued - 161204

    TStringList *slCONSOLETEXT;
    TStringList *slCONSOLEEXEC;
    TStringList *slCONSOLEEXECUTED;

    static int iCLC;
    static int iCMDLISTINDEX;

    static bool bCMDEXECUTE;
    static bool bCONSOLEMODE;
    static bool bCONSOLESHOW;
    static bool bFIRSTEXECUTE;

    static float cps;
    static float cph;
    static float ctp;

    static AnsiString asCONSOLEPROMPT;
    static AnsiString asCONSOLETEXT;
    static AnsiString asLASTCMD;
    static AnsiString asLASTCMDALL;
    static AnsiString asCHARACTER;
    static AnsiString asGUISTRING;
    static AnsiString asEU07IRCTEXT;
    static AnsiString ascommand;
    static AnsiString ascommandpar1;
    static AnsiString ascommandpar2;

    void __fastcall CONSOLE__init();
    void __fastcall CONSOLE__addkey(int key);
    void __fastcall CONSOLE__execute();
    void __fastcall CONSOLE__closeconsole();
    void __fastcall CONSOLE__dump();
    void __fastcall CONSOLE__clear();
    void __fastcall CONSOLE__createstrl();
    void __fastcall CONSOLE__fastprevcmd();
    void __fastcall CONSOLE__fastnextcmd();

    TDynamicObject *Controlled; //pojazd, który prowadzimy  // PRZENIESIONE Z WORLD.CPP

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
    TSun SUN;
    TEvent *KeyEvents[10];
    int iCheckFPS; //kiedy znów sprawdziæ FPS, ¿eby wy³¹czaæ optymalizacji od razu do zera 
};
//---------------------------------------------------------------------------
#endif
