/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include "Texture.h"
#include "translation.h"
#include "widgets/popup.h"

// GuiLayer -- basic user interface class. draws requested information on top of openGL screen

class ui_panel {

public:
// constructor
    ui_panel( std::string const &Identifier, bool const Isopen );
	virtual ~ui_panel() = default;
// methods
    virtual void update() {};
	virtual void render();
	virtual void render_contents();
    // temporary  access
// types
    struct text_line {

        std::string data;
        glm::vec4 color;

        text_line( std::string const &Data, glm::vec4 const &Color)
            : data(Data), color(Color)
        {}
    };
// members
    std::string title;
    bool is_open;

    glm::ivec2 size { -1, -1 };
    glm::ivec2 size_min { -1, -1 };
    glm::ivec2 size_max { -1, -1 };
    std::deque<text_line> text_lines;
	int window_flags = -1;

	const std::string& name() { return m_name; }
	void register_popup(std::unique_ptr<ui::popup> &&popup);

protected:
// members
	std::string m_name;
	std::list<std::unique_ptr<ui::popup>> popups;
};

class ui_expandable_panel : public ui_panel {
public:
    using ui_panel::ui_panel;

    bool is_expanded { false };

	void render_contents() override;
};

class ui_log_panel : public ui_panel {
    using ui_panel::ui_panel;

	void render_contents() override;
};

class ImFont;
class ui_layer {

public:
// constructors
    ui_layer();
// destructor
    virtual ~ui_layer();

// methods
    static
    bool
        init( GLFWwindow *Window );

	static
	void
	    imgui_style();

    // assign texturing hardware unit
    static
    void
        set_unit( GLint const Textureunit ) { m_textureunit = GL_TEXTURE0 + Textureunit; }
    static
    void
        shutdown();
    // potentially processes provided input key. returns: true if the input was processed, false otherwise
    virtual
    bool
        on_key( int const Key, int const Action );
    // potentially processes provided mouse movement. returns: true if the input was processed, false otherwise
    virtual
    bool
        on_cursor_pos( double const Horizontal, double const Vertical );
    // potentially processes provided mouse button. returns: true if the input was processed, false otherwise
    virtual
    bool
        on_mouse_button( int const Button, int const Action );
    // updates state of UI elements
    virtual
    void
        update();
	// draws requested UI elements
	void
	    render();
    static void
        render_internal();
	// begins new UI frame
	// (this is separate from render() to allow for debug GUI outside of proper UI framework)
	void
	    begin_ui_frame();
    static void
        begin_ui_frame_internal();
    //
    static
    void
        set_cursor( int const Mode );
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
    void
        clear_panels();
    void
	    add_external_panel( ui_panel *Panel ) { m_panels.emplace_back( Panel ); }
	void
	    add_owned_panel( ui_panel *Panel );

    // callback functions for imgui input
    // returns true if input is consumed
    static bool key_callback(int key, int scancode, int action, int mods);
    static bool char_callback(unsigned int c);
    static bool scroll_callback(double xoffset, double yoffset);
    static bool mouse_button_callback(int button, int action, int mods);

	static ImFont *font_default;
	static ImFont *font_mono;

protected:
// members
    static GLFWwindow *m_window;
    static ImGuiIO *m_imguiio;
    static bool m_cursorvisible;    

	void load_random_background();
   virtual void render_menu_contents();
   ui_log_panel m_logpanel { "Log", true };

private:
// methods
	// render() subclass details
    virtual
    void
	    render_() {};
    // draws background quad with specified earlier texture
    void
	    render_background();
    // draws a progress bar in defined earlier state
    void
	    render_progress();
    void
	    render_tooltip();
	void render_panels();

   void render_menu();

   void render_quit_widget();

    // draws a quad between coordinates x,y and z,w with uv-coordinates spanning 0-1
    void
        quad( glm::vec4 const &Coordinates, glm::vec4 const &Color );
// members
    static GLint m_textureunit;

    // progress bar config. TODO: put these together into an object
    float m_progress { 0.0f }; // percentage of filled progres bar, to indicate lengthy operations.
    float m_subtaskprogress{ 0.0f }; // percentage of filled progres bar, to indicate lengthy operations.
    std::string m_progresstext; // label placed over the progress bar

    texture_handle m_background { null_handle }; // path to texture used as the background. size depends on mAspect.
    std::vector<ui_panel *> m_panels;
	std::vector<std::unique_ptr<ui_panel>> m_ownedpanels;
    std::string m_tooltip;
    bool m_quit_active = false;
	bool m_imgui_demo = false;
};
