/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include "DynObj.h"
#include "Driver.h"
#include "Button.h"
#include "Gauge.h"
#include "sound.h"
#include "PyInt.h"
#include "command.h"
#include "pythonscreenviewer.h"
#include "dictionary.h"

#undef snprintf // pyint.h->python

// typedef enum {st_Off, st_Starting, st_On, st_ShuttingDown} T4State;

const double fCzuwakBlink = 0.15;
const float fConverterPrzekaznik = 1.5f; // hunter-261211: do przekaznika nadmiarowego przetwornicy
// 0.33f
// const double fBuzzerTime= 5.0f;
const float fHaslerTime = 1.2f;

class TCab {

public:
// methods
    void Load(cParser &Parser);
    void Update( bool const Power );
    TGauge &Gauge( int n = -1 ); // pobranie adresu obiektu
    TButton &Button( int n = -1 ); // pobranie adresu obiektu
// members
    Math3D::vector3 CabPos1 { 0, 1, 1 };
    Math3D::vector3 CabPos2 { 0, 1, -1 };
    bool bEnabled { false };
    bool bOccupied { true };
/*
    glm::vec3 dimm; // McZapkie-120503: tlumienie swiatla
    glm::vec3 intlit; // McZapkie-120503: oswietlenie kabiny
    glm::vec3 intlitlow; // McZapkie-120503: przyciemnione oswietlenie kabiny
*/
    bool bLight { false }; // hunter-091012: czy swiatlo jest zapalone?
    bool bLightDim { false }; // hunter-091012: czy przyciemnienie kabiny jest zapalone?
    float LightLevel{ 0.f }; // last calculated interior light level

private:
// members
    std::deque<TGauge> ggList; // need a container which doesn't invalidate references
    std::vector<TButton> btList;
};

class control_mapper {
    typedef std::unordered_map< TSubModel const *, std::string> submodelstring_map;
    submodelstring_map m_controlnames;
    using stringset = std::unordered_set<std::string>;
    stringset m_names; // names of registered controls
public:
    void
        clear();
    void
        insert( TGauge const &Gauge, std::string const &Label );
    std::string
        find( TSubModel const *Control ) const;
    bool
        contains( std::string const Control ) const;
};

class TTrain {

    friend class drivingaid_panel;

  public:
// types
    struct state_t {
        std::uint8_t shp;
        std::uint8_t alerter;
        std::uint8_t radio_stop;
        std::uint8_t motor_resistors;
        std::uint8_t line_breaker;
        std::uint8_t ground_relay;
        std::uint8_t motor_overload;
        std::uint8_t motor_connectors;
        std::uint8_t wheelslip;
        std::uint8_t converter_overload;
        std::uint8_t converter_off;
        std::uint8_t compressor_overload;
        std::uint8_t ventilator_overload;
        std::uint8_t motor_overload_threshold;
        std::uint8_t train_heating;
        std::uint8_t cab;
        std::uint8_t recorder_braking;
        std::uint8_t recorder_power;
        std::uint8_t alerter_sound;
        std::uint8_t coupled_hv_voltage_relays;
        float velocity;
        float reservoir_pressure;
        float pipe_pressure;
        float brake_pressure;
        float pantograph_pressure;
        float hv_voltage;
        std::array<float, 3> hv_current;
        float lv_voltage;
		double distance;
		std::uint8_t radio_channel;
		std::uint8_t springbrake_active;
        std::uint8_t epbrake_enabled;
		std::uint8_t dir_forward;
		std::uint8_t dir_backward;
		std::uint8_t doorleftallowed;
		std::uint8_t doorleftopened;
		std::uint8_t doorrightallowed;
		std::uint8_t doorrightopened;
		std::uint8_t doorstepallowed;
		std::uint8_t battery;
		std::uint8_t emergencybrake;
		std::uint8_t lockpipe;
		bool radiomessageindicator;
    };

    struct screen_entry {

        std::string script;
        std::string target;
		double updatetime = 0;
		double updatetimecounter = 0;
        std::shared_ptr<python_rt> rt;
        std::shared_ptr<python_screen_viewer> viewer;
        std::shared_ptr<std::vector<glm::vec2>> touch_list;

        dictionary_source parameters; // cached pre-processed optional per-screen parameters

        void deserialize( cParser &Input );
        bool deserialize_mapping( cParser &Input );
    };

	typedef std::vector<screen_entry> screenentry_sequence;

// methods
    bool CabChange(int iDirection);
    bool ShowNextCurrent; // pokaz przd w podlaczonej lokomotywie (ET41)
    bool InitializeCab(int NewCabNo, std::string const &asFileName);
    TTrain();
	~TTrain();
    // McZapkie-010302
    bool Init(TDynamicObject *NewDynamicObject, bool e3d = false);

    inline
    Math3D::vector3 GetDirection() const {
        return DynamicObject->VectorFront(); };
    inline
    Math3D::vector3 GetUp() const {
        return DynamicObject->VectorUp(); };
    inline
    std::string GetLabel( TSubModel const *Control ) const {
        return m_controlmapper.find( Control ); }
    void UpdateCab();
    bool Update( double const Deltatime );
    void add_distance( double const Distance );
    // McZapkie-310302: ladowanie parametrow z pliku
    bool LoadMMediaFile(std::string const &asFileName);
    dictionary_source *GetTrainState( dictionary_source const &Extraparameters );
    state_t get_state() const;
    // basic_table interface
    inline
    std::string name() const {
        return Dynamic()->name(); }

  private:
// types
    typedef void( *command_handler )( TTrain *Train, command_data const &Command );
    typedef std::unordered_map<user_command, command_handler> commandhandler_map;
// methods
    // clears state of all cabin controls
    void clear_cab_controls();
    // sets cabin controls based on current state of the vehicle
    // NOTE: we can get rid of this function once we have per-cab persistent state
    void set_cab_controls( int const Cab );
    // initializes a gauge matching provided label. returns: true if the label was found, false otherwise
    bool initialize_gauge(cParser &Parser, std::string const &Label, int const Cabindex);
    // initializes a button matching provided label. returns: true if the label was found, false otherwise
    bool initialize_button(cParser &Parser, std::string const &Label, int const Cabindex);
    // helper, returns true for EMU with oerlikon brake
    bool is_eztoer() const;
    // locates nearest vehicle belonging to the consist
	TDynamicObject *find_nearest_consist_vehicle(bool freefly, glm::vec3 pos) const;
    // mover master controller to specified position
    void set_master_controller( double const Position );
    // moves train brake lever to specified position, potentially emits switch sound if conditions are met
    void set_train_brake( double const Position );
    // potentially moves train brake lever to neutral position
    void zero_charging_train_brake();
    // sets specified brake acting speed for specified vehicle, potentially updating state of cab controls to match
    void set_train_brake_speed( TDynamicObject *Vehicle, int const Speed );
    // sets the motor connector button in paired unit to specified state
    void set_paired_open_motor_connectors_button( bool const State );
    // helper, common part of pantograph selection methods
    void change_pantograph_selection( int const Change );
    // helper, common part of pantograh valves state update methods
    void update_pantograph_valves();
    // update function subroutines
    void update_sounds( double const Deltatime );
    void update_sounds_runningnoise( sound_source &Sound );
    void update_sounds_radio();
    inline
    end cab_to_end( int const End ) const {
        return (
            End == 2 ?
                end::rear :
                end::front ); }
    inline
    end cab_to_end() const {
        return cab_to_end( iCabn ); }
	void update_screens(double dt);

    // command handlers
    // NOTE: we're currently using universal handlers and static handler map but it may be beneficial to have these implemented on individual class instance basis
    // TBD, TODO: consider this approach if we ever want to have customized consist behaviour to received commands, based on the consist/vehicle type or whatever
    static void OnCommand_aidriverenable( TTrain *Train, command_data const &Command );
    static void OnCommand_aidriverdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_jointcontrollerset( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerincreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerdecreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_mastercontrollerset( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerincreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerdecreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_secondcontrollerset( TTrain *Train, command_data const &Command );
    static void OnCommand_notchingrelaytoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_tempomattoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_mucurrentindicatorothersourceactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakeincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakeincreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakedecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakedecreasefast( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakeset( TTrain *Train, command_data const &Command );
    static void OnCommand_independentbrakebailoff( TTrain *Train, command_data const &Command );
	static void OnCommand_universalbrakebutton1(TTrain *Train, command_data const &Command);
	static void OnCommand_universalbrakebutton2(TTrain *Train, command_data const &Command);
	static void OnCommand_universalbrakebutton3(TTrain *Train, command_data const &Command);
    static void OnCommand_trainbrakeincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakedecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeset( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakecharging( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakerelease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakefirstservice( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeservice( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakefullservice( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakehandleoff( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeemergency( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakebasepressureincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakebasepressuredecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakebasepressurereset( TTrain *Train, command_data const &Command );
    static void OnCommand_trainbrakeoperationtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_manualbrakeincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_manualbrakedecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_alarmchaintoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_alarmchainenable(TTrain *Train, command_data const &Command);
    static void OnCommand_alarmchaindisable(TTrain *Train, command_data const &Command);
    static void OnCommand_wheelspinbrakeactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_sandboxactivate( TTrain *Train, command_data const &Command );
	static void OnCommand_autosandboxtoggle(TTrain *Train, command_data const &Command);
	static void OnCommand_autosandboxactivate(TTrain *Train, command_data const &Command);
	static void OnCommand_autosandboxdeactivate(TTrain *Train, command_data const &Command);
    static void OnCommand_epbrakecontroltoggle( TTrain *Train, command_data const &Command );
	static void OnCommand_trainbrakeoperationmodeincrease(TTrain *Train, command_data const &Command);
	static void OnCommand_trainbrakeoperationmodedecrease(TTrain *Train, command_data const &Command);
    static void OnCommand_brakeactingspeedincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeactingspeeddecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeactingspeedsetcargo( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeactingspeedsetpassenger( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeactingspeedsetrapid( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeloadcompensationincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_brakeloadcompensationdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_mubrakingindicatortoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserdecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserforwardhigh( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserforward( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserneutral( TTrain *Train, command_data const &Command );
    static void OnCommand_reverserbackward( TTrain *Train, command_data const &Command );
    static void OnCommand_alerteracknowledge( TTrain *Train, command_data const &Command );
	static void OnCommand_cabsignalacknowledge( TTrain *Train, command_data const &Command );
    static void OnCommand_batterytoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_batteryenable( TTrain *Train, command_data const &Command );
    static void OnCommand_batterydisable( TTrain *Train, command_data const &Command );
	static void OnCommand_cabactivationtoggle(TTrain *Train, command_data const &Command);
	static void OnCommand_cabactivationenable(TTrain *Train, command_data const &Command);
	static void OnCommand_cabactivationdisable(TTrain *Train, command_data const &Command);
    static void OnCommand_pantographcompressorvalvetoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographcompressorvalveenable( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographcompressorvalvedisable( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographcompressoractivate( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographtogglefront( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographtogglerear( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographraisefront( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographraiserear( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographlowerfront( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographlowerrear( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographlowerall( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographselectnext( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographselectprevious( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographtoggleselected( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographraiseselected( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographlowerselected( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographvalvesupdate( TTrain *Train, command_data const &Command );
    static void OnCommand_pantographvalvesoff( TTrain *Train, command_data const &Command );
    static void OnCommand_linebreakertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_linebreakeropen( TTrain *Train, command_data const &Command );
    static void OnCommand_linebreakerclose( TTrain *Train, command_data const &Command );
    static void OnCommand_fuelpumptoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_fuelpumpenable( TTrain *Train, command_data const &Command );
    static void OnCommand_fuelpumpdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_oilpumptoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_oilpumpenable( TTrain *Train, command_data const &Command );
    static void OnCommand_oilpumpdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_waterheaterbreakertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_waterheaterbreakerclose( TTrain *Train, command_data const &Command );
    static void OnCommand_waterheaterbreakeropen( TTrain *Train, command_data const &Command );
    static void OnCommand_waterheatertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_waterheaterenable( TTrain *Train, command_data const &Command );
    static void OnCommand_waterheaterdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_waterpumpbreakertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_waterpumpbreakerclose( TTrain *Train, command_data const &Command );
    static void OnCommand_waterpumpbreakeropen( TTrain *Train, command_data const &Command );
    static void OnCommand_waterpumptoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_waterpumpenable( TTrain *Train, command_data const &Command );
    static void OnCommand_waterpumpdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_watercircuitslinktoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_watercircuitslinkenable( TTrain *Train, command_data const &Command );
    static void OnCommand_watercircuitslinkdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_convertertoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_converterenable( TTrain *Train, command_data const &Command );
    static void OnCommand_converterdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_convertertogglelocal( TTrain *Train, command_data const &Command );
    static void OnCommand_converteroverloadrelayreset( TTrain *Train, command_data const &Command );
    static void OnCommand_compressortoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_compressorenable( TTrain *Train, command_data const &Command );
    static void OnCommand_compressordisable( TTrain *Train, command_data const &Command );
    static void OnCommand_compressortogglelocal( TTrain *Train, command_data const &Command );
	static void OnCommand_compressorpresetactivatenext( TTrain *Train, command_data const &Command );
	static void OnCommand_compressorpresetactivateprevious( TTrain *Train, command_data const &Command );
	static void OnCommand_compressorpresetactivatedefault(TTrain *Train, command_data const &Command);
    static void OnCommand_motorblowerstogglefront( TTrain *Train, command_data const &Command );
    static void OnCommand_motorblowersenablefront( TTrain *Train, command_data const &Command );
    static void OnCommand_motorblowersdisablefront( TTrain *Train, command_data const &Command );
    static void OnCommand_motorblowerstogglerear( TTrain *Train, command_data const &Command );
    static void OnCommand_motorblowersenablerear( TTrain *Train, command_data const &Command );
    static void OnCommand_motorblowersdisablerear( TTrain *Train, command_data const &Command );
    static void OnCommand_motorblowersdisableall( TTrain *Train, command_data const &Command );
    static void OnCommand_coolingfanstoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_motorconnectorsopen( TTrain *Train, command_data const &Command );
    static void OnCommand_motorconnectorsclose( TTrain *Train, command_data const &Command );
    static void OnCommand_motordisconnect( TTrain *Train, command_data const &Command );
    static void OnCommand_motoroverloadrelaythresholdtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_motoroverloadrelaythresholdsetlow( TTrain *Train, command_data const &Command );
    static void OnCommand_motoroverloadrelaythresholdsethigh( TTrain *Train, command_data const &Command );
    static void OnCommand_motoroverloadrelayreset( TTrain *Train, command_data const &Command );
    static void OnCommand_universalrelayreset( TTrain *Train, command_data const &Command );
    static void OnCommand_heatingtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_heatingenable( TTrain *Train, command_data const &Command );
    static void OnCommand_heatingdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_lightspresetactivatenext( TTrain *Train, command_data const &Command );
    static void OnCommand_lightspresetactivateprevious( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttoggleleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightenableleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightdisableleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttoggleright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightenableright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightdisableright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttoggleupper( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightenableupper( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightdisableupper( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertoggleleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerenableleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerdisableleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertoggleright( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerenableright( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerdisableright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttogglerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightenablerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightdisablerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttogglerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightenablerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightdisablerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_headlighttogglerearupper( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightenablerearupper( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightdisablerearupper( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertogglerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerenablerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerdisablerearleft( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkertogglerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerenablerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerdisablerearright( TTrain *Train, command_data const &Command );
    static void OnCommand_redmarkerstoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_endsignalstoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightsdimtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightsdimenable( TTrain *Train, command_data const &Command );
    static void OnCommand_headlightsdimdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlighttoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlightenable( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlightdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlightdimtoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlightdimenable( TTrain *Train, command_data const &Command );
    static void OnCommand_interiorlightdimdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_compartmentlightstoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_compartmentlightsenable( TTrain *Train, command_data const &Command );
    static void OnCommand_compartmentlightsdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_instrumentlighttoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_instrumentlightenable( TTrain *Train, command_data const &Command );
    static void OnCommand_instrumentlightdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_dashboardlighttoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_dashboardlightenable( TTrain *Train, command_data const &Command );
    static void OnCommand_dashboardlightdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_timetablelighttoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_timetablelightenable( TTrain *Train, command_data const &Command );
    static void OnCommand_timetablelightdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_doorlocktoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_doortoggleleft( TTrain *Train, command_data const &Command );
    static void OnCommand_doortoggleright( TTrain *Train, command_data const &Command );
    static void OnCommand_doorpermitleft( TTrain *Train, command_data const &Command );
    static void OnCommand_doorpermitright( TTrain *Train, command_data const &Command );
    static void OnCommand_doorpermitpresetactivatenext( TTrain *Train, command_data const &Command );
    static void OnCommand_doorpermitpresetactivateprevious( TTrain *Train, command_data const &Command );
    static void OnCommand_dooropenleft( TTrain *Train, command_data const &Command );
    static void OnCommand_dooropenright( TTrain *Train, command_data const &Command );
    static void OnCommand_doorcloseleft( TTrain *Train, command_data const &Command );
    static void OnCommand_doorcloseright( TTrain *Train, command_data const &Command );
    static void OnCommand_dooropenall( TTrain *Train, command_data const &Command );
    static void OnCommand_doorcloseall( TTrain *Train, command_data const &Command );
    static void OnCommand_doorsteptoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_doormodetoggle( TTrain *Train, command_data const &Command );
	static void OnCommand_mirrorstoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_nearestcarcouplingincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_nearestcarcouplingdisconnect( TTrain *Train, command_data const &Command );
    static void OnCommand_nearestcarcoupleradapterattach( TTrain *Train, command_data const &Command );
    static void OnCommand_nearestcarcoupleradapterremove( TTrain *Train, command_data const &Command );
    static void OnCommand_occupiedcarcouplingdisconnect( TTrain *Train, command_data const &Command );
	static void OnCommand_occupiedcarcouplingdisconnectback( TTrain *Train, command_data const &Command );
    static void OnCommand_departureannounce( TTrain *Train, command_data const &Command );
    static void OnCommand_hornlowactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_hornhighactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_whistleactivate( TTrain *Train, command_data const &Command );
    static void OnCommand_radiotoggle( TTrain *Train, command_data const &Command );
    static void OnCommand_radioenable( TTrain *Train, command_data const &Command );
    static void OnCommand_radiodisable( TTrain *Train, command_data const &Command );
    static void OnCommand_radiochannelincrease( TTrain *Train, command_data const &Command );
    static void OnCommand_radiochanneldecrease( TTrain *Train, command_data const &Command );
    static void OnCommand_radiochannelset( TTrain *Train, command_data const &Command );
    static void OnCommand_radiostopsend( TTrain *Train, command_data const &Command );
    static void OnCommand_radiostopenable( TTrain *Train, command_data const &Command );
    static void OnCommand_radiostopdisable( TTrain *Train, command_data const &Command );
    static void OnCommand_radiostoptest( TTrain *Train, command_data const &Command );
    static void OnCommand_radiocall3send( TTrain *Train, command_data const &Command );
	static void OnCommand_radiovolumeincrease(TTrain *Train, command_data const &Command);
	static void OnCommand_radiovolumedecrease(TTrain *Train, command_data const &Command);
	static void OnCommand_radiovolumeset(TTrain *Train, command_data const &Command);
    static void OnCommand_cabchangeforward( TTrain *Train, command_data const &Command );
    static void OnCommand_cabchangebackward( TTrain *Train, command_data const &Command );
    static void OnCommand_generictoggle( TTrain *Train, command_data const &Command );
	static void OnCommand_vehiclemoveforwards( TTrain *Train, command_data const &Command );
	static void OnCommand_vehiclemovebackwards( TTrain *Train, command_data const &Command );
	static void OnCommand_vehicleboost( TTrain *Train, command_data const &Command );
	static void OnCommand_springbraketoggle(TTrain *Train, command_data const &Command);
	static void OnCommand_springbrakeenable(TTrain *Train, command_data const &Command);
	static void OnCommand_springbrakedisable(TTrain *Train, command_data const &Command);
	static void OnCommand_springbrakeshutofftoggle(TTrain *Train, command_data const &Command);
	static void OnCommand_springbrakeshutoffenable(TTrain *Train, command_data const &Command);
	static void OnCommand_springbrakeshutoffdisable(TTrain *Train, command_data const &Command);
	static void OnCommand_springbrakerelease(TTrain *Train, command_data const &Command);
    static void OnCommand_distancecounteractivate( TTrain *Train, command_data const &Command );
	static void OnCommand_speedcontrolincrease(TTrain *Train, command_data const &Command);
	static void OnCommand_speedcontroldecrease(TTrain *Train, command_data const &Command);
	static void OnCommand_speedcontrolpowerincrease(TTrain *Train, command_data const &Command);
	static void OnCommand_speedcontrolpowerdecrease(TTrain *Train, command_data const &Command);
	static void OnCommand_speedcontrolbutton(TTrain *Train, command_data const &Command);
	static void OnCommand_inverterenable(TTrain *Train, command_data const &Command);
	static void OnCommand_inverterdisable(TTrain *Train, command_data const &Command);
	static void OnCommand_invertertoggle(TTrain *Train, command_data const &Command);


// members
    TDynamicObject *DynamicObject { nullptr }; // przestawia zmiana pojazdu [F5]
    TMoverParameters *mvControlled { nullptr }; // człon, w którym sterujemy silnikiem
    TMoverParameters *mvOccupied { nullptr }; // człon, w którym sterujemy hamulcem
    TMoverParameters *mvSecond { nullptr }; // drugi człon (ET40, ET41, ET42, ukrotnienia)
    TMoverParameters *mvThird { nullptr }; // trzeci człon (SN61)
    TMoverParameters *mvPantographUnit { nullptr }; // nearest pantograph equipped unit
    // helper variable, to prevent immediate switch between closing and opening line breaker circuit
    int m_linebreakerstate { 0 }; // 0: open, 1: closed, 2: freshly closed (and yes this is awful way to go about it)
    static const commandhandler_map m_commandhandlers;
    control_mapper m_controlmapper;

public: // reszta może by?publiczna

    // McZapkie: definicje wskaźników
    // Ra 2014-08: częsciowo przeniesione do tablicy w TCab
    TGauge ggClockSInd;
    TGauge ggClockMInd;
    TGauge ggClockHInd;
    TGauge ggLVoltage;
    TGauge ggMainGearStatus;

    TGauge ggEngineVoltage;
    TGauge ggI1B; // drugi człon w postaci jawnej
    TGauge ggI2B;
    TGauge ggI3B;
    TGauge ggItotalB;

    TGauge ggOilPressB; // other unit oil pressure indicator
    TGauge ggWater1TempB;

    // McZapkie: definicje regulatorow
    TGauge ggJointCtrl;
    TGauge ggMainCtrl;
    TGauge ggMainCtrlAct;
    TGauge ggScndCtrl;
    TGauge ggScndCtrlButton;
    TGauge ggScndCtrlOffButton;
    TGauge ggDirKey;
    TGauge ggDirForwardButton;
    TGauge ggDirNeutralButton;
    TGauge ggDirBackwardButton;
    TGauge ggBrakeCtrl;
    TGauge ggLocalBrake;
    TGauge ggAlarmChain;
    TGauge ggBrakeProfileCtrl; // nastawiacz GPR - przelacznik obrotowy
    TGauge ggBrakeProfileG; // nastawiacz GP - hebelek towarowy
    TGauge ggBrakeProfileR; // nastawiacz PR - hamowanie dwustopniowe
	TGauge ggBrakeOperationModeCtrl; //przełącznik trybu pracy PS/PN/EP/MED

    TGauge ggMaxCurrentCtrl;

    TGauge ggMainOffButton;
    TGauge ggMainOnButton;
    TGauge ggMainButton; // EZT
    TGauge ggSecurityResetButton;
    TGauge ggSHPResetButton;
    TGauge ggReleaserButton;
	TGauge ggSpringBrakeOnButton;
	TGauge ggSpringBrakeOffButton;
	TGauge ggUniveralBrakeButton1;
	TGauge ggUniveralBrakeButton2;
	TGauge ggUniveralBrakeButton3;
    TGauge ggEPFuseButton;
    TGauge ggSandButton; // guzik piasecznicy
	TGauge ggAutoSandButton; // przełącznik piasecznicy
    TGauge ggAntiSlipButton;
    TGauge ggFuseButton;
    TGauge ggConverterFuseButton; // hunter-261211: przycisk odblokowania nadmiarowego przetwornic i ogrzewania
    TGauge ggStLinOffButton;
    TGauge ggRadioChannelSelector;
    TGauge ggRadioChannelPrevious;
    TGauge ggRadioChannelNext;
    TGauge ggRadioTest;
    TGauge ggRadioStop;
    TGauge ggRadioCall3;
	TGauge ggRadioVolumeSelector;
	TGauge ggRadioVolumePrevious;
	TGauge ggRadioVolumeNext;
    TGauge ggUpperLightButton;
    TGauge ggLeftLightButton;
    TGauge ggRightLightButton;
    TGauge ggLeftEndLightButton;
    TGauge ggRightEndLightButton;
    TGauge ggLightsButton; // przelacznik reflektorow (wszystkich)
    TGauge ggDimHeadlightsButton; // headlights dimming switch

    // hunter-230112: przelacznik swiatel tylnich
    TGauge ggRearUpperLightButton;
    TGauge ggRearLeftLightButton;
    TGauge ggRearRightLightButton;
    TGauge ggRearLeftEndLightButton;
    TGauge ggRearRightEndLightButton;

    TGauge ggIgnitionKey;

    TGauge ggCompressorButton;
    TGauge ggCompressorLocalButton; // controls only compressor of its own unit (et42-specific)
	TGauge ggCompressorListButton; // controls compressors with various settings
    TGauge ggConverterButton;
    TGauge ggConverterLocalButton; // controls only converter of its own unit (et42-specific)
    TGauge ggConverterOffButton;

    // ABu 090305 - syrena i prad nastepnego czlonu
    TGauge ggHornButton;
    TGauge ggHornLowButton;
    TGauge ggHornHighButton;
    TGauge ggWhistleButton;
	TGauge ggHelperButton;
    TGauge ggNextCurrentButton;

 	// yB 191005 - tempomat
	TGauge ggSpeedControlIncreaseButton;
	TGauge ggSpeedControlDecreaseButton;
	TGauge ggSpeedControlPowerIncreaseButton;
	TGauge ggSpeedControlPowerDecreaseButton;
	std::array<TGauge, 10> ggSpeedCtrlButtons; // NOTE: temporary arrangement until we have dynamically built control table

   std::array<TGauge, 10> ggUniversals; // NOTE: temporary arrangement until we have dynamically built control table
   std::array<TGauge, 3> ggRelayResetButtons; // NOTE: temporary arrangement until we have dynamically built control table
   std::array<TGauge, 12> ggInverterEnableButtons; // NOTE: temporary arrangement until we have dynamically built control table
   std::array<TGauge, 12> ggInverterDisableButtons; // NOTE: temporary arrangement until we have dynamically built control table
   std::array<TGauge, 12> ggInverterToggleButtons; // NOTE: temporary arrangement until we have dynamically built control table

    TGauge ggInstrumentLightButton;
    TGauge ggDashboardLightButton;
    TGauge ggTimetableLightButton;
//    TGauge ggCabLightButton; // hunter-091012: przelacznik oswietlania kabiny
    TGauge ggCabLightDimButton; // hunter-091012: przelacznik przyciemnienia
    // oswietlenia kabiny
    TGauge ggCompartmentLightsButton;
    TGauge ggCompartmentLightsOnButton;
    TGauge ggCompartmentLightsOffButton;
    TGauge ggBatteryButton; // Stele 161228 hebelek baterii
    TGauge ggBatteryOnButton;
    TGauge ggBatteryOffButton;
	TGauge ggCabActivationButton; // Stele 161228 hebelek baterii

    // NBMX wrzesien 2003 - obsluga drzwi
    TGauge ggDoorLeftPermitButton;
    TGauge ggDoorRightPermitButton;
    TGauge ggDoorPermitPresetButton;
    TGauge ggDoorLeftButton;
    TGauge ggDoorRightButton;
    TGauge ggDoorLeftOnButton;
    TGauge ggDoorRightOnButton;
    TGauge ggDoorLeftOffButton;
    TGauge ggDoorRightOffButton;
    TGauge ggDoorAllOnButton;
    TGauge ggDoorAllOffButton;
    TGauge ggDepartureSignalButton;
    TGauge ggDoorStepButton;

    // Winger 160204 - obsluga pantografow - ZROBIC
/*
    TGauge ggPantFrontButton;
    TGauge ggPantRearButton;
    TGauge ggPantFrontButtonOff; // EZT
    TGauge ggPantRearButtonOff;
*/
    TGauge ggPantAllDownButton;
    TGauge ggPantSelectedButton;
    TGauge ggPantSelectedDownButton;
    TGauge ggPantValvesButton;
    TGauge ggPantCompressorButton;
    TGauge ggPantCompressorValve;
    // Winger 020304 - wlacznik ogrzewania
    TGauge ggTrainHeatingButton;
    TGauge ggSignallingButton;
    TGauge ggDoorSignallingButton;

    TGauge ggWaterPumpBreakerButton; // water pump breaker switch
    TGauge ggWaterPumpButton; // water pump switch
    TGauge ggWaterHeaterBreakerButton; // water heater breaker switch
    TGauge ggWaterHeaterButton; // water heater switch
    TGauge ggWaterCircuitsLinkButton;
    TGauge ggFuelPumpButton; // fuel pump switch
    TGauge ggOilPumpButton; // fuel pump switch
    TGauge ggMotorBlowersFrontButton; // front traction motor fan switch
    TGauge ggMotorBlowersRearButton; // rear traction motor fan switch
    TGauge ggMotorBlowersAllOffButton; // motor fans shutdown switch

    TGauge ggDistanceCounterButton; // distance meter activation button

    TButton btLampkaPoslizg;
    TButton btLampkaStyczn;
    TButton btLampkaNadmPrzetw;
    TButton btLampkaPrzetw;
    TButton btLampkaPrzetwOff;
    TButton btLampkaPrzekRozn; // TODO: implement
    TButton btLampkaPrzekRoznPom; // TODO: implement
    TButton btLampkaNadmSil;
    TButton btLampkaWylSzybki;
    TButton btLampkaWylSzybkiOff;
    TButton btLampkaMainBreakerReady;
    TButton btLampkaMainBreakerBlinkingIfReady;
    TButton btLampkaNadmWent;
    TButton btLampkaNadmSpr; // TODO: implement
    // yB: drugie lampki dla EP05 i ET42
    TButton btLampkaOporyB;
    TButton btLampkaStycznB;
    TButton btLampkaWylSzybkiB;
    TButton btLampkaWylSzybkiBOff;
    TButton btLampkaNadmPrzetwB;
    TButton btLampkaPrzetwB;
    TButton btLampkaPrzetwBOff;
    TButton btLampkaHVoltageB; // TODO: implement
    // KURS90 lampki jazdy bezoporowej dla EU04
    TButton btLampkaBezoporowaB;
    TButton btLampkaBezoporowa;
    TButton btLampkaUkrotnienie;
    TButton btLampkaHamPosp;
    TButton btLampkaRadio;
	TButton btLampkaRadioMessage;
    TButton btLampkaRadioStop;
    TButton btLampkaHamowanie1zes;
    TButton btLampkaHamowanie2zes;
    TButton btLampkaOpory;
    TButton btLampkaWysRozr;
    std::array<TButton, 10> btUniversals; // NOTE: temporary arrangement until we have dynamically built control table
    TButton btInstrumentLight;
    TButton btDashboardLight;
    TButton btTimetableLight;
    int InstrumentLightType{ 0 }; // ABu 030405 - swiecenie uzaleznione od: 0-nic, 1-obw.gl, 2-przetw., 3-rozrzad, 4-external lights
    bool InstrumentLightActive{ false };
    bool DashboardLightActive{ false };
    bool TimetableLightActive{ false };
    TButton btLampkaWentZaluzje; // ET22 // TODO: implement
    TButton btLampkaOgrzewanieSkladu;
    TButton btLampkaSHP;
    TButton btLampkaCzuwaka; // McZapkie-141102
	TButton btLampkaCzuwakaSHP;
	TButton btLampkaRezerwa;
    // youBy - jakies dodatkowe lampki
    TButton btLampkaNapNastHam;
    TButton btLampkaSprezarka;
    TButton btLampkaSprezarkaB;
    TButton btLampkaSprezarkaOff;
    TButton btLampkaSprezarkaBOff;
    TButton btLampkaFuelPumpOff;
    TButton btLampkaBocznik1;
    TButton btLampkaBocznik2;
    TButton btLampkaBocznik3;
    TButton btLampkaBocznik4;
    TButton btLampkaRadiotelefon; // TODO: implement
    TButton btLampkaHamienie;
    TButton btLampkaBrakingOff;
    TButton btLampkaED; // Stele 161228 hamowanie elektrodynamiczne
    TButton btLampkaBrakeProfileG; // cargo train brake acting speed
    TButton btLampkaBrakeProfileP; // passenger train brake acting speed
    TButton btLampkaBrakeProfileR; // rapid brake acting speed
	TButton btLampkaSpringBrakeActive;
	TButton btLampkaSpringBrakeInactive;
    // KURS90
    TButton btLampkaBoczniki;
    TButton btLampkaMaxSila;
    TButton btLampkaPrzekrMaxSila;
    TButton btLampkaDoorLeft;
    TButton btLampkaDoorRight;
    TButton btLampkaDepartureSignal;
    TButton btLampkaBlokadaDrzwi;
    TButton btLampkaDoorLockOff;
    TButton btLampkaHamulecReczny;
    TButton btLampkaForward; // Ra: lampki w przód i w ty?dla komputerowych kabin
    TButton btLampkaBackward;
    // light indicators
    TButton btLampkaUpperLight;
    TButton btLampkaLeftLight;
    TButton btLampkaRightLight;
    TButton btLampkaLeftEndLight;
    TButton btLampkaRightEndLight;
    TButton btLampkaRearUpperLight;
    TButton btLampkaRearLeftLight;
    TButton btLampkaRearRightLight;
    TButton btLampkaRearLeftEndLight;
    TButton btLampkaRearRightEndLight;
    // other
    TButton btLampkaMalfunction;
    TButton btLampkaMalfunctionB;
    TButton btLampkaMotorBlowers;
    TButton btLampkaCoolingFans;
    TButton btLampkaTempomat;
    TButton btLampkaDistanceCounter;

//    TButton btCabLight; // hunter-171012: lampa oswietlajaca kabine
    // Ra 2013-12: wirtualne "lampki" do odbijania na haslerze w PoKeys
    TButton btHaslerBrakes; // ciśnienie w cylindrach
    TButton btHaslerCurrent; // prąd na silnikach

    std::optional<sound_source>
        dsbNastawnikJazdy,
        dsbNastawnikBocz,
        dsbReverserKey,
        dsbBuzzer,
        m_radiostop,
        dsbSlipAlarm,
        m_distancecounterclear,
        dsbHasler,
        dsbSwitch,
        dsbPneumaticSwitch,
        rsHiss,
        rsHissU,
        rsHissE,
        rsHissX,
        rsHissT,
        rsSBHiss,
        rsSBHissU,
        rsBrake,
        rsFadeSound,
        rsRunningNoise,
        rsHuntingNoise,
        m_rainsound;
    sound_source m_radiosound { sound_placement::internal, 2 * EU07_SOUND_CABCONTROLSCUTOFFRANGE }; // cached template for radio messages
    std::vector<std::pair<int, std::shared_ptr<sound_source>>> m_radiomessages; // list of currently played radio messages
	std::vector<std::pair<std::reference_wrapper<std::optional<sound_source>>, glm::vec3>> CabSoundLocations; // list of offsets for manually located sounds;
    float m_lastlocalbrakepressure { -1.f }; // helper, cached level of pressure in local brake cylinder
    float m_localbrakepressurechange { 0.f }; // recent change of pressure in local brake cylinder
/*
    int iCabLightFlag; // McZapkie:120503: oswietlenie kabiny (0: wyl, 1: przyciemnione, 2: pelne)
    bool bCabLight; // hunter-091012: czy swiatlo jest zapalone?
    bool bCabLightDim; // hunter-091012: czy przyciemnienie kabiny jest zapalone?
*/
    // McZapkie: opis kabiny - obszar poruszania sie mechanika oraz zajetosc
    std::array<TCab, 3> Cabine; // przedzial maszynowy, kabina 1 (A), kabina 2 (B)
    int iCabn { 0 }; // 0: mid, 1: front, 2: rear
    bool is_cab_initialized { false };
    // McZapkie: do poruszania sie po kabinie
    Math3D::vector3 pMechSittingPosition; // ABu 180404
    Math3D::vector3 MirrorPosition( bool lewe );
    Math3D::vector3 pMechOffset; // base position of the driver in the cab
    glm::vec2 pMechViewAngle { 0.0, 0.0 }; // camera pitch and yaw values, preserved while in external view

private:
    double fBlinkTimer;
    float fHaslerTimer;
    float fConverterTimer; // hunter-261211: dla przekaznika
    float fMainRelayTimer; // hunter-141211: zalaczanie WSa z opoznieniem
    int ScreenUpdateRate { 0 }; // vehicle specific python screen update rate override

    // McZapkie-240302 - przyda sie do tachometru
    float fTachoVelocity{ 0.0f };
    float fTachoVelocityJump{ 0.0f }; // ze skakaniem
    float fTachoTimer{ 0.0f };
    float fTachoCount{ 0.0f };
    float fHVoltage{ 0.0f }; // napi?cie dla dynamicznych ga?ek
    float fHCurrent[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f }; // pr?dy: suma i amperomierze 1,2,3
    float fEngine[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f }; // obroty te? trzeba pobra?
    int iCarNo, iPowerNo, iUnitNo; // liczba pojazdow, czlonow napednych i jednostek spiętych ze sobą
    bool bDoors[20][5]; // drzwi dla wszystkich czlonow; left+right, left, right, step_left, step_right
    int iUnits[20]; // numer jednostki
    int iDoorNo[20]; // liczba drzwi
    char cCode[20]; // kod pojazdu
	bool bSlip[20]; // poślizg kół pojazdu
    std::string asCarName[20]; // nazwa czlonu
    bool bMains[8]; // WSy
    float fCntVol[8]; // napiecie NN
    bool bPants[8][2]; // podniesienie pantografow
    bool bFuse[8]; // nadmiarowe
    bool bBatt[8]; // baterie
    bool bConv[8]; // przetwornice
    bool bComp[8][2]; // sprezarki
	std::vector<std::tuple<bool, bool, int>> bCompressors;
    bool bHeat[8]; // grzanie
    // McZapkie: do syczenia
    float fPPress, fNPress;
    bool m_mastercontrollerinuse { false };
    float m_mastercontrollerreturndelay { 0.f };
	screenentry_sequence m_screens;
	uint16_t vid { 0 }; // train network recipient id
    float m_distancecounter { -1.f }; // distance traveled since meter was activated or -1 if inactive
    double m_brakehandlecp{ 0.0 };
    bool m_doors{ false }; // helper, true if any door is open
	bool m_doorspermitleft{ false }; // helper, true if door is open, blinking if door is pemitted
	bool m_doorspermitright{ false }; // helper, true if door is open, blinking if door is pemitted
    bool m_dirforward{ false }; // helper, true if direction set to forward
    bool m_dirneutral{ false }; // helper, true if direction set to neutral
    bool m_dirbackward{ false }; // helper, true if direction set to backward
    bool m_doorpermits{ false }; // helper, true if any door permit is active
    float m_doorpermittimers[2] = { -1.f, -1.f };
    // ld substitute
    bool m_couplingdisconnect { false };
	bool m_couplingdisconnectback { false };

  public:
    float fPress[20][7]; // cisnienia dla wszystkich czlonow
	bool bBrakes[20][2]; // zalaczenie i dzialanie hamulcow
    static std::vector<std::string> const fPress_labels;
    float fEIMParams[9][10]; // parametry dla silnikow asynchronicznych
	float fDieselParams[9][10]; // parametry dla silnikow asynchronicznych
    // plays provided sound from position of the radio
	bool radio_message_played;
    void radio_message( sound_source *Message, int const Channel );
    inline auto const RadioChannel() const { return ( Dynamic()->Mechanik ? Dynamic()->Mechanik->iRadioChannel : 1 ); }
    inline auto &RadioChannel() { return Dynamic()->Mechanik->iRadioChannel; }
    inline TDynamicObject *Dynamic() { return DynamicObject; };
    inline TDynamicObject const *Dynamic() const { return DynamicObject; };
    inline TMoverParameters *Controlled() { return mvControlled; };
    inline TMoverParameters const *Controlled() const { return mvControlled; };
    inline TMoverParameters *Occupied() { return mvOccupied; };
    inline TMoverParameters const *Occupied() const { return mvOccupied; };
    void DynamicSet(TDynamicObject *d);
    void MoveToVehicle( TDynamicObject *target );
    // checks whether specified point is within boundaries of the active cab
    bool point_inside( Math3D::vector3 const Point ) const;
    Math3D::vector3 clamp_inside( Math3D::vector3 const &Point ) const;
    const screenentry_sequence & get_screens();

	float get_tacho();
	float get_tank_pressure();
	float get_pipe_pressure();
	float get_brake_pressure();
	float get_hv_voltage();
	std::array<float, 3> get_current();
	bool get_alarm();
	int get_drive_direction();

    void set_mainctrl(int);
    void set_scndctrl(int);
    void set_trainbrake(float);
    void set_localbrake(float);

	uint16_t id();
	bool pending_delete = false;
};

class train_table : public basic_table<TTrain> {
public:
    void update( double dt );
    TTrain *find_id( std::uint16_t const Id ) const;
};

//---------------------------------------------------------------------------
