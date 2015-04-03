//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Geometry.h"

inline double sqr(double a) { return (a * a); };

__fastcall TLine::TLine(){};

__fastcall TLine::TLine(vector3 NPoint, vector3 NVector)
{
    Vector = NVector;
    Point = NPoint;
};

__fastcall TLine::~TLine(){};

TPlane TLine::GetPlane() { return (TPlane(Point, Vector)); };

double TLine::GetDistance(vector3 Point1)
{
    return ((sqr((Point1.x - Point.x) * Vector.x - (Point1.y - Point.y) * Vector.y) -
             sqr((Point1.y - Point.y) * Vector.y - (Point1.z - Point.z) * Vector.z) -
             sqr((Point1.z - Point.z) * Vector.z - (Point1.x - Point.x) * Vector.x)) /
            (sqr(Vector.x) + sqr(Vector.y) + sqr(Vector.z)));
    ;
};

//---------------------------------------------------------------------------

__fastcall TPlane::TPlane()
{
    Vector = vector3(0, 0, 0);
    d = 0;
};

__fastcall TPlane::TPlane(vector3 NVector, double nd)
{
    Vector = NVector;
    d = nd;
}

__fastcall TPlane::TPlane(vector3 Point, vector3 NVector)
{
    Vector = NVector;
    d = -NVector.x * Point.x - NVector.y * Point.y - NVector.z * Point.z;
};

__fastcall TPlane::TPlane(vector3 Point1, vector3 Vector1, vector3 Vector2)
{
    Vector = CrossProduct(Vector1, Vector2);
    d = -Vector.x * Point1.x - Vector.y * Point1.y - Vector.z * Point1.z;
};

__fastcall TPlane::~TPlane(){};

void TPlane::Normalize()
{
    double mgn = Vector.Length();
    Vector = Vector / mgn;
    d /= mgn;
};

double TPlane::GetSide(vector3 Point)
{
    return (Vector.x * Point.x + Vector.y * Point.y + Vector.z * Point.z + d);
};

// void TPlane::Transform(D3DMATRIX &Transformations)
//{
//    vector3 src= Vector;
//    D3DMath_VectorMatrixMultiply(Vector,src,Transformations);
//};

bool TPlane::Defined() { return !(Vector == vector3(0, 0, 0)); };

//---------------------------------------------------------------------------

inline double Sum(vector3 &Vector) { return (Vector.x + Vector.y + Vector.z); };

bool CrossPoint(vector3 &RetPoint, TLine &Line, TPlane &Plane)
{
    double ro = DotProduct(Plane.Vector, Line.Vector);
    if (ro == 0)
        return (false);
    ro = (DotProduct(Plane.Vector, Line.Point) + Plane.d) / ro;
    RetPoint = Line.Point - (Line.Vector * ro);
    return (true);
};

inline double GetLength(vector3 &Vector) { return (Vector.Length()); };

inline vector3 SetLength(vector3 &Vector, double Length)
{
    Vector.Normalize();
    return (Vector * Length);
};
//---------------------------------------------------------------------------
//#pragma package(smart_init)
