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
#include "World.h"
#include "dynobj.h"
#include "uilayer.h"
#include "logs.h"

opengl_renderer GfxRenderer;
extern TWorld World;

// returns true if specified object is within camera frustum, false otherwise
bool
opengl_camera::visible( bounding_area const &Area ) const {

    return ( m_frustum.sphere_inside( Area.center, Area.radius ) > 0.0f );
}

bool
opengl_camera::visible( TDynamicObject const *Dynamic ) const {

    // sphere test is faster than AABB, so we'll use it here
    float3 diagonal(
        Dynamic->MoverParameters->Dim.L,
        Dynamic->MoverParameters->Dim.H,
        Dynamic->MoverParameters->Dim.W );
    float const radius = diagonal.Length() * 0.5f;

    return ( m_frustum.sphere_inside( Dynamic->GetPosition(), radius ) > 0.0f );
}

bool
opengl_renderer::Init( GLFWwindow *Window ) {

    if( false == Init_caps() ) {

        return false;
    }

    m_window = Window;

    glClearDepth( 1.0f );
    glClearColor( 51.0f / 255.0f, 102.0f / 255.0f, 85.0f / 255.0f, 1.0f ); // initial background Color

    glPolygonMode( GL_FRONT, GL_FILL );
    glFrontFace( GL_CCW ); // Counter clock-wise polygons face out
    glEnable( GL_CULL_FACE ); // Cull back-facing triangles
    glShadeModel( GL_SMOOTH ); // Enable Smooth Shading

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

    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );

    // setup lighting
    GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    // directional light
    // TODO, TBD: test omni-directional variant
    // rgb value for 5780 kelvin
    Global::daylight.color.x = 255.0f / 255.0f;
    Global::daylight.color.y = 242.0f / 255.0f;
    Global::daylight.color.z = 231.0f / 255.0f;

    // setup fog
    if( Global::fFogEnd > 0 ) {
        // fog setup
        ::glFogi( GL_FOG_MODE, GL_LINEAR );
        ::glFogfv( GL_FOG_COLOR, Global::FogColor );
        ::glFogf( GL_FOG_START, Global::fFogStart );
        ::glFogf( GL_FOG_END, Global::fFogEnd );
        ::glEnable( GL_FOG );
    }
    else { ::glDisable( GL_FOG ); }

	World.shader = gl_program_light({ gl_shader("lighting.vert"), gl_shader("blinnphong.frag") });
	Global::daylight.intensity = 1.0f; //m7todo: przenieść

    return true;
}

bool
opengl_renderer::Render() {

    auto timestart = std::chrono::steady_clock::now();
	
	glUseProgram(World.shader);

    ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    ::glDepthFunc( GL_LEQUAL );

    ::glMatrixMode( GL_PROJECTION ); // select the Projection Matrix
    glm::mat4 projection = glm::perspective(
        Global::FieldOfView / Global::ZoomFactor * 0.0174532925f,
        std::max( 1.0f, (float)Global::ScreenWidth ) / std::max( 1.0f, (float)Global::ScreenHeight ),
        0.1f * Global::ZoomFactor,
        m_drawrange * Global::fDistanceFactor );
    ::glLoadMatrixf( &projection[0][0] );

    ::glMatrixMode( GL_MODELVIEW ); // Select The Modelview Matrix
    glm::mat4 modelview( 1.0f );
    ::glLoadIdentity();

    if( World.InitPerformed() ) {

        World.Camera.SetMatrix( modelview );
        ::glLoadMatrixf( &modelview[ 0 ][ 0 ] );
        m_camera.update_frustum( projection, modelview );

        if( !Global::bWireFrame ) {
            // bez nieba w trybie rysowania linii
            World.Environment.render();
        }

		glDebug("rendering ground");
        World.Ground.Render( World.Camera.Pos );

		glDebug("rendering cab");
        World.Render_Cab();

        // accumulate last 20 frames worth of render time (cap at 1000 fps to prevent calculations going awry)
        m_drawtime = std::max( 20.0f, 0.95f * m_drawtime + std::chrono::duration_cast<std::chrono::milliseconds>( ( std::chrono::steady_clock::now() - timestart ) ).count());
    }

	glUseProgram(0);
    UILayer.render();

    glfwSwapBuffers( m_window );
    return true; // for now always succeed
}

#ifndef EU07_USE_OLD_RENDERCODE
bool
opengl_renderer::Render( TGround *Ground ) {

    glDisable( GL_BLEND );
    glAlphaFunc( GL_GREATER, 0.35f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    glEnable( GL_LIGHTING );
    glColor3f( 1.0f, 1.0f, 1.0f );

    float3 const cameraposition = float3( Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z );
    int const camerax = std::floor( cameraposition.x / 1000.0f ) + iNumRects / 2;
    int const cameraz = std::floor( cameraposition.z / 1000.0f ) + iNumRects / 2;
    int const segmentcount = 2 * static_cast<int>(std::ceil( m_drawrange * Global::fDistanceFactor / 1000.0f ));
    int const originx = std::max( 0, camerax - segmentcount / 2 );
    int const originz = std::max( 0, cameraz - segmentcount / 2 );

    for( int column = originx; column <= originx + segmentcount; ++column ) {
        for( int row = originz; row <= originz + segmentcount; ++row ) {

            auto &rectangle = Ground->Rects[ column ][ row ];
            if( m_camera.visible( rectangle.m_area ) ) {
                rectangle.RenderDL();
            }
        }
    }

    return true;
}

bool
opengl_renderer::Render( TDynamicObject *Dynamic ) {

    if( false == m_camera.visible( Dynamic ) ) {

        Dynamic->renderme = false;
        return false;
    }

    Dynamic->renderme = true;

    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    double squaredistance = SquareMagnitude( Global::pCameraPosition - Dynamic->vPosition ) / Global::ZoomFactor;
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu

    ::glPushMatrix();

    if( Dynamic == Global::pUserDynamic ) {
        //specjalne ustawienie, aby nie trzęsło
        //tu trzeba by ustawić animacje na modelu zewnętrznym
        ::glLoadIdentity(); // zacząć od macierzy jedynkowej
        Global::pCamera->SetCabMatrix( Dynamic->vPosition ); // specjalne ustawienie kamery
    }
    else
        ::glTranslated( Dynamic->vPosition.x, Dynamic->vPosition.y, Dynamic->vPosition.z ); // standardowe przesunięcie względem początku scenerii

    ::glMultMatrixd( Dynamic->mMatrix.getArray() );

    if( Dynamic->fShade > 0.0f ) {
        // change light level based on light level of the occupied track
		Global::daylight.intensity = Dynamic->fShade;
    }

    // TODO: implement universal render path down the road
    if( Global::bUseVBO ) {
        // wersja VBO
        if( Dynamic->mdLowPolyInt ) {
            if( FreeFlyModeFlag ? true : !Dynamic->mdKabina || !Dynamic->bDisplayCab ) {
                // enable cab light if needed
                if( Dynamic->InteriorLightLevel > 0.0f ) {

                    // crude way to light the cabin, until we have something more complete in place
                    auto const cablight = Dynamic->InteriorLight * Dynamic->InteriorLightLevel;
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &cablight.x );
                }

                Dynamic->mdLowPolyInt->RaRender( squaredistance, Dynamic->Material()->replacable_skins, Dynamic->Material()->textures_alpha );

                if( Dynamic->InteriorLightLevel > 0.0f ) {
                    // reset the overall ambient
                    GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
                }
            }
        }

        Dynamic->mdModel->RaRender( squaredistance, Dynamic->Material()->replacable_skins, Dynamic->Material()->textures_alpha );

        if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
            Dynamic->mdLoad->RaRender( squaredistance, Dynamic->Material()->replacable_skins, Dynamic->Material()->textures_alpha );
    }
    else {
        // wersja Display Lists
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
                    GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
                }
            }
        }

        Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );

        if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
            Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance );
    }

    if( Dynamic->fShade > 0.0f ) {
        // restore regular light level
		Global::daylight.intensity = 1.0f;
    }

    ::glPopMatrix();

    // TODO: check if this reset is needed. In theory each object should render all parts based on its own instance data anyway?
    if( Dynamic->btnOn )
        Dynamic->TurnOff(); // przywrócenie domyślnych pozycji submodeli

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
    
    Model->Root->ReplacableSet(
        ( Material != nullptr ?
            Material->replacable_skins :
            nullptr ),
        alpha );

    Model->Root->RenderDL();

    return true;
}

bool
opengl_renderer::Render( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotated( Angle.y, 0.0, 1.0, 0.0 );
    if( Angle.x != 0.0 )
        ::glRotated( Angle.x, 1.0, 0.0, 0.0 );
    if( Angle.z != 0.0 )
        ::glRotated( Angle.z, 0.0, 0.0, 1.0 );

    auto const result = Render( Model, Material, SquareMagnitude( Position - Global::GetCameraPosition() ) );

    ::glPopMatrix();

    return result;
}

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) {

        return false;
    }
    
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    double squaredistance = SquareMagnitude( Global::pCameraPosition - Dynamic->vPosition );
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu

    ::glPushMatrix();
    if( Dynamic == Global::pUserDynamic ) { // specjalne ustawienie, aby nie trzęsło
        ::glLoadIdentity(); // zacząć od macierzy jedynkowej
        Global::pCamera->SetCabMatrix( Dynamic->vPosition ); // specjalne ustawienie kamery
    }
    else
        ::glTranslated( Dynamic->vPosition.x, Dynamic->vPosition.y, Dynamic->vPosition.z ); // standardowe przesunięcie względem początku scenerii

    ::glMultMatrixd( Dynamic->mMatrix.getArray() );
    // wersja Display Lists
    // NOTE: VBO path is removed
    // TODO: implement universal render path down the road
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
                GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
            }
        }
    }

    Render_Alpha( Dynamic->mdModel, Dynamic->Material(), squaredistance );

    if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
        Render_Alpha( Dynamic->mdLoad, Dynamic->Material(), squaredistance );

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

    Model->Root->ReplacableSet(
        ( Material != nullptr ?
            Material->replacable_skins :
            nullptr ),
        alpha );

    Model->Root->RenderAlphaDL();

    return true;
}

bool
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotated( Angle.y, 0.0, 1.0, 0.0 );
    if( Angle.x != 0.0 )
        ::glRotated( Angle.x, 1.0, 0.0, 0.0 );
    if( Angle.z != 0.0 )
        ::glRotated( Angle.z, 0.0, 0.0, 1.0 );

    auto const result = Render_Alpha( Model, Material, SquareMagnitude( Position - Global::GetCameraPosition() ) );

    ::glPopMatrix();

    return result;
}
#endif

void
opengl_renderer::Update ( double const Deltatime ) {

    m_updateaccumulator += Deltatime;

    if( m_updateaccumulator < 1.0 ) {
        // too early for any work
        return;
    }

    m_updateaccumulator = 0.0;

    // adjust draw ranges etc, based on recent performance
    auto const framerate = 1000.0f / (m_drawtime / 20.0f);

    // NOTE: until we have quadtree in place we have to rely on the legacy rendering
    // once this is resolved we should be able to simply adjust draw range
    int targetsegments;
    float targetfactor;

         if( framerate > 65.0 ) { targetsegments = 400; targetfactor = 3.0f; }
    else if( framerate > 40.0 ) { targetsegments = 225; targetfactor = 1.5f; }
    else if( framerate > 15.0 ) { targetsegments =  90; targetfactor = Global::ScreenHeight / 768.0f; }
    else                        { targetsegments =   9; targetfactor = Global::ScreenHeight / 768.0f * 0.75f; }

    if( targetsegments > Global::iSegmentsRendered ) {

        Global::iSegmentsRendered = std::min( targetsegments, Global::iSegmentsRendered + 5 );
    }
    else if( targetsegments < Global::iSegmentsRendered ) {

        Global::iSegmentsRendered = std::max( targetsegments, Global::iSegmentsRendered - 5 );
    }
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

    // TODO: add garbage collection and other less frequent works here
    if( DebugModeFlag )
        m_debuginfo = m_textures.Info();
};

// debug performance string
std::string const &
opengl_renderer::Info() const {

    return m_debuginfo;
}

void
opengl_renderer::Update_Lights( light_array const &Lights ) {

    size_t const count = std::min( (size_t)Global::DynamicLightCount, Lights.data.size() );
    if( count == 0 ) { return; }

	size_t renderlight = 0;

    for( auto const &scenelight : Lights.data ) {

        if( renderlight == Global::DynamicLightCount ) {
            // we ran out of lights to assign
            break;
        }
        if( scenelight.intensity == 0.0f ) {
            // all lights past this one are bound to be off
            break;
        }
        if( ( Global::pCameraPosition - scenelight.position ).Length() > 1000.0f ) {
            // we don't care about lights past arbitrary limit of 1 km.
            // but there could still be weaker lights which are closer, so keep looking
            continue;
        }

		// if the light passed tests so far, it's good enough

		auto const luminance = Global::fLuminance; // TODO: adjust this based on location, e.g. for tunnels
		float3 position(scenelight.position.x, scenelight.position.y, scenelight.position.z);
		float3 direction(scenelight.direction.x, scenelight.direction.y, scenelight.direction.z);
		float3 color(scenelight.color.x,
		             scenelight.color.y,
		             scenelight.color.z);

		World.shader.set_light((GLuint)renderlight + 1, gl_program_light::SPOT, position, direction, 0.906f, 0.866f, color, 0.007f, 0.0002f);

        ++renderlight;
    }

	World.shader.set_light_count((GLuint)renderlight + 1);
}

void
opengl_renderer::Disable_Lights() {

	World.shader.set_light_count(0);
}

bool
opengl_renderer::Init_caps() {

    std::string oglversion = ( (char *)glGetString( GL_VERSION ) );

    WriteLog(
        "Gfx Renderer: " + std::string( (char *)glGetString( GL_RENDERER ) )
        + " Vendor: " + std::string( (char *)glGetString( GL_VENDOR ) )
        + " OpenGL Version: " + oglversion );

    if( !GLEW_VERSION_1_4 ) {
        ErrorLog( "Requires openGL >= 1.4" );
        return false;
    }

    WriteLog( "Supported extensions:" +  std::string((char *)glGetString( GL_EXTENSIONS )) );

    if( Global::iMultisampling )
        WriteLog( "Using multisampling x" + std::to_string( 1 << Global::iMultisampling ) );
    { // ograniczenie maksymalnego rozmiaru tekstur - parametr dla skalowania tekstur
        GLint i;
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &i );
        if( i < Global::iMaxTextureSize )
            Global::iMaxTextureSize = i;
        WriteLog( "Texture sizes capped at " + std::to_string( Global::iMaxTextureSize ) + " pixels" );

    }

    return true;
}
//---------------------------------------------------------------------------
