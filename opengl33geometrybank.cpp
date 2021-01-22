/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "opengl33geometrybank.h"

#include "Logs.h"

namespace gfx {

// opengl vao/vbo-based variant of the geometry bank

// create() subclass details
void
opengl33_vaogeometrybank::create_( gfx::geometry_handle const &Geometry ) {
    // adding a chunk means we'll be (re)building the buffer, which will fill the chunk records, amongst other things.
    // thus we don't need to initialize the values here
    m_chunkrecords.emplace_back( chunk_record() );
    // kiss the existing buffer goodbye, new overall data size means we'll be making a new one
    delete_buffer();
}

// replace() subclass details
void
opengl33_vaogeometrybank::replace_( gfx::geometry_handle const &Geometry ) {

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

void opengl33_vaogeometrybank::setup_buffer()
{
    if( m_vertexbuffer ) { return; }
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

    if( !m_vao ) {
        m_vao.emplace();
    }
    m_vao->bind();
    // try to set up the buffers we need:
    // optional index buffer...
    if( indexcount > 0 ) {
        m_indexbuffer.emplace();
        m_indexbuffer->allocate( gl::buffer::ELEMENT_ARRAY_BUFFER, indexcount * sizeof( gfx::basic_index ), GL_STATIC_DRAW );
        if( ::glGetError() == GL_OUT_OF_MEMORY ) {
            ErrorLog( "openGL error: out of memory; failed to create a geometry index buffer" );
            throw std::bad_alloc();
        }
        m_vao->setup_ebo( *m_indexbuffer );
    }
    else {
        gl::buffer::unbind( gl::buffer::ELEMENT_ARRAY_BUFFER );
    }
    // ...and geometry buffer
	m_vertexbuffer.emplace();
    // NOTE: we're using static_draw since it's generally true for all we have implemented at the moment
    // TODO: allow to specify usage hint at the object creation, and pass it here
    m_vertexbuffer->allocate( gl::buffer::ARRAY_BUFFER, vertexcount * sizeof( gfx::basic_vertex ), GL_STATIC_DRAW );
    if( ::glGetError() == GL_OUT_OF_MEMORY ) {
        ErrorLog( "openGL error: out of memory; failed to create a geometry buffer" );
        throw std::bad_alloc();
    }

    setup_attrib();
}

void
opengl33_vaogeometrybank::setup_attrib(size_t offset)
{
    m_vao->setup_attrib( *m_vertexbuffer, 0, 3, GL_FLOAT, sizeof( basic_vertex ), 0 * sizeof( float ) + offset * sizeof( basic_vertex ) );
    // NOTE: normal and color streams share the data
    m_vao->setup_attrib( *m_vertexbuffer, 1, 3, GL_FLOAT, sizeof( basic_vertex ), 3 * sizeof( float ) + offset * sizeof( basic_vertex ) );
    m_vao->setup_attrib( *m_vertexbuffer, 2, 2, GL_FLOAT, sizeof( basic_vertex ), 6 * sizeof( float ) + offset * sizeof( basic_vertex ) );
    m_vao->setup_attrib( *m_vertexbuffer, 3, 4, GL_FLOAT, sizeof( basic_vertex ), 8 * sizeof( float ) + offset * sizeof( basic_vertex ) );
}

// draw() subclass details
// NOTE: units and stream parameters are unused, but they're part of (legacy) interface
// TBD: specialized bank/manager pair without the cruft?
std::size_t
opengl33_vaogeometrybank::draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams )
{
    setup_buffer();

    auto &chunkrecord = m_chunkrecords.at(Geometry.chunk - 1);
	// sanity check; shouldn't be needed but, eh
	if( chunkrecord.vertex_count == 0 )
		return 0;

    auto const &chunk = gfx::geometry_bank::chunk( Geometry );
    if( false == chunkrecord.is_good ) {
        m_vao->bind();
        // we may potentially need to upload new buffer data before we can draw it
        if( chunkrecord.index_count > 0 ) {
            m_indexbuffer->upload( gl::buffer::ELEMENT_ARRAY_BUFFER, chunk.indices.data(), chunkrecord.index_offset * sizeof( gfx::basic_index ), chunkrecord.index_count * sizeof( gfx::basic_index ) );
        }
        m_vertexbuffer->upload( gl::buffer::ARRAY_BUFFER, chunk.vertices.data(), chunkrecord.vertex_offset * sizeof( gfx::basic_vertex ), chunkrecord.vertex_count * sizeof( gfx::basic_vertex ) );
        chunkrecord.is_good = true;
    }
    // render
    if( chunkrecord.index_count > 0 ) {
        if (glDrawRangeElementsBaseVertex) {
            m_vao->bind();
            ::glDrawRangeElementsBaseVertex(
                chunk.type,
                0, chunkrecord.vertex_count,
                chunkrecord.index_count, GL_UNSIGNED_INT, reinterpret_cast<void const *>( chunkrecord.index_offset * sizeof( gfx::basic_index ) ),
                chunkrecord.vertex_offset );
        }
        else if (glDrawElementsBaseVertexOES) {
            m_vao->bind();
            ::glDrawElementsBaseVertexOES(
                chunk.type,
                chunkrecord.index_count, GL_UNSIGNED_INT, reinterpret_cast<void const *>( chunkrecord.index_offset * sizeof( gfx::basic_index ) ),
                chunkrecord.vertex_offset );
        }
        else {
            setup_attrib(chunkrecord.vertex_offset);
            m_vao->bind();
            ::glDrawRangeElements(
                chunk.type,
                0, chunkrecord.vertex_count,
                chunkrecord.index_count, GL_UNSIGNED_INT, reinterpret_cast<void const *>( chunkrecord.index_offset * sizeof( gfx::basic_index ) ) );
        }
    }
    else {
        m_vao->bind();
        ::glDrawArrays( chunk.type, chunkrecord.vertex_offset, chunkrecord.vertex_count );
    }
/*
    m_vao->unbind();
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
opengl33_vaogeometrybank::release_() {

    delete_buffer();
}

void
opengl33_vaogeometrybank::delete_buffer() {

	m_vao.reset();
	m_vertexbuffer.reset();
    m_indexbuffer.reset();
    // NOTE: since we've deleted the buffer all chunks it held were rendered invalid as well
    // instead of clearing their state here we're delaying it until new buffer is created to avoid looping through chunk records twice
}

} // namespace gfx
