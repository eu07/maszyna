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

class mouse_input {

public:
// constructors
    mouse_input() { default_bindings(); }

// methods
    bool
        init();
    void
        move( double const Mousex, double const Mousey );
    void
        button( int const Button, int const Action );
    void
        poll();

private:
// types
    struct mouse_commands {

        user_command left;
        user_command right;

        mouse_commands( user_command const Left, user_command const Right ):
                                      left(Left),             right(Right)
        {}
    };

    typedef std::unordered_map<std::string, mouse_commands> controlcommands_map;

// methods
    void
        default_bindings();

// members
    command_relay m_relay;
    controlcommands_map m_mousecommands;
    user_command m_mousecommandleft { user_command::none }; // last if any command issued with left mouse button
    user_command m_mousecommandright { user_command::none }; // last if any command issued with right mouse button
    double m_updaterate { 0.075 };
    double m_updateaccumulator { 0.0 };
    bool m_pickmodepanning { false }; // indicates mouse is in view panning mode
    glm::dvec2 m_cursorposition; // stored last cursor position, used for panning
    glm::dvec2 m_commandstartcursor; // helper, cursor position when the command was initiated
    bool m_varyingpollrate { false }; // indicates rate of command repeats is affected by the cursor position
};

//---------------------------------------------------------------------------
