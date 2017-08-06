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
#include "Logs.h"
#include "Globals.h"

void
basic_vertex::serialize( std::ostream &s ) const {

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
geometry_bank::create( vertex_array &Vertices, unsigned int const Type ) {

    if( true == Vertices.empty() ) { return geometry_handle( 0, 0 ); }

    m_chunks.emplace_back( Vertices, Type );
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    geometry_handle chunkhandle { 0, static_cast<std::uint32_t>(m_chunks.size()) };
    // template method implementation
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
    // template method implementation
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
geometry_bank::draw( geometry_handle const &Geometry, unsigned int const Streams ) {
    // template method implementation
    draw_( Geometry, Streams );
}

// frees subclass-specific resources associated with the bank, typically called when the bank wasn't in use for a period of time
void
geometry_bank::release() {
    // template method implementation
    release_();
}

vertex_array const &
geometry_bank::vertices( geometry_handle const &Geometry ) const {

    return geometry_bank::chunk( Geometry ).vertices;
}

// opengl vbo-based variant of the geometry bank

GLuint opengl_vbogeometrybank::m_activebuffer { 0 }; // buffer bound currently on the opengl end, if any
unsigned int opengl_vbogeometrybank::m_activestreams { stream::none }; // currently enabled data type pointers

// create() subclass details
void
opengl_vbogeometrybank::create_( geometry_handle const &Geometry ) {
    // adding a chunk means we'll be (re)building the buffer, which will fill the chunk records, amongst other things.
    // thus we don't need to initialize the values here
    m_chunkrecords.emplace_back( chunk_record() );
    // kiss the existing buffer goodbye, new overall data size means we'll be making a new one
    delete_buffer();

	bind_streams(stream::none);
	glGenVertexArrays(1, &m_vao);
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
opengl_vbogeometrybank::draw_( geometry_handle const &Geometry, unsigned int const Streams ) {
    if( m_buffer == 0 ) {
        // if there's no buffer, we'll have to make one
        // NOTE: this isn't exactly optimal in terms of ensuring the gfx card doesn't stall waiting for the data
        // may be better to initiate upload earlier (during update phase) and trust this effort won't go to waste
        if( true == m_chunks.empty() ) { return; }
        std::size_t datasize{ 0 };
        auto chunkiterator = m_chunks.cbegin();
        for( auto &chunkrecord : m_chunkrecords ) {
            // fill records for all chunks, based on the chunk data
            chunkrecord.is_good = false; // if we're re-creating buffer, chunks might've been uploaded in the old one
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
	if( m_activestreams != Streams ) {
        bind_streams( Streams );
    }

    // ...render...
    ::glDrawArrays( chunk.type, (GLint)chunkrecord.offset, (GLsizei)chunkrecord.size );
    // ...post-render cleanup
}

// release () subclass details
void
opengl_vbogeometrybank::release_() {

    delete_buffer();
}

void
opengl_vbogeometrybank::bind_buffer() {
	glBindVertexArray(m_vao);
    ::glBindBuffer( GL_ARRAY_BUFFER, m_buffer );
    m_activebuffer = m_buffer;
	bind_streams(stream::none);
}

void
opengl_vbogeometrybank::delete_buffer() {
	glDeleteVertexArrays(1, &m_vao);
    if( m_buffer != 0 ) {
        
        ::glDeleteBuffers( 1, &m_buffer );
        if( m_activebuffer == m_buffer ) {
            m_activebuffer = 0;
            bind_streams( stream::none );
        }
        m_buffer = 0;
        m_buffercapacity = 0;
        // NOTE: since we've deleted the buffer all chunks it held were rendered invalid as well
        // instead of clearing their state here we're delaying it until new buffer is created to avoid looping through chunk records twice
    }
}

void
opengl_vbogeometrybank::bind_streams( unsigned int const Streams ) {
    if( Streams & stream::position ) {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
		glEnableVertexAttribArray(0);
    }
    else {
		glDisableVertexAttribArray(0);
    }

    // NOTE: normal and color streams share the data, making them effectively mutually exclusive
    if( Streams & stream::normal ) {
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));
		glEnableVertexAttribArray(1);
    }
    else {
		glDisableVertexAttribArray(1);
    }

    if( Streams & stream::color ) {

    }
    else {

    }

    if( Streams & stream::texture ) {
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 6));
		glEnableVertexAttribArray(2);
    }
    else {
		glDisableVertexAttribArray(2);
    }

    m_activestreams = Streams;
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
opengl_dlgeometrybank::draw_( geometry_handle const &Geometry, unsigned int const Streams ) {

    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    if( chunkrecord.streams != Streams ) {
        delete_list( Geometry );
    }
    if( chunkrecord.list == 0 ) {
        // we don't have a list ready, so compile one
        chunkrecord.streams = Streams;
        chunkrecord.list = ::glGenLists( 1 );
        auto const &chunk = geometry_bank::chunk( Geometry );
        ::glNewList( chunkrecord.list, GL_COMPILE );

        ::glBegin( chunk.type );
        for( auto const &vertex : chunk.vertices ) {
                 if( Streams & stream::normal ) { ::glNormal3fv( glm::value_ptr( vertex.normal ) ); }
            else if( Streams & stream::color )  { ::glColor3fv( glm::value_ptr( vertex.normal ) ); }
            if( Streams & stream::texture )     { ::glTexCoord2fv( glm::value_ptr( vertex.texture ) ); }
            if( Streams & stream::position )    { ::glVertex3fv( glm::value_ptr( vertex.position ) ); }
        }
        ::glEnd();
        ::glEndList();
    }
    // with the list done we can just play it
    ::glCallList( chunkrecord.list );
}

// release () subclass details
void
opengl_dlgeometrybank::release_() {

    for( auto &chunkrecord : m_chunkrecords ) {
        if( chunkrecord.list != 0 ) {
            ::glDeleteLists( chunkrecord.list, 1 );
            chunkrecord.list = 0;
        }
        chunkrecord.streams = stream::none;
    }
}

void
opengl_dlgeometrybank::delete_list( geometry_handle const &Geometry ) {
    // NOTE: given it's our own internal method we trust it to be called with valid parameters
    auto &chunkrecord = m_chunkrecords[ Geometry.chunk - 1 ];
    if( chunkrecord.list != 0 ) {
        ::glDeleteLists( chunkrecord.list, 1 );
        chunkrecord.list = 0;
    }
    chunkrecord.streams = stream::none;
}

// geometry bank manager, holds collection of geometry banks

// performs a resource sweep
void
geometrybank_manager::update() {

    m_garbagecollector.sweep();
}

// creates a new geometry bank. returns: handle to the bank or NULL
geometrybank_handle
geometrybank_manager::create_bank() {

	m_geometrybanks.emplace_back(std::make_shared<opengl_vbogeometrybank>(), std::chrono::steady_clock::time_point());
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    return geometrybank_handle( (uint32_t)m_geometrybanks.size(), 0 );
}

// creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
geometry_handle
geometrybank_manager::create_chunk( vertex_array &Vertices, geometrybank_handle const &Geometry, int const Type ) {

    auto const newchunkhandle = bank( Geometry ).first->create( Vertices, Type );

    if( newchunkhandle.chunk != 0 ) { return geometry_handle( Geometry.bank, newchunkhandle.chunk ); }
    else                            { return geometry_handle( 0, 0 ); }
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
geometrybank_manager::replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset ) {

    return bank( Geometry ).first->replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
geometrybank_manager::append( vertex_array &Vertices, geometry_handle const &Geometry ) {

    return bank( Geometry ).first->append( Vertices, Geometry );
}
// draws geometry stored in specified chunk
void
geometrybank_manager::draw( geometry_handle const &Geometry, unsigned int const Streams ) {

    if( Geometry == 0 ) { return; }

    auto &bankrecord = bank( Geometry );

    bankrecord.second = m_garbagecollector.timestamp();
    bankrecord.first->draw( Geometry, Streams );
}

// provides direct access to vertex data of specfied chunk
vertex_array const &
geometrybank_manager::vertices( geometry_handle const &Geometry ) const {

    return bank( Geometry ).first->vertices( Geometry );
}
