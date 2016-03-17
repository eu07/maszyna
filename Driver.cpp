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

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Driver.h"
#include <mtable.hpp>
#include "DynObj.h"
#include <math.h>
#include "Globals.h"
#include "Event.h"
#include "Ground.h"
#include "MemCell.h"
#include "World.h"
#include "dir.h"

#define LOGVELOCITY 0
#define LOGORDERS 0
#define LOGSTOPS 1
#define LOGBACKSCAN 0
#define LOGPRESS 0
/*

Modu³ obs³uguj¹cy sterowanie pojazdami (sk³adami poci¹gów, samochodami).
Ma dzia³aæ zarówno jako AI oraz przy prowadzeniu przez cz³owieka. W tym
drugim przypadku jedynie informuje za pomoc¹ napisów o tym, co by zrobi³
w tym pierwszym. Obejmuje zarówno maszynistê jak i kierownika poci¹gu
(dawanie sygna³u do odjazdu).

Przeniesiona tutaj zosta³a zawartoœæ ai_driver.pas przerobiona na C++.
Równie¿ niektóre funkcje dotycz¹ce sk³adów z DynObj.cpp.

Teoria jest wtedy kiedy wszystko wiemy, ale nic nie dzia³a.
Praktyka jest wtedy, kiedy wszystko dzia³a, ale nikt nie wie dlaczego.
Tutaj ³¹czymy teoriê z praktyk¹ - tu nic nie dzia³a i nikt nie wie dlaczego…

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
// 16. zmiana kierunku //Ra: z przesiadk¹ po ukrotnieniu
// 17. otwieranie/zamykanie drzwi
// 18. Ra: odczepianie z zahamowaniem i podczepianie
// 19. dla Humandriver: tasma szybkosciomierza - zapis do pliku!

// do zrobienia:
// 1. kierownik pociagu
// 2. madrzejsze unikanie grzania oporow rozruchowych i silnika
// 3. unikanie szarpniec, zerwania pociagu itp
// 4. obsluga innych awarii
// 5. raportowanie problemow, usterek nie do rozwiazania
// 7. samouczacy sie algorytm hamowania

// sta³e
const double EasyReactionTime = 0.5; //[s] przeb³yski œwiadomoœci dla zwyk³ej jazdy
const double HardReactionTime = 0.2;
const double EasyAcceleration = 0.5; //[m/ss]
const double HardAcceleration = 0.9;
const double PrepareTime = 2.0; //[s] przeb³yski œwiadomoœci przy odpalaniu
bool WriteLogFlag = false;

AnsiString StopReasonTable[] = {
    // przyczyny zatrzymania ruchu AI
    "", // stopNone, //nie ma powodu - powinien jechaæ
    "Off", // stopSleep, //nie zosta³ odpalony, to nie pojedzie
    "Semaphore", // stopSem, //semafor zamkniêty
    "Time", // stopTime, //czekanie na godzinê odjazdu
    "End of track", // stopEnd, //brak dalszej czêœci toru
    "Change direction", // stopDir, //trzeba stan¹æ, by zmieniæ kierunek jazdy
    "Joining", // stopJoin, //zatrzymanie przy (p)odczepianiu
    "Block", // stopBlock, //przeszkoda na drodze ruchu
    "A command", // stopComm, //otrzymano tak¹ komendê (niewiadomego pochodzenia)
    "Out of station", // stopOut, //komenda wyjazdu poza stacjê (raczej nie powinna zatrzymywaæ!)
    "Radiostop", // stopRadio, //komunikat przekazany radiem (Radiostop)
    "External", // stopExt, //przes³any z zewn¹trz
    "Error", // stopError //z powodu b³êdu w obliczeniu drogi hamowania
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void TSpeedPos::Clear()
{
    iFlags = 0; // brak flag to brak reakcji
    fVelNext = -1.0; // prêdkoœæ bez ograniczeñ
	fSectionVelocityDist = 0.0; //brak d³ugoœci
    fDist = 0.0;
    vPos = vector3(0, 0, 0);
    trTrack = NULL; // brak wskaŸnika
};

void TSpeedPos::CommandCheck()
{ // sprawdzenie typu komendy w evencie i okreœlenie prêdkoœci
    TCommandType command = evEvent->Command();
    double value1 = evEvent->ValueGet(1);
    double value2 = evEvent->ValueGet(2);
    switch (command)
    {
    case cm_ShuntVelocity:
        // prêdkoœæ manewrow¹ zapisaæ, najwy¿ej AI zignoruje przy analizie tabelki
        fVelNext = value1; // powinno byæ value2, bo druga okreœla "za"?
        iFlags |= spShuntSemaphor;
        break;
    case cm_SetVelocity:
        // w semaforze typu "m" jest ShuntVelocity dla Ms2 i SetVelocity dla S1
        // SetVelocity * 0    -> mo¿na jechaæ, ale stan¹æ przed
        // SetVelocity 0 20   -> stan¹æ przed, potem mo¿na jechaæ 20 (SBL)
        // SetVelocity -1 100 -> mo¿na jechaæ, przy nastêpnym ograniczenie (SBL)
        // SetVelocity 40 -1  -> PutValues: jechaæ 40 a¿ do miniêcia (koniec ograniczenia(
        fVelNext = value1;
        iFlags &= ~(spShuntSemaphor | spPassengerStopPoint | spStopOnSBL);
        iFlags |= spSemaphor;// nie manewrowa, nie przystanek, nie zatrzymaæ na SBL, ale semafor
        if (value1 == 0.0) // jeœli pierwsza zerowa
            if (value2 != 0.0) // a druga nie
            { // S1 na SBL, mo¿na przejechaæ po zatrzymaniu (tu nie mamy prêdkoœci ani odleg³oœci)
                fVelNext = value2; // normalnie bêdzie zezwolenie na jazdê, aby siê usun¹³ z tabelki
                iFlags |= spStopOnSBL; // flaga, ¿e ma zatrzymaæ; na pewno nie zezwoli na manewry
            }
        break;
    case cm_SectionVelocity:
        // odcinek z ograniczeniem prêdkoœci
        fVelNext = value1;
		fSectionVelocityDist = value2;
        iFlags |= spSectionVel;
        break;
    case cm_RoadVelocity:
        // prêdkoœæ drogowa (od tej pory bêdzie jako domyœlna najwy¿sza)
        fVelNext = value1;
        iFlags |= spRoadVel;
        break;
    case cm_PassengerStopPoint:
        // nie ma dostêpu do rozk³adu
        // przystanek, najwy¿ej AI zignoruje przy analizie tabelki
        if ((iFlags & spPassengerStopPoint) == 0)
            fVelNext = 0.0; // TrainParams->IsStop()?0.0:-1.0; //na razie tak
        iFlags |= spPassengerStopPoint; // niestety nie da siê w tym miejscu wspó³pracowaæ z rozk³adem
        break;
    case cm_SetProximityVelocity:
        // musi zostaæ gdy¿ inaczej nie dzia³aj¹ manewry
		fVelNext = -1;
        iFlags |= spProximityVelocity;
		// fSectionVelocityDist = value2;
        break;
    case cm_OutsideStation:
        // w trybie manewrowym: skanowaæ od niej wstecz i stan¹æ po wyjechaniu za sygnalizator i
        // zmieniæ kierunek
        // w trybie poci¹gowym: mo¿na przyspieszyæ do wskazanej prêdkoœci (po zjechaniu z rozjazdów)
        fVelNext = -1;
        iFlags |= spOutsideStation; // W5
        break;
	default:
        // inna komenda w evencie skanowanym powoduje zatrzymanie i wys³anie tej komendy
        iFlags &= ~(spShuntSemaphor | spPassengerStopPoint |
                    spStopOnSBL); // nie manewrowa, nie przystanek, nie zatrzymaæ na SBL
        fVelNext = 0.0; // jak nieznana komenda w komórce sygna³owej, to zatrzymujemy
    }
};

bool TSpeedPos::Update(vector3 *p, vector3 *dir, double &len)
{ // przeliczenie odleg³oœci od punktu (*p), w kierunku (*dir), zaczynaj¹c od pojazdu
    // dla kolejnych pozycji podawane s¹ wspó³rzêdne poprzedniego obiektu w (*p)
    vector3 v = vPos - *p; // wektor od poprzedniego obiektu (albo pojazdu) do punktu zmiany
    fDist =
        v.Length(); // d³ugoœæ wektora to odleg³oœæ pomiêdzy czo³em a sygna³em albo pocz¹tkiem toru
    // v.SafeNormalize(); //normalizacja w celu okreœlenia znaku (nie potrzebna?)
    if (len == 0.0)
    { // je¿eli liczymy wzglêdem pojazdu
        double iska = dir ? dir->x * v.x + dir->z * v.z :
                            fDist; // iloczyn skalarny to rzut na chwilow¹ prost¹ ruchu
        if (iska < 0.0) // iloczyn skalarny jest ujemny, gdy punkt jest z ty³u
        { // jeœli coœ jest z ty³u, to dok³adna odleg³oœæ nie ma ju¿ wiêkszego znaczenia
            fDist = -fDist; // potrzebne do badania wyjechania sk³adem poza ograniczenie
            if (iFlags & spElapsed) // 32 ustawione, gdy obiekt ju¿ zosta³ miniêty
            { // jeœli miniêty (musi byæ miniêty równie¿ przez koñcówkê sk³adu)
            }
            else
            {
                iFlags ^= spElapsed; // 32-miniêty - bêdziemy liczyæ odleg³oœæ wzglêdem przeciwnego koñca
                // toru (nadal mo¿e byæ z przodu i ograniczaæ)
                if ((iFlags & 0x43) == 3) // tylko jeœli (istotny) tor, bo eventy s¹ punktowe
                    if (trTrack) // mo¿e byæ NULL, jeœli koniec toru (????)
                        vPos =
                            (iFlags & spReverse) ?
                                trTrack->CurrentSegment()->FastGetPoint_0() :
                                trTrack->CurrentSegment()->FastGetPoint_1(); // drugi koniec istotny
            }
        }
        else if (fDist < 50.0) // przy du¿ym k¹cie ³uku iloczyn skalarny bardziej zani¿y odleg³oœæ
            // ni¿ ciêciwa
            fDist = iska; // ale przy ma³ych odleg³oœciach rzut na chwilow¹ prost¹ ruchu da
        // dok³adniejsze wartoœci
    }
    if (fDist > 0.0) // nie mo¿e byæ 0.0, a przypadkiem mog³o by siê trafiæ i by³o by Ÿle
        if ((iFlags & spElapsed) == 0) // 32 ustawione, gdy obiekt ju¿ zosta³ miniêty
        { // jeœli obiekt nie zosta³ miniêty, mo¿na od niego zliczaæ narastaj¹co (inaczej mo¿e byæ
            // problem z wektorem kierunku)
            len = fDist = len + fDist; // zliczanie dlugoœci narastaj¹co
            *p = vPos; // nowy punkt odniesienia
            *dir = Normalize(v); // nowy wektor kierunku od poprzedniego obiektu do aktualnego
        }
    if (iFlags & spTrack) // jeœli tor
    {
        if (trTrack) // mo¿e byæ NULL, jeœli koniec toru (???)
        {
            fVelNext = trTrack->VelocityGet(); // aktualizacja prêdkoœci (mo¿e byæ zmieniana
            // eventem)
            int i;
            if ((i = iFlags & 0xF0000000) != 0)
            { // jeœli skrzy¿owanie, ograniczyæ prêdkoœæ przy skrêcaniu
                if (abs(i) > 0x10000000) //±1 to jazda na wprost, ±2 nieby te¿, ale z przeciêciem
                    // g³ównej drogi - chyba ¿e jest równorzêdne...
                    fVelNext = 30.0; // uzale¿niæ prêdkoœæ od promienia; albo niech bêdzie
                // ograniczona w skrzy¿owaniu (velocity z ujemn¹ wartoœci¹)
                if ((iFlags & spElapsed) == 0) // jeœli nie wjecha³
					if (trTrack->iNumDynamics > 0) // a skrzy¿owanie zawiera pojazd
					{
						WriteLog("Tor " + trTrack->NameGet() + " zajety przed pojazdem. Num=" + trTrack->iNumDynamics + "Dist= " + fDist);
						fVelNext =
							0.0; // to zabroniæ wjazdu (chyba ¿e ten z przodu te¿ jedzie prosto)
					}
            }
            if (iFlags & spSwitch) // jeœli odcinek zmienny
            {
                if (bool(trTrack->GetSwitchState() & 1) !=
                    bool(iFlags & spSwitchStatus)) // czy stan siê zmieni³?
                { // Ra: zak³adam, ¿e s¹ tylko 2 mo¿liwe stany
                    iFlags ^= spSwitchStatus;
                    // fVelNext=trTrack->VelocityGet(); //nowa prêdkoœæ
                    if ((iFlags & spElapsed) == 0)
                        return true; // jeszcze trzeba skanowanie wykonaæ od tego toru
                    // problem jest chyba, jeœli zwrotnica siê prze³o¿y zaraz po zjechaniu z niej
                    // na Mydelniczce potrafi skanowaæ na wprost mimo pojechania na bok
                }
                // poni¿sze nie dotyczy trybu ³¹czenia?
				if ((iFlags & spElapsed) ? false :
					trTrack->iNumDynamics >
					0) // jeœli jeszcze nie wjechano na tor, a coœ na nim jest
				{
					WriteLog("Rozjazd " + trTrack->NameGet() + " zajety przed pojazdem. Num=" + trTrack->iNumDynamics + "Dist= "+fDist);
					//fDist -= 30.0;
					fVelNext = 0.0; // to niech stanie w zwiêkszonej odleg³oœci
				// else if (fVelNext==0.0) //jeœli zosta³a wyzerowana
				// fVelNext=trTrack->VelocityGet(); //odczyt prêdkoœci
				}
            }
        }
    }
    else if (iFlags & spEvent) // jeœli event
    { // odczyt komórki pamiêci najlepiej by by³o zrobiæ jako notyfikacjê, czyli zmiana komórki
        // wywo³a jak¹œ podan¹ funkcjê
        CommandCheck(); // sprawdzenie typu komendy w evencie i okreœlenie prêdkoœci
    }
    return false;
};

AnsiString TSpeedPos::GetName()
{
	if (iFlags & spTrack) // jeœli tor
		return trTrack->NameGet();
	else if (iFlags & spEvent) // jeœli event
		return evEvent->asName;
}

AnsiString TSpeedPos::TableText()
{ // pozycja tabelki prêdkoœci
    if (iFlags & spEnabled)
    { // o ile pozycja istotna
		return "Flags=#" + IntToHex(iFlags, 8) + ", Dist=" + FloatToStrF(fDist, ffFixed, 7, 1) +
			", Vel=" + AnsiString(fVelNext) + ", Name=" + GetName();
        //if (iFlags & spTrack) // jeœli tor
        //    return "Flags=#" + IntToHex(iFlags, 8) + ", Dist=" + FloatToStrF(fDist, ffFixed, 7, 1) +
        //           ", Vel=" + AnsiString(fVelNext) + ", Track=" + trTrack->NameGet();
        //else if (iFlags & spEvent) // jeœli event
        //    return "Flags=#" + IntToHex(iFlags, 8) + ", Dist=" + FloatToStrF(fDist, ffFixed, 7, 1) +
        //           ", Vel=" + AnsiString(fVelNext) + ", Event=" + evEvent->asName;
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
	return false; // true gdy zatrzymanie, wtedy nie ma po co skanowaæ dalej
}

bool TSpeedPos::Set(TEvent *event, double dist, TOrders order)
{ // zapamiêtanie zdarzenia
    fDist = dist;
    iFlags = spEnabled | spEvent; // event+istotny
    evEvent = event;
    vPos = event->PositionGet(); // wspó³rzêdne eventu albo komórki pamiêci (zrzutowaæ na tor?)
    CommandCheck(); // sprawdzenie typu komendy w evencie i okreœlenie prêdkoœci
	// zale¿nie od trybu sprawdzenie czy jest tutaj gdzieœ semafor lub tarcza manewrowa
	// jeœli wskazuje stop wtedy wystawiamy true jako koniec sprawdzania
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
    return false; // true gdy zatrzymanie, wtedy nie ma po co skanowaæ dalej
};

void TSpeedPos::Set(TTrack *track, double dist, int flag)
{ // zapamiêtanie zmiany prêdkoœci w torze
    fDist = dist; // odleg³oœæ do pocz¹tku toru
    trTrack = track; // TODO: (t) mo¿e byæ NULL i nie odczytamy koñca poprzedniego :/
    if (trTrack)
    {
        iFlags = flag | (trTrack->eType == tt_Normal ? 2 : 10); // zapamiêtanie kierunku wraz z typem
        if (iFlags & spSwitch)
            if (trTrack->GetSwitchState() & 1)
                iFlags |= spSwitchStatus;
        fVelNext = trTrack->VelocityGet();
        if (trTrack->iDamageFlag & 128)
            fVelNext = 0.0; // jeœli uszkodzony, to te¿ stój
        if (iFlags & spEnd)
            fVelNext = (trTrack->iCategoryFlag & 1) ?
                           0.0 :
                           20.0; // jeœli koniec, to poci¹g stój, a samochód zwolnij
        vPos = (bool(iFlags & spReverse) != bool(iFlags & spEnd)) ?
                   trTrack->CurrentSegment()->FastGetPoint_1() :
                   trTrack->CurrentSegment()->FastGetPoint_0();
    }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void TController::TableClear()
{ // wyczyszczenie tablicy
    iFirst = iLast = 0;
    iTableDirection = 0; // nieznany
    for (int i = 0; i < iSpeedTableSize; ++i) // czyszczenie tabeli prêdkoœci
        sSpeedTable[i].Clear();
    tLast = NULL;
    fLastVel = -1;
    eSignSkip = NULL; // nic nie pomijamy
};

TEvent *__fastcall TController::CheckTrackEvent(double fDirection, TTrack *Track)
{ // sprawdzanie eventów na podanym torze do podstawowego skanowania
    TEvent *e = (fDirection > 0) ? Track->evEvent2 : Track->evEvent1;
    if (!e)
        return NULL;
    if (e->bEnabled)
        return NULL;
    // jednak wszystkie W4 do tabelki, bo jej czyszczenie na przystanku wprowadza zamieszanie
    return e;
}

bool TController::TableAddNew()
{ // zwiêkszenie u¿ytej tabelki o jeden rekord
    iLast = (iLast + 1) % iSpeedTableSize;
    // TODO: jeszcze sprawdziæ, czy siê na iFirst nie na³o¿y
    // TODO: wstawiæ tu wywo³anie odtykacza - teraz jest to w TableTraceRoute()
    // TODO: jeœli ostatnia pozycja zajêta, ustawiaæ dodatkowe flagi - teraz jest to w
    // TableTraceRoute()
    // TODO: przyda³o by siê te¿ posortowaæ tabelkê wg odleg³oœci (ale nie w tym miejscu)
    return true; // false gdy siê na³o¿y
};

bool TController::TableNotFound(TEvent *e)
{ // sprawdzenie, czy nie zosta³ ju¿ dodany do tabelki (np. podwójne W4 robi problemy)
    int j = (iLast + 1) % iSpeedTableSize; // j, aby sprawdziæ te¿ ostatni¹ pozycjê
    for (int i = iFirst; i != j; i = (i + 1) % iSpeedTableSize)
        if ((sSpeedTable[i].iFlags & (spEnabled | spEvent)) == (spEnabled |
            spEvent)) // o ile u¿ywana pozycja
			if (sSpeedTable[i].evEvent == e)
			{
				WriteLog("TableNotFound: Event already in SpeedTable: " + sSpeedTable[i].evEvent->asName);
				return false; // ju¿ jest, drugi raz dodawaæ nie ma po co
			}
    return true; // nie ma, czyli mo¿na dodaæ
};

void TController::TableTraceRoute(double fDistance, TDynamicObject *pVehicle)
{ // skanowanie trajektorii na odleg³oœæ (fDistance) od (pVehicle) w kierunku przodu sk³adu i
    // uzupe³nianie tabelki
	// WriteLog("Starting TableTraceRoute");
	if (!iDirection) // kierunek pojazdu z napêdem
    { // jeœli kierunek jazdy nie jest okreslony
        iTableDirection = 0; // czekamy na ustawienie kierunku
    }
    TTrack *pTrack; // zaczynamy od ostatniego analizowanego toru
    // double fDistChVel=-1; //odleg³oœæ do toru ze zmian¹ prêdkoœci
    double fTrackLength; // d³ugoœæ aktualnego toru (krótsza dla pierwszego)
    double fCurrentDistance; // aktualna przeskanowana d³ugoœæ
    TEvent *pEvent;
    double fLastDir; // kierunek na ostatnim torze
    if (iTableDirection != iDirection)
    { // jeœli zmiana kierunku, zaczynamy od toru ze wskazanym pojazdem
        pTrack = pVehicle->RaTrackGet(); // odcinek, na którym stoi
        fLastDir = pVehicle->DirectionGet() *
                   pVehicle->RaDirectionGet(); // ustalenie kierunku skanowania na torze
        fCurrentDistance = 0; // na razie nic nie przeskanowano
        fTrackLength = pVehicle->RaTranslationGet(); // pozycja na tym torze (odleg³oœæ od Point1)
        if (fLastDir > 0) // jeœli w kierunku Point2 toru
            fTrackLength =
                pTrack->Length() - fTrackLength; // przeskanowana zostanie odleg³oœæ do Point2
        fLastVel = pTrack->VelocityGet(); // aktualna prêdkoœæ
        iTableDirection =
            iDirection; // ustalenie w jakim kierunku jest wype³niana tabelka wzglêdem pojazdu
        iFirst = iLast = 0;
        tLast = NULL; //¿aden nie sprawdzony
    }
    else
    { // kontynuacja skanowania od ostatnio sprawdzonego toru (w ostatniej pozycji zawsze jest tor)
		// WriteLog("TableTraceRoute: check last track");
        if (sSpeedTable[iLast].iFlags & spEndOfTable) // zatkanie
        { // jeœli zape³ni³a siê tabelka
			if ((iLast + 1) % iSpeedTableSize == iFirst) // jeœli nadal jest zape³niona
			{
				TablePurger(); // nic siê nie da zrobiæ
				return;
			}
			if ((iLast + 2) % iSpeedTableSize == iFirst) // musi byæ jeszcze miejsce wolne na
				// ewentualny event, bo tor jeszcze nie
				// sprawdzony
			{
				TablePurger();
				return; // ju¿ lepiej, ale jeszcze nie tym razem
			}
            sSpeedTable[iLast].iFlags &= 0xBE; // kontynuowaæ próby doskanowania
        }
        // znaleziono semafor lub tarczê lub tor  z prêdkoœci¹ zero
        // trzeba sprawdziæ czy to nada³ semafor
		// WriteLog("TableTraceRoute: "+OwnerName()+" check semaphor... ");
        // if (sSemNext)
        // WriteLog(sSemNext->TableText());
        if (sSemNextStop &&
            sSemNextStop->fVelNext ==
                0.0) // jeœli jest nastêpny semafor to sprawdzamy czy to on nada³ zero
        {
			// WriteLog("TableTraceRoute: "+sSemNext->TableText());
			if ((OrderCurrentGet() & Obey_train) && (sSemNextStop->iFlags & spSemaphor))
                return;
            else if ((OrderCurrentGet() < 0x40) &&
                     (sSemNextStop->iFlags & (spSemaphor | spShuntSemaphor | spOutsideStation)))
                return;
        }
        pTrack = sSpeedTable[iLast].trTrack; // ostatnio sprawdzony tor
        if (!pTrack)
            return; // koniec toru, to nie ma co sprawdzaæ (nie ma prawa tak byæ)
        fLastDir = sSpeedTable[iLast].iFlags & spReverse ?
                       -1.0 :
                       1.0; // flaga ustawiona, gdy Point2 toru jest bli¿ej
        fCurrentDistance = sSpeedTable[iLast].fDist; // aktualna odleg³oœæ do jego Point1
        fTrackLength =
            sSpeedTable[iLast].iFlags & (spElapsed | spEnd) ? 0.0 : pTrack->Length(); // nie doliczaæ d³ugoœci gdy:
        // 32-miniêty pocz¹tek,
        // 64-jazda do koñca toru
    }
    if (fCurrentDistance < fDistance)
    { // jeœli w ogóle jest po co analizowaæ
        // WriteLog("TableTraceRoute: checking next tracks");
        --iLast; // jak coœ siê znajdzie, zostanie wpisane w tê pozycjê, któr¹ w³aœnie odczytano
        while (fCurrentDistance < fDistance)
        {
            if (pTrack != tLast) // ostatni zapisany w tabelce nie by³ jeszcze sprawdzony
            { // jeœli tor nie by³ jeszcze sprawdzany
                // if (pTrack)
                // WriteLog("TableTraceRoute: " + OwnerName() + " checking track " +
                //         pTrack->NameGet());
                if ((pEvent = CheckTrackEvent(fLastDir, pTrack)) !=
                    NULL) // jeœli jest semafor na tym torze
                { // trzeba sprawdziæ tabelkê, bo dodawanie drugi raz tego samego przystanku nie
                    // jest korzystne
                    if (TableNotFound(pEvent)) // jeœli nie ma
                        if (TableAddNew())
                        {
                            WriteLog("TableTraceRoute: new event found " + pEvent->asName + " by " +
                                     OwnerName());
                            if (sSpeedTable[iLast].Set(
                                    pEvent, fCurrentDistance,
									OrderCurrentGet())) // dodanie odczytu sygna³u
                            {
                                fDistance = fCurrentDistance; // jeœli sygna³ stop, to nie ma
                                // potrzeby dalej skanowaæ
                                sSemNextStop = &sSpeedTable[iLast];
								if (!sSemNext)
									sSemNext = &sSpeedTable[iLast];
                                WriteLog("Signal stop. Next Semaphor ", false);
                                if (sSemNextStop)
                                    WriteLog(sSemNextStop->GetName());
                                else
                                    WriteLog("none");
                            }
                            else
                            {
                                if (sSpeedTable[iLast].IsProperSemaphor(OrderCurrentGet()) &&
                                    sSemNext == NULL)
                                    sSemNext =
                                        &sSpeedTable[iLast]; // sprawdzamy czy pierwszy na drodze
                                WriteLog("Signal forward. Next Semaphor ", false);
                                if (sSemNext)
                                    WriteLog(sSemNext->GetName());
                                else
                                    WriteLog("none");
                            }
                        }
                } // event dodajemy najpierw, ¿eby móc sprawdziæ, czy tor zosta³ dodany po
                // odczytaniu prêdkoœci nastêpnego
                if ((pTrack->VelocityGet() == 0.0) // zatrzymanie
                    || (pTrack->iAction) // jeœli tor ma w³asnoœci istotne dla skanowania
                    || (pTrack->VelocityGet() != fLastVel)) // nastêpuje zmiana prêdkoœci
                { // odcinek dodajemy do tabelki, gdy jest istotny dla ruchu
                    if (TableAddNew())
                    { // teraz dodatkowo zapamiêtanie wybranego segmentu dla skrzy¿owania
                        sSpeedTable[iLast].Set(
                            pTrack, fCurrentDistance,
                            fLastDir < 0 ?
                                5 :
                                1); // dodanie odcinka do tabelki z flag¹ kierunku wejœcia
                        if (pTrack->eType == tt_Cross) // na skrzy¿owaniach trzeba wybraæ segment,
                        // po którym pojedzie pojazd
                        { // dopiero tutaj jest ustalany kierunek segmentu na skrzy¿owaniu
                            sSpeedTable[iLast].iFlags |=
                                (pTrack->CrossSegment((fLastDir < 0) ? tLast->iPrevDirection :
                                                                       tLast->iNextDirection,
                                                      iRouteWanted) &
                                 15)
                                << 28; // ostatnie 4 bity pola flag
                            sSpeedTable[iLast].iFlags &=
                                ~spReverse; // usuniêcie flagi kierunku, bo mo¿e byæ b³êdna
                            if (sSpeedTable[iLast].iFlags < 0)
                                sSpeedTable[iLast].iFlags |=
                                    spReverse; // ustawienie flagi kierunku na podstawie wybranego segmentu
                            if (int(fLastDir) * sSpeedTable[iLast].iFlags < 0)
                                fLastDir = -fLastDir;
                            if (AIControllFlag) // dla AI na razie losujemy kierunek na kolejnym
                                // skrzy¿owaniu
                                iRouteWanted = 1 + random(3);
                        }
                    }
                }
                else if ((pTrack->fRadius != 0.0) // odleg³oœæ na ³uku lepiej aproksymowaæ ciêciwami
                         || (tLast ? tLast->fRadius != 0.0 : false)) // koniec ³uku te¿ jest istotny
                { // albo dla liczenia odleg³oœci przy pomocy ciêciw - te usuwaæ po przejechaniu
                    if (TableAddNew())
                        sSpeedTable[iLast].Set(pTrack, fCurrentDistance,
                                               fLastDir < 0 ? 0x85 :
                                                              0x81); // dodanie odcinka do tabelki
					// 0x85 = spEnabled, spReverse, SpCurve
                }
            }
            fCurrentDistance +=
                fTrackLength; // doliczenie kolejnego odcinka do przeskanowanej d³ugoœci
            tLast = pTrack; // odhaczenie, ¿e sprawdzony
            // Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
            fLastVel = pTrack->VelocityGet(); // prêdkoœæ na poprzednio sprawdzonym odcinku
            pTrack = pTrack->Neightbour(
                (pTrack->eType == tt_Cross) ? (sSpeedTable[iLast].iFlags >> 28) : int(fLastDir),
                fLastDir); // mo¿e byæ NULL
            /*
               if (fLastDir>0)
               {//jeœli szukanie od Point1 w kierunku Point2
                pTrack=pTrack->CurrentNext(); //mo¿e byæ NULL
                if (pTrack) //jeœli dalej brakuje toru, to zostajemy na tym samym, z t¹ sam¹
               orientacj¹
                 if (tLast->iNextDirection)
                  fLastDir=-fLastDir; //mo¿na by zamiêtaæ i zmieniæ tylko jeœli jest pTrack
               }
               else //if (fDirection<0)
               {//jeœli szukanie od Point2 w kierunku Point1
                pTrack=pTrack->CurrentPrev(); //mo¿e byæ NULL
                if (pTrack) //jeœli dalej brakuje toru, to zostajemy na tym samym, z t¹ sam¹
               orientacj¹
                 if (!tLast->iPrevDirection)
                  fLastDir=-fLastDir;
               }
            */
            if (pTrack)
            { // jeœli kolejny istnieje
                if (tLast)
                    if (pTrack->VelocityGet() < 0 ? tLast->VelocityGet() > 0 :
                                                    pTrack->VelocityGet() > tLast->VelocityGet())
                    { // jeœli kolejny ma wiêksz¹ prêdkoœæ ni¿ poprzedni, to zapamiêtaæ poprzedni
                        // (do czasu wyjechania)
                        if ((sSpeedTable[iLast].iFlags & 3) == 3 ?
                                (sSpeedTable[iLast].trTrack != tLast) :
                                true) // jeœli nie by³ dodany do tabelki
                            if (TableAddNew())
                                sSpeedTable[iLast].Set(
                                    tLast, fCurrentDistance,
                                    (fLastDir > 0 ? pTrack->iPrevDirection :
                                                    pTrack->iNextDirection) ?
                                        1 :
                                        5); // zapisanie toru z ograniczeniem prêdkoœci
                    }
                if (((iLast + 3) % iSpeedTableSize == iFirst) ?
                        true :
                        ((iLast + 2) % iSpeedTableSize == iFirst)) // czy tabelka siê nie zatka?
                { // jest ryzyko nieznalezienia ograniczenia - ograniczyæ prêdkoœæ do pozwalaj¹cej
                    // na zatrzymanie na koñcu przeskanowanej drogi
                    TablePurger(); // usun¹æ pilnie zbêdne pozycje
                    if (((iLast + 3) % iSpeedTableSize == iFirst) ?
                            true :
                            ((iLast + 2) % iSpeedTableSize == iFirst)) // czy tabelka siê nie zatka?
                    { // jeœli odtykacz nie pomóg³ (TODO: zwiêkszyæ rozmiar tabelki)
                        if (TableAddNew())
                            sSpeedTable[iLast].Set(
                                pTrack, fCurrentDistance,
                                fLastDir < 0 ?
                                    0x10045 :
                                    0x10041); // zapisanie toru jako koñcowego (ogranicza prêdkosæ)
                        // zapisaæ w logu, ¿e nale¿y poprawiæ sceneriê?
                        return; // nie skanujemy dalej, bo nie ma miejsca
                    }
                }
                fTrackLength = pTrack->Length(); // zwiêkszenie skanowanej odleg³oœci tylko jeœli
                // istnieje dalszy tor
            }
            else
            { // definitywny koniec skanowania, chyba ¿e dalej puszczamy samochód po gruncie...
                if (TableAddNew()) // kolejny, bo siê cofnêliœmy o 1
                    sSpeedTable[iLast].Set(
                        tLast, fCurrentDistance,
                        fLastDir < 0 ? 0x45 : 0x41); // zapisanie ostatniego sprawdzonego toru
                return; // to ostatnia pozycja, bo NULL nic nie da, a mo¿e siê podpi¹æ obrotnica,
                // czy jakieœ transportery
            }
        }
        if (TableAddNew())
            sSpeedTable[iLast].Set(pTrack, fCurrentDistance,
                                   fLastDir < 0 ? 4 : 0); // zapisanie ostatniego sprawdzonego toru
    }
};

void TController::TableCheck(double fDistance)
{ // przeliczenie odleg³oœci w tabelce, ewentualnie doskanowanie (bez analizy prêdkoœci itp.)
    if (iTableDirection != iDirection)
        TableTraceRoute(fDistance,
                        pVehicles[1]); // jak zmiana kierunku, to skanujemy od koñca sk³adu
    else if (iTableDirection)
    { // trzeba sprawdziæ, czy coœ siê zmieni³o
        vector3 dir =
            pVehicles[0]->VectorFront() * pVehicles[0]->DirectionGet(); // wektor kierunku jazdy
        vector3 pos = pVehicles[0]->HeadPosition(); // zaczynamy od pozycji pojazdu
        // double lastspeed=-1; //prêdkoœæ na torze do usuniêcia
        double len = 0.0; // odleg³oœæ bêdziemy zliczaæ narastaj¹co
        for (int i = iFirst; i != iLast; i = (i + 1) % iSpeedTableSize)
        { // aktualizacja rekordów z wyj¹tkiem ostatniego
            if (sSpeedTable[i].iFlags & spEnabled) // jeœli pozycja istotna
            {
                if (sSpeedTable[i].Update(&pos, &dir, len))
                {
                    WriteLog("TableCheck: Switch change. Delete next entries. (" +
                             sSpeedTable[i].trTrack->NameGet() + ")");
                    int k = (iLast + 1) % iSpeedTableSize; // skanujemy razem z ostatni¹ pozycj¹
                                        for (int j = (i+1) % iSpeedTableSize; j != k; j = (j + 1) % iSpeedTableSize)
					{ // kasowanie wszystkich rekordów za zmienion¹ zwrotnic¹
						WriteLog("TableCheck: Delete from table: " + sSpeedTable[j].GetName());
						sSpeedTable[j].iFlags = 0;
						if (&sSpeedTable[j] == sSemNext)
							sSemNext = NULL; // przy kasowaniu tabelki zrzucamy tak¿e semafor
						if (&sSpeedTable[j] == sSemNextStop)
							sSemNextStop = NULL; // przy kasowaniu tabelki zrzucamy tak¿e semafor
					}
					WriteLog("TableCheck: Delete entries OK.");
					WriteLog("TableCheck: New last element: " + sSpeedTable[i].GetName());
					iLast = i; // pokazujemy gdzie jest ostatni kawa³ek
					break; // nie kontynuujemy pêtli, trzeba doskanowaæ ci¹g dalszy
                }
                if (sSpeedTable[i].iFlags & spTrack) // jeœli odcinek
                {
                    if (sSpeedTable[i].fDist < -fLength) // a sk³ad wyjecha³ ca³¹ d³ugoœci¹ poza
                    { // degradacja pozycji
						// WriteLog( "TableCheck: Track is behind. Delete from table: " + sSpeedTable[i].trTrack->NameGet());
						sSpeedTable[i].iFlags &= ~spEnabled; // nie liczy siê
                    }
                    else if ((sSpeedTable[i].iFlags & 0xF0000028) ==
						spElapsed) // jest z ty³u (najechany) i nie jest zwrotnic¹ ani skrzy¿owaniem
						if (sSpeedTable[i].fVelNext < 0) // a nie ma ograniczenia prêdkoœci
						{
							sSpeedTable[i].iFlags =
								0; // to nie ma go po co trzymaæ (odtykacz usunie ze œrodka)
							// WriteLog("TableCheck: Track without speed. Delete from table: " + sSpeedTable[i].trTrack->NameGet());
						}
                }
                else if (sSpeedTable[i].iFlags & spEvent) // jeœli event
                {
                    if (sSpeedTable[i].fDist < (sSpeedTable[i].evEvent->Type == tp_PutValues ?
                                                    -fLength :
                                                    0)) // jeœli jest z ty³u
                        if ((mvOccupied->CategoryFlag & 1) ? false :
                                                             sSpeedTable[i].fDist < -fLength)
                        { // poci¹g staje zawsze, a samochód tylko jeœli nie przejedzie ca³¹
                            // d³ugoœci¹ (mo¿e byæ zaskoczony zmian¹)
							// WriteLog("TableCheck: Event is behind. Delete from table: " + sSpeedTable[i].evEvent->asName);
							sSpeedTable[i].iFlags &= ~1; // degradacja pozycji dla samochodu;
                            // semafory usuwane tylko przy sprawdzaniu,
                            // bo wysy³aj¹ komendy 
                        }
                }
                // if (sSpeedTable[i].fDist<-20.0*fLength) //jeœli to coœ jest 20 razy dalej ni¿
                // d³ugoœæ sk³adu
                //{sSpeedTable[i].iFlags&=~1; //to jest to jakby b³¹d w scenerii
                // //WriteLog("Error: too distant object in scan table");
                //}
                // if (sSpeedTable[i].fDist>20.0*fLength) //jeœli to coœ jest 20 razy dalej ni¿
                // d³ugoœæ sk³adu
                //{sSpeedTable[i].iFlags&=~1; //to jest to jakby b³¹d w scenerii
                // //WriteLog("Error: too distant object in scan table");
                //}
            }
            if (i == iFirst) // jeœli jest pierwsz¹ pozycj¹ tabeli
            { // pozbycie siê pocz¹tkowej pozycji
                if ((sSpeedTable[i].iFlags & 1) ==
                    0) // jeœli pozycja istotna (po Update() mo¿e siê zmieniæ)
                    // if (iFirst!=iLast) //ostatnia musi zostaæ - to za³atwia for()
                    iFirst = (iFirst + 1) %
                             iSpeedTableSize; // kolejne sprawdzanie bêdzie ju¿ od nastêpnej pozycji
            }
        }
        sSpeedTable[iLast].Update(&pos, &dir, len); // aktualizacja ostatniego
        // WriteLog("TableCheck: Upate last track. Dist=" + AnsiString(sSpeedTable[iLast].fDist));
        if (sSpeedTable[iLast].fDist < fDistance)
            TableTraceRoute(fDistance, pVehicles[1]); // doskanowanie dalszego odcinka
    }
};

TCommandType TController::TableUpdate(double &fVelDes, double &fDist, double &fNext, double &fAcc)
{ // ustalenie parametrów, zwraca typ komendy, jeœli sygna³ podaje prêdkoœæ do jazdy
    // fVelDes - prêdkoœæ zadana
    // fDist - dystans w jakim nale¿y rozwa¿yæ ruch
    // fNext - prêdkoœæ na koñcu tego dystansu
    // fAcc - zalecane przyspieszenie w chwili obecnej - kryterium wyboru dystansu
    double a; // przyspieszenie
    double v; // prêdkoœæ
    double d; // droga
	double d_to_next_sem = 10000.0; //ustaiwamy na pewno dalej ni¿ widzi AI
    TCommandType go = cm_Unknown;
    eSignNext = NULL;
    int i, k = iLast - iFirst + 1;
    if (k < 0)
        k += iSpeedTableSize; // iloœæ pozycji do przeanalizowania
    iDrivigFlags &= ~(moveTrackEnd | moveSwitchFound | moveSemaphorFound |
                      moveSpeedLimitFound); // te flagi s¹ ustawiane tutaj, w razie potrzeby
    for (i = iFirst; k > 0; --k, i = (i + 1) % iSpeedTableSize)
    { // sprawdzenie rekordów od (iFirst) do (iLast), o ile s¹ istotne
        if (sSpeedTable[i].iFlags & spEnabled) // badanie istotnoœci
        { // o ile dana pozycja tabelki jest istotna
            if (sSpeedTable[i].iFlags & spPassengerStopPoint)
            { // jeœli przystanek, trzeba obs³u¿yæ wg rozk³adu
                if (sSpeedTable[i].evEvent->CommandGet() != asNextStop)
                { // jeœli nazwa nie jest zgodna
                    if (sSpeedTable[i].fDist < -fLength) // jeœli zosta³ przejechany
                        sSpeedTable[i].iFlags =
                            0; // to mo¿na usun¹æ (nie mog¹ byæ usuwane w skanowaniu)
                    continue; // ignorowanie jakby nie by³o tej pozycji
                }
                else if (iDrivigFlags &
                         moveStopPoint) // jeœli pomijanie W4, to nie sprawdza czasu odjazdu
                { // tylko gdy nazwa zatrzymania siê zgadza
                    if (!TrainParams->IsStop())
                    { // jeœli nie ma tu postoju
                        sSpeedTable[i].fVelNext = -1; // maksymalna prêdkoœæ w tym miejscu
                        if (sSpeedTable[i].fDist <
                            200.0) // przy 160km/h jedzie 44m/s, to da dok³adnoœæ rzêdu 5 sekund
                        { // zaliczamy posterunek w pewnej odleg³oœci przed (choæ W4 nie zas³ania
// ju¿ semafora)
#if LOGSTOPS
                            WriteLog(pVehicle->asName + " as " + TrainParams->TrainName + ": at " +
                                     AnsiString(GlobalTime->hh) + ":" + AnsiString(GlobalTime->mm) +
                                     " skipped " + asNextStop); // informacja
#endif
                            fLastStopExpDist = mvOccupied->DistCounter + 0.250 +
                                               0.001 * fLength; // przy jakim dystansie (stanie
                            // licznika) ma przesun¹æ na
                            // nastêpny postój
                            TrainParams->UpdateMTable(
                                GlobalTime->hh, GlobalTime->mm,
                                asNextStop.SubString(20, asNextStop.Length()));
                            TrainParams->StationIndexInc(); // przejœcie do nastêpnej
                            asNextStop =
                                TrainParams->NextStop(); // pobranie kolejnego miejsca zatrzymania
                            // TableClear(); //aby od nowa sprawdzi³o W4 z inn¹ nazw¹ ju¿ - to nie
                            // jest dobry pomys³
                            sSpeedTable[i].iFlags = 0; // nie liczy siê ju¿
                            sSpeedTable[i].fVelNext = -1; // jechaæ
                            continue; // nie analizowaæ prêdkoœci
                        }
                    } // koniec obs³ugi przelotu na W4
                    else
                    { // zatrzymanie na W4
                        if (!eSignNext)
                            eSignNext = sSpeedTable[i].evEvent;
                        if (mvOccupied->Vel > 0.3) // jeœli jedzie (nie trzeba czekaæ, a¿ siê
                            // drgania wyt³umi¹ - drzwi zamykane od 1.0)
                            sSpeedTable[i].fVelNext = 0; // to bêdzie zatrzymanie
                        // else if
                        // ((iDrivigFlags&moveStopCloser)?sSpeedTable[i].fDist<=fMaxProximityDist*(AIControllFlag?1.0:10.0):true)
                        else if ((iDrivigFlags & moveStopCloser) ?
                                     sSpeedTable[i].fDist + fLength <=
                                         Max0R(sSpeedTable[i].evEvent->ValueGet(2),
                                               fMaxProximityDist + fLength) :
                                     sSpeedTable[i].fDist < d_to_next_sem)
                        // Ra 2F1I: odleg³oœæ plus d³ugoœæ poci¹gu musi byæ mniejsza od d³ugoœci
                        // peronu, chyba ¿e poci¹g jest d³u¿szy, to wtedy minimalna
                        // jeœli d³ugoœæ peronu ((sSpeedTable[i].evEvent->ValueGet(2)) nie podana,
                        // przyj¹æ odleg³oœæ fMinProximityDist
                        { // jeœli siê zatrzyma³ przy W4, albo sta³ w momencie zobaczenia W4
                            if (!AIControllFlag) // AI tylko sobie otwiera drzwi
                                iDrivigFlags &= ~moveStopCloser; // w razie prze³¹czenia na AI ma
                            // nie podci¹gaæ do W4, gdy
                            // u¿ytkownik zatrzyma³ za daleko
                            if ((iDrivigFlags & moveDoorOpened) == 0)
                            { // drzwi otwieraæ jednorazowo
                                iDrivigFlags |= moveDoorOpened; // nie wykonywaæ drugi raz
                                if (mvOccupied->DoorOpenCtrl == 1) //(mvOccupied->TrainType==dt_EZT)
                                { // otwieranie drzwi w EZT
                                    if (AIControllFlag) // tylko AI otwiera drzwi EZT, u¿ytkownik
                                        // musi samodzielnie
                                        if (!mvOccupied->DoorLeftOpened &&
                                            !mvOccupied->DoorRightOpened)
                                        { // otwieranie drzwi
                                            int p2 =
                                                int(floor(sSpeedTable[i].evEvent->ValueGet(2))) %
                                                10; // p7=platform side (1:left, 2:right, 3:both)
                                            int lewe = (iDirection > 0) ? 1 : 2; // jeœli jedzie do
                                            // ty³u, to drzwi
                                            // otwiera
                                            // odwrotnie
                                            int prawe = (iDirection > 0) ? 2 : 1;
                                            if (p2 & lewe)
                                                mvOccupied->DoorLeft(true);
                                            if (p2 & prawe)
                                                mvOccupied->DoorRight(true);
                                            // if (p2&3) //¿eby jeszcze poczeka³ chwilê, zanim
                                            // zamknie
                                            // WaitingSet(10); //10 sekund (wzi¹æ z rozk³adu????)
                                        }
                                }
                                else
                                { // otwieranie drzwi w sk³adach wagonowych - docelowo wysy³aæ
                                    // komendê zezwolenia na otwarcie drzwi
                                    int p7, lewe,
                                        prawe; // p7=platform side (1:left, 2:right, 3:both)
                                    p7 = int(floor(sSpeedTable[i].evEvent->ValueGet(2))) %
                                         10; // tu bêdzie jeszcze d³ugoœæ peronu zaokr¹glona do 10m
                                    // (20m bezpieczniej, bo nie modyfikuje bitu 1)
                                    TDynamicObject *p = pVehicles[0]; // pojazd na czole sk³adu
                                    while (p)
                                    { // otwieranie drzwi w pojazdach - flaga zezwolenia by³a by
                                        // lepsza
                                        lewe = (p->DirectionGet() > 0) ? 1 : 2; // jeœli jedzie do
                                        // ty³u, to drzwi
                                        // otwiera odwrotnie
                                        prawe = 3 - lewe;
                                        p->MoverParameters->BatterySwitch(true); // wagony musz¹
                                        // mieæ bateriê
                                        // za³¹czon¹ do
                                        // otwarcia
                                        // drzwi...
                                        if (p7 & lewe)
                                            p->MoverParameters->DoorLeft(true);
                                        if (p7 & prawe)
                                            p->MoverParameters->DoorRight(true);
                                        p = p->Next(); // pojazd pod³¹czony z ty³u (patrz¹c od
                                        // czo³a)
                                    }
                                    // if (p7&3) //¿eby jeszcze poczeka³ chwilê, zanim zamknie
                                    // WaitingSet(10); //10 sekund (wzi¹æ z rozk³adu????)
                                }
                                if (fStopTime >
                                    -5) // na koñcu rozk³adu siê ustawia 60s i tu by by³o skrócenie
                                    WaitingSet(10); // 10 sekund (wzi¹æ z rozk³adu????) - czekanie
                                // niezale¿ne od sposobu obs³ugi drzwi, bo
                                // opóŸnia równie¿ kierownika
                            }
                            if (TrainParams->UpdateMTable(
                                    GlobalTime->hh, GlobalTime->mm,
                                    asNextStop.SubString(20, asNextStop.Length())))
                            { // to siê wykona tylko raz po zatrzymaniu na W4
                                if (TrainParams->CheckTrainLatency() < 0.0)
                                    iDrivigFlags |= moveLate; // odnotowano spóŸnienie
                                else
                                    iDrivigFlags &= ~moveLate; // przyjazd o czasie
                                if (TrainParams->DirectionChange()) // jeœli "@" w rozk³adzie, to
                                // wykonanie dalszych komend
                                { // wykonanie kolejnej komendy, nie dotyczy ostatniej stacji
                                    if (iDrivigFlags & movePushPull) // SN61 ma siê te¿ nie ruszaæ,
                                    // chyba ¿e ma wagony
                                    {
                                        iDrivigFlags |= moveStopHere; // EZT ma staæ przy peronie
                                        if (OrderNextGet() != Change_direction)
                                        {
                                            OrderPush(Change_direction); // zmiana kierunku
                                            OrderPush(TrainParams->StationIndex <
                                                              TrainParams->StationCount ?
                                                          Obey_train :
                                                          Shunt); // to dalej wg rozk³adu
                                        }
                                    }
                                    else // a dla lokomotyw...
                                        iDrivigFlags &=
                                            ~(moveStopPoint | moveStopHere); // pozwolenie na
                                    // przejechanie za W4
                                    // przed czasem i nie
                                    // ma staæ
                                    JumpToNextOrder(); // przejœcie do kolejnego rozkazu (zmiana
                                    // kierunku, odczepianie)
                                    iDrivigFlags &= ~moveStopCloser; // ma nie podje¿d¿aæ pod W4 po
                                    // przeciwnej stronie
                                    sSpeedTable[i].iFlags = 0; // ten W4 nie liczy siê ju¿ zupe³nie
                                    // (nie wyœle SetVelocity)
                                    sSpeedTable[i].fVelNext = -1; // jechaæ
                                    continue; // nie analizowaæ prêdkoœci
                                }
                            }
                            if (OrderCurrentGet() == Shunt)
                            {
                                OrderNext(Obey_train); // uruchomiæ jazdê poci¹gow¹
                                CheckVehicles(); // zmieniæ œwiat³a
                            }
                            if (TrainParams->StationIndex < TrainParams->StationCount)
                            { // jeœli s¹ dalsze stacje, czekamy do godziny odjazdu
                                if (TrainParams->IsTimeToGo(GlobalTime->hh, GlobalTime->mm))
                                { // z dalsz¹ akcj¹ czekamy do godziny odjazdu
                                    // if (TrainParams->CheckTrainLatency()<0.0) //jak siê ma odjazd
                                    // do czasu odjazdu?
                                    // iDrivigFlags|=moveLate1; //oflagowaæ, gdy odjazd ze
                                    // spóŸnieniem, bêdzie jecha³ forsowniej
                                    fLastStopExpDist =
                                        mvOccupied->DistCounter + 0.050 +
                                        0.001 * fLength; // przy jakim dystansie (stanie licznika)
                                    // ma przesun¹æ na nastêpny postój
                                    //         Controlled->    //zapisaæ odleg³oœæ do przejechania
                                    TrainParams->StationIndexInc(); // przejœcie do nastêpnej
                                    asNextStop =
                                        TrainParams
                                            ->NextStop(); // pobranie kolejnego miejsca zatrzymania
// TableClear(); //aby od nowa sprawdzi³o W4 z inn¹ nazw¹ ju¿ - to nie jest dobry pomys³
#if LOGSTOPS
                                    WriteLog(pVehicle->asName + " as " + TrainParams->TrainName +
                                             ": at " + AnsiString(GlobalTime->hh) + ":" +
                                             AnsiString(GlobalTime->mm) + " next " +
                                             asNextStop); // informacja
#endif
									if (int(floor(sSpeedTable[i].evEvent->ValueGet(1))) & 1)
										iDrivigFlags |= moveStopHere; // nie podje¿d¿aæ do semafora,
									// jeœli droga nie jest wolna
									else
										iDrivigFlags &= ~moveStopHere; //po czasie jedŸ dalej
                                    iDrivigFlags |= moveStopCloser; // do nastêpnego W4 podjechaæ
                                    // blisko (z doci¹ganiem)
                                    iDrivigFlags &= ~moveStartHorn; // bez tr¹bienia przed odjazdem
                                    sSpeedTable[i].iFlags =
                                        0; // nie liczy siê ju¿ zupe³nie (nie wyœle SetVelocity)
                                    sSpeedTable[i].fVelNext = -1; // mo¿na jechaæ za W4
                                    if (go == cm_Unknown) // jeœli nie by³o komendy wczeœniej
                                        go = cm_Ready; // gotów do odjazdu z W4 (semafor mo¿e
                                    // zatrzymaæ)
                                    if (tsGuardSignal) // jeœli mamy g³os kierownika, to odegraæ
                                        iDrivigFlags |= moveGuardSignal;
                                    continue; // nie analizowaæ prêdkoœci
                                } // koniec startu z zatrzymania
                            } // koniec obs³ugi pocz¹tkowych stacji
                            else
                            { // jeœli dojechaliœmy do koñca rozk³adu
#if LOGSTOPS
                                WriteLog(pVehicle->asName + " as " + TrainParams->TrainName +
                                         ": at " + AnsiString(GlobalTime->hh) + ":" +
                                         AnsiString(GlobalTime->mm) +
                                         " end of route."); // informacja
#endif
                                asNextStop = TrainParams->NextStop(); // informacja o koñcu trasy
                                TrainParams->NewName("none"); // czyszczenie nieaktualnego rozk³adu
                                // TableClear(); //aby od nowa sprawdzi³o W4 z inn¹ nazw¹ ju¿ - to
                                // nie jest dobry pomys³
                                iDrivigFlags &=
                                    ~(moveStopCloser |
                                      moveStopPoint); // ma nie podje¿d¿aæ pod W4 i ma je pomijaæ
                                sSpeedTable[i].iFlags =
                                    0; // W4 nie liczy siê ju¿ (nie wyœle SetVelocity)
                                sSpeedTable[i].fVelNext = -1; // mo¿na jechaæ za W4
                                fLastStopExpDist = -1.0f; // nie ma rozk³adu, nie ma usuwania stacji
                                WaitingSet(60); // tak ze 2 minuty, a¿ wszyscy wysi¹d¹
                                JumpToNextOrder(); // wykonanie kolejnego rozkazu (Change_direction
                                // albo Shunt)
                                iDrivigFlags |= moveStopHere | moveStartHorn; // ma siê nie ruszaæ
                                // a¿ do momentu
                                // podania sygna³u
                                continue; // nie analizowaæ prêdkoœci
                            } // koniec obs³ugi ostatniej stacji
                        } // if (MoverParameters->Vel==0.0)
                    } // koniec obs³ugi zatrzymania na W4
                } // koniec warunku pomijania W4 podczas zmiany czo³a
                else
                { // skoro pomijanie, to jechaæ i ignorowaæ W4
                    sSpeedTable[i].iFlags = 0; // W4 nie liczy siê ju¿ (nie zatrzymuje jazdy)
                    sSpeedTable[i].fVelNext = -1;
                    continue; // nie analizowaæ prêdkoœci
                }
            } // koniec obs³ugi W4
            v = sSpeedTable[i].fVelNext; // odczyt prêdkoœci do zmiennej pomocniczej
            if (sSpeedTable[i].iFlags &
                spSwitch) // zwrotnice s¹ usuwane z tabelki dopiero po zjechaniu z nich
                iDrivigFlags |=
                    moveSwitchFound; // rozjazd z przodu/pod ogranicza np. sens skanowania wstecz
            else if (sSpeedTable[i].iFlags & spEvent) // W4 mo¿e siê deaktywowaæ
            { // je¿eli event, mo¿e byæ potrzeba wys³ania komendy, aby ruszy³
                // sprawdzanie eventów pasywnych miniêtych
                if (sSpeedTable[i].fDist < 0.0 && sSemNext == &sSpeedTable[i])
                {
                    WriteLog("TableUpdate: semaphor " + sSemNext->GetName() + " passed by " + OwnerName());
                    sSemNext = NULL; // jeœli minêliœmy semafor od ograniczenia to go kasujemy ze
                    // zmiennej sprawdzaj¹cej dla skanowania w przód
                }
                if (sSpeedTable[i].fDist < 0.0 && sSemNextStop == &sSpeedTable[i])
                {
                    WriteLog("TableUpdate: semaphor " + sSemNextStop->GetName() + " passed by " + OwnerName());
                    sSemNextStop = NULL; // jeœli minêliœmy semafor od ograniczenia to go kasujemy ze
                    // zmiennej sprawdzaj¹cej dla skanowania w przód
                }
                if (sSpeedTable[i].fDist > 0.0 &&
                    sSpeedTable[i].IsProperSemaphor(OrderCurrentGet()))
                {
                    if (!sSemNext)
                    {
                        sSemNext = &sSpeedTable[i]; // jeœli jest mieniêty poprzedni
                        // semafor a wczeœniej
                        // byl nowy to go dorzucamy do zmiennej, ¿eby ca³y
                        // czas widzia³ najbli¿szy
                        WriteLog("TableUpdate: Next semaphor: " + sSemNext->GetName() + " by " + OwnerName());
                    }
                    if (!sSemNextStop || (sSemNextStop && sSemNextStop->fVelNext != 0 &&
                                          sSpeedTable[i].fVelNext == 0))
                        sSemNextStop = &sSpeedTable[i];
               }
                if (sSpeedTable[i].iFlags & spOutsideStation)
                { // jeœli W5, to reakcja zale¿na od trybu jazdy
                    if (OrderCurrentGet() & Obey_train)
                    { // w trybie poci¹gowym: mo¿na przyspieszyæ do wskazanej prêdkoœci (po
                        // zjechaniu z rozjazdów)
                        v = -1.0; // ignorowaæ?
//TODO trzeba zmieniæ przypisywanie VelSignal na VelSignalLast
						if (sSpeedTable[i].fDist < 0.0) // jeœli wskaŸnik zosta³ miniêty
                        {
                            VelSignalLast = v; //ustawienie prêdkoœci na -1
                            //       iStationStart=TrainParams->StationIndex; //zaktualizowaæ
                            //       wyœwietlanie rozk³adu
                        }
                        else if (!(iDrivigFlags & moveSwitchFound)) // jeœli rozjazdy ju¿ miniête
                            VelSignalLast = v; //!!! to te¿ koniec ograniczenia
                    }
                    else
                    { // w trybie manewrowym: skanowaæ od niego wstecz, stan¹æ po wyjechaniu za
                        // sygnalizator i zmieniæ kierunek
                        v = 0.0; // zmiana kierunku mo¿e byæ podanym sygna³em, ale wypada³o by
                        // zmieniæ œwiat³o wczeœniej
                        if (!(iDrivigFlags & moveSwitchFound)) // jeœli nie ma rozjazdu
                            iDrivigFlags |= moveTrackEnd; // to dalsza jazda trwale ograniczona (W5,
                        // koniec toru)
                    }
                }
                else if (sSpeedTable[i].iFlags & spStopOnSBL)
                { // jeœli S1 na SBL
                    if (mvOccupied->Vel < 2.0) // stan¹æ nie musi, ale zwolniæ przynajmniej
                        if (sSpeedTable[i].fDist < fMaxProximityDist) // jest w maksymalnym zasiêgu
                        {
                            eSignSkip = sSpeedTable[i]
                                            .evEvent; // to mo¿na go pomin¹æ (wzi¹æ drug¹ prêdkosæ)
                            iDrivigFlags |= moveVisibility; // jazda na widocznoœæ - skanowaæ
                            // mo¿liwoœæ kolizji i nie podje¿d¿aæ
                            // zbyt blisko
                            // usun¹æ flagê po podjechaniu blisko semafora zezwalaj¹cego na jazdê
                            // ostro¿nie interpretowaæ sygna³y - semafor mo¿e zezwalaæ na jazdê
                            // poci¹gu z przodu!
                        }
                    if (eSignSkip != sSpeedTable[i].evEvent) // jeœli ten SBL nie jest do pominiêcia
    // TODO sprawdziæ do której zmiennej jest przypisywane v i zmieniæ to tutaj
						v = sSpeedTable[i].evEvent->ValueGet(1); // to ma 0 odczytywaæ
                }
                else if (sSpeedTable[i].IsProperSemaphor(OrderCurrentGet()))
                { // to semaphor
					if (sSpeedTable[i].fDist < 0)
						VelSignalLast = sSpeedTable[i].fVelNext; //miniêty daje prêdkoœæ obowi¹zuj¹c¹
					else
					{
						iDrivigFlags |= moveSemaphorFound; //jeœli z przodu to dajemy falgê, ¿e jest
						d_to_next_sem = Min0R(sSpeedTable[i].fDist, d_to_next_sem);
					}
                }
                else if (sSpeedTable[i].iFlags & spRoadVel)
                { // to W6
                    if (sSpeedTable[i].fDist < 0)
                        VelRoad = sSpeedTable[i].fVelNext;
                }
                else if (sSpeedTable[i].iFlags & spSectionVel)
                { // to W27
                    if (sSpeedTable[i].fDist < 0) // teraz trzeba sprawdziæ inne warunki
                    {
                        if (sSpeedTable[i].fSectionVelocityDist == 0.0)
                        {
                            WriteLog("TableUpdate: Event is behind. SVD = 0: " + sSpeedTable[i].evEvent->asName);
                            sSpeedTable[i].iFlags = 0; // jeœli punktowy to kasujemy i nie dajemy ograniczenia na sta³e
                        }
                        else if (sSpeedTable[i].fSectionVelocityDist < 0.0)
                        { // ograniczenie obowi¹zuj¹ce do nastêpnego
                            if (sSpeedTable[i].fVelNext == Global::Min0RSpeed(sSpeedTable[i].fVelNext, VelLimitLast) &&
                                sSpeedTable[i].fVelNext != VelLimitLast)
                            { // jeœli ograniczenie jest mniejsze ni¿ obecne to obowi¹zuje od zaraz
                                VelLimitLast = sSpeedTable[i].fVelNext;
                            }
                            else if (sSpeedTable[i].fDist < -fLength)
                            { // jeœli wiêksze to musi wyjechaæ za poprzednie
                                VelLimitLast = sSpeedTable[i].fVelNext;
                                WriteLog("TableUpdate: Event is behind. SVD < 0: " + sSpeedTable[i].evEvent->asName);
                                sSpeedTable[i].iFlags = 0; // wyjechaliœmy poza poprzednie, mo¿na skasowaæ
                            }
                        }
                        else
                        { // jeœli wiêksze to ograniczenie ma swoj¹ d³ugoœæ
                            if (sSpeedTable[i].fVelNext == Global::Min0RSpeed(sSpeedTable[i].fVelNext, VelLimitLast) &&
                                sSpeedTable[i].fVelNext != VelLimitLast)
                            { // jeœli ograniczenie jest mniejsze ni¿ obecne to obowi¹zuje od zaraz
                                VelLimitLast = sSpeedTable[i].fVelNext;
                            }
                            else if (sSpeedTable[i].fDist < -fLength && sSpeedTable[i].fVelNext != VelLimitLast)
                            { // jeœli wiêksze to musi wyjechaæ za poprzednie
                                VelLimitLast = sSpeedTable[i].fVelNext;
                            }
                            else if (sSpeedTable[i].fDist < -fLength - sSpeedTable[i].fSectionVelocityDist)
                            { //
                                VelLimitLast = -1.0;
                                WriteLog("TableUpdate: Event is behind. SVD > 0: " + sSpeedTable[i].evEvent->asName);
                                sSpeedTable[i].iFlags = 0; // wyjechaliœmy poza poprzednie, mo¿na skasowaæ
                            }
                        }
                    }
                }

                //sprawdzenie eventów pasywnych przed nami
                if ((mvOccupied->CategoryFlag & 1) ?
                        sSpeedTable[i].fDist > pVehicles[0]->fTrackBlock - 20.0 :
                        false) // jak sygna³ jest dalej ni¿ zawalidroga
                    v = 0.0; // to mo¿e byæ podany dla tamtego: jechaæ tak, jakby tam stop by³
                else
                { // zawalidrogi nie ma (albo pojazd jest samochodem), sprawdziæ sygna³
                    if (sSpeedTable[i].iFlags & spShuntSemaphor) // jeœli Tm - w zasadzie to sprawdziæ
                    // komendê!
                    { // jeœli podana prêdkoœæ manewrowa
                        if ((OrderCurrentGet() & Obey_train) ? v == 0.0 : false)
                        { // jeœli tryb poci¹gowy a tarcze ma ShuntVelocity 0 0
                            v = -1; // ignorowaæ, chyba ¿e prêdkoœæ stanie siê niezerowa
                            if (sSpeedTable[i].iFlags & spElapsed) // a jak przejechana
                                sSpeedTable[i].iFlags = 0; // to mo¿na usun¹æ, bo podstawowy automat
                            // usuwa tylko niezerowe
                        }
                        else if (go == cm_Unknown) // jeœli jeszcze nie ma komendy
                            if (v != 0.0) // komenda jest tylko gdy ma jechaæ, bo stoi na podstawie
                            // tabelki
                            { // jeœli nie by³o komendy wczeœniej - pierwsza siê liczy - ustawianie
                                // VelSignal
                                go = cm_ShuntVelocity; // w trybie poci¹gowym tylko jeœli w³¹cza
                                // tryb manewrowy (v!=0.0)
                                // Ra 2014-06: (VelSignal) nie mo¿e byæ tu ustawiane, bo Tm mo¿e byæ
                                // daleko
                                // VelSignal=v; //nie do koñca tak, to jest druga prêdkoœæ
                                if (VelSignal == 0.0)
                                    VelSignal = v; // aby stoj¹cy ruszy³
                                if (sSpeedTable[i].fDist < 0.0) // jeœli przejechany
                                {
                                    VelSignal = v; //!!! ustawienie, gdy przejechany jest lepsze ni¿
                                    // wcale, ale to jeszcze nie to
                                    sSpeedTable[i].iFlags =
                                        0; // to mo¿na usun¹æ (nie mog¹ byæ usuwane w skanowaniu)
                                }
                            }
                    }
                    else if (!(sSpeedTable[i].iFlags & spSectionVel)) //jeœli jakiœ event pasywny ale nie ograniczenie
                        if (go == cm_Unknown) // jeœli nie by³o komendy wczeœniej - pierwsza siê liczy
                        // - ustawianie VelSignal
                        if (v < 0.0 ? true : v >= 1.0) // bo wartoœæ 0.1 s³u¿y do hamowania tylko
                        {
                            go = cm_SetVelocity; // mo¿e odjechaæ
                            // Ra 2014-06: (VelSignal) nie mo¿e byæ tu ustawiane, bo semafor mo¿e
                            // byæ daleko
                            // VelSignal=v; //nie do koñca tak, to jest druga prêdkoœæ; -1 nie
                            // wpisywaæ...
                            if (VelSignal == 0.0)
                                VelSignal = -1.0; // aby stoj¹cy ruszy³
                            if (sSpeedTable[i].fDist < 0.0) // jeœli przejechany
                            {
                                if (v != 0 ? VelSignal = -1.0 : VelSignal = 0.0)
                                    ; // ustawienie, gdy przejechany jest lepsze ni¿
                                // wcale, ale to jeszcze nie to
                                if (sSpeedTable[i].iFlags & spEvent) // jeœli event
                                    if ((sSpeedTable[i].evEvent != eSignSkip) ?
                                            true :
                                            (sSpeedTable[i].fVelNext != 0.0)) // ale inny ni¿ ten,
                                        // na którym miniêto
                                        // S1, chyba ¿e siê
                                        // ju¿ zmieni³o
                                        iDrivigFlags &= ~moveVisibility; // sygna³ zezwalaj¹cy na
                                // jazdê wy³¹cza jazdê na
                                // widocznoœæ (S1 na SBL)
                                
								// usun¹æ jeœli nie jest ograniczeniem prêdkoœci
                                sSpeedTable[i].iFlags =
                                    0; // to mo¿na usun¹æ (nie mog¹ byæ usuwane w skanowaniu)
                            }
                        }
                        else if (sSpeedTable[i].evEvent->StopCommand())
                        { // jeœli prêdkoœæ jest zerowa, a komórka zawiera komendê
                            eSignNext = sSpeedTable[i].evEvent; // dla informacji
                            if (iDrivigFlags &
                                moveStopHere) // jeœli ma staæ, dostaje komendê od razu
                                go = cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                            else if (sSpeedTable[i].fDist <= 20.0) // jeœli ma doci¹gn¹æ, to niech
                                // doci¹ga (moveStopCloser
                                // dotyczy doci¹gania do W4, nie
                                // semafora)
                                go = cm_Command; // komenda z komórki, do wykonania po zatrzymaniu
                        }
                } // jeœli nie ma zawalidrogi
            } // jeœli event
            if (v >= 0.0)
            { // pozycje z prêdkoœci¹ -1 mo¿na spokojnie pomijaæ
                d = sSpeedTable[i].fDist; 
                if ((sSpeedTable[i].iFlags & spElapsed) ?
                        false :
                        d > 0.0) // sygna³ lub ograniczenie z przodu (+32=przejechane)
                { // 2014-02: jeœli stoi, a ma do przejechania kawa³ek, to niech jedzie
                    if ((mvOccupied->Vel == 0.0) ?
                            ((sSpeedTable[i].iFlags &
                              (spEnabled | spEvent | spPassengerStopPoint)) ==
                             (spEnabled | spEvent | spPassengerStopPoint)) &&
                                (d > fMaxProximityDist) :
                            false)
                        a = (iDrivigFlags & moveStopCloser) ? fAcc : 0.0; // ma podjechaæ bli¿ej -
                    // czy na pewno w tym
                    // miejscu taki warunek?
                    else
                    {
                        a = (v * v - mvOccupied->Vel * mvOccupied->Vel) /
                            (25.92 * d); // przyspieszenie: ujemne, gdy trzeba hamowaæ
                        if (d < fMinProximityDist) // jak jest ju¿ blisko
                            if (v < fVelDes)
                                fVelDes = v; // ograniczenie aktualnej prêdkoœci
                    }
                }
                else if (sSpeedTable[i].iFlags & spTrack) // jeœli tor
                { // tor ogranicza prêdkoœæ, dopóki ca³y sk³ad nie przejedzie,
                    // d=fLength+d; //zamiana na d³ugoœæ liczon¹ do przodu
                    if (v >= 1.0) // EU06 siê zawiesza³o po dojechaniu na koniec toru postojowego
                        if (d < -fLength)
                            continue; // zapêtlenie, jeœli ju¿ wyjecha³ za ten odcinek
                    if (v < fVelDes)
                        fVelDes =
                            v; // ograniczenie aktualnej prêdkoœci a¿ do wyjechania za ograniczenie
                    // if (v==0.0) fAcc=-0.9; //hamowanie jeœli stop
                    continue; // i tyle wystarczy
                }
                else // event trzyma tylko jeœli VelNext=0, nawet po przejechaniu (nie powinno
                    // dotyczyæ samochodów?)
                    a = (v == 0.0 ? -1.0 : fAcc); // ruszanie albo hamowanie
                if (a < fAcc && v == Min0R(v, fNext))
                { // mniejsze przyspieszenie to mniejsza mo¿liwoœæ rozpêdzenia siê albo koniecznoœæ
                    // hamowania
                    // jeœli droga wolna, to mo¿e byæ a>1.0 i siê tu nie za³apuje
                    // if (mvOccupied->Vel>10.0)
                    fAcc = a; // zalecane przyspieszenie (nie musi byæ uwzglêdniane przez AI)
                    fNext = v; // istotna jest prêdkoœæ na koñcu tego odcinka
                    fDist = d; // dlugoœæ odcinka
                }
                else if ((fAcc > 0) && (v > 0) && (v <= fNext))
                { // jeœli nie ma wskazañ do hamowania, mo¿na podaæ drogê i prêdkoœæ na jej koñcu
                    fNext = v; // istotna jest prêdkoœæ na koñcu tego odcinka
                    fDist = d; // dlugoœæ odcinka (kolejne pozycje mog¹ wyd³u¿aæ drogê, jeœli
                    // prêdkoœæ jest sta³a)
                }
            } // if (v>=0.0)
            if (fNext >= 0.0)
            { // jeœli ograniczenie
                if ((sSpeedTable[i].iFlags & (spEnabled | spEvent)) == 
					(spEnabled | spEvent)) // tylko sygna³ przypisujemy
                    if (!eSignNext) // jeœli jeszcze nic nie zapisane tam
                        eSignNext = sSpeedTable[i].evEvent; // dla informacji
                if (fNext == 0.0)
                    break; // nie ma sensu analizowaæ tabelki dalej
            }
        } // if (sSpeedTable[i].iFlags&1)
    } // for

	if (VelSignalLast >= 0.0 && !(iDrivigFlags & (moveSemaphorFound | moveSwitchFound)) &&
		(OrderCurrentGet() & Obey_train))
			VelSignalLast = -1.0; // jeœli mieliœmy ograniczenie z semafora i nie ma przed nami

	if (VelSignalLast >= 0.0) //analiza spisanych z tabelki ograniczeñ i nadpisanie aktualnego
		fVelDes = Min0R(fVelDes, VelSignalLast);
	if (VelLimitLast >= 0.0)
		fVelDes = Min0R(fVelDes, VelLimitLast);
	if (VelRoad >= 0.0)
		fVelDes = Min0R(fVelDes, VelRoad);
	// nastepnego semafora albo zwrotnicy to uznajemy, ¿e mijamy W5
	FirstSemaphorDist = d_to_next_sem; // przepisanie znalezionej wartosci do zmiennej
    return go;
};

void TController::TablePurger()
{ // odtykacz: usuwa mniej istotne pozycje ze œrodka tabelki, aby unikn¹æ zatkania
    //(np. brak ograniczenia pomiêdzy zwrotnicami, usuniête sygna³y, miniête odcinki ³uku)
	WriteLog("TablePurger: Czyszczenie tableki.");
	int i, j, k = iLast - iFirst; // mo¿e byæ 15 albo 16 pozycji, ostatniej nie ma co sprawdzaæ
    if (k < 0)
        k += iSpeedTableSize; // iloœæ pozycji do przeanalizowania
    for (i = iFirst; k > 0; --k, i = (i + 1) % iSpeedTableSize)
    { // sprawdzenie rekordów od (iFirst) do (iLast), o ile s¹ istotne
        if ((sSpeedTable[i].iFlags & spEnabled) ?
                (sSpeedTable[i].fVelNext < 0) && ((sSpeedTable[i].iFlags & 0xAB) == 0xA3) :
                true)
        { // jeœli jest to miniêty (0x20) tor (0x03) do liczenia ciêciw (0x80), a nie zwrotnica
            // (0x08)
			for (; k > 0; --k, i = (i + 1) % iSpeedTableSize)
			{
				sSpeedTable[i] = sSpeedTable[(i + 1) % iSpeedTableSize]; // skopiowanie
				if (&sSpeedTable[(i + 1) % iSpeedTableSize] == sSemNext)
					sSemNext = &sSpeedTable[i]; // przeniesienie znacznika o semaforze
				if (&sSpeedTable[(i + 1) % iSpeedTableSize] == sSemNextStop)
					sSemNextStop = &sSpeedTable[i]; // przeniesienie znacznika o semaforze
			}
            WriteLog("Odtykacz usuwa pozycjê");
            iLast = (iLast - 1 + iSpeedTableSize) % iSpeedTableSize; // cofniêcie z zawiniêciem
            return;
        }
    }
    // jeœli powy¿sze odtykane nie pomo¿e, mo¿na usun¹æ coœ wiêcej, albo powiêkszyæ tabelkê
    TSpeedPos *t = new TSpeedPos[iSpeedTableSize + 16]; // zwiêkszenie
    k = iLast - iFirst + 1; // tym razem wszystkie
    if (k < 0)
        k += iSpeedTableSize; // iloœæ pozycji do przeanalizowania
    for (j = -1, i = iFirst; k > 0; --k)
    { // przepisywanie rekordów iFirst..iLast na 0..k
        t[++j] = sSpeedTable[i];
        if (&sSpeedTable[i] == sSemNext)
            sSemNext = &t[j]; // przeniesienie znacznika o semaforze
        if (&sSpeedTable[i] == sSemNextStop)
            sSemNextStop = &t[j]; // przeniesienie znacznika o semaforze
        i = (i + 1) % iSpeedTableSize; // kolejna pozycja mog¹ byæ zawiniêta
    }
    iFirst = 0; // teraz bêdzie od zera
    iLast = j; // ostatnia
    delete[] sSpeedTable; // to ju¿ nie potrzebne
    sSpeedTable = t; // bo jest nowe
    iSpeedTableSize += 16;
    WriteLog("Tabelka powiêkszona do "+AnsiString(iSpeedTableSize)+" pozycji");
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

TController::TController(bool AI, TDynamicObject *NewControll, bool InitPsyche,
                         bool primary // czy ma aktywnie prowadziæ?
                         )
{
    iEngineActive = 0;
    LastUpdatedTime = 0.0;
    ElapsedTime = 0.0;
    // inicjalizacja zmiennych
    Psyche = InitPsyche;
    VelDesired = 0.0; // prêdkosæ pocz¹tkowa
    VelforDriver = -1;
    LastReactionTime = 0.0;
    HelpMeFlag = false;
    // fProximityDist=1; //nie u¿ywane
    ActualProximityDist = 1;
	FirstSemaphorDist = 10000.0;
    vCommandLocation.x = 0;
    vCommandLocation.y = 0;
    vCommandLocation.z = 0;
    VelSignal = 0.0; // normalnie na pocz¹tku ma staæ, no chyba ¿e jedzie
    VelLimit = -1.0; // brak ograniczenia prêdkoœci
    VelNext = 120.0;
    VelLimitLast = -1.0; // ostatnie ograniczenie bez ograniczenia
    VelSignalLast = -1.0; // ostatni semafor te¿ bez ograniczenia
    VelRoad = -1.0; // prêdkoœæ drogowa bez ograniczenia
    AIControllFlag = AI;
    pVehicle = NewControll;
    ControllingSet(); // utworzenie po³¹czenia do sterowanego pojazdu
    pVehicles[0] = pVehicle->GetFirstDynamic(0); // pierwszy w kierunku jazdy (Np. Pc1)
    pVehicles[1] = pVehicle->GetFirstDynamic(1); // ostatni w kierunku jazdy (koñcówki)
    /*
     switch (mvOccupied->CabNo)
     {
      case -1: SendCtrlBroadcast("CabActivisation",1); break;
      case  1: SendCtrlBroadcast("CabActivisation",2); break;
      default: AIControllFlag:=False; //na wszelki wypadek
     }
    */
    iDirection = 0;
    iDirectionOrder = mvOccupied->CabNo; // 1=do przodu (w kierunku sprzêgu 0)
    VehicleName = mvOccupied->Name;
    // TrainParams=NewTrainParams;
    // if (TrainParams)
    // asNextStop=TrainParams->NextStop();
    // else
    TrainParams = new TTrainParameters("none"); // rozk³ad jazdy
    // OrderCommand="";
    // OrderValue=0;
    OrdersClear();
    MaxVelFlag = false;
    MinVelFlag = false; // Ra: to nie jest u¿ywane
    iDriverFailCount = 0;
    Need_TryAgain = false; // true, jeœli druga pozycja w elektryku nie za³apa³a
    Need_BrakeRelease = true;
    deltalog = 0.05; // 1.0;

    if (WriteLogFlag)
    {
        mkdir("physicslog\\");
        LogFile.open(AnsiString("physicslog\\" + VehicleName + ".dat").c_str(),
                     std::ios::in | std::ios::out | std::ios::trunc);
#if LOGPRESS == 0
        LogFile << AnsiString(" Time [s]   Velocity [m/s]  Acceleration [m/ss]   Coupler.Dist[m]  "
                              "Coupler.Force[N]  TractionForce [kN]  FrictionForce [kN]   "
                              "BrakeForce [kN]    BrakePress [MPa]   PipePress [MPa]   "
                              "MotorCurrent [A]    MCP SCP BCP LBP DmgFlag Command CVal1 CVal2")
                       .c_str() << "\r\n";
#endif
#if LOGPRESS == 1
        LogFile << AnsiString("t\tVel\tAcc\tPP\tVVP\tBP\tBVP\tCVP").c_str() << "\n";
#endif
        LogFile.flush();
    }
    /*
      if (WriteLogFlag)
      {
       assignfile(AILogFile,VehicleName+".txt");
       rewrite(AILogFile);
       writeln(AILogFile,"AI driver log: started OK");
       close(AILogFile);
      }
    */

    // VelMargin=2; //Controlling->Vmax*0.015;
    fWarningDuration = 0.0; // nic do wytr¹bienia
    WaitingExpireTime = 31.0; // tyle ma czekaæ, zanim siê ruszy
    WaitingTime = 0.0;
    fMinProximityDist = 30.0; // stawanie miêdzy 30 a 60 m przed przeszkod¹
    fMaxProximityDist = 50.0;
    iVehicleCount = -2; // wartoœæ neutralna
    // Prepare2press=false; //bez dociskania
    eStopReason = stopSleep; // na pocz¹tku œpi
    fLength = 0.0;
    fMass = 0.0; //[kg]
    eSignNext = NULL; // sygna³ zmieniaj¹cy prêdkoœæ, do pokazania na [F2]
	sSemNext = NULL; // pierwszy semafor w przebiegu
	sSemNextStop = NULL; // pierwszy semafor z sygna³em stój
    fShuntVelocity = 40; // domyœlna prêdkoœæ manewrowa
    fStopTime = 0.0; // czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
    iDrivigFlags = moveStopPoint; // podjedŸ do W4 mo¿liwie blisko
    iDrivigFlags |= moveStopHere; // nie podje¿d¿aj do semafora, jeœli droga nie jest wolna
    iDrivigFlags |= moveStartHorn; // podaj sygna³ po podaniu wolnej drogi
    if (primary)
        iDrivigFlags |= movePrimary; // aktywnie prowadz¹ce pojazd
    Ready = false;
    if (mvOccupied->CategoryFlag & 2)
    { // samochody: na podst. http://www.prawko-kwartnik.info/hamowanie.html
        // fDriverBraking=0.0065; //mno¿one przez (v^2+40*v) [km/h] daje prawie drogê hamowania [m]
        fDriverBraking = 0.03; // coœ nie hamuj¹ te samochody zbyt dobrze
        fDriverDist = 5.0; // 5m - zachowywany odstêp przed kolizj¹
        fVelPlus = 10.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
        fVelMinus = 2.0; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
    }
    else
    { // poci¹gi i statki
        fDriverBraking = 0.06; // mno¿one przez (v^2+40*v) [km/h] daje prawie drogê hamowania [m]
        fDriverDist = 50.0; // 50m - zachowywany odstêp przed kolizj¹
        fVelPlus = 5.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
        fVelMinus = 5.0; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
    }
    SetDriverPsyche(); // na koñcu, bo wymaga ustawienia zmiennych
    AccDesired = AccPreferred;
    fVelMax = -1; // ustalenie prêdkoœci dla sk³adu
    fBrakeTime = 0.0; // po jakim czasie przekrêciæ hamulec
    iVehicles = 0; // na wszelki wypadek
    iSpeedTableSize = 16;
    sSpeedTable = new TSpeedPos[iSpeedTableSize];
    TableClear();
    iRadioChannel = 1; // numer aktualnego kana³u radiowego
    fActionTime = 0.0;
    eAction = actSleep;
    tsGuardSignal = NULL; // komunikat od kierownika
    iGuardRadio = 0; // nie przez radio
    iStationStart = 0; // nic?
    // fAccThreshold mo¿e podlegaæ uczeniu siê - hamowanie powinno byæ rejestrowane, a potem
    // analizowane
    fAccThreshold =
        (mvOccupied->TrainType & dt_EZT) ? -0.6 : -0.2; // próg opóŸnienia dla zadzia³ania hamulca
    fLastStopExpDist = -1.0f;
    iRouteWanted = 3; // powiedzmy, ¿e ma jechaæ prosto (1=w lewo)
    iCoupler = 0; // sprzêg; niezerowy gdy ma byæ pod³¹czanie; samo pod³¹czanie w trybie Connect
    // (wczeœniej mo¿e byæ np. Prepare_engine)
    fOverhead1 = 3000.0; // informacja o napiêciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
    fOverhead2 = -1.0; // informacja o sposobie jazdy (-1=normalnie, 0=bez pr¹du, >0=z opuszczonym i
    // ograniczeniem prêdkoœci)
    iOverheadZero = 0; // suma bitowa jezdy bezpr¹dowej, bity ustawiane przez pojazdy z
    // podniesionymi pantografami
    iOverheadDown = 0; // suma bitowa opuszczenia pantografów, bity ustawiane przez pojazdy z
    // podniesionymi pantografami
    fAccDesiredAv = 0.0; // uœrednione przyspieszenie z kolejnych przeb³ysków œwiadomoœci, ¿eby
    // ograniczyæ migotanie
    fVoltage = 0.0; // uœrednione napiêcie sieci: przy spadku poni¿ej wartoœci minimalnej opóŸniæ
    // rozruch o losowy czas
};

void TController::CloseLog()
{
    if (WriteLogFlag)
    {
        LogFile.close();
        // if WriteLogFlag)
        // CloseFile(AILogFile);
        /*  append(AIlogFile);
          writeln(AILogFile,ElapsedTime5:2,": QUIT");
          close(AILogFile); */
    }
};

TController::~TController()
{ // wykopanie mechanika z roboty
    delete tsGuardSignal;
    delete TrainParams;
    delete[] sSpeedTable;
    CloseLog();
};

AnsiString TController::Order2Str(TOrders Order)
{ // zamiana kodu rozkazu na opis
    if (Order & Change_direction)
        return "Change_direction"; // mo¿e byæ na³o¿ona na inn¹ i wtedy ma priorytet
    if (Order == Wait_for_orders)
        return "Wait_for_orders";
    if (Order == Prepare_engine)
        return "Prepare_engine";
    if (Order == Shunt)
        return "Shunt";
    if (Order == Connect)
        return "Connect";
    if (Order == Disconnect)
        return "Disconnect";
    if (Order == Obey_train)
        return "Obey_train";
    if (Order == Release_engine)
        return "Release_engine";
    if (Order == Jump_to_first_order)
        return "Jump_to_first_order";
    /* Ra: wersja ze switch nie dzia³a prawid³owo (czemu?)
     switch (Order)
     {
      Wait_for_orders:     return "Wait_for_orders";
      Prepare_engine:      return "Prepare_engine";
      Shunt:               return "Shunt";
      Change_direction:    return "Change_direction";
      Obey_train:          return "Obey_train";
      Release_engine:      return "Release_engine";
      Jump_to_first_order: return "Jump_to_first_order";
     }
    */
    return "Undefined!";
}

AnsiString TController::OrderCurrent()
{ // pobranie aktualnego rozkazu celem wyœwietlenia
    return AnsiString(OrderPos) + ". " + Order2Str(OrderList[OrderPos]);
};

void TController::OrdersClear()
{ // czyszczenie tabeli rozkazów na starcie albo po dojœciu do koñca
    OrderPos = 0;
    OrderTop = 1; // szczyt stosu rozkazów
    for (int b = 0; b < maxorders; b++)
        OrderList[b] = Wait_for_orders;
#if LOGORDERS
    WriteLog("--> OrdersClear");
#endif
};

void TController::Activation()
{ // umieszczenie obsady w odpowiednim cz³onie, wykonywane wy³¹cznie gdy steruje AI
    iDirection = iDirectionOrder; // kierunek (wzglêdem sprzêgów pojazdu z AI) w³aœnie zosta³
    // ustalony (zmieniony)
    if (iDirection)
    { // jeœli jest ustalony kierunek
        TDynamicObject *old = pVehicle, *d = pVehicle; // w tym siedzi AI
        TController *drugi; // jakby by³y dwa, to zamieniæ miejscami, a nie robiæ wycieku pamiêci
        // poprzez nadpisanie
        int brake = mvOccupied->LocalBrakePos;
        while (mvControlling->MainCtrlPos) // samo zapêtlenie DecSpeed() nie wystarcza :/
            DecSpeed(true); // wymuszenie zerowania nastawnika jazdy
        while (mvOccupied->ActiveDir < 0)
            mvOccupied->DirectionForward(); // kierunek na 0
        while (mvOccupied->ActiveDir > 0)
            mvOccupied->DirectionBackward();
        if (TestFlag(d->MoverParameters->Couplers[iDirectionOrder < 0 ? 1 : 0].CouplingFlag,
                     ctrain_controll))
        {
            mvControlling->MainSwitch(
                false); // dezaktywacja czuwaka, jeœli przejœcie do innego cz³onu
            mvOccupied->DecLocalBrakeLevel(10); // zwolnienie hamulca w opuszczanym pojeŸdzie
            //   mvOccupied->BrakeLevelSet((mvOccupied->BrakeHandle==FVel6)?4:-2); //odciêcie na
            //   zaworze maszynisty, FVel6 po drugiej stronie nie luzuje
            mvOccupied->BrakeLevelSet(
                mvOccupied->Handle->GetPos(bh_NP)); // odciêcie na zaworze maszynisty
        }
        mvOccupied->ActiveCab = mvOccupied->CabNo; // u¿ytkownik moze zmieniæ ActiveCab wychodz¹c
        mvOccupied->CabDeactivisation(); // tak jest w Train.cpp
        // przejœcie AI na drug¹ stronê EN57, ET41 itp.
        while (TestFlag(d->MoverParameters->Couplers[iDirection < 0 ? 1 : 0].CouplingFlag,
                        ctrain_controll))
        { // jeœli pojazd z przodu jest ukrotniony, to przechodzimy do niego
            d = iDirection * d->DirectionGet() < 0 ? d->Next() :
                                                     d->Prev(); // przechodzimy do nastêpnego cz³onu
            if (d)
            {
                drugi = d->Mechanik; // zapamiêtanie tego, co ewentualnie tam siedzi, ¿eby w razie
                // dwóch zamieniæ miejscami
                d->Mechanik = this; // na razie bilokacja
                d->MoverParameters->SetInternalCommand(
                    "", 0, 0); // usuniêcie ewentualnie zalegaj¹cej komendy (Change_direction?)
                if (d->DirectionGet() != pVehicle->DirectionGet()) // jeœli s¹ przeciwne do siebie
                    iDirection = -iDirection; // to bêdziemy jechaæ w drug¹ stronê wzglêdem
                // zasiedzianego pojazdu
                pVehicle->Mechanik = drugi; // wsadzamy tego, co ewentualnie by³ (podwójna trakcja)
                pVehicle->MoverParameters->CabNo = 0; // wy³¹czanie kabin po drodze
                pVehicle->MoverParameters->ActiveCab = 0; // i zaznaczenie, ¿e nie ma tam nikogo
                pVehicle = d; // a mechu ma nowy pojazd (no, cz³on)
            }
            else
                break; // jak koniec sk³adu, to mechanik dalej nie idzie
        }
        if (pVehicle != old)
        { // jeœli zmieniony zosta³ pojazd prowadzony
            Global::pWorld->CabChange(old, pVehicle); // ewentualna zmiana kabiny u¿ytkownikowi
            ControllingSet(); // utworzenie po³¹czenia do sterowanego pojazdu (mo¿e siê zmieniæ) -
            // silnikowy dla EZT
        }
        if (mvControlling->EngineType ==
            DieselEngine) // dla 2Ls150 - przed ustawieniem kierunku - mo¿na zmieniæ tryb pracy
            if (mvControlling->ShuntModeAllow)
                mvControlling->CurrentSwitch(
                    (OrderList[OrderPos] & Shunt) ||
                    (fMass > 224000.0)); // do tego na wzniesieniu mo¿e nie daæ rady na liniowym
        // Ra: to prze³¹czanie poni¿ej jest tu bez sensu
        mvOccupied->ActiveCab =
            iDirection; // aktywacja kabiny w prowadzonym poje¿dzie (silnikowy mo¿e byæ odwrotnie?)
        // mvOccupied->CabNo=iDirection;
        // mvOccupied->ActiveDir=0; //¿eby sam ustawi³ kierunek
        mvOccupied->CabActivisation(); // uruchomienie kabin w cz³onach
        DirectionForward(true); // nawrotnik do przodu
        if (brake) // hamowanie tylko jeœli by³ wczeœniej zahamowany (bo mo¿liwe, ¿e jedzie!)
            mvOccupied->IncLocalBrakeLevel(brake); // zahamuj jak wczeœniej
        CheckVehicles(); // sprawdzenie sk³adu, AI zapali œwiat³a
        TableClear(); // resetowanie tabelki skanowania torów
    }
};

void TController::AutoRewident()
{ // autorewident: nastawianie hamulców w sk³adzie
    int r = 0, g = 0, p = 0; // iloœci wagonów poszczególnych typów
    TDynamicObject *d = pVehicles[0]; // pojazd na czele sk³adu
    // 1. Zebranie informacji o sk³adzie poci¹gu — przejœcie wzd³u¿ sk³adu i odczyt parametrów:
    //   · iloœæ wagonów -> s¹ zliczane, wszystkich pojazdów jest (iVehicles)
    //   · d³ugoœæ (jako suma) -> jest w (fLength)
    //   · masa (jako suma) -> jest w (fMass)
    while (d)
    { // klasyfikacja pojazdów wg BrakeDelays i mocy (licznik)
        if (d->MoverParameters->Power <
            1) // - lokomotywa - Power>1 - ale mo¿e byæ nieczynna na koñcu...
            if (TestFlag(d->MoverParameters->BrakeDelays, bdelay_R))
                ++r; // - wagon pospieszny - jest R
            else if (TestFlag(d->MoverParameters->BrakeDelays, bdelay_G))
                ++g; // - wagon towarowy - jest G (nie ma R)
            else
                ++p; // - wagon osobowy - reszta (bez G i bez R)
        d = d->Next(); // kolejny pojazd, pod³¹czony od ty³u (licz¹c od czo³a)
    }
    // 2. Okreœlenie typu poci¹gu i nastawy:
    int ustaw; //+16 dla pasa¿erskiego
    if (r + g + p == 0)
        ustaw = 16 + bdelay_R; // lokomotywa luzem (mo¿e byæ wielocz³onowa)
    else
    { // jeœli s¹ wagony
        ustaw = (g < min(4, r + p) ? 16 : 0);
        if (ustaw) // jeœli towarowe < Min(4, pospieszne+osobowe)
        { // to sk³ad pasa¿erski - nastawianie pasa¿erskiego
            ustaw += (g && (r < g + p)) ? bdelay_P : bdelay_R;
            // je¿eli towarowe>0 oraz pospiesze<=towarowe+osobowe to P (0)
            // inaczej R (2)
        }
        else
        { // inaczej towarowy - nastawianie towarowego
            if ((fLength < 300.0) && (fMass < 600000.0)) //[kg]
                ustaw |= bdelay_P; // je¿eli d³ugoœæ<300 oraz masa<600 to P (0)
            else if ((fLength < 500.0) && (fMass < 1300000.0))
                ustaw |= bdelay_R; // je¿eli d³ugoœæ<500 oraz masa<1300 to GP (2)
            else
                ustaw |= bdelay_G; // inaczej G (1)
        }
        // zasadniczo na sieci PKP kilka lat temu na P/GP jeŸdzi³y tylko kontenerowce o
        // rozk³adowej 90 km/h. Pozosta³e jeŸdzi³y 70 km/h i by³y nastawione na G.
    }
    d = pVehicles[0]; // pojazd na czele sk³adu
    p = 0; // bêdziemy tu liczyæ wagony od lokomotywy dla nastawy GP
    while (d)
    { // 3. Nastawianie
        switch (ustaw)
        {
        case bdelay_P: // towarowy P - lokomotywa na G, reszta na P.
            d->MoverParameters->BrakeDelaySwitch(d->MoverParameters->Power > 1 ? bdelay_G :
                                                                                 bdelay_P);
            break;
        case bdelay_G: // towarowy G - wszystko na G, jeœli nie ma to P (powinno siê wy³¹czyæ
            // hamulec)
            d->MoverParameters->BrakeDelaySwitch(
                TestFlag(d->MoverParameters->BrakeDelays, bdelay_G) ? bdelay_G : bdelay_P);
            break;
        case bdelay_R: // towarowy GP - lokomotywa oraz 5 pierwszych pojazdów przy niej na G, reszta
            // na P
            if (d->MoverParameters->Power > 1)
            {
                d->MoverParameters->BrakeDelaySwitch(bdelay_G);
                p = 0; // a jak bêdzie druga w œrodku?
            }
            else
                d->MoverParameters->BrakeDelaySwitch(++p <= 5 ? bdelay_G : bdelay_P);
            break;
        case 16 + bdelay_R: // pasa¿erski R - na R, jeœli nie ma to P
            d->MoverParameters->BrakeDelaySwitch(
                TestFlag(d->MoverParameters->BrakeDelays, bdelay_R) ? bdelay_R : bdelay_P);
            break;
        case 16 + bdelay_P: // pasa¿erski P - wszystko na P
            d->MoverParameters->BrakeDelaySwitch(bdelay_P);
            break;
        }
        d = d->Next(); // kolejny pojazd, pod³¹czony od ty³u (licz¹c od czo³a)
    }
};

bool TController::CheckVehicles(TOrders user)
{ // sprawdzenie stanu posiadanych pojazdów w sk³adzie i zapalenie œwiate³
    TDynamicObject *p; // roboczy wskaŸnik na pojazd
    iVehicles = 0; // iloœæ pojazdów w sk³adzie
    int d = mvOccupied->DirAbsolute; // który sprzêg jest z przodu
    if (!d) // jeœli nie ma ustalonego kierunku
        d = mvOccupied->CabNo; // to jedziemy wg aktualnej kabiny
    iDirection = d; // ustalenie kierunku jazdy (powinno zrobiæ PrepareEngine?)
    d = d >= 0 ? 0 : 1; // kierunek szukania czo³a (numer sprzêgu)
    pVehicles[0] = p = pVehicle->FirstFind(d); // pojazd na czele sk³adu
    // liczenie pojazdów w sk³adzie i ustalenie parametrów
    int dir = d = 1 - d; // a dalej bêdziemy zliczaæ od czo³a do ty³u
    fLength = 0.0; // d³ugoœæ sk³adu do badania wyjechania za ograniczenie
    fMass = 0.0; // ca³kowita masa do liczenia stycznej sk³adowej grawitacji
    fVelMax = -1; // ustalenie prêdkoœci dla sk³adu
    bool main = true; // czy jest g³ównym steruj¹cym
    iDrivigFlags |= moveOerlikons; // zak³adamy, ¿e s¹ same Oerlikony
    // Ra 2014-09: ustawiæ moveMultiControl, jeœli wszystkie s¹ w ukrotnieniu (i skrajne maj¹
    // kabinê?)
    while (p)
    { // sprawdzanie, czy jest g³ównym steruj¹cym, ¿eby nie by³o konfliktu
        if (p->Mechanik) // jeœli ma obsadê
            if (p->Mechanik != this) // ale chodzi o inny pojazd, ni¿ aktualnie sprawdzaj¹cy
                if (p->Mechanik->iDrivigFlags & movePrimary) // a tamten ma priorytet
                    if ((iDrivigFlags & movePrimary) && (mvOccupied->DirAbsolute) &&
                        (mvOccupied->BrakeCtrlPos >= -1)) // jeœli rz¹dzi i ma kierunek
                        p->Mechanik->iDrivigFlags &= ~movePrimary; // dezaktywuje tamtego
                    else
                        main = false; // nici z rz¹dzenia
        ++iVehicles; // jest jeden pojazd wiêcej
        pVehicles[1] = p; // zapamiêtanie ostatniego
        fLength += p->MoverParameters->Dim.L; // dodanie d³ugoœci pojazdu
        fMass += p->MoverParameters->TotalMass; // dodanie masy ³¹cznie z ³adunkiem
        if (fVelMax < 0 ? true : p->MoverParameters->Vmax < fVelMax)
            fVelMax = p->MoverParameters->Vmax; // ustalenie maksymalnej prêdkoœci dla sk³adu
        /* //youBy: bez przesady, to jest proteza, napelniac mozna, a nawet trzeba, ale z umiarem!
          //uwzglêdniæ jeszcze wy³¹czenie hamulca
          if
          ((p->MoverParameters->BrakeSystem!=Pneumatic)&&(p->MoverParameters->BrakeSystem!=ElectroPneumatic))
           iDrivigFlags&=~moveOerlikons; //no jednak nie
          else if (p->MoverParameters->BrakeSubsystem!=Oerlikon)
           iDrivigFlags&=~moveOerlikons; //wtedy te¿ nie */
        p = p->Neightbour(dir); // pojazd pod³¹czony od wskazanej strony
    }
    if (main)
        iDrivigFlags |= movePrimary; // nie znaleziono innego, mo¿na siê porz¹dziæ
    /* //tabelka z list¹ pojazdów jest na razie nie potrzebna
     delete[] pVehicles;
     pVehicles=new TDynamicObject*[iVehicles];
    */
    ControllingSet(); // ustalenie cz³onu do sterowania (mo¿e byæ inny ni¿ zasiedziany)
    int pantmask = 1;
    if (iDrivigFlags & movePrimary)
    { // jeœli jest aktywnie prowadz¹cym pojazd, mo¿e zrobiæ w³asny porz¹dek
        p = pVehicles[0];
        while (p)
        {
            if (TrainParams)
                if (p->asDestination == "none")
                    p->DestinationSet(TrainParams->Relation2); // relacja docelowa, jeœli nie by³o
            if (AIControllFlag) // jeœli prowadzi komputer
                p->RaLightsSet(0, 0); // gasimy œwiat³a
            if (p->MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
            { // jeœli pojazd posiada pantograf, to przydzielamy mu maskê, któr¹ bêdzie informowa³ o
                // jeŸdzie bezpr¹dowej
                p->iOverheadMask = pantmask;
                pantmask << 1; // przesuniêcie bitów, max. 32 pojazdy z pantografami w sk³adzie
            }
            d = p->DirectionSet(d ? 1 : -1); // zwraca po³o¿enie nastêpnego (1=zgodny,0=odwrócony -
            // wzglêdem czo³a sk³adu)
            p->fScanDist = 300.0; // odleg³oœæ skanowania w poszukiwaniu innych pojazdów
            p->ctOwner = this; // dominator oznacza swoje terytorium
            p = p->Next(); // pojazd pod³¹czony od ty³u (licz¹c od czo³a)
        }
        if (AIControllFlag)
        { // jeœli prowadzi komputer
            if (OrderCurrentGet() == Obey_train) // jeœli jazda poci¹gowa
            {
                Lights(1 + 4 + 16, 2 + 32 + 64); //œwiat³a poci¹gowe (Pc1) i koñcówki (Pc5)
#if LOGPRESS == 0
                AutoRewident(); // nastawianie hamulca do jazdy poci¹gowej
#endif
            }
            else if (OrderCurrentGet() & (Shunt | Connect))
            {
                Lights(16, (pVehicles[1]->MoverParameters->CabNo) ?
                               1 :
                               0); //œwiat³a manewrowe (Tb1) na pojeŸdzie z napêdem
                if (OrderCurrentGet() & Connect) // jeœli ³¹czenie, skanowaæ dalej
                    pVehicles[0]->fScanDist =
                        5000.0; // odleg³oœæ skanowania w poszukiwaniu innych pojazdów
            }
            else if (OrderCurrentGet() == Disconnect)
                if (mvOccupied->ActiveDir > 0) // jak ma kierunek do przodu
                    Lights(16, 0); //œwiat³a manewrowe (Tb1) tylko z przodu, aby nie pozostawiæ
                // odczepionego ze œwiat³em
                else // jak dociska
                    Lights(0, 16); //œwiat³a manewrowe (Tb1) tylko z przodu, aby nie pozostawiæ
            // odczepionego ze œwiat³em
        }
        else // Ra 2014-02: lepiej tu ni¿ w pêtli obs³uguj¹cej komendy, bo tam siê zmieni informacja
            // o sk³adzie
            switch (user) // gdy cz³owiek i gdy nast¹pi³o po³¹cznie albo roz³¹czenie
            {
            case Change_direction:
                while (OrderCurrentGet() & (Change_direction))
                    JumpToNextOrder(); // zmianê kierunku te¿ mo¿na olaæ, ale zmieniæ kierunek
                // skanowania!
                break;
            case Connect:
                while (OrderCurrentGet() & (Change_direction))
                    JumpToNextOrder(); // zmianê kierunku te¿ mo¿na olaæ, ale zmieniæ kierunek
                // skanowania!
                if (OrderCurrentGet() & (Connect))
                { // jeœli mia³o byæ ³¹czenie, zak³adamy, ¿e jest dobrze (sprawdziæ?)
                    iCoupler = 0; // koniec z doczepianiem
                    iDrivigFlags &= ~moveConnect; // zdjêcie flagi doczepiania
                    JumpToNextOrder(); // wykonanie nastêpnej komendy
                    if (OrderCurrentGet() & (Change_direction))
                        JumpToNextOrder(); // zmianê kierunku te¿ mo¿na olaæ, ale zmieniæ kierunek
                    // skanowania!
                }
                break;
            case Disconnect:
                while (OrderCurrentGet() & (Change_direction))
                    JumpToNextOrder(); // zmianê kierunku te¿ mo¿na olaæ, ale zmieniæ kierunek
                // skanowania!
                if (OrderCurrentGet() & (Disconnect))
                { // wypada³o by sprawdziæ, czy odczepiono wagony w odpowiednim miejscu
                    // (iVehicleCount)
                    JumpToNextOrder(); // wykonanie nastêpnej komendy
                    if (OrderCurrentGet() & (Change_direction))
                        JumpToNextOrder(); // zmianê kierunku te¿ mo¿na olaæ, ale zmieniæ kierunek
                    // skanowania!
                }
            }
        // Ra 2014-09: tymczasowo prymitywne ustawienie warunku pod k¹tem SN61
        if ((mvOccupied->TrainType == dt_EZT) || (iVehicles == 1))
            iDrivigFlags |= movePushPull; // zmiana czo³a przez zmianê kabiny
        else
            iDrivigFlags &= ~movePushPull; // zmiana czo³a przez manewry
    } // blok wykonywany, gdy aktywnie prowadzi
    return true;
}

void TController::Lights(int head, int rear)
{ // zapalenie œwiate³ w sk³¹dzie
    pVehicles[0]->RaLightsSet(head, -1); // zapalenie przednich w pierwszym
    pVehicles[1]->RaLightsSet(-1, rear); // zapalenie koñcówek w ostatnim
}

void TController::DirectionInitial()
{ // ustawienie kierunku po wczytaniu trainset (mo¿e jechaæ na wstecznym
    mvOccupied->CabActivisation(); // za³¹czenie rozrz¹du (wirtualne kabiny)
    if (mvOccupied->Vel > 0.0)
    { // jeœli na starcie jedzie
        iDirection = iDirectionOrder =
            (mvOccupied->V > 0 ? 1 : -1); // pocz¹tkowa prêdkoœæ wymusza kierunek jazdy
        DirectionForward(mvOccupied->V * mvOccupied->CabNo >= 0.0); // a dalej ustawienie nawrotnika
    }
    CheckVehicles(); // sprawdzenie œwiate³ oraz skrajnych pojazdów do skanowania
};

int TController::OrderDirectionChange(int newdir, TMoverParameters *Vehicle)
{ // zmiana kierunku jazdy, niezale¿nie od kabiny
    int testd = newdir;
    if (Vehicle->Vel < 0.5)
    { // jeœli prawie stoi, mo¿na zmieniæ kierunek, musi byæ wykonane dwukrotnie, bo za pierwszym
        // razem daje na zero
        switch (newdir * Vehicle->CabNo)
        { // DirectionBackward() i DirectionForward() to zmiany wzglêdem kabiny
        case -1: // if (!Vehicle->DirectionBackward()) testd=0; break;
            DirectionForward(false);
            break;
        case 1: // if (!Vehicle->DirectionForward()) testd=0; break;
            DirectionForward(true);
            break;
        }
        if (testd == 0)
            VelforDriver = -1; // kierunek zosta³ zmieniony na ¿¹dany, mo¿na jechaæ
    }
    else // jeœli jedzie
        VelforDriver = 0; // ma siê zatrzymaæ w celu zmiany kierunku
    if ((Vehicle->ActiveDir == 0) && (VelforDriver < Vehicle->Vel)) // Ra: to jest chyba bez sensu
        IncBrake(); // niech hamuje
    if (Vehicle->ActiveDir == testd * Vehicle->CabNo)
        VelforDriver = -1; // mo¿na jechaæ, bo kierunek jest zgodny z ¿¹danym
    if (Vehicle->TrainType == dt_EZT)
        if (Vehicle->ActiveDir > 0)
            // if () //tylko jeœli jazda poci¹gowa (tego nie wiemy w momencie odpalania silnika)
            Vehicle->DirectionForward(); // Ra: z przekazaniem do silnikowego
    return (int)VelforDriver; // zwraca prêdkoœæ mechanika
}

void TController::WaitingSet(double Seconds)
{ // ustawienie odczekania po zatrzymaniu (ustawienie w trakcie jazdy zatrzyma)
    fStopTime = -Seconds; // ujemna wartoœæ oznacza oczekiwanie (potem >=0.0)
}

void TController::SetVelocity(double NewVel, double NewVelNext, TStopReason r)
{ // ustawienie nowej prêdkoœci
    WaitingTime = -WaitingExpireTime; // przypisujemy -WaitingExpireTime, a potem porównujemy z
    // zerem
    MaxVelFlag = False; // Ra: to nie jest u¿ywane
    MinVelFlag = False; // Ra: to nie jest u¿ywane
    /* nie u¿ywane
     if ((NewVel>NewVelNext) //jeœli oczekiwana wiêksza ni¿ nastêpna
      || (NewVel<mvOccupied->Vel)) //albo aktualna jest mniejsza ni¿ aktualna
      fProximityDist=-800.0; //droga hamowania do zmiany prêdkoœci
     else
      fProximityDist=-300.0; //Ra: ujemne wartoœci s¹ ignorowane
    */
    if (NewVel == 0.0) // jeœli ma stan¹æ
    {
        if (r != stopNone) // a jest powód podany
            eStopReason = r; // to zapamiêtaæ nowy powód
    }
    else
    {
        eStopReason = stopNone; // podana prêdkoœæ, to nie ma powodów do stania
        // to ca³e poni¿ej to warunki zatr¹bienia przed ruszeniem
        if (OrderList[OrderPos] ?
                OrderList[OrderPos] & (Obey_train | Shunt | Connect | Prepare_engine) :
                true) // jeœli jedzie w dowolnym trybie
            if ((mvOccupied->Vel <
                 1.0)) // jesli stoi (na razie, bo chyba powinien te¿, gdy hamuje przed semaforem)
                if (iDrivigFlags & moveStartHorn) // jezeli tr¹bienie w³¹czone
                    if (!(iDrivigFlags & (moveStartHornDone | moveConnect))) // jeœli nie zatr¹bione
                        // i nie jest to moment
                        // pod³¹czania sk³adu
                        if (mvOccupied->CategoryFlag & 1) // tylko poci¹gi tr¹bi¹ (unimogi tylko na
                            // torach, wiêc trzeba raczej sprawdzaæ
                            // tor)
                            if ((NewVel >= 1.0) || (NewVel < 0.0)) // o ile prêdkoœæ jest znacz¹ca
                            { // fWarningDuration=0.3; //czas tr¹bienia
                                // if (AIControllFlag) //jak siedzi krasnoludek, to w³¹czy tr¹bienie
                                // mvOccupied->WarningSignal=pVehicle->iHornWarning; //wysokoœæ tonu
                                // (2=wysoki)
                                // iDrivigFlags|=moveStartHornDone; //nie tr¹biæ a¿ do ruszenia
                                iDrivigFlags |= moveStartHornNow; // zatr¹b po odhamowaniu
                            }
    }
    VelSignal = NewVel; // prêdkoœæ zezwolona na aktualnym odcinku
    VelNext = NewVelNext; // prêdkoœæ przy nastêpnym obiekcie
}

/* //funkcja do niczego nie potrzebna (ew. do przesuniêcia pojazdu o odleg³oœæ NewDist)
bool TController::SetProximityVelocity(double NewDist,double NewVelNext)
{//informacja o prêdkoœci w pewnej odleg³oœci
#if 0
 if (NewVelNext==0.0)
  WaitingTime=0.0; //nie trzeba ju¿ czekaæ
 //if ((NewVelNext>=0)&&((VelNext>=0)&&(NewVelNext<VelNext))||(NewVelNext<VelSignal))||(VelNext<0))
 {MaxVelFlag=False; //Ra: to nie jest u¿ywane
  MinVelFlag=False; //Ra: to nie jest u¿ywane
  VelNext=NewVelNext;
  fProximityDist=NewDist; //dodatnie: przeliczyæ do punktu; ujemne: wzi¹æ dos³ownie
  return true;
 }
 //else return false
#endif
}
*/

void TController::SetDriverPsyche()
{
    // double maxdist=0.5; //skalowanie dystansu od innego pojazdu, zmienic to!!!
    if ((Psyche == Aggressive) && (OrderList[OrderPos] == Obey_train))
    {
        ReactionTime = HardReactionTime; // w zaleznosci od charakteru maszynisty
        // if (pOccupied)
        if (mvOccupied->CategoryFlag & 2)
        {
            WaitingExpireTime = 1; // tyle ma czekaæ samochód, zanim siê ruszy
            AccPreferred = 3.0; //[m/ss] agresywny
        }
        else
        {
            WaitingExpireTime = 61; // tyle ma czekaæ, zanim siê ruszy
            AccPreferred = HardAcceleration; // agresywny
        }
    }
    else
    {
        ReactionTime = EasyReactionTime; // spokojny
        if (mvOccupied->CategoryFlag & 2)
        {
            WaitingExpireTime = 3; // tyle ma czekaæ samochód, zanim siê ruszy
            AccPreferred = 2.0; //[m/ss]
        }
        else
        {
            WaitingExpireTime = 65; // tyle ma czekaæ, zanim siê ruszy
            AccPreferred = EasyAcceleration;
        }
    }
    if (mvControlling && mvOccupied)
    { // with Controlling do
        if (mvControlling->MainCtrlPos < 3)
            ReactionTime = mvControlling->InitialCtrlDelay + ReactionTime;
        if (mvOccupied->BrakeCtrlPos > 1)
            ReactionTime = 0.5 * ReactionTime;
        /*
          if (mvOccupied->Vel>0.1) //o ile jedziemy
          {//sprawdzenie jazdy na widocznoœæ
           TCoupling
          *c=pVehicles[0]->MoverParameters->Couplers+(pVehicles[0]->DirectionGet()>0?0:1); //sprzêg
          z przodu sk³adu
           if (c->Connected) //a mamy coœ z przodu
            if (c->CouplingFlag==0) //jeœli to coœ jest pod³¹czone sprzêgiem wirtualnym
            {//wyliczanie optymalnego przyspieszenia do jazdy na widocznoœæ (Ra: na pewno tutaj?)
             double k=c->Connected->Vel; //prêdkoœæ pojazdu z przodu (zak³adaj¹c, ¿e jedzie w tê
          sam¹ stronê!!!)
             if (k<=mvOccupied->Vel) //porównanie modu³ów prêdkoœci [km/h]
             {if (pVehicles[0]->fTrackBlock<fMaxProximityDist) //porównianie z minimaln¹ odleg³oœci¹
          kolizyjn¹
               k=-AccPreferred; //hamowanie maksymalne, bo jest za blisko
              else
              {//jeœli tamten jedzie szybciej, to nie potrzeba modyfikowaæ przyspieszenia
               double d=(pVehicles[0]->fTrackBlock-0.5*fabs(mvOccupied->V)-fMaxProximityDist);
          //bezpieczna odleg³oœæ za poprzednim
               //a=(v2*v2-v1*v1)/(25.92*(d-0.5*v1))
               //(v2*v2-v1*v1)/2 to ró¿nica energii kinetycznych na jednostkê masy
               //jeœli v2=50km/h,v1=60km/h,d=200m => k=(192.9-277.8)/(25.92*(200-0.5*16.7)=-0.0171
          [m/s^2]
               //jeœli v2=50km/h,v1=60km/h,d=100m => k=(192.9-277.8)/(25.92*(100-0.5*16.7)=-0.0357
          [m/s^2]
               //jeœli v2=50km/h,v1=60km/h,d=50m  => k=(192.9-277.8)/(25.92*( 50-0.5*16.7)=-0.0786
          [m/s^2]
               //jeœli v2=50km/h,v1=60km/h,d=25m  => k=(192.9-277.8)/(25.92*( 25-0.5*16.7)=-0.1967
          [m/s^2]
               if (d>0) //bo jak ujemne, to zacznie przyspieszaæ, aby siê zderzyæ
                k=(k*k-mvOccupied->Vel*mvOccupied->Vel)/(25.92*d); //energia kinetyczna dzielona
          przez masê i drogê daje przyspieszenie
               else
                k=0.0; //mo¿e lepiej nie przyspieszaæ -AccPreferred; //hamowanie
               //WriteLog(pVehicle->asName+" "+AnsiString(k));
              }
              if (d<fBrakeDist) //bo z daleka nie ma co hamowaæ
               AccPreferred=Min0R(k,AccPreferred);
             }
            }
          }
        */
    }
};

bool TController::PrepareEngine()
{ // odpalanie silnika
    bool OK;
    bool voltfront, voltrear;
    voltfront = false;
    voltrear = false;
    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;
    iDrivigFlags |= moveActive; // mo¿e skanowaæ sygna³y i reagowaæ na komendy
    // with Controlling do
    if (((mvControlling->EnginePowerSource.SourceType ==
          CurrentCollector) /*||(mvOccupied->TrainType==dt_EZT)*/))
    {
        if (mvControlling
                ->GetTrainsetVoltage()) // sprawdzanie, czy zasilanie jest mo¿e w innym cz³onie
        {
            voltfront = true;
            voltrear = true;
        }
    }
    //   begin
    //     if Couplers[0].Connected<>nil)
    //     begin
    //       if Couplers[0].Connected^.PantFrontVolt or Couplers[0].Connected^.PantRearVolt)
    //         voltfront:=true
    //       else
    //         voltfront:=false;
    //     end
    //     else
    //        voltfront:=false;
    //     if Couplers[1].Connected<>nil)
    //     begin
    //      if Couplers[1].Connected^.PantFrontVolt or Couplers[1].Connected^.PantRearVolt)
    //        voltrear:=true
    //      else
    //        voltrear:=false;
    //     end
    //     else
    //        voltrear:=false;
    //   end
    else
        // if EnginePowerSource.SourceType<>CurrentCollector)
        if (mvOccupied->TrainType != dt_EZT)
        voltfront = true; // Ra 2014-06: to jest wirtualny pr¹d dla spalinowych???
    if (AIControllFlag) // jeœli prowadzi komputer
    { // czêœæ wykonawcza dla sterowania przez komputer
        mvOccupied->BatterySwitch(true);
        if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
        { // jeœli silnikowy jest pantografuj¹cym
            if (mvControlling->PantPress > 4.3)
            { // je¿eli jest wystarczaj¹ce ciœnienie w pantografach
                if ((!mvControlling->bPantKurek3) ||
                    (mvControlling->PantPress <=
                     mvControlling->ScndPipePress)) // kurek prze³¹czony albo g³ówna ju¿ pompuje
                    mvControlling->PantCompFlag = false; // sprê¿arkê pantografów mo¿na ju¿ wy³¹czyæ
                mvControlling->PantFront(true);
                mvControlling->PantRear(true);
            }
            else if (mvControlling->PantPress < 4.2) //¿eby nie za³¹cza³ zaraz po przekroczeniu 4.0
            { // za³¹czenie ma³ej sprê¿arki
                mvControlling->bPantKurek3 =
                    false; // od³¹czenie zbiornika g³ównego, bo z nim nie da rady napompowaæ
                mvControlling->PantCompFlag = true; // za³¹czenie sprê¿arki pantografów
            }
        }
        // if (mvOccupied->TrainType==dt_EZT)
        //{//Ra 2014-12: po co to tutaj?
        // mvControlling->PantFront(true);
        // mvControlling->PantRear(true);
        //}
        // if (mvControlling->EngineType==DieselElectric)
        // mvControlling->Battery=true; //Ra: to musi byæ tak?
    }
    if (mvControlling->PantFrontVolt || mvControlling->PantRearVolt || voltfront || voltrear)
    { // najpierw ustalamy kierunek, jeœli nie zosta³ ustalony
        if (!iDirection) // jeœli nie ma ustalonego kierunku
            if (mvOccupied->V == 0)
            { // ustalenie kierunku, gdy stoi
                iDirection = mvOccupied->CabNo; // wg wybranej kabiny
                if (!iDirection) // jeœli nie ma ustalonego kierunku
                    if ((mvControlling->PantFrontVolt != 0.0) ||
                        (mvControlling->PantRearVolt != 0.0) || voltfront || voltrear)
                    {
                        if (mvOccupied->Couplers[1].CouplingFlag ==
                            ctrain_virtual) // jeœli z ty³u nie ma nic
                            iDirection = -1; // jazda w kierunku sprzêgu 1
                        if (mvOccupied->Couplers[0].CouplingFlag ==
                            ctrain_virtual) // jeœli z przodu nie ma nic
                            iDirection = 1; // jazda w kierunku sprzêgu 0
                    }
            }
            else // ustalenie kierunku, gdy jedzie
                if ((mvControlling->PantFrontVolt != 0.0) || (mvControlling->PantRearVolt != 0.0) ||
                    voltfront || voltrear)
                if (mvOccupied->V < 0) // jedzie do ty³u
                    iDirection = -1; // jazda w kierunku sprzêgu 1
                else // jak nie do ty³u, to do przodu
                    iDirection = 1; // jazda w kierunku sprzêgu 0
        if (AIControllFlag) // jeœli prowadzi komputer
        { // czêœæ wykonawcza dla sterowania przez komputer
            if (mvControlling->ConvOvldFlag)
            { // wywali³ bezpiecznik nadmiarowy przetwornicy
                while (DecSpeed(true))
                    ; // zerowanie napêdu
                mvControlling->ConvOvldFlag = false; // reset nadmiarowego
            }
            else if (!mvControlling->Mains)
            {
                // if TrainType=dt_SN61)
                //   begin
                //      OK:=(OrderDirectionChange(ChangeDir,Controlling)=-1);
                //      OK:=IncMainCtrl(1);
                //   end;
                while (DecSpeed(true))
                    ; // zerowanie napêdu
                OK = mvControlling->MainSwitch(true);
                if (mvControlling->EngineType == DieselEngine)
                { // Ra 2014-06: dla SN61 trzeba wrzuciæ pierwsz¹ pozycjê - nie wiem, czy tutaj...
                    // kiedyœ dzia³a³o...
                    if (!mvControlling->MainCtrlPos)
                    {
                        if (mvControlling->RList[0].R ==
                            0.0) // gdy na pozycji 0 dawka paliwa jest zerowa, to zgaœnie
                            mvControlling->IncMainCtrl(1); // dlatego trzeba zwiêkszyæ pozycjê
                        if (!mvControlling->ScndCtrlPos) // jeœli bieg nie zosta³ ustawiony
                            if (!mvControlling->MotorParam[0].AutoSwitch) // gdy biegi rêczne
                                if (mvControlling->MotorParam[0].mIsat == 0.0) // bl,mIsat,fi,mfi
                                    mvControlling->IncScndCtrl(1); // pierwszy bieg
                    }
                }
            }
            else
            { // Ra: iDirection okreœla, w któr¹ stronê jedzie sk³ad wzglêdem sprzêgów pojazdu z AI
                OK = (OrderDirectionChange(iDirection, mvOccupied) == -1);
                // w EN57 sprê¿arka w ra jest zasilana z silnikowego
                mvControlling->CompressorSwitch(true);
                mvControlling->ConverterSwitch(true);
                mvControlling->CompressorSwitch(true);
            }
        }
        else
            OK = mvControlling->Mains;
    }
    else
        OK = false;
    OK = OK && (mvOccupied->ActiveDir != 0) && (mvControlling->CompressorAllow);
    if (OK)
    {
        if (eStopReason == stopSleep) // jeœli dotychczas spa³
            eStopReason == stopNone; // teraz nie ma powodu do stania
        iEngineActive = 1;
        return true;
    }
    else
    {
        iEngineActive = 0;
        return false;
    }
};

bool TController::ReleaseEngine()
{ // wy³¹czanie silnika (test wy³¹czenia, a czêœæ wykonawcza tylko jeœli steruje komputer)
    bool OK = false;
    LastReactionTime = 0.0;
    ReactionTime = PrepareTime;
    if (AIControllFlag)
    { // jeœli steruje komputer
        if (mvOccupied->DoorOpenCtrl == 1)
        { // zamykanie drzwi
            if (mvOccupied->DoorLeftOpened)
                mvOccupied->DoorLeft(false);
            if (mvOccupied->DoorRightOpened)
                mvOccupied->DoorRight(false);
        }
        if (mvOccupied->ActiveDir == 0)
            if (mvControlling->Mains)
            {
                mvControlling->CompressorSwitch(false);
                mvControlling->ConverterSwitch(false);
                if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
                {
                    mvControlling->PantFront(false);
                    mvControlling->PantRear(false);
                }
                OK = mvControlling->MainSwitch(false);
            }
            else
                OK = true;
    }
    else if (mvOccupied->ActiveDir == 0)
        OK = mvControlling->Mains; // tylko to testujemy dla pojazdu cz³owieka
    if (AIControllFlag)
        if (!mvOccupied->DecBrakeLevel()) // tu moze zmieniaæ na -2, ale to bez znaczenia
            if (!mvOccupied->IncLocalBrakeLevel(1))
            {
                while (DecSpeed(true))
                    ; // zerowanie nastawników
                while (mvOccupied->ActiveDir > 0)
                    mvOccupied->DirectionBackward();
                while (mvOccupied->ActiveDir < 0)
                    mvOccupied->DirectionForward();
            }
    OK = OK && (mvOccupied->Vel < 0.01);
    if (OK)
    { // jeœli siê zatrzyma³
        iEngineActive = 0;
        eStopReason = stopSleep; // stoimy z powodu wy³¹czenia
        eAction = actSleep; //œpi (wygaszony)
        if (AIControllFlag)
        {
            Lights(0, 0); // gasimy œwiat³a
            mvOccupied->BatterySwitch(false);
        }
        OrderNext(Wait_for_orders); //¿eby nie próbowa³ coœ robiæ dalej
        TableClear(); // zapominamy ograniczenia
        iDrivigFlags &= ~moveActive; // ma nie skanowaæ sygna³ów i nie reagowaæ na komendy
    }
    return OK;
}

bool TController::IncBrake()
{ // zwiêkszenie hamowania
    bool OK = false;
    switch (mvOccupied->BrakeSystem)
    {
    case Individual:
		if (mvOccupied->LocalBrake == ManualBrake)
			OK = mvOccupied->IncManualBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
		else
			OK = mvOccupied->IncLocalBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
        break;
    case Pneumatic:
        if ((mvOccupied->Couplers[0].Connected == NULL) &&
            (mvOccupied->Couplers[1].Connected == NULL))
            OK = mvOccupied->IncLocalBrakeLevel(
                1 + floor(0.5 + fabs(AccDesired))); // hamowanie lokalnym bo luzem jedzie
        else
        {
            if (mvOccupied->BrakeCtrlPos + 1 == mvOccupied->BrakeCtrlPosNo)
            {
                if (AccDesired < -1.5) // hamowanie nagle
                    OK = mvOccupied->IncBrakeLevel();
                else
                    OK = false;
            }
            else
            {
                /*
                    if (AccDesired>-0.2) and ((Vel<20) or (Vel-VelNext<10)))
                            begin
                              if BrakeCtrlPos>0)
                               OK:=IncBrakeLevel
                              else;
                               OK:=IncLocalBrakeLevel(1);   //finezyjne hamowanie lokalnym
                             end
                           else
                */
                // dodane dla towarowego
                if (mvOccupied->BrakeDelayFlag == bdelay_G ?
                        -AccDesired * 6.6 > Min0R(2, mvOccupied->BrakeCtrlPos) :
                        true)
                {
                    OK = mvOccupied->IncBrakeLevel();
                }
                else
                    OK = false;
            }
        }
        if (mvOccupied->BrakeCtrlPos > 0)
            mvOccupied->BrakeReleaser(0);
        break;
    case ElectroPneumatic:
		if (mvOccupied->EngineType == ElectricInductionMotor)
		{
			OK = mvOccupied->IncLocalBrakeLevel(1);
		}
		else if (mvOccupied->fBrakeCtrlPos != mvOccupied->Handle->GetPos(bh_EPB))
        {
            mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPB));
            if (mvOccupied->Handle->GetPos(bh_EPR) - mvOccupied->Handle->GetPos(bh_EPN) < 0.1)
                mvOccupied->SwitchEPBrake(1); // to nie chce dzia³aæ
            OK = true;
        }
        else
            OK = false;
        //   if (mvOccupied->BrakeCtrlPos<mvOccupied->BrakeCtrlPosNo)
        //    if
        //    (mvOccupied->BrakePressureTable[mvOccupied->BrakeCtrlPos+1+2].BrakeType==ElectroPneumatic)
        //    //+2 to indeks Pascala
        //     OK=mvOccupied->IncBrakeLevel();
        //    else
        //     OK=false;
    }
    return OK;
}

bool TController::DecBrake()
{ // zmniejszenie si³y hamowania
    bool OK = false;
    switch (mvOccupied->BrakeSystem)
    {
    case Individual:
		if (mvOccupied->LocalBrake == ManualBrake)
			OK = mvOccupied->DecManualBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
		else
			OK = mvOccupied->DecLocalBrakeLevel(1 + floor(0.5 + fabs(AccDesired)));
        break;
    case Pneumatic:
        if (mvOccupied->BrakeCtrlPos > 0)
            OK = mvOccupied->DecBrakeLevel();
        if (!OK)
            OK = mvOccupied->DecLocalBrakeLevel(2);
        if (mvOccupied->PipePress < 3.0)
            Need_BrakeRelease = true;
        break;
    case ElectroPneumatic:
		if (mvOccupied->EngineType == ElectricInductionMotor)
		{
			OK = mvOccupied->DecLocalBrakeLevel(1);
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
{ // zwiêkszenie prêdkoœci; zwraca false, jeœli dalej siê nie da zwiêkszaæ
    if (tsGuardSignal) // jeœli jest dŸwiêk kierownika
        if (tsGuardSignal->GetStatus() & DSBSTATUS_PLAYING) // jeœli gada, to nie jedziemy
            return false;
    bool OK = true;
    if (iDrivigFlags & moveDoorOpened)
        Doors(false); // zamykanie drzwi - tutaj wykonuje tylko AI (zmienia fActionTime)
    if (fActionTime < 0.0) // gdy jest nakaz poczekaæ z jazd¹, to nie ruszaæ
        return false;
    if (mvControlling->SlippingWheels)
        return false; // jak poœlizg, to nie przyspieszamy
    switch (mvOccupied->EngineType)
    {
    case None: // McZapkie-041003: wagon sterowniczy
        if (mvControlling->MainCtrlPosNo > 0) // jeœli ma czym krêciæ
            iDrivigFlags |= moveIncSpeed; // ustawienie flagi jazdy
        return false;
    case ElectricSeriesMotor:
        if (mvControlling->EnginePowerSource.SourceType == CurrentCollector) // jeœli pantografuj¹cy
        {
            if (fOverhead2 >= 0.0) // a jazda bezpr¹dowa ustawiana eventami (albo opuszczenie)
                return false; // to nici z ruszania
            if (iOverheadZero) // jazda bezpr¹dowa z poziomu toru ustawia bity
                return false; // to nici z ruszania
        }
        if (!mvControlling->FuseFlag) //&&mvControlling->StLinFlag) //yBARC
            if ((mvControlling->MainCtrlPos == 0) ||
                (mvControlling->StLinFlag)) // youBy poleci³ dodaæ 2012-09-08 v367
                // na pozycji 0 przejdzie, a na pozosta³ych bêdzie czekaæ, a¿ siê za³¹cz¹ liniowe
                // (zgaœnie DelayCtrlFlag)
                if (Ready || (iDrivigFlags & movePress))
                    if (fabs(mvControlling->Im) <
                        (fReady < 0.4 ? mvControlling->Imin : mvControlling->IminLo))
                    { // Ra: wywala³ nadmiarowy, bo Im mo¿e byæ ujemne; jak nie odhamowany, to nie
                        // przesadzaæ z pr¹dem
                        if ((mvOccupied->Vel <= 30) ||
                            (mvControlling->Imax > mvControlling->ImaxLo) ||
                            (fVoltage + fVoltage <
                             mvControlling->EnginePowerSource.CollectorParameters.MinV +
                                 mvControlling->EnginePowerSource.CollectorParameters.MaxV))
                        { // bocznik na szeregowej przy ciezkich bruttach albo przy wysokim rozruchu
                            // pod górê albo przy niskim napiêciu
                            if (mvControlling->MainCtrlPos ?
                                    mvControlling->RList[mvControlling->MainCtrlPos].R > 0.0 :
                                    true) // oporowa
                            {
                                OK = (mvControlling->DelayCtrlFlag ?
                                          true :
                                          mvControlling->IncMainCtrl(1)); // krêcimy nastawnik jazdy
                                if ((OK) &&
                                    (mvControlling->MainCtrlPos ==
                                     1)) // czekaj na 1 pozycji, zanim siê nie w³¹cz¹ liniowe
                                    iDrivigFlags |= moveIncSpeed;
                                else
                                    iDrivigFlags &= ~moveIncSpeed; // usuniêcie flagi czekania
                            }
                            else // jeœli bezoporowa (z wyj¹tekiem 0)
                                OK = false; // to daæ bocznik
                        }
                        else
                        { // przekroczone 30km/h, mo¿na wejœæ na jazdê równoleg³¹
                            if (mvControlling->ScndCtrlPos) // jeœli ustawiony bocznik
                                if (mvControlling->MainCtrlPos <
                                    mvControlling->MainCtrlPosNo - 1) // a nie jest ostatnia pozycja
                                    mvControlling->DecScndCtrl(2); // to bocznik na zero po chamsku
                            // (ktoœ mia³ to poprawiæ...)
                            OK = mvControlling->IncMainCtrl(1);
                        }
                        if ((mvControlling->MainCtrlPos > 2) &&
                            (mvControlling->Im == 0)) // brak pr¹du na dalszych pozycjach
                            Need_TryAgain = true; // nie za³¹czona lokomotywa albo wywali³
                        // nadmiarowy
                        else if (!OK) // nie da siê wrzuciæ kolejnej pozycji
                            OK = mvControlling->IncScndCtrl(1); // to daæ bocznik
                    }
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case Dumb:
    case DieselElectric:
        if (!mvControlling->FuseFlag)
            if (Ready || (iDrivigFlags & movePress)) //{(BrakePress<=0.01*MaxBrakePress)}
            {
                OK = mvControlling->IncMainCtrl(1);
                if (!OK)
                    OK = mvControlling->IncScndCtrl(1);
            }
        break;
	case ElectricInductionMotor:
		if (!mvControlling->FuseFlag)
			if (Ready || (iDrivigFlags & movePress) || (mvOccupied->ShuntMode)) //{(BrakePress<=0.01*MaxBrakePress)}
			{
				OK = mvControlling->IncMainCtrl(1);
			}
		break;
		break;
	case WheelsDriven:
        if (!mvControlling->CabNo)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) > 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case DieselEngine:
        if (mvControlling->ShuntModeAllow)
        { // dla 2Ls150 mo¿na zmieniæ tryb pracy, jeœli jest w liniowym i nie daje rady (wymaga
            // zerowania kierunku)
            // mvControlling->ShuntMode=(OrderList[OrderPos]&Shunt)||(fMass>224000.0);
        }
        if ((mvControlling->Vel > mvControlling->dizel_minVelfullengage) &&
            (mvControlling->RList[mvControlling->MainCtrlPos].Mn > 0))
            OK = mvControlling->IncMainCtrl(1);
        if (mvControlling->RList[mvControlling->MainCtrlPos].Mn == 0)
            OK = mvControlling->IncMainCtrl(1);
        if (!mvControlling->Mains)
        {
            mvControlling->MainSwitch(true);
            mvControlling->ConverterSwitch(true);
            mvControlling->CompressorSwitch(true);
        }
        break;
    }
    return OK;
}

bool TController::DecSpeed(bool force)
{ // zmniejszenie prêdkoœci (ale nie hamowanie)
    bool OK = false; // domyœlnie false, aby wysz³o z pêtli while
    switch (mvOccupied->EngineType)
    {
    case None: // McZapkie-041003: wagon sterowniczy
        iDrivigFlags &= ~moveIncSpeed; // usuniêcie flagi jazdy
        if (force) // przy aktywacji kabiny jest potrzeba natychmiastowego wyzerowania
            if (mvControlling->MainCtrlPosNo > 0) // McZapkie-041003: wagon sterowniczy, np. EZT
                mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        return false;
    case ElectricSeriesMotor:
        OK = mvControlling->DecScndCtrl(2); // najpierw bocznik na zero
        if (!OK)
            OK = mvControlling->DecMainCtrl(1 + (mvControlling->MainCtrlPos > 2 ? 1 : 0));
        mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
        break;
    case Dumb:
    case DieselElectric:
	case ElectricInductionMotor:
		OK = mvControlling->DecScndCtrl(2);
        if (!OK)
            OK = mvControlling->DecMainCtrl(2 + (mvControlling->MainCtrlPos / 2));
        break;
    case WheelsDriven:
        if (!mvControlling->CabNo)
            mvControlling->CabActivisation();
        if (sin(mvControlling->eAngle) < 0)
            mvControlling->IncMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        else
            mvControlling->DecMainCtrl(3 + 3 * floor(0.5 + fabs(AccDesired)));
        break;
    case DieselEngine:
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
{ // Ra: regulacja prêdkoœci, wykonywana w ka¿dym przeb³ysku œwiadomoœci AI
    // ma dokrêcaæ do bezoporowych i zdejmowaæ pozycje w przypadku przekroczenia pr¹du
    switch (mvOccupied->EngineType)
    {
    case None: // McZapkie-041003: wagon sterowniczy
        if (fActionTime >= -1.0)
            mvOccupied->DepartureSignal = false; // trochê niech pobuczy, zanim pojedzie
        if (mvControlling->MainCtrlPosNo > 0)
        { // jeœli ma czym krêciæ
            // TODO: sprawdzanie innego czlonu //if (!FuseFlagCheck())
            if ((AccDesired < fAccGravity - 0.05) ||
                (mvOccupied->Vel > VelDesired)) // jeœli nie ma przyspieszaæ
                mvControlling->DecMainCtrl(2); // na zero
            else if (fActionTime >= 0.0)
            { // jak ju¿ mo¿na coœ poruszaæ, przetok roz³¹czaæ od razu
                if (iDrivigFlags & moveIncSpeed)
                { // jak ma jechaæ
                    if (fReady < 0.4) // 0.05*Controlling->MaxBrakePress)
                    { // jak jest odhamowany
                        if (mvOccupied->ActiveDir > 0)
                            mvOccupied->DirectionForward(); //¿eby EN57 jecha³y na drugiej nastawie
                        {
                            if (mvControlling->MainCtrlPos &&
                                !mvControlling
                                     ->StLinFlag) // jak niby jedzie, ale ma roz³¹czone liniowe
                                mvControlling->DecMainCtrl(
                                    2); // to na zero i czekaæ na przewalenie ku³akowego
                            else
                                switch (mvControlling->MainCtrlPos)
                                { // ruch nastawnika uzale¿niony jest od aktualnie ustawionej
                                // pozycji
                                case 0:
                                    if (mvControlling->MainCtrlActualPos) // jeœli ku³akowy nie jest
                                        // wyzerowany
                                        break; // to czekaæ na wyzerowanie
                                    mvControlling->IncMainCtrl(1); // przetok; bez "break", bo nie
                                // ma czekania na 1. pozycji
                                case 1:
                                    if (VelDesired >= 20)
                                        mvControlling->IncMainCtrl(1); // szeregowa
                                case 2:
                                    if (VelDesired >= 50)
                                        mvControlling->IncMainCtrl(1); // równoleg³a
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
                            if (mvControlling->MainCtrlPos) // jak za³¹czy³ pozycjê
                            {
                                fActionTime = -5.0; // niech trochê potrzyma
                                mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                            }
                        }
                    }
                }
                else
                {
                    while (mvControlling->MainCtrlPos)
                        mvControlling->DecMainCtrl(1); // na zero
                    fActionTime = -5.0; // niech trochê potrzyma
                    mvControlling->AutoRelayCheck(); // sprawdzenie logiki sterowania
                }
            }
        }
        break;
    case ElectricSeriesMotor:
        if ((!mvControlling->StLinFlag) && (!mvControlling->DelayCtrlFlag) &&
            (!iDrivigFlags & moveIncSpeed)) // styczniki liniowe roz³¹czone    yBARC
            //    if (iDrivigFlags&moveIncSpeed) {} //jeœli czeka na za³¹czenie liniowych
            //    else
            while (DecSpeed())
                ; // zerowanie napêdu
        else if (Ready || (iDrivigFlags & movePress)) // o ile mo¿e jechaæ
            if (fAccGravity < -0.10) // i jedzie pod górê wiêksz¹ ni¿ 10 promil
            { // procedura wje¿d¿ania na ekstremalne wzniesienia
                if (fabs(mvControlling->Im) >
                    0.85 * mvControlling->Imax) // a pr¹d jest wiêkszy ni¿ 85% nadmiarowego
                    // if (mvControlling->Imin*mvControlling->Voltage/(fMass*fAccGravity)<-2.8) //a
                    // na niskim siê za szybko nie pojedzie
                    if (mvControlling->Imax * mvControlling->Voltage / (fMass * fAccGravity) <
                        -2.8) // a na niskim siê za szybko nie pojedzie
                    { // w³¹czenie wysokiego rozruchu;
                        // (I*U)[A*V=W=kg*m*m/sss]/(m[kg]*a[m/ss])=v[m/s]; 2.8m/ss=10km/h
                        if (mvControlling->RList[mvControlling->MainCtrlPos].Bn > 1)
                        { // jeœli jedzie na równoleg³ym, to zbijamy do szeregowego, aby w³¹czyæ
                            // wysoki rozruch
                            if (mvControlling->ScndCtrlPos > 0) // je¿eli jest bocznik
                                mvControlling->DecScndCtrl(
                                    2); // wy³¹czyæ bocznik, bo mo¿e blokowaæ skrêcenie NJ
                            do // skrêcanie do bezoporowej na szeregowym
                                mvControlling->DecMainCtrl(1); // krêcimy nastawnik jazdy o 1 wstecz
                            while (mvControlling->MainCtrlPos ?
                                       mvControlling->RList[mvControlling->MainCtrlPos].Bn > 1 :
                                       false); // oporowa zapêtla
                        }
                        if (mvControlling->Imax < mvControlling->ImaxHi) // jeœli da siê na wysokim
                            mvControlling->CurrentSwitch(
                                true); // rozruch wysoki (za to mo¿e siê œlizgaæ)
                        if (ReactionTime > 0.1)
                            ReactionTime = 0.1; // orientuj siê szybciej
                    } // if (Im>Imin)
                if (fabs(mvControlling->Im) > 0.75 * mvControlling->ImaxHi) // jeœli pr¹d jest du¿y
                    mvControlling->SandDoseOn(); // piaskujemy tory, coby siê nie œlizgaæ
                if ((fabs(mvControlling->Im) > 0.96 * mvControlling->Imax) ||
                    mvControlling->SlippingWheels) // jeœli pr¹d jest du¿y (mo¿na 690 na 750)
                    if (mvControlling->ScndCtrlPos > 0) // je¿eli jest bocznik
                        mvControlling->DecScndCtrl(2); // zmniejszyæ bocznik
                    else
                        mvControlling->DecMainCtrl(1); // krêcimy nastawnik jazdy o 1 wstecz
            }
            else // gdy nie jedzie ambitnie pod górê
            { // sprawdzenie, czy rozruch wysoki jest potrzebny
                if (mvControlling->Imax > mvControlling->ImaxLo)
                    if (mvOccupied->Vel >= 30.0) // jak siê rozpêdzi³
                        if (fAccGravity > -0.02) // a i pochylenie mnijsze ni¿ 2‰
                            mvControlling->CurrentSwitch(false); // rozruch wysoki wy³¹cz
                // dokrêcanie do bezoporowej, bo IncSpeed() mo¿e nie byæ wywo³ywane
                // if (mvOccupied->Vel<VelDesired)
                // if (AccDesired>-0.1) //nie ma hamowaæ
                //  if (Controlling->RList[MainCtrlPos].R>0.0)
                //   if (Im<1.3*Imin) //lekkie przekroczenie miimalnego pr¹du jest dopuszczalne
                //    IncMainCtrl(1); //zwieksz nastawnik skoro mo¿esz - tak aby siê ustawic na
                //    bezoporowej
            }
        break;
    case Dumb:
    case DieselElectric:
	case ElectricInductionMotor:
		break;
    // WheelsDriven :
    // begin
    //  OK:=False;
    // end;
    case DieselEngine:
        // Ra 2014-06: "automatyczna" skrzynia biegów...
        if (!mvControlling->MotorParam[mvControlling->ScndCtrlPos].AutoSwitch) // gdy biegi rêczne
            if ((mvControlling->ShuntMode ? mvControlling->AnPos : 1.0) * mvControlling->Vel >
                0.6 * mvControlling->MotorParam[mvControlling->ScndCtrlPos].mfi)
            // if (mvControlling->enrot>0.95*mvControlling->dizel_nMmax) //youBy: jeœli obroty >
            // 0,95 nmax, wrzuæ wy¿szy bieg - Ra: to nie dzia³a
            { // jak prêdkoœæ wiêksza ni¿ 0.6 maksymalnej na danym biegu, wrzuciæ wy¿szy
                mvControlling->DecMainCtrl(2);
                if (mvControlling->IncScndCtrl(1))
                    if (mvControlling->MotorParam[mvControlling->ScndCtrlPos].mIsat ==
                        0.0) // jeœli bieg ja³owy
                        mvControlling->IncScndCtrl(1); // to kolejny
            }
            else if ((mvControlling->ShuntMode ? mvControlling->AnPos : 1.0) * mvControlling->Vel <
                     mvControlling->MotorParam[mvControlling->ScndCtrlPos].fi)
            { // jak prêdkoœæ mniejsza ni¿ minimalna na danym biegu, wrzuciæ ni¿szy
                mvControlling->DecMainCtrl(2);
                mvControlling->DecScndCtrl(1);
                if (mvControlling->MotorParam[mvControlling->ScndCtrlPos].mIsat ==
                    0.0) // jeœli bieg ja³owy
                    if (mvControlling->ScndCtrlPos) // a jeszcze zera nie osi¹gniêto
                        mvControlling->DecScndCtrl(1); // to kolejny wczeœniejszy
                    else
                        mvControlling->IncScndCtrl(1); // a jak zesz³o na zero, to powrót
            }
        break;
    }
};

void TController::Doors(bool what)
{ // otwieranie/zamykanie drzwi w sk³adzie albo (tylko AI) EZT
    if (what)
    { // otwarcie
    }
    else
    { // zamykanie
        if (mvOccupied->DoorOpenCtrl == 1)
        { // jeœli drzwi sterowane z kabiny
            if (AIControllFlag)
                if (mvOccupied->DoorLeftOpened || mvOccupied->DoorRightOpened)
                { // AI zamyka drzwi przed odjazdem
                    if (mvOccupied->DoorClosureWarning)
                        mvOccupied->DepartureSignal = true; // za³¹cenie bzyczka
                    mvOccupied->DoorLeft(false); // zamykanie drzwi
                    mvOccupied->DoorRight(false);
                    // Ra: trzeba by ustawiæ jakiœ czas oczekiwania na zamkniêcie siê drzwi
                    fActionTime = -1.5 - 0.1 * random(10); // czekanie sekundê, mo¿e trochê d³u¿ej
                    iDrivigFlags &= ~moveDoorOpened; // nie wykonywaæ drugi raz
                }
        }
        else
        { // jeœli nie, to zamykanie w sk³adzie wagonowym
            TDynamicObject *p = pVehicles[0]; // pojazd na czole sk³adu
            while (p)
            { // zamykanie drzwi w pojazdach - flaga zezwolenia by³a by lepsza
                p->MoverParameters->DoorLeft(false); // w lokomotywie mo¿na by nie zamykaæ...
                p->MoverParameters->DoorRight(false);
                p = p->Next(); // pojazd pod³¹czony z ty³u (patrz¹c od czo³a)
            }
            // WaitingSet(5); //10 sekund tu to za d³ugo, opóŸnia odjazd o pó³ minuty
            fActionTime = -1.5 - 0.1 * random(10); // czekanie sekundê, mo¿e trochê d³u¿ej
            iDrivigFlags &= ~moveDoorOpened; // zosta³y zamkniête - nie wykonywaæ drugi raz
        }
    }
};

void TController::RecognizeCommand()
{ // odczytuje i wykonuje komendê przekazan¹ lokomotywie
    TCommand *c = &mvOccupied->CommandIn;
    PutCommand(c->Command, c->Value1, c->Value2, c->Location, stopComm);
    c->Command = ""; // usuniêcie obs³u¿onej komendy
}

void TController::PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                             const TLocation &NewLocation, TStopReason reason)
{ // wys³anie komendy przez event PutValues, jak pojazd ma obsadê, to wysy³a tutaj, a nie do pojazdu
    // bezpoœrednio
    vector3 sl;
    sl.x = -NewLocation.X; // zamiana na wspó³rzêdne scenerii
    sl.z = NewLocation.Y;
    sl.y = NewLocation.Z;
    if (!PutCommand(NewCommand, NewValue1, NewValue2, &sl, reason))
        mvOccupied->PutCommand(NewCommand, NewValue1, NewValue2, NewLocation);
}

bool TController::PutCommand(AnsiString NewCommand, double NewValue1, double NewValue2,
                             const vector3 *NewLocation, TStopReason reason)
{ // analiza komendy
    if (NewCommand == "CabSignal")
    { // SHP wyzwalane jest przez cz³on z obsad¹, ale obs³ugiwane przez silnikowy
        // nie jest to najlepiej zrobione, ale bez symulacji obwodów lepiej nie bêdzie
        // Ra 2014-04: jednak przenios³em do rozrz¹dczego
        mvOccupied->PutCommand(NewCommand, NewValue1, NewValue2, mvOccupied->Loc);
        mvOccupied->RunInternalCommand(); // rozpoznaj komende bo lokomotywa jej nie rozpoznaje
        return true; // za³atwione
    }
    if (NewCommand == "Overhead")
    { // informacja o stanie sieci trakcyjnej
        fOverhead1 =
            NewValue1; // informacja o napiêciu w sieci trakcyjnej (0=brak drutu, zatrzymaj!)
        fOverhead2 = NewValue2; // informacja o sposobie jazdy (-1=normalnie, 0=bez pr¹du, >0=z
        // opuszczonym i ograniczeniem prêdkoœci)
        return true; // za³atwione
    }
    else if (NewCommand == "Emergency_brake") // wymuszenie zatrzymania, niezale¿nie kto prowadzi
    { // Ra: no nadal nie jest zbyt piêknie
        SetVelocity(0, 0, reason);
        mvOccupied->PutCommand("Emergency_brake", 1.0, 1.0, mvOccupied->Loc);
        return true; // za³atwione
    }
    else if (NewCommand.Pos("Timetable:") == 1)
    { // przypisanie nowego rozk³adu jazdy, równie¿ prowadzonemu przez u¿ytkownika
        NewCommand.Delete(1, 10); // zostanie nazwa pliku z rozk³adem
#if LOGSTOPS
        WriteLog("New timetable for " + pVehicle->asName + ": " + NewCommand); // informacja
#endif
        if (!TrainParams)
            TrainParams = new TTrainParameters(NewCommand); // rozk³ad jazdy
        else
            TrainParams->NewName(NewCommand); // czyœci tabelkê przystanków
        delete tsGuardSignal;
        tsGuardSignal = NULL; // wywalenie kierownika
        if (NewCommand != "none")
        {
            if (!TrainParams->LoadTTfile(
                    Global::asCurrentSceneryPath, floor(NewValue2 + 0.5),
                    NewValue1)) // pierwszy parametr to przesuniêcie rozk³adu w czasie
            {
                if (ConversionError == -8)
                    ErrorLog("Missed timetable: " + NewCommand);
                WriteLog("Cannot load timetable file " + NewCommand + "\r\nError " +
                         ConversionError + " in position " + TrainParams->StationCount);
                NewCommand = ""; // puste, dla wymiennej tekstury
            }
            else
            { // inicjacja pierwszego przystanku i pobranie jego nazwy
                TrainParams->UpdateMTable(GlobalTime->hh, GlobalTime->mm,
                                          TrainParams->NextStationName);
                TrainParams->StationIndexInc(); // przejœcie do nastêpnej
                iStationStart = TrainParams->StationIndex;
                asNextStop = TrainParams->NextStop();
                iDrivigFlags |= movePrimary; // skoro dosta³ rozk³ad, to jest teraz g³ównym
                NewCommand = Global::asCurrentSceneryPath + NewCommand + ".wav"; // na razie jeden
                if (FileExists(NewCommand))
                { // wczytanie dŸwiêku odjazdu podawanego bezpoœrenido
                    tsGuardSignal = new TTextSound();
                    tsGuardSignal->Init(NewCommand.c_str(), 30, pVehicle->GetPosition().x,
                                        pVehicle->GetPosition().y, pVehicle->GetPosition().z,
                                        false);
                    // rsGuardSignal->Stop();
                    iGuardRadio = 0; // nie przez radio
                }
                else
                {
                    NewCommand.Insert("radio", NewCommand.Length() - 3); // wstawienie przed kropk¹
                    if (FileExists(NewCommand))
                    { // wczytanie dŸwiêku odjazdu w wersji radiowej (s³ychaæ tylko w kabinie)
                        tsGuardSignal = new TTextSound();
                        tsGuardSignal->Init(NewCommand.c_str(), -1, pVehicle->GetPosition().x,
                                            pVehicle->GetPosition().y, pVehicle->GetPosition().z,
                                            false);
                        iGuardRadio = iRadioChannel;
                    }
                }
                NewCommand = TrainParams->Relation2; // relacja docelowa z rozk³adu
            }
            // jeszcze poustawiaæ tekstury na wyœwietlaczach
            TDynamicObject *p = pVehicles[0];
            while (p)
            {
                p->DestinationSet(NewCommand); // relacja docelowa
                p = p->Next(); // pojazd pod³¹czony od ty³u (licz¹c od czo³a)
            }
        }
        if (NewLocation) // jeœli podane wspó³rzêdne eventu/komórki ustawiaj¹cej rozk³ad (trainset
        // nie podaje)
        {
            vector3 v = *NewLocation - pVehicle->GetPosition(); // wektor do punktu steruj¹cego
            vector3 d = pVehicle->VectorFront(); // wektor wskazuj¹cy przód
            iDirectionOrder = ((v.x * d.x + v.z * d.z) * NewValue1 > 0) ?
                                  1 :
                                  -1; // do przodu, gdy iloczyn skalarny i prêdkoœæ dodatnie
            /*
              if (NewValue1!=0.0) //jeœli ma jechaæ
               if (iDirectionOrder==0) //a kierunek nie by³ okreœlony (normalnie okreœlany przez
              reardriver/headdriver)
                iDirectionOrder=NewValue1>0?1:-1; //ustalenie kierunku jazdy wzglêdem sprzêgów
               else
                if (NewValue1<0) //dla ujemnej prêdkoœci
                 iDirectionOrder=-iDirectionOrder; //ma jechaæ w drug¹ stronê
            */
            if (AIControllFlag) // jeœli prowadzi komputer
                Activation(); // umieszczenie obs³ugi we w³aœciwym cz³onie, ustawienie nawrotnika w
            // przód
        }
        /*
          else
           if (!iDirectionOrder)
          {//jeœli nie ma kierunku, trzeba ustaliæ
           if (mvOccupied->V!=0.0)
            iDirectionOrder=mvOccupied->V>0?1:-1;
           else
            iDirectionOrder=mvOccupied->ActiveCab;
           if (!iDirectionOrder) iDirectionOrder=1;
          }
        */
        // jeœli wysy³ane z Trainset, to wszystko jest ju¿ odpowiednio ustawione
        // if (!NewLocation) //jeœli wysy³ane z Trainset
        // if (mvOccupied->CabNo*mvOccupied->V*NewValue1<0) //jeœli zadana prêdkoœæ niezgodna z
        // aktualnym kierunkiem jazdy
        //  DirectionForward(false); //jedziemy do ty³u (nawrotnik do ty³u)
        // CheckVehicles(); //sprawdzenie sk³adu, AI zapali œwiat³a
        TableClear(); // wyczyszczenie tabelki prêdkoœci, bo na nowo trzeba okreœliæ kierunek i
        // sprawdziæ przystanki
        OrdersInit(
            fabs(NewValue1)); // ustalenie tabelki komend wg rozk³adu oraz prêdkoœci pocz¹tkowej
        // if (NewValue1!=0.0) if (!AIControllFlag) DirectionForward(NewValue1>0.0); //ustawienie
        // nawrotnika u¿ytkownikowi (propaguje siê do cz³onów)
        // if (NewValue1>0)
        // TrainNumber=floor(NewValue1); //i co potem ???
        return true; // za³atwione
    }
    if (NewCommand == "SetVelocity")
    {
        if (NewLocation)
            vCommandLocation = *NewLocation;
        if ((NewValue1 != 0.0) && (OrderList[OrderPos] != Obey_train))
        { // o ile jazda
            if (!iEngineActive)
                OrderNext(Prepare_engine); // trzeba odpaliæ silnik najpierw, œwiat³a ustawi
            // JumpToNextOrder()
            // if (OrderList[OrderPos]!=Obey_train) //jeœli nie poci¹gowa
            OrderNext(Obey_train); // to uruchomiæ jazdê poci¹gow¹ (od razu albo po odpaleniu
            // silnika
            OrderCheck(); // jeœli jazda poci¹gowa teraz, to wykonaæ niezbêdne operacje
        }
        if (NewValue1 != 0.0) // jeœli jechaæ
            iDrivigFlags &= ~moveStopHere; // podje¿anie do semaforów zezwolone
        else
            iDrivigFlags |= moveStopHere; // staæ do momentu podania komendy jazdy
        SetVelocity(NewValue1, NewValue2, reason); // bylo: nic nie rob bo SetVelocity zewnetrznie
        // jest wywolywane przez dynobj.cpp
    }
    else if (NewCommand == "SetProximityVelocity")
    {
        /*
          if (SetProximityVelocity(NewValue1,NewValue2))
           if (NewLocation)
            vCommandLocation=*NewLocation;
        */
    }
    else if (NewCommand == "ShuntVelocity")
    { // uruchomienie jazdy manewrowej b¹dŸ zmiana prêdkoœci
        if (NewLocation)
            vCommandLocation = *NewLocation;
        // if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0))
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpaliæ silnik najpierw
        OrderNext(Shunt); // zamieniamy w aktualnej pozycji, albo dodajey za odpaleniem silnika
        if (NewValue1 != 0.0)
        {
            // if (iVehicleCount>=0) WriteLog("Skasowano ilosæ wagonów w ShuntVelocity!");
            iVehicleCount = -2; // wartoœæ neutralna
            CheckVehicles(); // zabraæ to do OrderCheck()
        }
        // dla prêdkoœci ujemnej przestawiæ nawrotnik do ty³u? ale -1=brak ograniczenia !!!!
        // if (iDrivigFlags&movePress) WriteLog("Skasowano docisk w ShuntVelocity!");
        iDrivigFlags &= ~movePress; // bez dociskania
        SetVelocity(NewValue1, NewValue2, reason);
        if (NewValue1 != 0.0)
            iDrivigFlags &= ~moveStopHere; // podje¿anie do semaforów zezwolone
        else
            iDrivigFlags |= moveStopHere; // ma staæ w miejscu
        if (fabs(NewValue1) > 2.0) // o ile wartoœæ jest sensowna (-1 nie jest konkretn¹ wartoœci¹)
            fShuntVelocity = fabs(NewValue1); // zapamiêtanie obowi¹zuj¹cej prêdkoœci dla manewrów
    }
    else if (NewCommand == "Wait_for_orders")
    { // oczekiwanie; NewValue1 - czas oczekiwania, -1 = na inn¹ komendê
        if (NewValue1 > 0.0 ? NewValue1 > fStopTime : false)
            fStopTime = NewValue1; // Ra: w³¹czenie czekania bez zmiany komendy
        else
            OrderList[OrderPos] = Wait_for_orders; // czekanie na komendê (albo daæ OrderPos=0)
    }
    else if (NewCommand == "Prepare_engine")
    { // w³¹czenie albo wy³¹czenie silnika (w szerokim sensie)
        OrdersClear(); // czyszczenie tabelki rozkazów, aby nic dalej nie robi³
        if (NewValue1 == 0.0)
            OrderNext(Release_engine); // wy³¹czyæ silnik (przygotowaæ pojazd do jazdy)
        else if (NewValue1 > 0.0)
            OrderNext(Prepare_engine); // odpaliæ silnik (wy³¹czyæ wszystko, co siê da)
        // po za³¹czeniu przejdzie do kolejnej komendy, po wy³¹czeniu na Wait_for_orders
    }
    else if (NewCommand == "Change_direction")
    {
        TOrders o = OrderList[OrderPos]; // co robi³ przed zmian¹ kierunku
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpaliæ silnik najpierw
        if (NewValue1 == 0.0)
            iDirectionOrder = -iDirection; // zmiana na przeciwny ni¿ obecny
        else if (NewLocation) // jeœli podane wspó³rzêdne eventu/komórki ustawiaj¹cej rozk³ad
        // (trainset nie podaje)
        {
            vector3 v = *NewLocation - pVehicle->GetPosition(); // wektor do punktu steruj¹cego
            vector3 d = pVehicle->VectorFront(); // wektor wskazuj¹cy przód
            iDirectionOrder = ((v.x * d.x + v.z * d.z) * NewValue1 > 0) ?
                                  1 :
                                  -1; // do przodu, gdy iloczyn skalarny i prêdkoœæ dodatnie
            // iDirectionOrder=1; else if (NewValue1<0.0) iDirectionOrder=-1;
        }
        if (iDirectionOrder != iDirection)
            OrderNext(Change_direction); // zadanie komendy do wykonania
        if (o >= Shunt) // jeœli jazda manewrowa albo poci¹gowa
            OrderNext(o); // to samo robiæ po zmianie
        else if (!o) // jeœli wczeœniej by³o czekanie
            OrderNext(Shunt); // to dalej jazda manewrowa
        if (mvOccupied->Vel >= 1.0) // jeœli jedzie
            iDrivigFlags &= ~moveStartHorn; // to bez tr¹bienia po ruszeniu z zatrzymania
        // Change_direction wykona siê samo i nastêpnie przejdzie do kolejnej komendy
    }
    else if (NewCommand == "Obey_train")
    {
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpaliæ silnik najpierw
        OrderNext(Obey_train);
        // if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
        OrderCheck(); // jeœli jazda poci¹gowa teraz, to wykonaæ niezbêdne operacje
    }
    else if (NewCommand == "Shunt")
    { // NewValue1 - iloœæ wagonów (-1=wszystkie); NewValue2: 0=odczep, 1..63=do³¹cz, -1=bez zmian
        //-3,-y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1), zmieniæ kierunek i czekaæ w
        // trybie poci¹gowym
        //-2,-y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1), zmieniæ kierunek i czekaæ
        //-2, y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1) i czekaæ
        //-1,-y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1) i jechaæ w powrotn¹ stronê
        //-1, y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1) i jechaæ dalej
        //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagonów nie ma sensu)
        // 0, 0 - odczepienie lokomotywy
        // 1,-y - pod³¹czyæ siê do sk³adu (sprzêgiem y>=1), a nastêpnie odczepiæ i zabraæ (x)
        // wagonów
        // 1, 0 - odczepienie lokomotywy z jednym wagonem
        iDrivigFlags &= ~moveStopHere; // podje¿anie do semaforów zezwolone
        if (!iEngineActive)
            OrderNext(Prepare_engine); // trzeba odpaliæ silnik najpierw
        if (NewValue2 != 0) // jeœli podany jest sprzêg
        {
            iCoupler = floor(fabs(NewValue2)); // jakim sprzêgiem
            OrderNext(Connect); // najpierw po³¹cz pojazdy
            if (NewValue1 >= 0.0) // jeœli iloœæ wagonów inna ni¿ wszystkie
            { // to po podpiêciu nale¿y siê odczepiæ
                iDirectionOrder = -iDirection; // zmiana na ci¹gniêcie
                OrderPush(Change_direction); // najpierw zmieñ kierunek, bo odczepiamy z ty³u
                OrderPush(Disconnect); // a odczep ju¿ po zmianie kierunku
            }
            else if (NewValue2 < 0.0) // jeœli wszystkie, a sprzêg ujemny, to tylko zmiana kierunku
            // po podczepieniu
            { // np. Shunt -1 -3
                iDirectionOrder = -iDirection; // jak siê podczepi, to jazda w przeciwn¹ stronê
                OrderNext(Change_direction);
            }
            WaitingTime =
                0.0; // nie ma co dalej czekaæ, mo¿na zatr¹biæ i jechaæ, chyba ¿e ju¿ jedzie
        }
        else // if (NewValue2==0.0) //zerowy sprzêg
            if (NewValue1 >= 0.0) // jeœli iloœæ wagonów inna ni¿ wszystkie
        { // bêdzie odczepianie, ale jeœli wagony s¹ z przodu, to trzeba najpierw zmieniæ kierunek
            if ((mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 1 : 0].CouplingFlag ==
                 0) ? // z ty³u nic
                    (mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 0 : 1].CouplingFlag > 0) :
                    false) // a z przodu sk³ad
            {
                iDirectionOrder = -iDirection; // zmiana na ci¹gniêcie
                OrderNext(Change_direction); // najpierw zmieñ kierunek (zast¹pi Disconnect)
                OrderPush(Disconnect); // a odczep ju¿ po zmianie kierunku
            }
            else if (mvOccupied->Couplers[mvOccupied->DirAbsolute > 0 ? 1 : 0].CouplingFlag >
                     0) // z ty³u coœ
                OrderNext(Disconnect); // jak ci¹gnie, to tylko odczep (NewValue1) wagonów
            WaitingTime =
                0.0; // nie ma co dalej czekaæ, mo¿na zatr¹biæ i jechaæ, chyba ¿e ju¿ jedzie
        }
        if (NewValue1 == -1.0)
        {
            iDrivigFlags &= ~moveStopHere; // ma jechaæ
            WaitingTime = 0.0; // nie ma co dalej czekaæ, mo¿na zatr¹biæ i jechaæ
        }
        if (NewValue1 < -1.5) // jeœli -2/-3, czyli czekanie z ruszeniem na sygna³
            iDrivigFlags |= moveStopHere; // nie podje¿d¿aæ do semafora, jeœli droga nie jest wolna
        // (nie dotyczy Connect)
        if (NewValue1 < -2.5) // jeœli
            OrderNext(Obey_train); // to potem jazda poci¹gowa
        else
            OrderNext(Shunt); // to potem manewruj dalej
        CheckVehicles(); // sprawdziæ œwiat³a
        // if ((iVehicleCount>=0)&&(NewValue1<0)) WriteLog("Skasowano ilosæ wagonów w Shunt!");
        if (NewValue1 != iVehicleCount)
            iVehicleCount = floor(NewValue1); // i co potem ? - trzeba zaprogramowac odczepianie
        /*
          if (NewValue1!=-1.0)
           if (NewValue2!=0.0)

            if (VelDesired==0)
             SetVelocity(20,0); //to niech jedzie
        */
    }
    else if (NewCommand == "Jump_to_first_order")
        JumpToFirstOrder();
    else if (NewCommand == "Jump_to_order")
    {
        if (NewValue1 == -1.0)
            JumpToNextOrder();
        else if ((NewValue1 >= 0) && (NewValue1 < maxorders))
        {
            OrderPos = floor(NewValue1);
            if (!OrderPos)
                OrderPos = 1; // zgodnoœæ wstecz: dopiero pierwsza uruchamia
#if LOGORDERS
            WriteLog("--> Jump_to_order");
            OrdersDump();
#endif
        }
        /*
          if (WriteLogFlag)
          {
           append(AIlogFile);
           writeln(AILogFile,ElapsedTime:5:2," - new order: ",Order2Str( OrderList[OrderPos])," @
          ",OrderPos);
           close(AILogFile);
          }
        */
    }
    /* //ta komenda jest teraz skanowana, wiêc wysy³anie jej eventem nie ma sensu
     else if (NewCommand=="OutsideStation") //wskaznik W5
     {
      if (OrderList[OrderPos]==Obey_train)
       SetVelocity(NewValue1,NewValue2,stopOut); //koniec stacji - predkosc szlakowa
      else //manewry - zawracaj
      {
       iDirectionOrder=-iDirection; //zmiana na przeciwny ni¿ obecny
       OrderNext(Change_direction); //zmiana kierunku
       OrderNext(Shunt); //a dalej manewry
       iDrivigFlags&=~moveStartHorn; //bez tr¹bienia po zatrzymaniu
      }
     }
    */
    else if (NewCommand == "Warning_signal")
    {
        if (AIControllFlag) // poni¿sza komenda nie jest wykonywana przez u¿ytkownika
            if (NewValue1 > 0)
            {
                fWarningDuration = NewValue1; // czas tr¹bienia
                mvOccupied->WarningSignal = (NewValue2 > 1) ? 2 : 1; // wysokoœæ tonu
            }
    }
    else if (NewCommand == "Radio_channel")
    { // wybór kana³u radiowego (którego powinien u¿ywaæ AI, rêczny maszynista musi go ustawiæ sam)
        if (NewValue1 >= 0) // wartoœci ujemne s¹ zarezerwowane, -1 = nie zmieniaæ kana³u
        {
            iRadioChannel = NewValue1;
            if (iGuardRadio)
                iGuardRadio = iRadioChannel; // kierownikowi te¿ zmieniæ
        }
        // NewValue2 mo¿e zawieraæ dodatkowo oczekiwany kod odpowiedzi, np. dla W29 "nawi¹zaæ
        // ³¹cznoœæ radiow¹ z dy¿urnym ruchu odcinkowym"
    }
    else
        return false; // nierozpoznana - wys³aæ bezpoœrednio do pojazdu
    return true; // komenda zosta³a przetworzona
};

void TController::PhysicsLog()
{ // zapis logu - na razie tylko wypisanie parametrów
    if (LogFile.is_open())
    {
#if LOGPRESS == 0
        LogFile << ElapsedTime << " " << fabs(11.31 * mvOccupied->WheelDiameter * mvOccupied->nrot)
                << " ";
        LogFile << mvControlling->AccS << " " << mvOccupied->Couplers[1].Dist << " "
                << mvOccupied->Couplers[1].CForce << " ";
        LogFile << mvOccupied->Ft << " " << mvOccupied->Ff << " " << mvOccupied->Fb << " "
                << mvOccupied->BrakePress << " ";
        LogFile << mvOccupied->PipePress << " " << mvControlling->Im << " "
                << int(mvControlling->MainCtrlPos) << "   ";
        LogFile << int(mvControlling->ScndCtrlPos) << "   " << int(mvOccupied->BrakeCtrlPos)
                << "   " << int(mvOccupied->LocalBrakePos) << "   ";
        LogFile << int(mvControlling->ActiveDir) << "   " << mvOccupied->CommandIn.Command.c_str()
                << " " << mvOccupied->CommandIn.Value1 << " ";
        LogFile << mvOccupied->CommandIn.Value2 << " " << int(mvControlling->SecuritySystem.Status)
                << " " << int(mvControlling->SlippingWheels) << "\r\n";
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

bool TController::UpdateSituation(double dt)
{ // uruchamiaæ przynajmniej raz na sekundê
    if ((iDrivigFlags & movePrimary) == 0)
        return true; // pasywny nic nie robi
    double AbsAccS;
    // double VelReduced; //o ile km/h mo¿e przekroczyæ dozwolon¹ prêdkoœæ bez hamowania
    bool UpdateOK = false;
    if (AIControllFlag)
    { // yb: zeby EP nie musial sie bawic z ciesnieniem w PG
        //  if (mvOccupied->BrakeSystem==ElectroPneumatic)
        //   mvOccupied->PipePress=0.5; //yB: w SPKS s¹ poprawnie zrobione pozycje
        if (mvControlling->SlippingWheels)
        {
            mvControlling->SandDoseOn(); // piasku!
            // Controlling->SlippingWheels=false; //a to tu nie ma sensu, flaga u¿ywana w dalszej
            // czêœci
        }
    }
    // ABu-160305 testowanie gotowoœci do jazdy
    // Ra: przeniesione z DynObj, sk³ad u¿ytkownika te¿ jest testowany, ¿eby mu przekazaæ, ¿e ma
    // odhamowaæ
    Ready = true; // wstêpnie gotowy
    fReady = 0.0; // za³o¿enie, ¿e odhamowany
    fAccGravity = 0.0; // przyspieszenie wynikaj¹ce z pochylenia
    double dy; // sk³adowa styczna grawitacji, w przedziale <0,1>
    TDynamicObject *p = pVehicles[0]; // pojazd na czole sk³adu
    while (p)
    { // sprawdzenie odhamowania wszystkich po³¹czonych pojazdów
        if (Ready) // bo jak coœ nie odhamowane, to dalej nie ma co sprawdzaæ
            // if (p->MoverParameters->BrakePress>=0.03*p->MoverParameters->MaxBrakePress)
            if (p->MoverParameters->BrakePress >= 0.4) // wg UIC okreœlone sztywno na 0.04
            {
                Ready = false; // nie gotowy
                // Ra: odluŸnianie prze³adowanych lokomotyw, ci¹gniêtych na zimno - prowizorka...
                if (AIControllFlag) // sk³ad jak dot¹d by³ wyluzowany
                {
                    if (mvOccupied->BrakeCtrlPos == 0) // jest pozycja jazdy
                        if ((p->MoverParameters->PipePress - 5.0) >
                            -0.1) // jeœli ciœnienie jak dla jazdy
                            if (p->MoverParameters->Hamulec->GetCRP() >
                                p->MoverParameters->PipePress + 0.12) // za du¿o w zbiorniku
                                p->MoverParameters->BrakeReleaser(1); // indywidualne luzowanko
                    if (p->MoverParameters->Power > 0.01) // jeœli ma silnik
                        if (p->MoverParameters->FuseFlag) // wywalony nadmiarowy
                            Need_TryAgain = true; // reset jak przy wywaleniu nadmiarowego
                }
            }
        if (fReady < p->MoverParameters->BrakePress)
            fReady = p->MoverParameters->BrakePress; // szukanie najbardziej zahamowanego
        if ((dy = p->VectorFront().y) != 0.0) // istotne tylko dla pojazdów na pochyleniu
            fAccGravity -= p->DirectionGet() * p->MoverParameters->TotalMassxg *
                           dy; // ciê¿ar razy sk³adowa styczna grawitacji
        p = p->Next(); // pojazd pod³¹czony z ty³u (patrz¹c od czo³a)
    }
    if (iDirection)
        fAccGravity /=
            iDirection *
            fMass; // si³ê generuj¹ pojazdy na pochyleniu ale dzia³a ona ca³oœæ sk³adu, wiêc a=F/m
    if (!Ready) // v367: jeœli wg powy¿szych warunków sk³ad nie jest odhamowany
        if (fAccGravity < -0.05) // jeœli ma pod górê na tyle, by siê stoczyæ
            // if (mvOccupied->BrakePress<0.08) //to wystarczy, ¿e zadzia³aj¹ liniowe (nie ma ich
            // jeszcze!!!)
            if (fReady < 0.8) // delikatniejszy warunek, obejmuje wszystkie wagony
                Ready = true; //¿eby uznaæ za odhamowany
    HelpMeFlag = false;
    // Winger 020304
    if (AIControllFlag)
    {
        if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
        {
            if (mvOccupied->ScndPipePress > 4.3) // gdy g³ówna sprê¿arka bezpiecznie nabije
                // ciœnienie
                mvControlling->bPantKurek3 =
                    true; // to mo¿na przestawiæ kurek na zasilanie pantografów z g³ównej pneumatyki
            fVoltage =
                0.5 * (fVoltage +
                       fabs(mvControlling->RunningTraction.TractionVoltage)); // uœrednione napiêcie
            // sieci: przy spadku
            // poni¿ej wartoœci
            // minimalnej opóŸniæ
            // rozruch o losowy
            // czas
            if (fVoltage < mvControlling->EnginePowerSource.CollectorParameters
                               .MinV) // gdy roz³¹czenie WS z powodu niskiego napiêcia
                if (fActionTime >= 0) // jeœli czas oczekiwania nie zosta³ ustawiony
                    fActionTime =
                        -2 - random(10); // losowy czas oczekiwania przed ponownym za³¹czeniem jazdy
        }
        if (mvOccupied->Vel > 0.0)
        { // je¿eli jedzie
            if (iDrivigFlags & moveDoorOpened) // jeœli drzwi otwarte
                if (mvOccupied->Vel > 1.0) // nie zamykaæ drzwi przy drganiach, bo zatrzymanie na W4
                    // akceptuje niewielkie prêdkoœci
                    Doors(false);
            // przy prowadzeniu samochodu trzeba ka¿d¹ oœ odsuwaæ oddzielnie, inaczej kicha wychodzi
            if (mvOccupied->CategoryFlag & 2) // jeœli samochód
                // if (fabs(mvOccupied->OffsetTrackH)<mvOccupied->Dim.W) //Ra: szerokoœæ drogi tu
                // powinna byæ?
                if (!mvOccupied->ChangeOffsetH(-0.01 * mvOccupied->Vel * dt)) // ruch w poprzek
                    // drogi
                    mvOccupied->ChangeOffsetH(0.01 * mvOccupied->Vel *
                                              dt); // Ra: co to mia³o byæ, to nie wiem
            if (mvControlling->EnginePowerSource.SourceType == CurrentCollector)
            {
                if ((fOverhead2 >= 0.0) || iOverheadZero)
                { // jeœli jazda bezpr¹dowa albo z opuszczonym pantografem
                    while (DecSpeed(true))
                        ; // zerowanie napêdu
                }
                if ((fOverhead2 > 0.0) || iOverheadDown)
                { // jazda z opuszczonymi pantografami
                    mvControlling->PantFront(false);
                    mvControlling->PantRear(false);
                }
                else
                { // jeœli nie trzeba opuszczaæ pantografów
                    if (iDirection >= 0) // jak jedzie w kierunku sprzêgu 0
                        mvControlling->PantRear(true); // jazda na tylnym
                    else
                        mvControlling->PantFront(true);
                }
                if (mvOccupied->Vel > 10) // opuszczenie przedniego po rozpêdzeniu siê
                {
                    if (mvControlling->EnginePowerSource.CollectorParameters.CollectorsNo >
                        1) // o ile jest wiêcej ni¿ jeden
                        if (iDirection >= 0) // jak jedzie w kierunku sprzêgu 0
                        { // poczekaæ na podniesienie tylnego
                            if (mvControlling->PantRearVolt !=
                                0.0) // czy jest napiêcie zasilaj¹ce na tylnym?
                                mvControlling->PantFront(false); // opuszcza od sprzêgu 0
                        }
                        else
                        { // poczekaæ na podniesienie przedniego
                            if (mvControlling->PantFrontVolt !=
                                0.0) // czy jest napiêcie zasilaj¹ce na przednim?
                                mvControlling->PantRear(false); // opuszcza od sprzêgu 1
                        }
                }
            }
        }
        if (iDrivigFlags & moveStartHornNow) // czy ma zatr¹biæ przed ruszeniem?
            if (Ready) // gotów do jazdy
                if (iEngineActive) // jeszcze siê odpaliæ musi
                    if (fStopTime >= 0) // i nie musi czekaæ
                    { // uruchomienie tr¹bienia
                        fWarningDuration = 0.3; // czas tr¹bienia
                        // if (AIControllFlag) //jak siedzi krasnoludek, to w³¹czy tr¹bienie
                        mvOccupied->WarningSignal =
                            pVehicle->iHornWarning; // wysokoœæ tonu (2=wysoki)
                        iDrivigFlags |= moveStartHornDone; // nie tr¹biæ a¿ do ruszenia
                        iDrivigFlags &= ~moveStartHornNow; // tr¹bienie zosta³o zorganizowane
                    }
    }
    ElapsedTime += dt;
    WaitingTime += dt;
    fBrakeTime -=
        dt; // wpisana wartoœæ jest zmniejszana do 0, gdy ujemna nale¿y zmieniæ nastawê hamulca
    fStopTime += dt; // zliczanie czasu postoju, nie ruszy dopóki ujemne
    fActionTime += dt; // czas u¿ywany przy regulacji prêdkoœci i zamykaniu drzwi
    if (WriteLogFlag)
    {
        if (LastUpdatedTime > deltalog)
        { // zapis do pliku DAT
            PhysicsLog();
            if (fabs(mvOccupied->V) > 0.1) // Ra: [m/s]
                deltalog = 0.05; // 0.2;
            else
                deltalog = 0.05; // 1.0;
            LastUpdatedTime = 0.0;
        }
        else
            LastUpdatedTime = LastUpdatedTime + dt;
    }
    // Ra: skanowanie równie¿ dla prowadzonego rêcznie, aby podpowiedzieæ prêdkoœæ
    if ((LastReactionTime > Min0R(ReactionTime, 2.0)))
    {
        // Ra: nie wiem czemu ReactionTime potrafi dostaæ 12 sekund, to jest przegiêcie, bo prze¿yna
        // STÓJ
        // yB: otó¿ jest to jedna trzecia czasu nape³niania na towarowym; mo¿e siê przydaæ przy
        // wdra¿aniu hamowania, ¿eby nie rusza³o kranem jak g³upie
        // Ra: ale nie mo¿e siê budziæ co pó³ minuty, bo prze¿yna semafory
        // Ra: trzeba by tak:
        // 1. Ustaliæ istotn¹ odleg³oœæ zainteresowania (np. 3×droga hamowania z V.max).
        fBrakeDist = fDriverBraking * mvOccupied->Vel *
                     (40.0 + mvOccupied->Vel); // przybli¿ona droga hamowania
        // dla hamowania -0.2 [m/ss] droga wynosi 0.389*Vel*Vel [km/h], czyli 600m dla 40km/h, 3.8km
        // dla 100km/h i 9.8km dla 160km/h
        // dla hamowania -0.4 [m/ss] droga wynosi 0.096*Vel*Vel [km/h], czyli 150m dla 40km/h, 1.0km
        // dla 100km/h i 2.5km dla 160km/h
        // ogólnie droga hamowania przy sta³ym opóŸnieniu to Vel*Vel/(3.6*3.6*a) [m]
        // fBrakeDist powinno byæ wyznaczane dla danego sk³adu za pomoc¹ sieci neuronowych, w
        // zale¿noœci od prêdkoœci i si³y (ciœnienia) hamowania
        // nastêpnie w drug¹ stronê, z drogi hamowania i chwilowej prêdkoœci powinno byæ wyznaczane
        // zalecane ciœnienie
        if (fMass > 1000000.0)
            fBrakeDist *= 2.0; // korekta dla ciê¿kich, bo prze¿ynaj¹ - da to coœ?
        if (mvOccupied->BrakeDelayFlag == bdelay_G)
            fBrakeDist = fBrakeDist + 2 * mvOccupied->Vel; // dla nastawienia G
        // koniecznie nale¿y wyd³u¿yæ drogê na czas reakcji
        // double scanmax=(mvOccupied->Vel>0.0)?3*fDriverDist+fBrakeDist:10.0*fDriverDist;
        double scanmax = (mvOccupied->Vel > 5.0) ?
                             400 + fBrakeDist :
                             30.0 * fDriverDist; // 1500m dla stoj¹cych poci¹gów; Ra 2015-01: przy
		//double scanmax = Max0R(400 + fBrakeDist, 1500);
        // d³u¿szej drodze skanowania AI jeŸdzi spokojniej
        // 2. Sprawdziæ, czy tabelka pokrywa za³o¿ony odcinek (nie musi, jeœli jest STOP).
        // 3. Sprawdziæ, czy trajektoria ruchu przechodzi przez zwrotnice - jeœli tak, to sprawdziæ,
        // czy stan siê nie zmieni³.
        // 4. Ewentualnie uzupe³niæ tabelkê informacjami o sygna³ach i ograniczeniach, jeœli siê
        // "zu¿y³a".
        TableCheck(scanmax); // wype³nianie tabelki i aktualizacja odleg³oœci
        // 5. Sprawdziæ stany sygnalizacji zapisanej w tabelce, wyznaczyæ prêdkoœci.
        // 6. Z tabelki wyznaczyæ krytyczn¹ odleg³oœæ i prêdkoœæ (najmniejsze przyspieszenie).
        // 7. Jeœli jest inny pojazd z przodu, ewentualnie skorygowaæ odleg³oœæ i prêdkoœæ.
        // 8. Ustaliæ czêstotliwoœæ œwiadomoœci AI (zatrzymanie precyzyjne - czêœciej, brak atrakcji
        // - rzadziej).
        if (AIControllFlag)
        { // tu bedzie logika sterowania
            if (mvOccupied->CommandIn.Command != "")
                if (!mvOccupied->RunInternalCommand()) // rozpoznaj komende bo lokomotywa jej nie
                    // rozpoznaje
                    RecognizeCommand(); // samo czyta komendê wstawion¹ do pojazdu?
            if (mvOccupied->SecuritySystem.Status > 1) // jak zadzia³a³o CA/SHP
                if (!mvOccupied->SecuritySystemReset()) // to skasuj
                    // if
                    // ((TestFlag(mvOccupied->SecuritySystem.Status,s_ebrake))&&(mvOccupied->BrakeCtrlPos==0)&&(AccDesired>0.0))
                    if ((TestFlag(mvOccupied->SecuritySystem.Status, s_SHPebrake) ||
                         TestFlag(mvOccupied->SecuritySystem.Status, s_CAebrake)) &&
                        (mvOccupied->BrakeCtrlPos == 0) && (AccDesired > 0.0))
                        mvOccupied->BrakeLevelSet(
                            0); //!!! hm, mo¿e po prostu normalnie sterowaæ hamulcem?
        }
        switch (OrderList[OrderPos])
        { // ustalenie prêdkoœci przy doczepianiu i odczepianiu, dystansów w pozosta³ych przypadkach
        case Connect: // pod³¹czanie do sk³adu
            if (iDrivigFlags & moveConnect)
            { // jeœli stan¹³ ju¿ blisko, unikaj¹c zderzenia i mo¿na próbowaæ pod³¹czyæ
                fMinProximityDist = -0.01;
                fMaxProximityDist = 0.0; //[m] dojechaæ maksymalnie
                fVelPlus = 0.5; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez
                // hamowania
                fVelMinus = 0.5; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
                if (AIControllFlag)
                { // to robi tylko AI, wersjê dla cz³owieka trzeba dopiero zrobiæ
                    // sprzêgi sprawdzamy w pierwszej kolejnoœci, bo jak po³¹czony, to koniec
                    bool ok; // true gdy siê pod³¹czy (uzyskany sprzêg bêdzie zgodny z ¿¹danym)
                    if (pVehicles[0]->DirectionGet() > 0) // jeœli sprzêg 0
                    { // sprzêg 0 - próba podczepienia
                        if (pVehicles[0]->MoverParameters->Couplers[0].Connected) // jeœli jest coœ
                            // wykryte (a
                            // chyba jest,
                            // nie?)
                            if (pVehicles[0]->MoverParameters->Attach(
                                    0, 2, pVehicles[0]->MoverParameters->Couplers[0].Connected,
                                    iCoupler))
                            {
                                // pVehicles[0]->dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                                // pVehicles[0]->dsbCouplerAttach->Play(0,0,0);
                            }
                        // WriteLog("CoupleDist[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CoupleDist)+",
                        // Connected[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag));
                        ok = (pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag ==
                              iCoupler); // uda³o siê? (mog³o czêœciowo)
                    }
                    else // if (pVehicles[0]->MoverParameters->DirAbsolute<0) //jeœli sprzêg 1
                    { // sprzêg 1 - próba podczepienia
                        if (pVehicles[0]->MoverParameters->Couplers[1].Connected) // jeœli jest coœ
                            // wykryte (a
                            // chyba jest,
                            // nie?)
                            if (pVehicles[0]->MoverParameters->Attach(
                                    1, 2, pVehicles[0]->MoverParameters->Couplers[1].Connected,
                                    iCoupler))
                            {
                                // pVehicles[0]->dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                                // pVehicles[0]->dsbCouplerAttach->Play(0,0,0);
                            }
                        // WriteLog("CoupleDist[1]="+AnsiString(Controlling->Couplers[1].CoupleDist)+",
                        // Connected[0]="+AnsiString(Controlling->Couplers[1].CouplingFlag));
                        ok = (pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag ==
                              iCoupler); // uda³o siê? (mog³o czêœciowo)
                    }
                    if (ok)
                    { // je¿eli zosta³ pod³¹czony
                        iCoupler = 0; // dalsza jazda manewrowa ju¿ bez ³¹czenia
                        iDrivigFlags &= ~moveConnect; // zdjêcie flagi doczepiania
                        SetVelocity(0, 0, stopJoin); // wy³¹czyæ przyspieszanie
                        CheckVehicles(); // sprawdziæ œwiat³a nowego sk³adu
                        JumpToNextOrder(); // wykonanie nastêpnej komendy
                    }
                    else
                        SetVelocity(2.0, 0.0); // jazda w ustawionym kierunku z prêdkoœci¹ 2 (18s)
                } // if (AIControllFlag) //koniec zblokowania, bo by³a zmienna lokalna
                /* //Ra 2014-02: lepiej tam, bo jak tam siê odŸwie¿y sk³ad, to tu pVehicles[0]
                   bêdzie czymœ innym
                     else
                     {//jeœli cz³owiek ma pod³¹czyæ, to czekamy na zmianê stanu sprzêgów na koñcach
                   dotychczasowego sk³adu
                      bool ok; //true gdy siê pod³¹czy (uzyskany sprzêg bêdzie zgodny z ¿¹danym)
                      if (pVehicles[0]->DirectionGet()>0) //jeœli sprzêg 0
                       ok=(pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag>0);
                   //==iCoupler); //uda³o siê? (mog³o czêœciowo)
                      else //if (pVehicles[0]->MoverParameters->DirAbsolute<0) //jeœli sprzêg 1
                       ok=(pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag>0);
                   //==iCoupler); //uda³o siê? (mog³o czêœciowo)
                      if (ok)
                      {//je¿eli zosta³ pod³¹czony
                       iDrivigFlags&=~moveConnect; //zdjêcie flagi doczepiania
                       JumpToNextOrder(); //wykonanie nastêpnej komendy
                      }
                     }
                */
            }
            else
            { // jak daleko, to jazda jak dla Shunt na kolizjê
                fMinProximityDist = 0.0;
                fMaxProximityDist = 5.0; //[m] w takim przedziale odleg³oœci powinien stan¹æ
                fVelPlus = 2.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez
                // hamowania
                fVelMinus = 1.0; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
                // VelReduced=5; //[km/h]
                // if (mvOccupied->Vel<0.5) //jeœli ju¿ prawie stan¹³
                if (pVehicles[0]->fTrackBlock <=
                    20.0) // przy zderzeniu fTrackBlock nie jest miarodajne
                    iDrivigFlags |=
                        moveConnect; // pocz¹tek podczepiania, z wy³¹czeniem sprawdzania fTrackBlock
            }
            break;
        case Disconnect: // 20.07.03 - manewrowanie wagonami
            fMinProximityDist = 1.0;
            fMaxProximityDist = 10.0; //[m]
            fVelPlus = 1.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
            fVelMinus = 0.5; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
            if (AIControllFlag)
            {
                if (iVehicleCount >= 0) // jeœli by³a podana iloœæ wagonów
                {
                    if (iDrivigFlags & movePress) // jeœli dociskanie w celu odczepienia
                    { // 3. faza odczepiania.
                        SetVelocity(2, 0); // jazda w ustawionym kierunku z prêdkoœci¹ 2
                        if ((mvControlling->MainCtrlPos > 0) ||
                            (mvOccupied->BrakeSystem == ElectroPneumatic)) // jeœli jazda
                        {
                            // WriteLog("Odczepianie w kierunku
                            // "+AnsiString(mvOccupied->DirAbsolute));
                            TDynamicObject *p =
                                pVehicle; // pojazd do odczepienia, w (pVehicle) siedzi AI
                            int d; // numer sprzêgu, który sprawdzamy albo odczepiamy
                            int n = iVehicleCount; // ile wagonów ma zostaæ
                            do
                            { // szukanie pojazdu do odczepienia
                                d = p->DirectionGet() > 0 ?
                                        0 :
                                        1; // numer sprzêgu od strony czo³a sk³adu
                                // if (p->MoverParameters->Couplers[d].CouplerType==Articulated)
                                // //jeœli sprzêg typu wózek (za ma³o)
                                if (p->MoverParameters->Couplers[d].CouplingFlag &
                                    ctrain_depot) // je¿eli sprzêg zablokowany
                                    // if (p->GetTrack()->) //a nie stoi na torze warsztatowym
                                    // (ustaliæ po czym poznaæ taki tor)
                                    ++n; // to  liczymy cz³ony jako jeden
                                p->MoverParameters->BrakeReleaser(
                                    1); // wyluzuj pojazd, aby da³o siê dopychaæ
                                p->MoverParameters->BrakeLevelSet(
                                    0); // hamulec na zero, aby nie hamowa³
                                if (n)
                                { // jeœli jeszcze nie koniec
                                    p = p->Prev(); // kolejny w stronê czo³a sk³adu (licz¹c od
                                    // ty³u), bo dociskamy
                                    if (!p)
                                        iVehicleCount = -2,
                                        n = 0; // nie ma co dalej sprawdzaæ, doczepianie zakoñczone
                                }
                            } while (n--);
                            if (p ? p->MoverParameters->Couplers[d].CouplingFlag == 0 : true)
                                iVehicleCount = -2; // odczepiono, co by³o do odczepienia
                            else if (!p->Dettach(d)) // zwraca maskê bitow¹ po³¹czenia; usuwa
                            // w³asnoœæ pojazdów
                            { // tylko jeœli odepnie
                                // WriteLog("Odczepiony od strony ");
                                iVehicleCount = -2;
                            } // a jak nie, to dociskaæ dalej
                        }
                        if (iVehicleCount >= 0) // zmieni siê po odczepieniu
                            if (!mvOccupied->DecLocalBrakeLevel(1))
                            { // dociœnij sklad
                                // WriteLog("Dociskanie");
                                // mvOccupied->BrakeReleaser(); //wyluzuj lokomotywê
                                // Ready=true; //zamiast sprawdzenia odhamowania ca³ego sk³adu
                                IncSpeed(); // dla (Ready)==false nie ruszy
                            }
                    }
                    if ((mvOccupied->Vel == 0.0) && !(iDrivigFlags & movePress))
                    { // 2. faza odczepiania: zmieñ kierunek na przeciwny i dociœnij
                        // za rad¹ yB ustawiamy pozycjê 3 kranu (ruszanie kranem w innych miejscach
                        // powino zostaæ wy³¹czone)
                        // WriteLog("Zahamowanie sk³adu");
                        // while ((mvOccupied->BrakeCtrlPos>3)&&mvOccupied->DecBrakeLevel());
                        // while ((mvOccupied->BrakeCtrlPos<3)&&mvOccupied->IncBrakeLevel());
                        mvOccupied->BrakeLevelSet(mvOccupied->BrakeSystem == ElectroPneumatic ? 1 :
                                                                                                3);
                        double p = mvOccupied->BrakePressureActual
                                       .PipePressureVal; // tu mo¿e byæ 0 albo -1 nawet
                        if (p < 3.9)
                            p = 3.9; // TODO: zabezpieczenie przed dziwnymi CHK do czasu wyjaœnienia
                        // sensu 0 oraz -1 w tym miejscu
                        if (mvOccupied->BrakeSystem == ElectroPneumatic ?
                                mvOccupied->BrakePress > 2 :
                                mvOccupied->PipePress < p + 0.1)
                        { // jeœli w miarê zosta³ zahamowany (ciœnienie mniejsze ni¿ podane na
                            // pozycji 3, zwyle 0.37)
                            if (mvOccupied->BrakeSystem == ElectroPneumatic)
                                mvOccupied->BrakeLevelSet(0); // wy³¹czenie EP, gdy wystarczy (mo¿e
                            // nie byæ potrzebne, bo na pocz¹tku
                            // jest)
                            // WriteLog("Luzowanie lokomotywy i zmiana kierunku");
                            mvOccupied->BrakeReleaser(1); // wyluzuj lokomotywê; a ST45?
                            mvOccupied->DecLocalBrakeLevel(10); // zwolnienie hamulca
                            iDrivigFlags |= movePress; // nastêpnie bêdzie dociskanie
                            DirectionForward(mvOccupied->ActiveDir <
                                             0); // zmiana kierunku jazdy na przeciwny (dociskanie)
                            CheckVehicles(); // od razu zmieniæ œwiat³a (zgasiæ) - bez tego siê nie
                            // odczepi
                            fStopTime = 0.0; // nie ma na co czekaæ z odczepianiem
                        }
                    }
                } // odczepiania
                else // to poni¿ej jeœli iloœæ wagonów ujemna
                    if (iDrivigFlags & movePress)
                { // 4. faza odczepiania: zwolnij i zmieñ kierunek
                    SetVelocity(0, 0, stopJoin); // wy³¹czyæ przyspieszanie
                    if (!DecSpeed()) // jeœli ju¿ bardziej wy³¹czyæ siê nie da
                    { // ponowna zmiana kierunku
                        // WriteLog("Ponowna zmiana kierunku");
                        DirectionForward(mvOccupied->ActiveDir <
                                         0); // zmiana kierunku jazdy na w³aœciwy
                        iDrivigFlags &= ~movePress; // koniec dociskania
                        JumpToNextOrder(); // zmieni œwiat³a
                        TableClear(); // skanowanie od nowa
                        iDrivigFlags &= ~moveStartHorn; // bez tr¹bienia przed ruszeniem
                        SetVelocity(fShuntVelocity, fShuntVelocity); // ustawienie prêdkoœci jazdy
                    }
                }
            }
            break;
        case Shunt:
            // na jak¹ odlegloœæ i z jak¹ predkoœci¹ ma podjechaæ
            fMinProximityDist = 2.0;
            fMaxProximityDist = 4.0; //[m]
            fVelPlus = 2.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
            fVelMinus = 3.0; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
            if (fVelMinus > 0.1 * fShuntVelocity)
                fVelMinus =
                    0.1 *
                    fShuntVelocity; // by³y problemy z jazd¹ np. 3km/h podczas ³adowania wagonów
            break;
        case Obey_train:
            // na jaka odleglosc i z jaka predkoscia ma podjechac do przeszkody
            if (mvOccupied->CategoryFlag & 1) // jeœli poci¹g
            {
                fMinProximityDist = 10.0;
                fMaxProximityDist =
                    (mvOccupied->Vel > 0.0) ?
                        20.0 :
                        50.0; //[m] jak stanie za daleko, to niech nie doci¹ga paru metrów
                if (iDrivigFlags & moveLate)
                {
                    fVelMinus = 1.0; // jeœli spóŸniony, to gna
                    fVelPlus =
                        5.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
                }
                else
                { // gdy nie musi siê sprê¿aæ
                    fVelMinus =
                        int(0.05 * VelDesired); // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
                    if (fVelMinus > 5.0)
                        fVelMinus = 5.0;
                    else if (fVelMinus < 1.0)
                        fVelMinus = 1.0; //¿eby nie rusza³ przy 0.1
                    fVelPlus = int(
                        0.5 +
                        0.05 * VelDesired); // normalnie dopuszczalne przekroczenie to 5% prêdkoœci
                    if (fVelPlus > 5.0)
                        fVelPlus = 5.0; // ale nie wiêcej ni¿ 5km/h
                }
            }
            else // samochod (sokista te¿)
            {
                fMinProximityDist = 7.0;
                fMaxProximityDist = 10.0; //[m]
                fVelPlus =
                    10.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
                fVelMinus = 2.0; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
            }
            // VelReduced=4; //[km/h]
            break;
        default:
            fMinProximityDist = 0.01;
            fMaxProximityDist = 2.0; //[m]
            fVelPlus = 2.0; // dopuszczalne przekroczenie prêdkoœci na ograniczeniu bez hamowania
            fVelMinus = 5.0; // margines prêdkoœci powoduj¹cy za³¹czenie napêdu
        } // switch
        switch (OrderList[OrderPos])
        { // co robi maszynista
        case Prepare_engine: // odpala silnik
            // if (AIControllFlag)
            if (PrepareEngine()) // dla u¿ytkownika tylko sprawdza, czy uruchomi³
            { // gotowy do drogi?
                SetDriverPsyche();
                // OrderList[OrderPos]:=Shunt; //Ra: to nie mo¿e tak byæ, bo scenerie robi¹
                // Jump_to_first_order i przechodzi w manewrowy
                JumpToNextOrder(); // w nastêpnym jest Shunt albo Obey_train, moze te¿ byæ
                // Change_direction, Connect albo Disconnect
                // if OrderList[OrderPos]<>Wait_for_Orders)
                // if BrakeSystem=Pneumatic)  //napelnianie uderzeniowe na wstepie
                //  if BrakeSubsystem=Oerlikon)
                //   if (BrakeCtrlPos=0))
                //    DecBrakeLevel;
            }
            break;
        case Release_engine:
            if (ReleaseEngine()) // zdana maszyna?
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
        case Wait_for_orders: // jeœli czeka, te¿ ma skanowaæ, ¿eby odpaliæ siê od semafora
        /*
             if ((mvOccupied->ActiveDir!=0))
             {//jeœli jest wybrany kierunek jazdy, mo¿na ustaliæ prêdkoœæ jazdy
              VelDesired=fVelMax; //wstêpnie prêdkoœæ maksymalna dla pojazdu(-ów), bêdzie nastêpnie
           ograniczana
              SetDriverPsyche(); //ustawia AccPreferred (potrzebne tu?)
              //Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prêdkosci
              AccDesired=AccPreferred; //AccPreferred wynika z osobowoœci mechanika
              VelNext=VelDesired; //maksymalna prêdkoœæ wynikaj¹ca z innych czynników ni¿
           trajektoria ruchu
              ActualProximityDist=scanmax; //funkcja Update() mo¿e pozostawiæ wartoœci bez zmian
              //hm, kiedyœ semafory wysy³a³y SetVelocity albo ShuntVelocity i ustaw³y tak VelSignal
           - a teraz jak to zrobiæ?
              TCommandType
           comm=TableUpdate(mvOccupied->Vel,VelDesired,ActualProximityDist,VelNext,AccDesired);
           //szukanie optymalnych wartoœci
             }
        */
        // break;
        case Shunt:
        case Obey_train:
        case Connect:
        case Disconnect:
        case Change_direction: // tryby wymagaj¹ce jazdy
        case Change_direction | Shunt: // zmiana kierunku podczas manewrów
        case Change_direction | Connect: // zmiana kierunku podczas pod³¹czania
            if (OrderList[OrderPos] != Obey_train) // spokojne manewry
            {
                VelSignal = Global::Min0RSpeed(VelSignal, 40); // jeœli manewry, to ograniczamy prêdkoœæ
                if (AIControllFlag)
                { // to poni¿ej tylko dla AI
                    if (iVehicleCount >= 0) // jeœli jest co odczepiæ
                        if (!(iDrivigFlags & movePress))
                            if (mvOccupied->Vel > 0.0)
                                if (!iCoupler) // jeœli nie ma wczeœniej potrzeby podczepienia
                                {
                                    SetVelocity(0, 0, stopJoin); // 1. faza odczepiania: zatrzymanie
                                    // WriteLog("Zatrzymanie w celu odczepienia");
                                }
                }
            }
            else
                SetDriverPsyche(); // Ra: by³o w PrepareEngine(), potrzebne tu?
            // no albo przypisujemy -WaitingExpireTime, albo porównujemy z WaitingExpireTime
            // if
            // ((VelSignal==0.0)&&(WaitingTime>WaitingExpireTime)&&(mvOccupied->RunningTrack.Velmax!=0.0))
            if (OrderList[OrderPos] &
                (Shunt | Obey_train | Connect)) // odjechaæ sam mo¿e tylko jeœli jest w trybie jazdy
            { // automatyczne ruszanie po odstaniu albo spod SBL
                if ((VelSignal == 0.0) && (WaitingTime > 0.0) &&
                    (mvOccupied->RunningTrack.Velmax != 0.0))
                { // jeœli stoi, a up³yn¹³ czas oczekiwania i tor ma niezerow¹ prêdkoœæ
                    /*
                         if (WriteLogFlag)
                          {
                            append(AIlogFile);
                            writeln(AILogFile,ElapsedTime:5:2,": ",Name," V=0 waiting time expired!
                       (",WaitingTime:4:1,")");
                            close(AILogFile);
                          }
                    */
                    if ((OrderList[OrderPos] & (Obey_train | Shunt)) ?
                            (iDrivigFlags & moveStopHere) :
                            false)
                        WaitingTime = -WaitingExpireTime; // zakaz ruszania z miejsca bez otrzymania
                    // wolnej drogi
                    else if (mvOccupied->CategoryFlag & 1)
                    { // jeœli poci¹g
                        if (AIControllFlag)
                        {
                            PrepareEngine(); // zmieni ustawiony kierunek
                            SetVelocity(20, 20); // jak siê nasta³, to niech jedzie 20km/h
                            WaitingTime = 0.0;
                            fWarningDuration = 1.5; // a zatr¹biæ trochê
                            mvOccupied->WarningSignal = 1;
                        }
                        else
                            SetVelocity(20, 20); // u¿ytkownikowi zezwalamy jechaæ
                    }
                    else
                    { // samochód ma staæ, a¿ dostanie odjazd, chyba ¿e stoi przez kolizjê
                        if (eStopReason == stopBlock)
                            if (pVehicles[0]->fTrackBlock > fDriverDist)
                                if (AIControllFlag)
                                {
                                    PrepareEngine(); // zmieni ustawiony kierunek
                                    SetVelocity(-1, -1); // jak siê nasta³, to niech jedzie
                                    WaitingTime = 0.0;
                                }
                                else
                                    SetVelocity(-1,
                                                -1); // u¿ytkownikowi pozwalamy jechaæ (samochodem?)
                    }
                }
                else if ((VelSignal == 0.0) && (VelNext > 0.0) && (mvOccupied->Vel < 1.0))
                    if (iCoupler ? true : (iDrivigFlags & moveStopHere) == 0) // Ra: tu jest coœ nie
                        // tak, bo bez tego
                        // warunku rusza³o w
                        // manewrowym !!!!
                        SetVelocity(VelNext, VelNext, stopSem); // omijanie SBL
            } // koniec samoistnego odje¿d¿ania
            if (AIControllFlag)
                if ((HelpMeFlag) || (mvControlling->DamageFlag > 0))
                {
                    HelpMeFlag = false;
                    /*
                          if (WriteLogFlag)
                           with Controlling do
                            {
                              append(AIlogFile);
                              writeln(AILogFile,ElapsedTime:5:2,": ",Name," HelpMe!
                       (",DamageFlag,")");
                              close(AILogFile);
                            }
                    */
                }
            if (AIControllFlag)
                if (OrderList[OrderPos] &
                    Change_direction) // mo¿e byæ zmieszane z jeszcze jak¹œ komend¹
                { // sprobuj zmienic kierunek
                    SetVelocity(0, 0, stopDir); // najpierw trzeba siê zatrzymaæ
                    if (mvOccupied->Vel < 0.1)
                    { // jeœli siê zatrzyma³, to zmieniamy kierunek jazdy, a nawet kabinê/cz³on
                        Activation(); // ustawienie zadanego wczeœniej kierunku i ewentualne
                        // przemieszczenie AI
                        PrepareEngine();
                        JumpToNextOrder(); // nastêpnie robimy, co jest do zrobienia (Shunt albo
                        // Obey_train)
                        if (OrderList[OrderPos] & (Shunt | Connect)) // jeœli dalej mamy manewry
                            if ((iDrivigFlags & moveStopHere) == 0) // o ile nie staæ w miejscu
                            { // jechaæ od razu w przeciwn¹ stronê i nie tr¹biæ z tego tytu³u
                                iDrivigFlags &= ~moveStartHorn; // bez tr¹bienia przed ruszeniem
                                SetVelocity(fShuntVelocity, fShuntVelocity); // to od razu jedziemy
                            }
                        // iDrivigFlags|=moveStartHorn; //a póŸniej ju¿ mo¿na tr¹biæ
                        /*
                               if (WriteLogFlag)
                               {
                                append(AIlogFile);
                                writeln(AILogFile,ElapsedTime:5:2,": ",Name," Direction changed!");
                                close(AILogFile);
                               }
                        */
                    }
                    // else
                    // VelSignal:=0.0; //na wszelki wypadek niech zahamuje
                } // Change_direction (tylko dla AI)
            // ustalanie zadanej predkosci
            if (AIControllFlag) // jeœli prowadzi AI
                if (!iEngineActive) // jeœli silnik nie odpalony, to próbowaæ naprawiæ
                    if (OrderList[OrderPos] & (Change_direction | Connect | Disconnect | Shunt |
                                               Obey_train)) // jeœli coœ ma robiæ
                        PrepareEngine(); // to niech odpala do skutku
            if (iDrivigFlags & moveActive) // jeœli mo¿e skanowaæ sygna³y i reagowaæ na komendy
            { // jeœli jest wybrany kierunek jazdy, mo¿na ustaliæ prêdkoœæ jazdy
                // Ra: tu by jeszcze trzeba by³o wstawiæ uzale¿nienie (VelDesired) od odleg³oœci od
                // przeszkody
                // no chyba ¿eby to uwzgldniæ ju¿ w (ActualProximityDist)
                VelDesired = fVelMax; // wstêpnie prêdkoœæ maksymalna dla pojazdu(-ów), bêdzie
                // nastêpnie ograniczana
                if (TrainParams) // jeœli ma rozk³ad
                    if (TrainParams->TTVmax > 0.0) // i ograniczenie w rozk³adzie
                        VelDesired = Global::Min0RSpeed(VelDesired,
                                           TrainParams->TTVmax); // to nie przekraczaæ rozkladowej
                SetDriverPsyche(); // ustawia AccPreferred (potrzebne tu?)
                // Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prêdkosci
                AccDesired = AccPreferred; // AccPreferred wynika z osobowoœci mechanika
                VelNext = VelDesired; // maksymalna prêdkoœæ wynikaj¹ca z innych czynników ni¿
                // trajektoria ruchu
                ActualProximityDist = scanmax; // funkcja Update() mo¿e pozostawiæ wartoœci bez
                // zmian
                // hm, kiedyœ semafory wysy³a³y SetVelocity albo ShuntVelocity i ustaw³y tak
                // VelSignal - a teraz jak to zrobiæ?
                TCommandType comm = TableUpdate(VelDesired, ActualProximityDist, VelNext,
                                                AccDesired); // szukanie optymalnych wartoœci
                // if (VelSignal!=VelDesired) //je¿eli prêdkoœæ zalecana jest inna (ale tryb te¿
                // mo¿e byæ inny)
                switch (comm)
                { // ustawienie VelSignal - trochê proteza = do przemyœlenia
                case cm_Ready: // W4 zezwoli³ na jazdê
                    TableCheck(
                        scanmax); // ewentualne doskanowanie trasy za W4, który zezwoli³ na jazdê
                    TableUpdate(VelDesired, ActualProximityDist, VelNext,
                                AccDesired); // aktualizacja po skanowaniu
                    // if (comm!=cm_SetVelocity) //jeœli dalej jest kolejny W4, to ma zwróciæ
                    // cm_SetVelocity
                    if (VelNext == 0.0)
                        break; // ale jak coœ z przodu zamyka, to ma staæ
                    if (iDrivigFlags & moveStopCloser)
                        VelSignal = -1.0; // niech jedzie, jak W4 puœci³o - nie, ma czekaæ na
                // sygna³ z sygnalizatora!
                case cm_SetVelocity: // od wersji 357 semafor nie budzi wy³¹czonej lokomotywy
                    if (!(OrderList[OrderPos] &
                          ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                        if (fabs(VelSignal) >=
                            1.0) // 0.1 nie wysy³a siê do samochodow, bo potem nie rusz¹
                            PutCommand("SetVelocity", VelSignal, VelNext,
                                       NULL); // komenda robi dodatkowe operacje
                    break;
                case cm_ShuntVelocity: // od wersji 357 Tm nie budzi wy³¹czonej lokomotywy
                    if (!(OrderList[OrderPos] &
                          ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                        PutCommand("ShuntVelocity", VelSignal, VelNext, NULL);
                    else if (iCoupler) // jeœli jedzie w celu po³¹czenia
                        SetVelocity(VelSignal, VelNext);
                    break;
                case cm_Command: // komenda z komórki
                    if (!(OrderList[OrderPos] &
                          ~(Obey_train | Shunt))) // jedzie w dowolnym trybie albo Wait_for_orders
                        if (mvOccupied->Vel < 0.1) // dopiero jak stanie
                        // iDrivigFlags|=moveStopHere moveStopCloser) //chyba ¿e stan¹³ za daleko
                        // (SU46 w WK staje za daleko)
                        {
                            PutCommand(eSignNext->CommandGet(), eSignNext->ValueGet(1),
                                       eSignNext->ValueGet(2), NULL);
                            eSignNext->StopCommandSent(); // siê wykona³o ju¿
                        }
                    break;
                }
                if (VelNext == 0.0)
                    if (!(OrderList[OrderPos] &
                          ~(Shunt | Connect))) // jedzie w Shunt albo Connect, albo Wait_for_orders
                    { // je¿eli wolnej drogi nie ma, a jest w trybie manewrowym albo oczekiwania
                        // if
                        // ((OrderList[OrderPos]&Connect)?pVehicles[0]->fTrackBlock>ActualProximityDist:true)
                        // //pomiar odleg³oœci nie dzia³a dobrze?
                        // w trybie Connect skanowaæ do ty³u tylko jeœli przed kolejnym sygna³em nie
                        // ma taboru do pod³¹czenia
                        // Ra 2F1H: z tym (fTrackBlock) to nie jest najlepszy pomys³, bo lepiej by
                        // by³o porównaæ z odleg³oœci¹ od sygnalizatora z przodu
                        if ((OrderList[OrderPos] & Connect) ? (pVehicles[0]->fTrackBlock > 2000 || pVehicles[0]->fTrackBlock > FirstSemaphorDist) :
                                                              true)
                            if ((comm = BackwardScan()) != cm_Unknown) // jeœli w drug¹ mo¿na jechaæ
                            { // nale¿y sprawdzaæ odleg³oœæ od znalezionego sygnalizatora,
                                // aby w przypadku prêdkoœci 0.1 wyci¹gn¹æ najpierw sk³ad za
                                // sygnalizator
                                // i dopiero wtedy zmieniæ kierunek jazdy, oczekuj¹c podania
                                // prêdkoœci >0.5
                                if (comm == cm_Command) // jeœli komenda Shunt
                                    iDrivigFlags |=
                                        moveStopHere; // to j¹ odbierz bez przemieszczania siê (np.
                                // odczep wagony po dopchniêciu do koñca toru)
                                iDirectionOrder = -iDirection; // zmiana kierunku jazdy
                                OrderList[OrderPos] = TOrders(OrderList[OrderPos] |
                                                              Change_direction); // zmiana kierunku
                                // bez psucia
                                // kolejnych komend
                            }
                    }
                double vel = mvOccupied->Vel; // prêdkoœæ w kierunku jazdy
                if (iDirection * mvOccupied->V < 0)
                    vel = -vel; // ujemna, gdy jedzie w przeciwn¹ stronê, ni¿ powinien
                if (VelDesired < 0.0)
                    VelDesired = fVelMax; // bo VelDesired<0 oznacza prêdkoœæ maksymaln¹
                // Ra: jazda na widocznoœæ
                if (pVehicles[0]->fTrackBlock < 1000.0) // przy 300m sta³ z zapamiêtan¹ kolizj¹
                { // Ra 2F3F: przy jeŸdzie poci¹gowej nie powinien doje¿d¿aæ do poprzedzaj¹cego
                    // sk³adu
                    if ((mvOccupied->CategoryFlag & 1) ?
                            ((OrderCurrentGet() & (Connect | Obey_train)) == Obey_train) :
                            false) // jeœli jesteœmy poci¹giem a jazda poci¹gowa i nie œci¹ganie ze
                    // szlaku
                    {
                        pVehicles[0]->ABuScanObjects(pVehicles[0]->DirectionGet(),
                                                     1000.0); // skanowanie sprawdzaj¹ce
                        // Ra 2F3F: i jest problem, jak droga za semaforem kieruje na jakiœ pojazd
                        // (np. w Skwarkach na ET22)
                        if (pVehicles[0]->fTrackBlock < 1000.0) // i jeœli nadal coœ jest
                            if (VelNext != 0.0) // a nastêpny sygna³ zezwala na jazdê
                                if (pVehicles[0]->fTrackBlock <
                                    ActualProximityDist) // i jest bli¿ej (tu by trzeba by³o wstawiæ
                                    // odleg³oœæ do semafora, z pominiêciem SBL
                                    VelDesired = 0.0; // to stoimy
                    }
                    else
                        pVehicles[0]->ABuScanObjects(pVehicles[0]->DirectionGet(),
                                                     300.0); // skanowanie sprawdzaj¹ce
                }
                // if (mvOccupied->Vel>=0.1) //o ile jedziemy; jak stoimy to te¿ trzeba jakoœ
                // zatrzymywaæ
                if ((iDrivigFlags & moveConnect) == 0) // przy koñcówce pod³¹czania nie hamowaæ
                { // sprawdzenie jazdy na widocznoœæ
                    TCoupling *c =
                        pVehicles[0]->MoverParameters->Couplers +
                        (pVehicles[0]->DirectionGet() > 0 ? 0 : 1); // sprzêg z przodu sk³adu
                    if (c->Connected) // a mamy coœ z przodu
                        if (c->CouplingFlag ==
                            0) // jeœli to coœ jest pod³¹czone sprzêgiem wirtualnym
                        { // wyliczanie optymalnego przyspieszenia do jazdy na widocznoœæ
                            double k = c->Connected->Vel; // prêdkoœæ pojazdu z przodu (zak³adaj¹c,
                            // ¿e jedzie w tê sam¹ stronê!!!)
                            if (k < vel + 10) // porównanie modu³ów prêdkoœci [km/h]
                            { // zatroszczyæ siê trzeba, jeœli tamten nie jedzie znacz¹co szybciej
                                double d =
                                    pVehicles[0]->fTrackBlock - 0.5 * vel -
                                    fMaxProximityDist; // odleg³oœæ bezpieczna zale¿y od prêdkoœci
                                if (d < 0) // jeœli odleg³oœæ jest zbyt ma³a
                                { // AccPreferred=-0.9; //hamowanie maksymalne, bo jest za blisko
                                    if (k < 10.0) // k - prêdkoœæ tego z przodu
                                    { // jeœli tamten porusza siê z niewielk¹ prêdkoœci¹ albo stoi
                                        if (OrderCurrentGet() & Connect)
                                        { // jeœli spinanie, to jechaæ dalej
                                            AccPreferred = 0.2; // nie hamuj
                                            VelNext = VelDesired = 2.0; // i pakuj siê na tamtego
                                        }
                                        else // a normalnie to hamowaæ
                                        {
                                            AccPreferred = -1.0; // to hamuj maksymalnie
                                            VelNext = VelDesired = 0.0; // i nie pakuj siê na
                                            // tamtego
                                        }
                                    }
                                    else // jeœli oba jad¹, to przyhamuj lekko i ogranicz prêdkoœæ
                                    {
                                        if (k < vel) // jak tamten jedzie wolniej
                                            if (d < fBrakeDist) // a jest w drodze hamowania
                                            {
                                                if (AccPreferred > fAccThreshold)
                                                    AccPreferred =
                                                        fAccThreshold; // to przyhamuj troszkê
                                                VelNext = VelDesired = int(k); // to chyba ju¿ sobie
                                                // dohamuje wed³ug
                                                // uznania
                                            }
                                    }
                                    ReactionTime = 0.1; // orientuj siê, bo jest goraco
                                }
                                else
                                { // jeœli odleg³oœæ jest wiêksza, ustaliæ maksymalne mo¿liwe
                                    // przyspieszenie (hamowanie)
                                    k = (k * k - vel * vel) / (25.92 * d); // energia kinetyczna
                                    // dzielona przez masê i
                                    // drogê daje
                                    // przyspieszenie
                                    if (k > 0.0)
                                        k *= 1.5; // jedŸ szybciej, jeœli mo¿esz
                                    // double ak=(c->Connected->V>0?1.0:-1.0)*c->Connected->AccS;
                                    // //przyspieszenie tamtego
                                    if (d < fBrakeDist) // a jest w drodze hamowania
                                        if (k < AccPreferred)
                                        { // jeœli nie ma innych powodów do wolniejszej jazdy
                                            AccPreferred = k;
                                            if (VelNext > c->Connected->Vel)
                                            {
                                                VelNext =
                                                    c->Connected
                                                        ->Vel; // ograniczenie do prêdkoœci tamtego
                                                ActualProximityDist =
                                                    d; // i odleg³oœæ od tamtego jest istotniejsza
                                            }
                                            ReactionTime = 0.2; // zwiêksz czujnoœæ
                                        }
#if LOGVELOCITY
                                    WriteLog("Collision: AccPreferred=" + AnsiString(k));
#endif
                                }
                            }
                        }
                }
                // sprawdzamy mo¿liwe ograniczenia prêdkoœci
                if (OrderCurrentGet() & (Shunt | Obey_train)) // w Connect nie, bo moveStopHere
                    // odnosi siê do stanu po po³¹czeniu
                    if (iDrivigFlags & moveStopHere) // jeœli ma czekaæ na woln¹ drogê
                        if (vel == 0.0) // a stoi
                            if (VelNext == 0.0) // a wyjazdu nie ma
                                VelDesired = 0.0; // to ma staæ
                if (fStopTime < 0) // czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
                    VelDesired = 0.0; // jak ma czekaæ, to nie ma jazdy
                // else if (VelSignal<0)
                // VelDesired=fVelMax; //ile fabryka dala (Ra: uwzglêdione wagony)
                else if (VelSignal >= 0) // jeœli sk³ad by³ zatrzymany na pocz¹tku i teraz ju¿ mo¿e jechaæ
                    VelDesired = Global::Min0RSpeed(VelDesired, VelSignal);
				
				if (mvOccupied->RunningTrack.Velmax >=
                    0) // ograniczenie prêdkoœci z trajektorii ruchu
                    VelDesired =
                        Global::Min0RSpeed(VelDesired,
                              mvOccupied->RunningTrack.Velmax); // uwaga na ograniczenia szlakowej!
                if (VelforDriver >= 0) // tu jest zero przy zmianie kierunku jazdy
                    VelDesired = Global::Min0RSpeed(VelDesired, VelforDriver); // Ra: tu mo¿e byæ 40, jeœli
                // mechanik nie ma znajomoœci
                // szlaaku, albo kierowca jeŸdzi
                // 70
                if (TrainParams)
                    if (TrainParams->CheckTrainLatency() < 5.0)
                        if (TrainParams->TTVmax > 0.0)
                            VelDesired = Global::Min0RSpeed(
                                VelDesired,
                                TrainParams
                                    ->TTVmax); // jesli nie spozniony to nie przekraczaæ rozkladowej
                if (VelDesired > 0.0)
                    if ((sSemNext && sSemNext->fVelNext != 0.0) || (iDrivigFlags & moveStopHere)==0)
                    { // jeœli mo¿na jechaæ, to odpaliæ dŸwiêk kierownika oraz zamkn¹æ drzwi w
                        // sk³adzie, jeœli nie mamy czekaæ na sygna³ te¿ trzeba odpaliæ
                        if (iDrivigFlags & moveGuardSignal)
                        { // komunikat od kierownika tu, bo musi byæ wolna droga i odczekany czas
                            // stania
                            iDrivigFlags &= ~moveGuardSignal; // tylko raz nadaæ
                            tsGuardSignal->Stop();
                            // w zasadzie to powinien mieæ flagê, czy jest dŸwiêkiem radiowym, czy
                            // bezpoœrednim
                            // albo trzeba zrobiæ dwa dŸwiêki, jeden bezpoœredni, s³yszalny w
                            // pobli¿u, a drugi radiowy, s³yszalny w innych lokomotywach
                            // na razie zak³adam, ¿e to nie jest dŸwiêk radiowy, bo trzeba by zrobiæ
                            // obs³ugê kana³ów radiowych itd.
                            if (!iGuardRadio) // jeœli nie przez radio
                                tsGuardSignal->Play(
                                    1.0, 0, !FreeFlyModeFlag,
                                    pVehicle->GetPosition()); // dla true jest g³oœniej
                            else
                                // if (iGuardRadio==iRadioChannel) //zgodnoœæ kana³u
                                // if (!FreeFlyModeFlag) //obserwator musi byæ w œrodku pojazdu
                                // (albo mo¿e mieæ radio przenoœne) - kierownik móg³by powtarzaæ
                                // przy braku reakcji
                                if (SquareMagnitude(pVehicle->GetPosition() -
                                                    Global::pCameraPosition) <
                                    2000 * 2000) // w odleg³oœci mniejszej ni¿ 2km
                                tsGuardSignal->Play(
                                    1.0, 0, true,
                                    pVehicle->GetPosition()); // dŸwiêk niby przez radio
                        }
                        if (iDrivigFlags & moveDoorOpened) // jeœli drzwi otwarte
                            if (!mvOccupied
                                     ->DoorOpenCtrl) // jeœli drzwi niesterowane przez maszynistê
                                Doors(false); // a EZT zamknie dopiero po odegraniu komunikatu
                        // kierownika
                    }
                if (mvOccupied->V == 0.0)
                    AbsAccS = fAccGravity; // Ra 2014-03: jesli sk³ad stoi, to dzia³a na niego
                // sk³adowa styczna grawitacji
                else
                    AbsAccS = iDirection * mvOccupied->AccS; // przyspieszenie chwilowe, liczone
// jako ró¿nica skierowanej prêdkoœci w
// czasie
// if (mvOccupied->V<0.0) AbsAccS=-AbsAccS; //Ra 2014-03: to trzeba przemyœleæ
// if (vel<0) //je¿eli siê stacza w ty³; 2014-03: to jest bez sensu, bo vel>=0
// AbsAccS=-AbsAccS; //to przyspieszenie te¿ dzia³a wtedy w nieodpowiedni¹ stronê
// AbsAccS+=fAccGravity; //wypadkowe przyspieszenie (czy to ma sens?)
#if LOGVELOCITY
                // WriteLog("VelDesired="+AnsiString(VelDesired)+",
                // VelSignal="+AnsiString(VelSignal));
                WriteLog("Vel=" + AnsiString(vel) + ", AbsAccS=" + AnsiString(AbsAccS) +
                         ", AccGrav=" + AnsiString(fAccGravity));
#endif
                // ustalanie zadanego przyspieszenia
                //(ActualProximityDist) - odleg³oœæ do miejsca zmniejszenia prêdkoœci
                //(AccPreferred) - wynika z psychyki oraz uwzglênia ju¿ ewentualne zderzenie z
                // pojazdem z przodu, ujemne gdy nale¿y hamowaæ
                //(AccDesired) - uwzglêdnia sygna³y na drodze ruchu, ujemne gdy nale¿y hamowaæ
                //(fAccGravity) - chwilowe przspieszenie grawitacyjne, ujemne dzia³a przeciwnie do
                // zadanego kierunku jazdy
                //(AbsAccS) - chwilowe przyspieszenie pojazu (uwzglêdnia grawitacjê), ujemne dzia³a
                // przeciwnie do zadanego kierunku jazdy
                //(AccDesired) porównujemy z (fAccGravity) albo (AbsAccS)
                // if ((VelNext>=0.0)&&(ActualProximityDist>=0)&&(mvOccupied->Vel>=VelNext)) //gdy
                // zbliza sie i jest za szybko do NOWEGO
                if ((VelNext >= 0.0) && (ActualProximityDist <= scanmax) && (vel >= VelNext))
                { // gdy zbli¿a siê i jest za szybki do nowej prêdkoœci, albo stoi na zatrzymaniu
                    if (vel > 0.0)
                    { // jeœli jedzie
                        if ((vel < VelNext) ?
                                (ActualProximityDist > fMaxProximityDist * (1 + 0.1 * vel)) :
                                false) // dojedz do semafora/przeszkody
                        { // jeœli jedzie wolniej ni¿ mo¿na i jest wystarczaj¹co daleko, to mo¿na
                            // przyspieszyæ
                            if (AccPreferred > 0.0) // jeœli nie ma zawalidrogi
                                AccDesired = AccPreferred;
                            // VelDesired:=Min0R(VelDesired,VelReduced+VelNext);
                        }
                        else if (ActualProximityDist > fMinProximityDist)
                        { // jedzie szybciej, ni¿ trzeba na koñcu ActualProximityDist, ale jeszcze
                            // jest daleko
                            if (vel <
                                VelNext + 40.0) // dwustopniowe hamowanie - niski przy ma³ej ró¿nicy
                            { // jeœli jedzie wolniej ni¿ VelNext+35km/h //Ra: 40, ¿eby nie
                                // kombinowa³ na zwrotnicach
                                if (VelNext == 0.0)
                                { // jeœli ma siê zatrzymaæ, musi byæ to robione precyzyjnie i
                                    // skutecznie
                                    if (ActualProximityDist <
                                        fMaxProximityDist) // jak min¹³ ju¿ maksymalny dystans
                                    { // po prostu hamuj (niski stopieñ) //ma stan¹æ, a jest w
                                        // drodze hamowania albo ma jechaæ
                                        AccDesired = fAccThreshold; // hamowanie tak, aby stan¹æ
                                        VelDesired = 0.0; // Min0R(VelDesired,VelNext);
                                    }
                                    else if (ActualProximityDist > fBrakeDist)
                                    { // jeœli ma stan¹æ, a mieœci siê w drodze hamowania
                                        if (vel < 10.0) // jeœli prêdkoœæ jest ³atwa do zatrzymania
                                        { // tu jest trochê problem, bo do punktu zatrzymania dobija
                                            // na raty
                                            // AccDesired=AccDesired<0.0?0.0:0.1*AccPreferred;
                                            AccDesired = AccPreferred; // proteza trochê; jak tu
                                            // wychodzi 0.05, to loki
                                            // maj¹ problem utrzymaæ
                                            // takie przyspieszenie
                                        }
                                        else if (vel <= 30.0) // trzymaj 30 km/h
                                            AccDesired = Min0R(0.5 * AccDesired,
                                                               AccPreferred); // jak jest tu 0.5, to
                                        // samochody siê
                                        // dobijaj¹ do siebie
                                        else
                                            AccDesired = 0.0;
                                    }
                                    else // 25.92 (=3.6*3.6*2) - przelicznik z km/h na m/s
                                        if (vel <
                                            VelNext + fVelPlus) // jeœli niewielkie przekroczenie
                                        // AccDesired=0.0;
                                        AccDesired = Min0R(0.0, AccPreferred); // proteza trochê: to
                                    // niech nie hamuje,
                                    // chyba ¿e coœ z
                                    // przodu
                                    else
                                        AccDesired = -(vel * vel) /
                                                     (25.92 * (ActualProximityDist +
                                                               0.1)); //-fMinProximityDist));//-0.1;
                                    ////mniejsze opóŸnienie przy
                                    // ma³ej ró¿nicy
                                    ReactionTime = 0.1; // i orientuj siê szybciej, jak masz stan¹æ
                                }
                                else if (vel < VelNext + fVelPlus) // jeœli niewielkie
                                    // przekroczenie, ale ma jechaæ
                                    AccDesired =
                                        Min0R(0.0, AccPreferred); // to olej (zacznij luzowaæ)
                                else
                                { // jeœli wiêksze przekroczenie ni¿ fVelPlus [km/h], ale ma jechaæ
                                    // Ra 2F1I: jak by³o (VelNext+fVelPlus) tu, to hamowa³ zbyt
                                    // póŸno przed 40, a potem zbyt mocno i zwalnia³ do 30
                                    AccDesired = (VelNext * VelNext - vel * vel) /
                                                 (25.92 * ActualProximityDist +
                                                  0.1); // mniejsze opóŸnienie przy ma³ej ró¿nicy
                                    if (ActualProximityDist < fMaxProximityDist)
                                        ReactionTime = 0.1; // i orientuj siê szybciej, jeœli w
                                    // krytycznym przedziale
                                }
                            }
                            else // przy du¿ej ró¿nicy wysoki stopieñ (1,25 potrzebnego opoznienia)
                                AccDesired = (VelNext * VelNext - vel * vel) /
                                             (20.73 * ActualProximityDist +
                                              0.1); // najpierw hamuje mocniej, potem zluzuje
                            if (AccPreferred < AccDesired)
                                AccDesired = AccPreferred; //(1+abs(AccDesired))
                            // ReactionTime=0.5*mvOccupied->BrakeDelay[2+2*mvOccupied->BrakeDelayFlag];
                            // //aby szybkosc hamowania zalezala od przyspieszenia i opoznienia
                            // hamulcow
                            // fBrakeTime=0.5*mvOccupied->BrakeDelay[2+2*mvOccupied->BrakeDelayFlag];
                            // //aby szybkosc hamowania zalezala od przyspieszenia i opoznienia
                            // hamulcow
                        }
                        else
                        { // jest bli¿ej ni¿ fMinProximityDist
                            VelDesired =
                                Min0R(VelDesired, VelNext); // utrzymuj predkosc bo juz blisko
                            if (vel <
                                VelNext + fVelPlus) // jeœli niewielkie przekroczenie, ale ma jechaæ
                                AccDesired = Min0R(0.0, AccPreferred); // to olej (zacznij luzowaæ)
                            ReactionTime = 0.1; // i orientuj siê szybciej
                        }
                    }
                    else // zatrzymany (vel==0.0)
                        // if (iDrivigFlags&moveStopHere) //to nie dotyczy podczepiania
                        // if ((VelNext>0.0)||(ActualProximityDist>fMaxProximityDist*1.2))
                        if (VelNext > 0.0)
							AccDesired = AccPreferred; // mo¿na jechaæ
						else // jeœli daleko jechaæ nie mo¿na
							if (ActualProximityDist >
								fMaxProximityDist) // ale ma kawa³ek do sygnalizatora
							{ // if ((iDrivigFlags&moveStopHere)?false:AccPreferred>0)
								if (AccPreferred > 0)
									AccDesired = AccPreferred; // dociagnij do semafora;
								else
									VelDesired = 0.0; //,AccDesired=-fabs(fAccGravity); //stoj (hamuj z si³¹
								// równ¹ sk³adowej stycznej grawitacji)
							}
							else
								VelDesired = 0.0; // VelNext=0 i stoi bli¿ej ni¿ fMaxProximityDist
                }
                else // gdy jedzie wolniej ni¿ potrzeba, albo nie ma przeszkód na drodze
                    AccDesired = (VelDesired != 0.0 ? AccPreferred : -0.01); // normalna jazda
                // koniec predkosci nastepnej
                if ((VelDesired >= 0.0) &&
                    (vel > VelDesired)) // jesli jedzie za szybko do AKTUALNEGO
                    if (VelDesired == 0.0) // jesli stoj, to hamuj, ale i tak juz za pozno :)
                        AccDesired = -0.9; // hamuj solidnie
                    else if ((vel < VelDesired + fVelPlus)) // o 5 km/h to olej
                    {
                        if ((AccDesired > 0.0))
                            AccDesired = 0.0;
                    }
                    else
                        AccDesired = fAccThreshold; // hamuj tak œrednio
                // koniec predkosci aktualnej
                if (fAccThreshold > -0.3) // bez sensu, ale dla towarowych korzystnie
                { // Ra 2014-03: to nie uwzglêdnia odleg³oœci i zaczyna hamowaæ, jak tylko zobaczy
                    // W4
                    if ((AccDesired > 0.0) &&
                        (VelNext >= 0.0)) // wybieg b¹dŸ lekkie hamowanie, warunki byly zamienione
                        if (vel > VelNext + 100.0) // lepiej zaczac hamowac
                            AccDesired = fAccThreshold;
                        else if (vel > VelNext + 70.0)
                            AccDesired = 0.0; // nie spiesz siê, bo bêdzie hamowanie
                    // koniec wybiegu i hamowania
                }
                if (AIControllFlag)
                { // czêœæ wykonawcza tylko dla AI, dla cz³owieka jedynie napisy
                    if (mvControlling->ConvOvldFlag ||
                        !mvControlling->Mains) // WS mo¿e wywaliæ z powodu b³êdu w drutach
                    { // wywali³ bezpiecznik nadmiarowy przetwornicy
                        // while (DecSpeed()); //zerowanie napêdu
                        // Controlling->ConvOvldFlag=false; //reset nadmiarowego
                        PrepareEngine(); // próba ponownego za³¹czenia
                    }
                    // w³¹czanie bezpiecznika
                    if ((mvControlling->EngineType == ElectricSeriesMotor) ||
                        (mvControlling->TrainType & dt_EZT) ||
                        (mvControlling->EngineType == DieselElectric))
                        if (mvControlling->FuseFlag || Need_TryAgain)
                        {
                            Need_TryAgain =
                                false; // true, jeœli druga pozycja w elektryku nie za³apa³a
                            // if (!Controlling->DecScndCtrl(1)) //krêcenie po ma³u
                            // if (!Controlling->DecMainCtrl(1)) //nastawnik jazdy na 0
                            mvControlling->DecScndCtrl(2); // nastawnik bocznikowania na 0
                            mvControlling->DecMainCtrl(2); // nastawnik jazdy na 0
                            mvControlling->MainSwitch(
                                true); // Ra: doda³em, bo EN57 stawa³y po wywaleniu
                            if (!mvControlling->FuseOn())
                                HelpMeFlag = true;
                            else
                            {
                                ++iDriverFailCount;
                                if (iDriverFailCount > maxdriverfails)
                                    Psyche = Easyman;
                                if (iDriverFailCount > maxdriverfails * 2)
                                    SetDriverPsyche();
                            }
                        }
                    if (mvOccupied->BrakeSystem == Pneumatic) // nape³nianie uderzeniowe
                        if (mvOccupied->BrakeHandle == FV4a)
                        {
                            if (mvOccupied->BrakeCtrlPos == -2)
                                mvOccupied->BrakeLevelSet(0);
                            //        if
                            //        ((mvOccupied->BrakeCtrlPos<0)&&(mvOccupied->PipeBrakePress<0.01))//{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
                            //         mvOccupied->IncBrakeLevel();
                            if ((mvOccupied->PipePress < 3.0) && (AccDesired > -0.03))
                                mvOccupied->BrakeReleaser(1);
                            if ((mvOccupied->BrakeCtrlPos == 0) && (AbsAccS < 0.0) &&
                                (AccDesired > -0.03))
                                // if FuzzyLogicAI(CntrlPipePress-PipePress,0.01,1))
                                //         if
                                //         ((mvOccupied->BrakePress>0.5)&&(mvOccupied->LocalBrakePos<0.5))//{((Volume/BrakeVVolume/10)<0.485)})
                                if ((mvOccupied->EqvtPipePress < 4.95) &&
                                    (fReady > 0.35)) //{((Volume/BrakeVVolume/10)<0.485)})
                                {
                                    if (iDrivigFlags &
                                        moveOerlikons) // a reszta sk³adu jest na to gotowa
                                        mvOccupied->BrakeLevelSet(-1); // nape³nianie w Oerlikonie
                                }
                                else if (Need_BrakeRelease)
                                {
                                    Need_BrakeRelease = false;
                                    mvOccupied->BrakeReleaser(1);
                                    // DecBrakeLevel(); //z tym by jeszcze mia³o jakiœ sens
                                }
                            //        if
                            //        ((mvOccupied->BrakeCtrlPos<0)&&(mvOccupied->BrakePress<0.3))//{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
                            if ((mvOccupied->BrakeCtrlPos < 0) &&
                                (mvOccupied->EqvtPipePress >
                                 (fReady < 0.25 ?
                                      5.1 :
                                      5.2))) //{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
                                mvOccupied->IncBrakeLevel();
                        }
#if LOGVELOCITY
                    WriteLog("Dist=" + FloatToStrF(ActualProximityDist, ffFixed, 7, 1) +
                             ", VelDesired=" + FloatToStrF(VelDesired, ffFixed, 7, 1) +
                             ", AccDesired=" + FloatToStrF(AccDesired, ffFixed, 7, 3) +
                             ", VelSignal=" + AnsiString(VelSignal) + ", VelNext=" +
                             AnsiString(VelNext));
#endif
                    if (AccDesired > 0.1)
                        if (vel < 10.0) // Ra 2F1H: jeœli prêdkoœæ jest ma³a, a mo¿na przyspieszaæ,
                            // to nie ograniczaæ przyspieszenia do 0.5m/ss
                            AccDesired = 0.9; // przy ma³ych prêdkoœciach mo¿e byæ trudno utrzymaæ
                    // ma³e przyspieszenie
                    // Ra 2F1I: wy³¹czyæ kiedyœ to uœrednianie i przeanalizowaæ skanowanie, czemu
                    // migocze
                    if (AccDesired > -0.15) // hamowania lepeiej nie uœredniaæ
                        AccDesired = fAccDesiredAv =
                            0.2 * AccDesired +
                            0.8 * fAccDesiredAv; // uœrednione, ¿eby ograniczyæ migotanie
                    if (VelDesired == 0.0)
                        if (AccDesired >= -0.01)
                            AccDesired = -0.01; // Ra 2F1J: jeszcze jedna prowizoryczna ³atka
                    if (AccDesired >= 0.0)
                        if (iDrivigFlags & movePress)
                            mvOccupied->BrakeReleaser(1); // wyluzuj lokomotywê - mo¿e byæ wiêcej!
                        else if (OrderList[OrderPos] !=
                                 Disconnect) // przy od³¹czaniu nie zwalniamy tu hamulca
                            if ((AccDesired > 0.0) ||
                                (fAccGravity * fAccGravity <
                                 0.001)) // luzuj tylko na plaskim lub przy ruszaniu
                            {
                                while (DecBrake())
                                    ; // jeœli przyspieszamy, to nie hamujemy
                                if (mvOccupied->BrakePress > 0.4)
                                    mvOccupied->BrakeReleaser(
                                        1); // wyluzuj lokomotywê, to szybciej ruszymy
                            }
                    // margines dla prêdkoœci jest doliczany tylko jeœli oczekiwana prêdkoœæ jest
                    // wiêksza od 5km/h
                    if (!(iDrivigFlags & movePress))
                    { // jeœli nie dociskanie
                        if (AccDesired < -0.1)
                            while (DecSpeed())
                                ; // jeœli hamujemy, to nie przyspieszamy
                        else if (((fAccGravity < -0.01) ? AccDesired < 0.0 :
                                                          AbsAccS > AccDesired) ||
                                 (vel > VelDesired)) // jak za bardzo przyspiesza albo prêdkoœæ
                            // przekroczona
                            DecSpeed(); // pojedyncze cofniêcie pozycji, bo na zero to przesada
                    }
                    // yB: usuniête ró¿ne dziwne warunki, oddzielamy czêœæ zadaj¹c¹ od wykonawczej
                    // zwiekszanie predkosci
                    // Ra 2F1H: jest konflikt histerezy pomiêdzy nastawion¹ pozycj¹ a uzyskiwanym
                    // przyspieszeniem - utrzymanie pozycji powoduje przekroczenie przyspieszenia
                    if (AbsAccS <
                        AccDesired) // jeœli przyspieszenie pojazdu jest mniejsze ni¿ ¿¹dane oraz
                        if (vel < VelDesired - fVelMinus) // jeœli prêdkoœæ w kierunku czo³a jest
                            // mniejsza od dozwolonej o margines
                            if ((ActualProximityDist > fMaxProximityDist) ? true : (vel < VelNext))
                                IncSpeed(); // to mo¿na przyspieszyæ
                    // if ((AbsAccS<AccDesired)&&(vel<VelDesired))
                    // if (!MaxVelFlag) //Ra: to nie jest u¿ywane
                    // if (!IncSpeed()) //Ra: to tutaj jest bez sensu, bo nie doci¹gnie do
                    // bezoporowej
                    // MaxVelFlag=true; //Ra: to nie jest u¿ywane
                    // else
                    // MaxVelFlag=false; //Ra: to nie jest u¿ywane
                    // if (Vel<VelDesired*0.85) and (AccDesired>0) and
                    // (EngineType=ElectricSeriesMotor)
                    // and (RList[MainCtrlPos].R>0.0) and (not DelayCtrlFlag))
                    // if (Im<Imin) and Ready=True {(BrakePress<0.01*MaxBrakePress)})
                    //  IncMainCtrl(1); //zwieksz nastawnik skoro mo¿esz - tak aby siê ustawic na
                    //  bezoporowej

                    // yB: usuniête ró¿ne dziwne warunki, oddzielamy czêœæ zadaj¹c¹ od wykonawczej
                    // zmniejszanie predkosci
                    if (mvOccupied->TrainType &
                        dt_EZT) // w³aœciwie, to warunek powinien byæ na dzia³aj¹cy EP
                    { // Ra: to dobrze hamuje EP w EZT
                        if ((AccDesired <= fAccThreshold) ? // jeœli hamowaæ - u góry ustawia siê
                                // hamowanie na fAccThreshold
                                ((AbsAccS > AccDesired) || (mvOccupied->BrakeCtrlPos < 0)) :
                                false) // hamowaæ bardziej, gdy aktualne opóŸnienie hamowania
                            // mniejsze ni¿ (AccDesired)
                            IncBrake();
                        else if (OrderList[OrderPos] !=
                                 Disconnect) // przy od³¹czaniu nie zwalniamy tu hamulca
                            if (AbsAccS <
                                AccDesired -
                                    0.05) // jeœli opóŸnienie wiêksze od wymaganego (z histerez¹)
                            { // luzowanie, gdy za du¿o
                                if (mvOccupied->BrakeCtrlPos >= 0)
                                    DecBrake(); // tutaj zmniejsza³o o 1 przy odczepianiu
                            }
                            else if (mvOccupied->Handle->TimeEP)
                            {
                                if (mvOccupied->Handle->GetPos(bh_EPR) -
                                        mvOccupied->Handle->GetPos(bh_EPN) <
                                    0.1)
                                    mvOccupied->SwitchEPBrake(0);
                                else
                                    mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EPN));
                            }
                        //         else if (mvOccupied->BrakeCtrlPos<0) IncBrake(); //ustawienie
                        //         jazdy (pozycja 0)
                        //         else if (mvOccupied->BrakeCtrlPos>0) DecBrake();
                    }
                    else
                    { // a stara wersja w miarê dobrze dzia³a na sk³ady wagonowe
                        //       if (mvOccupied->Handle->Time)
                        //         mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_MB));
                        //         //najwyzej sobie przestawi
                        if (((fAccGravity < -0.05) && (vel < 0)) ||
                            ((AccDesired < fAccGravity - 0.1) &&
                             (AbsAccS >
                              AccDesired + 0.05))) // u góry ustawia siê hamowanie na fAccThreshold
                            // if not MinVelFlag)
                            if (fBrakeTime < 0 ? true : (AccDesired < fAccGravity - 0.3) ||
                                                            (mvOccupied->BrakeCtrlPos <= 0))
                                if (!IncBrake()) // jeœli up³yn¹³ czas reakcji hamulca, chyba ¿e
                                    // nag³e albo luzowa³
                                    MinVelFlag = true;
                                else
                                {
                                    MinVelFlag = false;
                                    fBrakeTime =
                                        3 +
                                        0.5 *
                                            (mvOccupied
                                                 ->BrakeDelay[2 + 2 * mvOccupied->BrakeDelayFlag] -
                                             3);
                                    // Ra: ten czas nale¿y zmniejszyæ, jeœli czas dojazdu do
                                    // zatrzymania jest mniejszy
                                    fBrakeTime *= 0.5; // Ra: tymczasowo, bo prze¿yna S1
                                }
                        if ((AccDesired < fAccGravity - 0.05) && (AbsAccS < AccDesired - 0.2))
                        // if ((AccDesired<0.0)&&(AbsAccS<AccDesired-0.1)) //ST44 nie hamuje na
                        // czas, 2-4km/h po miniêciu tarczy
                        // if (fBrakeTime<0)
                        { // jak hamuje, to nie tykaj kranu za czêsto
                            // yB: luzuje hamulec dopiero przy ró¿nicy opóŸnieñ rzêdu 0.2
                            if (OrderList[OrderPos] !=
                                Disconnect) // przy od³¹czaniu nie zwalniamy tu hamulca
                                DecBrake(); // tutaj zmniejsza³o o 1 przy odczepianiu
                            fBrakeTime =
                                (mvOccupied->BrakeDelay[1 + 2 * mvOccupied->BrakeDelayFlag]) / 3.0;
                            fBrakeTime *= 0.5; // Ra: tymczasowo, bo prze¿yna S1
                        }
                    }
                    // Mietek-end1
                    SpeedSet(); // ci¹gla regulacja prêdkoœci
#if LOGVELOCITY
                    WriteLog("BrakePos=" + AnsiString(mvOccupied->BrakeCtrlPos) + ", MainCtrl=" +
                             AnsiString(mvControlling->MainCtrlPos));
#endif

                    /* //Ra: mamy teraz wska¿nik na cz³on silnikowy, gorzej jak s¹ dwa w
                       ukrotnieniu...
                          //zapobieganie poslizgowi w czlonie silnikowym; Ra: Couplers[1] powinno
                       byæ
                          if (Controlling->Couplers[0].Connected!=NULL)
                           if (TestFlag(Controlling->Couplers[0].CouplingFlag,ctrain_controll))
                            if (Controlling->Couplers[0].Connected->SlippingWheels)
                             if (Controlling->ScndCtrlPos>0?!Controlling->DecScndCtrl(1):true)
                             {
                              if (!Controlling->DecMainCtrl(1))
                               if (mvOccupied->BrakeCtrlPos==mvOccupied->BrakeCtrlPosNo)
                                mvOccupied->DecBrakeLevel();
                              ++iDriverFailCount;
                             }
                    */
                    // zapobieganie poslizgowi u nas
                    if (mvControlling->SlippingWheels)
                    {
                        if (!mvControlling->DecScndCtrl(2)) // bocznik na zero
                            mvControlling->DecMainCtrl(1);
                        if (mvOccupied->BrakeCtrlPos ==
                            mvOccupied->BrakeCtrlPosNo) // jeœli ostatnia pozycja hamowania
                            mvOccupied->DecBrakeLevel(); // to cofnij hamulec
                        else
                            mvControlling->AntiSlippingButton();
                        ++iDriverFailCount;
                        mvControlling->SlippingWheels = false; // flaga ju¿ wykorzystana
                    }
                    if (iDriverFailCount > maxdriverfails)
                    {
                        Psyche = Easyman;
                        if (iDriverFailCount > maxdriverfails * 2)
                            SetDriverPsyche();
                    }
                } // if (AIControllFlag)
                else
                { // tu mozna daæ komunikaty tekstowe albo s³owne: przyspiesz, hamuj (lekko,
                    // œrednio, mocno)
                }
            } // kierunek ró¿ny od zera
            else
            { // tutaj, gdy pojazd jest wy³¹czony
                if (!AIControllFlag) // jeœli sterowanie jest w gestii u¿ytkownika
                    if (mvOccupied->Battery) // czy u¿ytkownik za³¹czy³ bateriê?
                        if (mvOccupied->ActiveDir) // czy ustawi³ kierunek
                        { // jeœli tak, to uruchomienie skanowania
                            CheckVehicles(); // sprawdziæ sk³ad
                            TableClear(); // resetowanie tabelki skanowania
                            PrepareEngine(); // uruchomienie
                        }
            }
            if (AIControllFlag)
            { // odhamowywanie sk³adu po zatrzymaniu i zabezpieczanie lokomotywy
                if ((OrderList[OrderPos] & (Disconnect | Connect)) ==
                    0) // przy (p)od³¹czaniu nie zwalniamy tu hamulca
                    if ((mvOccupied->V == 0.0) && ((VelDesired == 0.0) || (AccDesired == 0.0)))
                        if ((mvOccupied->BrakeCtrlPos < 1) || !mvOccupied->DecBrakeLevel())
                            mvOccupied->IncLocalBrakeLevel(1); // dodatkowy na pozycjê 1
            }
            break; // rzeczy robione przy jezdzie
        } // switch (OrderList[OrderPos])
        // kasowanie licznika czasu
        LastReactionTime = 0.0;
        UpdateOK = true;
    } // if ((LastReactionTime>Min0R(ReactionTime,2.0)))
    else
        LastReactionTime += dt;

    if ((fLastStopExpDist > 0.0) && (mvOccupied->DistCounter > fLastStopExpDist))
    {
        iStationStart = TrainParams->StationIndex; // zaktualizowaæ wyœwietlanie rozk³adu
        fLastStopExpDist = -1.0f; // usun¹æ licznik
    }

    if (AIControllFlag)
    {
        if (fWarningDuration > 0.0) // jeœli pozosta³o coœ do wytr¹bienia
        { // tr¹bienie trwa nadal
            fWarningDuration = fWarningDuration - dt;
            if (fWarningDuration < 0.05)
                mvOccupied->WarningSignal = 0; // a tu siê koñczy
            if (ReactionTime > fWarningDuration)
                ReactionTime =
                    fWarningDuration; // wczeœniejszy przeb³ysk œwiadomoœci, by zakoñczyæ tr¹bienie
        }
        if (mvOccupied->Vel >=
            3.0) // jesli jedzie, mo¿na odblokowaæ tr¹bienie, bo siê wtedy nie w³¹czy
        {
            iDrivigFlags &= ~moveStartHornDone; // zatr¹bi dopiero jak nastêpnym razem stanie
            iDrivigFlags |= moveStartHorn; // i tr¹biæ przed nastêpnym ruszeniem
        }
        return UpdateOK;
    }
    else // if (AIControllFlag)
        return false; // AI nie obs³uguje
}

void TController::JumpToNextOrder()
{ // wykonanie kolejnej komendy z tablicy rozkazów
    if (OrderList[OrderPos] != Wait_for_orders)
    {
        if (OrderList[OrderPos] & Change_direction) // jeœli zmiana kierunku
            if (OrderList[OrderPos] != Change_direction) // ale na³o¿ona na coœ
            {
                OrderList[OrderPos] =
                    TOrders(OrderList[OrderPos] &
                            ~Change_direction); // usuniêcie zmiany kierunku z innej komendy
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
    OrdersDump(); // normalnie nie ma po co tego wypisywaæ
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
    OrdersDump(); // normalnie nie ma po co tego wypisywaæ
#endif
};

void TController::OrderCheck()
{ // reakcja na zmianê rozkazu
    if (OrderList[OrderPos] & (Shunt | Connect | Obey_train))
        CheckVehicles(); // sprawdziæ œwiat³a
    if (OrderList[OrderPos] & Change_direction) // mo¿e byæ na³o¿ona na inn¹ i wtedy ma priorytet
        iDirectionOrder = -iDirection; // trzeba zmieniæ jawnie, bo siê nie domyœli
    else if (OrderList[OrderPos] == Obey_train)
        iDrivigFlags |= moveStopPoint; // W4 s¹ widziane
    else if (OrderList[OrderPos] == Disconnect)
        iVehicleCount = iVehicleCount < 0 ? 0 : iVehicleCount; // odczepianie lokomotywy
    else if (OrderList[OrderPos] == Connect)
        iDrivigFlags &= ~moveStopPoint; // podczas jazdy na po³¹czenie nie zwracaæ uwagi na W4
    else if (OrderList[OrderPos] == Wait_for_orders)
        OrdersClear(); // czyszczenie rozkazów i przeskok do zerowej pozycji
}

void TController::OrderNext(TOrders NewOrder)
{ // ustawienie rozkazu do wykonania jako nastêpny
    if (OrderList[OrderPos] == NewOrder)
        return; // jeœli robi to, co trzeba, to koniec
    if (!OrderPos)
        OrderPos = 1; // na pozycji zerowej pozostaje czekanie
    OrderTop = OrderPos; // ale mo¿e jest czymœ zajêty na razie
    if (NewOrder >= Shunt) // jeœli ma jechaæ
    { // ale mo¿e byæ zajêty chwilowymi operacjami
        while (OrderList[OrderTop] ? OrderList[OrderTop] < Shunt : false) // jeœli coœ robi
            ++OrderTop; // pomijamy wszystkie tymczasowe prace
    }
    else
    { // jeœli ma ustawion¹ jazdê, to wy³¹czamy na rzecz operacji
        while (OrderList[OrderTop] ?
                   (OrderList[OrderTop] < Shunt) && (OrderList[OrderTop] != NewOrder) :
                   false) // jeœli coœ robi
            ++OrderTop; // pomijamy wszystkie tymczasowe prace
    }
    OrderList[OrderTop++] = NewOrder; // dodanie rozkazu jako nastêpnego
#if LOGORDERS
    WriteLog("--> OrderNext");
    OrdersDump(); // normalnie nie ma po co tego wypisywaæ
#endif
}

void TController::OrderPush(TOrders NewOrder)
{ // zapisanie na stosie kolejnego rozkazu do wykonania
    if (OrderPos == OrderTop) // jeœli mia³by byæ zapis na aktalnej pozycji
        if (OrderList[OrderPos] < Shunt) // ale nie jedzie
            ++OrderTop; // niektóre operacje musz¹ zostaæ najpierw dokoñczone => zapis na kolejnej
    if (OrderList[OrderTop] != NewOrder) // jeœli jest to samo, to nie dodajemy
        OrderList[OrderTop++] = NewOrder; // dodanie rozkazu na stos
    // if (OrderTop<OrderPos) OrderTop=OrderPos;
    if (OrderTop >= maxorders)
        ErrorLog("Commands overflow: The program will now crash");
#if LOGORDERS
    WriteLog("--> OrderPush");
    OrdersDump(); // normalnie nie ma po co tego wypisywaæ
#endif
}

void TController::OrdersDump()
{ // wypisanie kolejnych rozkazów do logu
    WriteLog("Orders for " + pVehicle->asName + ":");
    for (int b = 0; b < maxorders; ++b)
    {
        WriteLog(AnsiString(b) + ": " + Order2Str(OrderList[b]) + (OrderPos == b ? " <-" : ""));
        if (b) // z wyj¹tkiem pierwszej pozycji
            if (OrderList[b] == Wait_for_orders) // jeœli koñcowa komenda
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
{ // wype³nianie tabelki rozkazów na podstawie rozk³adu
    // ustawienie kolejnoœci komend, niezale¿nie kto prowadzi
    // Mechanik->OrderPush(Wait_for_orders); //czekanie na lepsze czasy
    // OrderPos=OrderTop=0; //wype³niamy od pozycji 0
    OrdersClear(); // usuniêcie poprzedniej tabeli
    OrderPush(Prepare_engine); // najpierw odpalenie silnika
    if (TrainParams->TrainName == AnsiString("none"))
    { // brak rozk³adu to jazda manewrowa
        if (fVel > 0.05) // typowo 0.1 oznacza gotowoœæ do jazdy, 0.01 tylko za³¹czenie silnika
            OrderPush(Shunt); // jeœli nie ma rozk³adu, to manewruje
    }
    else
    { // jeœli z rozk³adem, to jedzie na szlak
        if ((fVel > 0.0) && (fVel < 0.02))
            OrderPush(Shunt); // dla prêdkoœci 0.01 w³¹czamy jazdê manewrow¹
        else if (TrainParams ?
                     (TrainParams->TimeTable[1].StationWare.Pos(
                          "@") ? // jeœli obrót na pierwszym przystanku
                          ((iDrivigFlags &
                            movePushPull) ? // SZT równie¿! SN61 zale¿nie od wagonów...
                               (TrainParams->TimeTable[1].StationName == TrainParams->Relation1) :
                               false) :
                          false) :
                     true)
            OrderPush(Shunt); // a teraz start bêdzie w manewrowym, a tryb poci¹gowy w³¹czy W4
        else
            // jeœli start z pierwszej stacji i jednoczeœnie jest na niej zmiana kierunku, to EZT ma
            // mieæ Shunt
            OrderPush(Obey_train); // dla starych scenerii start w trybie pociagowym
        if (DebugModeFlag) // normalnie nie ma po co tego wypisywaæ
            WriteLog("/* Timetable: " + TrainParams->ShowRelation());
        TMTableLine *t;
        for (int i = 0; i <= TrainParams->StationCount; ++i)
        {
            t = TrainParams->TimeTable + i;
            if (DebugModeFlag) // normalnie nie ma po co tego wypisywaæ
                WriteLog(AnsiString(t->StationName) + " " + AnsiString((int)t->Ah) + ":" +
                         AnsiString((int)t->Am) + ", " + AnsiString((int)t->Dh) + ":" +
                         AnsiString((int)t->Dm) + " " + AnsiString(t->StationWare));
            if (AnsiString(t->StationWare).Pos("@"))
            { // zmiana kierunku i dalsza jazda wg rozk³adu
                if (iDrivigFlags & movePushPull) // SZT równie¿! SN61 zale¿nie od wagonów...
                { // jeœli sk³ad zespolony, wystarczy zmieniæ kierunek jazdy
                    OrderPush(Change_direction); // zmiana kierunku
                }
                else
                { // dla zwyk³ego sk³adu wagonowego odczepiamy lokomotywê
                    OrderPush(Disconnect); // odczepienie lokomotywy
                    OrderPush(Shunt); // a dalej manewry
                    if (i <= TrainParams->StationCount) // 130827: to siê jednak nie sprawdza
                    { //"@" na ostatniej robi tylko odpiêcie
                        // OrderPush(Change_direction); //zmiana kierunku
                        // OrderPush(Shunt); //jazda na drug¹ stronê sk³adu
                        // OrderPush(Change_direction); //zmiana kierunku
                        // OrderPush(Connect); //jazda pod wagony
                    }
                }
                if (i < TrainParams->StationCount) // jak nie ostatnia stacja
                    OrderPush(Obey_train); // to dalej wg rozk³adu
            }
        }
        if (DebugModeFlag) // normalnie nie ma po co tego wypisywaæ
            WriteLog("*/");
        OrderPush(Shunt); // po wykonaniu rozk³adu prze³¹czy siê na manewry
    }
    // McZapkie-100302 - to ma byc wyzwalane ze scenerii
    if (fVel == 0.0)
        SetVelocity(0, 0, stopSleep); // jeœli nie ma prêdkoœci pocz¹tkowej, to œpi
    else
    { // jeœli podana niezerowa prêdkoœæ
        if ((fVel >= 1.0) ||
            (fVel < 0.02)) // jeœli ma jechaæ - dla 0.01 ma podjechaæ manewrowo po podaniu sygna³u
            iDrivigFlags = (iDrivigFlags & ~moveStopHere) |
                           moveStopCloser; // to do nastêpnego W4 ma podjechaæ blisko
        else
            iDrivigFlags |= moveStopHere; // czekaæ na sygna³
        JumpToFirstOrder();
        if (fVel >= 1.0) // jeœli ma jechaæ
            SetVelocity(fVel, -1); // ma ustawiæ ¿¹dan¹ prêdkoœæ
        else
            SetVelocity(0, 0, stopSleep); // prêdkoœæ w przedziale (0;1) oznacza, ¿e ma staæ
    }
#if LOGORDERS
    WriteLog("--> OrdersInit");
#endif
    if (DebugModeFlag) // normalnie nie ma po co tego wypisywaæ
        OrdersDump(); // wypisanie kontrolne tabelki rozkazów
    // McZapkie! - zeby w ogole AI ruszyl to musi wykonac powyzsze rozkazy
    // Ale mozna by je zapodac ze scenerii
};

AnsiString TController::StopReasonText()
{ // informacja tekstowa o przyczynie zatrzymania
    if (eStopReason != 7) // zawalidroga bêdzie inaczej
        return StopReasonTable[eStopReason];
    else
        return "Blocked by " + (pVehicles[0]->PrevAny()->GetName());
};

//----------------------------------------------------------------------------------------------------------------------
// McZapkie: skanowanie semaforów
// Ra: stare funkcje skanuj¹ce, u¿ywane podczas manewrów do szukania sygnalizatora z ty³u
//- nie reaguj¹ na PutValues, bo nie ma takiej potrzeby
//- rozpoznaj¹ tylko zerow¹ prêdkoœæ (jako koniec toru i brak podstaw do dalszego skanowania)
//----------------------------------------------------------------------------------------------------------------------

/* //nie u¿ywane
double TController::Distance(vector3 &p1,vector3 &n,vector3 &p2)
{//Ra:obliczenie odleg³oœci punktu (p1) od p³aszczyzny o wektorze normalnym (n) przechodz¹cej przez
(p2)
 return n.x*(p1.x-p2.x)+n.y*(p1.y-p2.y)+n.z*(p1.z-p2.z); //ax1+by1+cz1+d, gdzie d=-(ax2+by2+cz2)
};
*/

bool TController::BackwardTrackBusy(TTrack *Track)
{ // najpierw sprawdzamy, czy na danym torze s¹ pojazdy z innego sk³adu
    if (Track->iNumDynamics)
    { // jeœli tylko z w³asnego sk³adu, to tor jest wolny
        for (int i = 0; i < Track->iNumDynamics; ++i)
            if (Track->Dynamics[i]->ctOwner != this) // jeœli jest jakiœ cudzy
                return true; // to tor jest zajêty i skanowanie nie obowi¹zuje
    }
    return false; // wolny
};

TEvent *__fastcall TController::CheckTrackEventBackward(double fDirection, TTrack *Track)
{ // sprawdzanie eventu w torze, czy jest sygna³owym - skanowanie do ty³u
    TEvent *e = (fDirection > 0) ? Track->evEvent2 : Track->evEvent1;
    if (e)
        if (!e->bEnabled) // jeœli sygna³owy (nie dodawany do kolejki)
            if (e->Type == tp_GetValues) // PutValues nie mo¿e siê zmieniæ
                return e;
    return NULL;
};

TTrack *__fastcall TController::BackwardTraceRoute(double &fDistance, double &fDirection,
                                                   TTrack *Track, TEvent *&Event)
{ // szukanie sygnalizatora w kierunku przeciwnym jazdy (eventu odczytu komórki pamiêci)
    TTrack *pTrackChVel = Track; // tor ze zmian¹ prêdkoœci
    TTrack *pTrackFrom; // odcinek poprzedni, do znajdywania koñca dróg
    double fDistChVel = -1; // odleg³oœæ do toru ze zmian¹ prêdkoœci
    double fCurrentDistance = pVehicle->RaTranslationGet(); // aktualna pozycja na torze
    double s = 0;
    if (fDirection > 0) // jeœli w kierunku Point2 toru
        fCurrentDistance = Track->Length() - fCurrentDistance;
    if (BackwardTrackBusy(Track))
    { // jak tor zajêty innym sk³adem, to nie ma po co skanowaæ
        fDistance = 0; // to na tym torze stoimy
        return NULL; // stop, skanowanie nie da³o sensownych rezultatów
    }
    if ((Event = CheckTrackEventBackward(fDirection, Track)) != NULL)
    { // jeœli jest semafor na tym torze
        fDistance = 0; // to na tym torze stoimy
        return Track;
    }
    if ((Track->VelocityGet() == 0.0) || (Track->iDamageFlag & 128))
    { // jak prêdkosæ 0 albo uszkadza, to nie ma po co skanowaæ
        fDistance = 0; // to na tym torze stoimy
        return NULL; // stop, skanowanie nie da³o sensownych rezultatów
    }
    while (s < fDistance)
    {
        // Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
        pTrackFrom = Track; // zapamiêtanie aktualnego odcinka
        s += fCurrentDistance; // doliczenie kolejnego odcinka do przeskanowanej d³ugoœci
        if (fDirection > 0)
        { // jeœli szukanie od Point1 w kierunku Point2
            if (Track->iNextDirection)
                fDirection = -fDirection;
            Track = Track->CurrentNext(); // mo¿e byæ NULL
        }
        else // if (fDirection<0)
        { // jeœli szukanie od Point2 w kierunku Point1
            if (!Track->iPrevDirection)
                fDirection = -fDirection;
            Track = Track->CurrentPrev(); // mo¿e byæ NULL
        }
        if (Track == pTrackFrom)
            Track = NULL; // koniec, tak jak dla torów
        if (Track ?
                (Track->VelocityGet() == 0.0) || (Track->iDamageFlag & 128) ||
                    BackwardTrackBusy(Track) :
                true)
        { // gdy dalej toru nie ma albo zerowa prêdkoœæ, albo uszkadza pojazd
            fDistance = s;
            return NULL; // zwraca NULL, ¿e skanowanie nie da³o sensownych rezultatów
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
    fDistance = fDistChVel; // odleg³oœæ do zmiany prêdkoœci
    return pTrackChVel; // i tor na którym siê zmienia
}

// sprawdzanie zdarzeñ semaforów i ograniczeñ szlakowych
void TController::SetProximityVelocity(double dist, double vel, const vector3 *pos)
{ // Ra:przeslanie do AI prêdkoœci
    /*
     //!!!! zast¹piæ prawid³ow¹ reakcj¹ AI na SetProximityVelocity !!!!
     if (vel==0)
     {//jeœli zatrzymanie, to zmniejszamy dystans o 10m
      dist-=10.0;
     };
     if (dist<0.0) dist=0.0;
     if ((vel<0)?true:dist>0.1*(MoverParameters->Vel*MoverParameters->Vel-vel*vel)+50)
     {//jeœli jest dalej od umownej drogi hamowania
    */
    PutCommand("SetProximityVelocity", dist, vel, pos);
    /*
     }
     else
     {//jeœli jest zagro¿enie, ¿e przekroczy
      Mechanik->SetVelocity(floor(0.2*sqrt(dist)+vel),vel,stopError);
     }
     */
}

TCommandType TController::BackwardScan()
{ // sprawdzanie zdarzeñ semaforów z ty³u pojazdu, zwraca komendê
    // dziêki temu bêdzie mo¿na stawaæ za wskazanym sygnalizatorem, a zw³aszcza jeœli bêdzie jazda
    // na kozio³
    // ograniczenia prêdkoœci nie s¹ wtedy istotne, równie¿ koniec toru jest do niczego nie
    // przydatny
    // zwraca true, jeœli nale¿y odwróciæ kierunek jazdy pojazdu
    if ((OrderList[OrderPos] & ~(Shunt | Connect)))
        return cm_Unknown; // skanowanie sygna³ów tylko gdy jedzie w trybie manewrowym albo czeka na
    // rozkazy
    vector3 sl;
    int startdir =
        -pVehicles[0]->DirectionGet(); // kierunek jazdy wzglêdem sprzêgów pojazdu na czele
    if (startdir == 0) // jeœli kabina i kierunek nie jest okreœlony
        return cm_Unknown; // nie robimy nic
    double scandir =
        startdir * pVehicles[0]->RaDirectionGet(); // szukamy od pierwszej osi w wybranym kierunku
    if (scandir !=
        0.0) // skanowanie toru w poszukiwaniu eventów GetValues (PutValues nie s¹ przydatne)
    { // Ra: przy wstecznym skanowaniu prêdkoœæ nie ma znaczenia
        // scanback=pVehicles[1]->NextDistance(fLength+1000.0); //odleg³oœæ do nastêpnego pojazdu,
        // 1000 gdy nic nie ma
        double scanmax = 1000; // 1000m do ty³u, ¿eby widzia³ przeciwny koniec stacji
        double scandist = scanmax; // zmodyfikuje na rzeczywiœcie przeskanowane
        TEvent *e = NULL; // event potencjalnie od semafora
        // opcjonalnie mo¿e byæ skanowanie od "wskaŸnika" z przodu, np. W5, Tm=Ms1, koniec toru
        TTrack *scantrack = BackwardTraceRoute(scandist, scandir, pVehicles[0]->RaTrackGet(),
                                               e); // wg drugiej osi w kierunku ruchu
        vector3 dir = startdir * pVehicles[0]->VectorFront(); // wektor w kierunku jazdy/szukania
        if (!scantrack) // jeœli wstecz wykryto koniec toru
            return cm_Unknown; // to raczej nic siê nie da w takiej sytuacji zrobiæ
        else
        { // a jeœli s¹ dalej tory
            double vmechmax; // prêdkoœæ ustawiona semaforem
            if (e)
            { // jeœli jest jakiœ sygna³ na widoku
#if LOGBACKSCAN
                AnsiString edir =
                    pVehicle->asName + " - " + AnsiString((scandir > 0) ? "Event2 " : "Event1 ");
#endif
                // najpierw sprawdzamy, czy semafor czy inny znak zosta³ przejechany
                vector3 pos = pVehicles[1]->RearPosition(); // pozycja ty³u
                vector3 sem; // wektor do sygna³u
                if (e->Type == tp_GetValues)
                { // przes³aæ info o zbli¿aj¹cym siê semaforze
#if LOGBACKSCAN
                    edir += "(" + (e->Params[8].asGroundNode->asName) + "): ";
#endif
                    sl = e->PositionGet(); // po³o¿enie komórki pamiêci
                    sem = sl - pos; // wektor do komórki pamiêci od koñca sk³adu
                    // sem=e->Params[8].asGroundNode->pCenter-pos; //wektor do komórki pamiêci
                    if (dir.x * sem.x + dir.z * sem.z < 0) // jeœli zosta³ miniêty
                    // if ((mvOccupied->CategoryFlag&1)?(VelNext!=0.0):true) //dla poci¹gu wymagany
                    // sygna³ zezwalaj¹cy
                    { // iloczyn skalarny jest ujemny, gdy sygna³ stoi z ty³u
#if LOGBACKSCAN
                        WriteLog(edir + "- ignored as not passed yet");
#endif
                        return cm_Unknown; // nic
                    }
                    vmechmax = e->ValueGet(1); // prêdkoœæ przy tym semaforze
                    // przeliczamy odleg³oœæ od semafora - potrzebne by by³y wspó³rzêdne pocz¹tku
                    // sk³adu
                    // scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-0.5*mvOccupied->Dim.L-10;
                    // //10m luzu
                    scandist = sem.Length() - 2; // 2m luzu przy manewrach wystarczy
                    if (scandist < 0)
                        scandist = 0; // ujemnych nie ma po co wysy³aæ
                    bool move = false; // czy AI w trybie manewerowym ma doci¹gn¹æ pod S1
                    if (e->Command() == cm_SetVelocity)
                        if ((vmechmax == 0.0) ? (OrderCurrentGet() & (Shunt | Connect)) :
                                                (OrderCurrentGet() &
                                                 Connect)) // przy podczepianiu ignorowaæ wyjazd?
                            move = true; // AI w trybie manewerowym ma doci¹gn¹æ pod S1
                        else
                        { //
                            if ((scandist > fMinProximityDist) ?
                                    (mvOccupied->Vel > 0.0) && (OrderCurrentGet() != Shunt) :
                                    false)
                            { // jeœli semafor jest daleko, a pojazd jedzie, to informujemy o
// zmianie prêdkoœci
// jeœli jedzie manewrowo, musi dostaæ SetVelocity, ¿eby sie na poci¹gowy prze³¹czy³
// Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGBACKSCAN
                                // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+"
                                // "+AnsiString(vmechmax));
                                WriteLog(edir);
#endif
                                // SetProximityVelocity(scandist,vmechmax,&sl);
                                return (vmechmax > 0) ? cm_SetVelocity : cm_Unknown;
                            }
                            else // ustawiamy prêdkoœæ tylko wtedy, gdy ma ruszyæ, stan¹æ albo ma
                            // staæ
                            // if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeœli stoi lub ma
                            // stan¹æ/staæ
                            { // semafor na tym torze albo lokomtywa stoi, a ma ruszyæ, albo ma
// stan¹æ, albo nie ruszaæ
// stop trzeba powtarzaæ, bo inaczej zatr¹bi i pojedzie sam
// PutCommand("SetVelocity",vmechmax,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                                WriteLog(edir + "SetVelocity " + AnsiString(vmechmax) + " " +
                                         AnsiString(e->Params[9].asMemCell->Value2()));
#endif
                                return (vmechmax > 0) ? cm_SetVelocity : cm_Unknown;
                            }
                        }
                    if (OrderCurrentGet() ? OrderCurrentGet() & (Shunt | Connect) :
                                            true) // w Wait_for_orders te¿ widzi tarcze
                    { // reakcja AI w trybie manewrowym dodatkowo na sygna³y manewrowe
                        if (move ? true : e->Command() == cm_ShuntVelocity)
                        { // jeœli powy¿ej by³o SetVelocity 0 0, to doci¹gamy pod S1
                            if ((scandist > fMinProximityDist) ?
                                    (mvOccupied->Vel > 0.0) || (vmechmax == 0.0) :
                                    false)
                            { // jeœli tarcza jest daleko, to:
                                //- jesli pojazd jedzie, to informujemy o zmianie prêdkoœci
                                //- jeœli stoi, to z w³asnej inicjatywy mo¿e podjechaæ pod zamkniêt¹
                                // tarczê
                                if (mvOccupied->Vel > 0.0) // tylko jeœli jedzie
                                { // Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGBACKSCAN
                                    // WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+"
                                    // "+AnsiString(vmechmax));
                                    WriteLog(edir);
#endif
                                    // SetProximityVelocity(scandist,vmechmax,&sl);
                                    return (iDrivigFlags & moveTrackEnd) ?
                                               cm_ChangeDirection :
                                               cm_Unknown; // jeœli jedzie na W5 albo koniec toru,
                                    // to mo¿na zmieniæ kierunek
                                }
                            }
                            else // ustawiamy prêdkoœæ tylko wtedy, gdy ma ruszyæ, albo stan¹æ albo
                            // ma staæ pod tarcz¹
                            { // stop trzeba powtarzaæ, bo inaczej zatr¹bi i pojedzie sam
                                // if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeœli jedzie
                                // lub ma stan¹æ/staæ
                                { // nie dostanie komendy jeœli jedzie i ma jechaæ
// PutCommand("ShuntVelocity",vmechmax,e->Params[9].asMemCell->Value2(),&sl,stopSem);
#if LOGBACKSCAN
                                    WriteLog(edir + "ShuntVelocity " + AnsiString(vmechmax) + " " +
                                             AnsiString(e->ValueGet(2)));
#endif
                                    return (vmechmax > 0) ? cm_ShuntVelocity : cm_Unknown;
                                }
                            }
                            if ((vmechmax != 0.0) && (scandist < 100.0))
                            { // jeœli Tm w odleg³oœci do 100m podaje zezwolenie na jazdê, to od
// razu j¹ ignorujemy, aby móc szukaæ kolejnej
// eSignSkip=e; //wtedy uznajemy ignorowan¹ przy poszukiwaniu nowej
#if LOGBACKSCAN
                                WriteLog(edir + "- will be ignored due to Ms2");
#endif
                                return (vmechmax > 0) ? cm_ShuntVelocity : cm_Unknown;
                            }
                        } // if (move?...
                    } // if (OrderCurrentGet()==Shunt)
                    if (!e->bEnabled) // jeœli skanowany
                        if (e->StopCommand()) // a pod³¹czona komórka ma komendê
                            return cm_Command; // to te¿ siê obróciæ
                } // if (e->Type==tp_GetValues)
            } // if (e)
        } // if (scantrack)
    } // if (scandir!=0.0)
    return cm_Unknown; // nic
};

AnsiString TController::NextStop()
{ // informacja o nastêpnym zatrzymaniu, wyœwietlane pod [F1]
    if (asNextStop.Length() < 20)
        return ""; // nie zawiera nazwy stacji, gdy dojecha³ do koñca
    // dodaæ godzinê odjazdu
    if (!TrainParams)
        return ""; // tu nie powinno nigdy wejœæ
    TMTableLine *t = TrainParams->TimeTable + TrainParams->StationIndex;
    if (t->Dh >= 0) // jeœli jest godzina odjazdu
        return asNextStop.SubString(20, 30) + AnsiString(" ") + AnsiString(int(t->Dh)) +
               AnsiString(":") + AnsiString(int(100 + t->Dm)).SubString(2, 2); // odjazd
    else if (t->Ah >= 0) // przyjazd
        return asNextStop.SubString(20, 30) + AnsiString(" (") + AnsiString(int(t->Ah)) +
               AnsiString(":") + AnsiString(int(100 + t->Am)).SubString(2, 2) +
               AnsiString(")"); // przyjazd
    return "";
};

//-----------koniec skanowania semaforow

void TController::TakeControl(bool yes)
{ // przejêcie kontroli przez AI albo oddanie
    if (AIControllFlag == yes)
        return; // ju¿ jest jak ma byæ
    if (yes) //¿eby nie wykonywaæ dwa razy
    { // teraz AI prowadzi
        AIControllFlag = AIdriver;
        pVehicle->Controller = AIdriver;
        iDirection = 0; // kierunek jazdy trzeba dopiero zgadn¹æ
        // gdy zgaszone œwiat³a, flaga podje¿d¿ania pod semafory pozostaje bez zmiany
        if (OrderCurrentGet()) // jeœli coœ robi
            PrepareEngine(); // niech sprawdzi stan silnika
        else // jeœli nic nie robi
            if (pVehicle->iLights[mvOccupied->CabNo < 0 ? 1 : 0] &
                21) // któreœ ze œwiate³ zapalone?
        { // od wersji 357 oczekujemy podania komend dla AI przez sceneriê
            OrderNext(Prepare_engine);
            if (pVehicle->iLights[mvOccupied->CabNo < 0 ? 1 : 0] & 4) // górne œwiat³o zapalone
                OrderNext(Obey_train); // jazda poci¹gowa
            else
                OrderNext(Shunt); // jazda manewrowa
            if (mvOccupied->Vel >= 1.0) // jeœli jedzie (dla 0.1 ma staæ)
                iDrivigFlags &= ~moveStopHere; // to ma nie czekaæ na sygna³, tylko jechaæ
            else
                iDrivigFlags |= moveStopHere; // a jak stoi, to niech czeka
        }
        /* od wersji 357 oczekujemy podania komend dla AI przez sceneriê
          if (OrderCurrentGet())
          {if (OrderCurrentGet()<Shunt)
           {OrderNext(Prepare_engine);
            if (pVehicle->iLights[mvOccupied->CabNo<0?1:0]&4) //górne œwiat³o
             OrderNext(Obey_train); //jazda poci¹gowa
            else
             OrderNext(Shunt); //jazda manewrowa
           }
          }
          else //jeœli jest w stanie Wait_for_orders
           JumpToFirstOrder(); //uruchomienie?
          // czy dac ponizsze? to problematyczne
          //SetVelocity(pVehicle->GetVelocity(),-1); //utrzymanie dotychczasowej?
          if (pVehicle->GetVelocity()>0.0)
           SetVelocity(-1,-1); //AI ustali sobie odpowiedni¹ prêdkoœæ
        */
        // Activation(); //przeniesie u¿ytkownika w ostatnio wybranym kierunku
        CheckVehicles(); // ustawienie œwiate³
        TableClear(); // ponowne utworzenie tabelki, bo cz³owiek móg³ pojechaæ niezgodnie z
        // sygna³ami
    }
    else
    { // a teraz u¿ytkownik
        AIControllFlag = Humandriver;
        pVehicle->Controller = Humandriver;
    }
};

void TController::DirectionForward(bool forward)
{ // ustawienie jazdy do przodu dla true i do ty³u dla false (zale¿y od kabiny)
    while (mvControlling->MainCtrlPos) // samo zapêtlenie DecSpeed() nie wystarcza
        DecSpeed(true); // wymuszenie zerowania nastawnika jazdy, inaczej siê mo¿e zawiesiæ
    if (forward)
        while (mvOccupied->ActiveDir <= 0)
            mvOccupied->DirectionForward(); // do przodu w obecnej kabinie
    else
        while (mvOccupied->ActiveDir >= 0)
            mvOccupied->DirectionBackward(); // do ty³u w obecnej kabinie
    if (mvOccupied->EngineType == DieselEngine) // specjalnie dla SN61
        if (iDrivigFlags & moveActive) // jeœli by³ ju¿ odpalony
            if (mvControlling->RList[mvControlling->MainCtrlPos].Mn == 0)
                mvControlling->IncMainCtrl(1); //¿eby nie zgas³
};

AnsiString TController::Relation()
{ // zwraca relacjê poci¹gu
    return TrainParams->ShowRelation();
};

AnsiString TController::TrainName()
{ // zwraca numer poci¹gu
    return TrainParams->TrainName;
};

int TController::StationCount()
{ // zwraca iloœæ stacji (miejsc zatrzymania)
    return TrainParams->StationCount;
};

int TController::StationIndex()
{ // zwraca indeks aktualnej stacji (miejsca zatrzymania)
    return TrainParams->StationIndex;
};

bool TController::IsStop()
{ // informuje, czy jest zatrzymanie na najbli¿szej stacji
    return TrainParams->IsStop();
};

void TController::MoveTo(TDynamicObject *to)
{ // przesuniêcie AI do innego pojazdu (przy zmianie kabiny)
    // mvOccupied->CabDeactivisation(); //wy³¹czenie kabiny w opuszczanym
    pVehicle->Mechanik = to->Mechanik; //¿eby siê zamieni³y, jak jest jakieœ drugie
    pVehicle = to;
    ControllingSet(); // utworzenie po³¹czenia do sterowanego pojazdu
    pVehicle->Mechanik = this;
    // iDirection=0; //kierunek jazdy trzeba dopiero zgadn¹æ
};

void TController::ControllingSet()
{ // znajduje cz³on do sterowania w EZT bêdzie to silnikowy
    // problematyczne jest sterowanie z cz³onu biernego, dlatego damy AI silnikowy
    // dziêki temu bêdzie wirtualna kabina w silnikowym, dzia³aj¹ca w rozrz¹dczym
    // w plikach FIZ zosta³y zgubione ujemne maski sprzêgów, st¹d problemy z EZT
    mvOccupied = pVehicle->MoverParameters; // domyœlny skrót do obiektu parametrów
    mvControlling = pVehicle->ControlledFind()->MoverParameters; // poszukiwanie cz³onu sterowanego
};

AnsiString TController::TableText(int i)
{ // pozycja tabelki prêdkoœci
    i = (iFirst + i) % iSpeedTableSize; // numer pozycji
    if (i != iLast) // w (iLast) znajduje siê kolejny tor do przeskanowania, ale nie jest ona
        // aktywn¹
        return sSpeedTable[i].TableText();
    return ""; // wskaŸnik koñca
};

int TController::CrossRoute(TTrack *tr)
{ // zwraca numer segmentu dla skrzy¿owania (tr)
    // po¿¹dany numer segmentu jest okreœlany podczas skanowania drogi
    // droga powinna byæ okreœlona sposobem przejazdu przez skrzy¿owania albo wspó³rzêdnymi miejsca
    // docelowego
    for (int i = iFirst; i != iLast; i = (i + 1) % iSpeedTableSize)
    { // trzeba przejrzeæ tabelê skanowania w poszukiwaniu (tr)
        // i jak siê znajdzie, to zwróciæ zapamiêtany numer segmentu i kierunek przejazdu
        // (-6..-1,1..6)
        if ((sSpeedTable[i].iFlags & 3) == 3) // jeœli pozycja istotna (1) oraz odcinek (2)
            if (sSpeedTable[i].trTrack == tr) // jeœli pozycja odpowiadaj¹ca skrzy¿owaniu (tr)
                return (sSpeedTable[i].iFlags >> 28); // najstarsze 4 bity jako liczba -8..7
    }
    return 0; // nic nie znaleziono?
};

void TController::RouteSwitch(int d)
{ // ustawienie kierunku jazdy z kabiny
    d &= 3;
    if (d)
        if (iRouteWanted != d)
        { // nowy kierunek
            iRouteWanted = d; // zapamiêtanie
            if (mvOccupied->CategoryFlag & 2) // jeœli samochód
                for (int i = iFirst; i != iLast; i = (i + 1) % iSpeedTableSize)
                { // szukanie pierwszego skrzy¿owania i resetowanie kierunku na nim
                    if ((sSpeedTable[i].iFlags & 3) ==
                        3) // jeœli pozycja istotna (1) oraz odcinek (2)
                        if ((sSpeedTable[i].iFlags & 32) == 0) // odcinek nie mo¿e byæ miniêtym
                            if (sSpeedTable[i].trTrack->eType == tt_Cross) // jeœli skrzy¿owanie
                            { // obciêcie tabelki skanowania przed skrzy¿owaniem, aby ponownie
                                // wybraæ drogê
                                iLast = i - 1; // ponowne skanowanie skrzy¿owania (w zwrotnicach
                                // jest iLast=i, ale tam jest proœciej)
                                if (iLast < 0)
                                    iLast += iSpeedTableSize; // bo tabelka jest zapêtlona
                                return;
                            }
                }
        }
};
AnsiString TController::OwnerName()
{
    return pVehicle ? pVehicle->MoverParameters->Name : AnsiString("none");
};
