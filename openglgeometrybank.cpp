/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "openglgeometrybank.h"
#include "openglcolor.h"

#include "Logs.h"

namespace gfx {

// opengl vbo-based variant of the geometry bank

GLuint opengl_vbogeometrybank::m_activevertexbuffer { 0 }; // buffer bound currently on the opengl end, if any
unsigned int opengl_vbogeometrybank::m_activestreams { gfx::stream::none }; // currently enabled data type pointers
std::vector<GLint> opengl_vbogeometrybank::m_activetexturearrays; // currently enabled texture coord arrays

// create() subclass details
void
opengl_vbogeometrybank::create_( gfx::geometry_handle const &Geometry ) {
    // adding a chunk means we'll be (re)building the buffer, which will fill the chunk records, amongst other things.
    // thus we don't need to initialize the values here
    m_chunkrecords.emplace_back( chunk_record() );
    // kiss the existing buffer goodbye, new overall data size means we'll be making a new one
    delete_buffer();
}

// replace() subclass details
void
opengl_vbogeometrybank::replace_( gfx::geometry_handle const &Geometry ) {

    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    chunkrecord.is_good = false;
    // if the overall length of the chunk didn't change we can get away with reusing the old buffer...
    if( geometry_bank::chunk( Geometry ).vertices.size() != chunkrecord.vertex_count ) {
        // ...but otherwise we'll need to allocate a new one
        // TBD: we could keep and reuse the old buffer also if the new chunk is smaller than the old one,
        // but it'd require some extra tracking and work to keep all chunks up to date; also wasting vram; may be not worth it?
        delete_buffer();
    }
}

// draw() subclass details
std::size_t
opengl_vbogeometrybank::draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) {

    setup_buffer();

    auto &chunkrecord { m_chunkrecords[ Geometry.chunk - 1 ] };
    // sanity check; shouldn't be needed but, eh
    if( chunkrecord.vertex_count == 0 ) { return 0; }
    // setup...
    if( m_activevertexbuffer != m_vertexbuffer ) {
        bind_buffer();
    }
    auto const &chunk = gfx::geometry_bank::chunk( Geometry );
    if( false == chunkrecord.is_good ) {
        // we may potentially need to upload new buffer data before we can draw it
        if( chunkrecord.index_count > 0 ) {
            ::glBufferSubData(
                GL_ELEMENT_ARRAY_BUFFER,
                chunkrecord.index_offset * sizeof( gfx::basic_index ),
                chunkrecord.index_count * sizeof( gfx::basic_index ),
                chunk.indices.data() );
        }
        ::glBufferSubData(
            GL_ARRAY_BUFFER,
            chunkrecord.vertex_offset * sizeof( gfx::basic_vertex ),
            chunkrecord.vertex_count * sizeof( gfx::basic_vertex ),
            chunk.vertices.data() );
        chunkrecord.is_good = true;
    }
    // ...render...
    if( chunkrecord.index_count > 0 ) {
/*
        ::glDrawElementsBaseVertex(
            chunk.type,
            chunkrecord.index_count, GL_UNSIGNED_INT, reinterpret_cast<void const *>( chunkrecord.index_offset * sizeof( gfx::basic_index ) ),
            chunkrecord.vertex_offset );
*/
        if (glDrawRangeElementsBaseVertex) {
            if( m_activestreams != Streams ) {
                bind_streams( Units, Streams );
            }
            ::glDrawRangeElementsBaseVertex(
                chunk.type,
                0, chunkrecord.vertex_count,
                chunkrecord.index_count, GL_UNSIGNED_INT, reinterpret_cast<void const *>( chunkrecord.index_offset * sizeof( gfx::basic_index ) ),
                chunkrecord.vertex_offset );
        }
        else {
            bind_streams( Units, Streams, chunkrecord.vertex_offset );
            ::glDrawRangeElements(
                chunk.type,
                0, chunkrecord.vertex_count,
                chunkrecord.index_count, GL_UNSIGNED_INT, reinterpret_cast<void const *>( chunkrecord.index_offset * sizeof( gfx::basic_index ) ) );
        }
    }
    else {
        if( m_activestreams != Streams ) {
            bind_streams( Units, Streams );
        }
        ::glDrawArrays( chunk.type, chunkrecord.vertex_offset, chunkrecord.vertex_count );
    }
    // ...post-render cleanup
/*
    ::glDisableClientState( GL_VERTEX_ARRAY );
    ::glDisableClientState( GL_NORMAL_ARRAY );
    ::glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 ); m_activebuffer = 0;
*/
    auto const vertexcount { ( chunkrecord.index_count > 0 ? chunkrecord.index_count : chunkrecord.vertex_count ) };
    switch( chunk.type ) {
        case GL_TRIANGLES:      { return vertexcount / 3; }
        case GL_TRIANGLE_STRIP: { return vertexcount - 2; }
        default:                { return 0; }
    }
}

// release () subclass details
void
opengl_vbogeometrybank::release_() {

    delete_buffer();
}

void
opengl_vbogeometrybank::setup_buffer() {

    if( m_vertexbuffer != 0 ) { return; }
    // if there's no buffer, we'll have to make one
    // NOTE: this isn't exactly optimal in terms of ensuring the gfx card doesn't stall waiting for the data
    // may be better to initiate upload earlier (during update phase) and trust this effort won't go to waste
    if( true == m_chunks.empty() ) { return; }

    std::size_t
        vertexcount{ 0 },
        indexcount{ 0 };
    auto chunkiterator = m_chunks.cbegin();
    for( auto &chunkrecord : m_chunkrecords ) {
        // fill records for all chunks, based on the chunk data
        chunkrecord.is_good = false; // if we're re-creating buffer, chunks might've been uploaded in the old one
        chunkrecord.vertex_offset = vertexcount;
        chunkrecord.vertex_count = chunkiterator->vertices.size();
        vertexcount += chunkrecord.vertex_count;
        chunkrecord.index_offset = indexcount;
        chunkrecord.index_count = chunkiterator->indices.size();
        indexcount += chunkrecord.index_count;
        ++chunkiterator;
    }
    // the odds for all created chunks to get replaced with empty ones are quite low, but the possibility does exist
    if( vertexcount == 0 ) { return; }
    // try to set up the buffers we need
    if( ( indexcount > 0 ) && ( m_indexbuffer == 0 ) ) {
        ::glGenBuffers( 1, &m_indexbuffer );
        if( m_indexbuffer == 0 ) { return; } // if we didn't get a buffer we'll try again during the next draw call
    }
    if( m_vertexbuffer == 0 ) {
        ::glGenBuffers( 1, &m_vertexbuffer );
        if( m_vertexbuffer == 0 ) { return; } // if we didn't get a buffer we'll try again during the next draw call
    }
    bind_buffer();
    // NOTE: we're using static_draw since it's generally true for all we have implemented at the moment
    // TODO: allow to specify usage hint at the object creation, and pass it here
    if( indexcount > 0 ) {
        ::glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indexcount * sizeof( gfx::basic_index ),
            nullptr,
            GL_STATIC_DRAW );
        if( ::glGetError() == GL_OUT_OF_MEMORY ) {
            // TBD: throw a bad_alloc?
            ErrorLog( "openGL error: out of memory; failed to create a geometry index buffer" );
            delete_buffer();
            return;
        }
    }
    ::glBufferData(
        GL_ARRAY_BUFFER,
        vertexcount * sizeof( gfx::basic_vertex ),
        nullptr,
        GL_STATIC_DRAW );
    if( ::glGetError() == GL_OUT_OF_MEMORY ) {
        // TBD: throw a bad_alloc?
        ErrorLog( "openGL error: out of memory; failed to create a geometry buffer" );
        delete_buffer();
        return;
    }
}

void
opengl_vbogeometrybank::bind_buffer() {

    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
    ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
    m_activevertexbuffer = m_vertexbuffer;
    m_activestreams = gfx::stream::none;
}

void
opengl_vbogeometrybank::delete_buffer() {

    if( m_indexbuffer != 0 ) {
        ::glDeleteBuffers( 1, &m_indexbuffer );
        m_indexbuffer = 0;
    }
    if( m_vertexbuffer != 0 ) {
        
        ::glDeleteBuffers( 1, &m_vertexbuffer );
        if( m_activevertexbuffer == m_vertexbuffer ) {
            m_activevertexbuffer = 0;
            release_streams();
        }
        m_vertexbuffer = 0;
        // NOTE: since we've deleted the buffer all chunks it held were rendered invalid as well
        // instead of clearing their state here we're delaying it until new buffer is created to avoid looping through chunk records twice
    }
}

void
opengl_vbogeometrybank::bind_streams( gfx::stream_units const &Units, unsigned int const Streams, size_t offset ) {

    if( Streams & gfx::stream::position ) {
        ::glVertexPointer( 3, GL_FLOAT, sizeof( gfx::basic_vertex ), reinterpret_cast<void const *>( sizeof( gfx::basic_vertex ) * offset ) );
        ::glEnableClientState( GL_VERTEX_ARRAY );
    }
    else {
        ::glDisableClientState( GL_VERTEX_ARRAY );
    }
    // NOTE: normal and color streams share the data, making them effectively mutually exclusive
    if( Streams & gfx::stream::normal ) {
        ::glNormalPointer( GL_FLOAT, sizeof( gfx::basic_vertex ), reinterpret_cast<void const *>( 12 + sizeof( gfx::basic_vertex ) * offset ) );
        ::glEnableClientState( GL_NORMAL_ARRAY );
    }
    else {
        ::glDisableClientState( GL_NORMAL_ARRAY );
    }
    if( Streams & gfx::stream::color ) {
        ::glColorPointer( 3, GL_FLOAT, sizeof( gfx::basic_vertex ), reinterpret_cast<void const *>( 12 + sizeof( gfx::basic_vertex ) * offset ) );
        ::glEnableClientState( GL_COLOR_ARRAY );
    }
    else {
        ::glDisableClientState( GL_COLOR_ARRAY );
    }
    if( Streams & gfx::stream::texture ) {
        for( auto unit : Units.texture ) {
            ::glClientActiveTexture( GL_TEXTURE0 + unit );
            ::glTexCoordPointer( 2, GL_FLOAT, sizeof( gfx::basic_vertex ), reinterpret_cast<void const *>( 24 + sizeof( gfx::basic_vertex ) * offset ) );
            ::glEnableClientState( GL_TEXTURE_COORD_ARRAY );
        }
        m_activetexturearrays = Units.texture;
    }
    else {
        for( auto unit : Units.texture ) {
            ::glClientActiveTexture( GL_TEXTURE0 + unit );
            ::glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        }
        m_activetexturearrays.clear(); // NOTE: we're simplifying here, since we always toggle the same texture coord sets
    }

    m_activestreams = Streams;
}

void
opengl_vbogeometrybank::release_streams() {

    ::glDisableClientState( GL_VERTEX_ARRAY );
    ::glDisableClientState( GL_NORMAL_ARRAY );
    ::glDisableClientState( GL_COLOR_ARRAY );
    for( auto unit : m_activetexturearrays ) {
        ::glClientActiveTexture( GL_TEXTURE0 + unit );
        ::glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    }

    m_activestreams = gfx::stream::none;
    m_activetexturearrays.clear();
}

// opengl display list based variant of the geometry bank

// create() subclass details
void
opengl_dlgeometrybank::create_( gfx::geometry_handle const &Geometry ) {

    m_chunkrecords.emplace_back( chunk_record() );
}

// replace() subclass details
void
opengl_dlgeometrybank::replace_( gfx::geometry_handle const &Geometry ) {

    delete_list( Geometry );
}

// draw() subclass details
std::size_t
opengl_dlgeometrybank::draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) {

    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    if( chunkrecord.streams != Streams ) {
        delete_list( Geometry );
        chunkrecord.primitive_count = 0;
    }
    if( chunkrecord.list == 0 ) {
        // we don't have a list ready, so compile one
        chunkrecord.streams = Streams;
        chunkrecord.list = ::glGenLists( 1 );
        auto const &chunk = gfx::geometry_bank::chunk( Geometry );
        ::glNewList( chunkrecord.list, GL_COMPILE );

        ::glBegin( chunk.type );
        if( chunk.indices.size() > 0 ) {
            // indexed geometry
            for( auto const &index : chunk.indices ) {
                auto const &vertex { chunk.vertices[ index ] };
                     if( Streams & gfx::stream::normal ) { ::glNormal3fv( glm::value_ptr( vertex.normal ) ); }
                else if( Streams & gfx::stream::color ) { ::glColor3fv( glm::value_ptr( vertex.normal ) ); }
                if( Streams & gfx::stream::texture ) { for( auto unit : Units.texture ) { ::glMultiTexCoord2fv( unit, glm::value_ptr( vertex.texture ) ); } }
                if( Streams & gfx::stream::position ) { ::glVertex3fv( glm::value_ptr( vertex.position ) ); }
            }
        }
        else {
            // raw geometry
            for( auto const &vertex : chunk.vertices ) {
                     if( Streams & gfx::stream::normal ) { ::glNormal3fv( glm::value_ptr( vertex.normal ) ); }
                else if( Streams & gfx::stream::color ) { ::glColor3fv( glm::value_ptr( vertex.normal ) ); }
                if( Streams & gfx::stream::texture ) { for( auto unit : Units.texture ) { ::glMultiTexCoord2fv( unit, glm::value_ptr( vertex.texture ) ); } }
                if( Streams & gfx::stream::position ) { ::glVertex3fv( glm::value_ptr( vertex.position ) ); }
            }
        }
        ::glEnd();
        ::glEndList();

        auto const vertexcount { ( chunk.indices.empty() ? chunk.vertices.size() : chunk.indices.size() ) };
        switch( chunk.type ) {
            case GL_TRIANGLES:      { chunkrecord.primitive_count += vertexcount / 3; break; }
            case GL_TRIANGLE_STRIP: { chunkrecord.primitive_count += vertexcount - 2; break; }
            default:                { break; }
        }
    }
    // with the list done we can just play it
    ::glCallList( chunkrecord.list );
    return chunkrecord.primitive_count;
}

// release () subclass details
void
opengl_dlgeometrybank::release_() {

    for( auto &chunkrecord : m_chunkrecords ) {
        if( chunkrecord.list != 0 ) {
            ::glDeleteLists( chunkrecord.list, 1 );
            chunkrecord.list = 0;
        }
        chunkrecord.streams = gfx::stream::none;
    }
}

void
opengl_dlgeometrybank::delete_list( gfx::geometry_handle const &Geometry ) {
    // NOTE: given it's our own internal method we trust it to be called with valid parameters
    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    if( chunkrecord.list != 0 ) {
        ::glDeleteLists( chunkrecord.list, 1 );
        chunkrecord.list = 0;
    }
    chunkrecord.streams = gfx::stream::none;
}

} // namespace gfx
