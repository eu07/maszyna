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
#include "material.h"

class gfx_renderer {

public:
// types
// constructors
// destructor
    virtual ~gfx_renderer() {}
// methods
    virtual auto Init( GLFWwindow *Window ) -> bool = 0;
    // main draw call. returns false on error
    virtual auto Render() -> bool = 0;
    virtual auto Framerate() -> float = 0;
    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    virtual auto Create_Bank() -> gfx::geometrybank_handle = 0;
    // creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
    virtual auto Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) -> gfx::geometry_handle = 0;
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    virtual auto Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 ) -> bool = 0;
    // adds supplied vertex data at the end of specified chunk
    virtual auto Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) -> bool = 0;
    // provides direct access to vertex data of specfied chunk
    virtual auto Vertices( gfx::geometry_handle const &Geometry ) const ->gfx::vertex_array const & = 0;
    // material methods
    virtual auto Fetch_Material( std::string const &Filename, bool const Loadnow = true ) -> material_handle = 0;
    virtual void Bind_Material( material_handle const Material ) = 0;
    virtual auto Material( material_handle const Material ) const -> opengl_material const & = 0;
    // texture methods
    virtual auto Fetch_Texture( std::string const &Filename, bool const Loadnow = true ) -> texture_handle = 0;
    virtual void Bind_Texture( texture_handle const Texture ) = 0;
    virtual auto Texture( texture_handle const Texture ) const -> opengl_texture const & = 0;
    // utility methods
    virtual auto Pick_Control() const -> TSubModel const * = 0;
    virtual auto Pick_Node() const -> scene::basic_node const * = 0;
    virtual auto Mouse_Position() const -> glm::dvec3 = 0;
    // maintenance methods
    virtual void Update( double const Deltatime ) = 0;
    virtual auto Update_Pick_Control() -> TSubModel * = 0;
    virtual auto Update_Pick_Node() -> scene::basic_node * = 0;
    virtual auto Update_Mouse_Position() -> glm::dvec3 = 0;
    // debug methods
    virtual auto info_times() const -> std::string const & = 0;
    virtual auto info_stats() const -> std::string const & = 0;
};

extern std::unique_ptr<gfx_renderer> GfxRenderer;

//---------------------------------------------------------------------------
