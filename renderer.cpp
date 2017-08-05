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
#include "globals.h"
#include "timer.h"
#include "world.h"
#include "train.h"
#include "data.h"
#include "dynobj.h"
#include "animmodel.h"
#include "traction.h"
#include "uilayer.h"
#include "logs.h"
#include "usefull.h"

opengl_renderer GfxRenderer;
extern TWorld World;

int const EU07_PICKBUFFERSIZE { 1024 }; // size of (square) textures bound with the pick framebuffer

namespace colors {

glm::vec4 const none { 0.f, 0.f, 0.f, 1.f };
glm::vec4 const white{ 1.f, 1.f, 1.f, 1.f };

} // namespace colors

void
opengl_camera::update_frustum( glm::mat4 const &Projection, glm::mat4 const &Modelview ) {

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
opengl_camera::visible( bounding_area const &Area ) const {

    return ( m_frustum.sphere_inside( Area.center, Area.radius ) > 0.0f );
}

bool
opengl_camera::visible( TDynamicObject const *Dynamic ) const {

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
opengl_camera::draw( glm::vec3 const &Offset ) const {

    ::glBegin( GL_LINES );
    for( auto const pointindex : frustumshapepoinstorder ) {
        ::glVertex3fv( glm::value_ptr( glm::vec3{ m_frustumpoints[ pointindex ] } - Offset ) );
    }
    ::glEnd();
}

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

    glActiveTexture( m_diffusetextureunit );
    m_geometry.units().texture = m_diffusetextureunit;
    UILayer.set_unit( m_diffusetextureunit );

    ::glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
    glAlphaFunc( GL_GREATER, 0.04f );
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
    if( true == Global::ScaleSpecularValues ) {
        m_specularopaquescalefactor = 0.25f;
        m_speculartranslucentscalefactor = 1.5f;
    }
    ::glEnable( GL_COLOR_MATERIAL );
    ::glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );

    // setup lighting
    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(m_baseambient) );
    ::glEnable( GL_LIGHTING );
    ::glEnable( GL_LIGHT0 );

    Global::DayLight.id = opengl_renderer::sunlight;
    // directional light
    // TODO, TBD: test omni-directional variant
    Global::DayLight.position[ 3 ] = 1.0f;
    ::glLightf( opengl_renderer::sunlight, GL_SPOT_CUTOFF, 90.0f );
    // rgb value for 5780 kelvin
    Global::DayLight.diffuse[ 0 ] = 255.0f / 255.0f;
    Global::DayLight.diffuse[ 1 ] = 242.0f / 255.0f;
    Global::DayLight.diffuse[ 2 ] = 231.0f / 255.0f;

    // create dynamic light pool
    for( int idx = 0; idx < Global::DynamicLightCount; ++idx ) {

        opengl_light light;
        light.id = GL_LIGHT1 + idx;

        light.position[ 3 ] = 1.0f;
        ::glLightf( light.id, GL_SPOT_CUTOFF, 7.5f );
        ::glLightf( light.id, GL_SPOT_EXPONENT, 7.5f );
        ::glLightf( light.id, GL_CONSTANT_ATTENUATION, 0.0f );
        ::glLightf( light.id, GL_LINEAR_ATTENUATION, 0.035f );

        m_lights.emplace_back( light );
    }
    // preload some common textures
    WriteLog( "Loading common gfx data..." );
    m_glaretexture = Fetch_Texture( "fx\\lightglare" );
    m_suntexture = Fetch_Texture( "fx\\sun" );
    m_moontexture = Fetch_Texture( "fx\\moon" );
    if( m_helpertextureunit >= 0 ) {
        m_reflectiontexture = Fetch_Texture( "fx\\reflections" );
    }
    WriteLog( "...gfx data pre-loading done" );

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    // pick buffer resources
    if( true == m_framebuffersupport ) {
        // try to create the pick buffer. RGBA8 2D texture, 24 bit depth texture, 1024x1024 (height of 512 would suffice but, eh)
        // texture
        ::glGenTextures( 1, &m_picktexture );
        ::glBindTexture( GL_TEXTURE_2D, m_picktexture );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        ::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        ::glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, EU07_PICKBUFFERSIZE, EU07_PICKBUFFERSIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL );
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
    if( ( true == Global::RenderShadows )
     && ( true == m_framebuffersupport ) ) {
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
        GLenum status = ::glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
        if( status == GL_FRAMEBUFFER_COMPLETE_EXT ) {
            WriteLog( "Shadows framebuffer setup complete" );
        }
        else {
            ErrorLog( "Shadows framebuffer setup failed" );
            m_framebuffersupport = false;
            Global::RenderShadows = false;
        }
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target for now
    }

    // prepare basic geometry chunks
    auto const geometrybank = m_geometry.create_bank();
    float const size = 2.5f;
    m_billboardgeometry = m_geometry.create_chunk(
        vertex_array{
            { { -size,  size, 0.f }, glm::vec3(), { 1.f, 1.f } },
            { {  size,  size, 0.f }, glm::vec3(), { 0.f, 1.f } },
            { { -size, -size, 0.f }, glm::vec3(), { 1.f, 0.f } },
            { {  size, -size, 0.f }, glm::vec3(), { 0.f, 0.f } } },
            geometrybank,
            GL_TRIANGLE_STRIP );
    // prepare debug mode objects
    m_quadric = ::gluNewQuadric();
    ::gluQuadricNormals( m_quadric, GLU_FLAT );

    return true;
}

bool
opengl_renderer::Render() {

    if( m_drawstart != std::chrono::steady_clock::time_point() ) {
        m_drawtime = std::max( 20.f, 0.95f * m_drawtime + std::chrono::duration_cast<std::chrono::microseconds>( ( std::chrono::steady_clock::now() - m_drawstart ) ).count() / 1000.f );
    }
    m_drawstart = std::chrono::steady_clock::now();
    auto const drawstartcolorpass = m_drawstart;

    m_renderpass.draw_mode = rendermode::none; // force setup anew
    m_debuginfo.clear();
    Render_pass( rendermode::color );

    m_drawcount = m_drawqueue.size();
    // accumulate last 20 frames worth of render time (cap at 1000 fps to prevent calculations going awry)
    m_drawtimecolorpass = std::max( 20.f, 0.95f * m_drawtimecolorpass + std::chrono::duration_cast<std::chrono::microseconds>( ( std::chrono::steady_clock::now() - drawstartcolorpass ) ).count() / 1000.f );
    m_debuginfo += "frame total: " + to_string( m_drawtimecolorpass / 20.f, 2 ) + " msec (" + std::to_string( m_drawqueue.size() ) + " sectors) ";

    glfwSwapBuffers( m_window );

    return true; // for now always succeed
}

// runs jobs needed to generate graphics for specified render pass
void
opengl_renderer::Render_pass( rendermode const Mode ) {

    setup_pass( m_renderpass, Mode );
    switch( m_renderpass.draw_mode ) {

        case rendermode::color: {

            opengl_camera shadowcamera; // temporary helper, remove once ortho shadowmap code is done
            if( ( true == Global::RenderShadows )
             && ( true == World.InitPerformed() )
             && ( m_shadowcolor != colors::white ) ) {
                // run shadowmap pass before color
                Render_pass( rendermode::shadows );
#ifdef EU07_USE_DEBUG_SHADOWMAP
                UILayer.set_texture( m_shadowdebugtexture );
#endif
                shadowcamera = m_renderpass.camera; // cache shadow camera placement for visualization
                setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack
#ifdef EU07_USE_DEBUG_CAMERA
                setup_pass(
                    m_worldcamera,
                    rendermode::color,
                    0.f,
                    std::min(
                        1.f,
                        Global::shadowtune.depth / ( Global::BaseDrawRange * Global::fDistanceFactor )
                        * std::max(
                            1.f,
                            Global::ZoomFactor * 0.5f ) ),
                    true );
#endif
                // setup shadowmap matrix
                m_shadowtexturematrix =
                    //bias from [-1, 1] to [0, 1] };
                    glm::mat4{ 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f }
                    * shadowcamera.projection()
                    // during colour pass coordinates are moved from camera-centric to light-centric, essentially the difference between these two origins
                    * glm::translate(
                        glm::mat4{ glm::mat3{ shadowcamera.modelview() } },
                        glm::vec3{ m_renderpass.camera.position() - shadowcamera.position() } );
            }

            ::glViewport( 0, 0, Global::iWindowWidth, Global::iWindowHeight );

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
                setup_units( true, false, false );
                Render( &World.Environment );
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
                    shadowcamera.draw( m_renderpass.camera.position() - shadowcamera.position() );
                    if( DebugCameraFlag ) {
                        ::glColor4f( 0.8f, 1.f, 0.9f, 1.f );
                        m_worldcamera.camera.draw( m_renderpass.camera.position() - m_worldcamera.camera.position() );
                    }
                    ::glLineWidth( 1.f );
                    ::glEnable( GL_LIGHTING );
                    ::glEnable( GL_TEXTURE_2D );
                }
#endif
                Render( &World.Ground );
                // ...translucent parts
                setup_drawing( true );
                Render_Alpha( &World.Ground );
                // cab render is performed without shadows, due to low resolution and number of models without windows :|
                switch_units( true, false, false );
                // cab render is done in translucent phase to deal with badly configured vehicles
                if( World.Train != nullptr ) { Render_cab( World.Train->Dynamic() ); }
            }
            UILayer.render();
            break;
        }

        case rendermode::shadows: {

            if( World.InitPerformed() ) {
                // setup
                auto const shadowdrawstart = std::chrono::steady_clock::now();

                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_shadowframebuffer );

                ::glViewport( 0, 0, m_shadowbuffersize, m_shadowbuffersize );

#ifdef EU07_USE_DEBUG_SHADOWMAP
                ::glClearColor( 0.f / 255.f, 0.f / 255.f, 0.f / 255.f, 1.f ); // initial background Color
                ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
                opengl_camera worldcamera{ m_renderpass.camera }; // cache shadow camera placement for visualization
#else
                ::glClear( GL_DEPTH_BUFFER_BIT ); 
#endif
                ::glScissor( 1, 1, m_shadowbuffersize - 2, m_shadowbuffersize - 2 );
                ::glEnable( GL_SCISSOR_TEST );
                setup_matrices();
                ::glEnable( GL_POLYGON_OFFSET_FILL ); // alleviate depth-fighting
                ::glPolygonOffset( 2.f, 2.f );
                // render
                // opaque parts...
                setup_drawing( false );
#ifdef EU07_USE_DEBUG_SHADOWMAP
                setup_units( true, false, false );
#else
                setup_units( false, false, false );
#endif
                Render( &World.Ground );
                // post-render restore
                ::glDisable( GL_POLYGON_OFFSET_FILL );
                ::glDisable( GL_SCISSOR_TEST );

                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target

                m_drawtimeshadowpass = 0.95f * m_drawtimeshadowpass + std::chrono::duration_cast<std::chrono::microseconds>( ( std::chrono::steady_clock::now() - shadowdrawstart ) ).count() / 1000.f;
                m_debuginfo += "shadows: " + to_string( m_drawtimeshadowpass / 20.f, 2 ) + " msec (" + std::to_string( m_drawqueue.size() ) + " sectors) ";
            }
            break;
        }

        case rendermode::pickcontrols: {
            if( World.InitPerformed() ) {
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
                if( World.Train != nullptr ) { Render_cab( World.Train->Dynamic() ); }
                // post-render cleanup
            }
            break;
        }

        case rendermode::pickscenery: {
            if( World.InitPerformed() ) {
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
                Render( &World.Ground );
                // post-render cleanup
            }
            break;
        }

        default: {
            break;
        }
    }
}

void
opengl_renderer::setup_pass( renderpass_config &Config, rendermode const Mode, float const Znear, float const Zfar, bool const Ignoredebug ) {

    Config.draw_mode = Mode;

    if( false == World.InitPerformed() ) { return; }
    // setup draw range
    switch( Mode ) {
        case rendermode::color:        { Config.draw_range = Global::BaseDrawRange; break; }
        case rendermode::shadows:      { Config.draw_range = Global::BaseDrawRange * 0.5f; break; }
        case rendermode::pickcontrols: { Config.draw_range = 50.f; break; }
        case rendermode::pickscenery:  { Config.draw_range = Global::BaseDrawRange * 0.5f; break; }
        default:                       { Config.draw_range = 0.f; break; }
    }
    // setup camera
    auto &camera = Config.camera;

    camera.projection() = glm::mat4( 1.f );
    glm::dmat4 viewmatrix( 1.0 );

    switch( Mode ) {
        case rendermode::color: {
            // projection
            auto const zfar = Config.draw_range * Global::fDistanceFactor * Zfar;
            auto const znear = (
                Znear > 0.f ?
                Znear * zfar :
                0.1f * Global::ZoomFactor );
            camera.projection() *=
                glm::perspective(
                    glm::radians( Global::FieldOfView / Global::ZoomFactor ),
                    std::max( 1.f, (float)Global::iWindowWidth ) / std::max( 1.f, (float)Global::iWindowHeight ),
                    znear,
                    zfar );
            // modelview
            if( ( false == DebugCameraFlag ) || ( true == Ignoredebug ) ) {
                camera.position() = Global::pCameraPosition;
                World.Camera.SetMatrix( viewmatrix );
            }
            else {
                camera.position() = Global::DebugCameraPosition;
                World.DebugCamera.SetMatrix( viewmatrix );
            }
            break;
        }

        case rendermode::shadows: {
            // calculate lightview boundaries based on relevant area of the world camera frustum:
            // ...setup chunk of frustum we're interested in...
            auto const znear = 0.f;
            auto const zfar = std::min( 1.f, Global::shadowtune.depth / ( Global::BaseDrawRange * Global::fDistanceFactor ) * std::max( 1.f, Global::ZoomFactor * 0.5f ) );
            renderpass_config worldview;
            setup_pass( worldview, rendermode::color, znear, zfar, true );
            auto &frustumchunkshapepoints = worldview.camera.frustum_points();
            // ...modelview matrix: determine the centre of frustum chunk in world space...
            glm::vec3 frustumchunkmin, frustumchunkmax;
            bounding_box( frustumchunkmin, frustumchunkmax, std::begin( frustumchunkshapepoints ), std::end( frustumchunkshapepoints ) );
            auto const frustumchunkcentre = ( frustumchunkmin + frustumchunkmax ) * 0.5f;
            // ...cap the vertical angle to keep shadows from getting too long...
            auto const lightvector =
                glm::normalize( glm::vec3{
                              Global::DayLight.direction.x,
                    std::min( Global::DayLight.direction.y, -0.15f ),
                              Global::DayLight.direction.z } );
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
            // ...use the dimensions to set up light projection boundaries
            // NOTE: since we only have one cascade map stage, we extend the chunk forward/back to catch areas normally covered by other stages
            auto const quantizationstep = ( Global::shadowtune.depth + 1000.f ) / m_shadowbuffersize;
            frustumchunkmin.x -= std::remainder( frustumchunkmin.x, quantizationstep );
            frustumchunkmin.y -= std::remainder( frustumchunkmin.y, quantizationstep );
            frustumchunkmax.x -= std::remainder( frustumchunkmax.x, quantizationstep );
            frustumchunkmax.y -= std::remainder( frustumchunkmax.y, quantizationstep );
            camera.projection() *=
                glm::ortho(
                    frustumchunkmin.x, frustumchunkmax.x,
                    frustumchunkmin.y, frustumchunkmax.y,
                    frustumchunkmin.z - 500.f, frustumchunkmax.z + 500.f );
            break;
        }

        case rendermode::pickcontrols:
        case rendermode::pickscenery: {
            // TODO: scissor test for pick modes
            // projection
            if( true == m_framebuffersupport ) {
                auto const angle = Global::FieldOfView / Global::ZoomFactor;
                auto const height = std::max( 1.0f, (float)Global::iWindowWidth ) / std::max( 1.0f, (float)Global::iWindowHeight ) / ( Global::iWindowWidth / EU07_PICKBUFFERSIZE );
                camera.projection() *=
                    glm::perspective(
                        glm::radians( Global::FieldOfView / Global::ZoomFactor ),
                        std::max( 1.f, (float)Global::iWindowWidth ) / std::max( 1.0f, (float)Global::iWindowHeight ) / ( Global::iWindowWidth / EU07_PICKBUFFERSIZE ),
                        0.1f * Global::ZoomFactor,
                        Config.draw_range * Global::fDistanceFactor );
            }
            else {
                camera.projection() *=
                    glm::perspective(
                        glm::radians( Global::FieldOfView / Global::ZoomFactor ),
                        std::max( 1.f, (float)Global::iWindowWidth ) / std::max( 1.f, (float)Global::iWindowHeight ),
                        0.1f * Global::ZoomFactor,
                        Config.draw_range * Global::fDistanceFactor );
            }
            // modelview
            camera.position() = Global::pCameraPosition;
            World.Camera.SetMatrix( viewmatrix );
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
    // trim modelview matrix just to rotation, since rendering is done in camera-centric world space
    ::glMatrixMode( GL_MODELVIEW );
    OpenGLMatrices.load_matrix( glm::mat4( glm::mat3( m_renderpass.camera.modelview() ) ) );
}

void
opengl_renderer::setup_drawing( bool const Alpha ) {

    if( true == Alpha ) {

        ::glEnable( GL_BLEND );
        ::glAlphaFunc( GL_GREATER, 0.04f );
    }
    else {
        ::glDisable( GL_BLEND );
        ::glAlphaFunc( GL_GREATER, 0.50f );
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {
            ::glEnable( GL_LIGHTING );
            ::glShadeModel( GL_SMOOTH );
            if( Global::iMultisampling ) {
                ::glEnable( GL_MULTISAMPLE );
            }
            // setup fog
            if( Global::fFogEnd > 0 ) {
                // fog setup
                ::glFogfv( GL_FOG_COLOR, Global::FogColor );
                ::glFogf( GL_FOG_DENSITY, static_cast<GLfloat>( 1.0 / Global::fFogEnd ) );
                ::glEnable( GL_FOG );
            }
            else { ::glDisable( GL_FOG ); }

            break;
        }
        case rendermode::shadows:
        case rendermode::pickcontrols:
        case rendermode::pickscenery: {
            ::glColor4fv( glm::value_ptr( colors::white ) );
            ::glDisable( GL_LIGHTING );
            ::glShadeModel( GL_FLAT );
            if( Global::iMultisampling ) {
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
        if( ( true == Global::RenderShadows ) && ( true == Shadows ) ) {
            // setup reflection texture unit
            ::glActiveTexture( m_helpertextureunit );
            Bind( m_reflectiontexture ); // TBD, TODO: move to reflection unit setup
            ::glEnable( GL_TEXTURE_2D );
/*
            glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
            glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
            glEnable( GL_TEXTURE_GEN_S );
            glEnable( GL_TEXTURE_GEN_T );
*/
            ::glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
            ::glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, glm::value_ptr( m_shadowcolor ) ); // TODO: dynamically calculated shadow colour, based on sun height

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE ); // darken the previous stage
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT );
            ::glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

            ::glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE ); // pass the previous stage alpha down the chain
            ::glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
        }
        else {
            ::glActiveTexture( m_helpertextureunit );
            ::glDisable( GL_TEXTURE_2D );
/*
            ::glDisable( GL_TEXTURE_GEN_S );
            ::glDisable( GL_TEXTURE_GEN_T );
            ::glDisable( GL_TEXTURE_GEN_R );
            ::glDisable( GL_TEXTURE_GEN_Q );
*/
        }
    }
    // shadow texture unit.
    // interpolates between primary colour and the previous unit, which should hold darkened variant of the primary colour
    if( m_shadowtextureunit >= 0 ) {
        if( ( true == Global::RenderShadows )
         && ( true == Shadows )
         && ( m_shadowcolor != colors::white ) ) {

            ::glActiveTexture( m_shadowtextureunit );
            ::glBindTexture( GL_TEXTURE_2D, m_shadowtexture );
            ::glEnable( GL_TEXTURE_2D );
            // s
            ::glTexGenfv( GL_S, GL_EYE_PLANE, glm::value_ptr( glm::row( m_shadowtexturematrix, 0 ) ) );
            ::glEnable( GL_TEXTURE_GEN_S );
            // t
            ::glTexGenfv( GL_T, GL_EYE_PLANE, glm::value_ptr( glm::row( m_shadowtexturematrix, 1 ) ) );
            ::glEnable( GL_TEXTURE_GEN_T );
            // r
            ::glTexGenfv( GL_R, GL_EYE_PLANE, glm::value_ptr( glm::row( m_shadowtexturematrix, 2 ) ) );
            ::glEnable( GL_TEXTURE_GEN_R );
            // q
            ::glTexGenfv( GL_Q, GL_EYE_PLANE, glm::value_ptr( glm::row( m_shadowtexturematrix, 3 ) ) );
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
            ::glActiveTexture( m_shadowtextureunit );

            ::glDisable( GL_TEXTURE_2D );
            ::glDisable( GL_TEXTURE_GEN_S );
            ::glDisable( GL_TEXTURE_GEN_T );
            ::glDisable( GL_TEXTURE_GEN_R );
            ::glDisable( GL_TEXTURE_GEN_Q );
        }
    }
    // diffuse texture unit.
    // NOTE: diffuse texture mapping is never fully disabled, alpha channel information is always included
    ::glActiveTexture( m_diffusetextureunit );
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
}

// enables and disables specified texture units
void
opengl_renderer::switch_units( bool const Diffuse, bool const Shadows, bool const Reflections ) {
    // helper texture unit.
    if( m_helpertextureunit >= 0 ) {
        if( ( true == Global::RenderShadows )
         && ( true == Shadows )
         && ( m_shadowcolor != colors::white ) ) {
            ::glActiveTexture( m_helpertextureunit );
            ::glEnable( GL_TEXTURE_2D );
        }
        else {
            ::glActiveTexture( m_helpertextureunit );
            ::glDisable( GL_TEXTURE_2D );
        }
    }
    // shadow texture unit.
    if( m_shadowtextureunit >= 0 ) {
        if( ( true == Global::RenderShadows ) && ( true == Shadows ) ) {

            ::glActiveTexture( m_shadowtextureunit );
            ::glEnable( GL_TEXTURE_2D );
        }
        else {

            ::glActiveTexture( m_shadowtextureunit );
            ::glDisable( GL_TEXTURE_2D );
        }
    }
    // diffuse texture unit.
    // NOTE: toggle actually disables diffuse texture mapping, unlike setup counterpart
    if( true == Diffuse ) {

        ::glActiveTexture( m_diffusetextureunit );
        ::glEnable( GL_TEXTURE_2D );
    }
    else {

        ::glActiveTexture( m_diffusetextureunit );
        ::glDisable( GL_TEXTURE_2D );
    }
}

void
opengl_renderer::setup_shadow_color( glm::vec4 const &Shadowcolor ) {

    ::glActiveTexture( m_helpertextureunit );
    ::glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, glm::value_ptr( Shadowcolor ) ); // in-shadow colour multiplier
    ::glActiveTexture( m_diffusetextureunit );

}

bool
opengl_renderer::Render( world_environment *Environment ) {

    // calculate shadow tone, based on positions of celestial bodies
    m_shadowcolor = interpolate(
        glm::vec4{ 0.5f, 0.5f, 0.5f, 1.f },
        glm::vec4{ colors::white },
        clamp( -Environment->m_sun.getAngle(), 0.f, 6.f ) / 6.f );
    if( ( Environment->m_sun.getAngle() < -18.f )
     && ( Environment->m_moon.getAngle() > 0.f ) ) {
        // turn on moon shadows after nautical twilight, if the moon is actually up
        m_shadowcolor = glm::vec4{ 0.5f, 0.5f, 0.5f, 1.f };
    }

    if( Global::bWireFrame ) {
        // bez nieba w trybie rysowania linii
        return false;
    }

    Bind( NULL );
    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_DEPTH_TEST );
    ::glDepthMask( GL_FALSE );
    ::glPushMatrix();

    // skydome
    Environment->m_skydome.Render();
    if( true == Global::bUseVBO ) {
        // skydome uses a custom vbo which could potentially confuse the main geometry system. hardly elegant but, eh
        opengl_vbogeometrybank::reset();
    }
    // stars
    if( Environment->m_stars.m_stars != nullptr ) {
        // setup
        ::glPushMatrix();
        ::glRotatef( Environment->m_stars.m_latitude, 1.f, 0.f, 0.f ); // ustawienie osi OY na północ
        ::glRotatef( -std::fmod( (float)Global::fTimeAngleDeg, 360.f ), 0.f, 1.f, 0.f ); // obrót dobowy osi OX
        ::glPointSize( 2.f );
        // render
        GfxRenderer.Render( Environment->m_stars.m_stars, nullptr, 1.0 );
        // post-render cleanup
        ::glPointSize( 3.f );
        ::glPopMatrix();
    }

    // celestial bodies
    float const duskfactor = 1.0f - clamp( std::abs( Environment->m_sun.getAngle() ), 0.0f, 12.0f ) / 12.0f;
    glm::vec3 suncolor = interpolate(
        glm::vec3( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f ),
        glm::vec3( 235.0f / 255.0f, 140.0f / 255.0f, 36.0f / 255.0f ),
        duskfactor );

    if( DebugModeFlag == true ) {
        // mark sun position for easier debugging
        Environment->m_sun.render();
        Environment->m_moon.render();
    }
    // render actual sun and moon
    ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );

    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_ALPHA_TEST );
    ::glEnable( GL_BLEND );
    ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
    // sun
    {
        Bind( m_suntexture );
        ::glColor4f( suncolor.x, suncolor.y, suncolor.z, 1.0f );
        auto const sunvector = Environment->m_sun.getDirection();
        auto const sunposition = modelview * glm::vec4( sunvector.x, sunvector.y, sunvector.z, 1.0f );

        ::glPushMatrix();
        ::glLoadIdentity(); // macierz jedynkowa
        ::glTranslatef( sunposition.x, sunposition.y, sunposition.z ); // początek układu zostaje bez zmian

        float const size = 0.045f;
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
        Bind( m_moontexture );
        glm::vec3 mooncolor( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f );
        ::glColor4f( mooncolor.x, mooncolor.y, mooncolor.z, static_cast<GLfloat>( 1.0 - Global::fLuminance * 0.5 ) );

        auto const moonposition = modelview * glm::vec4( Environment->m_moon.getDirection(), 1.0f );
        ::glPushMatrix();
        ::glLoadIdentity(); // macierz jedynkowa
        ::glTranslatef( moonposition.x, moonposition.y, moonposition.z );

        float const size = interpolate( // TODO: expose distance/scale factor from the moon object
            0.0175f,
            0.015f,
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

    // clouds
    if( Environment->m_clouds.mdCloud ) {
        // setup
        Disable_Lights();
        ::glEnable( GL_LIGHTING );
        ::glLightModelfv(
            GL_LIGHT_MODEL_AMBIENT,
            glm::value_ptr(
                interpolate( Environment->m_skydome.GetAverageColor(), suncolor, duskfactor * 0.25f )
                * ( 1.0f - Global::Overcast * 0.5f ) // overcast darkens the clouds
                * 2.5f // arbitrary adjustment factor
            ) );
        // render
        Render( Environment->m_clouds.mdCloud, nullptr, 100.0 );
        Render_Alpha( Environment->m_clouds.mdCloud, nullptr, 100.0 );
        // post-render cleanup
        GLfloat noambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, noambient );
        ::glEnable( GL_LIGHT0 ); // other lights will be enabled during lights update
        ::glDisable( GL_LIGHTING );
    }

    Global::DayLight.apply_angle();
    Global::DayLight.apply_intensity();

    ::glPopMatrix();
    ::glDepthMask( GL_TRUE );
    ::glEnable( GL_DEPTH_TEST );
    ::glEnable( GL_LIGHTING );

    return true;
}

// geometry methods
// creates a new geometry bank. returns: handle to the bank or NULL
geometrybank_handle
opengl_renderer::Create_Bank() {

    return m_geometry.create_bank();
}

// creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
geometry_handle
opengl_renderer::Insert( vertex_array &Vertices, geometrybank_handle const &Geometry, int const Type ) {

    return m_geometry.create_chunk( Vertices, Geometry, Type );
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
opengl_renderer::Replace( vertex_array &Vertices, geometry_handle const &Geometry, std::size_t const Offset ) {

    return m_geometry.replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
opengl_renderer::Append( vertex_array &Vertices, geometry_handle const &Geometry ) {

    return m_geometry.append( Vertices, Geometry );
}

// provides direct access to vertex data of specfied chunk
vertex_array const &
opengl_renderer::Vertices( geometry_handle const &Geometry ) const {

    return m_geometry.vertices( Geometry );
}

// texture methods
texture_handle
opengl_renderer::Fetch_Texture( std::string const &Filename, std::string const &Dir, int const Filter, bool const Loadnow ) {

    return m_textures.create( Filename, Dir, Filter, Loadnow );
}

void
opengl_renderer::Bind( texture_handle const Texture ) {
    // temporary until we separate the renderer
    m_textures.bind( Texture );
}

opengl_texture const &
opengl_renderer::Texture( texture_handle const Texture ) {

    return m_textures.texture( Texture );
}



bool
opengl_renderer::Render( TGround *Ground ) {

    ++TGroundRect::iFrameNumber; // zwięszenie licznika ramek (do usuwniania nadanimacji)

    m_drawqueue.clear();

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {
            // rednerowanie globalnych (nie za często?)
            for( TGroundNode *node = Ground->srGlobal.nRenderHidden; node; node = node->nNext3 ) {
                node->RenderHidden();
            }
            break;
        }
        default: {
            break;
        }
    }

    glm::vec3 const cameraposition { m_renderpass.camera.position() };
    int const camerax = static_cast<int>( std::floor( cameraposition.x / 1000.0f ) + iNumRects / 2 );
    int const cameraz = static_cast<int>( std::floor( cameraposition.z / 1000.0f ) + iNumRects / 2 );
    int const segmentcount = 2 * static_cast<int>(std::ceil( m_renderpass.draw_range * Global::fDistanceFactor / 1000.0f ));
    int const originx = std::max( 0, camerax - segmentcount / 2 );
    int const originz = std::max( 0, cameraz - segmentcount / 2 );

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {

            Update_Lights( Ground->m_lights );

            for( int column = originx; column <= originx + segmentcount; ++column ) {
                for( int row = originz; row <= originz + segmentcount; ++row ) {

                    auto *cell = &Ground->Rects[ column ][ row ];

                    for( int subcellcolumn = 0; subcellcolumn < iNumSubRects; ++subcellcolumn ) {
                        for( int subcellrow = 0; subcellrow < iNumSubRects; ++subcellrow ) {
                            auto subcell = cell->FastGetSubRect( subcellcolumn, subcellrow );
                            if( subcell == nullptr ) { continue; }
                            // renderowanie obiektów aktywnych a niewidocznych
                            for( auto node = subcell->nRenderHidden; node; node = node->nNext3 ) {
                                node->RenderHidden();
                            }
                            // jeszcze dźwięki pojazdów by się przydały, również niewidocznych
                            subcell->RenderSounds();
                        }
                    }

                    if( m_renderpass.camera.visible( cell->m_area ) ) {
                        Render( cell );
                    }
                }
            }
            // draw queue was filled while rendering content of ground cells. now sort the nodes based on their distance to viewer...
            std::sort(
                std::begin( m_drawqueue ),
                std::end( m_drawqueue ),
                []( distancesubcell_pair const &Left, distancesubcell_pair const &Right ) {
                    return ( Left.first ) < ( Right.first ); } );
            // ...then render the opaque content of the visible subcells.
            for( auto subcellpair : m_drawqueue ) {
                Render( subcellpair.second );
            }
            break;
        }
        case rendermode::shadows:
        case rendermode::pickscenery: {
            // these render modes don't bother with anything non-visual, or lights
            for( int column = originx; column <= originx + segmentcount; ++column ) {
                for( int row = originz; row <= originz + segmentcount; ++row ) {

                    auto *cell = &Ground->Rects[ column ][ row ];
                    if( m_renderpass.camera.visible( cell->m_area ) ) {
                        Render( cell );
                    }
                }
            }
            // they can also skip queue sorting, as they only deal with opaque geometry
            // NOTE: there's benefit from rendering front-to-back, but is it significant enough? TODO: investigate
            for( auto subcellpair : m_drawqueue ) {
                Render( subcellpair.second );
            }
            break;
        }
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }

    return true;
}

bool
opengl_renderer::Render( TGroundRect *Groundcell ) {

    bool result { false }; // will be true if we do any rendering

    if( Groundcell->iLastDisplay != Groundcell->iFrameNumber ) {
        // tylko jezeli dany kwadrat nie był jeszcze renderowany
        Groundcell->LoadNodes(); // ewentualne tworzenie siatek

        switch( m_renderpass.draw_mode ) {
            case rendermode::pickscenery: {
                // non-interactive scenery elements get neutral colour
                ::glColor3fv( glm::value_ptr( colors::none ) );
            }
            case rendermode::color: {
                if( Groundcell->nRenderRect != nullptr ) {
                    // nieprzezroczyste trójkąty kwadratu kilometrowego
                    for( TGroundNode *node = Groundcell->nRenderRect; node != nullptr; node = node->nNext3 ) {
                        Render( node );
                    }
                }
                break;
            }
            case rendermode::shadows: {
                if( Groundcell->nRenderRect != nullptr ) {
                    // experimental, for shadows render both back and front faces, to supply back faces of the 'forest strips'
                    ::glDisable( GL_CULL_FACE );
                    // nieprzezroczyste trójkąty kwadratu kilometrowego
                    for( TGroundNode *node = Groundcell->nRenderRect; node != nullptr; node = node->nNext3 ) {
                        Render( node );
                    }
                    ::glEnable( GL_CULL_FACE );
                }
            }
            case rendermode::pickcontrols:
            default: {
                break;
            }
        }

        if( Groundcell->nTerrain ) {

            Render( Groundcell->nTerrain );
        }
        Groundcell->iLastDisplay = Groundcell->iFrameNumber; // drugi raz nie potrzeba
        result = true;

        // add the subcells of the cell to the draw queue
        if( Groundcell->pSubRects != nullptr ) {
            for( std::size_t subcellindex = 0; subcellindex < iNumSubRects * iNumSubRects; ++subcellindex ) {
                auto subcell = Groundcell->pSubRects + subcellindex;
                if( subcell->iNodeCount ) {
                    // o ile są jakieś obiekty, bo po co puste sektory przelatywać
                    m_drawqueue.emplace_back(
                        glm::length2( m_renderpass.camera.position() - glm::dvec3( subcell->m_area.center ) ),
                        subcell );
                }
            }
        }
    }
    return result;
}

bool
opengl_renderer::Render( TSubRect *Groundsubcell ) {

    // oznaczanie aktywnych sektorów
    Groundsubcell->LoadNodes();

    Groundsubcell->RaAnimate(); // przeliczenia animacji torów w sektorze

    TGroundNode *node;

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::shadows: {
            // nieprzezroczyste obiekty terenu
            for( node = Groundsubcell->nRenderRect; node != nullptr; node = node->nNext3 ) {
                Render( node );
            }
            // nieprzezroczyste obiekty (oprócz pojazdów)
            for( node = Groundsubcell->nRender; node != nullptr; node = node->nNext3 ) {
                Render( node );
            }
            // nieprzezroczyste z mieszanych modeli
            for( node = Groundsubcell->nRenderMixed; node != nullptr; node = node->nNext3 ) {
                Render( node );
            }
            // nieprzezroczyste fragmenty pojazdów na torach
            for( int trackidx = 0; trackidx < Groundsubcell->iTracks; ++trackidx ) {
                for( auto dynamic : Groundsubcell->tTracks[ trackidx ]->Dynamics ) {
                    Render( dynamic );
                }
            }
#ifdef EU07_SCENERY_EDITOR
            // memcells
            if( EditorModeFlag ) {
                for( auto const memcell : Groundsubcell->m_memcells ) {
                    Render( memcell );
                }
            }
#endif
            break;
        }
        case rendermode::pickscenery: {
            // same procedure like with regular render, but each node receives custom colour used for picking
            // nieprzezroczyste obiekty terenu
            for( node = Groundsubcell->nRenderRect; node != nullptr; node = node->nNext3 ) {
                ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                Render( node );
            }
            // nieprzezroczyste obiekty (oprócz pojazdów)
            for( node = Groundsubcell->nRender; node != nullptr; node = node->nNext3 ) {
                ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                Render( node );
            }
            // nieprzezroczyste z mieszanych modeli
            for( node = Groundsubcell->nRenderMixed; node != nullptr; node = node->nNext3 ) {
                ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                Render( node );
            }
            // nieprzezroczyste fragmenty pojazdów na torach
            for( int trackidx = 0; trackidx < Groundsubcell->iTracks; ++trackidx ) {
                for( auto dynamic : Groundsubcell->tTracks[ trackidx ]->Dynamics ) {
                    ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                    Render( dynamic );
                }
            }
#ifdef EU07_SCENERY_EDITOR
            // memcells
            if( EditorModeFlag ) {
                for( auto const memcell : Groundsubcell->m_memcells ) {
                    ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                    Render( memcell );
                }
            }
#endif
            break;
        }
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }

    return true;
}

bool
opengl_renderer::Render( TGroundNode *Node ) {

    switch (Node->iType)
    { // obiekty renderowane niezależnie od odległości
    case TP_SUBMODEL:
        ::glPushMatrix();
        auto const originoffset = Node->pCenter - m_renderpass.camera.position();
        ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
        TSubModel::fSquareDist = 0;
        Render( Node->smTerrain );
        ::glPopMatrix();
        return true;
    }

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = SquareMagnitude( ( Node->pCenter - Global::pCameraPosition ) / Global::ZoomFactor );
            break;
        }
        default: {
            distancesquared = SquareMagnitude( ( Node->pCenter - m_renderpass.camera.position() ) / Global::ZoomFactor );
            break;
        }
    }
    if( ( distancesquared > ( Node->fSquareRadius    * Global::fDistanceFactor ) )
     || ( distancesquared < ( Node->fSquareMinRadius / Global::fDistanceFactor ) ) ) {
        return false;
    }

    switch (Node->iType) {

        case TP_TRACK: {
            // setup
            switch( m_renderpass.draw_mode ) {
                case rendermode::shadows: {
                    return false;
                }
                case rendermode::pickscenery: {
                    // add the node to the pick list
                    m_picksceneryitems.emplace_back( Node );
                    break;
                }
                default: {
                    break;
                }
            }
            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - m_renderpass.camera.position();
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
            // render
            Render( Node->pTrack );
            // post-render cleanup
            ::glPopMatrix();
            return true;
        }

        case TP_MODEL: {
            switch( m_renderpass.draw_mode ) {
                case rendermode::pickscenery: {
                    // add the node to the pick list
                    m_picksceneryitems.emplace_back( Node );
                    break;
                }
                default: {
                    break;
                }
            }
            Node->Model->RaAnimate(); // jednorazowe przeliczenie animacji
            Node->Model->RaPrepare();
            if( Node->Model->pModel ) {
                // renderowanie rekurencyjne submodeli
                switch( m_renderpass.draw_mode ) {
                    case rendermode::shadows: {
                        Render(
                            Node->Model->pModel,
                            Node->Model->Material(),
                            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
                            SquareMagnitude( Node->pCenter - Global::pCameraPosition ),
                            Node->pCenter - m_renderpass.camera.position(),
                            Node->Model->vAngle );
                        break;
                    }
                    default: {
                        auto const position = Node->pCenter - m_renderpass.camera.position();
                        Render(
                            Node->Model->pModel,
                            Node->Model->Material(),
                            SquareMagnitude( position ),
                            position,
                            Node->Model->vAngle );
                        break;
                    }
                }
            }
            return true;
        }

        case GL_LINES: {
            if( ( Node->Piece->geometry == NULL )
             || ( Node->fLineThickness > 0.0 ) ) {
                return false;
            }
            // setup
            auto const distance = std::sqrt( distancesquared );
            auto const linealpha =
                10.0 * Node->fLineThickness
                / std::max(
                    0.5 * Node->m_radius + 1.0,
                    distance - ( 0.5 * Node->m_radius ) );
            switch( m_renderpass.draw_mode ) {
                // wire colouring is disabled for modes other than colour
                case rendermode::color: {
                    ::glColor4fv(
                        glm::value_ptr(
                            glm::vec4(
                                Node->Diffuse * glm::vec3( Global::DayLight.ambient ), // w zaleznosci od koloru swiatla
                                1.0 ) ) ); // if the thickness is defined negative, lines are always drawn opaque
                    break;
                }
                case rendermode::shadows:
                case rendermode::pickcontrols:
                case rendermode::pickscenery:
                default: {
                    break;
                }
            }
            auto const linewidth = clamp( 0.5 * linealpha + Node->fLineThickness * Node->m_radius / 1000.0, 1.0, 32.0 );
            if( linewidth > 1.0 ) {
                ::glLineWidth( static_cast<float>( linewidth ) );
            }

            GfxRenderer.Bind( 0 );

            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - m_renderpass.camera.position();
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            switch( m_renderpass.draw_mode ) {
                case rendermode::pickscenery: {
                    // add the node to the pick list
                    m_picksceneryitems.emplace_back( Node );
                    break;
                }
                default: {
                    break;
                }
            }
            // render
            m_geometry.draw( Node->Piece->geometry );

            // post-render cleanup
            ::glPopMatrix();

            if( linewidth > 1.0 ) { ::glLineWidth( 1.0f ); }

            return true;
        }

        case GL_TRIANGLES: {
            if( ( Node->Piece->geometry == NULL )
             || ( ( Node->iFlags & 0x10 ) == 0 ) ) {
                return false;
            }
            // setup
            Bind( Node->TextureID );
            switch( m_renderpass.draw_mode ) {
                case rendermode::color: {
                    ::glColor3fv( glm::value_ptr( Node->Diffuse ) );
                    break;
                }
                // pick modes get custom colours, and shadow pass doesn't use any
                case rendermode::shadows:
                case rendermode::pickcontrols:
                case rendermode::pickscenery:
                default: {
                    break;
                }
            }

            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - m_renderpass.camera.position();
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            switch( m_renderpass.draw_mode ) {
                case rendermode::pickscenery: {
                    // add the node to the pick list
                    m_picksceneryitems.emplace_back( Node );
                    break;
                }
                default: {
                    break;
                }
            }
            // render
            m_geometry.draw( Node->Piece->geometry );

            // post-render cleanup
            ::glPopMatrix();

            return true;
        }

        case TP_MEMCELL: {
            switch( m_renderpass.draw_mode ) {
                case rendermode::pickscenery: {
                    // add the node to the pick list
                    m_picksceneryitems.emplace_back( Node );
                    break;
                }
                default: {
                    break;
                }
            }
            Render( Node->MemCell );
            return true;
        }

        default: { break; }
    }
    // in theory we shouldn't ever get here but, eh
    return false;
}

bool
opengl_renderer::Render( TDynamicObject *Dynamic ) {

    Dynamic->renderme = m_renderpass.camera.visible( Dynamic );
    if( false == Dynamic->renderme ) {
        return false;
    }
    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    auto const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    double squaredistance;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            squaredistance = SquareMagnitude( ( Dynamic->vPosition - Global::pCameraPosition ) / Global::ZoomFactor );
            break;
        }
        default: {
            squaredistance = SquareMagnitude( originoffset / Global::ZoomFactor );
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
                Global::DayLight.apply_intensity( Dynamic->fShade );
            }
            m_renderspecular = true; // vehicles are rendered with specular component. static models without, at least for the time being
            // render
            if( Dynamic->mdLowPolyInt ) {
                // low poly interior
                if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
                    // enable cab light if needed
                    if( Dynamic->InteriorLightLevel > 0.0f ) {

                        // crude way to light the cabin, until we have something more complete in place
                        auto const cablight = Dynamic->InteriorLight * Dynamic->InteriorLightLevel;
                        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &cablight.x );
                    }

                    Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );

                    if( Dynamic->InteriorLightLevel > 0.0f ) {
                        // reset the overall ambient
                        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( m_baseambient ) );
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
                Global::DayLight.apply_intensity();
            }
            break;
        }
        case rendermode::shadows: {
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
opengl_renderer::Render_cab( TDynamicObject *Dynamic ) {

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
        ::glMultMatrixd( Dynamic->mMatrix.getArray() );

        switch( m_renderpass.draw_mode ) {
            case rendermode::color: {
                // render path specific setup:
                if( Dynamic->fShade > 0.0f ) {
                    // change light level based on light level of the occupied track
                    Global::DayLight.apply_intensity( Dynamic->fShade );
                }
                if( Dynamic->InteriorLightLevel > 0.0f ) {
                    // crude way to light the cabin, until we have something more complete in place
                    auto const cablight = Dynamic->InteriorLight * Dynamic->InteriorLightLevel;
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &cablight.x );
                }
                // render
                Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                Render_Alpha( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                // post-render restore
                if( Dynamic->fShade > 0.0f ) {
                    // change light level based on light level of the occupied track
                    Global::DayLight.apply_intensity();
                }
                if( Dynamic->InteriorLightLevel > 0.0f ) {
                    // reset the overall ambient
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(m_baseambient) );
                }
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
opengl_renderer::Render( TModel3d *Model, material_data const *Material, double const Squaredistance ) {

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
opengl_renderer::Render( TModel3d *Model, material_data const *Material, double const Squaredistance, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

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

    if( ( Submodel->iVisible )
     && ( TSubModel::fSquareDist >= ( Submodel->fSquareMinDist / Global::fDistanceFactor ) )
     && ( TSubModel::fSquareDist <= ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) ) {

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
                        if( Submodel->TextureID < 0 ) { // zmienialne skóry
                            Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                        }
                        else {
                            // również 0
                            Bind( Submodel->TextureID );
                        }
                        // ...colors...
                        ::glColor3fv( glm::value_ptr( Submodel->f4Diffuse ) ); // McZapkie-240702: zamiast ub
                        if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                            // specular strength in legacy models is set uniformly to 150, 150, 150 so we scale it down for opaque elements
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * Global::DayLight.specular.a * m_specularopaquescalefactor ) );
                            ::glEnable( GL_RESCALE_NORMAL );
                        }
                        // ...luminance
                        if( Global::fLuminance < Submodel->fLight ) {
                            // zeby swiecilo na kolorowo
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( Submodel->f4Diffuse * Submodel->f4Emision.a ) );
                            // disable shadows so they don't obstruct self-lit items
                            setup_shadow_color( colors::white );
                        }

                        // main draw call
                        m_geometry.draw( Submodel->m_geometry );

                        // post-draw reset
                        if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( colors::none ) );
                        }
                        if( Global::fLuminance < Submodel->fLight ) {
                            // restore default (lack of) brightness
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( colors::none ) );
                            setup_shadow_color( m_shadowcolor );
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
                    case rendermode::pickscenery: {
                        // scenery picking and shadow both use enforced colour and no frills
                        // material configuration:
                        // textures...
                        if( Submodel->TextureID < 0 ) { // zmienialne skóry
                            Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                        }
                        else {
                            // również 0
                            Bind( Submodel->TextureID );
                        }
                        // main draw call
                        m_geometry.draw( Submodel->m_geometry );
                        // post-draw reset
                        break;
                    }
                    case rendermode::pickcontrols: {
                        // material configuration:
                        // control picking applies individual colour for each submodel
                        m_pickcontrolsitems.emplace_back( Submodel );
                        ::glColor3fv( glm::value_ptr( pick_color( m_pickcontrolsitems.size() ) ) );
                        // textures...
                        if( Submodel->TextureID < 0 ) { // zmienialne skóry
                            Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                        }
                        else {
                            // również 0
                            Bind( Submodel->TextureID );
                        }
                        // main draw call
                        m_geometry.draw( Submodel->m_geometry );
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
                case rendermode::color: {
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
                        float const anglefactor = ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                        // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                        // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                        float const distancefactor = static_cast<float>( std::max( 0.5, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) );

                        if( lightlevel > 0.f ) {
                            // material configuration:
                            ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT );

                            Bind( 0 );
                            ::glPointSize( std::max( 3.f, 5.f * distancefactor * anglefactor ) );
                            ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], lightlevel * anglefactor );
                            ::glDisable( GL_LIGHTING );
                            ::glEnable( GL_BLEND );

                            ::glPushMatrix();
                            ::glLoadIdentity();
                            ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian

                            setup_shadow_color( colors::white );

                            // main draw call
                            m_geometry.draw( Submodel->m_geometry );

                            // post-draw reset
                            // re-enable shadows
                            setup_shadow_color( m_shadowcolor );
                            ::glPopMatrix();
                            ::glPopAttrib();
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
                case rendermode::color: {
                    if( Global::fLuminance < Submodel->fLight ) {

                        // material configuration:
                        ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );

                        Bind( 0 );
                        ::glDisable( GL_LIGHTING );

                        // main draw call
                        m_geometry.draw( Submodel->m_geometry, color_streams );

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
        if( Submodel->Child != NULL )
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

    if( ( Track->TextureID1 == 0 )
     && ( Track->TextureID2 == 0 ) ) {
        return;
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {
            Track->EnvironmentSet();
            if( Track->TextureID1 != 0 ) {
                Bind( Track->TextureID1 );
                m_geometry.draw( std::begin( Track->Geometry1 ), std::end( Track->Geometry1 ) );
            }
            if( Track->TextureID2 != 0 ) {
                Bind( Track->TextureID2 );
                m_geometry.draw( std::begin( Track->Geometry2 ), std::end( Track->Geometry2 ) );
            }
            Track->EnvironmentReset();
            break;
        }
        case rendermode::pickscenery:
        case rendermode::shadows: {
            if( Track->TextureID1 != 0 ) {
                Bind( Track->TextureID1 );
                m_geometry.draw( std::begin( Track->Geometry1 ), std::end( Track->Geometry1 ) );
            }
            if( Track->TextureID2 != 0 ) {
                Bind( Track->TextureID2 );
                m_geometry.draw( std::begin( Track->Geometry2 ), std::end( Track->Geometry2 ) );
            }
            break;
        }
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }
}

void
opengl_renderer::Render( TMemCell *Memcell ) {

    ::glPushMatrix();
    auto const position = Memcell->Position() - m_renderpass.camera.position();
    ::glTranslated( position.x, position.y + 0.5, position.z );

    switch( m_renderpass.draw_mode ) {
        case rendermode::color: {
            ::glPushAttrib( GL_ENABLE_BIT );
            ::glDisable( GL_TEXTURE_2D );
            ::glColor3f( 0.36f, 0.75f, 0.35f );

            ::gluSphere( m_quadric, 0.35, 4, 2 );

            ::glPopAttrib();
            break;
        }
        case rendermode::pickscenery:
        case rendermode::shadows: {
            ::gluSphere( m_quadric, 0.35, 4, 2 );
            break;
        }
        case rendermode::pickcontrols: {
            break;
        }
        default: {
            break;
        }
    }

    ::glPopMatrix();
}

bool
opengl_renderer::Render_Alpha( TGround *Ground ) {

    TGroundNode *node;
    TSubRect *tmp;
    // Ra: renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
    for( auto subcellpair = std::rbegin( m_drawqueue ); subcellpair != std::rend( m_drawqueue ); ++subcellpair ) {
        // przezroczyste trójkąty w oddzielnym cyklu przed modelami
        tmp = subcellpair->second;
        for( node = tmp->nRenderRectAlpha; node; node = node->nNext3 ) {
            Render_Alpha( node );
        }
    }
    for( auto subcellpair = std::rbegin( m_drawqueue ); subcellpair != std::rend( m_drawqueue ); ++subcellpair )
    { // renderowanie przezroczystych modeli oraz pojazdów
        Render_Alpha( subcellpair->second );
    }

    ::glDisable( GL_LIGHTING ); // linie nie powinny świecić

    for( auto subcellpair = std::rbegin( m_drawqueue ); subcellpair != std::rend( m_drawqueue ); ++subcellpair ) {
        // druty na końcu, żeby się nie robiły białe plamy na tle lasu
        tmp = subcellpair->second;
        for( node = tmp->nRenderWires; node; node = node->nNext3 ) {
            Render_Alpha( node );
        }
    }

    ::glEnable( GL_LIGHTING );

    return true;
}

bool
opengl_renderer::Render_Alpha( TSubRect *Groundsubcell ) {

    TGroundNode *node;
    for( node = Groundsubcell->nRenderMixed; node; node = node->nNext3 )
        Render_Alpha( node ); // przezroczyste z mieszanych modeli
    for( node = Groundsubcell->nRenderAlpha; node; node = node->nNext3 )
        Render_Alpha( node ); // przezroczyste modele
    for( int trackidx = 0; trackidx < Groundsubcell->iTracks; ++trackidx ) {
        for( auto dynamic : Groundsubcell->tTracks[ trackidx ]->Dynamics ) {
            Render_Alpha( dynamic ); // przezroczyste fragmenty pojazdów na torach
        }
    }

    return true;
}

bool
opengl_renderer::Render_Alpha( TGroundNode *Node ) {

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = SquareMagnitude( ( Node->pCenter - Global::pCameraPosition ) / Global::ZoomFactor );
            break;
        }
        default: {
            distancesquared = SquareMagnitude( ( Node->pCenter - m_renderpass.camera.position() ) / Global::ZoomFactor );
            break;
        }
    }
    if( ( distancesquared > ( Node->fSquareRadius    * Global::fDistanceFactor ) )
     || ( distancesquared < ( Node->fSquareMinRadius / Global::fDistanceFactor ) ) ) {
        return false;
    }

    switch (Node->iType)
    {
        case TP_TRACTION: {
            if( Node->bVisible ) {
                // rysuj jesli sa druty i nie zerwana
                if( ( Node->hvTraction->Wires == 0 )
                 || ( true == TestFlag( Node->hvTraction->DamageFlag, 128 ) ) ) {
                    return false;
                }
                // setup
                if( !Global::bSmoothTraction ) {
                    // na liniach kiepsko wygląda - robi gradient
                    ::glDisable( GL_LINE_SMOOTH );
                }
                float const linealpha = static_cast<float>(
                    std::min(
                        1.25,
                        5000 * Node->hvTraction->WireThickness / ( distancesquared + 1.0 ) ) ); // zbyt grube nie są dobre
                ::glLineWidth( linealpha );
                // McZapkie-261102: kolor zalezy od materialu i zasniedzenia
                auto const color { Node->hvTraction->wire_color() };
                ::glColor4f( color.r, color.g, color.b, linealpha );

                Bind( NULL );

                ::glPushMatrix();
                auto const originoffset = Node->m_rootposition - m_renderpass.camera.position();
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

                // render
                m_geometry.draw( Node->hvTraction->m_geometry );

                // post-render cleanup
                ::glPopMatrix();

                ::glLineWidth( 1.0 );
                if( !Global::bSmoothTraction ) {
                    ::glEnable( GL_LINE_SMOOTH );
                }

                return true;
            }
            else {
                return false;
            }
        }
        case TP_MODEL: {

            Node->Model->RaPrepare();
            if( Node->Model->pModel ) {
                // renderowanie rekurencyjne submodeli
                switch( m_renderpass.draw_mode ) {
                    case rendermode::shadows: {
                        Render_Alpha(
                            Node->Model->pModel,
                            Node->Model->Material(),
                            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
                            SquareMagnitude( Node->pCenter - Global::pCameraPosition ),
                            Node->pCenter - m_renderpass.camera.position(),
                            Node->Model->vAngle );
                        break;
                    }
                    default: {
                        auto const position = Node->pCenter - m_renderpass.camera.position();
                        Render_Alpha(
                            Node->Model->pModel,
                            Node->Model->Material(),
                            SquareMagnitude( position ),
                            position,
                            Node->Model->vAngle );
                        break;
                    }
                }
            }
            return true;
        }

        case GL_LINES: {
            if( ( Node->Piece->geometry == NULL )
             || ( Node->fLineThickness < 0.0 ) ) {
                return false;
            }
            // setup
            auto const distance = std::sqrt( distancesquared );
            auto const linealpha =
                10.0 * Node->fLineThickness
                / std::max(
                    0.5 * Node->m_radius + 1.0,
                    distance - ( 0.5 * Node->m_radius ) );
            ::glColor4fv(
                glm::value_ptr(
                    glm::vec4(
                        Node->Diffuse * glm::vec3( Global::DayLight.ambient ), // w zaleznosci od koloru swiatla
                        std::min( 1.0, linealpha ) ) ) );
            auto const linewidth = clamp( 0.5 * linealpha + Node->fLineThickness * Node->m_radius / 1000.0, 1.0, 32.0 );
            if( linewidth > 1.0 ) {
                ::glLineWidth( static_cast<float>(linewidth) );
            }

            GfxRenderer.Bind( 0 );

            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - m_renderpass.camera.position();
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            // render
            m_geometry.draw( Node->Piece->geometry );

            // post-render cleanup
            ::glPopMatrix();

            if( linewidth > 1.0 ) { ::glLineWidth( 1.0f ); }

            return true;
        }

        case GL_TRIANGLES: {
            if( ( Node->Piece->geometry == NULL )
             || ( ( Node->iFlags & 0x20 ) == 0 ) ) {
                return false;
            }
            // setup
            ::glColor3fv( glm::value_ptr( Node->Diffuse ) );

            Bind( Node->TextureID );

            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - m_renderpass.camera.position();
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            // render
            m_geometry.draw( Node->Piece->geometry );

            // post-render cleanup
            ::glPopMatrix();

            return true;
        }

        default: { break; }
    }
    // in theory we shouldn't ever get here but, eh
    return false;
}

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) { return false; }

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    auto const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    double squaredistance;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            squaredistance = SquareMagnitude( ( Dynamic->vPosition - Global::pCameraPosition ) / Global::ZoomFactor );
            break;
        }
        default: {
            squaredistance = SquareMagnitude( originoffset / Global::ZoomFactor );
            break;
        }
    }
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu
    ::glPushMatrix();

    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
    ::glMultMatrixd( Dynamic->mMatrix.getArray() );

    if( Dynamic->fShade > 0.0f ) {
        // change light level based on light level of the occupied track
        Global::DayLight.apply_intensity( Dynamic->fShade );
    }
    m_renderspecular = true;

    // render
    if( Dynamic->mdLowPolyInt ) {
        // low poly interior
        if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
            // enable cab light if needed
            if( Dynamic->InteriorLightLevel > 0.0f ) {

                // crude way to light the cabin, until we have something more complete in place
                auto const cablight = Dynamic->InteriorLight * Dynamic->InteriorLightLevel;
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &cablight.x );
            }

            Render_Alpha( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );

            if( Dynamic->InteriorLightLevel > 0.0f ) {
                // reset the overall ambient
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( m_baseambient ) );
            }
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
        Global::DayLight.apply_intensity();
    }

    ::glPopMatrix();

    if( Dynamic->btnOn )
        Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli

    return true;
}

bool
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, double const Squaredistance ) {

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
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, double const Squaredistance, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

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
     && ( TSubModel::fSquareDist >= ( Submodel->fSquareMinDist / Global::fDistanceFactor ) )
     && ( TSubModel::fSquareDist <= ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) ) {

        if( Submodel->iFlags & 0xC000 ) {
            ::glPushMatrix();
            if( Submodel->fMatrix )
                ::glMultMatrixf( Submodel->fMatrix->readArray() );
            if( Submodel->b_aAnim )
                Submodel->RaAnimation( Submodel->b_aAnim );
        }

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x2F ) // rysuj gdy element przezroczysty
            {
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
                // textures...
                if( Submodel->TextureID < 0 ) { // zmienialne skóry
                    Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                }
                else {
                    // również 0
                    Bind( Submodel->TextureID );
                }
                // ...colors...
                ::glColor3fv( glm::value_ptr(Submodel->f4Diffuse) ); // McZapkie-240702: zamiast ub
                if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                    ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * Global::DayLight.specular.a * m_speculartranslucentscalefactor ) );
                }
                // ...luminance
                if( Global::fLuminance < Submodel->fLight ) {
                    // zeby swiecilo na kolorowo
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( Submodel->f4Diffuse * Submodel->f4Emision.a ) );
                    // disable shadows so they don't obstruct self-lit items
                    setup_shadow_color( colors::white );
                }

                // main draw call
                m_geometry.draw( Submodel->m_geometry );

                // post-draw reset
                if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                    ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( colors::none ) );
                }
                if( Global::fLuminance < Submodel->fLight ) {
                    // restore default (lack of) brightness
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( colors::none ) );
                    setup_shadow_color( m_shadowcolor );
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
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            if( Global::fLuminance < Submodel->fLight ) {
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

                float glarelevel = 0.6f; // luminosity at night is at level of ~0.1, so the overall resulting transparency is ~0.5 at full 'brightness'
                if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) {

                    glarelevel *= ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                    glarelevel = std::max( 0.0f, glarelevel - static_cast<float>(Global::fLuminance) );

                    if( glarelevel > 0.0f ) {
                        // setup
                        ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );

                        Bind( m_glaretexture );
                        ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], glarelevel );
                        ::glDisable( GL_LIGHTING );
                        ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                        ::glPushMatrix();
                        ::glLoadIdentity(); // macierz jedynkowa
                        ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
                        ::glRotated( std::atan2( lightcenter.x, lightcenter.z ) * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // jedynie obracamy w pionie o kąt
                                                                                                                 // disable shadows so they don't obstruct self-lit items
                        setup_shadow_color( colors::white );

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
                        setup_shadow_color( m_shadowcolor );

                        ::glPopMatrix();
                        ::glPopAttrib();
                    }
                }
            }
        }

        if( Submodel->Child != NULL ) {
            if( Submodel->eType == TP_TEXT ) { // tekst renderujemy w specjalny sposób, zamiast submodeli z łańcucha Child
                int i, j = (int)Submodel->pasText->size();
                TSubModel *p;
                if( !Submodel->smLetter ) { // jeśli nie ma tablicy, to ją stworzyć; miejsce nieodpowiednie, ale tymczasowo może być
                    Submodel->smLetter = new TSubModel *[ 256 ]; // tablica wskaźników submodeli dla wyświetlania tekstu
                    ::ZeroMemory( Submodel->smLetter, 256 * sizeof( TSubModel * ) ); // wypełnianie zerami
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

    if( Submodel->b_aAnim < at_SecondsJump )
        Submodel->b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu

    if( Submodel->Next != NULL )
        if( Submodel->iAlpha & Submodel->iFlags & 0x2F000000 )
            Render_Alpha( Submodel->Next );
};

// utility methods
TSubModel const *
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
    mousepos.y = Global::iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    glm::ivec2 pickbufferpos;
    if( true == m_framebuffersupport ) {
//        ::glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
        pickbufferpos = glm::ivec2{
            mousepos.x * EU07_PICKBUFFERSIZE / Global::iWindowWidth,
            mousepos.y * EU07_PICKBUFFERSIZE / Global::iWindowHeight
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
    auto const controlindex = pick_index( glm::ivec3{ pickreadout[ 2 ], pickreadout[ 1 ], pickreadout[ 0 ] } );
    TSubModel const *control { nullptr };
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
    return control;
}

TGroundNode const *
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
    mousepos.y = Global::iWindowHeight - mousepos.y; // cursor coordinates are flipped compared to opengl

#ifdef EU07_USE_PICKING_FRAMEBUFFER
    glm::ivec2 pickbufferpos;
    if( true == m_framebuffersupport ) {
//        ::glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
        pickbufferpos = glm::ivec2{
            mousepos.x * EU07_PICKBUFFERSIZE / Global::iWindowWidth,
            mousepos.y * EU07_PICKBUFFERSIZE / Global::iWindowHeight
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
    TGroundNode const *node { nullptr };
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
    return node;
}

void
opengl_renderer::Update( double const Deltatime ) {

    m_updateaccumulator += Deltatime;

    if( m_updateaccumulator < 1.0 ) {
        // too early for any work
        return;
    }

    m_updateaccumulator = 0.0;
    m_framerate = 1000.f / ( m_drawtime / 20.f );

    // adjust draw ranges etc, based on recent performance
    auto const framerate = 1000.0f / (m_drawtimecolorpass / 20.0f);

    float targetfactor;
         if( framerate > 90.0 ) { targetfactor = 3.0f; }
    else if( framerate > 60.0 ) { targetfactor = 1.5f; }
    else if( framerate > 30.0 ) { targetfactor = Global::iWindowHeight / 768.0f; }
    else                        { targetfactor = Global::iWindowHeight / 768.0f * 0.75f; }

    if( targetfactor > Global::fDistanceFactor ) {

        Global::fDistanceFactor = std::min( targetfactor, Global::fDistanceFactor + 0.05f );
    }
    else if( targetfactor < Global::fDistanceFactor ) {

        Global::fDistanceFactor = std::max( targetfactor, Global::fDistanceFactor - 0.05f );
    }

    if( ( framerate < 15.0 ) && ( Global::iSlowMotion < 7 ) ) {
        Global::iSlowMotion = ( Global::iSlowMotion << 1 ) + 1; // zapalenie kolejnego bitu
        if( Global::iSlowMotionMask & 1 )
            if( Global::iMultisampling ) // a multisampling jest włączony
                ::glDisable( GL_MULTISAMPLE ); // wyłączenie multisamplingu powinno poprawić FPS
    }
    else if( ( framerate > 20.0 ) && Global::iSlowMotion ) { // FPS się zwiększył, można włączyć bajery
        Global::iSlowMotion = ( Global::iSlowMotion >> 1 ); // zgaszenie bitu
        if( Global::iSlowMotion == 0 ) // jeśli jest pełna prędkość
            if( Global::iMultisampling ) // a multisampling jest włączony
                ::glEnable( GL_MULTISAMPLE );
    }

    if( true == World.InitPerformed() ) {
        // garbage collection
        m_geometry.update();
        m_textures.update();
    }

    if( true == DebugModeFlag ) {
        m_debuginfo += m_textures.info();
    }

    if( ( true  == Global::ControlPicking )
     && ( false == FreeFlyModeFlag ) ) {
        Update_Pick_Control();
    }
    else {
        m_pickcontrolitem = nullptr;
    }
    // temporary conditions for testing. eventually will be coupled with editor mode
    if( ( true == Global::ControlPicking )
     && ( true == DebugModeFlag ) 
     && ( true == FreeFlyModeFlag ) ) {
        Update_Pick_Node();
    }
    else {
        m_picksceneryitem = nullptr;
    }
};

// debug performance string
std::string const &
opengl_renderer::Info() const {

    return m_debuginfo;
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
            return ( glm::length2( camera - Left.position ) * ( 1.f - Left.intensity ) ) < ( glm::length2( camera - Right.position ) * ( 1.f - Right.intensity ) ); } );

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
        renderlight->set_position( lightoffset );
        renderlight->direction = scenelight.direction;

        auto luminance = static_cast<float>( Global::fLuminance );
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

    if( !GLEW_VERSION_1_5 ) {
        ErrorLog( "Requires openGL >= 1.5" );
        return false;
    }

    WriteLog( "Supported extensions:" +  std::string((char *)glGetString( GL_EXTENSIONS )) );

    WriteLog( std::string("Render path: ") + ( Global::bUseVBO ? "VBO" : "Display lists" ) );
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
        Global::iMaxTextureSize = std::min( Global::iMaxTextureSize, texturesize );
        WriteLog( "Texture sizes capped at " + std::to_string( Global::iMaxTextureSize ) + " pixels" );
        m_shadowbuffersize = Global::shadowtune.map_size;
        m_shadowbuffersize = std::min( m_shadowbuffersize, texturesize );
        WriteLog( "Shadows map size capped at " + std::to_string( m_shadowbuffersize ) + " pixels" );
    }
    // cap the number of supported lights based on hardware
    {
        GLint maxlights;
        ::glGetIntegerv( GL_MAX_LIGHTS, &maxlights );
        Global::DynamicLightCount = std::min( Global::DynamicLightCount, maxlights - 1 );
        WriteLog( "Dynamic light amount capped at " + std::to_string( Global::DynamicLightCount ) + " (" + std::to_string(maxlights) + " lights total supported by the gfx card)" );
    }
    {
        GLint maxtextureunits;
        ::glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxtextureunits );
        if( maxtextureunits < 4 ) {
            WriteLog( "Less than 4 texture units, shadow and reflection mapping will be disabled" );
            Global::RenderShadows = false;
            m_diffusetextureunit = GL_TEXTURE0;
            m_shadowtextureunit = -1;
            m_helpertextureunit = -1;
        }
    }
    if( Global::iMultisampling ) {
        WriteLog( "Using multisampling x" + std::to_string( 1 << Global::iMultisampling ) );
    }

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
