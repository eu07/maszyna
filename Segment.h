//---------------------------------------------------------------------------

#ifndef SegmentH
#define SegmentH

#include "VBO.h"
#include "dumb3d.h"
#include "Classes.h"

using namespace Math3D;

// 110405 Ra: klasa punktów przekroju z normalnymi

class vector6 : public vector3
{ // punkt przekroju wraz z wektorem normalnym
  public:
    vector3 n;
    vector6()
    {
        x = y = z = n.x = n.z = 0.0;
        n.y = 1.0;
    };
    vector6(double a, double b, double c, double d, double e, double f)
    //{x=a; y=b; z=c; n.x=d; n.y=e; n.z=f;};
    {
        x = a;
        y = b;
        z = c;
        n.x = 0.0;
        n.y = 1.0;
        n.z = 0.0;
    }; // Ra: bo na razie s¹ z tym problemy
    vector6(double a, double b, double c)
    {
        x = a;
        y = b;
        z = c;
        n.x = 0.0;
        n.y = 1.0;
        n.z = 0.0;
    };
};

class TSegment
{ // aproksymacja toru (zwrotnica ma dwa takie, jeden z nich jest aktywny)
  private:
    vector3 Point1, CPointOut, CPointIn, Point2;
    double fRoll1, fRoll2; // przechy³ka na koñcach
    double fLength; // d³ugoœæ policzona
    double *fTsBuffer; // wartoœci parametru krzywej dla równych odcinków
    double fStep;
    int iSegCount; // iloœæ odcinków do rysowania krzywej
    double fDirection; // Ra: k¹t prostego w planie; dla ³uku k¹t od Point1
    double fStoop; // Ra: k¹t wzniesienia; dla ³uku od Point1
    vector3 vA, vB, vC; // wspó³czynniki wielomianów trzeciego stopnia vD==Point1
    // TSegment *pPrev; //odcinek od strony punktu 1 - w segmencie, ¿eby nie skakaæ na zwrotnicach
    // TSegment *pNext; //odcinek od strony punktu 2
    TTrack *pOwner; // wskaŸnik na w³aœciciela
    double fAngle[2]; // k¹ty zakoñczenia drogi na przejazdach
    vector3 GetFirstDerivative(double fTime);
    double RombergIntegral(double fA, double fB);
    double GetTFromS(double s);
    vector3 RaInterpolate(double t);
    vector3 RaInterpolate0(double t);
    // TSegment *segNeightbour[2]; //s¹siednie odcinki - musi byæ przeniesione z Track
    // int iNeightbour[2]; //do którego koñca doczepiony
  public:
    bool bCurve;
    // int iShape; //Ra: flagi kszta³tu dadz¹ wiêcej mo¿liwoœci optymalizacji
    // (0-Bezier,1-prosty,2/3-³uk w lewo/prawo,6/7-przejœciowa w lewo/prawo)
    TSegment(TTrack *owner);
    ~TSegment();
    bool Init(vector3 NewPoint1, vector3 NewPoint2, double fNewStep,
                         double fNewRoll1 = 0, double fNewRoll2 = 0);
    bool Init(vector3 &NewPoint1, vector3 NewCPointOut, vector3 NewCPointIn,
                         vector3 &NewPoint2, double fNewStep, double fNewRoll1 = 0,
                         double fNewRoll2 = 0, bool bIsCurve = true);
    inline double ComputeLength(); // McZapkie-150503
    inline vector3 GetDirection1() { return bCurve ? CPointOut - Point1 : CPointOut; };
    inline vector3 GetDirection2() { return bCurve ? CPointIn - Point2 : CPointIn; };
    vector3 GetDirection(double fDistance);
    vector3 GetDirection() { return CPointOut; };
    vector3 FastGetDirection(double fDistance, double fOffset);
    vector3 GetPoint(double fDistance);
    void RaPositionGet(double fDistance, vector3 &p, vector3 &a);
    vector3 FastGetPoint(double t);
    inline vector3 FastGetPoint_0() { return Point1; };
    inline vector3 FastGetPoint_1() { return Point2; };
    inline double GetRoll(double s)
    {
        s /= fLength;
        return ((1.0 - s) * fRoll1 + s * fRoll2);
    }
    void GetRolls(double &r1, double &r2)
    { // pobranie przechy³ek (do generowania trójk¹tów)
        r1 = fRoll1;
        r2 = fRoll2;
    }
    void RenderLoft(const vector6 *ShapePoints, int iNumShapePoints,
                               double fTextureLength, int iSkip = 0, int iQualityFactor = 1,
                               vector3 **p = NULL, bool bRender = true);
    void RenderSwitchRail(const vector6 *ShapePoints1, const vector6 *ShapePoints2,
                                     int iNumShapePoints, double fTextureLength, int iSkip = 0,
                                     double fOffsetX = 0.0f);
    void Render();
    inline double GetLength() { return fLength; };
    void MoveMe(vector3 pPosition)
    {
        Point1 += pPosition;
        Point2 += pPosition;
        if (bCurve)
        {
            CPointIn += pPosition;
            CPointOut += pPosition;
        }
    }
    int RaSegCount() { return fTsBuffer ? iSegCount : 1; };
    void RaRenderLoft(CVertNormTex *&Vert, const vector6 *ShapePoints,
                                 int iNumShapePoints, double fTextureLength, int iSkip = 0,
                                 int iEnd = 0, double fOffsetX = 0.0);
    void RaAnimate(CVertNormTex *&Vert, const vector6 *ShapePoints, int iNumShapePoints,
                              double fTextureLength, int iSkip = 0, int iEnd = 0,
                              double fOffsetX = 0.0);
    void AngleSet(int i, double a) { fAngle[i] = a; };
    void Rollment(double w1, double w2); // poprawianie przechy³ki
};

//---------------------------------------------------------------------------
#endif
