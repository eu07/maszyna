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
    glm::vec3 diagonal(
        static_cast<float>( Dynamic->MoverParameters->Dim.L ),
        static_cast<float>( Dynamic->MoverParameters->Dim.H ),
        static_cast<float>( Dynamic->MoverParameters->Dim.W ) );
    float const radius = glm::length( diagonal ) * 0.5f;

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


    // preload some common textures
    WriteLog( "Loading common gfx data..." );
    m_glaretextureid = GetTextureId( "fx\\lightglare", szTexturePath );
    m_suntextureid = GetTextureId( "fx\\sun", szTexturePath );
    m_moontextureid = GetTextureId( "fx\\moon", szTexturePath );
    WriteLog( "...gfx data pre-loading done" );

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
/*
        World.Camera.SetMatrix();
        m_camera.update_frustum();
*/
        glm::dmat4 worldcamera;
        World.Camera.SetMatrix( worldcamera );
        m_camera.update_frustum( OpenGLMatrices.data( GL_PROJECTION ), worldcamera );
        // frustum tests are performed in 'world space' but after we set up frustum
        // we no longer need camera translation, only rotation
        ::glMultMatrixd( glm::value_ptr( glm::dmat4( glm::dmat3( worldcamera ))));

		glDisable(GL_FRAMEBUFFER_SRGB);
        Render( &World.Environment );
		glUseProgram(World.shader);
		glEnable(GL_FRAMEBUFFER_SRGB);
        Render( &World.Ground );

		glDebug("rendering cab");
        World.Render_Cab();

        // accumulate last 20 frames worth of render time (cap at 1000 fps to prevent calculations going awry)
        m_drawtime = std::max( 20.0f, 0.95f * m_drawtime + std::chrono::duration_cast<std::chrono::milliseconds>( ( std::chrono::steady_clock::now() - timestart ) ).count());
    }

	glUseProgram(0);
	glEnable(GL_FRAMEBUFFER_SRGB);
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
/*
    ::glTranslatef( Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z );
*/
/*
    glm::mat4 worldcamera;
    World.Camera.SetMatrix( worldcamera );
    glLoadIdentity();
    glMultMatrixf( glm::value_ptr( glm::mat4( glm::mat3( worldcamera ) ) ) );
*/   
    // setup fog
    if( Global::fFogEnd > 0 ) {
        // fog setup
        ::glFogfv( GL_FOG_COLOR, Global::FogColor );
        ::glFogf( GL_FOG_DENSITY, static_cast<GLfloat>( 1.0 / Global::fFogEnd ) );
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
        ::glColor4f( mooncolor.x, mooncolor.y, mooncolor.z, static_cast<GLfloat>( 1.0 - Global::fLuminance * 0.5 ) );

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

    ::glEnable( GL_LIGHTING );
    ::glDisable( GL_BLEND );
    ::glAlphaFunc( GL_GREATER, 0.50f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    ::glColor3f( 1.0f, 1.0f, 1.0f );

    ++TGroundRect::iFrameNumber; // zwięszenie licznika ramek (do usuwniania nadanimacji)

    Update_Lights( Ground->m_lights );
/*
    glm::vec3 const cameraposition( Global::pCameraPosition.x, Global::pCameraPosition.y, Global::pCameraPosition.z );
    int const camerax = static_cast<int>( std::floor( cameraposition.x / 1000.0f ) + iNumRects / 2 );
    int const cameraz = static_cast<int>( std::floor( cameraposition.z / 1000.0f ) + iNumRects / 2 );
    int const segmentcount = 2 * static_cast<int>(std::ceil( m_drawrange * Global::fDistanceFactor / 1000.0f ));
    int const originx = std::max( 0, camerax - segmentcount / 2 );
    int const originz = std::max( 0, cameraz - segmentcount / 2 );

    for( int column = originx; column <= originx + segmentcount; ++column ) {
        for( int row = originz; row <= originz + segmentcount; ++row ) {

            auto *rectangle = &Ground->Rects[ column ][ row ];
            if( m_camera.visible( rectangle->m_area ) ) {
                Render( rectangle );
            }
        }
    }
*/
    Ground->CameraDirection.x = std::sin( Global::pCameraRotation ); // wektor kierunkowy
    Ground->CameraDirection.z = std::cos( Global::pCameraRotation );
    TGroundNode *node;
    // rednerowanie globalnych (nie za często?)
    for( node = Ground->srGlobal.nRenderHidden; node; node = node->nNext3 ) {
        node->RenderHidden();
    }
    // renderowanie czołgowe dla obiektów aktywnych a niewidocznych
    int n = 2 * iNumSubRects; //(2*==2km) promień wyświetlanej mapy w sektorach
    int c = Ground->GetColFromX( Global::pCameraPosition.x );
    int r = Ground->GetRowFromZ( Global::pCameraPosition.z );
    TSubRect *tmp;
    int i, j, k;
    for( j = r - n; j <= r + n; j++ ) {
        for( i = c - n; i <= c + n; i++ ) {
            if( ( tmp = Ground->FastGetSubRect( i, j ) ) != nullptr ) {
                // oznaczanie aktywnych sektorów
                tmp->LoadNodes();

                for( node = tmp->nRenderHidden; node; node = node->nNext3 ) {
                    node->RenderHidden();
                }
                // jeszcze dźwięki pojazdów by się przydały, również niewidocznych
                tmp->RenderSounds();
            }
        }
    }
    // renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
    // pre-calculate camera view span
    double const fieldofviewcosine =
        std::cos(
            std::max(
                // vertical...
                Global::FieldOfView / Global::ZoomFactor,
                // ...or horizontal, whichever is bigger
                Global::FieldOfView / Global::ZoomFactor
                * std::max( 1.0f, (float)Global::ScreenWidth ) / std::max( 1.0f, (float)Global::ScreenHeight ) ) );

    Ground->iRendered = 0; // ilość renderowanych sektorów
    Math3D::vector3 direction;
    for( k = 0; k < Global::iSegmentsRendered; ++k ) // sektory w kolejności odległości
    { // przerobione na użycie SectorOrder
        i = SectorOrder[ k ].x; // na starcie oba >=0
        j = SectorOrder[ k ].y;
        do {
            // pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0 czwarty: j>=0, i>=0;
            if( j <= 0 )
                i = -i;
            j = -j; // i oraz j musi być zmienione wcześniej, żeby continue działało
            direction = Math3D::vector3( i, 0, j ); // wektor od kamery do danego sektora
            if( Math3D::LengthSquared3( direction ) > 5 ) // te blisko są zawsze wyświetlane
            {
                direction = Math3D::SafeNormalize( direction ); // normalizacja
                if( Ground->CameraDirection.x * direction.x + Ground->CameraDirection.z * direction.z < 0.5 )
                    continue; // pomijanie sektorów poza kątem patrzenia
            }
            // kwadrat kilometrowy nie zawsze, bo szkoda FPS
            Render( &Ground->Rects[ ( i + c ) / iNumSubRects ][ ( j + r ) / iNumSubRects ] );

            if( ( tmp = Ground->FastGetSubRect( i + c, j + r ) ) != nullptr ) {
                if( tmp->iNodeCount ) {
                    // o ile są jakieś obiekty, bo po co puste sektory przelatywać
                    Ground->pRendered[ Ground->iRendered++ ] = tmp; // tworzenie listy sektorów do renderowania
                }
            }
        } while( ( i < 0 ) || ( j < 0 ) ); // są 4 przypadki, oprócz i=j=0
    }
    // dodać renderowanie terenu z E3D - jedno VBO jest używane dla całego modelu, chyba że jest ich więcej
    if( Global::pTerrainCompact ) {
        Global::pTerrainCompact->TerrainRenderVBO( TGroundRect::iFrameNumber );
    }
    // renderowanie nieprzezroczystych
    for( i = 0; i < Ground->iRendered; ++i ) {
        Render( Ground->pRendered[ i ] );
    }
    // regular render takes care of all solid geometry present in the scene, thus we can launch alpha parts render here
    return Render_Alpha( Ground );
}

// TODO: unify ground render code, until then old version is in place
#define EU07_USE_OLD_RENDERCODE
bool
opengl_renderer::Render( TGroundRect *Groundcell ) {

    ::glPushMatrix();
    auto const &cellorigin = Groundcell->m_area.center;
    // TODO: unify all math objects
    auto const originoffset = Math3D::vector3( cellorigin.x, cellorigin.y, cellorigin.z ) - Global::pCameraPosition;
    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

    bool result{ false }; // will be true if we do any rendering

    // TODO: unify render paths

        if ( Groundcell->iLastDisplay != Groundcell->iFrameNumber)
        { // tylko jezeli dany kwadrat nie był jeszcze renderowany
            Groundcell->LoadNodes(); // ewentualne tworzenie siatek
            if ( Groundcell->nRenderRect && Groundcell->StartVBO())
            {
                for (TGroundNode *node = Groundcell->nRenderRect; node; node = node->nNext3) // następny tej grupy
#ifdef EU07_USE_OLD_RENDERCODE
                    node->RaRenderVBO(); // nieprzezroczyste trójkąty kwadratu kilometrowego
#else
                    Render( node ); // nieprzezroczyste trójkąty kwadratu kilometrowego
#endif
                Groundcell->EndVBO();
                Groundcell->iLastDisplay = Groundcell->iFrameNumber;
                result = true;
            }
            if ( Groundcell->nTerrain)
                Groundcell->nTerrain->smTerrain->iVisible = Groundcell->iFrameNumber; // ma się wyświetlić w tej ramce
        }
        ::glPopMatrix();

    return result;
}
#undef EU07_USE_OLD_RENDERCODE

bool
opengl_renderer::Render( TSubRect *Groundsubcell ) {

    Groundsubcell->RaAnimate(); // przeliczenia animacji torów w sektorze

    TGroundNode *node;
    // nieprzezroczyste obiekty terenu

    // vbo render path
    if( Groundsubcell->StartVBO() ) {
        for( node = Groundsubcell->nRenderRect; node; node = node->nNext3 ) {
            if( node->iVboPtr >= 0 ) {
                Render( node );
            }
        }
        Groundsubcell->EndVBO();
    }

    // nieprzezroczyste obiekty (oprócz pojazdów)
    for( node = Groundsubcell->nRender; node; node = node->nNext3 )
        Render( node );
    // nieprzezroczyste z mieszanych modeli
    for( node = Groundsubcell->nRenderMixed; node; node = node->nNext3 )
        Render( node );
    // nieprzezroczyste fragmenty pojazdów na torach
    for( int j = 0; j < Groundsubcell->iTracks; ++j )
        Groundsubcell->tTracks[ j ]->RenderDyn();
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

    Node->SetLastUsage( Timer::GetSimulationTime() );

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

    switch (Node->iType)
    {
        case TP_TRACK: {

            if( ( Node->iNumVerts <= 0 ) ) {
                return false;
            }
            // setup
            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            // TODO: unify the render code after generic buffers are in place
                // vbo render path
            Node->pTrack->RaRenderVBO( Node->iVboPtr );

            // post-render cleanup
            ::glPopMatrix();
            return true;
        }
        case TP_MODEL: {
            Node->Model->Render( Node->pCenter - Global::pCameraPosition );
            return true;
        }
        case TP_MEMCELL: {
            GfxRenderer.Render( Node->MemCell );
            return true;
        }
    }

    // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
    if( ( Node->iFlags & 0x10 )
     || ( Node->fLineThickness < 0 ) ) {
        // TODO: unify the render code after generic buffers are in place

        if( ( Node->iType == GL_LINES )
         || ( Node->iType == GL_LINE_STRIP )
         || ( Node->iType == GL_LINE_LOOP ) ) {
            // wszelkie linie są rysowane na samym końcu
            if( Node->iNumPts ) {
                // setup
                // w zaleznosci od koloru swiatla
                ::glColor4ub(
                    static_cast<GLubyte>( std::floor( Node->Diffuse[ 0 ] * Global::daylight.ambient.x ) ),
                    static_cast<GLubyte>( std::floor( Node->Diffuse[ 1 ] * Global::daylight.ambient.y ) ),
                    static_cast<GLubyte>( std::floor( Node->Diffuse[ 2 ] * Global::daylight.ambient.z ) ),
                    static_cast<GLubyte>( std::min( 255.0, 255000 * Node->fLineThickness / ( distancesquared + 1.0 ) ) ) );

                GfxRenderer.Bind( 0 );

                // render
                // TODO: unify the render code after generic buffers are in place
                ::glPushMatrix();
                auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                ::glDrawArrays( Node->iType, Node->iVboPtr, Node->iNumPts );
                ::glPopMatrix();
                // post-render cleanup
                return true;
            }
            else {
                return false;
            }
        }
        else {
            // GL_TRIANGLE etc
            if (Node->iVboPtr < 0) {
                return false;
            }
            // setup
            ::glColor3ub(
                static_cast<GLubyte>( Node->Diffuse[ 0 ] ),
                static_cast<GLubyte>( Node->Diffuse[ 1 ] ),
                static_cast<GLubyte>( Node->Diffuse[ 2 ] ) );

            Bind( Node->TextureID );

            // render
            // TODO: unify the render code after generic buffers are in place
            // vbo render path
            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
            ::glDrawArrays( Node->iType, Node->iVboPtr, Node->iNumVerts );
            ::glPopMatrix();
            // post-render cleanup
            return true;
        }
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
/*
    if( Dynamic == Global::pUserDynamic ) {
        //specjalne ustawienie, aby nie trzęsło
        //tu trzeba by ustawić animacje na modelu zewnętrznym
        ::glLoadIdentity(); // zacząć od macierzy jedynkowej
        Global::pCamera->SetCabMatrix( Dynamic->vPosition ); // specjalne ustawienie kamery
    }
    else
        ::glTranslated( Dynamic->vPosition.x, Dynamic->vPosition.y, Dynamic->vPosition.z ); // standardowe przesunięcie względem początku scenerii
*/
    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
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

//    auto const result = Render( Model, Material, SquareMagnitude( Position / Global::ZoomFactor ) ); // position is effectively camera offset
    auto const result = Render( Model, Material, SquareMagnitude( Position ) ); // position is effectively camera offset

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

				if (Global::fLuminance < Submodel->fLight)
					World.shader.set_material(glm::make_vec3(Submodel->f4Diffuse));

                // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                ::glDrawArrays( Submodel->eType, Submodel->iVboPtr, Submodel->iNumVerts );

				if (Global::fLuminance < Submodel->fLight)
					World.shader.set_material(glm::vec3(0.0f));
            }
        }
		
        else if( Submodel->eType == TP_FREESPOTLIGHT ) {
			//m7todo: shaderize
			
			auto const &modelview = mm;
            auto const lightcenter = modelview * glm::vec4( 0.0f, 0.0f, -0.05f, 1.0f ); // pozycja punktu świecącego względem kamery
            Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f ) - lightcenter ), glm::normalize( -lightcenter ) );

            if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) // kąt większy niż maksymalny stożek swiatła
            {
                float lightlevel = 1.0f;
                // view angle attenuation
                float const anglefactor = ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                // distance attenuation. NOTE: since it's fixed pipeline with built-in gamma correction we're using linear attenuation
                // we're capping how much effect the distance attenuation can have, otherwise the lights get too tiny at regular distances
                float const distancefactor = static_cast<float>( std::max( 0.5, ( Submodel->fSquareMaxDist - TSubModel::fSquareDist ) / ( Submodel->fSquareMaxDist * Global::fDistanceFactor ) ) );

                if( lightlevel > 0.0f ) {
					glUseProgram(0);
					glEnableClientState(GL_VERTEX_ARRAY);
					glLoadMatrixf(glm::value_ptr(mm));
					glVertexPointer(3, GL_FLOAT, sizeof(CVertNormTex), static_cast<char *>(nullptr)); // pozycje

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
					glDisableClientState(GL_VERTEX_ARRAY);

					glUseProgram(World.shader);
                }
            }
			
        }
        else if( Submodel->eType == TP_STARS ) {
			//m7todo: reenable
			/*
            if( Global::fLuminance < Submodel->fLight ) {
				glUseProgram(0);

                // material configuration:
                ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );

                Bind( 0 );
                ::glDisable( GL_LIGHTING );

                // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                // NOTE: we're doing manual switch to color vbo setup, because there doesn't seem to be any convenient way available atm
                // TODO: implement easier way to go about it

				glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState( GL_COLOR_ARRAY );
				glLoadMatrixf(glm::value_ptr(mm));

				glVertexPointer(3, GL_FLOAT, sizeof(CVertNormTex), static_cast<char *>(nullptr)); // pozycje
                glColorPointer( 3, GL_FLOAT, sizeof( CVertNormTex ), static_cast<char *>( nullptr ) + 12 ); // kolory

                glDrawArrays( GL_POINTS, Submodel->iVboPtr, Submodel->iNumVerts );

				glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState( GL_COLOR_ARRAY );

                // post-draw reset
                ::glPopAttrib();

				glUseProgram(World.shader);
            }
			*/
        }

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
            Render( Submodel->Next ); // dalsze rekurencyjnie
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

    // legacy version of the code:
    ::glEnable( GL_BLEND );
    ::glAlphaFunc( GL_GREATER, 0.04f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    ::glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    TGroundNode *node;
    TSubRect *tmp;
    // Ra: renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
    for( int i = Ground->iRendered - 1; i >= 0; --i ) // od najdalszych
    { // przezroczyste trójkąty w oddzielnym cyklu przed modelami
        tmp = Ground->pRendered[ i ];
        // vbo render path
        if( tmp->StartVBO() ) {
            for( node = tmp->nRenderRectAlpha; node; node = node->nNext3 ) {
                if( node->iVboPtr >= 0 ) {
                    Render_Alpha( node );
                }
            }
            tmp->EndVBO();
        }
    }
    for( int i = Ground->iRendered - 1; i >= 0; --i ) // od najdalszych
    { // renderowanie przezroczystych modeli oraz pojazdów
        Render_Alpha( Ground->pRendered[ i ] );
    }

    ::glDisable( GL_LIGHTING ); // linie nie powinny świecić

    for( int i = Ground->iRendered - 1; i >= 0; --i ) // od najdalszych
    { // druty na końcu, żeby się nie robiły białe plamy na tle lasu
        tmp = Ground->pRendered[ i ];
        // vbo render path
        if( tmp->StartVBO() ) {
            for( node = tmp->nRenderWires; node; node = node->nNext3 ) {
                Render_Alpha( node );
            }
            tmp->EndVBO();
        }
    }

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

// NOTE: legacy render system switch
#define _PROBLEND

bool
opengl_renderer::Render_Alpha( TGroundNode *Node ) {

    // SPOSOB NA POZBYCIE SIE RAMKI DOOKOLA TEXTURY ALPHA DLA OBIEKTOW ZAGNIEZDZONYCH W SCN JAKO
    // NODE

    // W GROUND.H dajemy do klasy TGroundNode zmienna bool PROBLEND to samo robimy w klasie TGround
    // nastepnie podczas wczytywania textury dla TRIANGLES w TGround::AddGroundNode
    // sprawdzamy czy w nazwie jest @ i wg tego
    // ustawiamy PROBLEND na true dla wlasnie wczytywanego trojkata (kazdy trojkat jest osobnym
    // nodem)
    // nastepnie podczas renderowania w bool TGroundNode::RenderAlpha()
    // na poczatku ustawiamy standardowe GL_GREATER = 0.04
    // pozniej sprawdzamy czy jest wlaczony PROBLEND dla aktualnie renderowanego noda TRIANGLE,
    // wlasciwie dla kazdego node'a
    // i jezeli tak to odpowiedni GL_GREATER w przeciwnym wypadku standardowy 0.04

    Node->SetLastUsage( Timer::GetSimulationTime() );

    double const distancesquared = SquareMagnitude( ( Node->pCenter - Global::pCameraPosition ) / Global::ZoomFactor );
    if( ( distancesquared > ( Node->fSquareRadius    * Global::fDistanceFactor ) )
     || ( distancesquared < ( Node->fSquareMinRadius / Global::fDistanceFactor ) ) ) {
        return false;
    }

    switch (Node->iType)
    {
        case TP_TRACTION: {
            // TODO: unify the render code after generic buffers are in place
            if( Node->bVisible ) {
                // setup
                ::glPushMatrix();
                auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                // render
                Node->hvTraction->RenderVBO( distancesquared, Node->iVboPtr );
                // post-render cleanup
                ::glPopMatrix();
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
    }

    // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
    if( ( Node->iNumVerts && ( Node->iFlags & 0x20 ) )
     || ( Node->iNumPts && ( Node->fLineThickness > 0 ) ) ) {

#ifdef _PROBLEND
        if( ( Node->PROBLEND ) ) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
        {
            ::glDisable( GL_BLEND );
            ::glAlphaFunc( GL_GREATER, 0.50f ); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
        };
#endif
        // TODO: unify the render code after generic buffers are in place
            // additional setup for display lists
        if( ( Node->DisplayListID == 0 )
            || ( Node->iVersion != Global::iReCompile ) ) { // Ra: wymuszenie rekompilacji
            Node->Compile(Node->m_rootposition);
            if( Global::bManageNodes )
                ResourceManager::Register( Node );
        };

        bool result( false );

        if( ( Node->iType == GL_LINES )
         || ( Node->iType == GL_LINE_STRIP )
         || ( Node->iType == GL_LINE_LOOP ) ) {
            // wszelkie linie są rysowane na samym końcu
            if( Node->iNumPts ) {
                // setup
                ::glPushMatrix();
                auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
                ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
                // w zaleznosci od koloru swiatla
                ::glColor4ub(
                    static_cast<GLubyte>( std::floor( Node->Diffuse[ 0 ] * Global::daylight.ambient.x ) ),
                    static_cast<GLubyte>( std::floor( Node->Diffuse[ 1 ] * Global::daylight.ambient.y ) ),
                    static_cast<GLubyte>( std::floor( Node->Diffuse[ 2 ] * Global::daylight.ambient.z ) ),
                    static_cast<GLubyte>( std::min( 255.0, 255000 * Node->fLineThickness / ( distancesquared + 1.0 ) ) ) );

                GfxRenderer.Bind( 0 );

                // render
				// TODO: unify the render code after generic buffers are in place
                ::glDrawArrays( Node->iType, Node->iVboPtr, Node->iNumPts );
                // post-render cleanup
                ::glPopMatrix();
                result = true;
            }
        }
        else {
            // GL_TRIANGLE etc
            // setup
            ::glPushMatrix();
            auto const originoffset = Node->m_rootposition - Global::pCameraPosition;
            ::glTranslated( originoffset.x, originoffset.y, originoffset.z );

            ::glColor3ub(
                static_cast<GLubyte>( Node->Diffuse[ 0 ] ),
                static_cast<GLubyte>( Node->Diffuse[ 1 ] ),
                static_cast<GLubyte>( Node->Diffuse[ 2 ] ) );

            Bind( Node->TextureID );

            // render
            // TODO: unify the render code after generic buffers are in place

            // vbo render path
            if( Node->iVboPtr >= 0 ) {
                ::glDrawArrays( Node->iType, Node->iVboPtr, Node->iNumVerts );
                result = true;
            }

            // post-render cleanup
            ::glPopMatrix();
        }

#ifdef _PROBLEND
        if( ( Node->PROBLEND ) ) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
        {
            ::glEnable( GL_BLEND );
            ::glAlphaFunc( GL_GREATER, 0.04f );
        }
#endif
        return result;
    }
    // in theory we shouldn't ever get here but, eh
    return false;
}

bool
opengl_renderer::Render_Alpha( TDynamicObject *Dynamic ) {

    if( false == Dynamic->renderme ) {

        return false;
    }

    // setup
    TSubModel::iInstance = ( size_t )this; //żeby nie robić cudzych animacji
    auto const originoffset = Dynamic->vPosition - Global::pCameraPosition;
    double const squaredistance = SquareMagnitude( originoffset / Global::ZoomFactor );
    Dynamic->ABuLittleUpdate( squaredistance ); // ustawianie zmiennych submodeli dla wspólnego modelu
    ::glPushMatrix();
/*
    if( Dynamic == Global::pUserDynamic ) { // specjalne ustawienie, aby nie trzęsło
        ::glLoadIdentity(); // zacząć od macierzy jedynkowej
        Global::pCamera->SetCabMatrix( Dynamic->vPosition ); // specjalne ustawienie kamery
    }
    else
        ::glTranslated( Dynamic->vPosition.x, Dynamic->vPosition.y, Dynamic->vPosition.z ); // standardowe przesunięcie względem początku scenerii
*/
    ::glTranslated( originoffset.x, originoffset.y, originoffset.z );
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

//    auto const result = Render_Alpha( Model, Material, SquareMagnitude( Position / Global::ZoomFactor ) ); // position is effectively camera offset
    auto const result = Render_Alpha( Model, Material, SquareMagnitude( Position ) ); // position is effectively camera offset

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

				if (Global::fLuminance < Submodel->fLight)
					World.shader.set_material(glm::make_vec3(Submodel->f4Diffuse));

			    // main draw call. TODO: generic buffer base class, specialized for vbo, dl etc
                ::glDrawArrays( Submodel->eType, Submodel->iVboPtr, Submodel->iNumVerts );

				if (Global::fLuminance < Submodel->fLight)
					World.shader.set_material(glm::vec3(0.0f));
            }
        }

        else if( Submodel->eType == TP_FREESPOTLIGHT ) {

            if( Global::fLuminance < Submodel->fLight ) {
                // NOTE: we're forced here to redo view angle calculations etc, because this data isn't instanced but stored along with the single mesh
                // TODO: separate instance data from reusable geometry
                auto const &modelview = mm;
                auto const lightcenter = modelview * glm::vec4( 0.0f, 0.0f, -0.05f, 1.0f ); // pozycja punktu świecącego względem kamery
                Submodel->fCosViewAngle = glm::dot( glm::normalize( modelview * glm::vec4( 0.0f, 0.0f, -1.0f, 1.0f ) - lightcenter ), glm::normalize( -lightcenter ) );

                float glarelevel = 0.6f; // luminosity at night is at level of ~0.1, so the overall resulting transparency is ~0.5 at full 'brightness'
                if( Submodel->fCosViewAngle > Submodel->fCosFalloffAngle ) {

                    glarelevel *= ( Submodel->fCosViewAngle - Submodel->fCosFalloffAngle ) / ( 1.0f - Submodel->fCosFalloffAngle );
                    glarelevel = std::max( 0.0f, glarelevel - static_cast<float>(Global::fLuminance) );

                    if( glarelevel > 0.0f ) {
						glUseProgram(0);

                        ::glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );

                        Bind( m_glaretextureid );
                        ::glColor4f( Submodel->f4Diffuse[ 0 ], Submodel->f4Diffuse[ 1 ], Submodel->f4Diffuse[ 2 ], glarelevel );
                        ::glDisable( GL_LIGHTING );
                        ::glBlendFunc( GL_SRC_ALPHA, GL_ONE );

						glm::mat4 x = glm::mat4(1.0f);
						x = glm::translate(x, glm::vec3(lightcenter.x, lightcenter.y, lightcenter.z)); // początek układu zostaje bez zmian
						x = glm::rotate(x, atan2(lightcenter.x, lightcenter.y), glm::vec3(0.0f, 1.0f, 0.0f)); // jedynie obracamy w pionie o kąt
						glLoadMatrixf(glm::value_ptr(x));

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
                        ::glPopAttrib();

						glUseProgram(World.shader);
                    }
                }
            }
        }

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

         if( framerate > 90.0 ) { targetsegments = 400; targetfactor = 3.0f; }
    else if( framerate > 60.0 ) { targetsegments = 225; targetfactor = 1.5f; }
    else if( framerate > 30.0 ) { targetsegments =  90; targetfactor = Global::ScreenHeight / 768.0f; }
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
