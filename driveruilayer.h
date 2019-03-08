/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "uilayer.h"
#include "driveruipanels.h"

class driver_ui : public ui_layer {

public:
// constructors
    driver_ui();
// methods
    // updates state of UI elements
    void
        update() override;

private:
// methods
    // sets visibility of the cursor
    void
        set_cursor( bool const Visible );
    // render() subclass details
    void
        render_() override;
    // on_key() subclass details
    bool
        on_key_( int const Key, int const Scancode, int const Action, int const Mods ) override;
    // on_cursor_pos() subclass details
    bool
        on_cursor_pos_( double const Horizontal, double const Vertical ) override;
    // on_mouse_button() subclass details
    bool
        on_mouse_button_( int const Button, int const Action, int const Mods ) override;
// members
    drivingaid_panel m_aidpanel { "Driving Aid", true };
    scenario_panel m_scenariopanel { "Scenario", true };
    timetable_panel m_timetablepanel { "Timetable", false };
    debug_panel m_debugpanel { "Debug Data", false };
    transcripts_panel m_transcriptspanel { "Transcripts", true }; // voice transcripts
    bool m_paused { false };

};
