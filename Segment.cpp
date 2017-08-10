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
#include "Usefull.h"
#include "Track.h"

//---------------------------------------------------------------------------

// 101206 Ra: trapezoidalne drogi
// 110806 Ra: odwrócone mapowanie wzdłuż - Point1 == 1.0

TSegment::TSegment(TTrack *owner) :
    pOwner( owner )
{
    fAngle[ 0 ] = 0.0;
    fAngle[ 1 ] = 0.0;
};

TSegment::~TSegment()
{
    SafeDeleteArray(fTsBuffer);
};

bool TSegment::Init(vector3 NewPoint1, vector3 NewPoint2, double fNewStep, double fNewRoll1, double fNewRoll2)
{ // wersja dla prostego - wyliczanie punktów kontrolnych
    vector3 dir;

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

bool TSegment::Init(vector3 &NewPoint1, vector3 NewCPointOut, vector3 NewCPointIn,
                    vector3 &NewPoint2, double fNewStep, double fNewRoll1, double fNewRoll2,
                    bool bIsCurve)
{ // wersja uniwersalna (dla krzywej i prostego)
    Point1 = NewPoint1;
    CPointOut = NewCPointOut;
    CPointIn = NewCPointIn;
    Point2 = NewPoint2;
    // poprawienie przechyłki
    fRoll1 = DegToRad(fNewRoll1); // Ra: przeliczone jest bardziej przydatne do obliczeń
    fRoll2 = DegToRad(fNewRoll2);
    if (Global::bRollFix)
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
            double w1 = fabs(sin(fRoll1) * 0.75); // 0.5*w2+0.0325; //0.75m dla 1.435
            Point1.y += w1; // modyfikacja musi być przed policzeniem dalszych parametrów
            if (bCurve)
                CPointOut.y += w1; // prosty ma wektory jednostkowe
            pOwner->MovedUp1(w1); // zwrócić trzeba informację o podwyższeniu podsypki
        }
        if (fRoll2 != 0.0)
        {
            double w2 = fabs(sin(fRoll2) * 0.75); // 0.5*w2+0.0325; //0.75m dla 1.435
            Point2.y += w2; // modyfikacja musi być przed policzeniem dalszych parametrów
            if (bCurve)
                CPointIn.y += w2; // prosty ma wektory jednostkowe
            // zwrócić trzeba informację o podwyższeniu podsypki
        }
    }
    // Ra: ten kąt jeszcze do przemyślenia jest
    fDirection = -atan2(Point2.x - Point1.x,
                        Point2.z - Point1.z); // kąt w planie, żeby nie liczyć wielokrotnie
    bCurve = bIsCurve;
    if (bCurve)
    { // przeliczenie współczynników wielomianu, będzie mniej mnożeń i można policzyć pochodne
        vC = 3.0 * (CPointOut - Point1); // t^1
        vB = 3.0 * (CPointIn - CPointOut) - vC; // t^2
        vA = Point2 - Point1 - vC - vB; // t^3
        fLength = ComputeLength();
    }
    else
        fLength = (Point1 - Point2).Length();
    fStep = fNewStep;
    if (fLength <= 0)
    {
        ErrorLog( "Bad geometry: zero length spline \"" + pOwner->NameGet() + "\" (location: " + to_string( glm::dvec3{ Point1 } ) + ")" );
        // MessageBox(0,"Length<=0","TSegment::Init",MB_OK);
        fLength = 0.01; // crude workaround TODO: fix this properly
/*
        return false; // zerowe nie mogą być
*/
    }

    if( ( pOwner->eType == tt_Switch )
     && ( fStep * 3.0 > fLength ) ) {
        // NOTE: a workaround for too short switches (less than 3 segments) messing up animation/generation of blades
        fStep = fLength / 3.0;
    }

    fStoop = std::atan2((Point2.y - Point1.y), fLength); // pochylenie toru prostego, żeby nie liczyć wielokrotnie
    SafeDeleteArray(fTsBuffer);

    iSegCount = static_cast<int>( std::ceil( fLength / fStep ) ); // potrzebne do VBO
    fStep = fLength / iSegCount; // update step to equalize size of individual pieces
    fTsBuffer = new double[ iSegCount + 1 ];
    fTsBuffer[ 0 ] = 0.0;
    for( int i = 1; i < iSegCount; ++i ) {
        fTsBuffer[ i ] = GetTFromS( i * fStep );
    }
    fTsBuffer[ iSegCount ] = 1.0;

    return true;
}

vector3 TSegment::GetFirstDerivative(double fTime)
{

    double fOmTime = 1.0 - fTime;
    double fPowTime = fTime;
    vector3 kResult = fOmTime * (CPointOut - Point1);

    // int iDegreeM1 = 3 - 1;

    double fCoeff = 2 * fPowTime;
    kResult = (kResult + fCoeff * (CPointIn - CPointOut)) * fOmTime;
    fPowTime *= fTime;

    kResult += fPowTime * (Point2 - CPointIn);
    kResult *= 3;

    return kResult;
}

double TSegment::RombergIntegral(double fA, double fB)
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

double TSegment::GetTFromS(double s)
{
    // initial guess for Newton's method
    double fTolerance = 0.001;
    double fRatio = s / RombergIntegral(0, 1);
    double fOmRatio = 1.0 - fRatio;
    double fTime = fOmRatio * 0 + fRatio * 1;
	int iteration = 0;

	do {
        double fDifference = RombergIntegral(0, fTime) - s;
		if( ( fDifference > 0 ? fDifference : -fDifference ) < fTolerance ) {
			return fTime;
		}

        fTime -= fDifference / GetFirstDerivative(fTime).Length();
		++iteration;
	}
	while( iteration < 10 ); // arbitrary limit

    // Newton's method failed.  If this happens, increase iterations or
    // tolerance or integration accuracy.
    // return -1; //Ra: tu nigdy nie dojdzie
    ErrorLog( "Bad geometry: shape estimation failed for spline \"" + pOwner->NameGet() + "\" (location: " + to_string( glm::dvec3{ Point1 } ) + ")" );
	// MessageBox(0,"Too many iterations","GetTFromS",MB_OK);
	return fTime;
};

vector3 TSegment::RaInterpolate(double t)
{ // wyliczenie XYZ na krzywej Beziera z użyciem współczynników
    return t * (t * (t * vA + vB) + vC) + Point1; // 9 mnożeń, 9 dodawań
};

vector3 TSegment::RaInterpolate0(double t)
{ // wyliczenie XYZ na krzywej Beziera, na użytek liczenia długości nie jest dodawane Point1
    return t * (t * (t * vA + vB) + vC); // 9 mnożeń, 6 dodawań
};

double TSegment::ComputeLength() // McZapkie-150503: dlugosc miedzy punktami krzywej
{ // obliczenie długości krzywej Beziera za pomocą interpolacji odcinkami
    // Ra: zamienić na liczenie rekurencyjne średniej z cięciwy i łamanej po kontrolnych
    // Ra: koniec rekurencji jeśli po podziale suma długości nie różni się więcej niż 0.5mm od
    // poprzedniej
    // Ra: ewentualnie rozpoznać łuk okręgu płaskiego i liczyć ze wzoru na długość łuku
    double t, l = 0;
    vector3 last = vector3(0, 0, 0); // długość liczona po przesunięciu odcinka do początku układu
    vector3 tmp = Point2 - Point1;
    int m = 20.0 * tmp.Length(); // było zawsze do 10000, teraz jest liczone odcinkami po około 5cm
    for (int i = 1; i <= m; i++)
    {
        t = double(i) / double(m); // wyznaczenie parametru na krzywej z przedziału (0,1>
        // tmp=Interpolate(t,p1,cp1,cp2,p2);
        tmp = RaInterpolate0(t); // obliczenie punktu dla tego parametru
        t = vector3(tmp - last).Length(); // obliczenie długości wektora
        l += t; // zwiększenie wyliczanej długości
        last = tmp;
    }
    return (l);
}

const double fDirectionOffset = 0.1; // długość wektora do wyliczenia kierunku

vector3 TSegment::GetDirection(double fDistance)
{ // takie toporne liczenie pochodnej dla podanego dystansu od Point1
    double t1 = GetTFromS(fDistance - fDirectionOffset);
    if (t1 <= 0.0)
        return (CPointOut - Point1); // na zewnątrz jako prosta
    double t2 = GetTFromS(fDistance + fDirectionOffset);
    if (t2 >= 1.0)
        return (Point1 - CPointIn); // na zewnątrz jako prosta
    return (FastGetPoint(t2) - FastGetPoint(t1));
}

vector3 TSegment::FastGetDirection(double fDistance, double fOffset)
{ // takie toporne liczenie pochodnej dla parametru 0.0÷1.0
    double t1 = fDistance - fOffset;
    if (t1 <= 0.0)
        return (CPointOut - Point1); // wektor na początku jest stały
    double t2 = fDistance + fOffset;
    if (t2 >= 1.0)
        return (Point2 - CPointIn); // wektor na końcu jest stały
    return (FastGetPoint(t2) - FastGetPoint(t1));
}

vector3 TSegment::GetPoint(double fDistance)
{ // wyliczenie współrzędnych XYZ na torze w odległości (fDistance) od Point1
    if (bCurve)
    { // można by wprowadzić uproszczony wzór dla okręgów płaskich
        double t = GetTFromS(fDistance); // aproksymacja dystansu na krzywej Beziera
        // return Interpolate(t,Point1,CPointOut,CPointIn,Point2);
        return RaInterpolate(t);
    }
    else
    { // wyliczenie dla odcinka prostego jest prostsze
        double t = fDistance / fLength; // zerowych torów nie ma
        return ((1.0 - t) * Point1 + (t)*Point2);
    }
};

void TSegment::RaPositionGet(double fDistance, vector3 &p, vector3 &a)
{ // ustalenie pozycji osi na torze, przechyłki, pochylenia i kierunku jazdy
    if (bCurve)
    { // można by wprowadzić uproszczony wzór dla okręgów płaskich
        double t = GetTFromS(fDistance); // aproksymacja dystansu na krzywej Beziera na parametr (t)
        p = RaInterpolate(t);
        a.x = (1.0 - t) * fRoll1 + (t)*fRoll2; // przechyłka w danym miejscu (zmienia się liniowo)
        // pochodna jest 3*A*t^2+2*B*t+C
        a.y = atan(t * (t * 3.0 * vA.y + vB.y + vB.y) + vC.y); // pochylenie krzywej (w pionie)
        a.z = -atan2(t * (t * 3.0 * vA.x + vB.x + vB.x) + vC.x,
                     t * (t * 3.0 * vA.z + vB.z + vB.z) + vC.z); // kierunek krzywej w planie
    }
    else
    { // wyliczenie dla odcinka prostego jest prostsze
        double t = fDistance / fLength; // zerowych torów nie ma
        p = ((1.0 - t) * Point1 + (t)*Point2);
        a.x = (1.0 - t) * fRoll1 + (t)*fRoll2; // przechyłka w danym miejscu (zmienia się liniowo)
        a.y = fStoop; // pochylenie toru prostego
        a.z = fDirection; // kierunek toru w planie
    }
};

vector3 TSegment::FastGetPoint(double t)
{
    // return (bCurve?Interpolate(t,Point1,CPointOut,CPointIn,Point2):((1.0-t)*Point1+(t)*Point2));
    return (bCurve ? RaInterpolate(t) : ((1.0 - t) * Point1 + (t)*Point2));
}

bool TSegment::RenderLoft( vertex_array &Output, Math3D::vector3 const &Origin, const vector6 *ShapePoints, int iNumShapePoints, double fTextureLength, double Texturescale, int iSkip, int iEnd, double fOffsetX, vector3 **p, bool bRender)
{ // generowanie trójkątów dla odcinka trajektorii ruchu
    // standardowo tworzy triangle_strip dla prostego albo ich zestaw dla łuku
    // po modyfikacji - dla ujemnego (iNumShapePoints) w dodatkowych polach tabeli
    // podany jest przekrój końcowy
    // podsypka toru jest robiona za pomocą 6 punktów, szyna 12, drogi i rzeki na 3+2+3

    if( !fTsBuffer )
        return false; // prowizoryczne zabezpieczenie przed wysypem - ustalić faktyczną przyczynę

    vector3 pos1, pos2, dir, parallel1, parallel2, pt, norm;
    double s, step, fOffset, tv1, tv2, t, fEnd;
    bool const trapez = iNumShapePoints < 0; // sygnalizacja trapezowatości
    iNumShapePoints = std::abs( iNumShapePoints );
    fTextureLength *= Texturescale;

    double m1, jmm1, m2, jmm2; // pozycje względne na odcinku 0...1 (ale nie parametr Beziera)
    step = fStep;
    tv1 = 1.0; // Ra: to by można było wyliczać dla odcinka, wyglądało by lepiej
    s = fStep * iSkip; // iSkip - ile odcinków z początku pominąć
    int i = iSkip; // domyślnie 0
    t = fTsBuffer[ i ]; // tabela wattości t dla segmentów
    // BUG: length of spline can be 0, we should skip geometry generation for such cases
    fOffset = 0.1 / fLength; // pierwsze 10cm
    pos1 = FastGetPoint( t ); // wektor początku segmentu
    dir = FastGetDirection( t, fOffset ); // wektor kierunku
    parallel1 = Normalize( vector3( -dir.z, 0.0, dir.x ) ); // wektor poprzeczny
    if( iEnd == 0 )
        iEnd = iSegCount;
    fEnd = fLength * double( iEnd ) / double( iSegCount );
    m2 = s / fEnd;
    jmm2 = 1.0 - m2;

    while( i < iEnd ) {

        ++i; // kolejny punkt łamanej
        s += step; // końcowa pozycja segmentu [m]
        m1 = m2;
        jmm1 = jmm2; // stara pozycja
        m2 = s / fEnd;
        jmm2 = 1.0 - m2; // nowa pozycja
        if( i == iEnd ) { // gdy przekroczyliśmy koniec - stąd dziury w torach...
            step -= ( s - fEnd ); // jeszcze do wyliczenia mapowania potrzebny
            s = fEnd;
            m2 = 1.0;
            jmm2 = 0.0;
        }

        while( tv1 < 0.0 ) {
            tv1 += 1.0;
        }
        tv2 = tv1 - step / fTextureLength; // mapowanie na końcu segmentu

        t = fTsBuffer[ i ]; // szybsze od GetTFromS(s);
        pos2 = FastGetPoint( t );
        dir = FastGetDirection( t, fOffset ); // nowy wektor kierunku
        parallel2 = Normalize( vector3( -dir.z, 0.0, dir.x ) ); // wektor poprzeczny

        if( trapez ) {
            for( int j = 0; j < iNumShapePoints; ++j ) {
                pt = parallel1 * ( jmm1 * ( ShapePoints[ j ].x - fOffsetX ) + m1 * ShapePoints[ j + iNumShapePoints ].x ) + pos1;
                pt.y += jmm1 * ShapePoints[ j ].y + m1 * ShapePoints[ j + iNumShapePoints ].y;
                pt -= Origin;
                norm = ( jmm1 * ShapePoints[ j ].n.x + m1 * ShapePoints[ j + iNumShapePoints ].n.x ) * parallel1;
                norm.y += jmm1 * ShapePoints[ j ].n.y + m1 * ShapePoints[ j + iNumShapePoints ].n.y;
                if( bRender ) {
                    // skrzyżowania podczas łączenia siatek mogą nie renderować poboczy, ale potrzebować punktów
                    Output.emplace_back(
                        glm::vec3 { pt.x, pt.y, pt.z },
                        glm::vec3 { norm.x, norm.y, norm.z },
                        glm::vec2 { ( jmm1 * ShapePoints[ j ].z + m1 * ShapePoints[ j + iNumShapePoints ].z ) / Texturescale, tv1 } );
                }
                if( p ) // jeśli jest wskaźnik do tablicy
                    if( *p )
                        if( !j ) // to dla pierwszego punktu
                        {
                            *( *p ) = pt;
                            ( *p )++;
                        } // zapamiętanie brzegu jezdni
                // dla trapezu drugi koniec ma inne współrzędne
                pt = parallel2 * ( jmm2 * ( ShapePoints[ j ].x - fOffsetX ) + m2 * ShapePoints[ j + iNumShapePoints ].x ) + pos2;
                pt.y += jmm2 * ShapePoints[ j ].y + m2 * ShapePoints[ j + iNumShapePoints ].y;
                pt -= Origin;
                norm = ( jmm1 * ShapePoints[ j ].n.x + m1 * ShapePoints[ j + iNumShapePoints ].n.x ) * parallel2;
                norm.y += jmm1 * ShapePoints[ j ].n.y + m1 * ShapePoints[ j + iNumShapePoints ].n.y;
                if( bRender ) {
                    // skrzyżowania podczas łączenia siatek mogą nie renderować poboczy, ale potrzebować punktów
                    Output.emplace_back(
                        glm::vec3 { pt.x, pt.y, pt.z },
                        glm::vec3 { norm.x, norm.y, norm.z },
                        glm::vec2 { ( jmm2 * ShapePoints[ j ].z + m2 * ShapePoints[ j + iNumShapePoints ].z ) / Texturescale, tv2 } );
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
                    pt = parallel1 * ( ShapePoints[ j ].x - fOffsetX ) + pos1;
                    pt.y += ShapePoints[ j ].y;
                    pt -= Origin;
                    norm = ShapePoints[ j ].n.x * parallel1;
                    norm.y += ShapePoints[ j ].n.y;

                    Output.emplace_back(
                        glm::vec3 { pt.x, pt.y, pt.z },
                        glm::vec3 { norm.x, norm.y, norm.z },
                        glm::vec2 { ShapePoints[ j ].z / Texturescale, tv1 } );

                    pt = parallel2 * ShapePoints[ j ].x + pos2;
                    pt.y += ShapePoints[ j ].y;
                    pt -= Origin;
                    norm = ShapePoints[ j ].n.x * parallel2;
                    norm.y += ShapePoints[ j ].n.y;

                    Output.emplace_back(
                        glm::vec3 { pt.x, pt.y, pt.z },
                        glm::vec3 { norm.x, norm.y, norm.z },
                        glm::vec2 { ShapePoints[ j ].z / Texturescale, tv2 } );
                }
            }
        }
        pos1 = pos2;
        parallel1 = parallel2;
        tv1 = tv2;
    }
    return true;
};

void TSegment::Render()
{
    vector3 pt;
    GfxRenderer.Bind_Material( NULL );

    if (bCurve)
    {
        glColor3f(0, 0, 1.0f);
        glBegin(GL_LINE_STRIP);
        glVertex3f(Point1.x, Point1.y, Point1.z);
        glVertex3f(CPointOut.x, CPointOut.y, CPointOut.z);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex3f(Point2.x, Point2.y, Point2.z);
        glVertex3f(CPointIn.x, CPointIn.y, CPointIn.z);
        glEnd();

        glColor3f(1.0f, 0, 0);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 8; i++)
        {
            pt = FastGetPoint(double(i) / 8.0f);
            glVertex3f(pt.x, pt.y, pt.z);
        }
        glEnd();
    }
    else
    {
        glColor3f(0, 0, 1.0f);
        glBegin(GL_LINE_STRIP);
        glVertex3f(Point1.x, Point1.y, Point1.z);
        glVertex3f(Point1.x + CPointOut.x, Point1.y + CPointOut.y, Point1.z + CPointOut.z);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex3f(Point2.x, Point2.y, Point2.z);
        glVertex3f(Point2.x + CPointIn.x, Point2.y + CPointIn.y, Point2.z + CPointIn.z);
        glEnd();

        glColor3f(0.5f, 0, 0);
        glBegin(GL_LINE_STRIP);
        glVertex3f(Point1.x + CPointOut.x, Point1.y + CPointOut.y, Point1.z + CPointOut.z);
        glVertex3f(Point2.x + CPointIn.x, Point2.y + CPointIn.y, Point2.z + CPointIn.z);
        glEnd();
    }
}

//---------------------------------------------------------------------------
