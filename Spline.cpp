//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Geometry.h"
#include "Spline.h"
#include "usefull.h"
#include "maptextfile.hpp"

//#define asSplinesPatch AnsiString("Scenery\\")

bool Connect(TKnot *k1, TKnot *k2)
{
    if (k1 && k2)
    {
        k1->Next = k2;
        k2->Prev = k1;
        k1->InitLength();
        return true;
    }
    else
        return false;
}

#define Precision 10000
float CurveLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
{
    float t, l = 0;
    vector3 tmp, last = p1;
    for (int i = 1; i <= Precision; i++)
    {
        t = float(i) / float(Precision);
        tmp = Interpolate(t, p1, cp1, cp2, p2);
        t = vector3(tmp - last).Length();
        l += t;
        last = tmp;
    }
    return (l);
}

__fastcall TKnot::TKnot()
{
    Next = Prev = NULL;
    Length = -1;
    IsCurve = false;
    fLengthIn = fLengthOut = 0;
    fRoll = 0;
    bSwitchDirectionForward = false;
    bSwitchDirectionBackward = false;
}

__fastcall TKnot::TKnot(int n)
{
    bSwitchDirectionForward = false;
    bSwitchDirectionBackward = false;
    Next = Prev = NULL;
    Length = -1;
    IsCurve = false;
    fLengthIn = fLengthOut = 0;
    if (n > 0)
    {
        Next = new TKnot(n - 1);
        Next->Prev = this;
    }
}

__fastcall TKnot::~TKnot() {}

vector3 TKnot::GetDirection(float t)
{
    if (IsCurve)
    {
        vector3 v1 = CPointOut - Point;
        vector3 v2 = Next->Point - Next->CPointIn; // BUGBUG
        return v1 * (1 - t) + v2 * (t);
    }
    else
        return (Next->Point -
                Point); // Spline->Knots[KnotIndex].Point-Spline->Knots[KnotIndex].Point);
}

float TKnot::GetTFromS(float s)
{
    // initial guess for Newton's method
    int it = 0;
    float fTolerance = 0.001;
    float fRatio = s / RombergIntegral(0, 1);
    float fOmRatio = 1.0 - fRatio;
    float fTime = fOmRatio * 0 + fRatio * 1;

    //    for (int i = 0; i < iIterations; i++)
    while (true)
    {
        it++;
        if (it > 10)
            MessageBox(0, "Too many iterations", "TSpline->GetTFromS", MB_OK);

        float fDifference = RombergIntegral(0, fTime) - s;
        if ((fDifference > 0 ? fDifference : -fDifference) < fTolerance)
            return fTime;

        fTime -= fDifference / GetFirstDerivative(fTime).Length();
    }

    // Newton's method failed.  If this happens, increase iterations or
    // tolerance or integration accuracy.
    return -1;
}

bool TKnot::Init(TKnot *NNext, TKnot *NPrev)
{
    if (NNext != NULL)
    {
        Next = NNext;
        Next->Prev = this;
    }
    if (NPrev != NULL)
    {
        Prev = NPrev;
        Prev->Next = this;
    }
}

bool TKnot::InitCPoints()
{
    if (Prev != NULL)
        if (Prev->IsCurve)
            if (Next != NULL)
            {
                if (!IsCurve)
                {
                    CPointIn = Point - fLengthIn * Normalize(Next->Point - Point);
                }
                else
                    CPointIn = Point -
                               fLengthIn * (Normalize(Normalize(Next->Point - Point) +
                                                      Normalize(Point - Prev->Point)));
            }
            else
                CPointIn = Point - fLengthIn * Normalize(Point - Prev->Point);
        else
            CPointIn = Prev->Point;

    if (Next != NULL)
        if (IsCurve)
            if (Prev != NULL)
            {
                if (!Prev->IsCurve)
                {
                    CPointOut = Point - fLengthOut * Normalize(Prev->Point - Point);
                }
                else
                    CPointOut = Point -
                                fLengthOut * (Normalize(Normalize(Prev->Point - Point) +
                                                        Normalize(Point - Next->Point)));
            }
            else
                ;
        //           CPointOut= -fLengthOut*Normalize(Next->Point-Point);
        else
            CPointOut = Next->Point;
    else
        CPointOut = Point - fLengthOut * Normalize(Prev->Point - Point);
}

bool TKnot::InitLength()
{

    if (IsCurve)
        if (Next)
            Length = RombergIntegral(0, 1);
        else
            ;
    else if (Next)
        Length = (Point - Next->Point).Length();

    return true;
}

vector3 TKnot::GetFirstDerivative(float fTime)
{

    float fOmTime = 1.0 - fTime;
    float fPowTime = fTime;
    vector3 kResult = fOmTime * (CPointOut - Point);

    int iDegreeM1 = 3 - 1;

    float fCoeff = 2 * fPowTime;
    kResult = (kResult + fCoeff * (Next->CPointIn - CPointOut)) * fOmTime;
    fPowTime *= fTime;

    kResult += fPowTime * (Next->Point - Next->CPointIn);
    kResult *= 3;

    return kResult;
}

float TKnot::RombergIntegral(float fA, float fB)
{
    float fH = fB - fA;

    const int ms_iOrder = 5;

    float ms_apfRom[2][ms_iOrder];

    ms_apfRom[0][0] =
        0.5 * fH * ((GetFirstDerivative(fA).Length()) + (GetFirstDerivative(fB).Length()));
    for (int i0 = 2, iP0 = 1; i0 <= ms_iOrder; i0++, iP0 *= 2, fH *= 0.5)
    {
        // approximations via the trapezoid rule
        float fSum = 0.0;
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

//----------------------------------------------------------------------------//

__fastcall TSpline::TSpline()
{
    //    Closed= true;
    //    asName= "foo";
    //    Knots= NULL;
    RootKnot = NULL;
    iNumKnots = 0;
    KnotsAllocated = false;
    //    Next=Prev= this;
}

__fastcall TSpline::TSpline(AnsiString asNName)
{
    //    Closed= true;
    //    asName= asNName;
    //    Knots= NULL;
    RootKnot = NULL;
    iNumKnots = 0;
    KnotsAllocated = false;
    //    Next=Prev= this;
}

__fastcall TSpline::~TSpline()
{
    //  if (KnotsAllocated)
    //        SafeDeleteArray(Knots);

    //    SafeDelete(RootKnot);

    TKnot *ck, *tk;
    ck = RootKnot;

    for (int i = 0; i < iNumKnots; i++)
    {
        tk = ck;
        ck = ck->Next;
        SafeDelete(tk);
    }
}

bool TSpline::Create(int n, AnsiString asNName)
{
    /*
    //    asName= asNName;
        iNumKnots= n;
        Knots= new TKnot[iNumKnots];
        KnotsAllocated= true;
        Knots[0].Prev= NULL;
        Knots[iNumKnots-1].Next= NULL;
        for (int i=1; i<iNumKnots; i++)
            Knots[i].Init(NULL,Knots+i-1);
      */
}

bool TSpline::AssignKnots(TKnot *FirstKnot, int n)
{
    //    iNumKnots= n;
    //    Knots= FirstKnot;
    //    KnotsAllocated= false;
}

int TSpline::LoadFromFile(AnsiString FileName, TKnot *FirstKnot)
{
    return false;
    /*
        int i;

        iNumKnots= -1;


        AnsiString buf;
        buf.SetLength(255);

        TMapTextfile *tf= new TMapTextfile();
        tf->Open(asSceneryPatch+FileName,mmRead);

        tf->ReadLn(buf);

        if (buf==AnsiString("\"SPLINE\""))
        {
            tf->ReadLn(buf);
            if (FirstKnot==NULL)
                Create(buf.ToInt());
            else
                AssignKnots(FirstKnot,buf.ToInt());
            TKnot *CurrentKnot= Knots;
    //        iNumKnots= buf.ToInt();
      //      Knots= new TKnot[iNumKnots];// malloc(*sizeof(TKnot));

            DecimalSeparator= '.';
            for (i=0; i<iNumKnots; i++)
            {

                tf->ReadLn(buf); CurrentKnot->Point.x= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->Point.z= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->Point.y= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->CPointIn.x= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->CPointIn.z= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->CPointIn.y= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->CPointOut.x= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->CPointOut.z= buf.ToDouble();
                tf->ReadLn(buf); CurrentKnot->CPointOut.y= buf.ToDouble();

                    tf->ReadLn(buf);
    //            buf.Trim();
                    Knots[i].IsCurve= (buf!="#line ");
                CurrentKnot= CurrentKnot->Next;


            }
            DecimalSeparator= ',';

            CurrentKnot= Knots;

            for (i=0; i<iNumKnots-1; i++)
            {
    //            Knots[i].InitLength();
                if (CurrentKnot->IsCurve)
                {

                    CurrentKnot->Length=
    CurveLength(CurrentKnot->Point,CurrentKnot->CPointOut,CurrentKnot->Next->CPointIn,CurrentKnot->Next->Point);
                }
                else
                {
                    CurrentKnot->CPointOut= 2*CurrentKnot->Point-CurrentKnot->CPointIn;
                    CurrentKnot->Next->CPointIn= CurrentKnot->Point;
                    CurrentKnot->Length= (CurrentKnot->Point-CurrentKnot->Next->Point).Length();
                }
                CurrentKnot= CurrentKnot->Next;
            }
            CurrentKnot->Length= -1;
        }



        tf->Close();

        delete tf;

        return (iNumKnots);
      */
}

int TSpline::Load(TQueryParserComp *Parser, AnsiString asEndString)
{
    TKnot *LastKnot = NULL;
    ;
    if (RootKnot != NULL)
        for (LastKnot = RootKnot; LastKnot->Next != NULL; LastKnot = LastKnot->Next)
            ;

    TKnot *tmp;

    tmp = new TKnot(0);
    tmp->Prev = LastKnot;
    if (LastKnot != NULL)
        LastKnot->Next = tmp;
    else
        RootKnot = tmp;

    LastKnot = tmp;

    float tf, r, pr;
    vector3 dir;

    //    asName= Parser->GetNextSymbol();

    iNumKnots = 0;

    do
    {
        LastKnot->fLengthIn = Parser->GetNextSymbol().ToDouble();
        tf = Parser->GetNextSymbol().ToDouble();
        LastKnot->Point.x = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        LastKnot->Point.y = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        LastKnot->Point.z = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        LastKnot->fRoll = tf / 180 * M_PI;

        LastKnot->fLengthOut = Parser->GetNextSymbol().ToDouble();

        LastKnot->IsCurve = (LastKnot->fLengthOut != 0);

        LastKnot->Next = new TKnot(0);
        LastKnot->Next->Prev = LastKnot;
        LastKnot = LastKnot->Next;

        iNumKnots++;

    } while (Parser->GetNextSymbol().LowerCase() != asEndString && !Parser->EOF);

    LastKnot->Prev->Next = NULL;
    delete LastKnot;
    if (RootKnot != NULL)
        for (LastKnot = RootKnot; LastKnot != NULL; LastKnot = LastKnot->Next)
            LastKnot->InitCPoints();
    if (RootKnot != NULL)
        for (LastKnot = RootKnot; LastKnot != NULL; LastKnot = LastKnot->Next)
            LastKnot->InitLength();
}

float TSpline::GetLength()
{
    TKnot *tmp = RootKnot;
    float l = 0;
    for (int i = 0; i < iNumKnots; i++)
    {
        l += tmp->Length;
        tmp = tmp->Next;
    }

    return l;
}

vector3 TSpline::GetCenter()
{
    TKnot *ck, *tk;
    ck = RootKnot;
    vector3 pt = vector3(0, 0, 0);

    for (int i = 0; i < iNumKnots; i++)
    {
        pt += ck->Point;
        ck = ck->Next;
    }
    if (iNumKnots > 0)
        pt /= iNumKnots;
}

TKnot *__fastcall TSpline::GetLastKnot()
{
    TKnot *ck;
    ck = RootKnot;

    for (int i = 0; i < iNumKnots - 1; i++)
    {
        ck = ck->Next;
    }
    return ck;
}

bool TSpline::Render()
{
    TKnot *LastKnot = NULL;
    ;

    //  if (RootKnot==NULL)
    //        RootKnot= Knots;

    if (RootKnot != NULL)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBegin(GL_LINE_STRIP);
        int i = 0;
        for (LastKnot = RootKnot; LastKnot != NULL && i < iNumKnots; LastKnot = LastKnot->Next, i++)
            //        for (LastKnot= RootKnot; LastKnot!=NULL; LastKnot= LastKnot->Next)
            glVertex3f(LastKnot->Point.x, LastKnot->Point.y, LastKnot->Point.z);
        glEnd();
        /*
                if (RootKnot->Prev!=NULL)
                {
                glBindTexture(GL_TEXTURE_2D, 0);
                glBegin(GL_LINE_STRIP);
                    glVertex3f(RootKnot->Point.x,RootKnot->Point.y,RootKnot->Point.z);
                    glVertex3f(RootKnot->Prev->Point.x,RootKnot->Prev->Point.y,RootKnot->Prev->Point.z);
                glEnd();
                }*/
    }
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
