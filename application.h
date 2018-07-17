/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

class eu07_application {

public:
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
        set_cursor( int const Mode );
    void
        set_cursor_pos( double const X, double const Y );
    void
        get_cursor_pos( double &X, double &Y ) const;
	void queue_screenshot();
    inline
    GLFWwindow *
        window() {
            return m_window; }

private:
// methods
    void init_debug();
    void init_files();
    int  init_settings( int Argc, char *Argv[] );
    int  init_glfw();
    void init_callbacks();
    int  init_gfx();
    int  init_audio();
// members
    GLFWwindow * m_window { nullptr };
	bool screenshot_queued = false;
};

extern eu07_application Application;
