/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "renderer.h"
#include "opengllight.h"
#include "openglcamera.h"
#include "openglparticles.h"
#include "openglskydome.h"
#include "openglprecipitation.h"
#include "lightarray.h"
#include "scene.h"
#include "simulationenvironment.h"
#include "MemCell.h"

#define EU07_USE_PICKING_FRAMEBUFFER
//#define EU07_USE_DEBUG_SHADOWMAP
//#define EU07_USE_DEBUG_CABSHADOWMAP
//#define EU07_USE_DEBUG_CAMERA
//#define EU07_USE_DEBUG_SOUNDEMITTERS
//#define EU07_DISABLECABREFLECTIONS

// bare-bones render controller, in lack of anything better yet
class opengl_renderer : public gfx_renderer {

public:
// types
// constructors
    opengl_renderer() = default;
// destructor
    ~opengl_renderer() { }
// methods
    bool
        Init( GLFWwindow *Window ) override;
    // main draw call. returns false on error
    bool
        Render() override;
    void
        SwapBuffers() override;
    inline
    float
        Framerate() override { return m_framerate; }

    bool AddViewport(const global_settings::extraviewport_config &conf) override { return false; }
    bool Debug_Ui_State(std::optional<bool>) override { return false; }
    void Shutdown() override {}

    // geometry methods
    // NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
    // creates a new geometry bank. returns: handle to the bank or NULL
    gfx::geometrybank_handle
        Create_Bank() override;
    // creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    gfx::geometry_handle
        Insert( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) override;
    // creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    gfx::geometry_handle
        Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) override;
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset = 0 ) override;
    // adds supplied vertex data at the end of specified chunk
    bool
        Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type ) override;
    // provides direct access to index data of specfied chunk
    gfx::index_array const &
        Indices( gfx::geometry_handle const &Geometry ) const override;
    // provides direct access to vertex data of specfied chunk
    gfx::vertex_array const &
        Vertices( gfx::geometry_handle const &Geometry ) const override;
    // material methods
    material_handle
        Fetch_Material( std::string const &Filename, bool const Loadnow = true ) override;
    void
        Bind_Material( material_handle const Material, TSubModel const *sm = nullptr, lighting_data const *lighting = nullptr ) override;
    opengl_material const &
        Material( material_handle const Material ) const override;
    // shader methods
    auto Fetch_Shader( std::string const &name ) -> std::shared_ptr<gl::program> override;
    // texture methods
    texture_handle
        Fetch_Texture( std::string const &Filename, bool const Loadnow = true, GLint format_hint = GL_SRGB_ALPHA ) override;
    void
        Bind_Texture( texture_handle const Texture ) override;
    void
        Bind_Texture( std::size_t const Unit, texture_handle const Texture ) override;
    opengl_texture &
        Texture( texture_handle const Texture ) override;
    opengl_texture const &
        Texture( texture_handle const Texture ) const override;
    // utility methods
    void
        Pick_Control_Callback( std::function<void( TSubModel const *, const glm::vec2  )> Callback ) override;
    void
        Pick_Node_Callback( std::function<void( scene::basic_node * )> Callback ) override;
    TSubModel const *
        Pick_Control() const override { return m_pickcontrolitem; }
    scene::basic_node const *
        Pick_Node() const override { return m_picksceneryitem; }
    glm::dvec3
        Mouse_Position() const override { return m_worldmousecoordinates; }
    // maintenance methods
    void
        Update( double const Deltatime ) override;
    void
        Update_Pick_Control() override;
    void
        Update_Pick_Node() override;
    glm::dvec3
        Update_Mouse_Position() override;
    // debug methods
    std::string const &
        info_times() const override;
    std::string const &
        info_stats() const override;

    opengl_material const & Material( TSubModel const * Submodel ) const;

// members
    GLenum static const sunlight { GL_LIGHT0 };

    static std::unique_ptr<gfx_renderer> create_func();

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
        int particles { 0 };
        int drawcalls { 0 };
    };

    using section_sequence = std::vector<scene::basic_section *>;
    using distancecell_pair = std::pair<double, scene::basic_cell *>;
    using cell_sequence = std::vector<distancecell_pair>;

    struct renderpass_config {

        opengl_camera camera;
        rendermode draw_mode { rendermode::none };
        float draw_range { 0.0f };
        debug_stats draw_stats;
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
    bool
        Render_interior( bool const Alpha = false );
    bool
        Render_lowpoly( TDynamicObject *Dynamic, float const Squaredistance, bool const Setup, bool const Alpha = false );
    bool
        Render_coupler_adapter( TDynamicObject *Dynamic, float const Squaredistance, int const End, bool const Alpha = false );
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
    opengl_skydome m_skydomerenderer;
    opengl_precipitation m_precipitationrenderer;
    opengl_particles m_particlerenderer; // particle visualization subsystem
/*
    float m_sunandviewangle; // cached dot product of sunlight and camera vectors
*/
    gfx::geometry_handle m_billboardgeometry { 0, 0 };
    texture_handle m_glaretexture { -1 };
    texture_handle m_suntexture { -1 };
    texture_handle m_moontexture { -1 };
    texture_handle m_reflectiontexture { -1 };
    texture_handle m_smoketexture { -1 };
    material_handle m_invalid_material;
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
    double m_environmentupdatetime { 0.0 }; // time of the most recent environment map update
    glm::dvec3 m_environmentupdatelocation; // coordinates of most recent environment map update

    int m_helpertextureunit { 0 };
    int m_shadowtextureunit { 1 };
    int m_normaltextureunit { 2 };
    int m_diffusetextureunit{ 3 };
    units_state m_unitstate;

    unsigned int m_framestamp; // id of currently rendered gfx frame
    float m_framerate;
    double m_updateaccumulator { 0.0 };
//    double m_pickupdateaccumulator { 0.0 };
    std::string m_debugtimestext;
    std::string m_pickdebuginfo;
    std::string m_debugstatstext;
    struct simulation_state {
        std::string weather;
        std::string season;
    } m_simulationstate;

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
    std::vector<std::function<void( TSubModel const *, const glm::vec2 )>> m_control_pick_requests;
    std::vector<std::function<void( scene::basic_node * )>> m_node_pick_requests;
#ifdef EU07_USE_DEBUG_CAMERA
    renderpass_config m_worldcamera; // debug item
#endif

    static bool renderer_register;
};

//---------------------------------------------------------------------------
