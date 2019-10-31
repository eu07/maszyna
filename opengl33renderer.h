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
#include "openglcamera.h"
#include "opengl33light.h"
#include "opengl33particles.h"
#include "opengl33skydome.h"
#include "opengl33precipitation.h"
#include "simulationenvironment.h"
#include "scene.h"
#include "MemCell.h"
#include "lightarray.h"
#include "gl/ubo.h"
#include "gl/framebuffer.h"
#include "gl/renderbuffer.h"
#include "gl/postfx.h"
#include "gl/shader.h"
#include "gl/cubemap.h"
#include "gl/glsl_common.h"
#include "gl/pbo.h"
#include "gl/query.h"

// bare-bones render controller, in lack of anything better yet
class opengl33_renderer : public gfx_renderer {

  public:
  // types
	/// Renderer runtime settings
	struct Settings
	{
		bool traction_debug { false };
	} settings;
// constructors
    opengl33_renderer() = default;
// destructor
    ~opengl33_renderer() {}
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
        Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset = 0 ) override;
    // adds supplied vertex data at the end of specified chunk
    bool
        Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type ) override;
    // provides direct access to vertex data of specfied chunk
    gfx::vertex_array const &
        Vertices( gfx::geometry_handle const &Geometry ) const override;
    // material methods
    material_handle
        Fetch_Material( std::string const &Filename, bool const Loadnow = true ) override;
    void
        Bind_Material( material_handle const Material, TSubModel *sm = nullptr ) override;
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
        Pick_Control_Callback( std::function<void( TSubModel const * )> Callback ) override;
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



    opengl_material & Material( material_handle const Material );
    void SwapBuffers();
    // draws supplied geometry handles
    void Draw_Geometry(std::vector<gfx::geometrybank_handle>::iterator begin, std::vector<gfx::geometrybank_handle>::iterator end);
	void Draw_Geometry(const gfx::geometrybank_handle &handle);
	// material methods
    void Bind_Material_Shadow(material_handle const Material);
	void Update_AnimModel(TAnimModel *model);

	// members
    GLenum static const sunlight{0};
	std::size_t m_drawcount{0};

	bool debug_ui_active = false;

  private:
	// types
	enum class rendermode
	{
		none,
		color,
		shadows,
		cabshadows,
		reflections,
		pickcontrols,
		pickscenery
	};

	struct debug_stats
	{
		int dynamics{0};
		int models{0};
		int submodels{0};
		int paths{0};
		int traction{0};
		int shapes{0};
		int lines{0};
		int drawcalls{0};
	};

	using section_sequence = std::vector<scene::basic_section *>;
	using distancecell_pair = std::pair<double, scene::basic_cell *>;
	using cell_sequence = std::vector<distancecell_pair>;

	struct renderpass_config
	{
		opengl_camera pass_camera;
		opengl_camera viewport_camera;
		rendermode draw_mode{rendermode::none};
		float draw_range{0.0f};
	};

	struct viewport_config {
		int width;
		int height;

		float draw_range;

		bool main = false;
		GLFWwindow *window = nullptr;

		glm::mat4 camera_transform;

		std::unique_ptr<gl::framebuffer> msaa_fb;
		std::unique_ptr<gl::renderbuffer> msaa_rbc;
		std::unique_ptr<gl::renderbuffer> msaa_rbv;
		std::unique_ptr<gl::renderbuffer> msaa_rbd;

		std::unique_ptr<gl::framebuffer> main_fb;
		std::unique_ptr<opengl_texture> main_texv;
		std::unique_ptr<opengl_texture> main_tex;

		std::unique_ptr<gl::framebuffer> main2_fb;
		std::unique_ptr<opengl_texture> main2_tex;
	};

	viewport_config *m_current_viewport = nullptr;

	typedef std::vector<opengl33_light> opengllight_array;

	// methods
    std::unique_ptr<gl::program> make_shader(std::string v, std::string f);
	bool Init_caps();
	void setup_pass(viewport_config &Viewport, renderpass_config &Config, rendermode const Mode, float const Znear = 0.f, float const Zfar = 1.f, bool const Ignoredebug = false);
	void setup_matrices();
    void setup_drawing(bool const Alpha = false);
    void setup_shadow_map(opengl_texture *tex, renderpass_config conf);
    void setup_env_map(gl::cubemap *tex);
	void setup_environment_light(TEnvironmentType const Environment = e_flat);
	// runs jobs needed to generate graphics for specified render pass
	void Render_pass(viewport_config &vp, rendermode const Mode);
	// creates dynamic environment cubemap
	bool Render_reflections(viewport_config &vp);
	bool Render(world_environment *Environment);
	void Render(scene::basic_region *Region);
	void Render(section_sequence::iterator First, section_sequence::iterator Last);
	void Render(cell_sequence::iterator First, cell_sequence::iterator Last);
    void Render(scene::shape_node const &Shape, bool const Ignorerange);
	void Render(TAnimModel *Instance);
	bool Render(TDynamicObject *Dynamic);
    bool Render(TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &Angle);
	bool Render(TModel3d *Model, material_data const *Material, float const Squaredistance);
	void Render(TSubModel *Submodel);
	void Render(TTrack *Track);
	void Render(scene::basic_cell::path_sequence::const_iterator First, scene::basic_cell::path_sequence::const_iterator Last);
	bool Render_cab(TDynamicObject const *Dynamic, float const Lightlevel, bool const Alpha = false);
	void Render(TMemCell *Memcell);
	void Render_particles();
	void Render_precipitation();
	void Render_Alpha(scene::basic_region *Region);
	void Render_Alpha(cell_sequence::reverse_iterator First, cell_sequence::reverse_iterator Last);
	void Render_Alpha(TAnimModel *Instance);
	void Render_Alpha(TTraction *Traction);
    void Render_Alpha(scene::lines_node const &Lines);
	bool Render_Alpha(TDynamicObject *Dynamic);
    bool Render_Alpha(TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &Angle);
	bool Render_Alpha(TModel3d *Model, material_data const *Material, float const Squaredistance);
	void Render_Alpha(TSubModel *Submodel);
	void Update_Lights(light_array &Lights);
	glm::vec3 pick_color(std::size_t const Index);
	std::size_t pick_index(glm::ivec3 const &Color);

	bool init_viewport(viewport_config &vp);

    void draw(const gfx::geometry_handle &handle);
    void draw(std::vector<gfx::geometrybank_handle>::iterator begin, std::vector<gfx::geometrybank_handle>::iterator end);

	void draw_debug_ui();

	// members
	GLFWwindow *m_window{nullptr}; // main window
	gfx::geometrybank_manager m_geometry;
	material_manager m_materials;
	texture_manager m_textures;
	opengl33_light m_sunlight;
	opengllight_array m_lights;
	/*
	    float m_sunandviewangle; // cached dot product of sunlight and camera vectors
    */
    gfx::geometry_handle m_billboardgeometry{0, 0};
	texture_handle m_glaretexture{-1};
	texture_handle m_suntexture{-1};
    texture_handle m_moontexture{-1};
	texture_handle m_smoketexture{-1};

	// main shadowmap resources
	int m_shadowbuffersize{2048};
	glm::mat4 m_shadowtexturematrix; // conversion from camera-centric world space to light-centric clip space
	glm::mat4 m_cabshadowtexturematrix; // conversion from cab-centric world space to light-centric clip space

	int m_environmentcubetextureface{0}; // helper, currently processed cube map face
	double m_environmentupdatetime{0}; // time of the most recent environment map update
	glm::dvec3 m_environmentupdatelocation; // coordinates of most recent environment map update
    opengl33_skydome m_skydomerenderer;
    opengl33_precipitation m_precipitationrenderer;
    opengl33_particles m_particlerenderer; // particle visualization subsystem

	unsigned int m_framestamp; // id of currently rendered gfx frame
	float m_framerate;
	double m_updateaccumulator{0.0};
	std::string m_debugtimestext;
	std::string m_pickdebuginfo;
	debug_stats m_debugstats;
	std::string m_debugstatstext;

	glm::vec4 m_baseambient{0.0f, 0.0f, 0.0f, 1.0f};
	glm::vec4 m_shadowcolor{colors::shadow};
	//    TEnvironmentType m_environment { e_flat };
	float m_specularopaquescalefactor{1.f};
    float m_speculartranslucentscalefactor{1.f};

	float m_fogrange = 2000.0f;

	renderpass_config m_renderpass; // parameters for current render pass
	section_sequence m_sectionqueue; // list of sections in current render pass
	cell_sequence m_cellqueue;
    renderpass_config m_colorpass; // parametrs of most recent color pass
	renderpass_config m_shadowpass; // parametrs of most recent shadowmap pass
	renderpass_config m_cabshadowpass; // parameters of most recent cab shadowmap pass
	std::vector<TSubModel const *> m_pickcontrolsitems;
	TSubModel const *m_pickcontrolitem{nullptr};
    std::vector<scene::basic_node *> m_picksceneryitems;
    scene::basic_node *m_picksceneryitem{nullptr};
    glm::vec3 m_worldmousecoordinates { 0.f };
#ifdef EU07_USE_DEBUG_CAMERA
	renderpass_config m_worldcamera; // debug item
#endif

	std::optional<gl::query> m_timequery;
	GLuint64 m_gllasttime = 0;

    double m_precipitationrotation;

    glm::mat4 perspective_projection(float fov, float aspect, float z_near, float z_far);
    glm::mat4 perpsective_frustumtest_projection(float fov, float aspect, float z_near, float z_far);
    glm::mat4 ortho_projection(float left, float right, float bottom, float top, float z_near, float z_far);
    glm::mat4 ortho_frustumtest_projection(float left, float right, float bottom, float top, float z_near, float z_far);

    std::vector<std::function<void(TSubModel const *)>> m_control_pick_requests;
    std::vector<std::function<void(scene::basic_node *)>> m_node_pick_requests;

	std::unique_ptr<gl::shader> m_vertex_shader;

	std::unique_ptr<gl::ubo> scene_ubo;
	std::unique_ptr<gl::ubo> model_ubo;
	std::unique_ptr<gl::ubo> light_ubo;
	gl::scene_ubs scene_ubs;
	gl::model_ubs model_ubs;
	gl::light_ubs light_ubs;

	std::unordered_map<std::string, std::shared_ptr<gl::program>> m_shaders;

	std::unique_ptr<gl::program> m_line_shader;
	std::unique_ptr<gl::program> m_freespot_shader;
    std::unique_ptr<gl::program> m_billboard_shader;
    std::unique_ptr<gl::program> m_celestial_shader;

    std::unique_ptr<gl::vao> m_empty_vao;

	std::vector<std::unique_ptr<viewport_config>> m_viewports;

	std::unique_ptr<gl::postfx> m_pfx_motionblur;
	std::unique_ptr<gl::postfx> m_pfx_tonemapping;

	std::unique_ptr<gl::program> m_shadow_shader;
	std::unique_ptr<gl::program> m_alpha_shadow_shader;

	std::unique_ptr<gl::framebuffer> m_pick_fb;
	std::unique_ptr<opengl_texture> m_pick_tex;
	std::unique_ptr<gl::renderbuffer> m_pick_rb;
	std::unique_ptr<gl::program> m_pick_shader;

	std::unique_ptr<gl::cubemap> m_empty_cubemap;

	std::unique_ptr<gl::framebuffer> m_cabshadows_fb;
	std::unique_ptr<opengl_texture> m_cabshadows_tex;

	std::unique_ptr<gl::framebuffer> m_env_fb;
	std::unique_ptr<gl::renderbuffer> m_env_rb;
	std::unique_ptr<gl::cubemap> m_env_tex;

	std::unique_ptr<gl::framebuffer> m_shadow_fb;
	std::unique_ptr<opengl_texture> m_shadow_tex;

    std::unique_ptr<gl::pbo> m_picking_pbo;
    std::unique_ptr<gl::pbo> m_picking_node_pbo;

    std::unique_ptr<gl::pbo> m_depth_pointer_pbo;
    std::unique_ptr<gl::framebuffer> m_depth_pointer_fb;
    std::unique_ptr<gl::framebuffer> m_depth_pointer_fb2;
    std::unique_ptr<gl::renderbuffer> m_depth_pointer_rb;
    std::unique_ptr<opengl_texture> m_depth_pointer_tex;
    std::unique_ptr<gl::program> m_depth_pointer_shader;

	material_handle m_invalid_material;

    bool m_blendingenabled;

	bool m_widelines_supported;

	struct headlight_config_s
	{
		float in_cutoff = 1.005f;
		float out_cutoff = 0.993f;

		float falloff_linear = 0.069f;
		float falloff_quadratic = 0.03f;

		float intensity = 1.0f;
		float ambient = 0.184f;
	};

	headlight_config_s headlight_config;
};

//---------------------------------------------------------------------------