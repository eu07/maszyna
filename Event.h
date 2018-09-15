/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "classes.h"
#include "dumb3d.h"
#include "names.h"
#include "evlaunch.h"

//#define EU07_USE_LEGACY_EVENTS

#ifdef EU07_USE_LEGACY_EVENTS
enum TEventType {
    tp_Unknown,
    tp_Sound,
    tp_Exit,
    tp_Disable,
    tp_Velocity,
    tp_Animation,
    tp_Lights,
    tp_UpdateValues,
    tp_GetValues,
    tp_PutValues,
    tp_Switch,
    tp_DynVel,
    tp_TrackVel,
    tp_Multiple,
    tp_AddValues,
    tp_CopyValues,
    tp_WhoIs,
    tp_LogValues,
    tp_Visible,
    tp_Voltage,
    tp_Message,
    tp_Friction
};

const int update_memstring = 0x0000001; // zmodyfikować tekst (UpdateValues)
const int update_memval1 = 0x0000002; // zmodyfikować pierwszą wartosć
const int update_memval2 = 0x0000004; // zmodyfikować drugą wartosć
const int update_memadd = 0x0000008; // dodać do poprzedniej zawartości
const int update_load = 0x0000010; // odczytać ładunek
const int update_only = 0x00000FF; // wartość graniczna
const int conditional_memstring = 0x0000100; // porównanie tekstu
const int conditional_memval1 = 0x0000200; // porównanie pierwszej wartości liczbowej
const int conditional_memval2 = 0x0000400; // porównanie drugiej wartości
const int conditional_trackoccupied = 0x1000000; // jeśli tor zajęty
const int conditional_trackfree = 0x2000000; // jeśli tor wolny
const int conditional_propability = 0x4000000; // zależnie od generatora lizcb losowych
const int conditional_memcompare = 0x8000000; // porównanie zawartości

union TParam
{
    void *asPointer;
    TMemCell *asMemCell;
    scene::basic_node *asSceneNode;
    glm::dvec3 const *asLocation;
    TTrack *asTrack;
    TAnimModel *asModel;
    TAnimContainer *asAnimContainer;
    TTrain *asTrain;
    TDynamicObject *asDynamic;
    TEvent *asEvent;
    double asdouble;
    int asInt;
    sound_source *tsTextSound;
    char *asText;
    TCommandType asCommand;
    TTractionPowerSource *psPower;
};

class TEvent // zmienne: ev*
{ // zdarzenie
public:
// types
    // wrapper for binding between editor-supplied name, event, and execution conditional flag
    using conditional_event = std::tuple<std::string, TEvent *, bool>;
// constructors
    TEvent(std::string const &m = "");
    ~TEvent();
// metody
    void Load(cParser *parser, Math3D::vector3 const &org);
    // sends basic content of the class in legacy (text) format to provided stream
    void
        export_as_text( std::ostream &Output ) const;
    static void AddToQuery( TEvent *Event, TEvent *&Start );
    std::string CommandGet();
    TCommandType Command();
    double ValueGet(int n);
    glm::dvec3 PositionGet() const;
    bool StopCommand();
    void StopCommandSent();
    void Append(TEvent *e);
    void
        group( scene::group_handle Group );
    scene::group_handle
        group() const;
// members
    std::string asName;
    bool m_ignored { false }; // replacement for tp_ignored
    bool bEnabled = false; // false gdy ma nie być dodawany do kolejki (skanowanie sygnałów)
    int iQueued = 0; // ile razy dodany do kolejki
    TEvent *evNext = nullptr; // następny w kolejce
    TEventType Type = tp_Unknown;
    double fStartTime = 0.0;
    double fDelay = 0.0;
    TDynamicObject const *Activator = nullptr;
    TParam Params[13]; // McZapkie-070502 //Ra: zamienić to na union/struct
    unsigned int iFlags = 0; // zamiast Params[8] z flagami warunku
    std::string asNodeName; // McZapkie-100302 - dodalem zeby zapamietac nazwe toru
    TEvent *evJoined = nullptr; // kolejny event z tą samą nazwą - od wersji 378
    double fRandomDelay = 0.0; // zakres dodatkowego opóźnienia // standardowo nie będzie dodatkowego losowego opóźnienia
    std::vector<conditional_event> m_children; // events which are placed in the query when this event is executed
    bool m_conditionalelse { false }; // TODO: make a part of condition struct

private:
// methods
    void Conditions( cParser *parser, std::string s );
// members
    scene::group_handle m_group { null_handle }; // group this event belongs to, if any
};

inline
void
TEvent::group( scene::group_handle Group ) {
    m_group = Group;
}

inline
scene::group_handle
TEvent::group() const {
    return m_group;
}

class event_manager {

public:
// constructors
    event_manager() = default;
// destructor
    ~event_manager();
// methods
    // adds specified event launcher to the list of global launchers
    void
        queue( TEventLauncher *Launcher );
    // legacy method, updates event queues
    void
        update();
    // adds provided event to the collection. returns: true on success
    // TBD, TODO: return handle to the event
    bool
        insert( TEvent *Event );
    bool
        insert( TEventLauncher *Launcher ) {
            return m_launchers.insert( Launcher ); }
    // returns first event in the queue
    TEvent *
        begin() {
            return QueryRootEvent; }
    // legacy method, returns pointer to specified event, or null
    TEvent *
        FindEvent( std::string const &Name );
    // legacy method, inserts specified event in the event query
    bool
        AddToQuery( TEvent *Event, TDynamicObject const *Owner );
    // legacy method, executes queued events
    bool
        CheckQuery();
    // legacy method, initializes events after deserialization from scenario file
    void
        InitEvents();
    // legacy method, initializes event launchers after deserialization from scenario file
    void
        InitLaunchers();
    // sends basic content of the class in legacy (text) format to provided stream
    void
        export_as_text( std::ostream &Output ) const;

private:
// types
    using event_sequence = std::deque<TEvent *>;
    using event_map = std::unordered_map<std::string, std::size_t>;
    using eventlauncher_sequence = std::vector<TEventLauncher *>;

// methods
    // legacy method, verifies condition for specified event
    bool
        EventConditon( TEvent *Event );

// members
    event_sequence m_events;
/*
    // NOTE: disabled until event class refactoring
    event_sequence m_eventqueue;
*/
    // legacy version of the above
    TEvent *QueryRootEvent { nullptr };
    TEvent *m_workevent { nullptr };
    event_map m_eventmap;
    basic_table<TEventLauncher> m_launchers;
    eventlauncher_sequence m_launcherqueue;
};
#else
enum TEventType {
    tp_Unknown,
    tp_Sound,
    tp_Animation,
    tp_Lights,
    tp_UpdateValues,
    tp_GetValues,
    tp_PutValues,
    tp_Switch,
    tp_TrackVel,
    tp_Multiple,
    tp_AddValues,
    tp_CopyValues,
    tp_WhoIs,
    tp_LogValues,
    tp_Visible,
    tp_Voltage,
    tp_Message,
    tp_Friction
};

const int update_memstring = 0x0000001; // zmodyfikować tekst (UpdateValues)
const int update_memval1 = 0x0000002; // zmodyfikować pierwszą wartosć
const int update_memval2 = 0x0000004; // zmodyfikować drugą wartosć
const int update_memadd = 0x0000008; // dodać do poprzedniej zawartości
const int update_load = 0x0000010; // odczytać ładunek
const int conditional_memstring = 0x0000100; // porównanie tekstu
const int conditional_memval1 = 0x0000200; // porównanie pierwszej wartości liczbowej
const int conditional_memval2 = 0x0000400; // porównanie drugiej wartości
const int conditional_trackoccupied = 0x1000000; // jeśli tor zajęty
const int conditional_trackfree = 0x2000000; // jeśli tor wolny
const int conditional_propability = 0x4000000; // zależnie od generatora lizcb losowych
const int conditional_memcompare = 0x8000000; // porównanie zawartości

// zdarzenie
class TEvent  {

    friend event_manager;

public:
// types
    // wrapper for binding between editor-supplied name, event, and execution conditional flag
    using conditional_event = std::tuple<std::string, TEvent *, bool>;
    using basic_node = std::tuple<std::string, scene::basic_node *>;
    using basic_sound = std::tuple<std::string, sound_source *>;
// constructors
    TEvent();
    ~TEvent();
// metody
    void Load(cParser *parser, Math3D::vector3 const &org);
    // sends basic content of the class in legacy (text) format to provided stream
    template <class TableType_>
    void
        init_targets( TableType_ &Repository, std::string const &Targettype, bool const Logerrors = true );
    void
        init_conditions();
    void
        export_as_text( std::ostream &Output ) const;
    static void AddToQuery( TEvent *Event, TEvent *&Start );
    std::string CommandGet() const;
    TCommandType Command() const;
    double ValueGet(int n) const;
    glm::dvec3 PositionGet() const;
    bool StopCommand() const;
    void StopCommandSent();
    void Append(TEvent *e);
    void
        group( scene::group_handle Group );
    scene::group_handle
        group() const;
// members
    std::string asName;
    TEventType Type = tp_Unknown;
    TEvent *evNext = nullptr; // następny w kolejce
    bool m_ignored{ false }; // replacement for tp_ignored
    bool bEnabled = false; // false gdy ma nie być dodawany do kolejki (skanowanie sygnałów)
    int iQueued = 0; // ile razy dodany do kolejki
    TDynamicObject const *Activator = nullptr;
    double fStartTime = 0.0;
    double fDelay = 0.0;
    double fRandomDelay = 0.0; // zakres dodatkowego opóźnienia // standardowo nie będzie dodatkowego losowego opóźnienia
    TEvent *evJoined = nullptr; // kolejny event z tą samą nazwą - od wersji 378
    std::vector<basic_node> m_targets; // targets of operation performed when this event is executed
    std::vector<conditional_event> m_children; // events which are placed in the query when this event is executed
    std::vector<float> m_lights; // for tp_lights
    bool m_visible { true }; // for tp_visible
    double m_velocity { 0.0 }; // for tp_trackvel
    std::vector<basic_sound> m_sounds; // for tp_sound
    int m_soundmode{ 0 };
    int m_soundradiochannel{ 0 };
    int m_animationtype{ 0 }; // for tp_animation
    std::array<double, 4> m_animationparams{ 0.0 };
    std::string m_animationsubmodel;
    std::vector<TAnimContainer *> m_animationcontainers;
    std::string m_animationfilename;
    std::size_t m_animationfilesize{ 0 };
    char *m_animationfiledata{ nullptr };
    int m_switchstate{ 0 }; // for tp_switch
    float m_switchmoverate{ -1.f };
    float m_switchmovedelay{ -1.f };
    float m_friction { -1.f }; // for tp_friction
    float m_voltage{ -1.f }; // for tp_voltage

private:
// types
    struct update_data {
        unsigned int flags { 0 };
        std::string data_text;
        double data_value_1 { 0.0 };
        double data_value_2 { 0.0 };
        basic_node data_source { "", nullptr };
        glm::dvec3 location { 0.0 };
        TCommandType command_type { TCommandType::cm_Unknown };

        TMemCell const * data_cell() const;
        TMemCell * data_cell();
    };
    struct condition_data {
        unsigned int flags { 0 };
        float probability { 0.0 }; // used by conditional_probability
        double match_value_1 { 0.0 }; // used by conditional_memcompare
        double match_value_2 { 0.0 }; // used by conditional_memcompare
        std::string match_text; // used by conditional_memcompare
        std::vector<TTrack *> tracks; // used by conditional_track
        bool has_else { false };

        void load( cParser &Input );
    };
// methods
    void load_targets( std::string const &Input );
    std::string type_to_string() const;
// members
    scene::group_handle m_group { null_handle }; // group this event belongs to, if any
    update_data m_update;
    condition_data m_condition;
};

inline
void
TEvent::group( scene::group_handle Group ) {
    m_group = Group;
}

inline
scene::group_handle
TEvent::group() const {
    return m_group;
}

template <class TableType_>
void
TEvent::init_targets( TableType_ &Repository, std::string const &Targettype, bool const Logerrors ) {

    for( auto &target : m_targets ) {
        std::get<scene::basic_node *>( target ) = Repository.find( std::get<std::string>( target ) );
        if( std::get<scene::basic_node *>( target ) == nullptr ) {
            m_ignored = true; // deaktywacja
            if( Logerrors )
                ErrorLog( "Bad event: " + type_to_string() + " event \"" + asName + "\" cannot find " + Targettype +" \"" + std::get<std::string>( target ) + "\"" );
        }
    }
}



class event_manager {

public:
// constructors
    event_manager() = default;
// destructor
    ~event_manager();
// methods
    // adds specified event launcher to the list of global launchers
    void
        queue( TEventLauncher *Launcher );
    // legacy method, updates event queues
    void
        update();
    // adds provided event to the collection. returns: true on success
    // TBD, TODO: return handle to the event
    bool
        insert( TEvent *Event );
    bool
        insert( TEventLauncher *Launcher ) {
            return m_launchers.insert( Launcher ); }
    // returns first event in the queue
    TEvent *
        begin() {
            return QueryRootEvent; }
    // legacy method, returns pointer to specified event, or null
    TEvent *
        FindEvent( std::string const &Name );
    // legacy method, inserts specified event in the event query
    bool
        AddToQuery( TEvent *Event, TDynamicObject const *Owner );
    // legacy method, executes queued events
    bool
        CheckQuery();
    // legacy method, initializes events after deserialization from scenario file
    void
        InitEvents();
    // legacy method, initializes event launchers after deserialization from scenario file
    void
        InitLaunchers();
    // sends basic content of the class in legacy (text) format to provided stream
    void
        export_as_text( std::ostream &Output ) const;

private:
// types
    using event_sequence = std::deque<TEvent *>;
    using event_map = std::unordered_map<std::string, std::size_t>;
    using eventlauncher_sequence = std::vector<TEventLauncher *>;

// methods
    // legacy method, verifies condition for specified event
    bool
        EventConditon( TEvent *Event );

// members
    event_sequence m_events;
/*
    // NOTE: disabled until event class refactoring
    event_sequence m_eventqueue;
*/
    // legacy version of the above
    TEvent *QueryRootEvent { nullptr };
    TEvent *m_workevent { nullptr };
    event_map m_eventmap;
    basic_table<TEventLauncher> m_launchers;
    eventlauncher_sequence m_launcherqueue;
};
#endif
//---------------------------------------------------------------------------
