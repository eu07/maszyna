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

struct geometry_handle {
// constructors
    geometry_handle() :
        bank( 0 ), chunk( 0 )
    {}
    geometry_handle( std::uint32_t const Bank, std::uint32_t const Chunk ) :
                                   bank( Bank ),            chunk( Chunk )
    {}
// methods
    inline
    operator std::uint32_t() const {
        return bank << 12 | chunk; }
// members
    std::uint32_t
        bank  : 20, // 1 mil banks
        chunk : 12; // 4 k chunks per bank
};

class geometry_bank {

public:
// types:

// constructors:

// destructor:
    virtual
        ~geometry_bank() {}

// methods:
    // creates a new geometry chunk of specified type from supplied vertex data. returns: handle to the chunk or NULL
    geometry_handle
        create( vertex_array &Vertices, int const Type );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset = 0 );
    // adds supplied vertex data at the end of specified chunk
    bool
        append( vertex_array &Vertices, geometry_handle const &Geometry );
    // draws geometry stored in specified chunk
    void
        draw( geometry_handle const &Geometry );
    // draws geometry stored in supplied list of chunks
    template <typename Iterator_>
    void
        draw( Iterator_ First, Iterator_ Last ) { while( First != Last ) { draw( *First ); ++First; } }
    // provides direct access to vertex data of specfied chunk
    vertex_array const &
        vertices( geometry_handle const &Geometry ) const;

protected:
// types:
    struct geometry_chunk {
        int type; // kind of geometry used by the chunk
        vertex_array vertices; // geometry data

        geometry_chunk( vertex_array &Vertices, int const Type ) :
                            vertices( Vertices ),   type( Type )
        {}
    };

    typedef std::vector<geometry_chunk> geometrychunk_sequence;

// methods
    inline
    geometry_chunk &
        chunk( geometry_handle const Geometry ) {
            return m_chunks[ Geometry.chunk - 1 ]; }
    inline
    geometry_chunk const &
        chunk( geometry_handle const Geometry ) const {
            return m_chunks[ Geometry.chunk - 1 ]; }

// members:
    geometrychunk_sequence m_chunks;

private:
// methods:
    // create() subclass details
    virtual void create_( geometry_handle const &Geometry ) = 0;
    // replace() subclass details
    virtual void replace_( geometry_handle const &Geometry ) = 0;
    // draw() subclass details
    virtual void draw_( geometry_handle const &Geometry ) = 0;
};

// opengl vbo-based variant of the geometry bank

class opengl_vbogeometrybank : public geometry_bank {

public:
// methods:
    ~opengl_vbogeometrybank() {
        delete_buffer(); }

private:
// types:
    struct chunk_record{
        std::size_t offset{ 0 }; // beginning of the chunk data as offset from the beginning of the last established buffer
        std::size_t size{ 0 }; // size of the chunk in the last established buffer
        bool is_good{ false }; // true if local content of the chunk matches the data on the opengl end
    };

    typedef std::vector<chunk_record> chunkrecord_sequence;

// methods:
    // create() subclass details
    void
        create_( geometry_handle const &Geometry );
    // replace() subclass details
    void
        replace_( geometry_handle const &Geometry );
    // draw() subclass details
    void
        draw_( geometry_handle const &Geometry );
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

// opengl display list based variant of the geometry bank

class opengl_dlgeometrybank : public geometry_bank {

public:
// methods:
    ~opengl_dlgeometrybank() {
        for( auto &chunkrecord : m_chunkrecords ) {
            ::glDeleteLists( chunkrecord.list, 1 ); } }

private:
// types:
    struct chunk_record {
        GLuint list{ 0 }; // display list associated with the chunk
    };

    typedef std::vector<chunk_record> chunkrecord_sequence;

// methods:
    // create() subclass details
    void
        create_( geometry_handle const &Geometry );
    // replace() subclass details
    void
        replace_( geometry_handle const &Geometry );
    // draw() subclass details
    void
        draw_( geometry_handle const &Geometry );
    void
        delete_list( geometry_handle const &Geometry );

// members:
    chunkrecord_sequence m_chunkrecords; // helper data for all stored geometry chunks, in matching order

};

// geometry bank manager, holds collection of geometry banks

typedef geometry_handle geometrybank_handle;

class geometrybank_manager {

public:
// methods:
    // creates a new geometry bank. returns: handle to the bank or NULL
    geometrybank_handle
        create_bank();
    // creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
    geometry_handle
        create_chunk( vertex_array &Vertices, geometrybank_handle const &Geometry, int const Type );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset = 0 );
    // adds supplied vertex data at the end of specified chunk
    bool
        append( vertex_array &Vertices, geometry_handle const &Geometry );
    // draws geometry stored in specified chunk
    void
        draw( geometry_handle const &Geometry );
    // provides direct access to vertex data of specfied chunk
    vertex_array const &
        vertices( geometry_handle const &Geometry ) const;

private:
// types:
    typedef std::deque< std::shared_ptr<geometry_bank> > geometrybank_sequence;

// members:
    geometrybank_sequence m_geometrybanks;

// methods
    inline
    bool
        valid( geometry_handle const &Geometry ) {
        return ( ( Geometry.bank != 0 )
              && ( Geometry.bank <= m_geometrybanks.size() ) ); }
    inline
    geometrybank_sequence::value_type &
        bank( geometry_handle const Geometry ) {
            return m_geometrybanks[ Geometry.bank - 1 ]; }
    inline
    geometrybank_sequence::value_type const &
        bank( geometry_handle const Geometry ) const {
            return m_geometrybanks[ Geometry.bank - 1 ]; }

};