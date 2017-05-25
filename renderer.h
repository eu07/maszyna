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
#include "frustum.h"
#include "world.h"
#include "memcell.h"

struct opengl_light {

    GLuint id{ (GLuint)-1 };
    Math3D::vector3 direction;
    GLfloat position[ 4 ]; // 4th parameter specifies directional(0) or omni-directional(1) light source
    GLfloat ambient[ 4 ];
    GLfloat diffuse[ 4 ];
    GLfloat specular[ 4 ];

    opengl_light() {
        position[ 0 ] = position[ 1 ] = position[ 2 ] = 0.0f; position[ 3 ] = 1.0f; // 0,0,0,1
        ambient[ 0 ] = ambient[ 1 ] = ambient[ 2 ] = 0.0f; ambient[ 3 ] = 1.0f; // 0,0,0,1
        diffuse[ 0 ] = diffuse[ 1 ] = diffuse[ 2 ] = diffuse[ 3 ] = 1.0f; // 1,1,1,1
        specular[ 0 ] = specular[ 1 ] = specular[ 2 ] = specular[ 3 ] = 1.0f; // 1,1,1,1
    }

    inline
    void apply_intensity( float const Factor = 1.0f ) {

        if( Factor == 1.0 ) {

            glLightfv( id, GL_AMBIENT, ambient );
            glLightfv( id, GL_DIFFUSE, diffuse );
            glLightfv( id, GL_SPECULAR, specular );
        }
        else {
            // temporary light scaling mechanics (ultimately this work will be left to the shaders
            float4 scaledambient( ambient[ 0 ] * Factor, ambient[ 1 ] * Factor, ambient[ 2 ] * Factor, ambient[ 3 ] );
            float4 scaleddiffuse( diffuse[ 0 ] * Factor, diffuse[ 1 ] * Factor, diffuse[ 2 ] * Factor, diffuse[ 3 ] );
            float4 scaledspecular( specular[ 0 ] * Factor, specular[ 1 ] * Factor, specular[ 2 ] * Factor, specular[ 3 ] );
            glLightfv( id, GL_AMBIENT, &scaledambient.x );
            glLightfv( id, GL_DIFFUSE, &scaleddiffuse.x );
            glLightfv( id, GL_SPECULAR, &scaledspecular.x );
        }
    }
    inline
    void apply_angle() {

        glLightfv( id, GL_POSITION, position );
        if( position[ 3 ] == 1.0f ) {
            GLfloat directionarray[] = { (GLfloat)direction.x, (GLfloat)direction.y, (GLfloat)direction.z };
            glLightfv( id, GL_SPOT_DIRECTION, directionarray );
        }
    }
    inline
    void set_position( Math3D::vector3 const &Position ) {

        position[ 0 ] = Position.x;
        position[ 1 ] = Position.y;
        position[ 2 ] = Position.z;
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

// simple camera object. paired with 'virtual camera' in the scene
class opengl_camera {

public:
// methods:
    inline
    void
        update_frustum() { m_frustum.calculate(); }
    inline
    void
        update_frustum(glm::mat4 const &Projection, glm::mat4 const &Modelview) { m_frustum.calculate(Projection, Modelview); }
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

// destructor
    ~opengl_renderer() { gluDeleteQuadric( m_quadric ); }

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
        Render( TGroundNode *Node );
    bool
        Render( TDynamicObject *Dynamic );
    bool
        Render( TModel3d *Model, material_data const *Material, double const Squaredistance );
    bool
        Render( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle );
    void
        Render( TSubModel *Submodel );
    void
        Render( TMemCell *Memcell );
    bool
        Render_Alpha( TGround *Ground );
    bool
        Render_Alpha( TGroundNode *Node );
    bool
        Render_Alpha( TDynamicObject *Dynamic );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, double const Squaredistance );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle );
    void
        Render_Alpha( TSubModel *Submodel );
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
    GLenum static const sunlight{ GL_LIGHT0 };

private:
// types
    enum class rendermode {
        color
    };

    typedef std::vector<opengl_light> opengllight_array;

// methods
    bool
        Init_caps();
    
// members
    rendermode renderpass{ rendermode::color };
    opengllight_array m_lights;
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
    GLUquadricObj *m_quadric; // helper object for drawing debug mode scene elements

};

extern opengl_renderer GfxRenderer;

//---------------------------------------------------------------------------
