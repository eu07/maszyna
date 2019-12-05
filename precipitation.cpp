/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "precipitation.h"

#include "Globals.h"
#include "Timer.h"
#include "simulation.h"
#include "Train.h"

basic_precipitation::~basic_precipitation() {
    // TODO: release allocated resources
}

bool
basic_precipitation::init() {

    auto const heightfactor { 10.f }; // height-to-radius factor
    m_moverate *= heightfactor;

    return true;
}

void 
basic_precipitation::update() {

    auto const timedelta { static_cast<float>( ( DebugModeFlag ? Timer::GetDeltaTime() : Timer::GetDeltaTime() ) ) };

    if( timedelta == 0.0 ) { return; }

    m_textureoffset += m_moverate * m_moverateweathertypefactor * timedelta;
    m_textureoffset = clamp_circular( m_textureoffset, 10.f );

    auto cameramove { glm::dvec3{ Global.pCamera.Pos - m_camerapos} };
    cameramove.y = 0.0; // vertical movement messes up vector calculation

    m_camerapos = Global.pCamera.Pos;

    // intercept sudden user-induced camera jumps...
    // ...from free fly mode change
    if( m_freeflymode != FreeFlyModeFlag ) {
        m_freeflymode = FreeFlyModeFlag;
        if( true == m_freeflymode ) {
            // cache last precipitation vector in the cab
            m_cabcameramove = m_cameramove;
            // don't carry previous precipitation vector to a new unrelated location
            m_cameramove = glm::dvec3{ 0.0 };
        }
        else {
            // restore last cached precipitation vector
            m_cameramove = m_cabcameramove;
        }
        cameramove = glm::dvec3{ 0.0 };
    }
    // ...from jump between cab and window/mirror view
    if( m_windowopen != Global.CabWindowOpen ) {
        m_windowopen = Global.CabWindowOpen;
        cameramove = glm::dvec3{ 0.0 };
    }
    // ... from cab change
    if( ( simulation::Train != nullptr ) && ( simulation::Train->iCabn != m_activecab ) ) {
        m_activecab = simulation::Train->iCabn;
        cameramove = glm::dvec3{ 0.0 };
    }
    // ... from camera jump to another location
    if( glm::length( cameramove ) > 100.0 ) {
        cameramove = glm::dvec3{ 0.0 };
    }

    m_cameramove = m_cameramove * std::max( 0.0, 1.0 - 5.0 * timedelta ) + cameramove * ( 30.0 * timedelta );
    if( std::abs( m_cameramove.x ) < 0.001 ) { m_cameramove.x = 0.0; }
    if( std::abs( m_cameramove.y ) < 0.001 ) { m_cameramove.y = 0.0; }
    if( std::abs( m_cameramove.z ) < 0.001 ) { m_cameramove.z = 0.0; }
}

float
basic_precipitation::get_textureoffset() const {

    return m_textureoffset;
}
