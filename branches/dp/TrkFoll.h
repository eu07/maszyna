//---------------------------------------------------------------------------

#ifndef TrkFollH
#define TrkFollH

#include "Track.h"

class TDynamicObject;

class TTrackFollower
{//wózek poruszaj¹cy sie po torze
private:
 TTrack *pCurrentTrack;
 TSegment *pCurrentSegment;
 double fCurrentDistance;
 double fDirection; //-1.0 albo 1.0, u¿ywane do mno¿enia przez dystans
 bool __fastcall ComputatePosition();
 TDynamicObject *Owner;
 int iEventFlag; //McZapkie-020602: informacja o tym czy wyzwalac zdarzenie: 0,1,2,3
 int iEventallFlag;

public:
 vector3 pPosition;
 __fastcall TTrackFollower();
 __fastcall ~TTrackFollower();
 inline void __fastcall SetCurrentTrack(TTrack *pTrack)
 {
  pCurrentTrack=pTrack;
  pCurrentSegment=(pCurrentTrack?pCurrentTrack->CurrentSegment():NULL);
 };
 bool __fastcall Move(double fDistance, bool bPrimary=false);
 inline TTrack* __fastcall GetTrack() {return pCurrentTrack;};
 inline double __fastcall GetRoll() {return pCurrentSegment->GetRoll(fCurrentDistance)*fDirection;};
 inline double __fastcall GetDirection() {return fDirection;};
 inline double __fastcall GetTranslation() {return fCurrentDistance;};  //ABu-030403
 //inline double __fastcall GetLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
 //{ return pCurrentSegment->ComputeLength(p1,cp1,cp2,p2); };
 //inline double __fastcall GetRadius(double L, double d);  //McZapkie-150503
 bool __fastcall Init(TTrack *pTrack,TDynamicObject *NewOwner=NULL,double fDir=1.0f);
 void __fastcall Render();
};
//---------------------------------------------------------------------------
#endif
