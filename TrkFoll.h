/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef TrkFollH
#define TrkFollH

#include "Track.h"
#include "McZapkie/MOVER.h"

class TTrackFollower
{ // oś poruszająca się po torze
  private:
    TTrack *pCurrentTrack = nullptr; // na którym torze siê znajduje
	std::shared_ptr<TSegment> pCurrentSegment; // zwrotnice mog¹ mieæ dwa segmenty
	double fCurrentDistance = 0.0; // przesuniêcie wzglêdem Point1 w stronê Point2
    double fDirection = 1.0; // ustawienie wzglêdem toru: -1.0 albo 1.0, mno¿one przez dystans // jest przodem do Point2
    bool ComputatePosition(); // przeliczenie pozycji na torze
    TDynamicObject *Owner = nullptr; // pojazd posiadający
    int iEventFlag = 0; // McZapkie-020602: informacja o tym czy wyzwalac zdarzenie: 0,1,2,3
    int iEventallFlag = 0;
    int iSegment = 0; // który segment toru jest używany (żeby nie przeskakiwało po przestawieniu
    // zwrotnicy pod taborem)
  public:
    double fOffsetH = 0.0; // Ra: odległość środka osi od osi toru (dla samochodów) - użyć do wężykowania
    vector3 pPosition; // współrzędne XYZ w układzie scenerii
    vector3 vAngles; // x:przechyłka, y:pochylenie, z:kierunek w planie (w radianach)
    TTrackFollower() = default;
    ~TTrackFollower();
    TTrack * SetCurrentTrack(TTrack *pTrack, int end);
    bool Move(double fDistance, bool bPrimary);
    inline TTrack * GetTrack() const
    {
        return pCurrentTrack;
    };
    inline double GetRoll()
    {
        return vAngles.x;
    }; // przechyłka policzona przy ustalaniu pozycji
    //{return pCurrentSegment->GetRoll(fCurrentDistance)*fDirection;}; //zamiast liczyć można pobrać
    inline double GetDirection() const
    {
        return fDirection;
    }; // zwrot na torze
    inline double GetTranslation()
    {
        return fCurrentDistance;
    }; // ABu-030403
    // inline double GetLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
    //{ return pCurrentSegment->ComputeLength(p1,cp1,cp2,p2); };
    // inline double GetRadius(double L, double d);  //McZapkie-150503
    bool Init(TTrack *pTrack, TDynamicObject *NewOwner, double fDir);
    void Render(float fNr);
};
//---------------------------------------------------------------------------
#endif
