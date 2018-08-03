#pragma once

#include <string>
#include "Texture.h"
#include "map.h"

// GuiLayer -- basic user interface class. draws requested information on top of openGL screen

class ui_layer {

public:
// parameters:

// constructors:

// destructor:
    ~ui_layer();

// methods:
    bool
        init( GLFWwindow *Window );
    void cleanup();

    // callback functions for imgui input
    // returns true if input is consumed
    bool key_callback(int key, int scancode, int action, int mods);
    bool char_callback(unsigned int c);
    bool scroll_callback(double xoffset, double yoffset);
    bool mouse_button_callback(int button, int action, int mods);

    // potentially processes provided input. returns: true if the input was processed, false otherwise
    bool
        on_key( int const Key, int const Action );
	// draws requested UI elements
	void
        render();
	// stores operation progress
	void
        set_progress( float const Progress = 0.0f, float const Subtaskprogress = 0.0f );
    void
        set_progress( std::string const &Text ) { m_progresstext = Text; }
	// sets the ui background texture, if any
	void
        set_background( std::string const &Filename = "" );
    void
        set_tooltip( std::string const &Tooltip ) { m_tooltip = Tooltip; }
    void set_cursor(int const Mode);

    std::deque<std::string> log;

// members:

private:
// methods:
    // draws background quad with specified earlier texture
    void
        render_background();
    // draws a progress bar in defined earlier state
    void
        render_progress();
    void
        render_tooltip();

// members:
    GLFWwindow *m_window { nullptr };
    bool basic_info_active = false;
    bool timetable_active = false;
    bool vehicle_info_active = false;
    bool renderer_debug_active = false;
    bool about_active = false;
    bool quit_active = false;
    bool events_active = false;
    bool log_active;

    bool m_cursorvisible;

    ImGuiIO *imgui_io;

    std::unique_ptr<map> m_map;

    // progress bar config. TODO: put these together into an object
    float m_progress { 0.0f }; // percentage of filled progres bar, to indicate lengthy operations.
    float m_subtaskprogress{ 0.0f }; // percentage of filled progres bar, to indicate lengthy operations.
    std::string m_progresstext; // label placed over the progress bar

    texture_handle m_background { null_handle }; // path to texture used as the background. size depends on mAspect.
    GLuint m_texture { 0 };

    std::string m_tooltip;

    // TODO: clean these legacy components up
    int tprev; // poprzedni czas
    double VelPrev; // poprzednia prędkość
    double Acc; // przyspieszenie styczne
};

extern ui_layer UILayer;
