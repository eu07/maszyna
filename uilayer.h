
#pragma once

#include <string>
#include "texture.h"
#include "float3d.h"

// GuiLayer -- basic user interface class. draws requested information on top of openGL screen

struct ui_panel {

    struct text_line {

        float4 color;
        std::string data;

        text_line( std::string const &Data, float4 const &Color):
            data(Data), color(Color)
        {}
    };

    ui_panel( const int X, const int Y):
        origin_x(X), origin_y(Y)
    {}

    std::vector<text_line> text_lines;
    int origin_x;
    int origin_y;
};

class ui_layer {

public:
// parameters:

// constructors:

// destructor:
    ~ui_layer();

// methods:
    bool
        init( GLFWwindow *Window );
    void
        set_unit( GLint const Textureunit ) { m_textureunit = Textureunit; }
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
        set_texture( GLuint Texture = NULL ) { m_texture = Texture; }
    void
        set_tooltip( std::string const &Tooltip ) { m_tooltip = Tooltip; }
    void
        clear_texts() { m_panels.clear(); }
    void
        push_back( std::shared_ptr<ui_panel> Panel ) { m_panels.emplace_back( Panel ); }

// members:

private:
// methods:
    // draws background quad with specified earlier texture
    void
        render_background();
    void
        render_texture();
    // draws a progress bar in defined earlier state
    void
        render_progress();
    void
        render_panels();
    void
        render_tooltip();
    // prints specified text, using display lists font
    void
        print( std::string const &Text );
    // draws a quad between coordinates x,y and z,w with uv-coordinates spanning 0-1
    void
        quad( float4 const &Coordinates, float4 const &Color );


// members:
    GLFWwindow *m_window { nullptr };
    GLint m_textureunit{ GL_TEXTURE0 };
    GLuint m_fontbase { (GLuint)-1 }; // numer DL dla znak�w w napisach

    // progress bar config. TODO: put these together into an object
    float m_progress { 0.0f }; // percentage of filled progres bar, to indicate lengthy operations.
    float m_subtaskprogress{ 0.0f }; // percentage of filled progres bar, to indicate lengthy operations.
    std::string m_progresstext; // label placed over the progress bar
    bool m_progressbottom { false }; // location of the progress bar

    texture_handle m_background { NULL }; // path to texture used as the background. size depends on mAspect.
    GLuint m_texture { NULL };
    std::vector<std::shared_ptr<ui_panel> > m_panels;
    std::string m_tooltip;
};

extern ui_layer UILayer;
