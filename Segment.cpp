/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Segment.h"

#include "Globals.h"
#include "Logs.h"
#include "utilities.h"
#include "Track.h"
#include "renderer.h"

void
segment_data::deserialize( cParser &Input, glm::dvec3 const &Offset ) {

    points[ segment_data::point::start ] = LoadPoint( Input ) + Offset;
    Input.getTokens();
    Input >> rolls[ 0 ];
    points[ segment_data::point::control1 ] = LoadPoint( Input );
    points[ segment_data::point::control2 ] = LoadPoint( Input );
    points[ segment_data::point::end ] = LoadPoint( Input ) + Offset;
    Input.getTokens( 2 );
    Input
        >> rolls[ 1 ]
        >> radius;
}


TSegment::TSegment(TTrack *owner) :
                   pOwner( owner )
{}

bool TSegment::Init(Math3D::vector3 NewPoint1, Math3D::vector3 NewPoint2, double fNewStep, double fNewRoll1, double fNewRoll2)
{ // wersja dla prostego - wyliczanie punktów kontrolnych
    Math3D::vector3 dir;

    // NOTE: we're enforcing division also for straight track, to ensure dense enough mesh for per-vertex lighting
/*
    if (fNewRoll1 == fNewRoll2)
    { // faktyczny prosty
        dir = Normalize(NewPoint2 - NewPoint1); // wektor kierunku o długości 1
        return TSegment::Init(NewPoint1, dir, -dir, NewPoint2, fNewStep, fNewRoll1, fNewRoll2,
                              false);
    }
    else
*/
    { // prosty ze zmienną przechyłką musi być segmentowany jak krzywe
        dir = (NewPoint2 - NewPoint1) / 3.0; // punkty kontrolne prostego są w 1/3 długości
        return TSegment::Init(
            NewPoint1, NewPoint1 + dir,
            NewPoint2 - dir, NewPoint2,
            fNewStep, fNewRoll1, fNewRoll2, true);
    }
};

bool TSegment::Init( Math3D::vector3 &NewPoint1, Math3D::vector3 NewCPointOut, Math3D::vector3 NewCPointIn, Math3D::vector3 &NewPoint2, double fNewStep, double fNewRoll1, double fNewRoll2, bool bIsCurve)
{ // wersja uniwersalna (dla krzywej i prostego)
    Point1 = NewPoint1;
    CPointOut = NewCPointOut;
    CPointIn = NewCPointIn;
    Point2 = NewPoint2;
    // poprawienie przechyłki
    fRoll1 = glm::radians(fNewRoll1); // Ra: przeliczone jest bardziej przydatne do obliczeń
    fRoll2 = glm::radians(fNewRoll2);
    bCurve = bIsCurve;
    if (Global.bRollFix)
    { // Ra: poprawianie przechyłki
        // Przechyłka powinna być na środku wewnętrznej szyny, a standardowo jest w osi
        // toru. Dlatego trzeba podnieść tor oraz odpowiednio podwyższyć podsypkę.
        // Nie wykonywać tej funkcji, jeśli podwyższenie zostało uwzględnione w edytorze.
        // Problematyczne mogą byc rozjazdy na przechyłce - lepiej je modelować w edytorze.
        // Na razie wszystkie scenerie powinny być poprawiane.
        // Jedynie problem będzie z podwójną rampą przechyłkową, która w środku będzie
        // mieć moment wypoziomowania, ale musi on być również podniesiony.
        if (fRoll1 != 0.0)
        { // tylko jeśli jest przechyłka
            double w1 = std::abs(std::sin(fRoll1) * 0.75); // 0.5*w2+0.0325; //0.75m dla 1.435
            Point1.y += w1; // modyfikacja musi być przed policzeniem dalszych parametrów
            if (bCurve)
                CPointOut.y += w1; // prosty ma wektory jednostkowe
            pOwner->MovedUp1(w1); // zwrócić trzeba informację o podwyższeniu podsypki
        }
        if (fRoll2 != 0.f)
        {
            double w2 = std::abs(std::sin(fRoll2) * 0.75); // 0.5*w2+0.0325; //0.75m dla 1.435
            Point2.y += w2; // modyfikacja musi być przed policzeniem dalszych parametrów
            if (bCurve)
                CPointIn.y += w2; // prosty ma wektory jednostkowe
            // zwrócić trzeba informację o podwyższeniu podsypki
        }
    }
    // kąt w planie, żeby nie liczyć wielokrotnie
    // Ra: ten kąt jeszcze do przemyślenia jest
    fDirection = -std::atan2(Point2.x - Point1.x, Point2.z - Point1.z);
    if (bCurve)
    { // przeliczenie współczynników wielomianu, będzie mniej mnożeń i można policzyć pochodne
        vC = 3.0 * (CPointOut - Point1); // t^1
        vB = 3.0 * (CPointIn - CPointOut) - vC; // t^2
        vA = Point2 - Point1 - vC - vB; // t^3
        fLength = ComputeLength();
    }
    else {
        fLength = ( Point1 - Point2 ).Length();
    }
    if (fLength <= 0) {

        ErrorLog( "Bad track: zero length spline \"" + pOwner->name() + "\" (location: " + to_string( glm::dvec3{ Point1 } ) + ")" );
        fLength = 0.01; // crude workaround TODO: fix this properly
    }

    fStoop = std::atan2((Point2.y - Point1.y), fLength); // pochylenie toru prostego, żeby nie liczyć wielokrotnie

    fStep = fNewStep;
    // NOTE: optionally replace this part with the commented version, after solving geometry issues with double switches
    if( ( pOwner->eType == tt_Switch )
     && ( fStep * ( 3.0 * Global.SplineFidelity ) > fLength ) ) {
        // NOTE: a workaround for too short switches (less than 3 segments) messing up animation/generation of blades
        fStep = fLength / ( 3.0 * Global.SplineFidelity );
    }
//    iSegCount = static_cast<int>( std::ceil( fLength / fStep ) ); // potrzebne do VBO
    iSegCount = (
        pOwner->eType == tt_Switch ?
            6 * Global.SplineFidelity :
            static_cast<int>( std::ceil( fLength / fStep ) ) ); // potrzebne do VBO

    fStep = fLength / iSegCount; // update step to equalize size of individual pieces

    fTsBuffer.resize( iSegCount + 1 );
    fTsBuffer[ 0 ] = 0.0;
    for( int i = 1; i < iSegCount; ++i ) {
        fTsBuffer[ i ] = GetTFromS( i * fStep );
    }
    fTsBuffer[ iSegCount ] = 1.0;

    return true;
}

Math3D::vector3 TSegment::GetFirstDerivative(double const fTime) const
{

    double fOmTime = 1.0 - fTime;
    double fPowTime = fTime;
    Math3D::vector3 kResult = fOmTime * (CPointOut - Point1);

    // int iDegreeM1 = 3 - 1;

    double fCoeff = 2 * fPowTime;
    kResult = (kResult + fCoeff * (CPointIn - CPointOut)) * fOmTime;
    fPowTime *= fTime;

    kResult += fPowTime * (Point2 - CPointIn);
    kResult *= 3;

    return kResult;
}

double TSegment::RombergIntegral(double const fA, double const fB) const
{
    double fH = fB - fA;

    const int ms_iOrder = 5;

    double ms_apfRom[2][ms_iOrder];

    ms_apfRom[0][0] =
        0.5 * fH * ((GetFirstDerivative(fA).Length()) + (GetFirstDerivative(fB).Length()));
    for (int i0 = 2, iP0 = 1; i0 <= ms_iOrder; i0++, iP0 *= 2, fH *= 0.5)
    {
        // approximations via the trapezoid rule
        double fSum = 0.0;
        int i1;
        for (i1 = 1; i1 <= iP0; i1++)
            fSum += (GetFirstDerivative(fA + fH * (i1 - 0.5)).Length());

        // Richardson extrapolation
        ms_apfRom[1][0] = 0.5 * (ms_apfRom[0][0] + fH * fSum);
        for (int i2 = 1, iP2 = 4; i2 < i0; i2++, iP2 *= 4)
        {
            ms_apfRom[1][i2] = (iP2 * ms_apfRom[1][i2 - 1] - ms_apfRom[0][i2 - 1]) / (iP2 - 1);
        }

        for (i1 = 0; i1 < i0; i1++)
            ms_apfRom[0][i1] = ms_apfRom[1][i1];
    }

    return ms_apfRom[0][ms_iOrder - 1];
}

double TSegment::GetTFromS(double const s) const
{
    // initial guess for Newton's method
    double fTolerance = 0.001;
    double fRatio = s / RombergIntegral(0, 1);
    double fTime = interpolate( 0.0, 1.0, fRatio );

    int iteration = 0;
    double fDifference {}; // exposed for debug down the road
	do {
        fDifference = RombergIntegral(0, fTime) - s;
		if( std::abs( fDifference ) < fTolerance ) {
			return fTime;
		}
        fTime -= fDifference / GetFirstDerivative(fTime).Length();
		++iteration;
	}
	while( iteration < 10 ); // arbitrary limit

    // Newton's method failed.  If this happens, increase iterations or
    // tolerance or integration accuracy.
    // return -1; //Ra: tu nigdy nie dojdzie
    ErrorLog( "Bad track: shape estimation failed for spline \"" + pOwner->name() + "\" (location: " + to_string( glm::dvec3{ Point1 } ) + ")" );
	// MessageBox(0,"Too many iterations","GetTFromS",MB_OK);
	return fTime;
};

Math3D::vector3 TSegment::RaInterpolate(double const t) const
{ // wyliczenie XYZ na krzywej Beziera z użyciem współczynników
    return t * (t * (t * vA + vB) + vC) + Point1; // 9 mnożeń, 9 dodawań
};

Math3D::vector3 TSegment::RaInterpolate0(double const t) const
{ // wyliczenie XYZ na krzywej Beziera, na użytek liczenia długości nie jest dodawane Point1
    return t * (t * (t * vA + vB) + vC); // 9 mnożeń, 6 dodawań
};

double TSegment::ComputeLength() const // McZapkie-150503: dlugosc miedzy punktami krzywej
{ // obliczenie długości krzywej Beziera za pomocą interpolacji odcinkami
    // Ra: zamienić na liczenie rekurencyjne średniej z cięciwy i łamanej po kontrolnych
    // Ra: koniec rekurencji jeśli po podziale suma długości nie różni się więcej niż 0.5mm od
    // poprzedniej
    // Ra: ewentualnie rozpoznać łuk okręgu płaskiego i liczyć ze wzoru na długość łuku
    double t, l = 0;
    Math3D::vector3 last = Math3D::vector3(0, 0, 0); // długość liczona po przesunięciu odcinka do początku układu
    Math3D::vector3 tmp = Point2 - Point1;
    int m = 20.0 * tmp.Length(); // było zawsze do 10000, teraz jest liczone odcinkami po około 5cm
    for (int i = 1; i <= m; i++)
    {
        t = double(i) / double(m); // wyznaczenie parametru na krzywej z przedziału (0,1>
        // tmp=Interpolate(t,p1,cp1,cp2,p2);
        tmp = RaInterpolate0(t); // obliczenie punktu dla tego parametru
        t = Math3D::vector3(tmp - last).Length(); // obliczenie długości wektora
        l += t; // zwiększenie wyliczanej długości
        last = tmp;
    }
    return (l);
}

// finds point on segment closest to specified point in 3d space. returns: point on segment as value in range 0-1
double
TSegment::find_nearest_point( glm::dvec3 const &Point ) const {

    if( ( false == bCurve ) || ( iSegCount == 1 ) ) {
        // for straight track just treat it as a single segment
        return
            nearest_segment_point(
                glm::dvec3{ FastGetPoint_0() },
                glm::dvec3{ FastGetPoint_1() },
                Point );
    }
    else {
        // for curves iterate through segment chunks, and find the one which gives us the least distance to the specified point
        double distance = std::numeric_limits<double>::max();
        double nearest;
        // NOTE: we're reusing already created segment chunks, which are created based on splinefidelity setting
        // this means depending on splinefidelity the results can be potentially slightly different
        for( int segmentidx = 0; segmentidx < iSegCount; ++segmentidx ) {

            auto const segmentpoint =
                clamp(
                    nearest_segment_point(
                        glm::dvec3{ FastGetPoint( fTsBuffer[ segmentidx ] ) },
                        glm::dvec3{ FastGetPoint( fTsBuffer[ segmentidx + 1 ] ) },
                        Point ) // point in range 0-1 on current segment
                    * ( fTsBuffer[ segmentidx + 1 ] - fTsBuffer[ segmentidx ] ) // segment length
                    + fTsBuffer[ segmentidx ], // segment offset
                    0.0, 1.0 ); // we clamp the range in case there's some floating point math inaccuracies

            auto const segmentdistance = glm::length2( Point - glm::dvec3{ FastGetPoint( segmentpoint ) } );
            if( segmentdistance < distance ) {

                nearest = segmentpoint;
                distance = segmentdistance;
            }
        }
        // 
        return nearest;
    }
}

const double fDirectionOffset = 0.1; // długość wektora do wyliczenia kierunku

Math3D::vector3 TSegment::GetDirection(double const fDistance) const
{ // takie toporne liczenie pochodnej dla podanego dystansu od Point1
    double t1 = GetTFromS(fDistance - fDirectionOffset);
    if (t1 <= 0.0)
        return (CPointOut - Point1); // na zewnątrz jako prosta
    double t2 = GetTFromS(fDistance + fDirectionOffset);
    if (t2 >= 1.0)
        return (Point1 - CPointIn); // na zewnątrz jako prosta
    return (FastGetPoint(t2) - FastGetPoint(t1));
}

Math3D::vector3 TSegment::FastGetDirection(double fDistance, double fOffset)
{ // takie toporne liczenie pochodnej dla parametru 0.0÷1.0
    double t1 = fDistance - fOffset;
    if (t1 <= 0.0)
        return (CPointOut - Point1); // wektor na początku jest stały
    double t2 = fDistance + fOffset;
    if (t2 >= 1.0)
        return (Point2 - CPointIn); // wektor na końcu jest stały
    return (FastGetPoint(t2) - FastGetPoint(t1));
}
/*
Math3D::vector3 TSegment::GetPoint(double const fDistance) const
{ // wyliczenie współrzędnych XYZ na torze w odległości (fDistance) od Point1
    if (bCurve)
    { // można by wprowadzić uproszczony wzór dla okręgów płaskich
        double t = GetTFromS(fDistance); // aproksymacja dystansu na krzywej Beziera
        // return Interpolate(t,Point1,CPointOut,CPointIn,Point2);
        return RaInterpolate(t);
    }
    else {
        // wyliczenie dla odcinka prostego jest prostsze
        return
            interpolate(
                Point1, Point2,
                clamp(
                    fDistance / fLength,
                    0.0, 1.0 ) );
    }
};
*/
// ustalenie pozycji osi na torze, przechyłki, pochylenia i kierunku jazdy
void TSegment::RaPositionGet(double const fDistance, Math3D::vector3 &p, Math3D::vector3 &a) const {

    if (bCurve) {
        // można by wprowadzić uproszczony wzór dla okręgów płaskich
        auto const t = GetTFromS(fDistance); // aproksymacja dystansu na krzywej Beziera na parametr (t)
        p = FastGetPoint( t );
        // przechyłka w danym miejscu (zmienia się liniowo)
        a.x = interpolate<double>( fRoll1, fRoll2, t );
        // pochodna jest 3*A*t^2+2*B*t+C
        auto const tangent = t * ( t * 3.0 * vA + vB + vB ) + vC;
        // pochylenie krzywej (w pionie)
        a.y = std::atan( tangent.y );
        // kierunek krzywej w planie
        a.z = -std::atan2( tangent.x, tangent.z );
    }
    else {
        // wyliczenie dla odcinka prostego jest prostsze
        auto const t = fDistance / fLength; // zerowych torów nie ma
        p = FastGetPoint( t );
        // przechyłka w danym miejscu (zmienia się liniowo)
        a.x = interpolate<double>( fRoll1, fRoll2, t );
        a.y = fStoop; // pochylenie toru prostego
        a.z = fDirection; // kierunek toru w planie
    }
};

Math3D::vector3 TSegment::FastGetPoint(double const t) const
{
    // return (bCurve?Interpolate(t,Point1,CPointOut,CPointIn,Point2):((1.0-t)*Point1+(t)*Point2));
    return (
        ( ( true == bCurve ) || ( iSegCount != 1 ) ) ?
            RaInterpolate( t ) :
            interpolate( Point1, Point2, t ) );
}

bool TSegment::RenderLoft( gfx::vertex_array &Output, Math3D::vector3 const &Origin, const gfx::vertex_array &ShapePoints, bool const Transition, double fTextureLength, double Texturescale, int iSkip, int iEnd, std::pair<float, float> fOffsetX, glm::vec3 **p, bool bRender)
{ // generowanie trójkątów dla odcinka trajektorii ruchu
    // standardowo tworzy triangle_strip dla prostego albo ich zestaw dla łuku
    // po modyfikacji - dla ujemnego (iNumShapePoints) w dodatkowych polach tabeli podany jest przekrój końcowy

    if( fTsBuffer.empty() )
        return false; // prowizoryczne zabezpieczenie przed wysypem - ustalić faktyczną przyczynę

    glm::vec3 pos1, pos2, dir, parallel1, parallel2, pt, norm;
    float s, step, fOffset, tv1, tv2, t, fEnd;
    auto const iNumShapePoints = Transition ? ShapePoints.size() / 2 : ShapePoints.size();
    float const texturelength = fTextureLength * Texturescale;
    float const texturescale = Texturescale;

    float m1, jmm1, m2, jmm2; // pozycje względne na odcinku 0...1 (ale nie parametr Beziera)
    step = fStep;
    tv1 = 1.0; // Ra: to by można było wyliczać dla odcinka, wyglądało by lepiej
    s = fStep * iSkip; // iSkip - ile odcinków z początku pominąć
    int i = iSkip; // domyślnie 0
    t = fTsBuffer[ i ]; // tabela wattości t dla segmentów
    // BUG: length of spline can be 0, we should skip geometry generation for such cases
    fOffset = 0.1 / fLength; // pierwsze 10cm
    pos1 = glm::dvec3{ FastGetPoint( t ) - Origin }; // wektor początku segmentu
    dir = glm::dvec3{ FastGetDirection( t, fOffset ) }; // wektor kierunku
    parallel1 = glm::vec3{ -dir.z, 0.f, dir.x }; // wektor poprzeczny
    if( glm::length2( parallel1 ) == 0.f ) {
        // temporary workaround for malformed situations with control points placed above endpoints
        parallel1 = glm::vec3{ glm::dvec3{ FastGetPoint_1() - FastGetPoint_0() } };
    }
    parallel1 = glm::normalize( parallel1 );
    if( iEnd == 0 )
        iEnd = iSegCount;
    fEnd = fLength * double( iEnd ) / double( iSegCount );
/*
    m2 = s / fEnd;
*/
    m2 = static_cast<float>( i - iSkip ) / ( iEnd - iSkip );

    jmm2 = 1.f - m2;

    while( i < iEnd ) {

        ++i; // kolejny punkt łamanej
        s += step; // końcowa pozycja segmentu [m]
        m1 = m2;
        jmm1 = jmm2; // stara pozycja
/*
        m2 = s / fEnd;
*/
        m2 = static_cast<float>( i - iSkip ) / ( iEnd - iSkip );

        jmm2 = 1.f - m2; // nowa pozycja
        if( i == iEnd ) { // gdy przekroczyliśmy koniec - stąd dziury w torach...
            step -= ( s - fEnd ); // jeszcze do wyliczenia mapowania potrzebny
            s = fEnd;
            m2 = 1.f;
            jmm2 = 0.f;
        }

        while( tv1 < 0.0 ) {
            tv1 += 1.0;
        }
        tv2 = tv1 - step / texturelength; // mapowanie na końcu segmentu

        t = fTsBuffer[ i ]; // szybsze od GetTFromS(s);
        pos2 = glm::dvec3{ FastGetPoint( t ) - Origin };
        dir = glm::dvec3{ FastGetDirection( t, fOffset ) }; // nowy wektor kierunku
        parallel2 = glm::vec3{ -dir.z, 0.f, dir.x }; // wektor poprzeczny
        if( glm::length2( parallel2 ) == 0.f ) {
            // temporary workaround for malformed situations with control points placed above endpoints
            parallel2 = glm::vec3{ glm::dvec3{ FastGetPoint_1() - FastGetPoint_0() } };
        }
        parallel2 = glm::normalize( parallel2 );

        // TODO: refactor the loop, there's no need to calculate starting points for each segment when we can copy the end points of the previous one
        if( Transition ) {
            for( int j = 0; j < iNumShapePoints; ++j ) {
                pt = parallel1 * ( jmm1 * ( ShapePoints[ j ].position.x - fOffsetX.first ) + m1 * ( ShapePoints[ j + iNumShapePoints ].position.x - fOffsetX.second ) ) + pos1;
                pt.y += jmm1 * ShapePoints[ j ].position.y + m1 * ShapePoints[ j + iNumShapePoints ].position.y;
                norm = ( jmm1 * ShapePoints[ j ].normal.x + m1 * ShapePoints[ j + iNumShapePoints ].normal.x ) * parallel1;
                norm.y += jmm1 * ShapePoints[ j ].normal.y + m1 * ShapePoints[ j + iNumShapePoints ].normal.y;
                if( bRender ) {
                    // skrzyżowania podczas łączenia siatek mogą nie renderować poboczy, ale potrzebować punktów
                    Output.emplace_back(
                        pt,
                        glm::normalize( norm ),
                        glm::vec2 { ( jmm1 * ShapePoints[ j ].texture.x + m1 * ShapePoints[ j + iNumShapePoints ].texture.x ) / texturescale, tv1 } );
                }
                if( p ) // jeśli jest wskaźnik do tablicy
                    if( *p )
                        if( !j ) // to dla pierwszego punktu
                        {
                            *( *p ) = pt;
                            ( *p )++;
                        } // zapamiętanie brzegu jezdni
                // dla trapezu drugi koniec ma inne współrzędne
                pt = parallel2 * ( jmm2 * ( ShapePoints[ j ].position.x - fOffsetX.first ) + m2 * ( ShapePoints[ j + iNumShapePoints ].position.x - fOffsetX.second ) ) + pos2;
                pt.y += jmm2 * ShapePoints[ j ].position.y + m2 * ShapePoints[ j + iNumShapePoints ].position.y;
                norm = ( jmm2 * ShapePoints[ j ].normal.x + m2 * ShapePoints[ j + iNumShapePoints ].normal.x ) * parallel2;
                norm.y += jmm2 * ShapePoints[ j ].normal.y + m2 * ShapePoints[ j + iNumShapePoints ].normal.y;
                if( bRender ) {
                    // skrzyżowania podczas łączenia siatek mogą nie renderować poboczy, ale potrzebować punktów
                    Output.emplace_back(
                        pt,
                        glm::normalize( norm ),
                        glm::vec2 { ( jmm2 * ShapePoints[ j ].texture.x + m2 * ShapePoints[ j + iNumShapePoints ].texture.x ) / texturescale, tv2 } );
                }
                if( p ) // jeśli jest wskaźnik do tablicy
                    if( *p )
                        if( !j ) // to dla pierwszego punktu
                            if( i == iSegCount ) {
                                *( *p ) = pt;
                                ( *p )++;
                            } // zapamiętanie brzegu jezdni
            }
        }
        else {
            if( bRender  ) {
                for( int j = 0; j < iNumShapePoints; ++j ) {
                    //łuk z jednym profilem
                    pt = parallel1 * ( jmm1 * ( ShapePoints[ j ].position.x - fOffsetX.first ) + m1 * ( ShapePoints[ j ].position.x - fOffsetX.second ) ) + pos1;
                    pt.y += ShapePoints[ j ].position.y;
                    norm = ShapePoints[ j ].normal.x * parallel1;
                    norm.y += ShapePoints[ j ].normal.y;

                    Output.emplace_back(
                        pt,
                        glm::normalize( norm ),
                        glm::vec2 { ShapePoints[ j ].texture.x / texturescale, tv1 } );

                    pt = parallel2 * ( jmm2 * ( ShapePoints[ j ].position.x - fOffsetX.first ) + m2 * ( ShapePoints[ j ].position.x - fOffsetX.second ) ) + pos2;
                    pt.y += ShapePoints[ j ].position.y;
                    norm = ShapePoints[ j ].normal.x * parallel2;
                    norm.y += ShapePoints[ j ].normal.y;

                    Output.emplace_back(
                        pt,
                        glm::normalize( norm ),
                        glm::vec2 { ShapePoints[ j ].texture.x / texturescale, tv2 } );
                }
            }
        }
        pos1 = pos2;
        parallel1 = parallel2;
        tv1 = tv2;
    }
    return true;
};

