/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"

// a simple station, performs freight and passenger exchanges with visiting consists
class basic_station {

public:
// methods
    // exchanges load with consist attached to specified vehicle, operating on specified schedule; returns: time needed for exchange, in seconds
    double
        update_load( TDynamicObject *First, Mtable::TTrainParameters &Schedule );
};
