﻿/*
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
#include "logs.h"
#include "globals.h"

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
geometry_handle
geometry_bank::create( vertex_array &Vertices, unsigned int const Type, unsigned int const Streams ) {

    if( true == Vertices.empty() ) { return geometry_handle( 0, 0 ); }

    m_chunks.emplace_back( Vertices, Type, Streams );
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    geometry_handle chunkhandle { 0, static_cast<std::uint32_t>(m_chunks.size()) };
    // template method
    create_( chunkhandle );
    // all done
    return chunkhandle;
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
geometry_bank::replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset ) {

    if( ( Geometry.chunk == 0 ) || ( Geometry.chunk > m_chunks.size() ) ) { return false; }

    auto &chunk = geometry_bank::chunk( Geometry );

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
    // template method
    replace_( Geometry );
    // all done
    return true;
}

// adds supplied vertex data at the end of specified chunk
bool
geometry_bank::append( vertex_array &Vertices, geometry_handle const &Geometry ) {

    if( ( Geometry.chunk == 0 ) || ( Geometry.chunk > m_chunks.size() ) ) { return false; }

    return replace( Vertices, Geometry, geometry_bank::chunk( Geometry ).vertices.size() );
}

// draws geometry stored in specified chunk
void
geometry_bank::draw( geometry_handle const &Geometry ) {
    // template method
    draw_( Geometry );
}

vertex_array const &
geometry_bank::vertices( geometry_handle const &Geometry ) const {

    return geometry_bank::chunk( Geometry ).vertices;
}

// opengl vbo-based variant of the geometry bank

GLuint opengl_vbogeometrybank::m_activebuffer{ NULL }; // buffer bound currently on the opengl end, if any

// create() subclass details
void
opengl_vbogeometrybank::create_( geometry_handle const &Geometry ) {
    // adding a chunk means we'll be (re)building the buffer, which will fill the chunk records, amongst other things.
    // thus we don't need to initialize the values here
    m_chunkrecords.emplace_back( chunk_record() );
    // kiss the existing buffer goodbye, new overall data size means we'll be making a new one
    delete_buffer();
}

// replace() subclass details
void
opengl_vbogeometrybank::replace_( geometry_handle const &Geometry ) {

    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    chunkrecord.is_good = false;
    // if the overall length of the chunk didn't change we can get away with reusing the old buffer...
    if( geometry_bank::chunk( Geometry ).vertices.size() != chunkrecord.size ) {
        // ...but otherwise we'll need to allocate a new one
        // TBD: we could keep and reuse the old buffer also if the new chunk is smaller than the old one,
        // but it'd require some extra tracking and work to keep all chunks up to date; also wasting vram; may be not worth it?
        delete_buffer();
    }
}

// draw() subclass details
void
opengl_vbogeometrybank::draw_( geometry_handle const &Geometry ) {

    if( m_buffer == NULL ) {
        // if there's no buffer, we'll have to make one
        // NOTE: this isn't exactly optimal in terms of ensuring the gfx card doesn't stall waiting for the data
        // may be better to initiate upload earlier (during update phase) and trust this effort won't go to waste
        if( true == m_chunks.empty() ) { return; }

        std::size_t datasize{ 0 };
        auto chunkiterator = m_chunks.cbegin();
        for( auto &chunkrecord : m_chunkrecords ) {
            // fill records for all chunks, based on the chunk data
            chunkrecord.offset = datasize;
            chunkrecord.size = chunkiterator->vertices.size();
            datasize += chunkrecord.size;
            ++chunkiterator;
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
        if( ::glGetError() == GL_OUT_OF_MEMORY ) {
            // TBD: throw a bad_alloc?
            ErrorLog( "openGL error: out of memory; failed to create a geometry buffer" );
            delete_buffer();
            return;
        }
        m_buffercapacity = datasize;
    }
    // actual draw procedure starts here
    // setup...
    if( m_activebuffer != m_buffer ) {
        bind_buffer();
    }
    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    auto const &chunk = geometry_bank::chunk( Geometry );
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

// opengl display list based variant of the geometry bank

// create() subclass details
void
opengl_dlgeometrybank::create_( geometry_handle const &Geometry ) {

    m_chunkrecords.emplace_back( chunk_record() );
}

// replace() subclass details
void
opengl_dlgeometrybank::replace_( geometry_handle const &Geometry ) {

    delete_list( Geometry );
}

// draw() subclass details
void
opengl_dlgeometrybank::draw_( geometry_handle const &Geometry ) {

    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    if( chunkrecord.list == 0 ) {
        // we don't have a list ready, so compile one
        chunkrecord.list = ::glGenLists( 1 );
        auto const &chunk = geometry_bank::chunk( Geometry );
        ::glNewList( chunkrecord.list, GL_COMPILE );

        ::glBegin( chunk.type );
        // TODO: add specification of chunk vertex attributes
        for( auto const &vertex : chunk.vertices ) {
            ::glNormal3fv( glm::value_ptr( vertex.normal ) );
            ::glTexCoord2fv( glm::value_ptr( vertex.texture ) );
            ::glVertex3fv( glm::value_ptr( vertex.position ) );
        }
        ::glEnd();
        ::glEndList();
    }
    // with the list done we can just play it
    ::glCallList( chunkrecord.list );
}

void
opengl_dlgeometrybank::delete_list( geometry_handle const &Geometry ) {
    // NOTE: given it's our own internal method we trust it to be called with valid parameters
    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    ::glDeleteLists( chunkrecord.list, 1 );
    chunkrecord.list = 0;
}

// geometry bank manager, holds collection of geometry banks

// creates a new geometry bank. returns: handle to the bank or NULL
geometrybank_handle
geometrybank_manager::create_bank() {

    if( true == Global::bUseVBO ) { m_geometrybanks.emplace_back( std::make_shared<opengl_vbogeometrybank>() ); }
    else                          { m_geometrybanks.emplace_back( std::make_shared<opengl_dlgeometrybank>() ); }
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    return geometrybank_handle( m_geometrybanks.size(), 0 );
}

// creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
geometry_handle
geometrybank_manager::create_chunk( vertex_array &Vertices, geometrybank_handle const &Geometry, int const Type ) {

    auto const newchunkhandle = bank( Geometry )->create( Vertices, Type );

    if( newchunkhandle.chunk != 0 ) { return geometry_handle( Geometry.bank, newchunkhandle.chunk ); }
    else                            { return geometry_handle( 0, 0 ); }
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
geometrybank_manager::replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset ) {

    return bank( Geometry )->replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
geometrybank_manager::append( vertex_array &Vertices, geometry_handle const &Geometry ) {

    return bank( Geometry )->append( Vertices, Geometry );
}
// draws geometry stored in specified chunk
void
geometrybank_manager::draw( geometry_handle const &Geometry ) {

    if( Geometry == NULL ) { return; }

    return bank( Geometry )->draw( Geometry );
}

// provides direct access to vertex data of specfied chunk
vertex_array const &
geometrybank_manager::vertices( geometry_handle const &Geometry ) const {

    return bank( Geometry )->vertices( Geometry );
}