//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Logs.h"
#include "Globals.h"

#include "screen.h"
#include <stdio.h>



double TSCREEN::CFOV = 45.0;
bool TSCREEN::ADJFOV = false;
bool TSCREEN::ADJSCR = false;
int x1 = 0;
int y1 = 0;
int x2 = GetSystemMetrics(SM_CXSCREEN);
int y2 = GetSystemMetrics(SM_CYSCREEN);

AnsiString FDATE, SSHOTFILETGA, SSHOTFILEBMP;

inline int GetFilePointer(HANDLE FileHandle){
return SetFilePointer(FileHandle, 0, 0, FILE_CURRENT);
}

bool TSCREEN::SaveBMPFile(char *filename, HBITMAP bitmap, HDC bitmapDC, int width, int height){
bool Success=0;
HDC SurfDC=NULL;
HBITMAP OffscrBmp=NULL;
HDC OffscrDC=NULL;
LPBITMAPINFO lpbi=NULL;
LPVOID lpvBits=NULL;
HANDLE BmpFile=INVALID_HANDLE_VALUE;
BITMAPFILEHEADER bmfh;

if ((OffscrBmp = CreateCompatibleBitmap(bitmapDC, width, height)) == NULL)
return 0;
if ((OffscrDC = CreateCompatibleDC(bitmapDC)) == NULL)
return 0;
HBITMAP OldBmp = (HBITMAP)SelectObject(OffscrDC, OffscrBmp);
BitBlt(OffscrDC, 0, 0, width, height, bitmapDC, 0, 0, SRCCOPY);
if ((lpbi = (LPBITMAPINFO)(new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)])) == NULL)
return 0;
ZeroMemory(&lpbi->bmiHeader, sizeof(BITMAPINFOHEADER));
lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
SelectObject(OffscrDC, OldBmp);
if (!GetDIBits(OffscrDC, OffscrBmp, 0, height, NULL, lpbi, DIB_RGB_COLORS))
return 0;
if ((lpvBits = new char[lpbi->bmiHeader.biSizeImage]) == NULL)
return 0;
if (!GetDIBits(OffscrDC, OffscrBmp, 0, height, lpvBits, lpbi, DIB_RGB_COLORS))
return 0;
if ((BmpFile = CreateFile(filename,
GENERIC_WRITE,
0, NULL,
CREATE_ALWAYS,
FILE_ATTRIBUTE_NORMAL,
NULL)) == INVALID_HANDLE_VALUE)
return 0;
DWORD Written;
bmfh.bfType = 19778;
bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
if (!WriteFile(BmpFile, &bmfh, sizeof(bmfh), &Written, NULL))
return 0;
if (Written < sizeof(bmfh))
return 0;
if (!WriteFile(BmpFile, &lpbi->bmiHeader, sizeof(BITMAPINFOHEADER), &Written, NULL))
return 0;
if (Written < sizeof(BITMAPINFOHEADER))
return 0;
int PalEntries;
if (lpbi->bmiHeader.biCompression == BI_BITFIELDS)
PalEntries = 3;
else PalEntries = (lpbi->bmiHeader.biBitCount <= 8) ?
(int)(1 << lpbi->bmiHeader.biBitCount) : 0;
if(lpbi->bmiHeader.biClrUsed)
PalEntries = lpbi->bmiHeader.biClrUsed;
if(PalEntries){
if (!WriteFile(BmpFile, &lpbi->bmiColors, PalEntries * sizeof(RGBQUAD), &Written, NULL))
return 0;
if (Written < PalEntries * sizeof(RGBQUAD))
return 0;
}
bmfh.bfOffBits = GetFilePointer(BmpFile);
if (!WriteFile(BmpFile, lpvBits, lpbi->bmiHeader.biSizeImage, &Written, NULL))
   {
    CloseHandle(BmpFile);
    return 0;
   }
if (Written < lpbi->bmiHeader.biSizeImage)
    {
     CloseHandle(BmpFile);
     return 0;
    }
bmfh.bfSize = GetFilePointer(BmpFile);
SetFilePointer(BmpFile, 0, 0, FILE_BEGIN);
if (!WriteFile(BmpFile, &bmfh, sizeof(bmfh), &Written, NULL))
    {
     CloseHandle(BmpFile);
     return 0;
    }
if (Written < sizeof(bmfh))
return 0;
CloseHandle(BmpFile);
return 1;
}

bool __fastcall TSCREEN::ScreenCapture(char *filename){

int x = 0;
int y = 0;
int width = Global::iWindowWidth;
int height = Global::iWindowHeight;

WriteLog("SAVING SCREEN TO BMP");

FDATE = FormatDateTime("hhmmss-ddmmyy", Now());

SSHOTFILEBMP = Global::asSSHOTDIR + FDATE + ".bmp";

WriteLog(SSHOTFILEBMP);

HDC hDc = CreateCompatibleDC(0);
HBITMAP hBmp = CreateCompatibleBitmap(GetDC(0), width, height);
SelectObject(hDc, hBmp);
BitBlt(hDc, 0, 0, width, height, GetDC(0), x, y, SRCCOPY);
bool ret = SaveBMPFile(SSHOTFILEBMP.c_str(), hBmp, hDc, width, height);
DeleteObject(hBmp);
return ret;
}


void __fastcall TSCREEN::FOVADD()
{
       ADJSCR = true;
       ADJFOV = true;
       if (Global::ffov < 46.0)Global::ffov += 0.15;
       ReSizeGLSceneEx(Global::ffov, Global::iWindowWidth, Global::iWindowHeight);
}

void __fastcall TSCREEN::FOVADDF()
{
       ADJSCR = true;
       ADJFOV = true;
       if (Global::ffov < 46.0)Global::ffov += 2.0;
       ReSizeGLSceneEx(Global::ffov, Global::iWindowWidth, Global::iWindowHeight);
}

void __fastcall TSCREEN::FOVREM()
{
       ADJSCR = true;
       ADJFOV = true;
       if (Global::ffov > 11.0)  Global::ffov -= 0.15;
       ReSizeGLSceneEx(Global::ffov, Global::iWindowWidth, Global::iWindowHeight);
}

void __fastcall TSCREEN::FOVDEL()
{
       ADJSCR = true;
       ADJFOV = true;
       Global::ffov = 45.0f;
       ReSizeGLSceneEx(Global::ffov, Global::iWindowWidth, Global::iWindowHeight);
}

void __fastcall TSCREEN::FOVREMF()
{
       ADJSCR = true;
       ADJFOV = true;
       if (Global::ffov > 3.0)  Global::ffov -= 2.0;
       ReSizeGLSceneEx(Global::ffov, Global::iWindowWidth, Global::iWindowHeight);
}

void __fastcall TSCREEN::RESADD(int mode)
{
       ADJSCR = true;
       ADJFOV = false;

       if (mode == 2) ReSizeGLSceneEx(Global::ffov,  640,  480);
       if (mode == 3) ReSizeGLSceneEx(Global::ffov,  800,  600);
       if (mode == 4) ReSizeGLSceneEx(Global::ffov, 1024,  768);
       if (mode == 5) ReSizeGLSceneEx(Global::ffov, 1152,  864);
       if (mode == 6) ReSizeGLSceneEx(Global::ffov, 1280, 1024);
}

void __fastcall TSCREEN::RESREM()
{

}

BOOL TSCREEN::ChangeScreenResolution (int width, int height, int bitsPerPixel)	// Change the screen resolution
{
	DEVMODE dmScreenSettings;					        // Device mode
	ZeroMemory (&dmScreenSettings, sizeof (DEVMODE));			// Make sure memory is cleared
	dmScreenSettings.dmSize	      = sizeof (DEVMODE);     	                // Size of the devmode structure
	dmScreenSettings.dmPelsWidth  = width;			                // Select screen width
	dmScreenSettings.dmPelsHeight = height;			                // Select screen height
	dmScreenSettings.dmBitsPerPel = bitsPerPixel;			        // Select bits per pixel
	dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	if (ChangeDisplaySettings (&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
         return FALSE;				                                // Display change failed, return false
	}
	return TRUE;                                                            // Display change was successful, return true
}


GLvoid __fastcall TSCREEN::ReSizeGLSceneEx(double fov, GLsizei width, GLsizei height)
{
    //WindowWidth= width;
    //WindowHeight= height;

    Global::iWindowWidth = width;
    Global::iWindowHeight = height;

/*
	if (height==0) { height=1; }	// Prevent A Divide By Zero By, Making Height Equal One

        RECT windowRect = {0, 0, Global::iWindowWidth, Global::iWindowHeight};	// Define our window coordinates

        DWORD windowStyle = WS_OVERLAPPEDWINDOW;	                        // Define our window style
	DWORD windowExtendedStyle = WS_EX_APPWINDOW;	                        // Define the window's extended style


if (!adjfov)
    {
		if (ChangeScreenResolution (Global::iWindowWidth, Global::iWindowHeight, Global::iBpp) == FALSE)
		{
			// Fullscreen Mode Failed.  Run In Windowed Mode Instead
			MessageBox (HWND_DESKTOP, "Mode Switch Failed.\nRunning In Windowed Mode.", "Error", MB_OK | MB_ICONEXCLAMATION);

		}
		else						                // Otherwise (If fullscreen mode was successful)
		{
                  windowStyle = WS_POPUP;			                // Set the windowStyle to WS_POPUP (Popup window)
                  windowExtendedStyle |= WS_EX_TOPMOST;	                        // Set the extended window style to WS_EX_TOPMOST
		}

                AdjustWindowRectEx (&windowRect, windowStyle, 0, windowExtendedStyle);
    }
*/

        glViewport(0,0,Global::iWindowWidth, Global::iWindowHeight);		// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	gluPerspective(fov,(GLdouble)Global::iWindowWidth/(GLdouble)Global::iWindowHeight, 0.1f, 950600000000.0f);       // Calculate The Aspect Ratio Of The Window

        TSCREEN::CFOV = fov;

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix


}


void __fastcall TSCREEN::SaveScreen_TGA()
{
	FILE *sub;
	unsigned char* buf = new unsigned char[3*1280*1024];
	char file[30];
	unsigned char ctmp = 0, type, mode, rb;
	short int width = Global::iWindowWidth, height = Global::iWindowHeight, itmp = 0;
	unsigned char pixelDepth = 24;

        WriteLog("SAVING SCREEN TO TGA");

	if(buf==NULL )return;

	glPixelStorei( GL_PACK_ALIGNMENT, 1);

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

        FDATE = FormatDateTime("hhmmss-ddmmyy", Now());

        SSHOTFILETGA = Global::asSSHOTDIR + FDATE + ".tga";

        WriteLog(SSHOTFILETGA);

	for(int c=0;c<99;c++)
	{
		char pom[20];
		strcpy(file,FDATE.c_str());
		//if(c<10)strcat(file,"0");
		//strcat(file,itoa(c,pom,10));
		strcat(file,".tga");
                strcpy(file,FDATE.c_str());

		sub = fopen(SSHOTFILETGA.c_str(),"r");
		if(sub==NULL)c=100;
		if(sub!=NULL)fclose(sub);
	}



	sub = fopen(SSHOTFILETGA.c_str(),"wb");
	
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

	// tu sa musi prehodit R->B - pozri specifikaciu TGA formatu.
	if (mode >= 3) {
		for (int i = 0; i < width * height * mode; i += mode)
		{
			rb = buf[i];
			buf[i] = buf[i+2];
			buf[i+2] = rb;
		}
	}

	// tu sa zapisuje hlavicka
	fwrite(&ctmp, sizeof(unsigned char), 1, sub);
	fwrite(&ctmp, sizeof(unsigned char), 1, sub);

	fwrite(&type, sizeof(unsigned char), 1, sub);

	fwrite(&itmp, sizeof(short int), 1, sub);
	fwrite(&itmp, sizeof(short int), 1, sub);
	fwrite(&ctmp, sizeof(unsigned char), 1, sub);
	fwrite(&itmp, sizeof(short int), 1, sub);
	fwrite(&itmp, sizeof(short int), 1, sub);

	fwrite(&width, sizeof(short int), 1, sub);
	fwrite(&height, sizeof(short int), 1, sub);
	fwrite(&pixelDepth, sizeof(unsigned char), 1, sub);

	fwrite(&ctmp, sizeof(unsigned char), 1, sub);
	
	fwrite(buf, 3*width*height, 1, sub);

	delete[] buf;
	fclose(sub);
}




//---------------------------------------------------------------------------

#pragma package(smart_init)
