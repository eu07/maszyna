/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef SegmentH
#define SegmentH
/*
#include "VBO.h"
*/
#include "openglgeometrybank.h"
#include "dumb3d.h"
#include "Classes.h"
#include "usefull.h"

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
    }; // Ra: bo na razie są z tym problemy
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
    double fRoll1 = 0.0,
           fRoll2 = 0.0; // przechyłka na końcach
    double fLength = 0.0; // długość policzona
    double *fTsBuffer = nullptr; // wartości parametru krzywej dla równych odcinków
    double fStep = 0.0;
    int iSegCount = 0; // ilość odcinków do rysowania krzywej
    double fDirection = 0.0; // Ra: kąt prostego w planie; dla łuku kąt od Point1
    double fStoop = 0.0; // Ra: kąt wzniesienia; dla łuku od Point1
    vector3 vA, vB, vC; // współczynniki wielomianów trzeciego stopnia vD==Point1
    TTrack *pOwner = nullptr; // wskaźnik na właściciela
    double fAngle[2]; // kąty zakończenia drogi na przejazdach

    vector3 GetFirstDerivative(double fTime);
    double RombergIntegral(double fA, double fB);
    double GetTFromS(double s);
    vector3 RaInterpolate(double t);
    vector3 RaInterpolate0(double t);
  public:
    bool bCurve = false;
    TSegment(TTrack *owner);
    ~TSegment();
    bool Init(vector3 NewPoint1, vector3 NewPoint2, double fNewStep, double fNewRoll1 = 0, double fNewRoll2 = 0);
    bool Init(vector3 &NewPoint1, vector3 NewCPointOut, vector3 NewCPointIn, vector3 &NewPoint2,
              double fNewStep, double fNewRoll1 = 0, double fNewRoll2 = 0, bool bIsCurve = true);
    inline double ComputeLength(); // McZapkie-150503
    inline vector3 GetDirection1() {
        return bCurve ? CPointOut - Point1 : CPointOut; };
    inline vector3 GetDirection2() {
        return bCurve ? CPointIn - Point2 : CPointIn; };
    vector3 GetDirection(double fDistance);
    vector3 GetDirection() {
        return CPointOut; };
    vector3 FastGetDirection(double fDistance, double fOffset);
    vector3 GetPoint(double fDistance);
    void RaPositionGet(double fDistance, vector3 &p, vector3 &a);
    vector3 FastGetPoint(double t);
    inline vector3 FastGetPoint_0() {
        return Point1; };
    inline vector3 FastGetPoint_1() {
        return Point2; };
    inline double GetRoll(double const Distance) {
        return interpolate( fRoll1, fRoll2, Distance / fLength ); }
    void GetRolls(double &r1, double &r2) {
        // pobranie przechyłek (do generowania trójkątów)
        r1 = fRoll1;
        r2 = fRoll2; }
    bool RenderLoft( vertex_array &Output, Math3D::vector3 const &Origin, vector6 const *ShapePoints, int iNumShapePoints, double fTextureLength, double Texturescale = 1.0, int iSkip = 0, int iEnd = 0, double fOffsetX = 0.0, vector3 **p = nullptr, bool bRender = true);
    void Render();
    inline double GetLength() {
        return fLength; };
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
	int RaSegCount()
	{
		if (!fTsBuffer || !bCurve)
			return 1;
		return iSegCount;
	};
    void AngleSet(int i, double a) {
        fAngle[i] = a; };
};

//---------------------------------------------------------------------------
#endif
