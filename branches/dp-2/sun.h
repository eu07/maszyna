//---------------------------------------------------------------------------

#ifndef sunH
#define sunH

#include "MdlMngr.h"

struct color4{

        float R;
        float G;
        float B;
        float A;
};

class TSun
{
private:

  TModel3d *mdSUN;
  
double SSRX;  // SUN ROTATE X
double SSRY;  // SUN ROTATE Y
double SSRZ;  // SUN ROTATE Z

double sunrottime;

AnsiString SUN_NAME;

color4 SUNS1_COLOR;
color4 SUNS2_COLOR;
color4 SUNS3_COLOR;


float  SUNS1_DIST;
float  SUNS1_RADI;
float  SUNS2_DIST;
float  SUNS2_RADI;
float  SUNS3_DIST;
float  SUNS3_RADI;

float  SUNL1_DIST;
float  SUNL1_RADI;
GLint  SUNL1_TEXT;
color4 SUNL1_COLOR;

float  SUNL2_DIST;
float  SUNL2_RADI;
GLint  SUNL2_TEXT;
color4 SUNL2_COLOR;

float  SUNL3_DIST;
float  SUNL3_RADI;
GLint  SUNL3_TEXT;
color4 SUNL3_COLOR;

float  SUNL4_DIST;
float  SUNL4_RADI;
GLint  SUNL4_TEXT;
color4 SUNL4_COLOR;

float  SUNL5_DIST;
float  SUNL5_RADI;
GLint  SUNL5_TEXT;
color4 SUNL5_COLOR;

float  SUNL6_DIST;
float  SUNL6_RADI;
GLint  SUNL6_TEXT;
color4 SUNL6_COLOR;

public:
  __fastcall LOADCONFIG();
  __fastcall Render_A(double dt);
  __fastcall Render_1(double dt);
  __fastcall Render_2(double dt);
  __fastcall Render_luneplane(double size, int tid);
  __fastcall TSun();
  __fastcall Init();
  __fastcall ~TSun();
};

//---------------------------------------------------------------------------
#endif
