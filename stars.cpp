
#include "stdafx.h"
#include "stars.h"
#include "globals.h"
#include "MdlMngr.h"

//////////////////////////////////////////////////////////////////////////////////////////
// cStars -- simple starfield model, simulating appearance of starry sky

void
cStars::init() {

    m_stars = TModelsManager::GetModel( "models\\skydome_stars.t3d", false );
}
#ifdef EU07_USE_OLD_RENDERCODE
void
cStars::render() {
    // setup
    ::glPushMatrix();

    ::glRotatef( m_latitude, 1.0f, 0.0f, 0.0f ); // ustawienie osi OY na północ
    ::glRotatef( -std::fmod( (float)Global::fTimeAngleDeg, 360.0f ), 0.0f, 1.0f, 0.0f ); // obrót dobowy osi OX

    ::glPointSize( 2.0f );
    // render
    GfxRenderer.Render( &m_stars, nullptr, 1.0 );
    // post-render cleanup
    ::glPointSize( 3.0f );

    ::glPopMatrix();
}
#endif
