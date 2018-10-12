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

#include "globals.h"
#include "openglmatrixstack.h"
#include "renderer.h"
#include "timer.h"
#include "simulation.h"
#include "train.h"

basic_precipitation::~basic_precipitation() {
    // TODO: release allocated resources
}

void
basic_precipitation::create( int const Tesselation ) {
	
    auto const heightfactor { 10.f }; // height-to-radius factor
    m_moverate *= heightfactor;
    auto const verticaltexturestretchfactor { 1.5f }; // crude motion blur

	// create geometry chunk
    auto const latitudes { 3 }; // just a cylinder with end cones
    auto const longitudes { Tesselation };
    auto const longitudehalfstep { 0.5f * static_cast<float>( 2.0 * M_PI * 1.f / longitudes ) }; // for crude uv correction

    std::uint16_t index = 0;

//    auto const radius { 25.f }; // cylinder radius
    std::vector<float> radii { 25.f, 10.f, 5.f, 1.f };
    for( auto radius : radii ) {

        for( int i = 0; i <= latitudes; ++i ) {

            auto const latitude{ static_cast<float>( M_PI * ( -0.5f + (float)( i ) / latitudes ) ) };
            auto const z{ std::sin( latitude ) };
            auto const zr{ std::cos( latitude ) };

            for( int j = 0; j <= longitudes; ++j ) {
                // NOTE: for the first and last row half of the points we create end up unused but, eh
                auto const longitude{ static_cast<float>( 2.0 * M_PI * (float)( j ) / longitudes ) };
                auto const x{ std::cos( longitude ) };
                auto const y{ std::sin( longitude ) };
                // NOTE: cartesian to opengl swap would be: -x, -z, -y
                m_vertices.emplace_back( glm::vec3( -x * zr, -z * heightfactor, -y * zr ) * radius );
                // uvs
                // NOTE: first and last row receives modified u values to deal with limitation of mapping onto triangles
                auto u = (
                    i == 0 ? longitude + longitudehalfstep :
                    i == latitudes ? longitude - longitudehalfstep :
                    longitude );
                m_uvs.emplace_back(
                    u / ( 2.0 * M_PI ) * radius,
                    1.f - (float)( i ) / latitudes * radius * heightfactor * 0.5f / verticaltexturestretchfactor );

                if( ( i == 0 ) || ( j == 0 ) ) {
                    // initial edge of the dome, don't start indices yet
                    ++index;
                }
                else {
                    // the end cones are built from one triangle of each quad, the middle rows use both
                    if( i < latitudes ) {
                        m_indices.emplace_back( index - 1 - ( longitudes + 1 ) );
                        m_indices.emplace_back( index - 1 );
                        m_indices.emplace_back( index );
                    }
                    if( i > 1 ) {
                        m_indices.emplace_back( index );
                        m_indices.emplace_back( index - ( longitudes + 1 ) );
                        m_indices.emplace_back( index - 1 - ( longitudes + 1 ) );
                    }
                    ++index;
                }
            } // longitude
        } // latitude
    } // radius
}

bool
basic_precipitation::init() {

    create( 18 );

    // TODO: select texture based on current overcast level
    // TODO: when the overcast level dynamic change is in check the current level during render and pick the appropriate texture on the fly
    std::string const densitysuffix { (
        Global.Overcast < 1.35 ?
            "_light" :
            "_medium" ) };
    if( Global.Weather == "rain:" ) {
        m_moverateweathertypefactor = 2.f;
        m_texture = GfxRenderer.Fetch_Texture( "fx/rain" + densitysuffix );
    }
    else if( Global.Weather == "snow:" ) {
        m_moverateweathertypefactor = 1.25f;
        m_texture = GfxRenderer.Fetch_Texture( "fx/snow" + densitysuffix );
    }

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

    // intercept sudden user-induced camera jumps
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
    if( m_windowopen != Global.CabWindowOpen ) {
        m_windowopen = Global.CabWindowOpen;
        cameramove = glm::dvec3{ 0.0 };
    }
    if( ( simulation::Train != nullptr ) && ( simulation::Train->iCabn != m_activecab ) ) {
        m_activecab = simulation::Train->iCabn;
        cameramove = glm::dvec3{ 0.0 };
    }
    if( glm::length( cameramove ) > 100.0 ) {
        cameramove = glm::dvec3{ 0.0 };
    }

    m_cameramove = m_cameramove * std::max( 0.0, 1.0 - 5.0 * timedelta ) + cameramove * ( 30.0 * timedelta );
    if( std::abs( m_cameramove.x ) < 0.001 ) { m_cameramove.x = 0.0; }
    if( std::abs( m_cameramove.y ) < 0.001 ) { m_cameramove.y = 0.0; }
    if( std::abs( m_cameramove.z ) < 0.001 ) { m_cameramove.z = 0.0; }
}

void
basic_precipitation::render() {

    GfxRenderer.Bind_Texture( m_texture );

    // cache entry state
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

    if( m_vertexbuffer == -1 ) {
        // build the buffers
        ::glGenBuffers( 1, &m_vertexbuffer );
        ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
        ::glBufferData( GL_ARRAY_BUFFER, m_vertices.size() * sizeof( glm::vec3 ), m_vertices.data(), GL_STATIC_DRAW );

        ::glGenBuffers( 1, &m_uvbuffer );
        ::glBindBuffer( GL_ARRAY_BUFFER, m_uvbuffer );
        ::glBufferData( GL_ARRAY_BUFFER, m_uvs.size() * sizeof( glm::vec2 ), m_uvs.data(), GL_STATIC_DRAW );

        ::glGenBuffers( 1, &m_indexbuffer );
        ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
        ::glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof( unsigned short ), m_indices.data(), GL_STATIC_DRAW );
        // NOTE: vertex and index source data is superfluous past this point, but, eh
    }
    // positions
    ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
    ::glVertexPointer( 3, GL_FLOAT, sizeof( glm::vec3 ), reinterpret_cast<void const*>( 0 ) );
    ::glEnableClientState( GL_VERTEX_ARRAY );
    // uvs
    ::glBindBuffer( GL_ARRAY_BUFFER, m_uvbuffer );
    ::glClientActiveTexture( m_textureunit );
    ::glTexCoordPointer( 2, GL_FLOAT, sizeof( glm::vec2 ), reinterpret_cast<void const*>( 0 ) );
    ::glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    // uv transformation matrix
    ::glMatrixMode( GL_TEXTURE );
    ::glLoadIdentity();
    ::glTranslatef( 0.f, m_textureoffset, 0.f );
    // indices
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
    ::glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( m_indices.size() ), GL_UNSIGNED_SHORT, reinterpret_cast<void const*>( 0 ) );
    // cleanup
    ::glLoadIdentity();
    ::glMatrixMode( GL_MODELVIEW );
    ::glPopClientAttrib();
}
