

#include "stdafx.h"
#include "uilayer.h"
#include "globals.h"
#include "utilities.h"
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

    m_window = Window;
    HFONT font; // Windows Font ID
    m_fontbase = ::glGenLists(96); // storage for 96 characters
    HDC hDC = ::GetDC( glfwGetWin32Window( m_window ) );
    font = ::CreateFont( -MulDiv( 10, ::GetDeviceCaps( hDC, LOGPIXELSY ), 72 ), // height of font
                        0, // width of font
                        0, // angle of escapement
                        0, // orientation angle
                        (Global.bGlutFont ? FW_MEDIUM : FW_HEAVY), // font weight
                        FALSE, // italic
                        FALSE, // underline
                        FALSE, // strikeout
                        DEFAULT_CHARSET, // character set identifier
                        OUT_DEFAULT_PRECIS, // output precision
                        CLIP_DEFAULT_PRECIS, // clipping precision
                        (Global.bGlutFont ? CLEARTYPE_QUALITY : PROOF_QUALITY), // output quality
                        DEFAULT_PITCH | FF_DONTCARE, // family and pitch
                        "Lucida Console"); // font name
    ::SelectObject(hDC, font); // selects the font we want
    if( TRUE == ::wglUseFontBitmaps( hDC, 32, 96, m_fontbase ) ) {
        // builds 96 characters starting at character 32
        WriteLog( "Display Lists font used" ); //+AnsiString(glGetError())
        WriteLog( "Font init OK" ); //+AnsiString(glGetError())
        Global.DLFont = true;
        return true;
    }
    else {
        ErrorLog( "Font init failed" );
//        return false;
        // NOTE: we report success anyway, given some cards can't produce fonts in this manner
        Global.DLFont = false;
        return true;
    }
}

void
ui_layer::render() {

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0, std::max( 1, Global.iWindowWidth ), std::max( 1, Global.iWindowHeight ), 0, -1, 1 );

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );
	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_ALPHA_TEST );
	glEnable( GL_BLEND );

    // render code here
    render_background();
    render_texture();
    render_progress();
    render_panels();
    render_tooltip();

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
        m_background = GfxRenderer.Fetch_Texture( Filename );
    }
    else {
        m_background = null_handle;
    }
    if( m_background != null_handle ) {
        auto const &texture = GfxRenderer.Texture( m_background );
        m_progressbottom = ( texture.width() != texture.height() );
    }
}

void
ui_layer::render_progress() {

	if( (m_progress == 0.0f) && (m_subtaskprogress == 0.0f) ) return;

	glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_TEXTURE_2D );

    glm::vec2 origin, size;
    if( m_progressbottom == true ) {
        origin = glm::vec2{ 0.0f, 768.0f - 20.0f };
        size   = glm::vec2{ 1024.0f, 20.0f };
    }
    else {
        origin = glm::vec2{ 75.0f, 640.0f };
        size   = glm::vec2{ 320.0f, 16.0f };
    }

    quad( glm::vec4( origin.x, origin.y, origin.x + size.x, origin.y + size.y ), glm::vec4(0.0f, 0.0f, 0.0f, 0.25f) );
    // secondary bar
    if( m_subtaskprogress ) {
        quad(
            glm::vec4( origin.x, origin.y, origin.x + size.x * m_subtaskprogress, origin.y + size.y),
            glm::vec4( 8.0f/255.0f, 160.0f/255.0f, 8.0f/255.0f, 0.35f ) );
    }
    // primary bar
	if( m_progress ) {
        quad(
            glm::vec4( origin.x, origin.y, origin.x + size.x * m_progress, origin.y + size.y ),
            glm::vec4( 8.0f / 255.0f, 160.0f / 255.0f, 8.0f / 255.0f, 1.0f ) );
    }

    if( false == m_progresstext.empty() ) {
        float const screenratio = static_cast<float>( Global.iWindowWidth ) / Global.iWindowHeight;
        float const width =
            ( screenratio >= (4.0f/3.0f) ?
                ( 4.0f / 3.0f ) * Global.iWindowHeight :
                Global.iWindowWidth );
        float const heightratio =
            ( screenratio >= ( 4.0f / 3.0f ) ?
                Global.iWindowHeight / 768.f :
                Global.iWindowHeight / 768.f * screenratio / ( 4.0f / 3.0f ) );
        float const height = 768.0f * heightratio;

        ::glColor4f( 216.0f / 255.0f, 216.0f / 255.0f, 216.0f / 255.0f, 1.0f );
        auto const charsize = 9.0f;
        auto const textwidth = m_progresstext.size() * charsize;
        auto const textheight = 12.0f;
        ::glRasterPos2f(
            ( 0.5f * ( Global.iWindowWidth  - width )  + origin.x * heightratio ) + ( ( size.x * heightratio - textwidth ) * 0.5f * heightratio ),
            ( 0.5f * ( Global.iWindowHeight - height ) + origin.y * heightratio ) + ( charsize ) + ( ( size.y * heightratio - textheight ) * 0.5f * heightratio ) );
        print( m_progresstext );
    }

    glPopAttrib();
}

void
ui_layer::render_panels() {

    if( m_panels.empty() ) { return; }

    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_TEXTURE_2D );

    float const width = std::min( 4.f / 3.f, static_cast<float>(Global.iWindowWidth) / std::max( 1, Global.iWindowHeight ) ) * Global.iWindowHeight;
    float const height = Global.iWindowHeight / 768.f;

    for( auto const &panel : m_panels ) {

        int lineidx = 0;
        for( auto const &line : panel->text_lines ) {

            ::glColor4fv( &line.color.x );
            ::glRasterPos2f(
                0.5f * ( Global.iWindowWidth - width ) + panel->origin_x * height,
                panel->origin_y * height + 20.f * lineidx );
            print( line.data );
            ++lineidx;
        }
    }

    glPopAttrib();
}

void
ui_layer::render_tooltip() {

    if( m_tooltip.empty() ) { return; }

    glm::dvec2 mousepos;
    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );

    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_TEXTURE_2D );
    ::glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    ::glRasterPos2f( mousepos.x + 20.0f, mousepos.y + 25.0f );
    print( m_tooltip );

    glPopAttrib();
}

void
ui_layer::render_background() {

	if( m_background == 0 ) return;
    // NOTE: we limit/expect the background to come with 4:3 ratio.
    // TODO, TBD: if we expose texture width or ratio from texture object, this limitation could be lifted
    GfxRenderer.Bind_Texture( m_background );
    auto const height { 768.0f };
    auto const &texture = GfxRenderer.Texture( m_background );
    float const width = (
        texture.width() == texture.height() ?
            1024.0f : // legacy mode, square texture displayed as 4:3 image
            texture.width() / ( texture.height() / 768.0f ) );
    quad(
        glm::vec4(
            ( 1024.0f * 0.5f ) - ( width  * 0.5f ),
            (  768.0f * 0.5f ) - ( height * 0.5f ),
            ( 1024.0f * 0.5f ) - ( width  * 0.5f ) + width,
            (  768.0f * 0.5f ) - ( height * 0.5f ) + height ),
        colors::white );
}

void
ui_layer::render_texture() {

    if( m_texture != 0 ) {
        ::glColor4f( 1.f, 1.f, 1.f, 1.f );
        ::glDisable( GL_BLEND );

        GfxRenderer.Bind_Texture( null_handle );
        ::glBindTexture( GL_TEXTURE_2D, m_texture );

        auto const size = 512.f;
        auto const offset = 64.f;

        glBegin( GL_TRIANGLE_STRIP );

        glMultiTexCoord2f( m_textureunit, 0.f, 1.f ); glVertex2f( offset, Global.iWindowHeight - offset - size );
        glMultiTexCoord2f( m_textureunit, 0.f, 0.f ); glVertex2f( offset, Global.iWindowHeight - offset );
        glMultiTexCoord2f( m_textureunit, 1.f, 1.f ); glVertex2f( offset + size, Global.iWindowHeight - offset - size );
        glMultiTexCoord2f( m_textureunit, 1.f, 0.f ); glVertex2f( offset + size, Global.iWindowHeight - offset );

        glEnd();

        ::glBindTexture( GL_TEXTURE_2D, 0 );

        ::glEnable( GL_BLEND );
    }
}

void
ui_layer::print( std::string const &Text )
{
    if( (false == Global.DLFont)
     || (true == Text.empty()) )
        return;
    
    ::glPushAttrib( GL_LIST_BIT );

    ::glListBase( m_fontbase - 32 );
    ::glCallLists( Text.size(), GL_UNSIGNED_BYTE, Text.c_str() );

    ::glPopAttrib();
}

void
ui_layer::quad( glm::vec4 const &Coordinates, glm::vec4 const &Color ) {

    float const screenratio = static_cast<float>( Global.iWindowWidth ) / Global.iWindowHeight;
    float const width =
        ( screenratio >= ( 4.f / 3.f ) ?
            ( 4.f / 3.f ) * Global.iWindowHeight :
            Global.iWindowWidth );
    float const heightratio =
        ( screenratio >= ( 4.f / 3.f ) ?
            Global.iWindowHeight / 768.f :
            Global.iWindowHeight / 768.f * screenratio / ( 4.f / 3.f ) );
    float const height = 768.f * heightratio;

    glColor4fv(glm::value_ptr(Color));

    glBegin( GL_TRIANGLE_STRIP );

    glMultiTexCoord2f( m_textureunit, 0.f, 1.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.x * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.y * heightratio );
    glMultiTexCoord2f( m_textureunit, 0.f, 0.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.x * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.w * heightratio );
    glMultiTexCoord2f( m_textureunit, 1.f, 1.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.z * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.y * heightratio );
    glMultiTexCoord2f( m_textureunit, 1.f, 0.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.z * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.w * heightratio );

    glEnd();
}
