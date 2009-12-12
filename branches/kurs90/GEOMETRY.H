//---------------------------------------------------------------------------
#ifndef GeometryH
#define GeometryH

#include <gl/gl.h>
#include "dumb3d.h"
using namespace Math3D;
//---------------------------------------------------------------------------

class TPlane;

class TLine
{
public:
    vector3 Vector, Point;
    __fastcall TLine();
    __fastcall TLine(vector3 NPoint, vector3 NVector);
    __fastcall ~TLine();
    TPlane __fastcall GetPlane();
    double __fastcall GetDistance(vector3 Point1);
};

//---------------------------------------------------------------------------

class TPlane
{
public:
    vector3 Vector;
    double d;
    __fastcall TPlane();
    __fastcall TPlane(vector3 NVector, double nd);
    __fastcall TPlane(vector3 NPoint, vector3 NVector);
    __fastcall TPlane(vector3 Point1, vector3 Vector1,  vector3 Vector2);
    __fastcall ~TPlane();
    void __fastcall Normalize();
    double __fastcall GetSide(vector3 Point);
//    void __fastcall Transform(D3DMATRIX &Transformations);
    bool __fastcall Defined();
};

//---------------------------------------------------------------------------

inline double __fastcall Sum(vector3 &Vector);
bool __fastcall CrossPoint(vector3 &RetPoint, TLine &Line, TPlane &Plane);

//---------------------------------------------------------------------------
#endif
