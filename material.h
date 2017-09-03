/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "texture.h"
#include "parser.h"

typedef int material_handle;

// a collection of parameters for the rendering setup.
// for modern opengl this translates to set of attributes for the active shaders,
// for legacy opengl this is basically just texture(s) assigned to geometry
struct opengl_material {

    texture_handle texture1 { null_handle }; // primary texture, typically diffuse+apha
    texture_handle texture2 { null_handle }; // secondary texture, typically normal+reflection

    bool has_alpha { false }; // alpha state, calculated from presence of alpha in texture1
    std::string name;
// methods:
    bool
        deserialize( cParser &Input, bool const Loadnow );
private:
    // imports member data pair from the config file
    bool
        deserialize_mapping( cParser &Input, bool const Loadnow );
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
    std::string
        find_on_disk( std::string const &Materialname ) const;
// members:
    material_handle const npos { -1 };
    material_sequence m_materials;
    index_map m_materialmappings;

};

//---------------------------------------------------------------------------
