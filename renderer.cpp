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

opengl_renderer GfxRenderer;
extern TWorld World;

void
opengl_renderer::Init() {

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
}

bool
opengl_renderer::Render() {

    ::glColor3ub( 255, 255, 255 );
    ::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    ::glDepthFunc( GL_LEQUAL );

    ::glMatrixMode( GL_PROJECTION ); // select the Projection Matrix
    ::glLoadIdentity(); // reset the Projection Matrix
    // calculate the aspect ratio of the window
    ::gluPerspective(
        Global::FieldOfView / Global::ZoomFactor,
        (GLdouble)Global::ScreenWidth / std::max( (GLdouble)Global::ScreenHeight, 1.0 ),
        0.1f * Global::ZoomFactor,
        m_drawrange * Global::fDistanceFactor );

    ::glMatrixMode( GL_MODELVIEW ); // Select The Modelview Matrix
    ::glLoadIdentity();
    World.Camera.SetMatrix(); // ustawienie macierzy kamery względem początku scenerii
    m_camera.update_frustum();

    if( !Global::bWireFrame ) {
        // bez nieba w trybie rysowania linii
        World.Environment.render();
    }

    World.Ground.Render_Hidden( World.Camera.Pos );
    Render( &World.Ground );
    World.Ground.RenderDL( World.Camera.Pos );
    World.Ground.RenderAlphaDL( World.Camera.Pos );

    World.Render_Cab();
    World.Render_UI();

    return true; // for now always succeed
}

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

            Render( Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance );

            if( Dynamic->InteriorLightLevel > 0.0f ) {
                // reset the overall ambient
                GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, ambient );
            }
        }
    }

//    Dynamic->mdModel->Render( squaredistance, Dynamic->ReplacableSkinID, Dynamic->iAlpha );
    Render( Dynamic->mdModel, Dynamic->Material(), squaredistance );

    if( Dynamic->mdLoad ) // renderowanie nieprzezroczystego ładunku
        Render( Dynamic->mdLoad, Dynamic->Material(), squaredistance );

    ::glPopMatrix();

    // TODO: check if this reset is needed. In theory each object should render all buttons based on its own instance data anyway?
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

void
opengl_renderer::Update ( double const Deltatile ) {

    // TODO: add garbage collection and other works here
};

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

        auto const luminance = Global::fLuminance; // TODO: adjust this based on location, e.g. for tunnels
        renderlight->diffuse[ 0 ] = std::max( 0.0, scenelight.color.x - luminance );
        renderlight->diffuse[ 1 ] = std::max( 0.0, scenelight.color.y - luminance );
        renderlight->diffuse[ 2 ] = std::max( 0.0, scenelight.color.z - luminance );
        renderlight->ambient[ 0 ] = std::max( 0.0, scenelight.color.x * scenelight.intensity - luminance);
        renderlight->ambient[ 1 ] = std::max( 0.0, scenelight.color.y * scenelight.intensity - luminance );
        renderlight->ambient[ 2 ] = std::max( 0.0, scenelight.color.z * scenelight.intensity - luminance );

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
//---------------------------------------------------------------------------
