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
    //    inline bool __fastcall IsCurve() { return (!(Point==CPointIn)); };
    //    inline vector3 __fastcall GetDirection() { return (CPointOut-Point); };
    __fastcall TKnot();
    __fastcall TKnot(int n);
    __fastcall ~TKnot();
    inline float __fastcall GetRoll(float s)
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
    vector3 __fastcall GetDirection(float t = 0);
    inline vector3 __fastcall InterpolateSegm(float t)
    {
        return (Interpolate(t, Point, CPointOut, Next->CPointIn, Next->Point));
    };
    float __fastcall GetTFromS(float s);
    bool __fastcall Init(TKnot *NNext, TKnot *NPrev);
    bool __fastcall InitCPoints();
    bool __fastcall InitLength();
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
    __fastcall TSpline();
    __fastcall TSpline(AnsiString asNName);
    __fastcall ~TSpline();
    bool __fastcall Create(int n, AnsiString asNName = "foo");
    bool __fastcall AssignKnots(TKnot *FirstKnot, int n);
    int __fastcall LoadFromFile(AnsiString FileName, TKnot *FirstKnot = NULL);
    int __fastcall Load(TQueryParserComp *Parser, AnsiString asEndString = "endspline");
    float __fastcall GetLength();
    vector3 __fastcall GetCenter();
    TKnot *__fastcall GetLastKnot();
    bool __fastcall Render();

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

bool __fastcall Connect(TKnot *k1, TKnot *k2);

//---------------------------------------------------------------------------
#endif
