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

#include "sn_utils.h"

void
basic_vertex::serialize( std::ostream &s ) {

    sn_utils::ls_float32( s, position.x );
    sn_utils::ls_float32( s, position.y );
    sn_utils::ls_float32( s, position.z );

    sn_utils::ls_float32( s, normal.x );
    sn_utils::ls_float32( s, normal.y );
    sn_utils::ls_float32( s, normal.z );

    sn_utils::ls_float32( s, texture.x );
    sn_utils::ls_float32( s, texture.y );
}

void
basic_vertex::deserialize( std::istream &s ) {

    position.x = sn_utils::ld_float32( s );
    position.y = sn_utils::ld_float32( s );
    position.z = sn_utils::ld_float32( s );

    normal.x = sn_utils::ld_float32( s );
    normal.y = sn_utils::ld_float32( s );
    normal.z = sn_utils::ld_float32( s );

    texture.x = sn_utils::ld_float32( s );
    texture.y = sn_utils::ld_float32( s );
}

// generic geometry bank class, allows storage, update and drawing of geometry chunks

// creates a new geometry chunk of specified type from supplied vertex data. returns: handle to the chunk
geometrychunk_handle
geometry_bank::create( vertex_array &Vertices, int const Datatype ) {

    if( true == Vertices.empty() ) { return NULL; }

    m_chunks.emplace_back( Vertices, Datatype );
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    return m_chunks.size();
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
geometry_bank::replace( vertex_array &Vertices, geometrychunk_handle const Chunk, std::size_t const Offset ) {

    if( ( Chunk == 0 ) || ( Chunk > m_chunks.size() ) ) { return false; }

    auto &chunk = m_chunks[ Chunk - 1 ];

    if( ( Offset == 0 )
     && ( Vertices.size() == chunk.vertices.size() ) ) {
        // check first if we can get away with a simple swap...
        chunk.vertices.swap( Vertices );
    }
    else {
        // ...otherwise we need to do some legwork
        // NOTE: if the offset is larger than existing size of the chunk, it'll bridge the gap with 'blank' vertices
        // TBD: we could bail out with an error instead if such request occurs
        chunk.vertices.resize( Offset + Vertices.size(), basic_vertex() );
        chunk.vertices.insert( std::end( chunk.vertices ), std::begin( Vertices ), std::end( Vertices ) );
    }
    return true;
}

vertex_array &
geometry_bank::data( geometrychunk_handle const Chunk ) {

    return m_chunks.at( Chunk - 1 ).vertices;
}

// opengl vbo-based variant of the geometry bank

GLuint opengl_vbogeometrybank::m_activebuffer{ NULL }; // buffer bound currently on the opengl end, if any

// creates a new geometry chunk of specified type from supplied vertex data. returns: handle to the chunk
geometrychunk_handle
opengl_vbogeometrybank::create( vertex_array &Vertices, int const Datatype ) {

    auto const handle = geometry_bank::create( Vertices, Datatype );
    if( handle == NULL ) {
        // if we didn't get anything, bail early
        return handle;
    }
    // adding a chunk means we'll be (re)building the buffer, which will fill the chunk records, amongst other things
    // so we don't need to initialize the values here
    m_chunkrecords.emplace_back( chunk_record() );
    // kiss the existing buffer goodbye, new overall data size means we'll be making a new one
    delete_buffer();

    return handle;
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
opengl_vbogeometrybank::replace( vertex_array &Vertices, geometrychunk_handle const Chunk, std::size_t const Offset ) {

    auto const result = geometry_bank::replace( Vertices, Chunk, Offset );
    if( false == result ) {
        // if nothing happened we can bail out early
        return result;
    }
    auto &chunkrecord = m_chunkrecords[ Chunk - 1 ];
    chunkrecord.is_good = false;
    // if the overall length of the chunk didn't change we can get away with reusing the old buffer...
    if( m_chunks[ Chunk - 1 ].vertices.size() != chunkrecord.size ) {
        // ...but otherwise we'll need to allocate a new one
        // TBD: we could keep and reuse the old buffer also if the new chunk is smaller than the old one,
        // but it'd require some extra tracking and work to keep all chunks up to date; also wasting vram; may be not worth it?
        delete_buffer();
    }
    return result;
}

// draws geometry stored in specified chunk
void
opengl_vbogeometrybank::draw( geometrychunk_handle const Chunk ) {

    if( m_buffer == NULL ) {
        // if there's no buffer, we'll have to make one
        // NOTE: this isn't exactly optimal in terms of ensuring the gfx card doesn't stall waiting for the data
        // may be better to initiate upload earlier (during update phase) and trust this effort won't go to waste
        if( true == m_chunks.empty() ) { return; }

        std::size_t datasize{ 0 };
        auto &chunkrecord = m_chunkrecords.begin();
        for( auto &chunk : m_chunks ) {
            // fill all chunk records, based on the chunk data
            chunkrecord->offset = datasize;
            chunkrecord->size = chunk.vertices.size();
            datasize += chunkrecord->size;
            ++chunkrecord;
        }
        // the odds for all created chunks to get replaced with empty ones are quite low, but the possibility does exist
        if( datasize == 0 ) { return; }
        // try to set up the buffer we need
        ::glGenBuffers( 1, &m_buffer );
        bind_buffer();
        // NOTE: we're using static_draw since it's generally true for all we have implemented at the moment
        // TODO: allow to specify usage hint at the object creation, and pass it here
        ::glBufferData(
            GL_ARRAY_BUFFER,
            datasize * sizeof( basic_vertex ),
            nullptr,
            GL_STATIC_DRAW );
        m_buffercapacity = datasize;
    }
    // actual draw procedure starts here
    // setup...
    if( m_activebuffer != m_buffer ) {
        bind_buffer();
    }
    auto &chunkrecord = m_chunkrecords[ Chunk - 1 ];
    auto const &chunk = m_chunks[ Chunk - 1 ];
    if( false == chunkrecord.is_good ) {
        // we may potentially need to upload new buffer data before we can draw it
        ::glBufferSubData(
            GL_ARRAY_BUFFER,
            chunkrecord.offset * sizeof( basic_vertex ),
            chunkrecord.size * sizeof( basic_vertex ),
            chunk.vertices.data() );
        chunkrecord.is_good = true;
    }
    // ...render...
    ::glDrawArrays( chunk.type, chunkrecord.offset, chunkrecord.size );
    // ...post-render cleanup
/*
    ::glDisableClientState( GL_VERTEX_ARRAY );
    ::glDisableClientState( GL_NORMAL_ARRAY );
    ::glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 ); m_activebuffer = 0;
*/
}

void
opengl_vbogeometrybank::bind_buffer() {

    ::glBindBuffer( GL_ARRAY_BUFFER, m_buffer );
    // TODO: allow specifying other vertex data setups
    ::glVertexPointer( 3, GL_FLOAT, sizeof( basic_vertex ), static_cast<char *>( nullptr ) );
    ::glNormalPointer( GL_FLOAT, sizeof( basic_vertex ), static_cast<char *>( nullptr ) + sizeof(float) * 3 ); // normalne
    ::glTexCoordPointer( 2, GL_FLOAT, sizeof( basic_vertex ), static_cast<char *>( nullptr ) + 24 ); // wierzchołki
    // TODO: allow specifying other vertex data setups, either in the draw() parameters or during chunk or buffer creation
    ::glEnableClientState( GL_VERTEX_ARRAY );
    ::glEnableClientState( GL_NORMAL_ARRAY );
    ::glEnableClientState( GL_TEXTURE_COORD_ARRAY );

    m_activebuffer = m_buffer;
}

void
opengl_vbogeometrybank::delete_buffer() {

    if( m_buffer != NULL ) {
        
        ::glDeleteBuffers( 1, &m_buffer );
        if( m_activebuffer == m_buffer ) {
            m_activebuffer = NULL;
        }
        m_buffer = NULL;
        m_buffercapacity = 0;
    }
}
