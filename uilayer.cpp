/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "uilayer.h"

#include "globals.h"
#include "translation.h"
#include "simulation.h"
#include "simulationtime.h"
#include "event.h"
#include "camera.h"
#include "mtable.h"
#include "train.h"
#include "driver.h"
#include "sceneeditor.h"
#include "animmodel.h"
#include "dynobj.h"
#include "model3d.h"
#include "renderer.h"
#include "utilities.h"
#include "logs.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window ); //m7todo: potrzebne do directsound
}

GLFWwindow * ui_layer::m_window { nullptr };
ImGuiIO *ui_layer::m_imguiio { nullptr };
GLint ui_layer::m_textureunit { GL_TEXTURE0 };
bool ui_layer::m_cursorvisible { true };


ui_panel::ui_panel( std::string const Name, bool const Isopen )
    : name( Name ), is_open( Isopen )
{}

void
ui_panel::render() {

    if( false == is_open ) { return; }
    if( true  == text_lines.empty() ) { return; }

    auto flags =
        ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoCollapse
        | ( size.x > 0 ? ImGuiWindowFlags_NoResize : 0 );

    if( size.x > 0 ) {
        ImGui::SetNextWindowSize( ImVec2( size.x, size.y ) );
    }
    if( size_min.x > 0 ) {
        ImGui::SetNextWindowSizeConstraints( ImVec2( size_min.x, size_min.y ), ImVec2( size_max.x, size_max.y ) );
    }
    if( true == ImGui::Begin( name.c_str(), &is_open, flags ) ) {
        for( auto const &line : text_lines ) {
            ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
        }
    }
    ImGui::End();
}

ui_layer::~ui_layer() {}

bool
ui_layer::init( GLFWwindow *Window ) {

    m_window = Window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_imguiio = &ImGui::GetIO();
//    m_imguiio->Fonts->AddFontFromFileTTF( "c:/windows/fonts/lucon.ttf", 13.0f );

    ImGui_ImplGlfw_InitForOpenGL( m_window, false );
    ImGui_ImplOpenGL2_Init();

    init_colors();

    return true;
}

void
ui_layer::init_colors() {

    // configure ui colours
    auto *style = &ImGui::GetStyle();
    auto *colors = style->Colors;
    auto const background { ImVec4( 38.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, 0.65f ) };
    auto const accent     { ImVec4( 44.0f / 255.0f, 88.0f / 255.0f, 72.0f / 255.0f, 0.75f ) };
    auto const itembase { ImVec4( accent.x, accent.y, accent.z, 0.35f ) };
    auto const itemhover { ImVec4( accent.x, accent.y, accent.z, 0.65f ) };
    auto const itemactive { ImVec4( accent.x, accent.y, accent.z, 0.95f ) };

    colors[ ImGuiCol_WindowBg ] = background;
    colors[ ImGuiCol_PopupBg ] = background;
    colors[ ImGuiCol_FrameBg ] = itembase;
    colors[ ImGuiCol_FrameBgHovered ] = itemhover;
    colors[ ImGuiCol_FrameBgActive ] = itemactive;
    colors[ ImGuiCol_TitleBg ] = background;
    colors[ ImGuiCol_TitleBgActive ] = background;
    colors[ ImGuiCol_TitleBgCollapsed ] = background;
    colors[ ImGuiCol_CheckMark ] = colors[ ImGuiCol_Text ];
    colors[ ImGuiCol_Button ] = itembase;
    colors[ ImGuiCol_ButtonHovered ] = itemhover;
    colors[ ImGuiCol_ButtonActive ] = itemactive;
    colors[ ImGuiCol_Header ] = itembase;
    colors[ ImGuiCol_HeaderHovered ] = itemhover;
    colors[ ImGuiCol_HeaderActive ] = itemactive;
    colors[ ImGuiCol_ResizeGrip ] = itembase;
    colors[ ImGuiCol_ResizeGripHovered ] = itemhover;
    colors[ ImGuiCol_ResizeGripActive ] = itemactive;
}

void
ui_layer::shutdown() {

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool
ui_layer::on_key( int const Key, int const Action ) {

    return false;
}

void
ui_layer::update() {

    for( auto *panel : m_panels ) {
        panel->update();
    }
}

void
ui_layer::render() {

    // legacy ui code
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0, std::max( 1, Global.iWindowWidth ), std::max( 1, Global.iWindowHeight ), 0, -1, 1 );

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT ); // blendfunc included since 3rd party gui doesn't play nice
	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_ALPHA_TEST );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );

    ::glColor4fv( glm::value_ptr( colors::white ) );

    // render code here
    render_background();
    render_texture();

    glDisable( GL_TEXTURE_2D );
    glDisable( GL_TEXTURE_CUBE_MAP );

    render_progress();

    glDisable( GL_BLEND );

    glPopAttrib();

    // imgui ui code
    ::glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

    ::glClientActiveTexture( m_textureunit );
    ::glBindBuffer( GL_ARRAY_BUFFER, 0 );

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    render_panels();
    render_tooltip();
    // template method implementation
    render_();

    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData( ImGui::GetDrawData() );

    ::glPopClientAttrib();
}

void
ui_layer::set_cursor( int const Mode ) {

    glfwSetInputMode( m_window, GLFW_CURSOR, Mode );
    m_cursorvisible = ( Mode != GLFW_CURSOR_DISABLED );
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
    else {
        m_progressbottom = true;
    }
}

void
ui_layer::render_progress() {

	if( (m_progress == 0.0f) && (m_subtaskprogress == 0.0f) ) return;

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
    }
}

void
ui_layer::render_panels() {

    for( auto *panel : m_panels ) {
        panel->render();
    }
}

void
ui_layer::render_tooltip() {

    if( m_tooltip.empty() ) { return; }
    if( false == m_cursorvisible ) { return; }

    ImGui::SetTooltip( m_tooltip.c_str() );
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
        ::glColor4fv( glm::value_ptr( colors::white ) );

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
    }
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
