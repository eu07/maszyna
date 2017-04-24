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
    int m_yearday;
    char m_monthdaycounts[ 2 ][ 13 ];
};

namespace Simulation {

extern simulation_time Time;

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
    void OnKeyUp(int cKey);
    // void UpdateWindow();
    void OnMouseMove(double x, double y);
    void OnCommandGet(DaneRozkaz *pRozkaz);
    bool Update();
    void TrainDelete(TDynamicObject *d = NULL);
    // switches between static and dynamic daylight calculation
    void ToggleDaylight();

private:
    void Update_Environment();
    void Update_Camera( const double Deltatime );
    void Update_UI();
    void ResourceSweep();
    void Render_Cab();

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

	gl_program_light shader; //m7todo: tmp
};

//---------------------------------------------------------------------------

