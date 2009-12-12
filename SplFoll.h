//---------------------------------------------------------------------------

#ifndef SplFollH
#define SplFollH

#include "dumb3d.h"
using namespace Math3D;
#include "Spline.h"

class TSplineFollower
{
public:
    __fastcall TSplineFollower();
    __fastcall ~TSplineFollower();
    bool __fastcall Init(TKnot *Knot, bool NAccurate= false);
    void __fastcall Move(float Distance);
    bool __fastcall Refresh(float DeltaTime);
    inline float __fastcall TotalDistance() { return dist+s; };

    inline vector3 __fastcall GetDirection()
    {
        return (CurrentKnot->GetDirection((CurrentKnot->IsCurve?CurrentKnot->GetTFromS(s):0)));
//        return (CurrentKnot->GetDirection(oldt));
//        float t= GetTFromS
//    CurrentKnot
//        return ( Spline->GetDirection(KnotIndex,s/Spline->Knots[KnotIndex].Length) );
//        return ( CurrentKnot->GetDirection(s/CurrentKnot->Length) );
    }

    vector3 Position;
//    vector3 vDirection;
    float s,v,a,dist,oldt;
    float fDirection;
//    int KnotIndex;
    TKnot *CurrentKnot;
    bool Accurate;
    bool bChangeableDirection;
private:
//    TSpline *Spline;
};
//---------------------------------------------------------------------------
#endif
 