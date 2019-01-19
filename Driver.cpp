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
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "stdafx.h"
#include "Driver.h"

#include "Globals.h"
#include "Logs.h"
#include "Train.h"
#include "mtable.h"
#include "DynObj.h"
#include "Event.h"
#include "MemCell.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Track.h"
#include "station.h"
#include "keyboardinput.h"

#define LOGVELOCITY 0
#define LOGORDERS 1
#define LOGSTOPS 1
#define LOGBACKSCAN 0
#define LOGPRESS 0

// finds point of specified track nearest to specified event. returns: distance to that point from the specified end of the track
// TODO: move this to file with all generic routines, too easy to forget it's here and it may come useful
double
ProjectEventOnTrack( basic_event const *Event, TTrack const *Track, double const Direction ) {

    auto const segment = Track->CurrentSegment();
    auto const nearestpoint = segment->find_nearest_point( Event->input_location() );
    return (
        Direction > 0 ?
            nearestpoint * segment->GetLength() : // measure from point1
            ( 1.0 - nearestpoint ) * segment->GetLength() ); // measure from point2
};

double GetDistanceToEvent(TTrack const *track, basic_event const *event, double scan_dir, double start_dist, int iter = 0, bool back = false)
{
    if( track == nullptr ) { return start_dist; }

    auto const segment = track->CurrentSegment();
    auto const pos_event = event->input_location();
    double len1, len2;
    double sd = scan_dir;
    double seg_len = scan_dir > 0 ? 0.0 : 1.0;
    double const dzielnik = 1.0 / segment->GetLength();// rozdzielczosc mniej wiecej 1m
    int krok = 0; // krok obliczeniowy do sprawdzania czy odwracamy
    len2 = (pos_event - segment->FastGetPoint(seg_len)).LengthSquared();
    do
    {
        len1 = len2;
        seg_len += scan_dir > 0 ? dzielnik : -dzielnik;
        len2 = (pos_event - segment->FastGetPoint(seg_len)).LengthSquared();
        ++krok;
    } while ((len1 > len2) && (seg_len >= dzielnik && (seg_len <= (1.0 - dzielnik))));
    //trzeba sprawdzić czy seg_len nie osiągnął skrajnych wartości, bo wtedy
    // trzeba sprawdzić tor obok
    if (1 == krok)
        sd = -sd; // jeśli tylko jeden krok tzn, że event przy poprzednim sprawdzaym torze
    if (((1 == krok) || (seg_len <= dzielnik) || (seg_len > (1.0 - dzielnik))) && (iter < 3))
    { // przejście na inny tor
        track = track->Connected(int(sd), sd);
        start_dist += (1 == krok) ? 0 : back ? -segment->GetLength() : segment->GetLength();
        if( ( track != nullptr )
         && ( track->eType == tt_Cross ) ) {
            // NOTE: tracing through crossroads currently poses risk of tracing through wrong segment
            // as it's possible to be performerd before setting a route through the crossroads
            // as a stop-gap measure we don't trace through crossroads which should be reasonable in most cases
            // TODO: establish route before the scan, or a way for the function to know which route to pick
            return start_dist;
        }
        else {
            return GetDistanceToEvent( track, event, sd, start_dist, ++iter, 1 == krok ? true : false );
        }
    }
    else
    { // obliczenie mojego toru
        seg_len -= scan_dir > 0 ? dzielnik : -dzielnik; //trzeba wrócić do pozycji len1
        seg_len = scan_dir < 0 ? 1 - seg_len : seg_len;
        seg_len = back ? 1 - seg_len : seg_len; // odwracamy jeśli idzie do tyłu
        start_dist -= back ? segment->GetLength() : 0;
        return start_dist + (segment->GetLength() * seg_len);
    }
};

/*

Moduł obsługujący sterowanie pojazdami (składami pociągów, samochodami).
Ma działać zarówno jako AI oraz przy prowadzeniu przez człowieka. W tym
drugim przypadku jedynie informuje za pomocą napisów o tym, co by zrobił
w tym pierwszym. Obejmuje zarówno maszynistę jak i kierownika pociągu
(dawanie sygnału do odjazdu).

Przeniesiona tutaj została zawartość ai_driver.pas przerobiona na C++.
Również niektóre funkcje dotyczące składów z DynObj.cpp.

Teoria jest wtedy kiedy wszystko wiemy, ale nic nie działa.
Praktyka jest wtedy, kiedy wszystko działa, ale nikt nie wie dlaczego.
Tutaj łączymy teorię z praktyką - tu nic nie działa i nikt nie wie dlaczego…

*/

// zrobione:
// 0. pobieranie komend z dwoma parametrami
// 1. przyspieszanie do zadanej predkosci, ew. hamowanie jesli przekroczona
// 2. hamowanie na zadanym odcinku do zadanej predkosci (ze stabilizacja przyspieszenia)
// 3. wychodzenie z sytuacji awaryjnych: bezpiecznik nadmiarowy, poslizg
// 4. przygotowanie pojazdu do drogi, zmiana kierunku ruchu
// 5. dwa sposoby jazdy - manewrowy i pociagowy
// 6. dwa zestawy psychiki: spokojny i agresywny
// 7. przejscie na zestaw spokojny jesli wystepuje duzo poslizgow lub wybic nadmiarowego.
// 8. lagodne ruszanie (przedluzony czas reakcji na 2 pierwszych nastawnikach)
// 9. unikanie jazdy na oporach rozruchowych
// 10. logowanie fizyki //Ra: nie przeniesione do C++
// 11. kasowanie czuwaka/SHP
// 12. procedury wspomagajace "patrzenie" na odlegle semafory
// 13. ulepszone procedury sterowania
// 14. zglaszanie problemow z dlugim staniem na sygnale S1
// 15. sterowanie EN57
// 16. zmiana kierunku //Ra: z przesiadką po ukrotnieniu
// 17. otwieranie/zamykanie drzwi
// 18. Ra: odczepianie z zahamowaniem i podczepianie
// 19. dla Humandriver: tasma szybkosciomierza - zapis do pliku!
// 20. wybór pozycji zaworu maszynisty w oparciu o zadane opoznienie hamowania


// do zrobienia:
// 1. kierownik pociagu
// 2. madrzejsze unikanie grzania oporow rozruchowych i silnika
// 3. unikanie szarpniec, zerwania pociagu itp
// 4. obsluga innych awarii
// 5. raportowanie problemow, usterek nie do rozwiazania
// 7. samouczacy sie algorytm hamowania

// stałe
const double EasyReactionTime = 0.5; //[s] przebłyski świadomości dla zwykłej jazdy
const double HardReactionTime = 0.2;
const double EasyAcceleration = 0.85; //[m/ss]
const double HardAcceleration = 9.81;
const double PrepareTime = 2.0; //[s] przebłyski świadomości przy odpalaniu
bool WriteLogFlag = false;
double const deltalog = 0.05; // przyrost czasu

std::string StopReasonTable[] = {
    // przyczyny zatrzymania ruchu AI
    "", // stopNone, //nie ma powodu - powinien jechać
    "Off", // stopSleep, //nie został odpalony, to nie pojedzie
    "Semaphore", // stopSem, //semafor zamknięty
    "Time", // stopTime, //czekanie na godzinę odjazdu
    "End of track", // stopEnd, //brak dalszej części toru
    "Change direction", // stopDir, //trzeba stanąć, by zmienić kierunek jazdy
    "Joining", // stopJoin, //zatrzymanie przy (p)odczepianiu
    "Block", // stopBlock, //przeszkoda na drodze ruchu
    "A command", // stopComm, //otrzymano taką komendę (niewiadomego pochodzenia)
    "Out of station", // stopOut, //komenda wyjazdu poza stację (raczej nie powinna zatrzymywać!)
    "Radiostop", // stopRadio, //komunikat przekazany radiem (Radiostop)
    "External", // stopExt, //przesłany z zewnątrz
    "Error", // stopError //z powodu błędu w obliczeniu drogi hamowania
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

TSpeedPos::TSpeedPos(TTrack *track, double dist, int flag)
{
    Set(track, dist, flag);
};

TSpeedPos::TSpeedPos(basic_event *event, double dist, double length, TOrders order)
{
    Set(event, dist, length, order);
};

void TSpeedPos::Clear()
{
    iFlags = 0; // brak flag to brak reakcji
    fVelNext = -1.0; // prędkość bez ograniczeń
	fSectionVelocityDist = 0.0; //brak długości
    fDist = 0.0;
    vPos = Math3D::vector3(0, 0, 0);
    trTrack = NULL; // brak wskaźnika
};

void TSpeedPos::CommandCheck()
{ // sprawdzenie typu komendy w evencie i określenie prędkości
    TCommandType command = evEvent->input_command();
    double value1 = evEvent->input_value(1);
    double value2 = evEvent->input_value(2);
    switch (command)
    {
    case TCommandType::cm_ShuntVelocity:
        // prędkość manewrową zapisać, najwyżej AI zignoruje przy analizie tabelki
        fVelNext = value1; // powinno być value2, bo druga określa "za"?
        iFlags |= spShuntSemaphor;
        break;
    case TCommandType::cm_SetVelocity:
        // w semaforze typu "m" jest ShuntVelocity dla Ms2 i SetVelocity dla S1
        // SetVelocity * 0    -> można jechać, ale stanąć przed
        // SetVelocity 0 20   -> stanąć przed, potem można jechać 20 (SBL)
        // SetVelocity -1 100 -> można jechać, przy następnym ograniczenie (SBL)
        // SetVelocity 40 -1  -> PutValues: jechać 40 aż do minięcia (koniec ograniczenia(
        fVelNext = value1;
        iFlags &= ~(spShuntSemaphor | spPassengerStopPoint | spStopOnSBL);
        iFlags |= spSemaphor;// nie manewrowa, nie przystanek, nie zatrzymać na SBL, ale semafor
        if (value1 == 0.0) // jeśli pierwsza zerowa
            if (value2 != 0.0) // a druga nie
            { // S1 na SBL, można przejechać po zatrzymaniu (tu nie mamy prędkości ani odległości)
                fVelNext = value2; // normalnie będzie zezwolenie na jazdę, aby się usunął z tabelki
                iFlags |= spStopOnSBL; // flaga, że ma zatrzymać; na pewno nie zezwoli na manewry
            }
        break;
    case TCommandType::cm_SectionVelocity:
        // odcinek z ograniczeniem prędkości
        fVelNext = value1;
        fSectionVelocityDist = value2;
        iFlags |= spSectionVel;
        break;
    case TCommandType::cm_RoadVelocity:
        // prędkość drogowa (od tej pory będzie jako domyślna najwyższa)
        fVelNext = value1;
        iFlags |= spRoadVel;
        break;
    case TCommandType::cm_PassengerStopPoint:
        // nie ma dostępu do rozkładu
        // przystanek, najwyżej AI zignoruje przy analizie tabelki
        fVelNext = 0.0;
        iFlags |= spPassengerStopPoint; // niestety nie da się w tym miejscu współpracować z rozkładem
/*
        // NOTE: not used for now as it might be unnecessary
        // special case, potentially override any set speed limits if requested
        // NOTE: we test it here because for the time being it's only used for passenger stops
        if( TestFlag( iFlags, spDontApplySpeedLimit ) ) {
            fVelNext = -1;
        }
*/
        break;
    case TCommandType::cm_SetProximityVelocity:
        // musi zostać gdyż inaczej nie działają manewry
        fVelNext = -1;
        iFlags |= spProximityVelocity;
        // fSectionVelocityDist = value2;
        break;
    case TCommandType::cm_OutsideStation:
        // w trybie manewrowym: skanować od niej wstecz i stanąć po wyjechaniu za sygnalizator i
        // zmienić kierunek
        // w trybie pociągowym: można przyspieszyć do wskazanej prędkości (po zjechaniu z rozjazdów)
        fVelNext = -1;
        iFlags |= spOutsideStation; // W5
        break;
    default:
        // inna komenda w evencie skanowanym powoduje zatrzymanie i wysłanie tej komendy
        // nie manewrowa, nie przystanek, nie zatrzymać na SBL
        iFlags &= ~(spShuntSemaphor | spPassengerStopPoint | spStopOnSBL);
        // jak nieznana komenda w komórce sygnałowej, to zatrzymujemy
        fVelNext = 0.0;
    }
};

bool TSpeedPos::Update()
{
    if( fDist < 0.0 ) {
        // trzeba zazanaczyć, że minięty
        iFlags |= spElapsed;
    }
    if (iFlags & spTrack) {
        // road/track
        if (trTrack) {
            // może być NULL, jeśli koniec toru (???)
            fVelNext = trTrack->VelocityGet(); // aktualizacja prędkości (może być zmieniana eventem)

            if( trTrack->iCategoryFlag & 1 ) {
                // railways
                if( iFlags & spSwitch ) {
                    // jeśli odcinek zmienny
                    if( ( ( trTrack->GetSwitchState() & 1 ) != 0 ) !=
                        ( ( iFlags & spSwitchStatus ) != 0 ) ) {
                        // czy stan się zmienił?
                        // Ra: zakładam, że są tylko 2 możliwe stany
                        iFlags ^= spSwitchStatus;
                        if( ( iFlags & spElapsed ) == 0 ) {
                            // jeszcze trzeba skanowanie wykonać od tego toru
                            // problem jest chyba, jeśli zwrotnica się przełoży zaraz po zjechaniu z niej
                            // na Mydelniczce potrafi skanować na wprost mimo pojechania na bok
                            return true;
                        }
                    }
                }
            }
            else {
                // roads and others
                if( iFlags & 0xF0000000 ) {
                    // jeśli skrzyżowanie, ograniczyć prędkość przy skręcaniu
                    if( ( iFlags & 0xF0000000 ) > 0x10000000 ) {
                        // ±1 to jazda na wprost, ±2 nieby też, ale z przecięciem głównej drogi - chyba że jest równorzędne...
                        // TODO: uzależnić prędkość od promienia;
                        // albo niech będzie ograniczona w skrzyżowaniu (velocity z ujemną wartością)
                        fVelNext = 30.0;
                    }
                }
            }
        }
    }
    else if (iFlags & spEvent) {
        // jeśli event
        if( ( ( iFlags & spElapsed ) == 0 )
         || ( fVelNext == 0.0 ) ) {
            // ignore already passed signals, but keep an eye on overrun stops
            // odczyt komórki pamięci najlepiej by było zrobić jako notyfikację,
            // czyli zmiana komórki wywoła jakąś podaną funkcję
            CommandCheck(); // sprawdzenie typu komendy w evencie i określenie prędkości
        }
    }
    return false;
};

std::string TSpeedPos::GetName() const
{
	if (iFlags & spTrack) // jeśli tor
        return trTrack->name();
    else if( iFlags & spEvent ) // jeśli event
        return evEvent->m_name;
    else
        return "";
}

std::string TSpeedPos::TableText() const
{ // pozycja tabelki pr?dko?ci
    if (iFlags & spEnabled)
    { // o ile pozycja istotna
		return to_hex_str(iFlags, 8) + "   " + to_string(fDist, 1, 6) +
               "   " + (fVelNext == -1.0 ? "  -" : to_string(static_cast<int>(fVelNext), 0, 3)) + "   " + GetName();
    }
    return "Empty";
}

bool TSpeedPos::IsProperSemaphor(TOrders order)
{ // sprawdzenie czy semafor jest zgodny z trybem jazdy
	if (order < 0x40) // Wait_for_orders, Prepare_engine, Change_direction, Connect, Disconnect, Shunt
    {
        if (iFlags & (spSemaphor | spShuntSemaphor))
            return true;
        else if (iFlags & spOutsideStation)
            return true;
    }
    else if (order & Obey_train)
    {
        if (iFlags & spSemaphor)
            return true;
    }
	return false; // true gdy zatrzymanie, wtedy nie ma po co skanować dalej
}

bool TSpeedPos::Set(basic_event *event, double dist, double length, TOrders order)
{ // zapamiętanie zdarzenia
    fDist = dist;
    iFlags = spEvent;
    evEvent = event;
    vPos = event->input_location(); // współrzędne eventu albo komórki pamięci (zrzutować na tor?)
    if( dist + length >= 0 ) {
        iFlags |= spEnabled;
        CommandCheck(); // sprawdzenie typu komendy w evencie i określenie prędkości
    }
    else {
        // located behind the tracking consist, don't bother with it
        return false;
    }
	// zależnie od trybu sprawdzenie czy jest tutaj gdzieś semafor lub tarcza manewrowa
	// jeśli wskazuje stop wtedy wystawiamy true jako koniec sprawdzania
	// WriteLog("EventSet: Vel=" + AnsiString(fVelNext) + " iFlags=" + AnsiString(iFlags) + " order="+AnsiString(order));
	if (order < 0x40) // Wait_for_orders, Prepare_engine, Change_direction, Connect, Disconnect, Shunt
    {
        if (iFlags & (spSemaphor | spShuntSemaphor) && fVelNext == 0.0)
            return true;
        else if (iFlags & spOutsideStation)
            return true;
    }
    else if (order & Obey_train)
    {
        if (iFlags & spSemaphor && fVelNext == 0.0)
            return true;
    }
    return false; // true gdy zatrzymanie, wtedy nie ma po co skanować dalej
};

void TSpeedPos::Set(TTrack *track, double dist, int flag)
{ // zapamiętanie zmiany prędkości w torze
    fDist = dist; // odległość do początku toru
    trTrack = track; // TODO: (t) może być NULL i nie odczytamy końca poprzedniego :/
    if (trTrack)
    {
        iFlags = flag | (trTrack->eType == tt_Normal ? spTrack : (spTrack | spSwitch) ); // zapamiętanie kierunku wraz z typem
        if (iFlags & spSwitch)
            if (trTrack->GetSwitchState() & 1)
                iFlags |= spSwitchStatus;
        fVelNext = trTrack->VelocityGet();
        if (trTrack->iDamageFlag & 128)
            fVelNext = 0.0; // jeśli uszkodzony, to też stój
        if (iFlags & spEnd)
            fVelNext = (trTrack->iCategoryFlag & 1) ?
                           0.0 :
                           20.0; // jeśli koniec, to pociąg stój, a samochód zwolnij
        vPos =
            ( iFlags & spReverse ) ?
                trTrack->CurrentSegment()->FastGetPoint_1() :
                trTrack->CurrentSegment()->FastGetPoint_0();
    }
};

//---------------------------------------------------------------------------

void TController::TableClear()
{ // wyczyszczenie tablicy
    sSpeedTable.clear();
    iLast = -1;
    iTableDirection = 0; // nieznany
    tLast = nullptr;
    fLastVel = -1.0;
    SemNextIndex = -1;
    SemNextStopIndex = -1;
    eSignSkip = nullptr; // nic nie pomijamy
};

std::vector<basic_event *> TController::CheckTrackEvent( TTrack *Track, double const fDirection ) const
{ // sprawdzanie eventów na podanym torze do podstawowego skanowania
    std::vector<basic_event *> events;
    auto const &eventsequence { ( fDirection > 0 ? Track->m_events2 : Track->m_events1 ) };
    for( auto const &event : eventsequence ) {
        if( ( event.second != nullptr )
         && ( event.second->m_passive ) ) {
            events.emplace_back( event.second );
        }
    }
    return events;
}

bool TController::TableAddNew()
{ // zwiększenie użytej tabelki o jeden rekord
    sSpeedTable.emplace_back(); // add a new slot
    iLast = sSpeedTable.size() - 1;
    return true; // false gdy się nałoży
};

bool TController::TableNotFound(basic_event const *Event) const
{ // sprawdzenie, czy nie został już dodany do tabelki (np. podwójne W4 robi problemy)
    auto lookup =
        std::find_if(
            sSpeedTable.begin(),
            sSpeedTable.end(),
            [Event]( TSpeedPos const &speedpoint ){
                return ( ( true == TestFlag( speedpoint.iFlags, spEnabled | spEvent ) )
                      && ( speedpoint.evEvent == Event ) ); } );

    if( ( Global.iWriteLogEnabled & 8 )
     && ( lookup != sSpeedTable.end() ) ) {
        WriteLog( "Speed table for " + OwnerName() + " already contains event " + lookup->evEvent->m_name );
    }

    return lookup == sSpeedTable.end();
};

void TController::TableTraceRoute(double fDistance, TDynamicObject *pVehicle)
{ // skanowanie trajektorii na odległość (fDistance) od (pVehicle) w kierunku przodu składu i
    // uzupełnianie tabelki
    // WriteLog("Starting TableTraceRoute");

    TTrack *pTrack{ nullptr }; // zaczynamy od ostatniego analizowanego toru
    double fTrackLength{ 0.0 }; // długość aktualnego toru (krótsza dla pierwszego)
    double fCurrentDistance{ 0.0 }; // aktualna przeskanowana długość
    double fLastDir{ 0.0 };

    if (iTableDirection != iDirection ) {
        // jeśli zmiana kierunku, zaczynamy od toru ze wskazanym pojazdem
        iTableDirection = iDirection; // ustalenie w jakim kierunku jest wypełniana tabelka względem pojazdu
        pTrack = pVehicle->RaTrackGet(); // odcinek, na którym stoi
        fTrackLength = pVehicle->RaTranslationGet(); // pozycja na tym torze (odległość od Point1)
        fLastDir = pVehicle->DirectionGet() * pVehicle->RaDirectionGet(); // ustalenie kierunku skanowania na torze
        if( fLastDir < 0.0 ) {
            // jeśli w kierunku Point2 toru
            fTrackLength = pTrack->Length() - fTrackLength; // przeskanowana zostanie odległość do Point2
        }
        // account for the fact tracing begins from active axle, not the actual front of the vehicle
        // NOTE: position of the couplers is modified by track offset, but the axles ain't, so we need to account for this as well
        fTrackLength -= (
            pVehicle->AxlePositionGet()
            - pVehicle->RearPosition()
            + pVehicle->VectorLeft() * pVehicle->MoverParameters->OffsetTrackH )
            .Length();
        // aktualna odległość ma być ujemna gdyż jesteśmy na końcu składu
        fCurrentDistance = -fLength - fTrackLength;
        // aktualna prędkość // changed to -1 to recognize speed limit, if any
        fLastVel = -1.0;
        sSpeedTable.clear();
        iLast = -1;
        tLast = nullptr; //żaden nie sprawdzony
        SemNextIndex = -1;
        SemNextStopIndex = -1;
        if( VelSignalLast == 0.0 ) {
            // don't allow potential red light overrun keep us from reversing
            VelSignalLast = -1.0;
        }
        fTrackLength = pTrack->Length(); //skasowanie zmian w zmiennej żeby poprawnie liczyło w dalszych krokach
        MoveDistanceReset(); // AI startuje 1s po zaczęciu jazdy i mógł już coś przejechać
    }
    else {
        if( iTableDirection == 0 ) { return; }
        // NOTE: provisory fix for BUG: sempahor indices no longer matching table size
        // TODO: find and really fix the reason it happens
        if( ( SemNextIndex != -1 )
         && ( SemNextIndex >= sSpeedTable.size() ) ) {
            SemNextIndex = -1;
        }
        if( ( SemNextStopIndex != -1 )
         && ( SemNextStopIndex >= sSpeedTable.size() ) ) {
            SemNextStopIndex = -1;
        }
        // kontynuacja skanowania od ostatnio sprawdzonego toru (w ostatniej pozycji zawsze jest tor)
        if( ( SemNextStopIndex != -1 )
         && ( sSpeedTable[SemNextStopIndex].fVelNext == 0.0 ) ) {
            // znaleziono semafor lub tarczę lub tor z prędkością zero, trzeba sprawdzić czy to nadał semafor
            // jeśli jest następny semafor to sprawdzamy czy to on nadał zero
            if( ( OrderCurrentGet() & Obey_train )
             && ( sSpeedTable[SemNextStopIndex].iFlags & spSemaphor ) ) {
                return;
            }
            else {
                if( ( OrderCurrentGet() < 0x40 )
                 && ( sSpeedTable[SemNextStopIndex].iFlags & ( spSemaphor | spShuntSemaphor | spOutsideStation ) ) ) {
                    return;
                }
            }
        }

        auto const &lastspeedpoint = sSpeedTable[ iLast ];
        pTrack = lastspeedpoint.trTrack;
        assert( pTrack != nullptr );
        // flaga ustawiona, gdy Point2 toru jest blizej
        fLastDir = ( ( ( lastspeedpoint.iFlags & spReverse ) != 0 )  ? -1.0 : 1.0 );
        fCurrentDistance = lastspeedpoint.fDist; // aktualna odleglosc do jego Point1
        fTrackLength = pTrack->Length();
    }

    if( iTableDirection == 0 ) {
        // don't bother
        return;
    }

    while (fCurrentDistance < fDistance)
    {
        if (pTrack != tLast) // ostatni zapisany w tabelce nie był jeszcze sprawdzony
        { // jeśli tor nie był jeszcze sprawdzany
            if( Global.iWriteLogEnabled & 8 ) {
                WriteLog( "Speed table for " + OwnerName() + " tracing through track " + pTrack->name() );
            }

            auto const events { CheckTrackEvent( pTrack, fLastDir ) };
            for( auto *pEvent : events ) {
                if( pEvent != nullptr ) // jeśli jest semafor na tym torze
                { // trzeba sprawdzić tabelkę, bo dodawanie drugi raz tego samego przystanku nie jest korzystne
                    if (TableNotFound(pEvent)) // jeśli nie ma
                    {
                        TableAddNew(); // zawsze jest true

                        if (Global.iWriteLogEnabled & 8) {
                            WriteLog("Speed table for " + OwnerName() + " found new event, " + pEvent->m_name);
                        }
                        auto &newspeedpoint = sSpeedTable[iLast];
/*
                        if( newspeedpoint.Set(
                            pEvent,
                            fCurrentDistance + ProjectEventOnTrack( pEvent, pTrack, fLastDir ),
                            OrderCurrentGet() ) ) {
*/
                        if( newspeedpoint.Set(
                            pEvent,
                            GetDistanceToEvent( pTrack, pEvent, fLastDir, fCurrentDistance ),
                            fLength,
                            OrderCurrentGet() ) ) {

                            fDistance = newspeedpoint.fDist; // jeśli sygnał stop, to nie ma potrzeby dalej skanować
                            SemNextStopIndex = iLast;
                            if (SemNextIndex == -1) {
                                SemNextIndex = iLast;
                            }
                            if (Global.iWriteLogEnabled & 8) {
                                WriteLog("(stop signal from "
                                    + (SemNextStopIndex != -1 ? sSpeedTable[SemNextStopIndex].GetName() : "unknown semaphor")
                                    + ")");
                            }
                        }
                        else {
                            if( ( true == newspeedpoint.IsProperSemaphor( OrderCurrentGet() ) )
                             && ( SemNextIndex == -1 ) ) {
                                SemNextIndex = iLast; // sprawdzamy czy pierwszy na drodze
                            }
                            if (Global.iWriteLogEnabled & 8) {
                                WriteLog("(forward signal for "
                                    + (SemNextIndex != -1 ? sSpeedTable[SemNextIndex].GetName() : "unknown semaphor")
                                    + ")");
                            }
                        }
                    }
                }
            } // event dodajemy najpierw, żeby móc sprawdzić, czy tor został dodany po odczytaniu prędkości następnego

            if( ( pTrack->VelocityGet() == 0.0 ) // zatrzymanie
             || ( pTrack->iAction ) // jeśli tor ma własności istotne dla skanowania
             || ( pTrack->VelocityGet() != fLastVel ) ) // następuje zmiana prędkości
            { // odcinek dodajemy do tabelki, gdy jest istotny dla ruchu
                TableAddNew();
                sSpeedTable[ iLast ].Set(
                    pTrack, fCurrentDistance,
                    ( fLastDir < 0 ?
                        spEnabled | spReverse :
                        spEnabled ) ); // dodanie odcinka do tabelki z flagą kierunku wejścia
                if (pTrack->eType == tt_Cross) {
                    // na skrzyżowaniach trzeba wybrać segment, po którym pojedzie pojazd
                    // dopiero tutaj jest ustalany kierunek segmentu na skrzyżowaniu
                    int routewanted;
                    if( false == AIControllFlag ) {
                        routewanted = (
                            input::keys[ GLFW_KEY_LEFT ] != GLFW_RELEASE ? 1 :
                            input::keys[ GLFW_KEY_RIGHT ] != GLFW_RELEASE ? 2 :
                            3 );
                    }
                    else {
                        routewanted = 1 + std::floor( Random( static_cast<double>( pTrack->RouteCount() ) - 0.001 ) );
                    }

                    sSpeedTable[iLast].iFlags |=
                        ( ( pTrack->CrossSegment(
                                (fLastDir < 0 ?
                                    tLast->iPrevDirection :
                                    tLast->iNextDirection),
/*
                                iRouteWanted )
*/
                                routewanted )
                            & 0xf ) << 28 ); // ostatnie 4 bity pola flag
                    sSpeedTable[iLast].iFlags &= ~spReverse; // usunięcie flagi kierunku, bo może być błędna
                    if (sSpeedTable[iLast].iFlags < 0) {
                        sSpeedTable[iLast].iFlags |= spReverse; // ustawienie flagi kierunku na podstawie wybranego segmentu
                    }
                    if (int(fLastDir) * sSpeedTable[iLast].iFlags < 0) {
                        fLastDir = -fLastDir;
                    }
/*
                    if (AIControllFlag) {
                        // dla AI na razie losujemy kierunek na kolejnym skrzyżowaniu
                        iRouteWanted = 1 + Random(3);
                    }
*/
                }
            }
            else if ( ( pTrack->fRadius != 0.0 ) // odległość na łuku lepiej aproksymować cięciwami
                   || ( ( tLast != nullptr )
                     && ( tLast->fRadius != 0.0 ) )) // koniec łuku też jest istotny
            { // albo dla liczenia odległości przy pomocy cięciw - te usuwać po przejechaniu
                if (TableAddNew()) {
                    // dodanie odcinka do tabelki
                    sSpeedTable[iLast].Set(
                        pTrack, fCurrentDistance,
                        ( fLastDir < 0 ?
                            spEnabled | spCurve | spReverse :
                            spEnabled | spCurve ) );
                }
            }
        }

        fCurrentDistance += fTrackLength; // doliczenie kolejnego odcinka do przeskanowanej długości
        tLast = pTrack; // odhaczenie, że sprawdzony
        fLastVel = pTrack->VelocityGet(); // prędkość na poprzednio sprawdzonym odcinku
        pTrack = pTrack->Connected(
            ( pTrack->eType == tt_Cross ?
                (sSpeedTable[iLast].iFlags >> 28) :
                static_cast<int>(fLastDir) ),
            fLastDir); // może być NULL

        if (pTrack != nullptr )
        { // jeśli kolejny istnieje
            if( tLast != nullptr ) {

                if( ( tLast->VelocityGet() > 0 )
                 && ( ( tLast->VelocityGet() < pTrack->VelocityGet() )
                   || ( pTrack->VelocityGet() < 0 ) ) ) {

                    if( ( iLast != -1 )
                     && ( sSpeedTable[ iLast ].trTrack == tLast ) ) {
                        // if the track is already in the table we only need to mark it as relevant
                        sSpeedTable[ iLast ].iFlags |= spEnabled;
                    }
                    else {
                        // otherwise add it
                        TableAddNew();
                        sSpeedTable[ iLast ].Set(
                            tLast,
                            fCurrentDistance - fTrackLength, // by now the current distance points to beginning of next track
                            ( fLastDir > 0 ?
                                pTrack->iPrevDirection :
                                pTrack->iNextDirection ) ?
                                    spEnabled :
                                    spEnabled | spReverse );
                    }
                }
            }
            // zwiększenie skanowanej odległości tylko jeśli istnieje dalszy tor
            fTrackLength = pTrack->Length();
        }
        else
        { // definitywny koniec skanowania, chyba że dalej puszczamy samochód po gruncie...
            if( ( iLast == -1 )
             || ( ( false == TestFlag( sSpeedTable[iLast].iFlags, spEnabled | spEnd ) )
               && ( sSpeedTable[iLast].trTrack != tLast ) ) ) {
                // only if we haven't already marked end of the track and if the new track doesn't duplicate last one
                if( TableAddNew() ) {
                    // zapisanie ostatniego sprawdzonego toru
                    sSpeedTable[iLast].Set(
                        tLast, fCurrentDistance - fTrackLength, // by now the current distance points to beginning of next track,
                        ( fLastDir < 0 ?
                            spEnabled | spEnd | spReverse :
                            spEnabled | spEnd ));
                }
            }
            else if( sSpeedTable[ iLast ].trTrack == tLast ) {
                // otherwise just mark the last added track as the final one
                // TODO: investigate exactly how we can wind up not marking the last existing track as actual end
                sSpeedTable[ iLast ].iFlags |= spEnd;
            }
            // to ostatnia pozycja, bo NULL nic nie da, a może się podpiąć obrotnica, czy jakieś transportery
            return;
        }
    }
    if( TableAddNew() ) {
        // zapisanie ostatniego sprawdzonego toru
        sSpeedTable[ iLast ].Set(
            pTrack, fCurrentDistance,
            ( fLastDir < 0 ?
                spNone | spReverse :
                spNone ) );
    }
};

void TController::TableCheck(double fDistance)
{ // przeliczenie odległości w tabelce, ewentualnie doskanowanie (bez analizy prędkości itp.)
    if( iTableDirection != iDirection ) {
        // jak zmiana kierunku, to skanujemy od końca składu
        TableTraceRoute( fDistance, pVehicles[ 1 ] );
        TableSort();
    }
    else if (iTableDirection)
    { // trzeba sprawdzić, czy coś się zmieniło
        auto const distance = MoveDistanceGet();
        for (auto &sp : sSpeedTable) {
            // aktualizacja odległości dla wszystkich pozycji tabeli
            sp.UpdateDistance(distance);
        }
        MoveDistanceReset(); // kasowanie odległości po aktualizacji tabelki
        for( int i = 0; i < iLast; ++i )
        { // aktualizacja rekordów z wyjątkiem ostatniego
            if (sSpeedTable[i].iFlags & spEnabled) // jeśli pozycja istotna
            {
                if (sSpeedTable[i].Update())
                {
                    if( Global.iWriteLogEnabled & 8 ) {
                        WriteLog( "Speed table for " + OwnerName() + " detected switch change at " + sSpeedTable[ i ].trTrack->name() + " (generating fresh trace)" );
                    }
                    // usuwamy wszystko za tym torem
                    while (sSpeedTable.back().trTrack != sSpeedTable[i].trTrack)
                    { // usuwamy wszystko dopóki nie trafimy na tą zwrotnicę
                        sSpeedTable.pop_back();
                        --iLast;
                    }
                    tLast = sSpeedTable[ i ].trTrack;
                    TableTraceRoute( fDistance, pVehicles[ 1 ] );
                    TableSort();
                    // nie kontynuujemy pętli, trzeba doskanować ciąg dalszy
                    break;
                }
                if (sSpeedTable[i].iFlags & spTrack) // jeśli odcinek
                {
                    if( sSpeedTable[ i ].fDist + sSpeedTable[ i ].trTrack->Length() < -fLength ) // a skład wyjechał całą długością poza
                    { // degradacja pozycji
						sSpeedTable[i].iFlags &= ~spEnabled; // nie liczy się
                    }
                    else if( ( sSpeedTable[ i ].iFlags & 0xF0000028 ) == spElapsed ) {
                        // jest z tyłu (najechany) i nie jest zwrotnicą ani skrzyżowaniem
                        if( sSpeedTable[ i ].fVelNext < 0 ) // a nie ma ograniczenia prędkości
                        {
                            sSpeedTable[ i ].iFlags = 0; // to nie ma go po co trzymać (odtykacz usunie ze środka)
                        }
                    }
                }
                else if (sSpeedTable[i].iFlags & spEvent) // jeśli event
                {
                    if (sSpeedTable[i].fDist < (
                            typeid( *(sSpeedTable[i].evEvent) ) == typeid( putvalues_event ) ?
                                -fLength :
                                0)) // jeśli jest z tyłu
                        if ((mvOccupied->CategoryFlag & 1) ? false :
                                                             sSpeedTable[i].fDist < -fLength)
                        { // pociąg staje zawsze, a samochód tylko jeśli nie przejedzie całą długością (może być zaskoczony zmianą)
							// WriteLog("TableCheck: Event is behind. Delete from table: " + sSpeedTable[i].evEvent->asName);
                            sSpeedTable[i].iFlags &= ~spEnabled; // degradacja pozycji dla samochodu;
                            // semafory usuwane tylko przy sprawdzaniu, bo wysyłają komendy 
                        }
                }
            }
        }
        sSpeedTable[iLast].Update(); // aktualizacja ostatniego
        // WriteLog("TableCheck: Upate last track. Dist=" + AnsiString(sSpeedTable[iLast].fDist));
        if( sSpeedTable[ iLast ].fDist < fDistance ) {
            TableTraceRoute( fDistance, pVehicles[ 1 ] ); // doskanowanie dalszego odcinka
            TableSort();
        }
        // garbage collection
        TablePurger();
    }
};

auto const passengerstopmaxdistance { 400.0 }; // maximum allowed distance between passenger stop point and consist head

TCommandType TController::TableUpdate(double &fVelDes, double &fDist, double &fNext, double &fAcc)
{ // ustalenie parametrów, zwraca typ komendy, jeśli sygnał podaje prędkość do jazdy
    // fVelDes - prędkość zadana
    // fDist - dystans w jakim należy rozważyć ruch
    // fNext - prędkość na końcu tego dystansu
    // fAcc - zalecane przyspieszenie w chwili obecnej - kryterium wyboru dystansu
    double a; // przyspieszenie
    double v; // prędkość
    double d; // droga
	double d_to_next_sem = 10000.0; //ustaiwamy na pewno dalej niż widzi AI
    IsAtPassengerStop = false; 
    TCommandType go = TCommandType::cm_Unknown;
    eSignNext = NULL;
    // te flagi są ustawiane tutaj, w razie potrzeby
    iDrivigFlags &= ~(moveTrackEnd | moveSwitchFound | moveSemaphorFound | /*moveSpeedLimitFound*/ moveStopPointFound );

    for( std::size_t i = 0; i < sSpeedTable.size(); ++i )
    { // sprawdzenie rekordów od (iFirst) do (iLast), o ile są istotne
        if (sSpeedTable[i].iFlags & spEnabled) // badanie istotności
        { // o ile dana pozycja tabelki jest istotna
            if (sSpeedTable[i].iFlags & spPassengerStopPoint) {
                // jeśli przystanek, trzeba obsłużyć wg rozkładu
                iDrivigFlags |= moveStopPointFound;
                // stop points are irrelevant when not in one of the basic modes
                if( ( OrderCurrentGet() & ( Obey_train | Shunt ) ) == 0 ) { continue; }
                // first 19 chars of the command is expected to be "PassengerStopPoint:" so we skip them
                if ( ToLower(sSpeedTable[i].evEvent->input_text()).compare( 19, sizeof(asNextStop), ToLower(asNextStop)) != 0 )
                { // jeśli nazwa nie jest zgodna
                    if (sSpeedTable[i].fDist < 300.0 && sSpeedTable[i].fDist > 0) // tylko jeśli W4 jest blisko, przy dwóch może zaczać szaleć
                    {
                        // porównuje do następnej stacji, więc trzeba przewinąć do poprzedniej
                        // nastepnie ustawić następną na aktualną tak żeby prawidłowo ją obsłużył w następnym kroku
                        if( true == TrainParams->RewindTimeTable( sSpeedTable[ i ].evEvent->input_text() ) ) {
                            asNextStop = TrainParams->NextStop();
                            iStationStart = TrainParams->StationIndex;
                        }
                    }
                    else if( sSpeedTable[ i ].fDist < -fLength ) {
                        // jeśli został przejechany
                        sSpeedTable[ i ].iFlags = 0; // to można usunąć (nie mogą być usuwane w skanowaniu)
                    }
                    continue; // ignorowanie jakby nie było tej pozycji
                }
                else if (iDrivigFlags & moveStopPoint) // jeśli pomijanie W4, to nie sprawdza czasu odjazdu
                { // tylko gdy nazwa zatrzymania się zgadza
                    if (!TrainParams->IsStop())
                    { // jeśli nie ma tu postoju
                        sSpeedTable[i].fVelNext = -1; // maksymalna prędkość w tym miejscu
                        // przy 160km/h jedzie 44m/s, to da dokładność rzędu 5 sekund
                        if (sSpeedTable[i].fDist < passengerstopmaxdistance * 0.5 ) {
                            // zaliczamy posterunek w pewnej odległości przed (choć W4 nie zasłania już semafora)
#if LOGSTOPS
                            WriteLog(
                                pVehicle->asName + " as " + TrainParams->TrainName
                                + ": at " + std::to_string(simulation::Time.data().wHour) + ":" + std::to_string(simulation::Time.data().wMinute)
                                + " passed " + asNextStop); // informacja
#endif
                            // przy jakim dystansie (stanie licznika) ma przesunąć na następny postój
                            fLastStopExpDist = mvOccupied->DistCounter + 0.250 + 0.001 * fLength;
                            TrainParams->UpdateMTable( simulation::Time, asNextStop );
                            UpdateDelayFlag();
                            TrainParams->StationIndexInc(); // przejście do następnej
                            asNextStop = TrainParams->NextStop(); // pobranie kolejnego miejsca zatrzymania
                            sSpeedTable[i].iFlags = 0; // nie liczy się już
                            continue; // nie analizować prędkości
                        }
                    } // koniec obsługi przelotu na W4
                    else {
                        // zatrzymanie na W4
                        if ( false == sSpeedTable[i].bMoved ) {
                            // potentially shift the stop point in accordance with its defined parameters
                            /*
                            // https://rainsted.com/pl/Wersja/18.2.133#Okr.C4.99gi_dla_W4_i_W32
                            Pierwszy parametr ujemny - preferowane zatrzymanie czoła składu (np. przed przejściem).
                            Pierwszy parametr dodatni - preferowane zatrzymanie środka składu (np. przy wiacie, przejściu podziemnym).
                            Drugi parametr ujemny - wskazanie zatrzymania dla krótszych składów (W32).
                            Drugi paramer dodatni - długość peronu (W4).
                            */
							auto L = 0.0;
							auto Par1 = sSpeedTable[i].evEvent->input_value(1);
							auto Par2 = sSpeedTable[i].evEvent->input_value(2);
							if ((Par2 >= 0) || (fLength < -Par2)) { //użyj tego W4
								if (Par1 < 0) {
                                    L = -Par1;
                                }
								else {
                                    //środek
                                    L = Par1 - fMinProximityDist - fLength * 0.5;
								}
								L = std::max(0.0, std::min(L, std::abs(Par2) - fMinProximityDist - fLength));
								sSpeedTable[i].UpdateDistance(L);
								sSpeedTable[i].bMoved = true;
							}
							else {
								sSpeedTable[i].iFlags = 0;
							}
						}
                        IsAtPassengerStop = (
                            ( sSpeedTable[ i ].fDist <= passengerstopmaxdistance )
                            // Ra 2F1I: odległość plus długość pociągu musi być mniejsza od długości
                            // peronu, chyba że pociąg jest dłuższy, to wtedy minimalna.
                            // jeśli długość peronu ((sSpeedTable[i].evEvent->ValueGet(2)) nie podana,
                            // przyjąć odległość fMinProximityDist
                            && ( ( iDrivigFlags & moveStopCloser ) != 0 ?
                                sSpeedTable[ i ].fDist + fLength <=
                                    std::max(
                                        std::abs( sSpeedTable[ i ].evEvent->input_value( 2 ) ),
                                        2.0 * fMaxProximityDist + fLength ) : // fmaxproximitydist typically equals ~50 m
                                sSpeedTable[ i ].fDist < d_to_next_sem ) );

                        if( !eSignNext ) {
                            //jeśli nie widzi następnego sygnału ustawia dotychczasową
                            eSignNext = sSpeedTable[ i ].evEvent;
                        }
                        if( mvOccupied->Vel > 0.3 ) {
                            // jeśli jedzie (nie trzeba czekać, aż się drgania wytłumią - drzwi zamykane od 1.0) to będzie zatrzymanie
                            sSpeedTable[ i ].fVelNext = 0;
                        } else if( true == IsAtPassengerStop ) {
                            // jeśli się zatrzymał przy W4, albo stał w momencie zobaczenia W4
                            if( !AIControllFlag ) {
                                // w razie przełączenia na AI ma nie podciągać do W4, gdy użytkownik zatrzymał za daleko
                                iDrivigFlags &= ~moveStopCloser;
                            }
                            
                            if( ( iDrivigFlags & moveDoorOpened ) == 0 ) {
                                // drzwi otwierać jednorazowo
                                iDrivigFlags |= moveDoorOpened; // nie wykonywać drugi raz
                                Doors( true, static_cast<int>( std::floor( std::abs( sSpeedTable[ i ].evEvent->input_value( 2 ) ) ) ) % 10 );
                            }

                            if (TrainParams->UpdateMTable( simulation::Time, asNextStop) ) {
                                // to się wykona tylko raz po zatrzymaniu na W4
                                if( TrainParams->StationIndex < TrainParams->StationCount ) {
                                    // jeśli są dalsze stacje, bez trąbienia przed odjazdem
                                    // also ignore any horn cue that may be potentially set below 1 km/h and before the actual full stop
                                    iDrivigFlags &= ~( moveStartHorn | moveStartHornNow );
                                }
                                UpdateDelayFlag();

                                // perform loading/unloading
                                auto const platformside = static_cast<int>( std::floor( std::abs( sSpeedTable[ i ].evEvent->input_value( 2 ) ) ) ) % 10;
                                auto const exchangetime = simulation::Station.update_load( pVehicles[ 0 ], *TrainParams, platformside );
                                WaitingSet( exchangetime );

                                if (TrainParams->DirectionChange()) {
                                    // jeśli "@" w rozkładzie, to wykonanie dalszych komend
                                    // wykonanie kolejnej komendy, nie dotyczy ostatniej stacji
                                    if (iDrivigFlags & movePushPull) {
                                        // SN61 ma się też nie ruszać, chyba że ma wagony
                                        iDrivigFlags |= moveStopHere; // EZT ma stać przy peronie
                                        if (OrderNextGet() != Change_direction) {
                                            OrderPush(Change_direction); // zmiana kierunku
                                            OrderPush(
                                                TrainParams->StationIndex < TrainParams->StationCount ?
                                                    Obey_train :
                                                    Shunt); // to dalej wg rozkładu
                                        }
                                    }
                                    else {
                                        // a dla lokomotyw...
                                        // pozwolenie na przejechanie za W4 przed czasem i nie ma stać
                                        iDrivigFlags &= ~( moveStopPoint | moveStopHere );
                                    }
                                    // przejście do kolejnego rozkazu (zmiana kierunku, odczepianie)
                                    JumpToNextOrder();
                                    // ma nie podjeżdżać pod W4 po przeciwnej stronie
                                    iDrivigFlags &= ~moveStopCloser;
                                    // ten W4 nie liczy się już zupełnie (nie wyśle SetVelocity)
                                    sSpeedTable[i].iFlags = 0;
                                    // jechać
                                    sSpeedTable[i].fVelNext = -1;
                                    // nie analizować prędkości
                                    continue;
                                }
                            }
                            else {
                                // sitting at passenger stop
                                if( fStopTime < 0 ) {
                                // verify progress of load exchange
                                    auto exchangetime { 0.f };
                                    auto *vehicle { pVehicles[ 0 ] };
                                    while( vehicle != nullptr ) {
                                        exchangetime = std::max( exchangetime, vehicle->LoadExchangeTime() );
                                        vehicle = vehicle->Next();
                                    }
                                    if( exchangetime > 0 ) {
                                        WaitingSet( exchangetime );
                                    }
                                }
                            }

                            if (OrderCurrentGet() & Shunt) {
                                OrderNext(Obey_train); // uruchomić jazdę pociągową
                                CheckVehicles(); // zmienić światła
                            }

                            if (TrainParams->StationIndex < TrainParams->StationCount) {
                                // jeśli są dalsze stacje, czekamy do godziny odjazdu
                                if (TrainParams->IsTimeToGo(simulation::Time.data().wHour, simulation::Time.data().wMinute)) {
                                    // z dalszą akcją czekamy do godziny odjazdu
                                    IsAtPassengerStop = false;
                                    // przy jakim dystansie (stanie licznika) ma przesunąć na następny postój
                                    fLastStopExpDist = mvOccupied->DistCounter + 0.050 + 0.001 * fLength;
                                    TrainParams->StationIndexInc(); // przejście do następnej
                                    asNextStop = TrainParams->NextStop(); // pobranie kolejnego miejsca zatrzymania
#if LOGSTOPS
                                    WriteLog(
                                        pVehicle->asName + " as " + TrainParams->TrainName
                                        + ": at " + std::to_string(simulation::Time.data().wHour) + ":" + std::to_string(simulation::Time.data().wMinute)
                                        + " next " + asNextStop); // informacja
#endif
                                    // update brake settings and ai braking tables
                                    // NOTE: this calculation is expected to run after completing loading/unloading
                                    AutoRewident(); // nastawianie hamulca do jazdy pociągowej

                                    if( static_cast<int>( std::floor( std::abs( sSpeedTable[ i ].evEvent->input_value( 1 ) ) ) ) % 2 ) {
                                        // nie podjeżdżać do semafora, jeśli droga nie jest wolna
                                        iDrivigFlags |= moveStopHere;
                                    }
                                    else {
                                        //po czasie jedź dalej
                                        iDrivigFlags &= ~moveStopHere;
                                    }
                                    iDrivigFlags |= moveStopCloser; // do następnego W4 podjechać blisko (z dociąganiem)
                                    sSpeedTable[i].iFlags = 0; // nie liczy się już zupełnie (nie wyśle SetVelocity)
                                    sSpeedTable[i].fVelNext = -1; // można jechać za W4
                                    if( ( sSpeedTable[ i ].fDist <= 0.0 ) && ( eSignNext == sSpeedTable[ i ].evEvent ) ) {
                                        // sanity check, if we're held by this stop point, let us go
                                        VelSignalLast = -1;
                                    }
                                    if (go == TCommandType::cm_Unknown) // jeśli nie było komendy wcześniej
                                        go = TCommandType::cm_Ready; // gotów do odjazdu z W4 (semafor może zatrzymać)
                                    if( false == tsGuardSignal.empty() ) {
                                        // jeśli mamy głos kierownika, to odegrać
                                        iDrivigFlags |= moveGuardSignal;
                                    }
                                    continue; // nie analizować prędkości
                                } // koniec startu z zatrzymania
                            } // koniec obsługi początkowych stacji
                            else {
                                // jeśli dojechaliśmy do końca rozkładu
#if LOGSTOPS
                                WriteLog(
                                    pVehicle->asName + " as " + TrainParams->TrainName
                                    + ": at " + std::to_string(simulation::Time.data().wHour) + ":" + std::to_string(simulation::Time.data().wMinute)
                                    + " end of route."); // informacja
#endif
                                asNextStop = TrainParams->NextStop(); // informacja o końcu trasy
                                TrainParams->NewName("none"); // czyszczenie nieaktualnego rozkładu
                                // ma nie podjeżdżać pod W4 i ma je pomijać
                                iDrivigFlags &= ~( moveStopCloser );
                                if( false == TestFlag( iDrivigFlags, movePushPull ) ) {
                                    // if the consist can change direction through a simple cab change it doesn't need fiddling with recognition of passenger stops
                                    iDrivigFlags &= ~( moveStopPoint );
                                }
                                fLastStopExpDist = -1.0f; // nie ma rozkładu, nie ma usuwania stacji
                                sSpeedTable[i].iFlags = 0; // W4 nie liczy się już (nie wyśle SetVelocity)
                                sSpeedTable[i].fVelNext = -1; // można jechać za W4
                                if( ( sSpeedTable[ i ].fDist <= 0.0 ) && ( eSignNext == sSpeedTable[ i ].evEvent ) ) {
                                    // sanity check, if we're held by this stop point, let us go
                                    VelSignalLast = -1;
                                }
                                // wykonanie kolejnego rozkazu (Change_direction albo Shunt)
                                JumpToNextOrder();
                                // ma się nie ruszać aż do momentu podania sygnału
                                iDrivigFlags |= moveStopHere | moveStartHorn;
                                continue; // nie analizować prędkości
                            } // koniec obsługi ostatniej stacji
                        } // vel 0, at passenger stop
                        else {
                            // HACK: momentarily deactivate W4 to trick the controller into moving closer
                            sSpeedTable[ i ].fVelNext = -1;
                        } // vel 0, outside of passenger stop
                    } // koniec obsługi zatrzymania na W4
                } // koniec warunku pomijania W4 podczas zmiany czoła
                else
                { // skoro pomijanie, to jechać i ignorować W4
                    sSpeedTable[i].iFlags = 0; // W4 nie liczy się już (nie zatrzymuje jazdy)
                    sSpeedTable[i].fVelNext = -1;
                    continue; // nie analizować prędkości
                }
            } // koniec obsługi W4
            v = sSpeedTable[ i ].fVelNext; // odczyt prędkości do zmiennej pomocniczej
            if( sSpeedTable[ i ].iFlags & spSwitch ) {
                // zwrotnice są usuwane z tabelki dopiero po zjechaniu z nich
                iDrivigFlags |= moveSwitchFound; // rozjazd z przodu/pod ogranicza np. sens skanowania wstecz
            }
            else if (sSpeedTable[i].iFlags & spEvent) // W4 może się deaktywować
            { // jeżeli event, może być potrzeba wysłania komendy, aby ruszył
                if( sSpeedTable[ i ].fDist < 0.0 ) {
                    // sprawdzanie eventów pasywnych miniętych
/*
                    if( ( eSignNext != nullptr ) && ( sSpeedTable[ i ].evEvent == eSignNext ) ) {
                        VelSignalLast = sSpeedTable[ i ].fVelNext;
                    }
*/
                    if( SemNextIndex == i ) {
                        if( Global.iWriteLogEnabled & 8 ) {
                            WriteLog( "Speed table update for " + OwnerName() + ", passed semaphor " + sSpeedTable[ SemNextIndex ].GetName() );
                        }
                        SemNextIndex = -1; // jeśli minęliśmy semafor od ograniczenia to go kasujemy ze zmiennej sprawdzającej dla skanowania w przód
                    }
                    if( SemNextStopIndex == i ) {
                        if( Global.iWriteLogEnabled & 8 ) {
                            WriteLog( "Speed table update for " + OwnerName() + ", passed semaphor " + sSpeedTable[ SemNextStopIndex ].GetName() );
                        }
                        SemNextStopIndex = -1; // jeśli minęliśmy semafor od ograniczenia to go kasujemy ze zmiennej sprawdzającej dla skanowania w przód
                    }
                }
                if( sSpeedTable[ i ].fDist > 0.0 ) {
                    // check signals ahead
                    if( sSpeedTable[ i ].IsProperSemaphor( OrderCurrentGet() ) ) {
                        if( SemNextIndex == -1 ) {
                            // jeśli jest mienięty poprzedni semafor a wcześniej
                            // byl nowy to go dorzucamy do zmiennej, żeby cały czas widział najbliższy
                            SemNextIndex = i;
                            if( Global.iWriteLogEnabled & 8 ) {
                                WriteLog( "Speed table update for " + OwnerName() + ", next semaphor is " + sSpeedTable[ SemNextIndex ].GetName() );
                            }
                        }
                        if( ( SemNextStopIndex == -1 )
                         || ( ( sSpeedTable[ SemNextStopIndex ].fVelNext != 0 )
                           && ( sSpeedTable[ i ].fVelNext == 0 ) ) ) {
                            SemNextStopIndex = i;
                        }
                    }
                }
                if (sSpeedTable[i].iFlags & spOutsideStation)
                { // jeśli W5, to reakcja zależna od trybu jazdy
                    if (OrderCurrentGet() & Obey_train)
                    { // w trybie pociągowym: można przyspieszyć do wskazanej prędkości (po zjechaniu z rozjazdów)
                        v = -1.0; // ignorować?
						if (sSpeedTable[i].fDist < 0.0) // jeśli wskaźnik został minięty
                        {
                            VelSignalLast = v; //ustawienie prędkości na -1
                        }
                        else if (!(iDrivigFlags & moveSwitchFound)) // jeśli rozjazdy już minięte
                            VelSignalLast = v; //!!! to też koniec ograniczenia
                    }
                    else
                    { // w trybie manewrowym: skanować od niego wstecz, stanąć po wyjechaniu za
                        // sygnalizator i zmienić kierunek
                        v = 0.0; // zmiana kierunku może być podanym sygnałem, ale wypadało by
                        // zmienić światło wcześniej
                        if (!(iDrivigFlags & moveSwitchFound)) // jeśli nie ma rozjazdu
                            iDrivigFlags |= moveTrackEnd; // to dalsza jazda trwale ograniczona (W5,
                        // koniec toru)
                    }
                }
                else if (sSpeedTable[i].iFlags & spStopOnSBL) {
                    // jeśli S1 na SBL
                    if( mvOccupied->Vel < 2.0 ) {
                        // stanąć nie musi, ale zwolnić przynajmniej
                        if( ( sSpeedTable[ i ].fDist < fMaxProximityDist )
                         && ( TrackBlock() > 1000.0 ) ) {
                            // jest w maksymalnym zasięgu to można go pominąć (wziąć drugą prędkosć)
                            // as long as there isn't any obstacle in arbitrary view range
                            eSignSkip = sSpeedTable[ i ].evEvent;
                            // jazda na widoczność - skanować możliwość kolizji i nie podjeżdżać zbyt blisko
                            // usunąć flagę po podjechaniu blisko semafora zezwalającego na jazdę
                            // ostrożnie interpretować sygnały - semafor może zezwalać na jazdę pociągu z przodu!
                            iDrivigFlags |= moveVisibility;
                            // store the ordered restricted speed and don't exceed it until the flag is cleared
                            VelRestricted = sSpeedTable[ i ].evEvent->input_value( 2 );
                        }
                    }
                    if( eSignSkip != sSpeedTable[ i ].evEvent ) {
                        // jeśli ten SBL nie jest do pominięcia to ma 0 odczytywać
                        v = sSpeedTable[ i ].evEvent->input_value( 1 );
                        // TODO sprawdzić do której zmiennej jest przypisywane v i zmienić to tutaj
                    }
                }
                else if (sSpeedTable[i].IsProperSemaphor(OrderCurrentGet()))
                { // to semaphor
                    if (sSpeedTable[i].fDist < 0)
						VelSignalLast = sSpeedTable[i].fVelNext; //minięty daje prędkość obowiązującą
                    else
                    {
						iDrivigFlags |= moveSemaphorFound; //jeśli z przodu to dajemy falgę, że jest
                        d_to_next_sem = std::min(sSpeedTable[i].fDist, d_to_next_sem);
                    }
                    if( sSpeedTable[ i ].fDist <= d_to_next_sem )
                    {
                        VelSignalNext = sSpeedTable[ i ].fVelNext;
                    }
                }
                else if (sSpeedTable[i].iFlags & spRoadVel)
                { // to W6
                    if (sSpeedTable[i].fDist < 0)
                        VelRoad = sSpeedTable[i].fVelNext;
                }
                else if (sSpeedTable[i].iFlags & spSectionVel)
                { // to W27
                    if (sSpeedTable[i].fDist < 0) // teraz trzeba sprawdzić inne warunki
                    {
                        if (sSpeedTable[i].fSectionVelocityDist == 0.0)
                        {
                            if (Global.iWriteLogEnabled & 8)
                            WriteLog("TableUpdate: Event is behind. SVD = 0: " + sSpeedTable[i].evEvent->m_name);
                            sSpeedTable[i].iFlags = 0; // jeśli punktowy to kasujemy i nie dajemy ograniczenia na stałe
                        }
                        else if (sSpeedTable[i].fSectionVelocityDist < 0.0)
                        { // ograniczenie obowiązujące do następnego
                            if (sSpeedTable[i].fVelNext == min_speed(sSpeedTable[i].fVelNext, VelLimitLast) &&
                                sSpeedTable[i].fVelNext != VelLimitLast)
                            { // jeśli ograniczenie jest mniejsze niż obecne to obowiązuje od zaraz
                                VelLimitLast = sSpeedTable[i].fVelNext;
                            }
                            else if (sSpeedTable[i].fDist < -fLength)
                            { // jeśli większe to musi wyjechać za poprzednie
                                VelLimitLast = sSpeedTable[i].fVelNext;
                                if (Global.iWriteLogEnabled & 8)
                                WriteLog("TableUpdate: Event is behind. SVD < 0: " + sSpeedTable[i].evEvent->m_name);
                                sSpeedTable[i].iFlags = 0; // wyjechaliśmy poza poprzednie, można skasować
                            }
                        }
                        else
                        { // jeśli większe to ograniczenie ma swoją długość
                            if (sSpeedTable[i].fVelNext == min_speed(sSpeedTable[i].fVelNext, VelLimitLast) &&
                                sSpeedTable[i].fVelNext != VelLimitLast)
                            { // jeśli ograniczenie jest mniejsze niż obecne to obowiązuje od zaraz
                                VelLimitLast = sSpeedTable[i].fVelNext;
                            }
                            else if (sSpeedTable[i].fDist < -fLength && sSpeedTable[i].fVelNext != VelLimitLast)
                            { // jeśli większe to musi wyjechać za poprzednie
                                VelLimitLast = sSpeedTable[i].fVelNext;
                            }
                            else if (sSpeedTable[i].fDist < -fLength - sSpeedTable[i].fSectionVelocityDist)
                            { //
                                VelLimitLast = -1.0;
                                if (Global.iWriteLogEnabled & 8)
                                WriteLog("TableUpdate: Event is behind. SVD > 0: " + sSpeedTable[i].evEvent->m_name);
                                sSpeedTable[i].iFlags = 0; // wyjechaliśmy poza poprzednie, można skasować
                            }
                        }
                    }
                }

                //sprawdzenie eventów pasywnych przed nami
                if( ( mvOccupied->CategoryFlag & 1 )
                 && ( sSpeedTable[ i ].fDist > pVehicles[ 0 ]->fTrackBlock - 20.0 ) ) {
                    // jak sygnał jest dalej niż zawalidroga
                    v = 0.0; // to może być podany dla tamtego: jechać tak, jakby tam stop był
                }
                else
                { // zawalidrogi nie ma (albo pojazd jest samochodem), sprawdzić sygnał
                    if (sSpeedTable[i].iFlags & spShuntSemaphor) // jeśli Tm - w zasadzie to sprawdzić komendę!
                    { // jeśli podana prędkość manewrowa
                        if( ( v == 0.0 ) && ( true == TestFlag( OrderCurrentGet(), Obey_train ) ) ) {
                            // jeśli tryb pociągowy a tarcze ma ShuntVelocity 0 0
                            v = -1; // ignorować, chyba że prędkość stanie się niezerowa
                            if( true == TestFlag( sSpeedTable[ i ].iFlags, spElapsed ) ) {
                                // a jak przejechana to można usunąć, bo podstawowy automat usuwa tylko niezerowe
                                sSpeedTable[ i ].iFlags = 0;
                            }
                        }
                        else if( go == TCommandType::cm_Unknown ) {
                            // jeśli jeszcze nie ma komendy
                            // komenda jest tylko gdy ma jechać, bo stoi na podstawietabelki
                            if( v != 0.0 ) {
                                // jeśli nie było komendy wcześniej - pierwsza się liczy - ustawianie VelSignal
                                go = TCommandType::cm_ShuntVelocity; // w trybie pociągowym tylko jeśli włącza
                                // tryb manewrowy (v!=0.0)
                                // Ra 2014-06: (VelSignal) nie może być tu ustawiane, bo Tm może być
                                // daleko
                                // VelSignal=v; //nie do końca tak, to jest druga prędkość
                                if( VelSignal == 0.0 ) {
                                    // aby stojący ruszył
                                    VelSignal = v;
                                }
                                if( sSpeedTable[ i ].fDist < 0.0 ) {
                                    // jeśli przejechany
                                    //!!! ustawienie, gdy przejechany jest lepsze niż wcale, ale to jeszcze nie to
                                    VelSignal = v; 
                                    // to można usunąć (nie mogą być usuwane w skanowaniu)
                                    sSpeedTable[ i ].iFlags = 0;
                                }
                            }
                        }
                    }
                    else if( !( sSpeedTable[ i ].iFlags & spSectionVel ) ) {
                        //jeśli jakiś event pasywny ale nie ograniczenie
                        if( go == TCommandType::cm_Unknown ) {
                            // jeśli nie było komendy wcześniej - pierwsza się liczy - ustawianie VelSignal
                            if( ( v < 0.0 )
                             || ( v >= 1.0 ) ) {
                                // bo wartość 0.1 służy do hamowania tylko
                                go = TCommandType::cm_SetVelocity; // może odjechać
                                // Ra 2014-06: (VelSignal) nie może być tu ustawiane, bo semafor może być daleko
                                // VelSignal=v; //nie do końca tak, to jest druga prędkość; -1 nie wpisywać...
                                if( VelSignal == 0.0 ) {
                                    // aby stojący ruszył
                                    VelSignal = -1.0;
                                }
                                if( sSpeedTable[ i ].fDist < 0.0 ) {
                                    // jeśli przejechany
                                    VelSignal = ( v == 0.0 ? 0.0 : -1.0 );
                                    // ustawienie, gdy przejechany jest lepsze niż wcale, ale to jeszcze nie to
                                    if( sSpeedTable[ i ].iFlags & spEvent ) {
                                        // jeśli event
                                        if( ( sSpeedTable[ i ].evEvent != eSignSkip )
                                         || ( sSpeedTable[ i ].fVelNext != VelRestricted ) ) {
                                            // ale inny niż ten, na którym minięto S1, chyba że się już zmieniło
                                            // sygnał zezwalający na jazdę wyłącza jazdę na widoczność (po S1 na SBL)
                                            iDrivigFlags &= ~moveVisibility;
                                            // remove restricted speed
                                            VelRestricted = -1.0;
                                        }
                                    }
                                    // jeśli nie jest ograniczeniem prędkości to można usunąć
                                    // (nie mogą być usuwane w skanowaniu)
                                    sSpeedTable[ i ].iFlags = 0;
                                }
                            }
                            else if( sSpeedTable[ i ].evEvent->is_command() ) {
                                // jeśli prędkość jest zerowa, a komórka zawiera komendę
                                eSignNext = sSpeedTable[ i ].evEvent; // dla informacji
                                if( true == TestFlag( iDrivigFlags, moveStopHere ) ) {
                                    // jeśli ma stać, dostaje komendę od razu
                                    go = TCommandType::cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                                }
                                else if( sSpeedTable[ i ].fDist <= fMaxProximityDist ) {
                                    // jeśli ma dociągnąć, to niech dociąga
                                    // (moveStopCloser dotyczy dociągania do W4, nie semafora)
                                    go = TCommandType::cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                                }
                            }
                        }
                    }
                } // jeśli nie ma zawalidrogi
            } // jeśli event

            if (v >= 0.0) {
                // pozycje z prędkością -1 można spokojnie pomijać
                d = sSpeedTable[i].fDist;
                if( ( d > 0.0 )
                 && ( false == TestFlag( sSpeedTable[ i ].iFlags, spElapsed ) ) ) {
                    // sygnał lub ograniczenie z przodu (+32=przejechane)
                    // 2014-02: jeśli stoi, a ma do przejechania kawałek, to niech jedzie
                    if( ( mvOccupied->Vel < 0.01 )
                     && ( true == TestFlag( sSpeedTable[ i ].iFlags, ( spEnabled | spEvent | spPassengerStopPoint ) ) )
                     && ( false == IsAtPassengerStop ) ) {
                        // ma podjechać bliżej - czy na pewno w tym miejscu taki warunek?
                        a = ( ( d > passengerstopmaxdistance ) || ( ( iDrivigFlags & moveStopCloser ) != 0 ) ?
                                fAcc :
                                0.0 );
                    }
                    else {
                        // przyspieszenie: ujemne, gdy trzeba hamować
                        a = ( v * v - mvOccupied->Vel * mvOccupied->Vel ) / ( 25.92 * d );
                        if( ( mvOccupied->Vel < v )
                         || ( v == 0.0 ) ) {
                            // if we're going slower than the target velocity and there's enough room for safe stop, speed up
                            auto const brakingdistance = fBrakeDist * braking_distance_multiplier( v );
                            if( brakingdistance > 0.0 ) {
                                // maintain desired acc while we have enough room to brake safely, when close enough start paying attention
                                // try to make a smooth transition instead of sharp change
                                a = interpolate( a, AccPreferred, clamp( ( d - brakingdistance ) / brakingdistance, 0.0, 1.0 ) );
                            }
                        }
                        if( ( d < fMinProximityDist )
                         && ( v < fVelDes ) ) {
                            // jak jest już blisko, ograniczenie aktualnej prędkości
                            fVelDes = v;
                        }
                    }
                }
                else if (sSpeedTable[i].iFlags & spTrack) // jeśli tor
                { // tor ogranicza prędkość, dopóki cały skład nie przejedzie,
                    // d=fLength+d; //zamiana na długość liczoną do przodu
                    if( v >= 1.0 ) // EU06 się zawieszało po dojechaniu na koniec toru postojowego
                        if( d + sSpeedTable[ i ].trTrack->Length() < -fLength )
                            continue; // zapętlenie, jeśli już wyjechał za ten odcinek
                    if( v < fVelDes ) {
                        // ograniczenie aktualnej prędkości aż do wyjechania za ograniczenie
                        fVelDes = v;
                    }
                    if( ( sSpeedTable[ i ].iFlags & spEnd )
                     && ( mvOccupied->CategoryFlag & 1 ) ) {
                        // if the railway track ends here set the velnext accordingly as well
                        // TODO: test this with turntables and such
                        fNext = 0.0;
                    }
                    // if (v==0.0) fAcc=-0.9; //hamowanie jeśli stop
                    continue; // i tyle wystarczy
                }
                else // event trzyma tylko jeśli VelNext=0, nawet po przejechaniu (nie powinno
                    // dotyczyć samochodów?)
                    a = (v == 0.0 ? -1.0 : fAcc); // ruszanie albo hamowanie

                if ((a < fAcc) && (v == std::min(v, fNext))) {
                    // mniejsze przyspieszenie to mniejsza możliwość rozpędzenia się albo konieczność hamowania
                    // jeśli droga wolna, to może być a>1.0 i się tu nie załapuje
                    fAcc = a; // zalecane przyspieszenie (nie musi być uwzględniane przez AI)
                    fNext = v; // istotna jest prędkość na końcu tego odcinka
                    fDist = d; // dlugość odcinka
                }
                else if ((fAcc > 0) && (v >= 0) && (v <= fNext)) {
                    // jeśli nie ma wskazań do hamowania, można podać drogę i prędkość na jej końcu
                    fNext = v; // istotna jest prędkość na końcu tego odcinka
                    fDist = d; // dlugość odcinka (kolejne pozycje mogą wydłużać drogę, jeśli prędkość jest stała)
                }
            } // if (v>=0.0)
            if (fNext >= 0.0)
            { // jeśli ograniczenie
                if ((sSpeedTable[i].iFlags & (spEnabled | spEvent)) == (spEnabled | spEvent)) // tylko sygnał przypisujemy
                    if (!eSignNext) // jeśli jeszcze nic nie zapisane tam
                        eSignNext = sSpeedTable[i].evEvent; // dla informacji
                if (fNext == 0.0)
                    break; // nie ma sensu analizować tabelki dalej
            }
        } // if (sSpeedTable[i].iFlags&1)
    } // for

    // jeśli mieliśmy ograniczenie z semafora i nie ma przed nami
    if( ( VelSignalLast >= 0.0 )
     && ( ( iDrivigFlags & ( moveSemaphorFound | moveSwitchFound | moveStopPointFound ) ) == 0 )
     && ( true == TestFlag( OrderCurrentGet(), Obey_train ) ) ) {
        VelSignalLast = -1.0;
    }

    //analiza spisanych z tabelki ograniczeń i nadpisanie aktualnego
    if( ( true == IsAtPassengerStop ) && ( mvOccupied->Vel < 0.01 ) ) {
        // if stopped at a valid passenger stop, hold there
        fVelDes = 0.0;
    }
    else {
        fVelDes = min_speed( fVelDes, VelSignalLast );
        fVelDes = min_speed( fVelDes, VelLimitLast );
        fVelDes = min_speed( fVelDes, VelRoad );
        fVelDes = min_speed( fVelDes, VelRestricted );
    }
    // nastepnego semafora albo zwrotnicy to uznajemy, że mijamy W5
    FirstSemaphorDist = d_to_next_sem; // przepisanie znalezionej wartosci do zmiennej
    return go;
};

// modifies brake distance for low target speeds, to ease braking rate in such situations
float
TController::braking_distance_multiplier( float const Targetvelocity ) const {

    if( Targetvelocity > 65.f ) { return 1.f; }
    if( Targetvelocity < 5.f ) {
        // HACK: engaged automatic transmission means extra/earlier braking effort is needed for the last leg before full stop
        if( ( mvOccupied->TrainType == dt_DMU )
         && ( mvOccupied->Vel < 40.0 )
         && ( Targetvelocity == 0.f ) ) {
            return interpolate( 2.f, 1.f, static_cast<float>( mvOccupied->Vel / 40.0 ) );
        }
        // HACK: cargo trains or trains going downhill with high braking threshold need more distance to come to a full stop
        if( ( fBrake_a0[ 1 ] > 0.2 )
         && ( ( true == IsCargoTrain )
           || ( fAccGravity > 0.025 ) ) ) {
            return interpolate(
                1.f, 2.f,
                clamp(
                    ( fBrake_a0[ 1 ] - 0.2 ) / 0.2,
                    0.0, 1.0 ) );
        }

        return 1.f;
    }
    // stretch the braking distance up to 3 times; the lower the speed, the greater the stretch
    return interpolate( 3.f, 1.f, ( Targetvelocity - 5.f ) / 60.f );
}

void TController::TablePurger()
{ // odtykacz: usuwa mniej istotne pozycje ze środka tabelki, aby uniknąć zatkania
    //(np. brak ograniczenia pomiędzy zwrotnicami, usunięte sygnały, minięte odcinki łuku)
    if( sSpeedTable.size() < 2 ) {
        return;
    }
    // simplest approach should be good enough for start -- just copy whatever is still relevant, then swap
    // do a trial run first, to see if we need to bother at all
    std::size_t trimcount{ 0 };
    for( std::size_t idx = 0; idx < sSpeedTable.size() - 1; ++idx ) {
        auto const &speedpoint = sSpeedTable[ idx ];
        if( ( 0 == ( speedpoint.iFlags & spEnabled ) )
         || ( ( ( speedpoint.iFlags & ( spElapsed | spTrack | spCurve | spSwitch ) ) == ( spElapsed | spTrack | spCurve ) )
           && ( speedpoint.fVelNext < 0.0 ) ) ) {
            // NOTE: we could break out early here, but running through entire thing gives us exact size needed for new table
            ++trimcount;
        }
    }
    if( trimcount == 0 ) {
        // there'd be no gain, may as well bail
        return;
    }
    std::vector<TSpeedPos> trimmedtable; trimmedtable.reserve( sSpeedTable.size() - trimcount );
    // we can only update pointers safely after new table is finalized, so record their indices until then
    for( std::size_t idx = 0; idx < sSpeedTable.size() - 1; ++idx ) {
        // cache placement of semaphors in the new table, if we encounter them
        if( idx == SemNextIndex ) {
            SemNextIndex = trimmedtable.size();
        }
        if( idx == SemNextStopIndex ) {
            SemNextStopIndex = trimmedtable.size();
        }
        auto const &speedpoint = sSpeedTable[ idx ];
        if( ( 0 == ( speedpoint.iFlags & spEnabled ) )
         || ( ( ( speedpoint.iFlags & ( spElapsed | spTrack | spCurve | spSwitch ) ) == ( spElapsed | spTrack | spCurve ) )
           && ( speedpoint.fVelNext < 0.0 ) ) ) {
            // if the trimmed point happens to be currently active semaphor we need to invalidate their placements
            if( idx == SemNextIndex ) {
                SemNextIndex = -1;
            }
            if( idx == SemNextStopIndex ) {
                SemNextStopIndex = -1;
            }
            continue;
        }
        // we're left with useful speed point record we should copy
        trimmedtable.emplace_back( speedpoint );
    }
    // always copy the last entry
    trimmedtable.emplace_back( sSpeedTable.back() );

    if( Global.iWriteLogEnabled & 8 ) {
        WriteLog( "Speed table garbage collection for " + OwnerName() + " cut away " + std::to_string( trimcount ) + ( trimcount == 1 ? " record" : " records" ) );
    }
    // update the data
    std::swap( sSpeedTable, trimmedtable );
    iLast = sSpeedTable.size() - 1;
};

void TController::TableSort() {

    if( sSpeedTable.size() < 3 ) {
        // we skip last slot and no point in checking if there's only one other entry
        return;
    }
    TSpeedPos sp_temp = TSpeedPos(); // uzywany do przenoszenia
    for( int i = 0; i < ( iLast - 1 ); ++i ) {
        // pętla tylko do dwóch pozycji od końca bo ostatniej nie modyfikujemy
        if (sSpeedTable[i].fDist > sSpeedTable[i + 1].fDist)
        { // jesli pozycja wcześniejsza jest dalej to źle
            sp_temp = sSpeedTable[i + 1];
            sSpeedTable[i + 1] = sSpeedTable[i]; // zamiana
            sSpeedTable[i] = sp_temp;
            // jeszcze sprawdzenie czy pozycja nie była indeksowana dla eventów
            if (SemNextIndex == i)
                ++SemNextIndex;
            else if (SemNextIndex == i + 1)
                --SemNextIndex;
            if (SemNextStopIndex == i)
                ++SemNextStopIndex;
            else if (SemNextStopIndex == i + 1)
                --SemNextStopIndex;
        }
    }
}

//---------------------------------------------------------------------------

TController::TController(bool AI, TDynamicObject *NewControll, bool InitPsyche, bool primary) :// czy ma aktywnie prowadzić?
              AIControllFlag( AI ),     pVehicle( NewControll )
{
    ControllingSet(); // utworzenie połączenia do sterowanego pojazdu
    if( pVehicle != nullptr ) {
        pVehicles[ 0 ] = pVehicle->GetFirstDynamic( 0 ); // pierwszy w kierunku jazdy (Np. Pc1)
        pVehicles[ 1 ] = pVehicle->GetFirstDynamic( 1 ); // ostatni w kierunku jazdy (końcówki)
    }
    else {
        pVehicles[ 0 ] = nullptr;
        pVehicles[ 1 ] = nullptr;
    }
    if( mvOccupied != nullptr ) {
        iDirectionOrder = mvOccupied->CabNo; // 1=do przodu (w kierunku sprzęgu 0)
        VehicleName = mvOccupied->Name;

        if( mvOccupied->CategoryFlag & 2 ) { // samochody: na podst. http://www.prawko-kwartnik.info/hamowanie.html
            // fDriverBraking=0.0065; //mnożone przez (v^2+40*v) [km/h] daje prawie drogę hamowania [m]
            fDriverBraking = 0.03; // coś nie hamują te samochody zbyt dobrze
            fDriverDist = 5.0; // 5m - zachowywany odstęp przed kolizją
            fVelPlus = 10.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 2.0; // margines prędkości powodujący załączenie napędu
        }
        else { // pociągi i statki
            fDriverBraking = 0.06; // mnożone przez (v^2+40*v) [km/h] daje prawie drogę hamowania [m]
            fDriverDist = 50.0; // 50m - zachowywany odstęp przed kolizją
            fVelPlus = 5.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 5.0; // margines prędkości powodujący załączenie napędu
        }

        // fAccThreshold może podlegać uczeniu się - hamowanie powinno być rejestrowane, a potem analizowane
        // próg opóźnienia dla zadziałania hamulca
        fAccThreshold = (
            mvOccupied->TrainType == dt_EZT ? -0.55 :
            mvOccupied->TrainType == dt_DMU ? -0.45 :
            -0.2 );
        // HACK: emu with induction motors need to start their braking a bit sooner than the ones with series motors
        if( ( mvOccupied->TrainType == dt_EZT )
         && ( mvControlling->EngineType == TEngineType::ElectricInductionMotor ) ) {
            fAccThreshold += 0.10;
        }
    }
    // TrainParams=NewTrainParams;
    // if (TrainParams)
    // asNextStop=TrainParams->NextStop();
    // else
    TrainParams = new TTrainParameters("none"); // rozkład jazdy
    // OrderCommand="";
    // OrderValue=0;
    OrdersClear();

    if( true == primary ) {
        iDrivigFlags |= movePrimary; // aktywnie prowadzące pojazd
    }

    SetDriverPsyche(); // na końcu, bo wymaga ustawienia zmiennych
    TableClear();

    if( WriteLogFlag ) {
#ifdef _WIN32
		CreateDirectory( "physicslog", NULL );
#elif __linux__
        mkdir( "physicslog", 0644 );
#endif
        LogFile.open( std::string( "physicslog/" + VehicleName + ".dat" ),
            std::ios::in | std::ios::out | std::ios::trunc );
#if LOGPRESS == 0
        LogFile
            << "Time[s] Velocity[m/s] Acceleration[m/ss] "
            << "Coupler.Dist[m] Coupler.Force[N] TractionForce[kN] FrictionForce[kN] BrakeForce[kN] "
            << "BrakePress[MPa] PipePress[MPa] MotorCurrent[A] "
            << "MCP SCP BCP LBP Direction Command CVal1 CVal2 "
            << "Security Wheelslip "
            << "EngineTemp[Deg] OilTemp[Deg] WaterTemp[Deg] WaterAuxTemp[Deg]"
            << "\r\n";
        LogFile << std::fixed << std::setprecision( 4 );
#endif
#if LOGPRESS == 1
        LogFile << string( "t\tVel\tAcc\tPP\tVVP\tBP\tBVP\tCVP" ).c_str() << "\n";
#endif
        LogFile.flush();
    }
};

void TController::CloseLog()
{
    if (WriteLogFlag)
    {
        LogFile.close();
    }
};

TController::~TController()
{ // wykopanie mechanika z roboty
    delete TrainParams;
    CloseLog();
};

// zamiana kodu rozkazu na opis
std::string TController::Order2Str(TOrders Order) const {

    if( Order & Change_direction ) {
        // może być nałożona na inną i wtedy ma priorytet
        return "Change_direction";
    }
    switch( Order ) {
        case Wait_for_orders:     return "Wait_for_orders";
        case Prepare_engine:      return "Prepare_engine";
        case Release_engine:      return "Release_engine";
        case Change_direction:    return "Change_direction";
        case Connect:             return "Connect";
        case Disconnect:          return "Disconnect";
        case Shunt:               return "Shunt";
        case Obey_train:          return "Obey_train";
        case Jump_to_first_order: return "Jump_to_first_order";
        default:                  return "Undefined";
    }
}

std::string TController::OrderCurrent() const
{ // pobranie aktualnego rozkazu celem wyświetlenia
    return "[" + std::to_string(OrderPos) + "] " + Order2Str(OrderList[OrderPos]);
};

void TController::OrdersClear()
{ // czyszczenie tabeli rozkazów na starcie albo po dojściu do końca
    OrderPos = 0;
    OrderTop = 1; // szczyt stosu rozkazów
    for (int b = 0; b < maxorders; b++)
        OrderList[b] = Wait_for_orders;
#if LOGORDERS
    WriteLog("--> OrdersClear");
#endif
};

void TController::Activation()
{ // umieszczenie obsady w odpowiednim członie, wykonywane wyłącznie gdy steruje AI
    iDirection = iDirectionOrder; // kierunek (względem sprzęgów pojazdu z AI) właśnie został
    // ustalony (zmieniony)
    if (iDirection)
    { // jeśli jest ustalony kierunek
        TDynamicObject *old = pVehicle, *d = pVehicle; // w tym siedzi AI
        TController *drugi; // jakby były dwa, to zamienić miejscami, a nie robić wycieku pamięci
        // poprzez nadpisanie
        auto const localbrakelevel { mvOccupied->LocalBrakePosA };
        while (mvControlling->MainCtrlPos) // samo zapętlenie DecSpeed() nie wystarcza :/
            DecSpeed(true); // wymuszenie zerowania nastawnika jazdy
        while (mvOccupied->ActiveDir < 0)
            mvOccupied->DirectionForward(); // kierunek na 0
        while (mvOccupied->ActiveDir > 0)
            mvOccupied->DirectionBackward();
        if (TestFlag(d->MoverParameters->Couplers[iDirectionOrder < 0 ? 1 : 0].CouplingFlag, ctrain_controll)) {
            mvControlling->MainSwitch( false); // dezaktywacja czuwaka, jeśli przejście do innego członu
            mvOccupied->DecLocalBrakeLevel(LocalBrakePosNo); // zwolnienie hamulca w opuszczanym pojeździe
            //   mvOccupied->BrakeLevelSet((mvOccupied->BrakeHandle==FVel6)?4:-2); //odcięcie na
            //   zaworze maszynisty, FVel6 po drugiej stronie nie luzuje
            mvOccupied->BrakeLevelSet(
                mvOccupied->Handle->GetPos(bh_NP)); // odcięcie na zaworze maszynisty
        }
        mvOccupied->ActiveCab = mvOccupied->CabNo; // użytkownik moze zmienić ActiveCab wychodząc
        mvOccupied->CabDeactivisation(); // tak jest w Train.cpp
        // przejście AI na drugą stronę EN57, ET41 itp.
        while (TestFlag(d->MoverParameters->Couplers[iDirection < 0 ? 1 : 0].CouplingFlag, ctrain_controll))
        { // jeśli pojazd z przodu jest ukrotniony, to przechodzimy do niego
            d = iDirection * d->DirectionGet() < 0 ? d->Next() :
                                                     d->Prev(); // przechodzimy do następnego członu
            if (d)
            {
                drugi = d->Mechanik; // zapamiętanie tego, co ewentualnie tam siedzi, żeby w razie
                // dwóch zamienić miejscami
                d->Mechanik = this; // na razie bilokacja
                d->MoverParameters->SetInternalCommand(
                    "", 0, 0); // usunięcie ewentualnie zalegającej komendy (Change_direction?)
                if (d->DirectionGet() != pVehicle->DirectionGet()) // jeśli są przeciwne do siebie
                    iDirection = -iDirection; // to będziemy jechać w drugą stronę względem
                // zasiedzianego pojazdu
                pVehicle->Mechanik = drugi; // wsadzamy tego, co ewentualnie był (podwójna trakcja)
                pVehicle->MoverParameters->CabNo = 0; // wyłączanie kabin po drodze
                pVehicle->MoverParameters->ActiveCab = 0; // i zaznaczenie, że nie ma tam nikogo
                pVehicle = d; // a mechu ma nowy pojazd (no, człon)
            }
            else
                break; // jak koniec składu, to mechanik dalej nie idzie
        }
        if (pVehicle != old)
        { // jeśli zmieniony został pojazd prowadzony
            if( ( simulation::Train )
             && ( simulation::Train->Dynamic() == old ) ) {
                // ewentualna zmiana kabiny użytkownikowi
                Global.changeDynObj = pVehicle; // uruchomienie protezy
            }
            ControllingSet(); // utworzenie połączenia do sterowanego pojazdu (może się zmienić) -
            // silnikowy dla EZT
        }
        if( mvControlling->EngineType == TEngineType::DieselEngine ) {
            // dla 2Ls150 - przed ustawieniem kierunku - można zmienić tryb pracy
            if( mvControlling->ShuntModeAllow ) {
                mvControlling->CurrentSwitch(
                    ( ( OrderList[ OrderPos ] & Shunt ) == Shunt )
                   || ( fMass > 224000.0 ) ); // do tego na wzniesieniu może nie dać rady na liniowym
            }
        }
        // Ra: to przełączanie poniżej jest tu bez sensu
        mvOccupied->ActiveCab =
            iDirection; // aktywacja kabiny w prowadzonym pojeżdzie (silnikowy może być odwrotnie?)
        // mvOccupied->CabNo=iDirection;
        // mvOccupied->ActiveDir=0; //żeby sam ustawił kierunek
        mvOccupied->CabActivisation(); // uruchomienie kabin w członach
        DirectionForward(true); // nawrotnik do przodu
        if (localbrakelevel > 0.0) // hamowanie tylko jeśli był wcześniej zahamowany (bo możliwe, że jedzie!)
            mvOccupied->LocalBrakePosA = localbrakelevel; // zahamuj jak wcześniej
        CheckVehicles(); // sprawdzenie składu, AI zapali światła
        TableClear(); // resetowanie tabelki skanowania torów
    }
};

void TController::AutoRewident()
{ // autorewident: nastawianie hamulców w składzie
    int r = 0, g = 0, p = 0; // ilości wagonów poszczególnych typów
    TDynamicObject *d = pVehicles[0]; // pojazd na czele składu
    // 1. Zebranie informacji o składzie pociągu — przejście wzdłuż składu i odczyt parametrów:
    //   · ilość wagonów -> są zliczane, wszystkich pojazdów jest (iVehicles)
    //   · długość (jako suma) -> jest w (fLength)
    //   · masa (jako suma) -> jest w (fMass)
    while (d)
    { // klasyfikacja pojazdów wg BrakeDelays i mocy (licznik)
        if (d->MoverParameters->Power < 1) // - lokomotywa - Power>1 - ale może być nieczynna na końcu...
            if (TestFlag(d->MoverParameters->BrakeDelays, bdelay_R))
                ++r; // - wagon pospieszny - jest R
            else if (TestFlag(d->MoverParameters->BrakeDelays, bdelay_G))
                ++g; // - wagon towarowy - jest G (nie ma R)
            else
                ++p; // - wagon osobowy - reszta (bez G i bez R)
        d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
    }
    // 2. Określenie typu pociągu i nastawy:
    int ustaw; //+16 dla pasażerskiego
    if (r + g + p == 0)
        ustaw = 16 + bdelay_R; // lokomotywa luzem (może być wieloczłonowa)
    else
    { // jeśli są wagony
        ustaw = (g < std::min(4, r + p) ? 16 : 0);
        if (ustaw) // jeśli towarowe < Min(4, pospieszne+osobowe)
        { // to skład pasażerski - nastawianie pasażerskiego
            ustaw += (g && (r < g + p)) ? bdelay_P : bdelay_R;
            // jeżeli towarowe>0 oraz pospiesze<=towarowe+osobowe to P (0)
            // inaczej R (2)
        }
        else
        { // inaczej towarowy - nastawianie towarowego
            if ((fLength < 300.0) && (fMass < 600000.0)) //[kg]
                ustaw |= bdelay_P; // jeżeli długość<300 oraz masa<600 to P (0)
            else if ((fLength < 500.0) && (fMass < 1300000.0))
                ustaw |= bdelay_R; // jeżeli długość<500 oraz masa<1300 to GP (2)
            else
                ustaw |= bdelay_G; // inaczej G (1)
        }
        // zasadniczo na sieci PKP kilka lat temu na P/GP jeździły tylko kontenerowce o
        // rozkładowej 90 km/h. Pozostałe jeździły 70 km/h i były nastawione na G.
    }
    d = pVehicles[0]; // pojazd na czele składu
    p = 0; // będziemy tu liczyć wagony od lokomotywy dla nastawy GP
    while (d)
    { // 3. Nastawianie
        if( ( true == AIControllFlag )
         || ( d != pVehicle ) ) {
            // don't touch human-controlled vehicle, but others are free game
            switch( ustaw ) {

                case bdelay_P: {
                    // towarowy P - lokomotywa na G, reszta na P.
                    d->MoverParameters->BrakeDelaySwitch(
                        d->MoverParameters->Power > 1 ?
                            bdelay_G :
                            bdelay_P );
                    break;
                }
                case bdelay_G: {
                    // towarowy G - wszystko na G, jeśli nie ma to P (powinno się wyłączyć hamulec)
                    d->MoverParameters->BrakeDelaySwitch(
                        TestFlag( d->MoverParameters->BrakeDelays, bdelay_G ) ?
                            bdelay_G :
                            bdelay_P );
                    break;
                }
                case bdelay_R: {
                    // towarowy GP - lokomotywa oraz 5 pierwszych pojazdów przy niej na G, reszta na P
                    if( d->MoverParameters->Power > 1 ) {
                        d->MoverParameters->BrakeDelaySwitch( bdelay_G );
                        p = 0; // a jak będzie druga w środku?
                    }
                    else {
                        d->MoverParameters->BrakeDelaySwitch(
                            ++p <= 5 ?
                                bdelay_G :
                                bdelay_P );
                    }
                    break;
                }
                case 16 + bdelay_R: {
                    // pasażerski R - na R, jeśli nie ma to P
                    d->MoverParameters->BrakeDelaySwitch(
                        TestFlag( d->MoverParameters->BrakeDelays, bdelay_R ) ?
                            bdelay_R :
                            bdelay_P );
                    break;
                }
                case 16 + bdelay_P: {
                    // pasażerski P - wszystko na P
                    d->MoverParameters->BrakeDelaySwitch( bdelay_P );
                    break;
                }
            }
        }
        d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
    }
    //ustawianie trybu pracy zadajnika hamulca, wystarczy raz po inicjalizacji AI
    if( true == AIControllFlag ) {
        // if a human is in charge leave the brake mode up to them, otherwise do as you like
        for( int i = 1; i <= 8; i *= 2 ) {
            if( ( mvOccupied->BrakeOpModes & i ) > 0 ) {
                mvOccupied->BrakeOpModeFlag = i;
            }
        }
    }

	// teraz zerujemy tabelkę opóźnienia hamowania
	for (int i = 0; i < BrakeAccTableSize; ++i)
	{
		fBrake_a0[i+1] = 0;
		fBrake_a1[i+1] = 0;
	}

    if( OrderCurrentGet() & Shunt ) {
        // for uniform behaviour and compatibility with older scenarios set default acceleration table values for shunting
        fAccThreshold = (
            mvOccupied->TrainType == dt_EZT ? -0.55 :
            mvOccupied->TrainType == dt_DMU ? -0.45 :
            -0.2 );
        // HACK: emu with induction motors need to start their braking a bit sooner than the ones with series motors
        if( ( mvOccupied->TrainType == dt_EZT )
         && ( mvControlling->EngineType == TEngineType::ElectricInductionMotor ) ) {
            fAccThreshold += 0.10;
        }
    }

    if( OrderCurrentGet() & Obey_train ) {
        // 4. Przeliczanie siły hamowania
        double const velstep = ( mvOccupied->Vmax*0.5 ) / BrakeAccTableSize;
        d = pVehicles[0]; // pojazd na czele składu
	    while (d) { 
            for( int i = 0; i < BrakeAccTableSize; ++i ) {
                fBrake_a0[ i + 1 ] += d->MoverParameters->BrakeForceR( 0.25, velstep*( 1 + 2 * i ) );
                fBrake_a1[ i + 1 ] += d->MoverParameters->BrakeForceR( 1.00, velstep*( 1 + 2 * i ) );
		    }
		    d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
	    }
	    for (int i = 0; i < BrakeAccTableSize; ++i)
	    {
		    fBrake_a1[i+1] -= fBrake_a0[i+1];
		    fBrake_a0[i+1] /= fMass;
		    fBrake_a0[i + 1] += 0.001*velstep*(1 + 2 * i);
		    fBrake_a1[i+1] /= (12*fMass);
	    }

        IsCargoTrain = ( mvOccupied->CategoryFlag == 1 ) && ( ( mvOccupied->BrakeDelayFlag & bdelay_G ) != 0 );
        IsHeavyCargoTrain = ( true == IsCargoTrain ) && ( fBrake_a0[ 1 ] > 0.4 );

        BrakingInitialLevel = (
            IsHeavyCargoTrain ? 1.25 :
            IsCargoTrain      ? 1.25 :
                                1.00 );

        BrakingLevelIncrease = (
            IsHeavyCargoTrain ? 0.25 :
            IsCargoTrain      ? 0.25 :
                                0.25 );

        if( mvOccupied->TrainType == dt_EZT ) {
            if( mvControlling->EngineType == TEngineType::ElectricInductionMotor ) {
                // HACK: emu with induction motors need to start their braking a bit sooner than the ones with series motors
                fNominalAccThreshold = std::max( -0.60, -fBrake_a0[ BrakeAccTableSize ] - 8 * fBrake_a1[ BrakeAccTableSize ] );
            }
            else {
                fNominalAccThreshold = std::max( -0.75, -fBrake_a0[ BrakeAccTableSize ] - 8 * fBrake_a1[ BrakeAccTableSize ] );
            }       
		    fBrakeReaction = 0.25;
	    }
        else if( mvOccupied->TrainType == dt_DMU ) {
            fNominalAccThreshold = std::max( -0.45, -fBrake_a0[ BrakeAccTableSize ] - 8 * fBrake_a1[ BrakeAccTableSize ] );
            fBrakeReaction = 0.25;
        }
        else if (ustaw > 16) {
            fNominalAccThreshold = -fBrake_a0[ BrakeAccTableSize ] - 4 * fBrake_a1[ BrakeAccTableSize ];
		    fBrakeReaction = 1.00 + fLength*0.004;
	    }
	    else {
            fNominalAccThreshold = -fBrake_a0[ BrakeAccTableSize ] - 1 * fBrake_a1[ BrakeAccTableSize ];
		    fBrakeReaction = 1.00 + fLength*0.005;
	    }
	    fAccThreshold = fNominalAccThreshold;
    }
}

double TController::ESMVelocity(bool Main)
{
	double fCurrentCoeff = 0.9;
	double fFrictionCoeff = 0.85;
	double ESMVel = 9999;
	int MCPN = mvControlling->MainCtrlActualPos;
	int SCPN = mvControlling->ScndCtrlActualPos;
	if (Main)
		MCPN += 1;
	else
		SCPN += 1;
	if ((mvControlling->RList[MCPN].ScndAct < 255)&&(mvControlling->ScndCtrlActualPos==0))
		SCPN = mvControlling->RList[MCPN].ScndAct;
	double FrictionMax = mvControlling->Mass*9.81*mvControlling->Adhesive(mvControlling->RunningTrack.friction)*fFrictionCoeff;
	double IF = mvControlling->Imax;
	double MS = 0;
	double Fmax = 0;
	for (int i = 0; i < 5; i++)
	{
		MS = mvControlling->MomentumF(IF, IF, SCPN);
		Fmax = MS * mvControlling->RList[MCPN].Bn * mvControlling->RList[MCPN].Mn * 2 / mvControlling->WheelDiameter * mvControlling->Transmision.Ratio;
        if( Fmax != 0.0 ) {
            IF = 0.5 * IF * ( 1 + FrictionMax / Fmax );
        }
        else {
            // NOTE: gets trimmed to actual highest acceptable value after the loop
            IF = std::numeric_limits<double>::max();
            break;
        }
	}
	IF = std::min(IF, mvControlling->Imax*fCurrentCoeff);
	double R = mvControlling->RList[MCPN].R + mvControlling->CircuitRes + mvControlling->RList[MCPN].Mn*mvControlling->WindingRes;
	double pole = mvControlling->MotorParam[SCPN].fi *
		std::max(abs(IF) / (abs(IF) + mvControlling->MotorParam[SCPN].Isat) - mvControlling->MotorParam[SCPN].fi0, 0.0);
	double Us = abs(mvControlling->Voltage) - IF*R;
	double ns = std::max(0.0, Us / (pole*mvControlling->RList[MCPN].Mn));
	ESMVel = ns * mvControlling->WheelDiameter*M_PI*3.6/mvControlling->Transmision.Ratio;
	return ESMVel;
}

int TController::CheckDirection() {

    int d = mvOccupied->DirAbsolute; // który sprzęg jest z przodu
    if( !d ) // jeśli nie ma ustalonego kierunku
        d = mvOccupied->CabNo; // to jedziemy wg aktualnej kabiny
    iDirection = d; // ustalenie kierunku jazdy (powinno zrobić PrepareEngine?)
    return d;
}

bool TController::CheckVehicles(TOrders user)
{ // sprawdzenie stanu posiadanych pojazdów w składzie i zapalenie świateł
    TDynamicObject *p; // roboczy wskaźnik na pojazd
    iVehicles = 0; // ilość pojazdów w składzie
    int d = CheckDirection();
    d = d >= 0 ? 0 : 1; // kierunek szukania czoła (numer sprzęgu)
    pVehicles[0] = p = pVehicle->FirstFind(d); // pojazd na czele składu
    // liczenie pojazdów w składzie i ustalenie parametrów
    int dir = d = 1 - d; // a dalej będziemy zliczać od czoła do tyłu
    fLength = 0.0; // długość składu do badania wyjechania za ograniczenie
    fMass = 0.0; // całkowita masa do liczenia stycznej składowej grawitacji
    fVelMax = -1; // ustalenie prędkości dla składu
    bool main = true; // czy jest głównym sterującym
    iDrivigFlags |= moveOerlikons; // zakładamy, że są same Oerlikony
    // Ra 2014-09: ustawić moveMultiControl, jeśli wszystkie są w ukrotnieniu (i skrajne mają kabinę?)
    while (p)
    { // sprawdzanie, czy jest głównym sterującym, żeby nie było konfliktu
        if (p->Mechanik) // jeśli ma obsadę
            if (p->Mechanik != this) // ale chodzi o inny pojazd, niż aktualnie sprawdzający
                if (p->Mechanik->iDrivigFlags & movePrimary) // a tamten ma priorytet
                    if ((iDrivigFlags & movePrimary) && (mvOccupied->DirAbsolute) &&
                        (mvOccupied->BrakeCtrlPos >= -1)) // jeśli rządzi i ma kierunek
                        p->Mechanik->iDrivigFlags &= ~movePrimary; // dezaktywuje tamtego
                    else
                        main = false; // nici z rządzenia
        ++iVehicles; // jest jeden pojazd więcej
        pVehicles[1] = p; // zapamiętanie ostatniego
        fLength += p->MoverParameters->Dim.L; // dodanie długości pojazdu
        fMass += p->MoverParameters->TotalMass; // dodanie masy łącznie z ładunkiem
        fVelMax = min_speed( fVelMax, p->MoverParameters->Vmax ); // ustalenie maksymalnej prędkości dla składu
        // reset oerlikon brakes consist flag as different type was detected
        if( ( p->MoverParameters->BrakeSubsystem != TBrakeSubSystem::ss_ESt )
         && ( p->MoverParameters->BrakeSubsystem != TBrakeSubSystem::ss_LSt ) ) {
            iDrivigFlags &= ~( moveOerlikons );
        }
        p = p->Neightbour(dir); // pojazd podłączony od wskazanej strony
    }
    if (main)
        iDrivigFlags |= movePrimary; // nie znaleziono innego, można się porządzić
    ControllingSet(); // ustalenie członu do sterowania (może być inny niż zasiedziany)
    int pantmask = 1;
    if (iDrivigFlags & movePrimary)
    { // jeśli jest aktywnie prowadzącym pojazd, może zrobić własny porządek
        p = pVehicles[0];
        while (p)
        {
            // HACK: wagony muszą mieć baterię załączoną do otwarcia drzwi...
            if( ( p != pVehicle )
             && ( ( p->MoverParameters->Couplers[ side::front ].CouplingFlag & ( coupling::control | coupling::permanent ) ) == 0 )
             && ( ( p->MoverParameters->Couplers[ side::rear ].CouplingFlag  & ( coupling::control | coupling::permanent ) ) == 0 ) ) {
                // NOTE: don't set battery in the occupied vehicle, let the user/ai do it explicitly
                p->MoverParameters->BatterySwitch( true );
            }

            if (p->asDestination == "none")
                p->DestinationSet(TrainParams->Relation2, TrainParams->TrainName); // relacja docelowa, jeśli nie było
            if (AIControllFlag) // jeśli prowadzi komputer
                p->RaLightsSet(0, 0); // gasimy światła
            if (p->MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector)
            { // jeśli pojazd posiada pantograf, to przydzielamy mu maskę, którą będzie informował o jeździe bezprądowej
                p->iOverheadMask = pantmask;
                pantmask = pantmask << 1; // przesunięcie bitów, max. 32 pojazdy z pantografami w składzie
            }
            d = p->DirectionSet(d ? 1 : -1); // zwraca położenie następnego (1=zgodny,0=odwrócony - względem czoła składu)
            p->ctOwner = this; // dominator oznacza swoje terytorium
            p = p->Next(); // pojazd podłączony od tyłu (licząc od czoła)
        }
        if (AIControllFlag)
        { // jeśli prowadzi komputer
            if( true == TestFlag( OrderCurrentGet(), Obey_train ) ) {
                // jeśli jazda pociągowa
                // światła pociągowe (Pc1) i końcówki (Pc5)
                auto const frontlights { (
                    ( m_lighthints[ side::front ] != -1 ) ?
                        m_lighthints[ side::front ] :
                        light::headlight_left | light::headlight_right | light::headlight_upper ) };
                auto const rearlights { (
                    ( m_lighthints[ side::rear ] != -1 ) ?
                        m_lighthints[ side::rear ] :
                        light::redmarker_left | light::redmarker_right | light::rearendsignals ) };
                Lights(
                    frontlights,
                    rearlights );
            }
            else if (OrderCurrentGet() & (Shunt | Connect))
            {
                // HACK: the 'front' and 'rear' of the consist is determined by current consist direction
                // since direction shouldn't affect Tb1 light configuration, we 'counter' this behaviour by virtually swapping end vehicles
                if( mvOccupied->ActiveDir > 0 ) {
                    Lights(
                        light::headlight_right,
                        ( pVehicles[ 1 ]->MoverParameters->CabNo != 0 ?
                            light::headlight_left :
                            0 ) ); //światła manewrowe (Tb1) na pojeździe z napędem
                }
                else {
                    Lights(
                        ( pVehicles[ 1 ]->MoverParameters->CabNo != 0 ?
                            light::headlight_left :
                            0 ),
                        light::headlight_right ); //światła manewrowe (Tb1) na pojeździe z napędem
                }
            }
            else if( true == TestFlag( OrderCurrentGet(), Disconnect ) ) {
                if( mvOccupied->ActiveDir > 0 ) {
                    // jak ma kierunek do przodu
                    // światła manewrowe (Tb1) tylko z przodu, aby nie pozostawić odczepionego ze światłem
                    Lights( light::headlight_right, 0 );
                }
                else {
                    // jak dociska
                    // światła manewrowe (Tb1) tylko z przodu, aby nie pozostawić odczepionego ze światłem
                    Lights( 0, light::headlight_right );
                }
            }
            // nastawianie hamulca do jazdy pociągowej
            if( OrderCurrentGet() & ( Obey_train | Shunt ) ) {
                AutoRewident();
            }
        }
        else { // gdy człowiek i gdy nastąpiło połącznie albo rozłączenie
               // Ra 2014-02: lepiej tu niż w pętli obsługującej komendy, bo tam się zmieni informacja o składzie
            switch (user) {
            case Change_direction: {
                while (OrderCurrentGet() & (Change_direction)) {
                    // zmianę kierunku też można olać, ale zmienić kierunek skanowania!
                    JumpToNextOrder();
                }
                break;
            }
            case Connect: {
                while (OrderCurrentGet() & (Change_direction)) {
                    // zmianę kierunku też można olać, ale zmienić kierunek skanowania!
                    JumpToNextOrder();
                }
                if (OrderCurrentGet() & (Connect)) {
                    // jeśli miało być łączenie, zakładamy, że jest dobrze (sprawdzić?)
                    iCoupler = 0; // koniec z doczepianiem
                    iDrivigFlags &= ~moveConnect; // zdjęcie flagi doczepiania
                    JumpToNextOrder(); // wykonanie następnej komendy
                    if (OrderCurrentGet() & (Change_direction)) {
                        // zmianę kierunku też można olać, ale zmienić kierunek skanowania!
                        JumpToNextOrder();
                    }
                        
                }
                break;
            }
            case Disconnect: {
                while (OrderCurrentGet() & (Change_direction)) {
                    // zmianę kierunku też można olać, ale zmienić kierunek skanowania!
                    JumpToNextOrder();
                }
                if (OrderCurrentGet() & (Disconnect)) {
                    // wypadało by sprawdzić, czy odczepiono wagony w odpowiednim miejscu (iVehicleCount)
                    JumpToNextOrder(); // wykonanie następnej komendy
                    if (OrderCurrentGet() & (Change_direction)) {
                        // zmianę kierunku też można olać, ale zmienić kierunek skanowania!
                        JumpToNextOrder();
                    }
                }
                break;
            }
            default: {
                break;
            }
            } // switch
        }
        // Ra 2014-09: tymczasowo prymitywne ustawienie warunku pod kątem SN61
        if( ( mvOccupied->TrainType == dt_EZT )
         || ( mvOccupied->TrainType == dt_DMU )
         || ( iVehicles == 1 ) ) {
            // zmiana czoła przez zmianę kabiny
            iDrivigFlags |= movePushPull;
        }
        else {
            // zmiana czoła przez manewry
            iDrivigFlags &= ~movePushPull;
        }
    } // blok wykonywany, gdy aktywnie prowadzi
    return true;
}

void TController::Lights(int head, int rear)
{ // zapalenie świateł w skłądzie
    pVehicles[0]->RaLightsSet(head, -1); // zapalenie przednich w pierwszym
    pVehicles[1]->RaLightsSet(-1, rear); // zapalenie końcówek w ostatnim
}

void TController::DirectionInitial()
{ // ustawienie kierunku po wczytaniu trainset (może jechać na wstecznym
    mvOccupied->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
    if (mvOccupied->Vel > 0.0)
    { // jeśli na starcie jedzie
        iDirection = iDirectionOrder =
            (mvOccupied->V > 0 ? 1 : -1); // początkowa prędkość wymusza kierunek jazdy
        DirectionForward(mvOccupied->V * mvOccupied->CabNo >= 0.0); // a dalej ustawienie nawrotnika
    }
    CheckVehicles(); // sprawdzenie świateł oraz skrajnych pojazdów do skanowania
};

int TController::OrderDirectionChange(int newdir, TMoverParameters *Vehicle)
{ // zmiana kierunku jazdy, niezależnie od kabiny
    int testd = newdir;
    if (Vehicle->Vel < 0.5)
    { // jeśli prawie stoi, można zmienić kierunek, musi być wykonane dwukrotnie, bo za pierwszym
        // razem daje na zero
        switch (newdir * Vehicle->CabNo)
        { // DirectionBackward() i DirectionForward() to zmiany względem kabiny
        case -1: // if (!Vehicle->DirectionBackward()) testd=0; break;
            DirectionForward(false);
            break;
        case 1: // if (!Vehicle->DirectionForward()) testd=0; break;
            DirectionForward(true);
            break;
        }
        if (testd == 0)
            VelforDriver = -1; // kierunek został zmieniony na żądany, można jechać
    }
    else // jeśli jedzie
        VelforDriver = 0; // ma się zatrzymać w celu zmiany kierunku
    if ((Vehicle->ActiveDir == 0) && (VelforDriver < Vehicle->Vel)) // Ra: to jest chyba bez sensu
        IncBrake(); // niech hamuje
    if (Vehicle->ActiveDir == testd * Vehicle->CabNo)
        VelforDriver = -1; // można jechać, bo kierunek jest zgodny z żądanym
    if (Vehicle->TrainType == dt_EZT)
        if (Vehicle->ActiveDir > 0)
            // if () //tylko jeśli jazda pociągowa (tego nie wiemy w momencie odpalania silnika)
            Vehicle->DirectionForward(); // Ra: z przekazaniem do silnikowego
    return (int)VelforDriver; // zwraca prędkość mechanika
}

void TController::WaitingSet(double Seconds)
{ // ustawienie odczekania po zatrzymaniu (ustawienie w trakcie jazdy zatrzyma)
    fStopTime = -Seconds; // ujemna wartość oznacza oczekiwanie (potem >=0.0)
}

void TController::SetVelocity(double NewVel, double NewVelNext, TStopReason r)
{ // ustawienie nowej prędkości
    WaitingTime = -WaitingExpireTime; // przypisujemy -WaitingExpireTime, a potem porównujemy z zerem
    if (NewVel == 0.0) // jeśli ma stanąć
    {
        if (r != stopNone) // a jest powód podany
            eStopReason = r; // to zapamiętać nowy powód
    }
    else
    {
        eStopReason = stopNone; // podana prędkość, to nie ma powodów do stania
        // to całe poniżej to warunki zatrąbienia przed ruszeniem
        if (OrderList[OrderPos] ?
                OrderList[OrderPos] & (Obey_train | Shunt | Connect | Prepare_engine) :
                true) // jeśli jedzie w dowolnym trybie
            if ((mvOccupied->Vel < 1.0)) // jesli stoi (na razie, bo chyba powinien też, gdy hamuje przed semaforem)
                if (iDrivigFlags & moveStartHorn) // jezeli trąbienie włączone
                    if (!(iDrivigFlags & (moveStartHornDone | moveConnect)))
                        // jeśli nie zatrąbione i nie jest to moment podłączania składu
                        if (mvOccupied->CategoryFlag & 1)
                            // tylko pociągi trąbią (unimogi tylko na torach, więc trzeba raczej sprawdzać tor)
                            if ((NewVel >= 1.0) || (NewVel < 0.0)) {
                                // o ile prędkość jest znacząca
                                // zatrąb po odhamowaniu
                                iDrivigFlags |= moveStartHornNow;
                            }
    }
    VelSignal = NewVel; // prędkość zezwolona na aktualnym odcinku
    VelNext = NewVelNext; // prędkość przy następnym obiekcie
}

double TController::BrakeAccFactor() const
{
	double Factor = 1.0;
    if( ( ActualProximityDist > fMinProximityDist )
     || ( mvOccupied->Vel > VelDesired + fVelPlus ) ) {
        Factor += ( fBrakeReaction * ( mvOccupied->BrakeCtrlPosR < 0.5 ? 1.5 : 1 ) ) * mvOccupied->Vel / ( std::max( 0.0, ActualProximityDist ) + 1 ) * ( ( AccDesired - AbsAccS_pub ) / fAccThreshold );
    }
	return Factor;
}

void TController::SetDriverPsyche()
{
    if ((Psyche == Aggressive) && (OrderList[OrderPos] == Obey_train))
    {
        ReactionTime = HardReactionTime; // w zaleznosci od charakteru maszynisty
        if (mvOccupied->CategoryFlag & 2)
        {
            WaitingExpireTime = 1; // tyle ma czekać samochód, zanim się ruszy
            AccPreferred = 3.0; //[m/ss] agresywny
        }
        else
        {
            WaitingExpireTime = 61; // tyle ma czekać, zanim się ruszy
            AccPreferred = HardAcceleration; // agresywny
        }
    }
    else
    {
        ReactionTime = EasyReactionTime; // spokojny
        if (mvOccupied->CategoryFlag & 2)
        {
            WaitingExpireTime = 3; // tyle ma czekać samochód, zanim się ruszy
            AccPreferred = 2.0; //[m/ss]
        }
        else
        {
            WaitingExpireTime = 65; // tyle ma czekać, zanim się ruszy
            AccPreferred = EasyAcceleration;
        }
    }
    if (mvControlling && mvOccupied)
    { // with Controlling do
        if (mvControlling->MainCtrlPos < 3)
            ReactionTime = mvControlling->InitialCtrlDelay + ReactionTime;
        if (mvOccupied->BrakeCtrlPos > 1)
            ReactionTime = 0.5 * ReactionTime;
    }
};

bool TController::PrepareEngine()
{ // odpalanie silnika
	bool OK = false,
		voltfront = false,
		voltrear = false;
    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;
    iDrivigFlags |= moveActive; // może skanować sygnały i reagować na komendy

    if ( mvControlling->EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
        voltfront = true;
        voltrear = true;
    }
    else {
        if( mvOccupied->TrainType != dt_EZT ) {
            // Ra 2014-06: to jest wirtualny prąd dla spalinowych???
            voltfront = true;
        }
    }
    auto workingtemperature { true };
    if (AIControllFlag) {
        // część wykonawcza dla sterowania przez komputer
        mvOccupied->BatterySwitch( true );
        if( ( mvControlling->EngineType == TEngineType::DieselElectric )
         || ( mvControlling->EngineType == TEngineType::DieselEngine ) ) {
            mvControlling->OilPumpSwitch( true );
            workingtemperature = UpdateHeating();
            if( true == workingtemperature ) {
                mvControlling->FuelPumpSwitch( true );
            }
        }
        if (mvControlling->EnginePowerSource.SourceType == TPowerSource::CurrentCollector)
        { // jeśli silnikowy jest pantografującym
            mvOccupied->PantFront( true );
            mvOccupied->PantRear( true );
            if (mvControlling->PantPress < 4.2) {
                // załączenie małej sprężarki
                if( mvControlling->TrainType != dt_EZT ) {
                    // odłączenie zbiornika głównego, bo z nim nie da rady napompować
                    mvControlling->bPantKurek3 = false;
                }
                mvControlling->PantCompFlag = true; // załączenie sprężarki pantografów
            }
            else {
                // jeżeli jest wystarczające ciśnienie w pantografach
                if ((!mvControlling->bPantKurek3) ||
                    (mvControlling->PantPress <=
                     mvControlling->ScndPipePress)) // kurek przełączony albo główna już pompuje
                    mvControlling->PantCompFlag = false; // sprężarkę pantografów można już wyłączyć
            }
        }
    }

    if (mvControlling->PantFrontVolt || mvControlling->PantRearVolt || voltfront || voltrear)
    { // najpierw ustalamy kierunek, jeśli nie został ustalony
        if( !iDirection ) {
            // jeśli nie ma ustalonego kierunku
            if( mvOccupied->Vel < 0.01 ) { // ustalenie kierunku, gdy stoi
                iDirection = mvOccupied->CabNo; // wg wybranej kabiny
                if( !iDirection ) {
                    // jeśli nie ma ustalonego kierunku
                    if( ( mvControlling->PantFrontVolt != 0.0 ) || ( mvControlling->PantRearVolt != 0.0 ) || voltfront || voltrear ) {
                        if( mvOccupied->Couplers[ 1 ].CouplingFlag == coupling::faux ) {
                            // jeśli z tyłu nie ma nic
                            iDirection = -1; // jazda w kierunku sprzęgu 1
                        }
                        if( mvOccupied->Couplers[ 0 ].CouplingFlag == coupling::faux ) {
                            // jeśli z przodu nie ma nic
                            iDirection = 1; // jazda w kierunku sprzęgu 0
                        }
                    }
                }
            }
            else {
                // ustalenie kierunku, gdy jedzie
                if( ( mvControlling->PantFrontVolt != 0.0 ) || ( mvControlling->PantRearVolt != 0.0 ) || voltfront || voltrear ) {
                    if( mvOccupied->V < 0 ) {
                        // jedzie do tyłu
                        iDirection = -1; // jazda w kierunku sprzęgu 1
                    }
                    else {
                        // jak nie do tyłu, to do przodu
                        iDirection = 1; // jazda w kierunku sprzęgu 0
                    }
                }
            }
        }
        if (AIControllFlag) // jeśli prowadzi komputer
        { // część wykonawcza dla sterowania przez komputer
            if (mvControlling->ConvOvldFlag)
            { // wywalił bezpiecznik nadmiarowy przetwornicy
                while (DecSpeed(true))
                    ; // zerowanie napędu
                mvControlling->ConvOvldFlag = false; // reset nadmiarowego
            }
            else if (false == IsLineBreakerClosed) {
                while (DecSpeed(true))
                    ; // zerowanie napędu
                if( mvOccupied->TrainType == dt_SN61 ) {
                    // specjalnie dla SN61 żeby nie zgasł
                    if( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn == 0 ) {
                        mvControlling->IncMainCtrl( 1 );
                    }
                }
                if( ( mvControlling->EnginePowerSource.SourceType != TPowerSource::CurrentCollector )
                 || ( std::max( mvControlling->GetTrainsetVoltage(), std::abs( mvControlling->RunningTraction.TractionVoltage ) ) > mvControlling->EnginePowerSource.CollectorParameters.MinV ) ) {
                    mvControlling->MainSwitch( true );
                }
            }
            else { 
                OK = ( OrderDirectionChange( iDirection, mvOccupied ) == -1 );
                mvOccupied->ConverterSwitch( true );
                // w EN57 sprężarka w ra jest zasilana z silnikowego
                mvOccupied->CompressorSwitch( true );
                // enable motor blowers
                mvOccupied->MotorBlowersSwitchOff( false, side::front );
                mvOccupied->MotorBlowersSwitch( true, side::front );
                mvOccupied->MotorBlowersSwitchOff( false, side::rear );
                mvOccupied->MotorBlowersSwitch( true, side::rear );
            }
        }
        else
            OK = mvControlling->Mains;
    }
    else
        OK = false;

    if( ( true == OK )
     && ( mvOccupied->ActiveDir != 0 )
     && ( true == workingtemperature )
     && ( ( mvControlling->ScndPipePress > 4.5 ) || ( mvControlling->VeselVolume == 0.0 ) ) ) {

        if( eStopReason == stopSleep ) {
            // jeśli dotychczas spał teraz nie ma powodu do stania
            eStopReason = stopNone;
        }
        eAction = TAction::actUnknown;
        iEngineActive = 1;
        return true;
    }
    else {
        iEngineActive = 0;
        return false;
    }
};

// wyłączanie silnika (test wyłączenia, a część wykonawcza tylko jeśli steruje komputer)
bool TController::ReleaseEngine() {
    
    if( mvOccupied->Vel > 0.01 ) {
        // TBD, TODO: make a dedicated braking procedure out of it for potential reuse
        VelDesired = 0.0;
        AccDesired = std::min( AccDesired, -1.25 ); // hamuj solidnie
        ReactionTime = 0.1;
        while( DecSpeed( true ) ) {
            ; // zerowanie nastawników
        }
        IncBrake();
        // don't bother with the rest until we're standing still
        return false;
    }

    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;

    bool OK { false };

    if( false == AIControllFlag ) {
        // tylko to testujemy dla pojazdu człowieka
        OK = ( ( mvOccupied->ActiveDir == 0 ) && ( mvControlling->Mains ) );
    }
    else  {
        // jeśli steruje komputer
        mvOccupied->BrakeReleaser( 0 );
        if( std::abs( fAccGravity ) < 0.01 ) {
            // release train brake if on flats...
            // TODO: check if we shouldn't leave it engaged instead
            while( true == mvOccupied->DecBrakeLevel() ) {
                // tu moze zmieniać na -2, ale to bez znaczenia
                ;
            }
            // ...and engage independent brake
            while( true == mvOccupied->IncLocalBrakeLevel( 1 ) ) {
                ;
            }
        }
        else {
            // on slopes engage train brake
            AccDesired = std::min( AccDesired, -0.9 );
            while( true == IncBrake() ) {
                ;
            }
        }
        while( DecSpeed( true ) ) {
            ; // zerowanie nastawników
        }
        // set direction to neutral
        while( ( mvOccupied->ActiveDir > 0 ) && ( mvOccupied->DirectionBackward() ) ) { ; }
        while( ( mvOccupied->ActiveDir < 0 ) && ( mvOccupied->DirectionForward() ) ) { ; }

        if( mvOccupied->DoorCloseCtrl == control_t::driver ) {
            // zamykanie drzwi
            if( mvOccupied->DoorLeftOpened ) {
                mvOccupied->DoorLeft( false );
            }
            if( mvOccupied->DoorRightOpened ) {
                mvOccupied->DoorRight( false );
            }
        }

        if( true == mvControlling->Mains ) {
            mvControlling->CompressorSwitch( false );
            mvControlling->ConverterSwitch( false );
            // line breaker/engine
            OK = mvControlling->MainSwitch( false );
            if( mvControlling->EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
                mvControlling->PantFront( false );
                mvControlling->PantRear( false );
            }
        }
        else {
            OK = true;
        }

        if( OK ) {
            // finish vehicle shutdown
            if( ( mvControlling->EngineType == TEngineType::DieselElectric )
             || ( mvControlling->EngineType == TEngineType::DieselEngine ) ) {
                // heating/cooling subsystem
                mvControlling->WaterHeaterSwitch( false );
                // optionally turn off the water pump as well
                if( mvControlling->WaterPump.start_type != start_t::battery ) {
                    mvControlling->WaterPumpSwitch( false );
                }
                // fuel and oil subsystems
                mvControlling->FuelPumpSwitch( false );
                mvControlling->OilPumpSwitch( false );
            }
            // gasimy światła
            Lights( 0, 0 );
            mvOccupied->BatterySwitch( false );
        }
    }

    if (OK) {
        // jeśli się zatrzymał
        iEngineActive = 0;
        eStopReason = stopSleep; // stoimy z powodu wyłączenia
        eAction = TAction::actSleep; //śpi (wygaszony)

        OrderNext(Wait_for_orders); //żeby nie próbował coś robić dalej
        iDrivigFlags &= ~moveActive; // ma nie skanować sygnałów i nie reagować na komendy
        TableClear(); // zapominamy ograniczenia
        VelSignalLast = -1.0;
    }
    return OK;
}

bool TController::IncBrake()
{ // zwiększenie hamowania
    bool OK = false;
    switch( mvOccupied->BrakeSystem ) {
        case TBrakeSystem::Individual: {
            if( mvOccupied->LocalBrake == TLocalBrake::ManualBrake ) {
                OK = mvOccupied->IncManualBrakeLevel( 1 + static_cast<int>( std::floor( 0.5 + std::fabs( AccDesired ) ) ) );
            }
            else {
                OK = mvOccupied->IncLocalBrakeLevel( std::floor( 1.5 + std::abs( AccDesired ) ) );
            }
            break;
        }
        case TBrakeSystem::Pneumatic: {
            // NOTE: can't perform just test whether connected vehicle == nullptr, due to virtual couplers formed with nearby vehicles
            bool standalone { true };
            if( ( mvOccupied->TrainType == dt_ET41 )
             || ( mvOccupied->TrainType == dt_ET42 ) ) {
                // NOTE: we're doing simplified checks full of presuptions here.
                // they'll break if someone does strange thing like turning around the second unit
                if( ( mvOccupied->Couplers[ 1 ].CouplingFlag & coupling::permanent )
                 && ( mvOccupied->Couplers[ 1 ].Connected->Couplers[ 1 ].CouplingFlag > 0 ) ) {
                    standalone = false;
                }
                if( ( mvOccupied->Couplers[ 0 ].CouplingFlag & coupling::permanent )
                 && ( mvOccupied->Couplers[ 0 ].Connected->Couplers[ 0 ].CouplingFlag > 0 ) ) {
                    standalone = false;
                }
            }
            else if( mvOccupied->TrainType == dt_DMU ) {
                // enforce use of train brake for DMUs
                standalone = false;
            }
            else {
/*
                standalone =
                    ( ( mvOccupied->Couplers[ 0 ].CouplingFlag == 0 )
                   && ( mvOccupied->Couplers[ 1 ].CouplingFlag == 0 ) );
*/
                if( pVehicles[ 0 ] != pVehicles[ 1 ] ) {
                    // more detailed version, will use manual braking also for coupled sets of controlled vehicles
                    auto *vehicle = pVehicles[ 0 ]; // start from first
                    while( ( true == standalone )
                        && ( vehicle != nullptr ) ) {
                        // NOTE: we could simplify this by doing only check of the rear coupler, but this can be quite tricky in itself
                        // TODO: add easier ways to access front/rear coupler taking into account vehicle's direction
                        standalone =
                            ( ( ( vehicle->MoverParameters->Couplers[ 0 ].CouplingFlag == 0 ) || ( vehicle->MoverParameters->Couplers[ 0 ].CouplingFlag & coupling::control ) )
                           && ( ( vehicle->MoverParameters->Couplers[ 1 ].CouplingFlag == 0 ) || ( vehicle->MoverParameters->Couplers[ 1 ].CouplingFlag & coupling::control ) ) );
                        vehicle = vehicle->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
                    }
                }
            }
            if( true == standalone ) {
                OK = mvOccupied->IncLocalBrakeLevel(
                    1 + static_cast<int>( std::floor( 0.5 + std::fabs( AccDesired ) ) ) ); // hamowanie lokalnym bo luzem jedzie
            }
            else {
                if( mvOccupied->BrakeCtrlPos + 1 == mvOccupied->BrakeCtrlPosNo ) {
					if (AccDesired < -1.5) // hamowanie nagle
						OK = mvOccupied->BrakeLevelAdd(1.0);
                    else
                        OK = false;
                }
                else {
                    // dodane dla towarowego
					float pos_corr = 0;

					TDynamicObject *d;
					d = pVehicles[0]; // pojazd na czele składu
					while (d)
					{ // przeliczanie dodatkowego potrzebnego spadku ciśnienia
                        if( ( d->MoverParameters->Hamulec->GetBrakeStatus() & b_dmg ) == 0 ) {
                            pos_corr += ( d->MoverParameters->Hamulec->GetCRP() - 5.0 ) * d->MoverParameters->TotalMass;
                        }
						d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
					}
					pos_corr = pos_corr / fMass * 2.5;

					if (mvOccupied->BrakeHandle == TBrakeHandle::FV4a)
					{
						pos_corr += mvOccupied->Handle->GetCP()*0.2;
						
					}
					double deltaAcc = -AccDesired*BrakeAccFactor() - (fBrake_a0[0] + 4.0 * (mvOccupied->BrakeCtrlPosR - 1 - pos_corr)*fBrake_a1[0]);

                    if( deltaAcc > fBrake_a1[0])
					{
                        if( mvOccupied->BrakeCtrlPosR < 0.1 ) {
                            OK = mvOccupied->BrakeLevelAdd( BrakingInitialLevel );
/*
                            // HACK: stronger braking to overcome SA134 engine behaviour
                            if( ( mvOccupied->TrainType == dt_DMU )
                             && ( VelNext == 0.0 ) 
                             && ( fBrakeDist < 200.0 ) ) {
                                mvOccupied->BrakeLevelAdd(
                                    fBrakeDist / ActualProximityDist < 0.8 ?
                                        1.0 :
                                        3.0 );
                            }
*/
                        }
						else
						{
                            OK = mvOccupied->BrakeLevelAdd( BrakingLevelIncrease );
                            // brake harder if the acceleration is much higher than desired
                            if( ( deltaAcc > 2 * fBrake_a1[ 0 ] )
                             && ( mvOccupied->BrakeCtrlPosR + BrakingLevelIncrease <= 5.0 ) ) {
                                mvOccupied->BrakeLevelAdd( BrakingLevelIncrease );
                            }
						}
                    }
                    else
                        OK = false;
                }
            }
            if( mvOccupied->BrakeCtrlPos > 0 ) {
                mvOccupied->BrakeReleaser( 0 );
            }
            break;
        }
        case TBrakeSystem::ElectroPneumatic: {
            if( mvOccupied->EngineType == TEngineType::ElectricInductionMotor ) {
				if (mvOccupied->BrakeHandle == TBrakeHandle::MHZ_EN57) {
					if (mvOccupied->BrakeCtrlPos < mvOccupied->Handle->GetPos(bh_FB))
						OK = mvOccupied->BrakeLevelAdd(1.0);
				}
				else {
					OK = mvOccupied->IncLocalBrakeLevel(1);
				}
            }
            else if( mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos( bh_EPB ) ) {
                mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_EPB ) );
                if( mvOccupied->Handle->GetPos( bh_EPR ) - mvOccupied->Handle->GetPos( bh_EPN ) < 0.1 )
                    mvOccupied->SwitchEPBrake( 1 ); // to nie chce działać
                OK = true;
            }
            else
                OK = false;
            break;
        }
        default: { break; }
    }
    return OK;
}

bool TController::DecBrake()
{ // zmniejszenie siły hamowania
    bool OK = false;
	double deltaAcc = 0;
    switch (mvOccupied->BrakeSystem)
    {
    case TBrakeSystem::Individual:
        if (mvOccupied->LocalBrake == TLocalBrake::ManualBrake)
            OK = mvOccupied->DecManualBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
        else
            OK = mvOccupied->DecLocalBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
        break;
    case TBrakeSystem::Pneumatic:
		deltaAcc = -AccDesired*BrakeAccFactor() - (fBrake_a0[0] + 4 * (mvOccupied->BrakeCtrlPosR-1.0)*fBrake_a1[0]);
		if (deltaAcc < 0)
		{
			if (mvOccupied->BrakeCtrlPosR > 0)
			{
				OK = mvOccupied->BrakeLevelAdd(-0.25);
				//if ((deltaAcc < 5 * fBrake_a1[0]) && (mvOccupied->BrakeCtrlPosR >= 1.2))
				//	mvOccupied->BrakeLevelAdd(-1.0);
				if (mvOccupied->BrakeCtrlPosR < 0.74)
					mvOccupied->BrakeLevelSet(0.0);
			}
		}
        if (!OK)
            OK = mvOccupied->DecLocalBrakeLevel(2);
        if (mvOccupied->PipePress < 3.0)
            Need_BrakeRelease = true;
        break;
    case TBrakeSystem::ElectroPneumatic:
		if (mvOccupied->EngineType == TEngineType::ElectricInductionMotor) {
			if (mvOccupied->BrakeHandle == TBrakeHandle::MHZ_EN57) {
				if (mvOccupied->BrakeCtrlPos > mvOccupied->Handle->GetPos(bh_RP))
					OK = mvOccupied->BrakeLevelAdd(-1.0);
			}
			else {
				OK = mvOccupied->DecLocalBrakeLevel(1);
			}
		}
        else if (mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos(bh_EPR))
        {
            mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPR));
            if (mvOccupied->Handle->GetPos(bh_EPR) - mvOccupied->Handle->GetPos(bh_EPN) < 0.1)
                mvOccupied->SwitchEPBrake(1);
            OK = true;
        }
        else
            OK = false;
        if (!OK)
            OK = mvOccupied->DecLocalBrakeLevel(2);
        break;
    }
    return OK;
};

bool TController::IncSpeed()
{ // zwiększenie prędkości; zwraca false, jeśli dalej się nie da zwiększać
    if( true == tsGuardSignal.is_playing() ) {
        // jeśli gada, to nie jedziemy
        return false;
    }
    bool OK = true;
    if( ( iDrivigFlags & moveDoorOpened )
     && ( VelDesired > 0.0 ) ) { // to prevent door shuffle on stop
        // zamykanie drzwi - tutaj wykonuje tylko AI (zmienia fActionTime)
        Doors( false );
    }
    if( fActionTime < 0.0 ) {
        // gdy jest nakaz poczekać z jazdą, to nie ruszać
        return false;
    }
    if( true == mvOccupied->DepartureSignal ) {
        // shut off departure warning
        mvOccupied->signal_departure( false );
    }
    if (mvControlling->SlippingWheels)
        return false; // jak poślizg, to nie przyspieszamy
    switch (mvOccupied->EngineType)
    {
    case TEngineType::None: // McZapkie-041003: wagon sterowniczy
        if (mvControlling->MainCtrlPosNo > 0) // jeśli ma czym kręcić
            iDrivigFlags |= moveIncSpeed; // ustawienie flagi jazdy
        return false;
    case TEngineType::ElectricSeriesMotor:
        if (mvControlling->EnginePowerSource.SourceType == TPowerSource::CurrentCollector) // jeśli pantografujący
        {
            if (fOverhead2 >= 0.0) // a jazda bezprądowa ustawiana eventami (albo opuszczenie)
                return false; // to nici z ruszania
            if (iOverheadZero) // jazda bezprądowa z poziomu toru ustawia bity
                return false; // to nici z ruszania
        }
        if (!mvControlling->FuseFlag) //&&mvControlling->StLinFlag) //yBARC
            if ((mvControlling->MainCtrlPos == 0) ||
                (mvControlling->StLinFlag)) // youBy polecił dodać 2012-09-08 v367
                // na pozycji 0 przejdzie, a na pozostałych będzie czekać, aż się załączą liniowe (zgaśnie DelayCtrlFlag)
				if (Ready || (iDrivigFlags & movePress)) {
                    // use series mode:
                    // if high threshold is set for motor overload relay,
                    // if the power station is heavily burdened,
                    // if it generates enough traction force
                    // to build up speed to 30/40 km/h for passenger/cargo train (10 km/h less if going uphill)
                    auto const sufficienttractionforce { std::abs( mvControlling->Ft ) > ( IsHeavyCargoTrain ? 125 : 100 ) * 1000.0 };
                    auto const seriesmodefieldshunting { ( mvControlling->ScndCtrlPos > 0 ) && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn == 1 ) };
                    auto const parallelmodefieldshunting { ( mvControlling->ScndCtrlPos > 0 ) && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) };
                    auto const useseriesmodevoltage {
                        interpolate(
                            mvControlling->EnginePowerSource.CollectorParameters.MinV,
                            mvControlling->EnginePowerSource.CollectorParameters.MaxV,
                            ( IsHeavyCargoTrain ? 0.35 : 0.40 ) ) };
                    auto const useseriesmode = (
                        ( mvControlling->Imax > mvControlling->ImaxLo )
                     || ( fVoltage < useseriesmodevoltage )
                     || ( ( true == sufficienttractionforce )
                       && ( mvOccupied->Vel <= ( IsCargoTrain ? 35 : 25 ) + ( seriesmodefieldshunting ? 5 : 0 ) - ( ( fAccGravity < -0.025 ) ? 10 : 0 ) ) ) );
                    // when not in series mode use the first available parallel mode configuration until 50/60 km/h for passenger/cargo train
                    // (if there's only one parallel mode configuration it'll be used regardless of current speed)
                    auto const usefieldshunting = (
                        ( mvControlling->StLinFlag )
                     && ( mvControlling->RList[ mvControlling->MainCtrlPos ].R < 0.01 )
                     && ( useseriesmode ?
                            mvControlling->RList[ mvControlling->MainCtrlPos ].Bn == 1 :
                            ( ( true == sufficienttractionforce )
                           && ( mvOccupied->Vel <= ( IsCargoTrain ? 55 : 45 ) + ( parallelmodefieldshunting ? 5 : 0 ) ) ?
                                mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 :
                                mvControlling->MainCtrlPos == mvControlling->MainCtrlPosNo ) ) );

					double Vs = 99999;
                    if( usefieldshunting ?
                        ( mvControlling->ScndCtrlPos < mvControlling->ScndCtrlPosNo ) :
                        ( mvControlling->MainCtrlPos < mvControlling->MainCtrlPosNo ) ) {
                        Vs = ESMVelocity( !usefieldshunting );
                    }

                    if( ( std::abs( mvControlling->Im ) < ( fReady < 0.4 ? mvControlling->Imin : mvControlling->IminLo ) )
                     || ( mvControlling->Vel > Vs ) ) {
                        // Ra: wywalał nadmiarowy, bo Im może być ujemne; jak nie odhamowany, to nie przesadzać z prądem
                        if( usefieldshunting ) {
                            // to dać bocznik
                            // engage the shuntfield only if there's sufficient power margin to draw from
                            auto const sufficientpowermargin { fVoltage - useseriesmodevoltage > ( IsHeavyCargoTrain ? 100.0 : 75.0 ) };

                            OK = (
                                sufficientpowermargin ?
                                    mvControlling->IncScndCtrl( 1 ) :
                                    true );
                        }
                        else {
                            // jeśli ustawiony bocznik to bocznik na zero po chamsku
                            if( mvControlling->ScndCtrlPos ) {
                                mvControlling->DecScndCtrl( 2 );
                            }
                            // kręcimy nastawnik jazdy
                            // don't draw too much power;
                            // keep from dropping into series mode when entering/using parallel mode, and from shutting down in the series mode
                            auto const sufficientpowermargin {
                                fVoltage - (
                                    mvControlling->RList[ std::min( mvControlling->MainCtrlPos + 1, mvControlling->MainCtrlPosNo ) ].Bn == 1 ? 
                                        mvControlling->EnginePowerSource.CollectorParameters.MinV :
                                        useseriesmodevoltage )
                                > ( IsHeavyCargoTrain ? 80.0 : 60.0 ) };

                            OK = (
                                ( sufficientpowermargin && ( false == mvControlling->DelayCtrlFlag ) ) ?
                                    mvControlling->IncMainCtrl( 1 ) :
                                    true );
                            // czekaj na 1 pozycji, zanim się nie włączą liniowe
                            if( true == mvControlling->StLinFlag ) {
                                iDrivigFlags |= moveIncSpeed;
                            }
                            else {
                                iDrivigFlags &= ~moveIncSpeed;
                            }

                            if( ( mvControlling->Im == 0 )
                             && ( mvControlling->MainCtrlPos > 2 ) ) {
                                // brak prądu na dalszych pozycjach
                                // nie załączona lokomotywa albo wywalił nadmiarowy
                                Need_TryAgain = true;
                            }
                        }
					}
                    else {
                        OK = false;
                    }
				}
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case TEngineType::Dumb:
    case TEngineType::DieselElectric:
        if (!mvControlling->FuseFlag)
            if (Ready || (iDrivigFlags & movePress)) //{(BrakePress<=0.01*MaxBrakePress)}
            {
                OK = mvControlling->IncMainCtrl(1);
                if (!OK)
                    OK = mvControlling->IncScndCtrl(1);
            }
        break;
    case TEngineType::ElectricInductionMotor:
        if (!mvControlling->FuseFlag)
			if (Ready || (iDrivigFlags & movePress) || (mvOccupied->ShuntMode)) //{(BrakePress<=0.01*MaxBrakePress)}
            {
                OK = mvControlling->IncMainCtrl(std::max(1,mvOccupied->MainCtrlPosNo/10));
                // cruise control
                auto const SpeedCntrlVel { (
                    ( ActualProximityDist > std::max( 50.0, fMaxProximityDist ) ) ?
                        VelDesired :
                        min_speed( VelDesired, VelNext ) ) };
				SpeedCntrl(SpeedCntrlVel);
            }
        break;
    case TEngineType::WheelsDriven:
        if (!mvControlling->CabNo)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) > 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case TEngineType::DieselEngine:
        if (mvControlling->ShuntModeAllow)
        { // dla 2Ls150 można zmienić tryb pracy, jeśli jest w liniowym i nie daje rady (wymaga zerowania kierunku)
            // mvControlling->ShuntMode=(OrderList[OrderPos]&Shunt)||(fMass>224000.0);
        }
        if( true == Ready ) {
            if( ( mvControlling->Vel > mvControlling->dizel_minVelfullengage )
             && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn > 0 ) ) {
                OK = mvControlling->IncMainCtrl( 1 );
            }
            if( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn == 0 ) {
                OK = mvControlling->IncMainCtrl( 1 );
            }
        }
        if( false == mvControlling->Mains ) {
            mvControlling->MainSwitch( true );
            mvControlling->ConverterSwitch( true );
            mvControlling->CompressorSwitch( true );
        }
        break;
    }
    return OK;
}

bool TController::DecSpeed(bool force)
{ // zmniejszenie prędkości (ale nie hamowanie)
    bool OK = false; // domyślnie false, aby wyszło z pętli while
    switch (mvOccupied->EngineType)
    {
    case TEngineType::None: // McZapkie-041003: wagon sterowniczy
        iDrivigFlags &= ~moveIncSpeed; // usunięcie flagi jazdy
        if (force) // przy aktywacji kabiny jest potrzeba natychmiastowego wyzerowania
            if (mvControlling->MainCtrlPosNo > 0) // McZapkie-041003: wagon sterowniczy, np. EZT
                mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        return false;
    case TEngineType::ElectricSeriesMotor:
        OK = mvControlling->DecScndCtrl(2); // najpierw bocznik na zero
        if (!OK)
            OK = mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case TEngineType::Dumb:
    case TEngineType::DieselElectric:
        OK = mvControlling->DecScndCtrl(2);
        if (!OK)
            OK = mvControlling->DecMainCtrl(2 + (mvControlling->MainCtrlPos / 2));
        break;
	case TEngineType::ElectricInductionMotor:
		OK = mvControlling->DecMainCtrl(1);
		break;
    case TEngineType::WheelsDriven:
        if (!mvControlling->CabNo)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) < 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case TEngineType::DieselEngine:
        if ((mvControlling->Vel > mvControlling->dizel_minVelfullengage))
        {
            if (mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0)
                OK = mvControlling->DecMainCtrl(1);
        }
        else
            while ((mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0) &&
                   (mvControlling->MainCtrlPos > 1))
                OK = mvControlling->DecMainCtrl(1);
        if (force) // przy aktywacji kabiny jest potrzeba natychmiastowego wyzerowania
            OK = mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        break;
    }
    return OK;
};

void TController::SpeedSet()
{ // Ra: regulacja prędkości, wykonywana w każdym przebłysku świadomości AI
    // ma dokręcać do bezoporowych i zdejmować pozycje w przypadku przekroczenia prądu
    switch (mvOccupied->EngineType)
    {
    case TEngineType::None: // McZapkie-041003: wagon sterowniczy
        if (mvControlling->MainCtrlPosNo > 0)
        { // jeśli ma czym kręcić
            // TODO: sprawdzanie innego czlonu //if (!FuseFlagCheck())
            if ((AccDesired < fAccGravity - 0.05) ||
                (mvOccupied->Vel > VelDesired)) // jeśli nie ma przyspieszać
                mvControlling->DecMainCtrl(2); // na zero
            else if (fActionTime >= 0.0)
            { // jak już można coś poruszać, przetok rozłączać od razu
                if (iDrivigFlags & moveIncSpeed)
                { // jak ma jechać
                    if (fReady < 0.4) // 0.05*Controlling->MaxBrakePress)
                    { // jak jest odhamowany
                        if (mvOccupied->ActiveDir > 0)
                            mvOccupied->DirectionForward(); //żeby EN57 jechały na drugiej nastawie
                        {
                            if (mvControlling->MainCtrlPos &&
                                !mvControlling->StLinFlag) // jak niby jedzie, ale ma rozłączone liniowe
                                mvControlling->DecMainCtrl(2); // to na zero i czekać na przewalenie kułakowego
                            else
                                switch (mvControlling->MainCtrlPos)
                                { // ruch nastawnika uzależniony jest od aktualnie ustawionej
                                // pozycji
                                case 0:
                                    if (mvControlling->MainCtrlActualPos) // jeśli kułakowy nie jest
                                        // wyzerowany
                                        break; // to czekać na wyzerowanie
                                    mvControlling->IncMainCtrl(1); // przetok; bez "break", bo nie
                                // ma czekania na 1. pozycji
                                case 1:
                                    if (VelDesired >= 20)
                                        mvControlling->IncMainCtrl(1); // szeregowa
                                case 2:
                                    if (VelDesired >= 50)
                                        mvControlling->IncMainCtrl(1); // równoległa
                                case 3:
                                    if (VelDesired >= 80)
                                        mvControlling->IncMainCtrl(1); // bocznik 1
                                case 4:
                                    if (VelDesired >= 90)
                                        mvControlling->IncMainCtrl(1); // bocznik 2
                                case 5:
                                    if (VelDesired >= 100)
                                        mvControlling->IncMainCtrl(1); // bocznik 3
                                }
                            if (mvControlling->MainCtrlPos) // jak załączył pozycję
                            {
                                fActionTime = -5.0; // niech trochę potrzyma
                                mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                            }
                        }
                    }
                }
                else
                {
                    while (mvControlling->MainCtrlPos)
                        mvControlling->DecMainCtrl(1); // na zero
                    fActionTime = -5.0; // niech trochę potrzyma
                    mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                }
            }
        }
        break;
    case TEngineType::ElectricSeriesMotor:
        if( ( false == mvControlling->StLinFlag )
         && ( false == mvControlling->DelayCtrlFlag ) ) {
            // styczniki liniowe rozłączone    yBARC
            while( DecSpeed() )
                ; // zerowanie napędu
        }
        else if (Ready || (iDrivigFlags & movePress)) // o ile może jechać
            if (fAccGravity < -0.10) // i jedzie pod górę większą niż 10 promil
            { // procedura wjeżdżania na ekstremalne wzniesienia
                if (fabs(mvControlling->Im) > 0.85 * mvControlling->Imax) // a prąd jest większy niż 85% nadmiarowego
                    if (mvControlling->Imax * mvControlling->Voltage / (fMass * fAccGravity) < -2.8) // a na niskim się za szybko nie pojedzie
                    { // włączenie wysokiego rozruchu;
                        // (I*U)[A*V=W=kg*m*m/sss]/(m[kg]*a[m/ss])=v[m/s]; 2.8m/ss=10km/h
                        if (mvControlling->RList[mvControlling->MainCtrlPos].Bn > 1)
                        { // jeśli jedzie na równoległym, to zbijamy do szeregowego, aby włączyć
                            // wysoki rozruch
                            if (mvControlling->ScndCtrlPos > 0) // jeżeli jest bocznik
                                mvControlling->DecScndCtrl(2); // wyłączyć bocznik, bo może blokować skręcenie NJ
                            do // skręcanie do bezoporowej na szeregowym
                                mvControlling->DecMainCtrl(1); // kręcimy nastawnik jazdy o 1 wstecz
                            while (mvControlling->MainCtrlPos ?
                                       mvControlling->RList[mvControlling->MainCtrlPos].Bn > 1 :
                                       false); // oporowa zapętla
                        }
                        if (mvControlling->Imax < mvControlling->ImaxHi) // jeśli da się na wysokim
                            mvControlling->CurrentSwitch(true); // rozruch wysoki (za to może się ślizgać)
                        if (ReactionTime > 0.1)
                            ReactionTime = 0.1; // orientuj się szybciej
                    } // if (Im>Imin)
                // NOTE: this step is likely to conflict with directive to operate sandbox based on the state of slipping wheels
                // TODO: gather all sandbox operating logic in one place
                if( fabs( mvControlling->Im ) > 0.75 * mvControlling->ImaxHi ) {
                    // jeśli prąd jest duży
                    mvControlling->Sandbox( true ); // piaskujemy tory, coby się nie ślizgać
                }
                else {
                    // otherwise we switch the sander off, if it's active
                    if( mvControlling->SandDose ) {
                        mvControlling->Sandbox( false );
                    }
                }
                if ((fabs(mvControlling->Im) > 0.96 * mvControlling->Imax) ||
                    mvControlling->SlippingWheels) // jeśli prąd jest duży (można 690 na 750)
                    if (mvControlling->ScndCtrlPos > 0) // jeżeli jest bocznik
                        mvControlling->DecScndCtrl(2); // zmniejszyć bocznik
                    else
                        mvControlling->DecMainCtrl(1); // kręcimy nastawnik jazdy o 1 wstecz
            }
            else // gdy nie jedzie ambitnie pod górę
            { // sprawdzenie, czy rozruch wysoki jest potrzebny
                if (mvControlling->Imax > mvControlling->ImaxLo)
                    if (mvOccupied->Vel >= 30.0) // jak się rozpędził
                        if (fAccGravity > -0.02) // a i pochylenie mnijsze niż 2‰
                            mvControlling->CurrentSwitch(false); // rozruch wysoki wyłącz
            }
        break;
    case TEngineType::Dumb:
    case TEngineType::DieselElectric:
    case TEngineType::ElectricInductionMotor:
        break;
    case TEngineType::DieselEngine:
        // Ra 2014-06: "automatyczna" skrzynia biegów...
        if (!mvControlling->MotorParam[mvControlling->ScndCtrlPos].AutoSwitch) // gdy biegi ręczne
            if ((mvControlling->ShuntMode ? mvControlling->AnPos : 1.0) * mvControlling->Vel >
                0.6 * mvControlling->MotorParam[mvControlling->ScndCtrlPos].mfi)
            // if (mvControlling->enrot>0.95*mvControlling->dizel_nMmax) //youBy: jeśli obroty >
            // 0,95 nmax, wrzuć wyższy bieg - Ra: to nie działa
            { // jak prędkość większa niż 0.6 maksymalnej na danym biegu, wrzucić wyższy
                mvControlling->DecMainCtrl(2);
                if (mvControlling->IncScndCtrl(1))
                    if (mvControlling->MotorParam[mvControlling->ScndCtrlPos].mIsat ==
                        0.0) // jeśli bieg jałowy
                        mvControlling->IncScndCtrl(1); // to kolejny
            }
            else if ((mvControlling->ShuntMode ? mvControlling->AnPos : 1.0) * mvControlling->Vel <
                     mvControlling->MotorParam[mvControlling->ScndCtrlPos].fi)
            { // jak prędkość mniejsza niż minimalna na danym biegu, wrzucić niższy
                mvControlling->DecMainCtrl(2);
                mvControlling->DecScndCtrl(1);
                if (mvControlling->MotorParam[mvControlling->ScndCtrlPos].mIsat ==
                    0.0) // jeśli bieg jałowy
                    if (mvControlling->ScndCtrlPos) // a jeszcze zera nie osiągnięto
                        mvControlling->DecScndCtrl(1); // to kolejny wcześniejszy
                    else
                        mvControlling->IncScndCtrl(1); // a jak zeszło na zero, to powrót
            }
        break;
    }
};

void TController::SpeedCntrl(double DesiredSpeed)
{
	if (mvControlling->ScndCtrlPosNo == 1)
	{
		mvControlling->IncScndCtrl(1);
		mvControlling->RunCommand("SpeedCntrl", DesiredSpeed, mvControlling->CabNo);
	}
	else if (mvControlling->ScndCtrlPosNo > 1)
	{
		int DesiredPos = 1 + mvControlling->ScndCtrlPosNo * ((DesiredSpeed - 1.0) / mvControlling->Vmax);
        while( ( mvControlling->ScndCtrlPos > DesiredPos ) && ( true == mvControlling->DecScndCtrl( 1 ) ) ) { ; } // all work is done in the condition loop
        while( ( mvControlling->ScndCtrlPos < DesiredPos ) && ( true == mvControlling->IncScndCtrl( 1 ) ) ) { ; } // all work is done in the condition loop
	}
};

// otwieranie/zamykanie drzwi w składzie albo (tylko AI) EZT
void TController::Doors( bool const Open, int const Side ) {

    if( true == Open ) {
        // otwieranie drzwi
        // otwieranie drzwi w składach wagonowych - docelowo wysyłać komendę zezwolenia na otwarcie drzwi
        // tu będzie jeszcze długość peronu zaokrąglona do 10m (20m bezpieczniej, bo nie modyfikuje bitu 1)
        auto *vehicle = pVehicles[0]; // pojazd na czole składu
        while( vehicle != nullptr ) {
            // otwieranie drzwi w pojazdach - flaga zezwolenia była by lepsza
            if( vehicle->MoverParameters->DoorOpenCtrl != control_t::passenger ) {
                // if the door are controlled by the driver, we let the user operate them...
                if( true == AIControllFlag ) {
                    // ...unless this user is an ai
                    // Side=platform side (1:left, 2:right, 3:both)
                    // jeśli jedzie do tyłu, to drzwi otwiera odwrotnie
                    auto const lewe = ( vehicle->DirectionGet() > 0 ) ? 1 : 2;
                    auto const prawe = 3 - lewe;
                    if( Side & lewe )
                        vehicle->MoverParameters->DoorLeft( true, range_t::local );
                    if( Side & prawe )
                        vehicle->MoverParameters->DoorRight( true, range_t::local );
                }
            }
            // pojazd podłączony z tyłu (patrząc od czoła)
            vehicle = vehicle->Next();
        }
    }
    else {
        // zamykanie
        if( false == doors_open() ) {
            // the doors are already closed, we can skip all hard work
            iDrivigFlags &= ~moveDoorOpened;
        }

        if( AIControllFlag ) {
            if( ( true == mvOccupied->DoorClosureWarning )
             && ( false == mvOccupied->DepartureSignal )
             && ( true == TestFlag( iDrivigFlags, moveDoorOpened ) ) ) {
                mvOccupied->signal_departure( true ); // załącenie bzyczka
                fActionTime = -1.5 - 0.1 * Random( 10 ); // 1.5-2.5 second wait
            }
        }

        if( ( true == TestFlag( iDrivigFlags, moveDoorOpened ) )
         && ( ( fActionTime > -0.5 )
           || ( false == AIControllFlag ) ) ) {
            // ai doesn't close the door until it's free to depart, but human driver has free reign to do stupid things
            auto *vehicle = pVehicles[ 0 ]; // pojazd na czole składu
            while( vehicle != nullptr ) {
                // zamykanie drzwi w pojazdach - flaga zezwolenia była by lepsza
                if( vehicle->MoverParameters->DoorCloseCtrl != control_t::autonomous ) {
                    vehicle->MoverParameters->DoorLeft( false, range_t::local ); // w lokomotywie można by nie zamykać...
                    vehicle->MoverParameters->DoorRight( false, range_t::local );
                }
                vehicle = vehicle->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
            }
            fActionTime = -2.0 - 0.1 * Random( 15 ); // 2.0-3.5 sec wait, +potentially 0.5 remaining
            iDrivigFlags &= ~moveDoorOpened; // zostały zamknięte - nie wykonywać drugi raz

            if( Random( 100 ) < Random( 100 ) ) {
                // potentially turn off the warning before actual departure
                // TBD, TODO: dedicated buzzer duration counter
                mvOccupied->signal_departure( false );
            }

        }
    }
}

// returns true if any vehicle in the consist has open doors
bool
TController::doors_open() const {

    auto *vehicle = pVehicles[ 0 ]; // pojazd na czole składu
    while( vehicle != nullptr ) {
        if( ( vehicle->MoverParameters->DoorRightOpened == true )
         || ( vehicle->MoverParameters->DoorLeftOpened == true ) ) {
            // any open door is enough
            return true;
        }
        vehicle = vehicle->Next();
    }
    // if we're still here there's nothing open
    return false;
}

void TController::RecognizeCommand()
{ // odczytuje i wykonuje komendę przekazaną lokomotywie
    TCommand *c = &mvOccupied->CommandIn;
    PutCommand(c->Command, c->Value1, c->Value2, c->Location, stopComm);
    c->Command = ""; // usunięcie obsłużonej komendy
}

void TController::PutCommand(std::string NewCommand, double NewValue1, double NewValue2, const TLocation &NewLocation, TStopReason reason)
{ // wysłanie komendy przez event PutValues, jak pojazd ma obsadę, to wysyła tutaj, a nie do pojazdu bezpośrednio
    // zamiana na współrzędne scenerii
    glm::dvec3 sl { -NewLocation.X, NewLocation.Z, NewLocation.Y };
    if (!PutCommand(NewCommand, NewValue1, NewValue2, &sl, reason))
        mvOccupied->PutCommand(NewCommand, NewValue1, NewValue2, NewLocation);
}

bool TController::PutCommand( std::string NewCommand, double NewValue1, double NewValue2, glm::dvec3 const *NewLocation, TStopReason reason )
{ // analiza komendy
    if (NewCommand == "CabSignal")
    { // SHP wyzwalane jest przez człon z obsadą, ale obsługiwane przez silnikowy
        // nie jest to najlepiej zrobione, ale bez symulacji obwodów lepiej nie będzie
        // Ra 2014-04: jednak przeniosłem do rozrządczego
        mvOccupied->PutCommand(NewCommand, NewValue1, NewValue2, mvOccupied->Loc);
        mvOccupied->RunInternalCommand(); // rozpoznaj komende bo lokomotywa jej nie rozpoznaje
        return true; // załatwione
    }

    if (NewCommand == "Overhead")
    { // informacja o stanie sieci trakcyjnej
        fOverhead1 =
            NewValue1; // informacja o napięciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
        fOverhead2 = NewValue2; // informacja o sposobie jazdy (-1=normalnie, 0=bez prądu, >0=z
        // opuszczonym i ograniczeniem prędkości)
        return true; // załatwione
    }

    if (NewCommand == "Emergency_brake") // wymuszenie zatrzymania, niezależnie kto prowadzi
    { // Ra: no nadal nie jest zbyt pięknie
        SetVelocity(0, 0, reason);
        mvOccupied->PutCommand("Emergency_brake", 1.0, 1.0, mvOccupied->Loc);
        return true; // załatwione
    }

    if (NewCommand.compare(0, 10, "Timetable:") == 0)
    { // przypisanie nowego rozkładu jazdy, również prowadzonemu przez użytkownika
        NewCommand.erase(0, 10); // zostanie nazwa pliku z rozkładem
#if LOGSTOPS
        WriteLog("New timetable for " + pVehicle->asName + ": " + NewCommand); // informacja
#endif
        if (!TrainParams)
            TrainParams = new TTrainParameters(NewCommand); // rozkład jazdy
        else
            TrainParams->NewName(NewCommand); // czyści tabelkę przystanków
        tsGuardSignal = sound_source { sound_placement::internal, 2 * EU07_SOUND_CABCONTROLSCUTOFFRANGE }; // wywalenie kierownika
        if (NewCommand != "none")
        {
            if (!TrainParams->LoadTTfile(
                    std::string(Global.asCurrentSceneryPath.c_str()), floor(NewValue2 + 0.5),
                    NewValue1)) // pierwszy parametr to przesunięcie rozkładu w czasie
            {
                if (ConversionError == -8)
                    ErrorLog("Missed timetable: " + NewCommand);
                WriteLog("Cannot load timetable file " + NewCommand + "\r\nError " +
                         std::to_string(ConversionError) + " in position " + std::to_string(TrainParams->StationCount));
                NewCommand = ""; // puste, dla wymiennej tekstury
            }
            else
            { // inicjacja pierwszego przystanku i pobranie jego nazwy
                // HACK: clear the potentially set door state flag to ensure door get opened if applicable
                iDrivigFlags &= ~( moveDoorOpened );

                TrainParams->UpdateMTable( simulation::Time, TrainParams->NextStationName );
                TrainParams->StationIndexInc(); // przejście do następnej
                iStationStart = TrainParams->StationIndex;
                asNextStop = TrainParams->NextStop();
                iDrivigFlags |= movePrimary; // skoro dostał rozkład, to jest teraz głównym
//                NewCommand = Global.asCurrentSceneryPath + NewCommand;
                auto lookup =
                    FileExists(
                        { Global.asCurrentSceneryPath + NewCommand, szSoundPath + NewCommand },
                        { ".ogg", ".flac", ".wav" } );
                if( false == lookup.first.empty() ) {
                    //  wczytanie dźwięku odjazdu podawanego bezpośrenido
                    tsGuardSignal = sound_source( sound_placement::external, 75.f ).deserialize( lookup.first + lookup.second, sound_type::single );
                    iGuardRadio = 0; // nie przez radio
                }
                else {
                    NewCommand += "radio";
                    auto lookup =
                        FileExists(
                            { Global.asCurrentSceneryPath + NewCommand, szSoundPath + NewCommand },
                            { ".ogg", ".flac", ".wav" } );
                    if( false == lookup.first.empty() ) {
                        //  wczytanie dźwięku odjazdu w wersji radiowej (słychać tylko w kabinie)
                        tsGuardSignal = sound_source( sound_placement::internal, 2 * EU07_SOUND_CABCONTROLSCUTOFFRANGE ).deserialize( lookup.first + lookup.second, sound_type::single );
                        iGuardRadio = iRadioChannel;
                    }
                }
                NewCommand = TrainParams->Relation2; // relacja docelowa z rozkładu
            }
            // jeszcze poustawiać tekstury na wyświetlaczach
            TDynamicObject *p = pVehicles[0];
            while (p)
            {
                p->DestinationSet(NewCommand, TrainParams->TrainName); // relacja docelowa
                p = p->Next(); // pojazd podłączony od tyłu (licząc od czoła)
            }
        }
        if (NewLocation) // jeśli podane współrzędne eventu/komórki ustawiającej rozkład (trainset
        // nie podaje)
        {
            auto v = *NewLocation - pVehicle->GetPosition(); // wektor do punktu sterującego
            auto d = pVehicle->VectorFront(); // wektor wskazujący przód
            iDirectionOrder = ((v.x * d.x + v.z * d.z) * NewValue1 > 0) ?
                                  1 :
                                  -1; // do przodu, gdy iloczyn skalarny i prędkość dodatnie
            /*
              if (NewValue1!=0.0) //jeśli ma jechać
               if (iDirectionOrder==0) //a kierunek nie był określony (normalnie określany przez
              reardriver/headdriver)
                iDirectionOrder=NewValue1>0?1:-1; //ustalenie kierunku jazdy względem sprzęgów
               else
                if (NewValue1<0) //dla ujemnej prędkości
                 iDirectionOrder=-iDirectionOrder; //ma jechać w drugą stronę
            */
            if (AIControllFlag) // jeśli prowadzi komputer
                Activation(); // umieszczenie obsługi we właściwym członie, ustawienie nawrotnika w
            // przód
        }
        /*
          else
           if (!iDirectionOrder)
          {//jeśli nie ma kierunku, trzeba ustalić
           if (mvOccupied->V!=0.0)
            iDirectionOrder=mvOccupied->V>0?1:-1;
           else
            iDirectionOrder=mvOccupied->ActiveCab;
           if (!iDirectionOrder) iDirectionOrder=1;
          }
        */
        // jeśli wysyłane z Trainset, to wszystko jest już odpowiednio ustawione
        // if (!NewLocation) //jeśli wysyłane z Trainset
        // if (mvOccupied->CabNo*mvOccupied->V*NewValue1<0) //jeśli zadana prędkość niezgodna z
        // aktualnym kierunkiem jazdy
        //  DirectionForward(false); //jedziemy do tyłu (nawrotnik do tyłu)
        // CheckVehicles(); //sprawdzenie składu, AI zapali światła
        TableClear(); // wyczyszczenie tabelki prędkości, bo na nowo trzeba określić kierunek i
        // sprawdzić przystanki
        OrdersInit(fabs(NewValue1)); // ustalenie tabelki komend wg rozkładu oraz prędkości początkowej
        // if (NewValue1!=0.0) if (!AIControllFlag) DirectionForward(NewValue1>0.0); //ustawienie
        // nawrotnika użytkownikowi (propaguje się do członów)
        // if (NewValue1>0)
        // TrainNumber=floor(NewValue1); //i co potem ???
        return true; // załatwione
    }

    if (NewCommand == "SetVelocity")
    {
        if (NewLocation)
            vCommandLocation = *NewLocation;
        if ((NewValue1 != 0.0) && (OrderList[OrderPos] != Obey_train))
        { // o ile jazda
            if( iEngineActive == 0 ) {
                // trzeba odpalić silnik najpierw, światła ustawi
                OrderNext( Prepare_engine );
            }
            OrderNext(Obey_train); // to uruchomić jazdę pociągową (od razu albo po odpaleniu silnika
            OrderCheck(); // jeśli jazda pociągowa teraz, to wykonać niezbędne operacje
        }
        if (NewValue1 != 0.0) // jeśli jechać
            iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        else
            iDrivigFlags |= moveStopHere; // stać do momentu podania komendy jazdy
        SetVelocity(NewValue1, NewValue2, reason); // bylo: nic nie rob bo SetVelocity zewnetrznie
        // jest wywolywane przez dynobj.cpp
        return true;
    }

    if (NewCommand == "SetProximityVelocity")
    {
        /*
          if (SetProximityVelocity(NewValue1,NewValue2))
           if (NewLocation)
            vCommandLocation=*NewLocation;
        */
        return true;
    }

    if (NewCommand == "ShuntVelocity")
    { // uruchomienie jazdy manewrowej bądź zmiana prędkości
        if (NewLocation)
            vCommandLocation = *NewLocation;
        // if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0))
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        OrderNext(Shunt); // zamieniamy w aktualnej pozycji, albo dodajey za odpaleniem silnika
        if (NewValue1 != 0.0)
        {
            // if (iVehicleCount>=0) WriteLog("Skasowano ilosć wagonów w ShuntVelocity!");
            iVehicleCount = -2; // wartość neutralna
            CheckVehicles(); // zabrać to do OrderCheck()
        }
        // dla prędkości ujemnej przestawić nawrotnik do tyłu? ale -1=brak ograniczenia !!!!
        // if (iDrivigFlags&movePress) WriteLog("Skasowano docisk w ShuntVelocity!");
        iDrivigFlags &= ~movePress; // bez dociskania
        SetVelocity(NewValue1, NewValue2, reason);
        if (NewValue1 != 0.0)
            iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        else
            iDrivigFlags |= moveStopHere; // ma stać w miejscu
        if (fabs(NewValue1) > 2.0) // o ile wartość jest sensowna (-1 nie jest konkretną wartością)
            fShuntVelocity = fabs(NewValue1); // zapamiętanie obowiązującej prędkości dla manewrów

        return true;
    }

    if (NewCommand == "Wait_for_orders")
    { // oczekiwanie; NewValue1 - czas oczekiwania, -1 = na inną komendę
        if (NewValue1 > 0.0 ? NewValue1 > fStopTime : false)
            fStopTime = NewValue1; // Ra: włączenie czekania bez zmiany komendy
        else
            OrderList[OrderPos] = Wait_for_orders; // czekanie na komendę (albo dać OrderPos=0)

        return true;
    }

    if (NewCommand == "Prepare_engine")
    { // włączenie albo wyłączenie silnika (w szerokim sensie)
        OrdersClear(); // czyszczenie tabelki rozkazów, aby nic dalej nie robił
        if (NewValue1 == 0.0)
            OrderNext(Release_engine); // wyłączyć silnik (przygotować pojazd do jazdy)
        else if (NewValue1 > 0.0)
            OrderNext(Prepare_engine); // odpalić silnik (wyłączyć wszystko, co się da)
        // po załączeniu przejdzie do kolejnej komendy, po wyłączeniu na Wait_for_orders
        return true;
    }

    if (NewCommand == "Change_direction")
    {
        TOrders o = OrderList[OrderPos]; // co robił przed zmianą kierunku
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        if (NewValue1 == 0.0)
            iDirectionOrder = -iDirection; // zmiana na przeciwny niż obecny
        else if (NewLocation) // jeśli podane współrzędne eventu/komórki ustawiającej rozkład
        // (trainset nie podaje)
        {
            auto v = *NewLocation - pVehicle->GetPosition(); // wektor do punktu sterującego
            auto d = pVehicle->VectorFront(); // wektor wskazujący przód
            iDirectionOrder = ((v.x * d.x + v.z * d.z) * NewValue1 > 0) ?
                                  1 :
                                  -1; // do przodu, gdy iloczyn skalarny i prędkość dodatnie
            // iDirectionOrder=1; else if (NewValue1<0.0) iDirectionOrder=-1;
        }
        if (iDirectionOrder != iDirection)
            OrderNext(Change_direction); // zadanie komendy do wykonania
        if (o >= Shunt) // jeśli jazda manewrowa albo pociągowa
            OrderNext(o); // to samo robić po zmianie
        else if (!o) // jeśli wcześniej było czekanie
            OrderNext(Shunt); // to dalej jazda manewrowa
        if (mvOccupied->Vel >= 1.0) // jeśli jedzie
            iDrivigFlags &= ~moveStartHorn; // to bez trąbienia po ruszeniu z zatrzymania
        // Change_direction wykona się samo i następnie przejdzie do kolejnej komendy
        return true;
    }

    if (NewCommand == "Obey_train")
    {
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        OrderNext(Obey_train);
        // if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
        OrderCheck(); // jeśli jazda pociągowa teraz, to wykonać niezbędne operacje

        return true;
    }

    if (NewCommand == "Shunt")
    { // NewValue1 - ilość wagonów (-1=wszystkie); NewValue2: 0=odczep, 1..63=dołącz, -1=bez zmian
        //-3,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1), zmienić kierunek i czekać w
        // trybie pociągowym
        //-2,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1), zmienić kierunek i czekać
        //-2, y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i czekać
        //-1,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i jechać w powrotną stronę
        //-1, y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i jechać dalej
        //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagonów nie ma sensu)
        // 0, 0 - odczepienie lokomotywy
        // 1,-y - podłączyć się do składu (sprzęgiem y>=1), a następnie odczepić i zabrać (x)
        // wagonów
        // 1, 0 - odczepienie lokomotywy z jednym wagonem
        iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        if (NewValue2 != 0) // jeśli podany jest sprzęg
        {
            iCoupler = floor(fabs(NewValue2)); // jakim sprzęgiem
            OrderNext(Connect); // najpierw połącz pojazdy
            if (NewValue1 >= 0.0) // jeśli ilość wagonów inna niż wszystkie
            { // to po podpięciu należy się odczepić
                iDirectionOrder = -iDirection; // zmiana na ciągnięcie
                OrderPush(Change_direction); // najpierw zmień kierunek, bo odczepiamy z tyłu
                OrderPush(Disconnect); // a odczep już po zmianie kierunku
            }
            else if (NewValue2 < 0.0) // jeśli wszystkie, a sprzęg ujemny, to tylko zmiana kierunku
            // po podczepieniu
            { // np. Shunt -1 -3
                iDirectionOrder = -iDirection; // jak się podczepi, to jazda w przeciwną stronę
                OrderNext(Change_direction);
            }
            WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać, chyba że już jedzie
        }
        else // if (NewValue2==0.0) //zerowy sprzęg
            if (NewValue1 >= 0.0) // jeśli ilość wagonów inna niż wszystkie
        { // będzie odczepianie, ale jeśli wagony są z przodu, to trzeba najpierw zmienić kierunek
            if ((mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 1 : 0].CouplingFlag ==
                 0) ? // z tyłu nic
                    (mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 0 : 1].CouplingFlag > 0) :
                    false) // a z przodu skład
            {
                iDirectionOrder = -iDirection; // zmiana na ciągnięcie
                OrderNext(Change_direction); // najpierw zmień kierunek (zastąpi Disconnect)
                OrderPush(Disconnect); // a odczep już po zmianie kierunku
            }
            else if (mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 1 : 0].CouplingFlag >
                     0) // z tyłu coś
                OrderNext(Disconnect); // jak ciągnie, to tylko odczep (NewValue1) wagonów
            WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać, chyba że już jedzie
        }
        if (NewValue1 == -1.0)
        {
            iDrivigFlags &= ~moveStopHere; // ma jechać
            WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać
        }
        if (NewValue1 < -1.5) // jeśli -2/-3, czyli czekanie z ruszeniem na sygnał
            iDrivigFlags |= moveStopHere; // nie podjeżdżać do semafora, jeśli droga nie jest wolna
        // (nie dotyczy Connect)
        if (NewValue1 < -2.5) // jeśli
            OrderNext(Obey_train); // to potem jazda pociągowa
        else
            OrderNext(Shunt); // to potem manewruj dalej
        CheckVehicles(); // sprawdzić światła
        // if ((iVehicleCount>=0)&&(NewValue1<0)) WriteLog("Skasowano ilosć wagonów w Shunt!");
        if (NewValue1 != iVehicleCount)
            iVehicleCount = floor(NewValue1); // i co potem ? - trzeba zaprogramowac odczepianie
        /*
          if (NewValue1!=-1.0)
           if (NewValue2!=0.0)

            if (VelDesired==0)
             SetVelocity(20,0); //to niech jedzie
        */
        return true;
    }

    if( NewCommand == "Jump_to_first_order" ) {
        JumpToFirstOrder();
        return true;
    }

    if (NewCommand == "Jump_to_order")
    {
        if( NewValue1 == -1.0 ) {
            JumpToNextOrder();
        }
        else if ((NewValue1 >= 0) && (NewValue1 < maxorders))
        {
            OrderPos = floor(NewValue1);
            if( !OrderPos ) {
                // zgodność wstecz: dopiero pierwsza uruchamia
                OrderPos = 1;
            }
#if LOGORDERS
            WriteLog("--> Jump_to_order");
            OrdersDump();
#endif
        }
        return true;
    }

    if (NewCommand == "Warning_signal")
    {
        if( AIControllFlag ) {
            // poniższa komenda nie jest wykonywana przez użytkownika
            if( NewValue1 > 0 ) {
                fWarningDuration = NewValue1; // czas trąbienia
                mvOccupied->WarningSignal = NewValue2; // horn combination flag
            }
        }
        return true;
    }

    if (NewCommand == "Radio_channel") {
        // wybór kanału radiowego (którego powinien używać AI, ręczny maszynista musi go ustawić sam)
        if (NewValue1 >= 0) {
            // wartości ujemne są zarezerwowane, -1 = nie zmieniać kanału
            iRadioChannel = NewValue1;
            if( iGuardRadio ) {
                // kierownikowi też zmienić
                iGuardRadio = iRadioChannel;
            }
        }
        // NewValue2 może zawierać dodatkowo oczekiwany kod odpowiedzi, np. dla W29 "nawiązać
        // łączność radiową z dyżurnym ruchu odcinkowym"
        return true;
    }

    if( NewCommand == "SetLights" ) {
        // set consist lights pattern hints
        m_lighthints[ side::front ] = static_cast<int>( NewValue1 );
        m_lighthints[ side::rear ] = static_cast<int>( NewValue2 );
        if( true == TestFlag( OrderCurrentGet(), Obey_train ) ) {
            // light hints only apply in the obey_train mode
            CheckVehicles();
        }
        return true;
    }

    return false; // nierozpoznana - wysłać bezpośrednio do pojazdu
};

void TController::PhysicsLog()
{ // zapis logu - na razie tylko wypisanie parametrów
    if (LogFile.is_open())
    {
#if LOGPRESS == 0
/*
<< "Time[s] Velocity[m/s] Acceleration[m/ss] "
<< "Coupler.Dist[m] Coupler.Force[N] TractionForce[kN] FrictionForce[kN] BrakeForce[kN] "
<< "BrakePress[MPa] PipePress[MPa] MotorCurrent[A]    "
<< "MCP SCP BCP LBP DmgFlag Command CVal1 CVal2 "
<< "EngineTemp[Deg] OilTemp[Deg] WaterTemp[Deg] WaterAuxTemp[Deg]"
*/
        LogFile
            << ElapsedTime << " "
            << fabs(11.31 * mvOccupied->WheelDiameter * mvOccupied->nrot) << " "
            << mvControlling->AccS << " "
            << mvOccupied->Couplers[1].Dist << " "
            << mvOccupied->Couplers[1].CForce << " "
            << mvOccupied->Ft << " "
            << mvOccupied->Ff << " "
            << mvOccupied->Fb << " "
            << mvOccupied->BrakePress << " "
            << mvOccupied->PipePress << " "
            << mvControlling->Im << " "
            << int(mvControlling->MainCtrlPos) << " "
            << int(mvControlling->ScndCtrlPos) << " "
            << mvOccupied->fBrakeCtrlPos << " "
            << mvOccupied->LocalBrakePosA << " "
            << int(mvControlling->ActiveDir) << " "
            << ( mvOccupied->CommandIn.Command.empty() ? "none" : mvOccupied->CommandIn.Command.c_str() ) << " "
            << mvOccupied->CommandIn.Value1 << " "
            << mvOccupied->CommandIn.Value2 << " "
            << int(mvControlling->SecuritySystem.Status) << " "
            << int(mvControlling->SlippingWheels) << " "
            << mvControlling->dizel_heat.Ts << " "
            << mvControlling->dizel_heat.To << " "
            << mvControlling->dizel_heat.temperatura1 << " "
            << mvControlling->dizel_heat.temperatura2
            << "\r\n";
#endif
#if LOGPRESS == 1
        LogFile << ElapsedTime << "\t" << fabs(11.31 * mvOccupied->WheelDiameter * mvOccupied->nrot)
                << "\t";
        LogFile << Controlling->AccS << "\t";
        LogFile << mvOccupied->PipePress << "\t" << mvOccupied->CntrlPipePress << "\t"
                << mvOccupied->BrakePress << "\t";
        LogFile << mvOccupied->Volume << "\t" << mvOccupied->Hamulec->GetCRP() << "\n";
#endif
        LogFile.flush();
    }
};

void
TController::UpdateSituation(double dt) {
    // uruchamiać przynajmniej raz na sekundę
    if( ( iDrivigFlags & movePrimary ) == 0 ) { return; } // pasywny nic nie robi

    // update timers
    ElapsedTime += dt;
    WaitingTime += dt;
    fBrakeTime -= dt; // wpisana wartość jest zmniejszana do 0, gdy ujemna należy zmienić nastawę hamulca
    if( mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos( bh_FS ) ) {
        // brake charging timeout starts after charging ends
        BrakeChargingCooldown += dt;
    }
    fStopTime += dt; // zliczanie czasu postoju, nie ruszy dopóki ujemne
    fActionTime += dt; // czas używany przy regulacji prędkości i zamykaniu drzwi
    LastReactionTime += dt;
    LastUpdatedTime += dt;
    if( ( mvOccupied->Vel < 0.05 )
     && ( ( OrderCurrentGet() & ( Obey_train | Shunt ) ) != 0 ) ) {
        IdleTime += dt;
    }
    else {
        IdleTime = 0.0;
    }

    // log vehicle data
    if( ( WriteLogFlag )
     && ( LastUpdatedTime > deltalog ) ) {
        // zapis do pliku DAT
        PhysicsLog();
        LastUpdatedTime -= deltalog;
    }

    if( ( fLastStopExpDist > 0.0 )
     && ( mvOccupied->DistCounter > fLastStopExpDist ) ) {
        // zaktualizować wyświetlanie rozkładu
        iStationStart = TrainParams->StationIndex;
        fLastStopExpDist = -1.0; // usunąć licznik
    }

    // ABu-160305 testowanie gotowości do jazdy
    // Ra: przeniesione z DynObj, skład użytkownika też jest testowany, żeby mu przekazać, że ma odhamować
	int index = double(BrakeAccTableSize) * (mvOccupied->Vel / mvOccupied->Vmax);
	index = std::min(BrakeAccTableSize, std::max(1, index));
	fBrake_a0[0] = fBrake_a0[index];
	fBrake_a1[0] = fBrake_a1[index];

	if ((mvOccupied->TrainType == dt_EZT) || (mvOccupied->TrainType == dt_DMU)) {
		auto Coeff = clamp( mvOccupied->Vel*0.015 , 0.5 , 1.0);
		fAccThreshold = fNominalAccThreshold * Coeff - fBrake_a0[BrakeAccTableSize] * (1.0 - Coeff);
	}

    Ready = true; // wstępnie gotowy
    fReady = 0.0; // założenie, że odhamowany
    fAccGravity = 0.0; // przyspieszenie wynikające z pochylenia
    double dy; // składowa styczna grawitacji, w przedziale <0,1>
    double AbsAccS = 0;
    TDynamicObject *p = pVehicles[0]; // pojazd na czole składu
    while (p)
    { // sprawdzenie odhamowania wszystkich połączonych pojazdów
        auto *vehicle { p->MoverParameters };
        if (Ready) {
            // bo jak coś nie odhamowane, to dalej nie ma co sprawdzać
            if (vehicle->BrakePress >= 0.4) // wg UIC określone sztywno na 0.04
            {
                Ready = false; // nie gotowy
                // Ra: odluźnianie przeładowanych lokomotyw, ciągniętych na zimno - prowizorka...
                if (AIControllFlag) // skład jak dotąd był wyluzowany
                {
                    if( ( mvOccupied->BrakeCtrlPos == 0 ) // jest pozycja jazdy
                     && ( ( vehicle->Hamulec->GetBrakeStatus() & b_dmg ) == 0 ) // brake isn't broken
                     && ( vehicle->PipePress - 5.0 > -0.1 ) // jeśli ciśnienie jak dla jazdy
                     && ( vehicle->Hamulec->GetCRP() > vehicle->PipePress + 0.12 ) ) { // za dużo w zbiorniku
                        // indywidualne luzowanko
                        vehicle->BrakeReleaser( 1 );
                    }
                }
            }
        }
        if (fReady < vehicle->BrakePress)
            fReady = vehicle->BrakePress; // szukanie najbardziej zahamowanego
        if( ( dy = p->VectorFront().y ) != 0.0 ) {
            // istotne tylko dla pojazdów na pochyleniu
            // ciężar razy składowa styczna grawitacji
            fAccGravity -= vehicle->TotalMassxg * dy * ( p->DirectionGet() == iDirection ? 1 : -1 );
        }
        if( ( vehicle->Power > 0.01 ) // jeśli ma silnik
         && ( vehicle->FuseFlag ) ) { // wywalony nadmiarowy
            Need_TryAgain = true; // reset jak przy wywaleniu nadmiarowego
        }
        p = p->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
    }

    // test state of main switch in all powered vehicles under control
    IsLineBreakerClosed = ( mvOccupied->Power > 0.01 ? mvOccupied->Mains : true );
    p = pVehicle;
    while( ( true == IsLineBreakerClosed )
        && ( ( p = p->PrevC( coupling::control) ) != nullptr ) ) {
        auto const *vehicle { p->MoverParameters };
        if( vehicle->Power > 0.01 ) {
            IsLineBreakerClosed = ( IsLineBreakerClosed && vehicle->Mains );
        }
    }
    p = pVehicle;
    while( ( true == IsLineBreakerClosed )
        && ( ( p = p->NextC( coupling::control ) ) != nullptr ) ) {
        auto const *vehicle { p->MoverParameters };
        if( vehicle->Power > 0.01 ) {
            IsLineBreakerClosed = ( IsLineBreakerClosed && vehicle->Mains );
        }
    }

    if( iDirection ) {
        // siłę generują pojazdy na pochyleniu ale działa ona całość składu, więc a=F/m
        fAccGravity *= iDirection;
        fAccGravity /= fMass;
    }
    if (!Ready) // v367: jeśli wg powyższych warunków skład nie jest odhamowany
        if (fAccGravity < -0.05) // jeśli ma pod górę na tyle, by się stoczyć
            // if (mvOccupied->BrakePress<0.08) //to wystarczy, że zadziałają liniowe (nie ma ich jeszcze!!!)
            if (fReady < 0.8) // delikatniejszy warunek, obejmuje wszystkie wagony
                Ready = true; //żeby uznać za odhamowany
    // second pass, for diesel engines verify the (live) engines are fully started
    // TODO: cache presence of diesel engines in the consist, to skip this test if there isn't any
    p = pVehicles[ 0 ]; // pojazd na czole składu
    while( ( true == Ready )
        && ( p != nullptr ) ) {

        auto const *vehicle { p->MoverParameters };

        if( ( vehicle->EngineType == TEngineType::DieselEngine )
         || ( vehicle->EngineType == TEngineType::DieselElectric ) ) {

            Ready = (
                ( vehicle->Vel > 0.5 ) // already moving
             || ( false == vehicle->Mains ) // deadweight vehicle
             || ( vehicle->enrot > 0.8 * (
                    vehicle->EngineType == TEngineType::DieselEngine ?
                        vehicle->dizel_nmin :
                        vehicle->DElist[ 0 ].RPM / 60.0 ) ) );
        }
        p = p->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
    }

    // HACK: crude way to deal with automatic door opening on W4 preventing further ride
    // for human-controlled vehicles with no door control and dynamic brake auto-activating with door open
    // TODO: check if this situation still happens and the hack is still needed
    if( ( false == AIControllFlag )
     && ( iDrivigFlags & moveDoorOpened )
     && ( mvOccupied->DoorCloseCtrl != control_t::driver )
     && ( mvControlling->MainCtrlPos > ( mvControlling->EngineType != TEngineType::DieselEngine ? 0 : 1 ) ) ) { // for diesel 1st position is effectively 0
        Doors( false );
    }

    // basic situational ai operations
    // TBD, TODO: move these to main routine, if it's not neccessary for them to fire every time?
    if( AIControllFlag ) {

        // wheel slip
        if( mvControlling->SlippingWheels ) {
            mvControlling->Sandbox( true ); // piasku!
        }
        else {
            // deactivate sandbox if we aren't slipping
            if( mvControlling->SandDose ) {
                mvControlling->Sandbox( false );
            }
        }

        if (mvControlling->EnginePowerSource.SourceType == TPowerSource::CurrentCollector) {

            if( mvOccupied->ScndPipePress > 4.3 ) {
                // gdy główna sprężarka bezpiecznie nabije ciśnienie to można przestawić kurek na zasilanie pantografów z głównej pneumatyki
                mvControlling->bPantKurek3 = true;
            }

            // uśrednione napięcie sieci: przy spadku poniżej wartości minimalnej opóźnić rozruch o losowy czas
            fVoltage = 0.5 * (fVoltage + std::abs(mvControlling->RunningTraction.TractionVoltage));
            if( fVoltage < mvControlling->EnginePowerSource.CollectorParameters.MinV ) {
                // gdy rozłączenie WS z powodu niskiego napięcia
                if( fActionTime >= 0 ) {
                    // jeśli czas oczekiwania nie został ustawiony, losowy czas oczekiwania przed ponownym załączeniem jazdy
                    fActionTime = -2.0 - Random( 10 );
                }
            }
        }

        if( mvOccupied->Vel > 1.0 ) {
            // jeżeli jedzie
            if( iDrivigFlags & moveDoorOpened ) {
                // jeśli drzwi otwarte
                // nie zamykać drzwi przy drganiach, bo zatrzymanie na W4 akceptuje niewielkie prędkości
                Doors( false );
            }
/*
            // NOTE: this section moved all cars to the edge of their respective roads
            // it may have some use eventually for collision avoidance,
            // but when enabled all the time it produces silly effect
            // przy prowadzeniu samochodu trzeba każdą oś odsuwać oddzielnie, inaczej kicha wychodzi
            if (mvOccupied->CategoryFlag & 2) // jeśli samochód
                // if (fabs(mvOccupied->OffsetTrackH)<mvOccupied->Dim.W) //Ra: szerokość drogi tu powinna być?
                if (!mvOccupied->ChangeOffsetH(-0.01 * mvOccupied->Vel * dt)) // ruch w poprzek drogi
                    mvOccupied->ChangeOffsetH(0.01 * mvOccupied->Vel * dt); // Ra: co to miało być, to nie wiem
*/
        }

        if (mvControlling->EnginePowerSource.SourceType == TPowerSource::CurrentCollector) {

            if( mvOccupied->Vel > 0.05 ) {
                // is moving
                if( ( fOverhead2 >= 0.0 ) || iOverheadZero ) {
                    // jeśli jazda bezprądowa albo z opuszczonym pantografem
                    while( DecSpeed( true ) ) { ; } // zerowanie napędu
                }
                if( ( fOverhead2 > 0.0 ) || iOverheadDown ) {
                    // jazda z opuszczonymi pantografami
                    mvControlling->PantFront( false );
                    mvControlling->PantRear( false );
                }
                else {
                    // jeśli nie trzeba opuszczać pantografów
                    // jazda na tylnym
                    if( iDirection >= 0 ) {
                        // jak jedzie w kierunku sprzęgu 0
                        mvControlling->PantRear( true );
                    }
                    else {
                        mvControlling->PantFront( true );
                    }
                }
                if( mvOccupied->Vel > 10 ) {
                    // opuszczenie przedniego po rozpędzeniu się o ile jest więcej niż jeden
                    if( mvControlling->EnginePowerSource.CollectorParameters.CollectorsNo > 1 ) {
                        if( iDirection >= 0 ) // jak jedzie w kierunku sprzęgu 0
                        { // poczekać na podniesienie tylnego
                            if( mvControlling->PantRearVolt != 0.0 ) {
                                // czy jest napięcie zasilające na tylnym?
                                mvControlling->PantFront( false ); // opuszcza od sprzęgu 0
                            }
                        }
                        else { // poczekać na podniesienie przedniego
                            if( mvControlling->PantFrontVolt != 0.0 ) {
                                // czy jest napięcie zasilające na przednim?
                                mvControlling->PantRear( false ); // opuszcza od sprzęgu 1
                            }
                        }
                    }
                }

                // TODO: refactor this calculation into a subroutine
                auto const useseriesmodevoltage {
                    interpolate(
                        mvControlling->EnginePowerSource.CollectorParameters.MinV,
                        mvControlling->EnginePowerSource.CollectorParameters.MaxV,
                        ( IsHeavyCargoTrain ? 0.35 : 0.40 ) ) };

                if( fVoltage <= useseriesmodevoltage ) {
                    // if the power station is heavily burdened try to reduce the load
                    switch( mvControlling->EngineType ) {

                        case TEngineType::ElectricSeriesMotor: {
                            if( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) {
                                // limit yourself to series mode
                                if( mvControlling->ScndCtrlPos ) {
                                    mvControlling->DecScndCtrl( 2 );
                                }
                                while( ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 )
                                    && ( mvControlling->DecMainCtrl( 1 ) ) ) {
                                    ; // all work is performed in the header
                                }
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            else {
                if( ( IdleTime > 45.0 )
                    // NOTE: abs(stoptime) covers either at least 15 sec remaining for a scheduled stop, or 15+ secs spent at a basic stop
                 && ( std::abs( fStopTime ) > 15.0 ) ) {
                    // spending a longer at a stop, raise also front pantograph
                    if( iDirection >= 0 ) // jak jedzie w kierunku sprzęgu 0
                        mvControlling->PantFront( true );
                    else
                        mvControlling->PantRear( true );
                }
            }
        }

        // horn control
        if( fWarningDuration > 0.0 ) {
            // jeśli pozostało coś do wytrąbienia trąbienie trwa nadal
            fWarningDuration -= dt;
            if( fWarningDuration < 0.05 )
                mvOccupied->WarningSignal = 0; // a tu się kończy
        }
        if( mvOccupied->Vel >= 5.0 ) {
            // jesli jedzie, można odblokować trąbienie, bo się wtedy nie włączy
            iDrivigFlags &= ~moveStartHornDone; // zatrąbi dopiero jak następnym razem stanie
            iDrivigFlags |= moveStartHorn; // i trąbić przed następnym ruszeniem
        }

        if( ( true == TestFlag( iDrivigFlags, moveStartHornNow ) )
         && ( true == Ready )
         && ( iEngineActive != 0 )
         && ( mvControlling->MainCtrlPos > 0 ) ) {
            // uruchomienie trąbienia przed ruszeniem
            fWarningDuration = 0.3; // czas trąbienia
            mvOccupied->WarningSignal = pVehicle->iHornWarning; // wysokość tonu (2=wysoki)
            iDrivigFlags |= moveStartHornDone; // nie trąbić aż do ruszenia
            iDrivigFlags &= ~moveStartHornNow; // trąbienie zostało zorganizowane
        }
    }

    // main ai update routine
    auto const reactiontime = std::min( ReactionTime, 2.0 );
    if( LastReactionTime < reactiontime ) { return; }

    LastReactionTime -= reactiontime;
    // Ra: nie wiem czemu ReactionTime potrafi dostać 12 sekund, to jest przegięcie, bo przeżyna STÓJ
    // yB: otóż jest to jedna trzecia czasu napełniania na towarowym; może się przydać przy
    // wdrażaniu hamowania, żeby nie ruszało kranem jak głupie
    // Ra: ale nie może się budzić co pół minuty, bo przeżyna semafory
    // Ra: trzeba by tak:
    // 1. Ustalić istotną odległość zainteresowania (np. 3×droga hamowania z V.max).
    // dla hamowania -0.2 [m/ss] droga wynosi 0.389*Vel*Vel [km/h], czyli 600m dla 40km/h, 3.8km
    // dla 100km/h i 9.8km dla 160km/h
    // dla hamowania -0.4 [m/ss] droga wynosi 0.096*Vel*Vel [km/h], czyli 150m dla 40km/h, 1.0km
    // dla 100km/h i 2.5km dla 160km/h
    // ogólnie droga hamowania przy stałym opóźnieniu to Vel*Vel/(3.6*3.6*a) [m]
    // fBrakeDist powinno być wyznaczane dla danego składu za pomocą sieci neuronowych, w
    // zależności od prędkości i siły (ciśnienia) hamowania
    // następnie w drugą stronę, z drogi hamowania i chwilowej prędkości powinno być wyznaczane zalecane ciśnienie

    // przybliżona droga hamowania
    fBrakeDist = fDriverBraking * mvOccupied->Vel * ( 40.0 + mvOccupied->Vel );
    if( fMass > 1000000.0 ) {
        // korekta dla ciężkich, bo przeżynają - da to coś?
        fBrakeDist *= 2.0;
    }
    if( ( -fAccThreshold > 0.05 )
     && ( mvOccupied->CategoryFlag == 1 ) ) {
        fBrakeDist = mvOccupied->Vel *	mvOccupied->Vel / 25.92 / -fAccThreshold;
    }
    if( mvOccupied->BrakeDelayFlag == bdelay_G ) {
        // dla nastawienia G koniecznie należy wydłużyć drogę na czas reakcji
        fBrakeDist += 2 * mvOccupied->Vel;
    }
    if( ( mvOccupied->Vel > 0.05 )
     && ( mvControlling->EngineType == TEngineType::ElectricInductionMotor )
     && ( ( mvControlling->TrainType & dt_EZT ) != 0 ) ) {
        // HACK: make the induction motor powered EMUs start braking slightly earlier
        fBrakeDist += 10.0;
    }
/*
    // take into account effect of gravity (but to stay on safe side of calculations, only downhill)
    if( fAccGravity > 0.025 ) {
        fBrakeDist *= ( 1.0 + fAccGravity );
        // TBD: use version which shortens route going uphill, too
        //fBrakeDist = std::max( fBrakeDist, fBrakeDist * ( 1.0 + fAccGravity ) );
    }
*/
    // route scan
    auto const routescanrange {
        std::max(
            750.0,
            mvOccupied->Vel > 5.0 ?
                400 + fBrakeDist :
                30.0 * fDriverDist ) }; // 1500m dla stojących pociągów;
    // Ra 2015-01: przy dłuższej drodze skanowania AI jeździ spokojniej
    // 2. Sprawdzić, czy tabelka pokrywa założony odcinek (nie musi, jeśli jest STOP).
    // 3. Sprawdzić, czy trajektoria ruchu przechodzi przez zwrotnice - jeśli tak, to sprawdzić,
    // czy stan się nie zmienił.
    // 4. Ewentualnie uzupełnić tabelkę informacjami o sygnałach i ograniczeniach, jeśli się
    // "zużyła".
    TableCheck( routescanrange ); // wypełnianie tabelki i aktualizacja odległości
    // 5. Sprawdzić stany sygnalizacji zapisanej w tabelce, wyznaczyć prędkości.
    // 6. Z tabelki wyznaczyć krytyczną odległość i prędkość (najmniejsze przyspieszenie).
    // 7. Jeśli jest inny pojazd z przodu, ewentualnie skorygować odległość i prędkość.
    // 8. Ustalić częstotliwość świadomości AI (zatrzymanie precyzyjne - częściej, brak atrakcji
    // - rzadziej).

    // check for potential colliders
    {
        auto rearvehicle = (
            pVehicles[ 0 ] == pVehicles[ 1 ] ?
                pVehicles[ 0 ] :
                pVehicles[ 1 ] );
        // for moving vehicle determine heading from velocity; for standing fall back on the set direction
        if( ( std::abs( mvOccupied->V ) < 0.1 ? // ignore potential micro-stutters in oposite direction during "almost stop"
                iDirection > 0 :
                mvOccupied->V > 0.0 ) ) {
            // towards coupler 0
            if( ( mvOccupied->V * iDirection < 0.0 )
             || ( ( rearvehicle->NextConnected != nullptr )
               && ( rearvehicle->MoverParameters->Couplers[ ( rearvehicle->DirectionGet() > 0 ? 1 : 0 ) ].CouplingFlag == coupling::faux ) ) ) {
                // scan behind if we're moving backward, or if we had something connected there and are moving away
                rearvehicle->ABuScanObjects( (
                    pVehicle->DirectionGet() == rearvehicle->DirectionGet() ?
                        -1 :
                         1 ),
                    fMaxProximityDist );
            }
            pVehicles[ 0 ]->ABuScanObjects( (
                pVehicle->DirectionGet() == pVehicles[ 0 ]->DirectionGet() ?
                     1 :
                    -1 ),
                routescanrange );
        }
        else {
            // towards coupler 1
            if( ( mvOccupied->V * iDirection < 0.0 )
             || ( ( rearvehicle->PrevConnected != nullptr )
               && ( rearvehicle->MoverParameters->Couplers[ ( rearvehicle->DirectionGet() > 0 ? 0 : 1 ) ].CouplingFlag == coupling::faux ) ) ) {
                // scan behind if we're moving backward, or if we had something connected there and are moving away
                rearvehicle->ABuScanObjects( (
                    pVehicle->DirectionGet() == rearvehicle->DirectionGet() ?
                         1 :
                        -1 ),
                    fMaxProximityDist );
            }
            pVehicles[ 0 ]->ABuScanObjects( (
                pVehicle->DirectionGet() == pVehicles[ 0 ]->DirectionGet() ?
                    -1 :
                     1 ),
                routescanrange );
        }
    }



    // tu bedzie logika sterowania
    if (AIControllFlag) {

        if (mvOccupied->CommandIn.Command != "")
            if( !mvOccupied->RunInternalCommand() ) {
                // rozpoznaj komende bo lokomotywa jej nie rozpoznaje
                RecognizeCommand(); // samo czyta komendę wstawioną do pojazdu?
            }
        if( mvOccupied->SecuritySystem.Status > 1 ) {
            // jak zadziałało CA/SHP
            if( !mvOccupied->SecuritySystemReset() ) { // to skasuj
                if( ( mvOccupied->BrakeCtrlPos == 0 )
                 && ( AccDesired > 0.0 )
                 && ( ( TestFlag( mvOccupied->SecuritySystem.Status, s_SHPebrake ) )
                   || ( TestFlag( mvOccupied->SecuritySystem.Status, s_CAebrake ) ) ) ) {
                    //!!! hm, może po prostu normalnie sterować hamulcem?
                    mvOccupied->BrakeLevelSet( 0 );
                }
            }
        }
        // basic emergency stop handling, while at it
        if( ( true == mvOccupied->RadioStopFlag ) // radio-stop
         && ( mvOccupied->Vel == 0.0 ) // and actual stop
         && ( true == mvOccupied->Radio ) ) { // and we didn't touch the radio yet
            // turning off the radio should reset the flag, during security system check
            if( m_radiocontroltime > 5.0 ) {
                // arbitrary delay between stop and disabling the radio
                mvOccupied->Radio = false;
                m_radiocontroltime = 0.0;
            }
            else {
                m_radiocontroltime += reactiontime;
            }
        }
        if( ( false == mvOccupied->Radio )
         && ( false == mvOccupied->RadioStopFlag ) ) {
            // otherwise if it's safe to do so, turn the radio back on
            if( m_radiocontroltime > 10.0 ) {
                // arbitrary 5 sec delay before switching radio back on
                mvOccupied->Radio = true;
                m_radiocontroltime = 0.0;
            }
            else {
                m_radiocontroltime += reactiontime;
            }
        }
    }

    switch (OrderList[OrderPos])
    { // ustalenie prędkości przy doczepianiu i odczepianiu, dystansów w pozostałych przypadkach
    case Connect: {
        // podłączanie do składu
        if (iDrivigFlags & moveConnect) {
            // jeśli stanął już blisko, unikając zderzenia i można próbować podłączyć
            fMinProximityDist = -1.0;
            fMaxProximityDist =  0.0; //[m] dojechać maksymalnie
            fVelPlus = 1.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 0.5; // margines prędkości powodujący załączenie napędu
            if (AIControllFlag)
            { // to robi tylko AI, wersję dla człowieka trzeba dopiero zrobić
                // sprzęgi sprawdzamy w pierwszej kolejności, bo jak połączony, to koniec
                bool ok; // true gdy się podłączy (uzyskany sprzęg będzie zgodny z żądanym)
                if (pVehicles[0]->DirectionGet() > 0) // jeśli sprzęg 0
                { // sprzęg 0 - próba podczepienia
                    if( pVehicles[ 0 ]->MoverParameters->Couplers[ 0 ].Connected ) {
                        // jeśli jest coś wykryte (a chyba jest, nie?)
                        if( pVehicles[ 0 ]->MoverParameters->Attach(
                            0, 2, pVehicles[ 0 ]->MoverParameters->Couplers[ 0 ].Connected,
                            iCoupler ) ) {
                        // pVehicles[0]->dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                        // pVehicles[0]->dsbCouplerAttach->Play(0,0,0);
                        }
                    }
                    // udało się? (mogło częściowo)
                    ok = (pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag == iCoupler);
                }
                else // if (pVehicles[0]->MoverParameters->DirAbsolute<0) //jeśli sprzęg 1
                { // sprzęg 1 - próba podczepienia
                    if( pVehicles[ 0 ]->MoverParameters->Couplers[ 1 ].Connected ) {
                        // jeśli jest coś wykryte (a chyba jest, nie?)
                        if( pVehicles[ 0 ]->MoverParameters->Attach(
                            1, 2, pVehicles[ 0 ]->MoverParameters->Couplers[ 1 ].Connected,
                            iCoupler ) ) {
                        // pVehicles[0]->dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                        // pVehicles[0]->dsbCouplerAttach->Play(0,0,0);
                        }
                    }
                    // udało się? (mogło częściowo)
                    ok = (pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag == iCoupler); 
                }
                if (ok)
                { // jeżeli został podłączony
                    iCoupler = 0; // dalsza jazda manewrowa już bez łączenia
                    iDrivigFlags &= ~moveConnect; // zdjęcie flagi doczepiania
                    SetVelocity(0, 0, stopJoin); // wyłączyć przyspieszanie
                    CheckVehicles(); // sprawdzić światła nowego składu
                    JumpToNextOrder(); // wykonanie następnej komendy
                }

            } // if (AIControllFlag) //koniec zblokowania, bo była zmienna lokalna
        }
        else {
            // jak daleko, to jazda jak dla Shunt na kolizję
            fMinProximityDist = 2.0;
            fMaxProximityDist = 5.0; //[m] w takim przedziale odległości powinien stanąć
            fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 1.0; // margines prędkości powodujący załączenie napędu
            if( pVehicles[ 0 ]->fTrackBlock <= 20.0 ) {
                // przy zderzeniu fTrackBlock nie jest miarodajne
                // początek podczepiania, z wyłączeniem sprawdzania fTrackBlock
                iDrivigFlags |= moveConnect;
            }
        }
        break;
    }
    case Disconnect: {
        // 20.07.03 - manewrowanie wagonami
        fMinProximityDist = 1.0;
        fMaxProximityDist = 10.0; //[m]
        fVelPlus = 1.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        fVelMinus = 0.5; // margines prędkości powodujący załączenie napędu
        break;
    }
    case Shunt: {
        // na jaką odleglość i z jaką predkością ma podjechać
        // TODO: test if we can use the distances calculation from obey_train
        fMinProximityDist = std::min( 5 + iVehicles, 25 );
        fMaxProximityDist = std::min( 10 + iVehicles, 50 );
/*
        if( IsHeavyCargoTrain ) {
            fMaxProximityDist *= 1.5;
        }
*/
        fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        // margines prędkości powodujący załączenie napędu
        // były problemy z jazdą np. 3km/h podczas ładowania wagonów
        fVelMinus = std::min( 0.1 * fShuntVelocity, 3.0 );
        break;
    }
    case Obey_train: {
        // na jaka odleglosc i z jaka predkoscia ma podjechac do przeszkody
        if( mvOccupied->CategoryFlag & 1 ) {
            // jeśli pociąg
            fMinProximityDist = clamp(  5 + iVehicles, 10, 15 );
            fMaxProximityDist = clamp( 10 + iVehicles, 15, 40 );

            if( IsCargoTrain ) {
                // increase distances for cargo trains to take into account slower reaction to brakes
                fMinProximityDist += 10.0;
                fMaxProximityDist += 10.0;
/*
                if( IsHeavyCargoTrain ) {
                    // cargo trains with high braking threshold may require even larger safety margin
                    fMaxProximityDist += 20.0;
                }
*/
            }
            if( mvOccupied->Vel < 0.1 ) {
                // jak stanie za daleko, to niech nie dociąga paru metrów
                fMaxProximityDist = 50.0;
            }

            if( iDrivigFlags & moveLate ) {
                // jeśli spóźniony, to gna
                fVelMinus = 1.0;
                // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
                fVelPlus = 5.0;
            }
            else {
                // gdy nie musi się sprężać
                // margines prędkości powodujący załączenie napędu; min 1.0 żeby nie ruszał przy 0.1
                fVelMinus = clamp( std::round( 0.05 * VelDesired ), 1.0, 5.0 );
                // normalnie dopuszczalne przekroczenie to 5% prędkości ale nie więcej niż 5km/h
                // bottom margin raised to 2 km/h to give the AI more leeway at low speed limits
                fVelPlus = clamp( std::ceil( 0.05 * VelDesired ), 2.0, 5.0 );
            }
        }
        else {
            // samochod (sokista też)
            fMinProximityDist = std::max( 3.5, mvOccupied->Vel * 0.2   );
            fMaxProximityDist = std::max( 9.5, mvOccupied->Vel * 0.375 ); //[m]
            // margines prędkości powodujący załączenie napędu
            fVelMinus = 2.0;
            // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelPlus = std::min( 10.0, mvOccupied->Vel * 0.1 );
        }
        break;
    }
    default: {
        fMinProximityDist = 5.0;
        fMaxProximityDist = 10.0; //[m]
        fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        fVelMinus = 5.0; // margines prędkości powodujący załączenie napędu
    }
    } // switch OrderList[OrderPos]

    switch (OrderList[OrderPos])
    { // co robi maszynista
    case Prepare_engine: // odpala silnik
        // if (AIControllFlag)
        if (PrepareEngine()) // dla użytkownika tylko sprawdza, czy uruchomił
        { // gotowy do drogi?
            SetDriverPsyche();
            // OrderList[OrderPos]:=Shunt; //Ra: to nie może tak być, bo scenerie robią
            // Jump_to_first_order i przechodzi w manewrowy
            JumpToNextOrder(); // w następnym jest Shunt albo Obey_train, moze też być
            // Change_direction, Connect albo Disconnect
            // if OrderList[OrderPos]<>Wait_for_Orders)
            // if BrakeSystem=Pneumatic)  //napelnianie uderzeniowe na wstepie
            //  if BrakeSubsystem=Oerlikon)
            //   if (BrakeCtrlPos=0))
            //    DecBrakeLevel;
        }
        break;
    case Release_engine:
        if( ReleaseEngine() ) // zdana maszyna?
            JumpToNextOrder();
        break;
    case Jump_to_first_order:
        if (OrderPos > 1)
            OrderPos = 1; // w zerowym zawsze jest czekanie
        else
            ++OrderPos;
#if LOGORDERS
        WriteLog("--> Jump_to_first_order");
        OrdersDump();
#endif
        break;
    case Wait_for_orders: // jeśli czeka, też ma skanować, żeby odpalić się od semafora
    case Shunt:
    case Obey_train:
    case Connect:
    case Disconnect:
    case Change_direction: // tryby wymagające jazdy
    case Change_direction | Shunt: // zmiana kierunku podczas manewrów
    case Change_direction | Connect: // zmiana kierunku podczas podłączania
        if (OrderList[OrderPos] != Obey_train) // spokojne manewry
        {
            VelSignal = min_speed( VelSignal, 40.0 ); // jeśli manewry, to ograniczamy prędkość

            // NOTE: this section should be moved to disconnect or plain removed, but it seems to be (mis)used by some scenarios
            // to keep vehicle idling without moving :|
            if( ( true == AIControllFlag )
             && ( iVehicleCount >= 0 )
             && ( false == TestFlag( iDrivigFlags, movePress ) )
             && ( iCoupler == 0 )
//             && ( mvOccupied->Vel > 0.0 )
             && ( pVehicle->MoverParameters->Couplers[ side::front ].CouplingFlag == coupling::faux )
             && ( pVehicle->MoverParameters->Couplers[ side::rear ].CouplingFlag == coupling::faux ) ) {
                SetVelocity(0, 0, stopJoin); // 1. faza odczepiania: zatrzymanie
                // WriteLog("Zatrzymanie w celu odczepienia");
                AccPreferred = std::min( 0.0, AccPreferred );
            }

        }
        else
            SetDriverPsyche(); // Ra: było w PrepareEngine(), potrzebne tu?

        if (OrderList[OrderPos] & (Shunt | Obey_train | Connect)) {
            // odjechać sam może tylko jeśli jest w trybie jazdy
            // automatyczne ruszanie po odstaniu albo spod SBL
            if( ( VelSignal == 0.0 )
             && ( WaitingTime > 0.0 )
             && ( mvOccupied->RunningTrack.Velmax != 0.0 ) ) {
                // jeśli stoi, a upłynął czas oczekiwania i tor ma niezerową prędkość
                if( ( OrderList[ OrderPos ] & ( Obey_train | Shunt ) )
                 && ( iDrivigFlags & moveStopHere ) ) {
                    // zakaz ruszania z miejsca bez otrzymania wolnej drogi
                    WaitingTime = -WaitingExpireTime;
                }
                else if (mvOccupied->CategoryFlag & 1)
                { // jeśli pociąg
                    if (AIControllFlag)
                    {
                        PrepareEngine(); // zmieni ustawiony kierunek
                        SetVelocity(20, 20); // jak się nastał, to niech jedzie 20km/h
                        WaitingTime = 0.0;
                        fWarningDuration = 1.5; // a zatrąbić trochę
                        mvOccupied->WarningSignal = 1;
                    }
                    else
                        SetVelocity(20, 20); // użytkownikowi zezwalamy jechać
                }
                else
                { // samochód ma stać, aż dostanie odjazd, chyba że stoi przez kolizję
                    if (eStopReason == stopBlock)
                        if (pVehicles[0]->fTrackBlock > fDriverDist)
                            if (AIControllFlag)
                            {
                                PrepareEngine(); // zmieni ustawiony kierunek
                                SetVelocity(-1, -1); // jak się nastał, to niech jedzie
                                WaitingTime = 0.0;
                            }
                            else {
                                // użytkownikowi pozwalamy jechać (samochodem?)
                                SetVelocity( -1, -1 );
                            }
                }
            }
            else if( ( VelSignal == 0.0 ) && ( VelNext > 0.0 ) && ( mvOccupied->Vel < 1.0 ) ) {
                if( iCoupler ? true : ( iDrivigFlags & moveStopHere ) == 0 ) {
                    // Ra: tu jest coś nie tak, bo bez tego warunku ruszało w manewrowym !!!!
                    SetVelocity( VelNext, VelNext, stopSem ); // omijanie SBL
                }
            }
        } // koniec samoistnego odjeżdżania

        if( ( true == AIControllFlag)
         && ( true == TestFlag( OrderList[ OrderPos ], Change_direction ) ) ) {
            // sprobuj zmienic kierunek (może być zmieszane z jeszcze jakąś komendą)
            if( mvOccupied->Vel < 0.1 ) {
                // jeśli się zatrzymał, to zmieniamy kierunek jazdy, a nawet kabinę/człon
                Activation(); // ustawienie zadanego wcześniej kierunku i ewentualne przemieszczenie AI
                PrepareEngine();
                JumpToNextOrder(); // następnie robimy, co jest do zrobienia (Shunt albo Obey_train)
                if( OrderList[ OrderPos ] & ( Shunt | Connect ) ) {
                    // jeśli dalej mamy manewry
                    if( false == TestFlag( iDrivigFlags, moveStopHere ) ) {
                        // o ile nie ma stać w miejscu,
                        // jechać od razu w przeciwną stronę i nie trąbić z tego tytułu:
                        iDrivigFlags &= ~moveStartHorn;
                        SetVelocity( fShuntVelocity, fShuntVelocity );
                    }
                }
            }
        } // Change_direction (tylko dla AI)

        if( ( true == AIControllFlag )
         && ( iEngineActive == 0 )
         && ( OrderList[ OrderPos ] & ( Change_direction | Connect | Disconnect | Shunt | Obey_train ) ) ) {
            // jeśli coś ma robić to niech odpala do skutku
            PrepareEngine();
        }

        // ustalanie zadanej predkosci
        if (iDrivigFlags & moveActive) {

            SetDriverPsyche(); // ustawia AccPreferred (potrzebne tu?)

            // jeśli może skanować sygnały i reagować na komendy
            // jeśli jest wybrany kierunek jazdy, można ustalić prędkość jazdy
            // Ra: tu by jeszcze trzeba było wstawić uzależnienie (VelDesired) od odległości od przeszkody
            // no chyba żeby to uwzgldnić już w (ActualProximityDist)
            VelDesired = fVelMax; // wstępnie prędkość maksymalna dla pojazdu(-ów), będzie
            // następnie ograniczana
            if( ( TrainParams )
             && ( TrainParams->TTVmax > 0.0 ) ) {
                // jeśli ma rozkład i ograniczenie w rozkładzie to nie przekraczać rozkladowej
                VelDesired = min_speed( VelDesired, TrainParams->TTVmax );
            }
            // szukanie optymalnych wartości
            AccDesired = AccPreferred; // AccPreferred wynika z osobowości mechanika
            VelNext = VelDesired; // maksymalna prędkość wynikająca z innych czynników niż trajektoria ruchu
            ActualProximityDist = routescanrange; // funkcja Update() może pozostawić wartości bez zmian
            // Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prędkosci
            TCommandType comm = TableUpdate(VelDesired, ActualProximityDist, VelNext, AccDesired);

            switch (comm) {
                // ustawienie VelSignal - trochę proteza = do przemyślenia
            case TCommandType::cm_Ready: // W4 zezwolił na jazdę
                // ewentualne doskanowanie trasy za W4, który zezwolił na jazdę
                TableCheck( routescanrange);
                TableUpdate(VelDesired, ActualProximityDist, VelNext, AccDesired); // aktualizacja po skanowaniu
                if (VelNext == 0.0)
                    break; // ale jak coś z przodu zamyka, to ma stać
                if (iDrivigFlags & moveStopCloser)
                    VelSignal = -1.0; // ma czekać na sygnał z sygnalizatora!
            case TCommandType::cm_SetVelocity: // od wersji 357 semafor nie budzi wyłączonej lokomotywy
                if (!(OrderList[OrderPos] &
                        ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                    if (fabs(VelSignal) >=
                        1.0) // 0.1 nie wysyła się do samochodow, bo potem nie ruszą
                        PutCommand("SetVelocity", VelSignal, VelNext, nullptr); // komenda robi dodatkowe operacje
                break;
            case TCommandType::cm_ShuntVelocity: // od wersji 357 Tm nie budzi wyłączonej lokomotywy
                if (!(OrderList[OrderPos] &
                        ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                    PutCommand("ShuntVelocity", VelSignal, VelNext, nullptr);
                else if (iCoupler) // jeśli jedzie w celu połączenia
                    SetVelocity(VelSignal, VelNext);
                break;
            case TCommandType::cm_Command: // komenda z komórki
                if( !( OrderList[ OrderPos ] & ~( Obey_train | Shunt ) ) ) {
                    // jedzie w dowolnym trybie albo Wait_for_orders
                    if( mvOccupied->Vel < 0.1 ) {
                        // dopiero jak stanie
/*
                        PutCommand( eSignNext->text(), eSignNext->value( 1 ), eSignNext->value( 2 ), nullptr );
                        eSignNext->StopCommandSent(); // się wykonało już
*/                      // replacement of the above
                        eSignNext->send_command( *this );
                    }
                }
                break;
            default:
                break;
            }
            // disconnect mode potentially overrides scan results
            // TBD: when in this mode skip scanning altogether?
            if( ( OrderCurrentGet() & Disconnect ) != 0 ) {

                if (AIControllFlag) {
                    if (iVehicleCount >= 0) {
                        // jeśli była podana ilość wagonów
                        if (iDrivigFlags & movePress) {
                            // jeśli dociskanie w celu odczepienia
                            // 3. faza odczepiania.
                            SetVelocity(2, 0); // jazda w ustawionym kierunku z prędkością 2
                            if ((mvControlling->MainCtrlPos > 0) ||
                                (mvOccupied->BrakeSystem == TBrakeSystem::ElectroPneumatic)) // jeśli jazda
                            {
                                WriteLog(mvOccupied->Name + " odczepianie w kierunku " + std::to_string(mvOccupied->DirAbsolute));
                                TDynamicObject *p =
                                    pVehicle; // pojazd do odczepienia, w (pVehicle) siedzi AI
                                int d; // numer sprzęgu, który sprawdzamy albo odczepiamy
                                int n = iVehicleCount; // ile wagonów ma zostać
                                do
                                { // szukanie pojazdu do odczepienia
                                    d = p->DirectionGet() > 0 ?
                                            0 :
                                            1; // numer sprzęgu od strony czoła składu
                                    // if (p->MoverParameters->Couplers[d].CouplerType==Articulated)
                                    // //jeśli sprzęg typu wózek (za mało)
                                    if (p->MoverParameters->Couplers[d].CouplingFlag & ctrain_depot) // jeżeli sprzęg zablokowany
                                        // if (p->GetTrack()->) //a nie stoi na torze warsztatowym
                                        // (ustalić po czym poznać taki tor)
                                        ++n; // to  liczymy człony jako jeden
                                    p->MoverParameters->BrakeReleaser(1); // wyluzuj pojazd, aby dało się dopychać
                                    p->MoverParameters->BrakeLevelSet(0); // hamulec na zero, aby nie hamował
                                    if (n)
                                    { // jeśli jeszcze nie koniec
                                        p = p->Prev(); // kolejny w stronę czoła składu (licząc od
                                        // tyłu), bo dociskamy
                                        if (!p)
                                            iVehicleCount = -2,
                                            n = 0; // nie ma co dalej sprawdzać, doczepianie zakończone
                                    }
                                } while (n--);
                                if( p ? p->MoverParameters->Couplers[ d ].CouplingFlag == coupling::faux : true ) {
                                    // no target, or already just virtual coupling
                                    WriteLog( mvOccupied->Name + " didn't find anything to disconnect." );
                                    iVehicleCount = -2; // odczepiono, co było do odczepienia
                                } else if ( p->Dettach(d) == coupling::faux ) {
                                    // tylko jeśli odepnie
                                    WriteLog( mvOccupied->Name + " odczepiony." );
                                    iVehicleCount = -2;
                                } // a jak nie, to dociskać dalej
                            }
                            if (iVehicleCount >= 0) // zmieni się po odczepieniu
                                if (!mvOccupied->DecLocalBrakeLevel(1))
                                { // dociśnij sklad
                                    WriteLog( mvOccupied->Name + " dociskanie..." );
                                    // mvOccupied->BrakeReleaser(); //wyluzuj lokomotywę
                                    // Ready=true; //zamiast sprawdzenia odhamowania całego składu
                                    IncSpeed(); // dla (Ready)==false nie ruszy
                                }
                        }
                        if ((mvOccupied->Vel < 0.01) && !(iDrivigFlags & movePress))
                        { // 2. faza odczepiania: zmień kierunek na przeciwny i dociśnij
                            // za radą yB ustawiamy pozycję 3 kranu (ruszanie kranem w innych miejscach
                            // powino zostać wyłączone)
                            // WriteLog("Zahamowanie składu");
                            mvOccupied->BrakeLevelSet(
                                mvOccupied->BrakeSystem == TBrakeSystem::ElectroPneumatic ?
                                    1 :
                                    3 );
                            double p = mvOccupied->BrakePressureActual.PipePressureVal;
                            if( p < 3.9 ) {
                                // tu może być 0 albo -1 nawet
                                // TODO: zabezpieczenie przed dziwnymi CHK do czasu wyjaśnienia sensu 0 oraz -1 w tym miejscu
                                p = 3.9;
                            }
                            if (mvOccupied->BrakeSystem == TBrakeSystem::ElectroPneumatic ?
                                    mvOccupied->BrakePress > 2 :
                                    mvOccupied->PipePress < p + 0.1)
                            { // jeśli w miarę został zahamowany (ciśnienie mniejsze niż podane na
                                // pozycji 3, zwyle 0.37)
                                if (mvOccupied->BrakeSystem == TBrakeSystem::ElectroPneumatic)
                                    mvOccupied->BrakeLevelSet(0); // wyłączenie EP, gdy wystarczy (może
                                // nie być potrzebne, bo na początku jest)
                                WriteLog("Luzowanie lokomotywy i zmiana kierunku");
                                mvOccupied->BrakeReleaser(1); // wyluzuj lokomotywę; a ST45?
                                mvOccupied->DecLocalBrakeLevel(LocalBrakePosNo); // zwolnienie hamulca
                                iDrivigFlags |= movePress; // następnie będzie dociskanie
                                DirectionForward(mvOccupied->ActiveDir < 0); // zmiana kierunku jazdy na przeciwny (dociskanie)
                                CheckVehicles(); // od razu zmienić światła (zgasić) - bez tego się nie odczepi
        /*
                                // NOTE: disabled to prevent closing the door before passengers can disembark
                                fStopTime = 0.0; // nie ma na co czekać z odczepianiem
        */
                            }
                        }
                        else {
                            if( mvOccupied->Vel > 0.01 ) {
                                // 1st phase(?)
                                // bring it to stop if it's not already stopped
                                SetVelocity( 0, 0, stopJoin ); // wyłączyć przyspieszanie
                            }
                        }
                    } // odczepiania
                    else // to poniżej jeśli ilość wagonów ujemna
                        if (iDrivigFlags & movePress)
                    { // 4. faza odczepiania: zwolnij i zmień kierunek
                        SetVelocity(0, 0, stopJoin); // wyłączyć przyspieszanie
                        if (!DecSpeed()) // jeśli już bardziej wyłączyć się nie da
                        { // ponowna zmiana kierunku
                            WriteLog( mvOccupied->Name + " ponowna zmiana kierunku" );
                            DirectionForward(mvOccupied->ActiveDir < 0); // zmiana kierunku jazdy na właściwy
                            iDrivigFlags &= ~movePress; // koniec dociskania
                            JumpToNextOrder(); // zmieni światła
                            TableClear(); // skanowanie od nowa
                            iDrivigFlags &= ~moveStartHorn; // bez trąbienia przed ruszeniem
                            SetVelocity(fShuntVelocity, fShuntVelocity); // ustawienie prędkości jazdy
                        }
                    }
                }
            }

            if( true == TestFlag( OrderList[ OrderPos ], Change_direction ) ) {
                // if ordered to change direction, try to stop
                SetVelocity( 0, 0, stopDir );
            }

            if( VelNext == 0.0 ) {
                if( !( OrderList[ OrderPos ] & ~( Shunt | Connect ) ) ) {
                    // jedzie w Shunt albo Connect, albo Wait_for_orders
                    // w trybie Connect skanować do tyłu tylko jeśli przed kolejnym sygnałem nie ma taboru do podłączenia
                    // Ra 2F1H: z tym (fTrackBlock) to nie jest najlepszy pomysł, bo lepiej by
                    // było porównać z odległością od sygnalizatora z przodu
                    if( ( OrderList[ OrderPos ] & Connect ) ?
                            ( pVehicles[ 0 ]->fTrackBlock > 2000 || pVehicles[ 0 ]->fTrackBlock > FirstSemaphorDist ) :
                            true ) {
                        if( ( comm = BackwardScan() ) != TCommandType::cm_Unknown ) {
                            // jeśli w drugą można jechać
                            // należy sprawdzać odległość od znalezionego sygnalizatora,
                            // aby w przypadku prędkości 0.1 wyciągnąć najpierw skład za sygnalizator
                            // i dopiero wtedy zmienić kierunek jazdy, oczekując podania prędkości >0.5
                            if( comm == TCommandType::cm_Command ) {
                                // jeśli komenda Shunt to ją odbierz bez przemieszczania się (np. odczep wagony po dopchnięciu do końca toru)
                                iDrivigFlags |= moveStopHere;
                            }
                            iDirectionOrder = -iDirection; // zmiana kierunku jazdy
                            // zmiana kierunku bez psucia kolejnych komend
                            OrderList[ OrderPos ] = TOrders( OrderList[ OrderPos ] | Change_direction );
                        }
                    }
                }
            }

            double vel = mvOccupied->Vel; // prędkość w kierunku jazdy
            if (iDirection * mvOccupied->V < 0)
                vel = -vel; // ujemna, gdy jedzie w przeciwną stronę, niż powinien
            if (VelDesired < 0.0)
                VelDesired = fVelMax; // bo VelDesired<0 oznacza prędkość maksymalną

            // Ra: jazda na widoczność
/*
            // condition disabled, it'd prevent setting reduced acceleration in the last connect stage
            if ((iDrivigFlags & moveConnect) == 0) // przy końcówce podłączania nie hamować
*/
            { // sprawdzenie jazdy na widoczność
                auto const vehicle = pVehicles[ 0 ]; // base calculactions off relevant end of the consist
                auto const coupler = 
                    vehicle->MoverParameters->Couplers + (
                    vehicle->DirectionGet() > 0 ?
                        0 :
                        1 ); // sprzęg z przodu składu
                if( ( coupler->Connected )
                 && ( coupler->CouplingFlag == coupling::faux ) ) {
                    // mamy coś z przodu podłączone sprzęgiem wirtualnym
                    // wyliczanie optymalnego przyspieszenia do jazdy na widoczność
/*
                    ActualProximityDist = std::min(
                        ActualProximityDist,
                        vehicle->fTrackBlock - (
                            mvOccupied->CategoryFlag & 2 ?
                                fMinProximityDist : // cars can bunch up tighter
                                fMaxProximityDist ) ); // other vehicle types less so
*/
                    // prędkość pojazdu z przodu (zakładając, że jedzie w tę samą stronę!!!)
                    double k = coupler->Connected->Vel;
                    if( k - vel < 5 ) {
                        // porównanie modułów prędkości [km/h]
                        // zatroszczyć się trzeba, jeśli tamten nie jedzie znacząco szybciej
                        ActualProximityDist = std::min(
                            ActualProximityDist,
                            vehicle->fTrackBlock );

                        if( ActualProximityDist <= (
                            ( mvOccupied->CategoryFlag & 2 ) ?
                                100.0 : // cars
                                250.0 ) ) { // others
                            // regardless of driving mode at close distance take precaution measures:
                            // match the other vehicle's speed or slow down if the other vehicle is stopped
                            VelDesired =
                                min_speed(
                                    VelDesired,
                                    std::max(
                                        k,
                                        ( mvOccupied->CategoryFlag & 2 ) ?
                                            40.0 : // cars
                                            20.0 ) ); // others
                            if( vel > VelDesired + fVelPlus ) {
                                // if going too fast force some prompt braking
                                AccPreferred = std::min( -0.65, AccPreferred );
                            }
                        }

                        double const distance = vehicle->fTrackBlock - fMaxProximityDist - ( fBrakeDist * 1.15 ); // odległość bezpieczna zależy od prędkości
                        if( distance < 0.0 ) {
                            // jeśli odległość jest zbyt mała
                            if( k < 10.0 ) // k - prędkość tego z przodu
                            { // jeśli tamten porusza się z niewielką prędkością albo stoi
                                if( OrderCurrentGet() & Connect ) {
                                    // jeśli spinanie, to jechać dalej
                                    AccPreferred = std::min( 0.35, AccPreferred ); // nie hamuj
                                    VelDesired =
                                        min_speed(
                                            VelDesired,
                                            ( vehicle->fTrackBlock > 150.0 ?
                                                20.0:
                                                4.0 ) );
                                    VelNext = 2.0; // i pakuj się na tamtego
                                }
                                else {
                                    // a normalnie to hamować
                                    VelNext = 0.0;
                                    if( vehicle->fTrackBlock <= fMinProximityDist ) {
                                        VelDesired = 0.0;
                                    }

                                    if( ( mvOccupied->CategoryFlag & 1 )
                                     && ( OrderCurrentGet() & Obey_train ) ) {
                                        // trains which move normally should try to stop at safe margin
                                        ActualProximityDist -= fDriverDist;
                                    }
                                }
                            }
                            else {
                                // jeśli oba jadą, to przyhamuj lekko i ogranicz prędkość
                                if( vehicle->fTrackBlock < (
                                        ( mvOccupied->CategoryFlag & 2 ) ?
                                            fMaxProximityDist + 0.5 * vel : // cars
                                            2.0 * fMaxProximityDist + 2.0 * vel ) ) { //others
                                    // jak tamten jedzie wolniej a jest w drodze hamowania
                                    AccPreferred = std::min( -0.9, AccPreferred );
                                    VelNext = min_speed( std::round( k ) - 5.0, VelDesired );
                                    if( vehicle->fTrackBlock <= (
                                        ( mvOccupied->CategoryFlag & 2 ) ?
                                            fMaxProximityDist : // cars
                                            2.0 * fMaxProximityDist ) ) { //others
                                        // try to force speed change if obstacle is really close
                                        VelDesired = VelNext;
                                    }
                                }
                            }
                            ReactionTime = (
                                mvOccupied->Vel > 0.01 ?
                                    0.1 : // orientuj się, bo jest goraco
                                    2.0 ); // we're already standing still, so take it easy
                        }
                        else {
                            if( OrderCurrentGet() & Connect ) {
                                // if there's something nearby in the connect mode don't speed up too much
                                VelDesired =
                                    min_speed(
                                        VelDesired,
                                        ( vehicle->fTrackBlock > 100.0 ?
                                            20.0 :
                                            4.0 ) );
                            }
                        }
                    }
                }
            }

            // sprawdzamy możliwe ograniczenia prędkości
            if( VelSignal >= 0 ) {
                // jeśli skład był zatrzymany na początku i teraz już może jechać
                VelDesired =
                    min_speed(
                        VelDesired,
                        VelSignal );
            }
            if( mvOccupied->RunningTrack.Velmax >= 0 ) {
                // ograniczenie prędkości z trajektorii ruchu
                VelDesired =
                    min_speed(
                        VelDesired,
                        mvOccupied->RunningTrack.Velmax ); // uwaga na ograniczenia szlakowej!
            }
            if( VelforDriver >= 0 ) {
                // tu jest zero przy zmianie kierunku jazdy
                // Ra: tu może być 40, jeśli mechanik nie ma znajomości szlaaku, albo kierowca jeździ 70
                VelDesired =
                    min_speed(
                        VelDesired,
                        VelforDriver );
            }
            if( fStopTime < 0 ) {
                // czas postoju przed dalszą jazdą (np. na przystanku)
                VelDesired = 0.0; // jak ma czekać, to nie ma jazdy
            }

            if( ( OrderCurrentGet() & Obey_train ) != 0 ) {

                if( ( TrainParams->CheckTrainLatency() < 5.0 )
                 && ( TrainParams->TTVmax > 0.0 ) ) {
                    // jesli nie spozniony to nie przekraczać rozkladowej
                    VelDesired =
                        min_speed(
                            VelDesired,
                            TrainParams->TTVmax );
                }
            }

            if( ( OrderCurrentGet() & ( Shunt | Obey_train ) ) != 0 ) {
                // w Connect nie, bo moveStopHere odnosi się do stanu po połączeniu
                if( ( ( iDrivigFlags & moveStopHere ) != 0 )
                 && ( vel < 0.01 )
                 && ( VelSignalNext == 0.0 ) ) {
                    // jeśli ma czekać na wolną drogę, stoi a wyjazdu nie ma, to ma stać
                    VelDesired = 0.0;
                }
            }

            if( OrderCurrentGet() == Wait_for_orders ) {
                // wait means sit and wait
                VelDesired = 0.0;
            }
            // end of speed caps checks

            if( ( ( OrderCurrentGet() & Obey_train ) != 0 )
             && ( ( iDrivigFlags & moveGuardSignal ) != 0 )
             && ( VelDesired > 0.0 ) ) {
                // komunikat od kierownika tu, bo musi być wolna droga i odczekany czas stania
                iDrivigFlags &= ~moveGuardSignal; // tylko raz nadać
                if( false == tsGuardSignal.empty() ) {
                    tsGuardSignal.stop();
                    // w zasadzie to powinien mieć flagę, czy jest dźwiękiem radiowym, czy bezpośrednim
                    // albo trzeba zrobić dwa dźwięki, jeden bezpośredni, słyszalny w
                    // pobliżu, a drugi radiowy, słyszalny w innych lokomotywach
                    // na razie zakładam, że to nie jest dźwięk radiowy, bo trzeba by zrobić
                    // obsługę kanałów radiowych itd.
                    if( iGuardRadio == 0 ) {
                        // jeśli nie przez radio
                        tsGuardSignal.owner( pVehicle );
                        // place virtual conductor some distance away
                        tsGuardSignal.offset( { pVehicle->MoverParameters->Dim.W * -0.75f, 1.7f, std::min( -20.0, -0.2 * fLength ) } );
                        tsGuardSignal.play( sound_flags::exclusive );
                    }
                    else {
                        // if (iGuardRadio==iRadioChannel) //zgodność kanału
                        // if (!FreeFlyModeFlag) //obserwator musi być w środku pojazdu
                        // (albo może mieć radio przenośne) - kierownik mógłby powtarzać przy braku reakcji
                        // TODO: proper system for sending/receiving radio messages
                        // place the sound in appropriate cab of the manned vehicle
                        tsGuardSignal.owner( pVehicle );
                        tsGuardSignal.offset( { 0.f, 2.f, pVehicle->MoverParameters->Dim.L * 0.4f * ( pVehicle->MoverParameters->ActiveCab < 0 ? -1 : 1 ) } );
                        tsGuardSignal.play( sound_flags::exclusive );
                    }
                }
            }

            if( mvOccupied->Vel < 0.01 ) {
                // Ra 2014-03: jesli skład stoi, to działa na niego składowa styczna grawitacji
                AbsAccS = fAccGravity;
            }
			else {
				AbsAccS = 0;
				TDynamicObject *d = pVehicles[0]; // pojazd na czele składu
				while (d)
				{
					AbsAccS += d->MoverParameters->TotalMass * d->MoverParameters->AccS * ( d->DirectionGet() == iDirection ? 1 : -1 );
					d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
				}
                AbsAccS *= iDirection;
				AbsAccS /= fMass;
			}
			AbsAccS_pub = AbsAccS;

#if LOGVELOCITY
            // WriteLog("VelDesired="+AnsiString(VelDesired)+",
            // VelSignal="+AnsiString(VelSignal));
            WriteLog("Vel=" + AnsiString(vel) + ", AbsAccS=" + AnsiString(AbsAccS) +
                        ", AccGrav=" + AnsiString(fAccGravity));
#endif
            // ustalanie zadanego przyspieszenia
            //(ActualProximityDist) - odległość do miejsca zmniejszenia prędkości
            //(AccPreferred) - wynika z psychyki oraz uwzglęnia już ewentualne zderzenie z pojazdem z przodu, ujemne gdy należy hamować
            //(AccDesired) - uwzględnia sygnały na drodze ruchu, ujemne gdy należy hamować
            //(fAccGravity) - chwilowe przspieszenie grawitacyjne, ujemne działa przeciwnie do zadanego kierunku jazdy
            //(AbsAccS) - chwilowe przyspieszenie pojazu (uwzględnia grawitację), ujemne działa przeciwnie do zadanego kierunku jazdy
            //(AccDesired) porównujemy z (fAccGravity) albo (AbsAccS)
            if( ( VelNext >= 0.0 )
             && ( ActualProximityDist <= routescanrange )
             && ( vel >= VelNext ) ) {
                // gdy zbliża się i jest za szybki do nowej prędkości, albo stoi na zatrzymaniu
                if (vel > 0.0) {
                    // jeśli jedzie
                    if( ( vel < VelNext )
                     && ( ActualProximityDist > fMaxProximityDist * ( 1.0 + 0.1 * vel ) ) ) {
                        // jeśli jedzie wolniej niż można i jest wystarczająco daleko, to można przyspieszyć
                        if( AccPreferred > 0.0 ) {
                            // jeśli nie ma zawalidrogi dojedz do semafora/przeszkody
                            AccDesired = AccPreferred;
                        }
                    }
                    else if (ActualProximityDist > fMinProximityDist) {
                        // jedzie szybciej, niż trzeba na końcu ActualProximityDist, ale jeszcze jest daleko
						if (ActualProximityDist < fMaxProximityDist) {
                            // jak minął już maksymalny dystans po prostu hamuj (niski stopień)
                            // ma stanąć, a jest w drodze hamowania albo ma jechać
/*
                            VelDesired = min_speed( VelDesired, VelNext );
*/
                            if( VelNext == 0.0 ) {
                                if( mvOccupied->CategoryFlag & 1 ) {
                                    // trains
                                    if( ( OrderCurrentGet() & ( Shunt | Connect ) )
                                     && ( pVehicles[0]->fTrackBlock < 50.0 ) ) {
                                        // crude detection of edge case, if approaching another vehicle coast slowly until min distance
                                        // this should allow to bunch up trainsets more on sidings
                                        VelDesired = min_speed( 5.0, VelDesired );
                                    }
                                    else {
                                        // hamowanie tak, aby stanąć
                                        VelDesired = VelNext;
                                        AccDesired = ( VelNext * VelNext - vel * vel ) / ( 25.92 * ( ActualProximityDist + 0.1 - 0.5*fMinProximityDist ) );
                                        AccDesired = std::min( AccDesired, fAccThreshold );
                                    }
                                }
                                else {
                                    // for cars (and others) coast at low speed until we hit min proximity range
                                    VelDesired = min_speed( VelDesired, 5.0 );
                                }
                            }
						}
						else {
                            // outside of max safe range
                            AccDesired = AccPreferred;
                            if( vel > min_speed( (ActualProximityDist > 10.0 ? 10.0 : 5.0 ), VelDesired ) ) {
                                // allow to coast at reasonably low speed
                                auto const brakingdistance { fBrakeDist * braking_distance_multiplier( VelNext ) };
                                auto const slowdowndistance { (
                                    mvOccupied->CategoryFlag == 2 ? // cars can stop on a dime, for bigger vehicles we enforce some minimal braking distance
                                        brakingdistance :
                                        std::max(
                                            ( ( OrderCurrentGet() & Connect ) == 0 ? 100.0 : 25.0 ),
                                            brakingdistance ) ) };
                                if( ( brakingdistance + std::max( slowdowndistance, fMaxProximityDist ) ) >= ( ActualProximityDist - fMaxProximityDist ) ) {
                                    // don't slow down prematurely; as long as we have room to come to a full stop at a safe distance, we're good
                                    // ensure some minimal coasting speed, otherwise a vehicle entering this zone at very low speed will be crawling forever
                                    auto const brakingpointoffset = VelNext * braking_distance_multiplier( VelNext );
                                    AccDesired = std::min(
                                        AccDesired,
                                        ( VelNext * VelNext - vel * vel )
                                        / ( 25.92
                                            * std::max(
                                                ActualProximityDist - brakingpointoffset,
                                                std::min(
                                                    ActualProximityDist,
                                                    brakingpointoffset ) )
                                            + 0.1 ) ); // najpierw hamuje mocniej, potem zluzuje
                                }
                            }
						}
                        AccDesired = std::min( AccDesired, AccPreferred );
                    }
                    else {
                        // jest bliżej niż fMinProximityDist
                        // utrzymuj predkosc bo juz blisko
/*
                        VelDesired = min_speed( VelDesired, VelNext );
*/
                        if( VelNext == 0.0 ) {
                            VelDesired = VelNext;
                        }
                        else {
                            if( vel <= VelNext + fVelPlus ) {
                            // jeśli niewielkie przekroczenie, ale ma jechać
                                AccDesired = std::max( 0.0, AccPreferred ); // to olej (zacznij luzować)
                            }
                        }
                        ReactionTime = 0.1; // i orientuj się szybciej
                    }
                }
                else {
                    // zatrzymany (vel==0.0)
                    if( VelNext > 0.0 ) {
                        // można jechać
                        AccDesired = AccPreferred;
                    }
                    else {
                        // jeśli daleko jechać nie można
                        if( ActualProximityDist > (
                                ( mvOccupied->CategoryFlag & 2 ) ?
                                    fMinProximityDist : // cars
                                    fMaxProximityDist ) ) { // trains and others
                            // ale ma kawałek do sygnalizatora
                            if( AccPreferred > 0.0 ) {
                                // dociagnij do semafora;
                                AccDesired = AccPreferred;
                            }
                            else {
                                //stoj
                                VelDesired = 0.0;
                            }
                        }
                        else {
                            // VelNext=0 i stoi bliżej niż fMaxProximityDist
                            VelDesired = 0.0;
                        }
                    }
                }
            }
            else {
                // gdy jedzie wolniej niż potrzeba, albo nie ma przeszkód na drodze
                // normalna jazda
                AccDesired = (
                    VelDesired != 0.0 ?
                        AccPreferred :
                        -0.01 );
            }
            // koniec predkosci nastepnej

            // decisions based on current speed
            if( mvOccupied->CategoryFlag == 1 ) {

                // on flats or uphill we can be less careful
                if( vel > VelDesired ) {
                    // jesli jedzie za szybko do AKTUALNEGO
                    if( VelDesired == 0.0 ) {
                        // jesli stoj, to hamuj, ale i tak juz za pozno :)
                        AccDesired = std::min( AccDesired, -0.85 ); // hamuj solidnie
                    }
                    else {
                        // slow down, not full stop
                        if( vel > ( VelDesired + fVelPlus ) ) {
                            // hamuj tak średnio
                            AccDesired = std::min( AccDesired, -0.25 );
                        }
                        else {
                            // o 5 km/h to olej (zacznij luzować)
                            AccDesired = std::min(
                                AccDesired, // but don't override decceleration for VelNext 
                                std::max( 0.0, AccPreferred ) );
                        }
                    }
                }

                if( fAccGravity > 0.025 ) {
                    // going sharply downhill we may need to start braking sooner than usual
                    // try to estimate increase of current velocity before engaged brakes start working
                    auto const speedestimate = vel + ( 1.0 - fBrake_a0[ 0 ] ) * 30.0 * AbsAccS;
                    if( speedestimate > VelDesired ) {
                        // jesli jedzie za szybko do AKTUALNEGO
                        if( VelDesired == 0.0 ) {
                            // jesli stoj, to hamuj, ale i tak juz za pozno :)
                            AccDesired = std::min( AccDesired, -0.85 ); // hamuj solidnie
                        }
                        else {
                            // if it looks like we'll exceed maximum speed start thinking about slight slowing down
                            AccDesired = std::min( AccDesired, -0.25 );
                            // HACK: for cargo trains with high braking threshold ensure we cross that threshold
                            if( ( true == IsCargoTrain )
                             && ( fBrake_a0[ 0 ] > 0.2 ) ) {
                                AccDesired -= clamp( fBrake_a0[ 0 ] - 0.2, 0.0, 0.15 );
                            }
                        }
                    }
                    else {
                        // stop accelerating when close enough to target speed
                        AccDesired = std::min(
                            AccDesired, // but don't override decceleration for VelNext 
                            interpolate( // ease off as you close to the target velocity
                                -0.06, AccPreferred,
                                clamp( VelDesired - speedestimate, 0.0, fVelMinus ) / fVelMinus ) );
                    }
                    // final tweaks
                    if( vel > 0.1 ) {
                        // going downhill also take into account impact of gravity
                        AccDesired -= fAccGravity;
                        // HACK: if the max allowed speed was exceeded something went wrong; brake harder
                        AccDesired -= 0.15 * clamp( vel - VelDesired, 0.0, 5.0 );
/*
                        if( ( vel > VelDesired )
                         && ( ( mvOccupied->BrakeDelayFlag & bdelay_G ) != 0 )
                         && ( fBrake_a0[ 0 ] > 0.2 ) ) {
                            AccDesired = clamp(
                                AccDesired - clamp( fBrake_a0[ 0 ] - 0.2, 0.0, 0.15 ),
                                -0.9, 0.9 );
                        }
*/
                    }
                }
            }
            else {
                // for cars the older version works better
                if( vel > VelDesired ) {
                    // jesli jedzie za szybko do AKTUALNEGO
                    if( VelDesired == 0.0 ) {
                        // jesli stoj, to hamuj, ale i tak juz za pozno :)
                        AccDesired = std::min( AccDesired, -0.9 ); // hamuj solidnie
                    }
                    else {
                        // slow down, not full stop
                        if( vel > ( VelDesired + fVelPlus ) ) {
                            // hamuj tak średnio
                            AccDesired = std::min( AccDesired, -fBrake_a0[ 0 ] * 0.5 );
                        }
                        else {
                            // o 5 km/h to olej (zacznij luzować)
                            AccDesired = std::min(
                                AccDesired, // but don't override decceleration for VelNext 
                                std::max( 0.0, AccPreferred ) );
                        }
                    }
                }
            }
            // koniec predkosci aktualnej

            // last step sanity check, until the whole calculation is straightened out
            AccDesired = std::min( AccDesired, AccPreferred );
            AccDesired = clamp(
                AccDesired,
                    ( mvControlling->CategoryFlag == 2  ? -2.0 : -0.9 ),
                    ( mvControlling->CategoryFlag == 2 ?   2.0 :  0.9 ) );


			if ((-AccDesired > fBrake_a0[0] + 8 * fBrake_a1[0]) && (HelperState == 0))
			{
				HelperState = 1;
			}
			if ((-AccDesired > fBrake_a0[0] + 12 * fBrake_a1[0]) && (HelperState == 1))
			{
				HelperState = 2;
			}
			if ((-AccDesired > 0) && (HelperState == 2) && (-ActualProximityDist > 5))
			{
				HelperState = 3;
			}
			if ((-AccDesired < fBrake_a0[0] + 2 * fBrake_a1[0]) && (HelperState > 0) && (vel>1))
			{
				HelperState = 0;
			}

            if (AIControllFlag) {
                // część wykonawcza tylko dla AI, dla człowieka jedynie napisy

                // zapobieganie poslizgowi u nas
                if (mvControlling->SlippingWheels) {

                    if( false == mvControlling->DecScndCtrl( 2 ) ) {
                        // bocznik na zero
                        mvControlling->DecMainCtrl( 1 );
                    }
                    DecBrake(); // cofnij hamulec
                    mvControlling->AntiSlippingButton();
                    ++iDriverFailCount;
                }
                if (iDriverFailCount > maxdriverfails) {

                    Psyche = Easyman;
                    if (iDriverFailCount > maxdriverfails * 2)
                        SetDriverPsyche();
                }

                if( ( true == mvOccupied->RadioStopFlag ) // radio-stop
                 && ( mvOccupied->Vel > 0.0 ) ) { // and still moving
                    // if the radio-stop was issued don't waste effort trying to fight it
                    while( true == DecSpeed() ) { ; } // just throttle down...
                    return; // ...and don't touch any other controls
                }

                if( ( true == mvControlling->ConvOvldFlag ) // wywalił bezpiecznik nadmiarowy przetwornicy
                 || ( false == IsLineBreakerClosed ) ) { // WS może wywalić z powodu błędu w drutach
                    // próba ponownego załączenia
                    PrepareEngine();
                }
                // włączanie bezpiecznika
                if ((mvControlling->EngineType == TEngineType::ElectricSeriesMotor) ||
                    (mvControlling->TrainType & dt_EZT) ||
                    (mvControlling->EngineType == TEngineType::DieselElectric))
                    if (mvControlling->FuseFlag || Need_TryAgain)
                    {
                        Need_TryAgain = false; // true, jeśli druga pozycja w elektryku nie załapała
                        mvControlling->DecScndCtrl(2); // nastawnik bocznikowania na 0
                        mvControlling->DecMainCtrl(2); // nastawnik jazdy na 0
                        mvControlling->MainSwitch(true); // Ra: dodałem, bo EN57 stawały po wywaleniu
                        if (mvControlling->FuseOn()) {
                            ++iDriverFailCount;
                            if (iDriverFailCount > maxdriverfails)
                                Psyche = Easyman;
                            if (iDriverFailCount > maxdriverfails * 2)
                                SetDriverPsyche();
                        }
                    }
                // NOTE: as a stop-gap measure the routine is limited to trains only while car calculations seem off
                if( mvControlling->CategoryFlag == 1 ) {
                    if( vel < VelDesired ) {
                        // don't adjust acceleration when going above current goal speed
                        if( -AccDesired * BrakeAccFactor() < (
                            ( ( fReady > 0.4 )
                           || ( VelNext > vel - 40.0 ) ) ?
                                fBrake_a0[ 0 ] * 0.8 :
                                -fAccThreshold )
                            / braking_distance_multiplier( VelNext ) ) {
                            AccDesired = std::max( -0.06, AccDesired );
                        }
                    }
                    else {
                        // i orientuj się szybciej, jeśli hamujesz
                        ReactionTime = 0.25;
                    }
                }
                if (mvOccupied->BrakeSystem == TBrakeSystem::Pneumatic) // napełnianie uderzeniowe
                    if (mvOccupied->BrakeHandle == TBrakeHandle::FV4a)
                    {
                        if( mvOccupied->BrakeCtrlPos == -2 ) {
                            mvOccupied->BrakeLevelSet( 0 );
                        }
                        if( ( mvOccupied->PipePress < 3.0 )
                         && ( AccDesired > -0.03 ) ) {
                            mvOccupied->BrakeReleaser( 1 );
                        }

                        if( ( mvOccupied->BrakeCtrlPos == 0 )
                         && ( AbsAccS < 0.03 )
                         && ( AccDesired > -0.03 )
                         && ( VelDesired - mvOccupied->Vel > 2.0 ) ) {

                            if( ( mvOccupied->EqvtPipePress < 4.95 )
                             && ( fReady > 0.35 )
                             && ( BrakeChargingCooldown >= 0.0 ) )  {

                                if( ( iDrivigFlags & moveOerlikons )
                                 || ( true == IsCargoTrain ) ) {
                                    // napełnianie w Oerlikonie
                                    mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_FS ) );
                                    // don't charge the brakes too often, or we risk overcharging
                                    BrakeChargingCooldown = -120.0;
                                }
                            }
                            else if( Need_BrakeRelease ) {
                                Need_BrakeRelease = false;
                                mvOccupied->BrakeReleaser( 1 );
                            }
                        }

                        if( ( mvOccupied->BrakeCtrlPos < 0 )
                         && ( mvOccupied->EqvtPipePress > ( fReady < 0.25 ? 5.1 : 5.2 ) ) ) {
                            mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_RP ) );
                        }
                    }
#if LOGVELOCITY
                WriteLog("Dist=" + FloatToStrF(ActualProximityDist, ffFixed, 7, 1) +
                            ", VelDesired=" + FloatToStrF(VelDesired, ffFixed, 7, 1) +
                            ", AccDesired=" + FloatToStrF(AccDesired, ffFixed, 7, 3) +
                            ", VelSignal=" + AnsiString(VelSignal) + ", VelNext=" +
                            AnsiString(VelNext));
#endif
                if( ( vel < 10.0 )
                 && ( AccDesired > 0.1 ) ) {
                    // Ra 2F1H: jeśli prędkość jest mała, a można przyspieszać,
                    // to nie ograniczać przyspieszenia do 0.5m/ss
                    // przy małych prędkościach może być trudno utrzymać
                    AccDesired = std::max( 0.9, AccDesired );
                }
                // małe przyspieszenie
                // Ra 2F1I: wyłączyć kiedyś to uśrednianie i przeanalizować skanowanie, czemu migocze
                if (AccDesired > -0.05) // hamowania lepeiej nie uśredniać
                    AccDesired = fAccDesiredAv =
                        0.2 * AccDesired +
                        0.8 * fAccDesiredAv; // uśrednione, żeby ograniczyć migotanie
                if( VelDesired == 0.0 ) {
                    // Ra 2F1J: jeszcze jedna prowizoryczna łatka
                    if( AccDesired >= -0.01 ) {
                        AccDesired = -0.01;
                    }
                }
                if( AccDesired >= 0.0 ) {
                    if( true == TestFlag( iDrivigFlags, movePress ) ) {
                        // wyluzuj lokomotywę - może być więcej!
                        mvOccupied->BrakeReleaser( 1 );
                    }
                    else if( OrderList[ OrderPos ] != Disconnect ) {
                        // przy odłączaniu nie zwalniamy tu hamulca
                        if( ( fAccGravity * fAccGravity < 0.001 ?
                            true :
                            AccDesired > 0.0 ) ) {
                            // on slopes disengage the brakes only if you actually intend to accelerate
                            while( true == DecBrake() ) { ; } // jeśli przyspieszamy, to nie hamujemy
                            if( ( mvOccupied->BrakePress > 0.4 )
                             && ( mvOccupied->Hamulec->GetCRP() > 4.9 ) ) {
                                // wyluzuj lokomotywę, to szybciej ruszymy
                                mvOccupied->BrakeReleaser( 1 );
                            }
                            else {
                                if( mvOccupied->PipePress >= 3.0 ) {
                                    // TODO: combine all releaser handling in single decision tree instead of having bits all over the place
                                    mvOccupied->BrakeReleaser( 0 );
                                }
                            }
                        }
                    }
                }
                // yB: usunięte różne dziwne warunki, oddzielamy część zadającą od wykonawczej
                // zwiekszanie predkosci
                // Ra 2F1H: jest konflikt histerezy pomiędzy nastawioną pozycją a uzyskiwanym
                // przyspieszeniem - utrzymanie pozycji powoduje przekroczenie przyspieszenia
                if( ( AccDesired - AbsAccS > 0.01 ) ) {
                    // jeśli przyspieszenie pojazdu jest mniejsze niż żądane oraz...
                    if( vel < (
                        VelDesired == 1.0 ? // work around for trains getting stuck on tracks with speed limit = 1
                            VelDesired :
                            VelDesired - fVelMinus ) ) {
                        // ...jeśli prędkość w kierunku czoła jest mniejsza od dozwolonej o margines
                        if( ( ActualProximityDist > (
                            ( mvOccupied->CategoryFlag & 2 ) ?
                                fMinProximityDist : // cars are allowed to move within min proximity distance
                                fMaxProximityDist ) ? // other vehicle types keep wider margin
                                    true :
                                    ( vel + 1.0 ) < VelNext ) ) {
                            // to można przyspieszyć
                            IncSpeed();
                        }
                    }
                }
                // yB: usunięte różne dziwne warunki, oddzielamy część zadającą od wykonawczej
                // zmniejszanie predkosci
                // margines dla prędkości jest doliczany tylko jeśli oczekiwana prędkość jest większa od 5km/h
                if( false == TestFlag( iDrivigFlags, movePress ) ) {
                    // jeśli nie dociskanie
                    if( AccDesired < -0.05 ) {
                        while( true == DecSpeed() ) { ; } // jeśli hamujemy, to nie przyspieszamy
                    }
                    else if( ( vel > VelDesired )
                          || ( fAccGravity < -0.01 ?
                                    AccDesired < 0.0 :
                                    AbsAccS > AccDesired ) ) {
                        // jak za bardzo przyspiesza albo prędkość przekroczona
                        DecSpeed(); // pojedyncze cofnięcie pozycji, bo na zero to przesada
                    }
                }
                if( mvOccupied->TrainType == dt_EZT ) {
                    // właściwie, to warunek powinien być na działający EP
                    // Ra: to dobrze hamuje EP w EZT
                    // HACK: when going downhill be more responsive to desired deceleration
                    auto const accthreshold { (
                        fAccGravity < 0.025 ?
                            fAccThreshold :
                            std::max( -0.2, fAccThreshold ) ) };
                    if( ( AccDesired <= accthreshold ) // jeśli hamować - u góry ustawia się hamowanie na fAccThreshold
                     && ( ( AbsAccS > AccDesired )
                       || ( mvOccupied->BrakeCtrlPos < 0 ) ) ) {
                        // hamować bardziej, gdy aktualne opóźnienie hamowania mniejsze niż (AccDesired)
                        IncBrake();
                    }
                    else if( OrderList[ OrderPos ] != Disconnect ) {
                        // przy odłączaniu nie zwalniamy tu hamulca
                        if( AbsAccS < AccDesired - 0.05 ) {
                            // jeśli opóźnienie większe od wymaganego (z histerezą) luzowanie, gdy za dużo
                            if( mvOccupied->BrakeCtrlPos >= 0 ) {
                                DecBrake(); // tutaj zmniejszało o 1 przy odczepianiu
                            }
                        }
                        else if( mvOccupied->Handle->TimeEP ) {
                            if( mvOccupied->Handle->GetPos( bh_EPR ) -
                                mvOccupied->Handle->GetPos( bh_EPN ) <
                                0.1 ) {
                                mvOccupied->SwitchEPBrake( 0 );
                            }
                            else {
                                mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_EPN ) );
                            }
                        }
                    } // order != disconnect
                } // type & dt_ezt
                else {
                    // a stara wersja w miarę dobrze działa na składy wagonowe
                    if( ( ( fAccGravity < -0.05 ) && ( vel < -0.1 ) ) // brake if uphill and slipping back
                     || ( ( AccDesired < fAccGravity - 0.1 ) && ( AbsAccS > AccDesired + fBrake_a1[ 0 ] ) ) ) {
                        // u góry ustawia się hamowanie na fAccThreshold
                        if( ( fBrakeTime < 0.0 )
                         || ( AccDesired < fAccGravity - 0.5 )
                         || ( mvOccupied->BrakeCtrlPos <= 0 ) ) {
                            // jeśli upłynął czas reakcji hamulca, chyba że nagłe albo luzował
                            if( true == IncBrake() ) {
                                fBrakeTime =
                                    3.0
                                    + 0.5 * ( (
                                        mvOccupied->BrakeDelayFlag > bdelay_G ?
                                            mvOccupied->BrakeDelay[ 1 ] :
                                            mvOccupied->BrakeDelay[ 3 ] )
                                        - 3.0 );
                                // Ra: ten czas należy zmniejszyć, jeśli czas dojazdu do zatrzymania jest mniejszy
                                fBrakeTime *= 0.5; // Ra: tymczasowo, bo przeżyna S1
                            }
                        }
                    }
                    if ((AccDesired < fAccGravity - 0.05) && (AbsAccS < AccDesired - fBrake_a1[0]*0.51)) {
                        // jak hamuje, to nie tykaj kranu za często
                        // yB: luzuje hamulec dopiero przy różnicy opóźnień rzędu 0.2
                        if( OrderList[ OrderPos ] != Disconnect ) {
                            // przy odłączaniu nie zwalniamy tu hamulca
                            DecBrake(); // tutaj zmniejszało o 1 przy odczepianiu
                        }
                        fBrakeTime = (
                            mvOccupied->BrakeDelayFlag > bdelay_G ?
                                mvOccupied->BrakeDelay[ 0 ] :
                                mvOccupied->BrakeDelay[ 2 ] )
                            / 3.0;
                        fBrakeTime *= 0.5; // Ra: tymczasowo, bo przeżyna S1
                    }
                    // stop-gap measure to ensure cars actually brake to stop even when above calculactions go awry
                    // instead of releasing the brakes and creeping into obstacle at 1-2 km/h
                    if( mvControlling->CategoryFlag == 2 ) {
                        if( ( VelDesired == 0.0 )
                         && ( vel > VelDesired )
                         && ( ActualProximityDist <= fMinProximityDist )
                         && ( mvOccupied->LocalBrakePosA < 0.01 ) ) {
                            IncBrake();
                        }
                    }
                }
                // Mietek-end1
                SpeedSet(); // ciągla regulacja prędkości
#if LOGVELOCITY
                WriteLog("BrakePos=" + AnsiString(mvOccupied->BrakeCtrlPos) + ", MainCtrl=" +
                            AnsiString(mvControlling->MainCtrlPos));
#endif
            } // if (AIControllFlag)
        } // kierunek różny od zera
        else
        { // tutaj, gdy pojazd jest wyłączony
            if (!AIControllFlag) // jeśli sterowanie jest w gestii użytkownika
                if (mvOccupied->Battery) // czy użytkownik załączył baterię?
                    if (mvOccupied->ActiveDir) // czy ustawił kierunek
                    { // jeśli tak, to uruchomienie skanowania
                        CheckVehicles(); // sprawdzić skład
                        TableClear(); // resetowanie tabelki skanowania
                        PrepareEngine(); // uruchomienie
                    }
        }
        if (AIControllFlag)
        { // odhamowywanie składu po zatrzymaniu i zabezpieczanie lokomotywy
            if( ( ( OrderList[ OrderPos ] & ( Disconnect | Connect ) ) == 0 )
             && ( std::abs( fAccGravity ) < 0.01 ) ) {
                // przy (p)odłączaniu nie zwalniamy tu hamulca
                // only do this on flats, on slopes keep applied the train brake
                if( ( mvOccupied->Vel < 0.01 )
                 && ( ( VelDesired == 0.0 )
                   || ( AccDesired == 0.0 ) ) ) {
                    if( mvOccupied->BrakeCtrlPos == mvOccupied->Handle->GetPos( bh_RP ) ) {
                        // dodatkowy na pozycję 1
                        mvOccupied->IncLocalBrakeLevel( 1 );
                    }
                    else {
                        mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_RP ) );
                    }
                }
            }
        }
        break; // rzeczy robione przy jezdzie
    } // switch (OrderList[OrderPos])
}

// configures vehicle heating given current situation; returns: true if vehicle can be operated normally, false otherwise
bool
TController::UpdateHeating() {

    switch( mvControlling->EngineType ) {

        case TEngineType::DieselElectric:
        case TEngineType::DieselEngine: {

            auto const &heat { mvControlling->dizel_heat };

            // determine whether there's need to enable the water heater
            // if the heater has configured maximum temperature, it'll disable itself automatically, so we can leave it always running
            // otherwise enable the heater only to maintain minimum required temperature
            auto const lowtemperature { (
                ( ( heat.water.config.temp_min > 0 ) && ( heat.temperatura1 < heat.water.config.temp_min + ( mvControlling->WaterHeater.is_active ? 5 : 0 ) ) )
             || ( ( heat.water_aux.config.temp_min > 0 ) && ( heat.temperatura2 < heat.water_aux.config.temp_min + ( mvControlling->WaterHeater.is_active ? 5 : 0 ) ) )
             || ( ( heat.oil.config.temp_min > 0 ) && ( heat.To < heat.oil.config.temp_min + ( mvControlling->WaterHeater.is_active ? 5 : 0 ) ) ) ) };
            auto const heateron { (
                ( mvControlling->WaterHeater.config.temp_max > 0 )
             || ( true == lowtemperature ) ) };
            if( true == heateron ) {
                // make sure the water pump is running before enabling the heater
                if( false == mvControlling->WaterPump.is_active ) {
                    mvControlling->WaterPumpBreakerSwitch( true );
                    mvControlling->WaterPumpSwitch( true );
                }
                if( true == mvControlling->WaterPump.is_active ) {
                    mvControlling->WaterHeaterBreakerSwitch( true );
                    mvControlling->WaterHeaterSwitch( true );
                    mvControlling->WaterCircuitsLinkSwitch( true );
                }
            }
            else {
                // no need to heat anything up, switch the heater off
                mvControlling->WaterCircuitsLinkSwitch( false );
                mvControlling->WaterHeaterSwitch( false );
                mvControlling->WaterHeaterBreakerSwitch( false );
                // optionally turn off the water pump as well
                if( mvControlling->WaterPump.start_type != start_t::battery ) {
                    mvControlling->WaterPumpSwitch( false );
                    mvControlling->WaterPumpBreakerSwitch( false );
                }
            }

            return ( false == lowtemperature );
        }
        default: {
            return true;
        }
    }
}

void TController::JumpToNextOrder()
{ // wykonanie kolejnej komendy z tablicy rozkazów
    if (OrderList[OrderPos] != Wait_for_orders)
    {
        if (OrderList[OrderPos] & Change_direction) // jeśli zmiana kierunku
            if (OrderList[OrderPos] != Change_direction) // ale nałożona na coś
            {
                OrderList[OrderPos] =
                    TOrders(OrderList[OrderPos] &
                            ~Change_direction); // usunięcie zmiany kierunku z innej komendy
                OrderCheck();
                return;
            }
        if (OrderPos < maxorders - 1)
            ++OrderPos;
        else
            OrderPos = 0;
    }
    OrderCheck();
#if LOGORDERS
    WriteLog("--> JumpToNextOrder");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
};

void TController::JumpToFirstOrder()
{ // taki relikt
    OrderPos = 1;
    if (OrderTop == 0)
        OrderTop = 1;
    OrderCheck();
#if LOGORDERS
    WriteLog("--> JumpToFirstOrder");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
};

void TController::OrderCheck()
{ // reakcja na zmianę rozkazu

    if( OrderList[ OrderPos ] != Obey_train ) {
        // reset light hints
        m_lighthints[ side::front ] = m_lighthints[ side::rear ] = -1;
    }
    if( OrderList[ OrderPos ] & ( Shunt | Connect | Obey_train ) ) {
        CheckVehicles(); // sprawdzić światła
    }
    if( OrderList[ OrderPos ] & ( Shunt | Connect ) ) {
        // HACK: ensure consist doors will be closed on departure
        iDrivigFlags |= moveDoorOpened;
    }
    if (OrderList[OrderPos] & Change_direction) // może być nałożona na inną i wtedy ma priorytet
        iDirectionOrder = -iDirection; // trzeba zmienić jawnie, bo się nie domyśli
    else if (OrderList[OrderPos] == Obey_train)
        iDrivigFlags |= moveStopPoint; // W4 są widziane
    else if (OrderList[OrderPos] == Disconnect)
        iVehicleCount = iVehicleCount < 0 ? 0 : iVehicleCount; // odczepianie lokomotywy
    else if (OrderList[OrderPos] == Connect)
        iDrivigFlags &= ~moveStopPoint; // podczas jazdy na połączenie nie zwracać uwagi na W4
    else if (OrderList[OrderPos] == Wait_for_orders)
        OrdersClear(); // czyszczenie rozkazów i przeskok do zerowej pozycji
}

void TController::OrderNext(TOrders NewOrder)
{ // ustawienie rozkazu do wykonania jako następny
    if (OrderList[OrderPos] == NewOrder)
        return; // jeśli robi to, co trzeba, to koniec
    if (!OrderPos)
        OrderPos = 1; // na pozycji zerowej pozostaje czekanie
    OrderTop = OrderPos; // ale może jest czymś zajęty na razie
    if (NewOrder >= Shunt) // jeśli ma jechać
    { // ale może być zajęty chwilowymi operacjami
        while (OrderList[OrderTop] ? OrderList[OrderTop] < Shunt : false) // jeśli coś robi
            ++OrderTop; // pomijamy wszystkie tymczasowe prace
    }
    else
    { // jeśli ma ustawioną jazdę, to wyłączamy na rzecz operacji
        while (OrderList[OrderTop] ?
                   (OrderList[OrderTop] < Shunt) && (OrderList[OrderTop] != NewOrder) :
                   false) // jeśli coś robi
            ++OrderTop; // pomijamy wszystkie tymczasowe prace
    }
    OrderList[OrderTop++] = NewOrder; // dodanie rozkazu jako następnego
#if LOGORDERS
    WriteLog("--> OrderNext");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
}

void TController::OrderPush(TOrders NewOrder)
{ // zapisanie na stosie kolejnego rozkazu do wykonania
    if (OrderPos == OrderTop) // jeśli miałby być zapis na aktalnej pozycji
        if (OrderList[OrderPos] < Shunt) // ale nie jedzie
            ++OrderTop; // niektóre operacje muszą zostać najpierw dokończone => zapis na kolejnej
    if (OrderList[OrderTop] != NewOrder) // jeśli jest to samo, to nie dodajemy
        OrderList[OrderTop++] = NewOrder; // dodanie rozkazu na stos
    // if (OrderTop<OrderPos) OrderTop=OrderPos;
    if (OrderTop >= maxorders)
        ErrorLog("Commands overflow: The program will now crash");
#if LOGORDERS
    WriteLog("--> OrderPush: [" + Order2Str( NewOrder ) + "]");
    OrdersDump(); // normalnie nie ma po co tego wypisywać
#endif
}

void TController::OrdersDump()
{ // wypisanie kolejnych rozkazów do logu
    WriteLog("Orders for " + pVehicle->asName + ":");
    for (int b = 0; b < maxorders; ++b)
    {
        WriteLog((std::to_string(b) + ": " + Order2Str(OrderList[b]) + (OrderPos == b ? " <-" : "")));
        if (b) // z wyjątkiem pierwszej pozycji
            if (OrderList[b] == Wait_for_orders) // jeśli końcowa komenda
                break; // dalej nie trzeba
    }
};

inline TOrders TController::OrderCurrentGet()
{
    return OrderList[OrderPos];
}

inline TOrders TController::OrderNextGet()
{
    return OrderList[OrderPos + 1];
}

void TController::OrdersInit(double fVel)
{ // wypełnianie tabelki rozkazów na podstawie rozkładu
    // ustawienie kolejności komend, niezależnie kto prowadzi
    OrdersClear(); // usunięcie poprzedniej tabeli
    OrderPush(Prepare_engine); // najpierw odpalenie silnika
    if (TrainParams->TrainName == "none")
    { // brak rozkładu to jazda manewrowa
        if (fVel > 0.05) // typowo 0.1 oznacza gotowość do jazdy, 0.01 tylko załączenie silnika
            OrderPush(Shunt); // jeśli nie ma rozkładu, to manewruje
    }
    else
    { // jeśli z rozkładem, to jedzie na szlak
        if ((fVel > 0.0) && (fVel < 0.02))
            OrderPush(Shunt); // dla prędkości 0.01 włączamy jazdę manewrową
        else if (TrainParams ?
                     (TrainParams->DirectionChange() ? //  jeśli obrót na pierwszym przystanku
                          ((iDrivigFlags & movePushPull) ? // SZT również! SN61 zależnie od wagonów...
                               (TrainParams->TimeTable[1].StationName == TrainParams->Relation1) :
                               false) :
                          false) :
                     true)
            OrderPush(Shunt); // a teraz start będzie w manewrowym, a tryb pociągowy włączy W4
        else
            // jeśli start z pierwszej stacji i jednocześnie jest na niej zmiana kierunku, to EZT ma mieć Shunt
            OrderPush(Obey_train); // dla starych scenerii start w trybie pociagowym
        if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
            WriteLog("/* Timetable: " + TrainParams->ShowRelation());
        TMTableLine *t;
        for (int i = 0; i <= TrainParams->StationCount; ++i)
        {
            t = TrainParams->TimeTable + i;
            if (DebugModeFlag) {
                // normalnie nie ma po co tego wypisywa?
                WriteLog(
                    t->StationName
                    + " " + std::to_string(t->Ah) + ":" + std::to_string(t->Am)
                    + ", " + std::to_string(t->Dh) + ":" + std::to_string(t->Dm)
                    + " " + t->StationWare);
            }
            if (t->StationWare.find('@') != std::string::npos)
            { // zmiana kierunku i dalsza jazda wg rozk?adu
                if (iDrivigFlags & movePushPull) // SZT również! SN61 zależnie od wagonów...
                { // jeśli skład zespolony, wystarczy zmienić kierunek jazdy
                    OrderPush(Change_direction); // zmiana kierunku
                }
                else
                { // dla zwykłego składu wagonowego odczepiamy lokomotywę
                    OrderPush(Disconnect); // odczepienie lokomotywy
                    OrderPush(Shunt); // a dalej manewry
                }
                if (i < TrainParams->StationCount) // jak nie ostatnia stacja
                    OrderPush(Obey_train); // to dalej wg rozkładu
            }
        }
        if( DebugModeFlag ) {
            // normalnie nie ma po co tego wypisywać
            WriteLog( "*/" );
        }
        OrderPush(Shunt); // po wykonaniu rozkładu przełączy się na manewry
    }
    // McZapkie-100302 - to ma byc wyzwalane ze scenerii
    if( fVel == 0.0 ) {
        // jeśli nie ma prędkości początkowej, to śpi
        SetVelocity( 0, 0, stopSleep );
    }
    else {
        // jeśli podana niezerowa prędkość
        if( ( fVel >= 1.0 )
         || ( fVel < 0.02 ) ) {
            // jeśli ma jechać - dla 0.01 ma podjechać manewrowo po podaniu sygnału
            // to do następnego W4 ma podjechać blisko
            iDrivigFlags = ( iDrivigFlags & ~( moveStopHere ) ) | moveStopCloser;
        }
        else {
            // czekać na sygnał
            iDrivigFlags |= moveStopHere;
        }
        JumpToFirstOrder();
        if (fVel >= 1.0) // jeśli ma jechać
            SetVelocity(fVel, -1); // ma ustawić żądaną prędkość
        else
            SetVelocity(0, 0, stopSleep); // prędkość w przedziale (0;1) oznacza, że ma stać
    }
#if LOGORDERS
    WriteLog("--> OrdersInit");
#endif
    if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
        OrdersDump(); // wypisanie kontrolne tabelki rozkazów
    // McZapkie! - zeby w ogole AI ruszyl to musi wykonac powyzsze rozkazy
    // Ale mozna by je zapodac ze scenerii
};

std::string TController::StopReasonText() const
{ // informacja tekstowa o przyczynie zatrzymania
    if (eStopReason != 7) // zawalidroga będzie inaczej
        return StopReasonTable[eStopReason];
    else
        return "Blocked by " + (pVehicles[0]->PrevAny()->name());
};

//----------------------------------------------------------------------------------------------------------------------
// McZapkie: skanowanie semaforów
// Ra: stare funkcje skanujące, używane podczas manewrów do szukania sygnalizatora z tyłu
//- nie reagują na PutValues, bo nie ma takiej potrzeby
//- rozpoznają tylko zerową prędkość (jako koniec toru i brak podstaw do dalszego skanowania)
//----------------------------------------------------------------------------------------------------------------------

bool TController::BackwardTrackBusy(TTrack *Track)
{ // najpierw sprawdzamy, czy na danym torze są pojazdy z innego składu
    if( false == Track->Dynamics.empty() ) {
        for( auto dynamic : Track->Dynamics ) {
            if( dynamic->ctOwner != this ) {
                // jeśli jest jakiś cudzy to tor jest zajęty i skanowanie nie obowiązuje
                return true;
            }
        }
    }
    return false; // wolny
};

basic_event * TController::CheckTrackEventBackward(double fDirection, TTrack *Track)
{ // sprawdzanie eventu w torze, czy jest sygnałowym - skanowanie do tyłu
    // NOTE: this method returns only one event which meets the conditions, due to limitations in the caller
    // TBD, TODO: clean up the caller and return all suitable events, as in theory things will go awry if the track has more than one signal
    auto const dir{ pVehicles[ 0 ]->VectorFront() * pVehicles[ 0 ]->DirectionGet() };
    auto const pos{ pVehicles[ 0 ]->HeadPosition() };
    auto const &eventsequence { ( fDirection > 0 ? Track->m_events2 : Track->m_events1 ) };
    for( auto const &event : eventsequence ) {
        if( ( event.second != nullptr )
         && ( event.second->m_passive )
         && ( typeid(*(event.second)) == typeid( getvalues_event ) ) ) {
            // since we're checking for events behind us discard the sources in front of the scanning vehicle
            auto const sl{ event.second->input_location() }; // położenie komórki pamięci
            auto const sem{ sl - pos }; // wektor do komórki pamięci od końca składu
            if( dir.x * sem.x + dir.z * sem.z < 0 ) {
                // iloczyn skalarny jest ujemny, gdy sygnał stoi z tyłu
                return event.second;
            }
        }
    }
    return nullptr;
};

TTrack * TController::BackwardTraceRoute(double &fDistance, double &fDirection, TTrack *Track, basic_event *&Event)
{ // szukanie sygnalizatora w kierunku przeciwnym jazdy (eventu odczytu komórki pamięci)
    TTrack *pTrackChVel = Track; // tor ze zmianą prędkości
    TTrack *pTrackFrom; // odcinek poprzedni, do znajdywania końca dróg
    double fDistChVel = -1; // odległość do toru ze zmianą prędkości
    double fCurrentDistance = pVehicle->RaTranslationGet(); // aktualna pozycja na torze
    double s = 0;
    if (fDirection > 0) // jeśli w kierunku Point2 toru
        fCurrentDistance = Track->Length() - fCurrentDistance;
    if (BackwardTrackBusy(Track))
    { // jak tor zajęty innym składem, to nie ma po co skanować
        fDistance = 0; // to na tym torze stoimy
        return NULL; // stop, skanowanie nie dało sensownych rezultatów
    }
    if ((Event = CheckTrackEventBackward(fDirection, Track)) != NULL)
    { // jeśli jest semafor na tym torze
        fDistance = 0; // to na tym torze stoimy
        return Track;
    }
    if ((Track->VelocityGet() == 0.0) || (Track->iDamageFlag & 128))
    { // jak prędkosć 0 albo uszkadza, to nie ma po co skanować
        fDistance = 0; // to na tym torze stoimy
        return NULL; // stop, skanowanie nie dało sensownych rezultatów
    }
    while (s < fDistance)
    {
        // Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
        pTrackFrom = Track; // zapamiętanie aktualnego odcinka
        s += fCurrentDistance; // doliczenie kolejnego odcinka do przeskanowanej długości
        if (fDirection > 0)
        { // jeśli szukanie od Point1 w kierunku Point2
            if (Track->iNextDirection)
                fDirection = -fDirection;
            Track = Track->CurrentNext(); // może być NULL
        }
        else // if (fDirection<0)
        { // jeśli szukanie od Point2 w kierunku Point1
            if (!Track->iPrevDirection)
                fDirection = -fDirection;
            Track = Track->CurrentPrev(); // może być NULL
        }
        if (Track == pTrackFrom)
            Track = NULL; // koniec, tak jak dla torów
        if( ( Track ?
                ( ( Track->VelocityGet() == 0.0 )
               || ( Track->iDamageFlag & 128 )
               || ( true == BackwardTrackBusy( Track ) ) ) :
                true ) )
        { // gdy dalej toru nie ma albo zerowa prędkość, albo uszkadza pojazd
            fDistance = s;
            return NULL; // zwraca NULL, że skanowanie nie dało sensownych rezultatów
        }
        fCurrentDistance = Track->Length();
        if ((Event = CheckTrackEventBackward(fDirection, Track)) != NULL)
        { // znaleziony tor z eventem
            fDistance = s;
            return Track;
        }
    }
    Event = NULL; // jak dojdzie tu, to nie ma semafora
    if (fDistChVel < 0)
    { // zwraca ostatni sprawdzony tor
        fDistance = s;
        return Track;
    }
    fDistance = fDistChVel; // odległość do zmiany prędkości
    return pTrackChVel; // i tor na którym się zmienia
}

// sprawdzanie zdarzeń semaforów i ograniczeń szlakowych
void TController::SetProximityVelocity( double dist, double vel, glm::dvec3 const *pos )
{ // Ra:przeslanie do AI prędkości
    /*
     //!!!! zastąpić prawidłową reakcją AI na SetProximityVelocity !!!!
     if (vel==0)
     {//jeśli zatrzymanie, to zmniejszamy dystans o 10m
      dist-=10.0;
     };
     if (dist<0.0) dist=0.0;
     if ((vel<0)?true:dist>0.1*(MoverParameters->Vel*MoverParameters->Vel-vel*vel)+50)
     {//jeśli jest dalej od umownej drogi hamowania
    */
    PutCommand( "SetProximityVelocity", dist, vel, pos );
    /*
     }
     else
     {//jeśli jest zagrożenie, że przekroczy
      Mechanik->SetVelocity(floor(0.2*sqrt(dist)+vel),vel,stopError);
     }
     */
}

TCommandType TController::BackwardScan()
{ // sprawdzanie zdarzeń semaforów z tyłu pojazdu, zwraca komendę
    // dzięki temu będzie można stawać za wskazanym sygnalizatorem, a zwłaszcza jeśli będzie jazda na kozioł
    // ograniczenia prędkości nie są wtedy istotne, również koniec toru jest do niczego nie przydatny
    // zwraca true, jeśli należy odwrócić kierunek jazdy pojazdu
    if( ( OrderList[ OrderPos ] & ~( Shunt | Connect ) ) ) {
        // skanowanie sygnałów tylko gdy jedzie w trybie manewrowym albo czeka na rozkazy
        return TCommandType::cm_Unknown;
    }
    // kierunek jazdy względem sprzęgów pojazdu na czele
    int const startdir = -pVehicles[0]->DirectionGet();
    if( startdir == 0 ) {
        // jeśli kabina i kierunek nie jest określony nie robimy nic
        return TCommandType::cm_Unknown;
    }
    // szukamy od pierwszej osi w wybranym kierunku
    double scandir = startdir * pVehicles[0]->RaDirectionGet();
    if (scandir != 0.0) {
        // skanowanie toru w poszukiwaniu eventów GetValues (PutValues nie są przydatne)
        // Ra: przy wstecznym skanowaniu prędkość nie ma znaczenia
        double scanmax = 1000; // 1000m do tyłu, żeby widział przeciwny koniec stacji
        double scandist = scanmax; // zmodyfikuje na rzeczywiście przeskanowane
        basic_event *e = NULL; // event potencjalnie od semafora
        // opcjonalnie może być skanowanie od "wskaźnika" z przodu, np. W5, Tm=Ms1, koniec toru wg drugiej osi w kierunku ruchu
        TTrack *scantrack = BackwardTraceRoute(scandist, scandir, pVehicles[0]->RaTrackGet(), e);
        auto const dir = startdir * pVehicles[0]->VectorFront(); // wektor w kierunku jazdy/szukania
        if( !scantrack ) {
            // jeśli wstecz wykryto koniec toru to raczej nic się nie da w takiej sytuacji zrobić
            return TCommandType::cm_Unknown;
        }
        else {
            // a jeśli są dalej tory
            double vmechmax; // prędkość ustawiona semaforem
            if( e != nullptr ) {
                // jeśli jest jakiś sygnał na widoku
#if LOGBACKSCAN
                std::string edir {
                    "Backward scan by "
                    + pVehicle->asName + " - "
                    + ( ( scandir > 0 ) ?
                        "Event2 " :
                        "Event1 " ) };
#endif
                // najpierw sprawdzamy, czy semafor czy inny znak został przejechany
                auto pos = pVehicles[1]->RearPosition(); // pozycja tyłu
                if( typeid( *e ) == typeid( getvalues_event ) )
                { // przesłać info o zbliżającym się semaforze
#if LOGBACKSCAN
                    edir += "(" + ( e->asNodeName ) + ")";
#endif
                    auto sl = e->input_location(); // położenie komórki pamięci
                    auto sem = sl - pos; // wektor do komórki pamięci od końca składu
                    if (dir.x * sem.x + dir.z * sem.z < 0) {
                        // jeśli został minięty
                        // iloczyn skalarny jest ujemny, gdy sygnał stoi z tyłu
#if LOGBACKSCAN
                        WriteLog(edir + " - ignored as not passed yet");
#endif
                        return TCommandType::cm_Unknown; // nic
                    }
                    vmechmax = e->input_value(1); // prędkość przy tym semaforze
                    // przeliczamy odległość od semafora - potrzebne by były współrzędne początku składu
                    scandist = sem.Length() - 2; // 2m luzu przy manewrach wystarczy
                    if( scandist < 0 ) {
                        // ujemnych nie ma po co wysyłać
                        scandist = 0;
                    }
                    bool move = false; // czy AI w trybie manewerowym ma dociągnąć pod S1
                    if( e->input_command() == TCommandType::cm_SetVelocity ) {
                        if( ( vmechmax == 0.0 ) ?
                                ( OrderCurrentGet() & ( Shunt | Connect ) ) :
                                ( OrderCurrentGet() & Connect ) ) { // przy podczepianiu ignorować wyjazd?
                            move = true; // AI w trybie manewerowym ma dociągnąć pod S1
                        }
                        else {
                            if( ( scandist > fMinProximityDist )
                             && ( ( mvOccupied->Vel > 0.0 )
                               && ( OrderCurrentGet() != Shunt ) ) ) {
                                // jeśli semafor jest daleko, a pojazd jedzie, to informujemy o zmianie prędkości
                                // jeśli jedzie manewrowo, musi dostać SetVelocity, żeby sie na pociągowy przełączył
#if LOGBACKSCAN
                                // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist) + AnsiString(vmechmax));
                                WriteLog(edir);
#endif
                                // SetProximityVelocity(scandist,vmechmax,&sl);
                                return (
                                    vmechmax > 0 ?
                                        TCommandType::cm_SetVelocity :
                                        TCommandType::cm_Unknown );
                            }
                            else {
                                // ustawiamy prędkość tylko wtedy, gdy ma ruszyć, stanąć albo ma stać
                                // if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeśli stoi lub ma stanąć/stać
                                // semafor na tym torze albo lokomtywa stoi, a ma ruszyć, albo ma stanąć, albo nie ruszać
                                // stop trzeba powtarzać, bo inaczej zatrąbi i pojedzie sam
                                // PutCommand("SetVelocity",vmechmax,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                                WriteLog(
                                    edir + " - [SetVelocity] ["
                                    + to_string( vmechmax, 2 ) + "] ["
                                    + to_string( e->Params[ 9 ].asMemCell->Value2(), 2 ) + "]" );
#endif
                                return (
                                    vmechmax > 0 ?
                                        TCommandType::cm_SetVelocity :
                                        TCommandType::cm_Unknown );
                            }
                        }
                    }
                    if (OrderCurrentGet() ? OrderCurrentGet() & (Shunt | Connect) :
                                            true) // w Wait_for_orders też widzi tarcze
                    { // reakcja AI w trybie manewrowym dodatkowo na sygnały manewrowe
                        if (move ? true : e->input_command() == TCommandType::cm_ShuntVelocity)
                        { // jeśli powyżej było SetVelocity 0 0, to dociągamy pod S1
                            if ((scandist > fMinProximityDist) ?
                                    (mvOccupied->Vel > 0.0) || (vmechmax == 0.0) :
                                    false)
                            { // jeśli tarcza jest daleko, to:
                                //- jesli pojazd jedzie, to informujemy o zmianie prędkości
                                //- jeśli stoi, to z własnej inicjatywy może podjechać pod zamkniętą
                                // tarczę
                                if (mvOccupied->Vel > 0.0) // tylko jeśli jedzie
                                { // Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGBACKSCAN
                                    // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+"
                                    // "+AnsiString(vmechmax));
                                    WriteLog(edir);
#endif
                                    // SetProximityVelocity(scandist,vmechmax,&sl);
                                    return (iDrivigFlags & moveTrackEnd) ?
                                               TCommandType::cm_ChangeDirection :
                                               TCommandType::cm_Unknown; // jeśli jedzie na W5 albo koniec toru,
                                    // to można zmienić kierunek
                                }
                            }
                            else {
                                // ustawiamy prędkość tylko wtedy, gdy ma ruszyć, albo stanąć albo ma stać pod tarczą
                             // stop trzeba powtarzać, bo inaczej zatrąbi i pojedzie sam
                                // if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeśli jedzie lub ma stanąć/stać
                                { // nie dostanie komendy jeśli jedzie i ma jechać
                                  // PutCommand("ShuntVelocity",vmechmax,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                                    WriteLog(
                                        edir + " - [ShuntVelocity] ["
                                        + to_string( vmechmax, 2 ) + "] ["
                                        + to_string( e->value( 2 ), 2 ) + "]" );
#endif
                                    return (
                                        vmechmax > 0 ?
                                            TCommandType::cm_ShuntVelocity :
                                            TCommandType::cm_Unknown );
                                }
                            }
                            if ((vmechmax != 0.0) && (scandist < 100.0)) {
                                // jeśli Tm w odległości do 100m podaje zezwolenie na jazdę, to od razu ją ignorujemy, aby móc szukać kolejnej
                                // eSignSkip=e; //wtedy uznajemy ignorowaną przy poszukiwaniu nowej
#if LOGBACKSCAN
                                WriteLog(edir + " - will be ignored due to Ms2");
#endif
                                return (
                                    vmechmax > 0 ?
                                        TCommandType::cm_ShuntVelocity :
                                        TCommandType::cm_Unknown );
                            }
                        } // if (move?...
                    } // if (OrderCurrentGet()==Shunt)
                    if (e->m_passive) // jeśli skanowany
                        if (e->is_command()) // a podłączona komórka ma komendę
                            return TCommandType::cm_Command; // to też się obrócić
                } // if (e->Type==tp_GetValues)
            } // if (e)
        } // if (scantrack)
    } // if (scandir!=0.0)
    return TCommandType::cm_Unknown; // nic
};

std::string TController::NextStop() const
{ // informacja o następnym zatrzymaniu, wyświetlane pod [F1]
    if (asNextStop == "[End of route]")
        return ""; // nie zawiera nazwy stacji, gdy dojechał do końca
    // dodać godzinę odjazdu
    if (!TrainParams)
        return ""; // tu nie powinno nigdy wejść
    std::string nextstop = asNextStop;
    TMTableLine *t = TrainParams->TimeTable + TrainParams->StationIndex;
    if( t->Ah >= 0 ) {
        // przyjazd
        nextstop += " przyj." + std::to_string( t->Ah ) + ":"
      + ( t->Am < 10 ? "0" : "" ) + std::to_string( t->Am );
    }
    if( t->Dh >= 0 ) {
        // jeśli jest godzina odjazdu
        nextstop += " odj." + std::to_string( t->Dh ) + ":"
      + ( t->Dm < 10 ? "0" : "" ) + std::to_string( t->Dm );
    }
    return nextstop;
};

void TController::UpdateDelayFlag() {

    if( TrainParams->CheckTrainLatency() < 0.0 ) {
        // odnotowano spóźnienie
        iDrivigFlags |= moveLate;
    }
    else {
        // przyjazd o czasie
        iDrivigFlags &= ~moveLate;
    }
}

//-----------koniec skanowania semaforow

void TController::TakeControl(bool yes)
{ // przejęcie kontroli przez AI albo oddanie
    if (AIControllFlag == yes)
        return; // już jest jak ma być
    if (yes) //żeby nie wykonywać dwa razy
    { // teraz AI prowadzi
        AIControllFlag = AIdriver;
        pVehicle->Controller = AIdriver;
        iDirection = 0; // kierunek jazdy trzeba dopiero zgadnąć
        // gdy zgaszone światła, flaga podjeżdżania pod semafory pozostaje bez zmiany
        // conditional below disabled to get around the situation where the AI train does nothing ever
        // because it is waiting for orders which don't come until the engine is engaged, i.e. effectively never
        if (OrderCurrentGet()) // jeśli coś robi
            PrepareEngine(); // niech sprawdzi stan silnika
        else // jeśli nic nie robi
        if (pVehicle->iLights[mvOccupied->CabNo < 0 ? 1 : 0] &
                21) // któreś ze świateł zapalone?
        { // od wersji 357 oczekujemy podania komend dla AI przez scenerię
            OrderNext(Prepare_engine);
            if (pVehicle->iLights[mvOccupied->CabNo < 0 ? 1 : 0] & 4) // górne światło zapalone
                OrderNext(Obey_train); // jazda pociągowa
            else
                OrderNext(Shunt); // jazda manewrowa
            if (mvOccupied->Vel >= 1.0) // jeśli jedzie (dla 0.1 ma stać)
                iDrivigFlags &= ~moveStopHere; // to ma nie czekać na sygnał, tylko jechać
            else
                iDrivigFlags |= moveStopHere; // a jak stoi, to niech czeka
        }
        /* od wersji 357 oczekujemy podania komend dla AI przez scenerię
          if (OrderCurrentGet())
          {if (OrderCurrentGet()<Shunt)
           {OrderNext(Prepare_engine);
            if (pVehicle->iLights[mvOccupied->CabNo<0?1:0]&4) //górne światło
             OrderNext(Obey_train); //jazda pociągowa
            else
             OrderNext(Shunt); //jazda manewrowa
           }
          }
          else //jeśli jest w stanie Wait_for_orders
           JumpToFirstOrder(); //uruchomienie?
          // czy dac ponizsze? to problematyczne
          //SetVelocity(pVehicle->GetVelocity(),-1); //utrzymanie dotychczasowej?
          if (pVehicle->GetVelocity()>0.0)
           SetVelocity(-1,-1); //AI ustali sobie odpowiednią prędkość
        */
        // Activation(); //przeniesie użytkownika w ostatnio wybranym kierunku
        CheckVehicles(); // ustawienie świateł
        TableClear(); // ponowne utworzenie tabelki, bo człowiek mógł pojechać niezgodnie z
        // sygnałami
    }
    else
    { // a teraz użytkownik
        AIControllFlag = Humandriver;
        pVehicle->Controller = Humandriver;
    }
};

void TController::DirectionForward(bool forward)
{ // ustawienie jazdy do przodu dla true i do tyłu dla false (zależy od kabiny)
    while (mvControlling->MainCtrlPos) // samo zapętlenie DecSpeed() nie wystarcza
        DecSpeed(true); // wymuszenie zerowania nastawnika jazdy, inaczej się może zawiesić
    if (forward)
        while (mvOccupied->ActiveDir <= 0)
            mvOccupied->DirectionForward(); // do przodu w obecnej kabinie
    else
        while (mvOccupied->ActiveDir >= 0)
            mvOccupied->DirectionBackward(); // do tyłu w obecnej kabinie
    if( mvOccupied->TrainType == dt_SN61 ) {
        // specjalnie dla SN61 żeby nie zgasł
        if( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn == 0 ) {
            mvControlling->IncMainCtrl( 1 );
        }
    }
};

Mtable::TTrainParameters const *
TController::TrainTimetable() const {
    return TrainParams;
}

std::string TController::Relation() const
{ // zwraca relację pociągu
    return TrainParams->ShowRelation();
};

std::string TController::TrainName() const
{ // zwraca numer pociągu
    return TrainParams->TrainName;
};

int TController::StationCount() const
{ // zwraca ilość stacji (miejsc zatrzymania)
    return TrainParams->StationCount;
};

int TController::StationIndex() const
{ // zwraca indeks aktualnej stacji (miejsca zatrzymania)
    return TrainParams->StationIndex;
};

bool TController::IsStop() const
{ // informuje, czy jest zatrzymanie na najbliższej stacji
    return TrainParams->IsStop();
};

// returns most recently calculated distance to potential obstacle ahead
double
TController::TrackBlock() const {

    return pVehicles[ side::front ]->fTrackBlock;
}

void TController::MoveTo(TDynamicObject *to)
{ // przesunięcie AI do innego pojazdu (przy zmianie kabiny)
    // mvOccupied->CabDeactivisation(); //wyłączenie kabiny w opuszczanym
    pVehicle->Mechanik = to->Mechanik; //żeby się zamieniły, jak jest jakieś drugie
    pVehicle = to;
    ControllingSet(); // utworzenie połączenia do sterowanego pojazdu
    pVehicle->Mechanik = this;
    // iDirection=0; //kierunek jazdy trzeba dopiero zgadnąć
};

void TController::ControllingSet()
{ // znajduje człon do sterowania w EZT będzie to silnikowy
    // problematyczne jest sterowanie z członu biernego, dlatego damy AI silnikowy
    // dzięki temu będzie wirtualna kabina w silnikowym, działająca w rozrządczym
    // w plikach FIZ zostały zgubione ujemne maski sprzęgów, stąd problemy z EZT
    mvOccupied = pVehicle->MoverParameters; // domyślny skrót do obiektu parametrów
    mvControlling = pVehicle->ControlledFind()->MoverParameters; // poszukiwanie członu sterowanego
};

std::string TController::TableText( std::size_t const Index ) const
{ // pozycja tabelki prędkości
    if( Index < sSpeedTable.size() ) {
        return sSpeedTable[ Index ].TableText();
    }
    else {
        return "";
    }
};

int TController::CrossRoute(TTrack *tr)
{ // zwraca numer segmentu dla skrzyżowania (tr)
    // pożądany numer segmentu jest określany podczas skanowania drogi
    // droga powinna być określona sposobem przejazdu przez skrzyżowania albo współrzędnymi miejsca
    // docelowego
    for( std::size_t i = 0; i < sSpeedTable.size(); ++i )
    { // trzeba przejrzeć tabelę skanowania w poszukiwaniu (tr)
        // i jak się znajdzie, to zwrócić zapamiętany numer segmentu i kierunek przejazdu
        // (-6..-1,1..6)
        if( ( true == TestFlag( sSpeedTable[ i ].iFlags, spEnabled | spTrack ) )
         && ( sSpeedTable[ i ].trTrack == tr ) ) {
            // jeśli pozycja odpowiadająca skrzyżowaniu (tr)
            return ( sSpeedTable[ i ].iFlags >> 28 ); // najstarsze 4 bity jako liczba -8..7
        }
    }
    return 0; // nic nie znaleziono?
};
/*
void TController::RouteSwitch(int d)
{ // ustawienie kierunku jazdy z kabiny
    d &= 3;
    if( ( d != 0 )
     && ( iRouteWanted != d ) ) { // nowy kierunek
        iRouteWanted = d; // zapamiętanie
        if( mvOccupied->CategoryFlag & 2 ) {
            // jeśli samochód
            for( std::size_t i = 0; i < sSpeedTable.size(); ++i ) {
                // szukanie pierwszego skrzyżowania i resetowanie kierunku na nim
                if( true == TestFlag( sSpeedTable[ i ].iFlags, spEnabled | spTrack ) ) {
                    // jeśli pozycja istotna (1) oraz odcinek (2)
                    if( false == TestFlag( sSpeedTable[ i ].iFlags, spElapsed ) ) {
                        // odcinek nie może być miniętym
                        if( sSpeedTable[ i ].trTrack->eType == tt_Cross ) // jeśli skrzyżowanie
                        {
                            while( sSpeedTable.size() >= i ) {
                                // NOTE: we're ignoring semaphor flags and not resetting them like we do for train route trimming
                                // but what if there's street lights?
                                // TODO: investigate
                                sSpeedTable.pop_back();
                            }
                            iLast = sSpeedTable.size();
                        }
                    }
                }
            }
        }
    }
};
*/
std::string TController::OwnerName() const
{
    return ( pVehicle ? pVehicle->MoverParameters->Name : "none" );
};
