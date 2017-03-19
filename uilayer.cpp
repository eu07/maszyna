

#include "stdafx.h"
#include "uilayer.h"
#include "globals.h"
#include "usefull.h"
#include "renderer.h"
#include "logs.h"

ui_layer UILayer;

extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window ); //m7todo: potrzebne do directsound
}

ui_layer::~ui_layer() {
/*
// this should be invoked manually, or we risk trying to delete the lists after the context is gone
    if( m_fontbase != -1 )
        ::glDeleteLists( m_fontbase, 96 );
*/
}

bool
ui_layer::init( GLFWwindow *Window ) {

    HFONT font; // Windows Font ID
    m_fontbase = ::glGenLists(96); // storage for 96 characters
    HDC hDC = ::GetDC( glfwGetWin32Window( Window ) );
    font = ::CreateFont( -MulDiv( 10, ::GetDeviceCaps( hDC, LOGPIXELSY ), 72 ), // height of font
                        0, // width of font
                        0, // angle of escapement
                        0, // orientation angle
                        (Global::bGlutFont ? FW_MEDIUM : FW_HEAVY), // font weight
                        FALSE, // italic
                        FALSE, // underline
                        FALSE, // strikeout
                        DEFAULT_CHARSET, // character set identifier
                        OUT_DEFAULT_PRECIS, // output precision
                        CLIP_DEFAULT_PRECIS, // clipping precision
                        (Global::bGlutFont ? CLEARTYPE_QUALITY : PROOF_QUALITY), // output quality
                        DEFAULT_PITCH | FF_DONTCARE, // family and pitch
                        "Lucida Console"); // font name
    ::SelectObject(hDC, font); // selects the font we want
    if( true == ::wglUseFontBitmaps( hDC, 32, 96, m_fontbase ) ) {
        // builds 96 characters starting at character 32
        WriteLog( "Display Lists font used" ); //+AnsiString(glGetError())
        WriteLog( "Font init OK" ); //+AnsiString(glGetError())
        Global::DLFont = true;
        return true;
    }
    else {
        ErrorLog( "Font init failed" );
//        return false;
        // NOTE: we report success anyway, given some cards can't produce fonts in this manner
        Global::DLFont = false;
        return true;
    }
}

void
ui_layer::render() {

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0.f, Global::ScreenWidth, Global::ScreenHeight, 0.f, -1.f, 1.f );

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );
	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_ALPHA_TEST );
	glEnable( GL_BLEND );

    // render code here
    render_background();
    render_progress();
    render_panels();

	glPopAttrib();
}

void
ui_layer::set_progress( float const Progress, float const Subtaskprogress ) {

    m_progress = Progress * 0.01f;
    m_subtaskprogress = Subtaskprogress * 0.01f;
}

void
ui_layer::set_background( std::string const &Filename ) {

    if( false == Filename.empty() ) {

        m_background = GfxRenderer.GetTextureId( Filename, szTexturePath );
    }
    else {

        m_background = 0;
    }
}
/*
void cGuiLayer::setNote( const std::string Note ) { mNote = Note; }

std::string cGuiLayer::getNote() { return mNote; }
*/
void
ui_layer::render_progress() {

	if( (m_progress == 0.0f) && (m_subtaskprogress == 0.0f) ) return;

	glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_TEXTURE_2D );

    quad( float4( 75.0f, 640.0f, 75.0f + 320.0f, 640.0f + 16.0f ), float4(0.0f, 0.0f, 0.0f, 0.25f) );
    // secondary bar
    if( m_subtaskprogress ) {
        quad(
            float4( 75.0f, 640.0f, 75.0f + 320.0f * m_subtaskprogress, 640.0f + 16.0f),
            float4( 8.0f/255.0f, 160.0f/255.0f, 8.0f/255.0f, 0.35f ) );
    }
    // primary bar
	if( m_progress ) {
        quad(
            float4( 75.0f, 640.0f, 75.0f + 320.0f * m_progress, 640.0f + 16.0f ),
            float4( 8.0f / 255.0f, 160.0f / 255.0f, 8.0f / 255.0f, 1.0f ) );
    }

	glPopAttrib();
}

void
ui_layer::render_panels() {

    if( m_panels.empty() ) { return; }

    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_TEXTURE_2D );

    float const width = std::min( 4.0f / 3.0f, static_cast<float>(Global::iWindowWidth) / Global::iWindowHeight ) * Global::iWindowHeight;
    float const height = Global::iWindowHeight / 768.0;

    for( auto const &panel : m_panels ) {

        int lineidx = 0;
        for( auto const &line : panel->text_lines ) {

            ::glColor4fv( &line.color.x );
            ::glRasterPos2f(
                0.5 * ( Global::iWindowWidth - width ) + panel->origin_x * height,
                panel->origin_y * height + 20.0 * lineidx );
            print( line.data );
            ++lineidx;
        }
    }

    glPopAttrib();
}

void
ui_layer::render_background() {

	if( m_background == 0 ) return;

    // NOTE: we limit/expect the background to come with 4:3 ratio.
    // TODO, TBD: if we expose texture width or ratio from texture object, this limitation could be lifted

    GfxRenderer.Bind( m_background );
    quad( float4( 0.0f, 0.0f, 1024.0f, 768.0f ), float4( 1.0f, 1.0f, 1.0f, 1.0f ) );
}

void
ui_layer::print( std::string const &Text )
{
    if( (false == Global::DLFont)
     || (true == Text.empty()) )
        return;
    
    ::glPushAttrib( GL_LIST_BIT );

    ::glListBase( m_fontbase - 32 );
    ::glCallLists( Text.size(), GL_UNSIGNED_BYTE, Text.c_str() );

    ::glPopAttrib();
}

void
ui_layer::quad( float4 const &Coordinates, float4 const &Color ) {

    float const screenratio = static_cast<float>( Global::iWindowWidth ) / Global::iWindowHeight;
    float const width =
        ( screenratio >= (4.0f/3.0f) ?
            ( 4.0f / 3.0f ) * Global::iWindowHeight :
            Global::iWindowWidth );
    float const heightratio =
        ( screenratio >= ( 4.0f / 3.0f ) ?
            Global::iWindowHeight / 768.0 :
            Global::iWindowHeight / 768.0 * screenratio / ( 4.0f / 3.0f ) );
    float const height = 768.0f * heightratio;

    glColor4fv(&Color.x);

    glBegin( GL_TRIANGLE_STRIP );

    glTexCoord2f( 0.0f, 1.0f ); glVertex2f( 0.5 * ( Global::iWindowWidth - width ) + Coordinates.x * heightratio, 0.5 * ( Global::iWindowHeight - height ) + Coordinates.y * heightratio );
    glTexCoord2f( 0.0f, 0.0f ); glVertex2f( 0.5 * ( Global::iWindowWidth - width ) + Coordinates.x * heightratio, 0.5 * ( Global::iWindowHeight - height ) + Coordinates.w * heightratio );
    glTexCoord2f( 1.0f, 1.0f ); glVertex2f( 0.5 * ( Global::iWindowWidth - width ) + Coordinates.z * heightratio, 0.5 * ( Global::iWindowHeight - height ) + Coordinates.y * heightratio );
    glTexCoord2f( 1.0f, 0.0f ); glVertex2f( 0.5 * ( Global::iWindowWidth - width ) + Coordinates.z * heightratio, 0.5 * ( Global::iWindowHeight - height ) + Coordinates.w * heightratio );

    glEnd();
}
