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
    // input handlers
    void
        on_key( int const Key, int const Scancode, int const Action, int const Mods );
    void
        on_cursor_pos( double const Horizontal, double const Vertical );
    void
        on_mouse_button( int const Button, int const Action, int const Mods );
    void
        on_scroll( double const Xoffset, double const Yoffset );
    inline
    GLFWwindow *
        window() {
            return m_window; }

private:
// types
    using modeptr_array = std::array<std::shared_ptr<application_mode>, static_cast<std::size_t>( mode::count_ )>;
    using mode_stack = std::stack<mode>;
// methods
    void init_debug();
    void init_files();
    int  init_settings( int Argc, char *Argv[] );
    int  init_glfw();
    void init_callbacks();
    int  init_gfx();
    int  init_audio();
    int  init_modes();
// members
    GLFWwindow * m_window { nullptr };
    modeptr_array m_modes { nullptr }; // collection of available application behaviour modes
    mode_stack m_modestack; // current behaviour mode
};

extern eu07_application Application;
