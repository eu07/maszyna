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
    // potentially processes provided input key. returns: true if the input was processed, false otherwise
    bool
        on_key( int const Key, int const Action ) override;
    // potentially processes provided mouse movement. returns: true if the input was processed, false otherwise
    bool
        on_cursor_pos( double const Horizontal, double const Vertical ) override;
    // potentially processes provided mouse button. returns: true if the input was processed, false otherwise
    bool
        on_mouse_button( int const Button, int const Action ) override;
    // updates state of UI elements
    void
        update() override;

protected:
    void render_menu_contents() override;

private:
// methods
    // sets visibility of the cursor
    void
        set_cursor( bool const Visible );
    // render() subclass details
    void
        render_() override;

// members
    drivingaid_panel m_aidpanel { "Driving Aid", true };
    timetable_panel m_timetablepanel { "Timetable", false };
    debug_panel m_debugpanel { "Debug Data", false };
    transcripts_panel m_transcriptspanel { "Transcripts", true }; // voice transcripts
    bool m_paused { false };
	bool m_pause_modal_opened { false };
};
