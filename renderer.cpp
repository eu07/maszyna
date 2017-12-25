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
#include "Globals.h"
#include "Timer.h"
#include "World.h"
#include "Train.h"
#include "DynObj.h"
#include "AnimModel.h"
#include "Traction.h"
#include "simulation.h"
#include "uilayer.h"
#include "Logs.h"
#include "usefull.h"

#define EU07_NEW_CAB_RENDERCODE

opengl_renderer GfxRenderer;
extern TWorld World;

int const EU07_PICKBUFFERSIZE { 1024 }; // size of (square) textures bound with the pick framebuffer
int const EU07_ENVIRONMENTBUFFERSIZE { 256 }; // size of (square) environmental cube map texture

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
opengl_camera::visible( scene::bounding_area const &Area ) const {

    return ( m_frustum.sphere_inside( Area.center, Area.radius ) > 0.f );
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

    m_geometry.units().texture = (
        Global::BasicRenderer ?
            std::vector<GLint>{ m_diffusetextureunit } :
            std::vector<GLint>{ m_normaltextureunit, m_diffusetextureunit } );
    m_textures.assign_units( m_helpertextureunit, m_shadowtextureunit, m_normaltextureunit, m_diffusetextureunit ); // TODO: add reflections unit
    UILayer.set_unit( m_diffusetextureunit );
    select_unit( m_diffusetextureunit );

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
            Global::RenderShadows = false;
        }
        ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target for now
    }
    // environment cube map resources
    if( ( false == Global::BasicRenderer )
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
    auto const geometrybank = m_geometry.create_bank();
    float const size = 2.5f;
    m_billboardgeometry = m_geometry.create_chunk(
        gfx::vertex_array{
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

    Timer::subsystem.gfx_total.stop();
    Timer::subsystem.gfx_total.start(); // note: gfx_total is actually frame total, clean this up
    Timer::subsystem.gfx_color.start();

    m_renderpass.draw_mode = rendermode::none; // force setup anew
    m_debugtimestext.clear();
    m_debugstats = debug_stats();
    Render_pass( rendermode::color );
    Timer::subsystem.gfx_color.stop();

    Timer::subsystem.gfx_swap.start();
    glfwSwapBuffers( m_window );
    Timer::subsystem.gfx_swap.stop();

    m_drawcount = m_cellqueue.size();
    m_debugtimestext
        += "frame: " + to_string( Timer::subsystem.gfx_color.average(), 2 ) + " msec (" + std::to_string( m_cellqueue.size() ) + " sectors) "
        += "gpu side: " + to_string( Timer::subsystem.gfx_swap.average(), 2 ) + " msec "
        += "(" + to_string( Timer::subsystem.gfx_color.average() + Timer::subsystem.gfx_swap.average(), 2 ) + " msec total)";
    m_debugstatstext =
        "drawcalls: " + to_string( m_debugstats.drawcalls )
        + "; dyn: " + to_string( m_debugstats.dynamics ) + " mod: " + to_string( m_debugstats.models ) + " sub: " + to_string( m_debugstats.submodels )
        + "; trk: " + to_string( m_debugstats.paths ) + " shp: " + to_string( m_debugstats.shapes )
        + " trc: " + to_string( m_debugstats.traction ) + " lin: " + to_string( m_debugstats.lines );

    ++m_framestamp;

    return true; // for now always succeed
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

            opengl_camera shadowcamera; // temporary helper, remove once ortho shadowmap code is done
            if( ( true == Global::RenderShadows )
             && ( false == Global::bWireFrame )
             && ( true == World.InitPerformed() )
             && ( m_shadowcolor != colors::white ) ) {
                // run shadowmap pass before color
                Render_pass( rendermode::shadows );
#ifdef EU07_USE_DEBUG_SHADOWMAP
                UILayer.set_texture( m_shadowdebugtexture );
#endif
                shadowcamera = m_renderpass.camera; // cache shadow camera placement for visualization
                setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack
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
            if( ( true == m_environmentcubetexturesupport )
             && ( true == World.InitPerformed() ) ) {
                // potentially update environmental cube map
                if( true == Render_reflections() ) {
                    setup_pass( m_renderpass, Mode ); // restore draw mode. TBD, TODO: render mode stack
                }
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
                    if( ( true == Global::RenderShadows ) && ( false == Global::bWireFrame ) ) {
                        shadowcamera.draw( m_renderpass.camera.position() - shadowcamera.position() );
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
#ifdef EU07_NEW_CAB_RENDERCODE
                if( World.Train != nullptr ) {
                    // cab render is performed without shadows, due to low resolution and number of models without windows :|
                    switch_units( true, false, false );
                    Render_cab( World.Train->Dynamic(), false );
                }
#endif
                switch_units( true, true, true );
                Render( simulation::Region );
/*
                // debug: audio nodes
                for( auto const &audiosource : audio::renderer.m_sources ) {

                    ::glPushMatrix();
                    auto const position = audiosource.properties.location - m_renderpass.camera.position();
                    ::glTranslated( position.x, position.y, position.z );

                    ::glPushAttrib( GL_ENABLE_BIT );
                    ::glDisable( GL_TEXTURE_2D );
                    ::glColor3f( 0.36f, 0.75f, 0.35f );

                    ::gluSphere( m_quadric, 0.125, 4, 2 );

                    ::glPopAttrib();

                    ::glPopMatrix();
                }
*/
                // ...translucent parts
                setup_drawing( true );
                Render_Alpha( simulation::Region );
                if( World.Train != nullptr ) {
                    // cab render is performed without shadows, due to low resolution and number of models without windows :|
                    switch_units( true, false, false );
                    Render_cab( World.Train->Dynamic(), true );
                }

                if( m_environmentcubetexturesupport ) {
                    // restore default texture matrix for reflections cube map
                    select_unit( m_helpertextureunit );
                    ::glMatrixMode( GL_TEXTURE );
                    ::glPopMatrix();
                    select_unit( m_diffusetextureunit );
                    ::glMatrixMode( GL_MODELVIEW );
                }
            }
            UILayer.render();
            break;
        }

        case rendermode::shadows: {

            if( World.InitPerformed() ) {
                // setup
                Timer::subsystem.gfx_shadows.start();

                ::glBindFramebufferEXT( GL_FRAMEBUFFER, m_shadowframebuffer );

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
                ::glPolygonOffset( 1.f, 1.f );
                // render
                // opaque parts...
                setup_drawing( false );
#ifdef EU07_USE_DEBUG_SHADOWMAP
                setup_units( true, false, false );
#else
                setup_units( false, false, false );
#endif
                Render( simulation::Region );
                // post-render restore
                ::glDisable( GL_POLYGON_OFFSET_FILL );
                ::glDisable( GL_SCISSOR_TEST );

                ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch back to primary render target
                Timer::subsystem.gfx_shadows.stop();
                m_debugtimestext += "shadows: " + to_string( Timer::subsystem.gfx_shadows.average(), 2 ) + " msec (" + std::to_string( m_cellqueue.size() ) + " sectors) ";
            }
            break;
        }

        case rendermode::reflections: {
            // NOTE: buffer and viewport setup in this mode is handled by the wrapper method
            ::glClearColor( 0.f, 0.f, 0.f, 1.f );
            ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            if( World.InitPerformed() ) {

                ::glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE );
                // setup
                setup_matrices();
                // render
                setup_drawing( true );
                setup_units( true, false, false );
                Render( &World.Environment );
                // opaque parts...
                setup_drawing( false );
                setup_units( true, true, true );
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

// creates dynamic environment cubemap
bool
opengl_renderer::Render_reflections() {

    auto const &time = simulation::Time.data();
    auto const timestamp = time.wDay * 24 * 60 + time.wHour * 60 + time.wMinute;
    if( ( timestamp - m_environmentupdatetime < 5 )
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

        ::glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, m_environmentcubetextureface, m_environmentcubetexture, 0 );
        Render_pass( rendermode::reflections );
    }
    ::glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

    return true;
}

void
opengl_renderer::setup_pass( renderpass_config &Config, rendermode const Mode, float const Znear, float const Zfar, bool const Ignoredebug ) {

    Config.draw_mode = Mode;

    if( false == World.InitPerformed() ) { return; }
    // setup draw range
    switch( Mode ) {
        case rendermode::color:        { Config.draw_range = Global::BaseDrawRange; break; }
        case rendermode::shadows:      { Config.draw_range = Global::BaseDrawRange * 0.5f; break; }
        case rendermode::reflections:  { Config.draw_range = Global::BaseDrawRange; break; }
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
                    std::min( Global::DayLight.direction.y, -0.2f ),
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
            // quantize the frustum points and add some padding, to reduce shadow shimmer on scale changes
            auto const quantizationstep{ std::min( Global::shadowtune.depth, 50.f ) };
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
                    -Global::shadowtune.width, Global::shadowtune.width,
                    -Global::shadowtune.width, Global::shadowtune.width,
                    -Global::shadowtune.depth, Global::shadowtune.depth );
            camera.position() = Global::pCameraPosition - glm::dvec3{ Global::DayLight.direction };
            if( camera.position().y - Global::pCameraPosition.y < 0.1 ) {
                camera.position().y = Global::pCameraPosition.y + 0.1;
            }
            viewmatrix *= glm::lookAt(
                camera.position(),
                glm::dvec3{ Global::pCameraPosition },
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
            camera.projection() = glm::translate( glm::mat4{ 1.f }, glm::vec3{ shadowmapadjustment, 0.f } ) * camera.projection();

            break;
        }
        case rendermode::reflections: {
            // projection
            camera.projection() *=
                glm::perspective(
                    glm::radians( 90.f ),
                    1.f,
                    0.1f * Global::ZoomFactor,
                    Config.draw_range * Global::fDistanceFactor );
            // modelview
            camera.position() = (
                ( ( true == DebugCameraFlag ) && ( false == Ignoredebug ) ) ?
                    Global::DebugCameraPosition :
                    Global::pCameraPosition );
            glm::dvec3 const cubefacetargetvectors[ 6 ] = { {  1.0,  0.0,  0.0 }, { -1.0,  0.0,  0.0 }, {  0.0,  1.0,  0.0 }, {  0.0, -1.0,  0.0 }, {  0.0,  0.0,  1.0 }, {  0.0,  0.0, -1.0 } };
            glm::dvec3 const cubefaceupvectors[ 6 ] =     { {  0.0, -1.0,  0.0 }, {  0.0, -1.0,  0.0 }, {  0.0,  0.0,  1.0 }, {  0.0,  0.0, -1.0 }, {  0.0, -1.0,  0.0 }, {  0.0, -1.0,  0.0 } };
            auto const cubefaceindex = m_environmentcubetextureface - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            viewmatrix *=
                glm::lookAt(
                    camera.position(),
                    camera.position() + cubefacetargetvectors[ cubefaceindex ],
                    cubefaceupvectors[ cubefaceindex ] );
            break;
        }

        case rendermode::pickcontrols:
        case rendermode::pickscenery: {
            // TODO: scissor test for pick modes
            // projection
            camera.projection() *=
                glm::perspective(
                    glm::radians( Global::FieldOfView / Global::ZoomFactor ),
                    std::max( 1.f, (float)Global::iWindowWidth ) / std::max( 1.f, (float)Global::iWindowHeight ),
                    0.1f * Global::ZoomFactor,
                    Config.draw_range * Global::fDistanceFactor );
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

    if( ( m_renderpass.draw_mode == rendermode::color )
     && ( m_environmentcubetexturesupport ) ) {
        // special case, for colour render pass setup texture matrix for reflections cube map
        select_unit( m_helpertextureunit );
        ::glMatrixMode( GL_TEXTURE );
        ::glPushMatrix();
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
        ::glAlphaFunc( GL_GREATER, 0.0f );
    }
    else {
        ::glDisable( GL_BLEND );
        ::glAlphaFunc( GL_GREATER, 0.50f );
    }

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections: {
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

        select_unit( m_helpertextureunit );

        if( ( true == Reflections )
         || ( ( true == Global::RenderShadows ) && ( true == Shadows ) && ( false == Global::bWireFrame ) ) ) {
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

        if( ( true == Global::RenderShadows ) && ( true == Shadows ) && ( false == Global::bWireFrame ) ) {

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
        if( ( true == Global::RenderShadows )
         && ( true == Shadows )
         && ( false == Global::bWireFrame )
         && ( m_shadowcolor != colors::white ) ) {

            select_unit( m_shadowtextureunit );
            // NOTE: shadowmap isn't part of regular texture system, so we use direct bind call here
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

// enables and disables specified texture units
void
opengl_renderer::switch_units( bool const Diffuse, bool const Shadows, bool const Reflections ) {
    // helper texture unit.
    if( m_helpertextureunit >= 0 ) {

        select_unit( m_helpertextureunit );
        if( ( true == Reflections )
         || ( ( true == Global::RenderShadows )
           && ( true == Shadows )
           && ( false == Global::bWireFrame )
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
        if( ( true == Global::RenderShadows ) && ( true == Shadows ) && ( false == Global::bWireFrame ) ) {

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

    Bind_Material( null_handle );
    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_DEPTH_TEST );
    ::glDepthMask( GL_FALSE );
    ::glPushMatrix();

    // skydome
    Environment->m_skydome.Render();
    // skydome uses a custom vbo which could potentially confuse the main geometry system. hardly elegant but, eh
    gfx::opengl_vbogeometrybank::reset();
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
        Bind_Texture( m_suntexture );
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
        Bind_Texture( m_moontexture );
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
gfx::geometrybank_handle
opengl_renderer::Create_Bank() {

    return m_geometry.create_bank();
}

// creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle
opengl_renderer::Insert( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) {

    return m_geometry.create_chunk( Vertices, Geometry, Type );
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
opengl_renderer::Replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset ) {

    return m_geometry.replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
opengl_renderer::Append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) {

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

void
opengl_renderer::Bind_Material( material_handle const Material ) {

    auto const &material = m_materials.material( Material );
    if( false == Global::BasicRenderer ) {
        m_textures.bind( textureunit::normals, material.texture2 );
    }
    m_textures.bind( textureunit::diffuse, material.texture1 );
}

opengl_material const &
opengl_renderer::Material( material_handle const Material ) const {

    return m_materials.material( Material );
}

// texture methods
void
opengl_renderer::select_unit( GLint const Textureunit ) {

    return m_textures.unit( Textureunit );
}

texture_handle
opengl_renderer::Fetch_Texture( std::string const &Filename, bool const Loadnow ) {

    return m_textures.create( Filename, Loadnow );
}

void
opengl_renderer::Bind_Texture( texture_handle const Texture ) {

    m_textures.bind( textureunit::diffuse, Texture );
}

opengl_texture const &
opengl_renderer::Texture( texture_handle const Texture ) const {

    return m_textures.texture( Texture );
}

void
opengl_renderer::Render( scene::basic_region *Region ) {

    m_sectionqueue.clear();
    m_cellqueue.clear();
/*
    for( auto *section : Region->sections( m_renderpass.camera.position(), m_renderpass.draw_range * Global::fDistanceFactor ) ) {
#ifdef EU07_USE_DEBUG_CULLING
        if( m_worldcamera.camera.visible( section->m_area ) ) {
#else
        if( m_renderpass.camera.visible( section->m_area ) ) {
#endif
            m_sectionqueue.emplace_back( section );
        }
    }
*/
    // build a list of region sections to render
    glm::vec3 const cameraposition { m_renderpass.camera.position() };
    auto const camerax = static_cast<int>( std::floor( cameraposition.x / scene::EU07_SECTIONSIZE + scene::EU07_REGIONSIDESECTIONCOUNT / 2 ) );
    auto const cameraz = static_cast<int>( std::floor( cameraposition.z / scene::EU07_SECTIONSIZE + scene::EU07_REGIONSIDESECTIONCOUNT / 2 ) );
    int const segmentcount = 2 * static_cast<int>( std::ceil( m_renderpass.draw_range * Global::fDistanceFactor / scene::EU07_SECTIONSIZE ) );
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
#ifdef EU07_USE_DEBUG_CULLING
             && ( m_worldcamera.camera.visible( section->m_area ) ) ) {
#else
             && ( m_renderpass.camera.visible( section->m_area ) ) ) {
#endif
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
        case rendermode::pickscenery: {
            // these render modes don't bother with lights
            Render( std::begin( m_sectionqueue ), std::end( m_sectionqueue ) );
            // they can also skip queue sorting, as they only deal with opaque geometry
            // NOTE: there's benefit from rendering front-to-back, but is it significant enough? TODO: investigate
            Render( std::begin( m_cellqueue ), std::end( m_cellqueue ) );
            break;
        }
        case rendermode::reflections: {
            // for the time being reflections render only terrain geometry
            Render( std::begin( m_sectionqueue ), std::end( m_sectionqueue ) );
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
        case rendermode::shadows: {
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
            case rendermode::pickscenery: {
                if( false == section->m_shapes.empty() ) {
                    // since all shapes of the section share center point we can optimize out a few calls here
                    ::glPushMatrix();
                    auto const originoffset { section->m_area.center - m_renderpass.camera.position() };
                    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                    // render
#ifdef EU07_USE_DEBUG_CULLING
                    // debug
                    ::glLineWidth( 2.f );
                    float const width = section->m_area.radius;
                    float const height = section->m_area.radius * 0.2f;
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
                    // shapes
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
#ifdef EU07_USE_DEBUG_CULLING
                     && ( m_worldcamera.camera.visible( cell.m_area ) ) ) {
#else
                     && ( m_renderpass.camera.visible( cell.m_area ) ) ) {
#endif
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
#ifdef EU07_USE_DEBUG_CULLING
                // debug
                float const width = cell->m_area.radius;
                float const height = cell->m_area.radius * 0.15f;
                glDisable( GL_LIGHTING );
                glDisable( GL_TEXTURE_2D );
                glColor3ub( 255, 255, 0 );
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
#endif
                // opaque non-instanced shapes
                for( auto const &shape : cell->m_shapesopaque ) { Render( shape, false ); }
                // tracks
                // TODO: update after path node refactoring
                Render( std::begin( cell->m_paths ), std::end( cell->m_paths ) );
#ifdef EU07_SCENERY_EDITOR
                // TODO: re-implement
                // memcells
                if( EditorModeFlag ) {
                    for( auto const memcell : Groundsubcell->m_memcells ) {
                        Render( memcell );
                    }
                }
#endif
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
                for( auto const &shape : cell->m_shapesopaque ) { Render( shape, false ); }
                // tracks
                Render( std::begin( cell->m_paths ), std::end( cell->m_paths ) );

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
#ifdef EU07_SCENERY_EDITOR
                // memcells
                // TODO: re-implement
                if( EditorModeFlag ) {
                    for( auto const memcell : Groundsubcell->m_memcells ) {
                        ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                        Render( memcell );
                    }
                }
#endif
                // post-render cleanup
                ::glPopMatrix();

                break;
            }
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
            case rendermode::pickscenery: {
                // opaque parts of instanced models
                // same procedure like with regular render, but each node receives custom colour used for picking
                for( auto *instance : cell->m_instancesopaque ) {
                    ::glColor3fv( glm::value_ptr( pick_color( m_picksceneryitems.size() + 1 ) ) );
                    Render( instance );
                }
                // vehicles aren't included in scenery picking for the time being
                break;
            }
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
                distancesquared = SquareMagnitude( ( data.area.center - Global::pCameraPosition ) / Global::ZoomFactor ) / Global::fDistanceFactor;
                break;
            }
            default: {
                distancesquared = SquareMagnitude( ( data.area.center - m_renderpass.camera.position() ) / Global::ZoomFactor ) / Global::fDistanceFactor;
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
            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( data.lighting.specular * Global::DayLight.specular.a * m_specularopaquescalefactor ) );
*/
            break;
        }
        // pick modes are painted with custom colours, and shadow pass doesn't use any
        case rendermode::shadows:
        case rendermode::pickscenery:
        case rendermode::pickcontrols:
        default: {
            break;
        }
    }
    // render
    m_geometry.draw( data.geometry );
    // debug data
    ++m_debugstats.shapes;
    ++m_debugstats.drawcalls;
}

void
opengl_renderer::Render( TAnimModel *Instance ) {

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = SquareMagnitude( ( Instance->location() - Global::pCameraPosition ) / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
        default: {
            distancesquared = SquareMagnitude( ( Instance->location() - m_renderpass.camera.position() ) / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
    }
    if( ( distancesquared <  Instance->m_rangesquaredmin )
     || ( distancesquared >= Instance->m_rangesquaredmax ) ) {
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
    }
}

bool
opengl_renderer::Render( TDynamicObject *Dynamic ) {

    Dynamic->renderme = m_renderpass.camera.visible( Dynamic );
    if( false == Dynamic->renderme ) {
        return false;
    }
    // debug data
    ++m_debugstats.dynamics;

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    // lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
    float squaredistance;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            squaredistance = glm::length2( glm::vec3{ glm::dvec3{ Dynamic->vPosition - Global::pCameraPosition } } / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
        default: {
            squaredistance = glm::length2( glm::vec3{ originoffset } / Global::ZoomFactor ) / Global::fDistanceFactor;
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
                        ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( Dynamic->InteriorLight * Dynamic->InteriorLightLevel ) );
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
opengl_renderer::Render_cab( TDynamicObject *Dynamic, bool const Alpha ) {

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
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( Dynamic->InteriorLight * Dynamic->InteriorLightLevel ) );
                }
                // render
#ifdef EU07_NEW_CAB_RENDERCODE
                if( true == Alpha ) {
                    // translucent parts
                    Render_Alpha( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                }
                else {
                    // opaque parts
                    Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                }
#else
                Render( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
                Render_Alpha( Dynamic->mdKabina, Dynamic->Material(), 0.0 );
#endif
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
                        if( true == m_renderspecular ) {
                            ::glEnable( GL_NORMALIZE );
                        }
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
                        // ...colors...
                        ::glColor3fv( glm::value_ptr( Submodel->f4Diffuse ) ); // McZapkie-240702: zamiast ub
                        if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                            // specular strength in legacy models is set uniformly to 150, 150, 150 so we scale it down for opaque elements
                            ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * Global::DayLight.specular.a * m_specularopaquescalefactor ) );
                            ::glEnable( GL_RESCALE_NORMAL );
                        }
                        // ...luminance
                        auto const unitstate = m_unitstate;
                        if( Global::fLuminance < Submodel->fLight ) {
                            // zeby swiecilo na kolorowo
                            ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( Submodel->f4Diffuse * Submodel->f4Emision.a ) );
                            // disable shadows so they don't obstruct self-lit items
/*
                            setup_shadow_color( colors::white );
*/
                            switch_units( m_unitstate.diffuse, false, false );
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
/*
                            setup_shadow_color( m_shadowcolor );
*/
                            switch_units( m_unitstate.diffuse, unitstate.shadows, unitstate.reflections );
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
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            // również 0
                            Bind_Material( Submodel->m_material );
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
                        if( Submodel->m_material < 0 ) { // zmienialne skóry
                            Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                        }
                        else {
                            // również 0
                            Bind_Material( Submodel->m_material );
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
                        float const anglefactor = ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                        // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                        // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                        float const distancefactor = std::max( 0.5f, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / Submodel->fSquareMaxDist );

                        if( lightlevel > 0.f ) {
                            // material configuration:
                            ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT );

                            Bind_Material( null_handle );
                            ::glPointSize( std::max( 3.f, 5.f * distancefactor * anglefactor ) );
                            ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], lightlevel * anglefactor );
                            ::glDisable( GL_LIGHTING );
                            ::glEnable( GL_BLEND );

                            ::glPushMatrix();
                            ::glLoadIdentity();
                            ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
/*
                            setup_shadow_color( colors::white );
*/
                            auto const unitstate = m_unitstate;
                            switch_units( m_unitstate.diffuse, false, false );

                            // main draw call
                            m_geometry.draw( Submodel->m_geometry );

                            // post-draw reset
                            // re-enable shadows
/*
                            setup_shadow_color( m_shadowcolor );
*/
                            switch_units( m_unitstate.diffuse, unitstate.shadows, unitstate.reflections );

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
                case rendermode::color:
                case rendermode::reflections: {
                    if( Global::fLuminance < Submodel->fLight ) {

                        // material configuration:
                        ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );

                        Bind_Material( null_handle );
                        ::glDisable( GL_LIGHTING );

                        // main draw call
                        m_geometry.draw( Submodel->m_geometry, gfx::color_streams );

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
     && ( Track->m_material2 == 0 ) ) {
        return;
    }
    if( false == Track->m_visible ) {
        return;
    }

    ++m_debugstats.paths;
    ++m_debugstats.drawcalls;

    switch( m_renderpass.draw_mode ) {
        case rendermode::color:
        case rendermode::reflections: {
            Track->EnvironmentSet();
            if( Track->m_material1 != 0 ) {
                Bind_Material( Track->m_material1 );
                m_geometry.draw( std::begin( Track->Geometry1 ), std::end( Track->Geometry1 ) );
            }
            if( Track->m_material2 != 0 ) {
                Bind_Material( Track->m_material2 );
                m_geometry.draw( std::begin( Track->Geometry2 ), std::end( Track->Geometry2 ) );
            }
            Track->EnvironmentReset();
            break;
        }
        case rendermode::shadows: {
            // shadow pass includes trackbeds but not tracks themselves due to low resolution of the map
            // TODO: implement
            break;
        }
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

    ::glColor3fv( glm::value_ptr( colors::white ) );
    // setup
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // NOTE: roads-based platforms tend to miss parts of shadows if rendered with either back or front culling
            ::glDisable( GL_CULL_FACE );
            break;
        }
        default: {
            break;
        }
    }

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
                track->EnvironmentSet();
                Bind_Material( track->m_material1 );
                m_geometry.draw( std::begin( track->Geometry1 ), std::end( track->Geometry1 ) );
                track->EnvironmentReset();
                break;
            }
            case rendermode::shadows: {
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
                track->EnvironmentSet();
                Bind_Material( track->m_material2 );
                m_geometry.draw( std::begin( track->Geometry2 ), std::end( track->Geometry2 ) );
                track->EnvironmentReset();
                break;
            }
            case rendermode::shadows: {
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
    // post-render reset
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
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
            ::glPushAttrib( GL_ENABLE_BIT );
            ::glDisable( GL_TEXTURE_2D );
            ::glColor3f( 0.36f, 0.75f, 0.35f );

            ::gluSphere( m_quadric, 0.35, 4, 2 );

            ::glPopAttrib();
            break;
        }
        case rendermode::shadows:
        case rendermode::pickscenery: {
            ::gluSphere( m_quadric, 0.35, 4, 2 );
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
                if( !Global::bSmoothTraction ) {
                    // na liniach kiepsko wygląda - robi gradient
                    ::glDisable( GL_LINE_SMOOTH );
                }
                Bind_Material( null_handle );
                // render
                for( auto *traction : cell->m_traction ) { Render_Alpha( traction ); }
                for( auto &lines : cell->m_lines )       { Render_Alpha( lines ); }
                // post-render cleanup
                ::glLineWidth( 1.0 );
                if( !Global::bSmoothTraction ) {
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

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = SquareMagnitude( ( Instance->location() - Global::pCameraPosition ) / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
        default: {
            distancesquared = SquareMagnitude( ( Instance->location() - m_renderpass.camera.position() ) / Global::ZoomFactor ) / Global::fDistanceFactor;
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

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = SquareMagnitude( ( Traction->location() - Global::pCameraPosition ) / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
        default: {
            distancesquared = SquareMagnitude( ( Traction->location() - m_renderpass.camera.position() ) / Global::ZoomFactor ) / Global::fDistanceFactor;
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
            1.f, 1.5f ) );
    // McZapkie-261102: kolor zalezy od materialu i zasniedzenia
    ::glColor4fv(
        glm::value_ptr(
            glm::vec4{
                Traction->wire_color(),
                linealpha } ) );
    // render
    m_geometry.draw( Traction->m_geometry );
    // debug data
    ++m_debugstats.traction;
    ++m_debugstats.drawcalls;
}

void
opengl_renderer::Render_Alpha( scene::lines_node const &Lines ) {

    auto const &data { Lines.data() };

    double distancesquared;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            // 'camera' for the light pass is the light source, but we need to draw what the 'real' camera sees
            distancesquared = SquareMagnitude( ( data.area.center - Global::pCameraPosition ) / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
        default: {
            distancesquared = SquareMagnitude( ( data.area.center - m_renderpass.camera.position() ) / Global::ZoomFactor ) / Global::fDistanceFactor;
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
    ::glLineWidth(
        clamp(
            0.5f * linealpha + data.line_width * data.area.radius / 1000.f,
            1.f, 8.f ) );
    ::glColor4fv(
        glm::value_ptr(
            glm::vec4{
                glm::vec3{ data.lighting.diffuse * Global::DayLight.ambient }, // w zaleznosci od koloru swiatla
                std::min( 1.f, linealpha ) } ) );
    // render
    m_geometry.draw( data.geometry );
    ++m_debugstats.lines;
    ++m_debugstats.drawcalls;
}

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) { return false; }

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    glm::dvec3 const originoffset = Dynamic->vPosition - m_renderpass.camera.position();
    // lod visibility ranges are defined for base (x 1.0) viewing distance. for render we adjust them for actual range multiplier and zoom
    float squaredistance;
    switch( m_renderpass.draw_mode ) {
        case rendermode::shadows: {
            squaredistance = glm::length2( glm::vec3{ glm::dvec3{ Dynamic->vPosition - Global::pCameraPosition } } / Global::ZoomFactor ) / Global::fDistanceFactor;
            break;
        }
        default: {
            squaredistance = glm::length2( glm::vec3{ originoffset } / Global::ZoomFactor ) / Global::fDistanceFactor;
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
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr( Dynamic->InteriorLight * Dynamic->InteriorLightLevel ) );
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
                if( Submodel->m_material < 0 ) { // zmienialne skóry
                    Bind_Material( Submodel->ReplacableSkinId[ -Submodel->m_material ] );
                }
                else {
                    // również 0
                    Bind_Material( Submodel->m_material );
                }
                // ...colors...
                ::glColor3fv( glm::value_ptr(Submodel->f4Diffuse) ); // McZapkie-240702: zamiast ub
                if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                    ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * Global::DayLight.specular.a * m_speculartranslucentscalefactor ) );
                }
                // ...luminance
                auto const unitstate = m_unitstate;
                if( Global::fLuminance < Submodel->fLight ) {
                    // zeby swiecilo na kolorowo
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( Submodel->f4Diffuse * Submodel->f4Emision.a ) );
                    // disable shadows so they don't obstruct self-lit items
/*
                    setup_shadow_color( colors::white );
*/
                    switch_units( m_unitstate.diffuse, false, false );
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
/*
                    setup_shadow_color( m_shadowcolor );
*/
                    switch_units( m_unitstate.diffuse, unitstate.shadows, unitstate.reflections );
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
                        ::glPushAttrib( GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT );

                        Bind_Texture( m_glaretexture );
                        ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], glarelevel );
                        ::glDisable( GL_LIGHTING );
                        ::glDepthMask( GL_FALSE );
                        ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                        ::glPushMatrix();
                        ::glLoadIdentity(); // macierz jedynkowa
                        ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
                        ::glRotated( std::atan2( lightcenter.x, lightcenter.z ) * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // jedynie obracamy w pionie o kąt
                                                                                                                 // disable shadows so they don't obstruct self-lit items
/*
                        setup_shadow_color( colors::white );
*/
                        auto const unitstate = m_unitstate;
                        switch_units( m_unitstate.diffuse, false, false );

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
/*
                        setup_shadow_color( m_shadowcolor );
*/
                        switch_units( m_unitstate.diffuse, unitstate.shadows, unitstate.reflections );

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

    if( Submodel->b_aAnim < at_SecondsJump )
        Submodel->b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu

    if( Submodel->Next != nullptr )
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
            mousepos.x * EU07_PICKBUFFERSIZE / std::max( 1, Global::iWindowWidth ),
            mousepos.y * EU07_PICKBUFFERSIZE / std::max( 1, Global::iWindowHeight ) };
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

editor::basic_node const *
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
    editor::basic_node const *node { nullptr };
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
    m_framerate = 1000.f / ( Timer::subsystem.gfx_total.average() );

    // adjust draw ranges etc, based on recent performance
    auto const framerate = 1000.f / Timer::subsystem.gfx_color.average();

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
        m_debugtimestext += m_textures.info();
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
    // dump last opengl error, if any
    auto const glerror = ::glGetError();
    if( glerror != GL_NO_ERROR ) {
        std::string glerrorstring( ( char * )::gluErrorString( glerror ) );
        win1250_to_ascii( glerrorstring );
        Global::LastGLError = std::to_string( glerror ) + " (" + glerrorstring + ")";
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

    WriteLog( "Supported extensions: " +  std::string((char *)glGetString( GL_EXTENSIONS )) );

    WriteLog( std::string("Render path: NOGFX" ) );
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
    // select renderer mode
    if( true == Global::BasicRenderer ) {
        WriteLog( "Basic renderer selected, shadow and reflection mapping will be disabled" );
        Global::RenderShadows = false;
        m_diffusetextureunit = GL_TEXTURE0;
        m_helpertextureunit = -1;
        m_shadowtextureunit = -1;
        m_normaltextureunit = -1;
    }
    else {
        GLint maxtextureunits;
        ::glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxtextureunits );
        if( maxtextureunits < 4 ) {
            WriteLog( "Less than 4 texture units, shadow and reflection mapping will be disabled" );
            Global::BasicRenderer = true;
            Global::RenderShadows = false;
            m_diffusetextureunit = GL_TEXTURE0;
            m_helpertextureunit = -1;
            m_shadowtextureunit = -1;
            m_normaltextureunit = -1;
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
