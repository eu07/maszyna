/*========================================================================================

  Name:   ARB_multisample.cpp
	Author: Colt "MainRoach" McAnlis
	Date:   4/29/04
	Desc:   This file contains the context to load a WGL extension from a string
		    As well as collect the sample format available based upon the graphics card.

========================================================================================*/

#include "stdafx.h"
#include "arb_multisample.h"
#include "gl/glew.h"
#include "gl/wglew.h"

// Declairations We'll Use
#define WGL_SAMPLE_BUFFERS_ARB		 0x2041
#define WGL_SAMPLES_ARB			     0x2042

bool	arbMultisampleSupported	= false;
int		arbMultisampleFormat	= 0;

// WGLisExtensionSupported: This Is A Form Of The Extension For WGL
bool WGLisExtensionSupported(const char *extension)
{
	const size_t extlen = strlen(extension);
	const char *supported = NULL;

	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());

	// If That Failed, Try Standard Opengl Extensions String
	if (supported == NULL)
		supported = (char*)glGetString(GL_EXTENSIONS);

	// If That Failed Too, Must Be No Extensions Supported
	if (supported == NULL)
		return false;

	// Begin Examination At Start Of String, Increment By 1 On False Match
	for (const char* p = supported; ; p++)
	{
		// Advance p Up To The Next Possible Match
		p = strstr(p, extension);

		if (p == NULL)
			return false;															// No Match

		// Make Sure That Match Is At The Start Of The String Or That
		// The Previous Char Is A Space, Or Else We Could Accidentally
		// Match "wglFunkywglExtension" With "wglExtension"

		// Also, Make Sure That The Following Character Is Space Or NULL
		// Or Else "wglExtensionTwo" Might Match "wglExtension"
		if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return true;															// Match
	}
}

int InitMultisample(HINSTANCE hInstance,HWND hWnd,PIXELFORMATDESCRIPTOR pfd,int mode)
{//used to query the multisample frequencies
 arbMultisampleSupported=false;
 //see if the string exists in WGL!
 if (!WGLisExtensionSupported("WGL_ARB_multisample"))
  return 0;
 //get our pixel format
 PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB=(PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
 if (!wglChoosePixelFormatARB)
  return 0;
 //get our current device context
 HDC hDC=GetDC(hWnd);
 int pixelFormat;
 int valid;
 UINT numFormats;
 float fAttributes[]={0,0};
 // These attributes are the bits we want to test for in our sample
 // everything is pretty standard, the only one we want to
 // really focus on is the SAMPLE BUFFERS ARB and WGL SAMPLES
 // these two are going to do the main testing for whether or not
 // we support multisampling on this hardware.
 int iAttributes[]=
 {
  WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
  WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
  WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
  WGL_COLOR_BITS_ARB,24,
  WGL_ALPHA_BITS_ARB,8,
  WGL_DEPTH_BITS_ARB,16,
  WGL_STENCIL_BITS_ARB,0,
  WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
  WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
  WGL_SAMPLES_ARB,mode,
  0,0
 };
/*
 //first we check to see if we can get a pixel format for 4 samples
 valid=wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
 if (valid&&(numFormats>=1))
 {//if we returned true, and our format count is greater than 1
  arbMultisampleSupported=true;
  arbMultisampleFormat=pixelFormat;
  return arbMultisampleSupported;
 }
*/
 while (iAttributes[19]>1)
 {//our pixel format with (mode) samples failed, test for less samples
  valid=wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
  if (valid&&(numFormats>=1))
  {//if we returned true, and our format count is greater than 1
   arbMultisampleSupported=true;
   arbMultisampleFormat=pixelFormat;
   return iAttributes[19]; //return number of samples
  }
  iAttributes[19]>>=1;
 }
 return 0;
}
