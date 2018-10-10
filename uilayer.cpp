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
#include "Logs.h"
#include "Timer.h"
#include "simulation.h"
#include "translation.h"
#include "application.h"

#include "imgui/imgui_impl_glfw.h"
#ifdef EU07_USEIMGUIIMPLOPENGL2
#include "imgui/imgui_impl_opengl2.h"
#else
#include "imgui/imgui_impl_opengl3.h"
#endif

GLFWwindow *ui_layer::m_window{nullptr};
ImGuiIO *ui_layer::m_imguiio{nullptr};
std::unique_ptr<map> ui_layer::m_map;
bool ui_layer::m_cursorvisible;

#define LOC_STR(x) locale::strings[locale::string::x].c_str()

ui_panel::ui_panel(std::string const &Identifier, bool const Isopen) : name(Identifier), is_open(Isopen) {}

void ui_panel::render()
{
    if (false == is_open)
        return;

    auto flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse | (size.x > 0 ? ImGuiWindowFlags_NoResize : 0);

    if (size.x > 0)
        ImGui::SetNextWindowSize(ImVec2(size.x, size.y));
    else
        ImGui::SetNextWindowSize(ImVec2(0, 0));

    if (size_min.x > 0)
        ImGui::SetNextWindowSizeConstraints(ImVec2(size_min.x, size_min.y), ImVec2(size_max.x, size_max.y));

    auto const panelname{(title.empty() ? name : title) + "###" + name};
    if (true == ImGui::Begin(panelname.c_str(), &is_open, flags))
        render_contents();
    ImGui::End();
}

void ui_panel::render_contents()
{
    for (auto const &line : text_lines)
    {
        ImGui::TextColored(ImVec4(line.color.r, line.color.g, line.color.b, line.color.a), line.data.c_str());
    }
}

void ui_expandable_panel::render_contents()
{
    ImGui::Checkbox(LOC_STR(ui_expand), &is_expanded);
    ui_panel::render_contents();
}

void ui_log_panel::render_contents()
{
    for (const std::string &s : log_scrollback)
        ImGui::TextUnformatted(s.c_str());
    if (ImGui::GetScrollY() == ImGui::GetScrollMaxY())
        ImGui::SetScrollHere(1.0f);
}

ui_layer::~ui_layer() {}

bool ui_layer::key_callback(int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(m_window, key, scancode, action, mods);
    return m_imguiio->WantCaptureKeyboard;
}

bool ui_layer::char_callback(unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(m_window, c);
    return m_imguiio->WantCaptureKeyboard;
}

bool ui_layer::scroll_callback(double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(m_window, xoffset, yoffset);
    return m_imguiio->WantCaptureMouse;
}

bool ui_layer::mouse_button_callback(int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(m_window, button, action, mods);
    return m_imguiio->WantCaptureMouse;
}

ui_layer::ui_layer()
{
    if (Global.loading_log)
        push_back(&m_logpanel);
    m_logpanel.size = { 700, 400 };
}

bool ui_layer::init(GLFWwindow *Window)
{
    m_window = Window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_imguiio = &ImGui::GetIO();

    // m_imguiio->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // m_imguiio->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0,
    };

    m_imguiio->Fonts->AddFontFromFileTTF("DejaVuSansMono.ttf", 13.0f, nullptr, &ranges[0]);

    if (Global.map_enabled)
        m_map = std::make_unique<map>();

    ImGui::StyleColorsClassic();
    ImGui_ImplGlfw_InitForOpenGL(m_window);
#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplOpenGL2_NewFrame();
#else
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui_ImplOpenGL3_NewFrame();
#endif

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void ui_layer::shutdown()
{
    ImGui::EndFrame();

#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_Shutdown();
#else
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool ui_layer::on_key(int const Key, int const Action)
{
    if (Action == GLFW_PRESS)
    {
        if (Key == GLFW_KEY_TAB) {
            if (m_map)
                m_map->toggle_window();

            return true;
        }

        if (Key == GLFW_KEY_PRINT_SCREEN) {
            Application.queue_screenshot();
            return true;
        }

        if (Key == GLFW_KEY_F9) {
            m_logpanel.is_open = !m_logpanel.is_open;
            return true;
        }

        if (Key == GLFW_KEY_F10) {
            m_quit_active = !m_quit_active;
            return true;
        }

        if (m_quit_active)
        {
            if (Key == GLFW_KEY_Y) {
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
                return true;
            } else if (Key == GLFW_KEY_N) {
                m_quit_active = false;
                return true;
            }
        }
    }

    return false;
}

bool ui_layer::on_cursor_pos(double const Horizontal, double const Vertical)
{
    return false;
}

bool ui_layer::on_mouse_button(int const Button, int const Action)
{
    return false;
}

void ui_layer::update()
{
    for (auto *panel : m_panels)
    {
        panel->update();
    }
}

void ui_layer::render()
{
    Timer::subsystem.gfx_gui.start();

    render_background();
    render_progress();

    render_panels();
    render_tooltip();
    render_menu();
    render_quit_widget();

    if (m_map && simulation::is_ready)
        m_map->render(simulation::Region);

    // template method implementation
    render_();

    ImGui::Render();
    Timer::subsystem.gfx_gui.stop();

#ifdef EU07_USEIMGUIIMPLOPENGL2
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    ImGui_ImplOpenGL2_NewFrame();
#else
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui_ImplOpenGL3_NewFrame();
#endif

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ui_layer::render_quit_widget()
{
    if (!m_quit_active)
        return;

    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin(LOC_STR(ui_quit), &m_quit_active, ImGuiWindowFlags_NoResize);
    ImGui::TextUnformatted(LOC_STR(ui_quit_simulation_q));
    if (ImGui::Button(LOC_STR(ui_yes)))
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    ImGui::SameLine();
    if (ImGui::Button(LOC_STR(ui_no)))
        m_quit_active = false;
    ImGui::End();
}

void ui_layer::set_cursor(int const Mode)
{
    glfwSetInputMode(m_window, GLFW_CURSOR, Mode);
    m_cursorvisible = (Mode != GLFW_CURSOR_DISABLED);
}

void ui_layer::set_progress(float const Progress, float const Subtaskprogress)
{
    m_progress = Progress * 0.01f;
    m_subtaskprogress = Subtaskprogress * 0.01f;
}

void ui_layer::set_background(std::string const &Filename)
{
    if (false == Filename.empty())
    {
        m_background = GfxRenderer.Fetch_Texture(Filename);
    }
    else
    {
        m_background = null_handle;
    }
    if (m_background != null_handle)
    {
        auto const &texture = GfxRenderer.Texture(m_background);
    }
}

void ui_layer::clear_panels()
{
    m_panels.clear();
}

void ui_layer::render_progress()
{
    if ((m_progress == 0.0f) && (m_subtaskprogress == 0.0f))
        return;

    ImGui::SetNextWindowPos(ImVec2(50, 50));
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin(LOC_STR(ui_loading), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    if (!m_progresstext.empty())
        ImGui::ProgressBar(m_progress, ImVec2(300, 0), m_progresstext.c_str());
    else
        ImGui::ProgressBar(m_progress, ImVec2(300, 0));
    ImGui::ProgressBar(m_subtaskprogress, ImVec2(300, 0));
    ImGui::End();
}

void ui_layer::render_panels()
{
    for (auto *panel : m_panels)
    {
        panel->render();
    }
}

void ui_layer::render_tooltip()
{
    if (!m_cursorvisible || m_tooltip.empty())
        return;

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(m_tooltip.c_str());
    ImGui::EndTooltip();
}

void ui_layer::render_menu_contents()
{
    if (ImGui::BeginMenu(LOC_STR(ui_general)))
    {
        ImGui::MenuItem(LOC_STR(ui_debug_mode), nullptr, &DebugModeFlag);
        ImGui::MenuItem(LOC_STR(ui_quit), "F10", &m_quit_active);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(LOC_STR(ui_tools)))
    {
        static bool log = Global.iWriteLogEnabled & 1;

        ImGui::MenuItem(LOC_STR(ui_logging_to_log), nullptr, &log);
        if (log)
            Global.iWriteLogEnabled |= 1;
        else
            Global.iWriteLogEnabled &= ~1;

        if (ImGui::MenuItem(LOC_STR(ui_screenshot), "PrtScr"))
            Application.queue_screenshot();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(LOC_STR(ui_windows)))
    {
        ImGui::MenuItem(LOC_STR(ui_log), "F9", &m_logpanel.is_open);
        if (Global.map_enabled && m_map)
            ImGui::MenuItem(LOC_STR(ui_map), "Tab", &m_map->map_opened);
        ImGui::EndMenu();
    }
}

void ui_layer::render_menu()
{
    glm::dvec2 mousepos = Application.get_cursor_pos();

    if (!((Global.ControlPicking && mousepos.y < 50.0f) || m_imguiio->WantCaptureMouse) || m_progress != 0.0f)
        return;

    if (ImGui::BeginMainMenuBar())
    {
        render_menu_contents();
        ImGui::EndMainMenuBar();
    }
}

void ui_layer::render_background()
{
    if (m_background == 0)
        return;

    ImVec2 size = ImGui::GetIO().DisplaySize;
    opengl_texture &tex = GfxRenderer.Texture(m_background);
    tex.create();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("Logo", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Image(reinterpret_cast<void *>(tex.id), size, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}
