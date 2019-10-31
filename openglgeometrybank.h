/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "geometrybank.h"

namespace gfx {

// opengl vbo-based variant of the geometry bank

class opengl_vbogeometrybank : public geometry_bank {

public:
// constructors:
    opengl_vbogeometrybank() = default;
// destructor
    ~opengl_vbogeometrybank() {
        delete_buffer(); }
// methods:
    static
    void
        reset() {
            m_activebuffer = 0;
            m_activestreams = gfx::stream::none; }

private:
// types:
    struct chunk_record{
        std::size_t offset{ 0 }; // beginning of the chunk data as offset from the beginning of the last established buffer
        std::size_t size{ 0 }; // size of the chunk in the last established buffer
        bool is_good{ false }; // true if local content of the chunk matches the data on the opengl end
    };

    using chunkrecord_sequence = std::vector<chunk_record>;

// methods:
    // create() subclass details
    void
        create_( gfx::geometry_handle const &Geometry ) override;
    // replace() subclass details
    void
        replace_( gfx::geometry_handle const &Geometry ) override;
    // draw() subclass details
    void
        draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) override;
    // release() subclass details
    void
        release_() override;
    void
        setup_buffer();
    void
        bind_buffer();
    void
        delete_buffer();
    static
    void
        bind_streams( gfx::stream_units const &Units, unsigned int const Streams );
    static
    void
        release_streams();

// members:
    static GLuint m_activebuffer; // buffer bound currently on the opengl end, if any
    static unsigned int m_activestreams;
    static std::vector<GLint> m_activetexturearrays;
    GLuint m_buffer { 0 }; // id of the buffer holding data on the opengl end
    std::size_t m_buffercapacity{ 0 }; // total capacity of the last established buffer
    chunkrecord_sequence m_chunkrecords; // helper data for all stored geometry chunks, in matching order

};

// opengl display list based variant of the geometry bank

class opengl_dlgeometrybank : public geometry_bank {

public:
// constructors:
    opengl_dlgeometrybank() = default;
// destructor:
    ~opengl_dlgeometrybank() {
        for( auto &chunkrecord : m_chunkrecords ) {
            ::glDeleteLists( chunkrecord.list, 1 ); } }

private:
// types:
    struct chunk_record {
        GLuint list { 0 }; // display list associated with the chunk
        unsigned int streams { 0 }; // stream combination used to generate the display list
    };

    using chunkrecord_sequence = std::vector<chunk_record>;

// methods:
    // create() subclass details
    void
        create_( gfx::geometry_handle const &Geometry ) override;
    // replace() subclass details
    void
        replace_( gfx::geometry_handle const &Geometry ) override;
    // draw() subclass details
    void
        draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) override;
    // release () subclass details
    void
        release_() override;
    void
        delete_list( gfx::geometry_handle const &Geometry );

// members:
    chunkrecord_sequence m_chunkrecords; // helper data for all stored geometry chunks, in matching order

};

} // namespace gfx