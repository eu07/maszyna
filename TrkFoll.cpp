/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "stdafx.h"
#include "TrkFoll.h"

#include "simulation.h"
#include "Globals.h"
#include "DynObj.h"
#include "Driver.h"
#include "Logs.h"

TTrackFollower::~TTrackFollower()
{
}

bool TTrackFollower::Init(TTrack *pTrack, TDynamicObject *NewOwner, double fDir)
{
    fDirection = fDir;
    Owner = NewOwner;
    SetCurrentTrack(pTrack, 0);
    iEventFlag = 3; // na torze startowym również wykonać eventy 1/2
    iEventallFlag = 3;
    if ((pCurrentSegment)) // && (pCurrentSegment->GetLength()<fFirstDistance))
        return false;
    return true;
}

void TTrackFollower::Reset()
{
	fCurrentDistance = 0.0;
	fDirection = 1.0;
}

TTrack * TTrackFollower::SetCurrentTrack(TTrack *pTrack, int end)
{ // przejechanie na inny odcinkek toru, z ewentualnym rozpruciem
    if (pTrack)
        switch (pTrack->eType)
        {
        case tt_Switch: // jeśli zwrotnica, to przekładamy ją, aby uzyskać dobry segment
        {
            int i = (end ? pCurrentTrack->iNextDirection : pCurrentTrack->iPrevDirection);
            if (i > 0) // jeżeli wjazd z ostrza
                pTrack->SwitchForced(i >> 1, Owner); // to przełożenie zwrotnicy - rozprucie!
        }
        break;
        case tt_Cross: // skrzyżowanie trzeba tymczasowo przełączyć, aby wjechać na właściwy tor
        {
            iSegment = Owner->RouteWish(pTrack); // nr segmentu został ustalony podczas skanowania
            // Ra 2014-08: aby ustalić dalszą trasę, należy zapytać AI - trasa jest ustalana podczas
            // skanowania
            // Ra 15-01: zapytanie AI nie wybiera segmentu - kolejny skanujący może przestawić
            // pTrack->CrossSegment(end?pCurrentTrack->iNextDirection:pCurrentTrack->iPrevDirection,i);
            // //ustawienie właściwego wskaźnika
            // powinno zwracać kierunek do zapamiętania, bo segmenty mogą mieć różny kierunek
            // if fDirection=(iSegment>0)?1.0:-1.0; //kierunek na tym segmencie jest ustalany
            // bezpośrednio
            if (iSegment == 0)
            { // to jest błędna sytuacja - generuje pętle zawracające za skrzyżowaniem - ustalić,
                // kiedy powstaje!
                iSegment = 1; // doraźna poprawka
            }
            if ((end ? iSegment : -iSegment) < 0)
                fDirection = -fDirection; // wtórna zmiana
            pTrack->SwitchForced(abs(iSegment) - 1, NULL); // wybór zapamiętanego segmentu
        }
        break;
        }
    if (!pTrack) {
        // gdy nie ma toru w kierunku jazdy tworzenie toru wykolejącego na przedłużeniu pCurrentTrack
        pTrack = pCurrentTrack->NullCreate(end);
        if (!end) // jeśli dodana od strony zero, to zmiana kierunku
            fDirection = -fDirection; // wtórna zmiana
    }
    else
    { // najpierw +1, później -1, aby odcinek izolowany wspólny dla tych torów nie wykrył zera
        pTrack->AxleCounter(+1, Owner); // zajęcie nowego toru
        if (pCurrentTrack)
            pCurrentTrack->AxleCounter(-1, Owner); // opuszczenie tamtego toru
    }
    pCurrentTrack = pTrack;
    pCurrentSegment = ( pCurrentTrack != nullptr ? pCurrentTrack->CurrentSegment() : nullptr );
    if (!pCurrentTrack)
        Error(Owner->MoverParameters->Name + " at NULL track");
    return pCurrentTrack;
};

bool TTrackFollower::Move(double fDistance, bool bPrimary)
{ // przesuwanie wózka po torach o odległość (fDistance), z wyzwoleniem eventów
    // bPrimary=true - jest pierwszą osią w pojeździe, czyli generuje eventy i przepisuje pojazd
    // Ra: zwraca false, jeśli pojazd ma być usunięty
    auto const ismoving { /* ( std::abs( fDistance ) > 0.01 ) && */ ( Owner->GetVelocity() > 0.01 ) };
    fDistance *= fDirection; // dystans mnożnony przez kierunek
    double s; // roboczy dystans
    double dir; // zapamiętany kierunek do sprawdzenia, czy się zmienił
    bool bCanSkip; // czy przemieścić pojazd na inny tor
    while (true) // pętla wychodzi, gdy przesunięcie wyjdzie zerowe
    { // pętla przesuwająca wózek przez kolejne tory, aż do trafienia w jakiś
        if( pCurrentTrack == nullptr ) { return false; } // nie ma toru, to nie ma przesuwania
        // TODO: refactor following block as track method
        if( pCurrentTrack->m_events ) { // sumaryczna informacja o eventach
            // omijamy cały ten blok, gdy tor nie ma on żadnych eventów (większość nie ma)
            int const eventfilter { (
                false == ismoving ? 0 : // only moving vehicles activate events type 1/2
                false == bPrimary ? 0 : // only primary axle activates events type 1/2
                Owner->ctOwner == nullptr ?
                    ( fDistance > 0 ? 1 : -1 ) : // loose vehicle has no means to determine 'intended' direction so the filter does effectively nothing in such case
                    ( fDirection > 0 ? 1 : -1 ) * Owner->ctOwner->Direction() * ( Owner->ctOwner->Vehicle()->DirectionGet() == Owner->DirectionGet() ? 1 : -1 ) ) };

            if( false == ismoving ) {
                //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
                if( ( Owner->Mechanik != nullptr )
                 && ( Owner->Mechanik->primary() ) ) {
                    // tylko dla jednego członu
                    pCurrentTrack->QueueEvents( pCurrentTrack->m_events0, Owner );
                }
                pCurrentTrack->QueueEvents( pCurrentTrack->m_events0all, Owner );
            }
            else if( (fDistance < 0) && ( eventfilter < 0 ) ) {
                // event1, eventall1
                if( SetFlag( iEventFlag, -1 ) ) {
                    // zawsze zeruje flagę sprawdzenia, jak mechanik dosiądzie, to się nie wykona
                    if( ( Owner->Mechanik != nullptr )
                     && ( Owner->Mechanik->primary() ) ) {
                        // tylko dla jednego członu
                        // McZapkie-280503: wyzwalanie event tylko dla pojazdow z obsada
                        pCurrentTrack->QueueEvents( pCurrentTrack->m_events1, Owner );
                    }
                }
                if( SetFlag( iEventallFlag, -1 ) ) {
                    // McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
                    pCurrentTrack->QueueEvents( pCurrentTrack->m_events1all, Owner );
                }
            }
            else if( ( fDistance > 0 ) && ( eventfilter > 0 ) ) {
                // event2, eventall2
                if( SetFlag( iEventFlag, -2 ) ) {
                    // zawsze ustawia flagę sprawdzenia, jak mechanik dosiądzie, to się nie wykona
                    if( ( Owner->Mechanik != nullptr )
                     && ( Owner->Mechanik->primary() ) ) {
                        // tylko dla jednego członu
                         pCurrentTrack->QueueEvents( pCurrentTrack->m_events2, Owner );
                    }
                }
                if( SetFlag( iEventallFlag, -2 ) ) {
                    // sprawdza i zeruje na przyszłość, true jeśli zmieni z 2 na 0
                    pCurrentTrack->QueueEvents( pCurrentTrack->m_events2all, Owner );
                }
            }
        }
        if (!pCurrentSegment) // jeżeli nie ma powiązanego segmentu toru?
            return false;
        // if (fDistance==0.0) return true; //Ra: jak stoi, to chyba dalej nie ma co kombinować?
        s = fCurrentDistance + fDistance; // doliczenie przesunięcia
        // Ra: W Point2 toru może znajdować się "dziura", która zamieni energię kinetyczną
        // ruchu wzdłużnego na energię potencjalną, zamieniającą się potem na energię
        // sprężystości na amortyzatorach. Należałoby we wpisie toru umieścić współczynnik
        // podziału energii kinetycznej.
        // Współczynnik normalnie 1, z dziurą np. 0.99, a -1 będzie oznaczało 100% odbicia (kozioł).
        // Albo w postaci kąta: normalnie 0°, a 180° oznacza 100% odbicia (cosinus powyższego).
        /*
          if (pCurrentTrack->eType==tt_Cross)
          {//zjazdu ze skrzyżowania nie da się określić przez (iPrevDirection) i (iNextDirection)
           //int segment=Owner->RouteWish(pCurrentTrack); //numer segmentu dla skrzyżowań
           //pCurrentTrack->SwitchForced(abs(segment)-1,NULL); //tymczasowo ustawienie tego segmentu
           //pCurrentSegment=pCurrentTrack->CurrentSegment(); //odświeżyć sobie wskaźnik segmentu
          (?)
          }
        */
        if (s < 0)
        { // jeśli przekroczenie toru od strony Point1
            bCanSkip = ( bPrimary && pCurrentTrack->CheckDynamicObject( Owner ) );
            if( bCanSkip ) {
                // tylko główna oś przenosi pojazd do innego toru
                // zdejmujemy pojazd z dotychczasowego toru
                Owner->MyTrack->RemoveDynamicObject( Owner );
            }
            dir = fDirection;
            if (pCurrentTrack->eType == tt_Cross)
            {
                if (!SetCurrentTrack(pCurrentTrack->Connected(iSegment, fDirection), 0))
                    return false; // wyjście z błędem
            }
            else if (!SetCurrentTrack(pCurrentTrack->Connected(-1, fDirection), 0)) // ustawia fDirection
                return false; // wyjście z błędem
            if (dir == fDirection) //(pCurrentTrack->iPrevDirection)
            { // gdy kierunek bez zmiany (Point1->Point2)
                fCurrentDistance = pCurrentSegment->GetLength();
                fDistance = s;
            }
            else
            { // gdy zmiana kierunku toru (Point1->Point1)
                fCurrentDistance = 0;
                fDistance = -s;
            }
            if (bCanSkip)
            { // jak główna oś, to dodanie pojazdu do nowego toru
                pCurrentTrack->AddDynamicObject(Owner);
                iEventFlag = 3; // McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
                iEventallFlag = 3; // McZapkie-280503: jw, dla eventall1,2
                if (!Owner->MyTrack)
                    return false;
            }
            continue;
        }
        else if (s > pCurrentSegment->GetLength())
        { // jeśli przekroczenie toru od strony Point2
            bCanSkip = ( bPrimary && pCurrentTrack->CheckDynamicObject( Owner ) );
            if (bCanSkip) // tylko główna oś przenosi pojazd do innego toru
                Owner->MyTrack->RemoveDynamicObject(Owner); // zdejmujemy pojazd z dotychczasowego toru
            fDistance = s - pCurrentSegment->GetLength();
            dir = fDirection;
            if (pCurrentTrack->eType == tt_Cross)
            {
                if (!SetCurrentTrack(pCurrentTrack->Connected(iSegment, fDirection), 1))
                    return false; // wyjście z błędem
            }
            else if (!SetCurrentTrack(pCurrentTrack->Connected(1, fDirection), 1)) // ustawia fDirection
                return false; // wyjście z błędem
            if (dir != fDirection) //(pCurrentTrack->iNextDirection)
            { // gdy zmiana kierunku toru (Point2->Point2)
                fDistance = -fDistance; //(s-pCurrentSegment->GetLength());
                fCurrentDistance = pCurrentSegment->GetLength();
            }
            else // gdy kierunek bez zmiany (Point2->Point1)
                fCurrentDistance = 0;
            if (bCanSkip)
            { // jak główna oś, to dodanie pojazdu do nowego toru
                pCurrentTrack->AddDynamicObject(Owner);
                iEventFlag = 3; // McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
                iEventallFlag = 3;
                if (!Owner->MyTrack)
                    return false;
            }
            continue;
        }
        else
        { // gdy zostaje na tym samym torze (przesuwanie już nie zmienia toru)
            if (bPrimary)
            { // tylko gdy początkowe ustawienie, dodajemy eventy stania do kolejki
                if (Owner->MoverParameters->CabOccupied != 0) {

                    pCurrentTrack->QueueEvents( pCurrentTrack->m_events1, Owner, -1.0 );
                    pCurrentTrack->QueueEvents( pCurrentTrack->m_events2, Owner, -1.0 );
                }
                pCurrentTrack->QueueEvents( pCurrentTrack->m_events1all, Owner, -1.0 );
                pCurrentTrack->QueueEvents( pCurrentTrack->m_events2all, Owner, -1.0 );
            }
            fCurrentDistance = s;
            return ComputatePosition(); // przeliczenie XYZ, true o ile nie wyjechał na NULL
        }
    }
};

bool TTrackFollower::ComputatePosition()
{ // ustalenie współrzędnych XYZ
    if (pCurrentSegment) // o ile jest tor
    {
        pCurrentSegment->RaPositionGet(fCurrentDistance, pPosition, vAngles);
        if (fDirection < 0) // kąty zależą jeszcze od zwrotu na torze
        { // kąty są w przedziale <-M_PI;M_PI>
            vAngles.x = -vAngles.x; // przechyłka jest w przecinwą stronę
            vAngles.y = -vAngles.y; // pochylenie jest w przecinwą stronę
            vAngles.z +=
                (vAngles.z >= M_PI) ? -M_PI : M_PI; // ale kierunek w planie jest obrócony o 180°
        }
        if (fOffsetH != 0.0)
        { // jeśli przesunięcie względem osi toru, to je doliczyć
        }
        return true;
    }
    return false;
}
