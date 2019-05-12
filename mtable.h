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

namespace Mtable
{

static int const MaxTTableSize = 100; // można by to robić dynamicznie
static char const *hrsd = ".";

// Ra: pozycja zerowa rozkładu chyba nie ma sensu
// Ra: numeracja przystanków jest 1..StationCount

struct TMTableLine
{
    double km; // kilometraz linii
    double vmax; // predkosc rozkladowa przed przystankiem
    // StationName:string[32]; //nazwa stacji ('_' zamiast spacji)
    // StationWare:string[32]; //typ i wyposazenie stacji, oddz. przecinkami}
    std::string StationName; // nazwa stacji ('_' zamiast spacji)
    std::string StationWare; // typ i wyposazenie stacji, oddz. przecinkami}
    int TrackNo; // ilosc torow szlakowych
    int Ah;
    double Am; // godz. i min. przyjazdu, -1 gdy bez postoju
    int Dh;
    double Dm; // godz. i min. odjazdu
    double tm; // czas jazdy do tej stacji w min. (z kolumny)
    TMTableLine()
    {
        km = 0;
        vmax = -1;
        StationName = "nowhere", StationWare = "";
        TrackNo = 1;
        Ah = Am = Dh = Dm = -1;
        tm = 0;
    }
};

typedef TMTableLine TMTable[MaxTTableSize + 1];

// typedef TTrainParameters *PTrainParameters;

class TTrainParameters
{
  public:
    std::string TrainName;
    double TTVmax;
    std::string Relation1;
    std::string Relation2; // nazwy stacji danego odcinka
    double BrakeRatio;
    std::string LocSeries; // seria (typ) pojazdu
    double LocLoad;
    TMTable TimeTable;
    int StationCount; // ilość przystanków (0-techniczny)
    int StationIndex; // numer najbliższego (aktualnego) przystanku
    std::string NextStationName;
    double LastStationLatency;
    int Direction; /*kierunek jazdy w/g kilometrazu*/
    double CheckTrainLatency();
    /*todo: str hh:mm to int i z powrotem*/
    std::string ShowRelation() const;
    double WatchMTable(double DistCounter);
    std::string NextStop() const;
    bool IsStop() const;
    bool IsTimeToGo(double hh, double mm);
    bool UpdateMTable(double hh, double mm, std::string const &NewName);
    bool UpdateMTable( scenario_time const &Time, std::string const &NewName );
    bool RewindTimeTable( std::string actualStationName );
    TTrainParameters( std::string const &NewTrainName );
    void NewName(std::string const &NewTrainName);
    void UpdateVelocity(int StationCount, double vActual);
    bool LoadTTfile(std::string scnpath, int iPlus, double vmax);
    bool DirectionChange();
    void StationIndexInc();
    void serialize( dictionary_source *Output ) const;
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
