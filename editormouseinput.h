/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "command.h"

class editormouse_input {

public:
// constructors
    editormouse_input() = default;

// methods
    bool
        init();
    void
        position( double const Horizontal, double const Vertical );
    inline
    glm::dvec2
        position() const {
            return m_cursorposition; }
    void
        button( int const Button, int const Action );
    inline
    int
        button( int const Button ) const {
            return m_buttons[ Button ]; }

private:
// members
    command_relay m_relay;
    bool m_pickmodepanning { false }; // indicates mouse is in view panning mode
    glm::dvec2 m_cursorposition { 0.0 }; // stored last cursor position, used for panning
    std::array<int, GLFW_MOUSE_BUTTON_LAST> m_buttons { GLFW_RELEASE };
};

//---------------------------------------------------------------------------
