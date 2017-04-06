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
#include <queue>

enum class user_command {

    mastercontrollerincrease,
    mastercontrollerincreasefast,
    mastercontrollerdecrease,
    mastercontrollerdecreasefast,
    secondcontrollerincrease,
    secondcontrollerincreasefast,
    secondcontrollerdecrease,
    secondcontrollerdecreasefast,
    independentbrakeincrease,
    independentbrakeincreasefast,
    independentbrakedecrease,
    independentbrakedecreasefast,
    independentbrakebailoff,
    trainbrakeincrease,
    trainbrakedecrease,
    trainbrakecharging,
    trainbrakerelease,
    trainbrakefirstservice,
    trainbrakeservice,
    trainbrakefullservice,
    trainbrakeemergency,
/*
const int k_AntiSlipping = 21;
const int k_Sand = 22;
const int k_Main = 23;
*/
    reverserincrease,
    reverserdecrease,
/*
const int k_Fuse = 26;
const int k_Compressor = 27;
const int k_Converter = 28;
const int k_MaxCurrent = 29;
const int k_CurrentAutoRelay = 30;
const int k_BrakeProfile = 31;
const int k_Czuwak = 32;
const int k_Horn = 33;
const int k_Horn2 = 34;
const int k_FailedEngineCutOff = 35;
*/
    viewturn,
    movevector,
    moveleft,
    moveright,
    moveforward,
    moveback,
    moveup,
    movedown,
    moveleftfast,
    moverightfast,
    moveforwardfast,
    movebackfast,
    moveupfast,
    movedownfast
/*
    moveleftfastest,
    moverightfastest,
    moveforwardfastest,
    movebackfastest,
    moveupfastest,
    movedownfastest
*/
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
const int k_OpenLeft = 49;
const int k_OpenRight = 50;
const int k_CloseLeft = 51;
const int k_CloseRight = 52;
const int k_DepartureSignal = 53;
const int k_PantFrontUp = 54;
const int k_PantRearUp = 55;
const int k_PantFrontDown = 56;
const int k_PantRearDown = 57;
const int k_Heating = 58;
// const int k_FreeFlyMode= 59;
const int k_LeftSign = 60;
const int k_UpperSign = 61;
const int k_RightSign = 62;
const int k_SmallCompressor = 63;
const int k_StLinOff = 64;
const int k_CurrentNext = 65;
const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;
const int k_Active = 71;
const int k_Battery = 72;
const int k_WalkMode = 73;
int const k_DimHeadlights = 74;
*/

};

enum class command_target {

    userinterface,
    simulation,
/*
    // NOTE: there's no need for consist- and unit-specific commands at this point, but it's a possibility.
    // since command targets are mutually exclusive these don't reduce ranges for individual vehicles etc
    consist = 0x4000,
    unit    = 0x8000,
*/
    // values are combined with object id. 0xffff objects of each type should be quite enough ("for everyone")
    vehicle = 0x10000,
    signal  = 0x20000,
    entity  = 0x40000
};

struct command_data {

    user_command command;
    int action; // press, repeat or release
    std::uint64_t param1;
    std::uint64_t param2;
    double time_delta;
};

// command_queues: collects and holds commands from input sources, for processing by their intended recipients
// NOTE: this won't scale well.
// TODO: turn it into a dispatcher and build individual command sequences into the items, where they can be examined without lookups

class command_queue {

public:
// methods
    // posts specified command for specified recipient
    void
        push( command_data const &Command, std::size_t const Recipient );
    // retrieves oldest posted command for specified recipient, if any. returns: true on retrieval, false if there's nothing to retrieve
    bool
        pop( command_data &Command, std::size_t const Recipient );

private:
// types
    typedef std::queue<command_data> commanddata_sequence;
    typedef std::unordered_map<std::size_t, commanddata_sequence> commanddatasequence_map;
// members
    commanddatasequence_map m_commands;

};

// NOTE: simulation should be a (light) wrapper rather than namespace so we could potentially instance it,
//       but realistically it's not like we're going to run more than one simulation at a time
namespace simulation {

extern command_queue Commands;

}

// command_relay: composite class component, passes specified command to appropriate command stack

class command_relay {

public:
// constructors
    command_relay();
// methods
    // posts specified command for the specified recipient
    // TODO: replace uint16_t with recipient handle, based on item id
    void
        post( user_command const Command, std::uint64_t const Param1, std::uint64_t const Param2, int const Action, std::uint16_t const Recipient ) const;
private:
// types
    typedef std::unordered_map<user_command, command_target> commandtarget_map;
// members
    commandtarget_map m_targets;
#ifdef _DEBUG
    std::vector<std::string> m_commandnames;
#endif
};

//---------------------------------------------------------------------------
