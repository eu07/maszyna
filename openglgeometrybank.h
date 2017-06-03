/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL/glew.h"
#ifdef _WINDOWS
#include "GL/wglew.h"
#endif

struct basic_vertex {
    glm::vec3 position; // 3d space
    glm::vec3 normal; // 3d space
    glm::vec2 texture; // uv space

    void serialize( std::ostream& );
    void deserialize( std::istream& );
};

typedef std::vector<basic_vertex> vertex_array;

// generic geometry bank class, allows storage, update and drawing of geometry chunks

typedef std::size_t geometrychunk_handle;

class geometry_bank {

public:
// types:

// constructors:

// destructor:
    virtual
        ~geometry_bank() { ; }

// methods:
    // creates a new geometry chunk of specified type from supplied vertex data. returns: handle to the chunk or NULL
    virtual
    geometrychunk_handle
        create( vertex_array &Vertices, int const Datatype );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    virtual
    bool
        replace( vertex_array &Vertices, geometrychunk_handle const Chunk, std::size_t const Offset = 0 );
    // draws geometry stored in specified chunk
    virtual
    void
        draw( geometrychunk_handle const Chunk ) = 0;
    // draws geometry stored in supplied list of chunks
    template <typename _Iterator>
    void
        draw( _Iterator First, _Iterator Last ) { while( First != Last ) { draw( *First ); ++First; } }
    vertex_array &
        data( geometrychunk_handle const Chunk );

protected:
// types:
    struct geometry_chunk {
        int type; // kind of geometry used by the chunk
        vertex_array vertices; // geometry data

        geometry_chunk( vertex_array &Vertices, int const Datatype ) :
                            vertices( Vertices ),   type( Datatype )
        {}
    };

    typedef std::vector<geometry_chunk> geometrychunk_sequence;

// members:
    geometrychunk_sequence m_chunks;

};

// opengl vbo-based variant of the geometry bank

class opengl_vbogeometrybank : public geometry_bank {

public:
// methods:
    // creates a new geometry chunk of specified type from supplied vertex data. returns: handle to the chunk or NULL
    geometrychunk_handle
        create( vertex_array &Vertices, int const Datatype );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        replace( vertex_array &Vertices, geometrychunk_handle const Chunk, std::size_t const Offset = 0 );
    // draws geometry stored in specified chunk
    void
        draw( geometrychunk_handle const Chunk );

private:
// types:
    struct chunk_record{
        std::size_t offset{ 0 }; // beginning of the chunk data as offset from the beginning of the last established buffer
        std::size_t size{ 0 }; // size of the chunk in the last established buffer
        bool is_good{ false }; // true if local content of the chunk matches the data on the opengl end
    };

    typedef std::vector<chunk_record> chunkrecord_sequence;

// methods:
    void
        bind_buffer();
    void
        delete_buffer();

// members:
    static GLuint m_activebuffer; // buffer bound currently on the opengl end, if any
    GLuint m_buffer{ NULL }; // id of the buffer holding data on the opengl end
    std::size_t m_buffercapacity{ 0 }; // total capacity of the last established buffer
    chunkrecord_sequence m_chunkrecords; // helper data for all stored geometry chunks, in matching order

};
