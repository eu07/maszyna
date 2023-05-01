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
#include "imgui/imgui_impl_opengl2.h"
#include "imgui/imgui_impl_opengl3.h"

GLFWwindow *ui_layer::m_window{nullptr};
ImGuiIO *ui_layer::m_imguiio{nullptr};
GLint ui_layer::m_textureunit { GL_TEXTURE0 };
bool ui_layer::m_cursorvisible;
ImFont *ui_layer::font_default{nullptr};
ImFont *ui_layer::font_mono{nullptr};

ui_panel::ui_panel(std::string const &Identifier, bool const Isopen) : m_name(Identifier), is_open(Isopen) {}

void ui_panel::render()
{
    if (false == is_open)
        return;

	int flags = window_flags;
	if (flags == -1)
		flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse |
		        ((size.x > 0) ? ImGuiWindowFlags_NoResize : 0);

    if (size.x > 0)
        ImGui::SetNextWindowSize(ImVec2S(size.x, size.y));
	else if (size_min.x == -1)
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);

    if (size_min.x > 0)
        ImGui::SetNextWindowSizeConstraints(ImVec2S(size_min.x, size_min.y), ImVec2S(size_max.x, size_max.y));

	auto const panelname{(title.empty() ? m_name : title) + "###" + m_name};
	if (ImGui::Begin(panelname.c_str(), &is_open, flags)) {
        render_contents();

		popups.remove_if([](std::unique_ptr<ui::popup> &popup)
		{
			return popup->render();
		});
	}

    ImGui::End();
}

void ui_panel::render_contents()
{
    for (auto const &line : text_lines)
    {
        ImGui::TextColored(ImVec4(line.color.r, line.color.g, line.color.b, line.color.a), line.data.c_str());
    }
}

void ui_panel::register_popup(std::unique_ptr<ui::popup> &&popup)
{
	popups.push_back(std::move(popup));
}

void ui_expandable_panel::render_contents()
{
    ImGui::Checkbox(STR_C("expand"), &is_expanded);
    ui_panel::render_contents();
}

void ui_log_panel::render_contents()
{
	ImGui::PushFont(ui_layer::font_mono);

    for (const std::string &s : log_scrollback)
        ImGui::TextUnformatted(s.c_str());
    if (ImGui::GetScrollY() == ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	ImGui::PopFont();
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
		add_external_panel(&m_logpanel);
    m_logpanel.size = { 700, 400 };
}

void::ui_layer::load_random_background()
{
	std::vector<std::string> images;
	for (auto &f : std::filesystem::directory_iterator("textures/logo"))
		if (f.is_regular_file())
			images.emplace_back(std::filesystem::relative(f.path(), "textures/").string());

	if (!images.empty()) {
		std::string &selected = images[std::lround(LocalRandom(images.size() - 1))];
		set_background(selected);
	}
}

static ImVec4 imvec_lerp(const ImVec4& a, const ImVec4& b, float t)
{
	return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

void ui_layer::imgui_style()
{
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.31f, 0.34f, 0.31f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.21f, 0.28f, 0.17f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.64f, 0.23f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.64f, 0.23f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.28f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01f, 0.01f, 0.01f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.42f, 0.64f, 0.23f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.37f, 0.53f, 0.25f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.42f, 0.64f, 0.23f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.42f, 0.64f, 0.23f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.42f, 0.64f, 0.23f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.37f, 0.54f, 0.19f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.42f, 0.64f, 0.23f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.42f, 0.64f, 0.23f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.42f, 0.64f, 0.23f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.29f, 0.41f, 0.18f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.29f, 0.41f, 0.18f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.42f, 0.64f, 0.23f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.42f, 0.64f, 0.23f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.42f, 0.64f, 0.23f, 0.95f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);

	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.42f, 0.64f, 0.23f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_Tab] = imvec_lerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive] = imvec_lerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
	colors[ImGuiCol_TabUnfocused] = imvec_lerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabUnfocusedActive] = imvec_lerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);

    ImGui::GetStyle().ScaleAllSizes(Global.ui_scale);
}

bool ui_layer::init(GLFWwindow *Window)
{
    m_window = Window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_imguiio = &ImGui::GetIO();

	m_imguiio->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    // m_imguiio->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // m_imguiio->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x2070, 0x2079, // superscript
        0x2500, 0x256C, // box drawings
        0,
    };

	if (FileExists("fonts/dejavusans.ttf"))
        font_default = m_imguiio->Fonts->AddFontFromFileTTF("fonts/dejavusans.ttf", Global.ui_fontsize, nullptr, &ranges[0]);
	if (FileExists("fonts/dejavusansmono.ttf"))
        font_mono = m_imguiio->Fonts->AddFontFromFileTTF("fonts/dejavusansmono.ttf", Global.ui_fontsize, nullptr, &ranges[0]);

	if (!font_default && !font_mono)
		font_default = font_mono = m_imguiio->Fonts->AddFontDefault();
	else if (!font_default)
		font_default = font_mono;
	else if (!font_mono)
		font_mono = font_default;

	imgui_style();

    ImGui_ImplGlfw_InitForOpenGL(m_window, false);
	if (Global.LegacyRenderer) {
		crashreport_add_info("imgui_ver", "gl2");
		ImGui_ImplOpenGL2_Init();
	} else {
	    crashreport_add_info("imgui_ver", "gl3");
	    if (Global.gfx_usegles)
	        ImGui_ImplOpenGL3_Init("#version 300 es\nprecision highp float;");
	    else
	        ImGui_ImplOpenGL3_Init("#version 330 core");
	}

    return true;
}

void ui_layer::shutdown()
{
    ImGui::EndFrame();

    if (Global.LegacyRenderer)
        ImGui_ImplOpenGL2_Shutdown();
    else
        ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool ui_layer::on_key(int const Key, int const Action)
{
    if (Action == GLFW_PRESS)
    {
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
                Application.queue_quit(false);
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
        panel->update();

	for (auto it = m_ownedpanels.rbegin(); it != m_ownedpanels.rend(); it++) {
		(*it)->update();
		if (!(*it)->is_open)
			m_ownedpanels.erase(std::next(it).base());
	}
}

void ui_layer::render()
{
    render_background();
    render_progress();
    render_panels();
    render_tooltip();
    render_menu();
    render_quit_widget();

    // template method implementation
    render_();

	gl::buffer::unbind(gl::buffer::ARRAY_BUFFER);
    render_internal();
}

void ui_layer::render_internal()
{
    ImGui::Render();

    if (Global.LegacyRenderer)
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    else
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ui_layer::begin_ui_frame()
{
    begin_ui_frame_internal();
}

void ui_layer::begin_ui_frame_internal()
{
	if (Global.LegacyRenderer)
		ImGui_ImplOpenGL2_NewFrame();
	else
		ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ui_layer::render_quit_widget()
{
    if (!m_quit_active)
        return;

    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin(STR_C("Quit"), &m_quit_active, ImGuiWindowFlags_NoResize);
    ImGui::TextUnformatted(STR_C("Quit simulation?"));
    if (ImGui::Button(STR_C("Yes")))
        Application.queue_quit(false);

    ImGui::SameLine();
    if (ImGui::Button(STR_C("No")))
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
        m_background = GfxRenderer->Fetch_Texture(Filename);
    }
    else
    {
        m_background = null_handle;
    }
    if (m_background != null_handle)
    {
        auto const &texture = GfxRenderer->Texture(m_background);
    }
}

void ui_layer::clear_panels()
{
    m_panels.clear();
	m_ownedpanels.clear();
}

void ui_layer::add_owned_panel(ui_panel *Panel)
{
	for (auto &panel : m_ownedpanels)
		if (panel->name() == Panel->name()) {
			delete Panel;
			return;
		}

	Panel->is_open = true;
	m_ownedpanels.emplace_back( Panel );
}

void ui_layer::render_progress()
{
    if ((m_progress == 0.0f) && (m_subtaskprogress == 0.0f))
        return;

    ImGui::SetNextWindowPos(ImVec2(50, 50));
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin(STR_C("Loading"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
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
		panel->render();
	for (auto &panel : m_ownedpanels)
		panel->render();

	if (m_imgui_demo)
		ImGui::ShowDemoWindow(&m_imgui_demo);
}

void ui_layer::render_tooltip()
{
	if (!m_cursorvisible || m_imguiio->WantCaptureMouse || m_tooltip.empty())
        return;

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(m_tooltip.c_str());
    ImGui::EndTooltip();
}

void ui_layer::render_menu_contents()
{
	if (ImGui::BeginMenu(STR_C("General")))
    {
        ImGui::MenuItem(STR_C("Debug mode"), nullptr, &DebugModeFlag);
        ImGui::MenuItem(STR_C("Quit"), "F10", &m_quit_active);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(STR_C("Tools")))
    {
        static bool log = Global.iWriteLogEnabled & 1;

        ImGui::MenuItem(STR_C("Logging to log.txt"), nullptr, &log);
        if (log)
            Global.iWriteLogEnabled |= 1;
        else
            Global.iWriteLogEnabled &= ~1;

        if (ImGui::MenuItem(STR_C("Screenshot"), "PrtScr"))
            Application.queue_screenshot();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(STR_C("Windows")))
    {
        ImGui::MenuItem(STR_C("Log"), "F9", &m_logpanel.is_open);
		if (DebugModeFlag) {
			ImGui::MenuItem(STR_C("ImGui Demo"), nullptr, &m_imgui_demo);
            bool ret = ImGui::MenuItem(STR_C("Headlight config"), nullptr, GfxRenderer->Debug_Ui_State(std::nullopt));

            GfxRenderer->Debug_Ui_State(ret);
		}
        ImGui::EndMenu();
    }
}

void ui_layer::render_menu()
{
    glm::dvec2 mousepos = Global.cursor_pos;

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

    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    ImVec2 image_size;
    ImVec2 start_position;
    ImVec2 end_position;

    opengl_texture &tex = GfxRenderer->Texture(m_background);
    tex.create();

    // Get the scaling factor based on the image aspect ratio vs. the display aspect ratio.
    float scale_factor = (tex.width() / tex.height()) > (display_size.x / display_size.y)
        ? display_size.y / tex.height()
        : display_size.x / tex.width();
    
    // Resize the image to fill the display. This will zoom in on the image on ultrawide monitors.
    image_size = ImVec2((int)(tex.width() * scale_factor), (int)(tex.height() * scale_factor));
    // Center the image on the display.
    start_position = ImVec2((display_size.x - image_size.x) / 2, (display_size.y - image_size.y) / 2);
    end_position = ImVec2(image_size.x + start_position.x, image_size.y + start_position.y);
    // The image is flipped upside-down, we'll use the UV parameters to draw it from bottom up to un-flip it.
    ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)tex.id, start_position, end_position, ImVec2(0, 1), ImVec2(1, 0));
}
