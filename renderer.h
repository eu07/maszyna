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
#include "openglgeometrybank.h"
#include "texture.h"
#include "lightarray.h"
#include "dumb3d.h"
#include "frustum.h"
#include "world.h"
#include "memcell.h"

struct opengl_light {

    GLuint id{ (GLuint)-1 };
    glm::vec3 direction;
    glm::vec4
        position { 0.0f, 0.0f, 0.0f, 1.0f }, // 4th parameter specifies directional(0) or omni-directional(1) light source
        ambient { 0.0f, 0.0f, 0.0f, 1.0f },
        diffuse { 1.0f, 1.0f, 1.0f, 1.0f },
        specular { 1.0f, 1.0f, 1.0f, 1.0f };

    inline
    void apply_intensity( float const Factor = 1.0f ) {

        if( Factor == 1.0 ) {

            glLightfv( id, GL_AMBIENT, glm::value_ptr(ambient) );
            glLightfv( id, GL_DIFFUSE, glm::value_ptr(diffuse) );
            glLightfv( id, GL_SPECULAR, glm::value_ptr(specular) );
        }
        else {
            // temporary light scaling mechanics (ultimately this work will be left to the shaders
            glm::vec4 scaledambient( ambient.r * Factor, ambient.g * Factor, ambient.b * Factor, ambient.a );
            glm::vec4 scaleddiffuse( diffuse.r * Factor, diffuse.g * Factor, diffuse.b * Factor, diffuse.a );
            glm::vec4 scaledspecular( specular.r * Factor, specular.g * Factor, specular.b * Factor, specular.a );
            glLightfv( id, GL_AMBIENT, glm::value_ptr(scaledambient) );
            glLightfv( id, GL_DIFFUSE, glm::value_ptr(scaleddiffuse) );
            glLightfv( id, GL_SPECULAR, glm::value_ptr(scaledspecular) );
        }
    }
    inline
    void apply_angle() {

        glLightfv( id, GL_POSITION, glm::value_ptr(position) );
        if( position.w == 1.0f ) {
            glLightfv( id, GL_SPOT_DIRECTION, glm::value_ptr(direction) );
        }
    }
    inline
    void set_position( glm::vec3 const &Position ) {

        position = glm::vec4( Position, position.w );
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
    inline
    glm::dvec3 const &
        position() const { return m_position; }
    inline
    glm::dvec3 &
        position() { return m_position; }

private:
// members:
    cFrustum m_frustum;
    glm::dvec3 m_position;
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
    // render sub-methods, temporarily exposed until we complete migrating render code to the renderer
    bool
        Render( TDynamicObject *Dynamic );
    bool
        Render( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle );
    bool
        Render( TModel3d *Model, material_data const *Material, double const Squaredistance );
    void
        Render( TSubModel *Submodel );
    bool
        Render_Alpha( TDynamicObject *Dynamic );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, double const Squaredistance );
    // maintenance jobs
    void
        Update( double const Deltatime );
    // debug performance string
    std::string const &
        Info() const;
    // light methods
    void
        Disable_Lights();
    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    geometrybank_handle
        Create_Bank();
    // creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
    geometry_handle
        Insert( vertex_array &Vertices, geometrybank_handle const &Geometry, int const Type );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        Replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset = 0 );
    // adds supplied vertex data at the end of specified chunk
    bool
        Append( vertex_array &Vertices, geometry_handle const &Geometry );
    // provides direct access to vertex data of specfied chunk
    vertex_array const &
        Vertices( geometry_handle const &Geometry ) const;
    // texture methods
    texture_handle
        GetTextureId( std::string Filename, std::string const &Dir, int const Filter = -1, bool const Loadnow = true );
    void
        Bind( texture_handle const Texture );
    opengl_texture const &
        Texture( texture_handle const Texture );

// members
    GLenum static const sunlight{ GL_LIGHT0 };
    std::size_t m_drawcount { 0 };

private:
// types
    enum class rendermode {
        color
    };

    typedef std::vector<opengl_light> opengllight_array;

    typedef std::pair< double, TSubRect * > distancesubcell_pair;

// methods
    bool
        Init_caps();
    bool
        Render( world_environment *Environment );
    bool
        Render( TGround *Ground );
    bool
        Render( TGroundRect *Groundcell );
    bool
        Render( TSubRect *Groundsubcell );
    bool
        Render( TGroundNode *Node );
    void
        Render( TTrack *Track );
    void
        Render( TMemCell *Memcell );
    bool
        Render_Alpha( TGround *Ground );
    bool
        Render_Alpha( TSubRect *Groundsubcell );
    bool
        Render_Alpha( TGroundNode *Node );
    void
        Render_Alpha( TSubModel *Submodel );
    void
        Update_Lights( light_array const &Lights );

// members
    opengllight_array m_lights;
    geometrybank_manager m_geometry;
    texture_manager m_textures;
    opengl_camera m_camera;
    rendermode renderpass { rendermode::color };
    float m_drawrange { 2500.0f }; // current drawing range
    float m_drawtime { 1000.0f / 30.0f * 20.0f }; // start with presumed 'neutral' average of 30 fps
    double m_updateaccumulator { 0.0 };
    std::string m_debuginfo;
    GLFWwindow *m_window { nullptr };
    texture_handle m_glaretexture { -1 };
    texture_handle m_suntexture { -1 };
    texture_handle m_moontexture { -1 };
    geometry_handle m_billboardgeometry { 0, 0 };
    GLUquadricObj *m_quadric; // helper object for drawing debug mode scene elements
    std::vector<distancesubcell_pair> m_drawqueue; // list of subcells to be drawn in current render pass
    glm::vec4 m_baseambient { 0.0f, 0.0f, 0.0f, 1.0f };
    bool m_renderspecular{ false }; // controls whether to include specular component in the calculations
    float m_specularopaquescalefactor { 1.0f };
    float m_speculartranslucentscalefactor { 1.0f };

};

extern opengl_renderer GfxRenderer;

//---------------------------------------------------------------------------
