//---------------------------------------------------------------------------
#ifndef GeometryH
#define GeometryH

#include "opengl/glew.h"
#include "dumb3d.h"
using namespace Math3D;
//---------------------------------------------------------------------------

class TPlane;

class TLine
{
  public:
    vector3 Vector, Point;
    TLine();
    TLine(vector3 NPoint, vector3 NVector);
    ~TLine();
    TPlane GetPlane();
    double GetDistance(vector3 Point1);
};

//---------------------------------------------------------------------------

class TPlane
{
  public:
    vector3 Vector;
    double d;
    TPlane();
    TPlane(vector3 NVector, double nd);
    TPlane(vector3 NPoint, vector3 NVector);
    TPlane(vector3 Point1, vector3 Vector1, vector3 Vector2);
    ~TPlane();
    void Normalize();
    double GetSide(vector3 Point);
    //    void Transform(D3DMATRIX &Transformations);
    bool Defined();
};

//---------------------------------------------------------------------------

inline double Sum(vector3 &Vector);
bool CrossPoint(vector3 &RetPoint, TLine &Line, TPlane &Plane);

//---------------------------------------------------------------------------
#endif
