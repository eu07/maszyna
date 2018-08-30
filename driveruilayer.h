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
    // updates state of UI elements
    void
        update() override;

private:
// methods
    // render() subclass details
    void
        render_() override;
// members
    drivingaid_panel m_aidpanel { "Driving Aid", true };
    timetable_panel m_timetablepanel { "Timetable", false };
    debug_panel m_debugpanel { "Debug Data", false };
    transcripts_panel m_transcriptspanel { "Transcripts", true }; // voice transcripts

};
