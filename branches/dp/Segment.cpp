//---------------------------------------------------------------------------

#include "system.hpp"
#pragma hdrstop
#include "opengl/glew.h"
//#include "opengl/glut.h"

#include "Segment.h"
#include "Usefull.h"

#define Precision 10000

#pragma package(smart_init)
//---------------------------------------------------------------------------

//101206 Ra: trapezoidalne drogi
//110806 Ra: odwr�cone mapowanie wzd�u� - Point1 == 1.0

__fastcall TSegment::TSegment(TTrack *owner)
{
 Point1=CPointOut=CPointIn=Point2=vector3(0.0f,0.0f,0.0f);
 fLength=0;
 fRoll1=0;
 fRoll2=0;
 fTsBuffer=NULL;
 fStep=0;
 pOwner=owner;
}

__fastcall TSegment::~TSegment()
{
 SafeDeleteArray(fTsBuffer);
};

bool __fastcall TSegment::Init(
 vector3 NewPoint1,vector3 NewPoint2,double fNewStep,
 double fNewRoll1,double fNewRoll2)
{//wersja dla prostego - wyliczanie punkt�w kontrolnych
 vector3 dir;
 if (fNewRoll1==fNewRoll2)
 {//faktyczny prosty
  dir=Normalize(NewPoint2-NewPoint1); //wektor kierunku o d�ugo�ci 1
  return TSegment::Init(
   NewPoint1,dir,-dir,NewPoint2,
   fNewStep,fNewRoll1,fNewRoll2,
   false);
 }
 else
 {//prosty ze zmienn� przechy�k� musi by� segmentowany jak krzywe
  dir=(NewPoint2-NewPoint1)/3.0; //punkty kontrolne prostego s� w 1/3 d�ugo�ci
  return TSegment::Init(
   NewPoint1,NewPoint1+dir,NewPoint2-dir,NewPoint2,
   fNewStep,fNewRoll1,fNewRoll2,
   true);
 }
}

bool __fastcall TSegment::Init(
 vector3 NewPoint1,vector3 NewCPointOut,vector3 NewCPointIn,vector3 NewPoint2,
 double fNewStep,double fNewRoll1, double fNewRoll2, bool bIsCurve)
{//wersja uniwersalna (dla krzywej i prostego)
 Point1=NewPoint1;
 CPointOut=NewCPointOut;
 CPointIn=NewCPointIn;
 Point2=NewPoint2;
 fStoop=atan2((Point2.y-Point1.y),fLength); //pochylenie toru prostego, �eby nie liczy� wielokrotnie
 //Ra: ten k�t jeszcze do przemy�lenia jest
 fDirection=-atan2(Point2.x-Point1.x,Point2.z-Point1.z); //k�t w planie, �eby nie liczy� wielokrotnie
 bCurve=bIsCurve;
 if (bCurve)
 {//przeliczenie wsp�czynnik�w wielomianu, b�dzie mniej mno�e� i mo�na policzy� pochodne
  vC=3.0*(CPointOut-Point1); //t^1
  vB=3.0*(CPointIn-CPointOut)-vC; //t^2
  vA=Point2-Point1-vC-vB; //t^3
  fLength=ComputeLength(Point1,CPointOut,CPointIn,Point2);
 }
 else
  fLength=(Point1-Point2).Length();
 fRoll1=DegToRad(fNewRoll1); //Ra: przeliczone jest bardziej przydatne
 fRoll2=DegToRad(fNewRoll2);
 fStep=fNewStep;
 if (fLength<=0)
 {
  WriteLog("Length <= 0 in TSegment::Init");
  //MessageBox(0,"Length<=0","TSegment::Init",MB_OK);
  return false; //zerowe nie mog� by�
 }
 SafeDeleteArray(fTsBuffer);
 if ((bCurve) && (fStep>0))
 {//Ra: prosty dostanie podzia�, jak ma wpisane kontrolne :(
  double s=0;
  int i=0;
  iSegCount=ceil(fLength/fStep); //potrzebne do VBO
  //fStep=fLength/(double)(iSegCount-1); //wyr�wnanie podzia�u
  fTsBuffer=new double[iSegCount+1];
  fTsBuffer[0]=0;               /* TODO : fix fTsBuffer */
  while (s<fLength)
  {
   i++;
   s+=fStep;
   if (s>fLength) s=fLength;
   fTsBuffer[i]=GetTFromS(s);
  }
 }
 if (fLength>500)
 {//tor ma pojemno�� 40 pojazd�w, wi�c nie mo�e by� za d�ugi
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

};

vector3 __fastcall TSegment::RaInterpolate(double t)
{//wyliczenie XYZ na krzywej Beziera z u�yciem wsp�czynnik�w
 return t*(t*(t*vA+vB)+vC)+Point1; //9 mno�e�, 9 dodawa�
};

double __fastcall TSegment::ComputeLength(vector3 p1,vector3 cp1,vector3 cp2,vector3 p2)  //McZapkie-150503: dlugosc miedzy punktami krzywej
{//obliczenie d�ugo�ci krzywej Beziera za pomoc� interpolacji odcinkami
 double t,l=0;
 vector3 tmp,last=p1;
 for (int i=1;i<=Precision;i++)
 {
  t=double(i)/double(Precision);
  //tmp=Interpolate(t,p1,cp1,cp2,p2);
  tmp=RaInterpolate(t);
  t=vector3(tmp-last).Length();
  l+=t;
  last=tmp;
 }
 return (l);
}

const double fDirectionOffset=0.1; //d�ugo�� wektora do wyliczenia kierunku

vector3 __fastcall TSegment::GetDirection(double fDistance)
{//takie toporne liczenie pochodnej dla podanego dystansu od Point1
 double t1=GetTFromS(fDistance-fDirectionOffset);
 if (t1<0)
  return (CPointOut-Point1); //na zewn�trz jako prosta
 double t2=GetTFromS(fDistance+fDirectionOffset);
 if (t2>1)
  return (Point1-CPointIn); //na zewn�trz jako prosta
 return (FastGetPoint(t2)-FastGetPoint(t1));
}

vector3 __fastcall TSegment::FastGetDirection(double fDistance, double fOffset)
{//takie toporne liczenie pochodnej dla parametru 0.0�1.0
 double t1=fDistance-fOffset;
 if (t1<0)
  return (CPointOut-Point1);
 double t2=fDistance+fOffset;
 if (t2>1)
  return (Point2-CPointIn);
 return (FastGetPoint(t2)-FastGetPoint(t1));
}

vector3 __fastcall TSegment::GetPoint(double fDistance)
{//wyliczenie wsp�rz�dnych XYZ na torze w odleg�o�ci (fDistance) od Point1
 if (bCurve)
 {//mo�na by wprowadzi� uproszczony wz�r dla okr�g�w p�askich
  double t=GetTFromS(fDistance); //aproksymacja dystansu na krzywej Beziera
  //return Interpolate(t,Point1,CPointOut,CPointIn,Point2);
  return RaInterpolate(t);
 }
 else
 {//wyliczenie dla odcinka prostego jest prostsze
  double t=fDistance/fLength; //zerowych tor�w nie ma
  return ((1.0-t)*Point1+(t)*Point2);
 }
};

void __fastcall TSegment::RaPositionGet(double fDistance,vector3 &p,vector3 &a)
{//ustalenie pozycji osi na torze, przechy�ki, pochylenia i kierunku jazdy
 if (bCurve)
 {//mo�na by wprowadzi� uproszczony wz�r dla okr�g�w p�askich
  double t=GetTFromS(fDistance); //aproksymacja dystansu na krzywej Beziera na parametr (t)
  p=RaInterpolate(t);
  a.x=(1.0-t)*fRoll1+(t)*fRoll2; //przechy�ka w danym miejscu (zmienia si� liniowo)
  //pochodna jest 3*A*t^2+2*B*t+C
  a.y=atan(t*(t*3.0*vA.y+2.0*vB.y)+vC.y); //pochylenie krzywej
  a.z=-atan2(t*(t*3.0*vA.x+2.0*vB.x)+vC.x,t*(t*3.0*vA.z+2.0*vB.z)+vC.z); //kierunek krzywej w planie
 }
 else
 {//wyliczenie dla odcinka prostego jest prostsze
  double t=fDistance/fLength; //zerowych tor�w nie ma
  p=((1.0-t)*Point1+(t)*Point2);
  a.x=(1.0-t)*fRoll1+(t)*fRoll2; //przechy�ka w danym miejscu (zmienia si� liniowo)
  a.y=fStoop; //pochylenie toru prostego
  a.z=fDirection; //kierunek toru w planie
 }
};


vector3 __fastcall TSegment::FastGetPoint(double t)
{
 //return (bCurve?Interpolate(t,Point1,CPointOut,CPointIn,Point2):((1.0-t)*Point1+(t)*Point2));
 return (bCurve?RaInterpolate(t):((1.0-t)*Point1+(t)*Point2));
}

void __fastcall TSegment::RenderLoft(const vector6 *ShapePoints, int iNumShapePoints,
        double fTextureLength, int iSkip, int iQualityFactor)
{//generowanie tr�jk�t�w dla odcinka trajektorii ruchu
 //standardowo tworzy triangle_strip dla prostego albo ich zestaw dla �uku
 //po modyfikacji - dla ujemnego (iNumShapePoints) w dodatkowych polach tabeli
 // podany jest przekr�j ko�cowy
 //podsypka toru jest robiona za pomoc� 6 punkt�w, szyna 12, drogi i rzeki na 3+2+3
 if (iQualityFactor<1) iQualityFactor= 1; //co kt�ry segment ma by� uwzgl�dniony
 vector3 pos1,pos2,dir,parallel1,parallel2,pt,norm;
 double s,step,fOffset,tv1,tv2,t;
 int i,j ;
 bool trapez=iNumShapePoints<0; //sygnalizacja trapezowato�ci
 iNumShapePoints=abs(iNumShapePoints);
 if (bCurve)
 {
  double m1,jmm1,m2,jmm2; //pozycje wzgl�dne na odcinku 0...1 (ale nie parametr Beziera)
  tv1=1.0; //Ra: to by mo�na by�o wylicza� dla odcinka, wygl�da�o by lepiej
  step=fStep*iQualityFactor;
  s=fStep*iSkip; //iSkip - ile odcink�w z pocz�tku pomin��
  i=iSkip; //domy�lnie 0
  t=fTsBuffer[i]; //tabela wato�ci t dla segment�w
  fOffset=0.1/fLength; //pierwsze 10cm
  pos1=FastGetPoint(t); //wektor pocz�tku segmentu
  dir=FastGetDirection(t,fOffset); //wektor kierunku
  parallel1=Normalize(CrossProduct(dir,vector3(0,1,0))); //wektor poprzeczny
  m2=s/fLength; jmm2=1.0-m2;
  while (s<fLength)
  {
   //step=SquareMagnitude(Global::GetCameraPosition()+pos);
   i+=iQualityFactor; //kolejny punkt �amanej
   s+=step; //ko�cowa pozycja segmentu [m]
   m1=m2; jmm1=jmm2; //stara pozycja
   m2=s/fLength; jmm2=1.0-m2; //nowa pozycja
   if (s>fLength-0.5) //Ra: -0.5 �eby nie robi�o cieniasa na ko�cu
   {//gdy przekroczyli�my koniec - st�d dziury w torach...
    step-=(s-fLength); //jeszcze do wyliczenia mapowania potrzebny
    s=fLength;
    i=iSegCount; //20/5 ma dawa� 4
    m2=1.0; jmm2=0.0;
   }
   while (tv1<0.0) tv1+=1.0; //przestawienie mapowania
   tv2=tv1-step/fTextureLength; //mapowanie na ko�cu segmentu
   t=fTsBuffer[i]; //szybsze od GetTFromS(s);
   pos2=FastGetPoint(t);
   dir=Normalize(FastGetDirection(t,fOffset)); //nowy wektor kierunku
   parallel2=CrossProduct(dir,vector3(0,1,0)); //wektor poprzeczny
   glBegin(GL_TRIANGLE_STRIP);
   if (trapez)
    for (j=0;j<iNumShapePoints;j++)
    {
     norm=(jmm1*ShapePoints[j].n.x+m1*ShapePoints[j+iNumShapePoints].n.x)*parallel1;
     norm.y+=jmm1*ShapePoints[j].n.y+m1*ShapePoints[j+iNumShapePoints].n.y;
     pt=parallel1*(jmm1*ShapePoints[j].x+m1*ShapePoints[j+iNumShapePoints].x)+pos1;
     pt.y+=jmm1*ShapePoints[j].y+m1*ShapePoints[j+iNumShapePoints].y;
     glNormal3f(norm.x,norm.y,norm.z);
     glTexCoord2f(jmm1*ShapePoints[j].z+m1*ShapePoints[j+iNumShapePoints].z,tv1);
     glVertex3f(pt.x,pt.y,pt.z); //pt nie mamy gdzie zapami�ta�?
     //dla trapezu drugi koniec ma inne wsp�rz�dne
     norm=(jmm1*ShapePoints[j].n.x+m1*ShapePoints[j+iNumShapePoints].n.x)*parallel2;
     norm.y+=jmm1*ShapePoints[j].n.y+m1*ShapePoints[j+iNumShapePoints].n.y;
     pt=parallel2*(jmm2*ShapePoints[j].x+m2*ShapePoints[j+iNumShapePoints].x)+pos2;
     pt.y+=jmm2*ShapePoints[j].y+m2*ShapePoints[j+iNumShapePoints].y;
     glNormal3f(norm.x,norm.y,norm.z);
     glTexCoord2f(jmm2*ShapePoints[j].z+m2*ShapePoints[j+iNumShapePoints].z,tv2);
     glVertex3f(pt.x,pt.y,pt.z);
    }
   else
    for (j=0;j<iNumShapePoints;j++)
    {//�uk z jednym profilem
     norm=ShapePoints[j].n.x*parallel1;
     norm.y+=ShapePoints[j].n.y;
     pt=parallel1*ShapePoints[j].x+pos1;
     pt.y+=ShapePoints[j].y;
     glNormal3f(norm.x,norm.y,norm.z);
     glTexCoord2f(ShapePoints[j].z,tv1);
     glVertex3f(pt.x,pt.y,pt.z); //punkt na pocz�tku odcinka
     norm=ShapePoints[j].n.x*parallel2;
     norm.y+=ShapePoints[j].n.y;
     pt=parallel2*ShapePoints[j].x+pos2;
     pt.y+=ShapePoints[j].y;
     glNormal3f(norm.x,norm.y,norm.z);
     glTexCoord2f(ShapePoints[j].z,tv2);
     glVertex3f(pt.x,pt.y,pt.z); //punkt na ko�cu odcinka
    }
   glEnd();
   pos1=pos2;
   parallel1=parallel2;
   tv1=tv2;
  }
 }
 else
 {//gdy prosty, nie modyfikujemy wektora kierunkowego i poprzecznego
  pos1=FastGetPoint((fStep*iSkip)/fLength);
  pos2=FastGetPoint_1();
  dir=GetDirection();
  parallel1=Normalize(CrossProduct(dir,vector3(0,1,0)));
  glBegin(GL_TRIANGLE_STRIP);
  if (trapez)
   for (j=0;j<iNumShapePoints;j++)
   {
    norm=ShapePoints[j].n.x*parallel1;
    norm.y+=ShapePoints[j].n.y;
    pt=parallel1*ShapePoints[j].x+pos1;
    pt.y+=ShapePoints[j].y;
    glNormal3f(norm.x,norm.y,norm.z);
    glTexCoord2f(ShapePoints[j].z,0);
    glVertex3f(pt.x,pt.y,pt.z);
    //dla trapezu drugi koniec ma inne wsp�rz�dne wzgl�dne
    norm=ShapePoints[j+iNumShapePoints].n.x*parallel1;
    norm.y+=ShapePoints[j+iNumShapePoints].n.y;
    pt=parallel1*ShapePoints[j+iNumShapePoints].x+pos2; //odsuni�cie
    pt.y+=ShapePoints[j+iNumShapePoints].y; //wysoko��
    glNormal3f(norm.x,norm.y,norm.z);
    glTexCoord2f(ShapePoints[j+iNumShapePoints].z,fLength/fTextureLength);
    glVertex3f(pt.x,pt.y,pt.z);
   }
  else
   for (j=0;j<iNumShapePoints;j++)
   {
    norm=ShapePoints[j].n.x*parallel1;
    norm.y+=ShapePoints[j].n.y;
    pt=parallel1*ShapePoints[j].x+pos1;
    pt.y+=ShapePoints[j].y;
    glNormal3f(norm.x,norm.y,norm.z);
    glTexCoord2f(ShapePoints[j].z,0);
    glVertex3f(pt.x,pt.y,pt.z);
    pt=parallel1*ShapePoints[j].x+pos2;
    pt.y+=ShapePoints[j].y;
    glNormal3f(norm.x,norm.y,norm.z);
    glTexCoord2f(ShapePoints[j].z,fLength/fTextureLength);
    glVertex3f(pt.x,pt.y,pt.z);
   }
  glEnd();
 }
};

void __fastcall TSegment::RenderSwitchRail(const vector6 *ShapePoints1, const vector6 *ShapePoints2,
                            int iNumShapePoints,double fTextureLength, int iSkip, double fOffsetX)
{//tworzenie siatki tr�jk�t�w dla iglicy
    vector3 pos1,pos2,dir,parallel1,parallel2,pt;
    double a1,a2,s,step,offset,tv1,tv2,t,t2,t2step,oldt2,sp,oldsp;
    int i,j ;
    if (bCurve)
    {//dla toru odchylonego
            //t2= 0;
            t2step= 1/double(iSkip); //przesuni�cie tekstury?
            oldt2= 1;
            tv1=1.0;
            step= fStep; //d�ug�� segmentu
            s= 0;
            i= 0;
            t= fTsBuffer[i]; //warto�� t krzywej Beziera dla pocz�tku
            a1= 0;
//            step= fStep/fLength;
            offset= 0.1/fLength; //oko�o 10cm w sensie parametru t
            pos1= FastGetPoint( t ); //wsp�rz�dne dla parmatru t
//            dir= GetDirection1();
            dir= FastGetDirection( t, offset ); //wektor wzd�u�ny
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

                while (tv1<0.0) tv1+=1.0;
                tv2=tv1-step/fTextureLength;

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
                tv1=tv2;
                a1= a2;
            }
    }
    else
    {//dla toru prostego
            tv1=1.0;
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

                while (tv1<0.0) tv1+=1.0;

                tv2=tv1-step/fTextureLength;

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
                tv1=tv2;
                a1= a2;
            }
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


void __fastcall TSegment::RaRenderLoft(
 CVertNormTex* &Vert,const vector6 *ShapePoints,
 int iNumShapePoints,double fTextureLength,int iSkip,int iEnd,double fOffsetX)
{//generowanie tr�jk�t�w dla odcinka trajektorii ruchu
 //standardowo tworzy triangle_strip dla prostego albo ich zestaw dla �uku
 //po modyfikacji - dla ujemnego (iNumShapePoints) w dodatkowych polach tabeli
 // podany jest przekr�j ko�cowy
 //podsypka toru jest robiona za pomoc� 6 punkt�w, szyna 12, drogi i rzeki na 3+2+3
 //na u�ytek VBO strip dla �uk�w jest tworzony wzd�u�
 //dla skr�conego odcinka (iEnd<iSegCount), ShapePoints dotyczy
 //ko�c�w skr�conych, a nie ca�o�ci (to pod k�tem iglic jest)
 vector3 pos1,pos2,dir,parallel1,parallel2,pt,norm;
 double s,step,fOffset,tv1,tv2,t,fEnd;
 int i,j ;
 bool trapez=iNumShapePoints<0; //sygnalizacja trapezowato�ci
 iNumShapePoints=abs(iNumShapePoints);
 if (bCurve)
 {
  double m1,jmm1,m2,jmm2; //pozycje wzgl�dne na odcinku 0...1 (ale nie parametr Beziera)
  step=fStep;
  tv1=1.0; //Ra: to by mo�na by�o wylicza� dla odcinka, wygl�da�o by lepiej
  s=fStep*iSkip; //iSkip - ile odcink�w z pocz�tku pomin��
  i=iSkip; //domy�lnie 0
  t=fTsBuffer[i]; //tabela watto�ci t dla segment�w
  fOffset=0.1/fLength; //pierwsze 10cm
  pos1=FastGetPoint(t); //wektor pocz�tku segmentu
  dir=FastGetDirection(t,fOffset); //wektor kierunku
  parallel1=Normalize(CrossProduct(dir,vector3(0,1,0))); //wektor prostopad�y
  if (iEnd==0) iEnd=iSegCount;
  fEnd=fLength*double(iEnd)/double(iSegCount);
  m2=s/fEnd; jmm2=1.0-m2;
  while (i<iEnd)
  {
   ++i; //kolejny punkt �amanej
   s+=step; //ko�cowa pozycja segmentu [m]
   m1=m2; jmm1=jmm2; //stara pozycja
   m2=s/fEnd; jmm2=1.0-m2; //nowa pozycja
   if (i==iEnd)
   {//gdy przekroczyli�my koniec - st�d dziury w torach...
    step-=(s-fEnd); //jeszcze do wyliczenia mapowania potrzebny
    s=fEnd;
    //i=iEnd; //20/5 ma dawa� 4
    m2=1.0; jmm2=0.0;
   }
   while (tv1<0.0) tv1+=1.0;
   tv2=tv1-step/fTextureLength; //mapowanie na ko�cu segmentu
   t=fTsBuffer[i]; //szybsze od GetTFromS(s);
   pos2=FastGetPoint(t);
   dir=FastGetDirection(t,fOffset); //nowy wektor kierunku
   parallel2=Normalize(CrossProduct(dir,vector3(0,1,0)));
   if (trapez)
    for (j=0; j<iNumShapePoints; j++)
    {//wsp�rz�dne pocz�tku
     norm=(jmm1*ShapePoints[j].n.x+m1*ShapePoints[j+iNumShapePoints].n.x)*parallel1;
     norm.y+=jmm1*ShapePoints[j].n.y+m1*ShapePoints[j+iNumShapePoints].n.y;
     pt=parallel1*(jmm1*(ShapePoints[j].x-fOffsetX)+m1*ShapePoints[j+iNumShapePoints].x)+pos1;
     pt.y+=jmm1*ShapePoints[j].y+m1*ShapePoints[j+iNumShapePoints].y;
     Vert->nx=norm.x; //niekoniecznie tak
     Vert->ny=norm.y;
     Vert->nz=norm.z;
     Vert->u=jmm1*ShapePoints[j].z+m1*ShapePoints[j+iNumShapePoints].z;
     Vert->v=tv1;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz�tku odcinka
     Vert++;
     //dla trapezu drugi koniec ma inne wsp�rz�dne wzgl�dne
     norm=(jmm1*ShapePoints[j].n.x+m1*ShapePoints[j+iNumShapePoints].n.x)*parallel2;
     norm.y+=jmm1*ShapePoints[j].n.y+m1*ShapePoints[j+iNumShapePoints].n.y;
     pt=parallel2*(jmm2*(ShapePoints[j].x-fOffsetX)+m2*ShapePoints[j+iNumShapePoints].x)+pos2;
     pt.y+=jmm2*ShapePoints[j].y+m2*ShapePoints[j+iNumShapePoints].y;
     Vert->nx=norm.x; //niekoniecznie tak
     Vert->ny=norm.y;
     Vert->nz=norm.z;
     Vert->u=jmm2*ShapePoints[j].z+m2*ShapePoints[j+iNumShapePoints].z;
     Vert->v=tv2;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na ko�cu odcinka
     Vert++;
   }
   else
    for (j=0;j<iNumShapePoints;j++)
    {//wsp�rz�dne pocz�tku
     norm=ShapePoints[j].n.x*parallel1;
     norm.y+=ShapePoints[j].n.y;
     pt=parallel1*(ShapePoints[j].x-fOffsetX)+pos1;
     pt.y+=ShapePoints[j].y;
     Vert->nx=norm.x; //niekoniecznie tak
     Vert->ny=norm.y;
     Vert->nz=norm.z;
     Vert->u=ShapePoints[j].z;
     Vert->v=tv1;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz�tku odcinka
     Vert++;
     norm=ShapePoints[j].n.x*parallel2;
     norm.y+=ShapePoints[j].n.y;
     pt=parallel2*ShapePoints[j].x+pos2;
     pt.y+=ShapePoints[j].y;
     Vert->nx=norm.x; //niekoniecznie tak
     Vert->ny=norm.y;
     Vert->nz=norm.z;
     Vert->u=ShapePoints[j].z;
     Vert->v=tv2;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na ko�cu odcinka
     Vert++;
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
    norm=ShapePoints[j].n.x*parallel1;
    norm.y+=ShapePoints[j].n.y;
    pt=parallel1*(ShapePoints[j].x-fOffsetX)+pos1;
    pt.y+=ShapePoints[j].y;
    Vert->nx=norm.x; //niekoniecznie tak
    Vert->ny=norm.y;
    Vert->nz=norm.z;
    Vert->u=ShapePoints[j].z;
    Vert->v=0;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz�tku odcinka
    Vert++;
    //dla trapezu drugi koniec ma inne wsp�rz�dne
    norm=ShapePoints[j+iNumShapePoints].n.x*parallel1;
    norm.y+=ShapePoints[j+iNumShapePoints].n.y;
    pt=parallel1*(ShapePoints[j+iNumShapePoints].x-fOffsetX)+pos2; //odsuni�cie
    pt.y+=ShapePoints[j+iNumShapePoints].y; //wysoko��
    Vert->nx=norm.x; //niekoniecznie tak
    Vert->ny=norm.y;
    Vert->nz=norm.z;
    Vert->u=ShapePoints[j+iNumShapePoints].z;
    Vert->v=fLength/fTextureLength;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na ko�cu odcinka
    Vert++;
  }
  else
   for (j=0;j<iNumShapePoints;j++)
   {
    norm=ShapePoints[j].n.x*parallel1;
    norm.y+=ShapePoints[j].n.y;
    pt=parallel1*(ShapePoints[j].x-fOffsetX)+pos1;
    pt.y+=ShapePoints[j].y;
    Vert->nx=norm.x; //niekoniecznie tak
    Vert->ny=norm.y;
    Vert->nz=norm.z;
    Vert->u=ShapePoints[j].z;
    Vert->v=0;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz�tku odcinka
    Vert++;
    pt=parallel1*(ShapePoints[j].x-fOffsetX)+pos2;
    pt.y+=ShapePoints[j].y;
    Vert->nx=norm.x; //niekoniecznie tak
    Vert->ny=norm.y;
    Vert->nz=norm.z;
    Vert->u=ShapePoints[j].z;
    Vert->v=fLength/fTextureLength;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na ko�cu odcinka
    Vert++;
   }
 }
};

void __fastcall TSegment::RaAnimate(
 CVertNormTex* &Vert,const vector6 *ShapePoints,
 int iNumShapePoints,double fTextureLength,int iSkip,int iEnd,double fOffsetX)
{//jak wy�ej, tylko z pomini�ciem mapowania i braku trapezowania
 vector3 pos1,pos2,dir,parallel1,parallel2,pt;
 double s,step,fOffset,t,fEnd;
 int i,j ;
 bool trapez=iNumShapePoints<0; //sygnalizacja trapezowato�ci
 iNumShapePoints=abs(iNumShapePoints);
 if (bCurve)
 {
  double m1,jmm1,m2,jmm2; //pozycje wzgl�dne na odcinku 0...1 (ale nie parametr Beziera)
  step=fStep;
  s=fStep*iSkip; //iSkip - ile odcink�w z pocz�tku pomin��
  i=iSkip; //domy�lnie 0
  t=fTsBuffer[i]; //tabela watto�ci t dla segment�w
  fOffset=0.1/fLength; //pierwsze 10cm
  pos1=FastGetPoint(t); //wektor pocz�tku segmentu
  dir=FastGetDirection(t,fOffset); //wektor kierunku
  parallel1=Normalize(CrossProduct(dir,vector3(0,1,0))); //wektor prostopad�y
  if (iEnd==0) iEnd=iSegCount;
  fEnd=fLength*double(iEnd)/double(iSegCount);
  m2=s/fEnd; jmm2=1.0-m2;
  while (i<iEnd)
  {
   ++i; //kolejny punkt �amanej
   s+=step; //ko�cowa pozycja segmentu [m]
   m1=m2; jmm1=jmm2; //stara pozycja
   m2=s/fEnd; jmm2=1.0-m2; //nowa pozycja
   if (i==iEnd)
   {//gdy przekroczyli�my koniec - st�d dziury w torach...
    step-=(s-fEnd); //jeszcze do wyliczenia mapowania potrzebny
    s=fEnd;
    //i=iEnd; //20/5 ma dawa� 4
    m2=1.0; jmm2=0.0;
   }
   t=fTsBuffer[i]; //szybsze od GetTFromS(s);
   pos2=FastGetPoint(t);
   dir=FastGetDirection(t,fOffset); //nowy wektor kierunku
   parallel2=Normalize(CrossProduct(dir,vector3(0,1,0)));
   if (trapez)
    for (j=0; j<iNumShapePoints; j++)
    {//wsp�rz�dne pocz�tku
     pt=parallel1*(jmm1*(ShapePoints[j].x-fOffsetX)+m1*ShapePoints[j+iNumShapePoints].x)+pos1;
     pt.y+=jmm1*ShapePoints[j].y+m1*ShapePoints[j+iNumShapePoints].y;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz�tku odcinka
     Vert++;
     //dla trapezu drugi koniec ma inne wsp�rz�dne
     pt=parallel2*(jmm2*(ShapePoints[j].x-fOffsetX)+m2*ShapePoints[j+iNumShapePoints].x)+pos2;
     pt.y+=jmm2*ShapePoints[j].y+m2*ShapePoints[j+iNumShapePoints].y;
     Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na ko�cu odcinka
     Vert++;
   }
   pos1=pos2;
   parallel1=parallel2;
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
    pt=parallel1*(ShapePoints[j].x-fOffsetX)+pos1;
    pt.y+=ShapePoints[j].y;
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na pocz�tku odcinka
    Vert++;
    pt=parallel1*(ShapePoints[j+iNumShapePoints].x-fOffsetX)+pos2; //odsuni�cie
    pt.y+=ShapePoints[j+iNumShapePoints].y; //wysoko��
    Vert->x=pt.x; Vert->y=pt.y; Vert->z=pt.z; //punkt na ko�cu odcinka
    Vert++;
  }
 }
};


