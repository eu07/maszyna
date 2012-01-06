//---------------------------------------------------------------------------

#ifndef ScreenshotH
#define ScreenshotH

#include    "system.hpp"
#include    "classes.hpp"
#include "opengl/glew.h"
//#include <gl/gl.h>
//#include <gl/glu.h>
//#include "opengl/glut.h"


void __fastcall MakeScreenShot(HWND GL_HWND, HDC GL_HDC, HGLRC GL_hRC);
void __fastcall SaveScreen_JPG(HWND GL_HWND, HDC GL_HDC, HGLRC GL_hRC, int Q);


//---------------------------------------------------------------------------
#endif
