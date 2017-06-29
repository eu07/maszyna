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
#include "data.h"
#include "dynobj.h"
#include "animmodel.h"
#include "traction.h"
#include "uilayer.h"
#include "logs.h"
#include "usefull.h"

opengl_renderer GfxRenderer;
extern TWorld World;

namespace colors {

glm::vec4 const none { 0.0f, 0.0f, 0.0f, 1.0f };

} // namespace colors

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

bool
opengl_renderer::Init( GLFWwindow *Window ) {

    if( false == Init_caps() ) { return false; }

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
    m_glaretexture = GetTextureId( "fx\\lightglare", szTexturePath );
    m_suntexture = GetTextureId( "fx\\sun", szTexturePath );
    m_moontexture = GetTextureId( "fx\\moon", szTexturePath );
    WriteLog( "...gfx data pre-loading done" );
    // prepare basic geometry chunks
    auto const geometrybank = m_geometry.create_bank();
    float const size = 2.5f;
    m_billboardgeometry = m_geometry.create_chunk(
        vertex_array{
            { { -size,  size, 0.0f }, glm::vec3(), { 1.0f, 1.0f } },
            { {  size,  size, 0.0f }, glm::vec3(), { 0.0f, 1.0f } },
            { { -size, -size, 0.0f }, glm::vec3(), { 1.0f, 0.0f } },
            { {  size, -size, 0.0f }, glm::vec3(), { 0.0f, 0.0f } } },
            geometrybank,
            GL_TRIANGLE_STRIP );
    // prepare debug mode objects
    m_quadric = gluNewQuadric();
    gluQuadricNormals( m_quadric, GLU_FLAT );

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

    ::glMatrixMode( GL_MODELVIEW ); // Select The Modelview Matrix
    ::glLoadIdentity();

    if( World.InitPerformed() ) {

        glm::dmat4 worldcamera;
        World.Camera.SetMatrix( worldcamera );
        m_camera.update_frustum( OpenGLMatrices.data( GL_PROJECTION ), worldcamera );
        // frustum tests are performed in 'world space' but after we set up frustum
        // we no longer need camera translation, only rotation
        ::glMultMatrixd( glm::value_ptr( glm::dmat4( glm::dmat3( worldcamera ))));

        Render( &World.Environment );
        Render( &World.Ground );

        World.Render_Cab();

        // accumulate last 20 frames worth of render time (cap at 1000 fps to prevent calculations going awry)
        m_drawtime = std::max( 20.0f, 0.95f * m_drawtime + std::chrono::duration_cast<std::chrono::milliseconds>( ( std::chrono::steady_clock::now() - timestart ) ).count());
        m_drawcount = m_drawqueue.size();
    }

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
    // setup fog
    if( Global::fFogEnd > 0 ) {
        // fog setup
        ::glFogfv( GL_FOG_COLOR, Global::FogColor );
        ::glFogf( GL_FOG_DENSITY, static_cast<GLfloat>( 1.0 / Global::fFogEnd ) );
        ::glEnable( GL_FOG );
    }
    else { ::glDisable( GL_FOG ); }

    Environment->m_skydome.Render();
    if( true == Global::bUseVBO ) {
        // skydome uses a custom vbo which could potentially confuse the main geometry system. hardly elegant but, eh
        opengl_vbogeometrybank::reset();
    }
    Environment->m_stars.render();

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
        ::glTexCoord2f( 1.0f, 1.0f ); ::glVertex3f( -size, size, 0.0f );
        ::glTexCoord2f( 1.0f, 0.0f ); ::glVertex3f( -size, -size, 0.0f );
        ::glTexCoord2f( 0.0f, 1.0f ); ::glVertex3f( size, size, 0.0f );
        ::glTexCoord2f( 0.0f, 0.0f ); ::glVertex3f( size, -size, 0.0f );
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
opengl_renderer::GetTextureId( std::string Filename, std::string const &Dir, int const Filter, bool const Loadnow ) {

    return m_textures.create( Filename, Dir, Filter, Loadnow );
}

void
opengl_renderer::Bind( texture_handle const Texture ) {
    // temporary until we separate the renderer
    m_textures.bind( Texture );
}

opengl_texture &
opengl_renderer::Texture( texture_handle const Texture ) {

    return m_textures.texture( Texture );
}



bool
opengl_renderer::Render( TGround *Ground ) {

    ::glEnable( GL_LIGHTING );
    ::glDisable( GL_BLEND );
    ::glAlphaFunc( GL_GREATER, 0.50f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    ::glColor3f( 1.0f, 1.0f, 1.0f );

    ++TGroundRect::iFrameNumber; // zwięszenie licznika ramek (do usuwniania nadanimacji)

    Update_Lights( Ground->m_lights );

    m_drawqueue.clear();

    // rednerowanie globalnych (nie za często?)
    for( TGroundNode *node = Ground->srGlobal.nRenderHidden; node; node = node->nNext3 ) {
        node->RenderHidden();
    }

    glm::vec3 const cameraposition( Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z );
    int const camerax = static_cast<int>( std::floor( cameraposition.x / 1000.0f ) + iNumRects / 2 );
    int const cameraz = static_cast<int>( std::floor( cameraposition.z / 1000.0f ) + iNumRects / 2 );
    int const segmentcount = 2 * static_cast<int>(std::ceil( m_drawrange * Global::fDistanceFactor / 1000.0f ));
    int const originx = std::max( 0, camerax - segmentcount / 2 );
    int const originz = std::max( 0, cameraz - segmentcount / 2 );

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

            if( m_camera.visible( cell->m_area ) ) {
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
    // now hand the control over to the renderer of translucent parts, it'll do the rest
    return Render_Alpha( Ground );
}

bool
opengl_renderer::Render( TGroundRect *Groundcell ) {

    bool result { false }; // will be true if we do any rendering

    if( Groundcell->iLastDisplay != Groundcell->iFrameNumber ) {
        // tylko jezeli dany kwadrat nie był jeszcze renderowany
        Groundcell->LoadNodes(); // ewentualne tworzenie siatek

        if( Groundcell->nRenderRect != nullptr ) {
            // nieprzezroczyste trójkąty kwadratu kilometrowego
            for( TGroundNode *node = Groundcell->nRenderRect; node != nullptr; node = node->nNext3 ) {
                Render( node );
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
                        ( Global::pCameraPosition - glm::dvec3( subcell->m_area.center ) ).LengthSquared(),
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
    for( int j = 0; j < Groundsubcell->iTracks; ++j ) {
        Groundsubcell->tTracks[ j ]->RenderDyn();
    }
#ifdef EU07_SCENERY_EDITOR
    // memcells
    if( DebugModeFlag ) {
        for( auto const memcell : m_memcells ) {
            memcell->RenderDL();
        }
    }
#endif
    return true;
}

bool
opengl_renderer::Render( TGroundNode *Node ) {
/*
    Node->SetLastUsage( Timer::GetSimulationTime() );
*/
    switch (Node->iType)
    { // obiekty renderowane niezależnie od odległości
    case TP_SUBMODEL:
        ::glPushMatrix();
        auto const originoffset = Node->pCenter - Global::pCameraPosition;
        ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
        TSubModel::fSquareDist = 0;
        Render( Node->smTerrain );
        ::glPopMatrix();
        return true;
    }

    double const distancesquared = SquareMagnitude( ( Node->pCenter - Global::pCameraPosition ) / Global::ZoomFactor );
    if( ( distancesquared > ( Node->fSquareRadius    * Global::fDistanceFactor ) )
     || ( distancesquared < ( Node->fSquareMinRadius / Global::fDistanceFactor ) ) ) {
        return false;
    }

    switch (Node->iType) {

        case TP_TRACK: {
            // setup
            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
            // render
            Render( Node->pTrack );
            // post-render cleanup
            ::glPopMatrix();
            return true;
        }

        case TP_MODEL: {
            Node->Model->Render( Node->pCenter - Global::pCameraPosition );
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
            ::glColor4fv(
                glm::value_ptr(
                    glm::vec4(
                        Node->Diffuse * glm::vec3( Global::DayLight.ambient ), // w zaleznosci od koloru swiatla
                        1.0 ) ) ); // if the thickness is defined negative, lines are always drawn opaque
            auto const linewidth = clamp( 0.5 * linealpha + Node->fLineThickness * Node->m_radius / 1000.0, 1.0, 32.0 );
            if( linewidth > 1.0 ) {
                ::glLineWidth( static_cast<float>( linewidth ) );
            }

            GfxRenderer.Bind( 0 );

            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
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
             || ( ( Node->iFlags & 0x10 ) == 0 ) ) {
                return false;
            }
            // setup
            ::glColor3fv( glm::value_ptr( Node->Diffuse ) );

            Bind( Node->TextureID );

            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            // render
            m_geometry.draw( Node->Piece->geometry );

            // post-render cleanup
            ::glPopMatrix();

            return true;
        }

        case TP_MEMCELL: {
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

    Dynamic->renderme = m_camera.visible( Dynamic );
    if( false == Dynamic->renderme ) {
        return false;
    }

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    auto const originoffset = Dynamic->vPosition - Global::pCameraPosition;
    double const squaredistance = SquareMagnitude( originoffset / Global::ZoomFactor );
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu
    ::glPushMatrix();

    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
    ::glMultMatrixd( Dynamic->mMatrix.getArray() );

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
                ::glLightModelfv( GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(m_baseambient) );
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
opengl_renderer::Render( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotated( Angle.y, 0.0, 1.0, 0.0 );
    if( Angle.x != 0.0 )
        ::glRotated( Angle.x, 1.0, 0.0, 0.0 );
    if( Angle.z != 0.0 )
        ::glRotated( Angle.z, 0.0, 0.0, 1.0 );

    auto const result = Render( Model, Material, SquareMagnitude( Position ) ); // position is effectively camera offset

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
            if( Submodel->iAlpha & Submodel->iFlags & 0x1F ) // rysuj gdy element nieprzezroczysty
            {
                if( true == Submodel->m_normalizenormals ) {
                    ::glEnable( GL_NORMALIZE );
                }
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
                // ...colors...
                ::glColor3fv( glm::value_ptr(Submodel->f4Diffuse) ); // McZapkie-240702: zamiast ub
                if( ( true == m_renderspecular ) && ( Global::DayLight.specular.a > 0.01f ) ) {
                    // specular strength in legacy models is set uniformly to 150, 150, 150 so we scale it down for opaque elements
                    ::glMaterialfv( GL_FRONT, GL_SPECULAR, glm::value_ptr( Submodel->f4Specular * Global::DayLight.specular.a * m_specularopaquescalefactor ) );
                }
                // ...luminance
                if( Global::fLuminance < Submodel->fLight ) {
                    // zeby swiecilo na kolorowo
                    ::glMaterialfv( GL_FRONT, GL_EMISSION, glm::value_ptr( Submodel->f4Diffuse * Submodel->f4Emision.a ) );
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
                }
                if( true == Submodel->m_normalizenormals ) {
                    ::glDisable( GL_NORMALIZE );
                }
            }
        }
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            auto const &modelview = OpenGLMatrices.data( GL_MODELVIEW );
            auto const lightcenter = modelview * glm::vec4( 0.0f, 0.0f, -0.05f, 1.0f ); // pozycja punktu świecącego względem kamery
            Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f ) - lightcenter ), glm::normalize( -lightcenter ) );

            if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) // kąt większy niż maksymalny stożek swiatła
            {
                float lightlevel = 1.0f; // TODO, TBD: parameter to control light strength
                // view angle attenuation
                float const anglefactor = ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                float const distancefactor = static_cast<float>( std::max( 0.5, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) );

                if( lightlevel > 0.0f ) {
                    // material configuration:
                    ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT );

                    Bind( 0 );
                    ::glPointSize( std::max( 2.0f, 4.0f * distancefactor * anglefactor ) );
                    ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], lightlevel * anglefactor );
                    ::glDisable( GL_LIGHTING );
                    ::glEnable( GL_BLEND );

                    // main draw call
                    m_geometry.draw( Submodel->m_geometry );

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

                // main draw call
                m_geometry.draw( Submodel->m_geometry, color_streams );

                // post-draw reset
                ::glPopAttrib();
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
}

void
opengl_renderer::Render( TTrack *Track ) {

    if( ( Track->TextureID1 == 0 )
     && ( Track->TextureID2 == 0 ) ) {
        return;
    }

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
}

void
opengl_renderer::Render( TMemCell *Memcell ) {

    ::glPushAttrib( GL_ENABLE_BIT );
//    ::glDisable( GL_LIGHTING );
    ::glDisable( GL_TEXTURE_2D );
//    ::glEnable( GL_BLEND );
    ::glPushMatrix();

    auto const position = Memcell->Position();
    ::glTranslated( position.x, position.y + 0.5, position.z );
    ::glColor3f( 0.36f, 0.75f, 0.35f );
    ::gluSphere( m_quadric, 0.35, 4, 2 );

    ::glPopMatrix();
    ::glPopAttrib();
}

bool
opengl_renderer::Render_Alpha( TGround *Ground ) {

    ::glEnable( GL_BLEND );
    ::glAlphaFunc( GL_GREATER, 0.04f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    ::glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

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
    for( int j = 0; j < Groundsubcell->iTracks; ++j )
        Groundsubcell->tTracks[ j ]->RenderDynAlpha(); // przezroczyste fragmenty pojazdów na torach

    return true;
}

bool
opengl_renderer::Render_Alpha( TGroundNode *Node ) {

    double const distancesquared = SquareMagnitude( ( Node->pCenter - Global::pCameraPosition ) / Global::ZoomFactor );
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
                auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
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
            Node->Model->RenderAlpha( Node->pCenter - Global::pCameraPosition );
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
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
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
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
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
    auto const originoffset = Dynamic->vPosition - Global::pCameraPosition;
    double const squaredistance = SquareMagnitude( originoffset / Global::ZoomFactor );
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
opengl_renderer::Render_Alpha( TModel3d *Model, material_data const *Material, Math3D::vector3 const &Position, Math3D::vector3 const &Angle ) {

    ::glPushMatrix();
    ::glTranslated( Position.x, Position.y, Position.z );
    if( Angle.y != 0.0 )
        ::glRotated( Angle.y, 0.0, 1.0, 0.0 );
    if( Angle.x != 0.0 )
        ::glRotated( Angle.x, 1.0, 0.0, 0.0 );
    if( Angle.z != 0.0 )
        ::glRotated( Angle.z, 0.0, 0.0, 1.0 );

    auto const result = Render_Alpha( Model, Material, SquareMagnitude( Position ) ); // position is effectively camera offset

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
                if( true == Submodel->m_normalizenormals ) {
                    ::glEnable( GL_NORMALIZE );
                }
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
                }
                if( true == Submodel->m_normalizenormals ) {
                    ::glDisable( GL_NORMALIZE );
                }
            }
        }
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

         if( framerate > 90.0 ) { targetsegments = 400; targetfactor = 3.0f; }
    else if( framerate > 60.0 ) { targetsegments = 225; targetfactor = 1.5f; }
    else if( framerate > 30.0 ) { targetsegments =  90; targetfactor = Global::ScreenHeight / 768.0f; }
    else                        { targetsegments =   9; targetfactor = Global::ScreenHeight / 768.0f * 0.75f; }
/*
    if( targetsegments > Global::iSegmentsRendered ) {

        Global::iSegmentsRendered = std::min( targetsegments, Global::iSegmentsRendered + 5 );
    }
    else if( targetsegments < Global::iSegmentsRendered ) {

        Global::iSegmentsRendered = std::max( targetsegments, Global::iSegmentsRendered - 5 );
    }
*/
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
    m_geometry.update();

    if( true == DebugModeFlag ) {
        m_debuginfo = m_textures.info();
    }
    else {
        m_debuginfo.clear();
    }
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
        renderlight->set_position( glm::make_vec3( (scenelight.position - Global::pCameraPosition).readArray() ) );
        renderlight->direction = scenelight.direction;

        auto luminance = Global::fLuminance; // TODO: adjust this based on location, e.g. for tunnels
        auto const environment = scenelight.owner->fShade;
        if( environment > 0.0f ) {
            luminance *= environment;
        }
        renderlight->diffuse[ 0 ] = static_cast<GLfloat>( std::max( 0.0, scenelight.color.x - luminance ) );
        renderlight->diffuse[ 1 ] = static_cast<GLfloat>( std::max( 0.0, scenelight.color.y - luminance ) );
        renderlight->diffuse[ 2 ] = static_cast<GLfloat>( std::max( 0.0, scenelight.color.z - luminance ) );
        renderlight->ambient[ 0 ] = static_cast<GLfloat>( std::max( 0.0, scenelight.color.x * scenelight.intensity - luminance) );
        renderlight->ambient[ 1 ] = static_cast<GLfloat>( std::max( 0.0, scenelight.color.y * scenelight.intensity - luminance ) );
        renderlight->ambient[ 2 ] = static_cast<GLfloat>( std::max( 0.0, scenelight.color.z * scenelight.intensity - luminance ) );
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
