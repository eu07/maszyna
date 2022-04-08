/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "openglskydome.h"

#include "simulationenvironment.h"

opengl_skydome::~opengl_skydome() {

    if( m_vertexbuffer != -1 )  { ::glDeleteBuffers( 1, &m_vertexbuffer ); }
    if( m_indexbuffer != -1 )   { ::glDeleteBuffers( 1, &m_indexbuffer ); }
    if( m_coloursbuffer != -1 ) { ::glDeleteBuffers( 1, &m_coloursbuffer ); }
}

void opengl_skydome::update() {

    auto &skydome { simulation::Environment.skydome() };
    // cache entry state
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );
    // setup gpu data buffers:
    // ...static data...
    if( m_indexbuffer == (GLuint)-1 ) {
        ::glGenBuffers( 1, &m_indexbuffer );
        if( ( m_indexbuffer > 0 ) && ( m_indexbuffer != (GLuint)-1 ) ) {
            ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
            auto const &indices { skydome.indices() };
            ::glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( unsigned short ), indices.data(), GL_STATIC_DRAW );
            m_indexcount = indices.size();
        }
    }
    if( m_vertexbuffer == (GLuint)-1 ) {
        ::glGenBuffers( 1, &m_vertexbuffer );
        if( ( m_vertexbuffer > 0 ) && ( m_vertexbuffer != (GLuint)-1 ) ) {
            ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
            auto const &vertices { skydome.vertices() };
            ::glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( glm::vec3 ), vertices.data(), GL_STATIC_DRAW );
        }
    }
    // ...and dynamic data
    if( m_coloursbuffer == (GLuint)-1 ) {
        ::glGenBuffers( 1, &m_coloursbuffer );
        if( ( m_coloursbuffer > 0 ) && ( m_coloursbuffer != (GLuint)-1 ) ) {
            ::glBindBuffer( GL_ARRAY_BUFFER, m_coloursbuffer );
            auto const &colors { skydome.colors() };
            ::glBufferData( GL_ARRAY_BUFFER, colors.size() * sizeof( glm::vec3 ), colors.data(), GL_DYNAMIC_DRAW );
        }
    }
    // ship the current dynamic data to the gpu
    // TODO: ship the data if it changed since the last update
    if( true == skydome.is_dirty() ) {
        if( ( m_coloursbuffer > 0 ) && ( m_coloursbuffer != (GLuint)-1 ) ) {
            ::glBindBuffer( GL_ARRAY_BUFFER, m_coloursbuffer );
            auto &colors{ skydome.colors() };
            /*
            float twilightfactor = clamp( -simulation::Environment.sun().getAngle(), 0.0f, 18.0f ) / 18.0f;
            auto gamma = interpolate( glm::vec3( 0.45f ), glm::vec3( 1.0f ), twilightfactor );
            for( auto & color : colors ) {
                color = glm::pow( color, gamma );
            }
            */
            ::glBufferSubData( GL_ARRAY_BUFFER, 0, colors.size() * sizeof( glm::vec3 ), colors.data() );
            skydome.is_dirty() = false;
        }
    }
    // cleanup
    ::glPopClientAttrib();
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

// render skydome to screen
void opengl_skydome::render() {

    if( ( m_indexbuffer   == (GLuint)-1 )
     || ( m_vertexbuffer  == (GLuint)-1 )
     || ( m_coloursbuffer == (GLuint)-1 ) ) {
        return;
    }
    // setup:
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );
    // ...positions...
    ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
    ::glVertexPointer( 3, GL_FLOAT, sizeof( glm::vec3 ), reinterpret_cast<void const*>( 0 ) );
    ::glEnableClientState( GL_VERTEX_ARRAY );
    // ...colours...
    ::glBindBuffer( GL_ARRAY_BUFFER, m_coloursbuffer );
    ::glColorPointer( 3, GL_FLOAT, sizeof( glm::vec3 ), reinterpret_cast<void const*>( 0 ) );
    ::glEnableClientState( GL_COLOR_ARRAY );
    // ...indices
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
    // draw
    ::glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    ::glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( m_indexcount ), GL_UNSIGNED_SHORT, reinterpret_cast<void const*>( 0 ) );
    // cleanup
    ::glPopClientAttrib();
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}
