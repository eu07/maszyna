/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "mtable.h"
#include "Globals.h"
#include "simulationtime.h"
#include "dictionary.h"
#include "utilities.h"

double TTrainParameters::CheckTrainLatency()
{
    if ((LastStationLatency > 1.0) || (LastStationLatency < 0))
        return LastStationLatency; /*spoznienie + lub do przodu - z tolerancja 1 min*/
    else
        return 0;
}

double TTrainParameters::WatchMTable(double DistCounter)
{ // zwraca odleglość do najblizszej stacji z zatrzymaniem
    double dist;

    if (Direction == 1)
        dist = TimeTable[StationIndex].km - TimeTable[0].km - DistCounter;
    else
        dist = TimeTable[0].km - TimeTable[StationIndex].km - DistCounter;
    return dist;
}

std::string TTrainParameters::NextStop() const
{ // pobranie nazwy następnego miejsca zatrzymania
    if (StationIndex <= StationCount)
        return NextStationName; // nazwa następnego przystanku;
    else
        return "[End of route]"; //że niby koniec
}

sound_source
TTrainParameters::next_stop_sound() const {
    if( StationIndex > StationCount ) {
        return { sound_placement::engine };
    }
    for( auto stationidx { StationIndex }; stationidx < StationCount + 1; ++stationidx ) {
        auto &station{ TimeTable[ stationidx ] };
        if( station.Ah == -1 ) {
            continue;
        }
        // specified arrival time means it's a scheduled stop
        return station.name_sound;
    }
    // shouldn't normally get here, unless the timetable is malformed
    return { sound_placement::engine };
}

sound_source
TTrainParameters::last_stop_sound() const {

    return TimeTable[ StationCount ].name_sound;
}

bool TTrainParameters::IsStop() const
{ // zapytanie, czy zatrzymywać na następnym punkcie rozkładu
    if ((StationIndex <= StationCount))
        return TimeTable[StationIndex].Ah >= 0; //-1 to brak postoju
    else
        return true; // na ostatnim się zatrzymać zawsze
}

bool TTrainParameters::IsLastStop() const {

    return ( StationIndex >= StationCount );
}


bool TTrainParameters::IsMaintenance() const {
    if( ( StationIndex <= StationCount ) )
        return TimeTable[ StationIndex ].is_maintenance;
    else
        return false;
}

int TTrainParameters::radio_channel() const {
    if( ( StationIndex <= StationCount ) )
        return TimeTable[ StationIndex ].radio_channel;
    else
        return -1;
}

// returns: sound file associated with current station, or -1
sound_source TTrainParameters::current_stop_sound() const {
    if( ( StationIndex <= StationCount ) )
        return TimeTable[ StationIndex ].name_sound;
    else
        return { sound_placement::engine };
}

bool TTrainParameters::UpdateMTable( scenario_time const &Time, std::string const &NewName ) {

    return UpdateMTable( Time.data().wHour, Time.data().wMinute + Time.data().wSecond * 0.0167, NewName );
}

bool TTrainParameters::UpdateMTable(double hh, double mm, std::string const &NewName)
/*odfajkowanie dojechania do stacji (NewName) i przeliczenie opóźnienia*/
{
    bool OK;
    OK = false;
    if (StationIndex <= StationCount) // Ra: "<=", bo ostatni przystanek jest traktowany wyjątkowo
    {
        if (NewName == NextStationName) // jeśli dojechane do następnego
        { // Ra: wywołanie może być powtarzane, jak stoi na W4
            if (TimeTable[StationIndex + 1].km - TimeTable[StationIndex].km < 0) // to jest bez sensu
                Direction = -1;
            else
                Direction = 1; // prowizorka bo moze byc zmiana kilometrazu
            // ustalenie, czy opóźniony (porównanie z czasem odjazdu)
            LastStationLatency =
                CompareTime(hh, mm, TimeTable[StationIndex].Dh, TimeTable[StationIndex].Dm);
            // inc(StationIndex); //przejście do następnej pozycji StationIndex<=StationCount
            // Ra: "<", bo dodaje 1 przy przejściu do następnej stacji
            if (StationIndex < StationCount) {
                // jeśli nie ostatnia stacja
                NextStationName = TimeTable[StationIndex + 1].StationName; // zapamiętanie nazwy
                // Ra: nowa prędkość rozkładowa na kolejnym odcinku
                TTVmax = TimeTable[StationIndex + 1].vmax;
            }
            else {
                // gdy ostatnia stacja, nie ma następnej stacji
                NextStationName = "";
            }
            OK = true;
        }
    }
    return OK; /*czy jest nastepna stacja*/
}

bool Mtable::TTrainParameters::RewindTimeTable(std::string actualStationName) {

    if( actualStationName.compare( 0, 19, "PassengerStopPoint:" ) == 0 ) {
        actualStationName = ToLower( actualStationName.substr( 19 ) );
    }
    for( auto i = 1; i <= StationCount; ++i ) {
        // przechodzimy po całej tabelce i sprawdzamy nazwy stacji (bez pierwszej)
        if (ToLower(TimeTable[i].StationName) == actualStationName) {
            // nazwa stacji zgodna więc ustawiamy na poprzednią, żeby w następnym kroku poprawnie obsłużyć
            StationIndex = i;
            NextStationName = TimeTable[ i ].StationName;
            TTVmax = TimeTable[ i ].vmax;
            return true; // znaleźliśmy więc kończymy
        }
    }
    // failed to find a match
    return false;
}

void TTrainParameters::StationIndexInc()
{ // przejście do następnej pozycji StationIndex<=StationCount
    ++StationIndex;
}

bool TTrainParameters::IsTimeToGo(double hh, double mm)
// sprawdzenie, czy można już odjechać z aktualnego zatrzymania
// StationIndex to numer następnego po dodarciu do aktualnego
{
    if ((StationIndex < 1))
        return true; // przed pierwszą jechać
    else if ((StationIndex < StationCount))
    { // oprócz ostatniego przystanku
        if ((TimeTable[StationIndex].Ah < 0)) // odjazd z poprzedniego
            return true; // czas przyjazdu nie był podany - przelot
        else
            return CompareTime(hh, mm, TimeTable[StationIndex].Dh, TimeTable[StationIndex].Dm) <= 0;
    }
    else // gdy rozkład się skończył
        return false; // dalej nie jechać
}

// returns: difference between specified time and scheduled departure from current stop, in seconds
double TTrainParameters::seconds_until_departure( double const Hour, double const Minute ) const {

    if( ( TimeTable[ StationStart ].Ah < 0 ) ) { // passthrough
        return 0;
    }
    return ( 60.0 * CompareTime( Hour, Minute, TimeTable[ StationStart ].Dh, TimeTable[ StationStart ].Dm ) );
}

std::string TTrainParameters::ShowRelation() const
/*zwraca informację o relacji*/
{
    // if (Relation1=TimeTable[1].StationName) and (Relation2=TimeTable[StationCount].StationName)
    if ((Relation1 != "") && (Relation2 != ""))
        return Relation1 + " - " + Relation2;
    else
        return "";
}

TTrainParameters::TTrainParameters(std::string const &NewTrainName)
/*wstępne ustawienie parametrów rozkładu jazdy*/
{
    NewName(NewTrainName);
}

void TTrainParameters::NewName(std::string const &NewTrainName)
/*wstępne ustawienie parametrów rozkładu jazdy*/
{
    TrainName = NewTrainName;
    StationCount = 0;
    StationIndex = 0;
    StationStart = 0;
    NextStationName = "nowhere";
    LastStationLatency = 0;
    Direction = 1;
    Relation1 = "";
    Relation2 = "";
    for (int i = 0; i < MaxTTableSize + 1; ++i)
    {
        TimeTable[ i ] = TMTableLine();
    }
    TTVmax = 100; /*wykasowac*/
    BrakeRatio = 0;
    LocSeries = "";
    LocLoad = 0;
}

void TTrainParameters::UpdateVelocity(int StationCount, double vActual)
// zapisywanie prędkości maksymalnej do wcześniejszych odcinków
// wywoływane z numerem ostatniego przetworzonego przystanku
{
    int i = StationCount;
    // TTVmax:=vActual;  {PROWIZORKA!!!}
    while ((i >= 0) && (TimeTable[i].vmax == -1))
    {
        TimeTable[i].vmax = vActual; // prędkość dojazdu do przystanku i
        --i; // ewentualnie do poprzedniego też
    }
}

// bool TTrainParameters::LoadTTfile(std::string scnpath, int iPlus, double vmax)
//{
//	return false;
//}

bool TTrainParameters::LoadTTfile(std::string scnpath, int iPlus, double vmax)
// wczytanie pliku-tabeli z rozkładem przesuniętym o (fPlus); (vMax) nie ma znaczenia
{
    std::string lines;
    std::string s;
    std::ifstream fin;
    bool EndTable;
    double vActual;

    int ConversionError = 0;
    EndTable = false;
    if ((TrainName == ""))
    { // jeśli pusty rozkład
        // UpdateVelocity(StationCount,vMax); //ograniczenie do prędkości startowej
    }
    else
    {
        ConversionError = 666;
        vActual = -1;
        s = scnpath + TrainName + ".txt";
		replace_slashes(s);
        // Ra 2014-09: ustalić zasady wyznaczenia pierwotnego pliku przy przesuniętych rozkładach
        // (kolejny pociąg dostaje numer +2)
        fin.open(s.c_str()); // otwieranie pliku
		replace_slashes(s);

        if (!fin.is_open())
        { // jeśli nie ma pliku
            vmax = atoi(TrainName.c_str()); // nie ma pliku ale jest liczba
            if ((vmax > 10) && (vmax < 200))
            {
                TTVmax = vmax; // Ra 2014-07: zamiast rozkładu można podać Vmax
                UpdateVelocity(StationCount, vmax); // ograniczenie do prędkości startowej
                ConversionError = 0;
            }
            else
                ConversionError = -8; /*Ra: ten błąd jest niepotrzebny*/
        }
        else
        { /*analiza rozkładu jazdy*/
            ConversionError = 0;
            while (fin.good() && !((ConversionError != 0) || EndTable))
            {
                std::getline(fin, lines); /*wczytanie linii*/
                if (contains( lines, "___________________") ) /*linia pozioma górna*/
                {
                    fin >> s;
                    if (s == "[") /*lewy pion*/
                    {
                        fin >> s;
                        if (s == "Rodzaj") /*"Rodzaj i numer pociagu"*/
                            do
                            {
                                fin >> s;
                            } while (!(s == "|") || (fin.eof())); /*środkowy pion*/
                    }
                }
                else if (lines == "")
                {
                    fin.close();
                    break;
                }
                fin >> s; /*nazwa pociągu*/
                // if LowerCase(s)<>ExtractFileName(TrainName) then {musi być taka sama, jak nazwa
                // pliku}
                // ConversionError:=-7 {błąd niezgodności}
                TrainName = s; // nadanie nazwy z pliku TXT (bez ścieżki do pliku)
				while (fin >> s || fin.bad())
				{
					if (contains( s,"_______|") )
					{
						break;
					}
					if (s == "Kategoria")
					{
						do
						{
							fin >> s;
						} while (!((s == "|") || (fin.bad())));
						fin >> TrainCategory;
		                        continue;
					}
					if (s == "Nazwa")
			                {
			                        do
			                        {
			                            fin >> s;
				                } while (!((s == "|") || (fin.bad())));
			                        fin >> TrainLabel;
			                        continue;
			                 }
				} // while (!(s == "Seria"));
                // else
                { /*czytaj naglowek*/
                    while (fin >> s || !fin.bad())
                    {
                        if (s == "[")
                            break;
                    } // while (!() || (fin.eof())); /*pierwsza linia z relacją*/
                    while (fin >> s || !fin.bad())
                    {
                        if (s != "|")
                            break;
                    } // while (!(() || fin.eof()));
                    if( s != "|" ) {
                        Relation1 = s;
                        win1250_to_ascii( Relation1 );
                    }
                    else
                        ConversionError = -5;
                    while (fin >> s || !fin.bad())
                    {
                        if (s == "Relacja")
                            break;
                    } // while (
                    //  !( || (fin.eof()))); /*druga linia z relacją*/
                    while (fin >> s || !fin.bad())
                    {
                        if (s == "|")
                            break;
                    } // while (!( || (fin.eof())));
                    fin >> Relation2;
                    win1250_to_ascii( Relation2 );
                    while (fin >> s || !fin.bad())
                    {
                        if (s == "Wymagany")
                            break;
                    } // while (!();
                    while (fin >> s || !fin.bad())
                    {
                        if ((s == "|") || (s == "\n"))
                            break;
                    } // while (!());
                    fin >> s;
                    s = s.substr(0, s.find("%"));
                    BrakeRatio = atof(s.c_str());
                    while (fin >> s || fin.bad())
                    {
                        if (s == "Seria")
                            break;
                    } // while (!(s == "Seria"));
                    do
                    {
                        fin >> s;
                    } while (!((s == "|") || (fin.bad())));
                    fin >> LocSeries;
                    fin >> LocLoad; // = s2rE(ReadWord(fin));
                    do
                    {
                        fin >> s;
                    } while (!(contains( s,"[______________" ) || fin.bad()));
                    auto activeradiochannel{ -1 };
                    while (!fin.bad() && !EndTable)
                    {
                        ++StationCount;
                        do
                        {
                            fin >> s;
                        } while (!((s == "[") || (fin.bad())));
                        TMTableLine *record = &TimeTable[StationCount];
                        {
                            if (s == "[")
                                fin >> s;
                            else
                                ConversionError = -4;
                            if (false == contains( s,"|") )
                            {
                                record->km = atof(s.c_str());
                                fin >> s;
                            }
                            if (contains( s,"|_____|")) /*zmiana predkosci szlakowej*/
                                UpdateVelocity(StationCount, vActual);
                            else
                            {
                                fin >> s;
                                if (false == contains(s,"|"))
                                    vActual = atof(s.c_str());
                            }
                            while (false == contains( s,"|"))
                                fin >> s;
                            fin >> record->StationName;
                            // get rid of non-ascii chars. TODO: run correct version based on locale
                            win1250_to_ascii( record->StationName ); 
                            do
                            {
                                fin >> s;
                            } while (!((s == "1") || (s == "2") || fin.bad()));
                            record->TrackNo = atoi(s.c_str());
                            fin >> s;
                            if (s != "|")
                            {
                                if (contains( s, hrsd) )
                                {
                                    record->Ah = atoi( s.substr(0, s.find(hrsd)).c_str()); // godzina przyjazdu
                                    record->Am = atof(s.substr(s.find(hrsd) + 1, s.length()).c_str()); // minuta przyjazdu
                                }
                                else
                                {
                                    record->Ah = TimeTable[StationCount - 1].Ah; // godzina z poprzedniej pozycji
                                    record->Am = atof(s.c_str()); // bo tylko minuty podane
                                }
                            }
                            do
                            {
                                fin >> s;
                            } while (!((s != "|") || (fin.bad())));
                            if (s != "]")
                                record->tm = atof(s.c_str());
                            do
                            {
                                fin >> s;
                            } while (!((s == "[") || fin.bad()));
                            fin >> s;
                            if (false == contains(s,"|"))
                            {
                                /*tu s moze byc miejscem zmiany predkosci szlakowej*/
                                fin >> s;
                            }
                            if (s.find("|_____|") !=
                                std::string::npos) /*zmiana predkosci szlakowej*/
                                UpdateVelocity(StationCount, vActual);
                            else
                            {
                                fin >> s;
                                if (false == contains(s,"|"))
                                    vActual = atof(s.c_str());
                            }
                            while (false == contains(s,"|"))
                                fin >> s;
                            // stationware. added fix for empty entry
                            fin >> s;
                            while( false == ( ( s == "1" )
                                           || ( s == "2" )
                                           || fin.bad() ) ) {
                                record->StationWare += s;
                                fin >> s;
                            }
                            // cache relevant station data
                            record->is_maintenance = ( contains( s, "pt" ) );
                            {
                                auto const stationware { Split( record->StationWare, ',' ) };
                                for( auto const &entry : stationware ) {
                                    if( entry.front() != 'R' ) {
                                        continue;
                                    }
                                    auto const entrysplit { split_string_and_number( entry ) };
                                    if( ( entrysplit.first == "R" )
                                     && ( entrysplit.second <= 10 ) ) {
                                        auto const radiochannel { entrysplit.second };
                                        if( ( record->radio_channel == -1 )
                                         || ( radiochannel != activeradiochannel ) ) {
                                            // if the station has more than one radiochannel listed,
                                            // it generally means we should switch to the one we weren't using so far
                                            // TODO: reverse this behaviour (keep the channel used so far) once W28 signs are included in the system
                                            record->radio_channel = radiochannel;
                                        }
                                    }
                                }
                                if( record->radio_channel != -1 ) {
                                    activeradiochannel = record->radio_channel;
                                }
                            }
                            record->TrackNo = atoi(s.c_str());
                            fin >> s;
                            if (s != "|")
                            {
                                if (contains( s, hrsd) )
                                {
                                    record->Dh = atoi(s.substr(0, s.find(hrsd)).c_str()); // godzina odjazdu
                                    record->Dm = atof(s.substr(s.find(hrsd) + 1, s.length()).c_str()); // minuta odjazdu
                                }
                                else
                                {
                                    record->Dh = TimeTable[StationCount - 1].Dh; // godzina z poprzedniej pozycji
                                    record->Dm = atof(s.c_str()); // bo tylko minuty podane
                                }
                            }
                            else
                            {
                                record->Dh = record->Ah; // odjazd o tej samej, co przyjazd (dla ostatniego też)
                                record->Dm = record->Am; // bo są używane do wyliczenia opóźnienia po dojechaniu
                            }
                            do
                            {
                                fin >> s;
                            } while (!((s != "|") || (fin.bad())));
                            if (s != "]")
                                record->tm = atof(s.c_str());
                            do
                            {
                                fin >> s;
                            } while (!(contains( s, "[" ) || fin.bad()));
                            if (false == contains( s, "_|_"))
                                fin >> s;
                            if (false == contains( s, "|"))
                            {
                                /*tu s moze byc miejscem zmiany predkosci szlakowej*/
                                fin >> s;
                            }
                            if (contains( s, "|_____|") ) /*zmiana predkosci szlakowej*/
                                UpdateVelocity(StationCount, vActual);
                            else
                            {
                                fin >> s;
                                if (false == contains( s, "|"))
                                    vActual = atof(s.c_str());
                            }
                            while (false == contains( s, "|" ) )
                                fin >> s;
                            while ((false == contains( s,"]") ))
                                fin >> s;
                            if (contains( s,"_|_") )
                                EndTable = true;
                        } /*timetableline*/
                    }
                }
            } /*while eof*/
            fin.close();
        }
    }
    if (ConversionError == 0)
    {
        if ((TimeTable[1].StationName == Relation1)) // jeśli nazwa pierwszego zgodna z relacją
            if ((TimeTable[1].Ah < 0)) // a nie podany czas przyjazdu
            { // to mamy zatrzymanie na pierwszym, a nie przelot
                TimeTable[1].Ah = TimeTable[1].Dh;
                TimeTable[1].Am = TimeTable[1].Dm;
            }
    }
    //
    load_sounds();
    // potentially offset table times
    auto const timeoffset { static_cast<int>( Global.ScenarioTimeOffset * 60 ) + iPlus };
    if( timeoffset != 0 ) // jeżeli jest przesunięcie rozkładu
    {
        long i_end = StationCount + 1;
        float adjustedtime; // do zwiększania czasu
        for (auto i = 1; i < i_end; ++i) // bez with, bo ciężko się przenosi na C++
        {
            if ((TimeTable[i].Ah >= 0))
            {
                adjustedtime = clamp_circular<float>( TimeTable[i].Ah * 60 + TimeTable[i].Am + timeoffset, 24 * 60 ); // nowe minuty
                TimeTable[i].Am = (int(60 * adjustedtime) % 3600) / 60.f;
                TimeTable[i].Ah = int((adjustedtime) / 60) % 24;
            }
            if ((TimeTable[i].Dh >= 0))
            {
                adjustedtime = clamp_circular<float>( TimeTable[i].Dh * 60 + TimeTable[i].Dm + timeoffset, 24 * 60 ); // nowe minuty
                TimeTable[i].Dm = (int(60 * adjustedtime) % 3600) / 60.f;
                TimeTable[i].Dh = int((adjustedtime) / 60) % 24;
            }
        }
    }
    return ConversionError == 0;
}

void
TTrainParameters::load_sounds() {

    for( auto stationidx = 1; stationidx < StationCount + 1; ++stationidx ) {
        auto &station { TimeTable[ stationidx ] };
        if( station.Ah == -1 ) {
            continue;
        }
        // specified arrival time means it's a scheduled stop
        auto const stationname { (
            ends_with( station.StationName, "_po" ) ?
                station.StationName.substr( 0, station.StationName.size() - 3 ) :
                station.StationName ) };

        auto const lookup {
            FileExists(
                { Global.asCurrentSceneryPath + stationname, std::string{ szSoundPath } + "sip/" + stationname },
                { ".ogg", ".flac", ".wav" } ) };
        if( lookup.first.empty() ) {
            continue;
        }
        //  wczytanie dźwięku odjazdu w wersji radiowej (słychać tylko w kabinie)
        station.name_sound =
            sound_source{ sound_placement::engine, EU07_SOUND_CABANNOUNCEMENTCUTOFFRANGE }
                .deserialize( lookup.first + lookup.second, sound_type::single );
    }
}

bool TTrainParameters::DirectionChange()
// sprawdzenie, czy po zatrzymaniu wykonać kolejne komendy
{
    if ((StationIndex > 0) && (StationIndex < StationCount)) // dla ostatniej stacji nie
        if (contains( TimeTable[StationIndex].StationWare, '@') )
            return true;
    return false;
}

void TTrainParameters::serialize( dictionary_source *Output ) const {

    Output->insert( "trainnumber", TrainName );
	Output->insert( "traincategory", TrainCategory );
	Output->insert( "trainname", TrainLabel );
    Output->insert( "train_brakingmassratio", BrakeRatio );
    Output->insert( "train_enginetype", LocSeries );
    Output->insert( "train_engineload", LocLoad );

    Output->insert( "train_stationfrom", Relation1 );
    Output->insert( "train_stationto", Relation2 );
    Output->insert( "train_stationindex", StationIndex );
    Output->insert( "train_stationcount", StationCount );
    Output->insert( "train_stationstart", StationStart );
    if( StationCount > 0 ) {
        // timetable stations data, if there's any
        for( auto stationidx = 1; stationidx <= StationCount; ++stationidx ) {
            auto const stationlabel { "train_station" + std::to_string( stationidx ) + "_" };
            auto const &timetableline { TimeTable[ stationidx ] };
            Output->insert( ( stationlabel + "name" ), Bezogonkow( timetableline.StationName ) );
            Output->insert( ( stationlabel + "fclt" ), Bezogonkow( timetableline.StationWare ) );
            Output->insert( ( stationlabel + "lctn" ), timetableline.km );
            Output->insert( ( stationlabel + "vmax" ), timetableline.vmax );
            Output->insert( ( stationlabel + "ah" ), timetableline.Ah );
            Output->insert( ( stationlabel + "am" ), timetableline.Am );
            Output->insert( ( stationlabel + "dh" ), timetableline.Dh );
            Output->insert( ( stationlabel + "dm" ), timetableline.Dm );
            Output->insert( ( stationlabel + "tracks" ), timetableline.TrackNo );
        }
    }
}

void TMTableTime::UpdateMTableTime(double deltaT)
// dodanie czasu (deltaT) w sekundach, z przeliczeniem godziny
{
    mr += deltaT; // dodawanie sekund
    while (mr >= 60.0) // przeliczenie sekund do właściwego przedziału
    {
        mr -= 60.0;
        ++mm;
    }
    while (mm > 59) // przeliczenie minut do właściwego przedziału
    {
        mm -= 60;
        ++hh;
    }
    while (hh > 23) // przeliczenie godzin do właściwego przedziału
    {
        hh -= 24;
        ++dd; // zwiększenie numeru dnia
    }
}
