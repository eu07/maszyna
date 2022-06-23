/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "messaging.h"

#include "Globals.h"
#include "application.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Event.h"
#include "DynObj.h"
#include "Driver.h"
#include "mtable.h"
#include "Logs.h"

#ifdef _WIN32
extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window );
}
#endif

namespace multiplayer {

std::uint32_t const EU07_MESSAGEHEADER { MAKE_ID4( 'E','U','0','7' ) };

void
Navigate(std::string const &ClassName, UINT Msg, WPARAM wParam, LPARAM lParam) {
#ifdef _WIN32
    // wysłanie komunikatu do sterującego
    HWND h = FindWindow(ClassName.c_str(), 0); // można by to zapamiętać
    if (h == 0)
        h = FindWindow(0, ClassName.c_str()); // można by to zapamiętać
    SendMessage(h, Msg, wParam, lParam);
#endif
}

void
OnCommandGet(multiplayer::DaneRozkaz *pRozkaz)
{ // odebranie komunikatu z serwera
    if (pRozkaz->iSygn == EU07_MESSAGEHEADER )
        switch (pRozkaz->iComm)
        {
        case 0: // odesłanie identyfikatora wersji
			CommLog( Now() + " " + std::to_string(pRozkaz->iComm) + " version" + " rcvd");
            WyslijString(Global.asVersion, 0); // przedsatwienie się
            break;
        case 1: // odesłanie identyfikatora wersji
			CommLog( Now() + " " + std::to_string(pRozkaz->iComm) + " scenery" + " rcvd");
            WyslijString(Global.SceneryFile, 1); // nazwa scenerii
            break;
        case 2: {
            // event
            CommLog( Now() + " " + std::to_string( pRozkaz->iComm ) + " " +
                std::string( pRozkaz->cString + 1, (unsigned)( pRozkaz->cString[ 0 ] ) ) + " rcvd" );

            if( Global.iMultiplayer ) {
                auto *event = simulation::Events.FindEvent( std::string( pRozkaz->cString + 1, (unsigned)( pRozkaz->cString[ 0 ] ) ) );
                if( event != nullptr ) {
                    if( ( typeid( *event ) == typeid( multi_event ) )
                     || ( typeid( *event ) == typeid( lights_event ) )
                     || ( event->m_sibling != 0 ) ) {
                        // tylko jawne albo niejawne Multiple
						command_relay relay;
						relay.post(user_command::queueevent, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &event->name());
                    }
                }
            }
            break;
        }
        case 3: // rozkaz dla AI
            if (Global.iMultiplayer)
            {
                int i = int(pRozkaz->cString[8]); // długość pierwszego łańcucha (z przodu dwa floaty)
                CommLog(
                    Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 11 + i, (unsigned)(pRozkaz->cString[10 + i])) +
                    " rcvd");
                // nazwa pojazdu jest druga
                auto *vehicle = simulation::Vehicles.find( { pRozkaz->cString + 11 + i, (unsigned)pRozkaz->cString[ 10 + i ] } );
                if( ( vehicle != nullptr )
                 && ( vehicle->Mechanik != nullptr ) ) {
                    vehicle->Mechanik->PutCommand(
                        { pRozkaz->cString + 9, static_cast<std::size_t>(i) },
                        pRozkaz->fPar[0], pRozkaz->fPar[1],
                        nullptr,
                        stopExt ); // floaty są z przodu
                    WriteLog("AI command: " + std::string(pRozkaz->cString + 9, i));
                }
            }
            break;
        case 4: // badanie zajętości toru
        {
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");

            auto *track = simulation::Paths.find( std::string( pRozkaz->cString + 1, (unsigned)( pRozkaz->cString[ 0 ] ) ) );
            if( ( track != nullptr )
             && ( track->IsEmpty() ) ) {
                WyslijWolny( track->name() );
            }
        }
        break;
        case 5: // ustawienie parametrów
        {
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " params " + to_string(*pRozkaz->iPar) + " rcvd");
            if (*pRozkaz->iPar == 0) // sprawdzenie czasu
                if (*pRozkaz->iPar & 1) // ustawienie czasu
                {
                    auto t = pRozkaz->fPar[1];
                    simulation::Time.data().wDay = std::floor(t); // niby nie powinno być dnia, ale...
                    if (Global.fMoveLight >= 0)
                        Global.fMoveLight = t; // trzeba by deklinację Słońca przeliczyć
                    simulation::Time.data().wHour = std::floor(24 * t) - 24.0 * simulation::Time.data().wDay;
                    simulation::Time.data().wMinute = std::floor(60 * 24 * t) - 60.0 * (24.0 * simulation::Time.data().wDay + simulation::Time.data().wHour);
                    simulation::Time.data().wSecond = std::floor( 60 * 60 * 24 * t ) - 60.0 * ( 60.0 * ( 24.0 * simulation::Time.data().wDay + simulation::Time.data().wHour ) + simulation::Time.data().wMinute );
                }
            if (*pRozkaz->iPar & 2)
            { // ustawienie flag zapauzowania
                Global.iPause = pRozkaz->fPar[2]; // zakładamy, że wysyłający wie, co robi
            }
        }
        break;
        case 6: // pobranie parametrów ruchu pojazdu
            if (Global.iMultiplayer) {
                // Ra 2014-12: to ma działać również dla pojazdów bez obsady
                CommLog(
                    Now() + " "
                  + to_string( pRozkaz->iComm ) + " "
                  + std::string{ pRozkaz->cString + 1, (unsigned)( pRozkaz->cString[ 0 ] ) }
                  + " rcvd" );
                if (pRozkaz->cString[0]) {
                    // jeśli długość nazwy jest niezerowa szukamy pierwszego pojazdu o takiej nazwie i odsyłamy parametry ramką #7
                    auto *vehicle = (
                        pRozkaz->cString[ 1 ] == '*' ?
					        simulation::Train->Dynamic() :
                            simulation::Vehicles.find( std::string{ pRozkaz->cString + 1, (unsigned)pRozkaz->cString[ 0 ] } ) );
                    if( vehicle != nullptr ) {
                        WyslijNamiary( vehicle ); // wysłanie informacji o pojeździe
                    }
                }
                else {
                    // dla pustego wysyłamy ramki 6 z nazwami pojazdów AI (jeśli potrzebne wszystkie, to rozpoznać np. "*")
                    simulation::Vehicles.DynamicList();
                }
            }
            break;
        case 8: // ponowne wysłanie informacji o zajętych odcinkach toru
			CommLog(Now() + " " + to_string(pRozkaz->iComm) + " all busy track" + " rcvd");
            simulation::Paths.TrackBusyList();
            break;
        case 9: // ponowne wysłanie informacji o zajętych odcinkach izolowanych
			CommLog(Now() + " " + to_string(pRozkaz->iComm) + " all busy isolated" + " rcvd");
            simulation::Paths.IsolatedBusyList();
            break;
        case 10: // badanie zajętości jednego odcinka izolowanego
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");
            simulation::Paths.IsolatedBusy( std::string( pRozkaz->cString + 1, (unsigned)( pRozkaz->cString[ 0 ] ) ) );
            break;
        case 11: // ustawienie parametrów ruchu pojazdu
            //    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
            break;
		case 12: // skrocona ramka parametrow pojazdow AI (wszystkich!!)
			CommLog(Now() + " " + to_string(pRozkaz->iComm) + " obsadzone" + " rcvd");
            WyslijObsadzone();
			//    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
			break;
		case 13: // ramka uszkodzenia i innych stanow pojazdu, np. wylaczenie CA, wlaczenie recznego itd.
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) +
                    " rcvd");
            if( pRozkaz->cString[ 1 ] ) // jeśli długość nazwy jest niezerowa
            { // szukamy pierwszego pojazdu o takiej nazwie i odsyłamy parametry ramką #13
                auto *lookup = (
                    pRozkaz->cString[ 2 ] == '*' ?
				        simulation::Train->Dynamic() : // nazwa pojazdu użytkownika
                        simulation::Vehicles.find( std::string( pRozkaz->cString + 2, (unsigned)pRozkaz->cString[ 1 ] ) ) ); // nazwa pojazdu
                if( lookup == nullptr ) { break; } // nothing found, nothing to do
                auto *d { lookup };
                while( d != nullptr ) {
                    d->Damage( pRozkaz->cString[ 0 ] );
                    d = d->Next(); // pozostałe też
                }
                d = lookup->Prev();
                while( d != nullptr ) {
                    d->Damage( pRozkaz->cString[ 0 ] );
                    d = d->Prev(); // w drugą stronę też
                }
                WyslijUszkodzenia( lookup->asName, lookup->MoverParameters->EngDmgFlag ); // zwrot informacji o pojeździe
            }
			break;
        default:
            break;
		}
}

void
WyslijEvent(const std::string &e, const std::string &d)
{ // Ra: jeszcze do wyczyszczenia
#ifdef _WIN32
    DaneRozkaz r;
    r.iSygn = EU07_MESSAGEHEADER;
    r.iComm = 2; // 2 - event
    size_t i = e.length(), j = d.length();
    r.cString[0] = char(i);
    strcpy(r.cString + 1, e.c_str()); // zakończony zerem
    r.cString[i + 2] = char(j); // licznik po zerze kończącym
    strcpy(r.cString + 3 + i, d.c_str()); // zakończony zerem
    COPYDATASTRUCT cData;
    cData.dwData = EU07_MESSAGEHEADER; // sygnatura
    cData.cbData = (DWORD)(12 + i + j); // 8+dwa liczniki i dwa zera kończące
    cData.lpData = &r;
    Navigate( "TEU07SRK", WM_COPYDATA, (WPARAM)glfwGetWin32Window( Application.window() ), (LPARAM)&cData );
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + e + " sent" );
#endif
}

void
WyslijUszkodzenia(const std::string &t, char fl)
{ // wysłanie informacji w postaci pojedynczego tekstu
#ifdef _WIN32
    DaneRozkaz r;
    r.iSygn = EU07_MESSAGEHEADER;
	r.iComm = 13; // numer komunikatu
	size_t i = t.length();
	r.cString[0] = char(fl);
	r.cString[1] = char(i);
	strcpy(r.cString + 2, t.c_str()); // z zerem kończącym
	COPYDATASTRUCT cData;
    cData.dwData = EU07_MESSAGEHEADER; // sygnatura
	cData.cbData = (DWORD)(11 + i); // 8+licznik i zero kończące
	cData.lpData = &r;
    Navigate( "TEU07SRK", WM_COPYDATA, (WPARAM)glfwGetWin32Window( Application.window() ), (LPARAM)&cData );
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + t + " sent");
#endif
}

void
WyslijString(const std::string &t, int n)
{ // wysłanie informacji w postaci pojedynczego tekstu
#ifdef _WIN32
    DaneRozkaz r;
    r.iSygn = EU07_MESSAGEHEADER;
    r.iComm = n; // numer komunikatu
    size_t i = t.length();
    r.cString[0] = char(i);
    strcpy(r.cString + 1, t.c_str()); // z zerem kończącym
    COPYDATASTRUCT cData;
    cData.dwData = EU07_MESSAGEHEADER; // sygnatura
    cData.cbData = (DWORD)(10 + i); // 8+licznik i zero kończące
    cData.lpData = &r;
    Navigate( "TEU07SRK", WM_COPYDATA, (WPARAM)glfwGetWin32Window( Application.window() ), (LPARAM)&cData );
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + t + " sent");
#endif
}

void
WyslijWolny(const std::string &t)
{ // Ra: jeszcze do wyczyszczenia
    WyslijString(t, 4); // tor wolny
}

void
WyslijNamiary(TDynamicObject const *Vehicle)
{ // wysłanie informacji o pojeździe - (float), długość ramki będzie zwiększana w miarę potrzeby
#ifdef _WIN32
    DaneRozkaz r;
    r.iSygn = EU07_MESSAGEHEADER;
    r.iComm = 7; // 7 - dane pojazdu
	int i = 32;
	size_t j = Vehicle->asName.length();
    r.iPar[0] = i; // ilość danych liczbowych
    r.fPar[1] = Global.fTimeAngleDeg / 360.0; // aktualny czas (1.0=doba)
    r.fPar[2] = Vehicle->MoverParameters->Loc.X; // pozycja X
    r.fPar[3] = Vehicle->MoverParameters->Loc.Y; // pozycja Y
    r.fPar[4] = Vehicle->MoverParameters->Loc.Z; // pozycja Z
    r.fPar[5] = Vehicle->MoverParameters->V; // prędkość ruchu X
    r.fPar[6] = Vehicle->MoverParameters->nrot * M_PI *
                Vehicle->MoverParameters->WheelDiameter; // prędkość obrotowa kóŁ
    r.fPar[7] = 0; // prędkość ruchu Z
    r.fPar[8] = Vehicle->MoverParameters->AccS; // przyspieszenie X
    r.fPar[9] = Vehicle->MoverParameters->AccN; // przyspieszenie Y //na razie nie
    r.fPar[10] = Vehicle->MoverParameters->AccVert; // przyspieszenie Z
    r.fPar[11] = Vehicle->MoverParameters->DistCounter; // przejechana odległość w km
    r.fPar[12] = Vehicle->MoverParameters->PipePress; // ciśnienie w PG
    r.fPar[13] = Vehicle->MoverParameters->ScndPipePress; // ciśnienie w PZ
    r.fPar[14] = Vehicle->MoverParameters->BrakePress; // ciśnienie w CH
    r.fPar[15] = Vehicle->MoverParameters->Compressor; // ciśnienie w ZG
    r.fPar[16] = Vehicle->MoverParameters->Itot; // Prąd całkowity
    r.iPar[17] = Vehicle->MoverParameters->MainCtrlPos; // Pozycja NJ
    r.iPar[18] = Vehicle->MoverParameters->ScndCtrlPos; // Pozycja NB
    r.iPar[19] = Vehicle->MoverParameters->MainCtrlActualPos; // Pozycja jezdna
    r.iPar[20] = Vehicle->MoverParameters->ScndCtrlActualPos; // Pozycja bocznikowania
    r.iPar[21] = Vehicle->MoverParameters->ScndCtrlActualPos; // Pozycja bocznikowania
    r.iPar[22] = Vehicle->MoverParameters->ResistorsFlag * 1
                + Vehicle->MoverParameters->ConverterFlag * 2
                + Vehicle->MoverParameters->CompressorFlag * 4
                + Vehicle->MoverParameters->Mains * 8
#ifdef EU07_USEOLDDOORCODE
                + Vehicle->MoverParameters->DoorLeftOpened * 16
                + Vehicle->MoverParameters->DoorRightOpened * 32
#else
                + ( false == Vehicle->MoverParameters->Doors.instances[side::left].is_closed ) * 16
                + ( false == Vehicle->MoverParameters->Doors.instances[side::right].is_closed ) * 32
#endif
                + Vehicle->MoverParameters->FuseFlag * 64
                + Vehicle->MoverParameters->DepartureSignal * 128;
    // WriteLog("Zapisalem stare");
    // WriteLog("Mam patykow "+IntToStr(t->DynamicObject->iAnimType[ANIM_PANTS]));
    for (int p = 0; p < 4; p++)
    {
        //   WriteLog("Probuje pant "+IntToStr(p));
        if (p < Vehicle->iAnimType[ANIM_PANTS])
        {
            r.fPar[23 + p] = Vehicle->pants[p].fParamPants->PantWys; // stan pantografów 4
            //     WriteLog("Zapisalem pant "+IntToStr(p));
        }
        else
        {
            r.fPar[23 + p] = -2;
            //     WriteLog("Nie mam pant "+IntToStr(p));
        }
    }
    // WriteLog("Zapisalem pantografy");
    for (int p = 0; p < 3; p++)
        r.fPar[27 + p] =
            Vehicle->MoverParameters->ShowCurrent(p + 1); // amperomierze kolejnych grup
    // WriteLog("zapisalem prady");
    r.iPar[30] = Vehicle->MoverParameters->WarningSignal; // trabienie
    r.fPar[31] = Vehicle->MoverParameters->PantographVoltage; // napiecie WN
    // WriteLog("Parametry gotowe");
    i <<= 2; // ilość bajtów
    r.cString[i] = char(j); // na końcu nazwa, żeby jakoś zidentyfikować
    strcpy(r.cString + i + 1, Vehicle->asName.c_str()); // zakończony zerem
    COPYDATASTRUCT cData;
    cData.dwData = EU07_MESSAGEHEADER; // sygnatura
    cData.cbData = (DWORD)(10 + i + j); // 8+licznik i zero kończące
    cData.lpData = &r;
    // WriteLog("Ramka gotowa");
    Navigate( "TEU07SRK", WM_COPYDATA, (WPARAM)glfwGetWin32Window( Application.window() ), (LPARAM)&cData );
    // WriteLog("Ramka poszla!");
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + Vehicle->asName + " sent");
#endif
}

void
WyslijObsadzone()
{   // wysłanie informacji o pojeździe
#ifdef _WIN32
    DaneRozkaz2 r;
    r.iSygn = EU07_MESSAGEHEADER;
	r.iComm = 12;   // kod 12
	for (int i=0; i<1984; ++i) r.cString[i] = 0;

    // TODO: clean this up, we shouldn't be relying on direct list access
    auto &vehiclelist = simulation::Vehicles.sequence();

	int i = 0;
    for( auto *vehicle : vehiclelist ) {
        if( vehicle->Mechanik ) {
            strcpy( r.cString + 64 * i, vehicle->asName.c_str() );
            r.fPar[ 16 * i + 4 ] = vehicle->GetPosition().x;
            r.fPar[ 16 * i + 5 ] = vehicle->GetPosition().y;
            r.fPar[ 16 * i + 6 ] = vehicle->GetPosition().z;
            r.iPar[ 16 * i + 7 ] = static_cast<int>( vehicle->Mechanik->action() );
            strcpy( r.cString + 64 * i + 32, vehicle->GetTrack()->RemoteIsolatedName().c_str() );
            strcpy( r.cString + 64 * i + 48, vehicle->Mechanik->TrainName().c_str() );
            i++;
            if( i > 30 ) break;
        }
    }
	while (i <= 30)
	{
		strcpy(r.cString + 64 * i, "none");
		r.fPar[16 * i + 4] = 1;
		r.fPar[16 * i + 5] = 2;
		r.fPar[16 * i + 6] = 3;
		r.iPar[16 * i + 7] = 0;
		strcpy(r.cString + 64 * i + 32, "none");
		strcpy(r.cString + 64 * i + 48, "none");
		i++;
	}

	COPYDATASTRUCT cData;
    cData.dwData = EU07_MESSAGEHEADER;     // sygnatura
	cData.cbData = 8 + 1984; // 8+licznik i zero kończące
	cData.lpData = &r;
	// WriteLog("Ramka gotowa");
    Navigate( "TEU07SRK", WM_COPYDATA, (WPARAM)glfwGetWin32Window( Application.window() ), (LPARAM)&cData );
	CommLog( Now() + " " + std::to_string(r.iComm) + " obsadzone" + " sent");
#endif
}

void
WyslijParam(int nr, int fl)
{ // wysłanie parametrów symulacji w ramce (nr) z flagami (fl)
#ifdef _WIN32
    DaneRozkaz r;
    r.iSygn = EU07_MESSAGEHEADER;
    r.iComm = nr; // zwykle 5
    r.iPar[0] = fl; // flagi istotności kolejnych parametrów
    int i = 0; // domyślnie brak danych
    switch (nr)
    { // można tym przesyłać różne zestawy parametrów
    case 5: // czas i pauza
        r.fPar[1] = Global.fTimeAngleDeg / 360.0; // aktualny czas (1.0=doba)
        r.iPar[2] = Global.iPause; // stan zapauzowania
        i = 8; // dwa parametry po 4 bajty każdy
        break;
    }
    COPYDATASTRUCT cData;
    cData.dwData = EU07_MESSAGEHEADER; // sygnatura
    cData.cbData = 12 + i; // 12+rozmiar danych
    cData.lpData = &r;
    Navigate( "TEU07SRK", WM_COPYDATA, (WPARAM)glfwGetWin32Window( Application.window() ), (LPARAM)&cData );
#endif
}

} // multiplayer

//---------------------------------------------------------------------------
