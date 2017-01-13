/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef MTABLE_H
#define MTABLE_H

#include "McZapkie/mctools.h"
#include <string>
// using namespace std;
//#include "sysutils.h"

namespace Mtable
{

static int const MaxTTableSize = 100; // mo¿na by to robiæ dynamicznie
static char const *hrsd = ".";

// Ra: pozycja zerowa rozk³adu chyba nie ma sensu
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
    int Am; // godz. i min. przyjazdu, -1 gdy bez postoju
    int Dh;
    int Dm; // godz. i min. odjazdu
    double tm; // czas jazdy do tej stacji w min. (z kolumny)
    int WaitTime; // czas postoju (liczony plus 6 sekund)
    TMTableLine()
    {
        km = 0;
        vmax = -1;
        StationName = "nowhere", StationWare = "";
        TrackNo = 1;
        Ah, Am, Dh, Dm = -1;
        WaitTime, tm = 0;
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
    int StationCount; // iloœæ przystanków (0-techniczny)
    int StationIndex; // numer najbli¿szego (aktualnego) przystanku
    std::string NextStationName;
    double LastStationLatency;
    int Direction; /*kierunek jazdy w/g kilometrazu*/
    double CheckTrainLatency();
    /*todo: str hh:mm to int i z powrotem*/
    std::string ShowRelation();
    double WatchMTable(double DistCounter);
    std::string NextStop();
    bool IsStop();
    bool IsTimeToGo(double hh, double mm);
    bool UpdateMTable(double hh, double mm, std::string NewName);
    TTrainParameters(std::string NewTrainName);
    void NewName(std::string NewTrainName);
    void UpdateVelocity(int StationCount, double vActual);
    bool LoadTTfile(std::string scnpath, int iPlus, double vmax);
    bool DirectionChange();
    void StationIndexInc();
};

class TMTableTime

{
  public:
    double GameTime;
    int dd;
    int hh;
    int mm;
    int srh;
    int srm; /*wschod slonca*/
    int ssh;
    int ssm; /*zachod slonca*/
    double mr;
    void UpdateMTableTime(double deltaT);
    TMTableTime(int InitH, int InitM, int InitSRH, int InitSRM, int InitSSH, int InitSSM);
};

extern TMTableTime *GlobalTime;
}

#if !defined(NO_IMPLICIT_NAMESPACE_USE)
using namespace Mtable;
#endif

#endif // MTABLE_H
