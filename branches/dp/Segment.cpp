//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Segment.h"
#include "Usefull.h"

#define Precision 10000

//101206 Ra: trapezoidalne drogi

__fastcall TSegment::TSegment()
{
    Point1=CPointOut=CPointIn=Point2= vector3(0.0f,0.0f,0.0f);
    fLength= 0;
    fRoll1= 0;
    fRoll2= 0;
    fTsBuffer= NULL;
    fStep= 0;
}


bool __fastcall TSegment::Init(vector3 NewPoint1, vector3 NewPoint2, double fNewStep,
                                double fNewRoll1, double fNewRoll2)
{//wersja dla prostego - wyliczanie punktów kontrolnych
 vector3 dir;
 if (fNewRoll1==fNewRoll2)
 {//faktyczny prosty
  dir=Normalize(NewPoint2-NewPoint1); //wektor kierunku o d³ugoœci 1
  return TSegment::Init(
   NewPoint1,dir,-dir,NewPoint2,
   fNewStep,fNewRoll1,fNewRoll2,
   false);
 }
 else
 {//prosty ze zmienn¹ przechy³k¹ musi byæ segmentowany jak krzywe
  dir=(NewPoint2-NewPoint1)/3.0; //punkty kontrolne prostego s¹ w 1/3 d³ugoœci
  return TSegment::Init(
   NewPoint1,NewPoint1+dir,NewPoint2-dir,NewPoint2,
   fNewStep,fNewRoll1,fNewRoll2,
   true);
 }
}

bool __fastcall TSegment::Init(vector3 NewPoint1, vector3 NewCPointOut,
                               vector3 NewCPointIn, vector3 NewPoint2, double fNewStep,
                               double fNewRoll1, double fNewRoll2, bool bIsCurve)
{//wersja dla krzywej
    Point1= NewPoint1;
    CPointOut= NewCPointOut;
    CPointIn= NewCPointIn;
    Point2= NewPoint2;
    bCurve= bIsCurve;
    if (bCurve)
        fLength= ComputeLength(Point1,CPointOut,CPointIn,Point2);
    else
        fLength= (Point1-Point2).Length();
    fRoll1= DegToRad(fNewRoll1); //Ra: przeliczone jest bardziej przydatne
    fRoll2= DegToRad(fNewRoll2);
    fStep= fNewStep;

    if (fLength<=0)
    {
        MessageBox(0,"Length<=0","TSegment::Init",MB_OK);
        return false;
    }

    SafeDeleteArray(fTsBuffer);
    if ((bCurve) && (fStep>0))
    {//Ra: prosty dostanie podzia³, jak ma wpisane kontrolne :(
        double s=0;
        int i=0;
        iSegCount=ceil(fLength/fStep); //potrzebne do VBO
        //fStep=fLength/(double)(iSegCount-1); //wyrównanie podzia³u
        fTsBuffer= new double[iSegCount+1];
        fTsBuffer[0]= 0;               /* TODO : fix fTsBuffer */

        while (s<fLength)
        {
            i++;
            s+= fStep;
            if (s>fLength)
                s= fLength;
            fTsBuffer[i]= GetTFromS(s);
        }
    }


//    return true;

    if (fLength>500)
    {
        MessageBox(0,"Length>500","TSegment::Init",MB_OK);
        return false;
    }

    return true;
}


vector3 __fastcall TSegment::GetFirstDerivative(double fTime)
{

    double fOmTime = 1.0 - fTime;
    double fPowTime = fTime;
    vector3 kResult = fOmTime*(CPointOut-Point1);

    //int iDegreeM1 = 3 - 1;

    double fCoeff = 2*fPowTime;
    kResult = (kResult+fCoeff*(CPointIn-CPointOut))*fOmTime;
    fPowTime *= fTime;

    kResult += fPowTime*(Point2-CPointIn);
    kResult *= 3;

    return kResult;
}

double __fastcall TSegment::RombergIntegral(double fA, double fB)
{
    double fH = fB - fA;

    const int ms_iOrder= 5;

    double ms_apfRom[2][ms_iOrder];

    ms_apfRom[0][0] = 0.5*fH*((GetFirstDerivative(fA).Length())+(GetFirstDerivative(fB).Length()));
    for (int i0 = 2, iP0 = 1; i0 <= ms_iOrder; i0++, iP0 *= 2, fH *= 0.5)
    {
        // approximations via the trapezoid rule
        double fSum = 0.0;
        int i1;
        for (i1 = 1; i1 <= iP0; i1++)
            fSum += (GetFirstDerivative(fA + fH*(i1-0.5)).Length());

        // Richardson extrapolation
        ms_apfRom[1][0] = 0.5*(ms_apfRom[0][0] + fH*fSum);
        for (int i2 = 1, iP2 = 4; i2 < i0; i2++, iP2 *= 4)
        {
            ms_apfRom[1][i2] =
                (iP2*ms_apfRom[1][i2-1] - ms_apfRom[0][i2-1])/(iP2-1);
        }

        for (i1 = 0; i1 < i0; i1++)
            ms_apfRom[0][i1] = ms_apfRom[1][i1];
    }

    return ms_apfRom[0][ms_iOrder-1];
}

double __fastcall TSegment::GetTFromS(double s)
{
    // initial guess for Newton's method
    int it=0;
    double fTolerance= 0.001;
    double fRatio = s/RombergIntegral(0,1);
    double fOmRatio = 1.0 - fRatio;
    double fTime = fOmRatio*0 + fRatio*1;

//    for (int i = 0; i < iIterations; i++)
    while (true)
    {
        it++;
        if (it>10)
        {
            MessageBox(0,"Too many iterations","GetTFromS",MB_OK);
            return fTime;
        }

        double fDifference = RombergIntegral(0,fTime) - s;
        if ( ( fDifference>0 ? fDifference : -fDifference) < fTolerance )
            return fTime;

        fTime -= fDifference/GetFirstDerivative(fTime).Length();
    }

    // Newton's method failed.  If this happens, increase iterations or
    // tolerance or integration accuracy.
    //return -1; //Ra: tu nigdy nie dojdzie

}

double __fastcall TSegment::ComputeLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)  //McZapkie-150503: dlugosc miedzy punktami krzywej
{
    double t,l=0;
    vector3 tmp,last= p1;
    for (int i=1; i<=Precision; i++)
    {
        t= double(i)/double(Precision);
        tmp= Interpolate(t,p1,cp1,cp2,p2);
        t= vector3(tmp-last).Length();
        l+= t;
        last= tmp;
    }
    return (l);
}

const double fDirectionOffset= 0.1;

vector3 __fastcall TSegment::GetDirection(double fDistance)
{
    double t1= GetTFromS(fDistance-fDirectionOffset);
    if (t1<0)
        return (CPointOut-Point1);
    double t2= GetTFromS(fDistance+fDirectionOffset);
    if (t2>1)
        return (Point1-CPointIn);
    return (FastGetPoint(t2)-FastGetPoint(t1));
}

vector3 __fastcall TSegment::FastGetDirection(double fDistance, double fOffset)
{
    double t1= fDistance-fOffset;
    if (t1<0)
        return (CPointOut-Point1);
    double t2= fDistance+fOffset;
    if (t2>1)
        return (Point2-CPointIn);
    return (FastGetPoint(t2)-FastGetPoint(t1));
}

vector3 __fastcall TSegment::GetPoint(double fDistance)
{
    if (bCurve)
    {
        double t= GetTFromS(fDistance);
        return  Interpolate(t,Point1,CPointOut,CPointIn,Point2);
    }
    else
    {
        double t= fDistance/fLength;
        return  ((1.0-t)*Point1+(t)*Point2);
    }

}

vector3 __fastcall TSegment::FastGetPoint(double t)
{
    return  (bCurve ? Interpolate(t,Point1,CPointOut,CPointIn,Point2) : ((1.0-t)*Point1+(t)*Point2) );
}

void __fastcall TSegment::RenderLoft(const vector3 *ShapePoints, int iNumShapePoints,
        double fTextureLength, int iSkip, int iQualityFactor)
{//generowanie trójk¹tów dla odcinka trajektorii ruchu
 //standardowo tworzy triangle_strip dla prostego albo ich zestaw dla ³uku
 //po modyfikacji - dla ujemnego (iNumShapePoints) w dodatkowych polach tabeli
 // podany jest przekrój koñcowy
 //podsypka toru jest robiona za pomoc¹ 6 punktów, szyna 12, drogi i rzeki na 3+2+3
    if (iQualityFactor<1) iQualityFactor= 1; //co który segment ma byæ uwzglêdniony
    vector3 pos1,pos2,dir,parallel1,parallel2,pt;
    double s,step,fOffset,tv1,tv2,t;
    int i,j ;
    bool trapez=iNumShapePoints<0; //sygnalizacja trapezowatoœci
    iNumShapePoints=abs(iNumShapePoints);
    if (bCurve)
    {
       double m1,jmm1,m2,jmm2; //pozycje wzglêdne na odcinku 0...1 (ale nie parametr Beziera)
       tv1=0; //Ra: to by mo¿na by³o wyliczaæ dla odcinka, wygl¹da³o by lepiej
       step= fStep*iQualityFactor;
       s= fStep*iSkip; //iSkip - ile odcinków z pocz¹tku pomin¹æ
       i= iSkip; //domyœlnie 0
       t= fTsBuffer[i]; //tabela wattoœci t dla segmentów
       fOffset= 0.1/fLength; //pierwsze 10cm
       pos1= FastGetPoint( t ); //wektor pocz¹tku segmentu
       dir= FastGetDirection( t, fOffset ); //wektor kierunku
       parallel1= Normalize(CrossProduct(dir,vector3(0,1,0))); //wektor prostopad³y
       m2=s/fLength; jmm2=1.0-m2;

       while (s<fLength)
       {
//           step= SquareMagnitude(Global::GetCameraPosition()+pos);
           i+= iQualityFactor; //kolejny punkt ³amanej
           s+= step; //koñcowa pozycja segmentu [m]

           m1=m2; jmm1=jmm2; //stara pozycja
           m2=s/fLength; jmm2=1.0-m2; //nowa pozycja

           if (s>fLength-0.1) //Ra: -0.1 ¿eby nie robi³o cieniasa na koñcu
           {//gdy przekroczyliœmy koniec - st¹d dziury w torach...
               step-=(s-fLength); //jeszcze do wyliczenia mapowania potrzebny
               s=fLength;
               i=iSegCount; //20/5 ma dawaæ 4
               m2=1.0; jmm2=0.0;
           }

           while (tv1>1) tv1-= 1.0f;

           tv2=tv1+step/fTextureLength; //mapowanie na koñcu segmentu

           t= fTsBuffer[i]; //szybsze od GetTFromS(s);
           pos2= FastGetPoint( t );
           dir= FastGetDirection( t, fOffset ); //nowy wektor kierunku
           parallel2= Normalize(CrossProduct(dir,vector3(0,1,0)));


       glBegin(GL_TRIANGLE_STRIP);
         if (trapez)
          for (j=0; j<iNumShapePoints; j++)
          {
           pt=parallel1*(jmm1*ShapePoints[j].x+m1*ShapePoints[j+iNumShapePoints].x)+pos1;
           pt.y+=jmm1*ShapePoints[j].y+m1*ShapePoints[j+iNumShapePoints].y;
           glNormal3f(0.0f,1.0f,0.0f);
           glTexCoord2f(jmm1*ShapePoints[j].z+m1*ShapePoints[j+iNumShapePoints].z,tv1);
           glVertex3f(pt.x,pt.y,pt.z); //pt nie mamy gdzie zapamiêtaæ?
           //dla trapezu drugi koniec ma inne wspó³rzêdne
           pt=parallel2*(jmm2*ShapePoints[j].x+m2*ShapePoints[j+iNumShapePoints].x)+pos2;
           pt.y+=jmm2*ShapePoints[j].y+m2*ShapePoints[j+iNumShapePoints].y;
           glNormal3f(0.0f,1.0f,0.0f);
           glTexCoord2f(jmm2*ShapePoints[j].z+m2*ShapePoints[j+iNumShapePoints].z,tv2);
           glVertex3f(pt.x,pt.y,pt.z);
          }
         else
           for (j=0; j<iNumShapePoints; j++)
           {
           pt= parallel1*ShapePoints[j].x+pos1;
           pt.y+= ShapePoints[j].y;
           glNormal3f(0.0f,1.0f,0.0f);
           glTexCoord2f(ShapePoints[j].z,tv1);
           glVertex3f(pt.x,pt.y,pt.z); //punkt na pocz¹tku odcinka
           pt= parallel2*ShapePoints[j].x+pos2;
           pt.y+= ShapePoints[j].y;
           glNormal3f(0.0f,1.0f,0.0f);
           glTexCoord2f(ShapePoints[j].z,tv2);
           glVertex3f(pt.x,pt.y,pt.z); //punkt na koñcu odcinka
           }
       glEnd();
       pos1= pos2;
       parallel1= parallel2;
       tv1= tv2;
      }
    }
    else
    {//gdy prosty
            pos1= FastGetPoint( (fStep*iSkip)/fLength );
            pos2= FastGetPoint_1();
            dir= GetDirection();
            parallel1= Normalize(CrossProduct(dir,vector3(0,1,0)));

            glBegin(GL_TRIANGLE_STRIP);
            if (trapez)
                for (j=0; j<iNumShapePoints; j++)
                {
                pt= parallel1*ShapePoints[j].x+pos1;
                pt.y+= ShapePoints[j].y;
                glNormal3f(0.0f,1.0f,0.0f);
                glTexCoord2f(ShapePoints[j].z,0);
                glVertex3f(pt.x,pt.y,pt.z);
                //dla trapezu drugi koniec ma inne wspó³rzêdne
                pt= parallel1*ShapePoints[j+iNumShapePoints].x+pos2; //odsuniêcie
                pt.y+= ShapePoints[j+iNumShapePoints].y; //wysokoœæ
                glNormal3f(0.0f,1.0f,0.0f);
                glTexCoord2f(ShapePoints[j+iNumShapePoints].z,fLength/fTextureLength);
                glVertex3f(pt.x,pt.y,pt.z);
                }
            else
                for (j=0; j<iNumShapePoints; j++)
                {
                pt= parallel1*ShapePoints[j].x+pos1;
                pt.y+= ShapePoints[j].y;
                glNormal3f(0.0f,1.0f,0.0f);
                glTexCoord2f(ShapePoints[j].z,0);
                glVertex3f(pt.x,pt.y,pt.z);

                pt= parallel1*ShapePoints[j].x+pos2;
                pt.y+= ShapePoints[j].y;
                glNormal3f(0.0f,1.0f,0.0f);
                glTexCoord2f(ShapePoints[j].z,fLength/fTextureLength);
                glVertex3f(pt.x,pt.y,pt.z);
                }
            glEnd();
    }
};

void __fastcall TSegment::RenderSwitchRail(const vector3 *ShapePoints1, const vector3 *ShapePoints2,
                            int iNumShapePoints,double fTextureLength, int iSkip, double fOffsetX)
{//tworzenie siatki trójk¹tów dla iglicy
    vector3 pos1,pos2,dir,parallel1,parallel2,pt;
    double a1,a2,s,step,offset,tv1,tv2,t,t2,t2step,oldt2,sp,oldsp;
    int i,j ;
    if (bCurve)
    {//dla toru odchylonego
            //t2= 0;
            t2step= 1/double(iSkip); //przesuniêcie tekstury?
            oldt2= 1;
            tv1=0;
            step= fStep; //d³ugœæ segmentu
            s= 0;
            i= 0;
            t= fTsBuffer[i]; //wartoœæ t krzywej Beziera dla pocz¹tku
            a1= 0;
//            step= fStep/fLength;
            offset= 0.1/fLength; //oko³o 10cm w sensie parametru t
            pos1= FastGetPoint( t ); //wspó³rzêdne dla parmatru t
//            dir= GetDirection1();
            dir= FastGetDirection( t, offset ); //wektor wzd³u¿ny
            parallel1= Normalize(CrossProduct(dir,vector3(0,1,0))); //poprzeczny?

            while (s<fLength && i<iSkip)
            {
//                step= SquareMagnitude(Global::GetCameraPosition()+pos);
                //t2= oldt2+t2step;
                i++;
                s+= step;

                if (s>fLength)
                {
                    step-= (s-fLength);
                    s= fLength;
                }

                while (tv1>1)
                {
                    tv1-= 1.0f;
                }

                tv2=tv1+step/fTextureLength;

                t= fTsBuffer[i];
                pos2= FastGetPoint( t );
                dir= FastGetDirection( t, offset );
                parallel2= Normalize(CrossProduct(dir,vector3(0,1,0)));

                a2= double(i)/(iSkip);
                glBegin(GL_TRIANGLE_STRIP);
                    for (j=0; j<iNumShapePoints; j++)
                    {//po dwa punkty trapezu
                        pt= parallel1*(ShapePoints1[j].x*a1+(ShapePoints2[j].x-fOffsetX)*(1.0-a1))+pos1;
                        pt.y+= ShapePoints1[j].y*a1+ShapePoints2[j].y*(1.0-a1);
                        glNormal3f(0.0f,1.0f,0.0f);
                        glTexCoord2f(ShapePoints1[j].z*a1+ShapePoints2[j].z*(1.0-a1),tv1);
                        glVertex3f(pt.x,pt.y,pt.z);

                        pt= parallel2*(ShapePoints1[j].x*a2+(ShapePoints2[j].x-fOffsetX)*(1.0-a2))+pos2;
                        pt.y+= ShapePoints1[j].y*a2+ShapePoints2[j].y*(1.0-a2);
                        glNormal3f(0.0f,1.0f,0.0f);
                        glTexCoord2f(ShapePoints1[j].z*a2+ShapePoints2[j].z*(1.0-a2),tv2);
                        glVertex3f(pt.x,pt.y,pt.z);
                    }
                glEnd();
                pos1= pos2;
                parallel1= parallel2;
                tv1= tv2;
                a1= a2;
            }
    }
    else
    {//dla toru prostego
            tv1=0;
            s= 0;
            i= 0;
//            pos1= FastGetPoint( (5*iSkip)/fLength );
            pos1= FastGetPoint_0();
            dir= GetDirection();
            parallel1= CrossProduct(dir,vector3(0,1,0));
            step= 5;
            a1= 0;

            while (i<iSkip)
            {
//                step= SquareMagnitude(Global::GetCameraPosition()+pos);
                i++;
                s+= step;

                if (s>fLength)
                {
                    step-= (s-fLength);
                    s= fLength;
                }

                while (tv1>1)
                {
                    tv1-= 1.0f;
                }

                tv2=tv1+step/fTextureLength;

                t= s/fLength;
                pos2= FastGetPoint( t );

                a2= double(i)/(iSkip);
                glBegin(GL_TRIANGLE_STRIP);
                    for (j=0; j<iNumShapePoints; j++)
                    {
                        pt= parallel1*(ShapePoints1[j].x*a1+(ShapePoints2[j].x-fOffsetX)*(1.0-a1))+pos1;
                        pt.y+= ShapePoints1[j].y*a1+ShapePoints2[j].y*(1.0-a1);
                        glNormal3f(0.0f,1.0f,0.0f);
                        glTexCoord2f((ShapePoints1[j].z),tv1);
                        glVertex3f(pt.x,pt.y,pt.z);

                        pt= parallel1*(ShapePoints1[j].x*a2+(ShapePoints2[j].x-fOffsetX)*(1.0-a2))+pos2;
                        pt.y+= ShapePoints1[j].y*a2+ShapePoints2[j].y*(1.0-a2);
                        glNormal3f(0.0f,1.0f,0.0f);
                        glTexCoord2f(ShapePoints2[j].z,tv2);
                        glVertex3f(pt.x,pt.y,pt.z);
                    }
                glEnd();
                pos1= pos2;
                tv1= tv2;
                a1= a2;
            }
/*
            glBegin(GL_TRIANGLE_STRIP);
                for (j=0; j<iNumShapePoints; j++)
                {
                pt= parallel1*ShapePoints1[j].x+pos1;
                pt.y+= ShapePoints1[j].y;
                glNormal3f(0.0f,1.0f,0.0f);
                glTexCoord2f(ShapePoints1[j].z,0);
                glVertex3f(pt.x,pt.y,pt.z);

                pt= parallel1*ShapePoints1[j].x+pos2;
                pt.y+= ShapePoints1[j].y;
                glNormal3f(0.0f,1.0f,0.0f);
                glTexCoord2f(ShapePoints1[j].z,fLength/fTextureLength);
                glVertex3f(pt.x,pt.y,pt.z);
                }
            glEnd();*/
    }
};

void __fastcall TSegment::Render()
{
        vector3 pt;
        glBindTexture(GL_TEXTURE_2D, 0);
        int i;
            if (bCurve)
            {
                glColor3f(0,0,1.0f);
                glBegin(GL_LINE_STRIP);
                    glVertex3f(Point1.x,Point1.y,Point1.z);
                    glVertex3f(CPointOut.x,CPointOut.y,CPointOut.z);
                glEnd();

                glBegin(GL_LINE_STRIP);
                    glVertex3f(Point2.x,Point2.y,Point2.z);
                    glVertex3f(CPointIn.x,CPointIn.y,CPointIn.z);
                glEnd();

                glColor3f(1.0f,0,0);
                glBegin(GL_LINE_STRIP);
                    for (int i=0; i<=8; i++)
                    {
                        pt= FastGetPoint(double(i)/8.0f);
                        glVertex3f(pt.x,pt.y,pt.z);
                    }
                 glEnd();
            }
            else
            {
                glColor3f(0,0,1.0f);
                glBegin(GL_LINE_STRIP);
                    glVertex3f(Point1.x,Point1.y,Point1.z);
                    glVertex3f(Point1.x+CPointOut.x,Point1.y+CPointOut.y,Point1.z+CPointOut.z);
                glEnd();

                glBegin(GL_LINE_STRIP);
                    glVertex3f(Point2.x,Point2.y,Point2.z);
                    glVertex3f(Point2.x+CPointIn.x,Point2.y+CPointIn.y,Point2.z+CPointIn.z);
                glEnd();

                glColor3f(0.5f,0,0);
                glBegin(GL_LINE_STRIP);
                    glVertex3f(Point1.x+CPointOut.x,Point1.y+CPointOut.y,Point1.z+CPointOut.z);
                    glVertex3f(Point2.x+CPointIn.x,Point2.y+CPointIn.y,Point2.z+CPointIn.z);
                glEnd();
            }

}

void __fastcall TSegment::RaRenderLoft(CVert* &Vert,CVec* &Norm,CTexCoord* &Tex,
 const vector3 *ShapePoints, int iNumShapePoints, double fTextureLength, int iSkip)
{//generowanie trójk¹tów dla odcinka trajektorii ruchu
 //standardowo tworzy triangle_strip dla prostego albo ich zestaw dla ³uku
 //po modyfikacji - dla ujemnego (iNumShapePoints) w dodatkowych polach tabeli
 // podany jest przekrój koñcowy
 //podsypka toru jest robiona za pomoc¹ 6 punktów, szyna 12, drogi i rzeki na 3+2+3
 //na u¿ytek VBO strip dla ³uków jest tworzony wzd³u¿
 vector3 pos1,pos2,dir,parallel1,parallel2,pt;
 double s,step,fOffset,tv1,tv2,t;
 int i,j ;
 bool trapez=iNumShapePoints<0; //sygnalizacja trapezowatoœci
 iNumShapePoints=abs(iNumShapePoints);
 if (bCurve)
 {
  double m1,jmm1,m2,jmm2; //pozycje wzglêdne na odcinku 0...1 (ale nie parametr Beziera)
  step=fStep;
  tv1=0; //Ra: to by mo¿na by³o wyliczaæ dla odcinka, wygl¹da³o by lepiej
  s=fStep*iSkip; //iSkip - ile odcinków z pocz¹tku pomin¹æ
  i=iSkip; //domyœlnie 0
  t=fTsBuffer[i]; //tabela wattoœci t dla segmentów
  fOffset=0.1/fLength; //pierwsze 10cm
  pos1=FastGetPoint(t); //wektor pocz¹tku segmentu
  dir=FastGetDirection(t,fOffset); //wektor kierunku
  parallel1=Normalize(CrossProduct(dir,vector3(0,1,0))); //wektor prostopad³y
  m2=s/fLength; jmm2=1.0-m2;
  while (i<iSegCount)
  {
   ++i; //kolejny punkt ³amanej
   s+=step; //koñcowa pozycja segmentu [m]
   m1=m2; jmm1=jmm2; //stara pozycja
   m2=s/fLength; jmm2=1.0-m2; //nowa pozycja
   if (i==iSegCount)
   {//gdy przekroczyliœmy koniec - st¹d dziury w torach...
    step-=(s-fLength); //jeszcze do wyliczenia mapowania potrzebny
    s=fLength;
    i=iSegCount; //20/5 ma dawaæ 4
    m2=1.0; jmm2=0.0;
   }
   while (tv1>1) tv1-= 1.0f;
   tv2=tv1+step/fTextureLength; //mapowanie na koñcu segmentu
   t=fTsBuffer[i]; //szybsze od GetTFromS(s);
   pos2=FastGetPoint(t);
   dir=FastGetDirection(t,fOffset); //nowy wektor kierunku
   parallel2=Normalize(CrossProduct(dir,vector3(0,1,0)));
   if (trapez)
    for (j=0; j<iNumShapePoints; j++)
    {//strip wzd³u¿ powinien siê wyœwietlaæ szybciej
     pt=parallel1*(jmm1*ShapePoints[j].x+m1*ShapePoints[j+iNumShapePoints].x)+pos1;
     pt.y+=jmm1*ShapePoints[j].y+m1*ShapePoints[j+iNumShapePoints].y;
     Norm->x=0.0; //niekoniecznie tak
     Norm->y=1.0;
     Norm->z=0.0;
     Tex->u=jmm1*ShapePoints[j].z+m1*ShapePoints[j+iNumShapePoints].z;
     Tex->v=tv1;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz¹tku odcinka
     Vert++; Norm++; Tex++;
     //dla trapezu drugi koniec ma inne wspó³rzêdne
     pt=parallel2*(jmm2*ShapePoints[j].x+m2*ShapePoints[j+iNumShapePoints].x)+pos2;
     pt.y+=jmm2*ShapePoints[j].y+m2*ShapePoints[j+iNumShapePoints].y;
     Norm->x=0.0; //niekoniecznie tak
     Norm->y=1.0;
     Norm->z=0.0;
     Tex->u=jmm2*ShapePoints[j].z+m2*ShapePoints[j+iNumShapePoints].z;
     Tex->v=tv2;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na koñcu odcinka
     Vert++; Norm++; Tex++;
   }
   else
    for (j=0;j<iNumShapePoints;j++)
    {//strip wzd³u¿ powinien siê wyœwietlaæ szybciej
     pt=parallel1*ShapePoints[j].x+pos1;
     pt.y+=ShapePoints[j].y;
     Norm->x=0.0; //niekoniecznie tak
     Norm->y=1.0;
     Norm->z=0.0;
     Tex->u=ShapePoints[j].z;
     Tex->v=tv1;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz¹tku odcinka
     Vert++; Norm++; Tex++;
     pt=parallel2*ShapePoints[j].x+pos2;
     pt.y+=ShapePoints[j].y;
     Norm->x=0.0; //niekoniecznie tak
     Norm->y=1.0;
     Norm->z=0.0;
     Tex->u=ShapePoints[j].z;
     Tex->v=tv2;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na koñcu odcinka
     Vert++; Norm++; Tex++;
    }
   pos1=pos2;
   parallel1=parallel2;
   tv1=tv2;
  }
 }
 else
 {//gdy prosty
  pos1=FastGetPoint((fStep*iSkip)/fLength);
  pos2=FastGetPoint_1();
  dir=GetDirection();
  parallel1=Normalize(CrossProduct(dir,vector3(0,1,0)));
  if (trapez)
   for (j=0;j<iNumShapePoints;j++)
   {
    pt=parallel1*ShapePoints[j].x+pos1;
    pt.y+=ShapePoints[j].y;
    Norm->x=0.0; //niekoniecznie tak
    Norm->y=1.0;
    Norm->z=0.0;
    Tex->u=ShapePoints[j].z;
    Tex->v=0;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz¹tku odcinka
    Vert++; Norm++; Tex++;
    //dla trapezu drugi koniec ma inne wspó³rzêdne
    pt=parallel1*ShapePoints[j+iNumShapePoints].x+pos2; //odsuniêcie
    pt.y+=ShapePoints[j+iNumShapePoints].y; //wysokoœæ
    Norm->x=0.0; //niekoniecznie tak
    Norm->z=1.0;
    Norm->z=0.0;
    Tex->u=ShapePoints[j+iNumShapePoints].z;
    Tex->v=fLength/fTextureLength;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na koñcu odcinka
    Vert++; Norm++; Tex++;
  }
  else
   for (j=0;j<iNumShapePoints;j++)
   {
    pt=parallel1*ShapePoints[j].x+pos1;
    pt.y+=ShapePoints[j].y;
    Norm->x=0.0; //niekoniecznie tak
    Norm->y=1.0;
    Norm->z=0.0;
    Tex->u=ShapePoints[j].z;
    Tex->v=0;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz¹tku odcinka
    Vert++; Norm++; Tex++;
    pt=parallel1*ShapePoints[j].x+pos2;
    pt.y+=ShapePoints[j].y;
    Norm->x=0.0; //niekoniecznie tak
    Norm->y=1.0;
    Norm->z=0.0;
    Tex->u=ShapePoints[j].z;
    Tex->v=fLength/fTextureLength;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na koñcu odcinka
    Vert++; Norm++; Tex++;
   }
 }
};



//---------------------------------------------------------------------------

#pragma package(smart_init)
