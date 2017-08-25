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
#include "Globals.h"
#include "Logs.h"
#include "Driver.h"
#include "DynObj.h"
#include "Event.h"

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
    if (!pTrack)
    { // gdy nie ma toru w kierunku jazdy
        pTrack = pCurrentTrack->NullCreate(
            end); // tworzenie toru wykolejącego na przedłużeniu pCurrentTrack
        if (!end) // jeśli dodana od strony zero, to zmiana kierunku
            fDirection = -fDirection; // wtórna zmiana
        // if (pTrack->iCategoryFlag&2)
        //{//jeśli samochód, zepsuć na miejscu
        // Owner->MoverParameters->V=0; //zatrzymać
        // Owner->MoverParameters->Power=0; //ukraść silnik
        // Owner->MoverParameters->AccS=0; //wchłonąć moc
        // Global::iPause|=1; //zapauzowanie symulacji
        //}
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
    fDistance *= fDirection; // dystans mnożnony przez kierunek
    double s; // roboczy dystans
    double dir; // zapamiętany kierunek do sprawdzenia, czy się zmienił
    bool bCanSkip; // czy przemieścić pojazd na inny tor
    while (true) // pętla wychodzi, gdy przesunięcie wyjdzie zerowe
    { // pętla przesuwająca wózek przez kolejne tory, aż do trafienia w jakiś
        if (!pCurrentTrack)
            return false; // nie ma toru, to nie ma przesuwania
        if (pCurrentTrack->iEvents) // sumaryczna informacja o eventach
        { // omijamy cały ten blok, gdy tor nie ma on żadnych eventów (większość nie ma)
            if (fDistance < 0)
            {
                if( SetFlag( iEventFlag, -1 ) ) {
                    // zawsze zeruje flagę sprawdzenia, jak mechanik dosiądzie, to się nie wykona
                    if( ( Owner->Mechanik != nullptr )
                     && ( Owner->Mechanik->Primary() ) ) {
                        // tylko dla jednego członu
                        // McZapkie-280503: wyzwalanie event tylko dla pojazdow z obsada
                        if( ( bPrimary )
                         && ( pCurrentTrack->evEvent1 )
                         && ( !pCurrentTrack->evEvent1->iQueued ) ) {
                            // dodanie do kolejki
                            Global::AddToQuery( pCurrentTrack->evEvent1, Owner );
                        }
                    }
                }
                // Owner->RaAxleEvent(pCurrentTrack->Event1); //Ra: dynamic zdecyduje, czy dodać do
                // kolejki
                // if (TestFlag(iEventallFlag,1))
                if (SetFlag(iEventallFlag,
                             -1)) // McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
                    if (bPrimary && pCurrentTrack->evEventall1 &&
                        (!pCurrentTrack->evEventall1->iQueued))
                        Global::AddToQuery(pCurrentTrack->evEventall1, Owner); // dodanie do kolejki
                // Owner->RaAxleEvent(pCurrentTrack->Eventall1); //Ra: dynamic zdecyduje, czy dodać
                // do kolejki
            }
            else if (fDistance > 0)
            {
                if( SetFlag( iEventFlag, -2 ) ) {
                    // zawsze ustawia flagę sprawdzenia, jak mechanik
                    // dosiądzie, to się nie wykona
                    if( ( Owner->Mechanik != nullptr )
                     && ( Owner->Mechanik->Primary() ) ) {
                        // tylko dla jednego członu
                        if( ( bPrimary )
                         && ( pCurrentTrack->evEvent2 )
                         && ( !pCurrentTrack->evEvent2->iQueued ) ) {
                            Global::AddToQuery( pCurrentTrack->evEvent2, Owner );
                        }
                    }
                }
                // Owner->RaAxleEvent(pCurrentTrack->Event2); //Ra: dynamic zdecyduje, czy dodać do
                // kolejki
                // if (TestFlag(iEventallFlag,2))
                if( SetFlag( iEventallFlag, -2 ) ) {
                    // sprawdza i zeruje na przyszłość, true jeśli zmieni z 2 na 0
                    if( ( bPrimary )
                     && ( pCurrentTrack->evEventall2 )
                     && ( !pCurrentTrack->evEventall2->iQueued ) ) {
                        Global::AddToQuery( pCurrentTrack->evEventall2, Owner );
                    }
                }
                // Owner->RaAxleEvent(pCurrentTrack->Eventall2); //Ra: dynamic zdecyduje, czy dodać
                // do kolejki
            }
            else // if (fDistance==0) //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
            {
                if( ( Owner->Mechanik != nullptr )
                 && ( Owner->Mechanik->Primary() ) ) {
                    // tylko dla jednego członu
                    if( pCurrentTrack->evEvent0 )
                        if( !pCurrentTrack->evEvent0->iQueued )
                            Global::AddToQuery( pCurrentTrack->evEvent0, Owner );
                }
                // Owner->RaAxleEvent(pCurrentTrack->Event0); //Ra: dynamic zdecyduje, czy dodać do
                // kolejki
                if (pCurrentTrack->evEventall0)
                    if (!pCurrentTrack->evEventall0->iQueued)
                        Global::AddToQuery(pCurrentTrack->evEventall0, Owner);
                // Owner->RaAxleEvent(pCurrentTrack->Eventall0); //Ra: dynamic zdecyduje, czy dodać
                // do kolejki
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
            bCanSkip = bPrimary ? pCurrentTrack->CheckDynamicObject(Owner) : false;
            if (bCanSkip) // tylko główna oś przenosi pojazd do innego toru
                Owner->MyTrack->RemoveDynamicObject(
                    Owner); // zdejmujemy pojazd z dotychczasowego toru
            dir = fDirection;
            if (pCurrentTrack->eType == tt_Cross)
            {
                if (!SetCurrentTrack(pCurrentTrack->Neightbour(iSegment, fDirection), 0))
                    return false; // wyjście z błędem
            }
            else if (!SetCurrentTrack(pCurrentTrack->Neightbour(-1, fDirection),
                                      0)) // ustawia fDirection
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
                iEventFlag =
                    3; // McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
                iEventallFlag = 3; // McZapkie-280503: jw, dla eventall1,2
                if (!Owner->MyTrack)
                    return false;
            }
            continue;
        }
        else if (s > pCurrentSegment->GetLength())
        { // jeśli przekroczenie toru od strony Point2
            bCanSkip = bPrimary ? pCurrentTrack->CheckDynamicObject(Owner) : false;
            if (bCanSkip) // tylko główna oś przenosi pojazd do innego toru
                Owner->MyTrack->RemoveDynamicObject(
                    Owner); // zdejmujemy pojazd z dotychczasowego toru
            fDistance = s - pCurrentSegment->GetLength();
            dir = fDirection;
            if (pCurrentTrack->eType == tt_Cross)
            {
                if (!SetCurrentTrack(pCurrentTrack->Neightbour(iSegment, fDirection), 1))
                    return false; // wyjście z błędem
            }
            else if (!SetCurrentTrack(pCurrentTrack->Neightbour(1, fDirection),
                                      1)) // ustawia fDirection
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
                iEventFlag =
                    3; // McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
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
                if (Owner->MoverParameters->ActiveCab != 0)
                // if (Owner->MoverParameters->CabNo!=0)
                {
                    if (pCurrentTrack->evEvent1 && pCurrentTrack->evEvent1->fDelay <= -1.0f)
                        Global::AddToQuery(pCurrentTrack->evEvent1, Owner);
                    if (pCurrentTrack->evEvent2 && pCurrentTrack->evEvent2->fDelay <= -1.0f)
                        Global::AddToQuery(pCurrentTrack->evEvent2, Owner);
                }
                if (pCurrentTrack->evEventall1 && pCurrentTrack->evEventall1->fDelay <= -1.0f)
                    Global::AddToQuery(pCurrentTrack->evEventall1, Owner);
                if (pCurrentTrack->evEventall2 && pCurrentTrack->evEventall2->fDelay <= -1.0f)
                    Global::AddToQuery(pCurrentTrack->evEventall2, Owner);
            }
            fCurrentDistance = s;
            // fDistance=0;
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
#if RENDER_CONE
#include "GL/glew.h"
#include "GL/glut.h"
void TTrackFollower::Render(float fNr)
{ // funkcja rysująca stożek w miejscu osi
    glPushMatrix(); // matryca kamery
    glTranslatef(pPosition.x, pPosition.y + 6, pPosition.z); // 6m ponad
    glRotated(RadToDeg(-vAngles.z), 0, 1, 0); // obrót względem osi OY
    // glRotated(RadToDeg(vAngles.z),0,1,0); //obrót względem osi OY
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0 - fNr, 1.0 - fNr); // biały dla 0, czerwony dla 1
    // glutWireCone(promień podstawy,wysokość,kątność podstawy,ilość segmentów na wysokość)
    glutWireCone(0.5, 2, 4, 1); // rysowanie stożka (ostrosłupa o podstawie wieloboka)
    glEnable(GL_LIGHTING);
    glPopMatrix();
}
#endif
//---------------------------------------------------------------------------
