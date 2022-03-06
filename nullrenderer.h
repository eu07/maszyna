/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "Texture.h"
#include "geometrybank.h"
#include "material.h"
#include "renderer.h"
#include "stdafx.h"
#include <glm/fwd.hpp>
#include <stdexcept>

class null_geometrybank : public gfx::geometry_bank {
public:
// constructors:
    null_geometrybank() = default;
// destructor
    ~null_geometrybank() {};

private:
// methods:
    // create() subclass details
    void
        create_( gfx::geometry_handle const &Geometry ) override {}
    // replace() subclass details
    void
        replace_( gfx::geometry_handle const &Geometry ) override {}
    // draw() subclass details
    auto
        draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) -> std::size_t override { return 0; }
    // release() subclass details
    void
        release_() override {}
};

// bare-bones render controller, in lack of anything better yet
class null_renderer : public gfx_renderer {

public:
// types
// constructors
    null_renderer() = default;
// destructor
    ~null_renderer() { }
// methods
    bool
        Init( GLFWwindow *Window ) override { return true; }
    // main draw call. returns false on error
    bool
        Render() override { return true; }
    void
        SwapBuffers() override {}
    inline
    float
        Framerate() override { return 10.0f; }

    bool AddViewport(const global_settings::extraviewport_config &conf) override { return false; }
    bool Debug_Ui_State(std::optional<bool>) override { return false; }
    void Shutdown() override {}

    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    gfx::geometrybank_handle
        Create_Bank() override { return m_geometry.register_bank(std::make_unique<null_geometrybank>()); }
    // creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    gfx::geometry_handle
        Insert( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) override { return m_geometry.create_chunk( Indices, Vertices, Geometry, Type ); }
    // creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    gfx::geometry_handle
        Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) override {
                gfx::calculate_tangents(Vertices, gfx::index_array(), Type);

                return m_geometry.create_chunk(Vertices, Geometry, Type);
            }
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset = 0 ) override {
            gfx::calculate_tangents(Vertices, gfx::index_array(), Type);

            return m_geometry.replace(Vertices, Geometry, Offset);
        }
    // adds supplied vertex data at the end of specified chunk
    bool
        Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type ) override {
            gfx::calculate_tangents(Vertices, gfx::index_array(), Type);

            return m_geometry.append(Vertices, Geometry);
        }
    // provides direct access to index data of specfied chunk
    gfx::index_array const &
        Indices( gfx::geometry_handle const &Geometry ) const override { return m_geometry.indices(Geometry); }
    // provides direct access to vertex data of specfied chunk
    gfx::vertex_array const &
        Vertices( gfx::geometry_handle const &Geometry ) const override { return m_geometry.vertices(Geometry); }
    // material methods
    material_handle
        Fetch_Material( std::string const &Filename, bool const Loadnow = true ) override {
                m_materials.push_back(std::make_shared<opengl_material>());
                m_materials.back()->name = Filename;
                return m_materials.size();
            }
    void
        Bind_Material( material_handle const Material, TSubModel const *sm = nullptr, lighting_data const *lighting = nullptr ) override {}
    opengl_material const &
        Material( material_handle const Material ) const override { return *m_materials.at(Material - 1); }
    // shader methods
    auto Fetch_Shader( std::string const &name ) -> std::shared_ptr<gl::program> override { throw std::runtime_error("not impl"); }
    // texture methods
    texture_handle
        Fetch_Texture( std::string const &Filename, bool const Loadnow = true, GLint format_hint = GL_SRGB_ALPHA ) override { throw std::runtime_error("not impl"); }
    void
        Bind_Texture( texture_handle const Texture ) override {}
    void
        Bind_Texture( std::size_t const Unit, texture_handle const Texture ) override {}
    opengl_texture &
        Texture( texture_handle const Texture ) override { throw std::runtime_error("not impl"); }
    opengl_texture const &
        Texture( texture_handle const Texture ) const override { throw std::runtime_error("not impl"); }
    // utility methods
    void
        Pick_Control_Callback( std::function<void( TSubModel const *, const glm::vec2  )> Callback ) override {}
    void
        Pick_Node_Callback( std::function<void( scene::basic_node * )> Callback ) override {}
    TSubModel const *
        Pick_Control() const override { return nullptr; }
    scene::basic_node const *
        Pick_Node() const override { return nullptr; }
    glm::dvec3
        Mouse_Position() const override { return glm::dvec3(); }
    // maintenance methods
    void
        Update( double const Deltatime ) override {}
    void
        Update_Pick_Control() override {}
    void
        Update_Pick_Node() override {}
    glm::dvec3
        Update_Mouse_Position() override { return glm::dvec3(); }
    // debug methods
    std::string const &
        info_times() const override { return empty_str; }
    std::string const &
        info_stats() const override { return empty_str; }

    static std::unique_ptr<gfx_renderer> create_func();

    static bool renderer_register;

private:
    std::string empty_str;
    gfx::geometrybank_manager m_geometry;
    std::vector<std::shared_ptr<opengl_material>> m_materials;
};
