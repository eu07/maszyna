/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "editorkeyboardinput.h"

bool
editorkeyboard_input::init() {

    default_bindings();
    // TODO: re-enable after mode-specific binding import is in place
    // return recall_bindings();
	bind();

    return true;
}

void
editorkeyboard_input::default_bindings() {

    m_bindingsetups = {
        { user_command::moveleft, {GLFW_KEY_LEFT, "Move left"} },
        { user_command::moveright, {GLFW_KEY_RIGHT, "Move right"} },
        { user_command::moveforward, {GLFW_KEY_UP, "Move forwards"} },
        { user_command::moveback, {GLFW_KEY_DOWN, "Move backwards"} },
        { user_command::moveup, {GLFW_KEY_PAGE_UP, "Move up"} },
        { user_command::movedown, {GLFW_KEY_PAGE_DOWN, "Move down"} },
    };
}

//---------------------------------------------------------------------------
