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

#include "Classes.h"
#include "Segment.h"

// oś poruszająca się po torze
class TTrackFollower {

public:
// constructors
    TTrackFollower() = default;
// destructor
    ~TTrackFollower();
// methods
    TTrack * SetCurrentTrack(TTrack *pTrack, int end);
    bool Move(double fDistance, bool bPrimary);
    inline TTrack * GetTrack() const {
        return pCurrentTrack; };
    // przechyłka policzona przy ustalaniu pozycji
    inline double GetRoll() {
        return vAngles.x; };
    //{return pCurrentSegment->GetRoll(fCurrentDistance)*fDirection;}; //zamiast liczyć można pobrać
    // zwrot na torze
    inline double GetDirection() const {
        return fDirection; };
    // ABu-030403
    inline double GetTranslation() const {
        return fCurrentDistance; };
    // inline double GetLength(vector3 p1, vector3 cp1, vector3 cp2, vector3 p2)
    //{ return pCurrentSegment->ComputeLength(p1,cp1,cp2,p2); };
    // inline double GetRadius(double L, double d);  //McZapkie-150503
    bool Init(TTrack *pTrack, TDynamicObject *NewOwner, double fDir);
	void Reset();
    void Render(float fNr);
// members
    double fOffsetH = 0.0; // Ra: odległość środka osi od osi toru (dla samochodów) - użyć do wężykowania
    Math3D::vector3 pPosition; // współrzędne XYZ w układzie scenerii
    Math3D::vector3 vAngles; // x:przechyłka, y:pochylenie, z:kierunek w planie (w radianach)
private:
// methods
    bool ComputatePosition(); // przeliczenie pozycji na torze
// members
    TTrack *pCurrentTrack = nullptr; // na którym torze siê znajduje
	std::shared_ptr<TSegment> pCurrentSegment; // zwrotnice mog¹ mieæ dwa segmenty
	double fCurrentDistance = 0.0; // przesuniêcie wzglêdem Point1 w stronê Point2
    double fDirection = 1.0; // ustawienie wzglêdem toru: -1.0 albo 1.0, mno¿one przez dystans // jest przodem do Point2
    TDynamicObject *Owner = nullptr; // pojazd posiadający
    int iEventFlag = 0; // McZapkie-020602: informacja o tym czy wyzwalac zdarzenie: 0,1,2,3
    int iEventallFlag = 0;
    int iSegment = 0; // który segment toru jest używany (żeby nie przeskakiwało po przestawieniu zwrotnicy pod taborem)
};
//---------------------------------------------------------------------------
#endif
