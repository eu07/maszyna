/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "openglgeometrybank.h"
#include "dumb3d.h"
#include "Classes.h"
#include "usefull.h"

// 110405 Ra: klasa punktów przekroju z normalnymi

class vector6 : public Math3D::vector3
{ // punkt przekroju wraz z wektorem normalnym
  public:
    Math3D::vector3 n;
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
    Math3D::vector3 Point1, CPointOut, CPointIn, Point2;
    double fRoll1 = 0.0,
           fRoll2 = 0.0; // przechyłka na końcach
    double fLength = 0.0; // długość policzona
    double *fTsBuffer = nullptr; // wartości parametru krzywej dla równych odcinków
    double fStep = 0.0;
    int iSegCount = 0; // ilość odcinków do rysowania krzywej
    double fDirection = 0.0; // Ra: kąt prostego w planie; dla łuku kąt od Point1
    double fStoop = 0.0; // Ra: kąt wzniesienia; dla łuku od Point1
    Math3D::vector3 vA, vB, vC; // współczynniki wielomianów trzeciego stopnia vD==Point1
    TTrack *pOwner = nullptr; // wskaźnik na właściciela
    double fAngle[ 2 ] = { 0.0, 0.0 }; // kąty zakończenia drogi na przejazdach

    Math3D::vector3
        GetFirstDerivative(double const fTime) const;
    double
        RombergIntegral(double const fA, double const fB) const;
    double
        GetTFromS(double const s) const;
    Math3D::vector3
        RaInterpolate(double const t) const;
    Math3D::vector3
        RaInterpolate0(double const t) const;

public:
    bool bCurve = false;

    TSegment(TTrack *owner);
    ~TSegment();
<<<<<<< HEAD
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
    void AngleSet(int i, double a) {
        fAngle[i] = a; };
    bool
        Init( Math3D::vector3 NewPoint1, Math3D::vector3 NewPoint2, double fNewStep, double fNewRoll1 = 0, double fNewRoll2 = 0);
    bool
        Init( Math3D::vector3 &NewPoint1, Math3D::vector3 NewCPointOut, Math3D::vector3 NewCPointIn, Math3D::vector3 &NewPoint2, double fNewStep, double fNewRoll1 = 0, double fNewRoll2 = 0, bool bIsCurve = true);
    double
        ComputeLength() const; // McZapkie-150503
    // finds point on segment closest to specified point in 3d space. returns: point on segment as value in range 0-1
    double
        find_nearest_point( glm::dvec3 const &Point ) const;
    inline
    Math3D::vector3
        GetDirection1() const {
            return bCurve ? CPointOut - Point1 : CPointOut; };
    inline
    Math3D::vector3
        GetDirection2() const {
            return bCurve ? CPointIn - Point2 : CPointIn; };
    Math3D::vector3
        GetDirection(double const fDistance) const;
    inline
    Math3D::vector3
        GetDirection() const {
            return CPointOut; };
    Math3D::vector3
        FastGetDirection(double const fDistance, double const fOffset);
    Math3D::vector3
        GetPoint(double const fDistance) const;
    void
        RaPositionGet(double const fDistance, Math3D::vector3 &p, Math3D::vector3 &a) const;
    Math3D::vector3
        FastGetPoint(double const t) const;
    inline
    Math3D::vector3
        FastGetPoint_0() const {
            return Point1; };
    inline
    Math3D::vector3
        FastGetPoint_1() const {
            return Point2; };
    inline
    double
        GetRoll(double const Distance) const {
            return interpolate( fRoll1, fRoll2, Distance / fLength ); }
    inline
    void
        GetRolls(double &r1, double &r2) const {
            // pobranie przechyłek (do generowania trójkątów)
            r1 = fRoll1;
            r2 = fRoll2; }

    bool
        RenderLoft( vertex_array &Output, Math3D::vector3 const &Origin, vector6 const *ShapePoints, int iNumShapePoints, double fTextureLength, double Texturescale = 1.0, int iSkip = 0, int iEnd = 0, double fOffsetX = 0.0, Math3D::vector3 **p = nullptr, bool bRender = true);
    void
        Render();
    inline
    double
        GetLength() const {
            return fLength; };
    inline
	int RaSegCount()
	{
		if (!fTsBuffer || !bCurve)
			return 1;
		return iSegCount;
	};
    inline
    void
        AngleSet(int const i, double const a) {
            fAngle[i] = a; };
};

//---------------------------------------------------------------------------
