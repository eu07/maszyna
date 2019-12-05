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

#include "Globals.h"
#include "renderer.h"

#include "imgui_impl_glfw.h"
#ifdef EU07_USEIMGUIIMPLOPENGL2
#include "imgui_impl_opengl2.h"
#else
#include "imgui_impl_opengl3.h"
#endif

extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window ); //m7todo: potrzebne do directsound
}

GLFWwindow * ui_layer::m_window { nullptr };
ImGuiIO *ui_layer::m_imguiio { nullptr };
GLint ui_layer::m_textureunit { GL_TEXTURE0 };
bool ui_layer::m_cursorvisible { true };


ui_panel::ui_panel( std::string const &Identifier, bool const Isopen )
    : name( Identifier ), is_open( Isopen )
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
    auto const panelname { (
        title.empty() ?
            name :
            title )
        + "###" + name };
    if( true == ImGui::Begin( panelname.c_str(), &is_open, flags ) ) {
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

    static ImWchar const glyphranges[] = {
        0x0020, 0x00FF, // ascii + extension
        0x2070, 0x2079, // superscript
        0x2500, 0x256C, // box drawings
        0,
    };
    m_imguiio = &ImGui::GetIO();
    m_imguiio->Fonts->AddFontFromFileTTF( "fonts/dejavusansmono.ttf", 13.0f, nullptr, &glyphranges[ 0 ] );

    ImGui_ImplGlfw_InitForOpenGL( m_window, false );
#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_Init();
#else
//    ImGui_ImplOpenGL3_Init( "#version 140" );
    if( Global.GfxRenderer == "default" ) {
        // opengl 3.3 render path
        if( Global.gfx_usegles ) {
            ImGui_ImplOpenGL3_Init( "#version 300 es" );
        }
        else {
            ImGui_ImplOpenGL3_Init( "#version 330 core" );
        }
    }
    else {
        // legacy render path
        ImGui_ImplOpenGL3_Init();
    }
#endif

    init_colors();

    return true;
}

void
ui_layer::init_colors() {

    // configure ui colours
    auto *style = &ImGui::GetStyle();
    auto *colors = style->Colors;
    auto const background { ImVec4( 38.0f / 255.0f, 38.0f / 255.0f, 38.0f / 255.0f, Global.UIBgOpacity ) };
    auto const accent     { ImVec4( 44.0f / 255.0f, 88.0f / 255.0f, 72.0f / 255.0f, 0.75f ) };
    auto const itembase { ImVec4( accent.x, accent.y, accent.z, 0.35f ) };
    auto const itemhover { ImVec4( accent.x, accent.y, accent.z, 0.65f ) };
    auto const itemactive { ImVec4( accent.x, accent.y, accent.z, 0.95f ) };
    auto const modalbackground { ImVec4( accent.x, accent.y, accent.z, 0.95f ) };

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
    colors[ ImGuiCol_ModalWindowDimBg ] = modalbackground;
}

void
ui_layer::shutdown() {

#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_Shutdown();
#else
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool
ui_layer::on_key( int const Key, int const Scancode, int const Action, int const Mods ) {

    ImGui_ImplGlfw_KeyCallback( m_window, Key, Scancode, Action, Mods );
    if( m_imguiio->WantTextInput ) { return true; }

    return on_key_( Key, Scancode, Action, Mods );
}

bool
ui_layer::on_cursor_pos( double const Horizontal, double const Vertical ) {

    return on_cursor_pos_( Horizontal, Vertical );
}

bool
ui_layer::on_mouse_button( int const Button, int const Action, int const Mods ) {

    ImGui_ImplGlfw_MouseButtonCallback( m_window, Button, Action, Mods );
    if( m_imguiio->WantCaptureMouse ) { return true; }

    return on_mouse_button_( Button, Action, Mods );
}

// potentially processes provided mouse scroll event. returns: true if the input was processed, false otherwise
bool
ui_layer::on_scroll( double const Xoffset, double const Yoffset ) {

    ImGui_ImplGlfw_ScrollCallback( m_window, Xoffset, Yoffset );
    if( m_imguiio->WantCaptureMouse ) { return true; }

    return on_scroll_( Xoffset, Yoffset );
}

void
ui_layer::update() {

    for( auto *panel : m_panels ) {
        panel->update();
    }
}

void
ui_layer::render() {
    // imgui ui code
#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_NewFrame();
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    render_background();
    render_progress();
    render_panels();
    render_tooltip();
    // template method implementation
    render_();

    ImGui::Render();
#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_RenderDrawData( ImGui::GetDrawData() );
#else
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
#endif
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
        m_background = GfxRenderer->Fetch_Texture( Filename, true, GL_RGBA );
    }
    else {
        m_background = null_handle;
    }

    if( m_background != null_handle ) {
        auto const &texture = GfxRenderer->Texture( m_background );
        m_progressbottom = ( texture.width() != texture.height() );
    }
    else {
        m_progressbottom = true;
    }
}

glm::vec4
background_placement( texture_handle const Background ) {

    if( Background == null_handle ) {
        return glm::vec4( 0.f, 0.f, Global.iWindowWidth, Global.iWindowHeight );
    }

    auto const background { GfxRenderer->Texture( Background ) };
    // determine background placement, taking into account screen ratio may be different from background image ratio
    auto const height { 768.0f };
    auto const width = (
        background.width() == background.height() ?
            1024.0f : // legacy mode, square texture displayed as 4:3 image
            background.width() / ( background.height() / 768.0f ) );
    // background coordinates on virtual 1024x768 screen
    auto const coordinates {
        glm::vec4(
            ( 1024.0f * 0.5f ) - ( width  * 0.5f ),
            ( 768.0f * 0.5f ) - ( height * 0.5f ),
            ( 1024.0f * 0.5f ) - ( width  * 0.5f ) + width,
            ( 768.0f * 0.5f ) - ( height * 0.5f ) + height ) };
    // convert to current screen coordinates
    auto const screenratio { static_cast<float>( Global.iWindowWidth ) / Global.iWindowHeight };
    auto  const screenwidth {
        ( screenratio >= ( 4.f / 3.f ) ?
            ( 4.f / 3.f ) * Global.iWindowHeight :
            Global.iWindowWidth ) };
    auto const heightratio {
        ( screenratio >= ( 4.f / 3.f ) ?
            Global.iWindowHeight / 768.f :
            Global.iWindowHeight / 768.f * screenratio / ( 4.f / 3.f ) ) };
    auto const screenheight = 768.f * heightratio;

    return
        glm::vec4{
            0.5f * ( Global.iWindowWidth  - screenwidth )  + coordinates.x * heightratio,
            0.5f * ( Global.iWindowHeight - screenheight ) + coordinates.y * heightratio,
            0.5f * ( Global.iWindowWidth  - screenwidth )  + coordinates.z * heightratio,
            0.5f * ( Global.iWindowHeight - screenheight ) + coordinates.w * heightratio };
}

void
ui_layer::render_progress() {

    if ((m_progress == 0.0f) && (m_subtaskprogress == 0.0f))
        return;

    auto const area{ glm::clamp( background_placement( m_background ), glm::vec4(0.f), glm::vec4(Global.iWindowWidth, Global.iWindowHeight, Global.iWindowWidth, Global.iWindowHeight) ) };
    auto const areasize{ ImVec2( area.z - area.x, area.w - area.y ) };

    ImGui::SetNextWindowPos(ImVec2( area.x + 50, Global.iWindowHeight - 50));
    ImGui::SetNextWindowSize(ImVec2(areasize.x - 100, 0));
    ImGui::Begin(
        "Loading", nullptr,
        ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoCollapse );

    const ImU32 col = ImGui::ColorConvertFloat4ToU32(  ImVec4( 8.0f / 255.0f, 160.0f / 255.0f, 8.0f / 255.0f, 1.0f ) ); // ImGui::GetColorU32( ImGuiCol_ButtonHovered );
    const ImU32 bg = ImGui::ColorConvertFloat4ToU32( ImVec4( 8.0f / 255.0f, 160.0f / 255.0f, 8.0f / 255.0f, 0.35f ) ); // ImGui::GetColorU32( ImGuiCol_Button );
/*
    ImGui::Spinner( "##spinner", 8, 4, col );
    ImGui::SetCursorPos( ImVec2( 40, 18 ) );
*/
    ImGui::SetCursorPos( ImVec2( 15, 15 ) );
    ImGui::BufferingBar( "##buffer_bar", m_progress, ImVec2( areasize.x - 125, 4 ), bg, col );

    ImGui::End();
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

    if( m_background == null_handle ) { return; }

    opengl_texture &background = GfxRenderer->Texture(m_background);
    background.create();
    // determine background placement, taking into account screen ratio may be different from background image ratio
    auto const placement{ background_placement( m_background ) };
    //ImVec2 size = ImGui::GetIO().DisplaySize;
    auto const size{ ImVec2( placement.z - placement.x, placement.w - placement.y ) };
    // ready, set, draw
    ImGui::SetNextWindowPos(ImVec2(placement.x, placement.y));
    ImGui::SetNextWindowSize(size);
    ImGui::Begin(
        "Logo", nullptr,
        ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus );
    ImGui::Image(reinterpret_cast<void *>(background.id), size, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}

void
ui_layer::render_texture() {

    if( m_texture != 0 ) {
        ::glColor4fv( glm::value_ptr( colors::white ) );

        GfxRenderer->Bind_Texture( null_handle );
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
