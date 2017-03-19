/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "GL/glew.h"
#include "texture.h"
#include "lightarray.h"
#include "dumb3d.h"

// encapsulates basic rendering setup.
// for modern opengl this translates to a specific collection of glsl shaders,
// for legacy opengl this is combination of blending modes, active texture units etc
struct opengl_technique {

};

// a collection of parameters for the rendering setup.
// for modern opengl this translates to set of attributes for the active shaders,
// for legacy opengl this is basically just texture(s) assigned to geometry
struct opengl_material {

};

// bare-bones render controller, in lack of anything better yet
class opengl_renderer {

public:
// types

// methods
    void
        Init();

    void
        Update_Lights( light_array const &Lights );

    void
        Disable_Lights();

    texture_manager::size_type
        GetTextureId( std::string Filename, std::string const &Dir, int const Filter = -1, bool const Loadnow = true ) {

            return m_textures.GetTextureId( Filename, Dir, Filter, Loadnow );
        }

    void
        Bind( texture_manager::size_type const Id ) {
            // temporary until we separate the renderer
            m_textures.Bind( Id );
        }

    opengl_texture &
        Texture( texture_manager::size_type const Id ) {
        
            return m_textures.Texture( Id );
        }

// members
    GLenum static const sunlight{ 0 };

private:
// types
    enum class rendermode {
        color
    };
    
// members
    rendermode renderpass{ rendermode::color };
    texture_manager m_textures;
};

extern opengl_renderer GfxRenderer;

//---------------------------------------------------------------------------
