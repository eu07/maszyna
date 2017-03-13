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
#include "stars.h"
#include "skydome.h"
#include "mczapkie/mover.h"
#include "renderer.h"

// wrapper for environment elements -- sky, sun, stars, clouds etc
class world_environment {

public:
    void init();
    void update();
    void render();
    void time( int const Hour = -1, int const Minute = -1, int const Second = -1 );

private:
    CSkyDome m_skydome;
    cStars m_stars;
    cSun m_sun;
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
    bool Init( GLFWwindow *w );
    bool InitPerformed() { return m_init; }
    GLFWwindow *window;
    GLvoid glPrint(std::string const &Text);
    void OnKeyDown(int cKey);
    void OnKeyUp(int cKey);
    // void UpdateWindow();
    void OnMouseMove(double x, double y);
    void OnCommandGet(DaneRozkaz *pRozkaz);
    bool Update();
    void TrainDelete(TDynamicObject *d = NULL);
    // switches between static and dynamic daylight calculation
    void ToggleDaylight();
    TWorld();
    ~TWorld();
    // double Aspect;
  private:
    std::string OutText1; // teksty na ekranie
    std::string OutText2;
    std::string OutText3;
    std::string OutText4;
    void Update_Environment();
    void Update_Camera( const double Deltatime );
    void ResourceSweep();
    void Render_Cab();
    void Render_UI();
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
    double m_primaryupdateaccumulator{ 0.0 }; // keeps track of elapsed simulation time, for core fixed step routines
    double m_secondaryupdaterate{ 1.0 / 50.0 };
    double m_secondaryupdateaccumulator{ 0.0 }; // keeps track of elapsed simulation time, for less important fixed step routines
    int iPause; // wykrywanie zmian w zapauzowaniu
    double VelPrev; // poprzednia prędkość
    int tprev; // poprzedni czas
    double Acc; // przyspieszenie styczne
    bool m_init{ false }; // indicates whether initial update of the world was performed
  public:
    void ModifyTGA(std::string const &dir = "");
    void CreateE3D(std::string const &dir = "", bool dyn = false);
    void CabChange(TDynamicObject *old, TDynamicObject *now);
};

//---------------------------------------------------------------------------

