/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "opengl/glew.h"
#include "texture.h"
#include "dumb3d.h"

struct opengl_light {

    GLuint id{ -1 };
    Math3D::vector3 direction;
    GLfloat position[ 4 ]; // 4th parameter specifies directional(0) or omni-directional(1) light source
    GLfloat ambient[ 4 ];
    GLfloat diffuse[ 4 ];
    GLfloat specular[ 4 ];

    opengl_light() {
        position[ 0 ] = position[ 1 ] = position[ 2 ] = 0.0f; position[ 3 ] = 1.0f;
        ambient[ 0 ] = ambient[ 1 ] = ambient[ 2 ] = ambient[ 3 ] = 1.0f;
        diffuse[ 0 ] = diffuse[ 1 ] = diffuse[ 2 ] = diffuse[ 3 ] = 1.0f;
        specular[ 0 ] = specular[ 1 ] = specular[ 2 ] = specular[ 3 ] = 1.0f;
    }

    inline
    void apply_intensity() {

        glLightfv( id, GL_AMBIENT, ambient );
        glLightfv( id, GL_DIFFUSE, diffuse );
        glLightfv( id, GL_SPECULAR, specular );
    }
    inline
    void apply_angle() {

        glLightfv( id, GL_POSITION, position );
        if( position[ 3 ] == 1.0f ) {
            GLfloat directionarray[] = { direction.x, direction.y, direction.z };
            glLightfv( id, GL_SPOT_DIRECTION, directionarray );
        }
    }
};

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
    GLenum static const sunlight{ GL_LIGHT0 };
//    GLenum static const ambientlight{ GL_LIGHT1 };

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

private:
    enum class rendermode {
        color
    };

    rendermode renderpass{ rendermode::color };

    texture_manager m_textures;
};

extern opengl_renderer GfxRenderer;

//---------------------------------------------------------------------------
