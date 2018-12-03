/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "dumb3d.h"
#include "openglgeometrybank.h"
#include "utilities.h"

struct segment_data {
// types
    enum point {
        start = 0,
        control1,
        control2,
        end
    };
// members
    std::array<glm::dvec3, 4> points {};
    std::array<float, 2> rolls {};
    float radius {};
// constructors
    segment_data() = default;
// methods
    void deserialize( cParser &Input, glm::dvec3 const &Offset );
};

class TSegment
{ // aproksymacja toru (zwrotnica ma dwa takie, jeden z nich jest aktywny)
  private:
    Math3D::vector3 Point1, CPointOut, CPointIn, Point2;
    float
        fRoll1 { 0.f },
        fRoll2 { 0.f }; // przechyłka na końcach
    double fLength { -1.0 }; // długość policzona
    std::vector<double> fTsBuffer; // wartości parametru krzywej dla równych odcinków
    double fStep = 0.0;
    int iSegCount = 0; // ilość odcinków do rysowania krzywej
    double fDirection = 0.0; // Ra: kąt prostego w planie; dla łuku kąt od Point1
    double fStoop = 0.0; // Ra: kąt wzniesienia; dla łuku od Point1
    Math3D::vector3 vA, vB, vC; // współczynniki wielomianów trzeciego stopnia vD==Point1
    TTrack *pOwner = nullptr; // wskaźnik na właściciela

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
/*
    Math3D::vector3
        GetPoint(double const fDistance) const;
*/
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
    float
        GetRoll(double const Distance) const {
            return interpolate( fRoll1, fRoll2, static_cast<float>(Distance / fLength) ); }
    inline
    void
        GetRolls(float &r1, float &r2) const {
            // pobranie przechyłek (do generowania trójkątów)
            r1 = fRoll1;
            r2 = fRoll2; }

    bool
        RenderLoft( gfx::vertex_array &Output, Math3D::vector3 const &Origin, gfx::vertex_array const &ShapePoints, bool const Transition, double fTextureLength, double Texturescale = 1.0, int iSkip = 0, int iEnd = 0, std::pair<float, float> fOffsetX = {0.f, 0.f}, glm::vec3 **p = nullptr, bool bRender = true );
/*
    void
        Render();
*/
    inline
    double
        GetLength() const {
            return fLength; };
    inline
    int
        RaSegCount() const {
            return ( fTsBuffer.empty() ? 1 : iSegCount ); };
};

//---------------------------------------------------------------------------
