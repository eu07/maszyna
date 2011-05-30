//---------------------------------------------------------------------------

#ifndef SegmentH
#define SegmentH

#include "usefull.h"
#include "Geometry.h"
#include "VBO.h"

//110405 Ra: klasa punktów przekroju z normalnymi

class vector6 : public vector3
{//punkt przekroju wraz z wektorem normalnym
public:
 vector3 n;
 __fastcall vector6()
 {x=y=z=n.x=n.z=0.0; n.y=1.0;};
 __fastcall vector6(double a,double b,double c,double d,double e,double f)
 //{x=a; y=b; z=c; n.x=d; n.y=e; n.z=f;};
 {x=a; y=b; z=c; n.x=0.0; n.y=1.0; n.z=0.0;}; //Ra: bo na razie s¹ z tym problemy
 __fastcall vector6(double a,double b,double c)
 {x=a; y=b; z=c; n.x=0.0; n.y=1.0; n.z=0.0;};
};

class TTrack; //potrzebny jest wskaŸnik na w³aœciciela

class TSegment
{//aproksymacja toru (zwrotnica ma dwa takie, jeden z nich jest aktywny)
private:
 vector3 Point1,CPointOut,CPointIn,Point2;
 double fRoll1,fRoll2;
 double fLength;
 double *fTsBuffer;
 double fStep;
 int iSegCount; //iloœæ odcinków do rysowania krzywej
 double fDirection; //Ra: k¹t prostego w planie, dla ³uku k¹t od Point1
 double fStoop; //Ra: k¹t wzniesienia, dla ³uku od Point1
 vector3 vA,vB,vC; //wspó³czynniki wielomianów trzeciego stopnia vD==Point1
 //TSegment *pPrev; //odcinek od strony punktu 1 - w segmencie, ¿eby nie skakaæ na zwrotnicach
 //TSegment *pNext; //odcinek od strony punktu 2
 TTrack *pOwner; //wskaŸnik na w³aœciciela
 vector3 __fastcall GetFirstDerivative(double fTime);
 double __fastcall RombergIntegral(double fA, double fB);
 double __fastcall GetTFromS(double s);
 vector3 __fastcall RaInterpolate(double t);
public:
 bool bCurve;
 //int iShape; //Ra: flagi kszta³tu dadz¹ wiêcej mo¿liwoœci optymalizacji
 __fastcall TSegment(TTrack *owner);
 __fastcall ~TSegment() {SafeDeleteArray(fTsBuffer);};
 bool __fastcall Init(vector3 NewPoint1,vector3 NewPoint2,double fNewStep,
                      double fNewRoll1=0,double fNewRoll2=0);
 bool __fastcall Init(vector3 NewPoint1,vector3 NewCPointOut,
                      vector3 NewCPointIn,vector3 NewPoint2,double fNewStep,
                      double fNewRoll1=0,double fNewRoll2=0,bool bIsCurve=true);
 inline double __fastcall ComputeLength(vector3 p1,vector3 cp1,vector3 cp2,vector3 p2);  //McZapkie-150503
 inline vector3 __fastcall GetDirection1() {return bCurve?CPointOut-Point1:CPointOut;};
 inline vector3 __fastcall GetDirection2() {return bCurve?Point2-CPointIn:-CPointIn;};
 vector3 __fastcall GetDirection(double fDistance);
 vector3 __fastcall GetDirection() {return CPointOut;};
 vector3 __fastcall FastGetDirection(double fDistance,double fOffset);
 vector3 __fastcall GetPoint(double fDistance);
 void __fastcall RaPositionGet(double fDistance,vector3 &p,vector3 &a);
 vector3 __fastcall FastGetPoint(double t);
 inline vector3 __fastcall FastGetPoint_0() {return Point1;};
 inline vector3 __fastcall FastGetPoint_1() {return Point2;};
 inline double __fastcall GetRoll(double s)
 {
  s/=fLength;
  return ((1.0-s)*fRoll1+s*fRoll2);
 }
 void __fastcall GetRolls(double &r1,double &r2)
 {//pobranie przechy³ek (do generowania trójk¹tów)
  r1=fRoll1; r2=fRoll2;
 }
 void __fastcall RenderLoft(const vector6 *ShapePoints,int iNumShapePoints,
  double fTextureLength,int iSkip=0,int iQualityFactor=1);
 void __fastcall RenderSwitchRail(const vector6 *ShapePoints1,const vector6 *ShapePoints2,
  int iNumShapePoints,double fTextureLength,int iSkip=0,double fOffsetX=0.0f);
 void __fastcall Render();
 inline double __fastcall GetLength() {return fLength;};
 void __fastcall MoveMe(vector3 pPosition)
 {Point1+=pPosition;
  Point2+=pPosition;
  if (bCurve)
  {CPointIn+=pPosition;
   CPointOut+=pPosition;
  }
 }
 int __fastcall RaSegCount() {return fTsBuffer?iSegCount:1;};
 void __fastcall RaRenderLoft(CVertNormTex* &Vert,const vector6 *ShapePoints,int iNumShapePoints,
  double fTextureLength,int iSkip=0,int iEnd=0,double fOffsetX=0.0);
 void __fastcall RaAnimate(CVertNormTex* &Vert,const vector6 *ShapePoints,int iNumShapePoints,
  double fTextureLength,int iSkip=0,int iEnd=0,double fOffsetX=0.0);
};

//---------------------------------------------------------------------------
#endif
