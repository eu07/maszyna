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
// members
    ui_panel UIHeader { 20, 20 }; // header ui panel
    ui_panel UITable { 20, 100 };// schedule or scan table
    ui_panel UITranscripts { 85, 600 }; // voice transcripts
    int tprev { 0 }; // poprzedni czas
    double VelPrev { 0.0 }; // poprzednia prędkość
    double Acc { 0.0 }; // przyspieszenie styczne

};
