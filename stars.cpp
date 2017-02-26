
#include "stdafx.h"
#include "stars.h"
#include "globals.h"

//////////////////////////////////////////////////////////////////////////////////////////
// cStars -- simple starfield model, simulating appearance of starry sky

void
cStars::init() {

    m_stars.LoadFromFile( "models\\skydome_stars.t3d", false );
}

void
cStars::render() {

    ::glPushMatrix();

    ::glRotatef( m_latitude, 1.0f, 0.0f, 0.0f ); // ustawienie osi OY na północ
    ::glRotatef( -std::fmod( Global::fTimeAngleDeg, 360.0f ), 0.0f, 1.0f, 0.0f ); // obrót dobowy osi OX

    ::glPointSize( 2.0f );
    m_stars.Render( 1.0 );
    ::glPointSize( 3.0f );

    ::glPopMatrix();
}