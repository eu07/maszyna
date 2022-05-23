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
#include "translation.h"
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
const double PassengetTrainAcceleration = 0.40;
const double HeavyPassengetTrainAcceleration = 0.20;
const double CargoTrainAcceleration = 0.25;
const double HeavyCargoTrainAcceleration = 0.10;
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
    case TCommandType::cm_EmergencyBrake:
        fVelNext = -1;
        break;
    case TCommandType::cm_SecuritySystemMagnet:
        fVelNext = -1;
        break;
    default:
        // inna komenda w evencie skanowanym powoduje zatrzymanie i wysłanie tej komendy
        // nie manewrowa, nie przystanek, nie zatrzymać na SBL
        // jak nieznana komenda w komórce sygnałowej, to zatrzymujemy
        fVelNext = 0.0;
/*
        fVelNext = (
            evEvent->is_command() ? 0.0 : // ask for a stop if we have a command for the vehicle
            ( iFlags & ( spSemaphor | spShuntSemaphor )) != 0 ? fVelNext : // don't change semafor velocity
            -1.0 ); // otherwise don't be a bother
*/
        // TODO: check whether clearing spShuntSemaphor flag doesn't cause problems
        // potentially it can invalidate shunt semaphor used to transmit timetable or similar command
        iFlags &= ~(spShuntSemaphor | spPassengerStopPoint | spStopOnSBL);
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
    if( iFlags & spTrack ) // jeśli tor
        return trTrack->name();
    else if( iFlags & spEvent ) // jeśli event
        return
            evEvent->m_name
            + " [" + to_string( static_cast<int>( evEvent->input_value( 1 ) ) )
            + ", " + to_string( static_cast<int>( evEvent->input_value( 2 ) ) )
            + "]";
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
	if (order < Obey_train) // Wait_for_orders, Prepare_engine, Change_direction, Connect, Disconnect, Shunt
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
    // ignore events located behind the consist, but with exception of stop points which may be needed to update freshly received timetable
    if( ( dist + length < 0 )
     && ( event->input_command() != TCommandType::cm_PassengerStopPoint ) ) {
        return false;
    }
    iFlags |= spEnabled;
    CommandCheck(); // sprawdzenie typu komendy w evencie i określenie prędkości
	// zależnie od trybu sprawdzenie czy jest tutaj gdzieś semafor lub tarcza manewrowa
	// jeśli wskazuje stop wtedy wystawiamy true jako koniec sprawdzania
	// WriteLog("EventSet: Vel=" + AnsiString(fVelNext) + " iFlags=" + AnsiString(iFlags) + " order="+AnsiString(order));
	if (order < Obey_train) // Wait_for_orders, Prepare_engine, Change_direction, Connect, Disconnect, Shunt
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

bool TController::TableNotFound(basic_event const *Event, double const Distance ) const
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

    // ignore duplicates which seem to be reasonably apart from each other, on account of looping tracks
    // NOTE: since supplied distance is only rough approximation of distance to the event, we're using large safety margin
    return (
        ( lookup == sSpeedTable.end() )
     || ( Distance - lookup->fDist > 100.0 ) );
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
        TableClear();
/*
        // aktualna prędkość // changed to -1 to recognize speed limit, if any
        fLastVel = -1.0;
        sSpeedTable.clear();
        iLast = -1;
        tLast = nullptr; //żaden nie sprawdzony
        SemNextIndex = -1;
        SemNextStopIndex = -1;
*/
        if( VelSignalLast == 0.0 ) {
            // don't allow potential red light overrun keep us from reversing
            VelSignalLast = -1.0;
        }
        iTableDirection = iDirection; // ustalenie w jakim kierunku jest wypełniana tabelka względem pojazdu
        pTrack = pVehicle->GetTrack(); // odcinek, na którym stoi
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
                if( ( OrderCurrentGet() < Obey_train )
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
                    if (TableNotFound(pEvent, fCurrentDistance)) // jeśli nie ma
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
/*
                    // NOTE: manual selection disabled due to multiplayer
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
*/
                    auto const routewanted { 1 + std::floor( Random( static_cast<double>( pTrack->RouteCount() ) - 0.001 ) ) };
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
                if( false == TestFlag( sSpeedTable[ iLast ].trTrack->iCategoryFlag, 0x100 ) ) {
                    // don't mark portals, as these aren't exactly track ends, but teleport devices
                    sSpeedTable[ iLast ].iFlags |= ( spEnabled | spEnd );
                }
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
        TableTraceRoute( fDistance, pVehicles[ end::rear ] );
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
                    TableTraceRoute( fDistance, pVehicles[ end::rear ] );
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
                        if ((mvOccupied->CategoryFlag == 2) && (sSpeedTable[i].fDist < -0.75))
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
            TableTraceRoute( fDistance, pVehicles[ end::rear ] ); // doskanowanie dalszego odcinka
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
    auto a { 0.0 }; // przyspieszenie
    auto v { 0.0 }; // prędkość
    auto d { 0.0 }; // droga
    auto d_to_next_sem { 10000.0 }; //ustaiwamy na pewno dalej niż widzi AI
    auto go { TCommandType::cm_Unknown };
    auto speedlimitiscontinuous { true }; // stays true if potential active speed limit is unbroken to the last (relevant) point in scan table
    eSignNext = nullptr;
    IsAtPassengerStop = false;
    IsScheduledPassengerStopVisible = false;
    mvOccupied->SecuritySystem.set_cabsignal_lock(false);
    // te flagi są ustawiane tutaj, w razie potrzeby
    iDrivigFlags &= ~(moveTrackEnd | moveSwitchFound | moveSignalFound | /*moveSpeedLimitFound*/ moveStopPointFound );

    for( std::size_t idx = 0; idx < sSpeedTable.size(); ++idx ) {
        // o ile dana pozycja tabelki jest istotna
        if( ( sSpeedTable[ idx ].iFlags & spEnabled ) == 0 ) { continue; }

        auto &point { sSpeedTable[ idx ] };

        if( TestFlag( point.iFlags, spPassengerStopPoint ) ) {
            if( TableUpdateStopPoint( go, point, d_to_next_sem ) ) { continue; };
        }
        v = point.fVelNext; // odczyt prędkości do zmiennej pomocniczej
        if( TestFlag( point.iFlags, spSwitch ) ) {
            // zwrotnice są usuwane z tabelki dopiero po zjechaniu z nich
            iDrivigFlags |= moveSwitchFound; // rozjazd z przodu/pod ogranicza np. sens skanowania wstecz
            SwitchClearDist = point.fDist + point.trTrack->Length() + fLength;
        }
        else if ( TestFlag( point.iFlags, spEvent ) ) // W4 może się deaktywować
        { // jeżeli event, może być potrzeba wysłania komendy, aby ruszył
            if( TableUpdateEvent( v, go, point, d_to_next_sem, idx ) ) { continue; }
        }

        auto const railwaytrackend { ( true == TestFlag( point.iFlags, spEnd ) ) && ( is_train() ) };
        if( ( v >= 0.0 ) // pozycje z prędkością -1 można spokojnie pomijać
         || ( railwaytrackend ) ) {

            d = point.fDist;

            if( v >= 0.0 ) {
                // points located in front of us can potentially affect our acceleration and target speed
                if( ( d > 0.0 )
                 && ( false == TestFlag( point.iFlags, spElapsed ) ) ) {
                    // sygnał lub ograniczenie z przodu (+32=przejechane)
                    // 2014-02: jeśli stoi, a ma do przejechania kawałek, to niech jedzie
                    if( ( mvOccupied->Vel < 0.01 )
                     && ( true == TestFlag( point.iFlags, ( spEnabled | spEvent | spPassengerStopPoint ) ) )
                     && ( false == IsAtPassengerStop ) ) {
                        // ma podjechać bliżej - czy na pewno w tym miejscu taki warunek?
                        a = ( ( d > passengerstopmaxdistance ) || ( ( iDrivigFlags & moveStopCloser ) != 0 ) ?
                                fAcc :
                                0.0 );
                    }
                    else {
                        // przyspieszenie: ujemne, gdy trzeba hamować
                        if( v >= 0.0 ) {
                            a = ( v * v - mvOccupied->Vel * mvOccupied->Vel ) / ( 25.92 * d );
                            if( ( mvOccupied->Vel < v )
                             || ( v == 0.0 ) ) {
                                // if we're going slower than the target velocity and there's enough room for safe stop, speed up
                                auto const brakingdistance { 1.2 * fBrakeDist * braking_distance_multiplier( v ) };
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
                }
                // points located behind can affect our current speed, but little else
                else if ( point.iFlags & spTrack) // jeśli tor
                { // tor ogranicza prędkość, dopóki cały skład nie przejedzie,
                    if( v >= 1.0 ) { // EU06 się zawieszało po dojechaniu na koniec toru postojowego
                        if( d + point.trTrack->Length() < -fLength ) {
                            if( false == railwaytrackend ) {
                                 // zapętlenie, jeśli już wyjechał za ten odcinek
                                continue;
                            }
                        }
                    }
                    if( v < fVelDes ) {
                        // ograniczenie aktualnej prędkości aż do wyjechania za ograniczenie
                        fVelDes = v;
                    }
                    if( v < VelLimitLastDist.first ) {
                        VelLimitLastDist.second = d + point.trTrack->Length() + fLength;
                    }
                    else if( VelLimitLastDist.second > 0 ) { // the speed limit can potentially start afterwards, so don't mark it as broken too soon
                        if( ( point.iFlags & ( spSwitch | spPassengerStopPoint ) ) == 0 ) {
                            speedlimitiscontinuous = false;
                        }
                    }
                    if( false == railwaytrackend ) {
                        continue; // i tyle wystarczy
                    }
                }
                else {
                    // event trzyma tylko jeśli VelNext=0, nawet po przejechaniu (nie powinno dotyczyć samochodów?)
                    a = (v > 0.0 ?
                        fAcc :
                        mvOccupied->Vel < 0.01 ?
                            0.0 : // already standing still so no need to bother with brakes
                           -2.0 ); // ruszanie albo hamowanie
                }
            }
            // track can potentially end, which creates another virtual point of interest with speed limit of 0 at the end of it
            // TBD, TODO: when tracing the route create a dedicated table entry for it, to simplify the code?
            if( railwaytrackend ) {
                // if the railway track ends here set the velnext accordingly as well
                // TODO: test this with turntables and such
                auto const stopatendacceleration = ( -1.0 * mvOccupied->Vel * mvOccupied->Vel ) / ( 25.92 * ( d + point.trTrack->Length() ) );
                if( stopatendacceleration < a ) {
                    a = stopatendacceleration;
                    v = 0.0;
                    d += point.trTrack->Length();
                    if( d < fMinProximityDist ) {
                        // jak jest już blisko, ograniczenie aktualnej prędkości
                        fVelDes = v;
                    }
                }
            }

            if( ( a <= fAcc )
             && ( ( v < fNext ) || ( fNext < 0 ) ) ) { // filter out consecutive, farther out blocks with the same speed limit; they'd make us accelerate slower due to their lower a value
                // mniejsze przyspieszenie to mniejsza możliwość rozpędzenia się albo konieczność hamowania
                // jeśli droga wolna, to może być a>1.0 i się tu nie załapuje
                fAcc = a; // zalecane przyspieszenie (nie musi być uwzględniane przez AI)
                fNext = v; // istotna jest prędkość na końcu tego odcinka
                fDist = d; // dlugość odcinka
            }
/*
            else if ((fAcc > 0) && (v >= 0) && (v > fNext)) {
                // jeśli nie ma wskazań do hamowania, można podać drogę i prędkość na jej końcu
                fNext = v; // istotna jest prędkość na końcu tego odcinka
                fDist = d; // dlugość odcinka (kolejne pozycje mogą wydłużać drogę, jeśli prędkość jest stała)
            }
*/
            // we'll pick first scanned target speed as our goal
            // farther scan table points can override it through previous clause, if they require lower acceleration or speed reduction
            else if( ( a > 0 ) && ( a <= fAcc ) && ( v >= 0 ) && ( fNext < 0 ) ) {
                fAcc = a;
                fNext = v;
                fDist = d;
            }
            // potentially update our current speed limit
            if( ( v < VelLimitLastDist.first )
             && ( ( d < 0 ) // the point counts as part of last speed limit either if it's behind us
               || ( VelLimitLastDist.second > 0 ) ) ) { // or if we already have the last limit ongoing
                VelLimitLastDist.second = d + fLength;
                if( ( point.iFlags & spTrack ) != 0 ) {
                    VelLimitLastDist.second += point.trTrack->Length();
                }
            }
            else {
                // if we found a point which isn't part of the current speed limit mark the limit as broken
                // NOTE: we exclude switches from this rule, as speed limits generally extend through these
                if( ( point.iFlags & ( spSwitch | spPassengerStopPoint ) ) == 0 ) {
                    speedlimitiscontinuous = false;
                }
            }
        } // if (v>=0.0)
        // finding a point which doesn't impose speed limit means any potential active speed limit can't extend through entire scan table
        // NOTE: we test actual speed value for the given point, as events may receive (v = -1) as a way to disable them in irrelevant modes
        if( point.fVelNext < 0 ) {
            // NOTE: we exclude switches from this rule, as speed limits generally extend through these
            if( ( point.iFlags & ( spSwitch | spPassengerStopPoint ) ) == 0 ) {
                speedlimitiscontinuous = false;
            }
        }
        if (fNext >= 0.0)
        { // jeśli ograniczenie
            if( ( point.iFlags & ( spEnabled | spEvent ) ) == ( spEnabled | spEvent ) ) { // tylko sygnał przypisujemy
                if( eSignNext == nullptr ) { // jeśli jeszcze nic nie zapisane tam
                    eSignNext = point.evEvent; // dla informacji
                }
            }
            if( fNext == 0.0 ) {
                break; // nie ma sensu analizować tabelki dalej
            }
        }
    }

    // jeśli mieliśmy ograniczenie z semafora i nie ma przed nami
    if( ( VelSignalLast >= 0.0 )
     && ( ( iDrivigFlags & ( moveSignalFound | moveSwitchFound | moveStopPointFound ) ) == 0 )
       && ( true == TestFlag( OrderCurrentGet(), Obey_train ) ) ) {
        VelSignalLast = -1.0;
    }
    // if there's unbroken speed limit through our scan table take a note of it
    if( ( VelLimitLastDist.second > 0 )
     && ( speedlimitiscontinuous ) ) {
        VelLimitLastDist.second = EU07_AI_SPEEDLIMITEXTENDSBEYONDSCANRANGE;
    }
    // take into account the effect switches have on duration of signal-imposed speed limit, in calculation of speed limit end point
    if( ( VelSignalLast >= 0.0 ) && ( SwitchClearDist >= 0.0 ) ) {
        VelLimitLastDist.second = std::max( VelLimitLastDist.second, SwitchClearDist );
    }

    //analiza spisanych z tabelki ograniczeń i nadpisanie aktualnego
    // if stopped at a valid passenger stop, hold there
    if( ( true == IsAtPassengerStop ) && ( mvOccupied->Vel < 0.01 ) ) {
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

bool
TController::TableUpdateStopPoint( TCommandType &Command, TSpeedPos &Point, double const Signaldistance ) {
    // stop points are irrelevant when not in one of the basic modes
    if( ( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank ) ) == 0 ) { return true; }
    // jeśli przystanek, trzeba obsłużyć wg rozkładu
    iDrivigFlags |= moveStopPointFound;
    // first 19 chars of the command is expected to be "PassengerStopPoint:" so we skip them
    if( ToLower( Point.evEvent->input_text() ).compare( 19, sizeof( asNextStop ), ToLower( asNextStop ) ) != 0 )
    { // jeśli nazwa nie jest zgodna
        if( ( false == IsScheduledPassengerStopVisible ) // check if our next scheduled stop didn't show up earlier in the scan
         && ( Point.fDist < ( 1.15 * fBrakeDist + 300 ) )
         && ( Point.fDist > 0 ) ) // tylko jeśli W4 jest blisko, przy dwóch może zaczać szaleć
        {
            // porównuje do następnej stacji, więc trzeba przewinąć do poprzedniej
            // nastepnie ustawić następną na aktualną tak żeby prawidłowo ją obsłużył w następnym kroku
            if( true == TrainParams.RewindTimeTable( Point.evEvent->input_text() ) ) {
                asNextStop = TrainParams.NextStop();
                TrainParams.StationStart = TrainParams.StationIndex;
            }
        }
        else if( Point.fDist < -fLength ) {
            // jeśli został przejechany
            Point.iFlags = 0; // to można usunąć (nie mogą być usuwane w skanowaniu)
        }
        return true; // ignorowanie jakby nie było tej pozycji
    }
    else if (iDrivigFlags & moveStopPoint) // jeśli pomijanie W4, to nie sprawdza czasu odjazdu
    { // tylko gdy nazwa zatrzymania się zgadza
        if( ( OrderCurrentGet() & ( Obey_train | Bank ) ) != 0 ) {
            // check whether the station specifies radio channel change
            // NOTE: we don't do it in shunt mode, as shunting operations tend to use dedicated radio channel
            // NOTE: there's a risk radio channel change was specified by a station which we skipped during timetable rewind
            // we ignore this for the time being as it's not a high priority error
            auto const radiochannel { TrainParams.radio_channel() };
            if( radiochannel > 0 ) {
                if( iGuardRadio != 0 ) {
                    iGuardRadio = radiochannel;
                }
                if( iRadioChannel != radiochannel ) {
                    cue_action( driver_hint::radiochannel, radiochannel );
                }
            }
        }
        IsScheduledPassengerStopVisible = true; // block potential timetable rewind if the next stop shows up later in the scan
        if (false == TrainParams.IsStop())
        { // jeśli nie ma tu postoju
            Point.fVelNext = -1; // maksymalna prędkość w tym miejscu
            // przy 160km/h jedzie 44m/s, to da dokładność rzędu 5 sekund
            if (Point.fDist < passengerstopmaxdistance * 0.5 ) {
                // zaliczamy posterunek w pewnej odległości przed (choć W4 nie zasłania już semafora)
#if LOGSTOPS
                WriteLog(
                    pVehicle->asName + " as " + TrainParams.TrainName
                    + ": at " + std::to_string(simulation::Time.data().wHour) + ":" + std::to_string(simulation::Time.data().wMinute)
                    + " passed " + asNextStop); // informacja
#endif
                // przy jakim dystansie (stanie licznika) ma przesunąć na następny postój
                fLastStopExpDist = mvOccupied->DistCounter + 0.250 + 0.001 * fLength;
                TrainParams.UpdateMTable( simulation::Time, asNextStop );
                UpdateDelayFlag();
                TrainParams.StationIndexInc(); // przejście do następnej
                asNextStop = TrainParams.NextStop(); // pobranie kolejnego miejsca zatrzymania
                Point.iFlags = 0; // nie liczy się już
                return true; // table entry recognized and handled
            }
        } // koniec obsługi przelotu na W4
        else {
            // zatrzymanie na W4
            if ( false == Point.bMoved ) {
                // potentially shift the stop point in accordance with its defined parameters
                /*
                // https://rainsted.com/pl/Wersja/18.2.133#Okr.C4.99gi_dla_W4_i_W32
                Pierwszy parametr ujemny - preferowane zatrzymanie czoła składu (np. przed przejściem).
                Pierwszy parametr dodatni - preferowane zatrzymanie środka składu (np. przy wiacie, przejściu podziemnym).
                Drugi parametr ujemny - wskazanie zatrzymania dla krótszych składów (W32).
                Drugi paramer dodatni - długość peronu (W4).
                */
                auto L = 0.0;
				auto Par1 = Point.evEvent->input_value(1);
				auto Par2 = Point.evEvent->input_value(2);
				if ((Par2 >= 0) || (fLength < -Par2)) { //użyj tego W4
					if (Par1 < 0) {
                        L = -Par1;
                    }
                    else {
                        //środek
                        L = Par1 - fMinProximityDist - fLength * 0.5;
                    }
					L = std::max(0.0, std::min(L, std::abs(Par2) - fMinProximityDist - fLength));
					Point.UpdateDistance(L);
					Point.bMoved = true;
					Point.fMoved = L;
                }
                else {
					Point.iFlags = 0;
                }
            }
            // for human-driven vehicles discard the stop point if they leave it far enough behind
            if( ( false == AIControllFlag )
             && ( Point.fDist < -1 * std::max( fLength + 100, 250.0 ) - Point.fMoved ) ) {
                Point.iFlags = 0; // nie liczy się już zupełnie (nie wyśle SetVelocity)
                Point.fVelNext = -1; // można jechać za W4
                if( ( Point.fDist <= 0.0 ) && ( eSignNext == Point.evEvent ) ) {
                    // sanity check, if we're held by this stop point, let us go
                    VelSignalLast = -1;
                }
                return true;
            }

            IsAtPassengerStop = (
                ( Point.fDist <= passengerstopmaxdistance )
                // Ra 2F1I: odległość plus długość pociągu musi być mniejsza od długości
                // peronu, chyba że pociąg jest dłuższy, to wtedy minimalna.
                // jeśli długość peronu ((sSpeedTable[i].evEvent->ValueGet(2)) nie podana,
                // przyjąć odległość fMinProximityDist
             && ( ( iDrivigFlags & moveStopCloser ) != 0 ?
                    Point.fDist + fLength + (Point.fMoved - fMinProximityDist * 0.5f) <=
                    std::max(
                        std::abs( Point.evEvent->input_value( 2 ) ),
                        2.0 * fMaxProximityDist + fLength ) : // fmaxproximitydist typically equals ~50 m
                    Point.fDist < Signaldistance ) );

            if( !eSignNext ) {
                //jeśli nie widzi następnego sygnału ustawia dotychczasową
                eSignNext = Point.evEvent;
            }
            if( mvOccupied->Vel > EU07_AI_MOVEMENT ) {
                // jeśli jedzie (nie trzeba czekać, aż się drgania wytłumią - drzwi zamykane od 1.0) to będzie zatrzymanie
                Point.fVelNext = 0;
                // potentially announce pending stop
                if( ( m_lastannouncement != announcement_t::approaching )
                 && ( Point.fDist < 750 )
                 && ( Point.fDist > 250 ) ) {
                    announce( announcement_t::approaching );
                }
            } else if( true == IsAtPassengerStop ) {
                // jeśli się zatrzymał przy W4, albo stał w momencie zobaczenia W4
                if( !AIControllFlag ) {
                    // w razie przełączenia na AI ma nie podciągać do W4, gdy użytkownik zatrzymał za daleko
                    iDrivigFlags &= ~moveStopCloser;
                }
                if (TrainParams.UpdateMTable( simulation::Time, asNextStop) ) {
                    // to się wykona tylko raz po zatrzymaniu na W4
                    if( TrainParams.StationIndex < TrainParams.StationCount ) {
                        // jeśli są dalsze stacje, bez trąbienia przed odjazdem
                        // also ignore any horn cue that may be potentially set below 1 km/h and before the actual full stop
                        iDrivigFlags &= ~( moveStartHorn | moveStartHornNow );
                    }
                    UpdateDelayFlag();

                    // perform loading/unloading
                    // HACK: manual check if we didn't already do load exchange at this stop
                    // TODO: remove the check once the station system is in place
                    if( m_lastexchangestop != asNextStop ) {
                        m_lastexchangestop = asNextStop;
                        m_lastexchangedirection = pVehicle->DirectionGet();
                        m_lastexchangeplatforms = static_cast<int>( std::floor( std::abs( Point.evEvent->input_value( 2 ) ) ) ) % 10;
                        auto const exchangetime { simulation::Station.update_load( pVehicles[ end::front ], TrainParams, m_lastexchangeplatforms ) };
                        WaitingSet( exchangetime );
                        // announce the stop name while at it
                        announce( announcement_t::current );
                        m_makenextstopannouncement = true;
                    }

                    if (TrainParams.DirectionChange()) {
                        // jeśli "@" w rozkładzie, to wykonanie dalszych komend
                        // wykonanie kolejnej komendy, nie dotyczy ostatniej stacji
                        if (iDrivigFlags & movePushPull) {
                            // SN61 ma się też nie ruszać, chyba że ma wagony
                            iDrivigFlags |= moveStopHere; // EZT ma stać przy peronie
                            if (OrderNextGet() != Change_direction) {
                                OrderPush(Change_direction); // zmiana kierunku
                                OrderPush(
                                    TrainParams.StationIndex < TrainParams.StationCount ?
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
                        Point.iFlags = 0;
                        // jechać
                        Point.fVelNext = -1;
                        // nie analizować prędkości
                        return true;
                    }
                }

                if (OrderCurrentGet() & ( Shunt | Loose_shunt )) {
                    OrderNext(Obey_train); // uruchomić jazdę pociągową
                    CheckVehicles(); // zmienić światła
                }

                if (TrainParams.StationIndex < TrainParams.StationCount) {
                    // jeśli są dalsze stacje, czekamy do godziny odjazdu
                    if ( ( true == IsCargoTrain )
                      || ( true == TrainParams.IsMaintenance() )
                      || ( TrainParams.IsTimeToGo( simulation::Time.data().wHour, simulation::Time.data().wMinute + simulation::Time.data().wSecond*0.0167 ) ) ) {
                        // z dalszą akcją czekamy do godziny odjazdu
                        // cargo trains and passenger trains at maintenance stop don't need to wait
                        IsAtPassengerStop = false;
                        // przy jakim dystansie (stanie licznika) ma przesunąć na następny postój
                        fLastStopExpDist = mvOccupied->DistCounter + 0.050 + 0.001 * fLength;
                        TrainParams.StationIndexInc(); // przejście do następnej
                        asNextStop = TrainParams.NextStop(); // pobranie kolejnego miejsca zatrzymania
#if LOGSTOPS
                        WriteLog(
                            pVehicle->asName + " as " + TrainParams.TrainName
                            + ": at " + std::to_string(simulation::Time.data().wHour) + ":" + std::to_string(simulation::Time.data().wMinute)
                            + " next " + asNextStop); // informacja
#endif
                        // update consist weight, brake settings and ai braking tables
                        // NOTE: this calculation is expected to run after completing loading/unloading
                        CheckVehicles(); // nastawianie hamulca do jazdy pociągowej

                        if( static_cast<int>( std::floor( std::abs( Point.evEvent->input_value( 1 ) ) ) ) % 2 ) {
                            // nie podjeżdżać do semafora, jeśli droga nie jest wolna
                            iDrivigFlags |= moveStopHere;
                        }
                        else {
                            //po czasie jedź dalej
                            iDrivigFlags &= ~moveStopHere;
                        }
                        iDrivigFlags |= moveStopCloser; // do następnego W4 podjechać blisko (z dociąganiem)
                        Point.iFlags = 0; // nie liczy się już zupełnie (nie wyśle SetVelocity)
                        Point.fVelNext = -1; // można jechać za W4
                        if( ( Point.fDist <= 0.0 ) && ( eSignNext == Point.evEvent ) ) {
                            // sanity check, if we're held by this stop point, let us go
                            VelSignalLast = -1;
                        }
                        if( Command == TCommandType::cm_Unknown ) { // jeśli nie było komendy wcześniej
                            Command = TCommandType::cm_Ready; // gotów do odjazdu z W4 (semafor może zatrzymać)
                        }
                        if( false == tsGuardSignal.empty() ) {
                            // jeśli mamy głos kierownika, to odegrać
                            iDrivigFlags |= moveGuardSignal;
                        }
                        return true; // nie analizować prędkości
                    } // koniec startu z zatrzymania
					else {
						cue_action(driver_hint::waitdeparturetime);
						if (TrainParams.IsTimeToGo(simulation::Time.data().wHour, simulation::Time.data().wMinute + simulation::Time.data().wSecond*0.0167 + 0.1)) { //if is less than 6 sec to departure
							iDrivigFlags |= moveGuardOpenDoor;
							GuardOpenDoor();
						}
					}
                } // koniec obsługi początkowych stacji
                else {
                    // jeśli dojechaliśmy do końca rozkładu
#if LOGSTOPS
                    WriteLog(
                        pVehicle->asName + " as " + TrainParams.TrainName
                        + ": at " + std::to_string(simulation::Time.data().wHour) + ":" + std::to_string(simulation::Time.data().wMinute)
                        + " end of route."); // informacja
#endif
                    asNextStop = TrainParams.NextStop(); // informacja o końcu trasy
                    TrainParams.NewName("none"); // czyszczenie nieaktualnego rozkładu
                    // ma nie podjeżdżać pod W4 i ma je pomijać
                    iDrivigFlags &= ~( moveStopCloser );
                    if( false == TestFlag( iDrivigFlags, movePushPull ) ) {
                        // if the consist can change direction through a simple cab change it doesn't need fiddling with recognition of passenger stops
                        iDrivigFlags &= ~( moveStopPoint );
                    }
                    fLastStopExpDist = -1.0f; // nie ma rozkładu, nie ma usuwania stacji
                    Point.iFlags = 0; // W4 nie liczy się już (nie wyśle SetVelocity)
                    Point.fVelNext = -1; // można jechać za W4
                    if( ( Point.fDist <= 0.0 ) && ( eSignNext == Point.evEvent ) ) {
                        // sanity check, if we're held by this stop point, let us go
                        VelSignalLast = -1;
                    }
                    // wykonanie kolejnego rozkazu (Change_direction albo Shunt)
                    JumpToNextOrder();
                    // ma się nie ruszać aż do momentu podania sygnału
                    iDrivigFlags |= moveStopHere | moveStartHorn;
                    return true; // nie analizować prędkości
                } // koniec obsługi ostatniej stacji
            } // vel 0, at passenger stop
            else {
                // HACK: momentarily deactivate W4 to trick the controller into moving closer
                Point.fVelNext = -1;
            } // vel 0, outside of passenger stop
        } // koniec obsługi zatrzymania na W4
    } // koniec warunku pomijania W4 podczas zmiany czoła
    else
    { // skoro pomijanie, to jechać i ignorować W4
        Point.iFlags = 0; // W4 nie liczy się już (nie zatrzymuje jazdy)
        Point.fVelNext = -1;
        return true; // nie analizować prędkości
    }

    return false; //
}

bool
TController::TableUpdateEvent( double &Velocity, TCommandType &Command, TSpeedPos &Point, double &Signaldistance, int const Pointindex ) {

    // sprawdzanie eventów pasywnych miniętych
    if( Point.fDist < 0.0 ) {
        if( SemNextIndex == Pointindex ) {
            if( Global.iWriteLogEnabled & 8 ) {
                WriteLog( "Speed table update for " + OwnerName() + ", passed semaphor " + sSpeedTable[ SemNextIndex ].GetName() );
            }
            SemNextIndex = -1; // jeśli minęliśmy semafor od ograniczenia to go kasujemy ze zmiennej sprawdzającej dla skanowania w przód
        }
        if( SemNextStopIndex == Pointindex ) {
            if( Global.iWriteLogEnabled & 8 ) {
                WriteLog( "Speed table update for " + OwnerName() + ", passed semaphor " + sSpeedTable[ SemNextStopIndex ].GetName() );
            }
            SemNextStopIndex = -1; // jeśli minęliśmy semafor od ograniczenia to go kasujemy ze zmiennej sprawdzającej dla skanowania w przód
        }
        switch( Point.evEvent->input_command() ) {
            // TBD, TODO: expand emergency_brake handling to a more generic security system signal
            case TCommandType::cm_EmergencyBrake: {
                pVehicle->RadioStop();
                Point.Clear(); // signal received, deactivate
                return true;
            }
            case TCommandType::cm_SecuritySystemMagnet: {
                // NOTE: magnet induction calculation presumes the driver is located in the front vehicle
                // TBD, TODO: take into account actual position of controlled/occupied vehicle in the consist, whichever comes first
                auto const magnetlocation { pVehicles[ end::front ]->MoverParameters->SecuritySystem.MagnetLocation };
                auto const magnetrange { 1.0 };
                auto const ismagnetpassed { Point.fDist < -( magnetlocation + magnetrange ) };
                if( Point.fDist < -( magnetlocation ) ) {
                    // NOTE: normally we'd activate the magnet once the leading vehicle passes it
                    // but on a fresh scan after direction change it would be detected as long as it's under consist, and meet the (simple) activation condition
                    // thus we're doing a more precise check in such situation (we presume direction change takes place only if the vehicle is standing still)
                    if( ( mvOccupied->Vel > EU07_AI_NOMOVEMENT )
                     || ( false == ismagnetpassed ) ) {
                        mvOccupied->SecuritySystem.set_cabsignal_lock(!AIControllFlag); // don't make life difficult for the ai, but a human driver is a fair game
                        PutCommand(
                            Point.evEvent->input_text(),
                            Point.evEvent->input_value( 1 ),
                            Point.evEvent->input_value( 2 ),
                            nullptr );
                    }
                }
                if( ismagnetpassed ) {
                    Point.Clear();
                    mvOccupied->SecuritySystem.set_cabsignal_lock(false);
                }
                return true;
            }
            default: {
                break;
            }
        }
    }
    // check signals ahead
    if( Point.fDist > 0.0 ) {
        // bail out early for signals which activate when passed
        // TBD: make it an event method?
        switch( Point.evEvent->input_command() ) {
            case TCommandType::cm_EmergencyBrake: {
                return true;
            }
            case TCommandType::cm_SecuritySystemMagnet: {
                return true;
            }
            default: {
                break;
            }
        }

        if( Point.IsProperSemaphor( OrderCurrentGet() ) ) {
            // special rule for cars: ignore stop signals at distance too short to come to a stop
            // as trying to stop in such situation is likely to place the car on train tracks
            if( ( is_car() )
             && ( Point.fVelNext != -1.0 )
             && ( Point.fVelNext  <  1.0 )
             && ( Point.fDist < -0.5 + std::min( fBrakeDist * 0.2, mvOccupied->Vel * 0.2 ) ) ) {
                Point.Clear();
                return true;
            }
            // jeśli jest mienięty poprzedni semafor a wcześniej
            // byl nowy to go dorzucamy do zmiennej, żeby cały czas widział najbliższy
            if( SemNextIndex == -1 ) {
                SemNextIndex = Pointindex;
                if( Global.iWriteLogEnabled & 8 ) {
                    WriteLog( "Speed table update for " + OwnerName() + ", next semaphor is " + sSpeedTable[ SemNextIndex ].GetName() );
                }
            }
            if( ( SemNextStopIndex == -1 )
             || ( ( sSpeedTable[ SemNextStopIndex ].fVelNext != 0 )
               && ( Point.fVelNext == 0 ) ) ) {
                SemNextStopIndex = Pointindex;
            }
        }
    }
    if (Point.iFlags & spOutsideStation)
    { // jeśli W5, to reakcja zależna od trybu jazdy
        if (OrderCurrentGet() & Obey_train)
        { // w trybie pociągowym: można przyspieszyć do wskazanej prędkości (po zjechaniu z rozjazdów)
            Velocity = -1.0; // ignorować?
			if ( Point.fDist < 0.0) { // jeśli wskaźnik został minięty
                VelSignalLast = Velocity; //ustawienie prędkości na -1
            }
            else if( ( iDrivigFlags & moveSwitchFound ) == 0 ) { // jeśli rozjazdy już minięte
                VelSignalLast = Velocity; //!!! to też koniec ograniczenia
            }
        }
        else
        { // w trybie manewrowym: skanować od niego wstecz, stanąć po wyjechaniu za sygnalizator i zmienić kierunek
            Velocity = 0.0; // zmiana kierunku może być podanym sygnałem, ale wypadało by zmienić światło wcześniej
            if( ( iDrivigFlags & moveSwitchFound ) == 0 ) { // jeśli nie ma rozjazdu
                // check for presence of a signal facing the opposite direction
                // if there's one, we'll want to pass it before changing direction
                basic_event *foundevent = nullptr;
                if( Point.fDist - fMaxProximityDist > 0 ) {
                    auto scandistance{ Point.fDist + fLength - fMaxProximityDist };
                    auto *scanvehicle{ pVehicles[ end::rear ] };
                    auto scandirection{ scanvehicle->DirectionGet() * scanvehicle->RaDirectionGet() };
                    auto *foundtrack = BackwardTraceRoute( scandistance, scandirection, scanvehicle, foundevent, -1, end::front, false );
                }
                if( foundevent == nullptr ) {
                    iDrivigFlags |= moveTrackEnd; // to dalsza jazda trwale ograniczona (W5, koniec toru)
                }
            }
        }
    }
    else if ( Point.iFlags & spStopOnSBL) {
        // jeśli S1 na SBL
        if( mvOccupied->Vel < 2.0 ) {
            // stanąć nie musi, ale zwolnić przynajmniej
            // jest w maksymalnym zasięgu to można go pominąć (wziąć drugą prędkosć)
            // as long as there isn't any obstacle in arbitrary view range
            if( ( Point.fDist < fMaxProximityDist )
             && ( Obstacle.distance > 1000 ) ) {
                eSignSkip = Point.evEvent;
                // jazda na widoczność - skanować możliwość kolizji i nie podjeżdżać zbyt blisko
                // usunąć flagę po podjechaniu blisko semafora zezwalającego na jazdę
                // ostrożnie interpretować sygnały - semafor może zezwalać na jazdę pociągu z przodu!
                iDrivigFlags |= moveVisibility;
                // store the ordered restricted speed and don't exceed it until the flag is cleared
                VelRestricted = Point.evEvent->input_value( 2 );
            }
        }
        if( eSignSkip != Point.evEvent ) {
            // jeśli ten SBL nie jest do pominięcia to ma 0 odczytywać
            Velocity = Point.evEvent->input_value( 1 );
            // TODO sprawdzić do której zmiennej jest przypisywane v i zmienić to tutaj
        }
    }
    else if ( Point.IsProperSemaphor(OrderCurrentGet()))
    { // to semaphor
        if( Point.fDist < 0 ) {
            // for human-driven vehicles ignore the signal if it was passed by sufficient distance
            if( ( false == AIControllFlag )
             && ( Point.fDist < -1 * std::max( fLength + 100, 250.0 ) ) ) {
                VelSignal = -1.0;
                Point.Clear();
                return true;
            }
            // for ai-driven vehicles always play by the rules
            else {
                VelSignalLast = Point.fVelNext; //minięty daje prędkość obowiązującą
            }
        }
        else {
            //jeśli z przodu to dajemy flagę, że jest
			iDrivigFlags |= moveSignalFound;
            Signaldistance = std::min( Point.fDist, Signaldistance );
            // if there's another vehicle closer to the signal, then it's likely its intended recipient
            // HACK: if so, make it a stop point, to prevent non-signals farther down affect us
            auto const isforsomeoneelse { ( is_train() ) && ( Obstacle.distance < Point.fDist ) };
            if( Point.fDist <= Signaldistance ) {
                if( isforsomeoneelse ) {
                    VelSignalNext = 0.0;
                    Velocity = 0.0;
                }
                else {
                    VelSignalNext = Point.fVelNext;
                    if( Velocity < 0 ) {
                        Velocity = fVelMax;
                        VelSignal = fVelMax;
                    }
                }
            }
        }
    }
    else if ( Point.iFlags & spRoadVel)
    { // to W6
        if ( Point.fDist < 0)
            VelRoad = Point.fVelNext;
    }
    else if ( Point.iFlags & spSectionVel)
    { // to W27
        if ( Point.fDist < 0) // teraz trzeba sprawdzić inne warunki
        {
            if ( Point.fSectionVelocityDist == 0.0) {
                if( Global.iWriteLogEnabled & 8 ) {
                    WriteLog( "TableUpdate: Event is behind. SVD = 0: " + Point.evEvent->m_name );
                }
                Point.Clear(); // jeśli punktowy to kasujemy i nie dajemy ograniczenia na stałe
            }
            else if ( Point.fSectionVelocityDist < 0.0) {
                // ograniczenie obowiązujące do następnego
                if ( (Point.fVelNext == min_speed( Point.fVelNext, VelLimitLast))
                  && (Point.fVelNext != VelLimitLast)) {
                    // jeśli ograniczenie jest mniejsze niż obecne to obowiązuje od zaraz
                    VelLimitLast = Point.fVelNext;
                }
                else if ( Point.fDist < -fLength) {
                    // jeśli większe to musi wyjechać za poprzednie
                    VelLimitLast = Point.fVelNext;
                    if( Global.iWriteLogEnabled & 8 ) {
                        WriteLog( "TableUpdate: Event is behind. SVD < 0: " + Point.evEvent->m_name );
                    }
                    Point.Clear(); // wyjechaliśmy poza poprzednie, można skasować
                }
            }
            else
            { // jeśli większe to ograniczenie ma swoją długość
                if ( (Point.fVelNext == min_speed( Point.fVelNext, VelLimitLast))
                  && (Point.fVelNext != VelLimitLast)) {
                    // jeśli ograniczenie jest mniejsze niż obecne to obowiązuje od zaraz
                    VelLimitLast = Point.fVelNext;
                }
                else if ( (Point.fDist < -fLength)
                       && (Point.fVelNext != VelLimitLast)) {
                    // jeśli większe to musi wyjechać za poprzednie
                    VelLimitLast = Point.fVelNext;
                }
                else if (Point.fDist < -fLength - Point.fSectionVelocityDist) {
                    VelLimitLast = -1.0;
                    if( Global.iWriteLogEnabled & 8 ) {
                        WriteLog( "TableUpdate: Event is behind. SVD > 0: " + Point.evEvent->m_name );
                    }
                    Point.Clear(); // wyjechaliśmy poza poprzednie, można skasować
                }
            }
        }
    }

    //sprawdzenie eventów pasywnych przed nami
    { // zawalidrogi nie ma (albo pojazd jest samochodem), sprawdzić sygnał
        if ( Point.iFlags & spShuntSemaphor) // jeśli Tm - w zasadzie to sprawdzić komendę!
        { // jeśli podana prędkość manewrowa
            if( ( Velocity == 0.0 )
             && ( true == TestFlag( OrderCurrentGet(), Obey_train ) ) ) {
                // jeśli tryb pociągowy a tarcze ma ShuntVelocity 0 0
                Velocity = -1; // ignorować, chyba że prędkość stanie się niezerowa
                if( true == TestFlag( Point.iFlags, spElapsed ) ) {
                    // a jak przejechana to można usunąć, bo podstawowy automat usuwa tylko niezerowe
                    Point.Clear();
                }
            }
            else if( Command == TCommandType::cm_Unknown ) {
                // jeśli jeszcze nie ma komendy
                // komenda jest tylko gdy ma jechać, bo stoi na podstawie tabelki
                if( Velocity != 0.0 ) {
                    // jeśli nie było komendy wcześniej - pierwsza się liczy - ustawianie VelSignal
                    Command = TCommandType::cm_ShuntVelocity; // w trybie pociągowym tylko jeśli włącza tryb manewrowy (v!=0.0)
                    // Ra 2014-06: (VelSignal) nie może być tu ustawiane, bo Tm może być daleko
                    if( VelSignal == 0.0 ) {
                        // aby stojący ruszył
                        VelSignal = Velocity;
                    }
                    if( Point.fDist < 0.0 ) {
                        // jeśli przejechany
                        //!!! ustawienie, gdy przejechany jest lepsze niż wcale, ale to jeszcze nie to
                        VelSignal = Velocity;
                        // to można usunąć (nie mogą być usuwane w skanowaniu)
                        Point.Clear();
                    }
                }
            }
        }
        else if( ( Point.iFlags & spSectionVel ) == 0 ) {
            //jeśli jakiś event pasywny ale nie ograniczenie
            if( Command == TCommandType::cm_Unknown ) {
                // jeśli nie było komendy wcześniej - pierwsza się liczy - ustawianie VelSignal
                if( ( Velocity < 0.0 )
                 || ( Velocity >= 1.0 ) ) {
                    // bo wartość 0.1 służy do hamowania tylko
                    Command = TCommandType::cm_SetVelocity; // może odjechać
                    // Ra 2014-06: (VelSignal) nie może być tu ustawiane, bo semafor może być daleko
                    // VelSignal=v; //nie do końca tak, to jest druga prędkość; -1 nie wpisywać...
                    if( VelSignal == 0.0 ) {
                        // aby stojący ruszył
                        VelSignal = -1.0;
                    }
                    if( Point.fDist < 0.0 ) {
                        // jeśli przejechany
                        VelSignal = ( Velocity == 0.0 ? 0.0 : -1.0 );
                        // ustawienie, gdy przejechany jest lepsze niż wcale, ale to jeszcze nie to
                        if( Point.iFlags & spEvent ) {
                            // jeśli event
                            if( ( Point.evEvent != eSignSkip )
                             || ( Point.fVelNext != VelRestricted ) ) {
                                // ale inny niż ten, na którym minięto S1, chyba że się już zmieniło
                                // sygnał zezwalający na jazdę wyłącza jazdę na widoczność (po S1 na SBL)
                                iDrivigFlags &= ~moveVisibility;
                                // remove restricted speed
                                VelRestricted = -1.0;
                            }
                        }
                        // jeśli nie jest ograniczeniem prędkości to można usunąć
                        // (nie mogą być usuwane w skanowaniu)
                        Point.Clear();
                    }
                }
                else if( Point.evEvent->is_command() ) {
                    // jeśli prędkość jest zerowa, a komórka zawiera komendę
                    eSignNext = Point.evEvent; // dla informacji
                    // make sure the command isn't for someone else
                    // TBD, TODO: revise this check for bank/loose_shunt; we might want to ignore obstacles with no owner in these modes
                    if( Point.fDist < Obstacle.distance ) {
                        if( true == TestFlag( iDrivigFlags, moveStopHere ) ) {
                            // jeśli ma stać, dostaje komendę od razu
                            Command = TCommandType::cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                        }
                        // NOTE: human drivers are likely to stop wherever and expect things to still work, so we give them more leeway
                        else if( Point.fDist <= ( AIControllFlag ? fMaxProximityDist : std::max( 250.0, fMaxProximityDist ) ) ) {
                            // jeśli ma dociągnąć, to niech dociąga
                            // (moveStopCloser dotyczy dociągania do W4, nie semafora)
                            Command = TCommandType::cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                        }
                    }
                }
            }
        }
    } // jeśli nie ma zawalidrogi

    return false;
}

// modifies brake distance for low target speeds, to ease braking rate in such situations
float
TController::braking_distance_multiplier( float const Targetvelocity ) const {

    auto const frictionmultiplier { 1.f / Global.FrictionWeatherFactor };

    if( Targetvelocity > 65.f ) { return 1.f * frictionmultiplier; }
    if( Targetvelocity < 5.f ) {
        // HACK: engaged automatic transmission means extra/earlier braking effort is needed for the last leg before full stop
        if( ( is_dmu() )
         && ( mvOccupied->Vel < 40.0 )
         && ( Targetvelocity == 0.f ) ) {
            auto const multiplier { clamp( 1.f + iVehicles * 0.5f, 2.f, 4.f ) };
            return (
                interpolate(
                    multiplier, 1.f,
                    static_cast<float>( mvOccupied->Vel / 40.0 ) )
                * frictionmultiplier );
        }
        // HACK: cargo trains or trains going downhill with high braking threshold need more distance to come to a full stop
        if( ( fBrake_a0[ 1 ] > 0.2 )
         && ( ( true == IsCargoTrain )
           || ( fAccGravity > 0.025 ) ) ) {
            return (
                interpolate(
                    1.f, 2.f,
                    clamp(
                        ( fBrake_a0[ 1 ] - 0.2 ) / 0.2,
                        0.0, 1.0 ) )
                * frictionmultiplier );
        }

        return 1.f * frictionmultiplier;
    }
    // stretch the braking distance up to 3 times; the lower the speed, the greater the stretch
    return (
        interpolate(
            3.f, 1.f,
            ( Targetvelocity - 5.f ) / 60.f )
        * frictionmultiplier );
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

TController::TController(bool AI, TDynamicObject *NewControll, bool InitPsyche, bool Primary) :// czy ma aktywnie prowadzić?
              AIControllFlag( AI ),     pVehicle( NewControll )
{
    if( pVehicle != nullptr ) {
        pVehicles[ end::front ] = pVehicle->GetFirstDynamic( end::front ); // pierwszy w kierunku jazdy (Np. Pc1)
        pVehicles[ end::rear ] = pVehicle->GetFirstDynamic( end::rear ); // ostatni w kierunku jazdy (końcówki)
    }
    else {
        pVehicles[ end::front ] = nullptr;
        pVehicles[ end::rear ] = nullptr;
    }
    ControllingSet(); // utworzenie połączenia do sterowanego pojazdu
    if( mvOccupied != nullptr ) {
        iDirectionOrder = mvOccupied->CabActive; // 1=do przodu (w kierunku sprzęgu 0)
        VehicleName = mvOccupied->Name;

        if( is_car() ) { // samochody: na podst. http://www.prawko-kwartnik.info/hamowanie.html
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
            is_emu() ? -0.55 :
            is_dmu() ? -0.45 :
            -0.2 );
/*
        // HACK: emu with induction motors need to start their braking a bit sooner than the ones with series motors
        if( ( mvOccupied->TrainType == dt_EZT )
         && ( mvControlling->EngineType == TEngineType::ElectricInductionMotor ) ) {
            fAccThreshold += 0.10;
        }
*/
    }
    // TrainParams=NewTrainParams;
    // if (TrainParams)
    // asNextStop=TrainParams.NextStop();
    // else
    TrainParams = TTrainParameters("none"); // rozkład jazdy
    // OrderCommand="";
    // OrderValue=0;
    OrdersClear();

    if( true == Primary ) {
        iDrivigFlags |= movePrimary; // aktywnie prowadzące pojazd
    }

    SetDriverPsyche(); // na końcu, bo wymaga ustawienia zmiennych
    TableClear();

    if( WriteLogFlag ) {
#ifdef _WIN32
		CreateDirectory( "physicslog", NULL );
#elif __unix__
		mkdir( "physicslog", 0744 );
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

    // HACK: give the simulation a small window to potentially replace the AI with a human driver
    fActionTime = -5.0;
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
    CloseLog();
};

// zamiana kodu rozkazu na opis
std::string TController::Order2Str(TOrders Order) const {

    if( ( ( Order & Change_direction ) != 0 )
     && ( Order != Change_direction ) ) {
        // może być nałożona na inną i wtedy ma priorytet
        return
            Order2Str( Change_direction )
            + " + "
            + Order2Str( static_cast<TOrders>( Order & ~Change_direction ) );
    }
    switch( Order ) {
        case Wait_for_orders:     return "Wait_for_orders";
        case Prepare_engine:      return "Prepare_engine";
        case Release_engine:      return "Release_engine";
        case Change_direction:    return "Change_direction";
        case Connect:             return "Connect";
        case Disconnect:          return "Disconnect";
        case Shunt:               return "Shunt";
        case Loose_shunt:         return "Loose_shunt";
        case Obey_train:          return "Obey_train";
        case Bank:                return "Bank";
        case Jump_to_first_order: return "Jump_to_first_order";
        default:                  return "Undefined";
    }
}

std::array<char, 64> orderbuffer;

std::string TController::OrderCurrent() const
{ // pobranie aktualnego rozkazu celem wyświetlenia
    auto const order { OrderCurrentGet() };
    // generally prioritize direction change...
    if( order & Change_direction ) {
        // ...but not in the middle of coupling operation
        if( false == TestFlag( iDrivigFlags, moveConnect ) ) {
            return STR("Change direction");
        }
    }

    switch( static_cast<TOrders>( order & ~Change_direction ) ) {
        case Wait_for_orders: { return STR("Wait for orders"); }
        case Prepare_engine: { return STR("Start the engine"); }
        case Release_engine: { return STR("Shut down the engine"); }
        case Change_direction: { return STR("Change direction"); }
        case Connect: { return STR("Couple to consist ahead"); }
        case Disconnect: {
            if( iVehicleCount < 0 ) {
                // done with uncoupling, order should update shortly
				return STR("Wait for orders");
            }
            // try to provide some task details
            auto const count { iVehicleCount };

            if( iVehicleCount > 1 ) {
                std::snprintf(
                    orderbuffer.data(), orderbuffer.size(),
				    STR_C("the engine plus %d next vehicles"), // TODO: implement plural forms
                    count );
            }
            auto const countstring { (
				count == 0 ? STR("the engine") :
				count == 1 ? STR("the engine plus the next vehicle") :
                orderbuffer.data() ) };

            std::snprintf(
                orderbuffer.data(), orderbuffer.size(),
			    STR_C("Uncouple %s"),
                countstring.c_str() );
            return orderbuffer.data();
        }
	    case Shunt: { return STR("Shunt according to signals"); }
	    case Loose_shunt: { return STR("Loose shunt according to signals"); }
	    case Obey_train: { return STR("Drive according to signals and timetable"); }
	    case Bank: { return STR("Bank consist ahead"); }
        default: { return{}; }
    }
};

void TController::OrdersClear()
{ // czyszczenie tabeli rozkazów na starcie albo po dojściu do końca
    OrderPos = 0;
    OrderTop = 1; // szczyt stosu rozkazów
    for (int b = 0; b < maxorders; b++)
        OrderList[b] = Wait_for_orders;
#if LOGORDERS
    OrdersDump("OrdersClear", false);
#endif
};

void TController::Activation()
{ // umieszczenie obsady w odpowiednim członie, wykonywane wyłącznie gdy steruje AI
    iDirection = iDirectionOrder; // kierunek (względem sprzęgów pojazdu z AI) właśnie został ustalony (zmieniony)
    if (iDirection)
    { // jeśli jest ustalony kierunek
        auto *initialvehicle { pVehicle };
/*
        auto const initiallocalbrakelevel { mvOccupied->LocalBrakePosA };
*/
        auto const initialspringbrakestate { mvOccupied->SpringBrake.Activate };
        // switch off tempomat
        SpeedCntrl( 0.0 );
        mvControlling->DecScndCtrl( 2 );
        // reset controls
        ZeroSpeed();
        ZeroDirection();
        mvOccupied->CabOccupied = mvOccupied->CabActive; // użytkownik moze zmienić CabOccupied wychodząc
        mvOccupied->CabDeactivisation(); // tak jest w Train.cpp
        if (TestFlag(pVehicle->MoverParameters->Couplers[iDirectionOrder < 0 ? end::rear : end::front].CouplingFlag, coupling::control)) {
            ZeroLocalBrake();
            if( initialspringbrakestate ) {
                mvOccupied->SpringBrakeActivate( false );
            }
            // odcięcie na zaworze maszynisty, FVel6 po drugiej stronie nie luzuje
            mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos(bh_NP) ); // odcięcie na zaworze maszynisty
			BrakeLevelSet( mvOccupied->BrakeCtrlPos ); //ustawienie zmiennej GBH
        }
        // przejście AI na drugą stronę EN57, ET41 itp.
        // TODO: clean this up, there's lot of redundancy with TMoverParameters::ChangeCab() and TTrain::MoveToVehicle()
        {
            int movedirection { ( iDirection < 0 ? end::rear : end::front ) };
            auto *targetvehicle { pVehicle->FirstFind( movedirection, coupling::control ) };
            if( pVehicle != targetvehicle ) {
                auto *targetvehicledriver { targetvehicle->Mechanik }; // zapamiętanie tego, co ewentualnie tam siedzi, żeby w razie dwóch zamienić miejscami
                // move to the new vehicle
                targetvehicle->Mechanik = this; // na razie bilokacja
                targetvehicle->MoverParameters->SetInternalCommand("", 0, 0); // usunięcie ewentualnie zalegającej komendy (Change_direction?)
                if( targetvehicle->DirectionGet() != pVehicle->DirectionGet() ) {
                    // jeśli są przeciwne do siebie to będziemy jechać w drugą stronę względem zasiedzianego pojazdu
                    iDirection = -iDirection;
                }
                // move the other driver to our old vehicle
                pVehicle->Mechanik = targetvehicledriver; // wsadzamy tego, co ewentualnie był (podwójna trakcja)
                // NOTE: having caboccupied != 0 (by swapping in the driver from the target vehicle) may lead to dysfunctional brake
                // TODO: investigate and resolve if necessary
                pVehicle->MoverParameters->CabOccupied = targetvehicle->MoverParameters->CabOccupied;
                if( pVehicle->Mechanik ) { // not guaranteed, as target vehicle can be empty
                    pVehicle->Mechanik->BrakeLevelSet( pVehicle->MoverParameters->BrakeCtrlPos );
                }
                // finish driver swap
                pVehicle = targetvehicle;
            }
        }
        if (pVehicle != initialvehicle)
        { // jeśli zmieniony został pojazd prowadzony
            ControllingSet(); // utworzenie połączenia do sterowanego pojazdu (może się zmienić) - silnikowy dla EZT
            if( ( mvOccupied->BrakeCtrlPosNo > 0 )
             && ( ( BrakeSystem == TBrakeSystem::Pneumatic )
               || ( BrakeSystem == TBrakeSystem::ElectroPneumatic ) ) ) {
                mvOccupied->LimPipePress = mvOccupied->PipePress;
                mvOccupied->ActFlowSpeed = 0;
            }
            BrakeLevelSet( mvOccupied->BrakeCtrlPos );
        }
        if( mvControlling->EngineType == TEngineType::DieselEngine ) {
            // dla 2Ls150 - przed ustawieniem kierunku - można zmienić tryb pracy
            if( mvControlling->ShuntModeAllow ) {
                mvControlling->CurrentSwitch(
                    ( ( OrderCurrentGet() & ( Shunt | Loose_shunt ) ) != 0 )
                   || ( fMass > 224000.0 ) ); // do tego na wzniesieniu może nie dać rady na liniowym
            }
        }
        // Ra: to przełączanie poniżej jest tu bez sensu
        mvOccupied->CabOccupied = iDirection; // aktywacja kabiny w prowadzonym pojeżdzie (silnikowy może być odwrotnie?)
        mvOccupied->CabActivisation(); // uruchomienie kabin w członach
        DirectionForward(true); // nawrotnik do przodu
        mvOccupied->SpringBrakeActivate( initialspringbrakestate );
/*
        // NOTE: this won't restore local brake if the vehicle has integrated local brake control
        // TBD, TODO: fix or let the ai activate the brake again as part of its standard logic?
        if (initiallocalbrakelevel > 0.0) // hamowanie tylko jeśli był wcześniej zahamowany (bo możliwe, że jedzie!)
            mvOccupied->LocalBrakePosA = initiallocalbrakelevel; // zahamuj jak wcześniej
*/
        if( pVehicle != initialvehicle ) {
            auto *train { simulation::Trains.find( initialvehicle->name() ) };
            if( train ) {
                train->MoveToVehicle( pVehicle );
            }
        }

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
            // potentially release manual brake
            d->MoverParameters->DecManualBrakeLevel( ManualBrakePosNo );
            d->MoverParameters->SpringBrake.Activate = false;
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

    if( OrderCurrentGet() & ( Shunt | Loose_shunt ) ) {
        // for uniform behaviour and compatibility with older scenarios set default acceleration table values for shunting
        fAccThreshold = (
            is_emu() ? -0.55 :
            is_dmu() ? -0.45 :
            -0.2 );
        // HACK: emu with induction motors need to start their braking a bit sooner than the ones with series motors
        if( ( is_emu() )
         && ( mvControlling->EngineType == TEngineType::ElectricInductionMotor ) ) {
            fAccThreshold += 0.10;
        }
        fNominalAccThreshold = fAccThreshold;
    }

    BrakeSystem = consist_brake_system();
    mvOccupied->EpFuseSwitch( BrakeSystem == TBrakeSystem::ElectroPneumatic );

    if( OrderCurrentGet() & ( Obey_train | Bank ) ) {
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

        IsPassengerTrain = ( is_train() ) && ( false == is_emu() ) && ( false == is_dmu() ) && ( ( mvOccupied->BrakeDelayFlag & bdelay_G ) == 0 );
        IsCargoTrain = ( is_train() ) && ( ( mvOccupied->BrakeDelayFlag & bdelay_G ) != 0 );
        IsHeavyCargoTrain = ( true == IsCargoTrain ) && ( fBrake_a0[ 1 ] > 0.4 ) && ( iVehicles - ControlledEnginesCount > 0 ) && ( fMass / iVehicles > 50000 );

        BrakingInitialLevel = (
            IsHeavyCargoTrain ? 1.25 :
            IsCargoTrain      ? 1.25 :
                                1.00 );

        BrakingLevelIncrease = (
            IsHeavyCargoTrain ? 0.25 :
            IsCargoTrain      ? 0.25 :
                                0.25 );

        if( is_emu() ) {
            auto ep_factor { ( BrakeSystem == TBrakeSystem::ElectroPneumatic ? 8 : 4 ) };
            if( mvControlling->EngineType == TEngineType::ElectricInductionMotor ) {
                // HACK: emu with induction motors need to start their braking a bit sooner than the ones with series motors
                fNominalAccThreshold = std::max( -0.60, -fBrake_a0[ BrakeAccTableSize ] - ep_factor * fBrake_a1[ BrakeAccTableSize ] );
            }
            else {
                fNominalAccThreshold = std::max( -0.75, -fBrake_a0[ BrakeAccTableSize ] - ep_factor * fBrake_a1[ BrakeAccTableSize ] );
            }
		    fBrakeReaction = 0.25;
	    }
        else if( is_dmu() ) {
            fNominalAccThreshold = std::max( -0.75, -fBrake_a0[ BrakeAccTableSize ] - 8 * fBrake_a1[ BrakeAccTableSize ] );
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
	double Us = abs(mvControlling->EngineVoltage) - IF*R;
	double ns = std::max(0.0, Us / (pole*mvControlling->RList[MCPN].Mn));
	ESMVel = ns * mvControlling->WheelDiameter*M_PI*3.6/mvControlling->Transmision.Ratio;
	return ESMVel;
}

int TController::CheckDirection() {

    int d = mvOccupied->DirAbsolute; // który sprzęg jest z przodu
    if( !d ) {
        // jeśli nie ma ustalonego kierunku to jedziemy wg aktualnej kabiny
        d = mvOccupied->CabActive;
    }
    return d;
}

bool TController::CheckVehicles(TOrders user)
{ // sprawdzenie stanu posiadanych pojazdów w składzie i zapalenie świateł
    TDynamicObject *p; // roboczy wskaźnik na pojazd
    iVehicles = 0; // ilość pojazdów w składzie
    auto d = CheckDirection();
    d = d >= 0 ? 0 : 1; // kierunek szukania czoła (numer sprzęgu)
    pVehicles[end::front] = p = pVehicle->FirstFind(d); // pojazd na czele składu
    // liczenie pojazdów w składzie i ustalenie parametrów
    auto dir = d = 1 - d; // a dalej będziemy zliczać od czoła do tyłu
    fLength = 0.0; // długość składu do badania wyjechania za ograniczenie
    fMass = 0.0; // całkowita masa do liczenia stycznej składowej grawitacji
    fVelMax = -1; // ustalenie prędkości dla składu
    bool main = true; // czy jest głównym sterującym
    iDrivigFlags |= moveOerlikons; // zakładamy, że są same Oerlikony
    // Ra 2014-09: ustawić moveMultiControl, jeśli wszystkie są w ukrotnieniu (i skrajne mają kabinę?)
    while (p)
    { // sprawdzanie, czy jest głównym sterującym, żeby nie było konfliktu
        if( ( p->Mechanik ) // jeśli ma obsadę
         && ( p->Mechanik != this ) ) { // ale chodzi o inny pojazd, niż aktualnie sprawdzający
            if( p->Mechanik->iDrivigFlags & movePrimary ) {
                // a tamten ma priorytet
                // TODO: take into account drivers' operating modes, one or more of them might be on banking duty
                if( ( iDrivigFlags & movePrimary )
                 && ( mvOccupied->DirAbsolute )
                 && ( mvOccupied->BrakeCtrlPos >= -1 ) ) {
                    // jeśli rządzi i ma kierunek
                    p->Mechanik->primary( false ); // dezaktywuje tamtego
                    p->Mechanik->ZeroLocalBrake();
                    p->MoverParameters->BrakeLevelSet( p->MoverParameters->Handle->GetPos( bh_NP ) ); // odcięcie na zaworze maszynisty
                    p->Mechanik->BrakeLevelSet( p->MoverParameters->BrakeCtrlPos ); //ustawienie zmiennej GBH
                }
                else {
                    main = false; // nici z rządzenia
                }
            }
        }
        ++iVehicles; // jest jeden pojazd więcej
        pVehicles[end::rear] = p; // zapamiętanie ostatniego
        fLength += p->MoverParameters->Dim.L; // dodanie długości pojazdu
        fMass += p->MoverParameters->TotalMass; // dodanie masy łącznie z ładunkiem
        fVelMax = min_speed( fVelMax, p->MoverParameters->Vmax ); // ustalenie maksymalnej prędkości dla składu
        // reset oerlikon brakes consist flag as different type was detected
        if( ( p->MoverParameters->BrakeSubsystem != TBrakeSubSystem::ss_ESt )
         && ( p->MoverParameters->BrakeSubsystem != TBrakeSubSystem::ss_LSt ) ) {
            iDrivigFlags &= ~( moveOerlikons );
        }
        p = p->Neighbour(dir); // pojazd podłączony od wskazanej strony
    }
    if( main ) {
        iDrivigFlags |= movePrimary; // nie znaleziono innego, można się porządzić
    }

    ControllingSet(); // ustalenie członu do sterowania (może być inny niż zasiedziany)

    if (iDrivigFlags & movePrimary)
    { // jeśli jest aktywnie prowadzącym pojazd, może zrobić własny porządek
        auto pantmask = 1;
        p = pVehicles[end::front];
        // establish ownership and vehicle order
        while (p)
        {
            if (p->MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector)
            { // jeśli pojazd posiada pantograf, to przydzielamy mu maskę, którą będzie informował o jeździe bezprądowej
                p->iOverheadMask = pantmask;
                pantmask = pantmask << 1; // przesunięcie bitów, max. 32 pojazdy z pantografami w składzie
            }

            d = p->DirectionSet(d ? 1 : -1); // zwraca położenie następnego (1=zgodny,0=odwrócony - względem czoła składu)
            p->ctOwner = this; // dominator oznacza swoje terytorium
            p = p->Next(); // pojazd podłączony od tyłu (licząc od czoła)
        }
        // with the order established the virtual train manager can do their work
        p = pVehicles[ end::front ];
        ControlledEnginesCount = ( p->MoverParameters->Power > 1.0 ? 1 : 0 );
        auto hasheaters { false };
        while (p)
        {
            if( p != pVehicle ) {
                if( false == p->is_connected( pVehicle, coupling::control ) ) {
                    // NOTE: don't set battery in controllable vehicles, let the user/ai do it explicitly
                    // HACK: wagony muszą mieć baterię załączoną do otwarcia drzwi...
                    p->MoverParameters->BatterySwitch( true );
                    // enable heating and converter in carriages with can be heated
                    if( p->MoverParameters->HeatingPower > 0 ) {
                        p->MoverParameters->HeatingAllow = true;
                        p->MoverParameters->ConverterSwitch( true, range_t::local );
                        hasheaters = true;
                    }
                }
                else {
                    if( p->MoverParameters->Power > 1.0 ) {
                        ++ControlledEnginesCount;
                    }
                }
            }

            if( p->asDestination == "none" ) {
                p->DestinationSet( TrainParams.Relation2, TrainParams.TrainName ); // relacja docelowa, jeśli nie było
            }
            if( AIControllFlag ) { // jeśli prowadzi komputer
                p->RaLightsSet( 0, 0 ); // gasimy światła
            }
            p = p->Next(); // pojazd podłączony od tyłu (licząc od czoła)
        }

        if( ( user == Connect ) && ( true == main ) ) {
            // HACK: with additional vehicles in the consist ensure all linked vehicles are set to move in the same direction
            if( ( pVehicle->Prev( coupling::control ) != nullptr )
             || ( pVehicle->Next( coupling::control ) != nullptr ) ) {
                sync_consist_reversers();
            }
            // potentially sync compartment lighting state for the newly connected vehicles
            if( mvOccupied->CompartmentLights.start_type == start_t::manual ) {
                mvOccupied->CompartmentLightsSwitchOff( mvOccupied->CompartmentLights.is_disabled );
                mvOccupied->CompartmentLightsSwitch( mvOccupied->CompartmentLights.is_enabled );
            }
        }

        // kasowanie pamieci hamowania kontrolnego
        if (OrderCurrentGet() & (Shunt | Loose_shunt | Disconnect | Connect | Change_direction)) {
			DynamicBrakeTest = 0;
            DBT_VelocityBrake
            = DBT_VelocityRelease
            = DBT_VelocityFinish
            = DBT_BrakingTime
            = DBT_ReleasingTime
            = DBT_MidPointAcc
            = 0;
		}

        if( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank ) ) {
            // nastawianie hamulca do jazdy pociągowej
            AutoRewident();
/*
            if( ( true == TestFlag( iDrivigFlags, moveConnect ) )
                && ( true == TestFlag( OrderCurrentGet(), Connect ) ) ) {
                iCoupler = 0; // dalsza jazda manewrowa już bez łączenia
                iDrivigFlags &= ~moveConnect; // zdjęcie flagi doczepiania
                SetVelocity( 0, 0, stopJoin ); // wyłączyć przyspieszanie
                JumpToNextOrder(); // wykonanie następnej komendy
            }
*/
        }

        // detect push-pull train configurations and mark them accordingly
        if( pVehicles[ end::front ]->is_connected( pVehicles[ end::rear ], coupling::control ) ) {
            // zmiana czoła przez zmianę kabiny
            iDrivigFlags |= movePushPull;
        }
        else {
            // zmiana czoła przez manewry
            iDrivigFlags &= ~movePushPull;
        }

        // HACK: ensure vehicle lights are active from the beginning, if it had pre-activated battery
        if( mvOccupied->LightsPosNo > 0 ) {
            pVehicle->SetLights();
        }
        if( iEngineActive ) {
            // potentially adjust light state
            control_lights();
            // custom ai action for disconnect mode: switch off lights on disconnected vehicle(s)
            if( is_train() ) {
                if( true == TestFlag( OrderCurrentGet(), Disconnect ) ) {
                    if( AIControllFlag ) {
                        // światła manewrowe (Tb1) tylko z przodu, aby nie pozostawić odczepionego ze światłem
                        if( mvOccupied->DirActive >= 0 ) { // jak ma kierunek do przodu
                            pVehicles[ end::rear ]->RaLightsSet( -1, 0 );
                        }
                        else { // jak dociska
                            pVehicles[ end::front ]->RaLightsSet( 0, -1 );
                        }
                    }
                }
            }
            // enable door locks
            cue_action( driver_hint::consistdoorlockson );
            // potentially enable train heating
            {
                auto const ispassengertrain { ( IsPassengerTrain ) && ( iVehicles - ControlledEnginesCount > 0 ) };
                // TODO: replace connection test with connection check between last engine and first car, specifically
                auto const isheatingcouplingactive { (
                    ControlledEnginesCount == 1 ?
                        pVehicles[ end::front ]->is_connected( pVehicles[ end::rear ], coupling::heating ) :
                        true ) };
                auto const isheatingneeded {
                    (is_emu() || is_dmu() ? true :
                    (OrderCurrentGet() & (Obey_train | Bank)) == 0 ? false :
                    false == isheatingcouplingactive ? false :
                    ispassengertrain ? hasheaters :
                    false) };
                if( mvControlling->HeatingAllow != isheatingneeded ) {
                    cue_action(
                        isheatingneeded ?
                            driver_hint::consistheatingon :
                            driver_hint::consistheatingoff );
                }
            }
        }

        if( ( user == Connect )
         || ( user == Disconnect ) ) {
            // HACK: force route table update on consist change, new consist length means distances to points of interest are now wrong
            iTableDirection = 0;
        }
    } // blok wykonywany, gdy aktywnie prowadzi
    return true;
}

void TController::Lights(int head, int rear)
{ // zapalenie świateł w skłądzie
    pVehicles[ end::front ]->RaLightsSet(head, -1); // zapalenie przednich w pierwszym
    pVehicles[ end::rear ]->RaLightsSet(-1, rear); // zapalenie końcówek w ostatnim
}

void TController::DirectionInitial()
{ // ustawienie kierunku po wczytaniu trainset (może jechać na wstecznym
    mvOccupied->CabActivisationAuto(); // załączenie rozrządu (wirtualne kabiny)
    if (mvOccupied->Vel > EU07_AI_NOMOVEMENT)
    { // jeśli na starcie jedzie
        iDirection = iDirectionOrder =
            (mvOccupied->V > 0 ? 1 : -1); // początkowa prędkość wymusza kierunek jazdy
        DirectionForward(mvOccupied->V * mvOccupied->CabActive >= 0.0); // a dalej ustawienie nawrotnika
    }
    CheckVehicles(); // sprawdzenie świateł oraz skrajnych pojazdów do skanowania
};

void TController::DirectionChange() {

    auto const initialstate { iDirection };
    iDirection = CheckDirection();
    if( iDirection != initialstate ) {
        CheckVehicles( Change_direction );
    }
}

TBrakeSystem TController::consist_brake_system() const {

    if( mvOccupied->BrakeSystem != TBrakeSystem::ElectroPneumatic ) { return mvOccupied->BrakeSystem; }

    auto isepcapable = true;
    if( pVehicles[ end::front ] != pVehicles[ end::rear ] ) {
        // more detailed version, will use manual braking also for coupled sets of controlled vehicles
        auto *vehicle = pVehicles[ end::front ]; // start from first
        while( ( true == isepcapable )
            && ( vehicle != nullptr ) ) {
            // NOTE: we could simplify this by doing only check of the rear coupler, but this can be quite tricky in itself
            // TODO: add easier ways to access front/rear coupler taking into account vehicle's direction
            isepcapable = (
                ( ( vehicle->MoverParameters->Couplers[ end::front ].Connected == nullptr )
               || ( ( vehicle->MoverParameters->Couplers[ end::front ].CouplingFlag & coupling::control )
                 && ( vehicle->MoverParameters->Couplers[ end::front ].Connected->Power > -1 ) ) )
             && ( ( vehicle->MoverParameters->Couplers[ end::rear ].Connected == nullptr )
               || ( ( vehicle->MoverParameters->Couplers[ end::rear ].CouplingFlag & coupling::control )
                 && ( vehicle->MoverParameters->Couplers[ end::rear ].Connected->Power > -1 ) ) ) );
            vehicle = vehicle->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
        }
    }
    return ( isepcapable ? TBrakeSystem::ElectroPneumatic : TBrakeSystem::Pneumatic );
}

int TController::OrderDirectionChange(int newdir, TMoverParameters *Vehicle)
{ // zmiana kierunku jazdy, niezależnie od kabiny
    int testd = newdir;
    if (Vehicle->Vel < 0.5)
    { // jeśli prawie stoi, można zmienić kierunek, musi być wykonane dwukrotnie, bo za pierwszym razem daje na zero
        switch (newdir * Vehicle->CabActive)
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
    if ((Vehicle->DirActive == 0) && (VelforDriver < Vehicle->Vel)) // Ra: to jest chyba bez sensu
        IncBrake(); // niech hamuje
    if (Vehicle->DirActive == testd * Vehicle->CabActive)
        VelforDriver = -1; // można jechać, bo kierunek jest zgodny z żądanym
    if (is_emu())
        if (Vehicle->DirActive > 0)
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
    if (NewVel == 0.0) { // jeśli ma stanąć
        if (r != stopNone) // a jest powód podany
            eStopReason = r; // to zapamiętać nowy powód
    }
    else {
        eStopReason = stopNone; // podana prędkość, to nie ma powodów do stania
        // to całe poniżej to warunki zatrąbienia przed ruszeniem
        if( ( is_train() ) // tylko pociągi trąbią (unimogi tylko na torach, więc trzeba raczej sprawdzać tor)
         && ( ( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank | Connect | Prepare_engine ) ) != 0 ) // jeśli jedzie w dowolnym trybie
         && ( ( iDrivigFlags & moveStartHorn ) != 0 ) // jezeli trąbienie włączone
         && ( ( iDrivigFlags & ( moveStartHornDone | moveConnect ) ) == 0 ) // jeśli nie zatrąbione i nie jest to moment podłączania składu
         && ( mvOccupied->Vel < 1.0 ) // jesli stoi (na razie, bo chyba powinien też, gdy hamuje przed semaforem)
         && ( ( NewVel >= 1.0 ) || ( NewVel < 0.0 ) ) ) { // o ile prędkość jest znacząca zatrąb po odhamowaniu
            iDrivigFlags |= moveStartHornNow;
        }
    }
    VelSignal = NewVel; // prędkość zezwolona na aktualnym odcinku
    VelNext = NewVelNext; // prędkość przy następnym obiekcie
}

double TController::BrakeAccFactor() const
{
	double Factor = 1.0;

    if( ( fAccThreshold != 0.0 )
     && ( AccDesired < 0.0 )
     && ( ( ActualProximityDist > fMinProximityDist )
       || ( mvOccupied->Vel > VelDesired + fVelPlus ) ) ) {
        Factor += ( fBrakeReaction * ( /*mvOccupied->BrakeCtrlPosR*/BrakeCtrlPosition < 0.5 ? 1.5 : 1 ) ) * mvOccupied->Vel / ( std::max( 0.0, ActualProximityDist ) + 1 ) * ( ( AccDesired - AbsAccS ) / fAccThreshold );
    }
/*
	if (mvOccupied->TrainType == dt_DMU && mvOccupied->Vel > 40 && VelNext<40)
		Factor *= 1 + 0.25 * ( (1600 - VelNext * VelNext) / (mvOccupied->Vel * mvOccupied->Vel) );
*/
	return Factor;
}

void TController::SetDriverPsyche() {

    if ((Psyche == Aggressive) && (OrderCurrentGet() == Obey_train)) {
        ReactionTime = HardReactionTime; // w zaleznosci od charakteru maszynisty
        if (is_car()) {
            WaitingExpireTime = 1; // tyle ma czekać samochód, zanim się ruszy
            AccPreferred = 3.0; //[m/ss] agresywny
        }
        else {
            WaitingExpireTime = 61; // tyle ma czekać, zanim się ruszy
            AccPreferred = HardAcceleration; // agresywny
        }
    }
    else {
        ReactionTime = EasyReactionTime; // spokojny
        if (is_car()) {
            WaitingExpireTime = 3; // tyle ma czekać samochód, zanim się ruszy
            AccPreferred = 2.0; //[m/ss]
        }
        else {
            WaitingExpireTime = 65; // tyle ma czekać, zanim się ruszy
            AccPreferred = EasyAcceleration;
        }
    }
}

bool TController::PrepareEngine()
{ // odpalanie silnika
    // HACK: don't immediately activate inert vehicle in case the simulation is about to replace us with human driver
    if( ( mvOccupied->Vel < 1.0 ) && ( fActionTime < 0.0 ) ) { return false; }

    LastReactionTime = 0.0;
    ReactionTime = ( mvOccupied->Vel < 5 ? PrepareTime : EasyReactionTime ); // react faster with rolling start

    cue_action( driver_hint::batteryon );
	cue_action( driver_hint::cabactivation);
    cue_action( driver_hint::radioon );

    if( has_diesel_engine() ) {
        cue_action( driver_hint::oilpumpon );
        PrepareHeating();
        if( true == IsHeatingTemperatureOK ) {
            cue_action( driver_hint::fuelpumpon );
        }
        else {
            cue_action( driver_hint::waittemperaturetoolow );
        }
    }

    if( ( mvPantographUnit->EnginePowerSource.SourceType == TPowerSource::CurrentCollector )
     && ( mvPantographUnit->EnginePowerSource.CollectorParameters.CollectorsNo > 0 ) ) {
        // if our pantograph unit isn't a pantograph-devoid fallback
        auto const sufficientpantographpressure { (
            is_emu() ?
                2.5 : // Ra 2013-12: Niebugocław mówi, że w EZT podnoszą się przy 2.5
                3.5 )
            + 0.1 }; // leeway margin

        if (mvPantographUnit->PantPress < sufficientpantographpressure) {
            // załączenie małej sprężarki
            if( false == mvPantographUnit->PantAutoValve ) {
                // odłączenie zbiornika głównego, bo z nim nie da rady napompować
                cue_action( driver_hint::pantographairsourcesetauxiliary );
            }
            cue_action( driver_hint::pantographcompressoron ); // załączenie sprężarki pantografów
            if( mvPantographUnit->PantCompFlag ) {
                cue_action( driver_hint::waitpantographpressuretoolow );
            }
        }
        else {
            // jeżeli jest wystarczające ciśnienie w pantografach
            if( ( false == mvPantographUnit->bPantKurek3 )
             || ( mvPantographUnit->PantPress <= mvPantographUnit->ScndPipePress ) ) { // kurek przełączony albo główna już pompuje
                cue_action( driver_hint::pantographcompressoroff ); // sprężarkę pantografów można już wyłączyć
            }
        }
        // TODO: make pantograph setup a part of control_pantographs() and call it here instead
        if( ( fOverhead2 == -1.0 ) && ( iOverheadDown == 0 ) ) {
            cue_action( driver_hint::pantographsvalveon );
            cue_action( driver_hint::frontpantographvalveon, ( iDirection >= 0 ? 5 : 0 ) );
            cue_action( driver_hint::rearpantographvalveon, ( iDirection >= 0 ? 0 : 5 ) );
        }
    }

    auto const ispoweravailable =
        ( mvControlling->EnginePowerSource.SourceType != TPowerSource::CurrentCollector )
     || ( std::max( mvControlling->GetTrainsetHighVoltage(), mvControlling->PantographVoltage ) > mvControlling->EnginePowerSource.CollectorParameters.MinV );

    bool isready = false;

    if( ( IsHeatingTemperatureOK )
     && ( ispoweravailable ) ) {
        // najpierw ustalamy kierunek, jeśli nie został ustalony
        PrepareDirection();

        if( IsAnyConverterOverloadRelayOpen ) {
            // wywalił bezpiecznik nadmiarowy przetwornicy
            cue_action( driver_hint::compressoroff ); // TODO: discern whether compressor needs converter to operate
            cue_action( driver_hint::converteroff );
            cue_action( driver_hint::primaryconverteroverloadreset ); // reset nadmiarowego
        }
        if( IsAnyLineBreakerOpen ) {
            // activate main circuit or engine
            cue_action( driver_hint::mastercontrollersetzerospeed );
            // consist-wide ground relay reset
            cue_action( driver_hint::maincircuitgroundreset );
            cue_action( driver_hint::tractionnmotoroverloadreset );
            if( mvOccupied->EngineType == TEngineType::DieselEngine ) {
                // specjalnie dla SN61 żeby nie zgasł
                cue_action( driver_hint::mastercontrollersetidle );
            }
            cue_action( driver_hint::linebreakerclose );
        }
        else {
            // main circuit or engine is on, set up vehicle devices and controls
            if( false == IsAnyConverterOverloadRelayOpen ) {
                if( IsAnyConverterPresent ) {
                    cue_action( driver_hint::converteron );
                }
                // w EN57 sprężarka w ra jest zasilana z silnikowego
                // TODO: change condition to presence of required voltage type
                if( IsAnyCompressorPresent && IsAnyConverterEnabled ) {
                    cue_action( driver_hint::compressoron );
                }
                if( ( mvControlling->ScndPipePress < 4.5 ) && ( mvControlling->VeselVolume > 0.0 ) ) {
                    cue_action( driver_hint::waitpressuretoolow );
                }
                // enable motor blowers
                if( mvOccupied->MotorBlowers[ end::front ].speed != 0 ) {
                    cue_action( driver_hint::frontmotorblowerson );
                }
                if( mvOccupied->MotorBlowers[ end::rear ].speed != 0 ) {
                    cue_action( driver_hint::rearmotorblowerson );
                }
            }
            // sync virtual brake state with the 'real' one
            if( mvOccupied->BrakeHandle != TBrakeHandle::NoHandle ) {
                std::unordered_map<int, int> const brakepositions{
                    { static_cast<int>( mvOccupied->Handle->GetPos( bh_RP ) ), gbh_RP },
                    { static_cast<int>( mvOccupied->Handle->GetPos( bh_NP ) ), gbh_NP },
                    { static_cast<int>( mvOccupied->Handle->GetPos( bh_FS ) ), gbh_FS } };
                auto const lookup{ brakepositions.find( static_cast<int>( mvOccupied->fBrakeCtrlPos ) ) };
                if( lookup != brakepositions.end() ) {
                    BrakeLevelSet( lookup->second ); // GBH
                }
            }
            // set up train brake
            if( mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos( bh_RP ) ) {
				if ( ( !mvOccupied->Handle->Time)
					|| ( mvOccupied->Handle->GetCP() < mvOccupied->HighPipePress - 0.05 ) );
                cue_action( driver_hint::trainbrakerelease );
            }
            // sync spring brake state across consist
            cue_action(
                mvOccupied->SpringBrake.Activate ?
                    driver_hint::springbrakeon :
                    driver_hint::springbrakeoff );
        }
        isready = ( false == IsAnyConverterOverloadRelayOpen )
               && ( mvOccupied->DirActive != 0 )
               && ( false == IsAnyLineBreakerOpen )
               && ( ( false == IsAnyConverterPresent ) || ( true == IsAnyConverterEnabled ) )
               && ( ( false == IsAnyCompressorPresent ) || ( true == IsAnyCompressorEnabled ) )
               && ( ( mvControlling->ScndPipePress > 4.5 ) || ( mvControlling->VeselVolume == 0.0 ) )
               && ( ( static_cast<int>( mvOccupied->fBrakeCtrlPos ) == static_cast<int>( mvOccupied->Handle->GetPos( bh_RP ) ) )
                 || ( static_cast<int>( mvOccupied->fBrakeCtrlPos ) != static_cast<int>( mvOccupied->Handle->GetPos( bh_NP ) ) )
                 || ( mvOccupied->BrakeHandle == TBrakeHandle::NoHandle ) );
    }

    if( true == isready ) {
        // jeśli dotychczas spał teraz nie ma powodu do stania
        eAction = TAction::actUnknown;
        if( eStopReason == stopSleep ) {
            eStopReason = stopNone;
        }
        // może skanować sygnały i reagować na komendy
        iDrivigFlags |= moveActive;
    }

    iEngineActive = isready;

    return iEngineActive;
}

// wyłączanie silnika (test wyłączenia, a część wykonawcza tylko jeśli steruje komputer)
bool TController::ReleaseEngine() {
/*
    if( mvOccupied->Vel > 0.01 ) {
        VelDesired = 0.0;
        AccDesired = std::min( AccDesired, -1.25 ); // hamuj solidnie
        if( true == AIControllFlag ) {
            // TBD, TODO: make a dedicated braking procedure out of it for potential reuse
            ZeroSpeed();
            IncBrake();
            ReactionTime = 0.1;
        }
        // don't bother with the rest until we're standing still
        return false;
    }
*/
    // don't bother with the rest until we're standing still
    if( mvOccupied->Vel > EU07_AI_NOMOVEMENT ) { return false; }

    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;

    cue_action( driver_hint::releaseroff );
    // release train brake if on flats...
    if( std::abs( fAccGravity ) < 0.01 ) {
        apply_independent_brake_only();
    }
    // ... but on slopes engage train brake
    else {
//        AccDesired = std::min( AccDesired, -0.9 );
        cue_action( driver_hint::trainbrakeapply );
    }

    cue_action( driver_hint::mastercontrollersetreverserunlock );
    cue_action( driver_hint::directionnone );
    // zamykanie drzwi
    cue_action( driver_hint::doorrightclose );
    cue_action( driver_hint::doorleftclose );
    // heating
    cue_action( driver_hint::consistheatingoff );
    // devices
    cue_action( driver_hint::compressoroff );
    cue_action( driver_hint::converteroff );
    // line breaker/engine
    cue_action( driver_hint::linebreakeropen );
    // pantographs
    cue_action( driver_hint::frontpantographvalveoff );
    cue_action( driver_hint::rearpantographvalveoff );

    if( false == mvControlling->Mains ) {
        // finish vehicle shutdown
        if( has_diesel_engine() ) {
            // heating/cooling subsystem
            cue_action( driver_hint::waterheateroff );
            // optionally turn off the water pump as well
            if( mvControlling->WaterPump.start_type != start_t::battery ) {
                cue_action( driver_hint::waterpumpoff );
            }
            // fuel and oil subsystems
            cue_action( driver_hint::fuelpumpoff );
            cue_action( driver_hint::oilpumpoff );
        }
        // gasimy światła
        cue_action( driver_hint::lightsoff );
        // activate parking brake
        // TBD: do it earlier?
        cue_action( driver_hint::springbrakeon );
        if( ( mvOccupied->LocalBrake == TLocalBrake::ManualBrake )
         || ( mvOccupied->MBrake == true ) ) {
            cue_action( driver_hint::manualbrakon );
        }
        // switch off remaining power
        cue_action( driver_hint::radiooff );
        cue_action( driver_hint::batteryoff );
    }

    auto const OK {
               ( mvOccupied->DirActive == 0 )
//            && ( false == IsAnyCompressorEnabled )
//            && ( false == IsAnyConverterEnabled )
            && ( false == mvControlling->Mains )
            && ( false == mvOccupied->Power24vIsAvailable ) };

    if (OK) {
        // jeśli się zatrzymał
        iEngineActive = false;
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
    auto OK { false };
    auto const bs {
        ( ( BrakeSystem == TBrakeSystem::ElectroPneumatic ) && ( ForcePNBrake ) ) ?
            TBrakeSystem::Pneumatic :
            BrakeSystem };
    switch( bs ) {
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
			if (bs != mvOccupied->BrakeSystem)
			{
				while (mvOccupied->BrakeOpModeFlag > bom_PN)
				{
					mvOccupied->BrakeOpModeFlag >>= 1;
				}
			}
            // NOTE: can't perform just test whether connected vehicle == nullptr, due to virtual couplers formed with nearby vehicles
            auto standalone { true };
            if( ( mvOccupied->TrainType == dt_ET41 )
             || ( mvOccupied->TrainType == dt_ET42 ) ) {
                // NOTE: we're doing simplified checks full of presuptions here.
                // they'll break if someone does strange thing like turning around the second unit
                if( ( mvOccupied->Couplers[ end::rear ].CouplingFlag & coupling::permanent )
                 && ( mvOccupied->Couplers[ end::rear ].Connected->Couplers[ end::rear ].Connected != nullptr ) ) {
                    standalone = false;
                }
                if( ( mvOccupied->Couplers[ end::front ].CouplingFlag & coupling::permanent )
                 && ( mvOccupied->Couplers[ end::front ].Connected->Couplers[ end::front ].Connected != nullptr ) ) {
                    standalone = false;
                }
            }
            else if( is_dmu() ) {
                // enforce use of train brake for DMUs
                standalone = false;
            }
            else if( ( ( OrderCurrentGet() & ( Loose_shunt ) ) != 0 )
                  && ( mvOccupied->Vel - VelDesired < fVelPlus + fVelMinus ) ) {
                // try to reduce train brake use in loose shunting mode
                standalone = true;
            }
            else {
/*
                standalone =
                    ( ( mvOccupied->Couplers[ 0 ].CouplingFlag == 0 )
                   && ( mvOccupied->Couplers[ 1 ].CouplingFlag == 0 ) );
*/
                if( pVehicles[ end::front ] != pVehicles[ end::rear ] ) {
                    // more detailed version, will use manual braking also for coupled sets of controlled vehicles
                    auto *vehicle = pVehicles[ end::front ]; // start from first
                    while( ( true == standalone )
                        && ( vehicle != nullptr ) ) {
                        // NOTE: we could simplify this by doing only check of the rear coupler, but this can be quite tricky in itself
                        // TODO: add easier ways to access front/rear coupler taking into account vehicle's direction
                        standalone =
                            ( ( ( vehicle->MoverParameters->Couplers[ end::front ].Connected == nullptr )
                             || ( ( vehicle->MoverParameters->Couplers[ end::front ].CouplingFlag & coupling::control )
                               && ( vehicle->MoverParameters->Couplers[ end::front ].Connected->Power > 1 ) ) )
                           && ( ( vehicle->MoverParameters->Couplers[ end::rear  ].Connected == nullptr )
                             || ( ( vehicle->MoverParameters->Couplers[ end::rear  ].CouplingFlag & coupling::control )
                               && ( vehicle->MoverParameters->Couplers[ end::rear  ].Connected->Power > 1 ) ) ) );
                        vehicle = vehicle->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
                    }
                }
            }

            //standalone = standalone && ( mvControlling->EIMCtrlType == 0 );

            if( ( true == standalone ) && ( false == ForcePNBrake ) ) {
                if( mvControlling->EIMCtrlType > 0 ) {
                    OK = IncBrakeEIM();
                }
                else {
                    OK = mvOccupied->IncLocalBrakeLevel(
                        1 + static_cast<int>( std::floor( 0.5 + std::fabs( AccDesired ) ) ) ); // hamowanie lokalnym bo luzem jedzie
                }
            }
            else {
                if( /*GBH mvOccupied->BrakeCtrlPos*/ BrakeCtrlPosition + 1 == gbh_MAX ) {
					if (AccDesired < -1.5) // hamowanie nagle
						OK = /*mvOccupied->*/BrakeLevelAdd(1.0); //GBH
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
                            pos_corr -= ( std::min( 5.0, d->MoverParameters->Hamulec->GetCRP() ) - 5.0 ) * d->MoverParameters->TotalMass;
                        }
						d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
					}
					pos_corr = pos_corr / fMass * 2.5;

					if (mvOccupied->BrakeHandle == TBrakeHandle::FV4a)
					{
						pos_corr += mvOccupied->Handle->GetCP()*0.2;

					}
					double deltaAcc = -AccDesired*BrakeAccFactor() - (fBrake_a0[0] + 4.0 * (/*GBH mvOccupied->BrakeCtrlPosR*/BrakeCtrlPosition - 1 - pos_corr)*fBrake_a1[0]);

                    if( deltaAcc > fBrake_a1[0])
					{
                        if( /*GBH mvOccupied->BrakeCtrlPosR*/BrakeCtrlPosition < 0.1 ) {
                            OK = /*mvOccupied->*/BrakeLevelAdd( BrakingInitialLevel ); //GBH
                            // HACK: stronger braking to overcome SA134 engine behaviour
                            if( ( is_dmu() )
                             && ( VelNext == 0.0 )
                             && ( fBrakeDist < 200.0 ) ) {
                                BrakeLevelAdd(
                                    fBrakeDist / ActualProximityDist < 0.8 ?
                                        0.5 :
                                        1.0 );
                            }
                        }
						else {
                            OK = /*mvOccupied->*/BrakeLevelAdd( BrakingLevelIncrease ); //GBH
                            // brake harder if the acceleration is much higher than desired
                            /*if( ( deltaAcc > 2 * fBrake_a1[ 0 ] )
                             && ( mvOccupied->BrakeCtrlPosR + BrakingLevelIncrease <= 5.0 ) ) {
                                mvOccupied->BrakeLevelAdd( BrakingLevelIncrease );
                            }  GBH */
                            if( ( deltaAcc > 2 * fBrake_a1[ 0 ] )
                             && ( BrakeCtrlPosition + BrakingLevelIncrease <= 5.0 ) ) {
                                /*mvOccupied->*/BrakeLevelAdd( BrakingLevelIncrease );
							}
						}
                    }
                    else
                        OK = false;
                }
            }
            if( /*GBH mvOccupied->BrakeCtrlPos*/BrakeCtrlPosition > 0 ) {
                if( mvOccupied->Hamulec->Releaser() ) {
                    mvOccupied->BrakeReleaser( 0 );
                }
            }
            break;
        }
        case TBrakeSystem::ElectroPneumatic: {
            while( ( mvOccupied->BrakeOpModeFlag << 1 ) <= mvOccupied->BrakeOpModes ) {
                mvOccupied->BrakeOpModeFlag <<= 1;
            }
            if( mvOccupied->EngineType == TEngineType::ElectricInductionMotor ) {
				if (mvOccupied->BrakeHandle == TBrakeHandle::MHZ_EN57) {
					if (mvOccupied->BrakeCtrlPos < mvOccupied->Handle->GetPos(bh_FB))
						OK = mvOccupied->BrakeLevelAdd(1.0);
				}
				else {
					OK = IncBrakeEIM();
				}
            }
            else if( mvOccupied->Handle->TimeEP == false ) {
                auto const initialbrakeposition { mvOccupied->fBrakeCtrlPos };
                auto const AccMax { std::min( fBrake_a0[ 0 ] + 12 * fBrake_a1[ 0 ], mvOccupied->MED_amax ) };
                mvOccupied->BrakeLevelSet(
                    interpolate(
                        mvOccupied->Handle->GetPos( bh_EPR ),
                        mvOccupied->Handle->GetPos( bh_EPB ),
                        clamp( -AccDesired / AccMax * mvOccupied->AIHintLocalBrakeAccFactor, 0.0, 1.0 ) ) );
                OK = ( mvOccupied->fBrakeCtrlPos != initialbrakeposition );
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

bool TController::IncBrakeEIM()
{ // zwiększenie hamowania
	bool OK = false;
	switch (mvOccupied->EIMCtrlType)
	{
        case 0: {
            if( mvOccupied->MED_amax != 9.81 ) {
                auto const maxpos{mvOccupied->EIMCtrlEmergency ? 0.9 : 1.0 };
                auto const brakelimit{ -2.2 * AccDesired / fMedAmax - 1.0}; //additional limit when hinted is too low
				auto const brakehinted{ -1.0 * mvOccupied->AIHintLocalBrakeAccFactor * AccDesired / fMedAmax }; //preffered by AI
                auto const brakeposition{ maxpos * clamp(std::max(brakelimit, brakehinted), 0.0, 1.0)};
                OK = ( brakeposition != mvOccupied->LocalBrakePosA );
                mvOccupied->LocalBrakePosA = brakeposition;
            }
            else {
                OK = mvOccupied->IncLocalBrakeLevel( 1 );
            }
            break;
        }
        case 1: {
            OK = mvOccupied->MainCtrlPos > 0;
            if( OK )
                mvOccupied->MainCtrlPos = 0;
            break;
        }
        case 2: {
            OK = mvOccupied->MainCtrlPos > 1;
            if( OK )
                mvOccupied->MainCtrlPos = 1;
            break;
        }
        case 3: {
            if( mvOccupied->UniCtrlIntegratedLocalBrakeCtrl ) {
                OK = mvOccupied->MainCtrlPos > 0;
                if( OK )
                    mvOccupied->MainCtrlPos = 0;
            }
            else {
                OK = mvOccupied->IncLocalBrakeLevel( 1 );
            }
            break;
        }
        default: {
            break;
        }
	}
	return OK;
}

// zmniejszenie siły hamowania
bool TController::DecBrake() {

    auto OK { false };
    auto const bs {
        ( ( BrakeSystem == TBrakeSystem::ElectroPneumatic ) && ( ForcePNBrake ) ) ?
            TBrakeSystem::Pneumatic :
            BrakeSystem };
    switch( bs ) {
        case TBrakeSystem::Individual: {
            auto const positionchange { 1 + std::floor( 0.5 + std::abs( AccDesired ) ) };
            OK = (
                mvOccupied->LocalBrake == TLocalBrake::ManualBrake ?
                    mvOccupied->DecManualBrakeLevel( positionchange ) :
                    mvOccupied->DecLocalBrakeLevel( positionchange ) );
            break;
        }
        case TBrakeSystem::Pneumatic: {
            auto deltaAcc { -1.0 };
            if( ( fBrake_a0[ 0 ] != 0.0 )
             || ( fBrake_a1[ 0 ] != 0.0 ) ) {
                auto const pos_diff { ( is_dmu() ? 0.25 : 1.0 ) };
                deltaAcc = -AccDesired * BrakeAccFactor() - ( fBrake_a0[ 0 ] + 4 * (/*GBH mvOccupied->BrakeCtrlPosR*/BrakeCtrlPosition - pos_diff )*fBrake_a1[ 0 ] );
            }
		    if (deltaAcc < 0) {
                if(/*GBH mvOccupied->BrakeCtrlPosR*/BrakeCtrlPosition > 0 ) {
                    OK = /*mvOccupied->*/BrakeLevelAdd( -0.25 );
                    //if ((deltaAcc < 5 * fBrake_a1[0]) && (mvOccupied->BrakeCtrlPosR >= 1.2))
                    //	mvOccupied->BrakeLevelAdd(-1.0);
                    /* if (mvOccupied->BrakeCtrlPosR < 0.74) GBH */
                    if( BrakeCtrlPosition < 0.74 )
                        /*mvOccupied->*/BrakeLevelSet( gbh_RP );
                }
		    }
            if( !OK ) {
                OK = mvOccupied->DecLocalBrakeLevel(2);
            }
            if( !OK ) {
			    OK = DecBrakeEIM();
		    }
    /*
    // NOTE: disabled, duplicate of AI's behaviour in UpdateSituation()
            if (mvOccupied->PipePress < 3.0)
                Need_BrakeRelease = true;
    */
            break;
        }
        case TBrakeSystem::ElectroPneumatic: {
            if( mvOccupied->EngineType == TEngineType::ElectricInductionMotor ) {
                if( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_EN57 ) {
                    if( mvOccupied->BrakeCtrlPos > mvOccupied->Handle->GetPos( bh_RP ) ) {
                        OK = mvOccupied->BrakeLevelAdd( -1.0 );
                    }
                }
                else {
                    OK = DecBrakeEIM();
                }
            }
            else if( mvOccupied->Handle->TimeEP == false ) {
                auto const initialbrakeposition { mvOccupied->fBrakeCtrlPos };
                auto const AccMax { std::min( fBrake_a0[ 0 ] + 12 * fBrake_a1[ 0 ], mvOccupied->MED_amax ) };
                mvOccupied->BrakeLevelSet(
                    interpolate(
                        mvOccupied->Handle->GetPos( bh_EPR ),
                        mvOccupied->Handle->GetPos( bh_EPB ),
                        clamp( -AccDesired / AccMax * mvOccupied->AIHintLocalBrakeAccFactor, 0.0, 1.0 ) ) );
                OK = ( mvOccupied->fBrakeCtrlPos != initialbrakeposition );
            }
            else {
				OK = false;
				if (mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos(bh_EPR)) {
					mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPR));
					OK = true;
				}
				if (mvOccupied->Handle->GetPos(bh_EPR) - mvOccupied->Handle->GetPos(bh_EPN) < 0.1) {
					OK = OK || mvOccupied->SwitchEPBrake(1);
				}
            }
            if( !OK ) {
                OK = mvOccupied->DecLocalBrakeLevel( 2 );
            }
            break;
        }
        default: {
            break;
        }
    }
    return OK;
}

void TController::LapBrake() {

    if( mvOccupied->Handle->TimeEP ) {
        if( mvOccupied->Handle->GetPos( bh_EPR ) - mvOccupied->Handle->GetPos( bh_EPN ) < 0.1 ) {
            mvOccupied->SwitchEPBrake( 0 );
        }
        else {
            mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_EPN ) );
        }
    }
}

void TController::ZeroLocalBrake() {

    if( mvOccupied->UniCtrlIntegratedLocalBrakeCtrl ) {
        while( DecBrakeEIM() ) {
            ;
        }
    }
    else {
        mvOccupied->DecLocalBrakeLevel( LocalBrakePosNo );
    }
}

bool TController::DecBrakeEIM()
{ // zmniejszenie siły hamowania
	bool OK = false;
	switch (mvOccupied->EIMCtrlType)
	{
        case 0: {
            if( mvOccupied->MED_amax != 9.81 ) {
                auto const desiredacceleration { ( mvOccupied->Vel > EU07_AI_NOMOVEMENT ? AccDesired : std::max( 0.0, AccDesired ) ) };
                auto const brakeposition { clamp( -1.0 * mvOccupied->AIHintLocalBrakeAccFactor * desiredacceleration / mvOccupied->MED_amax, 0.0, 1.0 ) };
                OK = ( brakeposition != mvOccupied->LocalBrakePosA );
                mvOccupied->LocalBrakePosA = brakeposition;
            }
            else {
                OK = mvOccupied->DecLocalBrakeLevel( 1 );
            }
            break;
        }
        case 1: {
            OK = mvOccupied->MainCtrlPos < 2;
            if( OK )
                mvOccupied->MainCtrlPos = 2;
            break;
        }
        case 2: {
            OK = mvOccupied->MainCtrlPos < 3;
            if( OK )
                mvOccupied->MainCtrlPos = 3;
            break;
        }
        case 3: {
            OK = mvOccupied->MainCtrlPos < mvOccupied->MainCtrlMaxDirChangePos;
            if( OK )
                mvOccupied->MainCtrlPos = mvOccupied->MainCtrlMaxDirChangePos;
            break;
        }
    }
	return OK;
}

bool TController::IncSpeed()
{ // zwiększenie prędkości; zwraca false, jeśli dalej się nie da zwiększać
    auto OK { false };
    switch (mvOccupied->EngineType)
    {
    case TEngineType::None: // McZapkie-041003: wagon sterowniczy
        if (mvControlling->MainCtrlPosNo > 0) // jeśli ma czym kręcić
            iDrivigFlags |= moveIncSpeed; // ustawienie flagi jazdy
        return false;
    case TEngineType::ElectricSeriesMotor:
        if (mvPantographUnit->EnginePowerSource.SourceType == TPowerSource::CurrentCollector) // jeśli pantografujący
        {
            if (fOverhead2 >= 0.0) // a jazda bezprądowa ustawiana eventami (albo opuszczenie)
                return false; // to nici z ruszania
            if (iOverheadZero) // jazda bezprądowa z poziomu toru ustawia bity
                return false; // to nici z ruszania
        }
        // TODO: move all fVoltage assignments to a single, more generic place
        if( mvControlling->EnginePowerSource.SourceType == TPowerSource::Accumulator ) {
            fVoltage = mvControlling->BatteryVoltage;
        }
        if ((!IsAnyMotorOverloadRelayOpen)
          &&(!mvControlling->ControlPressureSwitch)) {
            if ((mvControlling->IsMainCtrlNoPowerPos())
             || (mvControlling->StLinFlag)) { // youBy polecił dodać 2012-09-08 v367
                // na pozycji 0 przejdzie, a na pozostałych będzie czekać, aż się załączą liniowe (zgaśnie DelayCtrlFlag)
				if (Ready || (iDrivigFlags & movePress)) {
                    auto const usehighoverloadrelaythreshold {
                        ( mvControlling->TrainType != dt_ET42 ) // ET42 uses these variables for different purpose. TODO: fix this
                     && ( mvControlling->ImaxHi > mvControlling->ImaxLo )
                     && ( mvOccupied->Vel < ( mvControlling->IsMotorOverloadRelayHighThresholdOn() ? 30.0 : 20.0 ) )
                     && ( iVehicles - ControlledEnginesCount > 0 )
//                     && ( std::fabs( mvControlling->Im ) > 0.85 * mvControlling->Imax )
                     && ( ( mvControlling->Imax * mvControlling->EngineVoltage * ControlledEnginesCount )
                         / ( fMass * (
                             fAccGravity == 0.025 ?
                                -0.01 : // prevent div/0
                                fAccGravity - 0.025 ) )
                         < -2.8 ) };
                    // use series mode:
                    // if high threshold is set for motor overload relay,
                    // if the power station is heavily burdened,
                    // if it generates enough traction force
                    // to build up speed to 30/40 km/h for passenger/cargo train (10 km/h less if going uphill)
                    auto const sufficienttractionforce { std::abs( mvControlling->Ft ) * ControlledEnginesCount > ( IsHeavyCargoTrain ? 75 : 50 ) * 1000.0 };
                    auto const sufficientacceleration { AbsAccS >= ( IsHeavyCargoTrain ? 0.03 : IsCargoTrain ? 0.06 : 0.09 ) };
                    auto const seriesmodefieldshunting { ( mvControlling->ScndCtrlPos > 0 ) && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn == 1 ) };
                    auto const parallelmodefieldshunting { ( mvControlling->ScndCtrlPos > 0 ) && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) };
                    auto const minvoltage { ( mvPantographUnit->EnginePowerSource.SourceType == TPowerSource::CurrentCollector ? mvPantographUnit->EnginePowerSource.CollectorParameters.MinV : 0.0 ) };
                    auto const maxvoltage { ( mvPantographUnit->EnginePowerSource.SourceType == TPowerSource::CurrentCollector ? mvPantographUnit->EnginePowerSource.CollectorParameters.MaxV : 0.0 ) };
                    auto const seriesmodevoltage {
                        interpolate(
                            minvoltage,
                            maxvoltage,
                            ( IsHeavyCargoTrain ? 0.35 : 0.40 ) ) };
                    auto const useseriesmode = (
                        ( mvControlling->Imax > mvControlling->ImaxLo )
                     || ( true == usehighoverloadrelaythreshold )
                     || ( fVoltage < seriesmodevoltage )
                     || ( ( true == sufficientacceleration )
                       && ( true == sufficienttractionforce )
                       && ( mvOccupied->Vel <= ( IsCargoTrain ? 40 : 30 ) + ( seriesmodefieldshunting ? 5 : 0 ) - ( ( fAccGravity < -0.025 ) ? 10 : 0 ) ) ) );
                    // when not in series mode use the first available parallel mode configuration until 50/60 km/h for passenger/cargo train
                    // (if there's only one parallel mode configuration it'll be used regardless of current speed)
                    auto const usefieldshunting = (
                        ( mvControlling->StLinFlag )
                     && ( mvControlling->RList[ mvControlling->MainCtrlPos ].R < 0.01 )
                     && ( useseriesmode ?
                            mvControlling->RList[ mvControlling->MainCtrlPos ].Bn == 1 :
                            ( ( true == sufficientacceleration )
                           && ( true == sufficienttractionforce )
                           && ( mvOccupied->Vel <= ( IsCargoTrain ? 60 : 50 ) + ( parallelmodefieldshunting ? 5 : 0 ) ) ?
                                mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 :
                                mvControlling->MainCtrlPos == mvControlling->MainCtrlPosNo ) ) );

                    // if needed enable high threshold for overload relay...
                    if( mvControlling->TrainType != dt_ET42 ) { // ET42 uses these variables for different purpose. TODO: fix this
                        if( usehighoverloadrelaythreshold ) {
                            if( mvControlling->Imax < mvControlling->ImaxHi ) {
                                // to enable this setting we'll typically need the main controller in series mode (which is not guaranteed)
                                if( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) {
                                    if( false == mvControlling->IsScndCtrlNoPowerPos() ) {
                                        mvControlling->DecScndCtrl( 2 );
                                    }
                                    while( ( false == mvControlling->IsMainCtrlNoPowerPos() )
                                        && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) ) {
                                        mvControlling->DecMainCtrl( 1 ); // kręcimy nastawnik jazdy o 1 wstecz
                                    }
                                }
                                mvControlling->CurrentSwitch( true ); // rozruch wysoki (za to może się ślizgać)
                            }
                        }
                        // ...or disable high threshold for overload relay if no longer needed
                        else {
                            if( ( mvControlling->IsMotorOverloadRelayHighThresholdOn() )
                             && ( std::fabs( mvControlling->Im ) < mvControlling->ImaxLo ) ) {
                                mvControlling->CurrentSwitch( false ); // rozruch wysoki wyłącz
                            }
                        }
                    }

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
                            auto const sufficientpowermargin { fVoltage - seriesmodevoltage > ( IsHeavyCargoTrain ? 100.0 : 75.0 ) * ControlledEnginesCount };

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
                                        minvoltage :
                                        seriesmodevoltage )
                                > ( IsHeavyCargoTrain ? 100.0 : 75.0 ) * ControlledEnginesCount };

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
                             && ( mvControlling->MainCtrlPowerPos() > 1 ) ) {
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
            }
        }
//        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case TEngineType::Dumb:
        if (!IsAnyMotorOverloadRelayOpen)
            if (Ready || (iDrivigFlags & movePress)) //{(BrakePress<=0.01*MaxBrakePress)}
            {
                OK = IncSpeedEIM();
                if( !OK )
                    OK = mvControlling->IncMainCtrl(1);
                if (!OK)
                    OK = mvControlling->IncScndCtrl(1);
            }
        break;
    case TEngineType::DieselElectric:
        if ((!IsAnyMotorOverloadRelayOpen)
          &&(!mvControlling->ControlPressureSwitch)) {
            if ((mvControlling->IsMainCtrlNoPowerPos()) ||
                (mvControlling->StLinFlag)) { // youBy polecił dodać 2012-09-08 v367
                // na pozycji 0 przejdzie, a na pozostałych będzie czekać, aż się załączą liniowe (zgaśnie DelayCtrlFlag)
                if (Ready || (iDrivigFlags & movePress)) //{(BrakePress<=0.01*MaxBrakePress)}
                {
                    OK = IncSpeedEIM();
                    if( !OK )
                        OK = mvControlling->IncMainCtrl(1);
                    if (!OK)
                        OK = mvControlling->IncScndCtrl(1);
                }
            }
        }
        break;
    case TEngineType::ElectricInductionMotor:
        if( !IsAnyMotorOverloadRelayOpen ) {
            if( Ready || ( iDrivigFlags & movePress ) || ( mvOccupied->ShuntMode ) ) //{(BrakePress<=0.01*MaxBrakePress)}
            {
                OK = IncSpeedEIM();
                if( ( mvControlling->SpeedCtrl ) && ( mvControlling->Mains ) ) {
                    // cruise control
                    auto const couplinginprogress{ ( true == TestFlag( iDrivigFlags, moveConnect ) ) && ( true == TestFlag( OrderCurrentGet(), Connect ) ) };
                    auto const SpeedCntrlVel{ (
                        ( ActualProximityDist > std::max( 50.0, fMaxProximityDist ) ) || ( couplinginprogress ) ?
                            VelDesired :
                            min_speed( VelDesired, VelNext ) ) };
                    if( ( SpeedCntrlVel >= mvControlling->SpeedCtrlUnit.MinVelocity )
                     && ( SpeedCntrlVel - mvControlling->SpeedCtrlUnit.MinVelocity == quantize( SpeedCntrlVel - mvControlling->SpeedCtrlUnit.MinVelocity, mvControlling->SpeedCtrlUnit.VelocityStep ) ) ) {
                        SpeedCntrl( SpeedCntrlVel );
                    }
                    else if( SpeedCntrlVel > 0.1 ) {
                        SpeedCntrl( 0.0 );
                        mvControlling->DecScndCtrl( 2 );
                    }
                }
            }
        }
        break;
    case TEngineType::WheelsDriven:
        if (!mvControlling->CabActive)
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
		if ((mvControlling->SpeedCtrl)&&(mvControlling->Mains)) {// cruise control
            auto const couplinginprogress { ( true == TestFlag( iDrivigFlags, moveConnect ) ) && ( true == TestFlag( OrderCurrentGet(), Connect ) ) };
            auto const SpeedCntrlVel { (
                ( ActualProximityDist > std::max( 50.0, fMaxProximityDist ) ) || ( couplinginprogress ) ?
                    VelDesired :
                    min_speed( VelDesired, VelNext ) ) };
            if( ( SpeedCntrlVel >= mvControlling->SpeedCtrlUnit.MinVelocity )
             && ( SpeedCntrlVel - mvControlling->SpeedCtrlUnit.MinVelocity == quantize( SpeedCntrlVel - mvControlling->SpeedCtrlUnit.MinVelocity, mvControlling->SpeedCtrlUnit.VelocityStep ) ) ) {
                SpeedCntrl(SpeedCntrlVel);
            }
            else if (SpeedCntrlVel > 0.1) {
                SpeedCntrl(0.0);
                mvControlling->DecScndCtrl( 2 );
            }
		}
		if (mvControlling->EIMCtrlType > 0) {
			if (true == Ready)
			{
				bool max = (mvControlling->Vel > mvControlling->dizel_minVelfullengage)
				    	|| (mvControlling->SpeedCtrl && mvControlling->ScndCtrlPos > 0);
				DizelPercentage = (max ? 100 : 1);
			}
			break;
		}
        else {
            if( true == Ready ) {
                if( ( mvControlling->Vel > mvControlling->dizel_minVelfullengage )
                 && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn > 0 ) ) {
                    OK = mvControlling->IncMainCtrl( 1 );
                }
                if( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn == 0 ) {
                    OK = mvControlling->IncMainCtrl( 1 );
                }
            }
        }
        // TODO: move this check to a more suitable place, or scrap it
        if( false == mvControlling->Mains ) {
			SpeedCntrl(0.0);
            mvControlling->DecScndCtrl( 2 );
            cue_action( driver_hint::linebreakerclose );
            cue_action( driver_hint::converteron );
            cue_action( driver_hint::compressoron );
        }
		break;
    }
    return OK;
}

void TController::ZeroSpeed( bool const Enforce ) {

    while( DecSpeed( Enforce ) ) {
        ;
    }
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
                mvControlling->DecMainCtrl( std::min( mvControlling->MainCtrlPowerPos(), 2 ) );
//        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        return false;
    case TEngineType::ElectricSeriesMotor:
        OK = mvControlling->DecScndCtrl(2); // najpierw bocznik na zero
        if (!OK)
            OK = mvControlling->DecMainCtrl( std::min( mvControlling->MainCtrlPowerPos(), 2 ) );
//        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case TEngineType::Dumb:
    case TEngineType::DieselElectric:
        OK = mvControlling->DecScndCtrl(2);
        if (!OK)
            OK = mvControlling->DecMainCtrl( std::min( mvControlling->MainCtrlPowerPos(), ( 2 + (mvControlling->MainCtrlPowerPos() / 2) ) ) );
        break;
	case TEngineType::ElectricInductionMotor:
		OK = DecSpeedEIM();
		break;
    case TEngineType::WheelsDriven:
        if (!mvControlling->CabActive)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) < 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case TEngineType::DieselEngine:
		if (mvControlling->EIMCtrlType > 0)
		{
			DizelPercentage = 0;
			if (force) {
				SpeedCntrl(0.0); //wylacz od razu tempomat
				mvControlling->DecScndCtrl(2);
			}
			if ((VelDesired > 0.1) && (mvControlling->SpeedCtrlUnit.MinVelocity > VelDesired)) {
				SpeedCntrl(0.0); //wylacz od razu tempomat
				mvControlling->DecScndCtrl(2);
			}
			break;
		}

        if ((mvControlling->Vel > mvControlling->dizel_minVelfullengage))
        {
            if (mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0)
                OK = mvControlling->DecMainCtrl(1);
        }
        else {
            while( ( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn > 0 )
                && ( mvControlling->MainCtrlPowerPos() > 1 ) ) {
                OK = mvControlling->DecMainCtrl( 1 );
            }
        }
		if (force) { // przy aktywacji kabiny jest potrzeba natychmiastowego wyzerowania
			OK = mvControlling->DecMainCtrl((mvControlling->MainCtrlPowerPos() > 2 ? 2 : 1));
			SpeedCntrl(0.0); //wylacz od razu tempomat
			mvControlling->DecScndCtrl( 2 );
		}
        break;
    }
    return OK;
};

bool TController::IncSpeedEIM() {

    bool OK = false; // domyślnie false, aby wyszło z pętli while
    switch( mvControlling->EIMCtrlType ) {
        case 0:
            OK = mvControlling->IncMainCtrl( 1 );
            break;
        case 1:
            OK = mvControlling->MainCtrlPos < 6;
            if( OK ) {
                mvControlling->MainCtrlPos = 6;
            }
/*
            // TBD, TODO: set position based on desired acceleration?
            OK = mvControlling->MainCtrlPos < mvControlling->MainCtrlPosNo;
            if( OK ) {
                mvControlling->MainCtrlPos = clamp( mvControlling->MainCtrlPos + 1, 6, mvControlling->MainCtrlPosNo );
            }
*/
            break;
        case 2:
            OK = mvControlling->MainCtrlPos < 4;
            if( OK ) {
                mvControlling->MainCtrlPos = 4;
            }
            break;
    }
    return OK;
}

bool TController::DecSpeedEIM( int const Amount )
{ // zmniejszenie prędkości (ale nie hamowanie)
	bool OK = false; // domyślnie false, aby wyszło z pętli while
    switch( mvControlling->EIMCtrlType ) {
        case 0: {
            OK = mvControlling->DecMainCtrl( Amount );
            break;
        }
        case 1: {
            OK = mvControlling->MainCtrlPos > 4;
            if( OK ) {
                mvControlling->MainCtrlPos = 4;
            }
            break;
        }
        case 2: {
            if( ( AccDesired > 0 )
             && ( mvControlling->SpeedCtrlUnit.IsActive )
             && ( mvControlling->SpeedCtrlUnit.PowerStep > 0 )
             && ( mvControlling->SpeedCtrlUnit.DesiredPower > mvControlling->SpeedCtrlUnit.MinPower ) ) {
                mvControlling->SpeedCtrlPowerDec();
            }
            else {
                OK = mvControlling->MainCtrlPos > 2;
                if( OK ) {
                    mvControlling->MainCtrlPos = 2;
                }
            }
            break;
        }
        default: {
            break;
        }
	}
	return OK;
}

void TController::BrakeLevelSet(double b)
{ // ustawienie pozycji hamulca na wartość (b) w zakresie od -2 do BrakeCtrlPosNo
  // jedyny dopuszczalny sposób przestawienia hamulca zasadniczego
	if (BrakeCtrlPosition == b)
		return; // nie przeliczać, jak nie ma zmiany
	BrakeCtrlPosition = clamp(b, (double)gbh_MIN, (double)gbh_MAX);
}

bool TController::BrakeLevelAdd(double b)
{ // dodanie wartości (b) do pozycji hamulca (w tym ujemnej)
  // zwraca false, gdy po dodaniu było by poza zakresem
	BrakeLevelSet(BrakeCtrlPosition + b);
	return b > 0.0 ? (BrakeCtrlPosition < gbh_MAX) :
		(BrakeCtrlPosition > -1.0); // true, jeśli można kontynuować
}

// Ra: regulacja prędkości, wykonywana w każdym przebłysku świadomości AI
// ma dokręcać do bezoporowych i zdejmować pozycje w przypadku przekroczenia prądu
void TController::SpeedSet() {

    if( false == AIControllFlag ) { return; }

    switch (mvOccupied->EngineType)
    {
    case TEngineType::None: { // McZapkie-041003: wagon sterowniczy
        if (mvControlling->MainCtrlPosNo > 0)
        { // jeśli ma czym kręcić
            // TODO: sprawdzanie innego czlonu //if (!FuseFlagCheck())
            if( ( iDrivigFlags & moveIncSpeed ) == 0 ) {
                // przetok rozłączać od razu (no dependency on fActionTime)
                while( ( mvControlling->MainCtrlPos )
                    && ( mvControlling->DecMainCtrl( 1 ) ) ) {
                    ; // na zero
                }
                if( fActionTime >= 0.0 ) {
                    fActionTime = -5.0; // niech trochę potrzyma
                }
//                mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
            }
            else {
                // jak ma jechać
                if( fActionTime < 0.0 ) { break; }
                if( false == Ready )    { break; }

                if( mvOccupied->DirActive > 0 ) {
                    mvOccupied->DirectionForward(); //żeby EN57 jechały na drugiej nastawie
                }

                if( ( mvControlling->MainCtrlPos > 0 )
                 && ( false == mvControlling->StLinFlag ) ) {
                    // jak niby jedzie, ale ma rozłączone liniowe to na zero i czekać na przewalenie kułakowego
                    mvControlling->DecMainCtrl( 2 );
                }
                else {
                    // ruch nastawnika uzależniony jest od aktualnie ustawionej pozycji
                    switch( mvControlling->MainCtrlPos ) {
                        case 0:
                            if( mvControlling->MainCtrlActualPos ) {
                                // jeśli kułakowy nie jest wyzerowany to czekać na wyzerowanie
                                break;
                            }
                            mvControlling->IncMainCtrl( 1 ); // przetok; bez "break", bo nie ma czekania na 1. pozycji
                        case 1:
                            if( VelDesired >= 20 )
                                mvControlling->IncMainCtrl( 1 ); // szeregowa
                        case 2:
                            if( VelDesired >= 50 )
                                mvControlling->IncMainCtrl( 1 ); // równoległa
                        case 3:
                            if( VelDesired >= 80 )
                                mvControlling->IncMainCtrl( 1 ); // bocznik 1
                        case 4:
                            if( VelDesired >= 90 )
                                mvControlling->IncMainCtrl( 1 ); // bocznik 2
                        case 5:
                            if( VelDesired >= 100 )
                                mvControlling->IncMainCtrl( 1 ); // bocznik 3
                    }
                }
                if( mvControlling->MainCtrlPos ) // jak załączył pozycję
                {
                    fActionTime = -5.0; // niech trochę potrzyma
//                    mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                }
            }
        }
        break;
    }
    case TEngineType::ElectricSeriesMotor: {
/*
        if (Ready || (iDrivigFlags & movePress)) { // o ile może jechać
//            if (fAccGravity < -0.10) // i jedzie pod górę większą niż 10 promil
            { // procedura wjeżdżania na ekstremalne wzniesienia
                if( std::fabs( mvControlling->Im ) > 0.85 * mvControlling->Imax ) { // a prąd jest większy niż 85% nadmiarowego
                    if( mvControlling->Imax * mvControlling->EngineVoltage / ( fMass * fAccGravity ) < -2.8 ) // a na niskim się za szybko nie pojedzie
                    { // włączenie wysokiego rozruchu;
                        // (I*U)[A*V=W=kg*m*m/sss]/(m[kg]*a[m/ss])=v[m/s]; 2.8m/ss=10km/h
                        if( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) { // jeśli jedzie na równoległym, to zbijamy do szeregowego, aby włączyć wysoki rozruch
                            if( mvControlling->ScndCtrlPos > 0 ) { // jeżeli jest bocznik
                                mvControlling->DecScndCtrl( 2 ); // wyłączyć bocznik, bo może blokować skręcenie NJ
                            }
                            do { // skręcanie do bezoporowej na szeregowym
                                mvControlling->DecMainCtrl( 1 ); // kręcimy nastawnik jazdy o 1 wstecz
                            }
                            while( ( false == mvControlling->IsMainCtrlNoPowerPos() )
                                && ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) ); // oporowa zapętla
                        }
                        if( mvControlling->Imax < mvControlling->ImaxHi ) { // jeśli da się na wysokim
                            mvControlling->CurrentSwitch( true ); // rozruch wysoki (za to może się ślizgać)
                        }
                        if( ReactionTime > 0.1 ) {
                            ReactionTime = 0.1; // orientuj się szybciej
                        }
                    } // if (Im>Imin)
                }
                if( std::fabs( mvControlling->Im ) > 0.96 * mvControlling->Imax ) {// jeśli prąd jest duży (można 690 na 750)
                    if( mvControlling->ScndCtrlPos > 0 ) { // jeżeli jest bocznik
                        mvControlling->DecScndCtrl( 2 ); // zmniejszyć bocznik
                    }
                    else {
                        mvControlling->DecMainCtrl( 1 ); // kręcimy nastawnik jazdy o 1 wstecz
                    }
                }
            }
//            else // gdy nie jedzie ambitnie pod górę
            { // sprawdzenie, czy rozruch wysoki jest potrzebny
                if( ( mvControlling->Imax > mvControlling->ImaxLo )
                 && ( mvOccupied->Vel >= 30.0 ) ) { // jak się rozpędził
//                        if (fAccGravity > -0.02) // a i pochylenie mnijsze niż 2‰
                    mvControlling->CurrentSwitch( false ); // rozruch wysoki wyłącz
                }
            }
        }
*/
        break;
    }
    case TEngineType::Dumb: {
        break;
    }
    case TEngineType::DieselElectric: {
        break;
    }
    case TEngineType::ElectricInductionMotor: {
        break;
    }
    case TEngineType::DieselEngine: {
        // Ra 2014-06: "automatyczna" skrzynia biegów...
        auto const &motorparams { mvControlling->MotorParam[ mvControlling->ScndCtrlPos ] };
        auto const velocity { ( mvControlling->ShuntMode ? mvControlling->AnPos : 1.0 ) * mvControlling->Vel };
        if( false == motorparams.AutoSwitch ) {
            // gdy biegi ręczne
            // jak prędkość większa niż procent maksymalnej na danym biegu, wrzucić wyższy
            if( velocity > 0.75 * motorparams.mfi ) {
                // ...presuming there is a higher gear
                if( mvControlling->ScndCtrlPos < mvControlling->ScndCtrlPosNo ) {
                    mvControlling->DecMainCtrl( 2 );
                    while( ( mvControlling->IncScndCtrl( 1 ) )
                        && ( mvControlling->MotorParam[ mvControlling->ScndCtrlPos ].mIsat == 0.0 ) ) { // jeśli bieg jałowy to kolejny
                        ;
                    }
                }
            }
            // jak prędkość mniejsza niż minimalna na danym biegu, wrzucić niższy
            else if( velocity < motorparams.fi ) {
                // ... but ensure we don't switch all way down to 0
                if( mvControlling->ScndCtrlPos > 1 ) {
                    mvControlling->DecMainCtrl( 2 );
                    while( ( mvControlling->ScndCtrlPos > 1 ) // repeat the entry check as we're working in a loop
                        && ( mvControlling->DecScndCtrl( 1 ) )
                        && ( mvControlling->MotorParam[ mvControlling->ScndCtrlPos ].mIsat == 0.0 ) ) { // jeśli bieg jałowy to kolejny
                        ;
                    }
                }
            }
        }
        break;
    }
    default: {
        break;
    }
    } // enginetype
};

void TController::SpeedCntrl(double DesiredSpeed)
{
    if( false == mvControlling->SpeedCtrl ) { return; }

	if (mvControlling->EngineType == TEngineType::DieselEngine)
	{
		if (DesiredSpeed < 0.1) {
			mvControlling->DecScndCtrl(2);
			DesiredSpeed = 0;
        }
		else if (mvControlling->ScndCtrlPos < 1) {
			mvControlling->IncScndCtrl(1);
		}
		mvControlling->RunCommand("SpeedCntrl", DesiredSpeed, mvControlling->CabActive);
	}
	else
	if (mvControlling->ScndCtrlPosNo == 1)
	{
		mvControlling->IncScndCtrl(1);
		mvControlling->RunCommand("SpeedCntrl", DesiredSpeed, mvControlling->CabActive);
	}
	else if ((mvControlling->ScndCtrlPosNo > 1) && (!mvOccupied->SpeedCtrlTypeTime))
	{
		int DesiredPos = 1 + mvControlling->ScndCtrlPosNo * ((DesiredSpeed - 1.0) / mvControlling->Vmax);
        while( ( mvControlling->ScndCtrlPos > DesiredPos ) && ( true == mvControlling->DecScndCtrl( 1 ) ) ) { ; } // all work is done in the condition loop
        while( ( mvControlling->ScndCtrlPos < DesiredPos ) && ( true == mvControlling->IncScndCtrl( 1 ) ) ) { ; } // all work is done in the condition loop
	}

    if( ( mvControlling->SpeedCtrlUnit.PowerStep > 0 ) && ( mvControlling->ScndCtrlPos > 0 ) ) {
		while (mvControlling->SpeedCtrlUnit.DesiredPower < mvControlling->SpeedCtrlUnit.MaxPower)
		{
			mvControlling->SpeedCtrlPowerInc();
		}
	}
};

void TController::SetTimeControllers()
{
    // TBD, TODO: rework this method to use hint system and regardless of driver type
    if( false == AIControllFlag || 0 == mvOccupied->CabActive ) { return; }

	//1. Check the type of Main Brake Handle
    if( BrakeSystem == TBrakeSystem::Pneumatic || ForcePNBrake )
    {
		if (mvOccupied->Handle->Time)
		{
            auto const pressuredifference { mvOccupied->Handle->GetCP() - ( mvOccupied->HighPipePress - BrakeCtrlPosition * 0.25*mvOccupied->DeltaPipePress ) };
			if ((BrakeCtrlPosition > 0) && (pressuredifference > 0.05))
				mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_FB));
			else if ((BrakeCtrlPosition > 0) && (pressuredifference < -0.05))
				mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_RP));
			else if (BrakeCtrlPosition == 0)
				mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_RP));
			else if (BrakeCtrlPosition == -1)
				mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_FS));
			else if (BrakeCtrlPosition == -2)
				mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_NP));
		}
        if( mvOccupied->BrakeHandle == TBrakeHandle::FV4a ) {
            mvOccupied->BrakeLevelSet( BrakeCtrlPosition );
        }
        if( ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_K8P )
         || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_EN57 ) ) {
            if( BrakeCtrlPosition == 0 ) {
                mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_RP ) );
            }
            else if( BrakeCtrlPosition == -1 ) {
                mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_FS ) );
            }
            else if( BrakeCtrlPosition == -2 ) {
                mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_NP ) );
            }
            else if( BrakeCtrlPosition > 4.5 ) {
                mvOccupied->BrakeLevelSet( 10 );
            }
            else if( BrakeCtrlPosition > 3.70 ) {
                mvOccupied->BrakeLevelSet( 9 );
            }
            else {
                mvOccupied->BrakeLevelSet( round( ( BrakeCtrlPosition * 0.4 - 0.1 ) / 0.15 ) );
            }
		}
	}
	//2. Check the type of Secondary Brake Handle

	//3. Check the type od EIMCtrlType
/*
	if (mvOccupied->EIMCtrlType > 0)
	{
		if (mvOccupied->EIMCtrlType == 1) //traxx
		{
			if (mvOccupied->LocalBrakePosA > 0.95) mvOccupied->MainCtrlPos = 0;
		}
		else if (mvOccupied->EIMCtrlType == 2) //elf
		{
			if (mvOccupied->LocalBrakePosA > 0.95) mvOccupied->MainCtrlPos = 1;
		}
	}
*/
    if( ( mvOccupied->UniCtrlIntegratedLocalBrakeCtrl )
     && ( mvOccupied->LocalBrakePosA > 0.95 ) ) {
        while( IncBrakeEIM() ) { ; }
    }

	//4. Check Speed Control System
	if (mvOccupied->EngineType == TEngineType::ElectricInductionMotor && mvOccupied->ScndCtrlPosNo > 1 && mvOccupied->SpeedCtrlTypeTime)
	{
		double SpeedCntrlVel =
			(ActualProximityDist > std::max(50.0, fMaxProximityDist)) ?
			VelDesired :
			min_speed(VelDesired, VelNext);
		SpeedCntrlVel = 10 * std::floor(SpeedCntrlVel*0.1);
		if (mvOccupied->ScndCtrlPosNo == 4)
		{
			if (mvOccupied->NewSpeed + 0.1 < SpeedCntrlVel)
				mvOccupied->ScndCtrlPos = 3;
			if (mvOccupied->NewSpeed - 0.1 > SpeedCntrlVel)
				mvOccupied->ScndCtrlPos = 1;
		}
	}
	//5. Check Main Controller in Dizels
	//5.1. Digital controller in DMUs with hydro
	if ((mvControlling->EngineType == TEngineType::DieselEngine) && (mvControlling->EIMCtrlType == 3))
	{

        DizelPercentage_Speed = DizelPercentage; //wstepnie procenty
		auto MinVel{ std::min(mvControlling->hydro_TC_LockupSpeed, mvControlling->Vmax / 6) }; //minimal velocity
		//when speed controll unit is active - start with the procedure
		if ((mvControlling->SpeedCtrl) && (mvControlling->ScndCtrlPos > 0)) {
			if ((mvControlling->ScndCtrlPos > 0) && (mvControlling->Vel < 1 + mvControlling->SpeedCtrlUnit.StartVelocity) && (DizelPercentage > 0))
				DizelPercentage_Speed = 101; //keep last position to start
		}
		else if (VelDesired > MinVel) //more power for faster ride
		{
			auto const Factor{ 10 * (mvControlling->Vmax) / (mvControlling->Vmax + 3 * mvControlling->Vel) };
			auto DesiredPercentage{ clamp(
				(VelDesired > mvControlling->Vel ?
					(VelDesired - mvControlling->Vel) / Factor :
					0),
				0.0, 1.0) }; //correction for reaching desired velocity
			if ((VelDesired < 0.5 * mvControlling->Vmax) //low velocity and reaching desired
				&& (VelDesired - mvControlling->Vel < 10)) {
				DesiredPercentage = std::min(DesiredPercentage, 0.75);
			}
			DizelPercentage_Speed = std::round(DesiredPercentage * DizelPercentage);
		}
		else //slow acceleration during shunting wth minimal velocity
		{
			DizelPercentage_Speed = std::min(DizelPercentage, mvControlling->Vel < 0.99 * VelDesired ? 1 : 0);
		}

        auto const DizelActualPercentage { int(100.4 * mvControlling->eimic_real) };

        auto const PosInc { mvControlling->MainCtrlPosNo };
        auto PosDec { 0 };
        for( int i = PosInc; i >= 0; --i ) {
            if( ( mvControlling->UniCtrlList[ i ].SetCtrlVal <= 0 )
             && ( mvControlling->UniCtrlList[ i ].SpeedDown > 0.01 ) ) {
                PosDec = i;
                break;
            }
        }

        if( std::abs( DizelPercentage_Speed - DizelActualPercentage ) > ( DizelPercentage > 1 ? 0 : 0 ) ) {

            if( ( PosDec > 0 )
             && ( ( DizelActualPercentage - DizelPercentage_Speed > 50 )
               || ( ( DizelPercentage_Speed == 0 )
                 && ( DizelActualPercentage > 10 ) ) ) ) {
                //one position earlier should be fast decreasing
                PosDec -= 1;
            }
            auto const DesiredPos { (
                DizelPercentage_Speed > DizelActualPercentage ?
                    PosInc :
                    PosDec ) };
            while( mvControlling->MainCtrlPos > DesiredPos ) { mvControlling->DecMainCtrl( 1 ); }
            while( mvControlling->MainCtrlPos < DesiredPos ) { mvControlling->IncMainCtrl( 1 ); }
		}
		if (BrakeCtrlPosition < 0.1) //jesli nie hamuje
			mvOccupied->BrakeLevelSet(mvControlling->UniCtrlList[mvControlling->MainCtrlPos].mode); //zeby nie bruzdzilo machanie zespolonym
	}
	else
	//5.2. Analog direct controller
	if ((mvControlling->EngineType == TEngineType::DieselEngine)&&(mvControlling->Vmax>30))
	{
		int MaxPos = mvControlling->MainCtrlPosNo;
		int MinPos = MaxPos;
		for (int i = MaxPos; (i > 1) && (mvControlling->RList[i].Mn > 0); i--) MinPos = i;
		if ((MaxPos > MinPos)&&(mvControlling->MainCtrlPos>0)&&(AccDesired>0))
		{
			double Factor = 5 * (mvControlling->Vmax) / (mvControlling->Vmax + mvControlling->Vel);
			int DesiredPos = MinPos + (MaxPos - MinPos)*(VelDesired > mvControlling->Vel ? (VelDesired - mvControlling->Vel) / Factor : 0);
			if (DesiredPos > MaxPos) DesiredPos = MaxPos;
			if (DesiredPos < MinPos) DesiredPos = MinPos;
			if (!mvControlling->SlippingWheels)
			{
				while (mvControlling->MainCtrlPos > DesiredPos) mvControlling->DecMainCtrl(1);
				if (mvControlling->Vel>mvControlling->dizel_minVelfullengage)
				  while (mvControlling->MainCtrlPos < DesiredPos) mvControlling->IncMainCtrl(1);
			}
		}
	}
    // 5.5 universal control for diesel electric vehicles
	if ((mvControlling->EngineType == TEngineType::DieselElectric) && (mvControlling->EIMCtrlType == 3))
	{
        // NOTE: partial implementation for speedinc/dec
        // TBD, TODO: implement fully?
        if( ( AccDesired >= 0.0 )
         && ( mvControlling->StLinFlag ) ) {

            auto const PosInc { mvControlling->MainCtrlPosNo };
            auto PosKeep { 0 };
            auto PosDec{ 0 };
            for( int i = PosInc; i >= 0; --i ) {
                if( mvControlling->UniCtrlList[ i ].SetCtrlVal <= 0 ) {
                    if( mvControlling->UniCtrlList[ i ].SpeedDown == 0.0 ) {
                        PosKeep = i;
                    }
                    if( mvControlling->UniCtrlList[ i ].SpeedDown > 0.01 ) {
                        PosDec = i;
                        break;
                    }
                }
            }

            auto const DesiredPos { (
                AccDesired > AbsAccS + 0.05 ? PosInc :
                AccDesired < AbsAccS - 0.05 ? PosDec :
                PosKeep ) };

            while( ( mvControlling->MainCtrlPos > DesiredPos ) && mvControlling->DecMainCtrl( 1 ) ) { ; }
            while( ( mvControlling->MainCtrlPos < DesiredPos ) && mvControlling->IncMainCtrl( 1 ) ) { ; }
        }
    }
	//6. UniversalBrakeButtons
	//6.1. Checking flags for Over pressure
	if (std::abs(BrakeCtrlPosition - gbh_FS)<0.5) {
		UniversalBrakeButtons |= (TUniversalBrake::ub_HighPressure | TUniversalBrake::ub_Overload);
	}
	else {
		UniversalBrakeButtons &= ~(TUniversalBrake::ub_HighPressure | TUniversalBrake::ub_Overload);
	}
	//6.2. Setting buttons
	for (int i = 0; i < 3; i++) {
		mvOccupied->UniversalBrakeButton(i, (UniversalBrakeButtons & mvOccupied->UniversalBrakeButtonFlag[i]));
	}
};

void TController::CheckTimeControllers()
{
    // TODO: rework this method to use hint system and regardless of driver type
    if( false == AIControllFlag || 0 == mvControlling->CabActive ) { return; }

	//1. Check the type of Main Brake Handle
    if( BrakeSystem == TBrakeSystem::ElectroPneumatic && mvOccupied->Handle->TimeEP && !ForcePNBrake )
    {
		mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPN));
	}
    if( ( BrakeSystem == TBrakeSystem::Pneumatic || ForcePNBrake ) && mvOccupied->Handle->Time )
    {
		if (BrakeCtrlPosition > 0)
			mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_MB));
		else
			mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_RP));
	}
	//2. Check the type of Secondary Brake Handle

	//3. Check the type od EIMCtrlType
	if (mvOccupied->EIMCtrlType > 0)
	{
		if (mvOccupied->EIMCtrlType == 1) //traxx
		{
			if (mvOccupied->MainCtrlPos > 3) mvOccupied->MainCtrlPos = 5;
			if (mvOccupied->MainCtrlPos < 3) mvOccupied->MainCtrlPos = 1;
		}
		else if (mvOccupied->EIMCtrlType == 2) //elf
		{
			if (mvOccupied->eimic > 0) mvOccupied->MainCtrlPos = 3;
			if (mvOccupied->eimic < 0) mvOccupied->MainCtrlPos = 2;
		}
	}
	//4. Check Speed Control System
	if (mvOccupied->EngineType == TEngineType::ElectricInductionMotor && mvOccupied->ScndCtrlPosNo>1 && mvOccupied->SpeedCtrlTypeTime)
	{
		if (mvOccupied->ScndCtrlPosNo == 4)
		{
				mvOccupied->ScndCtrlPos = 2;
		}
	}

	//5. Check Main Controller in Dizels
	//5.1. Digital controller in DMUs with hydro
	if ((mvControlling->EngineType == TEngineType::DieselEngine) && (mvControlling->EIMCtrlType == 3))
	{
		int DizelActualPercentage = 100.4 * mvControlling->eimic_real;
		int NeutralPos = mvControlling->MainCtrlPosNo - 1; //przedostatnia powinna wstrzymywać - hipoteza robocza
		for (int i = mvControlling->MainCtrlPosNo; i >= 0; i--)
			if ((mvControlling->UniCtrlList[i].SetCtrlVal <= 0) && (mvControlling->UniCtrlList[i].SpeedDown < 0.01)) //niby zero, ale nie zmniejsza procentów
			{
				NeutralPos = i;
				break;
			}
		if (BrakeCtrlPosition < 0.1) //jesli nie hamuje
		{
			if ((DizelActualPercentage >= DizelPercentage_Speed) && (mvControlling->MainCtrlPos > NeutralPos))
				while (mvControlling->MainCtrlPos > NeutralPos) mvControlling->DecMainCtrl(1);
			if ((DizelActualPercentage <= DizelPercentage_Speed) && (mvControlling->MainCtrlPos < NeutralPos))
				while (mvControlling->MainCtrlPos < NeutralPos) mvControlling->IncMainCtrl(1);
			mvOccupied->BrakeLevelSet(mvControlling->UniCtrlList[mvControlling->MainCtrlPos].mode); //zeby nie bruzdzilo machanie zespolonym
		}
	}
};

// otwieranie/zamykanie drzwi w składzie albo (tylko AI) EZT
void TController::Doors( bool const Open, int const Side ) {
    // otwieranie drzwi
    if( true == Open ) {

        auto const lewe = ( pVehicle->DirectionGet() > 0 ) ? 1 : 2;
        auto const prawe = 3 - lewe;
        // grant door control permission if it's not automatic
        // TBD: stricter requirements?
        if( true == pVehicle->MoverParameters->Doors.permit_needed ) {
            if( Side & prawe ) {
                cue_action( driver_hint::doorrightpermiton );
            }
            if( Side & lewe ) {
                cue_action( driver_hint::doorleftpermiton );
            }
        }
        // consist-wide remote signals to open doors doors
        if( ( pVehicle->MoverParameters->Doors.open_control == control_t::conductor )
         || ( pVehicle->MoverParameters->Doors.open_control == control_t::driver )
            // NOTE: disabled for mixed controls, leave it up to passengers to open doors by themselves
/*         || ( pVehicle->MoverParameters->Doors.open_control == control_t::mixed ) */ ) {
            if( Side & prawe ) {
                cue_action( driver_hint::doorrightopen );
            }
            if( Side & lewe ) {
                cue_action( driver_hint::doorleftopen );
            }
        }
    }
    // zamykanie drzwi
    else {
        // if the doors are already closed and locked then there's nothing to do
        if( ( false == doors_permit_active() )
         && ( false == doors_open() ) ) {
            iDrivigFlags &= ~moveDoorOpened;
            return;
        }

        if( true == doors_open() ) {
            if( ( true == mvOccupied->Doors.has_warning )
             && ( false == mvOccupied->DepartureSignal )
             && ( true == TestFlag( iDrivigFlags, moveDoorOpened ) ) ) {
                cue_action( driver_hint::departuresignalon ); // załącenie bzyczka
                fActionTime = -1.5 - 0.1 * Random( 10 ); // 1.5-2.5 second wait
            }
        }

        if( ( false == AIControllFlag ) || ( fActionTime > -0.5 ) ) {
            // ai doesn't close the door until it's free to depart, but human driver has free reign to do stupid things
            if( true == doors_open() ) {
                // consist-wide remote signals to close doors
                if( ( pVehicle->MoverParameters->Doors.close_control == control_t::conductor )
                 || ( pVehicle->MoverParameters->Doors.close_control == control_t::driver )
                 || ( pVehicle->MoverParameters->Doors.close_control == control_t::mixed ) ) {
                    cue_action( driver_hint::doorrightclose );
                    cue_action( driver_hint::doorleftclose );
                }
            }
            if( true == doors_permit_active() ) {
                cue_action( driver_hint::doorrightpermitoff );
                cue_action( driver_hint::doorleftpermitoff );
            }
            // if applicable close manually-operated doors in vehicles which may ignore remote signals
            {
                auto *vehicle = pVehicles[ end::front ]; // pojazd na czole składu
                while( vehicle != nullptr ) {
                    // zamykanie drzwi w pojazdach - flaga zezwolenia była by lepsza
                    auto const ismanualdoor {
                        ( vehicle->MoverParameters->Doors.auto_velocity == -1.f )
                     && ( ( vehicle->MoverParameters->Doors.close_control == control_t::passenger )
                       || ( vehicle->MoverParameters->Doors.close_control == control_t::mixed ) ) };

                    if( ( true == ismanualdoor )
                     && ( ( vehicle->LoadExchangeTime() == 0.f )
                       || ( vehicle->MoverParameters->Vel > EU07_AI_MOVEMENT ) ) ) {
                        vehicle->MoverParameters->OperateDoors( side::right, false, range_t::local );
                        vehicle->MoverParameters->OperateDoors( side::left, false, range_t::local );
                    }
                    vehicle = vehicle->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
                }
            }
            fActionTime = -2.0 - 0.1 * Random( 15 ); // 2.0-3.5 sec wait, +potentially 0.5 remaining
            iDrivigFlags &= ~moveDoorOpened; // zostały zamknięte - nie wykonywać drugi raz

            if( ( true == mvOccupied->DepartureSignal )
             && ( Random( 100 ) < Random( 100 ) ) ) {
                // potentially turn off the warning before actual departure
                // TBD, TODO: dedicated buzzer duration counter?
                cue_action( driver_hint::departuresignaloff );
            }
        }
    }
}

// returns true if any vehicle in the consist has open doors
bool
TController::doors_open() const {

    return (
        IsAnyDoorOpen[ side::right ]
     || IsAnyDoorOpen[ side::left ] );
}

bool
TController::doors_permit_active() const {

    return (
        IsAnyDoorPermitActive[ side::right ]
     || IsAnyDoorPermitActive[ side::left ] );
}

void
TController::announce( announcement_t const Announcement ) {

    m_lastannouncement = Announcement;

    if( IsCargoTrain ) {
        // cargo trains aren't likely to have announcement systems, so we can skip the procedure altogether
        return;
    }

    auto *vehicle { pVehicles[ end::front ] };
    while( vehicle ) {
        vehicle->announce( Announcement );
        vehicle = vehicle->Next();
    }
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
        fOverhead1 = NewValue1; // informacja o napięciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
        fOverhead2 = NewValue2; // informacja o sposobie jazdy (-1=normalnie, 0=bez prądu, >0=z
        // opuszczonym i ograniczeniem prędkości)
        return true; // załatwione
    }

    if (NewCommand == "Emergency_brake") // wymuszenie zatrzymania, niezależnie kto prowadzi
    { // Ra: no nadal nie jest zbyt pięknie
//        SetVelocity(0, 0, reason);
        mvOccupied->PutCommand("Emergency_brake", 1.0, 1.0, mvOccupied->Loc);
        return true; // załatwione
    }

    if (NewCommand.compare(0, 10, "Timetable:") == 0)
    { // przypisanie nowego rozkładu jazdy, również prowadzonemu przez użytkownika
        NewCommand.erase(0, 10); // zostanie nazwa pliku z rozkładem
#if LOGSTOPS
        WriteLog("New timetable for " + pVehicle->asName + ": " + NewCommand); // informacja
#endif
        TrainParams = TTrainParameters(NewCommand); // rozkład jazdy
        tsGuardSignal = sound_source { sound_placement::internal, 2 * EU07_SOUND_CABCONTROLSCUTOFFRANGE }; // wywalenie kierownika
        m_lastexchangestop.clear();
        if (NewCommand != "none")
        {
            if (false == TrainParams.LoadTTfile(
                    std::string(Global.asCurrentSceneryPath.c_str()), floor(NewValue2 + 0.5),
                    NewValue1)) // pierwszy parametr to przesunięcie rozkładu w czasie
            {
                if (ConversionError == -8)
                    ErrorLog("Missed timetable: " + NewCommand);
                WriteLog("Cannot load timetable file " + NewCommand + "\r\nError " +
                         std::to_string(ConversionError) + " in position " + std::to_string(TrainParams.StationCount));
                NewCommand = ""; // puste, dla wymiennej tekstury
            }
            else
            { // inicjacja pierwszego przystanku i pobranie jego nazwy
                // HACK: clear the potentially set door state flag to ensure door get opened if applicable
                if( true == AIControllFlag ) {
                    // simplified door closing procedure, to sync actual door state with the door state flag
                    // NOTE: this may result in visually ugly quick switch between closing and opening the doors, but eh
                    if( ( pVehicle->MoverParameters->Doors.close_control == control_t::driver )
                     || ( pVehicle->MoverParameters->Doors.close_control == control_t::mixed ) ) {
                        pVehicle->MoverParameters->OperateDoors( side::right, false );
                        pVehicle->MoverParameters->OperateDoors( side::left, false );
                    }
                    if( pVehicle->MoverParameters->Doors.permit_needed ) {
                        pVehicle->MoverParameters->PermitDoors( side::right, false );
                        pVehicle->MoverParameters->PermitDoors( side::left, false );
                    }
                }
                iDrivigFlags &= ~( moveDoorOpened );

                TrainParams.UpdateMTable( simulation::Time, TrainParams.NextStationName );
                TrainParams.StationIndexInc(); // przejście do następnej
                TrainParams.StationStart = TrainParams.StationIndex;
                asNextStop = TrainParams.NextStop();
                m_lastannouncement = announcement_t::idle;
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
                    lookup =
                        FileExists(
                            { Global.asCurrentSceneryPath + NewCommand, szSoundPath + NewCommand },
                            { ".ogg", ".flac", ".wav" } );
                    if( false == lookup.first.empty() ) {
                        //  wczytanie dźwięku odjazdu w wersji radiowej (słychać tylko w kabinie)
                        tsGuardSignal = sound_source( sound_placement::internal, EU07_SOUND_HANDHELDRADIORANGE ).deserialize( lookup.first + lookup.second, sound_type::single );
                        iGuardRadio = iRadioChannel;
                    }
                }
                NewCommand = TrainParams.Relation2; // relacja docelowa z rozkładu
            }
            // jeszcze poustawiać tekstury na wyświetlaczach
            TDynamicObject *p = pVehicles[end::front];
            while (p)
            {
                p->DestinationSet(NewCommand, TrainParams.TrainName); // relacja docelowa
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
            iDirectionOrder=mvOccupied->CabOccupied;
           if (!iDirectionOrder) iDirectionOrder=1;
          }
        */
        // jeśli wysyłane z Trainset, to wszystko jest już odpowiednio ustawione
        // if (!NewLocation) //jeśli wysyłane z Trainset
        // if (mvOccupied->CabActive*mvOccupied->V*NewValue1<0) //jeśli zadana prędkość niezgodna z
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
        if ((NewValue1 != 0.0) && (OrderCurrentGet() != Obey_train))
        { // o ile jazda
            if( false == iEngineActive ) {
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
        if (false == iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpalić silnik najpierw
        OrderNext(Shunt); // zamieniamy w aktualnej pozycji, albo dodajey za odpaleniem silnika
        if (NewValue1 != 0.0)
        {
            // if (iVehicleCount>=0) WriteLog("Skasowano ilosć wagonów w ShuntVelocity!");
            iVehicleCount = -2; // wartość neutralna
            CheckVehicles(); // zabrać to do OrderCheck()
        }
        // dla prędkości ujemnej przestawić nawrotnik do tyłu? ale -1=brak ograniczenia !!!!
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
        TOrders o = OrderCurrentGet(); // co robił przed zmianą kierunku
        if (false == iEngineActive)
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
        if (

            iDirectionOrder != iDirection)
            OrderNext(Change_direction); // zadanie komendy do wykonania
        if (o >= Shunt) // jeśli jazda manewrowa albo pociągowa
            OrderNext(o); // to samo robić po zmianie
        else if (!o) // jeśli wcześniej było czekanie
            OrderNext(Shunt); // to dalej jazda manewrowa
        if (mvOccupied->Vel >= EU07_AI_MOVEMENT) // jeśli jedzie
            iDrivigFlags &= ~moveStartHorn; // to bez trąbienia po ruszeniu z zatrzymania
        // Change_direction wykona się samo i następnie przejdzie do kolejnej komendy
        return true;
    }

    if (NewCommand == "Obey_train") {
        if( false == iEngineActive ) {
            OrderNext( Prepare_engine ); // trzeba odpalić silnik najpierw
        }
        OrderNext( Obey_train );
        // if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
        OrderCheck(); // jeśli jazda pociągowa teraz, to wykonać niezbędne operacje

        return true;
    }

    if (NewCommand == "Bank") {
        if( false == iEngineActive ) {
            OrderNext( Prepare_engine ); // trzeba odpalić silnik najpierw
        }
        OrderNext( Bank );
        // if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
        OrderCheck(); // jeśli jazda pociągowa teraz, to wykonać niezbędne operacje

        return true;
    }

    if( ( NewCommand == "Shunt" ) || ( NewCommand == "Loose_shunt" ) )
    { // NewValue1 - ilość wagonów (-1=wszystkie); NewValue2: 0=odczep, 1..63=dołącz, -1=bez zmian
        //-3,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1), zmienić kierunek i czekać w trybie pociągowym
        //-2,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1), zmienić kierunek i czekać
        //-2, y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i czekać
        //-1,-y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i jechać w powrotną stronę
        //-1, y - podłączyć do całego stojącego składu (sprzęgiem y>=1) i jechać dalej
        //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagonów nie ma sensu)
        // 0, 0 - odczepienie lokomotywy
        // x,-y - podłączyć się do składu (sprzęgiem y>=1), a następnie odczepić i zabrać (x) wagonów
        // 1, 0 - odczepienie lokomotywy z jednym wagonem
        iDrivigFlags &= ~moveStopHere; // podjeżanie do semaforów zezwolone
        if( false == iEngineActive ) {
            OrderNext( Prepare_engine ); // trzeba odpalić silnik najpierw
        }
        if (NewValue2 != 0) // jeśli podany jest sprzęg
        {
            iCoupler = std::floor( std::abs( NewValue2 ) ); // jakim sprzęgiem
            OrderNext(Connect); // najpierw połącz pojazdy
            if (NewValue1 >= 0.0) // jeśli ilość wagonów inna niż wszystkie
            { // to po podpięciu należy się odczepić
                iDirectionOrder = -iDirection; // zmiana na ciągnięcie
                OrderPush(Change_direction); // najpierw zmień kierunek, bo odczepiamy z tyłu
                OrderPush(Disconnect); // a odczep już po zmianie kierunku
                if( ( NewValue2 > 0.0 )
                 && ( NewCommand == "Loose_shunt" ) ) {
                    // after decoupling continue pushing in the original direction
                    // NOTE: for backward compatibility this option isn't supported for basic shunting mode
                    iDirectionOrder = iDirection; // back to pushing
                    OrderPush( Change_direction );
                }
            }
            else if (NewValue2 < 0.0) // jeśli wszystkie, a sprzęg ujemny, to tylko zmiana kierunku po podczepieniu
            { // np. Shunt -1 -3
                iDirectionOrder = -iDirection; // jak się podczepi, to jazda w przeciwną stronę
                OrderNext(Change_direction);
            }
            WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać, chyba że już jedzie
        }
        else { // if (NewValue2==0.0) //zerowy sprzęg
            if( NewValue1 >= 0.0 ) {
                // jeśli ilość wagonów inna niż wszystkie będzie odczepianie,
                // ale jeśli wagony są z przodu, to trzeba najpierw zmienić kierunek
                if( ( mvOccupied->Couplers[ mvOccupied->DirAbsolute > 0 ? end::rear : end::front ].Connected == nullptr ) // z tyłu nic
                 && ( mvOccupied->Couplers[ mvOccupied->DirAbsolute > 0 ? end::front : end::rear ].Connected != nullptr ) ) { // a z przodu skład
                    iDirectionOrder = -iDirection; // zmiana na ciągnięcie
                    OrderNext( Change_direction ); // najpierw zmień kierunek (zastąpi Disconnect)
                    OrderPush( Disconnect ); // a odczep już po zmianie kierunku
                }
                else if( mvOccupied->Couplers[ mvOccupied->DirAbsolute > 0 ? end::rear : end::front ].Connected != nullptr ) { // z tyłu coś
                    OrderNext( Disconnect ); // jak ciągnie, to tylko odczep (NewValue1) wagonów
                }
                else {
                    // both couplers empty
                    // NOTE: this condition implements legacy behavior (mis)used by some scenarios to keep vehicle idling without moving
                    SetVelocity( 0, 0, stopJoin );
                    iDrivigFlags |= moveStopHere; // nie podjeżdżać do semafora, jeśli droga nie jest wolna
                }
                WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać, chyba że już jedzie
            }
        }
        if (NewValue1 == -1.0) {
            iDrivigFlags &= ~moveStopHere; // ma jechać
            WaitingTime = 0.0; // nie ma co dalej czekać, można zatrąbić i jechać
        }
        if( NewValue1 < -1.5 ) { // jeśli -2/-3, czyli czekanie z ruszeniem na sygnał
            iDrivigFlags |= moveStopHere; // nie podjeżdżać do semafora, jeśli droga nie jest wolna
        }
        // (nie dotyczy Connect)
        if( NewValue1 < -2.5 ) { // jeśli -3 to potem jazda pociągowa
            OrderNext( ( NewCommand == "Shunt" ? Obey_train : Bank ) );
        }
        else { // otherwise continue shunting
            OrderNext( ( NewCommand == "Shunt" ? Shunt : Loose_shunt ) );
        }
        CheckVehicles(); // sprawdzić światła
        iVehicleCount = std::floor( NewValue1 );

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
            OrdersDump("Jump_to_order");
#endif
        }
        return true;
    }

    if (NewCommand == "Warning_signal")
    {
        if( ( NewValue1 > 0 ) && ( NewValue2 > 0 ) ) {
            fWarningDuration = NewValue1; // czas trąbienia
            cue_action( driver_hint::hornon, NewValue2 ); // horn combination flag
        }
        return true;
    }

    if (NewCommand == "Radio_channel") {
        // wybór kanału radiowego (którego powinien używać AI, ręczny maszynista musi go ustawić sam)
        if (NewValue1 >= 0) {
            // wartości ujemne są zarezerwowane, -1 = nie zmieniać kanału
            if( AIControllFlag ) {
                iRadioChannel = NewValue1;
            }
            if( iGuardRadio ) {
                // kierownikowi też zmienić
                iGuardRadio = NewValue1;
            }
        }
        // NewValue2 może zawierać dodatkowo oczekiwany kod odpowiedzi, np. dla W29 "nawiązać
        // łączność radiową z dyżurnym ruchu odcinkowym"
        return true;
    }

    if( NewCommand == "SetLights" ) {
        // set consist lights pattern hints
        m_lighthints[ end::front ] = static_cast<int>( NewValue1 );
        m_lighthints[ end::rear ] = static_cast<int>( NewValue2 );
        if( true == TestFlag( OrderCurrentGet(), Obey_train ) ) {
            // light hints only apply in the obey_train mode
            CheckVehicles();
        }
        return true;
    }

	if (NewCommand == "SetSignal") {
		TSignals signal = (TSignals)std::lrint(NewValue1);

		for (int i = Signal_START; i <= Signal_MAX; i++)
			iDrivigFlags &= ~(1 << i);

		if (NewValue2 > 0.5)
			iDrivigFlags |= 1 << (uint32_t)signal;
		else
			CheckVehicles(); // restore light state
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
            << int(mvControlling->DirActive) << " "
            << ( mvOccupied->CommandIn.Command.empty() ? "none" : mvOccupied->CommandIn.Command.c_str() ) << " "
            << mvOccupied->CommandIn.Value1 << " "
            << mvOccupied->CommandIn.Value2 << " "
		    << int(mvControlling->SecuritySystem.is_blinking()) << " "
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
TController::Update( double const Timedelta ) {
    // uruchamiać przynajmniej raz na sekundę
    if( ( iDrivigFlags & movePrimary ) == 0 ) { return; } // pasywny nic nie robi
    if( false == simulation::is_ready )       { return; }

    update_timers( Timedelta );
    update_logs( Timedelta );

    auto const reactiontime { std::min( ReactionTime, 2.0 ) };

    if( LastReactionTime < reactiontime ) { return; }
    LastReactionTime -= reactiontime;
/*
    // TBD, TODO: put this in an appropriate place, or get rid of it
    // NOTE: this section moved all cars to the edge of their respective roads
    // it may have some use eventually for collision avoidance,
    // but when enabled all the time it produces silly effect
    if( mvOccupied->Vel > 1.0 ) {
        // przy prowadzeniu samochodu trzeba każdą oś odsuwać oddzielnie, inaczej kicha wychodzi
        if (mvOccupied->CategoryFlag & 2) // jeśli samochód
            // if (fabs(mvOccupied->OffsetTrackH)<mvOccupied->Dim.W) //Ra: szerokość drogi tu powinna być?
            if (!mvOccupied->ChangeOffsetH(-0.01 * mvOccupied->Vel * dt)) // ruch w poprzek drogi
                mvOccupied->ChangeOffsetH(0.01 * mvOccupied->Vel * dt); // Ra: co to miało być, to nie wiem
    }
*/
    update_hints();
    // main ai update routine
    determine_consist_state();
    determine_braking_distance();
    determine_proximity_ranges();
    // vicinity check
    auto const awarenessrange {
        std::max(
            750.0,
            mvOccupied->Vel > EU07_AI_MOVEMENT ?
                400 + fBrakeDist :
                30.0 * fDriverDist ) }; // 1500m dla stojących pociągów;
    if( is_active() ) {
        scan_route( awarenessrange );
    }
    scan_obstacles( awarenessrange );
    // generic actions
    control_security_system( reactiontime );
    if( iEngineActive ) {
        control_wheelslip();
        control_horns( reactiontime );
        control_pantographs();
        if( fActionTime > 0.0 ) {
            // potentially delayed and/or low priority actions
            control_lights();
            control_doors();
            control_compartment_lights();
        }
    }
    CheckTimeControllers();
    // external command actions
    UpdateCommand();
    // mode specific actions
    handle_engine();
    handle_orders();
    // situational velocity and acceleration adjustments
    pick_optimal_speed( awarenessrange );
    if( iEngineActive ) {
        control_tractive_and_braking_force();
    }
    SetTimeControllers();
}

// configures vehicle heating given current situation; returns: true if vehicle can be operated normally, false otherwise
bool
TController::PrepareHeating() {

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
                    cue_action( driver_hint::waterpumpbreakeron );
                    cue_action( driver_hint::waterpumpon );
                }
                if( true == mvControlling->WaterPump.is_active ) {
                    cue_action( driver_hint::waterheaterbreakeron );
                    cue_action( driver_hint::waterheateron );
                    cue_action( driver_hint::watercircuitslinkon );
                }
            }
            else {
                // no need to heat anything up, switch the heater off
                cue_action( driver_hint::watercircuitslinkoff );
                cue_action( driver_hint::waterheateroff );
                cue_action( driver_hint::waterheaterbreakeroff );
                // optionally turn off the water pump as well
                if( mvControlling->WaterPump.start_type != start_t::battery ) {
                    cue_action( driver_hint::waterpumpoff );
                    cue_action( driver_hint::waterpumpbreakeroff );
                }
            }

            IsHeatingTemperatureTooLow = lowtemperature;
            break;
        }
        default: {
            IsHeatingTemperatureTooLow = false;
            break;
        }
    }
    // TBD, TODO: potentially pay attention to too high temperature as well?
    IsHeatingTemperatureOK = ( false == IsHeatingTemperatureTooLow );

    return IsHeatingTemperatureOK;
}

void
TController::PrepareDirection() {

    if( iDirection == 0 ) {
        // jeśli nie ma ustalonego kierunku
        if( mvOccupied->Vel < EU07_AI_NOMOVEMENT ) { // ustalenie kierunku, gdy stoi
            iDirection = mvOccupied->CabActive; // wg wybranej kabiny
/*
            if( iDirection == 0 ) {
                // jeśli nie ma ustalonego kierunku
                if( mvOccupied->Couplers[ end::rear ].Connected == nullptr ) {
                    // jeśli z tyłu nie ma nic
                    iDirection = -1; // jazda w kierunku sprzęgu 1
                }
                if( mvOccupied->Couplers[ end::front ].Connected == nullptr ) {
                    // jeśli z przodu nie ma nic
                    iDirection = 1; // jazda w kierunku sprzęgu 0
                }
            }
*/
        }
        else {
            // ustalenie kierunku, gdy jedzie
            iDirection = (
                mvOccupied->V >= 0 ?
                        1 : // jazda w kierunku sprzęgu 0
                       -1 ); // jazda w kierunku sprzęgu 1
        }
    }
    cue_action( driver_hint::mastercontrollersetreverserunlock );
    cue_action(
        iDirection > 0 ?
            driver_hint::directionforward :
            driver_hint::directionbackward );
}

void TController::JumpToNextOrder( bool const Ignoremergedchangedirection )
{ // wykonanie kolejnej komendy z tablicy rozkazów
    if (OrderList[OrderPos] != Wait_for_orders)
    {
        if( ( ( OrderList[ OrderPos ] & Change_direction ) != 0 ) // jeśli zmiana kierunku
        &&  ( OrderList[ OrderPos ] != Change_direction ) ) { // ale nałożona na coś

            if( false == Ignoremergedchangedirection ) {
                // usunięcie zmiany kierunku z innej komendy
                OrderList[ OrderPos ] = TOrders( OrderList[ OrderPos ] & ~Change_direction );
                OrderCheck();
                return;
            }
        }
        if (OrderPos < maxorders - 1)
            ++OrderPos;
        else
            OrderPos = 0;
    }
    OrderCheck();
#if LOGORDERS
    OrdersDump("JumpToNextOrder"); // normalnie nie ma po co tego wypisywać
#endif
};

void TController::JumpToFirstOrder()
{ // taki relikt
    OrderPos = 1;
    if (OrderTop == 0)
        OrderTop = 1;
    OrderCheck();
#if LOGORDERS
    OrdersDump("JumpToFirstOrder"); // normalnie nie ma po co tego wypisywać
#endif
};

void TController::OrderCheck()
{ // reakcja na zmianę rozkazu

    if( OrderCurrentGet() != Obey_train ) {
        // reset light hints
        m_lighthints[ end::front ] = m_lighthints[ end::rear ] = -1;
    }
    if( OrderCurrentGet() & ( Shunt | Loose_shunt | Connect | Obey_train | Bank ) ) {
        CheckVehicles(); // sprawdzić światła
    }
    if( OrderCurrentGet() & ( Shunt | Loose_shunt | Connect ) ) {
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
        while ((OrderList[OrderTop] != Wait_for_orders) && (OrderList[OrderTop] < Shunt)) // jeśli coś robi
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
    OrdersDump( "OrderNext" ); // normalnie nie ma po co tego wypisywać
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
    OrdersDump( "OrderPush: [" + Order2Str( NewOrder ) + "]" ); // normalnie nie ma po co tego wypisywać
#endif
}

void TController::OrdersDump( std::string const Neworder, bool const Verbose )
{ // wypisanie kolejnych rozkazów do logu
    WriteLog( "New order for [" + pVehicle->asName + "]: " + Neworder );
    if( false == Verbose ) { return; }
    for (int b = 0; b < maxorders; ++b)
    {
        WriteLog( ( OrderPos == b ? ">" : " " ) + (std::to_string(b) + ": " + Order2Str(OrderList[b])));
        if (b) // z wyjątkiem pierwszej pozycji
            if (OrderList[b] == Wait_for_orders) // jeśli końcowa komenda
                break; // dalej nie trzeba
    }
};

void TController::OrdersInit(double fVel)
{ // wypełnianie tabelki rozkazów na podstawie rozkładu
    // ustawienie kolejności komend, niezależnie kto prowadzi
    OrdersClear(); // usunięcie poprzedniej tabeli
    OrderPush(Prepare_engine); // najpierw odpalenie silnika
    if (TrainParams.StationCount == 0)
    { // brak rozkładu to jazda manewrowa
//        if (fVel > 0.05) // typowo 0.1 oznacza gotowość do jazdy, 0.01 tylko załączenie silnika
            OrderPush(Shunt); // jeśli nie ma rozkładu, to manewruje
    }
    else
    { // jeśli z rozkładem, to jedzie na szlak
        if ((fVel > 0.0) && (fVel < 0.02))
            OrderPush(Shunt); // dla prędkości 0.01 włączamy jazdę manewrową
        else if (TrainParams.DirectionChange() ? //  jeśli obrót na pierwszym przystanku
                    ((iDrivigFlags & movePushPull) ? // SZT również! SN61 zależnie od wagonów...
                        (TrainParams.TimeTable[1].StationName == TrainParams.Relation1) :
                        false) :
                    false)
            OrderPush(Shunt); // a teraz start będzie w manewrowym, a tryb pociągowy włączy W4
        else
            // jeśli start z pierwszej stacji i jednocześnie jest na niej zmiana kierunku, to EZT ma mieć Shunt
            OrderPush(Obey_train); // dla starych scenerii start w trybie pociagowym
        if (DebugModeFlag) // normalnie nie ma po co tego wypisywać
            WriteLog("/* Timetable: " + TrainParams.ShowRelation());
        TMTableLine *t;
        for (int i = 0; i <= TrainParams.StationCount; ++i)
        {
            t = TrainParams.TimeTable + i;
            if (DebugModeFlag) {
                // normalnie nie ma po co tego wypisywa?
                WriteLog(
                    t->StationName
                    + " " + std::to_string(t->Ah) + ":" + std::to_string(t->Am)
                    + ", " + std::to_string(t->Dh) + ":" + std::to_string(t->Dm)
                    + " " + t->StationWare);
            }
            if ( contains( t->StationWare, '@' ) )
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
                if (i < TrainParams.StationCount) // jak nie ostatnia stacja
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
    OrdersDump( "OrdersInit", DebugModeFlag ); // wypisanie kontrolne tabelki rozkazów
#endif
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

bool TController::IsOccupiedByAnotherConsist( TTrack *Track, double const Distance = 0 )
{ // najpierw sprawdzamy, czy na danym torze są pojazdy z innego składu
    if( false == Track->Dynamics.empty() ) {
        for( auto dynamic : Track->Dynamics ) {
            if( dynamic->ctOwner != this ) {
                // jeśli jest jakiś cudzy to tor jest zajęty i skanowanie nie obowiązuje
                if( Distance == 0 ) {
                    return true;
                }
                else {
                    // based on provided position of scanning vehicle and scan direction, filter out vehicles on irrelevant end
                    auto const scandirection { Distance > 0 ? 1 : -1 }; // positive value means scan towards point2 end of the track
                    auto const obstaclelocation { scandirection > 0 ? Track->Length() - dynamic->RaTranslationGet() : dynamic->RaTranslationGet() };
                    // if detected vehicle is closer to the end of the track in scanned direction than we are, it means it's blocking our way
                    if( obstaclelocation < std::abs( Distance ) ) {
                        return true;
                    }
                }
            }
        }
    }
    return false; // wolny
};

basic_event * TController::CheckTrackEventBackward(double fDirection, TTrack *Track, TDynamicObject *Vehicle, int const Eventdirection, end const End)
{ // sprawdzanie eventu w torze, czy jest sygnałowym - skanowanie do tyłu
    // NOTE: this method returns only one event which meets the conditions, due to limitations in the caller
    // TBD, TODO: clean up the caller and return all suitable events, as in theory things will go awry if the track has more than one signal
    auto const dir{ Vehicle->VectorFront() * Vehicle->DirectionGet() };
    auto const pos{ End == end::front ? Vehicle->RearPosition() : Vehicle->HeadPosition() };
    auto const &eventsequence { ( fDirection * Eventdirection > 0 ? Track->m_events2 : Track->m_events1 ) };
    for( auto const &event : eventsequence ) {
        if( ( event.second != nullptr )
         && ( event.second->m_passive )
         && ( typeid(*(event.second)) == typeid( getvalues_event ) ) ) {
            // since we're checking for events behind us discard the sources in front of the scanning vehicle
            auto const sl{ event.second->input_location() }; // położenie komórki pamięci
            auto const sem{ sl - pos }; // wektor do komórki pamięci od końca składu
            auto const isahead { dir.x * sem.x + dir.z * sem.z > 0 };
            if( ( End == end::front ? isahead : !isahead ) ) {
                // iloczyn skalarny jest ujemny, gdy sygnał stoi z tyłu
                return event.second;
            }
        }
    }
    return nullptr;
};

TTrack * TController::BackwardTraceRoute(double &fDistance, double &fDirection, TDynamicObject *Vehicle, basic_event *&Event, int const Eventdirection, end const End, bool const Untiloccupied )
{ // szukanie sygnalizatora w kierunku przeciwnym jazdy (eventu odczytu komórki pamięci)
    auto *Track = Vehicle->GetTrack();
    double fCurrentDistance = Vehicle->RaTranslationGet(); // aktualna pozycja na torze
    if (fDirection > 0) // jeśli w kierunku Point2 toru
        fCurrentDistance = Track->Length() - fCurrentDistance;
    double s = 0;

    if( Track->VelocityGet() == 0.0 ) { // jak prędkosć 0 albo uszkadza, to nie ma po co skanować
        fDistance = s; // to na tym torze stoimy
        return nullptr; // stop, skanowanie nie dało sensownych rezultatów
    }
    // jak tor zajęty innym składem, to nie ma po co skanować
    if( Untiloccupied && IsOccupiedByAnotherConsist( Track, fCurrentDistance * fDirection ) ) {
        fDistance = s; // to na tym torze stoimy
        return nullptr; // stop, skanowanie nie dało sensownych rezultatów
    }
    if ((Event = CheckTrackEventBackward(fDirection, Track, Vehicle, Eventdirection, End)) != nullptr)
    { // jeśli jest semafor na tym torze
        fDistance = s; // to na tym torze stoimy
        return Track;
    }
    TTrack *pTrackFrom; // odcinek poprzedni, do znajdywania końca dróg
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
            Track = nullptr; // koniec, tak jak dla torów

        // gdy dalej toru nie ma albo zerowa prędkość, albo uszkadza pojazd
        // zwraca NULL, że skanowanie nie dało sensownych rezultatów
        if( Track == nullptr ) {
            fDistance = s;
            return nullptr;
        }
        if( Track->VelocityGet() == 0.0 )
        {
            fDistance = s;
            return nullptr;
        }
        if( Untiloccupied && IsOccupiedByAnotherConsist( Track ) ) {
            fDistance = s;
            return nullptr;
        }
        fCurrentDistance = Track->Length();
        if ((Event = CheckTrackEventBackward(fDirection, Track, Vehicle, Eventdirection, End)) != nullptr)
        { // znaleziony tor z eventem
            fDistance = s;
            return Track;
        }
    }
    Event = nullptr; // jak dojdzie tu, to nie ma semafora
    // zwraca ostatni sprawdzony tor
    fDistance = s;
    return Track;
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

TCommandType TController::BackwardScan( double const Range )
{ // sprawdzanie zdarzeń semaforów z tyłu pojazdu, zwraca komendę
    // dzięki temu będzie można stawać za wskazanym sygnalizatorem, a zwłaszcza jeśli będzie jazda na kozioł
    // ograniczenia prędkości nie są wtedy istotne, również koniec toru jest do niczego nie przydatny
    // zwraca true, jeśli należy odwrócić kierunek jazdy pojazdu
    if( ( OrderCurrentGet() & ~( Shunt | Loose_shunt | Connect ) ) ) {
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

    if( scandir == 0.0 ) { return TCommandType::cm_Unknown; }

    // skanowanie toru w poszukiwaniu eventów GetValues (PutValues nie są przydatne)
    // Ra: przy wstecznym skanowaniu prędkość nie ma znaczenia
    double scandist = Range; // zmodyfikuje na rzeczywiście przeskanowane
    basic_event *e = nullptr; // event potencjalnie od semafora
    // opcjonalnie może być skanowanie od "wskaźnika" z przodu, np. W5, Tm=Ms1, koniec toru wg
    // drugiej osi w kierunku ruchu
    auto const *scantrack{BackwardTraceRoute(scandist, scandir, pVehicles[end::front], e)};
    auto const dir{startdir *
                   pVehicles[end::front]->VectorFront()}; // wektor w kierunku jazdy/szukania

    // jeśli wstecz wykryto koniec toru to raczej nic się nie da w takiej sytuacji zrobić
    if (e == nullptr)
    {
        return TCommandType::cm_Unknown;
    }
    if (typeid(*e) != typeid(getvalues_event))
    {
        return TCommandType::cm_Unknown;
    }

    // jeśli jest jakiś sygnał na widoku
    double scanvel; // prędkość ustawiona semaforem
#if LOGBACKSCAN
    std::string edir{"Backward scan by " + pVehicle->asName + " - " +
                     ((scandir > 0) ? "Event2 " : "Event1 ")};
    edir += "(" + (e->name()) + ")";
#endif
    {
        // najpierw sprawdzamy, czy semafor czy inny znak został przejechany
        auto const sl{e->input_location()}; // położenie komórki pamięci
        auto const pos{pVehicles[end::rear]->RearPosition()}; // pozycja tyłu
        auto const sem{sl - pos}; // wektor do komórki pamięci od końca składu
        if (dir.x * sem.x + dir.z * sem.z < 0)
        {
            // jeśli został minięty
            // iloczyn skalarny jest ujemny, gdy sygnał stoi z tyłu
#if LOGBACKSCAN
            WriteLog(edir + " - ignored as not passed yet");
#endif
            return TCommandType::cm_Unknown; // nic
        }
        scanvel = e->input_value(1); // prędkość przy tym semaforze
        // przeliczamy odległość od semafora - potrzebne by były współrzędne początku składu
        scandist = sem.Length() - 2; // 2m luzu przy manewrach wystarczy
        if (scandist < 0)
        {
            // ujemnych nie ma po co wysyłać
            scandist = 0;
        }
    }

    auto move{false}; // czy AI w trybie manewerowym ma dociągnąć pod S1
    if (e->input_command() == TCommandType::cm_SetVelocity)
    {
        if ((scanvel == 0.0) ? (OrderCurrentGet() & (Shunt | Loose_shunt | Connect)) :
                               (OrderCurrentGet() & Connect))
        { // przy podczepianiu ignorować wyjazd?
            move = true; // AI w trybie manewerowym ma dociągnąć pod S1
        }
        else
        {
            if ((scandist > fMinProximityDist) &&
                ((mvOccupied->Vel > EU07_AI_NOMOVEMENT) &&
                 ((OrderCurrentGet() & (Shunt | Loose_shunt)) == 0)))
            {
                // jeśli semafor jest daleko, a pojazd jedzie, to informujemy o zmianie prędkości
                // jeśli jedzie manewrowo, musi dostać SetVelocity, żeby sie na pociągowy przełączył
#if LOGBACKSCAN
                // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist) +
                // AnsiString(scanvel));
                WriteLog(edir);
#endif
                // SetProximityVelocity(scandist,scanvel,&sl);
                return (scanvel > 0 ? TCommandType::cm_SetVelocity : TCommandType::cm_Unknown);
            }
            else
            {
                // ustawiamy prędkość tylko wtedy, gdy ma ruszyć, stanąć albo ma stać
                // if ((MoverParameters->Vel==0.0)||(scanvel==0.0)) //jeśli stoi lub ma stanąć/stać
                // semafor na tym torze albo lokomtywa stoi, a ma ruszyć, albo ma stanąć, albo nie
                // ruszać stop trzeba powtarzać, bo inaczej zatrąbi i pojedzie sam
                // PutCommand("SetVelocity",scanvel,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                WriteLog(edir + " - [SetVelocity] [" + to_string(scanvel, 2) + "] [" +
                         to_string(e->input_value(2), 2) + "]");
#endif
                return (scanvel > 0 ? TCommandType::cm_SetVelocity : TCommandType::cm_Unknown);
            }
        }
    }
    // reakcja AI w trybie manewrowym dodatkowo na sygnały manewrowe
    if (OrderCurrentGet() ? OrderCurrentGet() & (Shunt | Loose_shunt | Connect) :
                            true) // w Wait_for_orders też widzi tarcze
    {
        if (move || e->input_command() == TCommandType::cm_ShuntVelocity)
        { // jeśli powyżej było SetVelocity 0 0, to dociągamy pod S1
/*
            if ((scandist > fMinProximityDist) &&
                ((mvOccupied->Vel > EU07_AI_NOMOVEMENT) || (scanvel == 0.0)))
            {
                // jeśli tarcza jest daleko, to:
                //- jesli pojazd jedzie, to informujemy o zmianie prędkości
                //- jeśli stoi, to z własnej inicjatywy może podjechać pod zamkniętą tarczę
                if (mvOccupied->Vel > EU07_AI_NOMOVEMENT) // tylko jeśli jedzie
                { // Mechanik->PutCommand("SetProximityVelocity",scandist,scanvel,sl);
#if LOGBACKSCAN
                  // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+"
                    // "+AnsiString(scanvel));
                    WriteLog(edir);
#endif
                    // SetProximityVelocity(scandist,scanvel,&sl);
                    // jeśli jedzie na W5 albo koniec toru, to można zmienić kierunek
                    return (iDrivigFlags & moveTrackEnd) ? TCommandType::cm_ChangeDirection :
                                                           TCommandType::cm_Unknown;
                    // TBD: request direction change also if VelNext in current direction is 0,
                    // while signal behind requests movement?
                }
            }
            else
            {
                // ustawiamy prędkość tylko wtedy, gdy ma ruszyć, albo stanąć albo ma stać pod
                // tarczą stop trzeba powtarzać, bo inaczej zatrąbi i pojedzie sam if
                // ((MoverParameters->Vel==0.0)||(scanvel==0.0)) //jeśli jedzie lub ma stanąć/stać
                { // nie dostanie komendy jeśli jedzie i ma jechać
                  // PutCommand("ShuntVelocity",scanvel,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                    WriteLog(edir + " - [ShuntVelocity] [" + to_string(scanvel, 2) + "] [" +
                             to_string(e->input_value(2), 2) + "]");
#endif
                    return (scanvel > 0 ? TCommandType::cm_ShuntVelocity :
                                          TCommandType::cm_Unknown);
                }
            }
            // NOTE: dead code due to returns in branching above
            if ((scanvel != 0.0) && (scandist < 100.0))
            {
                // jeśli Tm w odległości do 100m podaje zezwolenie na jazdę, to od razu ją
                // ignorujemy, aby móc szukać kolejnej eSignSkip=e; //wtedy uznajemy ignorowaną przy
                // poszukiwaniu nowej
#if LOGBACKSCAN
                WriteLog(edir + " - will be ignored due to Ms2");
#endif
                return (scanvel > 0 ? TCommandType::cm_ShuntVelocity : TCommandType::cm_Unknown);
            }
*/
            return ((VelNext > 0) ?
                        TCommandType::cm_Unknown : // no need to bother if we can continue in current direction
                        (scanvel == 0) ?
                            TCommandType::cm_Unknown : // no need to bother with a stop signal
                            TCommandType::cm_ShuntVelocity); // otherwise report a relevant shunt signal behind
        } // if (move?...
    } // if (OrderCurrentGet()==Shunt)

    return (((e->m_passive) && (e->is_command())) ? TCommandType::cm_Command :
                                                    TCommandType::cm_Unknown);
};

std::string TController::NextStop() const
{ // informacja o następnym zatrzymaniu, wyświetlane pod [F1]
    if (asNextStop == "[End of route]")
        return ""; // nie zawiera nazwy stacji, gdy dojechał do końca
    // dodać godzinę odjazdu
    auto nextstop{ Bezogonkow( asNextStop, true ) };
    auto const *t{ TrainParams.TimeTable + TrainParams.StationIndex };
    if( t->Ah >= 0 ) {
        // przyjazd
        nextstop += "  przyj." + std::to_string( t->Ah ) + ":"
      + to_minutes_str( t->Am, true, 3 );

    }
    if( t->Dh >= 0 ) {
        // jeśli jest godzina odjazdu
        nextstop += " odj." + std::to_string( t->Dh ) + ":"
      + to_minutes_str( t->Dm, true, 3 );
    }
    return nextstop;
};

void TController::UpdateDelayFlag() {

    if( TrainParams.CheckTrainLatency() < 0.0 ) {
        // odnotowano spóźnienie
        iDrivigFlags |= moveLate;
    }
    else {
        // przyjazd o czasie
        iDrivigFlags &= ~moveLate;
    }
}

//-----------koniec skanowania semaforow

void TController::TakeControl( bool const Aidriver, bool const Forcevehiclecheck )
{ // przejęcie kontroli przez AI albo oddanie
    if ((AIControllFlag == Aidriver) && (!Forcevehiclecheck))
        return; // już jest jak ma być
    if (Aidriver) //żeby nie wykonywać dwa razy
    { // teraz AI prowadzi
        AIControllFlag = AIdriver;
        pVehicle->Controller = AIdriver;
		mvOccupied->CabActivisation(true);
        iDirection = 0; // kierunek jazdy trzeba dopiero zgadnąć
        TableClear(); // ponowne utworzenie tabelki, bo człowiek mógł pojechać niezgodnie z sygnałami
        if( action() != TAction::actSleep ) {
            // gdy zgaszone światła, flaga podjeżdżania pod semafory pozostaje bez zmiany
            // conditional below disabled to get around the situation where the AI train does nothing ever
            // because it is waiting for orders which don't come until the engine is engaged, i.e. effectively never
            if( OrderCurrentGet() ) {
                // jeśli coś robi
                PrepareEngine(); // niech sprawdzi stan silnika
            }
            else {
                // jeśli nic nie robi
                OrderNext( Prepare_engine );
                if( pVehicle->MoverParameters->iLights[ ( mvOccupied->CabActive < 0 ?
                        end::rear :
                        end::front ) ]
                    & ( light::headlight_left | light::headlight_right | light::headlight_upper ) ) // któreś ze świateł zapalone?
                { // od wersji 357 oczekujemy podania komend dla AI przez scenerię
/*
                    if( pVehicle->MoverParameters->iLights[ mvOccupied->CabActive < 0 ? end::rear : end::front ] & light::headlight_upper ) // górne światło zapalone
                        OrderNext( Obey_train ); // jazda pociągowa
                    else
                        OrderNext( Shunt ); // jazda manewrowa
*/
                    if( mvOccupied->Vel >= 1.0 ) // jeśli jedzie (dla 0.1 ma stać)
                        iDrivigFlags &= ~moveStopHere; // to ma nie czekać na sygnał, tylko jechać
                    else
                        iDrivigFlags |= moveStopHere; // a jak stoi, to niech czeka
                }
            }
            CheckVehicles(); // ustawienie świateł
        }
    }
    else
    { // a teraz użytkownik
        AIControllFlag = Humandriver;
        pVehicle->Controller = Humandriver;
        if( eAction == TAction::actSleep ) {
            eAction = TAction::actUnknown;
        }
        if( Forcevehiclecheck ) {
            // update consist ownership and other consist data
            CheckVehicles();
        }
    }
};

void TController::DirectionForward(bool forward)
{ // ustawienie jazdy do przodu dla true i do tyłu dla false (zależy od kabiny)
    ZeroSpeed( true ); // TODO: check if force switch is needed anymore here
    // HACK: make sure the master controller isn't set in position which prevents direction change
    mvControlling->MainCtrlPos = std::min( mvControlling->MainCtrlPos, mvControlling->MainCtrlMaxDirChangePos );

    if( forward ) {
        // do przodu w obecnej kabinie
        while( ( mvOccupied->DirActive <= 0 )
            && ( mvOccupied->DirectionForward() ) ) {
            // force scan table update
            iTableDirection = 0;
        }
    }
    else {
        // do tyłu w obecnej kabinie
        while( ( mvOccupied->DirActive >= 0 )
            && ( mvOccupied->DirectionBackward() ) ) {
            // force scan table update
            iTableDirection = 0;
        }
    }
    if( mvOccupied->TrainType == dt_SN61 ) {
        // specjalnie dla SN61 żeby nie zgasł
        while( ( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn == 0 )
            && ( mvControlling->IncMainCtrl( 1 ) ) ) {
            ; // all work is done in the header
        }
    }
}

void TController::ZeroDirection() {

    while( ( mvOccupied->DirActive > 0 ) && ( mvOccupied->DirectionBackward() ) ) { ; }
    while( ( mvOccupied->DirActive < 0 ) && ( mvOccupied->DirectionForward() ) ) { ; }
}

void TController::sync_consist_reversers() {

    auto const currentdirection { mvOccupied->DirActive };
    auto const fastforward { (
        ( is_emu() )
     && ( mvOccupied->EngineType != TEngineType::ElectricInductionMotor ) )
     && ( mvOccupied->Imin == mvOccupied->IminHi ) };

    // move reverser all way in the opposite direction...
    for( auto idx = 0; idx < 3; ++idx ) {
        if( currentdirection >= 0 ) {
            mvOccupied->DirectionBackward();
        }
        else {
            mvOccupied->DirectionForward();

        }
    }
    // ...then restore original setting
    while( mvOccupied->DirActive != currentdirection ) {
        if( false == (
            currentdirection >= 0 ?
                mvOccupied->DirectionForward() :
                mvOccupied->DirectionBackward() ) ) {
            // sanity check for potential endless loop
            break;
        }
    }
    // potentially restore 'fast forward' setting
    if( ( currentdirection > 0 ) && ( true == fastforward ) ) {
        mvOccupied->DirectionForward();
    }
}

Mtable::TTrainParameters const &
TController::TrainTimetable() const {
    return TrainParams;
}

std::string TController::Relation() const
{ // zwraca relację pociągu
    return TrainParams.ShowRelation();
};

const std::string& TController::TrainName() const
{ // zwraca numer pociągu
    return TrainParams.TrainName;
};

int TController::StationCount() const
{ // zwraca ilość stacji (miejsc zatrzymania)
    return TrainParams.StationCount;
};

int TController::StationIndex() const
{ // zwraca indeks aktualnej stacji (miejsca zatrzymania)
    return TrainParams.StationIndex;
};

bool TController::IsStop() const
{ // informuje, czy jest zatrzymanie na najbliższej stacji
    return TrainParams.IsStop();
};

// returns most recently calculated distance to potential obstacle ahead
double
TController::TrackObstacle() const {

    return Obstacle.distance;
}

void TController::MoveTo(TDynamicObject *to)
{ // przesunięcie AI do innego pojazdu (przy zmianie kabiny)
    if( ( to->Mechanik != nullptr )
     && ( to->Mechanik != this ) ) {
        // ai controller thunderdome, there can be only one
        if( to->Mechanik->AIControllFlag ) {
            if( to->Mechanik->primary() ) {
                // take over boss duties
                primary( true );
            }
            SafeDelete( to->Mechanik );
        }
        else {
            // can't quite delete a human
            if( AIControllFlag ) {
                delete this;
            }
            return;
        }
    }
    pVehicle->Mechanik = nullptr;
    pVehicle = to;
    ControllingSet(); // utworzenie połączenia do sterowanego pojazdu
    pVehicle->Mechanik = this;

};

void TController::ControllingSet()
{ // znajduje człon do sterowania w EZT będzie to silnikowy
    // problematyczne jest sterowanie z członu biernego, dlatego damy AI silnikowy
    // dzięki temu będzie wirtualna kabina w silnikowym, działająca w rozrządczym
    // w plikach FIZ zostały zgubione ujemne maski sprzęgów, stąd problemy z EZT
    mvOccupied = pVehicle->MoverParameters; // domyślny skrót do obiektu parametrów
	BrakeSystem = mvOccupied->BrakeSystem; // domyślny sposób hamowania
    mvControlling = pVehicle->FindPowered()->MoverParameters; // poszukiwanie członu sterowanego
    {
        auto *lookup { pVehicle->FindPantographCarrier() };
        mvPantographUnit = (
            lookup != nullptr ?
                lookup->MoverParameters :
                mvControlling );
    }
    BrakeSystem = consist_brake_system();
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

void
TController::update_timers( double dt ) {

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
    if( ( mvOccupied->Vel < 0.05 )
     && ( ( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank ) ) != 0 ) ) {
        IdleTime += dt;
    }
    else {
        IdleTime = 0.0;
    }
    fWarningDuration -= dt; // horn activation while the value is positive
}

void
TController::update_logs( double const dt ) {
    // log vehicle data
    LastUpdatedTime += dt;
    if( ( WriteLogFlag )
     && ( LastUpdatedTime > deltalog ) ) {
        // zapis do pliku DAT
        PhysicsLog();
        LastUpdatedTime -= deltalog;
    }
}

void
TController::determine_consist_state() {
    // ABu-160305 testowanie gotowości do jazdy
    // Ra: przeniesione z DynObj, skład użytkownika też jest testowany, żeby mu przekazać, że ma odhamować

	if (mvOccupied->CabActive == 0 && mvOccupied->Power24vIsAvailable)
	{
		cue_action( driver_hint::cabactivation );
	}
	else if (!mvOccupied->AutomaticCabActivation
			 && ( (mvOccupied->CabActive == -mvOccupied->CabOccupied) || (!mvOccupied->CabMaster) || (!mvOccupied->Power24vIsAvailable) ) )
	{
		cue_action( driver_hint::cabdeactivation );
	}

	int index = double(BrakeAccTableSize) * (mvOccupied->Vel / mvOccupied->Vmax);
	index = std::min(BrakeAccTableSize, std::max(1, index));
	fBrake_a0[0] = fBrake_a0[index];
	fBrake_a1[0] = fBrake_a1[index];

	if ((is_emu()) || (is_dmu())) {
		auto Coeff = clamp( mvOccupied->Vel*0.015 , 0.5 , 1.0);
		fAccThreshold = fNominalAccThreshold * Coeff - fBrake_a0[BrakeAccTableSize] * (1.0 - Coeff);
	}

    Ready = true; // wstępnie gotowy
    fReady = 0.0; // założenie, że odhamowany
    IsConsistBraked = (
        ( ( mvOccupied->BrakeSystem == TBrakeSystem::ElectroPneumatic ) && ( false == ForcePNBrake ) ) ?
            mvOccupied->BrakePress > 2.0 :
            mvOccupied->PipePress < std::max( 3.9, mvOccupied->BrakePressureActual.PipePressureVal ) + 0.1 );
    fAccGravity = 0.0; // przyspieszenie wynikające z pochylenia
    IsAnyCouplerStretched = false;
    IsAnyDoorOnlyOpen[ side::right ] = IsAnyDoorOnlyOpen[ side::left ] = false;
	IsAnyDoorOpen[ side::right ] = IsAnyDoorOpen[ side::left ] = false;
    IsAnyDoorPermitActive[ side::right ] = IsAnyDoorPermitActive[ side::left ] = false;
    ConsistShade = 0.0;
    auto *p { pVehicles[ end::front ] }; // pojazd na czole składu
    double dy; // składowa styczna grawitacji, w przedziale <0,1>
    while (p)
    { // sprawdzenie odhamowania wszystkich połączonych pojazdów
        auto *vehicle { p->MoverParameters };
        auto const bp { std::max( 0.0, vehicle->BrakePress - ( vehicle->SpeedCtrlUnit.Parking ? vehicle->MaxBrakePress[ 0 ] * vehicle->StopBrakeDecc : 0.0 ) ) };
        if (Ready) {
            // bo jak coś nie odhamowane, to dalej nie ma co sprawdzać
            if( ( TestFlagAny( vehicle->Hamulec->GetBrakeStatus(), ( b_hld | b_on ) ) )
             || ( ( vehicle->Vel < 1.0 ?
                    ( bp > 0.4 ) :  // ensure the brakes are sufficiently released when starting to move
                    ( vehicle->Fb * 0.001 > 10.0 ) ) ) ) { // once in motion we can make a more lenient check
                Ready = false;
            }
            // Ra: odluźnianie przeładowanych lokomotyw, ciągniętych na zimno - prowizorka...
            if( bp >= 0.4 ) { // wg UIC określone sztywno na 0.04
                if( AIControllFlag || (Global.AITrainman && mvOccupied->Vel < EU07_AI_NOMOVEMENT  && !is_emu() && !is_dmu())) {
                    if( ( BrakeCtrlPosition == gbh_RP ) // jest pozycja jazdy
                     && ( false == TestFlag( vehicle->Hamulec->GetBrakeStatus(), b_dmg ) ) // brake isn't broken
                     && ( vehicle->PipePress - mvOccupied->Handle->GetRP() > -0.1 ) // jeśli ciśnienie jak dla jazdy
                     && ( vehicle->Hamulec->GetCRP() > vehicle->PipePress + 0.12 ) ) { // za dużo w zbiorniku
                        // indywidualne luzowanko
                        vehicle->BrakeReleaser( 1 );
                    }
                }
            }
			if (bp < 0.1) {
				if ( AIControllFlag || Global.AITrainman ) {
					if (( false == TestFlag( vehicle->Hamulec->GetBrakeStatus(), b_dmg ) ) // brake isn't broken
						&& ( vehicle->Hamulec->GetCRP() < vehicle->PipePress - 0.1 ) ) { // już nie jest za dużo w zbiorniku
						   // koniec indywidualnego luzowanka
						vehicle->BrakeReleaser( 0 );
					}
				}
			}
        }
        fReady = std::max( bp, fReady ); // szukanie najbardziej zahamowanego
        if( ( dy = p->VectorFront().y ) != 0.0 ) {
            // istotne tylko dla pojazdów na pochyleniu
            // ciężar razy składowa styczna grawitacji
            fAccGravity -= vehicle->TotalMassxg * dy * ( p->DirectionGet() == iDirection ? 1 : -1 );
        }
        // check coupler state
        IsAnyCouplerStretched =
            IsAnyCouplerStretched
            || ( vehicle->Couplers[ end::front ].stretch_duration > 0.0 )
            || ( vehicle->Couplers[ end::rear  ].stretch_duration > 0.0 );
        // check door state
        {
			auto const switchsides{ p->DirectionGet() != (iDirection == 0 ? mvOccupied->CabOccupied : iDirection) };
            auto const &rightdoor { vehicle->Doors.instances[ ( switchsides ? side::left : side::right ) ] };
            auto const &leftdoor { vehicle->Doors.instances[ ( switchsides ? side::right : side::left ) ] };
            if( vehicle->Doors.close_control != control_t::autonomous ) {
                IsAnyDoorOpen[ side::right ] |= ( false == rightdoor.is_closed );
                IsAnyDoorOpen[ side::left  ] |= ( false == leftdoor.is_closed );
            }
			if (vehicle->Doors.close_control != control_t::autonomous) {
				IsAnyDoorOnlyOpen[ side::right ] |= ( false == rightdoor.is_door_closed );
				IsAnyDoorOnlyOpen[ side::left  ] |= ( false == leftdoor.is_door_closed );
			}
            if( vehicle->Doors.permit_needed ) {
                IsAnyDoorPermitActive[ side::right ] |= rightdoor.open_permit;
                IsAnyDoorPermitActive[ side::left  ] |= leftdoor.open_permit;
            }
        }
        // measure lighting level
        // TBD: apply weight (multiplier) to partially lit vehicles?
        ConsistShade += ( p->fShade > 0.0 ? p->fShade : 1.0 );
        p = p->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
    }
    // calculate average amount of received sunlight
    ConsistShade /= iVehicles;

    // test state of main devices in all vehicles under control
    IsAnyConverterOverloadRelayOpen = false;
    IsAnyMotorOverloadRelayOpen = false;
    IsAnyGroundRelayOpen = false;
    IsAnyLineBreakerOpen = false;
    IsAnyCompressorPresent = false;
    IsAnyCompressorEnabled = false;
    IsAnyCompressorExplicitlyEnabled = false;
    IsAnyConverterPresent = false;
    IsAnyConverterEnabled = false;
    IsAnyConverterExplicitlyEnabled = false;

    pVehicle->for_each(
        coupling::control,
        [this]( TDynamicObject * Vehicle ) {
            auto const *vehicle { Vehicle->MoverParameters };
            IsAnyConverterOverloadRelayOpen |= vehicle->ConvOvldFlag;
            IsAnyMotorOverloadRelayOpen |= vehicle->FuseFlag;
            IsAnyGroundRelayOpen |= !( vehicle->GroundRelay );
            IsAnyCompressorPresent |= ( vehicle->CompressorSpeed > 0.0 );
            IsAnyCompressorEnabled |= ( vehicle->CompressorSpeed > 0.0 ? ( vehicle->CompressorAllow || vehicle->CompressorStart == start_t::automatic ) && ( vehicle->CompressorAllowLocal ) : false );
            IsAnyCompressorExplicitlyEnabled |= ( vehicle->CompressorSpeed > 0.0 ? ( vehicle->CompressorAllow && vehicle->CompressorAllowLocal ) : false );
            IsAnyConverterPresent |= ( vehicle->ConverterStart != start_t::disabled );
            IsAnyConverterEnabled |= ( vehicle->ConverterAllow || vehicle->ConverterStart == start_t::automatic ) && ( vehicle->ConverterAllowLocal );
            IsAnyConverterExplicitlyEnabled |= ( vehicle->ConverterAllow && vehicle->ConverterAllowLocal );
            if( vehicle->Power > 0.01 ) {
                IsAnyLineBreakerOpen |= !( vehicle->Mains ); } } );

    // siłę generują pojazdy na pochyleniu ale działa ona całość składu, więc a=F/m
    fAccGravity *= ( iDirection >= 0 ? 1 : -1 );
    fAccGravity /= fMass;
    {
        auto absaccs { fAccGravity }; // Ra 2014-03: jesli skład stoi, to działa na niego składowa styczna grawitacji
        if( mvOccupied->Vel > EU07_AI_NOMOVEMENT ) {
            absaccs = 0;
            auto *d = pVehicles[ end::front ]; // pojazd na czele składu
            while( d ) {
                absaccs += d->MoverParameters->TotalMass * d->MoverParameters->AccS * ( d->DirectionGet() == iDirection ? 1 : -1 );
                d = d->Next(); // kolejny pojazd, podłączony od tyłu (licząc od czoła)
            }
            absaccs *= ( iDirection >= 0 ? 1 : -1 );
            absaccs /= fMass;
        }
        AbsAccS = absaccs;
    }
#if LOGVELOCITY
    WriteLog( "Vel=" + to_string( DirectionalVel() ) + ", AbsAccS=" + to_string( AbsAccS ) + ", AccGrav=" + to_string( fAccGravity ) );
#endif

    if( ( !Ready ) // v367: jeśli wg powyższych warunków skład nie jest odhamowany
     && ( fAccGravity < -0.05 ) // jeśli ma pod górę na tyle, by się stoczyć
     && ( fReady < 0.8 ) ) { // delikatniejszy warunek, obejmuje wszystkie wagony
        Ready = true; //żeby uznać za odhamowany
    }
    // second pass, for diesel engines verify the (live) engines are fully started
    // TODO: cache presence of diesel engines in the consist, to skip this test if there isn't any
    p = pVehicles[ end::front ]; // pojazd na czole składu
    while( ( true == Ready )
        && ( p != nullptr ) ) {

        auto const *vehicle { p->MoverParameters };

        if( has_diesel_engine() ) {

            Ready = (
                ( vehicle->Vel > EU07_AI_MOVEMENT ) // already moving
             || ( false == vehicle->Mains ) // deadweight vehicle
             || ( vehicle->enrot > 0.8 * (
                    vehicle->EngineType == TEngineType::DieselEngine ?
                        vehicle->dizel_nmin :
                        vehicle->DElist[ 0 ].RPM / 60.0 ) ) );
        }
        p = p->Next(); // pojazd podłączony z tyłu (patrząc od czoła)
    }
    // test consist state for external events and/or weird things human user could've done
    iEngineActive &=
           ( false == IsAnyConverterOverloadRelayOpen )
        && ( false == IsAnyLineBreakerOpen )
        && ( ( false == IsAnyConverterPresent ) || ( true == IsAnyConverterEnabled ) )
        && ( ( false == IsAnyCompressorPresent ) || ( true == IsAnyCompressorEnabled ) );
}

void
TController::control_wheelslip() {
    // wheel slip
    if( mvControlling->SlippingWheels ) {
        cue_action( driver_hint::sandingon ); // piasku!
        cue_action( driver_hint::tractiveforcedecrease );
        cue_action( driver_hint::brakingforcedecrease );
        cue_action( driver_hint::antislip );
        ++iDriverFailCount;
    }
    else if( std::fabs( mvControlling->Im ) > 0.75 * mvControlling->ImaxHi ) {
        cue_action( driver_hint::sandingon ); // piaskujemy tory, coby się nie ślizgać
    }
    else {
        // deactivate sandbox if we aren't slipping
        if( mvControlling->SandDose ) {
            cue_action( driver_hint::sandingoff );
        }
    }
}

void
TController::control_pantographs() {

    if( mvPantographUnit->EnginePowerSource.SourceType != TPowerSource::CurrentCollector ) { return; }

    if( ( false == mvPantographUnit->PantAutoValve )
     && ( mvOccupied->ScndPipePress > 4.3 ) ) {
        // gdy główna sprężarka bezpiecznie nabije ciśnienie to można przestawić kurek na zasilanie pantografów z głównej pneumatyki
        cue_action( driver_hint::pantographairsourcesetmain );
    }

    // uśrednione napięcie sieci: przy spadku poniżej wartości minimalnej opóźnić rozruch o losowy czas
    fVoltage = 0.5 * (fVoltage + std::max( mvControlling->GetTrainsetHighVoltage(), mvControlling->PantographVoltage ) );
    if( fVoltage < mvControlling->EnginePowerSource.CollectorParameters.MinV ) {
        // gdy rozłączenie WS z powodu niskiego napięcia
        if( fActionTime >= PrepareTime ) {
            // jeśli czas oczekiwania nie został ustawiony, losowy czas oczekiwania przed ponownym załączeniem jazdy
            fActionTime = -2.0 - Random( 10 );
        }
    }

    // raise/lower pantographs as needed
    auto const useregularpantographlayout {
        ( pVehicle->Next( coupling::control ) == nullptr ) // standalone
     || ( is_emu() ) // special case
     || ( mvControlling->TrainType == dt_ET41 ) }; // special case

    if( mvOccupied->Vel > EU07_AI_NOMOVEMENT ) {

        if( ( fOverhead2 >= 0.0 ) || iOverheadZero ) {
            // jeśli jazda bezprądowa albo z opuszczonym pantografem
            cue_action( driver_hint::mastercontrollersetzerospeed );
        }

        if( ( fOverhead2 > 0.0 ) || iOverheadDown ) {
            // jazda z opuszczonymi pantografami
            if( mvPantographUnit->Pantographs[ end::front ].is_active ) {
                cue_action( driver_hint::frontpantographvalveoff );
            }
            if( mvPantographUnit->Pantographs[ end::rear ].is_active ) {
                cue_action( driver_hint::rearpantographvalveoff );
            }
        }
        else {
            // jeśli nie trzeba opuszczać pantografów
            // TODO: check if faction alone reduces frequency enough
            if( fActionTime > 0.0 ) {
                if( mvOccupied->AIHintPantstate == 0 ) {
                    // jazda na tylnym
                    if( ( iDirection >= 0 ) && ( useregularpantographlayout ) ) {
                        // jak jedzie w kierunku sprzęgu 0
                        if( ( mvPantographUnit->PantRearVolt == 0.0 )
                            // filter out cases with single _other_ working pantograph so we don't try to raise something we can't
                         && ( ( mvPantographUnit->PantographVoltage == 0.0 )
                           || ( mvPantographUnit->EnginePowerSource.CollectorParameters.CollectorsNo > 1 ) ) ) {
                            cue_action( driver_hint::rearpantographvalveon );
                        }
                    }
                    else {
                        // jak jedzie w kierunku sprzęgu 0
                        if( ( mvPantographUnit->PantFrontVolt == 0.0 )
                            // filter out cases with single _other_ working pantograph so we don't try to raise something we can't
                         && ( ( mvPantographUnit->PantographVoltage == 0.0 )
                           || ( mvPantographUnit->EnginePowerSource.CollectorParameters.CollectorsNo > 1 ) ) ) {
                            cue_action( driver_hint::frontpantographvalveon );
                        }
                    }

                    if( mvOccupied->Vel > 5 ) {
                        // opuszczenie przedniego po rozpędzeniu się o ile jest więcej niż jeden
                        if( mvPantographUnit->EnginePowerSource.CollectorParameters.CollectorsNo > 1 ) {
                            if( ( iDirection >= 0 ) && ( useregularpantographlayout ) ) // jak jedzie w kierunku sprzęgu 0
                            { // poczekać na podniesienie tylnego
                                if( ( mvPantographUnit->PantFrontVolt != 0.0 )
                                 && ( mvPantographUnit->PantRearVolt != 0.0 ) ) { // czy jest napięcie zasilające na tylnym?
                                    cue_action( driver_hint::frontpantographvalveoff ); // opuszcza od sprzęgu 0
                                }
                            }
                            else { // poczekać na podniesienie przedniego
                                if( ( mvPantographUnit->PantRearVolt != 0.0 )
                                 && ( mvPantographUnit->PantFrontVolt != 0.0 ) ) { // czy jest napięcie zasilające na przednim?
                                    cue_action( driver_hint::rearpantographvalveoff ); // opuszcza od sprzęgu 1
                                }
                            }
                        }
                    }

                }
                else {
                    // use suggested pantograph setup
                    if( mvOccupied->Vel > 5 ) {
                        auto const pantographsetup{ mvOccupied->AIHintPantstate };
                        cue_action(
                            ( pantographsetup & ( 1 << 0 ) ?
                                driver_hint::frontpantographvalveon :
                                driver_hint::frontpantographvalveoff ) );
                        cue_action(
                            ( pantographsetup & ( 1 << 1 ) ?
                                driver_hint::rearpantographvalveon :
                                driver_hint::rearpantographvalveoff ) );
                    }
                }
            }
        }
    }
    else {
        if( ( mvOccupied->AIHintPantUpIfIdle )
         && ( IdleTime > 45.0 )
            // NOTE: abs(stoptime) covers either at least 15 sec remaining for a scheduled stop, or 15+ secs spent at a basic stop
         && ( std::abs( fStopTime ) > 15.0 ) ) {
            // spending a longer at a stop, raise also front pantograph
            if( mvPantographUnit->EnginePowerSource.CollectorParameters.CollectorsNo > 1 ) {
                if( ( iDirection >= 0 ) && ( useregularpantographlayout ) ) {
                    // jak jedzie w kierunku sprzęgu 0
                    if( mvPantographUnit->PantFrontVolt == 0.0 ) {
                        cue_action( driver_hint::frontpantographvalveon, 5 ); // discard the hint if speed exceeds 5 km/h
                    }
                }
                else {
                    if( mvPantographUnit->PantRearVolt == 0.0 ) {
                        cue_action( driver_hint::rearpantographvalveon, 5 ); // discard the hint if speed exceeds 5 km/h
                    }
                }
            }
        }
    }
}

void
TController::control_horns( double const Timedelta ) {
    // horn control
    if( fWarningDuration > 0.0 ) {
        // jeśli pozostało coś do wytrąbienia trąbienie trwa nadal
        if( mvOccupied->WarningSignal == 0 ) {
            cue_action( driver_hint::hornon, 1 ); // low tone horn by default
        }
    }
    else {
        if( mvOccupied->WarningSignal != 0 ) {
            cue_action( driver_hint::hornoff ); // a tu się kończy
        }
    }
    if( mvOccupied->Vel > EU07_AI_MOVEMENT ) {
        // jesli jedzie, można odblokować trąbienie, bo się wtedy nie włączy
        iDrivigFlags &= ~moveStartHornDone; // zatrąbi dopiero jak następnym razem stanie
        iDrivigFlags |= moveStartHorn; // i trąbić przed następnym ruszeniem
    }

    if( ( true == TestFlag( iDrivigFlags, moveStartHornNow ) )
     && ( true == Ready )
     && ( true == iEngineActive )
     && ( mvControlling->MainCtrlPowerPos() > 0 ) ) {
        // uruchomienie trąbienia przed ruszeniem
        fWarningDuration = 0.3; // czas trąbienia
        cue_action( driver_hint::hornon, pVehicle->iHornWarning ); // wysokość tonu (2=wysoki)
        iDrivigFlags |= moveStartHornDone; // nie trąbić aż do ruszenia
        iDrivigFlags &= ~moveStartHornNow; // trąbienie zostało zorganizowane
    }
}

void
TController::control_security_system( double const Timedelta ) {
    if( mvOccupied->SecuritySystem.is_cabsignal_blinking() && mvOccupied->SecuritySystem.has_separate_acknowledge()) {
        // jak zadziałało SHP
        if( ( false == is_emu() )
         && ( mvOccupied->DirActive == 0 ) ) {
            cue_action( driver_hint::directionforward );
        }
        cue_action( driver_hint::shpsystemreset ); // to skasuj
        if( BrakeCtrlPosition == 0 // TODO: verify whether it's 0 in all vehicle types
         && AccDesired > 0.0
         && mvOccupied->SecuritySystem.is_braking() ) {
            cue_action( driver_hint::trainbrakerelease );
        }
    }

    if( mvOccupied->SecuritySystem.is_blinking() ) {
        // jak zadziałało CA/SHP
        if( ( false == is_emu() )
         && ( mvOccupied->DirActive == 0 ) ) {
            cue_action( driver_hint::directionforward );
        }
        cue_action( driver_hint::securitysystemreset ); // to skasuj
        if( BrakeCtrlPosition == 0 // TODO: verify whether it's 0 in all vehicle types
         && AccDesired > 0.0
         && mvOccupied->SecuritySystem.is_braking() ) {
            cue_action( driver_hint::trainbrakerelease );
        }
    }
    // basic emergency stop handling, while at it
    if( ( true == mvOccupied->RadioStopFlag ) // radio-stop
     && ( mvOccupied->Vel < 0.01 ) // and actual stop
     && ( true == mvOccupied->Radio ) ) { // and we didn't touch the radio yet
        // turning off the radio should reset the flag, during security system check
        if( m_radiocontroltime > 5.0 ) { // arbitrary delay between stop and disabling the radio
            cue_action( driver_hint::radiooff );
        }
        else {
            m_radiocontroltime += Timedelta;
        }
    }
    if( ( iEngineActive )
     && ( false == mvOccupied->Radio )
     && ( false == mvOccupied->RadioStopFlag ) ) {
        // otherwise if it's safe to do so, turn the radio back on
        // arbitrary 5 sec delay before switching radio back on
        m_radiocontroltime = std::min( m_radiocontroltime, 5.0 );
        if( m_radiocontroltime < 0.0 ) {
            cue_action( driver_hint::radioon );
        }
        else {
            m_radiocontroltime -= Timedelta;
        }
    }
}

void
TController::control_handles() {

    switch( mvControlling->EngineType ) {
    case TEngineType::ElectricSeriesMotor: {
        // styczniki liniowe rozłączone    yBARC
        if( ( false == mvControlling->StLinFlag )
         && ( false == mvControlling->DelayCtrlFlag )
         && ( mvControlling->MainCtrlPowerPos() > 1 ) ) {
            cue_action( driver_hint::mastercontrollersetzerospeed );
        }
        // if the power station is heavily burdened drop down to series mode to reduce the load
        auto const useseriesmodevoltage {
            interpolate(
                mvControlling->EnginePowerSource.CollectorParameters.MinV,
                mvControlling->EnginePowerSource.CollectorParameters.MaxV,
                ( IsHeavyCargoTrain ? 0.35 : 0.40 ) ) };

        if( fVoltage <= useseriesmodevoltage ) {
            cue_action( driver_hint::mastercontrollersetseriesmode );
        }
        // sanity check
        if( ( false == Ready )
         && ( mvControlling->MainCtrlPowerPos() > 1 ) ) {
            cue_action( driver_hint::mastercontrollersetzerospeed );
        }
        break;
    }
    case TEngineType::DieselElectric: {
        // styczniki liniowe rozłączone    yBARC
        if( ( false == mvControlling->StLinFlag )
         && ( false == mvControlling->DelayCtrlFlag )
         && ( mvControlling->MainCtrlPowerPos() > 1 ) ) {
            cue_action( driver_hint::mastercontrollersetzerospeed );
        }
        // sanity check
        if( ( false == Ready )
         && ( mvControlling->MainCtrlPowerPos() > 1 ) ) {
            cue_action( driver_hint::mastercontrollersetzerospeed );
        }
        break;
    }
    default: {
        break;
    } } // enginetype
}

// activate consist lamp codes
void
TController::control_lights() {

    if( is_train() ) {
        // jeśli jazda pociągowa
        if( true == TestFlag( OrderCurrentGet(), Obey_train ) ) {
            // head lights
            if( m_lighthints[ end::front ] == -1 ) {
                cue_action( driver_hint::headcodepc1 );
            }
            else if( m_lighthints[ end::front ] == ( light::redmarker_left | light::headlight_right | light::headlight_upper ) ) {
                cue_action( driver_hint::headcodepc2 );
            }
            else {
                // custom light pattern
                if( AIControllFlag ) {
                    pVehicles[ end::front ]->RaLightsSet( m_lighthints[ end::front ], -1 );
                }
            }
            // tail lights
            if( m_lighthints[ end::rear ] == -1 ) {
                cue_action( driver_hint::headcodepc5 );
            }
            else {
                if( AIControllFlag ) {
                    pVehicles[ end::rear ]->RaLightsSet( -1, m_lighthints[ end::rear ] );
                }
            }
        }
        // for shunting mode
        else if( OrderCurrentGet() & ( Shunt | Loose_shunt | Connect ) ) {
            cue_action( driver_hint::headcodetb1 );
        }
    }
    else if( is_car() ) {
        Lights(
            light::headlight_left | light::headlight_right,
            light::redmarker_left | light::redmarker_right );
    }
}

void
TController::control_compartment_lights() {
    // compartment lights
    if( mvOccupied->CompartmentLights.start_type != start_t::manual ) { return; }

    auto const currentlightstate { mvOccupied->CompartmentLights.is_enabled };
    auto const lightlevel { Global.fLuminance * ConsistShade };
    auto const desiredlightstate{ (
        currentlightstate ?
            lightlevel < 0.40 : // turn off if lighting level goes above 0.4
            lightlevel < 0.35 ) }; // turn on if lighting level goes below 0.35
    if( desiredlightstate != currentlightstate ) {
        cue_action( (
            desiredlightstate ?
                driver_hint::consistlightson :
                driver_hint::consistlightsoff ) );
    }
}

void
TController::control_doors() {

    if( false == doors_open() ) { return; }
    // jeżeli jedzie
    // nie zamykać drzwi przy drganiach, bo zatrzymanie na W4 akceptuje niewielkie prędkości
    if( mvOccupied->Vel > EU07_AI_MOVEMENT ) {
        Doors( false );
        return;
    }
    // HACK: crude way to deal with automatic door opening on W4 preventing further ride
    // for human-controlled vehicles with no door control and dynamic brake auto-activating with door open
    // TODO: check if this situation still happens and the hack is still needed
    if( false == AIControllFlag ) {
        // for diesel engines react when engine is put past idle revolutions
        // for others straightforward master controller check
        if( ( mvControlling->EngineType == TEngineType::DieselEngine ?
                mvControlling->RList[ mvControlling->MainCtrlPos ].Mn > 0 :
                mvControlling->MainCtrlPowerPos() > 0 ) ) {
            Doors( false );
            return;
        }
    }
}

void
TController::UpdateCommand() {

    if( false == mvOccupied->CommandIn.Command.empty() ) {
        if( false == mvOccupied->RunInternalCommand() ) {
            // rozpoznaj komende bo lokomotywa jej nie rozpoznaje
            RecognizeCommand(); // samo czyta komendę wstawioną do pojazdu?
        }
    }
}

void
TController::UpdateNextStop() {

    if( ( fLastStopExpDist > 0.0 )
     && ( mvOccupied->DistCounter > fLastStopExpDist ) ) {
        // zaktualizować wyświetlanie rozkładu
        TrainParams.StationStart = TrainParams.StationIndex;
        fLastStopExpDist = -1.0; // usunąć licznik
        if( true == m_makenextstopannouncement ) {
            announce( announcement_t::next );
            m_makenextstopannouncement = false; // keep next stop announcements suppressed until another scheduled stop
        }
    }
}

void
TController::determine_braking_distance() {
    // przybliżona droga hamowania
    // NOTE: we're ensuring some minimal braking distance to reduce ai flipping states between starting and braking
    auto const velceil { std::max( 2.0, std::ceil( mvOccupied->Vel ) ) };
    fBrakeDist = fDriverBraking * velceil * ( 40.0 + velceil );
    if( fMass > 1000000.0 ) {
        // korekta dla ciężkich, bo przeżynają - da to coś?
        fBrakeDist *= 2.0;
    }
    if( ( -fAccThreshold > 0.05 )
     && ( mvOccupied->CategoryFlag == 1 ) ) {
        fBrakeDist = velceil *	velceil / 25.92 / -fAccThreshold;
    }
    if( mvOccupied->BrakeDelayFlag == bdelay_G ) {
        // dla nastawienia G koniecznie należy wydłużyć drogę na czas reakcji
        fBrakeDist += 2 * velceil;
    }
    if( ( mvOccupied->Vel > 15.0 )
     && ( mvControlling->EngineType == TEngineType::ElectricInductionMotor )
     && ( is_emu() ) ) {
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
}

void
TController::scan_route( double const Range ) {

    TableCheck( Range );
}

// check for potential collisions
void
TController::scan_obstacles( double const Range ) {
    // HACK: vehicle order in the consist is based on intended travel direction
    // if our actual travel direction doesn't match that, we should be scanning from the other end of the consist
    // we cast to int to avoid getting confused by microstutters
    auto *frontvehicle { pVehicles[ ( static_cast<int>( mvOccupied->V ) * iDirection >= 0 ? end::front : end::rear ) ] };

    int routescandirection;
    // for moving vehicle determine heading from velocity; for standing fall back on the set direction
    if( ( std::abs( frontvehicle->MoverParameters->V ) > 0.5 ? // ignore potential micro-stutters in oposite direction during "almost stop"
        frontvehicle->MoverParameters->V > 0.0 :
        ( pVehicle->DirectionGet() == frontvehicle->DirectionGet() ?
            iDirection >= 0 :
            iDirection <= 0 ) ) ) {
        // towards coupler 0
        routescandirection = end::front;
    }
    else {
        // towards coupler 1
        routescandirection = end::rear;
    }
/*
    if( pVehicle->MoverParameters->CabOccupied < 0 ) {
        // flip the scan direction in the rear cab
        routescandirection ^= routescandirection;
    }
*/
    Obstacle = neighbour_data();
    auto const obstaclescanrange { std::max( ( is_car() ? 250.0 : 1000.0 ), Range ) };
    auto const lookup { frontvehicle->find_vehicle( routescandirection, obstaclescanrange ) };

    if( std::get<bool>( lookup ) == true ) {

        Obstacle.vehicle = std::get<TDynamicObject *>( lookup );
        Obstacle.vehicle_end = std::get<int>( lookup );
        Obstacle.distance = std::get<double>( lookup );

        if( Obstacle.distance < ( is_car() ? 50 : 100 ) ) {
            // at short distances (re)calculate range between couplers directly
            Obstacle.distance = TMoverParameters::CouplerDist( frontvehicle->MoverParameters, Obstacle.vehicle->MoverParameters );
        }
    }
}

void
TController::determine_proximity_ranges() {
     // ustalenie dystansów w pozostałych przypadkach
    switch (OrderCurrentGet())
    {
    case Connect: {
        // podłączanie do składu
        if (iDrivigFlags & moveConnect) {
            // jeśli stanął już blisko, unikając zderzenia i można próbować podłączyć
            fMinProximityDist = -1.0;
            fMaxProximityDist =  0.0; //[m] dojechać maksymalnie
            fVelPlus = 1.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 0.5; // margines prędkości powodujący załączenie napędu
        }
        else {
            // jak daleko, to jazda jak dla Shunt na kolizję
            fMinProximityDist = 2.0;
            fMaxProximityDist = 5.0; //[m] w takim przedziale odległości powinien stanąć
            fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelMinus = 1.0; // margines prędkości powodujący załączenie napędu
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
        // HACK: modern vehicles might brake slower at low speeds, increase safety margin as crude counter
        if( mvControlling->EIMCtrlType > 0 ) {
            fMinProximityDist += 5.0;
            fMaxProximityDist += 5.0;
        }
        // take into account weather conditions
        if( ( Global.FrictionWeatherFactor < 1.f )
         && ( iVehicles > 1 ) ) {
            fMinProximityDist += 5.0;
            fMaxProximityDist += 5.0;
        }
        fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        // margines prędkości powodujący załączenie napędu
        // były problemy z jazdą np. 3km/h podczas ładowania wagonów
        fVelMinus = std::min( 0.1 * fShuntVelocity, 3.0 );
        break;
    }
    case Loose_shunt: {
        fMinProximityDist = -1.0;
        fMaxProximityDist = 0.0; //[m] dojechać maksymalnie
        fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        fVelMinus = 0.5; // margines prędkości powodujący załączenie napędu
        break;
    }
    case Obey_train: {
        // na jaka odleglosc i z jaka predkoscia ma podjechac do przeszkody
        // jeśli pociąg
        if( is_train() ) {
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

            if( ( Global.FrictionWeatherFactor < 1.f )
             && ( iVehicles > 1 ) ) {
            // take into account weather conditions
                fMinProximityDist += 5.0;
                fMaxProximityDist += 5.0;
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
        // samochod (sokista też)
        else {
            fMinProximityDist = std::max( 3.5, mvOccupied->Vel * 0.2   );
            fMaxProximityDist = std::max( 9.5, mvOccupied->Vel * 0.375 ); //[m]
            if( Global.FrictionWeatherFactor < 1.f ) {
                // take into account weather conditions
                fMinProximityDist += 0.75;
                fMaxProximityDist += 0.75;
            }
            // margines prędkości powodujący załączenie napędu
            fVelMinus = 2.0;
            // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
            fVelPlus = std::min( 10.0, mvOccupied->Vel * 0.1 );
        }
        break;
    }
    case Bank: {
        // TODO: implement
        break;
    }
    default: {
        fMinProximityDist = 5.0;
        fMaxProximityDist = 10.0; //[m]
        fVelPlus = 2.0; // dopuszczalne przekroczenie prędkości na ograniczeniu bez hamowania
        fVelMinus = 5.0; // margines prędkości powodujący załączenie napędu
    }
    } // switch OrderList[OrderPos]
}

void
TController::check_departure() {

    if(( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank | Connect ) ) == 0 ) { return; }
    // odjechać sam może tylko jeśli jest w trybie jazdy
    // automatyczne ruszanie po odstaniu albo spod SBL
    if( ( VelSignal == 0.0 )
     && ( WaitingTime > 0.0 )
     && ( mvOccupied->RunningTrack.Velmax != 0.0 ) ) {
        // jeśli stoi, a upłynął czas oczekiwania i tor ma niezerową prędkość
        if( ( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank ) )
         && ( iDrivigFlags & moveStopHere ) ) {
            // zakaz ruszania z miejsca bez otrzymania wolnej drogi
            WaitingTime = -WaitingExpireTime;
        }
        else {
            if( mvOccupied->CategoryFlag & 1 ) { // jeśli pociąg
                PrepareDirection(); // zmieni ustawiony kierunek
                SetVelocity( 20, 20 ); // jak się nastał, to niech jedzie 20km/h
                WaitingTime = 0.0;
                fWarningDuration = 1.5; // a zatrąbić trochę
            }
            else { // samochód ma stać, aż dostanie odjazd, chyba że stoi przez kolizję
                if( ( eStopReason == stopBlock )
                 && ( Obstacle.distance > fDriverDist ) ) {
                    PrepareDirection(); // zmieni ustawiony kierunek
                    SetVelocity( -1, -1 ); // jak się nastał, to niech jedzie
                    WaitingTime = 0.0;
                }
            }
        }
    }
    else if( ( VelSignal == 0.0 ) && ( VelNext > 0.0 ) && ( mvOccupied->Vel < 1.0 ) ) {
        if( ( iCoupler > 0 ) || ( ( iDrivigFlags & moveStopHere ) == 0 ) ) {
            // Ra: tu jest coś nie tak, bo bez tego warunku ruszało w manewrowym !!!!
            SetVelocity( VelNext, VelNext, stopSem ); // omijanie SBL
        }
    }
}

// verify progress of load exchange
void
TController::check_load_exchange() {

    ExchangeTime = 0.f;
    DoesAnyDoorNeedOpening = false;

    if( fStopTime > 0 ) { return; }

    // czas postoju przed dalszą jazdą (np. na przystanku)
    auto *vehicle { pVehicles[ end::front ] };
    while( vehicle != nullptr ) {
        auto const vehicleexchangetime { vehicle->LoadExchangeTime() };
        DoesAnyDoorNeedOpening |= ( ( vehicleexchangetime > 0 ) && ( vehicle->LoadExchangeSpeed() == 0 ) );
        ExchangeTime = std::max( ExchangeTime, vehicleexchangetime );
        vehicle = vehicle->Next();
    }

    if( pVehicle->DirectionGet() != m_lastexchangedirection ) {
        // generally means the ai driver moved to the opposite end of the consist
        // TODO: investigate whether user playing with the reverser can mess this up
        auto const left { ( m_lastexchangedirection > 0 ) ? 1 : 2 };
        auto const right { 3 - left };
        m_lastexchangeplatforms =
            ( ( m_lastexchangeplatforms & left )  != 0 ? right : 0 )
            + ( ( m_lastexchangeplatforms & right ) != 0 ? left : 0 );
        m_lastexchangedirection = pVehicle->DirectionGet();
    }
    if( ( false == TrainParams.IsMaintenance() )
     && ( ( false == TestFlag( iDrivigFlags, moveDoorOpened ) )
       || ( true == DoesAnyDoorNeedOpening ) ) ) {
        iDrivigFlags |= moveDoorOpened; // nie wykonywać drugi raz
        remove_hint( driver_hint::doorleftopen );
        remove_hint( driver_hint::doorrightopen );
        Doors( true, m_lastexchangeplatforms );
    }
}

void
TController::UpdateChangeDirection() {
    // TODO: rework into driver mode independent routine
    if( false == TestFlag( OrderCurrentGet(), Change_direction ) ) { return; }

    if( true == AIControllFlag ) {
        // sprobuj zmienic kierunek (może być zmieszane z jeszcze jakąś komendą)
        if( mvOccupied->Vel < EU07_AI_NOMOVEMENT ) {
            // jeśli się zatrzymał, to zmieniamy kierunek jazdy, a nawet kabinę/człon
            Activation(); // ustawienie zadanego wcześniej kierunku i ewentualne przemieszczenie AI
        } // Change_direction (tylko dla AI)
    }
    // shared part of the routine, implement if direction matches what was requested
    if( ( mvOccupied->Vel < EU07_AI_NOMOVEMENT )
     && ( iDirection == iDirectionOrder ) ) {
        // NOTE: we can't be sure there's a visible signal within scan range after direction change
        // which would normally overwrite the old limit, so we reset signal value manually here
        VelSignal = -1;
        VelSignalNext = -1;
        PrepareEngine();
        JumpToNextOrder(); // następnie robimy, co jest do zrobienia (Shunt albo Obey_train)
        if( OrderCurrentGet() & ( Shunt | Loose_shunt | Connect ) ) {
            // jeśli dalej mamy manewry
            if( false == TestFlag( iDrivigFlags, moveStopHere ) ) {
                // o ile nie ma stać w miejscu,
                // jechać od razu w przeciwną stronę i nie trąbić z tego tytułu:
                iDrivigFlags &= ~moveStartHorn;
                SetVelocity( fShuntVelocity, fShuntVelocity );
            }
        }
    }
}

void
TController::UpdateLooseShunt() {
    // when loose shunting try to detect situations where engaged brakes in a consist to be pushed prevent movement
    // TODO: run also for potential settings-based virtual assistant
    auto const autobrakerelease { ( true == AIControllFlag ) };

    if( ( autobrakerelease )
     && ( Obstacle.distance < 1.0 )
     && ( AccDesired > 0.1 )
     && ( mvOccupied->Vel < 1.0 ) ) {

        auto *vehicle { Obstacle.vehicle };
        auto const direction { ( vehicle->Prev() != nullptr ? end::front : end::rear ) };
        while( vehicle != nullptr ) {
            if( vehicle->MoverParameters->BrakePress > 0.2 ) {
                vehicle->MoverParameters->BrakeLevelSet( 0 ); // hamulec na zero, aby nie hamował
                vehicle->MoverParameters->BrakeReleaser( 1 ); // wyluzuj pojazd, aby dało się dopychać
            }
            // NOTE: we trust the consist to be arranged in a valid chain
            // TBD, TODO: traversal direction validation?
            vehicle = ( direction == end::front ? vehicle->Prev() : vehicle->Next() );
        }
    }
}

void
TController::UpdateObeyTrain() {

    UpdateNextStop();

    if( ( ( iDrivigFlags & moveGuardSignal ) != 0 )
     && ( VelDesired > 0.0 ) ) {
        // komunikat od kierownika tu, bo musi być wolna droga i odczekany czas stania
        iDrivigFlags &= ~moveGuardSignal; // tylko raz nadać
		iDrivigFlags &= ~moveGuardOpenDoor; // nie trzeba już otwierać drzwi
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
                // NOTE: we can't rely on is_playing() check as sound playback is based on distance from local camera
                fActionTime = -5.0; // niech trochę potrzyma
            }
            else {
                // radio message
                tsGuardSignal.owner( pVehicle );
/*
                tsGuardSignal.offset( { 0.f, 2.f, pVehicle->MoverParameters->Dim.L * 0.4f * ( pVehicle->MoverParameters->CabOccupied < 0 ? -1 : 1 ) } );
                tsGuardSignal.play( sound_flags::exclusive );
*/
                simulation::radio_message( &tsGuardSignal, iGuardRadio );
                // NOTE: we can't rely on is_playing() check as sound playback is based on distance from local camera
                fActionTime = -5.0; // niech trochę potrzyma
            }
        }
    }
}

void
TController::UpdateConnect() {
    // podłączanie do składu
    if (iDrivigFlags & moveConnect) {
        // sprzęgi sprawdzamy w pierwszej kolejności, bo jak połączony, to koniec
        auto *vehicle { iCouplingVehicle.value().first };
        auto const couplingend { iCouplingVehicle.value().second };
        auto *vehicleparameters { vehicle->MoverParameters };
        if( vehicleparameters->Couplers[ couplingend ].CouplingFlag != iCoupler ) {
            auto const &neighbour { vehicleparameters->Neighbours[ couplingend ] };
            if( neighbour.vehicle != nullptr ) {
                if( neighbour.distance < 10 ) {
                    // check whether we need to attach coupler adapter
                    auto coupleriscompatible { true };
                    auto const &coupler { vehicleparameters->Couplers[ couplingend ] };
                    if( coupler.type() != TCouplerType::Automatic ) {
                        auto const &othercoupler = neighbour.vehicle->MoverParameters->Couplers[ ( neighbour.vehicle_end != 2 ? neighbour.vehicle_end : coupler.ConnectedNr ) ];
                        if( othercoupler.type() == TCouplerType::Automatic ) {
                            coupleriscompatible = false;
                            cue_action( driver_hint::couplingadapterattach, couplingend );
                        }
                    }
                    // próba podczepienia
                    if( AIControllFlag || Global.AITrainman ) {
                        if( ( coupleriscompatible ) && ( neighbour.distance < 2 ) ) {
                            vehicleparameters->Attach(
                                couplingend, neighbour.vehicle_end,
                                neighbour.vehicle->MoverParameters,
                                iCoupler );
                        }
                    }
                }
            }
        }
        // NOTE: no else as the preceeding block can potentially establish expected connection
        if( vehicleparameters->Couplers[ couplingend ].CouplingFlag == iCoupler ) {
            // jeżeli został podłączony
            CheckVehicles( Connect ); // sprawdzić światła nowego składu

            iCoupler = 0; // dalsza jazda manewrowa już bez łączenia
            iCouplingVehicle.reset();
            iDrivigFlags &= ~moveConnect; // zdjęcie flagi doczepiania
            JumpToNextOrder( true ); // wykonanie następnej komendy
        }
    } // moveConnect
    else {
        // początek podczepiania, z wyłączeniem sprawdzania fTrackBlock
        if( Obstacle.distance <= 20.0 ) {
            if( !iCouplingVehicle ) {
                // write down which vehicle should be coupled with the target consist,
                // so we don't lose track of it if the user does something unexpected
                iCouplingVehicle = {
                    pVehicles[ end::front ],
                    ( pVehicles[ end::front ]->DirectionGet() > 0 ?
                        end::front :
                        end::rear ) };
                iDrivigFlags |= moveConnect;
            }
        }
    }
}

void
TController::GuardOpenDoor() {
	if ((iDrivigFlags & moveGuardOpenDoor) != 0) {
		auto *vehicle{ pVehicles[end::front] };
		while (vehicle != nullptr && vehicle->MoverParameters->Doors.range == 0) {
			vehicle = vehicle->Next();
		}
		if (vehicle != nullptr && vehicle->MoverParameters->Doors.range > 0) {
			auto const lewe = (vehicle->DirectionGet() > 0) ? 1 : 2;
			auto const prawe = 3 - lewe;
			if (m_lastexchangeplatforms & lewe) {
				vehicle->MoverParameters->OperateDoors(side::left, true, range_t::local);
			}
			if (m_lastexchangeplatforms & prawe) {
				vehicle->MoverParameters->OperateDoors(side::right, true, range_t::local);
			}
		}
		else {
			iDrivigFlags &= ~moveGuardOpenDoor;
		}
	}
}

int
TController::unit_count( int const Threshold ) const {

    auto *vehicle { pVehicle };
    auto unitcount { 1 };
    do {
        auto const decoupledend{ ( vehicle->DirectionGet() > 0 ? // numer sprzęgu od strony czoła składu
            end::rear :
            end::front ) };
        auto const coupling { vehicle->MoverParameters->Couplers[ decoupledend ].CouplingFlag };
        if( coupling == coupling::faux ) {
            break;
        }
        // jeżeli sprzęg zablokowany to liczymy człony jako jeden
        if( ( coupling & coupling::permanent ) == 0 ) {
            ++unitcount;
        }
        vehicle = vehicle->Next();
    } while( unitcount < Threshold );

    return unitcount;
}

void
TController::UpdateDisconnect() {

    if( iVehicleCount >= 0 ) {
        // early test for human drivers, who might disregard the proper procedure, or perform it before they receive the order to
        if( false == AIControllFlag ) {
            // iVehicleCount = 0 means the consist should be reduced to (1) leading unit, thus we increase our test values by 1
            if( unit_count( iVehicleCount + 2 ) <= iVehicleCount + 1 ) {
                iVehicleCount = -2; // odczepiono, co było do odczepienia
                return; // we'll wrap up the procedure on the next update beat
            }
        }
        // regular uncoupling procedure, performed by ai and naively expected from the human drivers
        // 3rd stage: change direction, compress buffers and uncouple
        if( iDirection != iDirectionOrder ) {
            cue_action( driver_hint::mastercontrollersetreverserunlock );
            cue_action( driver_hint::directionother );
        }
        if( ( ( iDrivigFlags & movePress ) != 0 ) && ( iDirection == iDirectionOrder ) ) {
            if( iVehicleCount >= 0 ) {
                // zmieni się po odczepieniu
                WriteLog( "Uncoupling [" + mvOccupied->Name + "]: actuating and compressing buffers..." );
                cue_action( driver_hint::independentbrakerelease );
                cue_action( driver_hint::bufferscompress );
            }

            WriteLog( "Uncoupling [" + mvOccupied->Name + "]: from " + ( mvOccupied->DirAbsolute > 0 ? "front" : "rear" ) );
            auto *decoupledvehicle { pVehicle }; // pojazd do odczepienia, w (pVehicle) siedzi AI
            int decoupledend; // numer sprzęgu, który sprawdzamy albo odczepiamy
            auto vehiclecount { iVehicleCount }; // ile wagonów ma zostać
            // szukanie pojazdu do odczepienia
            do {
                decoupledend = decoupledvehicle->DirectionGet() > 0 ? // numer sprzęgu od strony czoła składu
                    end::front :
                    end::rear;
                // jeżeli sprzęg zablokowany to liczymy człony jako jeden
                if( decoupledvehicle->MoverParameters->Couplers[ decoupledend ].CouplingFlag & coupling::permanent ) {
                    ++vehiclecount;
                }
                if( decoupledvehicle != pVehicle ) {
                    decoupledvehicle->MoverParameters->BrakeReleaser( 1 ); // wyluzuj pojazd, aby dało się dopychać
                }
                if( vehiclecount ) { // jeśli jeszcze nie koniec
                    decoupledvehicle = decoupledvehicle->Prev(); // kolejny w stronę czoła składu (licząc od tyłu), bo dociskamy
                    if( decoupledvehicle == nullptr ) {
                        vehiclecount = 0; // nie ma co dalej sprawdzać, odczepianie zakończone
                    }
                }
            } while( vehiclecount-- );
            if( decoupledvehicle == nullptr ) {
                // no target, or already just virtual coupling
                WriteLog( "Uncoupling [" + mvOccupied->Name + "]: didn't find anything to disconnect" );
                iVehicleCount = -2; // odczepiono, co było do odczepienia
            }
            else {
                if( AIControllFlag || Global.AITrainman ) {
                    decoupledvehicle->Dettach( decoupledend );
                }
                // tylko jeśli odepnie
                if( decoupledvehicle->MoverParameters->Couplers[ decoupledend ].CouplingFlag == coupling::faux ) {
                    WriteLog( "Uncoupling [" + mvOccupied->Name + "]: uncoupled" );
                    iVehicleCount = -2;
                    // update trainset state
                    CheckVehicles( Disconnect );
                    // potentially remove coupler adapter
/*
                    if( decoupledvehicle->MoverParameters->Couplers[ decoupledend ].has_adapter() ) {
                        decoupledvehicle->remove_coupler_adapter( decoupledend );
                    }
*/
                    if( pVehicles[ end::front ]->MoverParameters->Couplers[ decoupledend ].has_adapter() ) {
                        cue_action( driver_hint::couplingadapterremove, decoupledend );
                    }
                }
            } // a jak nie, to dociskać dalej
        }
        // 2nd stage: apply consist brakes and change direction
        if( ( iDrivigFlags & movePress ) == 0 ) {
            // store initial consist direction
            if( !iDirectionBackup ) {
                iDirectionBackup = iDirection;
            }
            if( false == IsConsistBraked ) {
                WriteLog( "Uncoupling [" + mvOccupied->Name + "]: applying consist brakes..." );
                cue_action( driver_hint::trainbrakeapply );
            }
            // jeśli w miarę został zahamowany (ciśnienie mniejsze niż podane na pozycji 3, zwyle 0.37)
            else {
                WriteLog( "Uncoupling [" + mvOccupied->Name + "]: direction change" );
/* // TODO: test if this block is needed
                if( BrakeSystem == TBrakeSystem::ElectroPneumatic ) {
                    // wyłączenie EP, gdy wystarczy (może nie być potrzebne, bo na początku jest)
                    mvOccupied->BrakeLevelSet( 0 );
                }
*/
                iDirectionOrder = -iDirection; // zmiana kierunku jazdy na przeciwny (dociskanie)
                iDrivigFlags |= movePress; // następnie będzie dociskanie
            }
        }
    } // odczepianie
    if( iVehicleCount < 0 ) {
        // 4th stage: restore initial direction
        if( iDirectionBackup ) {
            iDirectionOrder = iDirectionBackup.value();
            iDirectionBackup.reset();
        }
        if( iDirection != iDirectionOrder ) {
            WriteLog( "Uncoupling [" + mvOccupied->Name + "]: second direction change" );
            cue_action( driver_hint::mastercontrollersetreverserunlock );
            cue_action( driver_hint::directionother ); // zmiana kierunku jazdy na właściwy
        }
        // 5th stage: clean up and move on to next order
        if( iDirection == iDirectionOrder ) {
            iDrivigFlags &= ~movePress; // koniec dociskania
            JumpToNextOrder( true ); // zmieni światła
            TableClear(); // skanowanie od nowa
            iDrivigFlags &= ~moveStartHorn; // bez trąbienia przed ruszeniem
        }
    }
}

void
TController::handle_engine() {
    // HACK: activate route scanning if an idling vehicle is activated by a human user
    if( ( OrderCurrentGet() == Wait_for_orders )
     && ( false == iEngineActive )
//     && ( false == AIControllFlag )
     && ( true == mvOccupied->Power24vIsAvailable ) ) {
        OrderNext( Prepare_engine );
    }
    // basic engine preparation
    if( OrderCurrentGet() == Prepare_engine ) {
        if( PrepareEngine() ) { // gotowy do drogi?
            JumpToNextOrder();
        }
    }
    // engine state can potentially deteriorate in one of usual driving modes
    if( ( OrderCurrentGet() & ( Change_direction | Connect | Disconnect | Shunt | Loose_shunt | Obey_train | Bank ) )
     && ( false == iEngineActive ) ) {
        // jeśli coś ma robić to niech odpala do skutku
        PrepareEngine();
    }
}

void
TController::handle_orders() {

    switch (OrderCurrentGet())
    {
    case Release_engine: {
        if( ReleaseEngine() ) { // zdana maszyna?
            JumpToNextOrder();
        }
        break;
    }
    case Obey_train: {
        UpdateObeyTrain();
        break;
    }
    case Loose_shunt: {
        UpdateLooseShunt();
        break;
    }
    case Connect: {
        UpdateConnect();
        break;
    }
    case Disconnect: {
        UpdateDisconnect();
        break;
    }
    case Jump_to_first_order: {
        if( OrderPos > 1 ) {
            OrderPos = 1; // w zerowym zawsze jest czekanie
        }
        else {
            ++OrderPos;
        }
#if LOGORDERS
        OrdersDump("Jump_to_first_order");
#endif
        break;
    }
    default: {
        // special case, change_direction potentially received during partially completed coupling operation
        // this is unlikely to happen for the AI, but can happen for a (slower) human user
        if( TestFlag( iDrivigFlags, moveConnect ) ) {
            UpdateConnect();
        }
        break;
    }
    } // switch OrderList[OrderPos]
    UpdateChangeDirection();
}

void
TController::pick_optimal_speed( double const Range ) {

//    if( ( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank | Connect | Disconnect | Change_direction ) ) == 0 ) { return; }

    // set initial velocity and acceleration values
    SetDriverPsyche(); // ustawia AccPreferred (potrzebne tu?)
    VelDesired = fVelMax; // wstępnie prędkość maksymalna dla pojazdu(-ów), będzie następnie ograniczana
    AccDesired = AccPreferred; // AccPreferred wynika z osobowości mechanika

    // nie przekraczać rozkladowej
    if( ( ( OrderCurrentGet() & Obey_train ) != 0 )
     && ( TrainParams.TTVmax > 0.0 ) ) {
        VelDesired =
            min_speed(
                VelDesired,
                TrainParams.TTVmax );
    }

//    VelNext = VelDesired; // maksymalna prędkość wynikająca z innych czynników niż trajektoria ruchu
    // HACK: -1 means we can pick up 0 speed limits in ( point.velnext > velnext ) tests aimed to find highest next speed,
    // but also doubles as 'max speed' in min_speed tests, so it shouldn't interfere if we don't find any speed limits
    VelNext = -1.0;

    // basic velocity and acceleration adjustments
    // jeśli manewry, to ograniczamy prędkość
    if( ( OrderCurrentGet() & ( Obey_train | Bank ) ) == 0 ) { // spokojne manewry
        SetVelocity( fShuntVelocity, fShuntVelocity );
        VelDesired =
            min_speed(
                VelDesired,
                fShuntVelocity );
    }
    // uncoupling mode changes velocity/acceleration between stages
    if( ( OrderCurrentGet() & Disconnect ) != 0 ) {
        if( iVehicleCount >= 0 ) {
            // 3rd stage: compress buffers and uncouple
            if( ( ( iDrivigFlags & movePress ) != 0 ) && ( iDirection == iDirectionOrder ) ) {
                SetVelocity( 2, 0 ); // jazda w ustawionym kierunku z prędkością 2
            }
            // 1st stage: bring it to stop
            // 2nd stage: apply consist brakes and change direction
            else {
                SetVelocity( 0, 0, stopJoin );
            }
        }
        else {
            // 4th stage: restore initial direction
            if( ( iDrivigFlags & movePress ) != 0 ) {
                SetVelocity( 0, 0, stopJoin );
            }
        }
    }

    VelLimitLastDist = { VelDesired, -1 };
    SwitchClearDist = -1;
    ActualProximityDist = Range; // funkcja Update() może pozostawić wartości bez zmian

    // if we're idling bail out early
    if( false == is_active() ) {
        VelDesired = 0.0;
        VelNext = 0.0;
        AccDesired = std::min( AccDesired, EU07_AI_NOACCELERATION );
        return;
    }

    // Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prędkosci
    check_route_ahead( Range );

    check_departure();

    // if ordered to turn off the vehicle, try to stop
    if( true == TestFlag( OrderCurrentGet(), Release_engine ) ) {
        SetVelocity( 0, 0, stopSleep );
    }
    // if ordered to change direction, try to stop
    if( true == TestFlag( OrderCurrentGet(), Change_direction ) ) {
        SetVelocity( 0, 0, stopDir );
    }

    adjust_desired_speed_for_obstacles();
    adjust_desired_speed_for_limits();
    adjust_desired_speed_for_target_speed( Range );
    adjust_desired_speed_for_current_speed();
    adjust_desired_speed_for_braking_test();

    // Ra 2F1I: wyłączyć kiedyś to uśrednianie i przeanalizować skanowanie, czemu migocze
    if( AccDesired > EU07_AI_NOACCELERATION ) { // hamowania lepeiej nie uśredniać
        AccDesired = fAccDesiredAv =
            0.2 * AccDesired +
            0.8 * fAccDesiredAv; // uśrednione, żeby ograniczyć migotanie
    }
    if( VelDesired == 0.0 ) {
        // Ra 2F1J: jeszcze jedna prowizoryczna łatka
        AccDesired = std::min( AccDesired, EU07_AI_NOACCELERATION );
    }

    // last step sanity check, until the whole calculation is straightened out
    AccDesired = std::min( AccDesired, AccPreferred );
    AccDesired = clamp(
        AccDesired,
            ( is_car() ? -2.0 : -0.9 ),
            ( is_car() ?  2.0 :  0.9 ) );

    // if the route ahead is blocked we might need to head the other way
    check_route_behind( 1000 ); // NOTE: legacy scan range value
}

void
TController::adjust_desired_speed_for_obstacles() {
    // prędkość w kierunku jazdy, ujemna gdy jedzie w przeciwną stronę niż powinien
    auto const vel { DirectionalVel() };

    if (VelDesired < 0.0)
        VelDesired = fVelMax; // bo VelDesired<0 oznacza prędkość maksymalną

    // Ra: jazda na widoczność
    if( Obstacle.distance < 5000 ) {
        // mamy coś z przodu
        // prędkość pojazdu z przodu (zakładając, że jedzie w tę samą stronę!!!)
        auto const k { Obstacle.vehicle->MoverParameters->Vel };

        if( k - vel < 5 ) {
            // porównanie modułów prędkości [km/h]
            // zatroszczyć się trzeba, jeśli tamten nie jedzie znacząco szybciej
            ActualProximityDist = std::min<double>(
                ActualProximityDist,
                Obstacle.distance );

            if( Obstacle.distance <= (
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
                    AccPreferred = std::min(
                        ( ( mvOccupied->CategoryFlag & 2 ) ?
                        -0.65 : // cars
                        -0.30 ), // others
                        AccPreferred );
                }
            }

            double const distance = Obstacle.distance - fMaxProximityDist - ( fBrakeDist * 1.15 ); // odległość bezpieczna zależy od prędkości
            if( distance < 0.0 ) {
                // jeśli odległość jest zbyt mała
                if( k < 10.0 ) // k - prędkość tego z przodu
                { // jeśli tamten porusza się z niewielką prędkością albo stoi
                    // keep speed difference within a safer margin
                    VelDesired = std::floor(
                        min_speed(
                            VelDesired,
                            ( Obstacle.distance > 100 ?
                                k + 20.0:
                                std::min( 8.0, k + 4.0 ) ) ) );

                    if( ( OrderCurrentGet() & ( Connect | Loose_shunt ) ) != 0 ) {
                        // jeśli spinanie, to jechać dalej
                        AccPreferred = std::min( 0.35, AccPreferred ); // nie hamuj
                        VelNext = std::floor( std::min( 8.0, k + 2.0 ) ); // i pakuj się na tamtego
                    }
                    else {
                        // a normalnie to hamować
                        VelNext = 0.0;
                        if( Obstacle.distance <= fMinProximityDist ) {
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
                    if( Obstacle.distance < (
                            ( mvOccupied->CategoryFlag & 2 ) ?
                                fMaxProximityDist + 0.5 * vel : // cars
                                2.0 * fMaxProximityDist + 2.0 * vel ) ) { //others
                        // jak tamten jedzie wolniej a jest w drodze hamowania
                        AccPreferred = std::min( -0.9, AccPreferred );
                        VelNext = min_speed( std::round( k ) - 5.0, VelDesired );
                        if( Obstacle.distance <= (
                            ( mvOccupied->CategoryFlag & 2 ) ?
                                fMaxProximityDist : // cars
                                2.0 * fMaxProximityDist ) ) { //others
                            // try to force speed change if obstacle is really close
                            VelDesired = VelNext;
                        }
                    }
                }
                ReactionTime = (
                    mvOccupied->Vel > EU07_AI_NOMOVEMENT ?
                        0.1 : // orientuj się, bo jest goraco
                        2.0 ); // we're already standing still, so take it easy
            }
            else {
                if( OrderCurrentGet() & Connect ) {
                    // if there's something nearby in the connect mode don't speed up too much
                    VelDesired =
                        min_speed(
                            VelDesired,
                            ( Obstacle.distance > 100 ?
                                20.0 :
                                4.0 ) );
                }
            }
        }
    }
}

void
TController::adjust_desired_speed_for_limits() {
    // speed caps checks
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

    // recalculate potential load exchange duration
    check_load_exchange();
    if( ( ExchangeTime > 0 )
     || ( mvOccupied->Vel > 2.0 ) ) { // HACK: force timer reset if the load exchange is cancelled due to departure
        WaitingSet( ExchangeTime );
    }
    if( fStopTime < 0 ) {
        VelDesired = 0.0; // jak ma czekać, to nie ma jazdy
        cue_action( driver_hint::waitloadexchange );
        return; // speed limit can't get any lower
    }

    if( ( OrderCurrentGet() & ( Shunt | Loose_shunt | Obey_train | Bank ) ) != 0 ) {
        // w Connect nie, bo moveStopHere odnosi się do stanu po połączeniu
        if( ( ( iDrivigFlags & moveStopHere ) != 0 )
         && ( mvOccupied->Vel < 0.01 )
         && ( VelSignalNext == 0.0 ) ) {
            // jeśli ma czekać na wolną drogę, stoi a wyjazdu nie ma, to ma stać
            VelDesired = 0.0;
            return; // speed limit can't get any lower
        }
    }

    if( OrderCurrentGet() == Wait_for_orders ) {
        // wait means sit and wait
        VelDesired = 0.0;
        return; // speed limit can't get any lower
    }
}

void
TController::adjust_desired_speed_for_target_speed( double const Range ) {
    // ustalanie zadanego przyspieszenia
    //(ActualProximityDist) - odległość do miejsca zmniejszenia prędkości
    //(AccPreferred) - wynika z psychyki oraz uwzglęnia już ewentualne zderzenie z pojazdem z przodu, ujemne gdy należy hamować
    //(AccDesired) - uwzględnia sygnały na drodze ruchu, ujemne gdy należy hamować
    //(fAccGravity) - chwilowe przspieszenie grawitacyjne, ujemne działa przeciwnie do zadanego kierunku jazdy
    //(AbsAccS) - chwilowe przyspieszenie pojazu (uwzględnia grawitację), ujemne działa przeciwnie do zadanego kierunku jazdy
    //(AccDesired) porównujemy z (fAccGravity) albo (AbsAccS)

    // gdy jedzie wolniej niż potrzeba, albo nie ma przeszkód na drodze
    // normalna jazda
    AccDesired = (
        VelDesired != 0.0 ?
            AccPreferred :
            -0.01 );

    auto const vel { DirectionalVel() };

    if( ( VelNext >= 0.0 )
     && ( ActualProximityDist <= Range )
     && ( vel >= VelNext ) ) {
        // gdy zbliża się i jest za szybki do nowej prędkości, albo stoi na zatrzymaniu
        if (vel > EU07_AI_NOMOVEMENT) {
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
                             && ( Obstacle.distance < 50 ) ) {
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
}

void
TController::adjust_desired_speed_for_current_speed() {
    // decisions based on current speed
    auto const vel { DirectionalVel() };

    if( is_train() ) {
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
                    AccDesired = std::min( AccDesired, -0.05 + fAccThreshold );
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
                    AccDesired = std::min( AccDesired, -0.05 + fAccThreshold );
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
                        EU07_AI_NOACCELERATION, AccPreferred,
                        clamp( VelDesired - speedestimate, 0.0, fVelMinus ) / fVelMinus ) );
            }
            // final tweaks
            if( vel > EU07_AI_NOMOVEMENT ) {
                // going downhill also take into account impact of gravity
                AccDesired -= fAccGravity;
                // HACK: if the max allowed speed was exceeded something went wrong; brake harder
                AccDesired -= 0.15 * clamp( vel - VelDesired, 0.0, 5.0 );
            }
        }
        // HACK: limit acceleration for cargo trains, to reduce probability of breaking couplers on sudden jolts
		auto MaxAcc{ 0.5 * mvOccupied->Couplers[(mvOccupied->DirAbsolute >= 0 ? end::rear : end::front)].FmaxC / fMass };
        if( iVehicles - ControlledEnginesCount > 0 ) {
            MaxAcc *= clamp( vel * 0.025, 0.2, 1.0 );
        }
		AccDesired = std::min(AccDesired, clamp(MaxAcc, HeavyCargoTrainAcceleration, AccPreferred));
        // TBD: expand this behaviour to all trains with car(s) exceeding certain weight?
		/*
        if( ( IsPassengerTrain ) && ( iVehicles - ControlledEnginesCount > 0 ) ) {
            AccDesired = std::min( AccDesired, ( iVehicles - ControlledEnginesCount > 8 ? HeavyPassengetTrainAcceleration : PassengetTrainAcceleration ) );
        }
        if( ( IsCargoTrain ) && ( iVehicles - ControlledEnginesCount > 0 ) ) {
            AccDesired = std::min( AccDesired, CargoTrainAcceleration );
        }
        if( ( IsHeavyCargoTrain ) && ( iVehicles - ControlledEnginesCount > 0 ) ) {
            AccDesired = std::min( AccDesired, HeavyCargoTrainAcceleration );
        } */
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

    // NOTE: as a stop-gap measure the routine is limited to trains only while car calculations seem off
    if( is_train() ) {
        if( vel < VelDesired ) {
            // don't adjust acceleration when going above current target speed
            if( -AccDesired * BrakeAccFactor() < (
                ( ( false == Ready )
               || ( VelNext > vel - 40.0 ) ) ?
                    fBrake_a0[ 0 ] * 0.8 :
                    -fAccThreshold )
                / ( 1.2 * braking_distance_multiplier( VelNext ) ) ) {
                AccDesired = std::max( EU07_AI_NOACCELERATION, AccDesired );
            }
        }
    }
}

// hamowanie kontrolne
void
TController::adjust_desired_speed_for_braking_test() {

//    if( false == Global.DynamicBrakeTest ) { return; }
    if( DynamicBrakeTest >= 5 ) { return; } // all done
    if( false == is_train() ) { return; } // not applicable
    if( false == TestFlag( OrderCurrentGet(), Obey_train ) ) { return; } // not applicable
    if( false == AIControllFlag ) { return; } // TBD: add notification about braking test and enable it for human driver as well?

    auto const vel { DirectionalVel() };

    switch( DynamicBrakeTest ) {
        case 0: {
            if( ( primary() )
             && ( vel < VelDesired )
             && ( fAccGravity >= 0.0 ) // not if going uphill
             && ( AccDesired >= EU07_AI_ACCELERATION )
             && ( TrainParams.TTVmax >= 10.0 )
             && ( vel > std::min( TrainParams.TTVmax, 60.0 ) - 2.0 ) ) {
                DynamicBrakeTest = 1;
                DBT_BrakingTime = ElapsedTime;
                DBT_VelocityBrake = vel;
            }
            break;
        }
        case 1: {
            AccDesired = EU07_AI_BRAKINGTESTACCELERATION;
            VelDesired = DBT_VelocityBrake;
            if( ElapsedTime - DBT_BrakingTime > 1 ) {
                ForcePNBrake = true;
                mvOccupied->EpFuseSwitch( false );
                DynamicBrakeTest = 2;
            }
            break;
        }
        case 2: {
            AccDesired = EU07_AI_BRAKINGTESTACCELERATION;
            VelDesired = DBT_VelocityBrake;
            if( ElapsedTime - DBT_BrakingTime > 2 ) {
                DBT_BrakingTime = ElapsedTime;
                DBT_VelocityBrake = vel;
                DBT_VelocityRelease = vel - 8.0;
                DynamicBrakeTest = 3;
            }
            break;
        }
        case 3: {
            AccDesired = clamp( -AbsAccS, fAccThreshold * 1.01, fAccThreshold * 1.21 );
            VelDesired = DBT_VelocityBrake;
            if( vel <= DBT_VelocityRelease ) {
                DynamicBrakeTest = 4;
                DBT_BrakingTime = ElapsedTime - DBT_BrakingTime;
                DBT_MidPointAcc = AbsAccS;
                DBT_ReleasingTime = ElapsedTime;
            }
            break;
        }
        case 4: {
            if( Ready ) {
                if( BrakeSystem == TBrakeSystem::ElectroPneumatic ) {
                    mvOccupied->EpFuseSwitch( true );
                }
                ForcePNBrake = false;
                DynamicBrakeTest = 5;
                DBT_ReleasingTime = ElapsedTime - DBT_ReleasingTime;
                DBT_VelocityFinish = vel;
            }
            break;
        }
        default: {
            break;
        }
    }
}

void
TController::control_tractive_and_braking_force() {

#if LOGVELOCITY
    WriteLog("Dist=" + FloatToStrF(ActualProximityDist, ffFixed, 7, 1) +
                ", VelDesired=" + FloatToStrF(VelDesired, ffFixed, 7, 1) +
                ", AccDesired=" + FloatToStrF(AccDesired, ffFixed, 7, 3) +
                ", VelSignal=" + AnsiString(VelSignal) + ", VelNext=" +
                AnsiString(VelNext));
#endif

    if (iDriverFailCount > maxdriverfails) {
        Psyche = Easyman;
        if (iDriverFailCount > maxdriverfails * 2)
            SetDriverPsyche();
    }

    control_relays();
    control_motor_connectors();

    // if the radio-stop was issued don't waste effort trying to fight it
    if( ( true == mvOccupied->RadioStopFlag ) // radio-stop
     && ( mvOccupied->Vel > EU07_AI_NOMOVEMENT ) ) { // and still moving
        cue_action( driver_hint::mastercontrollersetzerospeed ); // just throttle down...
        return; // ...and don't touch any other controls
    }
    // if we're slipping leave the controls alone, there's a separate logic section trying to deal with it
    if( true == mvControlling->SlippingWheels ) {
        return;
    }

    // ensure train brake isn't locked
    if( BrakeCtrlPosition == gbh_NP ) {
        cue_action( driver_hint::trainbrakerelease );
    }
    control_releaser();
    control_main_pipe();

    control_tractive_force();
    control_braking_force();

#if LOGVELOCITY
    WriteLog("BrakePos=" + AnsiString(mvOccupied->BrakeCtrlPos) + ", MainCtrl=" +
                AnsiString(mvControlling->MainCtrlPos));
#endif
}

// ensure relays are active
void TController::control_relays() {

    if( fActionTime < 0.0 ) { return; }

    if( IsAnyMotorOverloadRelayOpen ) {
        cue_action( driver_hint::mastercontrollersetzerospeed );
        cue_action( driver_hint::tractionnmotoroverloadreset );
        ++iDriverFailCount;
    }
    if( IsAnyGroundRelayOpen ) {
        cue_action( driver_hint::mastercontrollersetzerospeed );
        cue_action( driver_hint::maincircuitgroundreset );
        ++iDriverFailCount;
    }
}

void TController::control_motor_connectors() {
    // ensure motor connectors are active
    if( ( mvControlling->EngineType == TEngineType::ElectricSeriesMotor )
     || ( mvControlling->EngineType == TEngineType::DieselElectric )
     || ( is_emu() ) ) {
        if( Need_TryAgain ) {
            // true, jeśli druga pozycja w elektryku nie załapała
            cue_action( driver_hint::mastercontrollersetzerospeed );
            Need_TryAgain = false;
        }
    }
}

void TController::control_tractive_force() {

    auto const velocity { DirectionalVel() };
    // jeśli przyspieszenie pojazdu jest mniejsze niż żądane oraz...
    if( ( AccDesired > EU07_AI_NOACCELERATION ) // don't add power if not asked for actual speed-up
     && (( AbsAccS < AccDesired /*- std::min( 0.05, 0.01 * iDriverFailCount )*/ ) || (mvOccupied->SpeedCtrlUnit.IsActive && velocity < mvOccupied->SpeedCtrlUnit.FullPowerVelocity))
     && ( false == TestFlag( iDrivigFlags, movePress ) ) ) {
        // ...jeśli prędkość w kierunku czoła jest mniejsza od dozwolonej o margines...
        if( velocity < (
            VelDesired == 1.0 ? // work around for trains getting stuck on tracks with speed limit = 1
                VelDesired :
                VelDesired - fVelMinus ) ) {
            // within proximity margin don't exceed next scheduled speed, outside of the margin accelerate as you like
            if( ( ActualProximityDist > (
                is_car() ?
                    fMinProximityDist : // cars are allowed to move within min proximity distance
                    fMaxProximityDist ) ? // other vehicle types keep wider margin
                        true :
                        ( velocity + 1.0 ) < VelNext ) ) {
                // ...to można przyspieszyć
                increase_tractive_force();
            }
        }
    }
    // zmniejszanie predkosci
    // margines dla prędkości jest doliczany tylko jeśli oczekiwana prędkość jest większa od 5km/h
    if( false == TestFlag( iDrivigFlags, movePress ) ) {
		double SpeedCtrlMargin = (mvControlling->SpeedCtrlUnit.IsActive && VelDesired > 5) ? 3 : 0;
        // jeśli nie dociskanie
        if( AccDesired <= EU07_AI_NOACCELERATION ) {
            cue_action( driver_hint::mastercontrollersetzerospeed );
        }
        else if( ( velocity > VelDesired + SpeedCtrlMargin)
              || ( fAccGravity < -0.01 ?
                        AccDesired < 0.0 :
                        ( AbsAccS > AccDesired + 10.05 ) )
               || ( IsAnyCouplerStretched ) ) {
                // jak za bardzo przyspiesza albo prędkość przekroczona
				// dodany wyjatek na "pelna w przod"
                cue_action( driver_hint::tractiveforcedecrease );
        }
    }

    control_handles();
    SpeedSet(); // ciągla regulacja prędkości
}

void TController::increase_tractive_force() {

    if( fActionTime < 0.0 )     { return; } // gdy jest nakaz poczekać z jazdą, to nie ruszać
    if( IsAnyCouplerStretched ) { return; } // train is already stretched past its limits, don't pull even harder

    if( mvOccupied->SpringBrake.Activate ) {
        cue_action( driver_hint::springbrakeoff );
    }
    if( ( VelDesired > 0.0 ) // to prevent door shuffle on stop
     && ( doors_open() || doors_permit_active() ) ) {
        // zamykanie drzwi - tutaj wykonuje tylko AI (zmienia fActionTime)
        Doors( false );
        // Doors() call can potentially adjust fActionTime
        if( fActionTime < 0.0 ) { return; }
    }
    if( true == mvOccupied->DepartureSignal ) {
        cue_action( driver_hint::departuresignaloff );
    }

    cue_action( driver_hint::tractiveforceincrease );
}

void TController::control_braking_force() {

    // don't touch the brakes when compressing buffers during coupling/uncoupling
    if( TestFlag( iDrivigFlags, movePress ) ) { return; }

    auto const velocity { DirectionalVel() };

    // jeśli przyspieszamy, to nie hamujemy
    if( AccDesired > 0.0 ) {
        if( ( OrderCurrentGet() != Disconnect ) // przy odłączaniu nie zwalniamy tu hamulca
         && ( false == TestFlag( iDrivigFlags, movePress ) ) ) {
            cue_action( driver_hint::brakingforcesetzero );
        }
    }

    if( is_emu() && ( !ForcePNBrake ) ) {
        // właściwie, to warunek powinien być na działający EP
        // Ra: to dobrze hamuje EP w EZT
        auto const accthreshold { (
            fAccGravity < 0.025 ? // HACK: when going downhill be more responsive to desired deceleration
                fAccThreshold :
                std::max( -0.2, fAccThreshold ) ) };
		auto const AccMax{ std::min(fBrake_a0[0] + 12 * fBrake_a1[0], mvOccupied->MED_amax) };
		auto const accmargin = ((AccMax > 1.1 * AccDesired) && (fAccGravity < 0.025)) ?
							0.05 : 0.0;
        if( ( AccDesired < accthreshold ) // jeśli hamować - u góry ustawia się hamowanie na fAccThreshold
         && ( ( AbsAccS > AccDesired + accmargin)
           || ( BrakeCtrlPosition < 0 ) ) ) {
            // hamować bardziej, gdy aktualne opóźnienie hamowania mniejsze niż (AccDesired)
            cue_action( driver_hint::brakingforceincrease );
        }
        else if( OrderCurrentGet() != Disconnect ) { // przy odłączaniu nie zwalniamy tu hamulca
            if( AbsAccS < AccDesired - 0.05 ) {
                // jeśli opóźnienie większe od wymaganego (z histerezą) luzowanie, gdy za dużo
                // TBD: check if the condition isn't redundant with the DecBrake() code
                if( BrakeCtrlPosition >= 0 ) {
                    if( VelDesired > 0.0 ) { // sanity check to prevent unintended brake release on sharp slopes
                        cue_action( driver_hint::brakingforcedecrease );
                    }
                }
            }
            else {
                cue_action( driver_hint::brakingforcelap );
            }
        }
    } // type & dt_ezt
    else {
        // a stara wersja w miarę dobrze działa na składy wagonowe
        if( ( ( AccDesired < fAccGravity - 0.1 ) && ( AbsAccS > AccDesired + fBrake_a1[ 0 ] ) ) // regular braking
         || ( ( fAccGravity < -0.05 ) && ( velocity < -0.1 ) ) ) { // also brake if uphill and slipping back
            // u góry ustawia się hamowanie na fAccThreshold
            if( ( fBrakeTime < 0.0 )
             || ( AccDesired < fAccGravity - 0.5 )
             || ( BrakeCtrlPosition <= 0 ) ) {
                // jeśli upłynął czas reakcji hamulca, chyba że nagłe albo luzował
                // TODO: check whether brake delay variable still has any purpose
                cue_action(
                    driver_hint::brakingforceincrease,
                    // Ra: ten czas należy zmniejszyć, jeśli czas dojazdu do zatrzymania jest mniejszy
                    ( 3.0
                    + 0.5 * ( (
                        mvOccupied->BrakeDelayFlag > bdelay_G ?
                            mvOccupied->BrakeDelay[ 1 ] :
                            mvOccupied->BrakeDelay[ 3 ] )
                        - 3.0 ) )
                    * 0.5 ); // Ra: tymczasowo, bo przeżyna S1
            }
        }
        if ( ( AccDesired < fAccGravity - 0.05 )
        && ( ( AccDesired - fBrake_a1[ 0 ] * 0.51 ) ) - AbsAccS > 0.05 ) {
            // jak hamuje, to nie tykaj kranu za często
            // yB: luzuje hamulec dopiero przy różnicy opóźnień rzędu 0.2
            if( OrderCurrentGet() != Disconnect ) { // przy odłączaniu nie zwalniamy tu hamulca
                if( VelDesired > 0.0 ) { // sanity check to prevent unintended brake release on sharp slopes
                    // TODO: check whether brake delay variable still has any purpose
                    cue_action(
                        driver_hint::brakingforcedecrease,
                        ( ( mvOccupied->BrakeDelayFlag > bdelay_G ?
                            mvOccupied->BrakeDelay[ 0 ] :
                            mvOccupied->BrakeDelay[ 2 ] )
                          / 3.0 )
                        * 0.5 ); // Ra: tymczasowo, bo przeżyna S1
                }
            }
        }
        // stop-gap measure to ensure cars actually brake to stop even when above calculactions go awry
        // instead of releasing the brakes and creeping into obstacle at 1-2 km/h
        if( is_car() ) {
            if( ( VelDesired == 0.0 )
             && ( velocity > VelDesired )
             && ( ActualProximityDist <= fMinProximityDist )
             && ( mvOccupied->LocalBrakePosA < 0.01 ) ) {
                cue_action( driver_hint::brakingforceincrease );
            }
        }
    }

    // odhamowywanie składu po zatrzymaniu i zabezpieczanie lokomotywy
    if( ( mvOccupied->Vel < EU07_AI_NOMOVEMENT )
     && ( ( VelDesired == 0.0 )
       || ( AccDesired <= EU07_AI_NOACCELERATION ) ) ) {

        if( ( ( OrderCurrentGet() & ( Disconnect | Connect | Change_direction ) ) == 0 ) // przy (p)odłączaniu nie zwalniamy tu hamulca
         && ( std::abs( fAccGravity ) < 0.01 ) ) { // only do this on flats, on slopes keep applied the train brake
            apply_independent_brake_only();
        }
        // if told to change direction don't confuse human driver with request to leave applied brake in the cab they're about to leave
        if( ( OrderCurrentGet() & ( Change_direction ) ) != 0 ) {
            cue_action( driver_hint::independentbrakerelease );
        }
    }
}

void TController::apply_independent_brake_only() {

    if( mvOccupied->LocalBrake != TLocalBrake::ManualBrake ) {
        if( ( false == mvOccupied->ShuntMode ) // no need for independent brake if autobrake is active
         && ( is_equal( mvOccupied->fBrakeCtrlPos, mvOccupied->Handle->GetPos( bh_RP ), 0.2 ) ) ) {
            cue_action( driver_hint::independentbrakeapply );
        }
        else {
            cue_action( driver_hint::trainbrakerelease );
        }
    }
}

void TController::control_releaser() {
    // HACK: don't immediately touch releaser in inert vehicle, in case the simulation is about to replace us with human driver
    if( ( mvOccupied->Vel < 1.0 ) && ( fActionTime < 0.0 ) ) { return; }
    // TODO: combine all releaser handling in single decision tree instead of having bits all over the place
    // TODO: replace handle and system conditions with flag indicating releaser presence
    if( BrakeSystem != TBrakeSystem::Pneumatic ) { return; }

    auto const hasreleaser {
        ( false == ( is_dmu() || is_emu() ) ) // TODO: a more refined test than rejecting these types wholesale
     && ( ( mvOccupied->BrakeHandle == TBrakeHandle::FV4a )
       || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_6P )
       || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_K5P )
       || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_K8P )
       || ( mvOccupied->BrakeHandle == TBrakeHandle::M394 ) ) };

    if( false == hasreleaser ) { return; }

    auto actuate { false };
    auto isbrakehandleinrightposition { true };

    if( AccDesired > EU07_AI_NOACCELERATION ) {
        // fill up brake pipe if needed
        if( mvOccupied->PipePress < 3.0 ) {
            actuate = true;
            // some vehicles require brake handle to be moved to specific position
            if( mvOccupied->HandleUnlock != -3 ) {
                cue_action( driver_hint::trainbrakesetpipeunlock );
                isbrakehandleinrightposition &= ( BrakeCtrlPosition == mvOccupied->HandleUnlock );
            }
        }
        // wyluzuj lokomotywę, to szybciej ruszymy
        if( ( mvOccupied->BrakePress > 0.4 )
         && ( mvOccupied->Hamulec->GetCRP() > 4.9 ) ) {
            actuate = true;
        }
        // keep engine brakes released during coupling/uncoupling
        if( ( true == TestFlag( iDrivigFlags, movePress ) )
         && ( mvOccupied->BrakePress > 0.1 ) ) {
            actuate = true;
        }
    }
    // don't overcharge train brake pipe
    if( mvOccupied->PipePress > 5.2 ) {
        actuate = false;
    }

    if( actuate ) {
        // some vehicles may require master controller in neutral position
        cue_action( driver_hint::mastercontrollersetzerospeed );
        if( ( false == mvOccupied->Hamulec->Releaser() )
         && ( isbrakehandleinrightposition ) ) {
            cue_action( driver_hint::releaseron );
        }
    }
    else {
        if( true == mvOccupied->Hamulec->Releaser() ) {
            cue_action( driver_hint::releaseroff );
        }
    }
}

void TController::control_main_pipe() {

    // unlocking main pipe
	if( ( AccDesired > -0.03 )
     && ( true == mvOccupied->LockPipe ) ) {
		UniversalBrakeButtons |= TUniversalBrake::ub_UnlockPipe;
	}
	else if (false == mvOccupied->LockPipe ) {
		UniversalBrakeButtons &= ~TUniversalBrake::ub_UnlockPipe;
	}

    // napełnianie uderzeniowe
    if( BrakeSystem != TBrakeSystem::Pneumatic ) { return; }

    if( ( mvOccupied->BrakeHandle == TBrakeHandle::FV4a )
     || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_6P )
     || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_K5P )
     || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_K8P )
     || ( mvOccupied->BrakeHandle == TBrakeHandle::M394 ) ) {

        if( ( iDrivigFlags & moveOerlikons )
         || ( true == IsCargoTrain ) ) {

            if( ( BrakeCtrlPosition == gbh_RP )
             && ( AbsAccS < 0.03 )
             && ( AccDesired > -0.03 )
             && ( VelDesired - mvOccupied->Vel > 2.0 ) ) {

                if( ( fReady > 0.35 )
                 && ( mvOccupied->EqvtPipePress < 4.5 ) // don't charge with a risk of overcharging the main pipe
                 && ( mvOccupied->Compressor >= 7.5 ) // don't charge without sufficient pressure in the tank
                 && ( BrakeChargingCooldown >= 0.0 ) // don't charge while cooldown is active
                 && ( ( ActualProximityDist > 100.0 ) // don't charge if we're about to be braking soon
                   || ( min_speed( mvOccupied->Vel, VelNext ) >= mvOccupied->Vel ) ) ) {

                    BrakeLevelSet( gbh_FS );
                    // don't charge the brakes too often, or we risk overcharging
                    BrakeChargingCooldown = -1 * clamp( iVehicles * 3, 30, 90 );
                }
            }
        }

        if( ( mvOccupied->Compressor < 5.0 )
         || ( ( BrakeCtrlPosition < gbh_RP )
           && ( mvOccupied->EqvtPipePress > ( fReady < 0.25 ? 5.1 : 5.2 ) ) ) ) {
            cue_action( driver_hint::trainbrakerelease );
        }
    }
}

void
TController::check_route_ahead( double const Range ) {

    auto const comm { TableUpdate( VelDesired, ActualProximityDist, VelNext, AccDesired ) };

    switch (comm) {
    // ustawienie VelSignal - trochę proteza = do przemyślenia
    case TCommandType::cm_Ready: { // W4 zezwolił na jazdę
        // ewentualne doskanowanie trasy za W4, który zezwolił na jazdę
        TableCheck( Range );
        TableUpdate( VelDesired, ActualProximityDist, VelNext, AccDesired ); // aktualizacja po skanowaniu
        if( VelNext == 0.0 ) {
            break; // ale jak coś z przodu zamyka, to ma stać
        }
        if( iDrivigFlags & moveStopCloser ) {
            VelSignal = -1.0; // ma czekać na sygnał z sygnalizatora!
        }
        break;
    }
    case TCommandType::cm_SetVelocity: { // od wersji 357 semafor nie budzi wyłączonej lokomotywy
        if( ( OrderCurrentGet() & ~( Shunt | Loose_shunt | Obey_train | Bank ) ) == 0 ) { // jedzie w dowolnym trybie albo Wait_for_orders
            if( std::fabs( VelSignal ) >= 1.0 ) { // 0.1 nie wysyła się do samochodow, bo potem nie ruszą
                PutCommand( "SetVelocity", VelSignal, VelNext, nullptr ); // komenda robi dodatkowe operacje
            }
        }
        break;
    }
    case TCommandType::cm_ShuntVelocity: { // od wersji 357 Tm nie budzi wyłączonej lokomotywy
        if( ( OrderCurrentGet() & ~( Shunt | Loose_shunt | Obey_train | Bank ) ) == 0 ) { // jedzie w dowolnym trybie albo Wait_for_orders
            PutCommand( "ShuntVelocity", VelSignal, VelNext, nullptr );
        }
        else if( iCoupler ) { // jeśli jedzie w celu połączenia
            SetVelocity( VelSignal, VelNext );
        }
        break;
    }
    case TCommandType::cm_Command: { // komenda z komórki
        if( ( OrderCurrentGet() & ~( Shunt | Loose_shunt | Obey_train | Bank ) ) == 0 ) {
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
    }
    default: {
        break;
    }
    }
}

void
TController::check_route_behind( double const Range ) {

    if( VelNext != 0.0 ) { return; } // not if we can move ahead
    if( TestFlag( iDrivigFlags, moveConnect ) ) { return; } // not if we're in middle of coupling operation

    if( ( OrderCurrentGet() & ~( Shunt | Loose_shunt | Connect ) ) == 0 ) {
        // jedzie w Shunt albo Connect, albo Wait_for_orders
        // w trybie Connect skanować do tyłu tylko jeśli przed kolejnym sygnałem nie ma taboru do podłączenia
        auto const couplinginprogress {
            ( TestFlag( OrderCurrentGet(), Connect ) )
         && ( ( Obstacle.distance < std::min( 2000.0, FirstSemaphorDist ) )
           || ( TestFlag( iDrivigFlags, moveConnect ) ) ) };
        if( couplinginprogress ) { return; }

        auto const commandbehind { BackwardScan( Range ) };
        if( commandbehind != TCommandType::cm_Unknown ) {
            // jeśli w drugą można jechać
            // należy sprawdzać odległość od znalezionego sygnalizatora,
            // aby w przypadku prędkości 0.1 wyciągnąć najpierw skład za sygnalizator
            // i dopiero wtedy zmienić kierunek jazdy, oczekując podania prędkości >0.5
            if( commandbehind == TCommandType::cm_Command ) {
                // jeśli komenda Shunt to ją odbierz bez przemieszczania się (np. odczep wagony po dopchnięciu do końca toru)
                iDrivigFlags |= moveStopHere;
            }
            iDirectionOrder = -iDirection; // zmiana kierunku jazdy
            // zmiana kierunku bez psucia kolejnych komend
            OrderList[ OrderPos ] = TOrders( OrderCurrentGet() | Change_direction );
        }
    }
}

void
TController::UpdateBrakingHelper() {

	if (( HelperState > 0 ) && (-AccDesired < fBrake_a0[0] + 2 * fBrake_a1[0]) && (mvOccupied->Vel > EU07_AI_NOMOVEMENT)) {
		HelperState = 0;
	}

    switch( HelperState ) {
        case 0: {
            if( ( -AccDesired > fBrake_a0[0] + 8 * fBrake_a1[0] ) ) {
                HelperState = 1;
            }
            break;
        }
        case 1: {
            if( ( -AccDesired > fBrake_a0[0] + 12 * fBrake_a1[0] ) ) {
                HelperState = 2;
            }
            break;
        }
        case 2: {
            if( ( -AccDesired > 0 ) && ( -ActualProximityDist > 5 ) ) {
                HelperState = 3;
            }
            break;
        }
        default: {
            break;
        }
    }
}
