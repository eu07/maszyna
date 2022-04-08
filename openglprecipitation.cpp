/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "openglprecipitation.h"

#include "Globals.h"
#include "renderer.h"
#include "simulationenvironment.h"

opengl_precipitation::~opengl_precipitation() {

    if( m_vertexbuffer != -1 ) { ::glDeleteBuffers( 1, &m_vertexbuffer ); }
    if( m_indexbuffer  != -1 ) { ::glDeleteBuffers( 1, &m_indexbuffer ); }
    if( m_uvbuffer     != -1 ) { ::glDeleteBuffers( 1, &m_uvbuffer ); }
}

void
opengl_precipitation::create( int const Tesselation ) {

    m_vertices.clear();
    m_uvs.clear();
    m_indices.clear();

    auto const heightfactor { 10.f }; // height-to-radius factor
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

void 
opengl_precipitation::update() {

    // NOTE: we should really be checking state of each buffer as theoretically allocation could go wrong mid-way, but, eh
    if( m_vertexbuffer == (GLuint)-1 ) {

        if( m_vertices.empty() ) {
            // create visualization mesh
            create( 18 );
        }
        // cache entry state
        ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

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
 
        // cleanup
        ::glPopClientAttrib();
        ::glBindBuffer( GL_ARRAY_BUFFER, 0 );
        ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }

    // TODO: include weather type check in the entry conditions
    if( m_overcast == Global.Overcast ) { return; }

    m_overcast = Global.Overcast;

    std::string const densitysuffix { (
        m_overcast < 1.35 ?
            "_light" :
            "_medium" ) };
    if( Global.Weather == "rain:" ) {
        m_texture = GfxRenderer->Fetch_Texture( "fx/rain" + densitysuffix );
    }
    else if( Global.Weather == "snow:" ) {
        m_texture = GfxRenderer->Fetch_Texture( "fx/snow" + densitysuffix );
    }
}

void
opengl_precipitation::render( GLint const Textureunit ) {

    if( m_texture == null_handle ) { return; }

    GfxRenderer->Bind_Texture( m_texture );

    // cache entry state
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

    // positions
    ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
    ::glVertexPointer( 3, GL_FLOAT, sizeof( glm::vec3 ), reinterpret_cast<void const*>( 0 ) );
    ::glEnableClientState( GL_VERTEX_ARRAY );
    // uvs
    ::glBindBuffer( GL_ARRAY_BUFFER, m_uvbuffer );
    ::glClientActiveTexture( GL_TEXTURE0 + Textureunit );
    ::glTexCoordPointer( 2, GL_FLOAT, sizeof( glm::vec2 ), reinterpret_cast<void const*>( 0 ) );
    ::glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    // uv transformation matrix
    ::glMatrixMode( GL_TEXTURE );
    ::glLoadIdentity();
    ::glTranslatef( 0.f,  simulation::Environment.precipitation().get_textureoffset(), 0.f );
    // indices
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
    ::glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( m_indices.size() ), GL_UNSIGNED_SHORT, reinterpret_cast<void const*>( 0 ) );
    // cleanup
    ::glLoadIdentity();
    ::glMatrixMode( GL_MODELVIEW );
    ::glPopClientAttrib();
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}
