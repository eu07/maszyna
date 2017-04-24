/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <GL/glew.h>
#include "texture.h"
#include "lightarray.h"
#include "dumb3d.h"
#include "frustum.h"
#include "ground.h"
#include "shader.h"
#include "World.h"

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

// simple camera object. paired with 'virtual camera' in the scene
class opengl_camera {

public:
// methods:
    inline
    void
        update_frustum() { m_frustum.calculate(); }
    inline
    void
        update_frustum(glm::mat4 &Projection, glm::mat4 &Modelview) { m_frustum.calculate(Projection, Modelview); }
    bool
        visible( bounding_area const &Area ) const;
    bool
        visible( TDynamicObject const *Dynamic ) const;

private:
// members:
    cFrustum m_frustum;
};

// bare-bones render controller, in lack of anything better yet
class opengl_renderer {

public:
// types

// methods
    bool
        Init( GLFWwindow *Window );
    // main draw call. returns false on error
    bool
        Render();
    bool
        Render( world_environment *Environment );
    bool
        Render( TGround *Ground );
    bool
        Render( TDynamicObject *Dynamic );
    bool
        Render( TModel3d *Model, material_data const *Material, double const Squaredistance );
    bool
        Render( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle );
    void
        Render( TSubModel *Submodel, glm::mat4 m );
	void Render(TSubModel *Submodel);
	void Render_Alpha(TSubModel *Submodel);
    bool
        Render_Alpha( TDynamicObject *Dynamic );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, double const Squaredistance );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle );
    void
        Render_Alpha( TSubModel *Submodel, glm::mat4 m);
    // maintenance jobs
    void
        Update( double const Deltatime);
    void
        Update_Lights( light_array const &Lights );
    void
        Disable_Lights();
    inline
    bool
        Visible( TDynamicObject const *Dynamic ) const { return m_camera.visible( Dynamic ); }
    // debug performance string
    std::string const &
        Info() const;

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

// methods
    bool
        Init_caps();
    
// members
    rendermode renderpass{ rendermode::color };
    texture_manager m_textures;
    opengl_camera m_camera;
    float m_drawrange{ 2500.0f }; // current drawing range
    float m_drawtime{ 1000.0f / 30.0f * 20.0f }; // start with presumed 'neutral' average of 30 fps
    double m_updateaccumulator{ 0.0 };
    std::string m_debuginfo;
    GLFWwindow *m_window{ nullptr };
    texture_manager::size_type m_glaretextureid{ -1 };
    texture_manager::size_type m_suntextureid{ -1 };
    texture_manager::size_type m_moontextureid{ -1 };
};

extern opengl_renderer GfxRenderer;

//---------------------------------------------------------------------------
