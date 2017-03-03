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
#include "glfw/glfw3.h"

// wrapper for environment elements -- sky, sun, stars, clouds etc
class world_environment {

public:
    void init();
    void update();
    void render();

private:
    CSkyDome m_skydome;
    cStars m_stars;
    cSun m_sun;
    TSky m_clouds;
};

class TWorld
{
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
    bool Render();
    void Render_Cab();
    void Render_UI();
    TCamera Camera;
    TGround Ground;
    world_environment Environment;
    TTrain *Train;
    TDynamicObject *pDynamicNearest;
    bool Paused{ true };
    GLuint base; // numer DL dla znaków w napisach
    texture_manager::size_type light; // numer tekstury dla smugi
    TEvent *KeyEvents[10]; // eventy wyzwalane z klawiaury
    TMoverParameters *mvControlled; // wskaźnik na człon silnikowy, do wyświetlania jego parametrów
    int iCheckFPS; // kiedy znów sprawdzić FPS, żeby wyłączać optymalizacji od razu do zera
    double fTime50Hz; // bufor czasu dla komunikacji z PoKeys
    double fTimeBuffer; // bufor czasu aktualizacji dla stałego kroku fizyki
    double fMaxDt; //[s] krok czasowy fizyki (0.01 dla normalnych warunków)
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

