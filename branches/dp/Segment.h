//---------------------------------------------------------------------------

#ifndef SegmentH
#define SegmentH

#include "usefull.h"
#include "Geometry.h"

class TSegment
{
private:
    vector3 Point1,CPointOut,CPointIn,Point2;
    double fRoll1,fRoll2;
    double fLength;
    double *fTsBuffer;
    double fStep;
    vector3 __fastcall GetFirstDerivative(double fTime);
    double __fastcall RombergIntegral(double fA, double fB);
    double __fastcall GetTFromS(double s);
public:
    bool bCurve;
    __fastcall TSegment();
    __fastcall ~TSegment() { SafeDeleteArray(fTsBuffer); };
    bool __fastcall Init(vector3 NewPoint1, vector3 NewPoint2, double fNewStep,
                         double fNewRoll1= 0, double fNewRoll2= 0);
    bool __fastcall Init(vector3 NewPoint1, vector3 NewCPointOut,
                         vector3 NewCPointIn, vector3 NewPoint2, double fNewStep,
                         double fNewRoll1= 0, double fNewRoll2= 0, bool bIsCurve= true);
    inline double __fastcall ComputeLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2);  //McZapkie-150503                       
    inline vector3 __fastcall GetDirection1() { return CPointOut-Point1; };
    inline vector3 __fastcall GetDirection2() { return CPointIn-Point2; };
    vector3 __fastcall GetDirection(double fDistance);
    vector3 __fastcall GetDirection() { return CPointOut; };
    vector3 __fastcall FastGetDirection(double fDistance, double fOffset);
    vector3 __fastcall GetPoint(double fDistance);
    vector3 __fastcall FastGetPoint(double t);
    inline double __fastcall GetRoll(double s)
    {
        s/= fLength;
        return ((1-s)*fRoll1+(s)*fRoll2);
    }
    bool __fastcall RenderLoft(const vector3 *ShapePoints, int iNumShapePoints,
        double fTextureLength, int iSkip=0, int iQualityFactor=1);
    bool __fastcall RenderSwitchRail(const vector3 *ShapePoints1, const vector3 *ShapePoints2,
                            int iNumShapePoints,double fTextureLength, int iSkip=0, double fOffsetX=0.0f);
    bool __fastcall Render();
    inline double __fastcall GetLength() { return fLength; };
};

//---------------------------------------------------------------------------
#endif
