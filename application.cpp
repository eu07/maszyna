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
#include "scenarioloadermode.h"
#include "drivermode.h"
#include "editormode.h"

#include "Globals.h"
#include "simulation.h"
#include "Train.h"
#include "dictionary.h"
#include "sceneeditor.h"
#include "openglrenderer.h"
#include "opengl33renderer.h"
#include "uilayer.h"
#include "translation.h"
#include "Logs.h"
#include "Timer.h"

#ifdef EU07_BUILD_STATIC
#pragma comment( lib, "glfw3.lib" )
#else
#ifdef _WIN32
#pragma comment( lib, "glfw3dll.lib" )
#else
#pragma comment( lib, "glfw3.lib" )
#endif
#endif // build_static
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
#pragma comment( lib, "openal32.lib")
#pragma comment( lib, "setupapi.lib" )
#pragma comment( lib, "python27.lib" )
#pragma comment( lib, "libserialport-0.lib" )
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "version.lib")

#ifdef __unix__
#include <unistd.h>
#include <sys/stat.h>
#endif

extern "C" { _declspec( dllexport ) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; }
extern "C" { _declspec( dllexport ) DWORD NvOptimusEnablement = 0x00000001; }

eu07_application Application;

ui_layer uilayerstaticinitializer;

#ifdef _WIN32
extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window );
}

LONG CALLBACK unhandled_handler( ::EXCEPTION_POINTERS* e );
LRESULT APIENTRY WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern HWND Hwnd;
extern WNDPROC BaseWindowProc;
#endif

// user input callbacks

void focus_callback( GLFWwindow *window, int focus ) {
	Application.on_focus_change(focus != 0);
}

void window_resize_callback( GLFWwindow *window, int w, int h ) {
    // NOTE: we have two variables which basically do the same thing as we don't have dynamic fullscreen toggle
    // TBD, TODO: merge them?
    Global.iWindowWidth = w;
	Global.iWindowHeight = h;
    glViewport( 0, 0, w, h );
}

void cursor_pos_callback( GLFWwindow *window, double x, double y ) {

    Application.on_cursor_pos( x, y );
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods ) {

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

int
eu07_application::init( int Argc, char *Argv[] ) {

    int result { 0 };

    init_debug();
    init_files();
    if( ( result = init_settings( Argc, Argv ) ) != 0 ) {
        return result;
    }
    if( ( result = init_locale() ) != 0 ) {
        return result;
    }

	WriteLog( "Starting MaSzyna rail vehicle simulator (release: " + Global.asVersion + ")" );
	WriteLog( "For online documentation and additional files refer to: http://eu07.pl" );
	WriteLog( "Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx, OLO_EU, Bart, Quark-t, "
	    "ShaXbee, Oli_EU, youBy, KURS90, Ra, hunter, szociu, Stele, Q, firleju and others\n" );

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
    if( ( result = init_gfx() ) != 0 ) {
        return result;
    }
    init_callbacks();
    if( ( result = init_audio() ) != 0 ) {
        return result;
    }
    if( ( result = init_data() ) != 0 ) {
        return result;
    }
    m_taskqueue.init();
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

void eu07_application::queue_quit() {
	glfwSetWindowShouldClose(m_windows[0], GLFW_TRUE);
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
#ifdef EU07_DEBUG_NETSYNC
                WriteLog( "net: frame " + std::to_string(++frame) + " start", logtype::net );
#endif
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

        // TODO: re-enable after python changes merge
		m_taskqueue.update();

        if (!GfxRenderer->Render())
            return 0;

        if (m_modestack.empty())
            return 0;

        m_modes[ m_modestack.top() ]->on_event_poll();

		if (m_network)
			m_network->update();

		auto frametime = Timer::subsystem.mainloop_total.stop();
		if (Global.minframetime.count() != 0.0f && (Global.minframetime - frametime).count() > 0.0f)
			std::this_thread::sleep_for(Global.minframetime - frametime);
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

bool
eu07_application::pop_mode() {
    if( m_modestack.empty() ) { return false; }

    m_modes[ m_modestack.top() ]->exit();
    m_modestack.pop();
    return true;
}

bool
eu07_application::push_mode( eu07_application::mode const Mode ) {

    if( Mode >= mode::count_ ) { return false; }

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

glm::dvec2
eu07_application::get_cursor_pos() const {

    glm::dvec2 pos;
    if( !m_windows.empty() ) {
        glfwGetCursorPos( m_windows.front(), &pos.x, &pos.y );
    }
    return pos;
}

void
eu07_application::get_cursor_pos( double &Horizontal, double &Vertical ) const {

    glfwGetCursorPos( m_windows.front(), &Horizontal, &Vertical );
}


int
eu07_application::get_mouse_button( int const Button ) const {

    return glfwGetMouseButton( m_windows.front(), Button );
}
/*
// provides keyboard mapping associated with specified control item
std::string
eu07_application::get_input_hint( user_command const Command ) const {

    if( m_modestack.empty() ) { return ""; }

    return m_modes[ m_modestack.top() ]->get_input_hint( Command );
}
*/
int
eu07_application::key_binding( user_command const Command ) const {

    if( m_modestack.empty() ) { return -1; }

    return m_modes[ m_modestack.top() ]->key_binding( Command );
}

void
eu07_application::on_key( int const Key, int const Scancode, int const Action, int const Mods ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_key( Key, Scancode, Action, Mods );
}

void eu07_application::on_char( unsigned int const Char ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_char( Char );
}

void
eu07_application::on_cursor_pos( double const Horizontal, double const Vertical ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_cursor_pos( Horizontal, Vertical );
}

void
eu07_application::on_mouse_button( int const Button, int const Action, int const Mods ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_mouse_button( Button, Action, Mods );
}

void
eu07_application::on_scroll( double const Xoffset, double const Yoffset ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_scroll( Xoffset, Yoffset );
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
#ifdef _WIN32
    ::SetUnhandledExceptionFilter( unhandled_handler );
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

    Global.LoadIniFile( "eu07.ini" );
#ifdef _WIN32
    if( ( Global.iWriteLogEnabled & 2 ) != 0 ) {
        // show output console if requested
        AllocConsole();
    }
#endif
/*
    std::string executable( argv[ 0 ] ); auto const pathend = executable.rfind( '\\' );
    Global.ExecutableName =
        ( pathend != std::string::npos ?
            executable.substr( executable.rfind( '\\' ) + 1 ) :
            executable );
*/
#ifdef _WIN32
    // retrieve product version from the file's version data table
    {
        auto const fileversionsize { ::GetFileVersionInfoSize( Argv[ 0 ], NULL ) };
        std::vector<BYTE>fileversiondata; fileversiondata.resize( fileversionsize );
        if( ::GetFileVersionInfo( Argv[ 0 ], 0, fileversionsize, fileversiondata.data() ) ) {

            struct lang_codepage {
                WORD language;
                WORD codepage;
            } *langcodepage;
            UINT datasize;

            ::VerQueryValue(
                fileversiondata.data(),
                TEXT( "\\VarFileInfo\\Translation" ),
                (LPVOID*)&langcodepage,
                &datasize );

            std::string subblock; subblock.resize( 50 );
            ::StringCchPrintf(
                &subblock[0], subblock.size(),
                TEXT( "\\StringFileInfo\\%04x%04x\\ProductVersion" ),
                langcodepage->language,
                langcodepage->codepage );

            VOID *stringdata;
            if( ::VerQueryValue(
                    fileversiondata.data(),
                    subblock.data(),
                    &stringdata,
                    &datasize ) ) {

                Global.asVersion = std::string( reinterpret_cast<char*>(stringdata) );
            }
        }
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

    locale::init();

    return 0;
}

int
eu07_application::init_glfw() {

    if( glfwInit() == GLFW_FALSE ) {
        ErrorLog( "Bad init: failed to initialize glfw" );
        return -1;
    }
    // match requested video mode to current to allow for
    // fullwindow creation when resolution is the same
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
    if( Global.iMultisampling > 0 ) {
        glfwWindowHint( GLFW_SAMPLES, 1 << Global.iMultisampling );
    }

    glfwWindowHint(GLFW_SRGB_CAPABLE, !Global.gfx_shadergamma);

    if( Global.GfxRenderer == "default" ) {
        // activate core profile for opengl 3.3 renderer
        if( !Global.gfx_usegles ) {
            glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
        }
        else {
#ifdef GLFW_CONTEXT_CREATION_API
            glfwWindowHint( GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API );
#endif
            glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_ES_API );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
            glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
        }
    }

    auto *mainwindow = window(
        -1, true, Global.iWindowWidth, Global.iWindowHeight, Global.bFullScreen ? monitor : nullptr, true, false );

    if( mainwindow == nullptr ) {
        ErrorLog( "Bad init: failed to create glfw window" );
        return -1;
    }

    glfwMakeContextCurrent( mainwindow );
    glfwSwapInterval( Global.VSync ? 1 : 0 ); //vsync

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
    glfwSetFramebufferSizeCallback( window, window_resize_callback );
    glfwSetCursorPosCallback( window, cursor_pos_callback );
    glfwSetMouseButtonCallback( window, mouse_button_callback );
    glfwSetKeyCallback( window, key_callback );
    glfwSetScrollCallback( window, scroll_callback );
    glfwSetCharCallback(window, char_callback);
    glfwSetWindowFocusCallback( window, focus_callback );
    {
        int width, height;
        glfwGetFramebufferSize( window, &width, &height );
        window_resize_callback( window, width, height );
    }
}

int
eu07_application::init_gfx() {

    if (Global.gfx_usegles)
    {
        if( 0 == gladLoadGLES2Loader( (GLADloadproc)glfwGetProcAddress ) ) {
            ErrorLog( "Bad init: failed to initialize glad" );
            return -1;
        }
    }
    else
    {
        if( 0 == gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) ) {
            ErrorLog( "Bad init: failed to initialize glad" );
            return -1;
        }
    }

    if( Global.GfxRenderer == "default" ) {
        // default render path
        GfxRenderer = std::make_unique<opengl33_renderer>();
    }
    else {
        // legacy render path
        GfxRenderer = std::make_unique<opengl_renderer>();
        Global.GfxFramebufferSRGB = false;
        Global.DisabledLogTypes |= static_cast<unsigned int>( logtype::material );
    }

    if( false == GfxRenderer->Init( m_windows.front() ) ) {
        return -1;
    }
    if( false == ui_layer::init( m_windows.front() ) ) {
        return -1;
    }
/*
	for (const global_settings::extraviewport_config &conf : Global.extra_viewports)
		if (!GfxRenderer.AddViewport(conf))
			return -1;
*/
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

    return 0;
}

int
eu07_application::init_modes() {
	Global.local_random_engine.seed(std::random_device{}());

	if ((!Global.network_servers.empty() || Global.network_client) && Global.SceneryFile.empty()) {
		ErrorLog("launcher mode is currently not supported in network mode");
		return -1;
	}
    // NOTE: we could delay creation/initialization until transition to specific mode is requested,
    // but doing it in one go at the start saves us some error checking headache down the road

    // create all application behaviour modes
    m_modes[ mode::scenarioloader ] = std::make_shared<scenarioloader_mode>();
    m_modes[ mode::driver ] = std::make_shared<driver_mode>();
    m_modes[ mode::editor ] = std::make_shared<editor_mode>();
    // initialize the mode objects
    for( auto &mode : m_modes ) {
        if( false == mode->init() ) {
            return -1;
        }
    }
    // activate the default mode
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
