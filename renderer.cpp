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
#include "world.h"
#include "dynobj.h"
#include "uilayer.h"
#include "logs.h"
#include "usefull.h"

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

    Global::DayLight.id = opengl_renderer::sunlight;
    // directional light
    // TODO, TBD: test omni-directional variant
    Global::DayLight.position[ 3 ] = 1.0f;
    ::glLightf( opengl_renderer::sunlight, GL_SPOT_CUTOFF, 90.0f );
    // rgb value for 5780 kelvin
    Global::DayLight.diffuse[ 0 ] = 255.0f / 255.0f;
    Global::DayLight.diffuse[ 1 ] = 242.0f / 255.0f;
    Global::DayLight.diffuse[ 2 ] = 231.0f / 255.0f;

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
    m_glaretextureid = GetTextureId( "fx\\lightglare", szTexturePath );

    return true;
}

bool
opengl_renderer::Render() {

    auto timestart = std::chrono::steady_clock::now();

    ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    ::glDepthFunc( GL_LEQUAL );

    ::glMatrixMode( GL_PROJECTION ); // select the Projection Matrix
    ::gluPerspective(
        Global::FieldOfView / Global::ZoomFactor,
        std::max( 1.0f, (float)Global::ScreenWidth ) / std::max( 1.0f, (float)Global::ScreenHeight ),
        0.1f * Global::ZoomFactor,
        m_drawrange * Global::fDistanceFactor );
/*
    glm::mat4 projection = glm::perspective(
        Global::FieldOfView / Global::ZoomFactor * 0.0174532925f,
        std::max( 1.0f, (float)Global::ScreenWidth ) / std::max( 1.0f, (float)Global::ScreenHeight ),
        0.1f * Global::ZoomFactor,
        m_drawrange * Global::fDistanceFactor );
    ::glLoadMatrixf( &projection[0][0] );
*/
    ::glMatrixMode( GL_MODELVIEW ); // Select The Modelview Matrix
    ::glLoadIdentity();

    if( World.InitPerformed() ) {
/*
        glm::mat4 modelview( 1.0f );
        World.Camera.SetMatrix( modelview );
        ::glLoadMatrixf( &modelview[ 0 ][ 0 ] );
        m_camera.update_frustum( projection, modelview );
*/
        World.Camera.SetMatrix();
        m_camera.update_frustum();

        if( !Global::bWireFrame ) {
            // bez nieba w trybie rysowania linii
            World.Environment.render();
        }

        World.Ground.Render( World.Camera.Pos );

        World.Render_Cab();

        // accumulate last 20 frames worth of render time (cap at 1000 fps to prevent calculations going awry)
        m_drawtime = std::max( 20.0f, 0.95f * m_drawtime + std::chrono::duration_cast<std::chrono::milliseconds>( ( std::chrono::steady_clock::now() - timestart ) ).count());
    }

    UILayer.render();

    glfwSwapBuffers( m_window );
    return true; // for now always succeed
}

#ifndef EU07_USE_OLD_RENDERCODE
bool
opengl_renderer::Render( TGround *Ground ) {

    glDisable( GL_BLEND );
    glAlphaFunc( GL_GREATER, 0.45f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
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
        Global::DayLight.apply_intensity( Dynamic->fShade );
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
        Global::DayLight.apply_intensity();
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

    Render( Model->Root );

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

void
opengl_renderer::Render( TSubModel *Submodel ) {
    // główna procedura renderowania przez DL
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
        if( Submodel->eType < TP_ROTATOR ) { // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x1F ) // rysuj gdy element nieprzezroczysty
            {
                if( Submodel->TextureID < 0 ) // && (ReplacableSkinId!=0))
                { // zmienialne skóry
                    GfxRenderer.Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                    // TexAlpha=!(iAlpha&1); //zmiana tylko w przypadku wymienej tekstury
                }
                else
                    GfxRenderer.Bind( Submodel->TextureID ); // również 0
                if( Global::fLuminance < Submodel->fLight ) {
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, Submodel->f4Diffuse ); // zeby swiecilo na kolorowo
                    ::glCallList( Submodel->uiDisplayList ); // tylko dla siatki
                    float4 const noemission( 0.0f, 0.0f, 0.0f, 1.0f );
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, &noemission.x );
                }
                else
                    ::glCallList( Submodel->uiDisplayList ); // tylko dla siatki
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {
            // wersja DL
/*
            matrix4x4 modelview;
            ::glGetDoublev( GL_MODELVIEW_MATRIX, modelview.getArray() );
*/
            matrix4x4 modelview; modelview.OpenGL_Matrix( OpenGLMatrices.data_array( GL_MODELVIEW ) );
            // kąt między kierunkiem światła a współrzędnymi kamery
            auto const lightcenter = modelview * vector3( 0.0, 0.0, 0.0 ); // pozycja punktu świecącego względem kamery
            Submodel->fCosViewAngle = DotProduct( Normalize( modelview * vector3( 0.0, 0.0, -1.0 ) - lightcenter ), Normalize( -lightcenter ) );
            if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) // kąt większy niż maksymalny stożek swiatła
            {
                double Distdimm = 1.0;
                if( Submodel->fCosViewAngle < Submodel->fCosHotspotAngle ) // zmniejszona jasność między Hotspot a Falloff
                    if( Submodel->fCosFalloffAngle < Submodel->fCosHotspotAngle )
                        Distdimm = 1.0 - ( Submodel->fCosHotspotAngle - Submodel->fCosViewAngle ) / ( Submodel->fCosHotspotAngle - Submodel->fCosFalloffAngle );
                ::glColor3f( Submodel->f4Diffuse[ 0 ] * Distdimm, Submodel->f4Diffuse[ 1 ] * Distdimm, Submodel->f4Diffuse[ 2 ] * Distdimm );
                ::glCallList( Submodel->uiDisplayList ); // wyświetlenie warunkowe
            }
        }
        else if( Submodel->eType == TP_STARS ) {
            // glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
            if( Global::fLuminance < Submodel->fLight ) {
                ::glMaterialfv( GL_FRONT, GL_EMISSION, Submodel->f4Diffuse ); // zeby swiecilo na kolorowo
                ::glCallList( Submodel->uiDisplayList ); // narysuj naraz wszystkie punkty z DL
                float4 const noemission( 0.0f, 0.0f, 0.0f, 1.0f );
                ::glMaterialfv( GL_FRONT, GL_EMISSION, &noemission.x );
            }
        }
        if( Submodel->Child != NULL )
            if( Submodel->iAlpha & Submodel->iFlags & 0x001F0000 )
                Render( Submodel->Child );

        if( Submodel->iFlags & 0xC000 )
            ::glPopMatrix();
    }

    if( Submodel->b_Anim < at_SecondsJump )
        Submodel->b_Anim = at_None; // wyłączenie animacji dla kolejnego użycia subm

    if( Submodel->Next )
        if( Submodel->iAlpha & Submodel->iFlags & 0x1F000000 )
            Render( Submodel->Next ); // dalsze rekurencyjnie
};

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) {

        return false;
    }
    
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    double squaredistance = SquareMagnitude( Global::pCameraPosition - Dynamic->vPosition ) / Global::ZoomFactor;
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu

    ::glPushMatrix();
    if( Dynamic == Global::pUserDynamic ) { // specjalne ustawienie, aby nie trzęsło
        ::glLoadIdentity(); // zacząć od macierzy jedynkowej
        Global::pCamera->SetCabMatrix( Dynamic->vPosition ); // specjalne ustawienie kamery
    }
    else
        ::glTranslated( Dynamic->vPosition.x, Dynamic->vPosition.y, Dynamic->vPosition.z ); // standardowe przesunięcie względem początku scenerii

    ::glMultMatrixd( Dynamic->mMatrix.getArray() );

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

                Dynamic->mdLowPolyInt->RaRenderAlpha( squaredistance, Dynamic->Material()->replacable_skins, Dynamic->Material()->textures_alpha );

                if( Dynamic->InteriorLightLevel > 0.0f ) {
                    // reset the overall ambient
                    GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
                }
            }
        }

        Dynamic->mdModel->RaRenderAlpha( squaredistance, Dynamic->Material()->replacable_skins, Dynamic->Material()->textures_alpha );

        if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
            Dynamic->mdLoad->RaRenderAlpha( squaredistance, Dynamic->Material()->replacable_skins, Dynamic->Material()->textures_alpha );
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

    Model->Root->ReplacableSet(
        ( Material != nullptr ?
            Material->replacable_skins :
            nullptr ),
        alpha );

    Render_Alpha( Model->Root );

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

        if( Submodel->eType < TP_ROTATOR ) { // renderowanie obiektów OpenGL
            if( Submodel->iAlpha & Submodel->iFlags & 0x2F ) // rysuj gdy element przezroczysty
            {
                if( Submodel->TextureID < 0 ) // && (ReplacableSkinId!=0))
                { // zmienialne skóry
                    GfxRenderer.Bind( Submodel->ReplacableSkinId[ -Submodel->TextureID ] );
                    // TexAlpha=iAlpha&1; //zmiana tylko w przypadku wymienej tekstury
                }
                else
                    GfxRenderer.Bind( Submodel->TextureID ); // również 0
                if( Global::fLuminance < Submodel->fLight ) {
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, Submodel->f4Diffuse ); // zeby swiecilo na kolorowo
                    ::glCallList( Submodel->uiDisplayList ); // tylko dla siatki
                    float4 const noemission( 0.0f, 0.0f, 0.0f, 1.0f );
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, &noemission.x );
                }
                else
                    ::glCallList( Submodel->uiDisplayList ); // tylko dla siatki
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {
            // dorobić aureolę!
            if( Global::fLuminance < Submodel->fLight ) {
                // NOTE: we're forced here to redo view angle calculations etc, because this data isn't instanced but stored along with the single mesh
                // TODO: separate instance data from reusable geometry
/*
                matrix4x4 modelview;
                ::glGetDoublev( GL_MODELVIEW_MATRIX, modelview.getArray() );
*/
/*
                matrix4x4 modelview; modelview.OpenGL_Matrix( OpenGLMatrices.data_array( GL_MODELVIEW ) );
                // kąt między kierunkiem światła a współrzędnymi kamery
                auto const lightcenter = modelview * vector3( 0.0, 0.0, -0.05 ); // pozycja punktu świecącego względem kamery
                Submodel->fCosViewAngle = DotProduct( Normalize( modelview * vector3( 0.0, 0.0, -1.0 ) - lightcenter ), Normalize( -lightcenter ) );
*/
                auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
                auto const lightcenter = modelview * glm::vec4( 0.0f, 0.0f, -0.05f, 1.0f ); // pozycja punktu świecącego względem kamery
                Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f ) - lightcenter ), glm::normalize( -lightcenter ) );

                float fadelevel = 0.5f;
                if( ( Submodel->fCosViewAngle > 0.0 ) && ( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) ) {

                    fadelevel *= ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0 - Submodel->fCosFalloffAngle );

                    ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
                    Bind( m_glaretextureid );
                    ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], fadelevel );
                    ::glDisable( GL_LIGHTING );
                    ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                    ::glPushMatrix();
                    ::glLoadIdentity(); // macierz jedynkowa
                    ::glTranslatef( lightcenter.x, lightcenter.y, lightcenter.z ); // początek układu zostaje bez zmian
                    ::glRotated( atan2( lightcenter.x, lightcenter.z ) * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // jedynie obracamy w pionie o kąt

                    ::glMaterialfv( GL_FRONT, GL_EMISSION, Submodel->f4Diffuse ); // zeby swiecilo na kolorowo

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

                    float4 const noemission( 0.0f, 0.0f, 0.0f, 1.0f );
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, &noemission.x );

                    ::glPopMatrix();
                    ::glPopAttrib();
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

    size_t const count = std::min( m_lights.size(), Lights.data.size() );
    if( count == 0 ) { return; }

    auto renderlight = m_lights.begin();

    for( auto const &scenelight : Lights.data ) {

        if( renderlight == m_lights.end() ) {
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
        renderlight->set_position( scenelight.position );
        renderlight->direction = scenelight.direction;

        auto luminance = Global::fLuminance; // TODO: adjust this based on location, e.g. for tunnels
        auto const environment = scenelight.owner->fShade;
        if( environment > 0.0f ) {
            luminance *= environment;
        }
        renderlight->diffuse[ 0 ] = std::max( 0.0, scenelight.color.x - luminance );
        renderlight->diffuse[ 1 ] = std::max( 0.0, scenelight.color.y - luminance );
        renderlight->diffuse[ 2 ] = std::max( 0.0, scenelight.color.z - luminance );
        renderlight->ambient[ 0 ] = std::max( 0.0, scenelight.color.x * scenelight.intensity - luminance);
        renderlight->ambient[ 1 ] = std::max( 0.0, scenelight.color.y * scenelight.intensity - luminance );
        renderlight->ambient[ 2 ] = std::max( 0.0, scenelight.color.z * scenelight.intensity - luminance );
/*
        // NOTE: we have no simple way to determine whether the lights are falling on objects located in darker environment
        // until this issue is resolved we're disabling reduction of light strenght based on the global luminance
        renderlight->diffuse[ 0 ] = std::max( 0.0f, scenelight.color.x );
        renderlight->diffuse[ 1 ] = std::max( 0.0f, scenelight.color.y );
        renderlight->diffuse[ 2 ] = std::max( 0.0f, scenelight.color.z );
        renderlight->ambient[ 0 ] = std::max( 0.0f, scenelight.color.x * scenelight.intensity );
        renderlight->ambient[ 1 ] = std::max( 0.0f, scenelight.color.y * scenelight.intensity );
        renderlight->ambient[ 2 ] = std::max( 0.0f, scenelight.color.z * scenelight.intensity );
*/
        ::glLightf( renderlight->id, GL_LINEAR_ATTENUATION, (0.25f * scenelight.count) / std::pow( scenelight.count, 2 ) * (scenelight.owner->DimHeadlights ? 1.25f : 1.0f) );
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
