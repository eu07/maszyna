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
    std::array<texture_handle, gl::MAX_TEXTURES> textures = { null_handle };
    std::array<glm::vec4, gl::MAX_PARAMS> params;
    std::vector<gl::shader::param_entry> params_state;

    std::shared_ptr<gl::program> shader;
    std::optional<float> opacity;
    std::optional<float> selfillum;
    float glossiness { 10.f };
    int shadow_rank { 0 }; // priority as shadow caster; higher = more likely to be skipped

    std::string name;
    glm::vec2 size { -1.f, -1.f }; // 'physical' size of bound texture, in meters

// constructors
    opengl_material();

// methods
    bool
        deserialize( cParser &Input, bool const Loadnow );
    void finalize(bool Loadnow);
    bool update();
    float get_or_guess_opacity() const;
    bool is_translucent() const;
// members
    static struct path_data {
        std::unordered_map<std::string, int> index_map;
        std::vector<std::string> data;
    } paths;
    bool is_good { false }; // indicates material was compiled without failure
    int path{ -1 }; // index to material path
    bool update_on_weather_change{ false };
    bool update_on_season_change{ false };

private:
// methods
    // imports member data pair from the config file
    bool
        deserialize_mapping( cParser &Input, int const Priority, bool const Loadnow );
        void log_error(const std::string &str);

// members
    // priorities for textures, shader, opacity
    int m_shader_priority = -1;
    int m_opacity_priority = -1;
    int m_selfillum_priority = -1;
    int m_glossiness_priority = -1;

    struct parse_info_s
    {
        struct tex_def
        {
            std::string name;
            int priority;
        };
        struct param_def
        {
            glm::vec4 data;
            int priority;
        };
        std::unordered_map<std::string, tex_def> tex_mapping;
        std::unordered_map<std::string, param_def> param_mapping;
    };
    std::unique_ptr<parse_info_s> parse_info;
};

class material_manager {

public:
    material_manager() { m_materials.emplace_back( opengl_material() ); } // empty bindings for null material

    material_handle
        create( std::string const &Filename, bool const Loadnow );
    opengl_material const &
        material( material_handle const Material ) const { return m_materials[ Material ]; }
    opengl_material &
        material( material_handle const Material ) { return m_materials[ Material ]; }
    // material updates executed when environment changes
    // TODO: registerable callbacks in environment manager
    void
        on_weather_change();
    void
        on_season_change();

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
