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

TTrackFollower::TTrackFollower()
{
    pCurrentTrack = NULL;
    pCurrentSegment = NULL;
    fCurrentDistance = 0;
    pPosition = vAngles = vector3(0, 0, 0);
    fDirection = 1; // jest przodem do Point2
    fOffsetH = 0.0; // na starcie stoi na œrodku
}

TTrackFollower::~TTrackFollower()
{
}

bool TTrackFollower::Init(TTrack *pTrack, TDynamicObject *NewOwner, double fDir)
{
    fDirection = fDir;
    Owner = NewOwner;
    SetCurrentTrack(pTrack, 0);
    iEventFlag = 3; // na torze startowym równie¿ wykonaæ eventy 1/2
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
        case tt_Switch: // jeœli zwrotnica, to przek³adamy j¹, aby uzyskaæ dobry segment
        {
            int i = (end ? pCurrentTrack->iNextDirection : pCurrentTrack->iPrevDirection);
            if (i > 0) // je¿eli wjazd z ostrza
                pTrack->SwitchForced(i >> 1, Owner); // to prze³o¿enie zwrotnicy - rozprucie!
        }
        break;
        case tt_Cross: // skrzy¿owanie trzeba tymczasowo prze³¹czyæ, aby wjechaæ na w³aœciwy tor
        {
            iSegment = Owner->RouteWish(pTrack); // nr segmentu zosta³ ustalony podczas skanowania
            // Ra 2014-08: aby ustaliæ dalsz¹ trasê, nale¿y zapytaæ AI - trasa jest ustalana podczas
            // skanowania
            // Ra 15-01: zapytanie AI nie wybiera segmentu - kolejny skanuj¹cy mo¿e przestawiæ
            // pTrack->CrossSegment(end?pCurrentTrack->iNextDirection:pCurrentTrack->iPrevDirection,i);
            // //ustawienie w³aœciwego wskaŸnika
            // powinno zwracaæ kierunek do zapamiêtania, bo segmenty mog¹ mieæ ró¿ny kierunek
            // if fDirection=(iSegment>0)?1.0:-1.0; //kierunek na tym segmencie jest ustalany
            // bezpoœrednio
            if (iSegment == 0)
            { // to jest b³êdna sytuacja - generuje pêtle zawracaj¹ce za skrzy¿owaniem - ustaliæ,
                // kiedy powstaje!
                iSegment = 1; // doraŸna poprawka
            }
            if ((end ? iSegment : -iSegment) < 0)
                fDirection = -fDirection; // wtórna zmiana
            pTrack->SwitchForced(abs(iSegment) - 1, NULL); // wybór zapamiêtanego segmentu
        }
        break;
        }
    if (!pTrack)
    { // gdy nie ma toru w kierunku jazdy
        pTrack = pCurrentTrack->NullCreate(
            end); // tworzenie toru wykolej¹cego na przed³u¿eniu pCurrentTrack
        if (!end) // jeœli dodana od strony zero, to zmiana kierunku
            fDirection = -fDirection; // wtórna zmiana
        // if (pTrack->iCategoryFlag&2)
        //{//jeœli samochód, zepsuæ na miejscu
        // Owner->MoverParameters->V=0; //zatrzymaæ
        // Owner->MoverParameters->Power=0; //ukraœæ silnik
        // Owner->MoverParameters->AccS=0; //wch³on¹æ moc
        // Global::iPause|=1; //zapauzowanie symulacji
        //}
    }
    else
    { // najpierw +1, póŸniej -1, aby odcinek izolowany wspólny dla tych torów nie wykry³ zera
        pTrack->AxleCounter(+1, Owner); // zajêcie nowego toru
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
{ // przesuwanie wózka po torach o odleg³oœæ (fDistance), z wyzwoleniem eventów
    // bPrimary=true - jest pierwsz¹ osi¹ w pojeŸdzie, czyli generuje eventy i przepisuje pojazd
    // Ra: zwraca false, jeœli pojazd ma byæ usuniêty
    fDistance *= fDirection; // dystans mno¿nony przez kierunek
    double s; // roboczy dystans
    double dir; // zapamiêtany kierunek do sprawdzenia, czy siê zmieni³
    bool bCanSkip; // czy przemieœciæ pojazd na inny tor
    while (true) // pêtla wychodzi, gdy przesuniêcie wyjdzie zerowe
    { // pêtla przesuwaj¹ca wózek przez kolejne tory, a¿ do trafienia w jakiœ
        if (!pCurrentTrack)
            return false; // nie ma toru, to nie ma przesuwania
        if (pCurrentTrack->iEvents) // sumaryczna informacja o eventach
        { // omijamy ca³y ten blok, gdy tor nie ma on ¿adnych eventów (wiêkszoœæ nie ma)
            if (fDistance < 0)
            {
                if (iSetFlag(iEventFlag, -1)) // zawsze zeruje flagê sprawdzenia, jak mechanik
                    // dosi¹dzie, to siê nie wykona
                    if (Owner->Mechanik->Primary()) // tylko dla jednego cz³onu
                        // if (TestFlag(iEventFlag,1)) //McZapkie-280503: wyzwalanie event tylko dla
                        // pojazdow z obsada
                        if (bPrimary && pCurrentTrack->evEvent1 &&
                            (!pCurrentTrack->evEvent1->iQueued))
                            Global::AddToQuery(pCurrentTrack->evEvent1, Owner); // dodanie do
                // kolejki
                // Owner->RaAxleEvent(pCurrentTrack->Event1); //Ra: dynamic zdecyduje, czy dodaæ do
                // kolejki
                // if (TestFlag(iEventallFlag,1))
                if (iSetFlag(iEventallFlag,
                             -1)) // McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
                    if (bPrimary && pCurrentTrack->evEventall1 &&
                        (!pCurrentTrack->evEventall1->iQueued))
                        Global::AddToQuery(pCurrentTrack->evEventall1, Owner); // dodanie do kolejki
                // Owner->RaAxleEvent(pCurrentTrack->Eventall1); //Ra: dynamic zdecyduje, czy dodaæ
                // do kolejki
            }
            else if (fDistance > 0)
            {
                if (iSetFlag(iEventFlag, -2)) // zawsze ustawia flagê sprawdzenia, jak mechanik
                    // dosi¹dzie, to siê nie wykona
                    if (Owner->Mechanik->Primary()) // tylko dla jednego cz³onu
                        // if (TestFlag(iEventFlag,2)) //sprawdzanie jest od razu w pierwszym
                        // warunku
                        if (bPrimary && pCurrentTrack->evEvent2 &&
                            (!pCurrentTrack->evEvent2->iQueued))
                            Global::AddToQuery(pCurrentTrack->evEvent2, Owner);
                // Owner->RaAxleEvent(pCurrentTrack->Event2); //Ra: dynamic zdecyduje, czy dodaæ do
                // kolejki
                // if (TestFlag(iEventallFlag,2))
                if (iSetFlag(iEventallFlag,
                             -2)) // sprawdza i zeruje na przysz³oœæ, true jeœli zmieni z 2 na 0
                    if (bPrimary && pCurrentTrack->evEventall2 &&
                        (!pCurrentTrack->evEventall2->iQueued))
                        Global::AddToQuery(pCurrentTrack->evEventall2, Owner);
                // Owner->RaAxleEvent(pCurrentTrack->Eventall2); //Ra: dynamic zdecyduje, czy dodaæ
                // do kolejki
            }
            else // if (fDistance==0) //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
            {
                if (Owner->Mechanik->Primary()) // tylko dla jednego cz³onu
                    if (pCurrentTrack->evEvent0)
                        if (!pCurrentTrack->evEvent0->iQueued)
                            Global::AddToQuery(pCurrentTrack->evEvent0, Owner);
                // Owner->RaAxleEvent(pCurrentTrack->Event0); //Ra: dynamic zdecyduje, czy dodaæ do
                // kolejki
                if (pCurrentTrack->evEventall0)
                    if (!pCurrentTrack->evEventall0->iQueued)
                        Global::AddToQuery(pCurrentTrack->evEventall0, Owner);
                // Owner->RaAxleEvent(pCurrentTrack->Eventall0); //Ra: dynamic zdecyduje, czy dodaæ
                // do kolejki
            }
        }
        if (!pCurrentSegment) // je¿eli nie ma powi¹zanego segmentu toru?
            return false;
        // if (fDistance==0.0) return true; //Ra: jak stoi, to chyba dalej nie ma co kombinowaæ?
        s = fCurrentDistance + fDistance; // doliczenie przesuniêcia
        // Ra: W Point2 toru mo¿e znajdowaæ siê "dziura", która zamieni energiê kinetyczn¹
        // ruchu wzd³u¿nego na energiê potencjaln¹, zamieniaj¹c¹ siê potem na energiê
        // sprê¿ystoœci na amortyzatorach. Nale¿a³oby we wpisie toru umieœciæ wspó³czynnik
        // podzia³u energii kinetycznej.
        // Wspó³czynnik normalnie 1, z dziur¹ np. 0.99, a -1 bêdzie oznacza³o 100% odbicia (kozio³).
        // Albo w postaci k¹ta: normalnie 0°, a 180° oznacza 100% odbicia (cosinus powy¿szego).
        /*
          if (pCurrentTrack->eType==tt_Cross)
          {//zjazdu ze skrzy¿owania nie da siê okreœliæ przez (iPrevDirection) i (iNextDirection)
           //int segment=Owner->RouteWish(pCurrentTrack); //numer segmentu dla skrzy¿owañ
           //pCurrentTrack->SwitchForced(abs(segment)-1,NULL); //tymczasowo ustawienie tego segmentu
           //pCurrentSegment=pCurrentTrack->CurrentSegment(); //odœwie¿yæ sobie wskaŸnik segmentu
          (?)
          }
        */
        if (s < 0)
        { // jeœli przekroczenie toru od strony Point1
            bCanSkip = bPrimary ? pCurrentTrack->CheckDynamicObject(Owner) : false;
            if (bCanSkip) // tylko g³ówna oœ przenosi pojazd do innego toru
                Owner->MyTrack->RemoveDynamicObject(
                    Owner); // zdejmujemy pojazd z dotychczasowego toru
            dir = fDirection;
            if (pCurrentTrack->eType == tt_Cross)
            {
                if (!SetCurrentTrack(pCurrentTrack->Neightbour(iSegment, fDirection), 0))
                    return false; // wyjœcie z b³êdem
            }
            else if (!SetCurrentTrack(pCurrentTrack->Neightbour(-1, fDirection),
                                      0)) // ustawia fDirection
                return false; // wyjœcie z b³êdem
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
            { // jak g³ówna oœ, to dodanie pojazdu do nowego toru
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
        { // jeœli przekroczenie toru od strony Point2
            bCanSkip = bPrimary ? pCurrentTrack->CheckDynamicObject(Owner) : false;
            if (bCanSkip) // tylko g³ówna oœ przenosi pojazd do innego toru
                Owner->MyTrack->RemoveDynamicObject(
                    Owner); // zdejmujemy pojazd z dotychczasowego toru
            fDistance = s - pCurrentSegment->GetLength();
            dir = fDirection;
            if (pCurrentTrack->eType == tt_Cross)
            {
                if (!SetCurrentTrack(pCurrentTrack->Neightbour(iSegment, fDirection), 1))
                    return false; // wyjœcie z b³êdem
            }
            else if (!SetCurrentTrack(pCurrentTrack->Neightbour(1, fDirection),
                                      1)) // ustawia fDirection
                return false; // wyjœcie z b³êdem
            if (dir != fDirection) //(pCurrentTrack->iNextDirection)
            { // gdy zmiana kierunku toru (Point2->Point2)
                fDistance = -fDistance; //(s-pCurrentSegment->GetLength());
                fCurrentDistance = pCurrentSegment->GetLength();
            }
            else // gdy kierunek bez zmiany (Point2->Point1)
                fCurrentDistance = 0;
            if (bCanSkip)
            { // jak g³ówna oœ, to dodanie pojazdu do nowego toru
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
        { // gdy zostaje na tym samym torze (przesuwanie ju¿ nie zmienia toru)
            if (bPrimary)
            { // tylko gdy pocz¹tkowe ustawienie, dodajemy eventy stania do kolejki
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
            return ComputatePosition(); // przeliczenie XYZ, true o ile nie wyjecha³ na NULL
        }
    }
};

bool TTrackFollower::ComputatePosition()
{ // ustalenie wspó³rzêdnych XYZ
    if (pCurrentSegment) // o ile jest tor
    {
        pCurrentSegment->RaPositionGet(fCurrentDistance, pPosition, vAngles);
        if (fDirection < 0) // k¹ty zale¿¹ jeszcze od zwrotu na torze
        { // k¹ty s¹ w przedziale <-M_PI;M_PI>
            vAngles.x = -vAngles.x; // przechy³ka jest w przecinw¹ stronê
            vAngles.y = -vAngles.y; // pochylenie jest w przecinw¹ stronê
            vAngles.z +=
                (vAngles.z >= M_PI) ? -M_PI : M_PI; // ale kierunek w planie jest obrócony o 180°
        }
        if (fOffsetH != 0.0)
        { // jeœli przesuniêcie wzglêdem osi toru, to je doliczyæ
        }
        return true;
    }
    return false;
}
#if RENDER_CONE
#include "opengl/glew.h"
#include "opengl/glut.h"
void TTrackFollower::Render(float fNr)
{ // funkcja rysuj¹ca sto¿ek w miejscu osi
    glPushMatrix(); // matryca kamery
    glTranslatef(pPosition.x, pPosition.y + 6, pPosition.z); // 6m ponad
    glRotated(RadToDeg(-vAngles.z), 0, 1, 0); // obrót wzglêdem osi OY
    // glRotated(RadToDeg(vAngles.z),0,1,0); //obrót wzglêdem osi OY
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0 - fNr, 1.0 - fNr); // bia³y dla 0, czerwony dla 1
    // glutWireCone(promieñ podstawy,wysokoœæ,k¹tnoœæ podstawy,iloœæ segmentów na wysokoœæ)
    glutWireCone(0.5, 2, 4, 1); // rysowanie sto¿ka (ostros³upa o podstawie wieloboka)
    glEnable(GL_LIGHTING);
    glPopMatrix();
}
#endif
//---------------------------------------------------------------------------
