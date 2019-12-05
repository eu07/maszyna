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
    if( geometry_bank::chunk( Geometry ).vertices.size() != chunkrecord.size ) {
        // ...but otherwise we'll need to allocate a new one
        // TBD: we could keep and reuse the old buffer also if the new chunk is smaller than the old one,
        // but it'd require some extra tracking and work to keep all chunks up to date; also wasting vram; may be not worth it?
        delete_buffer();
    }
}

void opengl33_vaogeometrybank::setup_buffer()
{
	if( !m_buffer ) {
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
		m_buffer.emplace();

        // NOTE: we're using static_draw since it's generally true for all we have implemented at the moment
        // TODO: allow to specify usage hint at the object creation, and pass it here
		m_buffer->allocate(gl::buffer::ARRAY_BUFFER, datasize * sizeof(gfx::basic_vertex), GL_STATIC_DRAW);

        if( ::glGetError() == GL_OUT_OF_MEMORY ) {
            ErrorLog( "openGL error: out of memory; failed to create a geometry buffer" );
            throw std::bad_alloc();
        }
        m_buffercapacity = datasize;
    }

    if (!m_vao)
    {
		m_vao.emplace();

		m_vao->setup_attrib(*m_buffer, 0, 3, GL_FLOAT, sizeof(basic_vertex), 0 * sizeof(float));
        // NOTE: normal and color streams share the data
		m_vao->setup_attrib(*m_buffer, 1, 3, GL_FLOAT, sizeof(basic_vertex), 3 * sizeof(float));
		m_vao->setup_attrib(*m_buffer, 2, 2, GL_FLOAT, sizeof(basic_vertex), 6 * sizeof(float));
		m_vao->setup_attrib(*m_buffer, 3, 4, GL_FLOAT, sizeof(basic_vertex), 8 * sizeof(float));

		m_buffer->unbind(gl::buffer::ARRAY_BUFFER);
        m_vao->unbind();
    }
}

// draw() subclass details
// NOTE: units and stream parameters are unused, but they're part of (legacy) interface
// TBD: specialized bank/manager pair without the cruft?
void
opengl33_vaogeometrybank::draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams )
{
    setup_buffer();

    auto &chunkrecord = m_chunkrecords.at(Geometry.chunk - 1);
	// sanity check; shouldn't be needed but, eh
	if( chunkrecord.size == 0 )
		return;
    auto const &chunk = gfx::geometry_bank::chunk( Geometry );
    if( false == chunkrecord.is_good ) {
        // we may potentially need to upload new buffer data before we can draw it
		m_buffer->upload(gl::buffer::ARRAY_BUFFER, chunk.vertices.data(),
		                 chunkrecord.offset * sizeof( gfx::basic_vertex ),
		                 chunkrecord.size * sizeof( gfx::basic_vertex ));
        chunkrecord.is_good = true;
    }
    // render
    m_vao->bind();
    ::glDrawArrays( chunk.type, chunkrecord.offset, chunkrecord.size );
}

// release () subclass details
void
opengl33_vaogeometrybank::release_() {

    delete_buffer();
}

void
opengl33_vaogeometrybank::delete_buffer() {

	if( m_buffer ) {
        
		m_vao.reset();
		m_buffer.reset();
        m_buffercapacity = 0;
        // NOTE: since we've deleted the buffer all chunks it held were rendered invalid as well
        // instead of clearing their state here we're delaying it until new buffer is created to avoid looping through chunk records twice
    }
}

} // namespace gfx
