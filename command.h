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
#include <unordered_set>

enum class user_command {

	aidriverenable = 0,
    aidriverdisable,
    jointcontrollerset,
    mastercontrollerincrease,
    mastercontrollerincreasefast,
    mastercontrollerdecrease,
    mastercontrollerdecreasefast,
    mastercontrollerset,
    secondcontrollerincrease,
    secondcontrollerincreasefast,
    secondcontrollerdecrease,
    secondcontrollerdecreasefast,
    secondcontrollerset,
    mucurrentindicatorothersourceactivate,
    independentbrakeincrease,
    independentbrakeincreasefast,
    independentbrakedecrease,
    independentbrakedecreasefast,
    independentbrakeset,
    independentbrakebailoff,
	universalbrakebutton1,
	universalbrakebutton2,
	universalbrakebutton3,
    trainbrakeincrease,
    trainbrakedecrease,
    trainbrakeset,
    trainbrakecharging,
    trainbrakerelease,
    trainbrakefirstservice,
    trainbrakeservice,
    trainbrakefullservice,
    trainbrakehandleoff,
    trainbrakeemergency,
    trainbrakebasepressureincrease,
    trainbrakebasepressuredecrease,
    trainbrakebasepressurereset,
    trainbrakeoperationtoggle,
    manualbrakeincrease,
    manualbrakedecrease,
    alarmchaintoggle,
	alarmchainenable,
	alarmchaindisable,
    wheelspinbrakeactivate,
    sandboxactivate,
	autosandboxtoggle,
	autosandboxactivate,
	autosandboxdeactivate,
    reverserincrease,
    reverserdecrease,
    reverserforwardhigh,
    reverserforward,
    reverserneutral,
    reverserbackward,
    waterpumpbreakertoggle,
    waterpumpbreakerclose,
    waterpumpbreakeropen,
    waterpumptoggle,
    waterpumpenable,
    waterpumpdisable,
    waterheaterbreakertoggle,
    waterheaterbreakerclose,
    waterheaterbreakeropen,
    waterheatertoggle,
    waterheaterenable,
    waterheaterdisable,
    watercircuitslinktoggle,
    watercircuitslinkenable,
    watercircuitslinkdisable,
    fuelpumptoggle,
    fuelpumpenable,
    fuelpumpdisable,
    oilpumptoggle,
    oilpumpenable,
    oilpumpdisable,
    linebreakertoggle,
    linebreakeropen,
    linebreakerclose,
    convertertoggle,
    converterenable,
    converterdisable,
    convertertogglelocal,
    converteroverloadrelayreset,
    compressortoggle,
    compressorenable,
    compressordisable,
    compressortogglelocal,
	compressorpresetactivatenext,
	compressorpresetactivateprevious,
	compressorpresetactivatedefault,
    motoroverloadrelaythresholdtoggle,
    motoroverloadrelaythresholdsetlow,
    motoroverloadrelaythresholdsethigh,
    motoroverloadrelayreset,
    universalrelayreset1,
    universalrelayreset2,
    universalrelayreset3,
    notchingrelaytoggle,
    epbrakecontroltoggle,
	trainbrakeoperationmodeincrease,
	trainbrakeoperationmodedecrease,
    brakeactingspeedincrease,
    brakeactingspeeddecrease,
    brakeactingspeedsetcargo,
    brakeactingspeedsetpassenger,
    brakeactingspeedsetrapid,
    brakeloadcompensationincrease,
    brakeloadcompensationdecrease,
    mubrakingindicatortoggle,
    alerteracknowledge,
	cabsignalacknowledge,
    hornlowactivate,
    hornhighactivate,
    whistleactivate,
    radiotoggle,
    radioenable,
    radiodisable,
    radiochannelincrease,
    radiochanneldecrease,
    radiochannelset,
    radiostopsend,
    radiostopenable,
    radiostopdisable,
    radiostoptest,
    radiocall3send,
	radiovolumeincrease,
	radiovolumedecrease,
	radiovolumeset,
    cabchangeforward,
    cabchangebackward,

    viewturn,
    movehorizontal,
    movehorizontalfast,
    movevertical,
    moveverticalfast,
    moveleft,
    moveright,
    moveforward,
    moveback,
    moveup,
    movedown,

    nearestcarcouplingincrease,
    nearestcarcouplingdisconnect,
    nearestcarcoupleradapterattach,
    nearestcarcoupleradapterremove,
    occupiedcarcouplingdisconnect,
	occupiedcarcouplingdisconnectback,
    doortoggleleft,
    doortoggleright,
    doorpermitleft,
    doorpermitright,
    doorpermitpresetactivatenext,
    doorpermitpresetactivateprevious,
    dooropenleft,
    dooropenright,
    dooropenall,
    doorcloseleft,
    doorcloseright,
    doorcloseall,
    doorsteptoggle,
    doormodetoggle,
	mirrorstoggle,
    departureannounce,
    doorlocktoggle,
    pantographcompressorvalvetoggle,
    pantographcompressorvalveenable,
    pantographcompressorvalvedisable,
    pantographcompressoractivate,
    pantographtogglefront,
    pantographtogglerear,
    pantographraisefront,
    pantographraiserear,
    pantographlowerfront,
    pantographlowerrear,
    pantographlowerall,
    pantographselectnext,
    pantographselectprevious,
    pantographtoggleselected,
    pantographraiseselected,
    pantographlowerselected,
    pantographvalvesupdate,
    pantographvalvesoff,
    heatingtoggle,
    heatingenable,
    heatingdisable,
    lightspresetactivatenext,
    lightspresetactivateprevious,
    headlighttoggleleft,
    headlightenableleft,
    headlightdisableleft,
    headlighttoggleright,
    headlightenableright,
    headlightdisableright,
    headlighttoggleupper,
    headlightenableupper,
    headlightdisableupper,
    redmarkertoggleleft,
    redmarkerenableleft,
    redmarkerdisableleft,
    redmarkertoggleright,
    redmarkerenableright,
    redmarkerdisableright,
    headlighttogglerearleft,
    headlightenablerearleft,
    headlightdisablerearleft,
    headlighttogglerearright,
    headlightenablerearright,
    headlightdisablerearright,
    headlighttogglerearupper,
    headlightenablerearupper,
    headlightdisablerearupper,
    redmarkertogglerearleft,
    redmarkerenablerearleft,
    redmarkerdisablerearleft,
    redmarkertogglerearright,
    redmarkerenablerearright,
    redmarkerdisablerearright,
    redmarkerstoggle,
    endsignalstoggle,
    headlightsdimtoggle,
    headlightsdimenable,
    headlightsdimdisable,
    motorconnectorsopen,
    motorconnectorsclose,
    motordisconnect,
    interiorlighttoggle,
    interiorlightenable,
    interiorlightdisable,
    interiorlightdimtoggle,
    interiorlightdimenable,
    interiorlightdimdisable,
    compartmentlightstoggle,
    compartmentlightsenable,
    compartmentlightsdisable,
    instrumentlighttoggle,
    instrumentlightenable,
    instrumentlightdisable,
    dashboardlighttoggle,
    dashboardlightenable,
    dashboardlightdisable,
    timetablelighttoggle,
    timetablelightenable,
    timetablelightdisable,
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
    batteryenable,
    batterydisable,
	cabactivationtoggle,
	cabactivationenable,
	cabactivationdisable,
    motorblowerstogglefront,
    motorblowerstogglerear,
    motorblowersdisableall,
    coolingfanstoggle,
    tempomattoggle,
	springbraketoggle,
	springbrakeenable,
	springbrakedisable,
	springbrakeshutofftoggle,
	springbrakeshutoffenable,
	springbrakeshutoffdisable,
	springbrakerelease,
    distancecounteractivate,
	speedcontrolincrease,
	speedcontroldecrease,
	speedcontrolpowerincrease,
	speedcontrolpowerdecrease,
	speedcontrolbutton0,
	speedcontrolbutton1,
	speedcontrolbutton2,
	speedcontrolbutton3,
	speedcontrolbutton4,
	speedcontrolbutton5,
	speedcontrolbutton6,
	speedcontrolbutton7,
	speedcontrolbutton8,
	speedcontrolbutton9,
	inverterenable1,
	inverterenable2,
	inverterenable3,
	inverterenable4,
	inverterenable5,
	inverterenable6,
	inverterenable7,
	inverterenable8,
	inverterenable9,
	inverterenable10,
	inverterenable11,
	inverterenable12,
	inverterdisable1,
	inverterdisable2,
	inverterdisable3,
	inverterdisable4,
	inverterdisable5,
	inverterdisable6,
	inverterdisable7,
	inverterdisable8,
	inverterdisable9,
	inverterdisable10,
	inverterdisable11,
	inverterdisable12,
	invertertoggle1,
	invertertoggle2,
	invertertoggle3,
	invertertoggle4,
	invertertoggle5,
	invertertoggle6,
	invertertoggle7,
	invertertoggle8,
	invertertoggle9,
	invertertoggle10,
	invertertoggle11,
	invertertoggle12,
    globalradiostop,
    timejump,
    timejumplarge,
    timejumpsmall,
    setdatetime,
    setweather,
    settemperature,
    vehiclemoveforwards,
    vehiclemovebackwards,
    vehicleboost,
    debugtoggle,
    focuspauseset,
    pausetoggle,
    entervehicle,
    resetconsist,
    fillcompressor,
    consistreleaser,
    queueevent,
    setlight,
    insertmodel,
    deletemodel,
    dynamicmove,
    consistteleport,
    pullalarmchain,
    sendaicommand,
    spawntrainset,
    destroytrainset,
    quitsimulation,

    none = -1
};

enum class command_target {

    userinterface,
/*
    // NOTE: there's no need for consist- and unit-specific commands at this point, but it's a possibility.
    // since command targets are mutually exclusive these don't reduce ranges for individual vehicles etc
    consist = 0x4000,
    unit    = 0x8000,
*/
    // values are combined with object id. 0xffff objects of each type should be quite enough ("for everyone")
    vehicle = 0x10000,
    signal  = 0x20000,
    entity  = 0x40000,
    simulation = 0x80000
};

enum class command_mode {
	oneoff,
	continuous
};

struct command_description {
    std::string name;
    command_target target;
	command_mode mode;
};

struct command_data {

    user_command command;
    int action; // press, repeat or release
    double param1;
    double param2;
    double time_delta;

	bool freefly;
	glm::vec3 location;

    std::string payload;
};

// command_queues: collects and holds commands from input sources, for processing by their intended recipients
// NOTE: this won't scale well.
// TODO: turn it into a dispatcher and build individual command sequences into the items, where they can be examined without lookups

class command_queue {

public:
// types
	typedef std::deque<command_data> commanddata_sequence;
	typedef std::unordered_map<uint32_t, commanddata_sequence> commands_map;
// methods
	// posts specified command for specified recipient into m_intercept_queue
	void
	    push( command_data const &Command, uint32_t const Recipient );
    // retrieves oldest posted command for specified recipient, if any. returns: true on retrieval, false if there's nothing to retrieve
    bool
        pop( command_data &Command, uint32_t const Recipient );
    // generates active continuous commands
    void
        update();
    // checks if given command must be scheduled on server
	bool
	    is_network_target(const uint32_t Recipient);

	// pops commands from intercept queue
	commands_map pop_intercept_queue();

	// pushes commands into main queue
	void push_commands(const commands_map &commands);

private:
// members
	// contains command ready to execution
	commands_map m_commands;

	// contains intercepted commands to be read by application layer
	commands_map m_intercept_queue;

	void push_direct( command_data const &Command, uint32_t const Recipient );

	// hash operator for m_active_continuous
	struct command_set_hash {
		uint64_t operator() (const std::pair<user_command, uint32_t> &pair) const {
			return ((uint64_t)pair.first << 32) | ((uint64_t) pair.second);
		}
	};

	// currently pressed continuous commands
	std::unordered_set<std::pair<user_command, uint32_t>, command_set_hash> m_active_continuous;
};

template<typename A, typename B>
void add_to_dequemap(std::unordered_map<A, std::deque<B>> &lhs, const std::unordered_map<A, std::deque<B>> &rhs) {
	for (auto const &kv : rhs) {
		auto lookup = lhs.emplace(kv.first, std::deque<B>());
		for (B const &data : kv.second)
			lookup.first->second.emplace_back(data);
	}
}

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
// methods
	// posts specified command for the specified recipient
    void
	    post(user_command const Command, double const Param1, double const Param2,
	        int const Action, uint16_t Recipient, glm::vec3 Position = glm::vec3(0.0f) , const std::string *Payload = nullptr) const;
private:
// types
// members
};

//---------------------------------------------------------------------------
