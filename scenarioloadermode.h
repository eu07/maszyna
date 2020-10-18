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
#include "simulation.h"

class scenarioloader_mode : public application_mode {
	std::shared_ptr<simulation::deserializer_state> state;
	std::chrono::system_clock::time_point timestart;

public:
// constructors
    scenarioloader_mode();
// methods
    // initializes internal data structures of the mode. returns: true on success, false otherwise
    bool
        init() override;
    // mode-specific update of simulation data. returns: false on error, true otherwise
    bool
        update() override;
    // maintenance method, called when the mode is activated
    void
        enter() override;
    // maintenance method, called when the mode is deactivated
    void
        exit() override;
    // input handlers
    void
        on_key( int const Key, int const Scancode, int const Action, int const Mods ) override { ; }
    void
        on_cursor_pos( double const Horizontal, double const Vertical ) override { ; }
    void
        on_mouse_button( int const Button, int const Action, int const Mods ) override { ; }
    void
        on_scroll( double const Xoffset, double const Yoffset ) override { ; }
    void
        on_event_poll() override { ; }
    bool
        is_command_processor() const override;
};
