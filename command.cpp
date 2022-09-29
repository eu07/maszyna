/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "command.h"

#include "Globals.h"
#include "Logs.h"
#include "Timer.h"
#include "utilities.h"
#include "simulation.h"
#include "Train.h"

namespace simulation {

command_queue Commands;
commanddescription_sequence Commands_descriptions = {

	{ "aidriverenable", command_target::vehicle, command_mode::oneoff },
	{ "aidriverdisable", command_target::vehicle, command_mode::oneoff },
	{ "jointcontrollerset", command_target::vehicle, command_mode::oneoff },
	{ "mastercontrollerincrease", command_target::vehicle, command_mode::oneoff },
	{ "mastercontrollerincreasefast", command_target::vehicle, command_mode::oneoff },
	{ "mastercontrollerdecrease", command_target::vehicle, command_mode::oneoff },
	{ "mastercontrollerdecreasefast", command_target::vehicle, command_mode::oneoff },
	{ "mastercontrollerset", command_target::vehicle, command_mode::oneoff },
	{ "secondcontrollerincrease", command_target::vehicle, command_mode::oneoff },
	{ "secondcontrollerincreasefast", command_target::vehicle, command_mode::oneoff },
	{ "secondcontrollerdecrease", command_target::vehicle, command_mode::oneoff },
	{ "secondcontrollerdecreasefast", command_target::vehicle, command_mode::oneoff },
	{ "secondcontrollerset", command_target::vehicle, command_mode::oneoff },
	{ "mucurrentindicatorothersourceactivate", command_target::vehicle, command_mode::oneoff },
	{ "independentbrakeincrease", command_target::vehicle, command_mode::continuous },
	{ "independentbrakeincreasefast", command_target::vehicle, command_mode::oneoff },
	{ "independentbrakedecrease", command_target::vehicle, command_mode::continuous },
	{ "independentbrakedecreasefast", command_target::vehicle, command_mode::oneoff },
	{ "independentbrakeset", command_target::vehicle, command_mode::oneoff },
	{ "independentbrakebailoff", command_target::vehicle, command_mode::oneoff },
	{ "universalbrakebutton1", command_target::vehicle, command_mode::oneoff },
	{ "universalbrakebutton2", command_target::vehicle, command_mode::oneoff },
	{ "universalbrakebutton3", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakeincrease", command_target::vehicle, command_mode::continuous },
	{ "trainbrakedecrease", command_target::vehicle, command_mode::continuous },
	{ "trainbrakeset", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakecharging", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakerelease", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakefirstservice", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakeservice", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakefullservice", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakehandleoff", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakeemergency", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakebasepressureincrease", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakebasepressuredecrease", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakebasepressurereset", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakeoperationtoggle", command_target::vehicle, command_mode::oneoff },
	{ "manualbrakeincrease", command_target::vehicle, command_mode::oneoff },
	{ "manualbrakedecrease", command_target::vehicle, command_mode::oneoff },
	{ "alarmchaintoggle", command_target::vehicle, command_mode::oneoff },
    { "alarmchainenable", command_target::vehicle, command_mode::oneoff},
    { "alarmchaindisable", command_target::vehicle, command_mode::oneoff},
	{ "wheelspinbrakeactivate", command_target::vehicle, command_mode::oneoff },
	{ "sandboxactivate", command_target::vehicle, command_mode::oneoff },
	{ "autosandboxtoggle", command_target::vehicle, command_mode::oneoff },
	{ "autosandboxactivate", command_target::vehicle, command_mode::oneoff },
	{ "autosandboxdeactivate", command_target::vehicle, command_mode::oneoff },
	{ "reverserincrease", command_target::vehicle, command_mode::oneoff },
	{ "reverserdecrease", command_target::vehicle, command_mode::oneoff },
	{ "reverserforwardhigh", command_target::vehicle, command_mode::oneoff },
	{ "reverserforward", command_target::vehicle, command_mode::oneoff },
	{ "reverserneutral", command_target::vehicle, command_mode::oneoff },
	{ "reverserbackward", command_target::vehicle, command_mode::oneoff },
	{ "waterpumpbreakertoggle", command_target::vehicle, command_mode::oneoff },
	{ "waterpumpbreakerclose", command_target::vehicle, command_mode::oneoff },
	{ "waterpumpbreakeropen", command_target::vehicle, command_mode::oneoff },
	{ "waterpumptoggle", command_target::vehicle, command_mode::oneoff },
	{ "waterpumpenable", command_target::vehicle, command_mode::oneoff },
	{ "waterpumpdisable", command_target::vehicle, command_mode::oneoff },
	{ "waterheaterbreakertoggle", command_target::vehicle, command_mode::oneoff },
	{ "waterheaterbreakerclose", command_target::vehicle, command_mode::oneoff },
	{ "waterheaterbreakeropen", command_target::vehicle, command_mode::oneoff },
	{ "waterheatertoggle", command_target::vehicle, command_mode::oneoff },
	{ "waterheaterenable", command_target::vehicle, command_mode::oneoff },
	{ "waterheaterdisable", command_target::vehicle, command_mode::oneoff },
	{ "watercircuitslinktoggle", command_target::vehicle, command_mode::oneoff },
	{ "watercircuitslinkenable", command_target::vehicle, command_mode::oneoff },
	{ "watercircuitslinkdisable", command_target::vehicle, command_mode::oneoff },
	{ "fuelpumptoggle", command_target::vehicle, command_mode::oneoff },
	{ "fuelpumpenable", command_target::vehicle, command_mode::oneoff },
	{ "fuelpumpdisable", command_target::vehicle, command_mode::oneoff },
	{ "oilpumptoggle", command_target::vehicle, command_mode::oneoff },
	{ "oilpumpenable", command_target::vehicle, command_mode::oneoff },
	{ "oilpumpdisable", command_target::vehicle, command_mode::oneoff },
	{ "linebreakertoggle", command_target::vehicle, command_mode::oneoff },
	{ "linebreakeropen", command_target::vehicle, command_mode::oneoff },
	{ "linebreakerclose", command_target::vehicle, command_mode::oneoff },
	{ "convertertoggle", command_target::vehicle, command_mode::oneoff },
	{ "converterenable", command_target::vehicle, command_mode::oneoff },
	{ "converterdisable", command_target::vehicle, command_mode::oneoff },
	{ "convertertogglelocal", command_target::vehicle, command_mode::oneoff },
	{ "converteroverloadrelayreset", command_target::vehicle, command_mode::oneoff },
	{ "compressortoggle", command_target::vehicle, command_mode::oneoff },
	{ "compressorenable", command_target::vehicle, command_mode::oneoff },
	{ "compressordisable", command_target::vehicle, command_mode::oneoff },
	{ "compressortogglelocal", command_target::vehicle, command_mode::oneoff },
	{ "compressorpresetactivatenext", command_target::vehicle, command_mode::oneoff },
	{ "compressorpresetactivateprevious", command_target::vehicle, command_mode::oneoff },
	{ "compressorpresetactivatedefault", command_target::vehicle, command_mode::oneoff },
	{ "motoroverloadrelaythresholdtoggle", command_target::vehicle, command_mode::oneoff },
	{ "motoroverloadrelaythresholdsetlow", command_target::vehicle, command_mode::oneoff },
	{ "motoroverloadrelaythresholdsethigh", command_target::vehicle, command_mode::oneoff },
	{ "motoroverloadrelayreset", command_target::vehicle, command_mode::oneoff },
	{ "universalrelayreset1", command_target::vehicle, command_mode::oneoff },
	{ "universalrelayreset2", command_target::vehicle, command_mode::oneoff },
	{ "universalrelayreset3", command_target::vehicle, command_mode::oneoff },
	{ "notchingrelaytoggle", command_target::vehicle, command_mode::oneoff },
	{ "epbrakecontroltoggle", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakeoperationmodeincrease", command_target::vehicle, command_mode::oneoff },
	{ "trainbrakeoperationmodedecrease", command_target::vehicle, command_mode::oneoff },
	{ "brakeactingspeedincrease", command_target::vehicle, command_mode::oneoff },
	{ "brakeactingspeeddecrease", command_target::vehicle, command_mode::oneoff },
	{ "brakeactingspeedsetcargo", command_target::vehicle, command_mode::oneoff },
	{ "brakeactingspeedsetpassenger", command_target::vehicle, command_mode::oneoff },
	{ "brakeactingspeedsetrapid", command_target::vehicle, command_mode::oneoff },
	{ "brakeloadcompensationincrease", command_target::vehicle, command_mode::oneoff },
	{ "brakeloadcompensationdecrease", command_target::vehicle, command_mode::oneoff },
	{ "mubrakingindicatortoggle", command_target::vehicle, command_mode::oneoff },
	{ "alerteracknowledge", command_target::vehicle, command_mode::oneoff },
	{ "cabsignalacknowledge", command_target::vehicle, command_mode::oneoff },
	{ "hornlowactivate", command_target::vehicle, command_mode::oneoff },
	{ "hornhighactivate", command_target::vehicle, command_mode::oneoff },
	{ "whistleactivate", command_target::vehicle, command_mode::oneoff },
	{ "radiotoggle", command_target::vehicle, command_mode::oneoff },
	{ "radioenable", command_target::vehicle, command_mode::oneoff },
	{ "radiodisable", command_target::vehicle, command_mode::oneoff },
	{ "radiochannelincrease", command_target::vehicle, command_mode::oneoff },
	{ "radiochanneldecrease", command_target::vehicle, command_mode::oneoff },
	{ "radiochannelset", command_target::vehicle, command_mode::oneoff },
	{ "radiostopsend", command_target::vehicle, command_mode::oneoff },
	{ "radiostopenable", command_target::vehicle, command_mode::oneoff },
	{ "radiostopdisable", command_target::vehicle, command_mode::oneoff },
	{ "radiostoptest", command_target::vehicle, command_mode::oneoff },
	{ "radiocall3send", command_target::vehicle, command_mode::oneoff },
	{ "radiovolumeincrease", command_target::vehicle, command_mode::oneoff },
	{ "radiovolumedecrease", command_target::vehicle, command_mode::oneoff },
	{ "radiovolumeset", command_target::vehicle, command_mode::oneoff },
	{ "cabchangeforward", command_target::vehicle, command_mode::oneoff },
	{ "cabchangebackward", command_target::vehicle, command_mode::oneoff },
	{ "viewturn", command_target::entity, command_mode::oneoff },
	{ "movehorizontal", command_target::entity, command_mode::oneoff },
	{ "movehorizontalfast", command_target::entity, command_mode::oneoff },
	{ "movevertical", command_target::entity, command_mode::oneoff },
	{ "moveverticalfast", command_target::entity, command_mode::oneoff },
	{ "moveleft", command_target::entity, command_mode::oneoff },
	{ "moveright", command_target::entity, command_mode::oneoff },
	{ "moveforward", command_target::entity, command_mode::oneoff },
	{ "moveback", command_target::entity, command_mode::oneoff },
	{ "moveup", command_target::entity, command_mode::oneoff },
	{ "movedown", command_target::entity, command_mode::oneoff },
	{ "nearestcarcouplingincrease", command_target::vehicle, command_mode::oneoff },
	{ "nearestcarcouplingdisconnect", command_target::vehicle, command_mode::oneoff },
	{ "nearestcarcoupleradapterattach", command_target::vehicle, command_mode::oneoff  },
	{ "nearestcarcoupleradapterremove", command_target::vehicle, command_mode::oneoff  },
	{ "occupiedcarcouplingdisconnect", command_target::vehicle, command_mode::oneoff  },
	{ "occupiedcarcouplingdisconnectback", command_target::vehicle, command_mode::oneoff  },
	{ "doortoggleleft", command_target::vehicle, command_mode::oneoff },
	{ "doortoggleright", command_target::vehicle, command_mode::oneoff },
	{ "doorpermitleft", command_target::vehicle, command_mode::oneoff },
	{ "doorpermitright", command_target::vehicle, command_mode::oneoff },
	{ "doorpermitpresetactivatenext", command_target::vehicle, command_mode::oneoff },
	{ "doorpermitpresetactivateprevious", command_target::vehicle, command_mode::oneoff },
	{ "dooropenleft", command_target::vehicle, command_mode::oneoff },
	{ "dooropenright", command_target::vehicle, command_mode::oneoff },
	{ "dooropenall", command_target::vehicle, command_mode::oneoff },
	{ "doorcloseleft", command_target::vehicle, command_mode::oneoff },
	{ "doorcloseright", command_target::vehicle, command_mode::oneoff },
	{ "doorcloseall", command_target::vehicle, command_mode::oneoff },
	{ "doorsteptoggle", command_target::vehicle, command_mode::oneoff },
	{ "doormodetoggle", command_target::vehicle, command_mode::oneoff },
	{ "mirrorstoggle", command_target::vehicle, command_mode::oneoff },
	{ "departureannounce", command_target::vehicle, command_mode::oneoff },
	{ "doorlocktoggle", command_target::vehicle, command_mode::oneoff },
	{ "pantographcompressorvalvetoggle", command_target::vehicle, command_mode::oneoff },
	{ "pantographcompressorvalveenable", command_target::vehicle, command_mode::oneoff },
	{ "pantographcompressorvalvedisable", command_target::vehicle, command_mode::oneoff },
	{ "pantographcompressoractivate", command_target::vehicle, command_mode::oneoff },
	{ "pantographtogglefront", command_target::vehicle, command_mode::oneoff },
	{ "pantographtogglerear", command_target::vehicle, command_mode::oneoff },
	{ "pantographraisefront", command_target::vehicle, command_mode::oneoff },
	{ "pantographraiserear", command_target::vehicle, command_mode::oneoff },
	{ "pantographlowerfront", command_target::vehicle, command_mode::oneoff },
	{ "pantographlowerrear", command_target::vehicle, command_mode::oneoff },
	{ "pantographlowerall", command_target::vehicle, command_mode::oneoff },
	{ "pantographselectnext", command_target::vehicle, command_mode::oneoff },
	{ "pantographselectprevious", command_target::vehicle, command_mode::oneoff },
	{ "pantographtoggleselected", command_target::vehicle, command_mode::oneoff },
	{ "pantographraiseselected", command_target::vehicle, command_mode::oneoff },
	{ "pantographlowerselected", command_target::vehicle, command_mode::oneoff },
	{ "pantographvalvesupdate", command_target::vehicle, command_mode::oneoff },
	{ "pantographvalvesoff", command_target::vehicle, command_mode::oneoff },
	{ "heatingtoggle", command_target::vehicle, command_mode::oneoff },
	{ "heatingenable", command_target::vehicle, command_mode::oneoff },
	{ "heatingdisable", command_target::vehicle, command_mode::oneoff },
	{ "lightspresetactivatenext", command_target::vehicle, command_mode::oneoff },
	{ "lightspresetactivateprevious", command_target::vehicle, command_mode::oneoff },
	{ "headlighttoggleleft", command_target::vehicle, command_mode::oneoff },
	{ "headlightenableleft", command_target::vehicle, command_mode::oneoff },
	{ "headlightdisableleft", command_target::vehicle, command_mode::oneoff },
	{ "headlighttoggleright", command_target::vehicle, command_mode::oneoff },
	{ "headlightenableright", command_target::vehicle, command_mode::oneoff },
	{ "headlightdisableright", command_target::vehicle, command_mode::oneoff },
	{ "headlighttoggleupper", command_target::vehicle, command_mode::oneoff },
	{ "headlightenableupper", command_target::vehicle, command_mode::oneoff },
	{ "headlightdisableupper", command_target::vehicle, command_mode::oneoff },
	{ "redmarkertoggleleft", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerenableleft", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerdisableleft", command_target::vehicle, command_mode::oneoff },
	{ "redmarkertoggleright", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerenableright", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerdisableright", command_target::vehicle, command_mode::oneoff },
	{ "headlighttogglerearleft", command_target::vehicle, command_mode::oneoff },
	{ "headlightenablerearleft", command_target::vehicle, command_mode::oneoff },
	{ "headlightdisablerearleft", command_target::vehicle, command_mode::oneoff },
	{ "headlighttogglerearright", command_target::vehicle, command_mode::oneoff },
	{ "headlightenablerearright", command_target::vehicle, command_mode::oneoff },
	{ "headlightdisablerearright", command_target::vehicle, command_mode::oneoff },
	{ "headlighttogglerearupper", command_target::vehicle, command_mode::oneoff },
	{ "headlightenablerearupper", command_target::vehicle, command_mode::oneoff },
	{ "headlightdisablerearupper", command_target::vehicle, command_mode::oneoff },
	{ "redmarkertogglerearleft", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerenablerearleft", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerdisablerearleft", command_target::vehicle, command_mode::oneoff },
	{ "redmarkertogglerearright", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerenablerearright", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerdisablerearright", command_target::vehicle, command_mode::oneoff },
	{ "redmarkerstoggle", command_target::vehicle, command_mode::oneoff },
	{ "endsignalstoggle", command_target::vehicle, command_mode::oneoff },
	{ "headlightsdimtoggle", command_target::vehicle, command_mode::oneoff },
	{ "headlightsdimenable", command_target::vehicle, command_mode::oneoff },
	{ "headlightsdimdisable", command_target::vehicle, command_mode::oneoff },
	{ "motorconnectorsopen", command_target::vehicle, command_mode::oneoff },
	{ "motorconnectorsclose", command_target::vehicle, command_mode::oneoff },
	{ "motordisconnect", command_target::vehicle, command_mode::oneoff },
	{ "interiorlighttoggle", command_target::vehicle, command_mode::oneoff },
	{ "interiorlightenable", command_target::vehicle, command_mode::oneoff },
	{ "interiorlightdisable", command_target::vehicle, command_mode::oneoff },
	{ "interiorlightdimtoggle", command_target::vehicle, command_mode::oneoff },
	{ "interiorlightdimenable", command_target::vehicle, command_mode::oneoff },
	{ "interiorlightdimdisable", command_target::vehicle, command_mode::oneoff },
	{ "compartmentlightstoggle", command_target::vehicle, command_mode::oneoff },
	{ "compartmentlightsenable", command_target::vehicle, command_mode::oneoff },
	{ "compartmentlightsdisable", command_target::vehicle, command_mode::oneoff },
	{ "instrumentlighttoggle", command_target::vehicle, command_mode::oneoff },
	{ "instrumentlightenable", command_target::vehicle, command_mode::oneoff },
	{ "instrumentlightdisable", command_target::vehicle, command_mode::oneoff },
	{ "dashboardlighttoggle", command_target::vehicle, command_mode::oneoff },
	{ "dashboardlightenable", command_target::vehicle, command_mode::oneoff },
	{ "dashboardlightdisable", command_target::vehicle, command_mode::oneoff },
	{ "timetablelighttoggle", command_target::vehicle, command_mode::oneoff },
	{ "timetablelightenable", command_target::vehicle, command_mode::oneoff },
	{ "timetablelightdisable", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle0", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle1", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle2", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle3", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle4", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle5", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle6", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle7", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle8", command_target::vehicle, command_mode::oneoff },
	{ "generictoggle9", command_target::vehicle, command_mode::oneoff },
	{ "batterytoggle", command_target::vehicle, command_mode::oneoff },
	{ "batteryenable", command_target::vehicle, command_mode::oneoff },
	{ "batterydisable", command_target::vehicle, command_mode::oneoff },
	{ "cabactivationtoggle", command_target::vehicle, command_mode::oneoff },
	{ "cabactivationenable", command_target::vehicle, command_mode::oneoff },
	{ "cabactivationdisable", command_target::vehicle, command_mode::oneoff },
	{ "motorblowerstogglefront", command_target::vehicle, command_mode::oneoff },
	{ "motorblowerstogglerear", command_target::vehicle, command_mode::oneoff },
	{ "motorblowersdisableall", command_target::vehicle, command_mode::oneoff },
	{ "coolingfanstoggle", command_target::vehicle, command_mode::oneoff },
	{ "tempomattoggle", command_target::vehicle, command_mode::oneoff },
	{ "springbraketoggle", command_target::vehicle, command_mode::oneoff },
	{ "springbrakeenable", command_target::vehicle, command_mode::oneoff },
	{ "springbrakedisable", command_target::vehicle, command_mode::oneoff },
	{ "springbrakeshutofftoggle", command_target::vehicle, command_mode::oneoff },
	{ "springbrakeshutoffenable", command_target::vehicle, command_mode::oneoff },
	{ "springbrakeshutoffdisable", command_target::vehicle, command_mode::oneoff },
	{ "springbrakerelease", command_target::vehicle },
	{ "distancecounteractivate", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolincrease", command_target::vehicle, command_mode::oneoff },
	{ "speedcontroldecrease", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolpowerincrease", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolpowerdecrease", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton0", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton1", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton2", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton3", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton4", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton5", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton6", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton7", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton8", command_target::vehicle, command_mode::oneoff },
	{ "speedcontrolbutton9", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable1", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable2", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable3", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable4", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable5", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable6", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable7", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable8", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable9", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable10", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable11", command_target::vehicle, command_mode::oneoff },
	{ "inverterenable12", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable1", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable2", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable3", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable4", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable5", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable6", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable7", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable8", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable9", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable10", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable11", command_target::vehicle, command_mode::oneoff },
	{ "inverterdisable12", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle1", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle2", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle3", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle4", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle5", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle6", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle7", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle8", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle9", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle10", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle11", command_target::vehicle, command_mode::oneoff },
	{ "invertertoggle12", command_target::vehicle, command_mode::oneoff },
	{ "globalradiostop", command_target::simulation, command_mode::oneoff },
	{ "timejump", command_target::simulation, command_mode::oneoff },
	{ "timejumplarge", command_target::simulation, command_mode::oneoff },
	{ "timejumpsmall", command_target::simulation, command_mode::oneoff },
	{ "setdatetime", command_target::simulation, command_mode::oneoff },
	{ "setweather", command_target::simulation, command_mode::oneoff },
	{ "settemperature", command_target::simulation, command_mode::oneoff },
	{ "vehiclemoveforwards", command_target::vehicle, command_mode::oneoff },
	{ "vehiclemovebackwards", command_target::vehicle, command_mode::oneoff },
	{ "vehicleboost", command_target::vehicle, command_mode::oneoff },
	{ "debugtoggle", command_target::simulation, command_mode::oneoff },
	{ "focuspauseset", command_target::simulation, command_mode::oneoff },
	{ "pausetoggle", command_target::simulation, command_mode::oneoff },
	{ "entervehicle", command_target::simulation, command_mode::oneoff },
	{ "resetconsist", command_target::simulation, command_mode::oneoff },
	{ "fillcompressor", command_target::simulation, command_mode::oneoff },
	{ "consistreleaser", command_target::simulation, command_mode::oneoff },
	{ "queueevent", command_target::simulation, command_mode::oneoff },
	{ "setlight", command_target::simulation, command_mode::oneoff },
	{ "insertmodel", command_target::simulation, command_mode::oneoff },
	{ "deletemodel", command_target::simulation, command_mode::oneoff },
	{ "trainsetmove", command_target::simulation, command_mode::oneoff },
	{ "consistteleport", command_target::simulation, command_mode::oneoff },
	{ "pullalarmchain", command_target::simulation, command_mode::oneoff },
	{ "sendaicommand", command_target::simulation, command_mode::oneoff },
	{ "spawntrainset", command_target::simulation, command_mode::oneoff },
	{ "destroytrainset", command_target::simulation, command_mode::oneoff },
	{ "quitsimulation", command_target::simulation, command_mode::oneoff },
};

} // simulation

void command_queue::update()
{
	double delta = Timer::GetDeltaTime();
	for (auto c : m_active_continuous)
	{
		command_data data({c.first, GLFW_REPEAT, 0.0, 0.0, delta, false, glm::vec3()}); // todo: improve
		auto lookup = m_commands.emplace( c.second, commanddata_sequence() );
		// recipient stack was either located or created, so we can add to it quite safely
		lookup.first->second.emplace_back( data );
	}
}

// posts specified command for specified recipient
void
command_queue::push( command_data const &Command, uint32_t const Recipient ) {
	if (is_network_target(Recipient)) {
		auto lookup = m_intercept_queue.emplace(Recipient, commanddata_sequence());
		lookup.first->second.emplace_back(Command);
	} else {
		push_direct(Command, Recipient);
	}
}

void command_queue::push_direct(const command_data &Command, const uint32_t Recipient) {
	auto const &desc = simulation::Commands_descriptions[ static_cast<std::size_t>( Command.command ) ];
	if (desc.mode == command_mode::continuous)
	{
		if (Command.action == GLFW_PRESS)
			m_active_continuous.emplace(std::make_pair(Command.command, Recipient));
		else if (Command.action == GLFW_RELEASE)
			m_active_continuous.erase(std::make_pair(Command.command, Recipient));
		else if (Command.action == GLFW_REPEAT)
			return;
	}

	auto lookup = m_commands.emplace( Recipient, commanddata_sequence() );
	// recipient stack was either located or created, so we can add to it quite safely
	lookup.first->second.emplace_back( Command );
}

// retrieves oldest posted command for specified recipient, if any. returns: true on retrieval, false if there's nothing to retrieve
bool
command_queue::pop( command_data &Command, uint32_t const Recipient ) {

    auto lookup = m_commands.find( Recipient );
    if( lookup == m_commands.end() ) {
        // no command stack for this recipient, so no commands
        return false;
    }
    auto &commands = lookup->second;
    if( true == commands.empty() ) {

        return false;
    }
    // we have command stack with command(s) on it, retrieve and pop the first one
    Command = commands.front();
    commands.pop_front();

    return true;
}

bool command_queue::is_network_target(uint32_t const Recipient) {
	const command_target target = (command_target)(Recipient & ~0xffff);

	if (target == command_target::entity)
		return false;

	return true;
}

command_queue::commands_map command_queue::pop_intercept_queue() {
	commands_map map(m_intercept_queue);
	m_intercept_queue.clear();
	return map;
}

void command_queue::push_commands(const commands_map &commands) {
	for (auto const &kv : commands)
		for (command_data const &data : kv.second)
			push_direct(data, kv.first);
}

void
command_relay::post(user_command const Command, double const Param1, double const Param2,
                     int const Action, uint16_t Recipient, glm::vec3 Position, const std::string *Payload) const {

    auto const &command = simulation::Commands_descriptions[ static_cast<std::size_t>( Command ) ];

	if (command.target == command_target::vehicle && Recipient == 0) {
		// default 0 recipient is currently controlled train
		if (simulation::Train == nullptr)
			return;
		Recipient = simulation::Train->id();
	}

    if( ( command.target == command_target::vehicle )
     && ( true == FreeFlyModeFlag )
     && ( ( false == DebugModeFlag )
       && ( true == Global.RealisticControlMode ) ) ) {
        // in realistic control mode don't pass vehicle commands if the user isn't in one, unless we're in debug mode
        return;
    }

	if (Position == glm::vec3(0.0f))
		Position = Global.pCamera.Pos;

	uint32_t combined_recipient = static_cast<uint32_t>( command.target ) | Recipient;
	command_data commanddata({Command, Action, Param1, Param2, Timer::GetDeltaTime(), FreeFlyModeFlag, Position });
	if (Payload)
		commanddata.payload = *Payload;

	simulation::Commands.push(commanddata, combined_recipient);
}
