//---------------------------------------------------------------------------

#ifndef screenH
#define screenH

#include "classes.hpp"
#include "Logs.h"
#include "Globals.h"

class TSCREEN
{
public:
void __fastcall FOVADD();
void __fastcall FOVREM();
void __fastcall FOVDEL();
void __fastcall FOVADDF();
void __fastcall FOVREMF();
void __fastcall RESADD(int mode);
void __fastcall RESREM();
void __fastcall SaveScreen_TGA();
void __fastcall SaveScreen_JPG(HWND GL_HWND, HDC GL_HDC, HGLRC GL_hRC, int Q);
bool __fastcall ScreenCapture(char *filename);
bool SaveBMPFile(char *filename, HBITMAP bitmap, HDC bitmapDC, int width, int height);

GLvoid __fastcall ReSizeGLSceneEx(double fov, GLsizei width, GLsizei height);
BOOL ChangeScreenResolution (int width, int height, int bitsPerPixel);



static double CFOV;
static bool ADJSCR;
static bool ADJFOV;


private:


};
//---------------------------------------------------------------------------
#endif
