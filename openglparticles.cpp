/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "openglparticles.h"

#include "particles.h"
#include "openglcamera.h"
#include "simulation.h"
#include "Logs.h"

std::vector<std::pair<glm::vec3, glm::vec2>> const billboard_vertices {

    { { -0.5f, -0.5f, 0.f }, { 0.f, 0.f } },
    { {  0.5f, -0.5f, 0.f }, { 1.f, 0.f } },
    { {  0.5f,  0.5f, 0.f }, { 1.f, 1.f } },
    { { -0.5f,  0.5f, 0.f }, { 0.f, 1.f } }
};

void
opengl_particles::update( opengl_camera const &Camera ) {

    m_particlevertices.clear();

    if( false == Global.Smoke ) { return; }

    // build a list of visible smoke sources
    // NOTE: arranged by distance to camera, if we ever need sorting and/or total amount cap-based culling
    std::multimap<float, smoke_source const &> sources;

    for( auto const &source : simulation::Particles.sequence() ) {
        if( false == Camera.visible( source.area() ) ) { continue; }
        // NOTE: the distance is negative when the camera is inside the source's bounding area
        sources.emplace(
            static_cast<float>( glm::length( Camera.position() - source.area().center ) - source.area().radius ),
            source );
    }

    if( true == sources.empty() ) { return; }

    // build billboard data for particles from visible sources
    auto const camerarotation { glm::mat3( Camera.modelview() ) };
    particle_vertex vertex;
    for( auto const &source : sources ) {

        auto const particlecolor {
            glm::clamp(
                source.second.color()
                * ( glm::vec3 { Global.DayLight.ambient } + 0.35f * glm::vec3{ Global.DayLight.diffuse } )
                * 255.f,
                glm::vec3{ 0.f }, glm::vec3{ 255.f } ) };
        auto const &particles { source.second.sequence() };
        // TODO: put sanity cap on the overall amount of particles that can be drawn
        auto const sizestep { 256.0 * billboard_vertices.size() };
        m_particlevertices.reserve(
            sizestep * std::ceil( m_particlevertices.size() + ( particles.size() * billboard_vertices.size() ) / sizestep ) );
        for( auto const &particle : particles ) {
            // TODO: particle color support
            vertex.color[ 0 ] = static_cast<std::uint_fast8_t>( particlecolor.r );
            vertex.color[ 1 ] = static_cast<std::uint_fast8_t>( particlecolor.g );
            vertex.color[ 2 ] = static_cast<std::uint_fast8_t>( particlecolor.b );
            vertex.color[ 3 ] = clamp<std::uint8_t>( particle.opacity * 255, 0, 255 );

            auto const offset { glm::vec3{ particle.position - Camera.position() } };
            auto const rotation { glm::angleAxis( particle.rotation, glm::vec3{ 0.f, 0.f, 1.f } ) };

            for( auto const &billboardvertex : billboard_vertices ) {
                vertex.position = offset + ( rotation * billboardvertex.first * particle.size ) * camerarotation;
                vertex.texture = billboardvertex.second;

                m_particlevertices.emplace_back( vertex );
            }
        }
    }

    // ship the billboard data to the gpu:
    // setup...
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );
    // ...make sure we have enough room...
    if( m_buffercapacity < m_particlevertices.size() ) {
        // allocate gpu side buffer big enough to hold the data
        m_buffercapacity = 0;
        if( m_buffer != (GLuint)-1 ) {
            // get rid of the old buffer
            ::glDeleteBuffers( 1, &m_buffer );
            m_buffer = (GLuint)-1;
        }
        ::glGenBuffers( 1, &m_buffer );
        if( ( m_buffer > 0 ) && ( m_buffer != (GLuint)-1 ) ) {
            // if we didn't get a buffer we'll try again during the next draw call
            // NOTE: we match capacity instead of current size to reduce number of re-allocations
            auto const particlecount { m_particlevertices.capacity() };
            ::glBindBuffer( GL_ARRAY_BUFFER, m_buffer );
            ::glBufferData(
                GL_ARRAY_BUFFER,
                particlecount * sizeof( particle_vertex ),
                nullptr,
                GL_DYNAMIC_DRAW );
            if( ::glGetError() == GL_OUT_OF_MEMORY ) {
                // TBD: throw a bad_alloc?
                ErrorLog( "openGL error: out of memory; failed to create a geometry buffer" );
                ::glDeleteBuffers( 1, &m_buffer );
                m_buffer = (GLuint)-1;
            }
            else {
                m_buffercapacity = particlecount;
            }
        }
    }
    // ...send the data...
    if( ( m_buffer > 0 ) && ( m_buffer != (GLuint)-1 ) ) {
        // if the buffer exists at this point it's guaranteed to be big enough to hold our data
        ::glBindBuffer( GL_ARRAY_BUFFER, m_buffer );
        ::glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            m_particlevertices.size() * sizeof( particle_vertex ),
            m_particlevertices.data() );
    }
    // ...and cleanup
    ::glPopClientAttrib();
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

std::size_t
opengl_particles::render( GLint const Textureunit ) {

    if( false == Global.Smoke ) { return 0; }
    if( m_buffercapacity == 0 ) { return 0; }
    if( m_particlevertices.empty() ) { return 0; }
    if( ( m_buffer == 0 ) || ( m_buffer == (GLuint)-1 ) ) { return 0; }

    // setup...
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );
    ::glBindBuffer( GL_ARRAY_BUFFER, m_buffer );
    ::glVertexPointer( 3, GL_FLOAT, sizeof( particle_vertex ), static_cast<char *>( nullptr ) );
    ::glEnableClientState( GL_VERTEX_ARRAY );
    ::glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( particle_vertex ), reinterpret_cast<void const *>( sizeof( float ) * 3 ) );
    ::glEnableClientState( GL_COLOR_ARRAY );
    ::glClientActiveTexture( GL_TEXTURE0 + Textureunit );
    ::glTexCoordPointer( 2, GL_FLOAT, sizeof( particle_vertex ), reinterpret_cast<void const *>( sizeof( float ) * 3 + sizeof( std::uint8_t ) * 4 ) );
    ::glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    // ...draw...
    ::glDrawArrays( GL_QUADS, 0, m_particlevertices.size() );
    // ...and cleanup
    ::glPopClientAttrib();
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );

    return m_particlevertices.size() / 4;
}

//---------------------------------------------------------------------------
