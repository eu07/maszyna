/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

// basic approximation of a fuel pump
// TODO: fuel consumption, optional automatic engine start after activation
struct fuel_pump {

    bool is_enabled { false }; // device is allowed/requested to operate
    bool is_active { false }; // device is working
};

//---------------------------------------------------------------------------
