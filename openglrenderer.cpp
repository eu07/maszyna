/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "openglrenderer.h"

#include "color.h"
#include "Globals.h"
#include "Camera.h"
#include "Timer.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Train.h"
#include "DynObj.h"
#include "AnimModel.h"
#include "Traction.h"
#include "application.h"
#include "Logs.h"
#include "utilities.h"
#include "openglgeometrybank.h"

int const EU07_PICKBUFFERSIZE { 1024 }; // size of (square) textures bound with the pick framebuffer
int const EU07_ENVIRONMENTBUFFERSIZE { 256 }; // size of (square) environmental cube map texture
int const EU07_REFLECTIONFIDELITYOFFSET { 250 }; // artificial increase of range for reflection pass detail reduction

float const EU07_OPACITYDEFAULT { 0.5f };

bool
opengl_renderer::Init( GLFWwindow *Window ) {

    if( false == Init_caps() ) { return false; }

    m_window = Window;

    ::glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    ::glPixelStorei( GL_PACK_ALIGNMENT, 1 );

    glClearDepth( 1.0f );
    glClearColor( 51.0f / 255.0f, 102.0f / 255.0f, 85.0f / 255.0f, 1.0f ); // initial background Color

    glPolygonMode( GL_FRONT, GL_FILL );
    glFrontFace( GL_CCW ); // Counter clock-wise polygons face out
    glEnable( GL_CULL_FACE ); // Cull back-facing triangles
    glShadeModel( GL_SMOOTH ); // Enable Smooth Shading

    m_geometry.units().texture = (
        Global.BasicRenderer ?
            std::vector<GLint>{ m_diffusetextureunit } :
            std::vector<GLint>{ m_normaltextureunit, m_diffusetextureunit } );
    ui_layer::set_unit( m_diffusetextureunit );
    select_unit( m_diffusetextureunit );

    ::glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
    glAlphaFunc( GL_GREATER, 0.f );
    glEnable( GL_ALPHA_TEST );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D ); // Enable Texture Mapping

    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ); // Really Nice Perspective Calculations
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glLineWidth( 1.0f );
    glPointSize( 3.0f );
    glEnable( GL_POINT_SMOOTH );

    ::glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR );
    ::glMaterialf( GL_FRONT, GL_SHININESS, 15.0f );
    if( true == Global.ScaleSpecularValues ) {
        m_specularopaquescalefactor = 0.25f;
        m_speculartranslucentscalefactor = 1.5f;
    }
    ::glEnable( GL_COLOR_MATERIAL );
    ::glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );

    // setup lighting
    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(m_baseambient) );
    ::glEnable( GL_LIGHTING );
    ::glEnable( opengl_renderer::sunlight );

    // rgb value for 5780 kelvin
    Global.DayLight.diffuse[ 0 ] = 255.0f / 255.0f;
    Global.DayLight.diffuse[ 1 ] = 242.0f / 255.0f;
    Global.DayLight.diffuse[ 2 ] = 231.0f / 255.0f;
    Global.DayLight.is_directional = true;
    m_sunlight.id = opengl_renderer::sunlight;
    //    ::glLightf( opengl_renderer::sunlight, GL_SPOT_CUTOFF, 90.0f );

    // create dynamic light pool
    for( int idx = 0; idx < Global.DynamicLightCount; ++idx ) {

        opengl_light light;
        light.id = GL_LIGHT1 + idx;

        light.is_directional = false;
        ::glEnable( light.id ); // experimental intel chipset fix
        ::glLightf( light.id, GL_SPOT_CUTOFF, 7.5f );
        ::glLightf( light.id, GL_SPOT_EXPONENT, 7.5f );
        ::glLightf( light.id, GL_CONSTANT_ATTENUATION, 0.0f );
        ::glLightf( light.id, GL_LINEAR_ATTENUATION, 0.035f );
        ::glDisable( light.id ); // experimental intel chipset fix

        m_lights.emplace_back( light );
    }
    // preload some common textures
    m_glaretexture = Fetch_Texture( "fx/lightglare" );
    m_suntexture = Fetch_Texture( "fx/sun" );
    m_moontexture = Fetch_Texture( "fx/moon" );
    if( m_helpertextureunit >= 0 ) {
        m_reflectiontexture = Fetch_Texture( "fx/reflections" );
    }
    m_smoketexture = Fetch_Texture( "fx/smoke" );
    m_invalid_material = Fetch_Material( "fx/invalid" );

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    // pick buffer resources
    if( true == m_framebuffersupport ) {
        // try to create the pick buffer. RGBA8 2D texture, 24 bit depth texture, 1024x1024 (height of 512 would suffice but, eh)
        // texture
        ::glGenTextures( 1, &m_picktexture );
        ::glBindTexture( GL_TEXTURE_2D, m_picktexture );
        ::glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        // depth buffer
        ::glGenRenderbuffersEXT( 1, &m_pickdepthbuffer );
        ::glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, m_pickdepthbuffer );
        ::glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE );
        // create and assemble the framebuffer
        ::glGenFramebuffersEXT( 1, &m_pickframebuffer );
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_pickframebuffer );
        ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_picktexture, 0 );
        ::glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_pickdepthbuffer );
        // check if we got it working
        GLenum status = ::glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
        if( status == GL_FRAMEBUFFER_COMPLETE_EXT ) {
            WriteLog( "Picking framebuffer setup complete" );
        }
        else{
            ErrorLog( "Picking framebuffer setup failed" );
            m_framebuffersupport = false;
        }
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target for now
    }
#endif
    // shadowmap resources
    if( ( true == Global.RenderShadows )
     && ( true == m_framebuffersupport ) ) {
        // primary shadow map
        {
            // texture:
            ::glGenTextures( 1, &m_shadowtexture );
            ::glBindTexture( GL_TEXTURE_2D, m_shadowtexture );
            // allocate memory
            ::glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadowbuffersize, m_shadowbuffersize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL );
            // setup parameters
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            // enable shadow comparison: true (ie not in shadow) if r<=texture...
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
            ::glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
            ::glBindTexture( GL_TEXTURE_2D, 0 );
#ifdef EU07_USE_DEBUG_SHADOWMAP
            ::glGenTextures( 1, &m_shadowdebugtexture );
            ::glBindTexture( GL_TEXTURE_2D, m_shadowdebugtexture );
            // allocate memory
            ::glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_shadowbuffersize, m_shadowbuffersize, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL );
            // setup parameters
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            ::glBindTexture( GL_TEXTURE_2D, 0 );
#endif
            // create and assemble the framebuffer
            ::glGenFramebuffersEXT( 1, &m_shadowframebuffer );
            ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_shadowframebuffer );
#ifdef EU07_USE_DEBUG_SHADOWMAP
            ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_shadowdebugtexture, 0 );
#else
            ::glDrawBuffer( GL_NONE ); // we won't be rendering colour data, so can skip the colour attachment
            ::glReadBuffer( GL_NONE );
#endif
            ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_shadowtexture, 0 );
            // check if we got it working
            GLenum const status{ ::glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT ) };
            if( status == GL_FRAMEBUFFER_COMPLETE_EXT ) {
                WriteLog( "Shadows framebuffer setup complete" );
            }
            else {
                ErrorLog( "Shadows framebuffer setup failed" );
                Global.RenderShadows = false;
            }
            // switch back to primary render target for now
            ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
         }
        // cab shadow map
        {
            // NOTE: using separate framebuffer as more efficient arrangement than swapping attachments on a single one
            // TODO: for shader-based implementation use instead a quarter of the CSM
            // texture:
            ::glGenTextures( 1, &m_cabshadowtexture );
            ::glBindTexture( GL_TEXTURE_2D, m_cabshadowtexture );
            // allocate memory
            ::glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadowbuffersize / 2, m_shadowbuffersize / 2, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL );
            // setup parameters
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            // enable shadow comparison: true (ie not in shadow) if r<=texture...
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
            ::glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
            ::glBindTexture( GL_TEXTURE_2D, 0 );
#ifdef EU07_USE_DEBUG_CABSHADOWMAP
            ::glGenTextures( 1, &m_cabshadowdebugtexture );
            ::glBindTexture( GL_TEXTURE_2D, m_cabshadowdebugtexture );
            // allocate memory
            ::glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, m_shadowbuffersize / 2, m_shadowbuffersize / 2, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL );
            // setup parameters
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            ::glBindTexture( GL_TEXTURE_2D, 0 );
#endif
            // create and assemble the framebuffer
            ::glGenFramebuffersEXT( 1, &m_cabshadowframebuffer );
            ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_cabshadowframebuffer );
#ifdef EU07_USE_DEBUG_CABSHADOWMAP
            ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_cabshadowdebugtexture, 0 );
#else
            ::glDrawBuffer( GL_NONE ); // we won't be rendering colour data, so can skip the colour attachment
            ::glReadBuffer( GL_NONE );
#endif
            ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_cabshadowtexture, 0 );
            // check if we got it working
            GLenum const status{ ::glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT ) };
            if( status == GL_FRAMEBUFFER_COMPLETE_EXT ) {
                WriteLog( "Cab shadows framebuffer setup complete" );
            }
            else {
                ErrorLog( "Cab shadows framebuffer setup failed" );
                Global.RenderShadows = false;
            }
        }
        // switch back to primary render target for now
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
    }
    // environment cube map resources
    if( ( false == Global.BasicRenderer )
     && ( true == m_framebuffersupport ) ) {
        // texture:
        ::glGenTextures( 1, &m_environmentcubetexture );
        ::glBindTexture( GL_TEXTURE_CUBE_MAP, m_environmentcubetexture );
        // allocate space
        for( GLuint faceindex = GL_TEXTURE_CUBE_MAP_POSITIVE_X; faceindex < GL_TEXTURE_CUBE_MAP_POSITIVE_X + 6; ++faceindex ) {
            ::glTexImage2D( faceindex, 0, GL_RGBA8, EU07_ENVIRONMENTBUFFERSIZE, EU07_ENVIRONMENTBUFFERSIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL );
        }
        // setup parameters
        ::glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        ::glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        ::glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        ::glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        ::glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
        // depth buffer
        ::glGenRenderbuffersEXT( 1, &m_environmentdepthbuffer );
        ::glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, m_environmentdepthbuffer );
        ::glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, EU07_ENVIRONMENTBUFFERSIZE, EU07_ENVIRONMENTBUFFERSIZE );
        // create and assemble the framebuffer
        ::glGenFramebuffersEXT( 1, &m_environmentframebuffer );
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_environmentframebuffer );
        ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_environmentcubetexture, 0 );
        ::glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_environmentdepthbuffer );
        // check if we got it working
        GLenum status = ::glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
        if( status == GL_FRAMEBUFFER_COMPLETE_EXT ) {
            WriteLog( "Reflections framebuffer setup complete" );
            m_environmentcubetexturesupport = true;
        }
        else {
            ErrorLog( "Reflections framebuffer setup failed" );
            m_environmentcubetexturesupport = false;
        }
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target for now
    }
    // prepare basic geometry chunks
    auto const geometrybank = Create_Bank();
    float const size = 2.5f;
    gfx::vertex_array billboard_array{
        { { -size,  size, 0.f }, glm::vec3(), { 1.f, 1.f } },
        { {  size,  size, 0.f }, glm::vec3(), { 0.f, 1.f } },
        { { -size, -size, 0.f }, glm::vec3(), { 1.f, 0.f } },
        { {  size, -size, 0.f }, glm::vec3(), { 0.f, 0.f } } };
    m_billboardgeometry = m_geometry.create_chunk(billboard_array, geometrybank, GL_TRIANGLE_STRIP);
    // prepare debug mode objects
    //m_quadric = ::gluNewQuadric();
    //::gluQuadricNormals( m_quadric, GLU_FLAT );

    WriteLog( "Gfx Renderer: setup complete" );

    return true;
}

bool
opengl_renderer::Render() {

    Timer::subsystem.gfx_total.stop();
    Timer::subsystem.gfx_total.start(); // note: gfx_total is actually frame total, clean this up
    Timer::subsystem.gfx_color.start();
    // fetch simulation data
    if( simulation::is_ready ) {
        m_sunlight = Global.DayLight;
        // quantize sun angle to reduce shadow crawl
        auto const quantizationstep { 0.004f };
        m_sunlight.direction = glm::normalize( quantizationstep * glm::roundEven( m_sunlight.direction * ( 1.f / quantizationstep ) ) );
    }
    // generate new frame
    opengl_texture::reset_unit_cache();
    m_renderpass.draw_mode = rendermode::none; // force setup anew
    m_renderpass.draw_stats = debug_stats();
    m_geometry.primitives_count() = 0;
    Render_pass( rendermode::color );
    Timer::subsystem.gfx_color.stop();
    // add user interface
    setup_units( true, false, false );
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );
    ::glClientActiveTexture( GL_TEXTURE0 + m_diffusetextureunit );
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );
    Application.render_ui();
    ::glPopClientAttrib();
    if( Global.bUseVBO ) {
        // swapbuffers() will unbind current buffers so we prepare for it on our end
        gfx::opengl_vbogeometrybank::reset();
    }
    ++m_framestamp;

    return true; // for now always succeed
}

void
opengl_renderer::SwapBuffers() {

    Timer::subsystem.gfx_swap.start();
    glfwSwapBuffers( m_window );
    Timer::subsystem.gfx_swap.stop();

    m_debugtimestext.clear();
    m_debugtimestext =
        "cpu frame total: " + to_string( Timer::subsystem.gfx_color.average() + Timer::subsystem.gfx_shadows.average() + Timer::subsystem.gfx_swap.average(), 2 ) + " ms\n"
        + " color: " + to_string( Timer::subsystem.gfx_color.average(), 2 ) + " ms (" + std::to_string( m_cellqueue.size() ) + " sectors)\n";
    if( Global.RenderShadows ) {
        m_debugtimestext +=
            " shadows: " + to_string( Timer::subsystem.gfx_shadows.average(), 2 ) + " ms\n";
    }
    m_debugtimestext += " swap: " + to_string( Timer::subsystem.gfx_swap.average(), 2 ) + " ms\n";
    m_debugtimestext += "uilayer: " + to_string( Timer::subsystem.gfx_gui.average(), 2 ) + " ms\n";

    m_debugstatstext =
        "triangles: " + to_string( static_cast<int>(m_geometry.primitives_count()), 7 ) + "\n"
        + "vehicles:  " + to_string( m_colorpass.draw_stats.dynamics, 7 ) + " +" + to_string( m_shadowpass.draw_stats.dynamics, 7 )
        + " =" + to_string( m_colorpass.draw_stats.dynamics + m_shadowpass.draw_stats.dynamics, 7 ) + "\n"
        + "models:    " + to_string( m_colorpass.draw_stats.models, 7 ) + " +" + to_string( m_shadowpass.draw_stats.models, 7 )
        + " =" + to_string( m_colorpass.draw_stats.models + m_shadowpass.draw_stats.models, 7 ) + "\n"
        + "drawcalls: " + to_string( m_colorpass.draw_stats.drawcalls, 7 ) + " +" + to_string( m_shadowpass.draw_stats.drawcalls, 7 )
        + " =" + to_string( m_colorpass.draw_stats.drawcalls + m_shadowpass.draw_stats.drawcalls, 7 ) + "\n"
        + " submodels:" + to_string( m_colorpass.draw_stats.submodels, 7 ) + " +" + to_string( m_shadowpass.draw_stats.submodels, 7 )
        + " =" + to_string( m_colorpass.draw_stats.submodels + m_shadowpass.draw_stats.submodels, 7 ) + "\n"
        + " paths:    " + to_string( m_colorpass.draw_stats.paths, 7 ) + " +" + to_string( m_shadowpass.draw_stats.paths, 7 )
        + " =" + to_string( m_colorpass.draw_stats.paths + m_shadowpass.draw_stats.paths, 7 ) + "\n"
        + " shapes:   " + to_string( m_colorpass.draw_stats.shapes, 7 ) + " +" + to_string( m_shadowpass.draw_stats.shapes, 7 )
        + " =" + to_string( m_colorpass.draw_stats.shapes + m_shadowpass.draw_stats.shapes, 7 ) + "\n"
        + " traction: " + to_string( m_colorpass.draw_stats.traction, 7 ) + "\n"
        + " lines:    " + to_string( m_colorpass.draw_stats.lines, 7 ) + "\n"
        + "particles: " + to_string( m_colorpass.draw_stats.particles, 7 );
}

// runs jobs needed to generate graphics for specified render pass
void
opengl_renderer::Render_pass( rendermode const Mode ) {

#ifdef EU07_USE_DEBUG_CAMERA
    // setup world camera for potential visualization
    setup_pass(
        m_worldcamera,
        rendermode::color,
        0.f,
        1.0,
        true );
#endif
    setup_pass( m_renderpass, Mode );
    switch( m_renderpass.draw_mode ) {

        case rendermode::color: {

            m_colorpass = m_renderpass;

            if( ( !simulation::is_ready ) || ( Global.gfx_skiprendering ) ) {
                ::glClearColor( 51.0f / 255.f, 102.0f / 255.f, 85.0f / 255.f, 1.f ); // initial background Color
                ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
                break;
            }

            if( ( true == Global.RenderShadows )
             && ( false == Global.bWireFrame )
             && ( m_shadowcolor != colors::white ) ) {

                // run shadowmaps pass before color
                Timer::subsystem.gfx_shadows.start();
                if( false == FreeFlyModeFlag ) {
                    Render_pass( rendermode::cabshadows );
                }
                Render_pass( rendermode::shadows );
                Timer::subsystem.gfx_shadows.stop();
#ifdef EU07_USE_DEBUG_SHADOWMAP
                UILayer.set_texture( m_shadowdebugtexture );
#endif
#ifdef EU07_USE_DEBUG_CABSHADOWMAP
                UILayer.set_texture( m_cabshadowdebugtexture );
#endif
                setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack
                // setup shadowmap matrices
                m_shadowtexturematrix =
                    glm::mat4 { 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f } // bias from [-1, 1] to [0, 1]
                    * m_shadowpass.camera.projection()
                    // during colour pass coordinates are moved from camera-centric to light-centric, essentially the difference between these two origins
                    * glm::translate(
                        glm::mat4{ glm::mat3{ m_shadowpass.camera.modelview() } },
                        glm::vec3{ m_renderpass.camera.position() - m_shadowpass.camera.position() } );

                if( false == FreeFlyModeFlag ) {
                    m_cabshadowtexturematrix =
                        glm::mat4{ 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f } // bias from [-1, 1] to [0, 1]
                        * m_cabshadowpass.camera.projection()
                        // during colour pass coordinates are moved from camera-centric to light-centric, essentially the difference between these two origins
                        * glm::translate(
                            glm::mat4{ glm::mat3{ m_cabshadowpass.camera.modelview() } },
                            glm::vec3{ m_renderpass.camera.position() - m_cabshadowpass.camera.position() } );
                }
            }

            if( true == m_environmentcubetexturesupport ) {
                // potentially update environmental cube map
                m_renderpass.draw_stats = {};
                if( true == Render_reflections() ) {
                    setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack
                }
            }

            ::glViewport( 0, 0, Global.iWindowWidth, Global.iWindowHeight );

            auto const skydomecolour = simulation::Environment.m_skydome.GetAverageColor();
            ::glClearColor( skydomecolour.x, skydomecolour.y, skydomecolour.z, 0.f ); // kolor nieba
            ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            // setup
            setup_matrices();
            // render
            setup_drawing( true );
            setup_units( true, false, false );
            m_renderpass.draw_stats = {};
            Render( &simulation::Environment );
            // opaque parts...
            setup_drawing( false );
            setup_units( true, true, true );
#ifdef EU07_USE_DEBUG_CAMERA
            if( DebugModeFlag ) {
                // draw light frustum
                ::glLineWidth( 2.f );
                ::glColor4f( 1.f, 0.9f, 0.8f, 1.f );
                ::glDisable( GL_LIGHTING );
                ::glDisable( GL_TEXTURE_2D );
                if( ( true == Global.RenderShadows ) && ( false == Global.bWireFrame ) ) {
                    m_shadowpass.camera.draw( m_renderpass.camera.position() - m_shadowpass.camera.position() );
                }
                if( DebugCameraFlag ) {
                    ::glColor4f( 0.8f, 1.f, 0.9f, 1.f );
                    m_worldcamera.camera.draw( m_renderpass.camera.position() - m_worldcamera.camera.position() );
                }
                ::glLineWidth( 1.f );
                ::glEnable( GL_LIGHTING );
                ::glEnable( GL_TEXTURE_2D );
            }
#endif
            // without rain/snow we can render the cab early to limit the overdraw
            if( ( false == FreeFlyModeFlag )
             && ( Global.Overcast <= 1.f ) ) { // precipitation happens when overcast is in 1-2 range
#ifdef EU07_DISABLECABREFLECTIONS
                switch_units( true, true, false );
#endif
                setup_shadow_map( m_cabshadowtexture, m_cabshadowtexturematrix );
                // cache shadow colour in case we need to account for cab light
                auto const shadowcolor { m_shadowcolor };
                auto const *vehicle{ simulation::Train->Dynamic() };
                if( vehicle->InteriorLightLevel > 0.f ) {
                    setup_shadow_color( glm::min( colors::white, shadowcolor + glm::vec4( vehicle->InteriorLight * vehicle->InteriorLightLevel, 1.f ) ) );
                }
                Render_cab( vehicle, vehicle->InteriorLightLevel, false );
                if( vehicle->InteriorLightLevel > 0.f ) {
                    setup_shadow_color( shadowcolor );
                }
            }
            switch_units( true, true, true );
            setup_shadow_map( m_shadowtexture, m_shadowtexturematrix );
            Render( simulation::Region );
            // ...translucent parts
            setup_drawing( true );
            Render_Alpha( simulation::Region );
            // particles
            Render_particles();
            // precipitation; done at the end, only before cab render
            Render_precipitation();
            // cab render
            if( false == FreeFlyModeFlag ) {
#ifdef EU07_DISABLECABREFLECTIONS
                switch_units( true, true, false );
#endif
                setup_shadow_map( m_cabshadowtexture, m_cabshadowtexturematrix );
                // cache shadow colour in case we need to account for cab light
                auto const shadowcolor{ m_shadowcolor };
                auto *vehicle { simulation::Train->Dynamic() };
                if( vehicle->InteriorLightLevel > 0.f ) {
                    setup_shadow_color( glm::min( colors::white, shadowcolor + glm::vec4( vehicle->InteriorLight * vehicle->InteriorLightLevel, 1.f ) ) );
                }
                if( Global.Overcast > 1.f ) {
                    // with active precipitation draw the opaque cab parts here to mask rain/snow placed 'inside' the cab
                    setup_drawing( false );
                    Render_cab( vehicle, vehicle->InteriorLightLevel, false );
                    Render_interior( false);
                    setup_drawing( true );
                    Render_interior( true );
                }
                Render_cab( vehicle, vehicle->InteriorLightLevel, true );
                if( vehicle->InteriorLightLevel > 0.f ) {
                    setup_shadow_color( shadowcolor );
                }
                // with the cab in place we can (finally) safely draw translucent part of the occupied vehicle
                Render_Alpha( vehicle );
            }

            if( m_environmentcubetexturesupport ) {
                // restore default texture matrix for reflections cube map
                select_unit( m_helpertextureunit );
                ::glMatrixMode( GL_TEXTURE );
//                ::glPopMatrix();
                ::glLoadIdentity();
                select_unit( m_diffusetextureunit );
                ::glMatrixMode( GL_MODELVIEW );
            }
            // store draw stats
            m_colorpass.draw_stats = m_renderpass.draw_stats;

            break;
        }

        case rendermode::shadows: {

            if( simulation::is_ready ) {
                // setup
                ::glEnable( GL_POLYGON_OFFSET_FILL ); // alleviate depth-fighting
                ::glPolygonOffset( 1.f, 1.f );

                // main shadowmap
                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_shadowframebuffer );
                ::glViewport( 0, 0, m_shadowbuffersize, m_shadowbuffersize );

#ifdef EU07_USE_DEBUG_SHADOWMAP
                ::glClearColor( 0.f / 255.f, 0.f / 255.f, 0.f / 255.f, 1.f ); // initial background Color
                ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
#else
                ::glClear( GL_DEPTH_BUFFER_BIT ); 
#endif
                ::glScissor( 1, 1, m_shadowbuffersize - 2, m_shadowbuffersize - 2 );
                ::glEnable( GL_SCISSOR_TEST );

                setup_matrices();
                // render
                // opaque parts...
                setup_drawing( false );
#ifdef EU07_USE_DEBUG_SHADOWMAP
                setup_units( true, false, false );
#else
                setup_units( false, false, false );
#endif
                Render( simulation::Region );
                m_shadowpass = m_renderpass;

                // post-render restore
                ::glDisable( GL_SCISSOR_TEST );
                ::glDisable( GL_POLYGON_OFFSET_FILL );

                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target
            }
            break;
        }

        case rendermode::cabshadows: {

            if( ( simulation::is_ready )
             && ( false == FreeFlyModeFlag ) ) {
                // setup
                ::glEnable( GL_POLYGON_OFFSET_FILL ); // alleviate depth-fighting
                ::glPolygonOffset( 1.f, 1.f );

                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_cabshadowframebuffer );
                ::glViewport( 0, 0, m_shadowbuffersize / 2, m_shadowbuffersize / 2 );

#ifdef EU07_USE_DEBUG_CABSHADOWMAP
                ::glClearColor( 0.f / 255.f, 0.f / 255.f, 0.f / 255.f, 1.f ); // initial background Color
                ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
#else
                ::glClear( GL_DEPTH_BUFFER_BIT );
#endif
                ::glScissor( 1, 1, m_shadowbuffersize / 2 - 2, m_shadowbuffersize / 2 - 2 );
                ::glEnable( GL_SCISSOR_TEST );

                setup_matrices();
                // render
                // opaque parts...
                setup_drawing( false );
#ifdef EU07_USE_DEBUG_CABSHADOWMAP
                setup_units( true, false, false );
#else
                setup_units( false, false, false );
#endif
                ::glDisable( GL_CULL_FACE );

                if( Global.RenderCabShadowsRange > 0 ) {
                    Render( simulation::Region );
                }
                Render_cab( simulation::Train->Dynamic(), 0.f, false );
                Render_cab( simulation::Train->Dynamic(), 0.f, true );
                m_cabshadowpass = m_renderpass;

                // post-render restore
                ::glDisable( GL_SCISSOR_TEST );
                ::glEnable( GL_CULL_FACE );
                ::glDisable( GL_POLYGON_OFFSET_FILL );

                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target
            }
            break;
        }

        case rendermode::reflections: {
            // NOTE: buffer and viewport setup in this mode is handled by the wrapper method
            ::glClearColor( 0.f, 0.f, 0.f, 1.f );
            ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            if( simulation::is_ready ) {

                ::glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE );
                // setup
                setup_matrices();
                // render
                setup_drawing( true );
                setup_units( true, false, false );
                Render( &simulation::Environment );
                // opaque parts...
                setup_drawing( false );
                setup_units( true, true, true );
                setup_shadow_map( m_shadowtexture, m_shadowtexturematrix );
                Render( simulation::Region );
/*
                // reflections are limited to sky and ground only, the update rate is too low for anything else
                // ...translucent parts
                setup_drawing( true );
                Render_Alpha( &World.Ground );
                // cab render is performed without shadows, due to low resolution and number of models without windows :|
                switch_units( true, false, false );
                // cab render is done in translucent phase to deal with badly configured vehicles
                if( World.Train != nullptr ) { Render_cab( World.Train->Dynamic() ); }
*/
                ::glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
            }
            break;
        }

        case rendermode::pickcontrols: {
            if( simulation::is_ready ) {
                // setup
#ifdef EU07_USE_PICKING_FRAMEBUFFER
                ::glViewport( 0, 0, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE );
#endif
                ::glClearColor( 0.f, 0.f, 0.f, 1.f );
                ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

                m_pickcontrolsitems.clear();
                setup_matrices();
                // render
                // opaque parts...
                setup_drawing( false );
                setup_units( false, false, false );
                // cab render skips translucent parts, so we can do it here
                if( simulation::Train != nullptr ) {
                    Render_cab( simulation::Train->Dynamic(), 0.f );
                    Render( simulation::Train->Dynamic() );
                }
                // post-render cleanup
            }
            break;
        }

        case rendermode::pickscenery: {
            if( simulation::is_ready ) {
                // setup
                m_picksceneryitems.clear();
#ifdef EU07_USE_PICKING_FRAMEBUFFER
                ::glViewport( 0, 0, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE );
#endif
                ::glClearColor( 0.f, 0.f, 0.f, 1.f );
                ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

                setup_matrices();
                // render
                // opaque parts...
                setup_drawing( false );
                setup_units( false, false, false );
                Render( simulation::Region );
                // post-render cleanup
            }
            break;
        }

        default: {
            break;
        }
    }
}

bool opengl_renderer::Render_interior( bool const Alpha ) {
    // NOTE: legacy renderer doesn't need to repaint translucent parts
    // TODO: investigate reasons (depth writing?) and duplicate in opengl33 branch
    if( Alpha ) { return false; }
    // TODO: early frustum based cull, camera might be pointing elsewhere
    std::vector< std::pair<float, TDynamicObject *> > dynamics;
    auto *dynamic { simulation::Train->Dynamic() };
    // draw interiors of the occupied vehicle, and all vehicles behind it with permanent coupling, in case they're open-ended
    while( dynamic != nullptr ) {

        glm::dvec3 const originoffset { dynamic->vPosition - m_renderpass.camera.position() };
        float const squaredistance{ glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor ) };
        dynamics.emplace_back( squaredistance, dynamic );
        dynamic = dynamic->Next( coupling::permanent );
    }
    // draw also interiors of permanently coupled vehicles in front, if there's any
    dynamic = simulation::Train->Dynamic()->Prev( coupling::permanent );
    while( dynamic != nullptr ) {

        glm::dvec3 const originoffset { dynamic->vPosition - m_renderpass.camera.position() };
        float const squaredistance{ glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor ) };
        dynamics.emplace_back( squaredistance, dynamic );
        dynamic = dynamic->Prev( coupling::permanent );
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

bool opengl_renderer::Render_lowpoly( TDynamicObject *Dynamic, float const Squaredistance, bool const Setup, bool const Alpha ) {

    if( Dynamic->mdLowPolyInt == nullptr ) { return false; }

    // low poly interior
    glm::dvec3 const originoffset { Dynamic->vPosition - m_renderpass.camera.position() };

    if( Setup ) {

        TSubModel::iInstance = reinterpret_cast<std::uintptr_t>( Dynamic ); //żeby nie robić cudzych animacji
        Dynamic->ABuLittleUpdate( Squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu
        ::glPushMatrix();

        ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
        ::glMultMatrixd( Dynamic->mMatrix.getArray() );

        m_renderspecular = true; // vehicles are rendered with specular component. static models without, at least for the time being
    }
    // HACK: reduce light level for vehicle interior if there's strong global lighting source
    if( false == Alpha ) {
        auto const luminance{ static_cast<float>(
            0.5
            * ( std::max(
                0.3,
                ( Global.fLuminance - Global.Overcast )
                * ( Dynamic->fShade > 0.0 ?
                    Dynamic->fShade :
                    1.0 ) ) ) ) };
        m_sunlight.apply_intensity(
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
        m_sunlight.apply_intensity( Dynamic->fShade > 0.f ? Dynamic->fShade : 1.f );
    }
    else {
 //       Render_Alpha( Dynamic->mdLowPolyInt, Dynamic->Material(), Squaredistance );
        // HACK: some models have windows included as part of the main model instead of lowpoly
        if( Dynamic->mdModel ) {
            // main model
            Render_Alpha( Dynamic->mdModel, Dynamic->Material(), Squaredistance );
        }
    }

    if( Setup ) {

        m_renderspecular = false;
        if( Dynamic->fShade > 0.0f ) {
            // restore regular light level
            m_sunlight.apply_intensity();
        }

        ::glPopMatrix();

        // TODO: check if this reset is needed. In theory each object should render all parts based on its own instance data anyway?
        if( Dynamic->btnOn ) {
            Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli
        }
    }

    return true;
}

bool opengl_renderer::Render_coupler_adapter( TDynamicObject *Dynamic, float const Squaredistance, int const End, bool const Alpha ) {

    if( Dynamic->m_coupleradapters[ End ] == nullptr ) { return false; }

    auto const position { Math3D::vector3 {
        0.f,
        Dynamic->MoverParameters->Couplers[ End ].adapter_height,
        ( Dynamic->MoverParameters->Couplers[ End ].adapter_length + Dynamic->MoverParameters->Dim.L * 0.5 ) * ( End == end::front ? 1 : -1 ) } };

    auto const angle{ glm::vec3{ 0, ( End == end::front ? 0 : 180 ), 0 } };

    if( Alpha ) {
        Render_Alpha( Dynamic->m_coupleradapters[ End ], Dynamic->Material(), Squaredistance, position, angle );
    }
    else {
        Render( Dynamic->m_coupleradapters[ End ], Dynamic->Material(), Squaredistance, position, angle );
    }

    return true;
}

// creates dynamic environment cubemap
bool
opengl_renderer::Render_reflections() {

    if( Global.reflectiontune.update_interval == 0 ) { return false; }

    auto const timestamp { Timer::GetRenderTime() };
    if( ( timestamp - m_environmentupdatetime < Global.reflectiontune.update_interval )
     && ( glm::length( m_renderpass.camera.position() - m_environmentupdatelocation ) < 1000.0 ) ) {
        // run update every 5+ mins of simulation time, or at least 1km from the last location
        return false;
    }
    m_environmentupdatetime = timestamp;
    m_environmentupdatelocation = m_renderpass.camera.position();

    ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_environmentframebuffer );
    ::glViewport( 0, 0, EU07_ENVIRONMENTBUFFERSIZE, EU07_ENVIRONMENTBUFFERSIZE );
    for( m_environmentcubetextureface = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
         m_environmentcubetextureface < GL_TEXTURE_CUBE_MAP_POSITIVE_X + 6;
       ++m_environmentcubetextureface ) {

        ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, m_environmentcubetextureface, m_environmentcubetexture, 0 );
        Render_pass( rendermode::reflections );
    }
    ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

    return true;
}

void
opengl_renderer::setup_pass( renderpass_config &Config, rendermode const Mode, float const Znear, float const Zfar, bool const Ignoredebug ) {

    Config.draw_mode = Mode;

    if( false == simulation::is_ready ) { return; }
    // setup draw range
    switch( Mode ) {
        case rendermode::color:        { Config.draw_range = Global.BaseDrawRange * Global.fDistanceFactor; break; }
        case rendermode::shadows:      { Config.draw_range = Global.shadowtune.range; break; }
        case rendermode::cabshadows:   { Config.draw_range = ( Global.RenderCabShadowsRange > 0 ? clamp( Global.RenderCabShadowsRange, 5, 100 ) : simulation::Train->Occupied()->Dim.L ); break; }
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
                camera.position() = Global.pCamera.Pos;
                Global.pCamera.SetMatrix( viewmatrix );
            }
            else {
                camera.position() = Global.pDebugCamera.Pos;
                Global.pDebugCamera.SetMatrix( viewmatrix );
            }
            // optimization: don't bother drawing things completely blended with the background
            // but ensure at least 2 km range to account for signals and lights
            Config.draw_range = std::min( Config.draw_range, m_fogrange * 2 );
            Config.draw_range = std::max( 2000.f, Config.draw_range );
            // projection
		    auto const zfar  = ( Zfar  > 1.f ? Zfar : Config.draw_range * Zfar );
		    auto const znear = ( Znear > 1.f ? Znear : Znear > 0.f ? Znear * zfar : 0.1f * Global.ZoomFactor);
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
//            auto const zfar = std::min( 1.f, Global.shadowtune.range / ( Global.BaseDrawRange * Global.fDistanceFactor ) * std::max( 1.f, Global.ZoomFactor * 0.5f ) );
//            auto const zfar = Global.shadowtune.range * std::max( Zfar, Global.ZoomFactor * 0.5f );
            auto const zfar = ( Zfar > 1.f ? Zfar : Zfar * Global.shadowtune.range * std::max( Zfar, Global.ZoomFactor * 0.5f ) );
            renderpass_config worldview;
            setup_pass( worldview, rendermode::color, Znear, zfar, true );
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
            auto const frustumchunkdepth { zfar - ( Znear > 1.f ? Znear : 0.f ) };
		    auto const quantizationstep{std::min(frustumchunkdepth, 50.f)};
            frustumchunkmin = quantizationstep * glm::floor( frustumchunkmin * ( 1.f / quantizationstep ) );
            frustumchunkmax = quantizationstep * glm::ceil( frustumchunkmax * ( 1.f / quantizationstep ) );
            // ...use the dimensions to set up light projection boundaries...
            // NOTE: since we only have one cascade map stage, we extend the chunk forward/back to catch areas normally covered by other stages
            camera.projection() *=
                glm::ortho(
                    frustumchunkmin.x, frustumchunkmax.x,
                    frustumchunkmin.y, frustumchunkmax.y,
                    frustumchunkmin.z - 500.f, frustumchunkmax.z + 500.f );
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
        case rendermode::cabshadows: {
            // fixed size cube large enough to enclose a vehicle compartment
            // modelview
            auto const lightvector =
                glm::normalize( glm::vec3{
                              m_sunlight.direction.x,
                    std::min( m_sunlight.direction.y, -0.2f ),
                              m_sunlight.direction.z } );
            camera.position() = Global.pCamera.Pos - glm::dvec3 { lightvector };
            viewmatrix *= glm::lookAt(
                camera.position(),
                glm::dvec3 { Global.pCamera.Pos },
                glm::dvec3 { 0.f, 1.f, 0.f } );
            // projection
            auto const maphalfsize { std::min( 10.f, Config.draw_range * 0.5f ) };
            camera.projection() *=
                glm::ortho(
                    -maphalfsize, maphalfsize,
                    -maphalfsize, maphalfsize,
                    -Config.draw_range, Config.draw_range );
/*
            // adjust the projection to sample complete shadow map texels
            auto shadowmaptexel = glm::vec2 { camera.projection() * glm::mat4{ viewmatrix } * glm::vec4{ 0.f, 0.f, 0.f, 1.f } };
            shadowmaptexel *= ( m_shadowbuffersize / 2 ) * 0.5f;
            auto shadowmapadjustment = glm::round( shadowmaptexel ) - shadowmaptexel;
            shadowmapadjustment /= ( m_shadowbuffersize / 2 ) * 0.5f;
            camera.projection() = glm::translate( glm::mat4{ 1.f }, glm::vec3{ shadowmapadjustment, 0.f } ) * camera.projection();
*/
            break;
        }
        case rendermode::reflections: {
            // modelview
            camera.position() = (
                ( ( true == DebugCameraFlag ) && ( false == Ignoredebug ) ) ?
                    Global.pDebugCamera.Pos :
                    Global.pCamera.Pos );
            glm::dvec3 const cubefacetargetvectors[ 6 ] = { {  1.0,  0.0,  0.0 }, { -1.0,  0.0,  0.0 }, {  0.0,  1.0,  0.0 }, {  0.0, -1.0,  0.0 }, {  0.0,  0.0,  1.0 }, {  0.0,  0.0, -1.0 } };
            glm::dvec3 const cubefaceupvectors[ 6 ] =     { {  0.0, -1.0,  0.0 }, {  0.0, -1.0,  0.0 }, {  0.0,  0.0,  1.0 }, {  0.0,  0.0, -1.0 }, {  0.0, -1.0,  0.0 }, {  0.0, -1.0,  0.0 } };
            auto const cubefaceindex = m_environmentcubetextureface - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            viewmatrix *=
                glm::lookAt(
                    camera.position(),
                    camera.position() + cubefacetargetvectors[ cubefaceindex ],
                    cubefaceupvectors[ cubefaceindex ] );
            // projection
            camera.projection() *=
                glm::perspective(
                    glm::radians( 90.f ),
                    1.f,
                    0.1f * Global.ZoomFactor,
                    Config.draw_range * Global.fDistanceFactor );
            break;
        }

        case rendermode::pickcontrols:
        case rendermode::pickscenery: {
            // TODO: scissor test for pick modes
            // modelview
            camera.position() = Global.pCamera.Pos;
            Global.pCamera.SetMatrix( viewmatrix );
            // projection
            camera.projection() *=
                glm::perspective(
                    glm::radians( Global.FieldOfView / Global.ZoomFactor ),
                    std::max( 1.f, (float)Global.iWindowWidth ) / std::max( 1.f, (float)Global.iWindowHeight ),
                    0.1f * Global.ZoomFactor,
                    Config.draw_range * Global.fDistanceFactor );
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

    if( ( m_renderpass.draw_mode == rendermode::color )
     && ( m_environmentcubetexturesupport ) ) {
        // special case, for colour render pass setup texture matrix for reflections cube map
        select_unit( m_helpertextureunit );
        ::glMatrixMode( GL_TEXTURE );
//        ::glPushMatrix();
        ::glLoadIdentity();
        ::glMultMatrixf( glm::value_ptr( glm::inverse( glm::mat4{ glm::mat3{ m_renderpass.camera.modelview() } } ) ) );
        select_unit( m_diffusetextureunit );
    }

    // trim modelview matrix just to rotation, since rendering is done in camera-centric world space
    ::glMatrixMode( GL_MODELVIEW );
    OpenGLMatrices.load_matrix( glm::mat4( glm::mat3( m_renderpass.camera.modelview() ) ) );
}

void
opengl_renderer::setup_drawing( bool const Alpha ) {

    if( true == Alpha ) {

        ::glEnable( GL_BLEND );
        ::glAlphaFunc( GL_GREATER, 0.f );
    }
    else {
        ::glDisable( GL_BLEND );
        ::glAlphaFunc( GL_GREATER, EU07_OPACITYDEFAULT );
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections: {
            ::glEnable( GL_LIGHTING );
            ::glShadeModel( GL_SMOOTH );
            if( Global.iMultisampling ) {
                ::glEnable( GL_MULTISAMPLE );
            }
            // setup fog
            if( Global.fFogEnd > 0 ) {
                // fog setup
                m_fogrange = Global.fFogEnd / std::max( 1.f, Global.Overcast * 2.f );
                ::glFogfv( GL_FOG_COLOR, glm::value_ptr( Global.FogColor ) );
                ::glFogf( GL_FOG_DENSITY, static_cast<GLfloat>( 1.0 / m_fogrange ) );
                ::glEnable( GL_FOG );
            }
            else { ::glDisable( GL_FOG ); }

            break;
        }
        case rendermode::shadows:
        case rendermode::cabshadows:
        case rendermode::pickcontrols:
        case rendermode::pickscenery: {
            ::glColor4fv( glm::value_ptr( colors::white ) );
            ::glDisable( GL_LIGHTING );
            ::glShadeModel( GL_FLAT );
            if( Global.iMultisampling ) {
                ::glDisable( GL_MULTISAMPLE );
            }
            ::glDisable( GL_FOG );

            break;
        }
        default: {
            break;
        }
    }
}

// configures, enables and disables specified texture units
void
opengl_renderer::setup_units( bool const Diffuse, bool const Shadows, bool const Reflections ) {
    // helper texture unit.
    // darkens previous stage, preparing data for the shadow texture unit to select from
    if( m_helpertextureunit >= 0 ) {

        select_unit( m_helpertextureunit );

        if( ( true == Reflections )
         || ( ( true == Global.RenderShadows ) && ( true == Shadows ) && ( false == Global.bWireFrame ) ) ) {
            // we need to have texture on the helper for either the reflection and shadow generation (or both)
            if( true == m_environmentcubetexturesupport ) {
                // bind dynamic environment cube if it's enabled...
                // NOTE: environment cube map isn't part of regular texture system, so we use direct bind call here
                ::glBindTexture( GL_TEXTURE_CUBE_MAP, m_environmentcubetexture );
                ::glEnable( GL_TEXTURE_CUBE_MAP );
            }
            else {
                // ...otherwise fallback on static spherical image
                m_textures.bind( textureunit::helper, m_reflectiontexture );
                ::glEnable( GL_TEXTURE_2D );
            }
        }
        else {
            if( true == m_environmentcubetexturesupport ) {
                ::glDisable( GL_TEXTURE_CUBE_MAP );
            }
            else {
                ::glDisable( GL_TEXTURE_2D );
            }
        }

        if( ( true == Global.RenderShadows ) && ( true == Shadows ) && ( false == Global.bWireFrame ) ) {

            ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
            ::glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, glm::value_ptr( m_shadowcolor ) );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE ); // darken the previous stage
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE ); // pass the previous stage alpha down the chain
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
        }
        else {

            ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE ); // pass the previous stage colour down the chain
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE ); // pass the previous stage alpha down the chain
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
        }

        if( true == Reflections ) {
            if( true == m_environmentcubetexturesupport ) {
                ::glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
                ::glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
                ::glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
                ::glEnable( GL_TEXTURE_GEN_S );
                ::glEnable( GL_TEXTURE_GEN_T );
                ::glEnable( GL_TEXTURE_GEN_R );
            }
            else {
                // fixed texture fall back
                ::glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
                ::glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
                ::glEnable( GL_TEXTURE_GEN_S );
                ::glEnable( GL_TEXTURE_GEN_T );
            }
        }
        else {
            if( true == m_environmentcubetexturesupport ) {
                ::glDisable( GL_TEXTURE_GEN_S );
                ::glDisable( GL_TEXTURE_GEN_T );
                ::glDisable( GL_TEXTURE_GEN_R );
            }
            else {
                ::glDisable( GL_TEXTURE_GEN_S );
                ::glDisable( GL_TEXTURE_GEN_T );
            }
        }
    }
    // shadow texture unit.
    // interpolates between primary colour and the previous unit, which should hold darkened variant of the primary colour
    if( m_shadowtextureunit >= 0 ) {
        if( ( true == Global.RenderShadows )
         && ( true == Shadows )
         && ( false == Global.bWireFrame )
         && ( m_shadowcolor != colors::white ) ) {

            select_unit( m_shadowtextureunit );
            // NOTE: texture and transformation matrix setup are done through separate call
            ::glEnable( GL_TEXTURE_2D );
            // s
            ::glEnable( GL_TEXTURE_GEN_S );
            // t
            ::glEnable( GL_TEXTURE_GEN_T );
            // r
            ::glEnable( GL_TEXTURE_GEN_R );
            // q
            ::glEnable( GL_TEXTURE_GEN_Q );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE ); // choose between lit and lit * shadow colour, based on depth test
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE ); // pass the previous stage alpha down the chain
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
        }
        else {
            // turn off shadow map tests
            select_unit( m_shadowtextureunit );

            ::glDisable( GL_TEXTURE_2D );
            ::glDisable( GL_TEXTURE_GEN_S );
            ::glDisable( GL_TEXTURE_GEN_T );
            ::glDisable( GL_TEXTURE_GEN_R );
            ::glDisable( GL_TEXTURE_GEN_Q );
        }
    }
    // reflections/normals texture unit
    // NOTE: comes after diffuse stage in the operation chain
    if( m_normaltextureunit >= 0 ) {

        select_unit( m_normaltextureunit );

        if( true == Reflections ) {
            ::glEnable( GL_TEXTURE_2D );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE ); // blend between object colour and env.reflection
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0 );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE ); // alpha channel of the normal map controls reflection strength
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE ); // pass the previous stage alpha down the chain
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
        }
        else {
            ::glDisable( GL_TEXTURE_2D );
        }
    }
    // diffuse texture unit.
    // NOTE: diffuse texture mapping is never fully disabled, alpha channel information is always included
    select_unit( m_diffusetextureunit );
    ::glEnable( GL_TEXTURE_2D );
    if( true == Diffuse ) {
        // default behaviour, modulate with previous stage
        ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
/*
        ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
*/
    }
    else {
        // solid colour with texture alpha
        ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
/*
        ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE );
        ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
*/
    }
    // update unit state
    m_unitstate.diffuse = Diffuse;
    m_unitstate.shadows = Shadows;
    m_unitstate.reflections = Reflections;
}

// configures shadow texture unit for specified shadow map and conersion matrix
void
opengl_renderer::setup_shadow_map( GLuint const Texture, glm::mat4 const &Transformation ) {

    if( ( m_shadowtextureunit == -1 )
     || ( false == Global.RenderShadows )
     || ( true == Global.bWireFrame )
     || ( m_shadowcolor == colors::white ) ) {
        // shadows are off
        return;
    }

    select_unit( m_shadowtextureunit );
    // NOTE: shadowmap isn't part of regular texture system, so we use direct bind call here
    ::glBindTexture( GL_TEXTURE_2D, Texture );
    // s
    ::glTexGenfv( GL_S, GL_EYE_PLANE, glm::value_ptr( glm::row( Transformation, 0 ) ) );
    // t
    ::glTexGenfv( GL_T, GL_EYE_PLANE, glm::value_ptr( glm::row( Transformation, 1 ) ) );
    // r
    ::glTexGenfv( GL_R, GL_EYE_PLANE, glm::value_ptr( glm::row( Transformation, 2 ) ) );
    // q
    ::glTexGenfv( GL_Q, GL_EYE_PLANE, glm::value_ptr( glm::row( Transformation, 3 ) ) );

    select_unit( m_diffusetextureunit );
}

// enables and disables specified texture units
void
opengl_renderer::switch_units( bool const Diffuse, bool const Shadows, bool const Reflections ) {
    // helper texture unit.
    if( m_helpertextureunit >= 0 ) {

        select_unit( m_helpertextureunit );
        if( ( true == Reflections )
         || ( ( true == Global.RenderShadows )
           && ( true == Shadows )
           && ( false == Global.bWireFrame )
           && ( m_shadowcolor != colors::white ) ) ) {
            if( true == m_environmentcubetexturesupport ) {
                ::glEnable( GL_TEXTURE_CUBE_MAP );
            }
            else {
                ::glEnable( GL_TEXTURE_2D );
            }
        }
        else {
            if( true == m_environmentcubetexturesupport ) {
                ::glDisable( GL_TEXTURE_CUBE_MAP );
            }
            else {
                ::glDisable( GL_TEXTURE_2D );
            }
        }
    }
    // shadow texture unit.
    if( m_shadowtextureunit >= 0 ) {
        if( ( true == Global.RenderShadows ) && ( true == Shadows ) && ( false == Global.bWireFrame ) ) {

            select_unit( m_shadowtextureunit );
            ::glEnable( GL_TEXTURE_2D );
        }
        else {

            select_unit( m_shadowtextureunit );
            ::glDisable( GL_TEXTURE_2D );
        }
    }
    // normal/reflection texture unit
    if( m_normaltextureunit >= 0 ) {
        if( true == Reflections ) {

            select_unit( m_normaltextureunit );
            ::glEnable( GL_TEXTURE_2D );
        }
        else {
            select_unit( m_normaltextureunit );
            ::glDisable( GL_TEXTURE_2D );
        }
    }
    // diffuse texture unit.
    // NOTE: toggle actually disables diffuse texture mapping, unlike setup counterpart
    if( true == Diffuse ) {

        select_unit( m_diffusetextureunit );
        ::glEnable( GL_TEXTURE_2D );
    }
    else {

        select_unit( m_diffusetextureunit );
        ::glDisable( GL_TEXTURE_2D );
    }
    // update unit state
    m_unitstate.diffuse = Diffuse;
    m_unitstate.shadows = Shadows;
    m_unitstate.reflections = Reflections;
}

void
opengl_renderer::setup_shadow_color( glm::vec4 const &Shadowcolor ) {

    select_unit( m_helpertextureunit );
    ::glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, glm::value_ptr( Shadowcolor ) ); // in-shadow colour multiplier
    select_unit( m_diffusetextureunit );
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
    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_DEPTH_TEST );
    ::glDepthMask( GL_FALSE );
    // skydome
    // drawn with 500m radius to blend in if the fog range is low
    ::glPushMatrix();
    ::glScalef( 500.f, 500.f, 500.f );
    m_skydomerenderer.render();
    if( Global.bUseVBO ) {
        gfx::opengl_vbogeometrybank::reset();
    }
    ::glPopMatrix();
    // stars
    if( Environment->m_stars.m_stars != nullptr ) {
        // setup
        ::glPushMatrix();
        ::glRotatef( Environment->m_stars.m_latitude, 1.f, 0.f, 0.f ); // ustawienie osi OY na północ
        ::glRotatef( -std::fmod( (float)Global.fTimeAngleDeg, 360.f ), 0.f, 1.f, 0.f ); // obrót dobowy osi OX
        ::glPointSize( 2.f );
        // render
        Render( Environment->m_stars.m_stars, nullptr, 1.0 );
        // post-render cleanup
        ::glPointSize( 3.f );
        ::glPopMatrix();
    }
    // celestial bodies
    if( DebugModeFlag == true ) {
        // mark sun position for easier debugging
        Environment->m_sun.render();
        Environment->m_moon.render();
    }
    // render actual sun and moon
    ::glPushAttrib( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );

    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_ALPHA_TEST );
    ::glEnable( GL_BLEND );
    ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );

    auto const fogfactor { clamp( Global.fFogEnd / 2000.f, 0.f, 1.f ) }; // closer/denser fog reduces opacity of the celestial bodies
    float const duskfactor = 1.0f - clamp( std::abs( Environment->m_sun.getAngle() ), 0.0f, 12.0f ) / 12.0f;
    glm::vec3 suncolor = interpolate(
        glm::vec3( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f ),
        glm::vec3( 235.0f / 255.0f, 140.0f / 255.0f, 36.0f / 255.0f ),
        duskfactor );
    // sun
    {
        Bind_Texture( m_suntexture );
        ::glColor4f( suncolor.x, suncolor.y, suncolor.z, clamp( 1.5f - Global.Overcast, 0.f, 1.f ) * fogfactor );
        auto const sunvector = Environment->m_sun.getDirection();
        auto const sunposition = modelview * glm::vec4( sunvector.x, sunvector.y, sunvector.z, 1.0f );

        ::glPushMatrix();
        ::glLoadIdentity(); // macierz jedynkowa
        ::glTranslatef( sunposition.x, sunposition.y, sunposition.z ); // początek układu zostaje bez zmian

        float const size = interpolate( // TODO: expose distance/scale factor from the moon object
            0.0325f,
            0.0275f,
            clamp( Environment->m_sun.getAngle(), 0.f, 90.f ) / 90.f );
        ::glBegin( GL_TRIANGLE_STRIP );
        ::glMultiTexCoord2f( m_diffusetextureunit, 1.f, 1.f ); ::glVertex3f( -size,  size, 0.f );
        ::glMultiTexCoord2f( m_diffusetextureunit, 1.f, 0.f ); ::glVertex3f( -size, -size, 0.f );
        ::glMultiTexCoord2f( m_diffusetextureunit, 0.f, 1.f ); ::glVertex3f(  size,  size, 0.f );
        ::glMultiTexCoord2f( m_diffusetextureunit, 0.f, 0.f ); ::glVertex3f(  size, -size, 0.f );
        ::glEnd();

        ::glPopMatrix();
    }
    // moon
    {
        Bind_Texture( m_moontexture );
        glm::vec3 mooncolor( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f );
        ::glColor4f(
            mooncolor.r, mooncolor.g, mooncolor.b,
            // fade the moon if it's near the sun in the sky, especially during the day
            std::max<float>(
                0.f,
                1.0
                - 0.5 * Global.fLuminance
                - 0.65 * std::max( 0.f, glm::dot( Environment->m_sun.getDirection(), Environment->m_moon.getDirection() ) ) )
            * fogfactor );

        auto const moonposition = modelview * glm::vec4( Environment->m_moon.getDirection(), 1.0f );
        ::glPushMatrix();
        ::glLoadIdentity(); // macierz jedynkowa
        ::glTranslatef( moonposition.x, moonposition.y, moonposition.z );

        float const size = interpolate( // TODO: expose distance/scale factor from the moon object
            0.0160f,
            0.0135f,
            clamp( Environment->m_moon.getAngle(), 0.f, 90.f ) / 90.f );
        // choose the moon appearance variant, based on current moon phase
        // NOTE: implementation specific, 8 variants are laid out in 3x3 arrangement
        // from new moon onwards, top left to right bottom (last spot is left for future use, if any)
        auto const moonphase = Environment->m_moon.getPhase();
        float moonu, moonv;
             if( moonphase <  1.84566f ) { moonv = 1.0f - 0.0f;   moonu = 0.0f; }
        else if( moonphase <  5.53699f ) { moonv = 1.0f - 0.0f;   moonu = 0.333f; }
        else if( moonphase <  9.22831f ) { moonv = 1.0f - 0.0f;   moonu = 0.667f; }
        else if( moonphase < 12.91963f ) { moonv = 1.0f - 0.333f; moonu = 0.0f; }
        else if( moonphase < 16.61096f ) { moonv = 1.0f - 0.333f; moonu = 0.333f; }
        else if( moonphase < 20.30228f ) { moonv = 1.0f - 0.333f; moonu = 0.667f; }
        else if( moonphase < 23.99361f ) { moonv = 1.0f - 0.667f; moonu = 0.0f; }
        else if( moonphase < 27.68493f ) { moonv = 1.0f - 0.667f; moonu = 0.333f; }
        else if( moonphase == 50 )       { moonv = 1.0f - 0.667f; moonu = 0.66f; } //9th slot used for easter egg
        else                             { moonv = 1.0f - 0.0f;   moonu = 0.0f; }

        ::glBegin( GL_TRIANGLE_STRIP );
        ::glMultiTexCoord2f( m_diffusetextureunit, moonu, moonv ); ::glVertex3f( -size, size, 0.0f );
        ::glMultiTexCoord2f( m_diffusetextureunit, moonu, moonv - 0.333f ); ::glVertex3f( -size, -size, 0.0f );
        ::glMultiTexCoord2f( m_diffusetextureunit, moonu + 0.333f, moonv ); ::glVertex3f( size, size, 0.0f );
        ::glMultiTexCoord2f( m_diffusetextureunit, moonu + 0.333f, moonv - 0.333f ); ::glVertex3f( size, -size, 0.0f );
        ::glEnd();

        ::glPopMatrix();
    }
    ::glPopAttrib();

    m_sunlight.apply_angle();
    m_sunlight.apply_intensity();

    // clouds
    if( Environment->m_clouds.mdCloud ) {
        // setup
        Disable_Lights();
        ::glEnable( GL_LIGHTING );
        ::glEnable( GL_LIGHT0 ); // other lights will be enabled during lights update
        ::glLightModelfv(
            GL_LIGHT_MODEL_AMBIENT,
            glm::value_ptr(
                interpolate( Environment->m_skydome.GetAverageColor(), suncolor, duskfactor * 0.25f )
                * interpolate( 1.f, 0.35f, Global.Overcast / 2.f ) // overcast darkens the clouds
                * 0.5f // arbitrary adjustment factor
            ) );
        // render
        Render( Environment->m_clouds.mdCloud, nullptr, 100.0 );
        Render_Alpha( Environment->m_clouds.mdCloud, nullptr, 100.0 );
        // post-render cleanup
        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( colors::none ) );
        ::glDisable( GL_LIGHTING );
    }

    ::glDepthMask( GL_TRUE );
    ::glEnable( GL_DEPTH_TEST );
    ::glEnable( GL_LIGHTING );

    return true;
}

// geometry methods
// creates a new geometry bank. returns: handle to the bank or NULL
gfx::geometrybank_handle
opengl_renderer::Create_Bank() {
    if (Global.bUseVBO)
        return m_geometry.register_bank(std::make_unique<gfx::opengl_vbogeometrybank>());
    else
        return m_geometry.register_bank(std::make_unique<gfx::opengl_dlgeometrybank>());
}

// creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle
opengl_renderer::Insert( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) {

    return m_geometry.create_chunk( Indices, Vertices, Geometry, Type );
}

// creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle
opengl_renderer::Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) {

    return m_geometry.create_chunk( Vertices, Geometry, Type );
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
opengl_renderer::Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type, std::size_t const Offset ) {

    return m_geometry.replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
opengl_renderer::Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, int const Type ) {

    return m_geometry.append( Vertices, Geometry );
}

// provides direct access to vertex data of specfied chunk
gfx::index_array const &
opengl_renderer::Indices( gfx::geometry_handle const &Geometry ) const {

    return m_geometry.indices( Geometry );
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

void
opengl_renderer::Bind_Material( material_handle const Material, TSubModel const *sm, lighting_data const *lighting ) {

	if( Material != null_handle ) {

        auto const &material { m_materials.material( Material ) };

        if( false == material.is_good ) {
            // use failure indicator instead
            if( Material != m_invalid_material ) {
                Bind_Material( m_invalid_material );
            }
            return;
        }

        if( false == Global.BasicRenderer ) {
            m_textures.bind( m_normaltextureunit, material.textures[1] );
        }
        m_textures.bind( m_diffusetextureunit, material.textures[0] );
	}
    else {
        // null material, unbind all textures
        if( false == Global.BasicRenderer ) {
            m_textures.bind( m_normaltextureunit, null_handle );
        }
        m_textures.bind( m_diffusetextureunit, null_handle );
    }
}

opengl_material const &
opengl_renderer::Material( material_handle const Material ) const {

    return m_materials.material( Material );
}

// shader methods
std::shared_ptr<gl::program>
opengl_renderer::Fetch_Shader( std::string const &name ) {

    return std::shared_ptr<gl::program>();
}

// texture methods
void
opengl_renderer::select_unit( GLint const Textureunit ) {

    return m_textures.unit( Textureunit );
}

texture_handle
opengl_renderer::Fetch_Texture( std::string const &Filename, bool const Loadnow, GLint format_hint ) {

    return m_textures.create( Filename, Loadnow, GL_RGBA );
}

void
opengl_renderer::Bind_Texture( texture_handle const Texture ) {

    m_textures.bind( m_diffusetextureunit, Texture );
}

void
opengl_renderer::Bind_Texture( std::size_t const Unit, texture_handle const Texture ) {

    m_textures.bind( Unit, Texture );
}

opengl_texture &
opengl_renderer::Texture( texture_handle const Texture ) {

    return m_textures.texture( Texture );
}

opengl_texture const &
opengl_renderer::Texture( texture_handle const Texture ) const {

    return m_textures.texture( Texture );
}

void
opengl_renderer::Pick_Control_Callback( std::function<void( TSubModel const *, const glm::vec2 )> Callback ) {

    m_control_pick_requests.emplace_back( Callback );
}

void
opengl_renderer::Pick_Node_Callback( std::function<void( scene::basic_node * )> Callback ) {

    m_node_pick_requests.emplace_back( Callback );
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
            if( EditorModeFlag ) {
                // when editor mode is active calculate world position of the cursor
                // at this stage the z-buffer is filled with only ground geometry
                Update_Mouse_Position();
            }
            // draw queue is filled while rendering sections
            Render( std::begin( m_cellqueue ), std::end( m_cellqueue ) );
            break;
        }
        case rendermode::shadows:
        case rendermode::cabshadows:
        case rendermode::pickscenery: {
            // these render modes don't bother with lights
            Render( std::begin( m_sectionqueue ), std::end( m_sectionqueue ) );
            // they can also skip queue sorting, as they only deal with opaque geometry
            // NOTE: there's benefit from rendering front-to-back, but is it significant enough? TODO: investigate
            Render( std::begin( m_cellqueue ), std::end( m_cellqueue ) );
            break;
        }
        case rendermode::reflections: {
            Render( std::begin( m_sectionqueue ), std::end( m_sectionqueue ) );
            if( Global.reflectiontune.fidelity >= 1 ) {
                Render(std::begin(m_cellqueue), std::end(m_cellqueue));
            }
            break;
        }
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
        case rendermode::shadows:
        case rendermode::cabshadows: {
            // experimental, for shadows render both back and front faces, to supply back faces of the 'forest strips'
            ::glDisable( GL_CULL_FACE );
            break; }
        case rendermode::pickscenery: {
            // non-interactive scenery elements get neutral colour
            ::glColor3fv( glm::value_ptr( colors::none ) );
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
            case rendermode::cabshadows:
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
            case rendermode::cabshadows:
            case rendermode::pickscenery: {
                // TBD, TODO: refactor into a method to reuse below?
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
            case rendermode::reflections: {
                // we can skip filling the cell queue if reflections pass isn't going to use it
                if( Global.reflectiontune.fidelity == 0 ) { break; }
                for( auto &cell : section->m_cells ) {
                    if( ( true == cell.m_active ) && ( m_renderpass.camera.visible( cell.m_area ) ) ) {
                        // store visible cells with content as well as their current distance, for sorting later
                        m_cellqueue.emplace_back( glm::length2( m_renderpass.camera.position() - cell.m_area.center ), &cell );
                    }
                }
                break;
            }
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }
        // proceed to next section
        ++First;
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        case rendermode::cabshadows: {
            // restore standard face cull mode
            ::glEnable( GL_CULL_FACE );
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
#ifdef EU07_USE_DEBUG_CULLING
                // debug
                ::glLineWidth( 2.f );
                float const width = cell->m_area.radius;
                float const height = cell->m_area.radius * 0.2f;
                glDisable( GL_LIGHTING );
                glDisable( GL_TEXTURE_2D );
                glColor3ub( 255, 128, 128 );
                glBegin( GL_LINE_LOOP );
                glVertex3f( -width, height, width );
                glVertex3f( -width, height, -width );
                glVertex3f( width, height, -width );
                glVertex3f( width, height, width );
                glEnd();
                glBegin( GL_LINE_LOOP );
                glVertex3f( -width, 0, width );
                glVertex3f( -width, 0, -width );
                glVertex3f( width, 0, -width );
                glVertex3f( width, 0, width );
                glEnd();
                glBegin( GL_LINES );
                glVertex3f( -width, height, width ); glVertex3f( -width, 0, width );
                glVertex3f( -width, height, -width ); glVertex3f( -width, 0, -width );
                glVertex3f( width, height, -width ); glVertex3f( width, 0, -width );
                glVertex3f( width, height, width ); glVertex3f( width, 0, width );
                glEnd();
                glColor3ub( 255, 255, 255 );
                glEnable( GL_TEXTURE_2D );
                glEnable( GL_LIGHTING );
                glLineWidth( 1.f );
#endif
                // post-render cleanup
                ::glPopMatrix();

                break;
            }
            case rendermode::reflections:
            {
                // since all shapes of the section share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset{ cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

                // render
                // opaque non-instanced shapes
                for( auto const &shape : cell->m_shapesopaque ) { Render( shape, false ); }

                // post-render cleanup
                ::glPopMatrix();

                break;
            }
            case rendermode::shadows: {
                // since all shapes of the section share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

                // render
                // opaque non-instanced shapes
                for( auto const &shape : cell->m_shapesopaque ) {
                    Render( shape, false );
                }
                // tracks
                Render( std::begin( cell->m_paths ), std::end( cell->m_paths ) );

                // post-render cleanup
                ::glPopMatrix();

                break;
            }
            case rendermode::cabshadows: {
                // since all shapes of the section share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

                // render
                // opaque non-instanced shapes
                for( auto const &shape : cell->m_shapesopaque ) {
                    Render( shape, false );
                }
                // NOTE: tracks aren't likely to cast shadows into the cab, so we skip them in this pass

                // post-render cleanup
                ::glPopMatrix();

                break;
            }
            case rendermode::pickscenery: {
                // same procedure like with regular render, but editor-enabled nodes receive custom colour used for picking
                // since all shapes of the section share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                // render
                // opaque non-instanced shapes
                // non-interactive scenery elements get neutral colour
                ::glColor3fv( glm::value_ptr( colors::none ) );
                for( auto const &shape : cell->m_shapesopaque ) { Render( shape, false ); }
                // tracks
                for( auto *path : cell->m_paths ) {
                    ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                    Render( path );
                }

                // post-render cleanup
                ::glPopMatrix();

                break;
            }
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
            case rendermode::shadows:
            case rendermode::cabshadows: {
                // TBD, TODO: refactor in to a method to reuse in branch below?
                // opaque parts of instanced models
                for( auto *instance : cell->m_instancesopaque ) {
                    Render( instance );
                }
                // opaque parts of vehicles
                for( auto *path : cell->m_paths ) {
                    for( auto *dynamic : path->Dynamics ) {
                        Render( dynamic );
                    }
                }
                // memcells
                if( ( EditorModeFlag )
                 && ( DebugModeFlag ) ) {
                    ::glPushAttrib( GL_ENABLE_BIT );
                    ::glDisable( GL_TEXTURE_2D );
                    ::glColor3f( 0.36f, 0.75f, 0.35f );
                    for( auto *memorycell : cell->m_memorycells ) {
                        Render( memorycell );
                    }
                    ::glPopAttrib();
                }
                break;
            }
            case rendermode::reflections: {
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
            case rendermode::pickscenery: {
                // opaque parts of instanced models
                // same procedure like with regular render, but each node receives custom colour used for picking
                for( auto *instance : cell->m_instancesopaque ) {
                    ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                    Render( instance );
                }
                // memcells
                if( ( EditorModeFlag )
                 && ( DebugModeFlag ) ) {
                    for( auto *memorycell : cell->m_memorycells ) {
                        ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                        Render( memorycell );
                    }
                }
                // vehicles aren't included in scenery picking for the time being
                break;
            }
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }

        ++first;
    }
#ifdef EU07_USE_DEBUG_SOUNDEMITTERS
    // sound emitters
    if( DebugModeFlag ) {
        switch( m_renderpass.draw_mode ) {
            case rendermode::color: {

                ::glPushAttrib( GL_ENABLE_BIT );
                ::glDisable( GL_TEXTURE_2D );
                ::glColor3f( 0.36f, 0.75f, 0.35f );

                for( auto const &audiosource : audio::renderer.m_sources ) {

                    ::glPushMatrix();
                    auto const position = audiosource.properties.location - m_renderpass.camera.position();
                    ::glTranslated( position.x, position.y, position.z );

                    ::gluSphere( m_quadric, 0.1, 4, 2 );

                    ::glPopMatrix();
                }

                ::glPopAttrib();

                break;
            }
            default: {
                break;
            }
        }
    }
#endif

}

void
opengl_renderer::Render( scene::shape_node const &Shape, bool const Ignorerange ) {

    auto const &data { Shape.data() };

    if( false == Ignorerange ) {
        double distancesquared;
        switch( m_renderpass.draw_mode ) {
            case rendermode::shadows:
            case rendermode::cabshadows: {
                // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
                distancesquared = Math3D::SquareMagnitude( ( data.area.center - Global.pCamera.Pos ) / Global.ZoomFactor ) / Global.fDistanceFactor;
                break;
            }
            case rendermode::reflections: {
                // reflection mode draws simplified version of the shapes, by artificially increasing view range
                distancesquared =
                    // TBD, TODO: bind offset value with setting variable?
                    ( EU07_REFLECTIONFIDELITYOFFSET * EU07_REFLECTIONFIDELITYOFFSET )
                    // TBD: take into account distance multipliers?
                    + glm::length2( ( data.area.center - m_renderpass.camera.position() ) ) /* / Global.fDistanceFactor */;
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
        case rendermode::reflections: {
            ::glColor3fv( glm::value_ptr( data.lighting.diffuse ) );
/*
            // NOTE: ambient component is set by diffuse component
            // NOTE: for the time being non-instanced shapes are rendered without specular component due to wrong/arbitrary values set in legacy scenarios
            // TBD, TODO: find a way to resolve this with the least amount of tears?
            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( data.lighting.specular * m_sunlight.specular.a * m_specularopaquescalefactor ) );
*/
            break;
        }
        // pick modes are painted with custom colours, and shadow pass doesn't use any
        case rendermode::shadows:
        case rendermode::cabshadows:
        case rendermode::pickscenery:
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }
    // render
    m_geometry.draw( data.geometry );
    // debug data
	++m_renderpass.draw_stats.shapes;
	++m_renderpass.draw_stats.drawcalls;
}

void
opengl_renderer::Render( TAnimModel *Instance ) {

    if( false == Instance->m_visible ) { return; }
    if( false == m_renderpass.camera.visible( Instance->m_area ) ) { return; }

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        case rendermode::cabshadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
    		distancesquared = glm::length2((Instance->location() - m_renderpass.camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
            break;
        }
        case rendermode::reflections: {
            // reflection mode draws simplified version of the shapes, by artificially increasing view range
            // it also ignores zoom settings and distance multipliers
            // TBD: take into account distance multipliers?
            distancesquared = glm::length2( ( Instance->location() - m_renderpass.camera.position() ) ) /* / Global.fDistanceFactor */;
            // NOTE: arbitrary draw range limit
            if( distancesquared > Global.reflectiontune.range_instances * Global.reflectiontune.range_instances ) {
                return;
            }
            // TBD, TODO: bind offset value with setting variable?
            distancesquared += ( EU07_REFLECTIONFIDELITYOFFSET * EU07_REFLECTIONFIDELITYOFFSET );
            break;
        }
        default: {
    		distancesquared = glm::length2((Instance->location() - m_renderpass.camera.position()) / (double)Global.ZoomFactor) / Global.fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  Instance->m_rangesquaredmin )
     || ( distancesquared >= Instance->m_rangesquaredmax ) ) {
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

    switch( m_renderpass.draw_mode ) {
        case rendermode::pickscenery: {
            // add the node to the pick list
            m_picksceneryitems.emplace_back( Instance );
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
        // debug data
        ++m_renderpass.draw_stats.models;
    }
}

bool
opengl_renderer::Render( TDynamicObject *Dynamic ) {

    Dynamic->renderme = (
        ( Global.pCamera.m_owner == Dynamic && !FreeFlyModeFlag )
        || ( m_renderpass.camera.visible( Dynamic ) ) );

    if( false == Dynamic->renderme ) { return false; }

    // lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
    float squaredistance;
    glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            squaredistance = glm::length2( glm::vec3{ glm::dvec3{ Dynamic->vPosition - Global.pCamera.Pos } } / Global.ZoomFactor );
            if( false == FreeFlyModeFlag ) {
                // filter out small details if we're in vehicle cab
                squaredistance = std::max( 100.f * 100.f, squaredistance );
            }
            break;
        }
        case rendermode::cabshadows: {
            squaredistance = glm::length2( glm::vec3{ glm::dvec3{ Dynamic->vPosition - Global.pCamera.Pos } } / Global.ZoomFactor );
            // filter out small details
            squaredistance = std::max( 100.f * 100.f, squaredistance );
            break;
        }
        case rendermode::reflections: {
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
        default: {
            squaredistance = glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor );
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
    Dynamic->renderme = ( Dynamic->radius() * Global.ZoomFactor / std::sqrt( squaredistance ) > 0.003 );
    if( false == Dynamic->renderme ) {
        return false;
    }

    // debug data
    ++m_renderpass.draw_stats.dynamics;


    // setup
    TSubModel::iInstance = reinterpret_cast<std::uintptr_t>( Dynamic ); //żeby nie robić cudzych animacji
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
            m_renderspecular = false;
            if( Dynamic->fShade > 0.0f ) {
                // restore regular light level
                m_sunlight.apply_intensity();
            }
            break;
        }
        case rendermode::shadows:
        case rendermode::cabshadows: {
            if( Dynamic->mdLowPolyInt ) {
                // low poly interior
//                if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
                    Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
//                }
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
        case rendermode::pickcontrols: {
            if( Dynamic->mdLowPolyInt ) {
                // low poly interior
                Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
            }
            break;
        }
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
opengl_renderer::Render_cab( TDynamicObject const *Dynamic, float const Lightlevel, bool const Alpha ) {

    if( Dynamic == nullptr ) {

        TSubModel::iInstance = 0;
        return false;
    }

    TSubModel::iInstance = reinterpret_cast<std::uintptr_t>( Dynamic );

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
                if( Lightlevel > 0.f ) {
                    // crude way to light the cabin, until we have something more complete in place
                    auto const luminance { Global.fLuminance * ( Dynamic->fShade > 0.0f ? Dynamic->fShade : 1.0f ) };
                    ::glLightModelfv(
                        GL_LIGHT_MODEL_AMBIENT,
                        glm::value_ptr(
                            glm::vec3( m_baseambient )
                            + ( Dynamic->InteriorLight * Lightlevel ) * static_cast<float>( clamp( 1.25 - luminance, 0.0, 1.0 ) ) ) );
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
                if( Lightlevel > 0.f ) {
                    // reset the overall ambient
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(m_baseambient) );
                }
                break;
            }
            case rendermode::cabshadows: {
                // cab shadowmap mode skips lighting setup and translucent parts
                // render
                if( true == Alpha ) {
                    // translucent parts
                    Render_Alpha( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                }
                else {
                    // opaque parts
                    Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                }
                // since the setup is simpler, there's nothing to reset afterwards
                break;
            }
            case rendermode::pickcontrols: {
                // control picking mode skips lighting setup and translucent parts
                // render
                Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                // since the setup is simpler, there's nothing to reset afterwards
                break;
            }
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

    // post-render cleanup

    return true;
}

bool
opengl_renderer::Render( TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotatef( Angle.y, 0.f, 1.f, 0.f );
    if( Angle.x != 0.0 )
        ::glRotatef( Angle.x, 1.f, 0.f, 0.f );
    if( Angle.z != 0.0 )
        ::glRotatef( Angle.z, 0.f, 0.f, 1.f );

    auto const result = Render( Model, Material, Squaredistance );

    ::glPopMatrix();

    return result;
}

void
opengl_renderer::Render( TSubModel *Submodel ) {

    if( ( Submodel->iVisible )
     && ( TSubModel::fSquareDist >= Submodel->fSquareMinDist )
     && ( TSubModel::fSquareDist <  Submodel->fSquareMaxDist ) ) {

        if( Submodel->iFlags & 0xC000 ) {
            ::glPushMatrix();
            if( Submodel->fMatrix )
                ::glMultMatrixf( Submodel->fMatrix->readArray() );
            if( Submodel->b_Anim != TAnimType::at_None )
                Submodel->RaAnimation( Submodel->b_Anim );
        }

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x1F ) {
                // rysuj gdy element nieprzezroczysty

                // debug data
                ++m_renderpass.draw_stats.submodels;
                ++m_renderpass.draw_stats.drawcalls;

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
                        if( true == m_renderspecular ) {
                            ::glEnable( GL_NORMALIZE );
                        }
#endif
                        // material configuration:
                        auto const material{ (
                            Submodel->m_material < 0 ?
                                Submodel->ReplacableSkinId[ -Submodel->m_material ] : // zmienialne skóry
                                Submodel->m_material ) }; // również 0
                        // textures...
                        Bind_Material( material );
                        // ...colors and opacity...
                        auto const opacity { clamp( Material( material ).get_or_guess_opacity(), 0.f, 1.f ) };
                        if( Submodel->fVisible < 1.f ) {
                            // setup
                            ::glAlphaFunc( GL_GREATER, 0.f );
                            ::glEnable( GL_BLEND );
                            ::glColor4f(
                                Submodel->f4Diffuse.r,
                                Submodel->f4Diffuse.g,
                                Submodel->f4Diffuse.b,
                                Submodel->fVisible );
                        }
                        else {
                            ::glColor3fv( glm::value_ptr( Submodel->f4Diffuse ) ); // McZapkie-240702: zamiast ub
                            if( ( opacity != 0.f )
                             && ( opacity != EU07_OPACITYDEFAULT ) ) {
                                ::glAlphaFunc( GL_GREATER, opacity );
                            }
                        }
                        // ...specular...
                        if( ( true == m_renderspecular ) && ( m_sunlight.specular.a > 0.01f ) ) {
                            // specular strength in legacy models is set uniformly to 150, 150, 150 so we scale it down for opaque elements
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * m_sunlight.specular.a * m_specularopaquescalefactor ) );
                            ::glEnable( GL_RESCALE_NORMAL );
                        }
                        // ...luminance
                        auto const unitstate = m_unitstate;
                        auto const isemissive { ( Submodel->f4Emision.a > 0.f ) && ( Global.fLuminance < Submodel->fLight ) };
                        if( isemissive ) {
                            // zeby swiecilo na kolorowo
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( /* Submodel->f4Emision */ Submodel->f4Diffuse * Submodel->f4Emision.a ) );
                            // disable shadows so they don't obstruct self-lit items
/*
                            setup_shadow_color( colors::white );
*/
                            switch_units( unitstate.diffuse, false, false );
                        }

                        // main draw call
                        m_geometry.draw( Submodel->m_geometry.handle );
/*
                        if( DebugModeFlag ) {
                            auto const & vertices { m_geometry.vertices( Submodel->m_geometry ) };
                            ::glBegin( GL_LINES );
                            for( auto const &vertex : vertices ) {
                                ::glVertex3fv( glm::value_ptr( vertex.position ) );
                                ::glVertex3fv( glm::value_ptr( vertex.position + vertex.normal * 0.2f ) );
                            }
                            ::glEnd();
                        }
*/
                        // post-draw reset
                        if( Submodel->fVisible < 1.f ) {
                            ::glAlphaFunc( GL_GREATER, EU07_OPACITYDEFAULT );
                            ::glDisable( GL_BLEND );
                        }
                        else if( ( opacity != 0.f )
                              && ( opacity != EU07_OPACITYDEFAULT ) ) {
                            ::glAlphaFunc( GL_GREATER, EU07_OPACITYDEFAULT );
                        }
                        if( ( true == m_renderspecular ) && ( m_sunlight.specular.a > 0.01f ) ) {
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( colors::none ) );
                        }
                        if( isemissive ) {
                            // restore default (lack of) brightness
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( colors::none ) );
/*
                            setup_shadow_color( m_shadowcolor );
*/
                            switch_units( unitstate.diffuse, unitstate.shadows, unitstate.reflections );
                        }
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
                        if( true == m_renderspecular ) {
                            ::glDisable( GL_NORMALIZE );
                        }
#endif
                        break;
                    }
                    case rendermode::shadows:
                    case rendermode::cabshadows:
                    case rendermode::pickscenery: {
                        // scenery picking and shadow both use enforced colour and no frills
                        // material configuration:
                        // textures...
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            // również 0
                            Bind_Material( Submodel->m_material );
                        }
                        // main draw call
                        m_geometry.draw( Submodel->m_geometry.handle );
                        // post-draw reset
                        break;
                    }
                    case rendermode::pickcontrols: {
                        // material configuration:
                        // control picking applies individual colour for each submodel
                        m_pickcontrolsitems.emplace_back( Submodel );
                        ::glColor3fv( glm::value_ptr( pick_color( m_pickcontrolsitems.size() ) ) );
                        // textures...
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            // również 0
                            Bind_Material( Submodel->m_material );
                        }
                        // main draw call
                        m_geometry.draw( Submodel->m_geometry.handle );
                        // post-draw reset
                        break;
                    }
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
                        lightlevel *= anglefactor;
                        // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                        // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                        float const distancefactor { std::max( 0.5f, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / Submodel->fSquareMaxDist ) };
                        auto const pointsize { std::max( 3.f, 5.f * distancefactor * anglefactor ) };
                        auto const resolutionratio { Global.iWindowHeight / 1080.f };
                        // additionally reduce light strength for farther sources in rain or snow
                        if( Global.Overcast > 0.75f ) {
                            float const precipitationfactor{
                                interpolate(
                                    interpolate( 1.f, 0.25f, clamp( Global.Overcast * 0.75f - 0.5f, 0.f, 1.f ) ),
                                    1.f,
                                distancefactor ) };
                            lightlevel *= precipitationfactor;
                        }

                        if( lightlevel > 0.f ) {
                            // material configuration:
                            Bind_Material( null_handle );
                            // limit impact of dense fog on the lights
                            auto const lightrange { std::max<float>( 500, m_fogrange * 2 ) }; // arbitrary, visibility at least 500m
                            ::glFogf( GL_FOG_DENSITY, static_cast<GLfloat>( 1.0 / lightrange ) );

                            ::glPushAttrib( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT );
                            ::glDisable( GL_LIGHTING );
                            ::glEnable( GL_BLEND );
                            ::glAlphaFunc( GL_GREATER, 0.f );

                            ::glPushMatrix();
                            ::glLoadIdentity();
                            ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian

                            auto const unitstate = m_unitstate;
                            switch_units( m_unitstate.diffuse, false, false );

                            auto const *lightcolor {
                                  Submodel->DiffuseOverride.r < 0.f ? // -1 indicates no override
                                    glm::value_ptr( Submodel->f4Diffuse ) :
                                    glm::value_ptr( Submodel->DiffuseOverride ) };

                            // main draw call
                            if( Global.Overcast > 1.f ) {
                                // fake fog halo
                                float const fogfactor {
                                    interpolate(
                                        2.f, 1.f,
                                        clamp( Global.fFogEnd / 2000, 0.f, 1.f ) )
                                    * std::max( 1.f, Global.Overcast ) };

                                ::glPointSize( pointsize * resolutionratio * fogfactor );
                                ::glColor4f(
                                    lightcolor[ 0 ],
                                    lightcolor[ 1 ],
                                    lightcolor[ 2 ],
                                    Submodel->fVisible * std::min( 1.f, lightlevel ) * 0.5f );
                                ::glDepthMask( GL_FALSE );
                                m_geometry.draw( Submodel->m_geometry.handle );
                                ::glDepthMask( GL_TRUE );
                            }
                            ::glPointSize( pointsize * resolutionratio );
                            ::glColor4f(
                                lightcolor[ 0 ],
                                lightcolor[ 1 ],
                                lightcolor[ 2 ],
                                Submodel->fVisible * std::min( 1.f, lightlevel ) );
                            m_geometry.draw( Submodel->m_geometry.handle );

                            // post-draw reset
                            switch_units( unitstate.diffuse, unitstate.shadows, unitstate.reflections );

                            ::glPopMatrix();
                            ::glPopAttrib();
                            ::glFogf( GL_FOG_DENSITY, static_cast<GLfloat>( 1.0 / m_fogrange ) );
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

                        // material configuration:
                        ::glPushAttrib( GL_ENABLE_BIT );

                        Bind_Material( null_handle );
                        ::glDisable( GL_LIGHTING );

                        // main draw call
                        m_geometry.draw( Submodel->m_geometry.handle, gfx::color_streams );

                        // post-draw reset
                        ::glPopAttrib();
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
     && ( Track->m_material2 == 0 )
     && ( ( Track->eType != tt_Switch )
       || ( Track->SwitchExtension->m_material3 == 0 ) ) ) {
        return;
    }
    if( false == Track->m_visible ) {
        return;
    }

    ++m_renderpass.draw_stats.paths;
    ++m_renderpass.draw_stats.drawcalls;

    switch( m_renderpass.draw_mode ) {
        // single path pieces are rendererd in pick scenery mode only
        case rendermode::pickscenery: {
            // add the node to the pick list
            m_picksceneryitems.emplace_back( Track );

            if( Track->m_material1 != 0 ) {
                Bind_Material( Track->m_material1 );
                m_geometry.draw( std::begin( Track->Geometry1 ), std::end( Track->Geometry1 ) );
            }
            if( Track->m_material2 != 0 ) {
                Bind_Material( Track->m_material2 );
                m_geometry.draw( std::begin( Track->Geometry2 ), std::end( Track->Geometry2 ) );
            }
            if( ( Track->eType == tt_Switch )
             && ( Track->SwitchExtension->m_material3 != 0 ) ) {
                Bind_Material( Track->SwitchExtension->m_material3 );
                m_geometry.draw( Track->SwitchExtension->Geometry3 );
            }
            break;
        }
        default: {
            break;
        }
    }
}

// experimental, does track rendering in two passes, to take advantage of reduced texture switching
void
opengl_renderer::Render( scene::basic_cell::path_sequence::const_iterator First, scene::basic_cell::path_sequence::const_iterator Last ) {

    ::glColor3fv( glm::value_ptr( colors::white ) );
    // setup
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows:
        case rendermode::cabshadows: {
            // NOTE: roads-based platforms tend to miss parts of shadows if rendered with either back or front culling
            ::glDisable( GL_CULL_FACE );
            break;
        }
        default: {
            break;
        }
    }

    // TODO: render auto generated trackbeds together with regular trackbeds in pass 1, and all rails in pass 2
    // first pass, material 1
    for( auto first { First }; first != Last; ++first ) {

        auto const track { *first };

        if( track->m_material1 == 0 )  {
            continue;
        }
        if( false == track->m_visible ) {
            continue;
        }

        ++m_renderpass.draw_stats.paths;
        ++m_renderpass.draw_stats.drawcalls;

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
            case rendermode::shadows:
            case rendermode::cabshadows: {
                if( ( std::abs( track->fTexHeight1 ) < 0.35f )
                 || ( track->iCategoryFlag != 2 ) ) {
                    // shadows are only calculated for high enough roads, typically meaning track platforms
                    continue;
                }
                Bind_Material( track->m_material1 );
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
            case rendermode::shadows:
            case rendermode::cabshadows: {
                if( ( std::abs( track->fTexHeight1 ) < 0.35f )
                 || ( ( track->iCategoryFlag == 1 )
                   && ( track->eType != tt_Normal ) ) ) {
                    // shadows are only calculated for high enough trackbeds
                    continue;
                }
                Bind_Material( track->m_material2 );
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
    // third pass, material 3
    for( auto first { First }; first != Last; ++first ) {

        auto const track { *first };

        if( track->eType != tt_Switch ) {
            continue;
        }
        if( track->SwitchExtension->m_material3 == 0 ) {
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
                Bind_Material( track->SwitchExtension->m_material3 );
                m_geometry.draw( track->SwitchExtension->Geometry3 );
                if( track->eEnvironment != e_flat ) {
                    // restore default lighting
                    setup_environment_light();
                }
                break;
            }
            case rendermode::shadows:
            case rendermode::cabshadows: {
                if( ( std::abs( track->fTexHeight1 ) < 0.35f )
                 || ( ( track->iCategoryFlag == 1 )
                   && ( track->eType != tt_Normal ) ) ) {
                    // shadows are only calculated for high enough trackbeds
                    continue;
                }
                Bind_Material( track->SwitchExtension->m_material3 );
                m_geometry.draw( track->SwitchExtension->Geometry3 );
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
        case rendermode::shadows:
        case rendermode::cabshadows: {
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
        case rendermode::color:
        case rendermode::shadows:
        case rendermode::cabshadows: {
            //::gluSphere( m_quadric, 0.35, 4, 2 );
            break;
        }
        case rendermode::pickscenery: {
            // add the node to the pick list
            m_picksceneryitems.emplace_back( Memcell );

            //::gluSphere( m_quadric, 0.35, 4, 2 );
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
opengl_renderer::Render_particles() {

    switch_units( true, false, false );

    Bind_Material( null_handle ); // TODO: bind smoke texture

    // TBD: leave lighting on to allow vehicle lights to affect it?
    ::glDisable( GL_LIGHTING );
    // momentarily disable depth write, to allow vehicle cab drawn afterwards to mask it instead of leaving it 'inside'
    ::glDepthMask( GL_FALSE );

    Bind_Texture( m_smoketexture );
    m_renderpass.draw_stats.particles = m_particlerenderer.render( m_diffusetextureunit );
    if( Global.bUseVBO ) {
        gfx::opengl_vbogeometrybank::reset();
    }
    ::glDepthMask( GL_TRUE );
    ::glEnable( GL_LIGHTING );
}

void
opengl_renderer::Render_precipitation() {

    if( Global.Overcast <= 1.f ) { return; }

    switch_units( true, false, false );

//    ::glColor4fv( glm::value_ptr( glm::vec4( glm::min( glm::vec3( Global.fLuminance ), glm::vec3( 1 ) ), 1 ) ) );
    ::glColor4fv(
        glm::value_ptr(
            interpolate(
                0.5f * ( Global.DayLight.diffuse + Global.DayLight.ambient ),
                colors::white,
                0.5f * clamp<float>( Global.fLuminance, 0.f, 1.f ) ) ) );
    ::glPushMatrix();
    // tilt the precipitation cone against the camera movement vector for crude motion blur
    // include current wind vector while at it
    auto const velocity {
        simulation::Environment.m_precipitation.m_cameramove * -1.0
        + glm::dvec3{ simulation::Environment.wind() } * 0.5 };
    if( glm::length2( velocity ) > 0.0 ) {
        auto const forward{ glm::normalize( velocity ) };
        auto left { glm::cross( forward, {0.0,1.0,0.0} ) };
        auto const rotationangle {
            std::min(
                45.0,
                ( FreeFlyModeFlag ?
                    5 * glm::length( velocity ) :
                    simulation::Train->Dynamic()->GetVelocity() * 0.2 ) ) };
        ::glRotated( rotationangle, left.x, 0.0, left.z );
    }
    if( false == FreeFlyModeFlag ) {
        // counter potential vehicle roll
        auto const roll { 0.5 * glm::degrees( simulation::Train->Dynamic()->Roll() ) };
        if( roll != 0.0 ) {
            auto const forward { simulation::Train->Dynamic()->VectorFront() };
            auto const vehicledirection = simulation::Train->Dynamic()->DirectionGet();
            ::glRotated( roll, forward.x, 0.0, forward.z );
        }
    }
    if( Global.Weather == "rain:" ) {
        // oddly enough random streaks produce more natural looking rain than ones the eye can follow
        ::glRotated( LocalRandom() * 360, 0.0, 1.0, 0.0 );
    }

    // TBD: leave lighting on to allow vehicle lights to affect it?
    ::glDisable( GL_LIGHTING );
    // momentarily disable depth write, to allow vehicle cab drawn afterwards to mask it instead of leaving it 'inside'
    ::glDepthMask( GL_FALSE );

    m_precipitationrenderer.render( m_diffusetextureunit );
    if( Global.bUseVBO ) {
        gfx::opengl_vbogeometrybank::reset();
    }

    ::glDepthMask( GL_TRUE );
    ::glEnable( GL_LIGHTING );
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
                    if( ( false == FreeFlyModeFlag )
                     && ( simulation::Train->Dynamic() == dynamic ) ) {
                        // delay drawing translucent parts of occupied vehicle until after the cab was drawn
                        continue;
                    }
                    Render_Alpha( dynamic );
                }
            }

            ++first;
        }
    }
    // third pass draws the wires;
    // wires use section vbos, but for the time being we want to draw them at the very end
    {
        ::glDisable( GL_LIGHTING ); // linie nie powinny świecić

        auto first{ First };
        while( first != Last ) {

            auto const *cell = first->second;

            if( ( false == cell->m_traction.empty()
             || ( false == cell->m_lines.empty() ) ) ) {
                // since all shapes of the cell share center point we can optimize out a few calls here
                ::glPushMatrix();
                auto const originoffset { cell->m_area.center - m_renderpass.camera.position() };
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                if( !Global.bSmoothTraction ) {
                    // na liniach kiepsko wygląda - robi gradient
                    ::glDisable( GL_LINE_SMOOTH );
                }
                Bind_Material( null_handle );
                // render
                for( auto *traction : cell->m_traction ) { Render_Alpha( traction ); }
                for( auto &lines : cell->m_lines )       { Render_Alpha( lines ); }
                // post-render cleanup
                ::glLineWidth( 1.0 );
                if( !Global.bSmoothTraction ) {
                    ::glEnable( GL_LINE_SMOOTH );
                }
                ::glPopMatrix();
            }

            ++first;
        }

        ::glEnable( GL_LIGHTING );
    }
}

void
opengl_renderer::Render_Alpha( TAnimModel *Instance ) {

	if (false == Instance->m_visible) { return; }
    if( false == m_renderpass.camera.visible( Instance->m_area ) ) { return; }

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = Math3D::SquareMagnitude( ( Instance->location() - Global.pCamera.Pos ) / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
        default: {
            distancesquared = glm::length2( ( Instance->location() - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  Instance->m_rangesquaredmin )
     || ( distancesquared >= Instance->m_rangesquaredmax ) ) {
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
/*
// NOTE: test disabled as this call is only executed as part of the colour pass
    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = Math3D::SquareMagnitude( ( Traction->location() - Global.pCamera.Pos ) / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
        default: {
            distancesquared = glm::length2( ( Traction->location() - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
*/
    auto const distancesquared { glm::length2( ( Traction->location() - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor };
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
/*
    float const linealpha = static_cast<float>(
        std::min(
            1.25,
            5000 * Traction->WireThickness / ( distancesquared + 1.0 ) ) ); // zbyt grube nie są dobre
    ::glLineWidth( linealpha );
*/
    auto const distance { static_cast<float>( std::sqrt( distancesquared ) ) };
    auto const linealpha {
        20.f * Traction->WireThickness
        / std::max(
            0.5f * Traction->radius() + 1.f,
            distance - ( 0.5f * Traction->radius() ) ) };
    ::glLineWidth(
        clamp(
            0.5f * linealpha + Traction->WireThickness * Traction->radius() / 1000.f,
            1.f, 1.75f ) );
    // McZapkie-261102: kolor zalezy od materialu i zasniedzenia
    ::glColor4fv(
        glm::value_ptr(
            glm::vec4{
                Traction->wire_color() /* * ( DebugModeFlag ? 1.f : clamp( m_sunandviewangle, 0.25f, 1.f ) ) */,
                linealpha } ) );
    // render
    m_geometry.draw( Traction->m_geometry );
    // debug data
    ++m_renderpass.draw_stats.traction;
    ++m_renderpass.draw_stats.drawcalls;
}

void
opengl_renderer::Render_Alpha( scene::lines_node const &Lines ) {

    auto const &data { Lines.data() };
/*
    // NOTE: test disabled as this call is only executed as part of the colour pass
    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = Math3D::SquareMagnitude( ( data.area.center - Global.pCamera.Pos ) / Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
        default: {
            distancesquared = glm::length2( ( data.area.center - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor;
            break;
        }
    }
*/
    auto const distancesquared { glm::length2( ( data.area.center - m_renderpass.camera.position() ) / (double)Global.ZoomFactor ) / Global.fDistanceFactor };
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
    ::glLineWidth(
        clamp(
            0.5f * linealpha + data.line_width * data.area.radius / 1000.f,
            1.f, 8.f ) );
    ::glColor4fv(
        glm::value_ptr(
            glm::vec4{
                glm::vec3{ data.lighting.diffuse * m_sunlight.ambient }, // w zaleznosci od koloru swiatla
                std::min( 1.f, linealpha ) } ) );
    // render
    m_geometry.draw( data.geometry );
    ++m_renderpass.draw_stats.lines;
    ++m_renderpass.draw_stats.drawcalls;
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
        case rendermode::shadows: {
            squaredistance = glm::length2( glm::vec3{ glm::dvec3{ Dynamic->vPosition - Global.pCamera.Pos } } / Global.ZoomFactor );
            break;
        }
        default: {
            squaredistance = glm::length2( glm::vec3{ originoffset } / Global.ZoomFactor );
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
//        if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
/*
            // enable cab light if needed
            if( Dynamic->InteriorLightLevel > 0.0f ) {

                // crude way to light the cabin, until we have something more complete in place
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( Dynamic->InteriorLight * Dynamic->InteriorLightLevel ) );
            }
*/
            Render_Alpha( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );
/*
            if( Dynamic->InteriorLightLevel > 0.0f ) {
                // reset the overall ambient
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( m_baseambient ) );
            }
*/
//        }
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
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, float const Squaredistance, Math3D::vector3 const &Position, glm::vec3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotatef( Angle.y, 0.f, 1.f, 0.f );
    if( Angle.x != 0.0 )
        ::glRotatef( Angle.x, 1.f, 0.f, 0.f );
    if( Angle.z != 0.0 )
        ::glRotatef( Angle.z, 0.f, 0.f, 1.f );

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

        if( Submodel->iFlags & 0xC000 ) {
            ::glPushMatrix();
            if( Submodel->fMatrix )
                ::glMultMatrixf( Submodel->fMatrix->readArray() );
            if( Submodel->b_aAnim != TAnimType::at_None )
                Submodel->RaAnimation( Submodel->b_aAnim );
        }

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x2F ) {
                // rysuj gdy element przezroczysty

                // debug data
                ++m_renderpass.draw_stats.submodels;
                ++m_renderpass.draw_stats.drawcalls;

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
                        if( true == m_renderspecular ) {
                            ::glEnable( GL_NORMALIZE );
                        }
#endif
                        // material configuration:
                        // textures...
                        auto const material { (
                            Submodel->m_material < 0 ?
                                Submodel->ReplacableSkinId[ -Submodel->m_material ] : // zmienialne skóry
                                Submodel->m_material ) }; // również 0
                        // textures...
                        Bind_Material( material );
                        // ...colors and opacity...
                        auto const opacity { clamp( Material( material ).get_or_guess_opacity(), 0.f, 1.f ) };
                        if( Submodel->fVisible < 1.f ) {
                            ::glColor4f(
                                Submodel->f4Diffuse.r,
                                Submodel->f4Diffuse.g,
                                Submodel->f4Diffuse.b,
                                Submodel->fVisible );
                        }
                        else {
                            ::glColor3fv( glm::value_ptr( Submodel->f4Diffuse ) ); // McZapkie-240702: zamiast ub
                        }
                        if( opacity != 0.f ) {
                            // discard fragments with opacity exceeding the threshold
                            ::glAlphaFunc( GL_LEQUAL, opacity );
                        }
                        if( ( true == m_renderspecular ) && ( m_sunlight.specular.a > 0.01f ) ) {
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * m_sunlight.specular.a * m_speculartranslucentscalefactor ) );
                        }
                        // ...luminance
                        auto const unitstate = m_unitstate;
                        auto const isemissive { ( Submodel->f4Emision.a > 0.f ) && ( Global.fLuminance < Submodel->fLight ) };
                        if( isemissive ) {
                            // zeby swiecilo na kolorowo
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( /* Submodel->f4Emision */ Submodel->f4Diffuse * Submodel->f4Emision.a ) );
                            // disable shadows so they don't obstruct self-lit items
/*
                            setup_shadow_color( colors::white );
*/
                            switch_units( unitstate.diffuse, false, false );
                        }

                        // main draw call
                        m_geometry.draw( Submodel->m_geometry.handle );

                        // post-draw reset
                        if( opacity != 0.f ) {
                            // discard fragments with opacity exceeding the threshold
                            ::glAlphaFunc( GL_GREATER, 0.0f );
                        }
                        if( ( true == m_renderspecular ) && ( m_sunlight.specular.a > 0.01f ) ) {
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( colors::none ) );
                        }
                        if( isemissive ) {
                            // restore default (lack of) brightness
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( colors::none ) );
/*
                            setup_shadow_color( m_shadowcolor );
*/
                            switch_units( unitstate.diffuse, unitstate.shadows, unitstate.reflections );
                        }
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
                        if( true == m_renderspecular ) {
                            ::glDisable( GL_NORMALIZE );
                        }
#endif
                        break;
                    }
                    case rendermode::cabshadows: {
                        // scenery picking and shadow both use enforced colour and no frills
                        // material configuration:
                        // textures...
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            // również 0
                            Bind_Material( Submodel->m_material );
                        }
                        // main draw call
                        m_geometry.draw( Submodel->m_geometry.handle );
                        // post-draw reset
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            if( ( Global.fLuminance < Submodel->fLight ) || ( Global.Overcast > 1.f ) ) {
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

                if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) {
                    // only bother if the viewer is inside the visibility cone
                    // luminosity at night is at level of ~0.1, so the overall resulting transparency in clear conditions is ~0.5 at full 'brightness'
                    auto glarelevel { clamp(
                        std::max<float>(
                            0.6f - Global.fLuminance, // reduce the glare in bright daylight
                            Global.Overcast - 1.f ), // ensure some glare in rainy/foggy conditions
                        0.f, 1.f ) };
                    // view angle attenuation
                    float const anglefactor { clamp(
                        ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( Submodel->fCosHotspotAngle - Submodel->fCosFalloffAngle ),
                        0.f, 1.f ) };
                    glarelevel *= anglefactor;

                    if( glarelevel > 0.0f ) {
                        // setup
                        ::glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT );

                        Bind_Texture( m_glaretexture );
                        ::glDisable( GL_LIGHTING );
                        ::glDisable( GL_FOG );
                        ::glDepthMask( GL_FALSE );
                        ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                        ::glPushMatrix();
                        ::glLoadIdentity(); // macierz jedynkowa
                        ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
                        ::glRotated( std::atan2( lightcenter.x, lightcenter.z ) * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // jedynie obracamy w pionie o kąt
                                                                                                                 // disable shadows so they don't obstruct self-lit items
                        auto const unitstate = m_unitstate;
                        switch_units( unitstate.diffuse, false, false );

                        auto const *lightcolor {
                            Submodel->DiffuseOverride.r < 0.f ? // -1 indicates no override
                                glm::value_ptr( Submodel->f4Diffuse ) :
                                glm::value_ptr( Submodel->DiffuseOverride ) };
                        ::glColor4f(
                            lightcolor[ 0 ],
                            lightcolor[ 1 ],
                            lightcolor[ 2 ],
                            Submodel->fVisible * glarelevel );

                        // main draw call
                        m_geometry.draw( m_billboardgeometry );
/*
                        // NOTE: we could do simply...
                        vec3 vertexPosition_worldspace =
                        particleCenter_wordspace
                        + CameraRight_worldspace * squareVertices.x * BillboardSize.x
                        + CameraUp_worldspace * squareVertices.y * BillboardSize.y;
                        // ...etc instead IF we had easy access to camera's forward and right vectors. TODO: check if Camera matrix is accessible
*/
                        // post-render cleanup
                        switch_units( unitstate.diffuse, unitstate.shadows, unitstate.reflections );

                        ::glPopMatrix();
                        ::glPopAttrib();
                    }
                }
            }
        }

        if( Submodel->Child != nullptr ) {
            if( Submodel->eType == TP_TEXT ) { // tekst renderujemy w specjalny sposób, zamiast submodeli z łańcucha Child
                int i, j = (int)Submodel->pasText->size();
                TSubModel *p;
                if( !Submodel->smLetter ) { // jeśli nie ma tablicy, to ją stworzyć; miejsce nieodpowiednie, ale tymczasowo może być
                    Submodel->smLetter = new TSubModel *[ 256 ]; // tablica wskaźników submodeli dla wyświetlania tekstu
                    std::memset(Submodel->smLetter, 0, 256 * sizeof( TSubModel * )); // wypełnianie zerami
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
void
opengl_renderer::Update_Pick_Control() {

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    if( true == m_framebuffersupport ) {
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_pickframebuffer );
    }
#endif
    Render_pass( rendermode::pickcontrols );
    // determine point to examine
    glm::dvec2 mousepos;
    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );
    mousepos.x *= Global.fWindowXScale;
    mousepos.y *= Global.fWindowYScale;
    mousepos.y = Global.iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    glm::ivec2 pickbufferpos;
    if( true == m_framebuffersupport ) {
//        ::glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
        pickbufferpos = glm::ivec2{
            mousepos.x * EU07_PICKBUFFERSIZE / std::max( 1, Global.iWindowWidth),
            mousepos.y * EU07_PICKBUFFERSIZE / std::max( 1, Global.iWindowHeight) };
    }
    else {
//        ::glReadBuffer( GL_BACK );
        pickbufferpos = glm::ivec2{ mousepos };
    }
#else
//    ::glReadBuffer( GL_BACK );
    glm::ivec2 pickbufferpos{ mousepos };
#endif
    unsigned char pickreadout[4];
    ::glReadPixels( pickbufferpos.x, pickbufferpos.y, 1, 1, GL_BGRA, GL_UNSIGNED_BYTE, pickreadout );
    auto const controlindex = pick_index( glm::ivec3{ pickreadout[ 2 ], pickreadout[ 1 ], pickreadout[ 0 ] } );
    TSubModel *control { nullptr };
    if( ( controlindex > 0 )
     && ( controlindex <= m_pickcontrolsitems.size() ) ) {
        control = m_pickcontrolsitems[ controlindex - 1 ];
    }
#ifdef EU07_USE_PICKING_FRAMEBUFFER
    if( true == m_framebuffersupport ) {
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
    }
#endif
    m_pickcontrolitem = control;
//    return control;
    for( auto callback : m_control_pick_requests ) {
        callback( m_pickcontrolitem, glm::vec2() );
    }
    m_control_pick_requests.clear();
}

void
opengl_renderer::Update_Pick_Node() {

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    if( true == m_framebuffersupport ) {
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_pickframebuffer );
    }
#endif
    Render_pass( rendermode::pickscenery );
    // determine point to examine
    glm::dvec2 mousepos;
    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );
    mousepos.y = Global.iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    glm::ivec2 pickbufferpos;
    if( true == m_framebuffersupport ) {
//        ::glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
        pickbufferpos = glm::ivec2{
            mousepos.x * EU07_PICKBUFFERSIZE / Global.iWindowWidth,
            mousepos.y * EU07_PICKBUFFERSIZE / Global.iWindowHeight
        };
    }
    else {
//        ::glReadBuffer( GL_BACK );
        pickbufferpos = glm::ivec2{ mousepos };
    }
#else
//    ::glReadBuffer( GL_BACK );
    glm::ivec2 pickbufferpos{ mousepos };
#endif

    unsigned char pickreadout[4];
    ::glReadPixels( pickbufferpos.x, pickbufferpos.y, 1, 1, GL_BGRA, GL_UNSIGNED_BYTE, pickreadout );
    auto const nodeindex = pick_index( glm::ivec3{ pickreadout[ 2 ], pickreadout[ 1 ], pickreadout[ 0 ] } );
    scene::basic_node *node { nullptr };
    if( ( nodeindex > 0 )
     && ( nodeindex <= m_picksceneryitems.size() ) ) {
        node = m_picksceneryitems[ nodeindex - 1 ];
    }
#ifdef EU07_USE_PICKING_FRAMEBUFFER
    if( true == m_framebuffersupport ) {
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
    }
#endif
    m_picksceneryitem = node;
//    return node;
    for( auto callback : m_node_pick_requests ) {
        callback( m_picksceneryitem );
    }
    m_node_pick_requests.clear();
}

// converts provided screen coordinates to world coordinates of most recent color pass
glm::dvec3
opengl_renderer::Update_Mouse_Position() {

    glm::dvec2 mousepos;
    Application.get_cursor_pos( mousepos.x, mousepos.y );
//    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );
    mousepos.x = clamp<int>( mousepos.x, 0, Global.iWindowWidth - 1 );
    mousepos.y = clamp<int>( Global.iWindowHeight - clamp<int>( mousepos.y, 0, Global.iWindowHeight ), 0, Global.iWindowHeight - 1 ) ;
    GLfloat pointdepth;
    ::glReadPixels( mousepos.x, mousepos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &pointdepth );

    if( pointdepth < 1.0 ) {
        m_worldmousecoordinates =
            glm::unProject(
                glm::vec3{ mousepos, pointdepth },
                glm::mat4{ glm::mat3{ m_colorpass.camera.modelview() } },
                m_colorpass.camera.projection(),
                glm::vec4{ 0, 0, Global.iWindowWidth, Global.iWindowHeight } );
    }

    return m_colorpass.camera.position() + glm::dvec3{ m_worldmousecoordinates };
}

void
opengl_renderer::Update( double const Deltatime ) {

    // per frame updates
    if( simulation::is_ready ) {
        // update particle subsystem
        renderpass_config renderpass;
        setup_pass( renderpass, rendermode::color );
        m_skydomerenderer.update();
        m_precipitationrenderer.update();
        m_particlerenderer.update( renderpass.camera );
    }

    // fixed step updates
/*
    m_pickupdateaccumulator += Deltatime;

    if( m_updateaccumulator > 0.5 ) {

        m_pickupdateaccumulator = 0.0;

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
    }
*/
    m_updateaccumulator += Deltatime;

    if( ( true  == Global.ControlPicking )
     && ( false == FreeFlyModeFlag ) ) {
        if( ( false == m_control_pick_requests.empty() )
         || ( m_updateaccumulator >= 1.0 ) ) {
            Update_Pick_Control();
        }
    }
    else {
        m_pickcontrolitem = nullptr;
    }
    // temporary conditions for testing. eventually will be coupled with editor mode
    if( ( true == Global.ControlPicking )
     && ( true == FreeFlyModeFlag )
     && ( ( true == DebugModeFlag )
       || ( true == EditorModeFlag ) ) ) {
        if( ( false == m_node_pick_requests.empty() )
         || ( m_updateaccumulator >= 1.0 ) ) {
            Update_Pick_Node();
        }
    }
    else {
        m_picksceneryitem = nullptr;
    }

    if( m_updateaccumulator < 1.0 ) {
        // too early for any work
        return;
    }

    m_updateaccumulator = 0.0;
    m_framerate = 1000.f / ( Timer::subsystem.gfx_total.average() );

    // adjust draw ranges etc, based on recent performance
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
        simulation_state simulationstate{
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

    if( ( true == Global.ResourceSweep )
     && ( true == simulation::is_ready ) ) {
        // garbage collection
        m_geometry.update();
        m_textures.update();
    }

    if( true == DebugModeFlag ) {
        m_debugtimestext += m_textures.info();
    }

    // dump last opengl error, if any
    auto const glerror = ::glGetError();
    if( glerror != GL_NO_ERROR ) {
        std::string glerrorstring;
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
            return ( glm::length2( camera - Left.position ) / Left.intensity ) < ( glm::length2( camera - Right.position ) / Right.intensity ); } );

    size_t const count = std::min( m_lights.size(), Lights.data.size() );
    if( count == 0 ) { return; }

    auto renderlight = m_lights.begin();

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

        ::glLightf( renderlight->id, GL_LINEAR_ATTENUATION, static_cast<GLfloat>( (0.25 * scenelight.count) / std::pow( scenelight.count, 2 ) * (scenelight.owner->DimHeadlights ? 1.25 : 1.0) ) );
        ::glEnable( renderlight->id );

        renderlight->apply_intensity();
        renderlight->apply_angle();

        ++renderlight;
    }

    while( renderlight != m_lights.end() ) {
        // if we went through all scene lights and there's still opengl lights remaining, kill these
        ::glDisable( renderlight->id );
        ++renderlight;
    }
}

void
opengl_renderer::Disable_Lights() {

    for( size_t idx = 0; idx < m_lights.size() + 1; ++idx ) {

        ::glDisable( GL_LIGHT0 + (int)idx );
    }
}

bool
opengl_renderer::Init_caps() {

    std::string oglversion = ( (char *)glGetString( GL_VERSION ) );

    WriteLog(
        "Gfx Renderer: " + std::string( (char *)glGetString( GL_RENDERER ) )
        + " Vendor: " + std::string( (char *)glGetString( GL_VENDOR ) )
        + " OpenGL Version: " + oglversion );

#ifdef EU07_USEIMGUIIMPLOPENGL2
    if( !GLAD_GL_VERSION_1_5 ) {
        ErrorLog( "Requires openGL >= 1.5" );
        return false;
    }
#else
    if( !GLAD_GL_VERSION_3_0 ) {
        ErrorLog( "Requires openGL >= 3.0" );
        return false;
    }
#endif

    char* extensions = (char*)glGetString( GL_EXTENSIONS );

    WriteLog( "Supported extensions: \n"
        + std::string(extensions ? extensions : "") );

    WriteLog( std::string("render path: ") + ( Global.bUseVBO ? "VBO" : "Display lists" ) );
    if( GL_EXT_framebuffer_object ) {
        m_framebuffersupport = true;
        WriteLog( "framebuffer objects enabled" );
    }
    else {
        WriteLog( "framebuffer objects not supported, resorting to back buffer rendering where possible" );
    }
    // ograniczenie maksymalnego rozmiaru tekstur - parametr dla skalowania tekstur
    {
        GLint texturesize;
        ::glGetIntegerv( GL_MAX_TEXTURE_SIZE, &texturesize );
		Global.iMaxTextureSize = std::min(Global.iMaxTextureSize, texturesize);
        Global.iMaxCabTextureSize = std::min( Global.iMaxCabTextureSize, texturesize );
        WriteLog( "texture sizes capped at " + std::to_string(Global.iMaxTextureSize) + "p (" + std::to_string( Global.iMaxCabTextureSize ) + "p for cab textures)" );
        Global.CurrentMaxTextureSize = Global.iMaxTextureSize;
        m_shadowbuffersize = Global.shadowtune.map_size;
        m_shadowbuffersize = std::min( m_shadowbuffersize, texturesize );
        WriteLog( "shadows map size capped at " + std::to_string( m_shadowbuffersize ) + "p" );
    }
    // cap the number of supported lights based on hardware
    {
        GLint maxlights;
        ::glGetIntegerv( GL_MAX_LIGHTS, &maxlights );
        Global.DynamicLightCount = std::min( Global.DynamicLightCount, maxlights - 1 );
        WriteLog( "dynamic light amount capped at " + std::to_string( Global.DynamicLightCount ) + " (" + std::to_string(maxlights) + " lights total supported by the gfx card)" );
    }
    // select renderer mode
    if( true == Global.BasicRenderer ) {
        WriteLog( "basic renderer selected, shadow and reflection mapping will be disabled" );
        Global.RenderShadows = false;
        m_diffusetextureunit = 0;
        m_helpertextureunit = -1;
        m_shadowtextureunit = -1;
        m_normaltextureunit = -1;
    }
    else {
        GLint maxtextureunits;
        ::glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxtextureunits );
        if( maxtextureunits < 4 ) {
            WriteLog( "less than 4 texture units, shadow and reflection mapping will be disabled" );
            Global.BasicRenderer = true;
            Global.RenderShadows = false;
            m_diffusetextureunit = 0;
            m_helpertextureunit = -1;
            m_shadowtextureunit = -1;
            m_normaltextureunit = -1;
        }
    }

    if( Global.iMultisampling ) {
        WriteLog( "using multisampling x" + std::to_string( 1 << Global.iMultisampling ) );
    }

    WriteLog( "main window size: " + std::to_string( Global.iWindowWidth ) + "x" + std::to_string( Global.iWindowHeight ) );

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

std::unique_ptr<gfx_renderer> opengl_renderer::create_func()
{
    return std::unique_ptr<gfx_renderer>(new opengl_renderer());
}

bool opengl_renderer::renderer_register = gfx_renderer_factory::get_instance()->register_backend("legacy", opengl_renderer::create_func);
