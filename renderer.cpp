/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "renderer.h"
#include "color.h"
#include "Globals.h"
#include "Timer.h"
#include "Train.h"
#include "simulation.h"
#include "uilayer.h"
#include "Logs.h"
#include "utilities.h"

opengl_renderer GfxRenderer;
extern TWorld World;

int const EU07_PICKBUFFERSIZE { 1024 }; // size of (square) textures bound with the pick framebuffer
int const EU07_ENVIRONMENTBUFFERSIZE { 256 }; // size of (square) environmental cube map texture

void
opengl_light::apply_intensity( float const Factor ) {
    factor = Factor;
}

void
opengl_light::apply_angle() {
}

void
opengl_camera::update_frustum( glm::mat4 const &Projection, glm::mat4 const &Modelview )
{
    m_frustum.calculate( Projection, Modelview );
    // cache inverse tranformation matrix
    // NOTE: transformation is done only to camera-centric space
    m_inversetransformation = glm::inverse( Projection * glm::mat4{ glm::mat3{ Modelview } } );
    // calculate frustum corners
    m_frustumpoints = ndcfrustumshapepoints;
    transform_to_world(
        std::begin( m_frustumpoints ),
        std::end( m_frustumpoints ) );
}

// returns true if specified object is within camera frustum, false otherwise
bool
opengl_camera::visible( scene::bounding_area const &Area ) const
{
    return ( m_frustum.sphere_inside( Area.center, Area.radius ) > 0.f );
}

bool
opengl_camera::visible( TDynamicObject const *Dynamic ) const
{
    // sphere test is faster than AABB, so we'll use it here
    glm::vec3 diagonal(
        static_cast<float>( Dynamic->MoverParameters->Dim.L ),
        static_cast<float>( Dynamic->MoverParameters->Dim.H ),
        static_cast<float>( Dynamic->MoverParameters->Dim.W ) );
    // we're giving vehicles some extra padding, to allow for things like shared bogeys extending past the main body
    float const radius = glm::length( diagonal ) * 0.65f;

    return ( m_frustum.sphere_inside( Dynamic->GetPosition(), radius ) > 0.0f );
}

// debug helper, draws shape of frustum in world space
void
opengl_camera::draw( glm::vec3 const &Offset ) const
{
    // m7t port to core gl
    /*
    ::glBegin( GL_LINES );
    for( auto const pointindex : frustumshapepoinstorder ) {
        ::glVertex3fv( glm::value_ptr( glm::vec3{ m_frustumpoints[ pointindex ] } - Offset ) );
    }
    ::glEnd();
    */
}

bool
opengl_renderer::Init( GLFWwindow *Window ) {

    if( false == Init_caps() ) { return false; }

    m_window = Window;

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );

    glClearDepth( 1.0f );
    glClearColor( 51.0f / 255.0f, 102.0f / 255.0f, 85.0f / 255.0f, 1.0f ); // initial background Color

    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );

    glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );

    if( true == Global.ScaleSpecularValues ) {
        m_specularopaquescalefactor = 0.25f;
        m_speculartranslucentscalefactor = 1.5f;
    }

    // rgb value for 5780 kelvin
    Global.DayLight.diffuse[ 0 ] = 255.0f / 255.0f;
    Global.DayLight.diffuse[ 1 ] = 242.0f / 255.0f;
    Global.DayLight.diffuse[ 2 ] = 231.0f / 255.0f;
    Global.DayLight.is_directional = true;
    m_sunlight.id = opengl_renderer::sunlight;

    // create dynamic light pool
    for( int idx = 0; idx < Global.DynamicLightCount; ++idx ) {

        opengl_light light;
        light.id = GL_LIGHT1 + idx;

        light.is_directional = false;

        m_lights.emplace_back( light );
    }
    // preload some common textures
    WriteLog( "Loading common gfx data..." );
    m_glaretexture = Fetch_Texture( "fx/lightglare" );
    m_suntexture = Fetch_Texture( "fx/sun" );
    m_moontexture = Fetch_Texture( "fx/moon" );
    WriteLog( "...gfx data pre-loading done" );

    // prepare basic geometry chunks
    auto const geometrybank = m_geometry.create_bank();
    float const size = 2.5f;
    m_billboardgeometry = m_geometry.create_chunk(
        gfx::vertex_array{
            { { -size,  size, 0.f }, glm::vec3(), { 1.f, 1.f } },
            { {  size,  size, 0.f }, glm::vec3(), { 0.f, 1.f } },
            { { -size, -size, 0.f }, glm::vec3(), { 1.f, 0.f } },
            { {  size, -size, 0.f }, glm::vec3(), { 0.f, 0.f } } },
            geometrybank, GL_TRIANGLE_STRIP );

    m_vertex_shader = std::make_unique<gl::shader>("simple.vert");
    scene_ubo = std::make_unique<gl::ubo>(sizeof(gl::scene_ubs), 0);
    model_ubo = std::make_unique<gl::ubo>(sizeof(gl::model_ubs), 1);
    light_ubo = std::make_unique<gl::ubo>(sizeof(gl::light_ubs), 2);
    memset(&light_ubs, 0, sizeof(light_ubs));

    // m7t: tbd: plug into material system?
    {
        gl::shader vert("traction.vert");
        gl::shader frag("traction.frag");
        gl::program *prog = new gl::program_mvp({vert, frag});
        prog->init();
        m_line_shader = std::unique_ptr<gl::program>(prog);
    }

    {
        gl::shader vert("freespot.vert");
        gl::shader frag("freespot.frag");
        gl::program *prog = new gl::program_mvp({vert, frag});
        prog->init();
        m_freespot_shader = std::unique_ptr<gl::program>(prog);
    }

    {
        gl::shader vert("shadowmap.vert");
        gl::shader frag("shadowmap.frag");
        gl::program *prog = new gl::program_mvp({vert, frag});
        prog->init();
        m_shadow_shader = std::unique_ptr<gl::program>(prog);
    }

    m_invalid_material = Fetch_Material("invalid");

    m_main_fb = std::make_unique<gl::framebuffer>();
    m_main_tex = std::make_unique<opengl_texture>();
    m_main_rb = std::make_unique<gl::renderbuffer>();

    m_main_rb->alloc(GL_DEPTH_COMPONENT24, 1280, 720);
    m_main_tex->alloc_rendertarget(GL_RGB16F, GL_RGB, GL_FLOAT, 1280, 720);

    m_main_fb->attach(*m_main_tex, GL_COLOR_ATTACHMENT0);
    m_main_fb->attach(*m_main_rb, GL_DEPTH_ATTACHMENT);
    if (!m_main_fb->is_complete())
        return false;

    m_pfx = std::make_unique<gl::postfx>("copy");

    m_shadow_fb = std::make_unique<gl::framebuffer>();
    m_shadow_tex = std::make_unique<opengl_texture>();
    m_shadow_tex->alloc_rendertarget(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, m_shadowbuffersize, m_shadowbuffersize);
	m_shadow_fb->attach(*m_shadow_tex, GL_DEPTH_ATTACHMENT);

    if (!m_shadow_fb->is_complete())
        return false;

    return true;
}

bool
opengl_renderer::Render() {

    Timer::subsystem.gfx_total.stop();
    Timer::subsystem.gfx_total.start(); // note: gfx_total is actually frame total, clean this up
    Timer::subsystem.gfx_color.start();

	GLuint gl_time_ready = 0;
    if (GLEW_ARB_timer_query)
    {
        if (m_gltimequery)
        {
            glGetQueryObjectuiv(m_gltimequery, GL_QUERY_RESULT_AVAILABLE, &gl_time_ready);
            if (gl_time_ready)
                glGetQueryObjectui64v(m_gltimequery, GL_QUERY_RESULT, &m_gllasttime);
        }
        else
        {
            glGenQueries(1, &m_gltimequery);
            gl_time_ready = 1;
        }
    }

    if (gl_time_ready)
		glBeginQuery(GL_TIME_ELAPSED, m_gltimequery);

    // fetch simulation data
    if( World.InitPerformed() ) {
        m_sunlight = Global.DayLight;
        // quantize sun angle to reduce shadow crawl
        auto const quantizationstep { 0.004f };
        m_sunlight.direction = glm::normalize( quantizationstep * glm::roundEven( m_sunlight.direction * ( 1.f / quantizationstep ) ) );
    }
    // generate new frame
    m_renderpass.draw_mode = rendermode::none; // force setup anew
    m_debugstats = debug_stats();

    Render_pass( rendermode::color );
    Timer::subsystem.gfx_color.stop();

    if (gl_time_ready)
        glEndQuery(GL_TIME_ELAPSED);

    m_drawcount = m_cellqueue.size();
    m_debugtimestext.clear();
    m_debugtimestext
        += "cpu: " + to_string( Timer::subsystem.gfx_color.average(), 2 ) + " ms (" + std::to_string( m_cellqueue.size() ) + " sectors)\n"
        += "cpu swap: " + to_string( Timer::subsystem.gfx_swap.average(), 2 ) + " ms\n"
        += "uilayer: " + to_string(Timer::subsystem.gfx_gui.average(), 2) + "ms\n"
        += "mainloop total: " + to_string(Timer::subsystem.mainloop_total.average(), 2) + "ms\n";

	if (m_gllasttime)
		m_debugtimestext += "gpu: " + to_string((double)(m_gllasttime / 1000ULL) / 1000.0, 3) + "ms";

    m_debugstatstext =
        "drawcalls: " + to_string( m_debugstats.drawcalls )
        + "; dyn: " + to_string( m_debugstats.dynamics ) + " mod: " + to_string( m_debugstats.models ) + " sub: " + to_string( m_debugstats.submodels )
        + "; trk: " + to_string( m_debugstats.paths ) + " shp: " + to_string( m_debugstats.shapes )
        + " trc: " + to_string( m_debugstats.traction ) + " lin: " + to_string( m_debugstats.lines );

    ++m_framestamp;

    return true; // for now always succeed
}

void opengl_renderer::SwapBuffers()
{
    Timer::subsystem.gfx_swap.start();
    glfwSwapBuffers( m_window );
    Timer::subsystem.gfx_swap.stop();
}

// runs jobs needed to generate graphics for specified render pass
void
opengl_renderer::Render_pass( rendermode const Mode ) {
    setup_pass( m_renderpass, Mode );
    switch( m_renderpass.draw_mode ) {

        case rendermode::color: {
            glDebug("rendermode::color");

            {
                glDebug("render shadowmap start");
                Timer::subsystem.gfx_shadows.start();

                Render_pass(rendermode::shadows);
                setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack

                Timer::subsystem.gfx_shadows.stop();
                glDebug("render shadowmap end");
            }

            m_main_fb->bind();

            if( ( true == m_environmentcubetexturesupport )
             && ( true == World.InitPerformed() ) ) {
                // potentially update environmental cube map
                if( true == Render_reflections() ) {
                    setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack
                }
            }

            glViewport( 0, 0, 1280, 720 );

            if( World.InitPerformed() ) {
                auto const skydomecolour = World.Environment.m_skydome.GetAverageColor();
                ::glClearColor( skydomecolour.x, skydomecolour.y, skydomecolour.z, 0.f ); // kolor nieba
            }
            else {
                ::glClearColor( 51.0f / 255.f, 102.0f / 255.f, 85.0f / 255.f, 1.f ); // initial background Color
            }
            ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            if( World.InitPerformed() ) {
                // setup
                setup_matrices();
                // render
                setup_drawing( true );

                glDebug("render environment");

                scene_ubs.time = Timer::GetTime();
                scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
                scene_ubo->update(scene_ubs);
                Render( &World.Environment );

				{
					glm::mat4 coordmove(
						0.5, 0.0, 0.0, 0.0,
						0.0, 0.5, 0.0, 0.0,
						0.0, 0.0, 0.5, 0.0,
						0.5, 0.5, 0.5, 1.0);
					glm::mat4 depthproj = m_shadowpass.camera.projection();
					glm::mat4 depthcam = m_shadowpass.camera.modelview();
					glm::mat4 worldcam = m_renderpass.camera.modelview();

					scene_ubs.lightview = coordmove * depthproj * depthcam * glm::inverse(worldcam);
				}

                scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
                scene_ubo->update(scene_ubs);
                // opaque parts...
                setup_drawing( false );

                if( false == FreeFlyModeFlag ) {
                    glDebug("render cab opaque");
                    //setup_shadow_map( m_cabshadowtexture, m_cabshadowtexturematrix );
                    // cache shadow colour in case we need to account for cab light
                    auto const shadowcolor { m_shadowcolor };
                    auto const *vehicle{ World.Train->Dynamic() };
                    if( vehicle->InteriorLightLevel > 0.f ) {
                        setup_shadow_color( glm::min( colors::white, shadowcolor + glm::vec4( vehicle->InteriorLight * vehicle->InteriorLightLevel, 1.f ) ) );
                    }
                    Render_cab( vehicle, false );
                    if( vehicle->InteriorLightLevel > 0.f ) {
                        setup_shadow_color( shadowcolor );
                    }
                }

                glDebug("render opaque region");

                setup_shadow_map(*m_shadow_tex);
                Render( simulation::Region );

                // ...translucent parts
                glDebug("render translucent region");
                setup_drawing( true );
                Render_Alpha( simulation::Region );
                if( false == FreeFlyModeFlag ) {
                    glDebug("render translucent cab");
                    // cab render is performed without shadows, due to low resolution and number of models without windows :|
                    //setup_shadow_map( m_cabshadowtexture, m_cabshadowtexturematrix );
                    // cache shadow colour in case we need to account for cab light
                    auto const shadowcolor{ m_shadowcolor };
                    auto const *vehicle{ World.Train->Dynamic() };
                    if( vehicle->InteriorLightLevel > 0.f ) {
                        setup_shadow_color( glm::min( colors::white, shadowcolor + glm::vec4( vehicle->InteriorLight * vehicle->InteriorLightLevel, 1.f ) ) );
                    }
                    Render_cab( vehicle, true );
                    if( vehicle->InteriorLightLevel > 0.f ) {
                        setup_shadow_color( shadowcolor );
                    }
                }

                /*
                if( m_environmentcubetexturesupport ) {
                    // restore default texture matrix for reflections cube map
                    select_unit( m_helpertextureunit );
                    ::glMatrixMode( GL_TEXTURE );
                    ::glPopMatrix();
                    select_unit( m_diffusetextureunit );
                    ::glMatrixMode( GL_MODELVIEW );
                }
                */

                glDebug("color pass done");
            }

			glEnable(GL_FRAMEBUFFER_SRGB);
			glViewport(0, 0, Global.iWindowWidth, Global.iWindowHeight);
            m_pfx->apply(*m_main_tex, nullptr);
            m_textures.reset_unit_cache();
            glDisable(GL_FRAMEBUFFER_SRGB);

            UILayer.render();
            break;
        }

        case rendermode::shadows: {
            if (!World.InitPerformed())
                break;

            glDebug("rendermode::shadows");

            // bias (TBD: here or in glsl?)
            //glEnable(GL_POLYGON_OFFSET_FILL);
            //glPolygonOffset(1.0f, 1.0f);

			glViewport(0, 0, m_shadowbuffersize, m_shadowbuffersize);
            m_shadow_fb->bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            setup_matrices();
            setup_drawing(false);

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);

			scene_ubs.time = Timer::GetTime();
			scene_ubs.projection = OpenGLMatrices.data(GL_PROJECTION);
			scene_ubo->update(scene_ubs);
            Render(simulation::Region);
            m_shadowpass = m_renderpass;

            //glDisable(GL_POLYGON_OFFSET_FILL);

            m_shadow_fb->unbind();

            break;
        }

        case rendermode::cabshadows: {
            break;
        }

        case rendermode::reflections: {
            break;
        }

        case rendermode::pickcontrols: {
            break;
        }

        case rendermode::pickscenery: {
            break;
        }

        default: {
            break;
        }
    }
}

// creates dynamic environment cubemap
bool
opengl_renderer::Render_reflections() {

    return true;
}

void
opengl_renderer::setup_pass( renderpass_config &Config, rendermode const Mode, float const Znear, float const Zfar, bool const Ignoredebug ) {

    Config.draw_mode = Mode;

    if( false == World.InitPerformed() ) { return; }
    // setup draw range
    switch( Mode ) {
        case rendermode::color:        { Config.draw_range = Global.BaseDrawRange; break; }
        case rendermode::shadows:      { Config.draw_range = Global.BaseDrawRange * 0.5f; break; }
        case rendermode::cabshadows:   { Config.draw_range = ( Global.pWorld->train()->Dynamic()->MoverParameters->ActiveCab != 0 ? 10.f : 20.f ); break; }
        case rendermode::reflections:  { Config.draw_range = Global.BaseDrawRange; break; }
        case rendermode::pickcontrols: { Config.draw_range = 50.f; break; }
        case rendermode::pickscenery:  { Config.draw_range = Global.BaseDrawRange * 0.5f; break; }
        default:                       { Config.draw_range = 0.f; break; }
    }
    // setup camera
    auto &camera = Config.camera;

    camera.projection() = glm::mat4( 1.f );
    glm::dmat4 viewmatrix( 1.0 );

    switch( Mode ) {
        case rendermode::color: {
            // modelview
            if( ( false == DebugCameraFlag ) || ( true == Ignoredebug ) ) {
                camera.position() = Global.pCameraPosition;
                World.Camera.SetMatrix( viewmatrix );
            }
            else {
                camera.position() = Global.DebugCameraPosition;
                World.DebugCamera.SetMatrix( viewmatrix );
            }
            // projection
            auto const zfar = Config.draw_range * Global.fDistanceFactor * Zfar;
            auto const znear = (
                Znear > 0.f ?
                Znear * zfar :
                0.1f * Global.ZoomFactor );
            camera.projection() *=
                glm::perspective(
                    glm::radians( Global.FieldOfView / Global.ZoomFactor ),
                    std::max( 1.f, (float)Global.iWindowWidth ) / std::max( 1.f, (float)Global.iWindowHeight ),
                    znear,
                    zfar );
/*
            m_sunandviewangle =
                glm::dot(
                    m_sunlight.direction,
                    glm::vec3( 0.f, 0.f, -1.f ) * glm::mat3( viewmatrix ) );
*/
            break;
        }
        case rendermode::shadows: {
            // calculate lightview boundaries based on relevant area of the world camera frustum:
            // ...setup chunk of frustum we're interested in...
            auto const zfar = std::min( 1.f, Global.shadowtune.depth / ( Global.BaseDrawRange * Global.fDistanceFactor ) * std::max( 1.f, Global.ZoomFactor * 0.5f ) );
            renderpass_config worldview;
            setup_pass( worldview, rendermode::color, 0.f, zfar, true );
            auto &frustumchunkshapepoints = worldview.camera.frustum_points();
            // ...modelview matrix: determine the centre of frustum chunk in world space...
            glm::vec3 frustumchunkmin, frustumchunkmax;
            bounding_box( frustumchunkmin, frustumchunkmax, std::begin( frustumchunkshapepoints ), std::end( frustumchunkshapepoints ) );
            auto const frustumchunkcentre = ( frustumchunkmin + frustumchunkmax ) * 0.5f;
            // ...cap the vertical angle to keep shadows from getting too long...
            auto const lightvector =
                glm::normalize( glm::vec3{
                              m_sunlight.direction.x,
                    std::min( m_sunlight.direction.y, -0.2f ),
                              m_sunlight.direction.z } );
            // ...place the light source at the calculated centre and setup world space light view matrix...
            camera.position() = worldview.camera.position() + glm::dvec3{ frustumchunkcentre };
            viewmatrix *= glm::lookAt(
                camera.position(),
                camera.position() + glm::dvec3{ lightvector },
                glm::dvec3{ 0.f, 1.f, 0.f } );
            // ...projection matrix: calculate boundaries of the frustum chunk in light space...
            auto const lightviewmatrix =
                glm::translate(
                    glm::mat4{ glm::mat3{ viewmatrix } },
                    -frustumchunkcentre );
            for( auto &point : frustumchunkshapepoints ) {
                point = lightviewmatrix * point;
            }
            bounding_box( frustumchunkmin, frustumchunkmax, std::begin( frustumchunkshapepoints ), std::end( frustumchunkshapepoints ) );
            // quantize the frustum points and add some padding, to reduce shadow shimmer on scale changes
            auto const quantizationstep{ std::min( Global.shadowtune.depth, 50.f ) };
            frustumchunkmin = quantizationstep * glm::floor( frustumchunkmin * ( 1.f / quantizationstep ) );
            frustumchunkmax = quantizationstep * glm::ceil( frustumchunkmax * ( 1.f / quantizationstep ) );
            // ...use the dimensions to set up light projection boundaries...
            // NOTE: since we only have one cascade map stage, we extend the chunk forward/back to catch areas normally covered by other stages
            camera.projection() *=
                glm::ortho(
                    frustumchunkmin.x, frustumchunkmax.x,
                    frustumchunkmin.y, frustumchunkmax.y,
                    frustumchunkmin.z - 500.f, frustumchunkmax.z + 500.f );
    /*
            // fixed ortho projection from old build, for quick quality comparisons
            camera.projection() *=
                glm::ortho(
                    -Global.shadowtune.width, Global.shadowtune.width,
                    -Global.shadowtune.width, Global.shadowtune.width,
                    -Global.shadowtune.depth, Global.shadowtune.depth );
            camera.position() = Global.pCameraPosition - glm::dvec3{ m_sunlight.direction };
            if( camera.position().y - Global.pCameraPosition.y < 0.1 ) {
                camera.position().y = Global.pCameraPosition.y + 0.1;
            }
            viewmatrix *= glm::lookAt(
                camera.position(),
                glm::dvec3{ Global.pCameraPosition },
                glm::dvec3{ 0.f, 1.f, 0.f } );
    */
            // ... and adjust the projection to sample complete shadow map texels:
            // get coordinates for a sample texel...
            auto shadowmaptexel = glm::vec2 { camera.projection() * glm::mat4{ viewmatrix } * glm::vec4{ 0.f, 0.f, 0.f, 1.f } };
            // ...convert result from clip space to texture coordinates, and calculate adjustment...
            shadowmaptexel *= m_shadowbuffersize * 0.5f;
            auto shadowmapadjustment = glm::round( shadowmaptexel ) - shadowmaptexel;
            // ...transform coordinate change back to homogenous light space...
            shadowmapadjustment /= m_shadowbuffersize * 0.5f;
            // ... and bake the adjustment into the projection matrix
            camera.projection() =
                glm::translate(
                    glm::mat4{ 1.f },
                    glm::vec3{ shadowmapadjustment, 0.f } )
                * camera.projection();

            break;
        }
        default: {
            break;
        }
    }
    camera.modelview() = viewmatrix;
    camera.update_frustum();
}

void
opengl_renderer::setup_matrices() {

    ::glMatrixMode( GL_PROJECTION );
    OpenGLMatrices.load_matrix( m_renderpass.camera.projection() );

    /*
    if( ( m_renderpass.draw_mode == rendermode::color )
     && ( m_environmentcubetexturesupport ) ) {
        // special case, for colour render pass setup texture matrix for reflections cube map
        select_unit( m_helpertextureunit );
        ::glMatrixMode( GL_TEXTURE );
        ::glPushMatrix();
        ::glMultMatrixf( glm::value_ptr( glm::inverse( glm::mat4{ glm::mat3{ m_renderpass.camera.modelview() } } ) ) );
        select_unit( m_diffusetextureunit );
    }
    */

    // trim modelview matrix just to rotation, since rendering is done in camera-centric world space
    ::glMatrixMode( GL_MODELVIEW );
    OpenGLMatrices.load_matrix( glm::mat4( glm::mat3( m_renderpass.camera.modelview() ) ) );
}

void
opengl_renderer::setup_drawing( bool const Alpha ) {

    if( true == Alpha )
        ::glEnable( GL_BLEND );
    else
        ::glDisable( GL_BLEND );

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections: {
            // setup fog
            if( Global.fFogEnd > 0 ) {
                // m7t setup fog ubo
            }

            break;
        }
        case rendermode::shadows:
        case rendermode::cabshadows:
        case rendermode::pickcontrols:
        case rendermode::pickscenery:
        {
            break;
        }
        default: {
            break;
        }
    }
}

// configures shadow texture unit for specified shadow map and conersion matrix
void
opengl_renderer::setup_shadow_map( opengl_texture &tex ) {
    glActiveTexture(GL_TEXTURE10);
    tex.bind();
    glActiveTexture(GL_TEXTURE0);
    m_textures.reset_unit_cache();
}

void
opengl_renderer::setup_shadow_color( glm::vec4 const &Shadowcolor ) {
}

void
opengl_renderer::setup_environment_light( TEnvironmentType const Environment ) {

    switch( Environment ) {
        case e_flat: {
            m_sunlight.apply_intensity();
//            m_environment = Environment;
            break;
        }
        case e_canyon: {
            m_sunlight.apply_intensity( 0.4f );
//            m_environment = Environment;
            break;
        }
        case e_tunnel: {
            m_sunlight.apply_intensity( 0.2f );
//            m_environment = Environment;
            break;
        }
        default: {
            break;
        }
    }
}

bool
opengl_renderer::Render( world_environment *Environment ) {

    // calculate shadow tone, based on positions of celestial bodies
    m_shadowcolor = interpolate(
        glm::vec4{ colors::shadow },
        glm::vec4{ colors::white },
        clamp( -Environment->m_sun.getAngle(), 0.f, 6.f ) / 6.f );
    if( ( Environment->m_sun.getAngle() < -18.f )
     && ( Environment->m_moon.getAngle() > 0.f ) ) {
        // turn on moon shadows after nautical twilight, if the moon is actually up
        m_shadowcolor = colors::shadow;
    }
    // soften shadows depending on sky overcast factor
    m_shadowcolor = glm::min(
        colors::white,
        m_shadowcolor + ( ( colors::white - colors::shadow ) * Global.Overcast ) );

    if( Global.bWireFrame ) {
        // bez nieba w trybie rysowania linii
        return false;
    }

    Bind_Material( null_handle );
    ::glDisable( GL_DEPTH_TEST );
    ::glDepthMask( GL_FALSE );
    ::glPushMatrix();

    model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
    model_ubo->update(model_ubs);

    // skydome
    Environment->m_skydome.Render();
    // skydome uses a custom vbo which could potentially confuse the main geometry system. hardly elegant but, eh
    gfx::opengl_vbogeometrybank::reset();

    //m7t: restore celestial bodies

    // clouds
    if( Environment->m_clouds.mdCloud ) {
        // setup
        //m7t set cloud color
        // render
        Render( Environment->m_clouds.mdCloud, nullptr, 100.0 );
        Render_Alpha( Environment->m_clouds.mdCloud, nullptr, 100.0 );
        // post-render cleanup
    }

    m_sunlight.apply_angle();
    m_sunlight.apply_intensity();

    ::glPopMatrix();
    ::glDepthMask( GL_TRUE );
    ::glEnable( GL_DEPTH_TEST );

    return true;
}

// geometry methods
// creates a new geometry bank. returns: handle to the bank or NULL
gfx::geometrybank_handle
opengl_renderer::Create_Bank() {

    return m_geometry.create_bank();
}

// creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle
opengl_renderer::Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) {
    gfx::calculate_tangent(Vertices, Type);

    return m_geometry.create_chunk( Vertices, Geometry, Type );
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
opengl_renderer::Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset ) {
    gfx::calculate_tangent(Vertices, Type);

    return m_geometry.replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
opengl_renderer::Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type ) {
    gfx::calculate_tangent(Vertices, Type);

    return m_geometry.append( Vertices, Geometry );
}

// provides direct access to vertex data of specfied chunk
gfx::vertex_array const &
opengl_renderer::Vertices( gfx::geometry_handle const &Geometry ) const {

    return m_geometry.vertices( Geometry );
}

// material methods
material_handle
opengl_renderer::Fetch_Material( std::string const &Filename, bool const Loadnow ) {

    return m_materials.create( Filename, Loadnow );
}

std::shared_ptr<gl::program>
opengl_renderer::Fetch_Shader(const std::string &name)
{
    auto it = m_shaders.find(name);
    if (it == m_shaders.end())
    {
        gl::shader fragment("mat_" + name + ".frag");
        gl::program *program = new gl::program_mvp({ fragment, *m_vertex_shader.get() });
        program->init();
        m_shaders.insert({name, std::shared_ptr<gl::program>(program)});
    }

    return m_shaders[name];
}

void
opengl_renderer::Bind_Material( material_handle const Material ) {

    if (Material != null_handle)
    {
        auto &material = m_materials.material( Material );
        for (size_t i = 0; i < gl::MAX_PARAMS; i++)
            model_ubs.param[i] = material.params[i];
        model_ubs.opacity = material.opacity;
        material.shader->bind();

        size_t unit = 0;
        for (auto &tex : material.textures)
        {
            if (tex == null_handle)
                break;
            m_textures.bind(unit, tex);
            unit++;
        }
    }
    else if (Material != m_invalid_material)
        Bind_Material(m_invalid_material);
}

opengl_material const &
opengl_renderer::Material( material_handle const Material ) const {

    return m_materials.material( Material );
}

texture_handle
opengl_renderer::Fetch_Texture( std::string const &Filename, bool const Loadnow ) {

    return m_textures.create( Filename, Loadnow );
}

void
opengl_renderer::Bind_Texture(size_t Unit, texture_handle const Texture ) {

    m_textures.bind( Unit, Texture );
}

opengl_texture &
opengl_renderer::Texture( texture_handle const Texture ) const {

    return m_textures.texture( Texture );
}

void
opengl_renderer::Render( scene::basic_region *Region ) {

    m_sectionqueue.clear();
    m_cellqueue.clear();
    // build a list of region sections to render
    glm::vec3 const cameraposition { m_renderpass.camera.position() };
    auto const camerax = static_cast<int>( std::floor( cameraposition.x / scene::EU07_SECTIONSIZE + scene::EU07_REGIONSIDESECTIONCOUNT / 2 ) );
    auto const cameraz = static_cast<int>( std::floor( cameraposition.z / scene::EU07_SECTIONSIZE + scene::EU07_REGIONSIDESECTIONCOUNT / 2 ) );
    int const segmentcount = 2 * static_cast<int>( std::ceil( m_renderpass.draw_range * Global.fDistanceFactor / scene::EU07_SECTIONSIZE ) );
    int const originx = camerax - segmentcount / 2;
    int const originz = cameraz - segmentcount / 2;

    for( int row = originz; row <= originz + segmentcount; ++row ) {
        if( row < 0 ) { continue; }
        if( row >= scene::EU07_REGIONSIDESECTIONCOUNT ) { break; }
        for( int column = originx; column <= originx + segmentcount; ++column ) {
            if( column < 0 ) { continue; }
            if( column >= scene::EU07_REGIONSIDESECTIONCOUNT ) { break; }
            auto *section { Region->m_sections[ row * scene::EU07_REGIONSIDESECTIONCOUNT + column ] };
            if( ( section != nullptr )
             && ( m_renderpass.camera.visible( section->m_area ) ) ) {
                m_sectionqueue.emplace_back( section );
            }
        }
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {

            Update_Lights( simulation::Lights );

            Render( std::begin( m_sectionqueue ), std::end( m_sectionqueue ) );
            // draw queue is filled while rendering sections
            Render( std::begin( m_cellqueue ), std::end( m_cellqueue ) );
            break;
        }
        case rendermode::shadows:
        case rendermode::pickscenery:
        {
            // these render modes don't bother with lights
            Render( std::begin( m_sectionqueue ), std::end( m_sectionqueue ) );
            // they can also skip queue sorting, as they only deal with opaque geometry
            // NOTE: there's benefit from rendering front-to-back, but is it significant enough? TODO: investigate
            Render( std::begin( m_cellqueue ), std::end( m_cellqueue ) );
            break;
        }
        case rendermode::reflections:
        case rendermode::pickcontrols:
        default: {
            // no need to render anything ourside of the cab in control picking mode
            break;
        }
    }
}

void
opengl_renderer::Render( section_sequence::iterator First, section_sequence::iterator Last ) {

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections: {

            break;
        }
        case rendermode::shadows: {
            // for shadows it is better to cull front faces
            glCullFace(GL_FRONT);
            break; }
        case rendermode::pickscenery: {
            break;
        }
        default: {
            break; }
    }

    while( First != Last ) {

        auto *section = *First;
        section->create_geometry();

        // render shapes held by the section
        switch( m_renderpass.draw_mode ) {
            case rendermode::color:
            case rendermode::reflections:
            case rendermode::shadows:
            case rendermode::pickscenery: {
                if( false == section->m_shapes.empty() ) {
                    // since all shapes of the section share center point we can optimize out a few calls here
                    ::glPushMatrix();
                    auto const originoffset { section->m_area.center - m_renderpass.camera.position() };
                    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                    // render
                    for( auto const &shape : section->m_shapes ) { Render( shape, true ); }
                    // post-render cleanup
                    ::glPopMatrix();
                }
                break;
            }
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }

        // add the section's cells to the cell queue
        switch( m_renderpass.draw_mode ) {
            case rendermode::color:
            case rendermode::shadows:
            case rendermode::pickscenery: {
                for( auto &cell : section->m_cells ) {
                    if( ( true == cell.m_active )
                     && ( m_renderpass.camera.visible( cell.m_area ) ) ) {
                        // store visible cells with content as well as their current distance, for sorting later
                        m_cellqueue.emplace_back(
                            glm::length2( m_renderpass.camera.position() - cell.m_area.center ),
                            &cell );
                    }
                }
                break;
            }
            case rendermode::reflections:
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }
        // proceed to next section
        ++First;
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // restore standard face cull mode
            ::glCullFace( GL_BACK );
            break; }
        default: {
            break; }
    }
}

void
opengl_renderer::Render( cell_sequence::iterator First, cell_sequence::iterator Last ) {

    // cache initial iterator for the second sweep
    auto first { First };
    // first pass draws elements which we know are located in section banks, to reduce vbo switching
    while( First != Last ) {

        auto *cell = First->second;
        // przeliczenia animacji torów w sektorze
        cell->RaAnimate( m_framestamp );

        switch( m_renderpass.draw_mode ) {
            case rendermode::color: {
                // since all shapes of the section share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

                // render
                // opaque non-instanced shapes
                for( auto const &shape : cell->m_shapesopaque ) { Render( shape, false ); }
                // tracks
                // TODO: update after path node refactoring
                Render( std::begin( cell->m_paths ), std::end( cell->m_paths ) );
                // post-render cleanup
                ::glPopMatrix();

                break;
            }
            case rendermode::shadows:
            {
                // since all shapes of the section share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

                // render
                // opaque non-instanced shapes
                for( auto const &shape : cell->m_shapesopaque )
                    Render( shape, false );
                // tracks
                Render( std::begin( cell->m_paths ), std::end( cell->m_paths ) );

                // post-render cleanup
                ::glPopMatrix();

                break;
            }
            case rendermode::pickscenery:
            case rendermode::reflections:
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }

        ++First;
    }
    // second pass draws elements with their own vbos
    while( first != Last ) {

        auto const *cell = first->second;

        switch( m_renderpass.draw_mode ) {
            case rendermode::color:
            case rendermode::shadows: {
                // opaque parts of instanced models
                for( auto *instance : cell->m_instancesopaque ) { Render( instance ); }
                // opaque parts of vehicles
                for( auto *path : cell->m_paths ) {
                    for( auto *dynamic : path->Dynamics ) {
                        Render( dynamic );
                    }
                }
                break;
            }
            case rendermode::pickscenery:
            case rendermode::reflections:
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }

        ++first;
    }
}

void
opengl_renderer::Render( scene::shape_node const &Shape, bool const Ignorerange ) {

    auto const &data { Shape.data() };

    if( false == Ignorerange ) {
        double distancesquared;
        switch( m_renderpass.draw_mode ) {
            case rendermode::shadows: {
                // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
                distancesquared = Math3D::SquareMagnitude( ( data.area.center - Global.pCameraPosition ) / Global.ZoomFactor ) / Global.fDistanceFactor;
                break;
            }
            default: {
                distancesquared = glm::length2( ( data.area.center - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
                break;
            }
        }
        if( ( distancesquared <  data.rangesquared_min )
         || ( distancesquared >= data.rangesquared_max ) ) {
            return;
        }
    }

    // setup
    Bind_Material( data.material );
    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections:
        // pick modes are painted with custom colours, and shadow pass doesn't use any
        case rendermode::shadows:
        case rendermode::pickscenery:
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }
    // render
    model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
    model_ubo->update(model_ubs);

    m_geometry.draw( data.geometry );
    // debug data
    ++m_debugstats.shapes;
    ++m_debugstats.drawcalls;
}

void
opengl_renderer::Render( TAnimModel *Instance ) {

    if( false == Instance->m_visible ) {
        return;
    }

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = Math3D::SquareMagnitude( ( Instance->location() - Global.pCameraPosition ) / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
        default: {
            distancesquared = Math3D::SquareMagnitude( ( Instance->location() - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  Instance->m_rangesquaredmin )
     || ( distancesquared >= Instance->m_rangesquaredmax ) ) {
        return;
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::pickscenery: {
            break;
        }
        default: {
            break;
        }
    }

    Instance->RaAnimate( m_framestamp ); // jednorazowe przeliczenie animacji
    Instance->RaPrepare();
    if( Instance->pModel ) {
        // renderowanie rekurencyjne submodeli
        Render(
            Instance->pModel,
            Instance->Material(),
            distancesquared,
            Instance->location() - m_renderpass.camera.position(),
            Instance->vAngle );
    }
}

bool
opengl_renderer::Render( TDynamicObject *Dynamic ) {
    glDebug("Render TDynamicObject");

    Dynamic->renderme = m_renderpass.camera.visible( Dynamic );
    if( false == Dynamic->renderme ) {
        return false;
    }
    // debug data
    ++m_debugstats.dynamics;

    // setup
    TSubModel::iInstance = ( size_t )Dynamic; //żeby nie robić cudzych animacji
    glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    // lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
    float squaredistance;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        {
            squaredistance = glm::length2( glm::vec3{ glm::dvec3{ Dynamic->vPosition - Global.pCameraPosition } } / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
        default: {
            squaredistance = glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu
    ::glPushMatrix();

    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
    ::glMultMatrixd( Dynamic->mMatrix.getArray() );

    switch( m_renderpass.draw_mode ) {

        case rendermode::color: {
            if( Dynamic->fShade > 0.0f ) {
                // change light level based on light level of the occupied track
                m_sunlight.apply_intensity( Dynamic->fShade );
            }
            m_renderspecular = true; // vehicles are rendered with specular component. static models without, at least for the time being
            // render
            if( Dynamic->mdLowPolyInt ) {
                // low poly interior
                if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
                    // enable cab light if needed
                    if( Dynamic->InteriorLightLevel > 0.0f ) {

                        // crude way to light the cabin, until we have something more complete in place
                        //m7t set cabin ambient
                    }

                    Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );

                    if( Dynamic->InteriorLightLevel > 0.0f ) {
                        // reset the overall ambient
                        //m7t set ambient
                    }
                }
            }

            if( Dynamic->mdModel )
                Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );

            if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
                Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance );

            // post-render cleanup
            m_renderspecular = false;
            if( Dynamic->fShade > 0.0f ) {
                // restore regular light level
                m_sunlight.apply_intensity();
            }
            break;
        }
        case rendermode::shadows:
        {
            if( Dynamic->mdLowPolyInt ) {
                // low poly interior
                if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
                    Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
                }
            }
            if( Dynamic->mdModel )
                Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );
            if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
                Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance );
            // post-render cleanup
            break;
        }
        case rendermode::pickcontrols:
        case rendermode::pickscenery:
        default: {
            break;
        }
    }

    ::glPopMatrix();

    // TODO: check if this reset is needed. In theory each object should render all parts based on its own instance data anyway?
    if( Dynamic->btnOn )
        Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli

    return true;
}

// rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
bool
opengl_renderer::Render_cab( TDynamicObject const *Dynamic, bool const Alpha ) {

    if( Dynamic == nullptr ) {

        TSubModel::iInstance = 0;
        return false;
    }

    TSubModel::iInstance = reinterpret_cast<std::size_t>( Dynamic );

    if( ( true == FreeFlyModeFlag )
     || ( false == Dynamic->bDisplayCab )
     || ( Dynamic->mdKabina == Dynamic->mdModel ) ) {
        // ABu: Rendering kabiny jako ostatniej, zeby bylo widac przez szyby, tylko w widoku ze srodka
        return false;
    }

    if( Dynamic->mdKabina ) { // bo mogła zniknąć przy przechodzeniu do innego pojazdu
        // setup shared by all render paths
        ::glPushMatrix();

        auto const originoffset = Dynamic->GetPosition() - m_renderpass.camera.position();
        ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
        ::glMultMatrixd( Dynamic->mMatrix.readArray() );

        switch( m_renderpass.draw_mode ) {
            case rendermode::color: {
                // render path specific setup:
                if( Dynamic->fShade > 0.0f ) {
                    // change light level based on light level of the occupied track
                    m_sunlight.apply_intensity( Dynamic->fShade );
                }
                if( Dynamic->InteriorLightLevel > 0.f ) {
                    // crude way to light the cabin, until we have something more complete in place

                }
                // render
                if( true == Alpha ) {
                    // translucent parts
                    Render_Alpha( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                }
                else {
                    // opaque parts
                    Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                }
                // post-render restore
                if( Dynamic->fShade > 0.0f ) {
                    // change light level based on light level of the occupied track
                    m_sunlight.apply_intensity();
                }
                if( Dynamic->InteriorLightLevel > 0.0f ) {
                    // reset the overall ambient
                    //m7t set ambient
                }
                break;
            }
            case rendermode::cabshadows:
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }
        // post-render restore
        ::glPopMatrix();
    }

    return true;
}

bool
opengl_renderer::Render( TModel3d *Model, material_data const *Material, float const Squaredistance ) {

    auto alpha =
        ( Material != nullptr ?
            Material->textures_alpha :
            0x30300030 );
    alpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
    if( 0 == ( alpha & Model->iFlags & 0x1F1F001F ) ) {
        // czy w ogóle jest co robić w tym cyklu?
        return false;
    }

    Model->Root->fSquareDist = Squaredistance; // zmienna globalna!

    // setup
    Model->Root->ReplacableSet(
        ( Material != nullptr ?
            Material->replacable_skins :
            nullptr ),
        alpha );

    Model->Root->pRoot = Model;

    // render
    Render( Model->Root );

    // debug data
    ++m_debugstats.models;

    // post-render cleanup

    return true;
}

bool
opengl_renderer::Render( TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotated( Angle.y, 0.0, 1.0, 0.0 );
    if( Angle.x != 0.0 )
        ::glRotated( Angle.x, 1.0, 0.0, 0.0 );
    if( Angle.z != 0.0 )
        ::glRotated( Angle.z, 0.0, 0.0, 1.0 );

    auto const result = Render( Model, Material, Squaredistance );

    ::glPopMatrix();

    return result;
}

void
opengl_renderer::Render( TSubModel *Submodel ) {
    glDebug("Render TSubModel");

    if( ( Submodel->iVisible )
     && ( TSubModel::fSquareDist >= Submodel->fSquareMinDist )
     && ( TSubModel::fSquareDist <  Submodel->fSquareMaxDist ) ) {

        // debug data
        ++m_debugstats.submodels;
        ++m_debugstats.drawcalls;

        if( Submodel->iFlags & 0xC000 ) {
            ::glPushMatrix();
            if( Submodel->fMatrix )
                ::glMultMatrixf( Submodel->fMatrix->readArray() );
            if( Submodel->b_Anim )
                Submodel->RaAnimation( Submodel->b_Anim );
        }

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x1F ) {
                // rysuj gdy element nieprzezroczysty
                switch( m_renderpass.draw_mode ) {
                    case rendermode::color:
                    case rendermode::reflections: {
// NOTE: code disabled as normalization marking doesn't take into account scaling propagation down hierarchy chains
// for the time being we'll do with enforced worst-case scaling method, when speculars are enabled
#ifdef EU07_USE_OPTIMIZED_NORMALIZATION
                        switch( Submodel->m_normalizenormals ) {
                            case TSubModel::normalize: {
                                ::glEnable( GL_NORMALIZE ); break; }
                            case TSubModel::rescale: {
                                ::glEnable( GL_RESCALE_NORMAL ); break; }
                            default: {
                                break; }
                        }
#else
#endif
                        // material configuration:
                        // textures...
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            // również 0
                            Bind_Material( Submodel->m_material );
                        }

                        //m7t set material

                        // ...colors...
                        if( ( true == m_renderspecular ) && ( m_sunlight.specular.a > 0.01f ) ) {
                            // specular strength in legacy models is set uniformly to 150, 150, 150 so we scale it down for opaque elements

                        }

                        model_ubs.emission = 0;
                        // ...luminance
                        if( Global.fLuminance < Submodel->fLight ) {
                            model_ubs.emission = Submodel->f4Emision.a;
                        }

                        // main draw call
                        model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
                        model_ubo->update(model_ubs);

                        m_geometry.draw( Submodel->m_geometry );


#ifdef EU07_USE_OPTIMIZED_NORMALIZATION
                        switch( Submodel->m_normalizenormals ) {
                            case TSubModel::normalize: {
                                ::glDisable( GL_NORMALIZE ); break; }
                            case TSubModel::rescale: {
                                ::glDisable( GL_RESCALE_NORMAL ); break; }
                            default: {
                                break; }
                        }
#else
#endif
                        break;
                    }
                    case rendermode::shadows:
                    {
                        m_shadow_shader->bind();
                        model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
                        model_ubo->update(model_ubs);
                        m_geometry.draw( Submodel->m_geometry );
                        break;
                    }
                    case rendermode::cabshadows:
                    case rendermode::pickscenery:
                    case rendermode::pickcontrols:
                    default: {
                        break;
                    }
                }
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            switch( m_renderpass.draw_mode ) {
                // spotlights are only rendered in colour mode(s)
                case rendermode::color:
                case rendermode::reflections: {
                    auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
                    auto const lightcenter =
                        modelview
                        * interpolate(
                            glm::vec4( 0.f, 0.f, -0.05f, 1.f ),
                            glm::vec4( 0.f, 0.f, -0.25f, 1.f ),
                            static_cast<float>( TSubModel::fSquareDist / Submodel->fSquareMaxDist ) ); // pozycja punktu świecącego względem kamery
                    Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.f, 0.f, -1.f, 1.f ) - lightcenter ), glm::normalize( -lightcenter ) );

                    if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) {
                        // kąt większy niż maksymalny stożek swiatła
                        float lightlevel = 1.f; // TODO, TBD: parameter to control light strength
                        // view angle attenuation
                        float const anglefactor = clamp(
                            ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( Submodel->fCosHotspotAngle - Submodel->fCosFalloffAngle ),
                            0.f, 1.f );
                        // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                        // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                        float const distancefactor = std::max( 0.5f, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / Submodel->fSquareMaxDist );

                        if( lightlevel > 0.f ) {
                            // material configuration:

                            ::glEnable( GL_BLEND );

                            ::glPushMatrix();
                            ::glLoadIdentity();
                            ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
/*
                            setup_shadow_color( colors::white );
*/
                            glPointSize( std::max( 3.f, 5.f * distancefactor * anglefactor ) * 2.0f );

                            // main draw call
                            model_ubs.param[0] = glm::vec4(glm::vec3(Submodel->f4Diffuse), 0.0f);
                            model_ubs.emission = lightlevel * anglefactor;
                            model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
                            model_ubo->update(model_ubs);
                            m_freespot_shader->bind();

                            m_geometry.draw( Submodel->m_geometry );

                            // post-draw reset
                            // re-enable shadows
/*
                            setup_shadow_color( m_shadowcolor );
*/

                            ::glPopMatrix();
                        }
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else if( Submodel->eType == TP_STARS ) {

            switch( m_renderpass.draw_mode ) {
                // colour points are only rendered in colour mode(s)
                case rendermode::color:
                case rendermode::reflections: {
                    if( Global.fLuminance < Submodel->fLight ) {

                        Bind_Material( null_handle );

                        // main draw call
                        model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
                        model_ubo->update(model_ubs);

                        //m_geometry.draw( Submodel->m_geometry, gfx::color_streams );
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }
        if( Submodel->Child != nullptr )
            if( Submodel->iAlpha & Submodel->iFlags & 0x001F0000 )
                Render( Submodel->Child );

        if( Submodel->iFlags & 0xC000 )
            ::glPopMatrix();
    }
/*
    if( Submodel->b_Anim < at_SecondsJump )
        Submodel->b_Anim = at_None; // wyłączenie animacji dla kolejnego użycia subm
*/
    if( Submodel->Next )
        if( Submodel->iAlpha & Submodel->iFlags & 0x1F000000 )
            Render( Submodel->Next ); // dalsze rekurencyjnie
}

void
opengl_renderer::Render( TTrack *Track ) {

    if( ( Track->m_material1 == 0 )
     && ( Track->m_material2 == 0 ) ) {
        return;
    }
    if( false == Track->m_visible ) {
        return;
    }

    ++m_debugstats.paths;
    ++m_debugstats.drawcalls;

    model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
    model_ubo->update(model_ubs);

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections: {
            setup_environment_light( Track->eEnvironment );
            if( Track->m_material1 != 0 ) {
                Bind_Material( Track->m_material1 );
                m_geometry.draw( std::begin( Track->Geometry1 ), std::end( Track->Geometry1 ) );
            }
            if( Track->m_material2 != 0 ) {
                Bind_Material( Track->m_material2 );
                m_geometry.draw( std::begin( Track->Geometry2 ), std::end( Track->Geometry2 ) );
            }
            setup_environment_light();
            break;
        }
        case rendermode::shadows: {
            // shadow pass includes trackbeds but not tracks themselves due to low resolution of the map
            // TODO: implement
            break;
        }
        case rendermode::pickscenery: {
            break;
        }
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }
}

// experimental, does track rendering in two passes, to take advantage of reduced texture switching
void
opengl_renderer::Render( scene::basic_cell::path_sequence::const_iterator First, scene::basic_cell::path_sequence::const_iterator Last ) {

    // setup
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // NOTE: roads-based platforms tend to miss parts of shadows if rendered with either back or front culling
            break;
        }
        default: {
            break;
        }
    }

    model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
    model_ubo->update(model_ubs);

    // first pass, material 1
    for( auto first { First }; first != Last; ++first ) {

        auto const track { *first };

        if( track->m_material1 == 0 )  {
            continue;
        }
        if( false == track->m_visible ) {
            continue;
        }

        ++m_debugstats.paths;
        ++m_debugstats.drawcalls;

        switch( m_renderpass.draw_mode ) {
            case rendermode::color:
            case rendermode::reflections: {
                if( track->eEnvironment != e_flat ) {
                    setup_environment_light( track->eEnvironment );
                }
                Bind_Material( track->m_material1 );
                m_geometry.draw( std::begin( track->Geometry1 ), std::end( track->Geometry1 ) );
                if( track->eEnvironment != e_flat ) {
                    // restore default lighting
                    setup_environment_light();
                }
                break;
            }
            case rendermode::shadows: {
                if( ( std::abs( track->fTexHeight1 ) < 0.35f )
                 || ( track->iCategoryFlag != 2 ) ) {
                    // shadows are only calculated for high enough roads, typically meaning track platforms
                    continue;
                }
                m_shadow_shader->bind();
                m_geometry.draw( std::begin( track->Geometry1 ), std::end( track->Geometry1 ) );
                break;
            }
            case rendermode::pickscenery: // pick scenery mode uses piece-by-piece approach
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }
    }
    // second pass, material 2
    for( auto first { First }; first != Last; ++first ) {

        auto const track { *first };

        if( track->m_material2 == 0 ) {
            continue;
        }
        if( false == track->m_visible ) {
            continue;
        }

        switch( m_renderpass.draw_mode ) {
            case rendermode::color:
            case rendermode::reflections: {
                if( track->eEnvironment != e_flat ) {
                    setup_environment_light( track->eEnvironment );
                }
                Bind_Material( track->m_material2 );
                m_geometry.draw( std::begin( track->Geometry2 ), std::end( track->Geometry2 ) );
                if( track->eEnvironment != e_flat ) {
                    // restore default lighting
                    setup_environment_light();
                }
                break;
            }
            case rendermode::shadows: {
                if( ( std::abs( track->fTexHeight1 ) < 0.35f )
                 || ( ( track->iCategoryFlag == 1 )
                   && ( track->eType != tt_Normal ) ) ) {
                    // shadows are only calculated for high enough trackbeds
                    continue;
                }
                m_shadow_shader->bind();
                m_geometry.draw( std::begin( track->Geometry2 ), std::end( track->Geometry2 ) );
                break;
            }
            case rendermode::pickscenery: // pick scenery mode uses piece-by-piece approach
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }
    }
    // post-render reset
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // restore standard face cull mode
            ::glEnable( GL_CULL_FACE );
            break;
        }
        default: {
            break;
        }
    }
}

void
opengl_renderer::Render( TMemCell *Memcell ) {

    ::glPushMatrix();
    auto const position = Memcell->location() - m_renderpass.camera.position();
    ::glTranslated( position.x, position.y + 0.5, position.z );

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {
            break;
        }
        case rendermode::shadows:
        case rendermode::pickscenery: {
            break;
        }
        case rendermode::reflections:
        case rendermode::pickcontrols: {
            break;
        }
        default: {
            break;
        }
    }

    ::glPopMatrix();
}

void
opengl_renderer::Render_Alpha( scene::basic_region *Region ) {

    // sort the nodes based on their distance to viewer
    std::sort(
        std::begin( m_cellqueue ),
        std::end( m_cellqueue ),
        []( distancecell_pair const &Left, distancecell_pair const &Right ) {
            return ( Left.first ) < ( Right.first ); } );

    Render_Alpha( std::rbegin( m_cellqueue ), std::rend( m_cellqueue ) );
}

void
opengl_renderer::Render_Alpha( cell_sequence::reverse_iterator First, cell_sequence::reverse_iterator Last ) {

    // NOTE: this method is launched only during color pass therefore we don't bother with mode test here
    // first pass draws elements which we know are located in section banks, to reduce vbo switching
    {
        auto first { First };
        while( first != Last ) {

            auto const *cell = first->second;

            if( false == cell->m_shapestranslucent.empty() ) {
                // since all shapes of the cell share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset{ cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                // render
                // NOTE: we can reuse the method used to draw opaque geometry
                for( auto const &shape : cell->m_shapestranslucent ) { Render( shape, false ); }
                // post-render cleanup
                ::glPopMatrix();
            }

            ++first;
        }
    }
    // second pass draws elements with their own vbos
    {
        auto first { First };
        while( first != Last ) {

            auto const *cell = first->second;

            // translucent parts of instanced models
            for( auto *instance : cell->m_instancetranslucent ) { Render_Alpha( instance ); }
            // translucent parts of vehicles
            for( auto *path : cell->m_paths ) {
                for( auto *dynamic : path->Dynamics ) {
                    Render_Alpha( dynamic );
                }
            }

            ++first;
        }
    }
    // third pass draws the wires;
    // wires use section vbos, but for the time being we want to draw them at the very end
    {
        auto first{ First };
        while( first != Last ) {

            auto const *cell = first->second;

            if( ( false == cell->m_traction.empty()
             || ( false == cell->m_lines.empty() ) ) ) {
                // since all shapes of the cell share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                Bind_Material( null_handle );
                // render
                for( auto *traction : cell->m_traction ) { Render_Alpha( traction ); }
                for( auto &lines : cell->m_lines )       { Render_Alpha( lines ); }
                // post-render cleanup
                ::glPopMatrix();
            }

            ++first;
        }
    }
}

void
opengl_renderer::Render_Alpha( TAnimModel *Instance ) {

    if( false == Instance->m_visible ) {
        return;
    }

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        default: {
            distancesquared = glm::length2( ( Instance->location() - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  Instance->m_rangesquaredmin )
     || ( distancesquared >= Instance->m_rangesquaredmax ) ) {
        return;
    }

    Instance->RaPrepare();
    if( Instance->pModel ) {
        // renderowanie rekurencyjne submodeli
        Render_Alpha(
            Instance->pModel,
            Instance->Material(),
            distancesquared,
            Instance->location() - m_renderpass.camera.position(),
            Instance->vAngle );
    }
}

void
opengl_renderer::Render_Alpha( TTraction *Traction ) {
    glDebug("Render_Alpha TTraction");

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        default: {
            distancesquared = glm::length2( ( Traction->location() - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  Traction->m_rangesquaredmin )
     || ( distancesquared >= Traction->m_rangesquaredmax ) ) {
        return;
    }

    if( false == Traction->m_visible ) {
        return;
    }
    // rysuj jesli sa druty i nie zerwana
    if( ( Traction->Wires == 0 )
     || ( true == TestFlag( Traction->DamageFlag, 128 ) ) ) {
        return;
    }
    // setup
    auto const distance { static_cast<float>( std::sqrt( distancesquared ) ) };
    auto const linealpha {
        20.f * Traction->WireThickness
        / std::max(
            0.5f * Traction->radius() + 1.f,
            distance - ( 0.5f * Traction->radius() ) ) };
    if (m_widelines_supported)
        glLineWidth(clamp(
                0.5f * linealpha + Traction->WireThickness * Traction->radius() / 1000.f,
                1.f, 1.5f ) );
    // McZapkie-261102: kolor zalezy od materialu i zasniedzenia
    // render

    model_ubs.param[0] = glm::vec4(Traction->wire_color(), linealpha);
    model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
    model_ubo->update(model_ubs);
    m_line_shader->bind();

    m_geometry.draw(Traction->m_geometry);

    // debug data
    ++m_debugstats.traction;
    ++m_debugstats.drawcalls;
}

void
opengl_renderer::Render_Alpha( scene::lines_node const &Lines ) {
    glDebug("Render_Alpha scene::lines_node");

    auto const &data { Lines.data() };

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        default: {
            distancesquared = glm::length2( ( data.area.center - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  data.rangesquared_min )
     || ( distancesquared >= data.rangesquared_max ) ) {
        return;
    }
    // setup
    auto const distance { static_cast<float>( std::sqrt( distancesquared ) ) };
    auto const linealpha = (
        data.line_width > 0.f ?
            10.f * data.line_width
            / std::max(
                0.5f * data.area.radius + 1.f,
                distance - ( 0.5f * data.area.radius ) ) :
            1.f ); // negative width means the lines are always opague
    if (m_widelines_supported)
        glLineWidth(clamp(
                0.5f * linealpha + data.line_width * data.area.radius / 1000.f,
                1.f, 8.f ) );

    model_ubs.param[0] = glm::vec4(glm::vec3(data.lighting.diffuse * m_sunlight.ambient), linealpha);
    model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
    model_ubo->update(model_ubs);
    m_line_shader->bind();

    m_geometry.draw( data.geometry);

    ++m_debugstats.lines;
    ++m_debugstats.drawcalls;
}

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) { return false; }

    // setup
    TSubModel::iInstance = ( size_t )Dynamic; //żeby nie robić cudzych animacji
    glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    // lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
    float squaredistance;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        default: {
            squaredistance = glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu
    ::glPushMatrix();

    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
    ::glMultMatrixd( Dynamic->mMatrix.getArray() );

    if( Dynamic->fShade > 0.0f ) {
        // change light level based on light level of the occupied track
        m_sunlight.apply_intensity( Dynamic->fShade );
    }
    m_renderspecular = true;

    // render
    if( Dynamic->mdLowPolyInt ) {
        // low poly interior
        if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
            // enable cab light if needed
            if( Dynamic->InteriorLightLevel > 0.0f ) {

                // crude way to light the cabin, until we have something more complete in place
                // m7t
            }

            Render_Alpha( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
        }
    }

    if( Dynamic->mdModel )
        Render_Alpha( Dynamic->mdModel, Dynamic->Material(), squaredistance );

    if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
        Render_Alpha( Dynamic->mdLoad, Dynamic->Material(), squaredistance );

    // post-render cleanup
    m_renderspecular = false;
    if( Dynamic->fShade > 0.0f ) {
        // restore regular light level
        m_sunlight.apply_intensity();
    }

    ::glPopMatrix();

    if( Dynamic->btnOn )
        Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli

    return true;
}

bool
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, float const Squaredistance ) {

    auto alpha =
        ( Material != nullptr ?
            Material->textures_alpha :
            0x30300030 );

    if( 0 == ( alpha & Model->iFlags & 0x2F2F002F ) ) {
        // nothing to render
        return false;
    }

    Model->Root->fSquareDist = Squaredistance; // zmienna globalna!

    // setup
    Model->Root->ReplacableSet(
        ( Material != nullptr ?
            Material->replacable_skins :
            nullptr ),
        alpha );

    Model->Root->pRoot = Model;

    // render
    Render_Alpha( Model->Root );

    // post-render cleanup

    return true;
}

bool
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotated( Angle.y, 0.0, 1.0, 0.0 );
    if( Angle.x != 0.0 )
        ::glRotated( Angle.x, 1.0, 0.0, 0.0 );
    if( Angle.z != 0.0 )
        ::glRotated( Angle.z, 0.0, 0.0, 1.0 );

    auto const result = Render_Alpha( Model, Material, Squaredistance ); // position is effectively camera offset

    ::glPopMatrix();

    return result;
}

void
opengl_renderer::Render_Alpha( TSubModel *Submodel ) {
    // renderowanie przezroczystych przez DL
    if( ( Submodel->iVisible )
     && ( TSubModel::fSquareDist >= Submodel->fSquareMinDist )
     && ( TSubModel::fSquareDist <  Submodel->fSquareMaxDist ) ) {

        // debug data
        ++m_debugstats.submodels;
        ++m_debugstats.drawcalls;

        if( Submodel->iFlags & 0xC000 ) {
            ::glPushMatrix();
            if( Submodel->fMatrix )
                ::glMultMatrixf( Submodel->fMatrix->readArray() );
            if( Submodel->b_aAnim )
                Submodel->RaAnimation( Submodel->b_aAnim );
        }

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x2F ) {
                // rysuj gdy element przezroczysty
                switch( m_renderpass.draw_mode ) {
                    case rendermode::color: {

// NOTE: code disabled as normalization marking doesn't take into account scaling propagation down hierarchy chains
// for the time being we'll do with enforced worst-case scaling method, when speculars are enabled
#ifdef EU07_USE_OPTIMIZED_NORMALIZATION
                        switch( Submodel->m_normalizenormals ) {
                            case TSubModel::normalize: {
                                ::glEnable( GL_NORMALIZE ); break; }
                            case TSubModel::rescale: {
                                ::glEnable( GL_RESCALE_NORMAL ); break; }
                            default: {
                                break; }
                        }
#else
#endif
                        // material configuration:
                        // textures...
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            Bind_Material( Submodel->m_material );
                        }
                        // ...colors...
                        // m7t set material
                        // ...luminance
                        if( Global.fLuminance < Submodel->fLight ) {
                        }

                        // main draw call
                        model_ubs.set_modelview(OpenGLMatrices.data(GL_MODELVIEW));
                        model_ubo->update(model_ubs);
                        m_geometry.draw( Submodel->m_geometry );

#ifdef EU07_USE_OPTIMIZED_NORMALIZATION
                        switch( Submodel->m_normalizenormals ) {
                            case TSubModel::normalize: {
                                ::glDisable( GL_NORMALIZE ); break; }
                            case TSubModel::rescale: {
                                ::glDisable( GL_RESCALE_NORMAL ); break; }
                            default: {
                                break; }
                        }
#else
#endif
                        break;
                    }
                    case rendermode::cabshadows: {
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            if( Global.fLuminance < Submodel->fLight ) {
                // NOTE: we're forced here to redo view angle calculations etc, because this data isn't instanced but stored along with the single mesh
                // TODO: separate instance data from reusable geometry
                auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
                auto const lightcenter =
                    modelview
                    * interpolate(
                        glm::vec4( 0.f, 0.f, -0.05f, 1.f ),
                        glm::vec4( 0.f, 0.f, -0.10f, 1.f ),
                        static_cast<float>( TSubModel::fSquareDist / Submodel->fSquareMaxDist ) ); // pozycja punktu świecącego względem kamery
                Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.f, 0.f, -1.f, 1.f ) - lightcenter ), glm::normalize( -lightcenter ) );

                float glarelevel = 0.6f; // luminosity at night is at level of ~0.1, so the overall resulting transparency in clear conditions is ~0.5 at full 'brightness'
                if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) {
                    // only bother if the viewer is inside the visibility cone
                    if( Global.Overcast > 1.0 ) {
                        // increase the glare in rainy/foggy conditions
                        glarelevel += std::max( 0.f, 0.5f * ( Global.Overcast - 1.f ) );
                    }
                    // scale it down based on view angle
                    glarelevel *= ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                    // reduce the glare in bright daylight
                    glarelevel = clamp( glarelevel - static_cast<float>(Global.fLuminance), 0.f, 1.f );

                    /*
                    if( glarelevel > 0.0f ) {
                        Bind_Texture( m_glaretexture );
                        ::glDepthMask( GL_FALSE );
                        ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                        ::glPushMatrix();
                        ::glLoadIdentity(); // macierz jedynkowa
                        ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
                        ::glRotated( std::atan2( lightcenter.x, lightcenter.z ) * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // jedynie obracamy w pionie o kąt
                                                                                                                 // disable shadows so they don't obstruct self-lit items
                        auto const unitstate = m_unitstate;
                        switch_units( unitstate.diffuse, false, false );

                        // main draw call
                        model_ubs.set(OpenGLMatrices.data(GL_MODELVIEW));
                        model_ubo->update(model_ubs);
                        m_geometry.draw( m_billboardgeometry );

                        ::glPopMatrix();
                    }
                    */
                }
            }
        }

        if( Submodel->Child != nullptr ) {
            if( Submodel->eType == TP_TEXT ) { // tekst renderujemy w specjalny sposób, zamiast submodeli z łańcucha Child
                int i, j = (int)Submodel->pasText->size();
                TSubModel *p;
                if( !Submodel->smLetter ) { // jeśli nie ma tablicy, to ją stworzyć; miejsce nieodpowiednie, ale tymczasowo może być
                    Submodel->smLetter = new TSubModel *[ 256 ]; // tablica wskaźników submodeli dla wyświetlania tekstu
                    memset( Submodel->smLetter, 0, 256 * sizeof( TSubModel * ) ); // wypełnianie zerami
                    p = Submodel->Child;
                    while( p ) {
                        Submodel->smLetter[ p->pName[ 0 ] ] = p;
                        p = p->Next; // kolejny znak
                    }
                }
                for( i = 1; i <= j; ++i ) {
                    p = Submodel->smLetter[ ( *( Submodel->pasText) )[ i ] ]; // znak do wyświetlenia
                    if( p ) { // na razie tylko jako przezroczyste
                        Render_Alpha( p );
                        if( p->fMatrix )
                            ::glMultMatrixf( p->fMatrix->readArray() ); // przesuwanie widoku
                    }
                }
            }
            else if( Submodel->iAlpha & Submodel->iFlags & 0x002F0000 )
                Render_Alpha( Submodel->Child );
        }

        if( Submodel->iFlags & 0xC000 )
            ::glPopMatrix();
    }
/*
    if( Submodel->b_aAnim < at_SecondsJump )
        Submodel->b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu
*/
    if( Submodel->Next != nullptr )
        if( Submodel->iAlpha & Submodel->iFlags & 0x2F000000 )
            Render_Alpha( Submodel->Next );
};



// utility methods
TSubModel const *
opengl_renderer::Update_Pick_Control() {
    return nullptr;
}

scene::basic_node const *
opengl_renderer::Update_Pick_Node() {
    return nullptr;
}

void
opengl_renderer::Update( double const Deltatime ) {

    m_updateaccumulator += Deltatime;

    if( m_updateaccumulator < 1.0 ) {
        // too early for any work
        return;
    }

    m_updateaccumulator = 0.0;
    m_framerate = 1000.f / ( Timer::subsystem.gfx_total.average() );

    // adjust draw ranges etc, based on recent performance
    auto const framerate = 1000.f / Timer::subsystem.gfx_color.average();

    float targetfactor;
         if( framerate > 90.0 ) { targetfactor = 3.0f; }
    else if( framerate > 60.0 ) { targetfactor = 1.5f; }
    else if( framerate > 30.0 ) { targetfactor = 1.25; }
    else                        { targetfactor = std::max( Global.iWindowHeight / 768.f, 1.f ); }

    if( targetfactor > Global.fDistanceFactor ) {

        Global.fDistanceFactor = std::min( targetfactor, Global.fDistanceFactor + 0.05f );
    }
    else if( targetfactor < Global.fDistanceFactor ) {

        Global.fDistanceFactor = std::max( targetfactor, Global.fDistanceFactor - 0.05f );
    }

    if( ( framerate < 15.0 ) && ( Global.iSlowMotion < 7 ) ) {
        Global.iSlowMotion = ( Global.iSlowMotion << 1 ) + 1; // zapalenie kolejnego bitu
        if( Global.iSlowMotionMask & 1 )
            if( Global.iMultisampling ) // a multisampling jest włączony
                ::glDisable( GL_MULTISAMPLE ); // wyłączenie multisamplingu powinno poprawić FPS
    }
    else if( ( framerate > 20.0 ) && Global.iSlowMotion ) { // FPS się zwiększył, można włączyć bajery
        Global.iSlowMotion = ( Global.iSlowMotion >> 1 ); // zgaszenie bitu
        if( Global.iSlowMotion == 0 ) // jeśli jest pełna prędkość
            if( Global.iMultisampling ) // a multisampling jest włączony
                ::glEnable( GL_MULTISAMPLE );
    }

    if( ( true == Global.ResourceSweep )
     && ( true == World.InitPerformed() ) ) {
        // garbage collection
        m_geometry.update();
        m_textures.update();
    }

    if( true == DebugModeFlag ) {
        m_debugtimestext += m_textures.info();
    }

    if( ( true  == Global.ControlPicking )
     && ( false == FreeFlyModeFlag ) ) {
        Update_Pick_Control();
    }
    else {
        m_pickcontrolitem = nullptr;
    }
    // temporary conditions for testing. eventually will be coupled with editor mode
    if( ( true == Global.ControlPicking )
     && ( true == DebugModeFlag ) 
     && ( true == FreeFlyModeFlag ) ) {
        Update_Pick_Node();
    }
    else {
        m_picksceneryitem = nullptr;
    }
    // dump last opengl error, if any
    auto const glerror = ::glGetError();
    if( glerror != GL_NO_ERROR ) {
        std::string glerrorstring( ( char * )::gluErrorString( glerror ) );
        win1250_to_ascii( glerrorstring );
        Global.LastGLError = std::to_string( glerror ) + " (" + glerrorstring + ")";
    }
}

// debug performance string
std::string const &
opengl_renderer::info_times() const {

    return m_debugtimestext;
}

std::string const &
opengl_renderer::info_stats() const {

    return m_debugstatstext;
}

void
opengl_renderer::Update_Lights( light_array &Lights ) {
    glDebug("Update_Lights");

    // arrange the light array from closest to farthest from current position of the camera
    auto const camera = m_renderpass.camera.position();
    std::sort(
        std::begin( Lights.data ),
        std::end( Lights.data ),
        [&camera]( light_array::light_record const &Left, light_array::light_record const &Right ) {
            // move lights which are off at the end...
            if( Left.intensity  == 0.f ) { return false; }
            if( Right.intensity == 0.f ) { return true; }
            // ...otherwise prefer closer and/or brigher light sources
            return ( glm::length2( camera - Left.position ) * ( 1.f - Left.intensity ) ) < ( glm::length2( camera - Right.position ) * ( 1.f - Right.intensity ) ); } );

    size_t const count = std::min( m_lights.size(), Lights.data.size() );
    if( count == 0 ) { return; }

    auto renderlight = m_lights.begin();
    size_t light_i = 1;

    glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);

    for( auto const &scenelight : Lights.data ) {

        if( renderlight == m_lights.end() ) {
            // we ran out of lights to assign
            break;
        }
        if( scenelight.intensity == 0.f ) {
            // all lights past this one are bound to be off
            break;
        }
        auto const lightoffset = glm::vec3{ scenelight.position - camera };
        if( glm::length( lightoffset ) > 1000.f ) {
            // we don't care about lights past arbitrary limit of 1 km.
            // but there could still be weaker lights which are closer, so keep looking
            continue;
        }
        // if the light passed tests so far, it's good enough
        renderlight->position = lightoffset;
        renderlight->direction = scenelight.direction;

        auto luminance = static_cast<float>( Global.fLuminance );
        // adjust luminance level based on vehicle's location, e.g. tunnels
        auto const environment = scenelight.owner->fShade;
        if( environment > 0.f ) {
            luminance *= environment;
        }
        renderlight->diffuse =
            glm::vec4{
                glm::max( glm::vec3{ colors::none }, scenelight.color - glm::vec3{ luminance } ),
                renderlight->diffuse[ 3 ] };
        renderlight->ambient =
            glm::vec4{
                glm::max( glm::vec3{ colors::none }, scenelight.color * glm::vec3{ scenelight.intensity } - glm::vec3{ luminance } ),
                renderlight->ambient[ 3 ] };

        renderlight->apply_intensity();
        renderlight->apply_angle();

        gl::light_element_ubs *l = &light_ubs.lights[light_i];
        l->pos = mv * glm::vec4(renderlight->position, 1.0f);
        l->dir = mv * glm::vec4(renderlight->direction, 0.0f);
        l->type = gl::light_element_ubs::SPOT;
        l->in_cutoff = 0.906f;
        l->out_cutoff = 0.866f;
        l->color = renderlight->diffuse * renderlight->factor;
        l->linear = 0.007f;
        l->quadratic = 0.0002f;
        light_i++;

        ++renderlight;
    }

    light_ubs.ambient = m_sunlight.ambient * m_sunlight.factor;
    light_ubs.lights[0].type = gl::light_element_ubs::DIR;
    light_ubs.lights[0].dir = mv * glm::vec4(m_sunlight.direction, 0.0f);
    light_ubs.lights[0].color = m_sunlight.diffuse * m_sunlight.factor;
    light_ubs.lights_count = light_i;

    light_ubo->update(light_ubs);
}

bool
opengl_renderer::Init_caps() {

    std::string oglversion = ( (char *)glGetString( GL_VERSION ) );

    WriteLog(
        "Gfx Renderer: " + std::string( (char *)glGetString( GL_RENDERER ) )
        + " Vendor: " + std::string( (char *)glGetString( GL_VENDOR ) )
        + " OpenGL Version: " + oglversion );

    if( !GLEW_VERSION_3_3 ) {
        ErrorLog( "Requires openGL >= 3.3" );
        return false;
    }

    GLint extCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);

    WriteLog("Supported extensions: ");
    for (int i = 0; i < extCount; i++)
    {
        const char *ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
        WriteLog(ext);
    }

    if( GLEW_EXT_framebuffer_object ) {
        m_framebuffersupport = true;
        WriteLog( "Framebuffer objects enabled" );
    }
    else {
        WriteLog( "Framebuffer objects not supported, resorting to back buffer rendering where possible" );
    }
    // ograniczenie maksymalnego rozmiaru tekstur - parametr dla skalowania tekstur
    {
        GLint texturesize;
        ::glGetIntegerv( GL_MAX_TEXTURE_SIZE, &texturesize );
        Global.iMaxTextureSize = std::min( Global.iMaxTextureSize, texturesize );
        WriteLog( "Texture sizes capped at " + std::to_string( Global.iMaxTextureSize ) + " pixels" );
        m_shadowbuffersize = Global.shadowtune.map_size;
        m_shadowbuffersize = std::min( m_shadowbuffersize, texturesize );
        WriteLog( "Shadows map size capped at " + std::to_string( m_shadowbuffersize ) + " pixels" );
    }
    Global.DynamicLightCount = std::min(Global.DynamicLightCount, 8);

    if( Global.iMultisampling ) {
        WriteLog( "Using multisampling x" + std::to_string( 1 << Global.iMultisampling ) );
    }

    glGetError();
    glLineWidth(2.0f);
    if (!glGetError())
        m_widelines_supported = true;
    else
        WriteLog("warning: wide lines not supported");

    return true;
}

glm::vec3
opengl_renderer::pick_color( std::size_t const Index ) {
/*
    // pick colours are set with step of 4 for some slightly easier visual debugging. not strictly needed but, eh
    int const colourstep = 4;
    int const componentcapacity = 256 / colourstep;
    auto const redgreen = std::div( Index, componentcapacity * componentcapacity );
    auto const greenblue = std::div( redgreen.rem, componentcapacity );
    auto const blue = Index % componentcapacity;
    return
        glm::vec3 {
           redgreen.quot * colourstep / 255.0f,
           greenblue.quot * colourstep / 255.0f,
           greenblue.rem * colourstep / 255.0f };
*/
    // alternatively
    return
        glm::vec3{
        ( ( Index & 0xff0000 ) >> 16 ) / 255.0f,
        ( ( Index & 0x00ff00 ) >> 8 )  / 255.0f,
          ( Index & 0x0000ff )         / 255.0f };

}

std::size_t
opengl_renderer::pick_index( glm::ivec3 const &Color ) {
/*
    return (
          std::floor( Color.b / 4 )
        + std::floor( Color.g / 4 ) * 64
        + std::floor( Color.r / 4 ) * 64 * 64 );
*/
    // alternatively
    return
            Color.b
        + ( Color.g * 256 )
        + ( Color.r * 256 * 256 );

}

//---------------------------------------------------------------------------
