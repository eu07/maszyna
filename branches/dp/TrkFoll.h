//---------------------------------------------------------------------------

#ifndef TrkFollH
#define TrkFollH

#include "Track.h"

class TDynamicObject;

class TTrackFollower
{//o� poruszaj�ca si� po torze
private:
 TTrack *pCurrentTrack; //na kt�rym torze si� znajduje
 TSegment *pCurrentSegment; //zwrotnice mog� mie� dwa segmenty
 double fCurrentDistance; //przesuni�cie wzgl�dem Point1 w stron� Point2
 double fDirection; //ustawienie wzgl�dem toru: -1.0 albo 1.0, mno�one przez dystans
 bool __fastcall ComputatePosition(); //przeliczenie pozycji na torze
 TDynamicObject *Owner; //pojazd posiadaj�cy
 int iEventFlag; //McZapkie-020602: informacja o tym czy wyzwalac zdarzenie: 0,1,2,3
 int iEventallFlag;
 int iSegment; //kt�ry segment toru jest u�ywany (�eby nie przeskakiwa�o po przestawieniu zwrotnicy pod taborem)
public:
 double fOffsetH; //Ra: odleg�o�� �rodka osi od osi toru (dla samochod�w)
 vector3 pPosition; //wsp�rz�dne XYZ w uk�adzie scenerii
 vector3 vAngles; //x:przechy�ka, y:pochylenie, z:kierunek w planie (w radianach)
 __fastcall TTrackFollower();
 __fastcall ~TTrackFollower();
 void __fastcall SetCurrentTrack(TTrack *pTrack,int end);
 bool __fastcall Move(double fDistance,bool bPrimary);
 inline TTrack* __fastcall GetTrack() {return pCurrentTrack;};
 inline double __fastcall GetRoll() {return vAngles.x;}; //przechy�ka policzona przy ustalaniu pozycji
 //{return pCurrentSegment->GetRoll(fCurrentDistance)*fDirection;}; //zamiast liczy� mo�na pobra�
 inline double __fastcall GetDirection() {return fDirection;}; //zwrot na torze
 inline double __fastcall GetTranslation() {return fCurrentDistance;};  //ABu-030403
 //inline double __fastcall GetLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
 //{ return pCurrentSegment->ComputeLength(p1,cp1,cp2,p2); };
 //inline double __fastcall GetRadius(double L, double d);  //McZapkie-150503
 bool __fastcall Init(TTrack *pTrack,TDynamicObject *NewOwner,double fDir);
 void __fastcall Render(float fNr);
};
//---------------------------------------------------------------------------
#endif
