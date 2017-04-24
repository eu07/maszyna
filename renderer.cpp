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
#include "usefull.h"
#include "World.h"

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

	WriteLog("Loading common gfx data...");
	m_glaretextureid = GetTextureId("fx\\lightglare", szTexturePath);
	m_suntextureid = GetTextureId("fx\\sun", szTexturePath);
	m_moontextureid = GetTextureId("fx\\moon", szTexturePath);
	WriteLog("...gfx data pre-loading done");
	
	return true;
}

bool
opengl_renderer::Render() {

    auto timestart = std::chrono::steady_clock::now();
	
	glUseProgram(World.shader);

    ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    ::glDepthFunc( GL_LEQUAL );

    ::glMatrixMode( GL_PROJECTION ); // select the Projection Matrix
    ::gluPerspective(
        Global::FieldOfView / Global::ZoomFactor,
        std::max( 1.0f, (float)Global::ScreenWidth ) / std::max( 1.0f, (float)Global::ScreenHeight ),
        0.1f * Global::ZoomFactor,
        m_drawrange * Global::fDistanceFactor );

    ::glMatrixMode( GL_MODELVIEW ); // Select The Modelview Matrix
    ::glLoadIdentity();

    if( World.InitPerformed() ) {

        World.Camera.SetMatrix();
        m_camera.update_frustum();

        Render( &World.Environment );
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

bool
opengl_renderer::Render( world_environment *Environment ) {

    if( Global::bWireFrame ) {
        // bez nieba w trybie rysowania linii
        return false;
    }

    Bind( 0 );

    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_DEPTH_TEST );
    ::glDepthMask( GL_FALSE );
    ::glPushMatrix();
    ::glTranslatef( Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z );

    // setup fog
    if( Global::fFogEnd > 0 ) {
        // fog setup
        ::glFogfv( GL_FOG_COLOR, Global::FogColor );
        ::glFogf( GL_FOG_DENSITY, 1.0f / Global::fFogEnd );
        ::glEnable( GL_FOG );
    }
    else { ::glDisable( GL_FOG ); }

    Environment->m_skydome.Render();
    Environment->m_stars.render();

    float const duskfactor = 1.0f - clamp( std::abs( Environment->m_sun.getAngle() ), 0.0f, 12.0f ) / 12.0f;
    float3 suncolor = interpolate(
        float3( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f ),
        float3( 235.0f / 255.0f, 140.0f / 255.0f, 36.0f / 255.0f ),
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
        Bind( m_suntextureid );
        ::glColor4f( suncolor.x, suncolor.y, suncolor.z, 1.0f );
        auto const sunvector = Environment->m_sun.getDirection();
        auto const sunposition = modelview * glm::vec4( sunvector.x, sunvector.y, sunvector.z, 1.0f );

        ::glPushMatrix();
        ::glLoadIdentity(); // macierz jedynkowa
        ::glTranslatef( sunposition.x, sunposition.y, sunposition.z ); // początek układu zostaje bez zmian

        float const size = 0.045f;
        ::glBegin( GL_TRIANGLE_STRIP );
        ::glTexCoord2f( 1.0f, 1.0f ); ::glVertex3f( -size, size, 0.0f );
        ::glTexCoord2f( 1.0f, 0.0f ); ::glVertex3f( -size, -size, 0.0f );
        ::glTexCoord2f( 0.0f, 1.0f ); ::glVertex3f( size, size, 0.0f );
        ::glTexCoord2f( 0.0f, 0.0f ); ::glVertex3f( size, -size, 0.0f );
        ::glEnd();

        ::glPopMatrix();
    }
    // moon
    {
        Bind( m_moontextureid );
        float3 mooncolor( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f );
        ::glColor4f( mooncolor.x, mooncolor.y, mooncolor.z, 1.0f - Global::fLuminance * 0.5f );

        auto const moonvector = Environment->m_moon.getDirection();
        auto const moonposition = modelview * glm::vec4( moonvector.x, moonvector.y, moonvector.z, 1.0f );
        ::glPushMatrix();
        ::glLoadIdentity(); // macierz jedynkowa
        ::glTranslatef( moonposition.x, moonposition.y, moonposition.z );

        float const size = 0.02f; // TODO: expose distance/scale factor from the moon object
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
        ::glTexCoord2f( moonu, moonv ); ::glVertex3f( -size, size, 0.0f );
        ::glTexCoord2f( moonu, moonv - 0.333f ); ::glVertex3f( -size, -size, 0.0f );
        ::glTexCoord2f( moonu + 0.333f, moonv ); ::glVertex3f( size, size, 0.0f );
        ::glTexCoord2f( moonu + 0.333f, moonv - 0.333f ); ::glVertex3f( size, -size, 0.0f );
        ::glEnd();

        ::glPopMatrix();
    }
    ::glPopAttrib();

    // clouds
    Environment->m_clouds.Render(
        interpolate( Environment->m_skydome.GetAverageColor(), suncolor, duskfactor * 0.25f )
        * ( 1.0f - Global::Overcast * 0.5f ) // overcast darkens the clouds
        * 2.5f ); // arbitrary adjustment factor

	Global::daylight.intensity = 1.0f;

    ::glPopMatrix();
    ::glDepthMask( GL_TRUE );
    ::glEnable( GL_DEPTH_TEST );
    ::glEnable( GL_LIGHTING );

    return true;
}

bool
opengl_renderer::Render( TGround *Ground ) {

    glDisable( GL_BLEND );
    glAlphaFunc( GL_GREATER, 0.50f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
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

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    double squaredistance = SquareMagnitude( ( Global::pCameraPosition - Dynamic->vPosition ) / Global::ZoomFactor );
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
                GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
            }
        }
    }

    Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );

    if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
        Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance );

    // post-render cleanup
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

    // TODO: unify the render code after generic buffers are in place
    // setup
    if( false == Model->StartVBO() )
        return false;

    Model->Root->ReplacableSet(
        ( Material != nullptr ?
            Material->replacable_skins :
            nullptr ),
        alpha );

    Model->Root->pRoot = Model;

    // render
	Render(Model->Root);

    // post-render cleanup
    Model->EndVBO();

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

void opengl_renderer::Render(TSubModel *Submodel)
{
	World.shader.set_mv(OpenGLMatrices.data(GL_MODELVIEW));
	World.shader.set_p(OpenGLMatrices.data(GL_PROJECTION));
	Render(Submodel, OpenGLMatrices.data(GL_MODELVIEW));
}

void opengl_renderer::Render_Alpha(TSubModel *Submodel)
{
	World.shader.set_mv(OpenGLMatrices.data(GL_MODELVIEW));
	World.shader.set_p(OpenGLMatrices.data(GL_PROJECTION));
	Render_Alpha(Submodel, OpenGLMatrices.data(GL_MODELVIEW));
}

void
opengl_renderer::Render( TSubModel *Submodel, glm::mat4 m) {

    if( ( Submodel->iVisible )
     && ( TSubModel::fSquareDist >= ( Submodel->fSquareMinDist / Global::fDistanceFactor ) )
     && ( TSubModel::fSquareDist <= ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) )
	{
		glm::mat4 mm = m;
		if (Submodel->iFlags & 0xC000)
		{
			if (Submodel->fMatrix)
				mm *= glm::make_mat4(Submodel->fMatrix->e);
			if (Submodel->b_Anim)
				Submodel->RaAnimation(mm, Submodel->b_Anim);
			World.shader.set_mv(mm);
		}

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x1F ) // rysuj gdy element nieprzezroczysty
            {
                // material configuration:
                // textures...
                if( Submodel->TextureID < 0 )
                { // zmienialne skóry
                    Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                }
                else {
                    // również 0
                    Bind( Submodel->TextureID );
                }
				/*
                ::glColor3fv( Submodel->f4Diffuse ); // McZapkie-240702: zamiast ub
                // ...luminance
                if( Global::fLuminance < Submodel->fLight ) {
                    // zeby swiecilo na kolorowo
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, Submodel->f4Diffuse );
                }
				*/

                // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                ::glDrawArrays( Submodel->eType, Submodel->iVboPtr, Submodel->iNumVerts );

				/*
                // post-draw reset
                if( Global::fLuminance < Submodel->fLight ) {
                    // restore default (lack of) brightness
                    glm::vec4 const noemission( 0.0f, 0.0f, 0.0f, 1.0f );
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( noemission ) );
                }
				*/
            }
        }
		//m7todo: restore
		/*
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {
            auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
            auto const lightcenter = modelview * glm::vec4( 0.0f, 0.0f, -0.05f, 1.0f ); // pozycja punktu świecącego względem kamery
            Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f ) - lightcenter ), glm::normalize( -lightcenter ) );

            if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) // kąt większy niż maksymalny stożek swiatła
            {
                float lightlevel = 1.0f;
                // view angle attenuation
                float const anglefactor = ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                float const distancefactor = std::max( 0.5, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) );

                if( lightlevel > 0.0f ) {
                    // material configuration:
                    ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT );

                    Bind( 0 );
                    ::glPointSize( std::max( 2.0f, 4.0f * distancefactor * anglefactor ) );
                    ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], lightlevel * anglefactor );
                    ::glDisable( GL_LIGHTING );
                    ::glEnable( GL_BLEND );

                    // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                    ::glDrawArrays( GL_POINTS, Submodel->iVboPtr, Submodel->iNumVerts );

                    // post-draw reset
                    ::glPopAttrib();
                }
            }
        }
        else if( Submodel->eType == TP_STARS ) {

            if( Global::fLuminance < Submodel->fLight ) {

                // material configuration:
                ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );

                Bind( 0 );
                ::glDisable( GL_LIGHTING );

                // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                // NOTE: we're doing manual switch to color vbo setup, because there doesn't seem to be any convenient way available atm
                // TODO: implement easier way to go about it
                ::glDisableClientState( GL_NORMAL_ARRAY );
                ::glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                ::glEnableClientState( GL_COLOR_ARRAY );
                ::glColorPointer( 3, GL_FLOAT, sizeof( CVertNormTex ), static_cast<char *>( nullptr ) + 12 ); // kolory

                ::glDrawArrays( GL_POINTS, Submodel->iVboPtr, Submodel->iNumVerts );

                ::glDisableClientState( GL_COLOR_ARRAY );
                ::glEnableClientState( GL_NORMAL_ARRAY );
                ::glEnableClientState( GL_TEXTURE_COORD_ARRAY );

                // post-draw reset
                ::glPopAttrib();
            }
			
        }
		*/

        if( Submodel->Child != NULL )
            if( Submodel->iAlpha & Submodel->iFlags & 0x001F0000 )
                Render( Submodel->Child, mm );

        if( Submodel->iFlags & 0xC000 )
			World.shader.set_mv(m);
    }
	
    if( Submodel->b_Anim < at_SecondsJump )
        Submodel->b_Anim = at_None; // wyłączenie animacji dla kolejnego użycia subm

    if( Submodel->Next )
        if( Submodel->iAlpha & Submodel->iFlags & 0x1F000000 )
            Render( Submodel->Next, m ); // dalsze rekurencyjnie
}

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) {

        return false;
    }

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    double squaredistance = SquareMagnitude( ( Global::pCameraPosition - Dynamic->vPosition ) / Global::ZoomFactor );
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu

    ::glPushMatrix();
    if( Dynamic == Global::pUserDynamic ) { // specjalne ustawienie, aby nie trzęsło
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
                GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
            }
        }
    }

    Render_Alpha( Dynamic->mdModel, Dynamic->Material(), squaredistance );

    if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
        Render_Alpha( Dynamic->mdLoad, Dynamic->Material(), squaredistance );

    // post-render cleanup
    if( Dynamic->fShade > 0.0f ) {
        // restore regular light level
		Global::daylight.intensity = 1.0f;
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

    // TODO: unify the render code after generic buffers are in place
    // setup
    if( false == Model->StartVBO() )
        return false;

    Model->Root->ReplacableSet(
        ( Material != nullptr ?
        Material->replacable_skins :
        nullptr ),
        alpha );

    Model->Root->pRoot = Model;

    // render
    Render_Alpha( Model->Root );

    // post-render cleanup
    Model->EndVBO();

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

void
opengl_renderer::Render_Alpha( TSubModel *Submodel, glm::mat4 m) {
    // renderowanie przezroczystych przez DL
    if( ( Submodel->iVisible )
     && ( TSubModel::fSquareDist >= ( Submodel->fSquareMinDist / Global::fDistanceFactor ) )
     && ( TSubModel::fSquareDist <= ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) ) {

		glm::mat4 mm = m;
		if (Submodel->iFlags & 0xC000)
		{
			if (Submodel->fMatrix)
				mm *= glm::make_mat4(Submodel->fMatrix->e);
			if (Submodel->b_Anim)
				Submodel->RaAnimation(mm, Submodel->b_Anim);
			World.shader.set_mv(mm);
		}

        if( Submodel->eType < TP_ROTATOR ) {
            // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x2F ) // rysuj gdy element przezroczysty
            {
                // textures...
                if( Submodel->TextureID < 0 ) { // zmienialne skóry
                    Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                }
                else {
                    // również 0
                    Bind( Submodel->TextureID );
                }
				/*
                ::glColor3fv( Submodel->f4Diffuse ); // McZapkie-240702: zamiast ub
                // ...luminance
                if( Global::fLuminance < Submodel->fLight ) {
                    // zeby swiecilo na kolorowo
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, Submodel->f4Diffuse );
                }
				*/

			  // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                ::glDrawArrays( Submodel->eType, Submodel->iVboPtr, Submodel->iNumVerts );

				/*
                // post-draw reset
                if( Global::fLuminance < Submodel->fLight ) {
                    // restore default (lack of) brightness
                    glm::vec4 const noemission( 0.0f, 0.0f, 0.0f, 1.0f );
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( noemission ) );
                }
				*/
            }
        }
#if 0
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            if( Global::fLuminance < Submodel->fLight ) {
                // NOTE: we're forced here to redo view angle calculations etc, because this data isn't instanced but stored along with the single mesh
                // TODO: separate instance data from reusable geometry
                auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
                auto const lightcenter = modelview * glm::vec4( 0.0f, 0.0f, -0.05f, 1.0f ); // pozycja punktu świecącego względem kamery
                Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f ) - lightcenter ), glm::normalize( -lightcenter ) );

                float glarelevel = 0.6f; // luminosity at night is at level of ~0.1, so the overall resulting transparency is ~0.5 at full 'brightness'
                if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) {

                    glarelevel *= ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                    glarelevel = std::max( 0.0f, glarelevel - static_cast<float>(Global::fLuminance) );

                    if( glarelevel > 0.0f ) {

                        ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );

                        Bind( m_glaretextureid );
                        ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], glarelevel );
                        ::glDisable( GL_LIGHTING );
                        ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                        ::glPushMatrix();
                        ::glLoadIdentity(); // macierz jedynkowa
                        ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
                        ::glRotated( atan2( lightcenter.x, lightcenter.z ) * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // jedynie obracamy w pionie o kąt

                        // TODO: turn the drawing instructions into a compiled call / array
                        ::glBegin( GL_TRIANGLE_STRIP );
                        float const size = 2.5f;
                        ::glTexCoord2f( 1.0f, 1.0f ); ::glVertex3f( -size, size, 0.0f );
                        ::glTexCoord2f( 0.0f, 1.0f ); ::glVertex3f( size, size, 0.0f );
                        ::glTexCoord2f( 1.0f, 0.0f ); ::glVertex3f( -size, -size, 0.0f );
                        ::glTexCoord2f( 0.0f, 0.0f ); ::glVertex3f( size, -size, 0.0f );
/*
                        // NOTE: we could do simply...
                        vec3 vertexPosition_worldspace =
                        particleCenter_wordspace
                        + CameraRight_worldspace * squareVertices.x * BillboardSize.x
                        + CameraUp_worldspace * squareVertices.y * BillboardSize.y;
                        // ...etc instead IF we had easy access to camera's forward and right vectors. TODO: check if Camera matrix is accessible
*/
                        ::glEnd();

                        ::glPopMatrix();
                        ::glPopAttrib();
                    }
                }
            }
        }
#endif

        if( Submodel->Child != NULL )
			if( Submodel->iAlpha & Submodel->iFlags & 0x002F0000 )
                Render_Alpha( Submodel->Child, mm );

        if( Submodel->iFlags & 0xC000 )
			World.shader.set_mv(m);
    }

    if( Submodel->b_aAnim < at_SecondsJump )
        Submodel->b_aAnim = at_None; // wyłączenie animacji dla kolejnego użycia submodelu

    if( Submodel->Next != NULL )
        if( Submodel->iAlpha & Submodel->iFlags & 0x2F000000 )
            Render_Alpha( Submodel->Next, m );
};

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
		glm::vec3 position(scenelight.position.x, scenelight.position.y, scenelight.position.z);
		glm::vec3 direction(scenelight.direction.x, scenelight.direction.y, scenelight.direction.z);
		glm::vec3 color(scenelight.color.x,
		                scenelight.color.y,
		                scenelight.color.z);

		World.shader.set_light((GLuint)renderlight + 1, gl_program_light::SPOT, position, direction, 0.906f, 0.866f, color, 0.007f, 0.0002f);

        ++renderlight;
    }

	World.shader.set_ambient(Global::daylight.ambient);
	World.shader.set_light(0, gl_program_light::DIR, glm::vec3(0.0f), Global::daylight.direction,
		0.0f, 0.0f, Global::daylight.color, 0.0f, 0.0f);
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

    if( !GLEW_VERSION_3_2 ) {
        ErrorLog( "Requires openGL >= 3.2" );
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
