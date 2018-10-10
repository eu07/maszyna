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
#include "lightarray.h"
#include "dumb3d.h"
#include "frustum.h"
#include "simulationenvironment.h"
#include "MemCell.h"
#include "scene.h"
#include "light.h"
#include "gl/ubo.h"
#include "gl/framebuffer.h"
#include "gl/renderbuffer.h"
#include "gl/postfx.h"
#include "gl/shader.h"
#include "gl/cubemap.h"
#include "gl/glsl_common.h"

#define EU07_USE_PICKING_FRAMEBUFFER
//#define EU07_USE_DEBUG_SHADOWMAP
//#define EU07_USE_DEBUG_CABSHADOWMAP
//#define EU07_USE_DEBUG_CAMERA
//#define EU07_USE_DEBUG_SOUNDEMITTERS

struct opengl_light : public basic_light
{

	GLuint id{(GLuint)-1};

	float factor;

	void apply_intensity(float const Factor = 1.0f);
	void apply_angle();

	opengl_light &operator=(basic_light const &Right)
	{
		basic_light::operator=(Right);
		return *this;
	}
};

// encapsulates basic rendering setup.
// for modern opengl this translates to a specific collection of glsl shaders,
// for legacy opengl this is combination of blending modes, active texture units etc
struct opengl_technique
{
};

// simple camera object. paired with 'virtual camera' in the scene
class opengl_camera
{

  public:
	// methods:
    inline void update_frustum(glm::mat4 frustumtest_proj)
	{
        update_frustum(frustumtest_proj, m_modelview);
	}
	void update_frustum(glm::mat4 const &Projection, glm::mat4 const &Modelview);
	bool visible(scene::bounding_area const &Area) const;
	bool visible(TDynamicObject const *Dynamic) const;
	inline glm::dvec3 const &position() const
	{
		return m_position;
	}
	inline glm::dvec3 &position()
	{
		return m_position;
	}
	inline glm::mat4 const &projection() const
	{
		return m_projection;
	}
	inline glm::mat4 &projection()
	{
		return m_projection;
	}
	inline glm::mat4 const &modelview() const
	{
		return m_modelview;
	}
	inline glm::mat4 &modelview()
	{
		return m_modelview;
	}
	inline std::vector<glm::vec4> &frustum_points()
	{
		return m_frustumpoints;
	}
	// transforms provided set of clip space points to world space
	template <class Iterator_> void transform_to_world(Iterator_ First, Iterator_ Last) const
	{
		std::for_each(First, Last, [this](glm::vec4 &point) {
			// transform each point using the cached matrix...
			point = this->m_inversetransformation * point;
			// ...and scale by transformed w
			point = glm::vec4{glm::vec3{point} / point.w, 1.f};
		});
	}
	// debug helper, draws shape of frustum in world space
	void draw(glm::vec3 const &Offset) const;

  private:
	// members:
	cFrustum m_frustum;
	std::vector<glm::vec4> m_frustumpoints; // visualization helper; corners of defined frustum, in world space
	glm::dvec3 m_position;
	glm::mat4 m_projection;
	glm::mat4 m_modelview;
	glm::mat4 m_inversetransformation; // cached transformation to world space
};

// bare-bones render controller, in lack of anything better yet
class opengl_renderer
{
  public:
	// types

	// methods
	bool Init(GLFWwindow *Window);
	// main draw call. returns false on error
	bool Render();
	void SwapBuffers();
	inline float Framerate()
	{
		return m_framerate;
	}
	// geometry methods
	// NOTE: hands-on geometry management is exposed as a temporary measure; ultimately all visualization data should be generated/handled automatically by the renderer itself
	// creates a new geometry bank. returns: handle to the bank or NULL
	gfx::geometrybank_handle Create_Bank();
	// creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
	gfx::geometry_handle Insert(gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type);
	// replaces data of specified chunk with the supplied vertex data, starting from specified offset
	bool Replace(gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset = 0);
	// adds supplied vertex data at the end of specified chunk
	bool Append(gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type);
    // draws supplied geometry handles
    void Draw_Geometry(std::vector<gfx::geometrybank_handle>::iterator begin, std::vector<gfx::geometrybank_handle>::iterator end);
	// provides direct access to vertex data of specfied chunk
	gfx::vertex_array const &Vertices(gfx::geometry_handle const &Geometry) const;
	// material methods
	material_handle Fetch_Material(std::string const &Filename, bool const Loadnow = true);
    void Bind_Material(material_handle const Material, TSubModel *sm = nullptr);
    void Bind_Material_Shadow(material_handle const Material);

	// shader methods
	std::shared_ptr<gl::program> Fetch_Shader(std::string const &name);

	opengl_material const &Material(material_handle const Material) const;
    opengl_material &Material(material_handle const Material);
	// texture methods
    texture_handle Fetch_Texture(std::string const &Filename, bool const Loadnow = true, GLint format_hint = GL_SRGB_ALPHA);
	void Bind_Texture(size_t Unit, texture_handle const Texture);
	opengl_texture &Texture(texture_handle const Texture) const;
	// utility methods
	TSubModel const *Pick_Control() const
	{
		return m_pickcontrolitem;
	}
	scene::basic_node const *Pick_Node() const
	{
		return m_picksceneryitem;
	}
    glm::dvec3 Mouse_Position() const
    {
        return m_worldmousecoordinates;
    }
	// maintenance methods
	void Update(double const Deltatime);
	TSubModel const *Update_Pick_Control();
    scene::basic_node *Update_Pick_Node();
    glm::dvec3 Update_Mouse_Position();
	// debug methods
	std::string const &info_times() const;
	std::string const &info_stats() const;

	// members
	GLenum static const sunlight{GL_LIGHT0};
	std::size_t m_drawcount{0};

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

		opengl_camera camera;
		rendermode draw_mode{rendermode::none};
		float draw_range{0.0f};
	};

	typedef std::vector<opengl_light> opengllight_array;

	// methods
    std::unique_ptr<gl::program> make_shader(std::string v, std::string f);
	bool Init_caps();
	void setup_pass(renderpass_config &Config, rendermode const Mode, float const Znear = 0.f, float const Zfar = 1.f, bool const Ignoredebug = false);
	void setup_matrices();
    void setup_drawing(bool const Alpha = false);
    void setup_shadow_map(opengl_texture *tex, renderpass_config conf);
    void setup_env_map(gl::cubemap *tex);
	void setup_environment_light(TEnvironmentType const Environment = e_flat);
	// runs jobs needed to generate graphics for specified render pass
	void Render_pass(rendermode const Mode);
	// creates dynamic environment cubemap
	bool Render_reflections();
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
	bool Render_cab(TDynamicObject const *Dynamic, bool const Alpha = false);
	void Render(TMemCell *Memcell);
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

    void draw(const gfx::geometry_handle &handle);
    void draw(std::vector<gfx::geometrybank_handle>::iterator begin, std::vector<gfx::geometrybank_handle>::iterator end);

	// members
	GLFWwindow *m_window{nullptr};
	gfx::geometrybank_manager m_geometry;
	material_manager m_materials;
	texture_manager m_textures;
	opengl_light m_sunlight;
	opengllight_array m_lights;
	/*
	    float m_sunandviewangle; // cached dot product of sunlight and camera vectors
    */
    gfx::geometry_handle m_billboardgeometry{0, 0};
	texture_handle m_glaretexture{-1};
	texture_handle m_suntexture{-1};
    texture_handle m_moontexture{-1};

	// main shadowmap resources
	int m_shadowbuffersize{2048};
	glm::mat4 m_shadowtexturematrix; // conversion from camera-centric world space to light-centric clip space
	glm::mat4 m_cabshadowtexturematrix; // conversion from cab-centric world space to light-centric clip space

	int m_environmentcubetextureface{0}; // helper, currently processed cube map face
	int m_environmentupdatetime{0}; // time of the most recent environment map update
	glm::dvec3 m_environmentupdatelocation; // coordinates of most recent environment map update

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

	renderpass_config m_renderpass; // parameters for current render pass
	section_sequence m_sectionqueue; // list of sections in current render pass
	cell_sequence m_cellqueue;
    renderpass_config m_colorpass; // parametrs of most recent color pass
	renderpass_config m_shadowpass; // parametrs of most recent shadowmap pass
	renderpass_config m_cabshadowpass; // parameters of most recent cab shadowmap pass
	std::vector<TSubModel const *> m_pickcontrolsitems;
	TSubModel const *m_pickcontrolitem{nullptr};
	std::vector<scene::basic_node const *> m_picksceneryitems;
	scene::basic_node const *m_picksceneryitem{nullptr};
    glm::vec3 m_worldmousecoordinates { 0.f };
#ifdef EU07_USE_DEBUG_CAMERA
	renderpass_config m_worldcamera; // debug item
#endif
	GLuint m_gltimequery = 0;
	GLuint64 m_gllasttime = 0;

    glm::mat4 perspective_projection(float fov, float aspect, float z_near, float z_far);
    glm::mat4 perpsective_frustumtest_projection(float fov, float aspect, float z_near, float z_far);
    glm::mat4 ortho_projection(float left, float right, float bottom, float top, float z_near, float z_far);
    glm::mat4 ortho_frustumtest_projection(float left, float right, float bottom, float top, float z_near, float z_far);

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

	std::unique_ptr<gl::framebuffer> m_msaa_fb;
	std::unique_ptr<gl::renderbuffer> m_msaa_rbc;
    std::unique_ptr<gl::renderbuffer> m_msaa_rbv;
	std::unique_ptr<gl::renderbuffer> m_msaa_rbd;

	std::unique_ptr<gl::framebuffer> m_main_fb;
    std::unique_ptr<opengl_texture> m_main_texv;
	std::unique_ptr<opengl_texture> m_main_tex;

    std::unique_ptr<gl::framebuffer> m_main2_fb;
    std::unique_ptr<opengl_texture> m_main2_tex;

    std::unique_ptr<gl::postfx> m_pfx_motionblur;
    std::unique_ptr<gl::postfx> m_pfx_tonemapping;

	std::unique_ptr<gl::framebuffer> m_shadow_fb;
	std::unique_ptr<opengl_texture> m_shadow_tex;
	std::unique_ptr<gl::program> m_shadow_shader;
    std::unique_ptr<gl::program> m_alpha_shadow_shader;

	std::unique_ptr<gl::framebuffer> m_pick_fb;
	std::unique_ptr<opengl_texture> m_pick_tex;
	std::unique_ptr<gl::renderbuffer> m_pick_rb;
	std::unique_ptr<gl::program> m_pick_shader;

    std::unique_ptr<gl::framebuffer> m_cabshadows_fb;
    std::unique_ptr<opengl_texture> m_cabshadows_tex;

    std::unique_ptr<gl::framebuffer> m_env_fb;
    std::unique_ptr<gl::renderbuffer> m_env_rb;
    std::unique_ptr<gl::cubemap> m_env_tex;

    std::unique_ptr<gl::cubemap> m_empty_cubemap;

	material_handle m_invalid_material;

    bool m_blendingenabled;

	bool m_widelines_supported;
};

extern opengl_renderer GfxRenderer;

//---------------------------------------------------------------------------
