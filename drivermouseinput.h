/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <unordered_map>
#include "command.h"

// virtual slider; value determined by position of the mouse
class mouse_slider {

public:
// constructors
    mouse_slider() = default;
// methods
    void
        bind( user_command const &Command );
    void
        release();
    void
        on_move( double const Mousex, double const Mousey );
    inline
    user_command
        command() const {
            return m_command; }
    double
        value() const {
            return m_value; }

private:
// members
    user_command m_command { user_command::none };
    double m_value { 0.0 };
    double m_valuerange { 0.0 };
    bool m_analogue { false };
    bool m_invertrange { false };
    glm::dvec2 m_cursorposition { 0.0 };
};

class drivermouse_input {

public:
// constructors
    drivermouse_input() = default;

// methods
    bool
        init();
    bool
        recall_bindings();
    void
        button( int const Button, int const Action );
    int
        button( int const Button ) const;
    void
        move( double const Horizontal, double const Vertical );
    void
        scroll( double const Xoffset, double const Yoffset );
    void
        poll();
    user_command
        command() const;
    // returns pair of bindings associated with specified cab control
    std::pair<user_command, user_command>
        bindings( std::string const &Control ) const;

private:
// types
    struct button_bindings {

        user_command left;
        user_command right;
    };

    struct wheel_bindings {
        
        user_command up;
        user_command down;
    };

    typedef std::unordered_map<std::string, button_bindings> buttonbindings_map;

// methods
    void
        default_bindings();
    // potentially replaces supplied command with a more relevant one
    user_command
        adjust_command( user_command Command );

// members
    command_relay m_relay;
    mouse_slider m_slider; // virtual control, when active translates intercepted mouse position to a value
    buttonbindings_map m_buttonbindings;
    wheel_bindings m_wheelbindings { user_command::mastercontrollerincrease, user_command::mastercontrollerdecrease }; // commands bound to the scroll wheel
    user_command m_mousecommandleft { user_command::none }; // last if any command issued with left mouse button
    user_command m_mousecommandright { user_command::none }; // last if any command issued with right mouse button
    double m_updaterate { 0.075 };
    double m_updatedelay { 0.25 };
    double m_updateaccumulator { 0.0 };
    bool m_pickmodepanning { false }; // indicates mouse is in view panning mode
    glm::dvec2 m_cursorposition; // stored last cursor position, used for panning
    bool m_varyingpollrate { false }; // indicates rate of command repeats is affected by the cursor position
    glm::dvec2 m_varyingpollrateorigin; // helper, cursor position when the command was initiated
    std::array<int, GLFW_MOUSE_BUTTON_LAST> m_buttons;
    bool m_pickwaiting;
};

//---------------------------------------------------------------------------