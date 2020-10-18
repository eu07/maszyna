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

namespace input {

extern std::array<char, GLFW_KEY_LAST + 1> keys;
extern bool key_alt;
extern bool key_ctrl;
extern bool key_shift;

}

namespace ui {
class keymapper_panel;
}

class keyboard_input {
    friend class ui::keymapper_panel;

public:
// types
    using bindingsetup_sequence = std::map<user_command, int>;

	enum keymodifier : int {

		shift   = 0x10000,
		control = 0x20000
	};

// constructors
    keyboard_input() = default;

// destructor
    virtual ~keyboard_input() = default;

// methods
    virtual
    bool
        init() { return true; }
    bool
        key( int const Key, int const Action );
    int
        key( int const Key ) const;
    void
        poll();
    inline
    user_command const
        command() const {
            return m_command; }
    bindingsetup_sequence&
        bindings() {
            return m_bindingsetups; }
    int
        binding( user_command const Command ) const;
    std::string
        binding_hint( user_command const Command ) const;
    void
        dump_bindings();

// members
	static std::unordered_map<int, std::string> keytonamemap;

protected:
// methods
    virtual
    void
        default_bindings() = 0;
    bool
        recall_bindings();
    void
        bind();

// members
	bindingsetup_sequence m_bindingsetups;

private:
// types
    using usercommand_map = std::unordered_map<int, user_command>;

    struct bindings_cache {

        int forward { -1 };
        int back { -1 };
        int left { -1 };
        int right { -1 };
        int up { -1 };
        int down { -1 };
    };

// methods
    bool
        is_movement_key( int const Key ) const;

// members
    user_command m_command { user_command::none }; // last, if any, issued command
    usercommand_map m_bindings;
    command_relay m_relay;
    bindings_cache m_bindingscache;
    glm::vec2 m_movementhorizontal { 0.f };
    float m_movementvertical { 0.f };
};

//---------------------------------------------------------------------------
