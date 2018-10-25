/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "applicationmode.h"
#include "PyInt.h"

class eu07_application {

public:
// types
    enum mode {
//        launcher = 0,
        scenarioloader,
        driver,
        editor,
        count_
    };
// constructors
    eu07_application() = default;
// methods
    int
        init( int Argc, char *Argv[] );
    int
        run();
    // issues request for a worker thread to perform specified task. returns: true if task was scheduled
    bool
        request( python_taskqueue::task_request const &Task );
    // ensures the main thread holds the python gil and can safely execute python calls
    void
        acquire_python_lock();
    // frees the python gil and swaps out the main thread
    void
        release_python_lock();
    void
        exit();
    void
        render_ui();
    // switches application to specified mode
    bool
        pop_mode();
    bool
        push_mode( eu07_application::mode const Mode );
    void
        set_title( std::string const &Title );
    void
        set_progress( float const Progress = 0.f, float const Subtaskprogress = 0.f );
    void
        set_cursor( int const Mode );
    void
        set_cursor_pos( double const Horizontal, double const Vertical );
    void
        get_cursor_pos( double &Horizontal, double &Vertical ) const;
    void queue_screenshot();
    // input handlers
    void
        on_key( int const Key, int const Scancode, int const Action, int const Mods );
    void
        on_cursor_pos( double const Horizontal, double const Vertical );
    void
        on_mouse_button( int const Button, int const Action, int const Mods );
    void
        on_scroll( double const Xoffset, double const Yoffset );
    void on_char(unsigned int c);
    
    // gives access to specified window, creates a new window if index == -1
    GLFWwindow *
        window( int const Windowindex = 0 );

private:
// types
    using modeptr_array = std::array<std::shared_ptr<application_mode>, static_cast<std::size_t>( mode::count_ )>;
    using mode_stack = std::stack<mode>;
// methods
    void init_debug();
    void init_files();
    int  init_settings( int Argc, char *Argv[] );
    int  init_locale();
    int  init_glfw();
    void init_callbacks();
    int  init_gfx();
    int  init_audio();
    int  init_modes();
// members

	bool screenshot_queued = false;

    modeptr_array m_modes { nullptr }; // collection of available application behaviour modes
    mode_stack m_modestack; // current behaviour mode
    python_taskqueue m_taskqueue;
    std::vector<GLFWwindow *> m_windows;
};

extern eu07_application Application;
