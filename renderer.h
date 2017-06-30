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
	gl_program_light shader;

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
    void Render( TSubModel *Submodel );
	void Render(TSubModel *Submodel, glm::mat4 m);
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
    opengl_texture &
        Texture( texture_handle const Texture );

// members
    GLenum static const sunlight{ 0 };
    std::size_t m_drawcount { 0 };

private:
// types
    enum class rendermode {
        color
    };

	typedef std::pair<double, TSubRect*> distancesubcell_pair;

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
		Render_Alpha(TSubModel *Submodel, glm::mat4 m);
    void
        Update_Lights( light_array const &Lights );

// members
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
