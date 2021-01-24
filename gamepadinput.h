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
#include "parser.h"

class gamepad_input {

public:
// constructors
    gamepad_input() = default;

// methods
    // checks state of the controls and sends issued commands
    bool
        init();
    void
        poll();

private:
// types
/*
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
*/
    enum class input_type {
        threestate, // two commands, mapped to positive and negative axis value respectively; press and release events on state change
        impulse, // one command; press event when set, release when cleared
        value, // one command; press event, axis value passed as specified param (stored as second 'user_command')
        value_invert // the passed value is additionally multiplied by -1
    };

    struct input_button {
        unsigned char state; // last polled state
        int mode; // associated control mode
        user_command binding; // associated command
    };

    struct input_axis {
        float state; // last polled state
        float accumulator; // multipurpose helper variable
        std::unordered_map< int, std::tuple< input_type, user_command, user_command > > bindings; // associated commands for respective modes
    };

    using inputbutton_sequence = std::vector<input_button>;
    using inputaxis_sequence = std::vector<input_axis>;
// methods
    bool recall_bindings();
    void bind( std::vector< std::reference_wrapper<user_command> > &Targets, cParser &Input, std::unordered_map<std::string, user_command> const &Translator, std::string const Point );
    void on_button( int const Button, int const Action );
    void process_axes();

// members
    command_relay m_relay;
    inputbutton_sequence m_inputbuttons;
    inputaxis_sequence m_inputaxes;
    int m_deviceid{ -1 };
    int m_mode{ -1 }; // currently active control mode
    float m_deadzone{ 0.15f }; // TODO: allow to configure this
//    double m_modeaccumulator{ 0.0 }; // used to throttle command input rate for vehicle controls
    std::unordered_map< user_command, std::tuple<double, double> > m_lastcommandparams; // cached parameters for last issued command of given type
};

//---------------------------------------------------------------------------
