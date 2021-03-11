/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "opengl33renderer.h"

#include "color.h"
#include "Globals.h"
#include "Timer.h"
#include "Train.h"
#include "Camera.h"
#include "simulation.h"
#include "Logs.h"
#include "utilities.h"
#include "simulationtime.h"
#include "application.h"
#include "AnimModel.h"

//#define EU07_DEBUG_OPENGL

int const EU07_PICKBUFFERSIZE{ 1024 }; // size of (square) textures bound with the pick framebuffer
int const EU07_REFLECTIONFIDELITYOFFSET { 250 }; // artificial increase of range for reflection pass detail reduction

auto const gammacorrection { glm::vec3( 2.2f ) };

void GLAPIENTRY
ErrorCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) {
/*
    auto const typestring {
        type == GL_DEBUG_TYPE_ERROR ? "error" :
        type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ? "deprecated behavior" :
        type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR ? "undefined behavior" :
        type == GL_DEBUG_TYPE_PORTABILITY ? "portability" :
        type == GL_DEBUG_TYPE_PERFORMANCE ? "performance" :
        type == GL_DEBUG_TYPE_OTHER ? "other" :
        "unknown" };
*/
    auto const severitystring {
        severity == GL_DEBUG_SEVERITY_HIGH ? "high severity" :
        severity == GL_DEBUG_SEVERITY_MEDIUM ? "medium severity" :
        severity == GL_DEBUG_SEVERITY_LOW ? "low severity" :
        severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "notification" :
        "unknown" };

    ErrorLog(
        "bad gfx code: " + std::string( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR ** " : "" )
        + to_hex_str( id )
        + " (" + severitystring + ") "
        + message );
}

bool opengl33_renderer::Init(GLFWwindow *Window)
{
#ifdef EU07_DEBUG_OPENGL
    if( GLAD_GL_KHR_debug ) {
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE );
        glDebugMessageControl( GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, nullptr, GL_FALSE );
        glDebugMessageCallback( ErrorCallback, 0 );
    }
#endif
	if (!Init_caps())
		return false;

    OpenGLMatrices.upload() = false; // set matrix stack in virtual mode

	m_window = Window;

	gl::glsl_common_setup();

	if (true == Global.ScaleSpecularValues)
	{
		m_specularopaquescalefactor = 0.25f;
		m_speculartranslucentscalefactor = 1.5f;
	}

	// rgb value for 5780 kelvin
	Global.DayLight.diffuse[0] = 255.0f / 255.0f;
	Global.DayLight.diffuse[1] = 242.0f / 255.0f;
	Global.DayLight.diffuse[2] = 231.0f / 255.0f;
	Global.DayLight.is_directional = true;
	m_sunlight.id = opengl33_renderer::sunlight;

	// create dynamic light pool
	for (int idx = 0; idx < Global.DynamicLightCount; ++idx)
	{
		opengl33_light light;
		light.id = 1 + idx;

		light.is_directional = false;

		m_lights.emplace_back(light);
	}
	// preload some common textures
	m_glaretexture = Fetch_Texture("fx/lightglare");
	m_suntexture = Fetch_Texture("fx/sun");
	m_moontexture = Fetch_Texture("fx/moon");
	m_smoketexture = Fetch_Texture("fx/smoke");
    m_headlightstexture = Fetch_Texture("fx/headlights:st");

	// prepare basic geometry chunks
	float const size = 2.5f / 2.0f;
	auto const geometrybank = m_geometry.create_bank();
	m_billboardgeometry = m_geometry.create_chunk(
	    gfx::vertex_array{
	        {{-size, size, 0.f}, glm::vec3(), {1.f, 1.f}}, {{size, size, 0.f}, glm::vec3(), {0.f, 1.f}}, {{-size, -size, 0.f}, glm::vec3(), {1.f, 0.f}}, {{size, -size, 0.f}, glm::vec3(), {0.f, 0.f}}},
	    geometrybank, GL_TRIANGLE_STRIP);
	m_empty_vao = std::make_unique<gl::vao>();

	try
	{
		m_vertex_shader = std::make_unique<gl::shader>("vertex.vert");
		m_line_shader = make_shader("traction.vert", "traction.frag");
		m_freespot_shader = make_shader("freespot.vert", "freespot.frag");
		m_shadow_shader = make_shader("simpleuv.vert", "shadowmap.frag");
		m_alpha_shadow_shader = make_shader("simpleuv.vert", "alphashadowmap.frag");
		m_pick_shader = make_shader("vertexonly.vert", "pick.frag");
		m_billboard_shader = make_shader("simpleuv.vert", "billboard.frag");
		m_celestial_shader = make_shader("celestial.vert", "celestial.frag");
		if (Global.gfx_usegles)
			m_depth_pointer_shader = make_shader("quad.vert", "gles_depthpointer.frag");
		m_invalid_material = Fetch_Material("invalid");
	}
	catch (gl::shader_exception const &e)
	{
		ErrorLog("invalid shader: " + std::string(e.what()));
		return false;
	}

	scene_ubo = std::make_unique<gl::ubo>(sizeof(gl::scene_ubs), 0);
	model_ubo = std::make_unique<gl::ubo>(sizeof(gl::model_ubs), 1, GL_STREAM_DRAW);
	light_ubo = std::make_unique<gl::ubo>(sizeof(gl::light_ubs), 2);

	// better initialize with 0 to not crash driver/whole system
	// when we forget
	memset(&light_ubs, 0, sizeof(light_ubs));
	memset(&model_ubs, 0, sizeof(model_ubs));
	memset(&scene_ubs, 0, sizeof(scene_ubs));

	light_ubo->update(light_ubs);
	model_ubo->update(model_ubs);
	scene_ubo->update(scene_ubs);

	int samples = 1 << Global.iMultisampling;
	if (!Global.gfx_usegles && samples > 1)
		glEnable(GL_MULTISAMPLE);

	m_pfx_motionblur = std::make_unique<gl::postfx>("motionblur");
	m_pfx_tonemapping = std::make_unique<gl::postfx>("tonemapping");
    m_pfx_chromaticaberration = std::make_unique<gl::postfx>( "chromaticaberration" );

	m_empty_cubemap = std::make_unique<gl::cubemap>();
	m_empty_cubemap->alloc(Global.gfx_format_color, 16, 16, GL_RGB, GL_FLOAT);

	m_viewports.push_back(std::make_unique<viewport_config>());
	viewport_config &default_viewport = *m_viewports.front().get();
    if( Global.gfx_framebuffer_fidelity >= 0 ) {
        std::vector<int> resolutions = { 480, 720, 1080, 1440 };
        default_viewport.height = resolutions[ clamp<int>( Global.gfx_framebuffer_fidelity, 0, resolutions.size() - 1 ) ];
        default_viewport.width = quantize( Global.iWindowWidth * default_viewport.height / Global.iWindowHeight, 4 );
    }
    else {
        default_viewport.width = Global.gfx_framebuffer_width;
        default_viewport.height = Global.gfx_framebuffer_height;
    }
	default_viewport.main = true;
	default_viewport.window = m_window;
	default_viewport.draw_range = 1.0f;

    if( !init_viewport( default_viewport ) ) {
        ErrorLog( "default viewport setup failed" );
        return false;
    }
	glfwMakeContextCurrent(m_window);
    gl::vao::unbind();
	gl::buffer::unbind();

	if (Global.gfx_shadowmap_enabled)
	{
		m_shadow_fb = std::make_unique<gl::framebuffer>();
		m_shadow_tex = std::make_unique<opengl_texture>();
		m_shadow_tex->alloc_rendertarget(Global.gfx_format_depth, GL_DEPTH_COMPONENT, m_shadowbuffersize, m_shadowbuffersize, m_shadowpass.size());
		m_shadow_fb->attach(*m_shadow_tex, GL_DEPTH_ATTACHMENT, 0);
        m_shadow_fb->setup_drawing(0);

        if( !m_shadow_fb->is_complete() ) {
            ErrorLog( "shadow framebuffer setup failed" );
            return false;
        }

		WriteLog("shadows enabled");
	}

    if (Global.gfx_envmap_enabled)
	{
		m_env_rb = std::make_unique<gl::renderbuffer>();
		m_env_rb->alloc(Global.gfx_format_depth, gl::ENVMAP_SIZE, gl::ENVMAP_SIZE);
		m_env_tex = std::make_unique<gl::cubemap>();
		m_env_tex->alloc(Global.gfx_format_color, gl::ENVMAP_SIZE, gl::ENVMAP_SIZE, GL_RGB, GL_FLOAT);

		m_env_fb = std::make_unique<gl::framebuffer>();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		for (int i = 0; i < 6; i++)
		{
			m_env_fb->attach(*m_empty_cubemap, i, GL_COLOR_ATTACHMENT0);
			m_env_fb->clear(GL_COLOR_BUFFER_BIT);
		}

		m_env_fb->detach(GL_COLOR_ATTACHMENT0);
		m_env_fb->attach(*m_env_rb, GL_DEPTH_ATTACHMENT);

		WriteLog("envmap enabled");
	}

	m_pick_tex = std::make_unique<opengl_texture>();
	m_pick_tex->alloc_rendertarget(GL_RGB8, GL_RGB, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE);
	m_pick_rb = std::make_unique<gl::renderbuffer>();
	m_pick_rb->alloc(Global.gfx_format_depth, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE);
	m_pick_fb = std::make_unique<gl::framebuffer>();
	m_pick_fb->attach(*m_pick_tex, GL_COLOR_ATTACHMENT0);
	m_pick_fb->attach(*m_pick_rb, GL_DEPTH_ATTACHMENT);

    if( !m_pick_fb->is_complete() ) {
        ErrorLog( "pick framebuffer setup failed" );
        return false;
    }

	m_picking_pbo = std::make_unique<gl::pbo>();
	m_picking_node_pbo = std::make_unique<gl::pbo>();

	m_depth_pointer_pbo = std::make_unique<gl::pbo>();
	if (!Global.gfx_usegles && Global.iMultisampling)
	{
		m_depth_pointer_rb = std::make_unique<gl::renderbuffer>();
		m_depth_pointer_rb->alloc(Global.gfx_format_depth, 1, 1);

		m_depth_pointer_fb = std::make_unique<gl::framebuffer>();
		m_depth_pointer_fb->attach(*m_depth_pointer_rb, GL_DEPTH_ATTACHMENT);
        m_depth_pointer_fb->setup_drawing(0);

        if( !m_depth_pointer_fb->is_complete() ) {
            ErrorLog( "depth pointer framebuffer setup failed" );
            return false;
        }
	}
	else if (Global.gfx_usegles)
	{
		m_depth_pointer_tex = std::make_unique<opengl_texture>();

		GLenum format = Global.gfx_format_depth;
		if (Global.gfx_skippipeline)
		{
			gl::framebuffer::unbind();
			GLint bits, type;
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
			if (type == GL_FLOAT && bits == 32)
				format = GL_DEPTH_COMPONENT32F;
			else if (bits == 16)
				format = GL_DEPTH_COMPONENT16;
			else if (bits == 24)
				format = GL_DEPTH_COMPONENT24;
			else if (bits == 32)
				format = GL_DEPTH_COMPONENT32;
		}

		m_depth_pointer_tex->alloc_rendertarget(format, GL_DEPTH_COMPONENT, Global.gfx_framebuffer_width, Global.gfx_framebuffer_height);

		m_depth_pointer_rb = std::make_unique<gl::renderbuffer>();
		m_depth_pointer_rb->alloc(GL_R16UI, Global.gfx_framebuffer_width, Global.gfx_framebuffer_height);

		m_depth_pointer_fb = std::make_unique<gl::framebuffer>();
		m_depth_pointer_fb->attach(*m_depth_pointer_tex, GL_DEPTH_ATTACHMENT);

		m_depth_pointer_fb2 = std::make_unique<gl::framebuffer>();
		m_depth_pointer_fb2->attach(*m_depth_pointer_rb, GL_COLOR_ATTACHMENT0);

        if( !m_depth_pointer_fb->is_complete() ) {
            ErrorLog( "depth pointer framebuffer setup failed" );
            return false;
        }

        if( !m_depth_pointer_fb2->is_complete() ) {
            ErrorLog( "depth pointer framebuffer2 setup failed" );
            return false;
        }
	}

	m_timequery.emplace(gl::query::TIME_ELAPSED);
	m_timequery->begin();

	WriteLog("picking objects created");

	WriteLog("Gfx Renderer: setup complete");

    return true;
}
/*
bool opengl33_renderer::AddViewport(const global_settings::extraviewport_config &conf)
{
	m_viewports.push_back(std::make_unique<viewport_config>());
	viewport_config &vp = *m_viewports.back().get();
	vp.width = conf.width;
	vp.height = conf.height;
	vp.window = Application.window(-1, true, vp.width, vp.height, Application.find_monitor(conf.monitor));
	vp.camera_transform = conf.transform;
	vp.draw_range = conf.draw_range;

	bool ret = init_viewport(vp);
	glfwMakeContextCurrent(m_window);
	gl::buffer::unbind();

	return ret;
}
*/
bool opengl33_renderer::init_viewport(viewport_config &vp)
{
	glfwMakeContextCurrent(vp.window);

	WriteLog("init viewport: " + std::to_string(vp.width) + " x " + std::to_string(vp.height));

//	glfwSwapInterval( Global.VSync ? 1 : 0 );

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glClearColor( 51.0f / 255.f, 102.0f / 255.f, 85.0f / 255.f, 1.f ); // initial background Color

	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	if (!Global.gfx_usegles)
		glClearDepth(0.0f);
	else
		glClearDepthf(0.0f);

	glDepthFunc(GL_GEQUAL);

	if (GLAD_GL_ARB_clip_control)
		glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	else if (GLAD_GL_EXT_clip_control)
		glClipControlEXT(GL_LOWER_LEFT_EXT, GL_ZERO_TO_ONE_EXT);

    if( !Global.gfx_usegles ) {
        glEnable( GL_PROGRAM_POINT_SIZE );
        // not present in core, but if we get compatibility profile instead we won't get gl_pointcoord without it
        if( std::string( (char *)glGetString( GL_VERSION ) ).find( "3.3" ) != 0 ) {
            glEnable( GL_POINT_SPRITE );
        }
    }

	if (!gl::vao::use_vao)
	{
		GLuint v;
		glGenVertexArrays(1, &v);
		glBindVertexArray(v);
	}

	int samples = 1 << Global.iMultisampling;

	model_ubo->bind_uniform();
	scene_ubo->bind_uniform();
	light_ubo->bind_uniform();

	if (!Global.gfx_skippipeline)
	{
		vp.msaa_rbc = std::make_unique<gl::renderbuffer>();
		vp.msaa_rbc->alloc(Global.gfx_format_color, vp.width, vp.height, samples);

		vp.msaa_rbd = std::make_unique<gl::renderbuffer>();
		vp.msaa_rbd->alloc(Global.gfx_format_depth, vp.width, vp.height, samples);

		vp.msaa_fb = std::make_unique<gl::framebuffer>();
		vp.msaa_fb->attach(*vp.msaa_rbc, GL_COLOR_ATTACHMENT0);
		vp.msaa_fb->attach(*vp.msaa_rbd, GL_DEPTH_ATTACHMENT);

        vp.main_tex = std::make_unique<opengl_texture>();
        vp.main_tex->alloc_rendertarget(Global.gfx_format_color, GL_RGB, vp.width, vp.height, 1, 1, GL_CLAMP_TO_EDGE);

        vp.main_fb = std::make_unique<gl::framebuffer>();
        vp.main_fb->attach(*vp.main_tex, GL_COLOR_ATTACHMENT0);

		if (Global.gfx_postfx_motionblur_enabled)
		{
			vp.msaa_rbv = std::make_unique<gl::renderbuffer>();
			vp.msaa_rbv->alloc(Global.gfx_postfx_motionblur_format, vp.width, vp.height, samples);
			vp.msaa_fb->attach(*vp.msaa_rbv, GL_COLOR_ATTACHMENT1);

			vp.main_texv = std::make_unique<opengl_texture>();
			vp.main_texv->alloc_rendertarget(Global.gfx_postfx_motionblur_format, GL_RG, vp.width, vp.height);
			vp.main_fb->attach(*vp.main_texv, GL_COLOR_ATTACHMENT1);
			vp.main_fb->setup_drawing(2);

			WriteLog("motion blur enabled");
		}

        if( !vp.main_fb->is_complete() ) {
            ErrorLog( "main framebuffer setup failed" );
            return false;
        }

        if( !vp.msaa_fb->is_complete() ) {
            ErrorLog( "msaa framebuffer setup failed" );
            return false;
        }

		vp.main2_tex = std::make_unique<opengl_texture>();
		vp.main2_tex->alloc_rendertarget(Global.gfx_format_color, GL_RGB, vp.width, vp.height);

		vp.main2_fb = std::make_unique<gl::framebuffer>();
		vp.main2_fb->attach(*vp.main2_tex, GL_COLOR_ATTACHMENT0);
        if( !vp.main2_fb->is_complete() ) {
            ErrorLog( "main2 framebuffer setup failed" );
            return false;
        }
	}

	return true;
}

std::unique_ptr<gl::program> opengl33_renderer::make_shader(std::string v, std::string f)
{
	gl::shader vert(v);
	gl::shader frag(f);
	gl::program *prog = new gl::program({vert, frag});
	return std::unique_ptr<gl::program>(prog);
}

bool opengl33_renderer::Render()
{
	Timer::subsystem.gfx_total.start();

	if (!Global.gfx_usegles)
	{
		std::optional<int64_t> result = m_timequery->result();
		if (result) {
			m_gllasttime = *result;
			m_timequery->begin();
		}
	}

	// fetch simulation data
	if (simulation::is_ready)
	{
		m_sunlight = Global.DayLight;
		// quantize sun angle to reduce shadow crawl
		auto const quantizationstep{0.004f};
		m_sunlight.direction = glm::normalize(quantizationstep * glm::roundEven(m_sunlight.direction * (1.f / quantizationstep)));
	}
	// generate new frame
    opengl_texture::reset_unit_cache();

	m_renderpass.draw_mode = rendermode::none; // force setup anew
	m_renderpass.draw_stats = debug_stats();
    m_geometry.primitives_count() = 0;

	for (auto &viewport : m_viewports) {
		Render_pass(*viewport, rendermode::color);
	}

	glfwMakeContextCurrent(m_window);
    gl::vao::unbind();
	gl::buffer::unbind();
	m_current_viewport = &(*m_viewports.front());
/*
	m_debugtimestext += "cpu: " + to_string(Timer::subsystem.gfx_color.average(), 2) + " ms (" + std::to_string(m_cellqueue.size()) + " sectors)\n" +=
	    "cpu swap: " + to_string(Timer::subsystem.gfx_swap.average(), 2) + " ms\n" += "uilayer: " + to_string(Timer::subsystem.gfx_gui.average(), 2) + "ms\n" +=
	    "mainloop total: " + to_string(Timer::subsystem.mainloop_total.average(), 2) + "ms\n";
*/
	if (!Global.gfx_usegles)
	{
		m_timequery->end();
/*
		if (m_gllasttime)
			m_debugtimestext += "gpu: " + to_string((double)(m_gllasttime / 1000ULL) / 1000.0, 3) + "ms";
*/
	}
    // swapbuffers() could unbind current buffers so we prepare for it on our end
    gfx::opengl_vbogeometrybank::reset();
    ++m_framestamp;
    SwapBuffers();
    Timer::subsystem.gfx_total.stop();

    return true; // for now always succeed
}

void opengl33_renderer::SwapBuffers()
{
	Timer::subsystem.gfx_swap.start();

	for (auto &viewport : m_viewports) {
		if (viewport->window)
			glfwSwapBuffers(viewport->window);
	}

	Timer::subsystem.gfx_swap.stop();

    m_debugtimestext.clear();
    m_debugtimestext =
        "cpu frame total: " + to_string( Timer::subsystem.gfx_color.average() + Timer::subsystem.gfx_shadows.average() + Timer::subsystem.gfx_swap.average(), 2 ) + " ms\n"
        + " color: " + to_string( Timer::subsystem.gfx_color.average(), 2 ) + " ms (" + std::to_string( m_cellqueue.size() ) + " sectors)\n";
    if( Global.gfx_shadowmap_enabled ) {
        m_debugtimestext +=
            " shadows: " + to_string( Timer::subsystem.gfx_shadows.average(), 2 ) + " ms\n";
    }
    m_debugtimestext += " swap: " + to_string( Timer::subsystem.gfx_swap.average(), 2 ) + " ms\n";
    if( !Global.gfx_usegles ) {
        if (m_gllasttime)
            m_debugtimestext += "gpu frame total: " + to_string((double)(m_gllasttime / 1000ULL) / 1000.0, 3) + " ms\n";
    }
    m_debugtimestext += "uilayer: " + to_string( Timer::subsystem.gfx_gui.average(), 2 ) + " ms\n";
    if( DebugModeFlag )
        m_debugtimestext += m_textures.info();

    debug_stats shadowstats;
    for( auto const &shadowpass : m_shadowpass ) {
        shadowstats += shadowpass.draw_stats;
    }

    m_debugstatstext =
        "triangles: " + to_string( static_cast<int>(m_geometry.primitives_count()), 7 ) + "\n"
        + "vehicles:  " + to_string( m_colorpass.draw_stats.dynamics, 7 ) + " +" + to_string( shadowstats.dynamics, 7 )
        + " =" + to_string( m_colorpass.draw_stats.dynamics + shadowstats.dynamics, 7 ) + "\n"
        + "models:    " + to_string( m_colorpass.draw_stats.models, 7 ) + " +" + to_string( shadowstats.models, 7 )
        + " =" + to_string( m_colorpass.draw_stats.models + shadowstats.models, 7 ) + "\n"
        + "drawcalls: " + to_string( m_colorpass.draw_stats.drawcalls, 7 ) + " +" + to_string( shadowstats.drawcalls, 7 )
        + " =" + to_string( m_colorpass.draw_stats.drawcalls + shadowstats.drawcalls, 7 ) + "\n"
        + " submodels:" + to_string( m_colorpass.draw_stats.submodels, 7 ) + " +" + to_string( shadowstats.submodels, 7 )
        + " =" + to_string( m_colorpass.draw_stats.submodels + shadowstats.submodels, 7 ) + "\n"
        + " paths:    " + to_string( m_colorpass.draw_stats.paths, 7 ) + " +" + to_string( shadowstats.paths, 7 )
        + " =" + to_string( m_colorpass.draw_stats.paths + shadowstats.paths, 7 ) + "\n"
        + " shapes:   " + to_string( m_colorpass.draw_stats.shapes, 7 ) + " +" + to_string( shadowstats.shapes, 7 )
        + " =" + to_string( m_colorpass.draw_stats.shapes + shadowstats.shapes, 7 ) + "\n"
        + " traction: " + to_string( m_colorpass.draw_stats.traction, 7 ) + "\n"
        + " lines:    " + to_string( m_colorpass.draw_stats.lines, 7 ) + "\n"
        + "particles: " + to_string( m_colorpass.draw_stats.particles, 7 );
}

void opengl33_renderer::draw_debug_ui()
{
	if (!debug_ui_active)
		return;

	if (ImGui::Begin("headlight config", &debug_ui_active))
	{
		ImGui::SetWindowSize(ImVec2(0, 0));

		headlight_config_s &conf = headlight_config;
		ImGui::SliderFloat("in_cutoff", &conf.in_cutoff, 0.9f, 1.1f);
		ImGui::SliderFloat("out_cutoff", &conf.out_cutoff, 0.9f, 1.1f);

		ImGui::SliderFloat("falloff_linear", &conf.falloff_linear, 0.0f, 1.0f, "%.3f", 2.0f);
		ImGui::SliderFloat("falloff_quadratic", &conf.falloff_quadratic, 0.0f, 1.0f, "%.3f", 2.0f);

		ImGui::SliderFloat("ambient", &conf.ambient, 0.0f, 3.0f);
		ImGui::SliderFloat("intensity", &conf.intensity, 0.0f, 10.0f);
	}
	ImGui::End();
}

// runs jobs needed to generate graphics for specified render pass
void opengl33_renderer::Render_pass(viewport_config &vp, rendermode const Mode)
{
	setup_pass(vp, m_renderpass, Mode);
    switch (m_renderpass.draw_mode)
	{

	case rendermode::color:
	{
		glDebug("rendermode::color");

		glDebug("context switch");
		glfwMakeContextCurrent(vp.window);
        gl::vao::unbind();
		gl::buffer::unbind();
		m_current_viewport = &vp;

		if ((!simulation::is_ready) || (Global.gfx_skiprendering))
		{
            gl::program::unbind();
			gl::framebuffer::unbind();
			glClearColor( 51.0f / 255.f, 102.0f / 255.f, 85.0f / 255.f, 1.f ); // initial background Color
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if( vp.main ) {
                //glEnable( GL_FRAMEBUFFER_SRGB );
                Application.render_ui();
                //glDisable( GL_FRAMEBUFFER_SRGB );
            }
			break;
		}

        m_colorpass = m_renderpass; // cache pass data

		scene_ubs.time = Timer::GetTime();
		scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
        scene_ubs.inv_view = glm::inverse( glm::mat4{ glm::mat3{ m_colorpass.pass_camera.modelview() } } );
        scene_ubo->update(scene_ubs);
		scene_ubo->bind_uniform();

		if (m_widelines_supported)
			glLineWidth(1.0f);

		if (!Global.gfx_usegles)
		{
			if (Global.bWireFrame)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

        setup_shadow_unbind_map();
		setup_env_map(nullptr);

		if (Global.gfx_shadowmap_enabled && vp.main)
		{
			glDebug("render shadowmap start");
			Timer::subsystem.gfx_shadows.start();

            m_renderpass.draw_stats = {};
            Render_pass(vp, rendermode::shadows);
			m_renderpass = m_colorpass; // restore draw mode. TBD, TODO: render mode stack

			Timer::subsystem.gfx_shadows.stop();
			glDebug("render shadowmap end");
        }

		if (Global.gfx_envmap_enabled && vp.main)
		{
			// potentially update environmental cube map
            m_renderpass.draw_stats = {};
            if (Render_reflections(vp))
				m_renderpass = m_colorpass; // restore color pass settings
			setup_env_map(m_env_tex.get());
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		setup_drawing(false);

		glm::ivec2 target_size(vp.width, vp.height);
		if (vp.main) // TODO: update window sizes also for extra viewports
			target_size = glm::ivec2(Global.iWindowWidth, Global.iWindowHeight);

		if (!Global.gfx_skippipeline)
		{
			vp.msaa_fb->bind();

			if (Global.gfx_postfx_motionblur_enabled)
				vp.msaa_fb->setup_drawing(2);
			else
				vp.msaa_fb->setup_drawing(1);

			glViewport(0, 0, vp.width, vp.height);

			vp.msaa_fb->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		else
		{
			if (!Global.gfx_usegles && !Global.gfx_shadergamma)
				glEnable(GL_FRAMEBUFFER_SRGB);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, target_size.x, target_size.y);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		glEnable(GL_DEPTH_TEST);

		Timer::subsystem.gfx_color.start();
		setup_matrices();
		setup_drawing(true);
        m_renderpass.draw_stats = {};

		model_ubs.future = glm::mat4();
        glm::mat4 future;
		if (Global.pCamera.m_owner != nullptr)
		{
			auto const *vehicle = Global.pCamera.m_owner;
			glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);
			future = glm::translate(mv, -glm::vec3(vehicle->get_future_movement())) * glm::inverse(mv);
		}

        Update_Lights( simulation::Lights );

		glDebug("render environment");

		scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
		scene_ubo->update(scene_ubs);
		Render(&simulation::Environment);

        if( Global.gfx_shadowmap_enabled )
            setup_shadow_bind_map();

		// opaque parts...
		setup_drawing(false);

		// without rain/snow we can render the cab early to limit the overdraw
		// precipitation happens when overcast is in 1-2 range
		if (!FreeFlyModeFlag && Global.Overcast <= 1.0f && Global.render_cab)
		{
			glDebug("render opaque cab");
            model_ubs.future = glm::mat4();
            auto const *vehicle{ simulation::Train->Dynamic() };
            if( vehicle->InteriorLightLevel > 0.f ) {
                setup_shadow_color( glm::min( colors::white, m_shadowcolor + glm::vec4( vehicle->InteriorLight * vehicle->InteriorLightLevel, 1.f ) ) );
            }
            Render_cab( vehicle, vehicle->InteriorLightLevel, false );
		}

        setup_shadow_color( m_shadowcolor );

		glDebug("render opaque region");

		model_ubs.future = future;

		Render(simulation::Region);

		// ...translucent parts
		glDebug("render translucent region");
		setup_drawing(true);
		Render_Alpha(simulation::Region);

		// particles
		Render_particles();

		// precipitation; done at end, only before cab render
		Render_precipitation();

		// cab render
		if (false == FreeFlyModeFlag && Global.render_cab)
		{
			model_ubs.future = glm::mat4();
            auto *vehicle { simulation::Train->Dynamic() };
            if( vehicle->InteriorLightLevel > 0.f ) {
                setup_shadow_color( glm::min( colors::white, m_shadowcolor + glm::vec4( vehicle->InteriorLight * vehicle->InteriorLightLevel, 1.f ) ) );
            }
			if (Global.Overcast > 1.0f)
			{
				// with active precipitation draw the opaque cab parts here to mask rain/snow placed 'inside' the cab
                glDebug( "render opaque cab" );
                setup_drawing(false);
				Render_cab(vehicle, vehicle->InteriorLightLevel, false);
                Render_interior(false);
				setup_drawing(true);
                Render_interior(true);
			}
            glDebug( "render translucent cab" );
            Render_cab(vehicle, vehicle->InteriorLightLevel, true);

            setup_shadow_color( m_shadowcolor );

            if( Global.Overcast > 1.0f ) {
                // with the cab in place we can (finally) safely draw translucent part of the occupied vehicle
                Render_Alpha( vehicle );
            }
        }

		Timer::subsystem.gfx_color.stop();

        // store draw stats
        m_colorpass.draw_stats = m_renderpass.draw_stats;

        setup_shadow_unbind_map();
		setup_env_map(nullptr);

		if (!Global.gfx_usegles)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (!Global.gfx_skippipeline)
		{
			if (Global.gfx_postfx_motionblur_enabled)
			{
                gl::program::unbind();
                vp.main_fb->clear(GL_COLOR_BUFFER_BIT);
				vp.msaa_fb->blit_to(vp.main_fb.get(), vp.width, vp.height, GL_COLOR_BUFFER_BIT, GL_COLOR_ATTACHMENT0);
				vp.msaa_fb->blit_to(vp.main_fb.get(), vp.width, vp.height, GL_COLOR_BUFFER_BIT, GL_COLOR_ATTACHMENT1);

				model_ubs.param[0].x = m_framerate / (1.0 / Global.gfx_postfx_motionblur_shutter);
				model_ubo->update(model_ubs);
				m_pfx_motionblur->apply({vp.main_tex.get(), vp.main_texv.get()}, vp.main2_fb.get());

                vp.main_fb->setup_drawing(1); // restore draw buffers after blit operation
            }
			else
			{
				vp.main2_fb->clear(GL_COLOR_BUFFER_BIT);
				vp.msaa_fb->blit_to(vp.main2_fb.get(), vp.width, vp.height, GL_COLOR_BUFFER_BIT, GL_COLOR_ATTACHMENT0);
			}

			if (!Global.gfx_usegles && !Global.gfx_shadergamma)
				glEnable(GL_FRAMEBUFFER_SRGB);

            glViewport(0, 0, target_size.x, target_size.y);

            if( Global.gfx_postfx_chromaticaberration_enabled ) {
                // NOTE: for some unexplained reason need this setup_drawing() call here for the tonemapping effects to show up on the main_tex?
                m_pfx_tonemapping->apply( *vp.main2_tex, vp.main_fb.get() );
                m_pfx_chromaticaberration->apply( *vp.main_tex, nullptr );
            }
            else {
                m_pfx_tonemapping->apply( *vp.main2_tex, nullptr );
            }

            opengl_texture::reset_unit_cache();
		}

		if (!Global.gfx_usegles && !Global.gfx_shadergamma)
			glDisable(GL_FRAMEBUFFER_SRGB);

		glDebug("uilayer render");
		Timer::subsystem.gfx_gui.start();

        if (vp.main) {
			draw_debug_ui();
			Application.render_ui();
		}

        Timer::subsystem.gfx_gui.stop();

		// restore binding
		scene_ubo->bind_uniform();

		glDebug("rendermode::color end");
		break;
	}

	case rendermode::shadows:
	{
		if (!simulation::is_ready)
			break;

		glDebug("rendermode::shadows");

        glEnable(GL_DEPTH_TEST);
        setup_drawing( false );

		glViewport(0, 0, m_shadowbuffersize, m_shadowbuffersize);

        float csmstageboundaries[] = { 0.0f, 5.0f, 25.0f, 1.0f };
        if( Global.shadowtune.map_size > 2048 ) {
            // increase coverage if our shadow map is large enough to produce decent results
            csmstageboundaries[ 1 ] *= Global.shadowtune.map_size / 2048;
            csmstageboundaries[ 2 ] *= Global.shadowtune.map_size / 2048;
        }

        for( auto idx = 0; idx < m_shadowpass.size(); ++idx ) {

            m_shadow_fb->attach( *m_shadow_tex, GL_DEPTH_ATTACHMENT, idx );
            m_shadow_fb->clear( GL_DEPTH_BUFFER_BIT );

            setup_pass( vp, m_renderpass, rendermode::shadows, csmstageboundaries[ idx ], csmstageboundaries[ idx + 1 ] );
            m_shadowpass[ idx ] = m_renderpass; // store pass config for reference in other stages

            setup_matrices();
            scene_ubs.projection = OpenGLMatrices.data( GL_PROJECTION );
            auto const csmstagezfar {
                ( csmstageboundaries[ idx + 1 ] > 1.f ? csmstageboundaries[ idx + 1 ] : Global.shadowtune.range )
              + ( m_shadowpass.size() - idx ) }; // we can fairly safely add some extra padding in the early stages
            scene_ubs.cascade_end[ idx ] = csmstagezfar * csmstagezfar; // store squared to allow semi-optimized length2 comparisons in the shader
            scene_ubo->update( scene_ubs );

            if( m_shadowcolor != colors::white ) {
                Render( simulation::Region );
                if( idx > 0 ) { continue; } // render cab only in the closest csm stage
                if( !FreeFlyModeFlag && Global.render_cab ) {
                    Render_cab( simulation::Train->Dynamic(), 0.0f, false );
                    Render_cab( simulation::Train->Dynamic(), 0.0f, true );
                }
            }
        }
		m_shadow_fb->unbind();

		glDebug("rendermode::shadows end");

		break;
	}

	case rendermode::reflections:
	{
		if (!simulation::is_ready)
			break;

		glDebug("rendermode::reflections");

		// NOTE: buffer attachment and viewport setup in this mode is handled by the wrapper method
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		m_env_fb->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_env_fb->bind();

		setup_env_map(m_empty_cubemap.get());

		setup_matrices();

		// render
        // environment...
		setup_drawing(true);
        setup_shadow_unbind_map();
		scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
		scene_ubo->update(scene_ubs);
		Render(&simulation::Environment);
		// opaque parts...
		setup_drawing(false);
		setup_shadow_bind_map();
		scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
		scene_ubo->update(scene_ubs);
		Render(simulation::Region);

		m_env_fb->unbind();

		glDebug("rendermode::reflections end");

		break;
	}

	case rendermode::pickcontrols:
	{
		if (!simulation::is_ready || !simulation::Train)
			break;

		glDebug("rendermode::pickcontrols");

		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE);
		m_pick_fb->bind();
		m_pick_fb->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_pickcontrolsitems.clear();
		setup_matrices();
		setup_drawing(false);

		scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
		scene_ubo->update(scene_ubs);
		if (simulation::Train != nullptr) {
			Render_cab(simulation::Train->Dynamic(), 0.0f);
			Render(simulation::Train->Dynamic());
		}

		m_pick_fb->unbind();

		glDebug("rendermode::pickcontrols end");
		break;
	}

	case rendermode::pickscenery:
	{
		if (!simulation::is_ready)
			break;

		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE);
		m_pick_fb->bind();
		m_pick_fb->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_picksceneryitems.clear();
		setup_matrices();
		setup_drawing(false);

		scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
		scene_ubo->update(scene_ubs);
 		Render(simulation::Region);

		break;
	}

	default:
	{
		break;
	}
	}
}


bool opengl33_renderer::Render_interior( bool const Alpha ) {
    // TODO: early frustum based cull, camera might be pointing elsewhere
    std::vector< std::pair<float, TDynamicObject *> > dynamics;
    auto *dynamic { simulation::Train->Dynamic() };
    // draw interiors of the occupied vehicle, and all vehicles behind it with permanent coupling, in case they're open-ended
    while( dynamic != nullptr ) {

        glm::dvec3 const originoffset { dynamic->vPosition - m_renderpass.pass_camera.position() };
        float const squaredistance{ glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor ) };
        dynamics.emplace_back( squaredistance, dynamic );
        dynamic = dynamic->NextC( coupling::permanent );
    }
    // draw also interiors of permanently coupled vehicles in front, if there's any
    dynamic = simulation::Train->Dynamic()->PrevC( coupling::permanent );
    while( dynamic != nullptr ) {

        glm::dvec3 const originoffset { dynamic->vPosition - m_renderpass.pass_camera.position() };
        float const squaredistance{ glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor ) };
        dynamics.emplace_back( squaredistance, dynamic );
        dynamic = dynamic->PrevC( coupling::permanent );
    }
    if( Alpha ) {
        std::sort(
            std::begin( dynamics ), std::end( dynamics ),
            []( std::pair<float, TDynamicObject *> const &Left, std::pair<float, TDynamicObject *> const &Right ) {
               return ( Left.first ) > ( Right.first ); } );
    }
    for( auto &dynamic : dynamics ) {
        Render_lowpoly( dynamic.second, dynamic.first, true, Alpha );
    }

    return true;
}

bool opengl33_renderer::Render_lowpoly( TDynamicObject *Dynamic, float const Squaredistance, bool const Setup, bool const Alpha ) {

    if( Dynamic->mdLowPolyInt == nullptr ) { return false; }

    // low poly interior
    if( Setup ) {

        TSubModel::iInstance = reinterpret_cast<std::uintptr_t>( Dynamic ); //żeby nie robić cudzych animacji
        glm::dvec3 const originoffset{ Dynamic->vPosition - m_renderpass.pass_camera.position() };

        Dynamic->ABuLittleUpdate( Squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu

        glm::mat4 mv = OpenGLMatrices.data( GL_MODELVIEW );

        ::glPushMatrix();
        ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
        ::glMultMatrixd( Dynamic->mMatrix.readArray() );
    }
    // HACK: reduce light level for vehicle interior if there's strong global lighting source
    if( false == Alpha ) {
/*
        auto const luminance { static_cast<float>( std::min(
            ( Dynamic->fShade > 0.0 ? Dynamic->fShade * Global.fLuminance : Global.fLuminance ),
            Global.fLuminance - 0.5 * ( std::max( 0.3, Global.fLuminance - Global.Overcast ) ) ) ) };
*/
        auto const luminance{ static_cast<float>(
            0.5
            * ( std::max(
                0.3,
                ( Global.fLuminance - Global.Overcast )
                * ( Dynamic->fShade > 0.0 ?
                    Dynamic->fShade :
                    1.0 ) ) ) ) };
        setup_sunlight_intensity(
            clamp( (
                Dynamic->fShade > 0.f ?
                    Dynamic->fShade :
                    1.f )
                - luminance,
                0.f, 1.f ) );
        Render( Dynamic->mdLowPolyInt, Dynamic->Material(), Squaredistance );
        // HACK: if the model has low poly interior, we presume the load is placed inside and also affected by reduced light level
        if( Dynamic->mdLoad ) {
            // renderowanie nieprzezroczystego ładunku
            Render( Dynamic->mdLoad, Dynamic->Material(), Squaredistance, { 0.f, Dynamic->LoadOffset, 0.f }, {} );
        }
        setup_sunlight_intensity( Dynamic->fShade > 0.f ? Dynamic->fShade : 1.f );
    }
    else {
//        Render_Alpha( Dynamic->mdLowPolyInt, Dynamic->Material(), Squaredistance );
        // HACK: some models have windows included as part of the main model instead of lowpoly
        if( Dynamic->mdModel ) {
            // main model
            Render_Alpha( Dynamic->mdModel, Dynamic->Material(), Squaredistance );
        }
    }

    if( Setup ) {

        if( Dynamic->fShade > 0.0f ) {
            // restore regular light level
            setup_sunlight_intensity();
        }

        ::glPopMatrix();

        // TODO: check if this reset is needed. In theory each object should render all parts based on its own instance data anyway?
        if( Dynamic->btnOn ) {
            Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli
        }
    }

    return true;
}

bool opengl33_renderer::Render_coupler_adapter( TDynamicObject *Dynamic, float const Squaredistance, int const End, bool const Alpha ) {

    if( Dynamic->m_coupleradapters[ End ] == nullptr ) { return false; }

    auto const position { Math3D::vector3 {
        0.f,
        Dynamic->MoverParameters->Couplers[ End ].adapter_height,
        ( Dynamic->MoverParameters->Couplers[ End ].adapter_length + Dynamic->MoverParameters->Dim.L * 0.5 ) * ( End == end::front ? 1 : -1 ) } };

    auto const angle { glm::vec3{
        0,
        ( End == end::front ? 0 : 180 ),
        0 } };

    if( Alpha ) {
        Render_Alpha( Dynamic->m_coupleradapters[ End ], Dynamic->Material(), Squaredistance, position, angle );
    }
    else {
        Render( Dynamic->m_coupleradapters[ End ], Dynamic->Material(), Squaredistance, position, angle );
    }

    return true;
}

// creates dynamic environment cubemap
bool opengl33_renderer::Render_reflections(viewport_config &vp)
{
    if( Global.reflectiontune.update_interval == 0 ) { return false; }

    auto const timestamp{ Timer::GetRenderTime() };
    if( ( timestamp - m_environmentupdatetime < Global.reflectiontune.update_interval )
     && ( glm::length( m_renderpass.pass_camera.position() - m_environmentupdatelocation ) < 1000.0 ) ) {
        // run update every 5+ mins of simulation time, or at least 1km from the last location
        return false;
    }
    m_environmentupdatetime = timestamp;
	m_environmentupdatelocation = m_renderpass.pass_camera.position();

	glViewport(0, 0, gl::ENVMAP_SIZE, gl::ENVMAP_SIZE);
	for (m_environmentcubetextureface = 0; m_environmentcubetextureface < 6; ++m_environmentcubetextureface)
	{
		m_env_fb->attach(*m_env_tex, m_environmentcubetextureface, GL_COLOR_ATTACHMENT0);
		if (m_env_fb->is_complete())
			Render_pass(vp, rendermode::reflections);
	}
	m_env_tex->generate_mipmaps();
	m_env_fb->detach(GL_COLOR_ATTACHMENT0);

	return true;
}

glm::mat4 opengl33_renderer::perspective_projection(float fovy, float aspect, float znear, float zfar)
{
	if (GLAD_GL_ARB_clip_control || GLAD_GL_EXT_clip_control)
	{
		const float f = 1.0f / tan(fovy / 2.0f);

		// when clip_control available, use projection matrix with 1..0 Z range and infinite zfar
		return glm::mat4( //
		    f / aspect, 0.0f, 0.0f, 0.0f, //
		    0.0f, f, 0.0f, 0.0f, //
		    0.0f, 0.0f, 0.0f, -1.0f, //
		    0.0f, 0.0f, znear, 0.0f //
		);
	}
	else
		// or use standard matrix but with 1..-1 Z range
		// (reverse Z don't give any extra precision without clip_control, but it is used anyway for consistency)
		return glm::mat4( //
		           1.0f, 0.0f, 0.0f, 0.0f, //
		           0.0f, 1.0f, 0.0f, 0.0f, //
		           0.0f, 0.0f, -1.0f, 0.0f, //
		           0.0f, 0.0f, 0.0f, 1.0f //
		           ) *
		       glm::perspective(fovy, aspect, znear, zfar);
}

glm::mat4 opengl33_renderer::perpsective_frustumtest_projection(float fovy, float aspect, float znear, float zfar)
{
	// for frustum calculation, use standard opengl matrix
	return glm::perspective(fovy, aspect, znear, zfar);
}

glm::mat4 opengl33_renderer::ortho_projection(float l, float r, float b, float t, float znear, float zfar)
{
	glm::mat4 proj = glm::ortho(l, r, b, t, znear, zfar);
	if (GLAD_GL_ARB_clip_control || GLAD_GL_EXT_clip_control)
		// when clip_control available, use projection matrix with 1..0 Z range
		return glm::mat4( //
		           1.0f, 0.0f, 0.0f, 0.0f, //
		           0.0f, 1.0f, 0.0f, 0.0f, //
		           0.0f, 0.0f, -0.5f, 0.0f, //
		           0.0f, 0.0f, 0.5f, 1.0f //
		           ) *
		       proj;
	else
		// or use standard matrix but with 1..-1 Z range
		// (reverse Z don't give any extra precision without clip_control, but it is used anyway for consistency)
		return glm::mat4( //
		           1.0f, 0.0f, 0.0f, 0.0f, //
		           0.0f, 1.0f, 0.0f, 0.0f, //
		           0.0f, 0.0f, -1.0f, 0.0f, //
		           0.0f, 0.0f, 0.0f, 1.0f //
		           ) *
		       proj;
}

glm::mat4 opengl33_renderer::ortho_frustumtest_projection(float l, float r, float b, float t, float znear, float zfar)
{
	// for frustum calculation, use standard opengl matrix
	return glm::ortho(l, r, b, t, znear, zfar);
}

void opengl33_renderer::setup_pass(viewport_config &Viewport, renderpass_config &Config, rendermode const Mode,
                                 float const Znear, float const Zfar, bool const Ignoredebug)
{
	Config.draw_mode = Mode;

    if( false == simulation::is_ready ) { return; }
    // setup draw range
    switch( Mode ) {
        case rendermode::color:        { Config.draw_range = Global.BaseDrawRange * Global.fDistanceFactor * Viewport.draw_range; break; }
        case rendermode::shadows:      { Config.draw_range = Global.shadowtune.range; break; }
        case rendermode::reflections:  { Config.draw_range = Global.BaseDrawRange * Viewport.draw_range; break; }
        case rendermode::pickcontrols: { Config.draw_range = 50.f; break; }
        case rendermode::pickscenery:  { Config.draw_range = Global.BaseDrawRange * Viewport.draw_range * 0.5f; break; }
        default:                       { Config.draw_range = 0.f; break; }
    }

	// setup camera
	auto &camera = Config.pass_camera;

	glm::dmat4 viewmatrix(1.0);

	glm::mat4 frustumtest_proj;

	glm::ivec2 target_size(Viewport.width, Viewport.height);
	if (Viewport.main) // TODO: update window sizes also for extra viewports
		target_size = glm::ivec2(Global.iWindowWidth, Global.iWindowHeight);

	float const fovy = glm::radians(Global.FieldOfView / Global.ZoomFactor);
	float const aspect = (float)target_size.x / std::max(1.f, (float)target_size.y);

	Config.viewport_camera.position() = Global.pCamera.Pos;

	switch (Mode)
	{
	case rendermode::color:
	{
		viewmatrix = glm::dmat4(Viewport.camera_transform);

		// modelview
		if ((false == DebugCameraFlag) || (true == Ignoredebug))
		{
			camera.position() = Global.pCamera.Pos;
			Global.pCamera.SetMatrix(viewmatrix);
		}
		else
		{
			camera.position() = Global.pDebugCamera.Pos;
			Global.pDebugCamera.SetMatrix(viewmatrix);
		}

        // optimization: don't bother drawing things completely blended with the background
        // but ensure at least 2 km range to account for signals and lights
        Config.draw_range = std::min( Config.draw_range, m_fogrange * 2 );
        Config.draw_range = std::max( 2000.f, Config.draw_range );

		// projection
		auto const zfar  = ( Zfar  > 1.f ? Zfar : Config.draw_range * Zfar );
		auto const znear = ( Znear > 1.f ? Znear : Znear > 0.f ? Znear * zfar : 0.1f * Global.ZoomFactor);

		camera.projection() = perspective_projection(fovy, aspect, znear, zfar);
		frustumtest_proj = perpsective_frustumtest_projection(fovy, aspect, znear, zfar);
		break;
	}
	case rendermode::shadows:
	{
		// calculate lightview boundaries based on relevant area of the world camera frustum:
		// ...setup chunk of frustum we're interested in...
//        auto const zfar = std::min(1.f, Global.shadowtune.range / (Global.BaseDrawRange * Global.fDistanceFactor) * std::max(1.f, Global.ZoomFactor * 0.5f));
//        auto const zfar = ( Zfar > 1.f ? Zfar * std::max( 1.f, Global.ZoomFactor * 0.5f ) : Zfar );
        auto const zfar = ( Zfar > 1.f ? Zfar : Zfar * Global.shadowtune.range * std::max( Zfar, Global.ZoomFactor * 0.5f ) );
		renderpass_config worldview;
		setup_pass(Viewport, worldview, rendermode::color, Znear, zfar, true);
		auto &frustumchunkshapepoints = worldview.pass_camera.frustum_points();
		// ...modelview matrix: determine the centre of frustum chunk in world space...
		glm::vec3 frustumchunkmin, frustumchunkmax;
		bounding_box(frustumchunkmin, frustumchunkmax, std::begin(frustumchunkshapepoints), std::end(frustumchunkshapepoints));
		auto const frustumchunkcentre = (frustumchunkmin + frustumchunkmax) * 0.5f;
		// ...cap the vertical angle to keep shadows from getting too long...
		auto const lightvector = glm::normalize(glm::vec3{m_sunlight.direction.x, std::min(m_sunlight.direction.y, -0.2f), m_sunlight.direction.z});
		// ...place the light source at the calculated centre and setup world space light view matrix...
		camera.position() = worldview.pass_camera.position() + glm::dvec3{frustumchunkcentre};
		viewmatrix *= glm::lookAt(camera.position(), camera.position() + glm::dvec3{lightvector}, glm::dvec3{0.f, 1.f, 0.f});
		// ...projection matrix: calculate boundaries of the frustum chunk in light space...
		auto const lightviewmatrix = glm::translate(glm::mat4{glm::mat3{viewmatrix}}, -frustumchunkcentre);
		for (auto &point : frustumchunkshapepoints)
		{
			point = lightviewmatrix * point;
		}
		bounding_box(frustumchunkmin, frustumchunkmax, std::begin(frustumchunkshapepoints), std::end(frustumchunkshapepoints));
		// quantize the frustum points and add some padding, to reduce shadow shimmer on scale changes
        auto const frustumchunkdepth { zfar - ( Znear > 1.f ? Znear : 0.f ) };
		auto const quantizationstep{std::min(frustumchunkdepth, 50.f)};
		frustumchunkmin = quantizationstep * glm::floor(frustumchunkmin * (1.f / quantizationstep));
		frustumchunkmax = quantizationstep * glm::ceil(frustumchunkmax * (1.f / quantizationstep));
		// ...use the dimensions to set up light projection boundaries...
		// NOTE: since we only have one cascade map stage, we extend the chunk forward/back to catch areas normally covered by other stages
		camera.projection() = ortho_projection(frustumchunkmin.x, frustumchunkmax.x, frustumchunkmin.y, frustumchunkmax.y, frustumchunkmin.z - 500.f, frustumchunkmax.z + 500.f);
		frustumtest_proj = ortho_frustumtest_projection(frustumchunkmin.x, frustumchunkmax.x, frustumchunkmin.y, frustumchunkmax.y, frustumchunkmin.z - 500.f, frustumchunkmax.z + 500.f);
		// ... and adjust the projection to sample complete shadow map texels:
		// get coordinates for a sample texel...
		auto shadowmaptexel = glm::vec2{camera.projection() * glm::mat4{viewmatrix} * glm::vec4{0.f, 0.f, 0.f, 1.f}};
		// ...convert result from clip space to texture coordinates, and calculate adjustment...
		shadowmaptexel *= m_shadowbuffersize * 0.5f;
		auto shadowmapadjustment = glm::round(shadowmaptexel) - shadowmaptexel;
		// ...transform coordinate change back to homogenous light space...
		shadowmapadjustment /= m_shadowbuffersize * 0.5f;
		// ... and bake the adjustment into the projection matrix
		camera.projection() = glm::translate(glm::mat4{1.f}, glm::vec3{shadowmapadjustment, 0.f}) * camera.projection();

		break;
	}
	case rendermode::pickcontrols:
	case rendermode::pickscenery:
	{
		viewmatrix = glm::dmat4(Viewport.camera_transform);

		// modelview
		camera.position() = Global.pCamera.Pos;
		Global.pCamera.SetMatrix(viewmatrix);

		// projection
		float znear = 0.1f * Global.ZoomFactor;
		float zfar = Config.draw_range * Global.fDistanceFactor;
		camera.projection() = perspective_projection(fovy, aspect, znear, zfar);
		frustumtest_proj = perpsective_frustumtest_projection(fovy, aspect, znear, zfar);
		break;
	}
	case rendermode::reflections:
	{
		// modelview
		camera.position() = (((true == DebugCameraFlag) && (false == Ignoredebug)) ? Global.pDebugCamera.Pos : Global.pCamera.Pos);
		glm::dvec3 const cubefacetargetvectors[6] = {{1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}};
		glm::dvec3 const cubefaceupvectors[6] = {{0.0, -1.0, 0.0}, {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}, {0.0, -1.0, 0.0}, {0.0, -1.0, 0.0}};
		auto const cubefaceindex = m_environmentcubetextureface;
		viewmatrix *= glm::lookAt(camera.position(), camera.position() + cubefacetargetvectors[cubefaceindex], cubefaceupvectors[cubefaceindex]);
		// projection
		float znear = 0.1f * Global.ZoomFactor;
		float zfar = Config.draw_range * Global.fDistanceFactor;
		camera.projection() = perspective_projection(glm::radians(90.f), 1.f, znear, zfar);
		frustumtest_proj = perpsective_frustumtest_projection(glm::radians(90.f), 1.f, znear, zfar);
		break;
	}
	default:
	{
		break;
	}
	}
	camera.modelview() = viewmatrix;
	camera.update_frustum(frustumtest_proj);
}

void opengl33_renderer::setup_matrices()
{
    OpenGLMatrices.mode(GL_PROJECTION);
	OpenGLMatrices.load_matrix(m_renderpass.pass_camera.projection());
	// trim modelview matrix just to rotation, since rendering is done in camera-centric world space
    OpenGLMatrices.mode(GL_MODELVIEW);
    OpenGLMatrices.load_matrix(glm::mat4{glm::mat3{m_renderpass.pass_camera.modelview()}});
}

void opengl33_renderer::setup_drawing(bool const Alpha)
{
	if (Alpha)
	{
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
		m_blendingenabled = true;
	}
	else
	{
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		m_blendingenabled = false;
	}

	switch (m_renderpass.draw_mode)
	{
	case rendermode::color:
	case rendermode::reflections:
	{
		glCullFace(GL_BACK);
		break;
	}
	case rendermode::shadows:
	{
		glCullFace(GL_FRONT);
		break;
	}
	case rendermode::pickcontrols:
	case rendermode::pickscenery:
	{
		glCullFace(GL_BACK);
		break;
	}
	default:
	{
		break;
	}
	}
}

void opengl33_renderer::setup_shadow_unbind_map()
{
    opengl_texture::unbind( gl::SHADOW_TEX );
}

// binds shadow map and updates shadow map uniform data
void opengl33_renderer::setup_shadow_bind_map()
{
    if( false == Global.gfx_shadowmap_enabled ) { return; }

    m_shadow_tex->bind(gl::SHADOW_TEX);

	glm::mat4 coordmove;

	if (GLAD_GL_ARB_clip_control || GLAD_GL_EXT_clip_control)
		// transform 1..-1 NDC xy coordinates to 1..0
		coordmove = glm::mat4( //
			0.5, 0.0, 0.0, 0.0, //
			0.0, 0.5, 0.0, 0.0, //
			0.0, 0.0, 1.0, 0.0, //
			0.5, 0.5, 0.0, 1.0 //
		);
	else
		// without clip_control we also need to transform z
		coordmove = glm::mat4( //
			0.5, 0.0, 0.0, 0.0, //
			0.0, 0.5, 0.0, 0.0, //
			0.0, 0.0, 0.5, 0.0, //
			0.5, 0.5, 0.5, 1.0 //
		);

    for( auto idx = 0; idx < m_shadowpass.size(); ++idx ) {

        glm::mat4 depthproj = m_shadowpass[ idx ].pass_camera.projection();
        // NOTE: we strip transformations from camera projections to remove jitter that occurs
        // with large (and unneded as we only need the offset) transformations back and forth
        auto const depthcam{ glm::mat3{ m_shadowpass[ idx ].pass_camera.modelview()} };
        auto const worldcam{ glm::mat3{ m_renderpass.pass_camera.modelview()} };

        scene_ubs.lightview[ idx ] =
            coordmove
            * depthproj
            * glm::translate(
                glm::mat4{ depthcam },
                glm::vec3{ m_renderpass.pass_camera.position() - m_shadowpass[ idx ].pass_camera.position() } )
            * glm::mat4{ glm::inverse( worldcam ) };
    }
	scene_ubo->update(scene_ubs);
}

void opengl33_renderer::setup_shadow_color( glm::vec4 const &Shadowcolor ) {

    model_ubs.shadow_tone = glm::pow( Shadowcolor.x, 2.2f );
}

void opengl33_renderer::setup_env_map(gl::cubemap *tex)
{
	if (tex)
	{
		tex->bind(GL_TEXTURE0 + gl::ENV_TEX);
		glActiveTexture(GL_TEXTURE0);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + gl::ENV_TEX);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glActiveTexture(GL_TEXTURE0);
	}
	opengl_texture::reset_unit_cache();
}

void opengl33_renderer::setup_environment_light(TEnvironmentType const Environment)
{

	switch (Environment)
	{
	case e_flat:
	{
        setup_sunlight_intensity();
		//            m_environment = Environment;
		break;
	}
	case e_canyon:
	{
		setup_sunlight_intensity(0.4f);
		//            m_environment = Environment;
		break;
	}
	case e_tunnel:
	{
		setup_sunlight_intensity(0.2f);
		//            m_environment = Environment;
		break;
	}
	default:
	{
		break;
	}
	}
}

void opengl33_renderer::setup_sunlight_intensity( float const Factor ) {

    m_sunlight.apply_intensity( Factor );
    light_ubs.lights[ 0 ].intensity = m_sunlight.factor;
    light_ubs.ambient = m_sunlight.ambient * m_sunlight.factor;
    light_ubo->update( light_ubs );
}

bool opengl33_renderer::Render(world_environment *Environment)
{
    m_shadowcolor = colors::white; // prevent shadow from affecting sky
    setup_shadow_color( m_shadowcolor );

	if (Global.bWireFrame)
	{
		// bez nieba w trybie rysowania linii
		return false;
	}

	Bind_Material(null_handle);
	::glDisable(GL_DEPTH_TEST);
	::glPushMatrix();

	// skydome
	// drawn with 500m radius to blend in if the fog range is low
	glPushMatrix();
    {
        glScalef( 500.0f, 500.0f, 500.0f );
        model_ubs.set_modelview( OpenGLMatrices.data( GL_MODELVIEW ) );
        model_ubo->update( model_ubs );

        m_skydomerenderer.update();
        m_skydomerenderer.render();
        // skydome uses a custom vbo which could potentially confuse the main geometry system. hardly elegant but, eh
        gfx::opengl_vbogeometrybank::reset();
    }
    glPopMatrix();

    ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    // stars
	if (Environment->m_stars.m_stars != nullptr)
	{
		// setup
		::glPushMatrix();
		::glRotatef(Environment->m_stars.m_latitude, 1.f, 0.f, 0.f); // ustawienie osi OY na północ
		::glRotatef(-std::fmod((float)Global.fTimeAngleDeg, 360.f), 0.f, 1.f, 0.f); // obrót dobowy osi OX

		// render
        m_pointsize = ( 4.f ); // smaller points for stars visualization
		Render(Environment->m_stars.m_stars, nullptr, 1.0);
        m_pointsize = ( 8.f ); // default point size

		// post-render cleanup
		::glPopMatrix();
	}

	// celestial bodies
	m_celestial_shader->bind();
	m_empty_vao->bind();

	auto const &modelview = OpenGLMatrices.data(GL_MODELVIEW);

    auto const fogfactor{clamp(Global.fFogEnd / 2000.f, 0.f, 1.f)}; // stronger fog reduces opacity of the celestial bodies
	float const duskfactor = 1.0f - clamp(std::abs(Environment->m_sun.getAngle()), 0.0f, 12.0f) / 12.0f;
	glm::vec3 suncolor = interpolate(glm::vec3(255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f), glm::vec3(235.0f / 255.0f, 140.0f / 255.0f, 36.0f / 255.0f), duskfactor);

	// sun
	{
		Bind_Texture(0, m_suntexture);
		glm::vec4 color(suncolor.x, suncolor.y, suncolor.z, clamp(1.5f - Global.Overcast, 0.f, 1.f) * fogfactor);
		auto const sunvector = Environment->m_sun.getDirection();

        float const size = interpolate( // TODO: expose distance/scale factor from the moon object
            0.0325f,
            0.0275f,
            clamp( Environment->m_sun.getAngle(), 0.f, 90.f ) / 90.f );
		model_ubs.param[0] = color;
		model_ubs.param[1] = glm::vec4(glm::vec3(modelview * glm::vec4(sunvector, 1.0f)), /*0.00463f*/ size);
		model_ubs.param[2] = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);
		model_ubo->update(model_ubs);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	// moon
	{
		Bind_Texture(0, m_moontexture);
		glm::vec3 mooncolor(255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f);
		glm::vec4 color(mooncolor.r, mooncolor.g, mooncolor.b,
		                // fade the moon if it's near the sun in the sky, especially during the day
		                std::max<float>(0.f, 1.0 - 0.5 * Global.fLuminance - 0.65 * std::max(0.f, glm::dot(Environment->m_sun.getDirection(), Environment->m_moon.getDirection()))) * fogfactor);

		auto const moonvector = Environment->m_moon.getDirection();

		// choose the moon appearance variant, based on current moon phase
		// NOTE: implementation specific, 8 variants are laid out in 3x3 arrangement
		// from new moon onwards, top left to right bottom (last spot is left for future use, if any)
		auto const moonphase = Environment->m_moon.getPhase();
		float moonu, moonv;
		if (moonphase < 1.84566f)
		{
			moonv = 1.0f - 0.0f;
			moonu = 0.0f;
		}
		else if (moonphase < 5.53699f)
		{
			moonv = 1.0f - 0.0f;
			moonu = 0.333f;
		}
		else if (moonphase < 9.22831f)
		{
			moonv = 1.0f - 0.0f;
			moonu = 0.667f;
		}
		else if (moonphase < 12.91963f)
		{
			moonv = 1.0f - 0.333f;
			moonu = 0.0f;
		}
		else if (moonphase < 16.61096f)
		{
			moonv = 1.0f - 0.333f;
			moonu = 0.333f;
		}
		else if (moonphase < 20.30228f)
		{
			moonv = 1.0f - 0.333f;
			moonu = 0.667f;
		}
		else if (moonphase < 23.99361f)
		{
			moonv = 1.0f - 0.667f;
			moonu = 0.0f;
		}
		else if (moonphase < 27.68493f)
		{
			moonv = 1.0f - 0.667f;
			moonu = 0.333f;
		}
		else if (moonphase == 50) //9th slot used for easter egg
		{
			moonv = 1.0f - 0.667f;
			moonu = 0.66f;
		}
		else
		{
			moonv = 1.0f - 0.0f;
			moonu = 0.0f;
		}

        float const size = interpolate( // TODO: expose distance/scale factor from the moon object
            0.0160f,
            0.0135f,
            clamp( Environment->m_moon.getAngle(), 0.f, 90.f ) / 90.f );

		model_ubs.param[0] = color;
		model_ubs.param[1] = glm::vec4(glm::vec3(modelview * glm::vec4(moonvector, 1.0f)), /*0.00451f*/ size);
		model_ubs.param[2] = glm::vec4(moonu, moonv, 0.333f, 0.0f);
		model_ubo->update(model_ubs);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

    ::glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  	// clouds
	if (Environment->m_clouds.mdCloud)
	{
		// setup
/*
		glm::vec3 color =
            interpolate(
                Environment->m_skydome.GetAverageColor(), suncolor,
                duskfactor * 0.25f)
            * interpolate(
                1.f, 0.35f,
                Global.Overcast / 2.f) // overcast darkens the clouds
            * 0.5f;
*/
		auto const color {
			glm::clamp(
			    ( glm::vec3 { Global.DayLight.ambient }
                + 0.35f * glm::vec3{ Global.DayLight.diffuse } ) * simulation::Environment.light_intensity()
                * 0.5f,
			    glm::vec3{ 0.f }, glm::vec3{ 1.f } ) };

		// write cloud color into material
		TSubModel *mdl = Environment->m_clouds.mdCloud->Root;
		if (mdl->m_material != null_handle)
			m_materials.material(mdl->m_material).params[0] = glm::vec4(color, 1.0f);

		// render
		Render(Environment->m_clouds.mdCloud, nullptr, 100.0);
		Render_Alpha(Environment->m_clouds.mdCloud, nullptr, 100.0);
		// post-render cleanup
	}

	gl::program::unbind();
	gl::vao::unbind();
	::glPopMatrix();
	::glEnable(GL_DEPTH_TEST);

	m_sunlight.apply_angle();
	m_sunlight.apply_intensity();

    // calculate shadow tone, based on positions of celestial bodies
	m_shadowcolor = interpolate(glm::vec4{colors::shadow}, glm::vec4{colors::white}, clamp(-Environment->m_sun.getAngle(), 0.f, 6.f) / 6.f);
	if ((Environment->m_sun.getAngle() < -18.f) && (Environment->m_moon.getAngle() > 0.f))
	{
		// turn on moon shadows after nautical twilight, if the moon is actually up
		m_shadowcolor = colors::shadow;
	}
	// soften shadows depending on sky overcast factor
	m_shadowcolor = glm::min(colors::white, m_shadowcolor + ((colors::white - colors::shadow) * Global.Overcast));

    setup_shadow_color( m_shadowcolor);

	return true;
}

// geometry methods
// creates a new geometry bank. returns: handle to the bank or NULL
gfx::geometrybank_handle opengl33_renderer::Create_Bank()
{

	return m_geometry.create_bank();
}

// creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle opengl33_renderer::Insert( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type )
{
    // NOTE: we expect indexed geometry to come with calculated tangents
    return m_geometry.create_chunk( Indices, Vertices, Geometry, Type );
}

// creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle opengl33_renderer::Insert(gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type)
{
	gfx::calculate_tangents(Vertices, gfx::index_array(), Type);

	return m_geometry.create_chunk(Vertices, Geometry, Type);
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool opengl33_renderer::Replace(gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset)
{
	gfx::calculate_tangents(Vertices, gfx::index_array(), Type);

	return m_geometry.replace(Vertices, Geometry, Offset);
}

// adds supplied vertex data at the end of specified chunk
bool opengl33_renderer::Append(gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type)
{
	gfx::calculate_tangents(Vertices, gfx::index_array(), Type);

	return m_geometry.append(Vertices, Geometry);
}

// provides direct access to index data of specfied chunk
gfx::index_array const & opengl33_renderer::Indices(gfx::geometry_handle const &Geometry) const 
{
    return m_geometry.indices(Geometry);
}

// provides direct access to vertex data of specfied chunk
gfx::vertex_array const &opengl33_renderer::Vertices(gfx::geometry_handle const &Geometry) const
{
	return m_geometry.vertices(Geometry);
}

// material methods
material_handle opengl33_renderer::Fetch_Material(std::string const &Filename, bool const Loadnow)
{

	return m_materials.create(Filename, Loadnow);
}

std::shared_ptr<gl::program> opengl33_renderer::Fetch_Shader(const std::string &name)
{
	auto it = m_shaders.find(name);
	if (it == m_shaders.end())
	{
		gl::shader fragment("mat_" + name + ".frag");
		gl::program *program = new gl::program({fragment, *m_vertex_shader.get()});
		m_shaders.insert({name, std::shared_ptr<gl::program>(program)});
	}

	return m_shaders[name];
}

void opengl33_renderer::Bind_Material( material_handle const Material, TSubModel const *sm, lighting_data const *lighting )
{
	if (Material != null_handle)
	{
        auto const &material { m_materials.material( Material ) };

        if( false == material.is_good ) {
            // use failure indicator instead
            if( Material != m_invalid_material ) {
                Bind_Material( m_invalid_material );
            }
            return;
        }

		memcpy(&model_ubs.param[0], &material.params[0], sizeof(model_ubs.param));

		for( auto const &entry : material.params_state )
		{
			glm::vec4 src(1.0f);

            // submodel-based parameters
			if (sm)
			{
                switch( entry.defaultparam ) {
                    case gl::shader::defaultparam_e::ambient: {
                        src = sm->f4Ambient;
                        break;
                    }
                    case gl::shader::defaultparam_e::diffuse: {
                        src = sm->f4Diffuse;
//                        src = glm::vec4( glm::pow( glm::vec3( sm->f4Diffuse ), gammacorrection ), sm->f4Diffuse.a );
                        break;
                    }
                    case gl::shader::defaultparam_e::specular: {
                        src = sm->f4Specular;// *( sm->Opacity > 0.f ? m_specularopaquescalefactor : m_speculartranslucentscalefactor );
                        break;
                    }
                    default: {
                        break;
                    }
                }
			}
            if( lighting )                 
            {
                switch( entry.defaultparam ) {
                    case gl::shader::defaultparam_e::ambient: {
                        src = lighting->ambient;
                        break;
                    }
                    case gl::shader::defaultparam_e::diffuse: {
                        src = lighting->diffuse;
                        break;
                    }
                    case gl::shader::defaultparam_e::specular: {
                        src = lighting->specular;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            // material-based parameters
            switch( entry.defaultparam ) {
                case gl::shader::defaultparam_e::glossiness: {
                    src = glm::vec4 { material.glossiness };
                }
                default: {
                    break;
                }
            }

            if( entry.size == 1 ) {
                // HACK: convert color to luminosity, if it's passed as single value
                src = glm::vec4 { colors::RGBtoHSV( glm::vec3 { src } ).z };
            }
			for (size_t j = 0; j < entry.size; j++)
				model_ubs.param[entry.location][entry.offset + j] = src[j];
		}

        if( material.opacity ) {
            model_ubs.opacity = (
                m_blendingenabled ?
                    -material.opacity.value() :
                     material.opacity.value() );
        }
        else {
            model_ubs.opacity = (
                m_blendingenabled ?
                     0.0f :
                     0.5f );
        }

		if (sm)
			model_ubs.alpha_mult = sm->fVisible;
		else
			model_ubs.alpha_mult = 1.0f;

		if (GLAD_GL_ARB_multi_bind)
		{
			GLuint lastdiff = 0;
			size_t i;
			for (i = 0; i < gl::MAX_TEXTURES; i++)
				if (material.textures[i] != null_handle)
				{
					opengl_texture &tex = m_textures.mark_as_used(material.textures[i]);
					tex.create();
					if (opengl_texture::units[i] != tex.id)
					{
						opengl_texture::units[i] = tex.id;
						lastdiff = i + 1;
					}
				}
				else
					break;

			if (lastdiff)
				glBindTextures(0, lastdiff, &opengl_texture::units[0]);
		}
		else
		{
			size_t unit = 0;
			for (auto &tex : material.textures)
			{
				if (tex == null_handle)
					break;
				m_textures.bind(unit, tex);
				unit++;
			}
		}

		material.shader->bind();
	}
	else if (Material != m_invalid_material)
		Bind_Material(m_invalid_material);
}

void opengl33_renderer::Bind_Material_Shadow(material_handle const Material)
{
	if (Material != null_handle)
	{
		auto &material = m_materials.material(Material);

		if (material.textures[0] != null_handle)
		{
			m_textures.bind(0, material.textures[0]);
			m_alpha_shadow_shader->bind();
		}
        else {
            m_shadow_shader->bind();
        }
	}
    else {
        m_shadow_shader->bind();
    }
}

opengl_material const &opengl33_renderer::Material(material_handle const Material) const
{
	return m_materials.material(Material);
}

opengl_material &opengl33_renderer::Material(material_handle const Material)
{
	return m_materials.material(Material);
}

texture_handle opengl33_renderer::Fetch_Texture(std::string const &Filename, bool const Loadnow, GLint format_hint)
{
	return m_textures.create(Filename, Loadnow, format_hint);
}

void opengl33_renderer::Bind_Texture( texture_handle const Texture )
{
    return Bind_Texture( 0, Texture );
}

void opengl33_renderer::Bind_Texture(std::size_t const Unit, texture_handle const Texture)
{
	m_textures.bind(Unit, Texture);
}

opengl_texture &opengl33_renderer::Texture(texture_handle const Texture)
{

	return m_textures.texture(Texture);
}

opengl_texture const &opengl33_renderer::Texture(texture_handle const Texture) const
{

	return m_textures.texture(Texture);
}

void opengl33_renderer::Update_AnimModel(TAnimModel *model)
{
	model->RaAnimate(m_framestamp);
}

void opengl33_renderer::Render(scene::basic_region *Region)
{

	m_sectionqueue.clear();
	m_cellqueue.clear();
	// build a list of region sections to render
	glm::vec3 const cameraposition{m_renderpass.pass_camera.position()};
	auto const camerax = static_cast<int>(std::floor(cameraposition.x / scene::EU07_SECTIONSIZE + scene::EU07_REGIONSIDESECTIONCOUNT / 2));
	auto const cameraz = static_cast<int>(std::floor(cameraposition.z / scene::EU07_SECTIONSIZE + scene::EU07_REGIONSIDESECTIONCOUNT / 2));
	int const segmentcount = 2 * static_cast<int>(std::ceil(m_renderpass.draw_range * Global.fDistanceFactor / scene::EU07_SECTIONSIZE));
	int const originx = camerax - segmentcount / 2;
	int const originz = cameraz - segmentcount / 2;

	for (int row = originz; row <= originz + segmentcount; ++row)
	{
		if (row < 0)
		{
			continue;
		}
		if (row >= scene::EU07_REGIONSIDESECTIONCOUNT)
		{
			break;
		}
		for (int column = originx; column <= originx + segmentcount; ++column)
		{
			if (column < 0)
			{
				continue;
			}
			if (column >= scene::EU07_REGIONSIDESECTIONCOUNT)
			{
				break;
			}
			auto *section{Region->m_sections[row * scene::EU07_REGIONSIDESECTIONCOUNT + column]};
			if ((section != nullptr) && (m_renderpass.pass_camera.visible(section->m_area)))
			{
				m_sectionqueue.emplace_back(section);
			}
		}
	}

	switch (m_renderpass.draw_mode)
	{
	case rendermode::color:
	{
		Render(std::begin(m_sectionqueue), std::end(m_sectionqueue));
		// draw queue is filled while rendering sections
		if (EditorModeFlag && m_current_viewport->main)
		{
			// when editor mode is active calculate world position of the cursor
			// at this stage the z-buffer is filled with only ground geometry
			Update_Mouse_Position();
		}
		Render(std::begin(m_cellqueue), std::end(m_cellqueue));
		break;
	}
	case rendermode::shadows:
    case rendermode::pickscenery:
	{
		// these render modes don't bother with lights
		Render(std::begin(m_sectionqueue), std::end(m_sectionqueue));
		// they can also skip queue sorting, as they only deal with opaque geometry
		// NOTE: there's benefit from rendering front-to-back, but is it significant enough? TODO: investigate
		Render(std::begin(m_cellqueue), std::end(m_cellqueue));
		break;
	}
	case rendermode::reflections:
	{
		Render(std::begin(m_sectionqueue), std::end(m_sectionqueue));
        if( Global.reflectiontune.fidelity >= 1 ) {
            Render(std::begin(m_cellqueue), std::end(m_cellqueue));
        }
		break;
	}
	case rendermode::pickcontrols:
	default:
	{
		// no need to render anything ourside of the cab in control picking mode
		break;
	}
	}
}

void opengl33_renderer::Render(section_sequence::iterator First, section_sequence::iterator Last)
{

	switch (m_renderpass.draw_mode)
	{
	case rendermode::color:
	case rendermode::reflections:
    case rendermode::shadows:
		break;
	case rendermode::pickscenery:
	{
		// non-interactive scenery elements get neutral colour
		model_ubs.param[0] = colors::none;
		break;
	}
	default:
		break;
	}

	while (First != Last)
	{

		auto *section = *First;
		section->create_geometry();

		// render shapes held by the section
		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		case rendermode::reflections:
		case rendermode::shadows:
        case rendermode::pickscenery:
		{
			if (false == section->m_shapes.empty())
			{
				// since all shapes of the section share center point we can optimize out a few calls here
				::glPushMatrix();
				auto const originoffset{section->m_area.center - m_renderpass.pass_camera.position()};
				::glTranslated(originoffset.x, originoffset.y, originoffset.z);
				// render
				for (auto const &shape : section->m_shapes)
				{
					Render(shape, true);
				}
				// post-render cleanup
				::glPopMatrix();
			}
			break;
		}
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}

		// add the section's cells to the cell queue
		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		case rendermode::shadows:
        case rendermode::pickscenery:
		{
            // TBD, TODO: refactor into a method to reuse below?
			for (auto &cell : section->m_cells)
			{
				if ((true == cell.m_active) && (m_renderpass.pass_camera.visible(cell.m_area)))
				{
					// store visible cells with content as well as their current distance, for sorting later
					m_cellqueue.emplace_back(glm::length2(m_renderpass.pass_camera.position() - cell.m_area.center), &cell);
				}
			}
			break;
		}
		case rendermode::reflections:
        {
            // we can skip filling the cell queue if reflections pass isn't going to use it
            if( Global.reflectiontune.fidelity == 0 ) { break; }
			for (auto &cell : section->m_cells)
			{
				if ((true == cell.m_active) && (m_renderpass.pass_camera.visible(cell.m_area)))
				{
					// store visible cells with content as well as their current distance, for sorting later
					m_cellqueue.emplace_back(glm::length2(m_renderpass.pass_camera.position() - cell.m_area.center), &cell);
				}
			}
			break;
        }
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}
		// proceed to next section
		++First;
	}

	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
    {
		break;
	}
	default:
	{
		break;
	}
	}
}

void opengl33_renderer::Render(cell_sequence::iterator First, cell_sequence::iterator Last)
{

	// cache initial iterator for the second sweep
	auto first{First};
	// first pass draws elements which we know are located in section banks, to reduce vbo switching
	while (First != Last)
	{

		auto *cell = First->second;
		// przeliczenia animacji torów w sektorze
		cell->RaAnimate(m_framestamp);

		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		{
			// since all shapes of the section share center point we can optimize out a few calls here
			::glPushMatrix();
			auto const originoffset{cell->m_area.center - m_renderpass.pass_camera.position()};
			::glTranslated(originoffset.x, originoffset.y, originoffset.z);

			// render
			// opaque non-instanced shapes
			for (auto const &shape : cell->m_shapesopaque)
			{
				Render(shape, false);
			}
			// tracks
			// TODO: update after path node refactoring
			Render(std::begin(cell->m_paths), std::end(cell->m_paths));
			// post-render cleanup
			::glPopMatrix();

			break;
		}
        case rendermode::reflections:
        {
			// since all shapes of the section share center point we can optimize out a few calls here
			::glPushMatrix();
			auto const originoffset{cell->m_area.center - m_renderpass.pass_camera.position()};
			::glTranslated(originoffset.x, originoffset.y, originoffset.z);

			// render
			// opaque non-instanced shapes
			for (auto const &shape : cell->m_shapesopaque)
				Render(shape, false);

			// post-render cleanup
			::glPopMatrix();

			break;
        }
		case rendermode::shadows:
		{
			// since all shapes of the section share center point we can optimize out a few calls here
			::glPushMatrix();
			auto const originoffset{cell->m_area.center - m_renderpass.pass_camera.position()};
			::glTranslated(originoffset.x, originoffset.y, originoffset.z);

			// render
			// opaque non-instanced shapes
			for (auto const &shape : cell->m_shapesopaque)
				Render(shape, false);
			// tracks
			Render(std::begin(cell->m_paths), std::end(cell->m_paths));

			// post-render cleanup
			::glPopMatrix();

			break;
		}
		case rendermode::pickscenery:
		{
			// same procedure like with regular render, but editor-enabled nodes receive custom colour used for picking
			// since all shapes of the section share center point we can optimize out a few calls here
			::glPushMatrix();
			auto const originoffset{cell->m_area.center - m_renderpass.pass_camera.position()};
			::glTranslated(originoffset.x, originoffset.y, originoffset.z);
			// render
			// opaque non-instanced shapes
			// non-interactive scenery elements get neutral colour
			model_ubs.param[0] = colors::none;
			for (auto const &shape : cell->m_shapesopaque)
				Render(shape, false);
			// tracks
			for (auto *path : cell->m_paths)
			{
				model_ubs.param[0] = glm::vec4(pick_color(m_picksceneryitems.size() + 1), 1.0f);
				Render(path);
			}
			// post-render cleanup
			::glPopMatrix();
			break;
		}
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}

		++First;
	}
	// second pass draws elements with their own vbos
	while (first != Last)
	{

		auto const *cell = first->second;

		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
        case rendermode::shadows:
        {
            // TBD, TODO: refactor in to a method to reuse in branch below?
			// opaque parts of instanced models
			for (auto *instance : cell->m_instancesopaque)
			{
				Render(instance);
			}
			// opaque parts of vehicles
			for (auto *path : cell->m_paths)
			{
				for (auto *dynamic : path->Dynamics)
				{
					Render(dynamic);
				}
			}
			break;
		}
        case rendermode::reflections:
        {
            if( Global.reflectiontune.fidelity >= 1 ) {
                // opaque parts of instanced models
                for( auto *instance : cell->m_instancesopaque ) {
                    Render( instance );
                }
            }
            if( Global.reflectiontune.fidelity >= 2 ) {
                // opaque parts of vehicles
                for( auto *path : cell->m_paths ) {
                    for( auto *dynamic : path->Dynamics ) {
                        Render( dynamic );
                    }
                }
            }
			break;
        }
        case rendermode::pickscenery:
		{
			// opaque parts of instanced models
			// same procedure like with regular render, but each node receives custom colour used for picking
			for (auto *instance : cell->m_instancesopaque)
			{
				model_ubs.param[0] = glm::vec4(pick_color(m_picksceneryitems.size() + 1), 1.0f);
				Render(instance);
			}
			// vehicles aren't included in scenery picking for the time being
			break;
		}
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}

		++first;
	}
}

void opengl33_renderer::Draw_Geometry(std::vector<gfx::geometrybank_handle>::iterator begin, std::vector<gfx::geometrybank_handle>::iterator end)
{
	m_geometry.draw(begin, end);
}

void opengl33_renderer::Draw_Geometry(const gfx::geometrybank_handle &handle)
{
	m_geometry.draw(handle);
}

void opengl33_renderer::draw(const gfx::geometry_handle &handle)
{
	model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
	model_ubo->update(model_ubs);

	m_geometry.draw(handle);
}

void opengl33_renderer::draw(std::vector<gfx::geometrybank_handle>::iterator it, std::vector<gfx::geometrybank_handle>::iterator end)
{
	model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
	model_ubo->update(model_ubs);

	Draw_Geometry(it, end);
}

void opengl33_renderer::Render(scene::shape_node const &Shape, bool const Ignorerange)
{
	auto const &data{Shape.data()};

	if (false == Ignorerange)
	{
		double distancesquared;
		switch (m_renderpass.draw_mode)
		{
		case rendermode::shadows:
        {
			// 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
			distancesquared = Math3D::SquareMagnitude((data.area.center - m_renderpass.viewport_camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
			break;
		}
        case rendermode::reflections:
        {
            // reflection mode draws simplified version of the shapes, by artificially increasing view range
            distancesquared =
                // TBD, TODO: bind offset value with setting variable?
                ( EU07_REFLECTIONFIDELITYOFFSET * EU07_REFLECTIONFIDELITYOFFSET )
                + glm::length2( ( data.area.center - m_renderpass.pass_camera.position() ) );
/*
                // TBD: take into account distance multipliers?
                / Global.fDistanceFactor;
*/
            break;
        }
		default:
		{
			distancesquared = glm::length2((data.area.center - m_renderpass.pass_camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
			break;
		}
		}
		if ((distancesquared < data.rangesquared_min) || (distancesquared >= data.rangesquared_max))
		{
			return;
		}
	}

	// setup
	switch (m_renderpass.draw_mode)
	{
	case rendermode::color:
	case rendermode::reflections:
		Bind_Material(data.material, nullptr, &Shape.data().lighting );
		break;
	case rendermode::shadows:
        Bind_Material_Shadow(data.material);
		break;
	case rendermode::pickscenery:
	case rendermode::pickcontrols:
		m_pick_shader->bind();
		break;
	default:
		break;
	}
	// render

	draw(data.geometry);
	// debug data
	++m_renderpass.draw_stats.shapes;
	++m_renderpass.draw_stats.drawcalls;
}

void opengl33_renderer::Render(TAnimModel *Instance)
{

	if (false == Instance->m_visible) { return; }
    if( false == m_renderpass.pass_camera.visible( Instance->m_area ) ) { return; }

    double distancesquared;
	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
    {
        // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
		distancesquared = glm::length2((Instance->location() - m_renderpass.viewport_camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
		break;
	}
    case rendermode::reflections:
    {
        // reflection mode draws simplified version of the shapes, by artificially increasing view range
        // it also ignores zoom settings and distance multipliers
        // TBD: take into account distance multipliers?
        distancesquared = glm::length2((Instance->location() - m_renderpass.pass_camera.position())) /* / Global.fDistanceFactor */;
        // NOTE: arbitrary draw range limit
        if( distancesquared > Global.reflectiontune.range_instances * Global.reflectiontune.range_instances ) {
            return;
        }
        // TBD, TODO: bind offset value with setting variable?
        distancesquared += ( EU07_REFLECTIONFIDELITYOFFSET * EU07_REFLECTIONFIDELITYOFFSET );
        break;
    }
	default:
	{
		distancesquared = glm::length2((Instance->location() - m_renderpass.pass_camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
		break;
	}
	}
	if ((distancesquared < Instance->m_rangesquaredmin) || (distancesquared >= Instance->m_rangesquaredmax))
	{
		return;
	}
     // crude way to reject early items too far to affect the output (mostly relevant for shadow passes)
    auto const drawdistancethreshold{ m_renderpass.draw_range + 250 };
    if( distancesquared > drawdistancethreshold * drawdistancethreshold ) {
        return;
    }
   // second stage visibility cull, reject modelstoo far away to be noticeable
    auto const radiussquared { Instance->radius() * Instance->radius() };
    if( radiussquared * Global.ZoomFactor / distancesquared < 0.003 * 0.003 ) {
        return;
    }

	switch (m_renderpass.draw_mode)
	{
	case rendermode::pickscenery:
	{
		// add the node to the pick list
		m_picksceneryitems.emplace_back(Instance);
		break;
	}
	default:
	{
		break;
	}
	}

	Instance->RaAnimate(m_framestamp); // jednorazowe przeliczenie animacji
	Instance->RaPrepare();
	if (Instance->pModel)
	{
		// renderowanie rekurencyjne submodeli
		Render(Instance->pModel, Instance->Material(), distancesquared, Instance->location() - m_renderpass.pass_camera.position(), Instance->vAngle);
	    // debug data
	    ++m_renderpass.draw_stats.models;
	}
}

bool opengl33_renderer::Render(TDynamicObject *Dynamic)
{
	glDebug("Render TDynamicObject");

	if (!Global.render_cab && Global.pCamera.m_owner == Dynamic)
		return false;

    Dynamic->renderme = (
        ( Global.pCamera.m_owner == Dynamic && !FreeFlyModeFlag )
     || ( m_renderpass.pass_camera.visible( Dynamic ) ) );

    if (false == Dynamic->renderme) { return false; }

	// setup
	TSubModel::iInstance = reinterpret_cast<std::uintptr_t>(Dynamic); //żeby nie robić cudzych animacji
	glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.pass_camera.position();
	// lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
	float squaredistance;
	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
	{
		squaredistance = glm::length2(glm::vec3{glm::dvec3{Dynamic->vPosition - m_renderpass.viewport_camera.position()}} / Global.ZoomFactor);
        if( false == FreeFlyModeFlag ) {
            // filter out small details if we're in vehicle cab
            squaredistance = std::max( 100.f * 100.f, squaredistance );
        }
		break;
	}
    case rendermode::reflections:
    {
        // reflection mode draws simplified version of the shapes, by artificially increasing view range
        // it also ignores zoom settings and distance multipliers
        squaredistance =
            std::max(
                100.f * 100.f,
                // TBD: take into account distance multipliers?
                glm::length2( glm::vec3{ originoffset } ) /* / Global.fDistanceFactor */ );
        // NOTE: arbitrary draw range limit
        if( squaredistance > Global.reflectiontune.range_vehicles * Global.reflectiontune.range_vehicles ) {
            return false;
        }
        // TBD, TODO: bind offset value with setting variable?
        // NOTE: combined 'squared' distance doesn't equal actual squared (distance + offset) but, eh
        squaredistance += ( EU07_REFLECTIONFIDELITYOFFSET * EU07_REFLECTIONFIDELITYOFFSET );
        break;
    }
	default:
	{
		squaredistance = glm::length2(glm::vec3{originoffset} / Global.ZoomFactor);
        // TODO: filter out small details based on fidelity setting
		break;
	}
	}
    // crude way to reject early items too far to affect the output (mostly relevant for shadow and reflection passes)
    auto const drawdistancethreshold{ m_renderpass.draw_range + 250 };
    if( squaredistance > drawdistancethreshold * drawdistancethreshold ) {
        return false;
    }
    // second stage visibility cull, reject vehicles too far away to be noticeable
    auto const squareradius{ Dynamic->radius() * Dynamic->radius() };
    Dynamic->renderme = ( squareradius * Global.ZoomFactor / squaredistance > 0.003 * 0.003 );
    if( false == Dynamic->renderme ) {
        return false;
    }

    // debug data
    ++m_renderpass.draw_stats.dynamics;

	Dynamic->ABuLittleUpdate(squaredistance); // ustawianie zmiennych submodeli dla wspólnego modelu

	glm::mat4 future_stack = model_ubs.future;

	glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);
	model_ubs.future *= glm::translate(mv, glm::vec3(Dynamic->get_future_movement())) * glm::inverse(mv);

	::glPushMatrix();
	::glTranslated(originoffset.x, originoffset.y, originoffset.z);
	::glMultMatrixd(Dynamic->mMatrix.getArray());

	switch (m_renderpass.draw_mode)
	{

	case rendermode::color:
    case rendermode::reflections:
	{
		if (Dynamic->fShade > 0.0f)
		{
			// change light level based on light level of the occupied track
			setup_sunlight_intensity(Dynamic->fShade);
        }

		// render
        if( Dynamic->mdLowPolyInt ) {
            // low poly interior
            Render_lowpoly( Dynamic, squaredistance, false );
        }
        else {
            // HACK: if the model lacks low poly interior, we presume the load is placed outside
            if( Dynamic->mdLoad ) {
                // renderowanie nieprzezroczystego ładunku
                Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance, { 0.f, Dynamic->LoadOffset, 0.f }, {} );
            }
        }
        if( Dynamic->mdModel ) {
            // main model
            Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );
        }
        // optional attached models
        for( auto *attachment : Dynamic->mdAttachments ) {
            Render( attachment, Dynamic->Material(), squaredistance );
        }
        // optional coupling adapters
        Render_coupler_adapter( Dynamic, squaredistance, end::front );
        Render_coupler_adapter( Dynamic, squaredistance, end::rear );

        // post-render cleanup
		if (Dynamic->fShade > 0.0f)
		{
			// restore regular light level
			setup_sunlight_intensity();
        }
		break;
	}
	case rendermode::shadows:
	{
        if( Dynamic->mdLowPolyInt ) {
            // low poly interior
            Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
        }
        if( Dynamic->mdModel ) {
            // main model
            Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );
        }
        // optional attached models
        for( auto *attachment : Dynamic->mdAttachments ) {
            Render( attachment, Dynamic->Material(), squaredistance );
        }
        // optional coupling adapters
        Render_coupler_adapter( Dynamic, squaredistance, end::front );
        Render_coupler_adapter( Dynamic, squaredistance, end::rear );
        if( Dynamic->mdLoad ) {
            // renderowanie nieprzezroczystego ładunku
            Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance, { 0.f, Dynamic->LoadOffset, 0.f }, {} );
        }
		// post-render cleanup
		break;
	}
	case rendermode::pickcontrols:
	{
		if (Dynamic->mdLowPolyInt) {
			// low poly interior
			Render(Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance);
		}
		break;
	}
	case rendermode::pickscenery:
	default:
	{
		break;
	}
	}

	::glPopMatrix();

	model_ubs.future = future_stack;

	// TODO: check if this reset is needed. In theory each object should render all parts based on its own instance data anyway?
	if (Dynamic->btnOn)
		Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli

	return true;
}

// rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
bool opengl33_renderer::Render_cab(TDynamicObject const *Dynamic, float const Lightlevel, bool const Alpha)
{

	if (Dynamic == nullptr)
	{

		TSubModel::iInstance = 0;
		return false;
	}

	TSubModel::iInstance = reinterpret_cast<std::uintptr_t>(Dynamic);

	if ((true == FreeFlyModeFlag) || (false == Dynamic->bDisplayCab) || (Dynamic->mdKabina == Dynamic->mdModel))
	{
		// ABu: Rendering kabiny jako ostatniej, zeby bylo widac przez szyby, tylko w widoku ze srodka
		return false;
	}

	if (Dynamic->mdKabina)
	{ // bo mogła zniknąć przy przechodzeniu do innego pojazdu
		// setup shared by all render paths
		::glPushMatrix();

		auto const originoffset = Dynamic->GetPosition() - m_renderpass.pass_camera.position();
		::glTranslated(originoffset.x, originoffset.y, originoffset.z);
		::glMultMatrixd(Dynamic->mMatrix.readArray());

        switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		{
			// render path specific setup:
			if (Dynamic->fShade > 0.0f)
			{
				// change light level based on light level of the occupied track
				setup_sunlight_intensity(Dynamic->fShade);
            }

            auto const old_ambient { light_ubs.ambient };
            auto const luminance { Global.fLuminance * ( Dynamic->fShade > 0.0f ? Dynamic->fShade : 1.0f ) };
            if( Lightlevel > 0.f ) {
                // crude way to light the cabin, until we have something more complete in place
                light_ubs.ambient += ( Dynamic->InteriorLight * Lightlevel ) * static_cast<float>( clamp( 1.25 - luminance, 0.0, 1.0 ) );
                light_ubo->update( light_ubs );
            }

			// render
			if (true == Alpha)
			{
				// translucent parts
				Render_Alpha(Dynamic->mdKabina, Dynamic->Material(), 0.0);
			}
			else
			{
				// opaque parts
				Render(Dynamic->mdKabina, Dynamic->Material(), 0.0);
			}
			// post-render restore
            light_ubs.ambient = old_ambient;
            setup_sunlight_intensity();

			break;
		}
        case rendermode::shadows:
        {
            if( true == Alpha ) {
                // translucent parts
                Render_Alpha( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
            }
            else {
                // opaque parts
                Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
            }
            break;
        }
		case rendermode::pickcontrols:
		{
			Render(Dynamic->mdKabina, Dynamic->Material(), 0.0);
			break;
		}
		default:
		{
			break;
		}
		}
		// post-render restore
		::glPopMatrix();
	}

	return true;
}

bool opengl33_renderer::Render(TModel3d *Model, material_data const *Material, float const Squaredistance)
{

	auto alpha = (Material != nullptr ? Material->textures_alpha : 0x30300030);
	alpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
	if (0 == (alpha & Model->iFlags & 0x1F1F001F))
	{
		// czy w ogóle jest co robić w tym cyklu?
		return false;
	}

	Model->Root->fSquareDist = Squaredistance; // zmienna globalna!

	// setup
	Model->Root->ReplacableSet((Material != nullptr ? Material->replacable_skins : nullptr), alpha);

	Model->Root->pRoot = Model;

	// render
	Render(Model->Root);

	// post-render cleanup

	return true;
}

bool opengl33_renderer::Render(TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &A)
{
	Math3D::vector3 Angle(A);
	::glPushMatrix();
	::glTranslated(Position.x, Position.y, Position.z);
	if (Angle.y != 0.0)
		::glRotated(Angle.y, 0.0, 1.0, 0.0);
	if (Angle.x != 0.0)
		::glRotated(Angle.x, 1.0, 0.0, 0.0);
	if (Angle.z != 0.0)
		::glRotated(Angle.z, 0.0, 0.0, 1.0);

	auto const result = Render(Model, Material, Squaredistance);

	::glPopMatrix();

	return result;
}

void opengl33_renderer::Render(TSubModel *Submodel)
{
	glDebug("Render TSubModel");

	if ((Submodel->iVisible) && (TSubModel::fSquareDist >= Submodel->fSquareMinDist) && (TSubModel::fSquareDist < Submodel->fSquareMaxDist))
	{
		glm::mat4 future_stack = model_ubs.future;

		if (Submodel->iFlags & 0xC000)
		{
			::glPushMatrix();
			if (Submodel->fMatrix)
				::glMultMatrixf(Submodel->fMatrix->readArray());
			if (Submodel->b_aAnim != TAnimType::at_None)
			{
				Submodel->RaAnimation(Submodel->b_aAnim);

				glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);
				model_ubs.future *= (mv * Submodel->future_transform) * glm::inverse(mv);
			}
		}

		if (Submodel->eType < TP_ROTATOR)
		{
			// renderowanie obiektów OpenGL
			if (Submodel->iAlpha & Submodel->iFlags & 0x1F)
			{
				// rysuj gdy element nieprzezroczysty

		        // debug data
		        ++m_renderpass.draw_stats.submodels;
		        ++m_renderpass.draw_stats.drawcalls;

				switch (m_renderpass.draw_mode)
				{
				case rendermode::color:
				case rendermode::reflections:
				{
					// material configuration:
					// transparency hack
					if (Submodel->fVisible < 1.0f)
						setup_drawing(true);

					// textures...
					if (Submodel->m_material < 0)
					{ // zmienialne skóry
						Bind_Material(Submodel->ReplacableSkinId[-Submodel->m_material], Submodel);
					}
					else
					{
						// również 0
						Bind_Material(Submodel->m_material, Submodel);
					}

					// ...luminance
					auto const isemissive { ( Submodel->f4Emision.a > 0.f ) && ( Global.fLuminance < Submodel->fLight ) };
					if (isemissive)
						model_ubs.emission = Submodel->f4Emision.a;

					// main draw call
					draw(Submodel->m_geometry.handle);

					// post-draw reset
					model_ubs.emission = 0.0f;
					if (Submodel->fVisible < 1.0f)
						setup_drawing(false);

					break;
				}
				case rendermode::shadows:
				{
                    if (Submodel->m_material < 0)
					{ // zmienialne skóry
						Bind_Material_Shadow(Submodel->ReplacableSkinId[-Submodel->m_material]);
					}
					else
					{
						// również 0
						Bind_Material_Shadow(Submodel->m_material);
					}
                    draw(Submodel->m_geometry.handle);
                    break;
				}
				case rendermode::pickscenery:
				{
					m_pick_shader->bind();
					draw(Submodel->m_geometry.handle);
					break;
				}
				case rendermode::pickcontrols:
				{
					m_pick_shader->bind();
					// control picking applies individual colour for each submodel
					m_pickcontrolsitems.emplace_back(Submodel);
					model_ubs.param[0] = glm::vec4(pick_color(m_pickcontrolsitems.size()), 1.0f);
					draw(Submodel->m_geometry.handle);
					break;
				}
				default:
				{
					break;
				}
				}
			}
		}
		else if (Submodel->eType == TP_STARS)
		{

			switch (m_renderpass.draw_mode)
			{
			// colour points are only rendered in colour mode(s)
			case rendermode::color:
			case rendermode::reflections:
			{
				if (Global.fLuminance < Submodel->fLight)
				{
					Bind_Material(Submodel->m_material, Submodel);

					// main draw call
                    model_ubs.param[1].x = m_pointsize;

					draw(Submodel->m_geometry.handle);
				}
				break;
			}
			default:
			{
				break;
			}
			}
		}
		if (Submodel->Child != nullptr)
			if (Submodel->iAlpha & Submodel->iFlags & 0x001F0000)
				Render(Submodel->Child);

		if (Submodel->iFlags & 0xC000)
		{
			model_ubs.future = future_stack;
			::glPopMatrix();
		}
	}
	/*
	    if( Submodel->b_Anim < at_SecondsJump )
	        Submodel->b_Anim = at_None; // wyłączenie animacji dla kolejnego użycia subm
	*/
	if (Submodel->Next)
		if (Submodel->iAlpha & Submodel->iFlags & 0x1F000000)
			Render(Submodel->Next); // dalsze rekurencyjnie
}

void opengl33_renderer::Render(TTrack *Track)
{
	if ((Track->m_material1 == 0) && (Track->m_material2 == 0) && (Track->eType != tt_Switch || Track->SwitchExtension->m_material3 == 0))
	{
		return;
	}
	if (false == Track->m_visible)
	{
		return;
	}

	++m_renderpass.draw_stats.paths;
	++m_renderpass.draw_stats.drawcalls;

	switch (m_renderpass.draw_mode)
	{
	// single path pieces are rendererd in pick scenery mode only
	case rendermode::pickscenery:
	{
        m_pick_shader->bind();
        m_picksceneryitems.emplace_back( Track );

		draw(std::begin(Track->Geometry1), std::end(Track->Geometry1));
		draw(std::begin(Track->Geometry2), std::end(Track->Geometry2));
		if (Track->eType == tt_Switch)
			draw(Track->SwitchExtension->Geometry3);
		break;
	}
	default:
	{
		break;
	}
	}
}

// experimental, does track rendering in two passes, to take advantage of reduced texture switching
void opengl33_renderer::Render(scene::basic_cell::path_sequence::const_iterator First, scene::basic_cell::path_sequence::const_iterator Last)
{

	// setup
	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
	{
		// NOTE: roads-based platforms tend to miss parts of shadows if rendered with either back or front culling
		glDisable(GL_CULL_FACE);
		break;
	}
	default:
	{
		break;
	}
	}

	// TODO: render auto generated trackbeds together with regular trackbeds in pass 1, and all rails in pass 2
	// first pass, material 1
	for (auto first{First}; first != Last; ++first)
	{

		auto const track{*first};

		if (track->m_material1 == 0)
		{
			continue;
		}
		if (false == track->m_visible)
		{
			continue;
		}

		++m_renderpass.draw_stats.paths;
		++m_renderpass.draw_stats.drawcalls;

		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		case rendermode::reflections:
		{
			if (track->eEnvironment != e_flat)
			{
				setup_environment_light(track->eEnvironment);
			}
			Bind_Material(track->m_material1);
			draw(std::begin(track->Geometry1), std::end(track->Geometry1));
			if (track->eEnvironment != e_flat)
			{
				// restore default lighting
				setup_environment_light();
			}
			break;
		}
		case rendermode::shadows:
        {
			if ((std::abs(track->fTexHeight1) < 0.35f) || (track->iCategoryFlag != 2))
			{
				// shadows are only calculated for high enough roads, typically meaning track platforms
				continue;
			}
			Bind_Material_Shadow(track->m_material1);
			draw(std::begin(track->Geometry1), std::end(track->Geometry1));
			break;
		}
		case rendermode::pickscenery: // pick scenery mode uses piece-by-piece approach
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}
	}
	// second pass, material 2
	for (auto first{First}; first != Last; ++first)
	{
		auto const track{*first};

		if (track->m_material2 == 0)
		{
			continue;
		}
		if (false == track->m_visible)
		{
			continue;
		}

		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		case rendermode::reflections:
		{
			if (track->eEnvironment != e_flat)
			{
				setup_environment_light(track->eEnvironment);
			}
			Bind_Material(track->m_material2);
			draw(std::begin(track->Geometry2), std::end(track->Geometry2));
			if (track->eEnvironment != e_flat)
			{
				// restore default lighting
				setup_environment_light();
			}
			break;
		}
		case rendermode::shadows:
        {
			if ((std::abs(track->fTexHeight1) < 0.35f) || ((track->iCategoryFlag == 1) && (track->eType != tt_Normal)))
			{
				// shadows are only calculated for high enough trackbeds
				continue;
			}
			Bind_Material_Shadow(track->m_material2);
			draw(std::begin(track->Geometry2), std::end(track->Geometry2));
			break;
		}
		case rendermode::pickscenery: // pick scenery mode uses piece-by-piece approach
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}
	}

	// third pass, material 3
	for (auto first{First}; first != Last; ++first)
	{

		auto const track{*first};

		if (track->eType != tt_Switch)
		{
			continue;
		}
		if (track->SwitchExtension->m_material3 == 0)
		{
			continue;
		}
		if (false == track->m_visible)
		{
			continue;
		}

		switch (m_renderpass.draw_mode)
		{
		case rendermode::color:
		case rendermode::reflections:
		{
			if (track->eEnvironment != e_flat)
			{
				setup_environment_light(track->eEnvironment);
			}
			Bind_Material(track->SwitchExtension->m_material3);
			draw(track->SwitchExtension->Geometry3);
			if (track->eEnvironment != e_flat)
			{
				// restore default lighting
				setup_environment_light();
			}
			break;
		}
		case rendermode::shadows:
        {
			if ((std::abs(track->fTexHeight1) < 0.35f) || ((track->iCategoryFlag == 1) && (track->eType != tt_Normal)))
			{
				// shadows are only calculated for high enough trackbeds
				continue;
			}
			Bind_Material_Shadow(track->SwitchExtension->m_material3);
			draw(track->SwitchExtension->Geometry3);
			break;
		}
		case rendermode::pickscenery: // pick scenery mode uses piece-by-piece approach
		case rendermode::pickcontrols:
		default:
		{
			break;
		}
		}
	}

	// post-render reset
	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
    {
		// restore standard face cull mode
		::glEnable(GL_CULL_FACE);
		break;
	}
	default:
	{
		break;
	}
	}
}

void opengl33_renderer::Render(TMemCell *Memcell)
{

	::glPushMatrix();
	auto const position = Memcell->location() - m_renderpass.pass_camera.position();
	::glTranslated(position.x, position.y + 0.5, position.z);

	switch (m_renderpass.draw_mode)
	{
	case rendermode::color:
	{
		break;
	}
	case rendermode::shadows:
    case rendermode::pickscenery:
	{
		break;
	}
	case rendermode::reflections:
	case rendermode::pickcontrols:
	{
		break;
	}
	default:
	{
		break;
	}
	}

	::glPopMatrix();
}

void opengl33_renderer::Render_particles()
{
	model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
	model_ubo->update(model_ubs);

	Bind_Texture(0, m_smoketexture);
	m_particlerenderer.update(m_renderpass.pass_camera);
    m_renderpass.draw_stats.particles = m_particlerenderer.render();
}

void opengl33_renderer::Render_precipitation()
{
	if (Global.Overcast <= 1.f)
	{
		return;
	}

	::glPushMatrix();
	// tilt the precipitation cone against the velocity vector for crude motion blur
	// include current wind vector while at it
	auto const velocity {
		simulation::Environment.m_precipitation.m_cameramove * -1.0
		+ glm::dvec3{ simulation::Environment.wind() } * 0.5 };
	if (glm::length2(velocity) > 0.0)
	{
		auto const forward{glm::normalize(velocity)};
		auto left{glm::cross(forward, {0.0, 1.0, 0.0})};
		auto const rotationangle{std::min(45.0, (FreeFlyModeFlag ? 5 * glm::length(velocity) : simulation::Train->Dynamic()->GetVelocity() * 0.2))};
		::glRotated(rotationangle, left.x, 0.0, left.z);
	}
	if (false == FreeFlyModeFlag)
	{
		// counter potential vehicle roll
		auto const roll{0.5 * glm::degrees(simulation::Train->Dynamic()->Roll())};
		if (roll != 0.0)
		{
			auto const forward{simulation::Train->Dynamic()->VectorFront()};
			auto const vehicledirection = simulation::Train->Dynamic()->DirectionGet();
			::glRotated(roll, forward.x, 0.0, forward.z);
		}
	}
	if (!Global.iPause)
	{
		if (Global.Weather == "rain:")
			// oddly enough random streaks produce more natural looking rain than ones the eye can follow
			m_precipitationrotation = LocalRandom() * 360;
		else
			m_precipitationrotation = 0.0;
	}

	::glRotated(m_precipitationrotation, 0.0, 1.0, 0.0);

	model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
	model_ubs.param[0] = interpolate(0.5f * (Global.DayLight.diffuse + Global.DayLight.ambient), colors::white, 0.5f * clamp<float>(Global.fLuminance, 0.f, 1.f));
	model_ubs.param[1].x = simulation::Environment.m_precipitation.get_textureoffset();
	model_ubo->update(model_ubs);

    m_precipitationrenderer.update();
    m_precipitationrenderer.render();

	::glPopMatrix();
}

void opengl33_renderer::Render_Alpha(scene::basic_region *Region)
{

	// sort the nodes based on their distance to viewer
	std::sort(std::begin(m_cellqueue), std::end(m_cellqueue), [](distancecell_pair const &Left, distancecell_pair const &Right) { return (Left.first) < (Right.first); });

	Render_Alpha(std::rbegin(m_cellqueue), std::rend(m_cellqueue));
}

void opengl33_renderer::Render_Alpha(cell_sequence::reverse_iterator First, cell_sequence::reverse_iterator Last)
{

	// NOTE: this method is launched only during color pass therefore we don't bother with mode test here
	// first pass draws elements which we know are located in section banks, to reduce vbo switching
	{
		auto first{First};
		while (first != Last)
		{

			auto const *cell = first->second;

			if (false == cell->m_shapestranslucent.empty())
			{
				// since all shapes of the cell share center point we can optimize out a few calls here
				::glPushMatrix();
				auto const originoffset{cell->m_area.center - m_renderpass.pass_camera.position()};
				::glTranslated(originoffset.x, originoffset.y, originoffset.z);
				// render
				// NOTE: we can reuse the method used to draw opaque geometry
				for (auto const &shape : cell->m_shapestranslucent)
				{
					Render(shape, false);
				}
				// post-render cleanup
				::glPopMatrix();
			}

			++first;
		}
	}
	// second pass draws elements with their own vbos
	{
		auto first{First};
		while (first != Last)
		{

			auto const *cell = first->second;

			// translucent parts of instanced models
			for (auto *instance : cell->m_instancetranslucent)
			{
				Render_Alpha(instance);
			}
			// translucent parts of vehicles
			for (auto *path : cell->m_paths)
			{
				for (auto *dynamic : path->Dynamics)
				{
					Render_Alpha(dynamic);
				}
			}

			++first;
		}
	}
	// third pass draws the wires;
	// wires use section vbos, but for the time being we want to draw them at the very end
	{
		auto first{First};
		while (first != Last)
		{

			auto const *cell = first->second;

			if ((false == cell->m_traction.empty() || (false == cell->m_lines.empty())))
			{
				// since all shapes of the cell share center point we can optimize out a few calls here
				::glPushMatrix();
				auto const originoffset{cell->m_area.center - m_renderpass.pass_camera.position()};
				::glTranslated(originoffset.x, originoffset.y, originoffset.z);
				Bind_Material(null_handle);
                ::glDepthMask( GL_TRUE ); // wires are 'solid' geometry, record them in the depth buffer
				// render
				for (auto *traction : cell->m_traction)
				{
					Render_Alpha(traction);
				}
				for (auto &lines : cell->m_lines)
				{
					Render_Alpha(lines);
				}
				// post-render cleanup
                ::glDepthMask( GL_FALSE );
				::glPopMatrix();
			}

			++first;
		}
	}
}

void opengl33_renderer::Render_Alpha(TAnimModel *Instance)
{

	if (false == Instance->m_visible) { return; }
    if( false == m_renderpass.pass_camera.visible( Instance->m_area ) ) { return; }

    double distancesquared;
	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
    {
        // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
		distancesquared = glm::length2((Instance->location() - m_renderpass.viewport_camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
		break;
	}
	default:
	{
		distancesquared = glm::length2((Instance->location() - m_renderpass.pass_camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
		break;
	}
	}
	if ((distancesquared < Instance->m_rangesquaredmin) || (distancesquared >= Instance->m_rangesquaredmax))
	{
		return;
	}
     // crude way to reject early items too far to affect the output (mostly relevant for shadow passes)
    auto const drawdistancethreshold{ m_renderpass.draw_range + 250 };
    if( distancesquared > drawdistancethreshold * drawdistancethreshold ) {
        return;
    }
   // second stage visibility cull, reject modelstoo far away to be noticeable
    auto const radiussquared { Instance->radius() * Instance->radius() };
    if( radiussquared * Global.ZoomFactor / distancesquared < 0.003 * 0.003 ) {
        return;
    }

    Instance->RaAnimate( m_framestamp ); // jednorazowe przeliczenie animacji
    Instance->RaPrepare();
	if (Instance->pModel)
	{
		// renderowanie rekurencyjne submodeli
		Render_Alpha(Instance->pModel, Instance->Material(), distancesquared, Instance->location() - m_renderpass.pass_camera.position(), Instance->vAngle);
	}
}

void opengl33_renderer::Render_Alpha(TTraction *Traction)
{
	glDebug("Render_Alpha TTraction");

	auto const distancesquared { glm::length2( ( Traction->location() - m_renderpass.pass_camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor };
	if ((distancesquared < Traction->m_rangesquaredmin) || (distancesquared >= Traction->m_rangesquaredmax))
	{
		return;
	}

	if (false == Traction->m_visible)
	{
		return;
	}
	// rysuj jesli sa druty i nie zerwana
	if ((Traction->Wires == 0) || (true == TestFlag(Traction->DamageFlag, 128)))
	{
		return;
	}
	// setup
	auto const distance{static_cast<float>(std::sqrt(distancesquared))};
	auto const linealpha = 20.f * Traction->WireThickness / std::max(0.5f * Traction->radius() + 1.f, distance - (0.5f * Traction->radius()));
	if (m_widelines_supported)
		glLineWidth(clamp(0.5f * linealpha + Traction->WireThickness * Traction->radius() / 1000.f, 1.f, 1.75f));

	// render

	// McZapkie-261102: kolor zalezy od materialu i zasniedzenia
	model_ubs.param[0] = glm::vec4(Traction->wire_color(), glm::min(1.0f, linealpha));

	if (m_renderpass.draw_mode == rendermode::shadows)
		Bind_Material_Shadow(null_handle);
	else
		m_line_shader->bind();

	draw(Traction->m_geometry);

	// debug data
	++m_renderpass.draw_stats.traction;
	++m_renderpass.draw_stats.drawcalls;

	if (m_widelines_supported)
		glLineWidth(1.0f);
}

void opengl33_renderer::Render_Alpha(scene::lines_node const &Lines)
{
	glDebug("Render_Alpha scene::lines_node");

	auto const &data{Lines.data()};

	auto const distancesquared { glm::length2( ( data.area.center - m_renderpass.pass_camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor };
	if ((distancesquared < data.rangesquared_min) || (distancesquared >= data.rangesquared_max))
	{
		return;
	}
	// setup
	auto const distance{static_cast<float>(std::sqrt(distancesquared))};
	auto const linealpha =
	    (data.line_width > 0.f ? 10.f * data.line_width / std::max(0.5f * data.area.radius + 1.f, distance - (0.5f * data.area.radius)) : 1.f); // negative width means the lines are always opague
	if (m_widelines_supported)
		glLineWidth(clamp(0.5f * linealpha + data.line_width * data.area.radius / 1000.f, 1.f, 8.f));

	model_ubs.param[0] = glm::vec4(glm::vec3(data.lighting.diffuse * m_sunlight.ambient), glm::min(1.0f, linealpha));

	if (m_renderpass.draw_mode == rendermode::shadows)
		Bind_Material_Shadow(null_handle);
	else
		m_line_shader->bind();
	draw(data.geometry);

	++m_renderpass.draw_stats.lines;
	++m_renderpass.draw_stats.drawcalls;

	if (m_widelines_supported)
		glLineWidth(1.0f);
}

bool opengl33_renderer::Render_Alpha(TDynamicObject *Dynamic)
{
	if (!Global.render_cab && Global.pCamera.m_owner == Dynamic)
		return false;

	if (false == Dynamic->renderme)
	{
		return false;
	}

	// setup
	TSubModel::iInstance = (size_t)Dynamic; //żeby nie robić cudzych animacji
	glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.pass_camera.position();
	// lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
	float squaredistance;
	switch (m_renderpass.draw_mode)
	{
	case rendermode::shadows:
	default:
	{
		squaredistance = glm::length2(glm::vec3{originoffset} / Global.ZoomFactor);
		break;
	}
	}
	Dynamic->ABuLittleUpdate(squaredistance); // ustawianie zmiennych submodeli dla wspólnego modelu

	glm::mat4 future_stack = model_ubs.future;

	glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);
	model_ubs.future *= glm::translate(mv, glm::vec3(Dynamic->get_future_movement())) * glm::inverse(mv);

	::glPushMatrix();

	::glTranslated(originoffset.x, originoffset.y, originoffset.z);
	::glMultMatrixd(Dynamic->mMatrix.getArray());

	if (Dynamic->fShade > 0.0f)
	{
		// change light level based on light level of the occupied track
		setup_sunlight_intensity(Dynamic->fShade);
	}

    // render
    if( Dynamic->mdLowPolyInt ) {
        // low poly interior
        Render_Alpha( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
    }
    if( Dynamic->mdModel ) {
        // main model
        Render_Alpha( Dynamic->mdModel, Dynamic->Material(), squaredistance );
    }
    // optional attached models
    for( auto *attachment : Dynamic->mdAttachments ) {
        Render_Alpha( attachment, Dynamic->Material(), squaredistance );
    }
    // optional coupling adapters
    Render_coupler_adapter( Dynamic, squaredistance, end::front, true );
    Render_coupler_adapter( Dynamic, squaredistance, end::rear, true );
    if( Dynamic->mdLoad ) {
        // renderowanie nieprzezroczystego ładunku
        Render_Alpha( Dynamic->mdLoad, Dynamic->Material(), squaredistance, { 0.f, Dynamic->LoadOffset, 0.f }, {} );
    }

	// post-render cleanup
	if (Dynamic->fShade > 0.0f)
	{
		// restore regular light level
		setup_sunlight_intensity();
	}

	::glPopMatrix();

	model_ubs.future = future_stack;

	if (Dynamic->btnOn)
		Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli

	return true;
}

bool opengl33_renderer::Render_Alpha(TModel3d *Model, material_data const *Material, float const Squaredistance)
{

	auto alpha = (Material != nullptr ? Material->textures_alpha : 0x30300030);

	if (0 == (alpha & Model->iFlags & 0x2F2F002F))
	{
		// nothing to render
		return false;
	}

	Model->Root->fSquareDist = Squaredistance; // zmienna globalna!

	// setup
	Model->Root->ReplacableSet((Material != nullptr ? Material->replacable_skins : nullptr), alpha);

	Model->Root->pRoot = Model;

	// render
	Render_Alpha(Model->Root);

	// post-render cleanup

	return true;
}

bool opengl33_renderer::Render_Alpha(TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &A)
{
	Math3D::vector3 Angle(A);
	::glPushMatrix();
	::glTranslated(Position.x, Position.y, Position.z);
	if (Angle.y != 0.0)
		::glRotated(Angle.y, 0.0, 1.0, 0.0);
	if (Angle.x != 0.0)
		::glRotated(Angle.x, 1.0, 0.0, 0.0);
	if (Angle.z != 0.0)
		::glRotated(Angle.z, 0.0, 0.0, 1.0);

	auto const result = Render_Alpha(Model, Material, Squaredistance); // position is effectively camera offset

	::glPopMatrix();

	return result;
}

void opengl33_renderer::Render_Alpha(TSubModel *Submodel)
{
	// renderowanie przezroczystych przez DL
	if ((Submodel->iVisible) && (TSubModel::fSquareDist >= Submodel->fSquareMinDist) && (TSubModel::fSquareDist < Submodel->fSquareMaxDist))
	{
		glm::mat4 future_stack = model_ubs.future;

		if (Submodel->iFlags & 0xC000)
		{
			::glPushMatrix();
			if (Submodel->fMatrix)
				::glMultMatrixf(Submodel->fMatrix->readArray());
			if (Submodel->b_aAnim != TAnimType::at_None)
			{
				Submodel->RaAnimation(Submodel->b_aAnim);

				glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);
				model_ubs.future *= (mv * Submodel->future_transform) * glm::inverse(mv);
			}
		}

		if (Submodel->eType < TP_ROTATOR)
		{
			// renderowanie obiektów OpenGL
			if (Submodel->iAlpha & Submodel->iFlags & 0x2F)
			{
                // rysuj gdy element przezroczysty

                // debug data
		        ++m_renderpass.draw_stats.submodels;
		        ++m_renderpass.draw_stats.drawcalls;

				switch (m_renderpass.draw_mode)
				{
				case rendermode::color:
				{
					// material configuration:
					// textures...
					if (Submodel->m_material < 0)
					{ // zmienialne skóry
						Bind_Material(Submodel->ReplacableSkinId[-Submodel->m_material], Submodel);
					}
					else
					{
						Bind_Material(Submodel->m_material, Submodel);
					}
					// ...luminance

					auto const isemissive { ( Submodel->f4Emision.a > 0.f ) && ( Global.fLuminance < Submodel->fLight ) };
					if (isemissive)
						model_ubs.emission = Submodel->f4Emision.a;

					// main draw call
					draw(Submodel->m_geometry.handle);

					model_ubs.emission = 0.0f;
					break;
				}
				case rendermode::shadows:
				{
					if (Submodel->m_material < 0)
					{ // zmienialne skóry
						Bind_Material_Shadow(Submodel->ReplacableSkinId[-Submodel->m_material]);
					}
					else
					{
						Bind_Material_Shadow(Submodel->m_material);
					}
					draw(Submodel->m_geometry.handle);
					break;
				}
				default:
				{
					break;
				}
				}
			}
		}
		else if (Submodel->eType == TP_FREESPOTLIGHT)
		{
			// NOTE: we're forced here to redo view angle calculations etc, because this data isn't instanced but stored along with the single mesh
			// TODO: separate instance data from reusable geometry
			auto const &modelview = OpenGLMatrices.data(GL_MODELVIEW);
			auto const lightcenter = modelview * interpolate(glm::vec4(0.f, 0.f, -0.05f, 1.f), glm::vec4(0.f, 0.f, -0.25f, 1.f),
			                                                 static_cast<float>(TSubModel::fSquareDist / Submodel->fSquareMaxDist)); // pozycja punktu świecącego względem kamery
			Submodel->fCosViewAngle = glm::dot(glm::normalize(modelview * glm::vec4(0.f, 0.f, -1.f, 1.f) - lightcenter), glm::normalize(-lightcenter));

			if (Global.fLuminance < Submodel->fLight || Global.Overcast > 1.0f)
			{
				if (Submodel->fCosViewAngle > Submodel->fCosFalloffAngle)
				{
					// only bother if the viewer is inside the visibility cone
					// luminosity at night is at level of ~0.1, so the overall resulting transparency in clear conditions is ~0.5 at full 'brightness'
					auto glarelevel{clamp(std::max<float>(0.6f - Global.fLuminance, // reduce the glare in bright daylight
						                                  Global.Overcast - 1.f), // ensure some glare in rainy/foggy conditions
						                  0.f, 1.f)};
					// view angle attenuation
					float const anglefactor{clamp((Submodel->fCosViewAngle - Submodel->fCosFalloffAngle) / (Submodel->fCosHotspotAngle - Submodel->fCosFalloffAngle), 0.f, 1.f)};

					glarelevel *= anglefactor;

					if (glarelevel > 0.0f)
					{
						glDisable(GL_DEPTH_TEST);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);

						::glPushMatrix();
						::glLoadIdentity(); // macierz jedynkowa
						::glTranslatef(lightcenter.x, lightcenter.y, lightcenter.z); // początek układu zostaje bez zmian
						::glRotated(std::atan2(lightcenter.x, lightcenter.z) * 180.0 / M_PI, 0.0, 1.0, 0.0); // jedynie obracamy w pionie o kąt

						auto const lightcolor = glm::vec3(Submodel->DiffuseOverride.r < 0.f ? // -1 indicates no override
						                           Submodel->f4Diffuse :
						                           Submodel->DiffuseOverride);

						m_billboard_shader->bind();
						Bind_Texture(0, m_glaretexture);
						model_ubs.param[0] = glm::vec4(glm::vec3(lightcolor), Submodel->fVisible * glarelevel);

						// main draw call
						if (Submodel->occlusion_query) {
							if (!Global.gfx_usegles) {
								glBeginConditionalRender(*Submodel->occlusion_query, GL_QUERY_WAIT);
								draw(m_billboardgeometry);
								glEndConditionalRender();
							}
							else {
								auto result = Submodel->occlusion_query->result();
								if (result && *result)
									draw(m_billboardgeometry);
							}
						}
						else
							draw(m_billboardgeometry);

						glEnable(GL_DEPTH_TEST);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

						::glPopMatrix();
					}
				}
			}

			if (Submodel->fCosViewAngle > Submodel->fCosFalloffAngle)
			{
				// kąt większy niż maksymalny stożek swiatła
				float lightlevel = 1.f; // TODO, TBD: parameter to control light strength
				// view angle attenuation
				float const anglefactor = clamp((Submodel->fCosViewAngle - Submodel->fCosFalloffAngle) / (Submodel->fCosHotspotAngle - Submodel->fCosFalloffAngle), 0.f, 1.f);
				lightlevel *= anglefactor;

				// distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
				// we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
				float const distancefactor{std::max(0.5f, (Submodel->fSquareMaxDist - TSubModel::fSquareDist) / Submodel->fSquareMaxDist)};
				auto const pointsize{std::max(3.f, 5.f * distancefactor * anglefactor)};
				auto const resolutionratio { Global.iWindowHeight / 1080.f };
				// additionally reduce light strength for farther sources in rain or snow
				if (Global.Overcast > 0.75f)
				{
					float const precipitationfactor{interpolate(interpolate(1.f, 0.25f, clamp(Global.Overcast * 0.75f - 0.5f, 0.f, 1.f)), 1.f, distancefactor)};
					lightlevel *= precipitationfactor;
				}

				if (lightlevel > 0.f)
				{
					::glPushMatrix();
					::glLoadIdentity();
					::glTranslatef(lightcenter.x, lightcenter.y, lightcenter.z); // początek układu zostaje bez zmian

					// material configuration:
					// limit impact of dense fog on the lights
					auto const lightrange { std::max<float>( 500, m_fogrange * 2 ) }; // arbitrary, visibility at least 500m
					model_ubs.fog_density = 1.0 / lightrange;

					// main draw call
					model_ubs.emission = 1.0f;

					auto lightcolor = glm::vec3(Submodel->DiffuseOverride.r < 0.f ? // -1 indicates no override
                                            Submodel->f4Diffuse :
                                            Submodel->DiffuseOverride);
//                    lightcolor = glm::pow( lightcolor, gammacorrection );

					m_freespot_shader->bind();

					if (Global.Overcast > 1.0f)
					{
						// fake fog halo
						float const fogfactor{interpolate(1.5f, 1.f, clamp(Global.fFogEnd / 2000, 0.f, 1.f)) * std::max(1.f, Global.Overcast)};
						model_ubs.param[1].x = pointsize * resolutionratio * fogfactor * 4.0f;
						model_ubs.param[0] = glm::vec4(glm::vec3(lightcolor), Submodel->fVisible * std::min(1.f, lightlevel) * 0.5f);

						draw(Submodel->m_geometry.handle);
					}
					model_ubs.param[1].x = pointsize * resolutionratio * 4.0f;
					model_ubs.param[0] = glm::vec4(glm::vec3(lightcolor), Submodel->fVisible * std::min(1.f, lightlevel));

					if (!Submodel->occlusion_query)
						Submodel->occlusion_query.emplace(gl::query::ANY_SAMPLES_PASSED);
					Submodel->occlusion_query->begin();

					draw(Submodel->m_geometry.handle);

					Submodel->occlusion_query->end();

					// post-draw reset
					model_ubs.emission = 0.0f;
					model_ubs.fog_density = 1.0f / m_fogrange;

					::glPopMatrix();
				}
			}
		}

		if (Submodel->Child != nullptr)
		{
			if (Submodel->eType == TP_TEXT)
			{ // tekst renderujemy w specjalny sposób, zamiast submodeli z łańcucha Child
				int i, j = (int)Submodel->pasText->size();
				TSubModel *p;
				if (!Submodel->smLetter)
				{ // jeśli nie ma tablicy, to ją stworzyć; miejsce nieodpowiednie, ale tymczasowo może być
					Submodel->smLetter = new TSubModel *[256]; // tablica wskaźników submodeli dla wyświetlania tekstu
					memset(Submodel->smLetter, 0, 256 * sizeof(TSubModel *)); // wypełnianie zerami
					p = Submodel->Child;
					while (p)
					{
						Submodel->smLetter[p->pName[0]] = p;
						p = p->Next; // kolejny znak
					}
				}
				for (i = 1; i <= j; ++i)
				{
					p = Submodel->smLetter[(*(Submodel->pasText))[i]]; // znak do wyświetlenia
					if (p)
					{ // na razie tylko jako przezroczyste
						Render_Alpha(p);
						if (p->fMatrix)
							::glMultMatrixf(p->fMatrix->readArray()); // przesuwanie widoku
					}
				}
			}
			else if (Submodel->iAlpha & Submodel->iFlags & 0x002F0000)
				Render_Alpha(Submodel->Child);
		}

		if (Submodel->iFlags & 0xC000)
		{
			model_ubs.future = future_stack;
			::glPopMatrix();
		}
	}
	/*
	    if( Submodel->b_aAnim < at_SecondsJump )
	        Submodel->b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu
	*/
	if (Submodel->Next != nullptr)
		if (Submodel->iAlpha & Submodel->iFlags & 0x2F000000)
			Render_Alpha(Submodel->Next);
};

// utility methods
void opengl33_renderer::Update_Pick_Control()
{
	// context-switch workaround
    gl::vao::unbind();
	gl::buffer::unbind();

	if (!m_picking_pbo->is_busy())
	{
		unsigned char pickreadout[4];
		if (m_picking_pbo->read_data(1, 1, pickreadout))
		{
			auto const controlindex = pick_index(glm::ivec3{pickreadout[0], pickreadout[1], pickreadout[2]});
			TSubModel const *control{nullptr};
			if ((controlindex > 0) && (controlindex <= m_pickcontrolsitems.size()))
			{
				control = m_pickcontrolsitems[controlindex - 1];
			}

			m_pickcontrolitem = control;

			for (auto f : m_control_pick_requests)
				f(m_pickcontrolitem);
			m_control_pick_requests.clear();
		}

		if (!m_control_pick_requests.empty())
		{
			// determine point to examine
			glm::dvec2 mousepos = Application.get_cursor_pos();
			mousepos.y = Global.iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

			glm::ivec2 pickbufferpos;
			pickbufferpos = glm::ivec2{mousepos.x * EU07_PICKBUFFERSIZE / std::max(1, Global.iWindowWidth), mousepos.y * EU07_PICKBUFFERSIZE / std::max(1, Global.iWindowHeight)};
			pickbufferpos = glm::clamp(pickbufferpos, glm::ivec2(0, 0), glm::ivec2(EU07_PICKBUFFERSIZE - 1, EU07_PICKBUFFERSIZE - 1));

			Render_pass(*m_viewports.front().get(), rendermode::pickcontrols);
			m_pick_fb->bind();
			m_picking_pbo->request_read(pickbufferpos.x, pickbufferpos.y, 1, 1);
			m_pick_fb->unbind();
		}
	}
}

void opengl33_renderer::Update_Pick_Node()
{
	if (!m_picking_node_pbo->is_busy())
	{
		unsigned char pickreadout[4];
		if (m_picking_node_pbo->read_data(1, 1, pickreadout))
		{
			auto const nodeindex = pick_index(glm::ivec3{pickreadout[0], pickreadout[1], pickreadout[2]});
			scene::basic_node *node{nullptr};
			if ((nodeindex > 0) && (nodeindex <= m_picksceneryitems.size()))
			{
				node = m_picksceneryitems[nodeindex - 1];
			}

			m_picksceneryitem = node;

			for (auto f : m_node_pick_requests)
				f(m_picksceneryitem);
			m_node_pick_requests.clear();
		}

		if (!m_node_pick_requests.empty())
		{
			// determine point to examine
			glm::dvec2 mousepos = Application.get_cursor_pos();
			mousepos.y = Global.iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

			glm::ivec2 pickbufferpos;
			pickbufferpos = glm::ivec2{mousepos.x * EU07_PICKBUFFERSIZE / std::max(1, Global.iWindowWidth), mousepos.y * EU07_PICKBUFFERSIZE / std::max(1, Global.iWindowHeight)};
			pickbufferpos = glm::clamp(pickbufferpos, glm::ivec2(0, 0), glm::ivec2(EU07_PICKBUFFERSIZE - 1, EU07_PICKBUFFERSIZE - 1));

			Render_pass(*m_viewports.front().get(), rendermode::pickscenery);
			m_pick_fb->bind();
			m_picking_node_pbo->request_read(pickbufferpos.x, pickbufferpos.y, 1, 1);
			m_pick_fb->unbind();
		}
	}
}

void opengl33_renderer::Pick_Control_Callback(std::function<void(TSubModel const *)> callback)
{
	m_control_pick_requests.push_back(callback);
}

void opengl33_renderer::Pick_Node_Callback(std::function<void(scene::basic_node *)> callback)
{
	m_node_pick_requests.push_back(callback);
}

glm::dvec3 opengl33_renderer::Update_Mouse_Position()
{
	if (!m_depth_pointer_pbo->is_busy())
	{
		// determine point to examine
		glm::dvec2 mousepos = Application.get_cursor_pos();
		mousepos.y = Global.iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

		glm::ivec2 bufferpos;
		bufferpos = glm::ivec2{mousepos.x * Global.gfx_framebuffer_width / std::max(1, Global.iWindowWidth), mousepos.y * Global.gfx_framebuffer_height / std::max(1, Global.iWindowHeight)};
		bufferpos = glm::clamp(bufferpos, glm::ivec2(0, 0), glm::ivec2(Global.gfx_framebuffer_width - 1, Global.gfx_framebuffer_height - 1));

		float pointdepth = std::numeric_limits<float>::max();

		if (!Global.gfx_usegles)
		{
			m_depth_pointer_pbo->read_data(1, 1, &pointdepth, 4);

			if (!Global.iMultisampling)
			{
				m_depth_pointer_pbo->request_read(bufferpos.x, bufferpos.y, 1, 1, 4, GL_DEPTH_COMPONENT, GL_FLOAT);
			}
			else if (Global.gfx_skippipeline)
			{
				gl::framebuffer::blit(nullptr, m_depth_pointer_fb.get(), bufferpos.x, bufferpos.y, 1, 1, GL_DEPTH_BUFFER_BIT, 0);

				m_depth_pointer_fb->bind();
				m_depth_pointer_pbo->request_read(0, 0, 1, 1, 4, GL_DEPTH_COMPONENT, GL_FLOAT);
				m_depth_pointer_fb->unbind();
			}
			else
			{
				gl::framebuffer::blit(m_viewports.front()->msaa_fb.get(), m_depth_pointer_fb.get(), bufferpos.x, bufferpos.y, 1, 1, GL_DEPTH_BUFFER_BIT, 0);

				m_depth_pointer_fb->bind();
				m_depth_pointer_pbo->request_read(0, 0, 1, 1, 4, GL_DEPTH_COMPONENT, GL_FLOAT);
				m_viewports.front()->msaa_fb->bind();
			}
		}
		else
		{
			unsigned int data[4];
			if (m_depth_pointer_pbo->read_data(1, 1, data, 16))
				pointdepth = (double)data[0] / 65535.0;

			if (Global.gfx_skippipeline)
			{
				gl::framebuffer::blit(nullptr, m_depth_pointer_fb.get(), 0, 0, Global.gfx_framebuffer_width, Global.gfx_framebuffer_height, GL_DEPTH_BUFFER_BIT, 0);

				m_empty_vao->bind();
				m_depth_pointer_tex->bind(0);
				m_depth_pointer_shader->bind();
				m_depth_pointer_fb2->bind();
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				m_depth_pointer_pbo->request_read(bufferpos.x, bufferpos.y, 1, 1, 16, GL_RGBA_INTEGER, GL_UNSIGNED_INT);
				m_depth_pointer_shader->unbind();
				m_empty_vao->unbind();
				m_depth_pointer_fb2->unbind();
			}
			else
			{
				gl::framebuffer::blit(m_viewports.front()->msaa_fb.get(), m_depth_pointer_fb.get(), 0, 0, Global.gfx_framebuffer_width, Global.gfx_framebuffer_height, GL_DEPTH_BUFFER_BIT, 0);

				m_empty_vao->bind();
				m_depth_pointer_tex->bind(0);
				m_depth_pointer_shader->bind();
				m_depth_pointer_fb2->bind();
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				m_depth_pointer_pbo->request_read(bufferpos.x, bufferpos.y, 1, 1, 16, GL_RGBA_INTEGER, GL_UNSIGNED_INT);
				m_depth_pointer_shader->unbind();
				m_empty_vao->unbind();
				m_viewports.front()->msaa_fb->bind();
			}
		}

		if (pointdepth != std::numeric_limits<float>::max())
		{
			if (GLAD_GL_ARB_clip_control || GLAD_GL_EXT_clip_control) {
				if (pointdepth > 0.0f)
					m_worldmousecoordinates = glm::unProjectZO(glm::vec3(bufferpos, pointdepth), glm::mat4(glm::mat3(m_colorpass.pass_camera.modelview())), m_colorpass.pass_camera.projection(),
					                                           glm::vec4(0, 0, Global.gfx_framebuffer_width, Global.gfx_framebuffer_height));
			} else if (pointdepth < 1.0f)
				m_worldmousecoordinates = glm::unProjectNO(glm::vec3(bufferpos, pointdepth), glm::mat4(glm::mat3(m_colorpass.pass_camera.modelview())), m_colorpass.pass_camera.projection(),
				                                           glm::vec4(0, 0, Global.gfx_framebuffer_width, Global.gfx_framebuffer_height));
		}
	}

	return m_colorpass.pass_camera.position() + glm::dvec3{m_worldmousecoordinates};
}

void opengl33_renderer::Update(double const Deltatime)
{
	Update_Pick_Control();
	Update_Pick_Node();

	m_updateaccumulator += Deltatime;

	if (m_updateaccumulator < 1.0)
	{
		// too early for any work
		return;
	}

	m_updateaccumulator = 0.0;
    m_framerate = 1000.f / ( Timer::subsystem.mainloop_total.average() );

	// adjust draw ranges etc, based on recent performance
	// TODO: it doesn't make much sense with vsync
    if( Global.targetfps == 0.0f ) {
        // automatic adjustment
        auto const framerate = interpolate( 1000.f / Timer::subsystem.gfx_color.average(), m_framerate, 0.75f );
        float targetfactor;
             if( framerate > 120.0 ) { targetfactor = 3.00f; }
        else if( framerate >  90.0 ) { targetfactor = 1.50f; }
        else if( framerate >  60.0 ) { targetfactor = 1.25f; }
        else                         { targetfactor = 1.00f; }
        if( targetfactor > Global.fDistanceFactor ) {
            Global.fDistanceFactor = std::min( targetfactor, Global.fDistanceFactor + 0.05f );
        }
        else if( targetfactor < Global.fDistanceFactor ) {
            Global.fDistanceFactor = std::max( targetfactor, Global.fDistanceFactor - 0.05f );
        }
    }
    else {
        auto const fps_diff = Global.targetfps - m_framerate;
        if( fps_diff > 0.5f ) {
            Global.fDistanceFactor = std::max( 1.0f, Global.fDistanceFactor - 0.05f );
        }
        else if( fps_diff < 1.0f ) {
            Global.fDistanceFactor = std::min( 3.0f, Global.fDistanceFactor + 0.05f );
        }
    }

    if( Global.UpdateMaterials ) {
        // update resources if there was environmental change
        simulation_state simulationstate {
            Global.Weather,
            Global.Season
        };
        std::swap( m_simulationstate, simulationstate );
        if( ( m_simulationstate.season != simulationstate.season ) && ( false == simulationstate.season.empty() ) ) {
            m_materials.on_season_change();
        }
        if( ( m_simulationstate.weather != simulationstate.weather ) && ( false == simulationstate.weather.empty() ) ) {
            m_materials.on_weather_change();
        }
    }

	if ((true == Global.ResourceSweep) && (true == simulation::is_ready))
	{
		// garbage collection
		m_geometry.update();
		m_textures.update();
	}

	if ((true == Global.ControlPicking) && (false == FreeFlyModeFlag))
		Pick_Control_Callback([](const TSubModel *) {});
	// temporary conditions for testing. eventually will be coupled with editor mode
	if ((true == Global.ControlPicking) && (true == DebugModeFlag) && (true == FreeFlyModeFlag))
		Pick_Node_Callback([](scene::basic_node *) {});

	// dump last opengl error, if any
	auto const glerror = ::glGetError();
	if (glerror != GL_NO_ERROR)
	{
		std::string glerrorstring;
		if (glerror == GL_INVALID_ENUM)
			glerrorstring = "GL_INVALID_ENUM";
		else if (glerror == GL_INVALID_VALUE)
			glerrorstring = "GL_INVALID_VALUE";
		else if (glerror == GL_INVALID_OPERATION)
			glerrorstring = "GL_INVALID_OPERATION";
		else if (glerror == GL_OUT_OF_MEMORY)
			glerrorstring = "GL_OUT_OF_MEMORY";
		else if (glerror == GL_INVALID_FRAMEBUFFER_OPERATION)
			glerrorstring = "GL_INVALID_FRAMEBUFFER_OPERATION";
		Global.LastGLError = std::to_string(glerror) + " (" + glerrorstring + ")";
	}
}

// debug performance string
std::string const &opengl33_renderer::info_times() const
{

	return m_debugtimestext;
}

std::string const &opengl33_renderer::info_stats() const
{

	return m_debugstatstext;
}

void opengl33_renderer::Update_Lights(light_array &Lights)
{
	glDebug("Update_Lights");

    Bind_Texture( gl::HEADLIGHT_TEX, m_headlightstexture );

	// arrange the light array from closest to farthest from current position of the camera
	auto const camera = m_colorpass.pass_camera.position();
	std::sort(
        std::begin(Lights.data), std::end(Lights.data),
        [&camera](light_array::light_record const &Left, light_array::light_record const &Right) {
		    // move lights which are off at the end...
		    if (Left.intensity  == 0.f) { return false; }
		    if (Right.intensity == 0.f) { return true; }
		    // ...otherwise prefer closer and/or brigher light sources
		    return (glm::length2(camera - Left.position) / Left.intensity) < (glm::length2(camera - Right.position) / Right.intensity);
    	});

    // set up helpers
   	glm::mat4 coordmove;
	if (GLAD_GL_ARB_clip_control || GLAD_GL_EXT_clip_control)
		// transform 1..-1 NDC xy coordinates to 1..0
		coordmove = glm::mat4( //
			0.5, 0.0, 0.0, 0.0, //
			0.0, 0.5, 0.0, 0.0, //
			0.0, 0.0, 1.0, 0.0, //
			0.5, 0.5, 0.0, 1.0 //
		);
	else
		// without clip_control we also need to transform z
		coordmove = glm::mat4( //
			0.5, 0.0, 0.0, 0.0, //
			0.0, 0.5, 0.0, 0.0, //
			0.0, 0.0, 0.5, 0.0, //
			0.5, 0.5, 0.5, 1.0 //
		);
    glm::mat4 mv = OpenGLMatrices.data( GL_MODELVIEW );

    // fill vehicle headlights data
    auto renderlight = m_lights.begin();
    size_t light_i = 1;

    for (auto const &scenelight : Lights.data)
	{
		if (renderlight == m_lights.end()) {
			// we ran out of lights to assign
			break;
		}
		if (scenelight.intensity == 0.f) {
			// all lights past this one are bound to be off
			break;
		}
		auto const lightoffset = glm::vec3{scenelight.position - camera};
		if (glm::length(lightoffset) > 1000.f) {
			// we don't care about lights past arbitrary limit of 1 km.
			// but there could still be weaker lights which are closer, so keep looking
			continue;
		}
		// if the light passed tests so far, it's good enough
		renderlight->position = lightoffset;
		renderlight->direction = scenelight.direction;

		auto luminance = static_cast<float>(Global.fLuminance);
		// adjust luminance level based on vehicle's location, e.g. tunnels
		auto const environment = scenelight.owner->fShade;
		if (environment > 0.f)
		{
			luminance *= environment;
		}
		renderlight->diffuse = glm::vec4{glm::max(glm::vec3{colors::none}, scenelight.color - glm::vec3{luminance}), renderlight->diffuse[3]};
		renderlight->ambient = glm::vec4{glm::max(glm::vec3{colors::none}, scenelight.color * glm::vec3{scenelight.intensity} - glm::vec3{luminance}), renderlight->ambient[3]};

		renderlight->apply_intensity( ( scenelight.count * 0.5 ) * ( scenelight.owner->DimHeadlights ? 0.5 : 1.0 ) );
		renderlight->apply_angle();

		gl::light_element_ubs *light = &light_ubs.lights[light_i];

		light->pos = mv * glm::vec4(renderlight->position, 1.0f);
		light->dir = mv * glm::vec4(renderlight->direction, 0.0f);
		light->type = gl::light_element_ubs::HEADLIGHTS;
		light->in_cutoff = headlight_config.in_cutoff;
		light->out_cutoff = headlight_config.out_cutoff;
		light->color = renderlight->diffuse * renderlight->factor;
		light->linear = headlight_config.falloff_linear / 10.0f;
		light->quadratic = headlight_config.falloff_quadratic / 100.0f;
		light->ambient = headlight_config.ambient;
		light->intensity = headlight_config.intensity;
        // headlights-to-world projection
        {
            // headlight projection is 1 km long box aligned with vehicle rotation and aimed slightly downwards
            opengl_camera headlights;
            auto const &ownerdimensions{ scenelight.owner->MoverParameters->Dim };
            auto const up{ static_cast<glm::dvec3>( scenelight.owner->VectorUp() ) };
            auto const size{ static_cast<float>( std::max( ownerdimensions.W, ownerdimensions.H ) * 1.0 ) }; // ensure square ratio
            auto const cone{ 5.f };
            auto const offset{ 75.f * ( 5.f / cone ) };
            headlights.position() =
                scenelight.owner->GetPosition()
                - scenelight.direction * offset
                + up * ( size * 0.5 );
/*
            headlights.projection() = ortho_projection(
                -size, size,
                -size, size,
                ownerdimensions.L * 0.5 - 0.5, 1000.0f );
*/
            headlights.projection() = perspective_projection(
                glm::radians( cone ),
                1.0,
                ownerdimensions.L * 0.5 + offset - 0.25, 1000.0f );
            glm::dmat4 viewmatrix{ 1.0 };
            viewmatrix *=
                glm::lookAt(
                    headlights.position(),
                    headlights.position()
                    - up * 1.0
                    + glm::dvec3{ scenelight.direction * 1000.f },
                    glm::dvec3{ 0.f, 1.f, 0.f } );
            headlights.modelview() = viewmatrix;
            // calculate world->headlight space projection matrix
            glm::mat4 const depthproj{ headlights.projection() };
            // NOTE: we strip transformations from camera projections to remove jitter that occurs
            // with large (and unneded as we only need the offset) transformations back and forth
            auto const lightcam{ glm::mat3{ headlights.modelview() } };
            auto const worldcam{ glm::mat3{ m_renderpass.pass_camera.modelview() } };

            light->headlight_projection =
                coordmove
                * depthproj
                * glm::translate(
                    glm::mat4{ lightcam },
                    glm::vec3{ m_renderpass.pass_camera.position() - headlights.position() } )
                * glm::mat4{ glm::inverse( worldcam ) };
        }
        // headlights weights
        light->headlight_weights = { scenelight.state, 0.f };

		++light_i;
		++renderlight;
	}
    light_ubs.lights_count = light_i;
    // fill sunlight data
    light_ubs.ambient = m_sunlight.ambient * m_sunlight.factor;// *simulation::Environment.light_intensity();
	light_ubs.lights[0].type = gl::light_element_ubs::DIR;
	light_ubs.lights[0].dir = mv * glm::vec4(m_sunlight.direction, 0.0f);
	light_ubs.lights[0].color = m_sunlight.diffuse * m_sunlight.factor * simulation::Environment.light_intensity();
	light_ubs.lights[0].ambient = 0.0f;
	light_ubs.lights[0].intensity = 1.0f;
    // fill fog data
	light_ubs.fog_color = Global.FogColor;
	if (Global.fFogEnd > 0)
	{
		m_fogrange = Global.fFogEnd / std::max(1.f, Global.Overcast * 2.f);
		model_ubs.fog_density = 1.0f / m_fogrange;
	}
    else
    {
        model_ubs.fog_density = 0.0f;
    }
    // ship config data to the gpu
	model_ubo->update(model_ubs);
	light_ubo->update(light_ubs);
}

bool opengl33_renderer::Init_caps()
{
    WriteLog(
        "Gfx Renderer: " + std::string( (char *)glGetString(GL_RENDERER))
        + " Vendor: " + std::string( (char *)glGetString(GL_VENDOR))
        + " OpenGL Version: " + std::string((char *)glGetString(GL_VERSION)) );

    {
        GLint extCount = 0;
        glGetIntegerv( GL_NUM_EXTENSIONS, &extCount );
        std::string extensions;

        WriteLog( "Supported extensions:" );
        for( int i = 0; i < extCount; i++ ) {
            const char *ext = (const char *)glGetStringi( GL_EXTENSIONS, i );
            extensions += ext;
            extensions += " ";
        }
        WriteLog( extensions );
    }

	if (!Global.gfx_usegles)
	{
		if (!GLAD_GL_VERSION_3_3)
		{
			ErrorLog("requires OpenGL >= 3.3!");
			return false;
		}

		if (!GLAD_GL_EXT_texture_sRGB)
			ErrorLog("warning: EXT_texture_sRGB not supported");

		if (!GLAD_GL_EXT_texture_compression_s3tc)
			ErrorLog("warning: EXT_texture_compression_s3tc not supported");

		if (GLAD_GL_ARB_texture_filter_anisotropic)
			WriteLog("ARB_texture_filter_anisotropic supported");

		if (GLAD_GL_ARB_multi_bind)
			WriteLog("ARB_multi_bind supported");

		if (GLAD_GL_ARB_direct_state_access)
			WriteLog("ARB_direct_state_access supported");

		if (GLAD_GL_ARB_clip_control)
			WriteLog("ARB_clip_control supported");
	}
	else
	{
		if (!GLAD_GL_ES_VERSION_3_0)
		{
			ErrorLog("requires OpenGL ES >= 3.0!");
			return false;
		}

		if (GLAD_GL_EXT_texture_filter_anisotropic)
			WriteLog("EXT_texture_filter_anisotropic supported");

		if (GLAD_GL_EXT_clip_control)
			WriteLog("EXT_clip_control supported");

		if (GLAD_GL_EXT_geometry_shader)
			WriteLog("EXT_geometry_shader supported");
	}

	glGetError();
	glLineWidth(2.0f);
	if (!glGetError()) {
		WriteLog("wide lines supported");
		m_widelines_supported = true;
	}
    else {
        WriteLog( "warning: wide lines not supported" );
    }

    WriteLog( "render path: Shaders, VBO" );

	// ograniczenie maksymalnego rozmiaru tekstur - parametr dla skalowania tekstur
	{
		GLint texturesize;
		::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texturesize);
		Global.iMaxTextureSize = std::min(Global.iMaxTextureSize, texturesize);
        Global.iMaxCabTextureSize = std::min( Global.iMaxCabTextureSize, texturesize );
        WriteLog( "texture sizes capped at " + std::to_string(Global.iMaxTextureSize) + "p (" + std::to_string( Global.iMaxCabTextureSize ) + "p for cab textures)" );
        Global.CurrentMaxTextureSize = Global.iMaxTextureSize;
        m_shadowbuffersize = Global.shadowtune.map_size;
		m_shadowbuffersize = std::min(m_shadowbuffersize, texturesize);
		WriteLog("shadows map size capped at " + std::to_string(m_shadowbuffersize) + "p");
	}
	Global.DynamicLightCount = std::min(Global.DynamicLightCount, 8 - 1);

	if (Global.iMultisampling)
	{
		WriteLog("using multisampling x" + std::to_string(1 << Global.iMultisampling));
	}

	if (Global.gfx_framebuffer_width == -1)
		Global.gfx_framebuffer_width = Global.iWindowWidth;
	if (Global.gfx_framebuffer_height == -1)
		Global.gfx_framebuffer_height = Global.iWindowHeight;

	WriteLog("main window size: " + std::to_string(Global.gfx_framebuffer_width) + " x " + std::to_string(Global.gfx_framebuffer_height));

	return true;
}

glm::vec3 opengl33_renderer::pick_color(std::size_t const Index)
{
	return glm::vec3{((Index & 0xff0000) >> 16) / 255.0f, ((Index & 0x00ff00) >> 8) / 255.0f, (Index & 0x0000ff) / 255.0f};
}

std::size_t opengl33_renderer::pick_index(glm::ivec3 const &Color)
{
	return Color.b + (Color.g * 256) + (Color.r * 256 * 256);
}

//---------------------------------------------------------------------------