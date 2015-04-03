//---------------------------------------------------------------------------

#ifndef GeomH
#define GeomH

#include <gl/gl.h>
#include "QueryParserComp.hpp"

struct TGeomVertex
{
    vector3 Point;
    vector3 Normal;
    double tu, tv;
};

class TGeometry
{
  private:
    GLuint iType;
    union
    {
        int iNumVerts;
        int iNumPts;
    };
    GLuint TextureID;
    TMaterialColor Ambient;
    TMaterialColor Diffuse;
    TMaterialColor Specular;

  public:
    __fastcall TGeometry();
    __fastcall ~TGeometry();
    bool __fastcall Init();
    vector3 __fastcall Load(TQueryParserComp *Parser);
    bool __fastcall Render();
};

//---------------------------------------------------------------------------
#endif
