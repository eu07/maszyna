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
#include "command.h"

class keyboard_input {

public:
// constructors
    keyboard_input() { default_bindings(); }

// methods
    void
        recall_bindings();
    bool
        key( int const Key, int const Action );
    void
        mouse( double const Mousex, double const Mousey );

private:
// types
    enum keymodifier : int {

        shift   = 0x10000,
        control = 0x20000
    };

    struct command_setup {

        std::string name;
        command_target target;
        int binding;
    };

    typedef std::vector<command_setup> commandsetup_sequence;
    typedef std::unordered_map<int, user_command> usercommand_map;

// methods
    void
        default_bindings();
    void
        bind();

// members
    commandsetup_sequence m_commands;
    usercommand_map m_bindings;
    command_relay m_relay;
    bool m_shift{ false };
    bool m_ctrl{ false };
};

//---------------------------------------------------------------------------
