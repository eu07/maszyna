//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"

#include "opengl/glew.h"
#include "opengl/glut.h"
#pragma hdrstop


#include "Globals.h"
#include "screenshot.h"

//#include <string>
//#include <sstream>
//#include <map>
//#include <vector>
//#include <fstream>
#include <stdio.h>

#include <Controls.hpp>      //  Graphics::TBitmap
#include <jpeg.hpp>          // TJPEGImage

AnsiString SSHOTFILEBMP, SSHOTFILEJPG;
int SSHOTQUALITY = 90;
bool SSHOTSMOOTHING = true;

int sw, sh;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// SCREENSHOT ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


void __fastcall MakeScreenShot(HWND GL_HWND, HDC GL_HDC, HGLRC GL_hRC)
     {

    sw = Global::iWindowWidth;
    sh = Global::iWindowHeight;

    RECT rc;
    int w, h, i, j;
    //const char* ch;
    GetClientRect( GL_HWND, &rc );
   // w = rc.right-rc.left;
   // h = rc.bottom-rc.top;

         Graphics::TBitmap *FormBitmap = new Graphics::TBitmap();

         FormBitmap->Handle = CreateCompatibleBitmap(GL_HDC, sw, sh);
         BitBlt(FormBitmap->Canvas->Handle, 0, 0, FormBitmap->Width, FormBitmap->Height,GL_HDC, 0, 0, SRCCOPY);

         SSHOTFILEBMP = Global::asSSHOTDIR + FormatDateTime("hhmmss-ddmmyy", Now()) + ".bmp";
         SSHOTFILEJPG = Global::asSSHOTDIR + FormatDateTime("hhmmss-ddmmyy", Now()) + ".jpg";

         Global::asSCREENSHOTFILE = SSHOTFILEJPG;
         
         //SSHOTFILEBMP = "N:\\" + FormatDateTime("hhmmss-ddmmyy", Now()) + ".bmp";

//         QueuedLog("FILE=" + SSHOTFILEJPG + ")");

         //FormBitmap->SaveToFile(SSHOTFILEBMP);

//       Graphics::TBitmap *BMP = new Graphics::TBitmap();      // KONWERSJA NA JPG
         TJPEGImage *JPG = new TJPEGImage();

//       BMP->LoadFromFile(SSHOTFILEBMP);


         JPG->Assign(FormBitmap);        // (BMP)
         JPG->Smoothing = SSHOTSMOOTHING;
         JPG->CompressionQuality = SSHOTQUALITY;
         JPG->Compress();
         JPG->SaveToFile(SSHOTFILEJPG);

         delete FormBitmap;

         Global::bSCREENSHOT = true;

         PlaySound("data\\shutter.wav", NULL, SND_ASYNC);

}


void __fastcall SaveScreen_JPG(HWND GL_HWND, HDC GL_HDC, HGLRC GL_hRC, int Q)
     {
AnsiString SSHOTFILEBMP, SSHOTFILEJPG, SSHOTFILETGA, FDATE;
int SSHOTQUALITY = 90;
bool SSHOTSMOOTHING = true;

int sw, sh;

     Graphics::TBitmap *FormBitmap;

    sw = Global::iWindowWidth;
    sh = Global::iWindowHeight;

    SSHOTQUALITY = Q;

    RECT rc;
    int w, h, i, j;
    //const char* ch;
    GetClientRect( GL_HWND, &rc );

         FormBitmap = new Graphics::TBitmap();

         FormBitmap->Handle = CreateCompatibleBitmap(GL_HDC, sw, sh);
         BitBlt(FormBitmap->Canvas->Handle, 0, 0, FormBitmap->Width, FormBitmap->Height,GL_HDC, 0, 0, SRCCOPY);

         SSHOTFILEBMP = Global::asSSHOTDIR + "last.bmp";
         SSHOTFILEJPG = Global::asSSHOTDIR + FormatDateTime("hhmmss-ddmmyy", Now()) + ".jpg";

         //SSHOTFILEBMP = "N:\\" + FormatDateTime("hhmmss-ddmmyy", Now()) + ".bmp";

//         QueuedLog("FILE=" + SSHOTFILEJPG + ")");

         FormBitmap->SaveToFile(SSHOTFILEBMP);

//       Graphics::TBitmap *BMP = new Graphics::TBitmap();      // KONWERSJA NA JPG
         TJPEGImage *JPG = new TJPEGImage();

//       BMP->LoadFromFile(SSHOTFILEBMP);

         JPG->Assign(FormBitmap);        // (BMP)
         JPG->Smoothing = SSHOTSMOOTHING;
         JPG->CompressionQuality = SSHOTQUALITY;
         JPG->Compress();
         JPG->SaveToFile(SSHOTFILEJPG);


         //Global::SCRPREVIEW       = TTexturesManager::GetTextureID(AnsiString(SSHOTFILEBMP).c_str());

         delete FormBitmap;

}

//---------------------------------------------------------------------------

#pragma package(smart_init)
