/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "sound.h"

namespace simulation {

using sound_overridemap = std::unordered_map<std::string, std::string>;

extern sound_overridemap Sound_overrides;
extern sound_table Sounds;

} // simulation

//---------------------------------------------------------------------------
