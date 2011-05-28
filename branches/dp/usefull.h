//---------------------------------------------------------------------------
#ifndef UsefullH
#define UsefullH

#include "dumb3d.h"
#include "Logs.h"

#define	B1(t)     (t*t*t)
#define	B2(t)     (3*t*t*(1-t))
#define	B3(t)     (3*t*(1-t)*(1-t))
#define	B4(t)     ((1-t)*(1-t)*(1-t))
//Ra: to jest mocno nieoptymalne: 10+3*4=22 mno¿enia, 6 odejmowañ, 3*3=9 dodawañ
//Ra: po przeliczeniu wspó³czynników mamy: 3*3=9 mno¿eñ i 3*3=9 dodawañ
#define	Interpolate(t,p1,cp1,cp2,p2)     (B4(t)*p1+B3(t)*cp1+B2(t)*cp2+B1(t)*p2)

#define Pressed(x) (GetKeyState(x)<0)

//Ra: "delete NULL" nic nie zrobi, wiêc "if (a!=NULL)" jest zbêdne
#define SafeFree(a) if (a!=NULL) free(a)
#define SafeDelete(a) { delete (a); a=NULL; }
#define SafeDeleteArray(a) { delete[] (a); a=NULL; }

#define sign(x) ((x)<0?-1:((x)>0?1:0))

const Math3D::vector3 vWorldFront= Math3D::vector3(0,0,1);
const Math3D::vector3 vWorldUp= Math3D::vector3(0,1,0);
const Math3D::vector3 vWorldLeft= CrossProduct(vWorldUp,vWorldFront);
const Math3D::vector3 vGravity= Math3D::vector3(0,-9.81,0);

#define DegToRad(a) ((M_PI/180.0)*a)
#define RadToDeg(r) ((180.0/M_PI)*r)

#define Fix(a,b,c) {if (a<b) a=b; if (a>c) a=c;}

#define asModelsPatch AnsiString("models\\")
#define asSceneryPatch AnsiString("scenery\\")
//#define asTexturePatch AnsiString("textures\\")
//#define asTextureExt AnsiString(".bmp")
#define szDefaultTexturePath "textures\\"
#define szDefaultTextureExt ".dds"

void __fastcall Error(const AnsiString &asMessage,bool box=true)
{
 if (box)
  MessageBox(NULL,asMessage.c_str(),"EU07-424",MB_OK);
 WriteLog(asMessage.c_str());
}
void __fastcall WriteLog(const AnsiString &str)
{//Ra: wersja z AnsiString jest zamienna z Error()
 WriteLog(str.c_str());
};

#define DevelopTime     //FIXME
//#define EditorMode

//---------------------------------------------------------------------------
#endif


 