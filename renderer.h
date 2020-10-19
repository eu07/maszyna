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
#include "Globals.h"

struct lighting_data;

class gfx_renderer {

public:
// types
// constructors
// destructor
    virtual ~gfx_renderer() {}
// methods
    virtual auto Init( GLFWwindow *Window ) -> bool = 0;
    virtual bool AddViewport(const global_settings::extraviewport_config &conf) = 0;
    virtual void Shutdown() = 0;
    // main draw call. returns false on error
    virtual auto Render() -> bool = 0;
    virtual void SwapBuffers() = 0;
    virtual auto Framerate() -> float = 0;
    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    virtual auto Create_Bank() -> gfx::geometrybank_handle = 0;
    // creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    virtual auto Insert( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) -> gfx::geometry_handle = 0;
    // creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    virtual auto Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) -> gfx::geometry_handle = 0;
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    virtual auto Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset = 0 ) -> bool = 0;
    // adds supplied vertex data at the end of specified chunk
    virtual auto Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type ) -> bool = 0;
    // provides direct access to index data of specfied chunk
    virtual auto Indices( gfx::geometry_handle const &Geometry ) const->gfx::index_array const & = 0;
    // provides direct access to vertex data of specfied chunk
    virtual auto Vertices( gfx::geometry_handle const &Geometry ) const ->gfx::vertex_array const & = 0;
    // material methods
    virtual auto Fetch_Material( std::string const &Filename, bool const Loadnow = true ) -> material_handle = 0;
    virtual void Bind_Material( material_handle const Material, TSubModel const *sm = nullptr, lighting_data const *lighting = nullptr ) = 0;
    virtual auto Material( material_handle const Material ) const -> opengl_material const & = 0;
    // shader methods
    virtual auto Fetch_Shader( std::string const &name ) -> std::shared_ptr<gl::program> = 0;
    // texture methods
    virtual auto Fetch_Texture( std::string const &Filename, bool const Loadnow = true, GLint format_hint = GL_SRGB_ALPHA ) -> texture_handle = 0;
    virtual void Bind_Texture( texture_handle const Texture ) = 0;
    virtual void Bind_Texture( std::size_t const Unit, texture_handle const Texture ) = 0;
    virtual auto Texture( texture_handle const Texture ) -> opengl_texture & = 0;
    virtual auto Texture( texture_handle const Texture ) const -> opengl_texture const & = 0;
    // utility methods
    virtual void Pick_Control_Callback( std::function<void( TSubModel const *, const glm::vec2 )> Callback ) = 0;
    virtual void Pick_Node_Callback( std::function<void( scene::basic_node * )> Callback ) = 0;
    virtual auto Pick_Control() const -> TSubModel const * = 0;
    virtual auto Pick_Node() const -> scene::basic_node const * = 0;

    virtual auto Mouse_Position() const -> glm::dvec3 = 0;
    // maintenance methods
    virtual void Update( double const Deltatime ) = 0;
    virtual void Update_Pick_Control() = 0;
    virtual void Update_Pick_Node() = 0;
    virtual auto Update_Mouse_Position() -> glm::dvec3 = 0;
    virtual bool Debug_Ui_State(std::optional<bool>) = 0;
    // debug methods
    virtual auto info_times() const -> std::string const & = 0;
    virtual auto info_stats() const -> std::string const & = 0;
};

class gfx_renderer_factory
{
public:
    using create_method = std::unique_ptr<gfx_renderer>(*)();

    bool register_backend(const std::string &backend, create_method func);
    std::unique_ptr<gfx_renderer> create(const std::string &name);

    static gfx_renderer_factory* get_instance();

private:
    std::unordered_map<std::string, create_method> backends;
    static gfx_renderer_factory *instance;
};


extern std::unique_ptr<gfx_renderer> GfxRenderer;

//---------------------------------------------------------------------------
