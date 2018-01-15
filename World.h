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
#include "scene.h"
#include "sky.h"
#include "sun.h"
#include "moon.h"
#include "stars.h"
#include "skydome.h"
#include "McZapkie/MOVER.h"
#include "messaging.h"

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
    // helper, calculates day of year from given date
    int
        year_day( int Day, int const Month, int const Year ) const;
    int
        julian_day() const;

private:
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

class opengl_renderer;

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

public:
// types

// constructors
TWorld();

// destructor
~TWorld();

// methods
    void CreateE3D( std::string const &dir = "", bool dyn = false );
    bool Init( GLFWwindow *w );
    bool InitPerformed() { return m_init; }
    bool Update();
    void OnKeyDown( int cKey );
    void OnCommandGet( multiplayer::DaneRozkaz *pRozkaz );
    // passes specified sound to all vehicles within range as a radio message broadcasted on specified channel
    void radio_message( sound_source *Message, int const Channel );
    void CabChange( TDynamicObject *old, TDynamicObject *now );
    void TrainDelete(TDynamicObject *d = NULL);
    TTrain* train() { return Train; }
    // switches between static and dynamic daylight calculation
    void ToggleDaylight();
    // calculates current season of the year based on set simulation date
    void compute_season( int const Yearday ) const;

// members

private:
    void Update_Environment();
    void Update_Camera( const double Deltatime );
    void Update_UI();
    void ResourceSweep();
    // handles vehicle change flag
    void ChangeDynamic();
    void InOutKey( bool const Near = true );
    void FollowView( bool wycisz = true );
    void DistantView( bool const Near = false );

    TCamera Camera;
    TCamera DebugCamera;
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
    bool m_init { false }; // indicates whether initial update of the world was performed
    GLFWwindow *window;
};

//---------------------------------------------------------------------------

