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
    TGeometry();
    ~TGeometry();
    bool Init();
    vector3 Load(TQueryParserComp *Parser);
    bool Render();
};

//---------------------------------------------------------------------------
#endif
