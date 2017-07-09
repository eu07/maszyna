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
        mouse( double const Mousex, double const Mousey );
    void
        mouse( int const Button, int const Action );
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

    struct mouse_commands {

        user_command left;
        user_command right;

        mouse_commands( user_command const Left, user_command const Right ):
                                      left(Left),             right(Right)
        {}
    };

    typedef std::unordered_map<std::string, mouse_commands> controlcommands_map;

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
        update_movement( int const Key, int const Action );

// members
    commandsetup_sequence m_commands;
    usercommand_map m_bindings;
    controlcommands_map m_mousecommands;
    user_command m_mousecommandleft { user_command::none }; // last if any command issued with mouse
    user_command m_mousecommandright { user_command::none };
    command_relay m_relay;
    bool m_shift{ false };
    bool m_ctrl{ false };
    double m_updateaccumulator { 0.0 };
    bindings_cache m_bindingscache;
    std::array<char, GLFW_KEY_LAST + 1> m_keys;
};

//---------------------------------------------------------------------------
