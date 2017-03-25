
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
    ::glRotatef( -std::fmod( (float)Global::fTimeAngleDeg, 360.0f ), 0.0f, 1.0f, 0.0f ); // obrót dobowy osi OX

    ::glPointSize( 2.0f );
#ifdef EU07_USE_OLD_RENDERCODE
    if( Global::bUseVBO ) {
        m_stars.RaRender( 1.0, 0 );
    }
    else {
        m_stars.Render( 1.0 );
    }
#else
    GfxRenderer.Render( &m_stars, nullptr, 1.0 );
#endif
    ::glPointSize( 3.0f );

    ::glPopMatrix();
}