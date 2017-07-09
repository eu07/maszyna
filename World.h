/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include "Camera.h"
#include "Ground.h"
#include "sky.h"
#include "sun.h"
#include "moon.h"
#include "stars.h"
#include "skydome.h"
#include "mczapkie/mover.h"

// wrapper for simulation time
class simulation_time {

public:
    simulation_time() { m_time.wHour = 10; m_time.wMinute = 30; }
    void
        init();
    void
        update( double const Deltatime );
    SYSTEMTIME &
        data() { return m_time; }
    SYSTEMTIME const &
        data() const { return m_time; }
    double
        second() const { return ( m_time.wMilliseconds * 0.001 + m_time.wSecond ); }
    int
        year_day() const { return m_yearday; }
    int
        julian_day() const;

private:
    // calculates day of year from given date
    int
        yearday( int Day, int const Month, int const Year );
    // calculates day and month from given day of year
    void
        daymonth( WORD &Day, WORD &Month, WORD const Year, WORD const Yearday );

    SYSTEMTIME m_time;
    double m_milliseconds{ 0.0 };
    int m_yearday;
    char m_monthdaycounts[ 2 ][ 13 ];
};

namespace simulation {

extern simulation_time Time;

}

namespace locale {

std::string
    control( std::string const &Label );

static std::unordered_map<std::string, std::string> m_controls = {
    { "mainctrl:", "master controller" },
    { "scndctrl:", "second controller" },
    { "dirkey:" , "reverser" },
    { "brakectrl:", "train brake" },
    { "localbrake:", "independent brake" },
    { "manualbrake:", "manual brake" },
    { "brakeprofile_sw:", "brake acting speed" },
    { "brakeprofileg_sw:", "brake acting speed: cargo" },
    { "brakeprofiler_sw:", "brake acting speed: rapid" },
    { "maxcurrent_sw:", "motor overload relay threshold" },
    { "main_off_bt:", "line breaker" },
    { "main_on_bt:", "line breaker" },
    { "security_reset_bt:", "alerter" },
    { "releaser_bt:", "independent brake releaser" },
    { "sand_bt:", "sandbox" },
    { "antislip_bt:", "wheelspin brake" },
    { "horn_bt:", "horn" },
    { "hornlow_bt:", "low tone horn" },
    { "hornhigh_bt:", "high tone horn" },
    { "fuse_bt:", "motor overload relay reset" },
    { "converterfuse_bt:", "converter overload relay reset" },
    { "stlinoff_bt:", "motor connectors" },
    { "door_left_sw:", "left door" },
    { "door_right_sw:", "right door" },
    { "departure_signal_bt:", "departure signal" },
    { "upperlight_sw:", "upper headlight" },
    { "leftlight_sw:", "left headlight" },
    { "rightlight_sw:", "right headlight" },
    { "dimheadlights_sw:", "headlights dimmer" },
    { "leftend_sw:", "left marker light" },
    { "rightend_sw:", "right marker light" },
    { "lights_sw:", "light pattern" },
    { "rearupperlight_sw:", "rear upper headlight" },
    { "rearleftlight_sw:", "rear left headlight" },
    { "rearrightlight_sw:", "rear right headlight" },
    { "rearleftend_sw:", "rear left marker light" },
    { "rearrightend_sw:",  "rear right marker light" },
    { "compressor_sw:", "compressor" },
    { "compressorlocal_sw:", "local compressor" },
    { "converter_sw:", "converter" },
    { "converterlocal_sw:", "local converter" },
    { "converteroff_sw:", "converter" },
    { "main_sw:", "line breaker" },
    { "radio_sw:", "radio" },
    { "pantfront_sw:", "front pantograph" },
    { "pantrear_sw:", "rear pantograph" },
    { "pantfrontoff_sw:", "front pantograph" },
    { "pantrearoff_sw:", "rear pantograph" },
    { "pantalloff_sw:", "all pantographs" },
    { "pantselected_sw:", "selected pantograph" },
    { "pantselectedoff_sw:", "selected pantograph" },
    { "trainheating_sw:", "heating" },
    { "signalling_sw:", "braking indicator" },
    { "door_signalling_sw:", "door locking" },
    { "nextcurrent_sw:", "current indicator source" },
    { "cablight_sw:", "interior light" },
    { "cablightdim_sw:", "interior light dimmer" },
    { "battery_sw:", "battery" }
};

}

// wrapper for environment elements -- sky, sun, stars, clouds etc
class world_environment {

    friend opengl_renderer;

public:
    void init();
    void update();
    void time( int const Hour = -1, int const Minute = -1, int const Second = -1 );

private:
    CSkyDome m_skydome;
    cStars m_stars;
    cSun m_sun;
    cMoon m_moon;
    TSky m_clouds;
};

class TWorld
{
    // NOTE: direct access is a shortcut, but world etc needs some restructuring regardless
    friend opengl_renderer;

    void InOutKey( bool const Near = true );
    void FollowView(bool wycisz = true);
    void DistantView( bool const Near = false );

public:
// types

// constructors
TWorld();

// destructor
~TWorld();

// methods
    bool Init( GLFWwindow *w );
    bool InitPerformed() { return m_init; }
    GLFWwindow *window;
    void OnKeyDown(int cKey);
    // void UpdateWindow();
    void OnMouseMove(double x, double y);
    void OnCommandGet(DaneRozkaz *pRozkaz);
    bool Update();
    void TrainDelete(TDynamicObject *d = NULL);
    TTrain const *
        train() const { return Train; }
    // switches between static and dynamic daylight calculation
    void ToggleDaylight();

private:
    void Update_Environment();
    void Update_Camera( const double Deltatime );
    void Update_UI();
    void ResourceSweep();

    TCamera Camera;
    TGround Ground;
    world_environment Environment;
    TTrain *Train;
    TDynamicObject *pDynamicNearest;
    bool Paused{ true };
    TEvent *KeyEvents[10]; // eventy wyzwalane z klawiaury
    TMoverParameters *mvControlled; // wskaźnik na człon silnikowy, do wyświetlania jego parametrów
    double fTime50Hz; // bufor czasu dla komunikacji z PoKeys
    double fTimeBuffer; // bufor czasu aktualizacji dla stałego kroku fizyki
    double fMaxDt; //[s] krok czasowy fizyki (0.01 dla normalnych warunków)
    double m_primaryupdaterate{ 1.0 / 100.0 };
    double m_secondaryupdaterate{ 1.0 / 50.0 };
    double m_primaryupdateaccumulator{ m_secondaryupdaterate }; // keeps track of elapsed simulation time, for core fixed step routines
    double m_secondaryupdateaccumulator{ m_secondaryupdaterate }; // keeps track of elapsed simulation time, for less important fixed step routines
    int iPause; // wykrywanie zmian w zapauzowaniu
    double VelPrev; // poprzednia prędkość
    int tprev; // poprzedni czas
    double Acc; // przyspieszenie styczne
    bool m_init{ false }; // indicates whether initial update of the world was performed

  public:
    void ModifyTGA(std::string const &dir = "");
    void CreateE3D(std::string const &dir = "", bool dyn = false);
    void CabChange(TDynamicObject *old, TDynamicObject *now);
    // handles vehicle change flag
    void ChangeDynamic();
};

//---------------------------------------------------------------------------

