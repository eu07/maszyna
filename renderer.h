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
#include "material.h"
#include "light.h"
#include "lightarray.h"
#include "dumb3d.h"
#include "frustum.h"
#include "scene.h"
#include "simulationenvironment.h"
#include "particles.h"
#include "MemCell.h"

#define EU07_USE_PICKING_FRAMEBUFFER
//#define EU07_USE_DEBUG_SHADOWMAP
//#define EU07_USE_DEBUG_CABSHADOWMAP
//#define EU07_USE_DEBUG_CAMERA
//#define EU07_USE_DEBUG_SOUNDEMITTERS
//#define EU07_USEIMGUIIMPLOPENGL2
//#define EU07_DISABLECABREFLECTIONS

class gfx_renderer {

public:
// types
// constructors
// destructor
    virtual ~gfx_renderer() {}
// methods
    virtual auto Init( GLFWwindow *Window ) -> bool = 0;
    // main draw call. returns false on error
    virtual auto Render() -> bool = 0;
    virtual auto Framerate() -> float = 0;
    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    virtual auto Create_Bank() -> gfx::geometrybank_handle = 0;
    // creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
    virtual auto Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) -> gfx::geometry_handle = 0;
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    virtual auto Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 ) -> bool = 0;
    // adds supplied vertex data at the end of specified chunk
    virtual auto Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) -> bool = 0;
    // provides direct access to vertex data of specfied chunk
    virtual auto Vertices( gfx::geometry_handle const &Geometry ) const ->gfx::vertex_array const & = 0;
    // material methods
    virtual auto Fetch_Material( std::string const &Filename, bool const Loadnow = true ) -> material_handle = 0;
    virtual void Bind_Material( material_handle const Material ) = 0;
    virtual auto Material( material_handle const Material ) const -> opengl_material const & = 0;
    // texture methods
    virtual auto Fetch_Texture( std::string const &Filename, bool const Loadnow = true ) -> texture_handle = 0;
    virtual void Bind_Texture( texture_handle const Texture ) = 0;
    virtual auto Texture( texture_handle const Texture ) const -> opengl_texture const & = 0;
    // utility methods
    virtual auto Pick_Control() const -> TSubModel const * = 0;
    virtual auto Pick_Node() const -> scene::basic_node const * = 0;
    virtual auto Mouse_Position() const -> glm::dvec3 = 0;
    // maintenance methods
    virtual void Update( double const Deltatime ) = 0;
    virtual auto Update_Pick_Control() -> TSubModel * = 0;
    virtual auto Update_Pick_Node() -> scene::basic_node * = 0;
    virtual auto Update_Mouse_Position() -> glm::dvec3 = 0;
    // debug methods
    virtual auto info_times() const -> std::string const & = 0;
    virtual auto info_stats() const -> std::string const & = 0;
};

struct opengl_light : public basic_light {

    GLuint id { (GLuint)-1 };

    void
        apply_intensity( float const Factor = 1.0f );
    void
        apply_angle();

    opengl_light &
        operator=( basic_light const &Right ) {
            basic_light::operator=( Right );
            return *this; }
};



// encapsulates basic rendering setup.
// for modern opengl this translates to a specific collection of glsl shaders,
// for legacy opengl this is combination of blending modes, active texture units etc
struct opengl_technique {

};



// simple camera object. paired with 'virtual camera' in the scene
class opengl_camera {

public:
// constructors
    opengl_camera() = default;
// methods:
    inline
    void
        update_frustum() { update_frustum( m_projection, m_modelview ); }
    void
        update_frustum( glm::mat4 const &Projection, glm::mat4 const &Modelview );
    bool
        visible( scene::bounding_area const &Area ) const;
    bool
        visible( TDynamicObject const *Dynamic ) const;
    inline
    glm::dvec3 const &
        position() const { return m_position; }
    inline
    glm::dvec3 &
        position() { return m_position; }
    inline
    glm::mat4 const &
        projection() const { return m_projection; }
    inline
    glm::mat4 &
        projection() { return m_projection; }
    inline
    glm::mat4 const &
        modelview() const { return m_modelview; }
    inline
    glm::mat4 &
        modelview() { return m_modelview; }
    inline
    std::vector<glm::vec4> &
        frustum_points() { return m_frustumpoints; }
    // transforms provided set of clip space points to world space
    template <class Iterator_>
    void
        transform_to_world( Iterator_ First, Iterator_ Last ) const {
            std::for_each(
                First, Last,
                [this]( glm::vec4 &point ) {
                    // transform each point using the cached matrix...
                    point = this->m_inversetransformation * point;
                    // ...and scale by transformed w
                    point = glm::vec4{ glm::vec3{ point } / point.w, 1.f }; } ); }
    // debug helper, draws shape of frustum in world space
    void
        draw( glm::vec3 const &Offset ) const;

private:
// members:
    cFrustum m_frustum;
    std::vector<glm::vec4> m_frustumpoints; // visualization helper; corners of defined frustum, in world space
    glm::dvec3 m_position;
    glm::mat4 m_projection;
    glm::mat4 m_modelview;
    glm::mat4 m_inversetransformation; // cached transformation to world space
};


// particle data visualizer
class opengl_particles {
public:
// constructors
    opengl_particles() = default;
// destructor
    ~opengl_particles() {
        if( m_buffer != 0 ) {
            ::glDeleteBuffers( 1, &m_buffer ); } }
// methods
    void
        update( opengl_camera const &Camera );
    void
        render( int const Textureunit );
private:
// types
    struct particle_vertex {
        glm::vec3 position; // 3d space
        std::uint8_t color[ 4 ]; // rgba, unsigned byte format
        glm::vec2 texture; // uv space
        float padding[ 2 ]; // experimental, some gfx hardware allegedly works better with 32-bit aligned data blocks
    };
/*
    using sourcedistance_pair = std::pair<smoke_source *, float>;
    using source_sequence = std::vector<sourcedistance_pair>;
*/
    using particlevertex_sequence = std::vector<particle_vertex>;
// methods
// members
/*
    source_sequence m_sources; // list of particle sources visible in current render pass, with their respective distances to the camera
*/
    particlevertex_sequence m_particlevertices; // geometry data of visible particles, generated on the cpu end
    GLuint m_buffer{ (GLuint)-1 }; // id of the buffer holding geometry data on the opengl end
    std::size_t m_buffercapacity{ 0 }; // total capacity of the last established buffer
};



// bare-bones render controller, in lack of anything better yet
class opengl_renderer : public gfx_renderer {

public:
// types
// constructors
    opengl_renderer() = default;
// destructor
    ~opengl_renderer() { gluDeleteQuadric( m_quadric ); }
// methods
    bool
        Init( GLFWwindow *Window ) override;
    // main draw call. returns false on error
    bool
        Render() override;
    inline
    float
        Framerate() override { return m_framerate; }
    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    gfx::geometrybank_handle
        Create_Bank() override;
    // creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
    gfx::geometry_handle
        Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) override;
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 ) override;
    // adds supplied vertex data at the end of specified chunk
    bool
        Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) override;
    // provides direct access to vertex data of specfied chunk
    gfx::vertex_array const &
        Vertices( gfx::geometry_handle const &Geometry ) const override;
    // material methods
    material_handle
        Fetch_Material( std::string const &Filename, bool const Loadnow = true ) override;
    void
        Bind_Material( material_handle const Material ) override;
    opengl_material const &
        Material( material_handle const Material ) const override;
    // texture methods
    texture_handle
        Fetch_Texture( std::string const &Filename, bool const Loadnow = true ) override;
    void
        Bind_Texture( texture_handle const Texture );
    opengl_texture const &
        Texture( texture_handle const Texture ) const override;
    // utility methods
    TSubModel const *
        Pick_Control() const override { return m_pickcontrolitem; }
    scene::basic_node const *
        Pick_Node() const override { return m_picksceneryitem; }
    glm::dvec3
        Mouse_Position() const override { return m_worldmousecoordinates; }
    // maintenance methods
    void
        Update( double const Deltatime ) override;
    TSubModel *
        Update_Pick_Control() override;
    scene::basic_node *
        Update_Pick_Node() override;
    glm::dvec3
        Update_Mouse_Position() override;
    // debug methods
    std::string const &
        info_times() const override;
    std::string const &
        info_stats() const override;

// members
    GLenum static const sunlight { GL_LIGHT0 };
    std::size_t m_drawcount { 0 };

private:
// types
    enum class rendermode {
        none,
        color,
        shadows,
        cabshadows,
        reflections,
        pickcontrols,
        pickscenery
    };

    enum textureunit {
        helper = 0,
        shadows,
        normals,
        diffuse
    };

    struct debug_stats {
        int dynamics { 0 };
        int models { 0 };
        int submodels { 0 };
        int paths { 0 };
        int traction { 0 };
        int shapes { 0 };
        int lines { 0 };
        int drawcalls { 0 };
    };

    using section_sequence = std::vector<scene::basic_section *>;
    using distancecell_pair = std::pair<double, scene::basic_cell *>;
    using cell_sequence = std::vector<distancecell_pair>;

    struct renderpass_config {

        opengl_camera camera;
        rendermode draw_mode { rendermode::none };
        float draw_range { 0.0f };
    };

    struct units_state {

        bool diffuse { false };
        bool shadows { false };
        bool reflections { false };
    };

    typedef std::vector<opengl_light> opengllight_array;

// methods
    void
        Disable_Lights();
    void
        setup_pass( renderpass_config &Config, rendermode const Mode, float const Znear = 0.f, float const Zfar = 1.f, bool const Ignoredebug = false );
    void
        setup_matrices();
    void
        setup_drawing( bool const Alpha = false );
    void
        setup_units( bool const Diffuse, bool const Shadows, bool const Reflections );
    void
        setup_shadow_map( GLuint const Texture, glm::mat4 const &Transformation );
    void
        setup_shadow_color( glm::vec4 const &Shadowcolor );
    void
        setup_environment_light( TEnvironmentType const Environment = e_flat );
    void
        switch_units( bool const Diffuse, bool const Shadows, bool const Reflections );
    // helper, texture manager method; activates specified texture unit
    void
        select_unit( GLint const Textureunit );
    // runs jobs needed to generate graphics for specified render pass
    void
        Render_pass( rendermode const Mode );
    // creates dynamic environment cubemap
    bool
        Render_reflections();
    bool
        Render( world_environment *Environment );
    void
        Render( scene::basic_region *Region );
    void
        Render( section_sequence::iterator First, section_sequence::iterator Last );
    void
        Render( cell_sequence::iterator First, cell_sequence::iterator Last );
    void
        Render( scene::shape_node const &Shape, bool const Ignorerange );
    void
        Render( TAnimModel *Instance );
    bool
        Render( TDynamicObject *Dynamic );
    bool
        Render( TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &Angle );
    bool
        Render( TModel3d *Model, material_data const *Material, float const Squaredistance );
    void
        Render( TSubModel *Submodel );
    void
        Render( TTrack *Track );
    void
        Render( scene::basic_cell::path_sequence::const_iterator First, scene::basic_cell::path_sequence::const_iterator Last );
    bool
        Render_cab( TDynamicObject const *Dynamic, float const Lightlevel, bool const Alpha = false );
    void
        Render( TMemCell *Memcell );
    void
        Render_particles();
    void
        Render_precipitation();
    void
        Render_Alpha( scene::basic_region *Region );
    void
        Render_Alpha( cell_sequence::reverse_iterator First, cell_sequence::reverse_iterator Last );
    void
        Render_Alpha( TAnimModel *Instance );
    void
        Render_Alpha( TTraction *Traction );
    void
        Render_Alpha( scene::lines_node const &Lines );
    bool
        Render_Alpha( TDynamicObject *Dynamic );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &Angle );
    bool
        Render_Alpha( TModel3d *Model, material_data const *Material, float const Squaredistance );
    void
        Render_Alpha( TSubModel *Submodel );
    void
        Update_Lights( light_array &Lights );
    bool
        Init_caps();
    glm::vec3
        pick_color( std::size_t const Index );
    std::size_t
        pick_index( glm::ivec3 const &Color );

// members
    GLFWwindow *m_window { nullptr };
    gfx::geometrybank_manager m_geometry;
    material_manager m_materials;
    texture_manager m_textures;
    opengl_light m_sunlight;
    opengllight_array m_lights;
/*
    float m_sunandviewangle; // cached dot product of sunlight and camera vectors
*/
    gfx::geometry_handle m_billboardgeometry { 0, 0 };
    texture_handle m_glaretexture { -1 };
    texture_handle m_suntexture { -1 };
    texture_handle m_moontexture { -1 };
    texture_handle m_reflectiontexture { -1 };
    texture_handle m_smoketexture { -1 };
    GLUquadricObj *m_quadric { nullptr }; // helper object for drawing debug mode scene elements
    // TODO: refactor framebuffer stuff into an object
    bool m_framebuffersupport { false };
#ifdef EU07_USE_PICKING_FRAMEBUFFER
    GLuint m_pickframebuffer { 0 };
    GLuint m_picktexture { 0 };
    GLuint m_pickdepthbuffer { 0 };
#endif
    // main shadowmap resources
    int m_shadowbuffersize { 2048 };
    GLuint m_shadowframebuffer { 0 };
    GLuint m_shadowtexture { 0 };
#ifdef EU07_USE_DEBUG_SHADOWMAP
    GLuint m_shadowdebugtexture{ 0 };
#endif
#ifdef EU07_USE_DEBUG_CABSHADOWMAP
    GLuint m_cabshadowdebugtexture{ 0 };
#endif
    glm::mat4 m_shadowtexturematrix; // conversion from camera-centric world space to light-centric clip space
    // cab shadowmap resources
    GLuint m_cabshadowframebuffer { 0 };
    GLuint m_cabshadowtexture { 0 };
    glm::mat4 m_cabshadowtexturematrix; // conversion from cab-centric world space to light-centric clip space
    // environment map resources
    GLuint m_environmentframebuffer { 0 };
    GLuint m_environmentcubetexture { 0 };
    GLuint m_environmentdepthbuffer { 0 };
    bool m_environmentcubetexturesupport { false }; // indicates whether we can use the dynamic environment cube map
    int m_environmentcubetextureface { 0 }; // helper, currently processed cube map face
    int m_environmentupdatetime { 0 }; // time of the most recent environment map update
    glm::dvec3 m_environmentupdatelocation; // coordinates of most recent environment map update
    // particle visualization subsystem
    opengl_particles m_particlerenderer;

    int m_helpertextureunit { GL_TEXTURE0 };
    int m_shadowtextureunit { GL_TEXTURE1 };
    int m_normaltextureunit { GL_TEXTURE2 };
    int m_diffusetextureunit{ GL_TEXTURE3 };
    units_state m_unitstate;

    unsigned int m_framestamp; // id of currently rendered gfx frame
    float m_framerate;
    double m_updateaccumulator { 0.0 };
//    double m_pickupdateaccumulator { 0.0 };
    std::string m_debugtimestext;
    std::string m_pickdebuginfo;
    debug_stats m_debugstats;
    std::string m_debugstatstext;

    glm::vec4 m_baseambient { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::vec4 m_shadowcolor { colors::shadow };
    float m_fogrange { 2000.f };
//    TEnvironmentType m_environment { e_flat };
    float m_specularopaquescalefactor { 1.f };
    float m_speculartranslucentscalefactor { 1.f };
    bool m_renderspecular{ false }; // controls whether to include specular component in the calculations

    renderpass_config m_renderpass; // parameters for current render pass
    section_sequence m_sectionqueue; // list of sections in current render pass
    cell_sequence m_cellqueue;
    renderpass_config m_colorpass; // parametrs of most recent color pass
    renderpass_config m_shadowpass; // parametrs of most recent shadowmap pass
    renderpass_config m_cabshadowpass; // parameters of most recent cab shadowmap pass
    std::vector<TSubModel *> m_pickcontrolsitems;
    TSubModel *m_pickcontrolitem { nullptr };
    std::vector<scene::basic_node *> m_picksceneryitems;
    scene::basic_node *m_picksceneryitem { nullptr };
    glm::vec3 m_worldmousecoordinates { 0.f };
#ifdef EU07_USE_DEBUG_CAMERA
    renderpass_config m_worldcamera; // debug item
#endif
};

extern std::unique_ptr<gfx_renderer> GfxRenderer;

//---------------------------------------------------------------------------
