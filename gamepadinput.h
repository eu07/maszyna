/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <vector>
#include "command.h"

class gamepad_input {

public:
// constructors
    gamepad_input();

// methods
    // checks state of the controls and sends issued commands
    bool
        init();
    void
        poll();

private:
// types
    enum gamepad_button {
        a,
        b,
        x,
        y,
        left_shoulder,
        right_shoulder,
        back,
        start,
        left_sticker,
        right_sticker,
        dpad_up,
        dpad_right,
        dpad_down,
        dpad_left
    };
    enum gamepad_axes {
        leftstick_x,
        leftstick_y,
        rightstick_x,
        rightstick_y,
        lefttrigger,
        righttrigger
    };
    enum class control_mode {
        entity = -1,
        vehicle_mastercontroller,
        vehicle_trainbrake,
        vehicle_secondarycontroller,
        vehicle_independentbrake
    };
    typedef std::vector<float> float_sequence;
    typedef std::vector<char> char_sequence;
    typedef std::vector< std::pair< user_command, user_command > > commandpair_sequence;

// methods
    void on_button( gamepad_button const Button, int const Action );
    void process_axes( glm::vec2 Leftstick, glm::vec2 const &Rightstick, glm::vec2 const &Triggers );
    void process_axis( float const Value, float const Previousvalue, float const Multiplier, user_command Command, std::uint16_t const Recipient );
    void process_axis( float const Value, float const Previousvalue, float const Multiplier, user_command Command1, user_command Command2, /*user_command Command3,*/ std::uint16_t const Recipient );
    void process_mode( float const Value, std::uint16_t const Recipient );

// members
    command_relay m_relay;
    float_sequence m_axes;
    char_sequence m_buttons;
    int m_deviceid{ -1 };
    control_mode m_mode{ control_mode::entity };
    commandpair_sequence m_modecommands; // sets of commands issued depending on the active control mode
    float m_deadzone{ 0.15f }; // TODO: allow to configure this
    glm::vec2 m_leftstick;
    glm::vec2 m_rightstick;
    glm::vec2 m_triggers;
    double m_modeaccumulator; // used to throttle command input rate for vehicle controls
};

//---------------------------------------------------------------------------
