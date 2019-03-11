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
#include "openglmatrixstack.h"
#include "renderer.h"
#include "Timer.h"
#include "simulation.h"
#include "Train.h"

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

    return true;
}

void
basic_precipitation::update_weather() {

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
}

void 
basic_precipitation::update() {

    auto const timedelta = Timer::GetDeltaTime();

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

float basic_precipitation::get_textureoffset()
{
	return m_textureoffset;
}

void
basic_precipitation::render() {

    GfxRenderer.Bind_Texture(0,  m_texture);

	if (!m_shader)
	{
        gl::shader vert("precipitation.vert");
		gl::shader frag("precipitation.frag");
		m_shader.emplace(std::vector<std::reference_wrapper<const gl::shader>>({vert, frag}));
	}

	if (!m_vertexbuffer) {
		m_vao.emplace();
		m_vao->bind();

        // build the buffers
		m_vertexbuffer.emplace();
		m_vertexbuffer->allocate(gl::buffer::ARRAY_BUFFER, m_vertices.size() * sizeof( glm::vec3 ), GL_STATIC_DRAW);
		m_vertexbuffer->upload(gl::buffer::ARRAY_BUFFER, m_vertices.data(), 0, m_vertices.size() * sizeof( glm::vec3 ));

		m_vao->setup_attrib(*m_vertexbuffer, 0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

		m_uvbuffer.emplace();
		m_uvbuffer->allocate(gl::buffer::ARRAY_BUFFER, m_uvs.size() * sizeof( glm::vec2 ), GL_STATIC_DRAW);
		m_uvbuffer->upload(gl::buffer::ARRAY_BUFFER, m_uvs.data(), 0, m_uvs.size() * sizeof( glm::vec2 ));

		m_vao->setup_attrib(*m_uvbuffer, 1, 2, GL_FLOAT, sizeof(glm::vec2), 0);

		m_indexbuffer.emplace();
		m_indexbuffer->allocate(gl::buffer::ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof( unsigned short ), GL_STATIC_DRAW);
		m_indexbuffer->upload(gl::buffer::ELEMENT_ARRAY_BUFFER, m_indices.data(), 0, m_indices.size() * sizeof( unsigned short ));
		m_vao->setup_ebo(*m_indexbuffer);

		m_vao->unbind();
        // NOTE: vertex and index source data is superfluous past this point, but, eh
    }

	m_shader->bind();
	m_vao->bind();

    ::glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( m_indices.size() ), GL_UNSIGNED_SHORT, reinterpret_cast<void const*>( 0 ) );

	m_vao->unbind();
}
