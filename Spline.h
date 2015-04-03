//---------------------------------------------------------------------------
#ifndef SplineH
#define SplineH

#include "dumb3d.h"
#include "QueryParserComp.hpp"
using namespace Math3D;

#define B1(t) (t * t * t)
#define B2(t) (3 * t * t * (1 - t))
#define B3(t) (3 * t * (1 - t) * (1 - t))
#define B4(t) ((1 - t) * (1 - t) * (1 - t))
#define Interpolate(t, p1, cp1, cp2, p2) (B4(t) * p1 + B3(t) * cp1 + B2(t) * cp2 + B1(t) * p2)

class TKnot
{
  public:
    //    inline bool IsCurve() { return (!(Point==CPointIn)); };
    //    inline vector3 GetDirection() { return (CPointOut-Point); };
    TKnot();
    TKnot(int n);
    ~TKnot();
    inline float GetRoll(float s)
    {
        if (Next)
        {
            s /= Length;
            return ((1 - s) * fRoll + (s)*Next->fRoll);
        }
        else
            return fRoll;
        //        (Length-s) (s-Length)
    }
    vector3 GetDirection(float t = 0);
    inline vector3 InterpolateSegm(float t)
    {
        return (Interpolate(t, Point, CPointOut, Next->CPointIn, Next->Point));
    };
    float GetTFromS(float s);
    bool Init(TKnot *NNext, TKnot *NPrev);
    bool InitCPoints();
    bool InitLength();
    vector3 Point, CPointIn, CPointOut;
    float fRoll;
    bool IsCurve;
    float Length;
    TKnot *Next, *Prev;
    float fLengthIn, fLengthOut;
    bool bSwitchDirectionForward;
    bool bSwitchDirectionBackward;

  private:
    vector3 GetFirstDerivative(float fTime);
    float RombergIntegral(float fA, float fB);
};

class TSpline
{
  public:
    TSpline();
    TSpline(AnsiString asNName);
    ~TSpline();
    bool Create(int n, AnsiString asNName = "foo");
    bool AssignKnots(TKnot *FirstKnot, int n);
    int LoadFromFile(AnsiString FileName, TKnot *FirstKnot = NULL);
    int Load(TQueryParserComp *Parser, AnsiString asEndString = "endspline");
    float GetLength();
    vector3 GetCenter();
    TKnot *__fastcall GetLastKnot();
    bool Render();

    //    inline int NextIndex(int n) { return (n<KnotsCount-1 ? n+1 : 0); };

    //    inline vector3 InterpolateSegm(int i, float t) { return
    //    (Interpolate(t,Knots[i].Point,Knots[i].CPointOut,Knots[NextIndex(i)].CPointIn,Knots[NextIndex(i)].Point)
    //    ); };
    //    inline vector3 InterpolateSegm(TKnot *Knot, float t) { return
    //    (Interpolate(t,Knots[i].Point,Knots[i].CPointOut,Knots[NextIndex(i)].CPointIn,Knots[NextIndex(i)].Point)
    //    ); };
    //    bool Closed;
    //    AnsiString asName;
    //    TKnot *Knots;
    TKnot *RootKnot;
    bool KnotsAllocated;
    int iNumKnots;
    //    TSpline *Next,*Prev;
    //    TKnot Next,Prev;

  private:
};

bool Connect(TKnot *k1, TKnot *k2);

//---------------------------------------------------------------------------
#endif
