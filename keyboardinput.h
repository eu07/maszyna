/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <unordered_map>
#include <array>
#include "command.h"

class keyboard_input {

public:
// constructors
    keyboard_input() { default_bindings(); }

// methods
    bool
        init() { return recall_bindings(); }
    bool
        recall_bindings();
    bool
        key( int const Key, int const Action );
    void
        poll();

private:
// types
    enum keymodifier : int {

        shift   = 0x10000,
        control = 0x20000
    };

    struct command_setup {

        int binding;
    };

    typedef std::vector<command_setup> commandsetup_sequence;
    typedef std::unordered_map<int, user_command> usercommand_map;

    struct bindings_cache {

        int forward{ -1 };
        int back{ -1 };
        int left{ -1 };
        int right{ -1 };
        int up{ -1 };
        int down{ -1 };
    };

// methods
    void
        default_bindings();
    void
        bind();
    bool
        is_movement_key( int const Key ) const;

// members
    commandsetup_sequence m_commands;
    usercommand_map m_bindings;
    command_relay m_relay;
    bool m_shift{ false };
    bool m_ctrl{ false };
    bindings_cache m_bindingscache;
    glm::vec2 m_movementhorizontal;
    float m_movementvertical;
    std::array<char, GLFW_KEY_LAST + 1> m_keys;
};

//---------------------------------------------------------------------------
