/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>

#include "Classes.h"
#include "sound.h"

namespace Mtable
{

static int const MaxTTableSize = 100; // można by to robić dynamicznie
static char const *hrsd = ".";

// Ra: pozycja zerowa rozkładu chyba nie ma sensu
// Ra: numeracja przystanków jest 1..StationCount

struct TMTableLine
{
    float km{ 0.f }; // kilometraz linii
    float vmax{ -1.f }; // predkosc rozkladowa przed przystankiem
    // StationName:string[32]; //nazwa stacji ('_' zamiast spacji)
    // StationWare:string[32]; //typ i wyposazenie stacji, oddz. przecinkami}
    std::string StationName{ "nowhere" }; // nazwa stacji ('_' zamiast spacji)
    std::string StationWare; // typ i wyposazenie stacji, oddz. przecinkami}
    int TrackNo{ 1 }; // ilosc torow szlakowych
    int Ah{ -1 };
    float Am{ -1.f }; // godz. i min. przyjazdu, -1 gdy bez postoju
    int Dh{ -1 };
    float Dm{ -1.f }; // godz. i min. odjazdu
    float tm{ 0.f }; // czas jazdy do tej stacji w min. (z kolumny)
    bool is_maintenance{ false };
    int radio_channel{ -1 };
    sound_source name_sound{ sound_placement::engine };
};

typedef TMTableLine TMTable[MaxTTableSize + 1];

// typedef TTrainParameters *PTrainParameters;

class TTrainParameters
{
  public:
    std::string TrainName;
	std::string TrainCategory;
    std::string TrainLabel;
    double TTVmax;
    std::string Relation1;
    std::string Relation2; // nazwy stacji danego odcinka
    double BrakeRatio;
    std::string LocSeries; // seria (typ) pojazdu
    double LocLoad;
    TMTable TimeTable;
    int StationCount; // ilość przystanków (0-techniczny)
    int StationIndex; // numer najbliższego (aktualnego) przystanku
    int StationStart; // numer pierwszej stacji pokazywanej na podglądzie rozkładu
    std::string NextStationName;
    double LastStationLatency;
    int Direction; /*kierunek jazdy w/g kilometrazu*/
    double CheckTrainLatency();
    /*todo: str hh:mm to int i z powrotem*/
    std::string ShowRelation() const;
    double WatchMTable(double DistCounter);
    std::string NextStop() const;
    sound_source next_stop_sound() const;
    sound_source last_stop_sound() const;
    bool IsStop() const;
    bool IsLastStop() const;
    bool IsMaintenance() const;
    bool IsTimeToGo(double hh, double mm);
    // returns: difference between specified time and scheduled departure from current stop, in seconds
    double seconds_until_departure( double const Hour, double const Minute ) const;
    bool UpdateMTable(double hh, double mm, std::string const &NewName);
    bool UpdateMTable( scenario_time const &Time, std::string const &NewName );
    bool RewindTimeTable( std::string actualStationName );
    TTrainParameters( std::string const &NewTrainName = "none" );
    void NewName(std::string const &NewTrainName);
    void UpdateVelocity(int StationCount, double vActual);
    bool LoadTTfile(std::string scnpath, int iPlus, double vmax);
    bool DirectionChange();
    void StationIndexInc();
    void serialize( dictionary_source *Output ) const;
    // returns: radio channel associated with current station, or -1
    int radio_channel() const;
    // returns: sound file associated with current station, or -1
    sound_source current_stop_sound() const;
private:
    void load_sounds();
};

class TMTableTime

{
  public:
    int dd = 0;
    int hh = 0;
    int mm = 0;
    double mr = 0.0;
    void UpdateMTableTime(double deltaT);
    TMTableTime(int InitH, int InitM ) :
        hh( InitH ),
        mm( InitM )
	{}

	TMTableTime() = default;
};

}

#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Mtable;
#endif
