/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "application.h"
#include "drivermode.h"
#include "editormode.h"
#include "scenarioloadermode.h"
#include "launcher/launchermode.h"

#include "Globals.h"
#include "simulation.h"
#include "simulationsounds.h"
#include "Train.h"
#include "dictionary.h"
#include "sceneeditor.h"
#include "renderer.h"
#include "uilayer.h"
#include "Logs.h"
#include "screenshot.h"
#include "translation.h"
#include "Train.h"
#include "Timer.h"
#include "dictionary.h"
#include "version_info.h"

#ifdef _WIN32
#pragma comment (lib, "dsound.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "version.lib")
#endif

#ifdef __unix__
#include <unistd.h>
#include <sys/stat.h>
#endif

eu07_application Application;
screenshot_manager screenshot_man;

ui_layer uilayerstaticinitializer;

#ifdef _WIN32
extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window );
}

LRESULT APIENTRY WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern HWND Hwnd;
extern WNDPROC BaseWindowProc;
#endif

// user input callbacks

void focus_callback( GLFWwindow *window, int focus ) {
	Application.on_focus_change(focus != 0);
}

void framebuffer_resize_callback( GLFWwindow *, int w, int h ) {
    Global.fb_size = glm::ivec2(w, h);
}

void window_resize_callback( GLFWwindow *, int w, int h ) {
    Global.window_size = glm::ivec2(w, h);
}

void cursor_pos_callback( GLFWwindow *window, double x, double y ) {
    Global.cursor_pos = glm::ivec2(x, y);
    Application.on_cursor_pos( x, y );
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods )
{
    Application.on_mouse_button( button, action, mods );
}

void scroll_callback( GLFWwindow* window, double xoffset, double yoffset ) {
    Application.on_scroll( xoffset, yoffset );
}

void key_callback( GLFWwindow *window, int key, int scancode, int action, int mods ) {

    Application.on_key( key, scancode, action, mods );
}

void char_callback(GLFWwindow *window, unsigned int c)
{
    Application.on_char(c);
}

// public:

void eu07_application::queue_screenshot()
{
    m_screenshot_queued = true;
}

int eu07_application::run_crashgui()
{
    bool autoup = false;

    while (!glfwWindowShouldClose(m_windows.front()))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ui_layer::begin_ui_frame_internal();

        bool y, n;

        if (Global.asLang == "pl") {
            ImGui::Begin(u8"Raportowanie błędów", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
            ImGui::TextUnformatted(u8"Podczas ostatniego uruchomienia symulatora wystąpił błąd.\nWysłać raport o błędzie do deweloperów?\n");
            ImGui::TextUnformatted((u8"Usługa udostępniana przez " + crashreport_get_provider() + "\n").c_str());
            y = ImGui::Button(u8"Tak", ImVec2S(60, 0)); ImGui::SameLine();
            ImGui::Checkbox(u8"W przyszłości przesyłaj raporty o błędach automatycznie", &autoup);

            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(u8"W celu wyłączenia tej funkcji będzie trzeba skasować plik crashdumps/autoupload_enabled.conf");
                ImGui::EndTooltip();
            }

            ImGui::NewLine();
            n = ImGui::Button(u8"Nie", ImVec2S(60, 0));
            ImGui::End();
        } else {
            ImGui::Begin("Crash reporting", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
            ImGui::TextUnformatted("Crash occurred during last launch of the simulator.\nSend crash report to developers?\n");
            ImGui::TextUnformatted(("Service provided by " + crashreport_get_provider() + "\n").c_str());
            y = ImGui::Button("Yes", ImVec2S(60, 0)); ImGui::SameLine();
            ImGui::Checkbox("In future send crash reports automatically", &autoup);

            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("To disable this feature remove file crashdumps/autoupload_enabled.conf");
                ImGui::EndTooltip();
            }

            ImGui::NewLine();
            n = ImGui::Button("No", ImVec2S(60, 0));
            ImGui::End();
        }

        ui_layer::render_internal();
        glfwSwapBuffers(m_windows.front());

        if (y) {
            crashreport_upload_accept();
            if (autoup)
                crashreport_set_autoupload();
            return 0;
        }

        if (n) {
            crashreport_upload_reject();
            return 0;
        }
    }
    return -1;
}

int
eu07_application::init( int Argc, char *Argv[] ) {

    int result { 0 };

    init_debug();
    init_files();
    if( ( result = init_settings( Argc, Argv ) ) != 0 ) {
        return result;
    }

	WriteLog( "Starting MaSzyna rail vehicle simulator (release: " + Global.asVersion + ")" );
	WriteLog( "For online documentation and additional files refer to: http://eu07.pl" );
	WriteLog( "Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx, OLO_EU, Bart, Quark-t, "
        "ShaXbee, Oli_EU, youBy, KURS90, Ra, hunter, szociu, Stele, Q, firleju and others" );

    if (!crashreport_get_provider().empty())
        WriteLog("Crashdump analysis provided by " + crashreport_get_provider() + "\n");

    {
        WriteLog( "// settings" );
        std::stringstream settingspipe;
        Global.export_as_text( settingspipe );
        WriteLog( settingspipe.str() );
    }

    WriteLog( "// startup" );

    if( ( result = init_glfw() ) != 0 ) {
        return result;
    }
    if( ( result = init_ogl() ) != 0 ) {
        return result;
    }
    if( ( result = init_ui() ) != 0 ) {
        return result;
    }
    if (crashreport_is_pending()) { // run crashgui as early as possible
        if ( ( result = run_crashgui() ) != 0 )
            return result;
    }
    if( ( result = init_locale() ) != 0 ) {
        return result;
    }
    if( ( result = init_gfx() ) != 0 ) {
        return result;
    }
    if( ( result = init_audio() ) != 0 ) {
        return result;
    }
    if( ( result = init_data() ) != 0 ) {
        return result;
    }
    crashreport_add_info("python_enabled", Global.python_enabled ? "yes" : "no");
    if( Global.python_enabled ) {
        m_taskqueue.init();
    }
    if( ( result = init_modes() ) != 0 ) {
        return result;
    }

	if (!init_network())
		return -1;

    return result;
}

double eu07_application::generate_sync() {
	if (Timer::GetDeltaTime() == 0.0)
		return 0.0;
	double sync = 0.0;
	for (const TDynamicObject* vehicle : simulation::Vehicles.sequence()) {
        auto const pos { vehicle->GetPosition() };
		 sync += pos.x + pos.y + pos.z;
	}
	sync += Random(1.0, 100.0);
	return sync;
}

void eu07_application::queue_quit(bool direct) {
    if (direct || !m_modes[m_modestack.top()]->is_command_processor()) {
        glfwSetWindowShouldClose(m_windows[0], GLFW_TRUE);
        return;
    }

    command_relay relay;
    relay.post(user_command::quitsimulation, 0.0, 0.0, GLFW_PRESS, 0);
}

bool
eu07_application::is_server() const {

    return ( m_network && m_network->servers );
}

bool
eu07_application::is_client() const {

    return ( m_network && m_network->client );
}

int
eu07_application::run() {
    auto frame{ 0 };
    // main application loop
    while (!glfwWindowShouldClose( m_windows.front() ) && !m_modestack.empty())
    {
        Timer::subsystem.mainloop_total.start();
        glfwPollEvents();

        if (m_headtrack)
            m_headtrack->update();

		begin_ui_frame();

		// -------------------------------------------------------------------
		// multiplayer command relaying logic can seem a bit complex
		//
		// we are simultaneously:
		// => master (not client) OR slave (client)
		// => server OR not
		//
		// trivia: being client and server is possible

		double frameStartTime = Timer::GetTime();

		if (m_modes[m_modestack.top()]->is_command_processor()) {
			// active mode is doing real calculations (e.g. drivermode)
			int loop_remaining = MAX_NETWORK_PER_FRAME;
			while (--loop_remaining > 0)
			{
				command_queue::commands_map commands_to_exec;
				command_queue::commands_map local_commands = simulation::Commands.pop_intercept_queue();
				double slave_sync;

				// if we're the server
				if (m_network && m_network->servers)
				{
					// fetch from network layer command requests received from clients
					command_queue::commands_map remote_commands = m_network->servers->pop_commands();

					// push these into local queue
					add_to_dequemap(local_commands, remote_commands);
				}

				// if we're slave
				if (m_network && m_network->client)
				{
					// fetch frame info from network layer,
					auto frame_info = m_network->client->get_next_delta(MAX_NETWORK_PER_FRAME - loop_remaining);

					// use delta and commands received from master
					double delta = std::get<0>(frame_info);
					Timer::set_delta_override(delta);
					slave_sync = std::get<1>(frame_info);
					add_to_dequemap(commands_to_exec, std::get<2>(frame_info));

					// and send our local commands to master
					m_network->client->send_commands(local_commands);

					if (delta == 0.0)
						loop_remaining = -1;
				}
				// if we're master
				else {
					// just push local commands to execution
					add_to_dequemap(commands_to_exec, local_commands);

					loop_remaining = -1;
				}

				// send commands to command queue
				simulation::Commands.push_commands(commands_to_exec);

				// do actual frame processing
				if (!m_modes[ m_modestack.top() ]->update())
					return 0;

				// update continuous commands
				simulation::Commands.update();

				double sync = generate_sync();

				// if we're the server
				if (m_network && m_network->servers)
				{
					// send delta, sync, and commands we just executed to clients
					double delta = Timer::GetDeltaTime();
					double render = Timer::GetDeltaRenderTime();
					m_network->servers->push_delta(render, delta, sync, commands_to_exec);
				}

				// if we're slave
				if (m_network && m_network->client)
				{
					// verify sync
					if (sync != slave_sync) {						
						WriteLog("net: desync! calculated: " + std::to_string(sync)
						         + ", received: " + std::to_string(slave_sync), logtype::net);

						Global.desync = slave_sync - sync;
					}

					// set total delta for rendering code
					double totalDelta = Timer::GetTime() - frameStartTime;
					Timer::set_delta_override(totalDelta);
				}
			}

			if (!loop_remaining) {
				// loop break forced by counter
				float received = m_network->client->get_frame_counter();
				float awaiting = m_network->client->get_awaiting_frames();

				// TODO: don't meddle with mode progresbar
				m_modes[m_modestack.top()]->set_progress(100.0f * (received - awaiting) / received);
			} else {
				m_modes[m_modestack.top()]->set_progress(0.0f, 0.0f);
			}
		} else {
			// active mode is loader

			// clear local command queue
			simulation::Commands.pop_intercept_queue();

			// do actual frame processing
            if (!m_modes[ m_modestack.top() ]->update())
                return 0;
		}

		// -------------------------------------------------------------------

		m_taskqueue.update();
		opengl_texture::reset_unit_cache();

        if (!GfxRenderer->Render())
            break;

        GfxRenderer->SwapBuffers();

        if (m_modestack.empty())
            return 0;

        m_modes[ m_modestack.top() ]->on_event_poll();

        if (m_screenshot_queued) {
            m_screenshot_queued = false;
            screenshot_man.make_screenshot();
        }

		if (m_network)
			m_network->update();

        auto const frametime{ Timer::subsystem.mainloop_total.stop() };
        if( Global.minframetime.count() != 0.0f && ( Global.minframetime - frametime ).count() > 0.0f ) {
            std::this_thread::sleep_for( Global.minframetime - frametime );
        }
    }

	return 0;
}

// issues request for a worker thread to perform specified task. returns: true if task was scheduled
bool
eu07_application::request( python_taskqueue::task_request const &Task ) {

    auto const result { m_taskqueue.insert( Task ) };
    if( ( false == result )
     && ( Task.input != nullptr ) ) {
        // clean up allocated resources since the worker won't
        delete Task.input;
    }
    return result;
}

// ensures the main thread holds the python gil and can safely execute python calls
void
eu07_application::acquire_python_lock() {

    m_taskqueue.acquire_lock();
}

// frees the python gil and swaps out the main thread
void
eu07_application::release_python_lock() {

    m_taskqueue.release_lock();
}

void
eu07_application::exit() {
	for (auto &mode : m_modes)
		mode.reset();

    GfxRenderer->Shutdown();
	m_network.reset();

//    SafeDelete( simulation::Train );
    SafeDelete( simulation::Region );

    ui_layer::shutdown();

    for( auto *window : m_windows ) {
        glfwDestroyWindow( window );
    }
    m_taskqueue.exit();
    glfwTerminate();

	if (!Global.exec_on_exit.empty())
		system(Global.exec_on_exit.c_str());
}

void
eu07_application::render_ui() {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->render_ui();
}

void eu07_application::begin_ui_frame() {

	if( m_modestack.empty() ) { return; }

	m_modes[ m_modestack.top() ]->begin_ui_frame();
}

bool
eu07_application::pop_mode() {
    if( m_modestack.empty() ) { return false; }

    m_modes[ m_modestack.top() ]->exit();
    m_modestack.pop();
    return true;
}

bool
eu07_application::push_mode( eu07_application::mode const Mode ) {

	if( Mode >= mode::count_ )
		return false;

	if (!m_modes[Mode]) {
		if (Mode == mode::launcher)
			m_modes[Mode] = std::make_shared<launcher_mode>();
		if (Mode == mode::scenarioloader)
			m_modes[Mode] = std::make_shared<scenarioloader_mode>();
		if (Mode == mode::driver)
			m_modes[Mode] = std::make_shared<driver_mode>();
		if (Mode == mode::editor)
			m_modes[Mode] = std::make_shared<editor_mode>();

		if (!m_modes[Mode]->init())
			return false;
	}

    m_modes[ Mode ]->enter();
    m_modestack.push( Mode );

    return true;
}

void
eu07_application::set_title( std::string const &Title ) {

    glfwSetWindowTitle( m_windows.front(), Title.c_str() );
}

void
eu07_application::set_progress( float const Progress, float const Subtaskprogress ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->set_progress( Progress, Subtaskprogress );
}

void
eu07_application::set_tooltip( std::string const &Tooltip ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->set_tooltip( Tooltip );
}

void
eu07_application::set_cursor( int const Mode ) {

    ui_layer::set_cursor( Mode );
}

void
eu07_application::set_cursor_pos( double const Horizontal, double const Vertical ) {

    glfwSetCursorPos( m_windows.front(), Horizontal, Vertical );
}

/*
// provides keyboard mapping associated with specified control item
std::string
eu07_application::get_input_hint( user_command const Command ) const {

    if( m_modestack.empty() ) { return ""; }

    return m_modes[ m_modestack.top() ]->get_input_hint( Command );
}
*/

void
eu07_application::on_key( int const Key, int const Scancode, int const Action, int const Mods ) {

    if (ui_layer::key_callback(Key, Scancode, Action, Mods))
        return;

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_key( Key, Scancode, Action, Mods );
}

void
eu07_application::on_cursor_pos( double const Horizontal, double const Vertical ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_cursor_pos( Horizontal, Vertical );
}

void
eu07_application::on_mouse_button( int const Button, int const Action, int const Mods ) {

    if (ui_layer::mouse_button_callback(Button, Action, Mods))
        return;

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_mouse_button( Button, Action, Mods );
}

void
eu07_application::on_scroll( double const Xoffset, double const Yoffset ) {

    if (ui_layer::scroll_callback(Xoffset, Yoffset))
        return;

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_scroll( Xoffset, Yoffset );
}

void eu07_application::on_char(unsigned int c) {
    if (ui_layer::char_callback(c))
        return;
}

void eu07_application::on_focus_change(bool focus) {
	if( Global.bInactivePause && !m_network->client) {// jeśli ma być pauzowanie okna w tle
		command_relay relay;
		relay.post(user_command::focuspauseset, focus ? 1.0 : 0.0, 0.0, GLFW_PRESS, 0);
	}
}

GLFWwindow *
eu07_application::window(int const Windowindex, bool visible, int width, int height, GLFWmonitor *monitor, bool keep_ownership , bool share_ctx) {

    if( Windowindex >= 0 ) {
        return (
            Windowindex < m_windows.size() ?
                m_windows[ Windowindex ] :
                nullptr );
    }
    // for index -1 create a new child window

	auto const *vmode { glfwGetVideoMode( monitor ? monitor : glfwGetPrimaryMonitor() ) };

    // match requested video mode to current to allow for
    // fullwindow creation when resolution is the same
	glfwWindowHint( GLFW_RED_BITS, vmode->redBits );
	glfwWindowHint( GLFW_GREEN_BITS, vmode->greenBits );
	glfwWindowHint( GLFW_BLUE_BITS, vmode->blueBits );
	glfwWindowHint( GLFW_REFRESH_RATE, vmode->refreshRate );

	glfwWindowHint( GLFW_VISIBLE, visible );

	auto *childwindow = glfwCreateWindow( width, height, "eu07window", monitor,
	                                      share_ctx ? m_windows.front() : nullptr);
	if (!childwindow)
		return nullptr;

	if (keep_ownership)
		m_windows.emplace_back( childwindow );

	glfwFocusWindow(m_windows.front()); // restore focus to main window

    return childwindow;
}

// private:
GLFWmonitor* eu07_application::find_monitor(const std::string &str) const {
    int monitor_count;
	GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);

	for (size_t i = 0; i < monitor_count; i++) {
		if (describe_monitor(monitors[i]) == str)
			return monitors[i];
	}

	return nullptr;
}

std::string eu07_application::describe_monitor(GLFWmonitor *monitor) const {
	std::string name(glfwGetMonitorName(monitor));
	std::replace(std::begin(name), std::end(name), ' ', '_');

	int x, y;
	glfwGetMonitorPos(monitor, &x, &y);

	return name + ":" + std::to_string(x) + "," + std::to_string(y);
}

// private:

void
eu07_application::init_debug() {

#if defined(_MSC_VER) && defined (_DEBUG)
    // memory leaks
    _CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
    /*
    // floating point operation errors
    auto state { _clearfp() };
    state = _control87( 0, 0 );
    // this will turn on FPE for #IND and zerodiv
    state = _control87( state & ~( _EM_ZERODIVIDE | _EM_INVALID ), _MCW_EM );
    */
#endif
}

void
eu07_application::init_files() {

#ifdef _WIN32
    DeleteFile( "log.txt" );
    DeleteFile( "errors.txt" );
    CreateDirectory( "logs", NULL );
#elif __unix__
	unlink("log.txt");
	unlink("errors.txt");
	mkdir("logs", 0755);
#endif
}

int
eu07_application::init_settings( int Argc, char *Argv[] ) {
    Global.asVersion = VERSION_INFO;

    Global.LoadIniFile( "eu07.ini" );
#ifdef _WIN32
    if( ( Global.iWriteLogEnabled & 2 ) != 0 ) {
        // show output console if requested
        AllocConsole();
    }
#endif

    // process command line arguments
    for( int i = 1; i < Argc; ++i ) {

        std::string token { Argv[ i ] };

        if( token == "-s" ) {
            if( i + 1 < Argc ) {
                Global.SceneryFile = ToLower( Argv[ ++i ] );
            }
        }
        else if( token == "-v" ) {
            if( i + 1 < Argc ) {
				Global.local_start_vehicle = ToLower( Argv[ ++i ] );
            }
        }
        else {
            std::cout
                << "usage: " << std::string( Argv[ 0 ] )
                << " [-s sceneryfilepath]"
                << " [-v vehiclename]"
                << std::endl;
            return -1;
        }
    }

    return 0;
}

int
eu07_application::init_locale() {

	Translations.init();

    return 0;
}

int
eu07_application::init_glfw() {
    {
        int glfw_major, glfw_minor, glfw_rev;
        glfwGetVersion(&glfw_major, &glfw_minor, &glfw_rev);
        m_glfwversion = glfw_major * 10000 + glfw_minor * 100 + glfw_minor;
    }

#ifdef GLFW_ANGLE_PLATFORM_TYPE
    if (m_glfwversion >= 30400) {
        int platform = GLFW_ANGLE_PLATFORM_TYPE_NONE;
        if (Global.gfx_angleplatform == "opengl")
            platform = GLFW_ANGLE_PLATFORM_TYPE_OPENGL;
        else if (Global.gfx_angleplatform == "opengles")
            platform = GLFW_ANGLE_PLATFORM_TYPE_OPENGLES;
        else if (Global.gfx_angleplatform == "d3d9")
            platform = GLFW_ANGLE_PLATFORM_TYPE_D3D9;
        else if (Global.gfx_angleplatform == "d3d11")
            platform = GLFW_ANGLE_PLATFORM_TYPE_D3D11;
        else if (Global.gfx_angleplatform == "vulkan")
            platform = GLFW_ANGLE_PLATFORM_TYPE_VULKAN;
        else if (Global.gfx_angleplatform == "metal")
            platform = GLFW_ANGLE_PLATFORM_TYPE_METAL;

        glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, platform);
    }
#endif

    if( glfwInit() == GLFW_FALSE ) {
        ErrorLog( "Bad init: failed to initialize glfw" );
        return -1;
    }

	{
		int monitor_count;
		GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);

		WriteLog("available monitors:");
		for (size_t i = 0; i < monitor_count; i++) {
			WriteLog(describe_monitor(monitors[i]));
		}
	}

	auto *monitor { find_monitor(Global.fullscreen_monitor) };
	if (!monitor)
		monitor = glfwGetPrimaryMonitor();

    glfwWindowHint( GLFW_AUTO_ICONIFY, GLFW_FALSE );
    if ((Global.gfx_skippipeline || Global.LegacyRenderer) && Global.iMultisampling > 0) {
        glfwWindowHint( GLFW_SAMPLES, 1 << Global.iMultisampling );
    }

    crashreport_add_info("gfxrenderer", Global.GfxRenderer);

    if( !Global.LegacyRenderer ) {
        Global.bUseVBO = true;
        // activate core profile for opengl 3.3 renderer
        if( !Global.gfx_usegles ) {
            glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
            glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
        }
        else {
#ifdef GLFW_CONTEXT_CREATION_API
            if (m_glfwversion >= 30200)
                glfwWindowHint( GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API );
#endif
            glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_ES_API );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
        }
    } else {
        if (Global.gfx_usegles) {
            ErrorLog("legacy renderer not supported in gles mode");
            return -1;
        }
        Global.gfx_shadergamma = false;
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    }

    if (Global.gfx_gldebug)
        glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );

    glfwWindowHint(GLFW_SRGB_CAPABLE, !Global.gfx_shadergamma);

    if( Global.fullscreen_windowed ) {
        auto const mode = glfwGetVideoMode( monitor );
        Global.window_size.x = mode->width;
        Global.window_size.y = mode->height;
        Global.bFullScreen = true;
    }

    auto *mainwindow = window(
        -1, true, Global.window_size.x, Global.window_size.y, ( Global.bFullScreen ? monitor : nullptr ), true, false );

    if( mainwindow == nullptr ) {
        ErrorLog( "Bad init: failed to create glfw window" );
        return -1;
    }

    glfwMakeContextCurrent( mainwindow );
    glfwSwapInterval( Global.VSync ? 1 : 0 ); //vsync

    {
        int width, height;

        glfwGetFramebufferSize( mainwindow, &width, &height );
        framebuffer_resize_callback( mainwindow, width, height );

        glfwGetWindowSize( mainwindow, &width, &height );
        window_resize_callback( mainwindow, width, height );
    }

#ifdef _WIN32
// setup wrapper for base glfw window proc, to handle copydata messages
    Hwnd = glfwGetWin32Window( mainwindow );
    BaseWindowProc = ( WNDPROC )::SetWindowLongPtr( Hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc );
    // switch off the topmost flag
    ::SetWindowPos( Hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
#endif

    return 0;
}

void
eu07_application::init_callbacks() {

    auto *window { m_windows.front() };
    glfwSetWindowSizeCallback( window, window_resize_callback );
    glfwSetFramebufferSizeCallback( window, framebuffer_resize_callback );
    glfwSetCursorPosCallback( window, cursor_pos_callback );
    glfwSetMouseButtonCallback( window, mouse_button_callback );
    glfwSetKeyCallback( window, key_callback );
    glfwSetScrollCallback( window, scroll_callback );
    glfwSetCharCallback(window, char_callback);
    glfwSetWindowFocusCallback( window, focus_callback );
}

int
eu07_application::init_ogl() {
    if (!Global.gfx_usegles)
    {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            ErrorLog( "Bad init: failed to initialize glad" );
            return -1;
        }
    }
    else
    {
        if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
        {
            ErrorLog( "Bad init: failed to initialize glad" );
            return -1;
        }
    }

    return 0;
}

int
eu07_application::init_ui() {
    if( false == ui_layer::init( m_windows.front() ) ) {
        return -1;
    }
    init_callbacks();
    return 0;
}

int
eu07_application::init_gfx() {

    if( Global.GfxRenderer == "default" ) {
        // default render path
        GfxRenderer = gfx_renderer_factory::get_instance()->create("modern");
    }
    else {
        // legacy render path
        GfxRenderer = gfx_renderer_factory::get_instance()->create("legacy");
        Global.DisabledLogTypes |= static_cast<unsigned int>( logtype::material );
    }

    if (!GfxRenderer) {
        ErrorLog("no renderer found!");
        return -1;
    }

    if( false == GfxRenderer->Init( m_windows.front() ) ) {
        return -1;
    }

	for (const global_settings::extraviewport_config &conf : Global.extra_viewports)
        if (!GfxRenderer->AddViewport(conf))
			return -1;

    if (!Global.headtrack_conf.joy.empty())
        m_headtrack.emplace();

    return 0;
}

int
eu07_application::init_audio() {

    if( Global.bSoundEnabled ) {
        Global.bSoundEnabled &= audio::renderer.init();
    }
    // NOTE: lack of audio isn't deemed a failure serious enough to throw in the towel
    return 0;
}

int
eu07_application::init_data() {

    // HACK: grab content of the first {} block in load_unit_weights using temporary parser, then parse it normally. on any error our weight list will be empty string
    auto loadweights { cParser( cParser( "data/load_weights.txt", cParser::buffer_FILE ).getToken<std::string>( true, "{}" ), cParser::buffer_TEXT ) };
    while( true == loadweights.getTokens( 2 ) ) {
        std::pair<std::string, float> weightpair;
        loadweights
            >> weightpair.first
            >> weightpair.second;
        weightpair.first.erase( weightpair.first.end() - 1 ); // trim trailing ':' from the key
        simulation::Weights.emplace( weightpair.first, weightpair.second );
    }
    cParser override_parser( "data/sound_overrides.txt", cParser::buffer_FILE );
    deserialize_map( simulation::Sound_overrides,  override_parser);

    return 0;
}

int
eu07_application::init_modes() {
	Global.local_random_engine.seed(std::random_device{}());

	if ((!Global.network_servers.empty() || Global.network_client) && Global.SceneryFile.empty()) {
		ErrorLog("launcher mode is currently not supported in network mode");
		return -1;
    }

    // activate the default mode
	if (Global.SceneryFile.empty())
		push_mode( mode::launcher );
	else
		push_mode( mode::scenarioloader );

    return 0;
}

bool eu07_application::init_network() {
	if (!Global.network_servers.empty() || Global.network_client) {
		// create network manager
		m_network.emplace();
	}

	for (auto const &pair : Global.network_servers) {
		// create all servers
		m_network->create_server(pair.first, pair.second);
	}

	if (Global.network_client) {
		// create client
		m_network->connect(Global.network_client->first, Global.network_client->second);
	} else {
		// we're simulation master
		if (!Global.random_seed)
			Global.random_seed = std::random_device{}();
		Global.random_engine.seed(Global.random_seed);

		// TODO: sort out this timezone mess
		std::time_t utc_now = std::time(nullptr);

		tm tm_local, tm_utc;
		tm *tmp = std::localtime(&utc_now);
		memcpy(&tm_local, tmp, sizeof(tm));
		tmp = std::gmtime(&utc_now);
		memcpy(&tm_utc, tmp, sizeof(tm));

		int64_t offset = (tm_local.tm_hour * 3600 + tm_local.tm_min * 60 + tm_local.tm_sec)
		        - (tm_utc.tm_hour * 3600 + tm_utc.tm_min * 60 + tm_utc.tm_sec);

		Global.starting_timestamp = utc_now + offset;
		Global.ready_to_load = true;
	}

	return true;
}
