/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "Texture.h"
#include "gl/shader.h"
#include "gl/ubo.h"

typedef int material_handle;

// a collection of parameters for the rendering setup.
// for modern opengl this translates to set of attributes for shaders
struct opengl_material {
    // primary texture, typically diffuse+apha
    // secondary texture, typically normal+reflection
    std::array<texture_handle, gl::MAX_TEXTURES> textures = { null_handle };
    std::array<glm::vec4, gl::MAX_PARAMS> params = { glm::vec4() };

    std::shared_ptr<gl::program> shader;
    float opacity = std::numeric_limits<float>::quiet_NaN();
    float selfillum = std::numeric_limits<float>::quiet_NaN();

    std::string name;

// constructors
    opengl_material() = default;

// methods
    bool
        deserialize( cParser &Input, bool const Loadnow );

private:
// methods
    // imports member data pair from the config file, overriding existing parameter values of lower priority
    bool
        deserialize_mapping( cParser &Input, int const Priority, bool const Loadnow );

// members
    // priorities for textures, shader, opacity
    int m_shader_priority = -1;
    int m_opacity_priority = -1;
    int m_selfillum_priority = -1;
    std::array<int, gl::MAX_TEXTURES> m_texture_priority = { -1 };
    std::array<int, gl::MAX_PARAMS> m_param_priority = { -1 };
};

class material_manager {

public:
    material_manager() { m_materials.emplace_back( opengl_material() ); } // empty bindings for null material

    material_handle
        create( std::string const &Filename, bool const Loadnow );
    opengl_material const &
        material( material_handle const Material ) const { return m_materials[ Material ]; }

private:
// types
    typedef std::vector<opengl_material> material_sequence;
    typedef std::unordered_map<std::string, std::size_t> index_map;
// methods:
    // checks whether specified texture is in the texture bank. returns texture id, or npos.
    material_handle
        find_in_databank( std::string const &Materialname ) const;
    // checks whether specified file exists. returns name of the located file, or empty string.
    std::pair<std::string, std::string>
        find_on_disk( std::string const &Materialname ) const;
// members:
    material_sequence m_materials;
    index_map m_materialmappings;

};

//---------------------------------------------------------------------------
