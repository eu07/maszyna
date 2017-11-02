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
    mucurrentindicatorothersourceactivate,
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
    manualbrakeincrease,
    manualbrakedecrease,
    alarmchaintoggle,
    wheelspinbrakeactivate,
    sandboxactivate,
    reverserincrease,
    reverserdecrease,
    linebreakertoggle,
    convertertoggle,
    convertertogglelocal,
    converteroverloadrelayreset,
    compressortoggle,
    compressortogglelocal,
    motoroverloadrelaythresholdtoggle,
    motoroverloadrelayreset,
    notchingrelaytoggle,
    epbrakecontroltoggle,
    brakeactingspeedincrease,
    brakeactingspeeddecrease,
    mubrakingindicatortoggle,
    alerteracknowledge,
    hornlowactivate,
    hornhighactivate,
    radiotoggle,
    radiostoptest,
/*
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
    movedownfast,
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
*/
    doortoggleleft,
    doortoggleright,
    departureannounce,
    doorlocktoggle,
    pantographcompressorvalvetoggle,
    pantographcompressoractivate,
    pantographtogglefront,
    pantographtogglerear,
    pantographlowerall,
    heatingtoggle,
/*
// const int k_FreeFlyMode= 59;
*/
    headlighttoggleleft,
    headlighttoggleright,
    headlighttoggleupper,
    redmarkertoggleleft,
    redmarkertoggleright,
    headlighttogglerearleft,
    headlighttogglerearright,
    headlighttogglerearupper,
    redmarkertogglerearleft,
    redmarkertogglerearright,
    headlightsdimtoggle,
    motorconnectorsopen,
    motordisconnect,
    interiorlighttoggle,
    interiorlightdimtoggle,
    instrumentlighttoggle,
/*
const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;
const int k_Active = 71;
*/
    generictoggle0,
    generictoggle1,
    generictoggle2,
    generictoggle3,
    generictoggle4,
    generictoggle5,
    generictoggle6,
    generictoggle7,
    generictoggle8,
    generictoggle9,
    batterytoggle,
/*
const int k_WalkMode = 73;
*/
    none = -1
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

struct command_description {

    std::string name;
    command_target target;
};

struct command_data {

    user_command command;
    int action; // press, repeat or release
    std::uint64_t param1;
    std::uint64_t param2;
    enum desired_state_t
    {
        TOGGLE,
        OFF,
        ON
    };
    desired_state_t desired_state;
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

typedef std::vector<command_description> commanddescription_sequence;

extern command_queue Commands;
// TODO: add name to command map, and wrap these two into helper object
extern commanddescription_sequence Commands_descriptions;

}

// command_relay: composite class component, passes specified command to appropriate command stack

class command_relay {

public:
// constructors
// methods
    // posts specified command for the specified recipient
    // TODO: replace uint16_t with recipient handle, based on item id
    void
        post( user_command const Command, std::uint64_t const Param1, std::uint64_t const Param2,
              int const Action, std::uint16_t const Recipient,
              command_data::desired_state_t state = command_data::TOGGLE ) const;
private:
// types
// members
};

//---------------------------------------------------------------------------
