/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "scene.h"
#include "Names.h"
#include "EvLaunch.h"
#include "Logs.h"
#include "lua.h"

// common event interface
class basic_event {

public:
// types
    enum flags {
        // shared values
        text        = 1 << 0,
        value_1     = 1 << 1,
        value_2     = 1 << 2,
        // update values
        load        = 1 << 3,
        mode_add    = 1 << 4,
        // condition values
        track_busy  = 1 << 3,
        track_free  = 1 << 4,
        probability = 1 << 5
    };
// constructor
    basic_event() = default;
// destructor
    virtual ~basic_event();
// methods
    
    static auto make( cParser& Input,
            state_serializer::scratch_data& Scratchpad ) -> basic_event*;

    // restores event data from provided stream
    virtual
    void
        deserialize( cParser &Input, scene::scratch_data &Scratchpad );
    // prepares event for use
    virtual
    void
        init() = 0;
    // executes event
    virtual
    void
        run();
    // sends basic content of the class in legacy (text) format to provided stream
    virtual
    void
        export_as_text( std::ostream &Output ) const;
    // adds a sibling event executed together
    void
        append( basic_event *Event );
    // returns: true if the event should be executed immediately
    virtual
    bool
        is_instant() const;
    // sends content of associated data cell to specified vehicle controller
    virtual
    void
        send_command( TController &Controller );
    // returns: true if associated data cell contains a command for vehicle controller
    virtual
    bool
        is_command() const;
    // input data access
    virtual std::string input_text() const;
    virtual TCommandType input_command() const;
    virtual double input_value( int Index ) const;
    virtual glm::dvec3 input_location() const;
    void group( scene::group_handle Group );
    scene::group_handle group() const;
// members
    basic_event *m_next { nullptr }; // następny w kolejce // TODO: replace with event list in the manager
    basic_event *m_sibling { nullptr }; // kolejny event z tą samą nazwą - od wersji 378
    std::string m_name;
    bool m_ignored { false }; // replacement for tp_ignored
    bool m_passive { false }; // false gdy ma nie być dodawany do kolejki (skanowanie sygnałów)
    int m_inqueue { 0 }; // ile razy dodany do kolejki
    TDynamicObject const *m_activator { nullptr };
    double m_launchtime { 0.0 };
    double m_delay { 0.0 };
    double m_delayrandom { 0.0 }; // zakres dodatkowego opóźnienia // standardowo nie będzie dodatkowego losowego opóźnienia

protected:
// types
    using basic_node = std::tuple<std::string, scene::basic_node *>;
    using node_sequence = std::vector<basic_node>;

    struct event_conditions {
        unsigned int flags { 0 };
        float probability { 0.0 }; // used by conditional_probability
        double match_value_1 { 0.0 }; // used by conditional_memcompare
        double match_value_2 { 0.0 }; // used by conditional_memcompare
        std::string match_text; // used by conditional_memcompare
        basic_event::node_sequence *cells; // used by conditional_memcompare
        std::vector<TTrack *> tracks; // used by conditional_track
        bool has_else { false };

        void deserialize( cParser &Input );
        void bind( basic_event::node_sequence *Nodes );
        void init();
        // verifies whether event meets execution condition(s)
        bool test() const;
        // sends basic content of the class in legacy (text) format to provided stream
        void export_as_text( std::ostream &Output ) const;
    };
// methods
    template <class TableType_>
    void init_targets( TableType_ &Repository, std::string const &Targettype, bool const Logerrors = true );
    // returns true if provided token is a an event desription keyword, false otherwise
    static bool is_keyword( std::string const &Token );

// members
    node_sequence m_targets; // targets of operation performed when this event is executed

private:
// methods
    // event type string
    virtual std::string type() const = 0;
    // deserialization helper, converts provided string to a list of target nodes
    virtual void deserialize_targets( std::string const &Input );
    // deserialize() subclass details
    virtual void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) = 0;
    // run() subclass details
    virtual void run_() = 0;
    // export_as_text() subclass details
    virtual void export_as_text_( std::ostream &Output ) const = 0;
// members
    scene::group_handle m_group { null_handle }; // group this event belongs to, if any
};



// specialized event, sends received input to its target(s)
// TBD: replace the generic module with specialized mixins
class input_event : public basic_event {

    friend class basic_event;

protected:
// types
    struct input_data {
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

// members
    input_data m_input;
};



class updatevalues_event : public input_event {

public:
// methods
    // prepares event for use
    void init() override;
    // returns: true if the event should be executed immediately
    bool is_instant() const override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    event_conditions m_conditions;
};



class copyvalues_event : public input_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
};



class getvalues_event : public input_event {

public:
// methods
    // prepares event for use
    void init() override;
    // sends content of associated data cell to specified vehicle controller
    void send_command( TController &Controller ) override;
    // returns: true if associated data cell contains a command for vehicle controller
    bool is_command() const override;
    // input data access
    std::string input_text() const override;
    TCommandType input_command() const override;
    double input_value( int Index ) const override;
    glm::dvec3 input_location() const override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
};



class putvalues_event : public input_event {

public:
// methods
    // prepares event for use
    void init() override;
    // input data access
    std::string input_text() const override;
    TCommandType input_command() const override;
    double input_value( int Index ) const override;
    glm::dvec3 input_location() const override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
};



class whois_event : public input_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
};



class logvalues_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
};



class multi_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// types
    // wrapper for binding between editor-supplied name, event, and execution conditional flag
    using conditional_event = std::tuple<std::string, basic_event *, bool>;
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    std::vector<conditional_event> m_children; // events which are placed in the query when this event is executed
    event_conditions m_conditions;
};

class sound_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// types
    // wrapper for binding between editor-supplied name and sound object
    using basic_sound = std::tuple<std::string, sound_source *>;
// methods
    // event type string
    std::string type() const override;
    // deserialization helper, converts provided string to a list of target nodes
    void deserialize_targets( std::string const &Input ) override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    std::vector<basic_sound> m_sounds;
    int m_soundmode{ 0 };
    int m_soundradiochannel{ 0 };
};

class animation_event : public basic_event {

public:
// destuctor
    ~animation_event() override;
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    int m_animationtype{ 0 };
    std::array<double, 4> m_animationparams{ 0.0 };
    std::string m_animationsubmodel;
    std::vector<TAnimContainer *> m_animationcontainers;
    std::string m_animationfilename;
    std::size_t m_animationfilesize{ 0 };
    char *m_animationfiledata{ nullptr };
};

class lights_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    std::vector<float> m_lights;
};

class switch_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    int m_switchstate{ 0 };
    float m_switchmoverate{ -1.f };
    float m_switchmovedelay{ -1.f };
};

class track_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    float m_velocity{ 0.f };
};

class voltage_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    float m_voltage{ -1.f };
};

class visible_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    bool m_visible{ true };
};

class friction_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    float m_friction{ -1.f };
};

class lua_event : public basic_event {
public:
    lua_event(lua::eventhandler_t func);
    void init() override;

private:
    std::string type() const override;
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    void run_() override;
    void export_as_text_( std::ostream &Output ) const override;

    lua::eventhandler_t lua_func = nullptr;
};

class message_event : public basic_event {

public:
// methods
    // prepares event for use
    void init() override;

private:
// methods
    // event type string
    std::string type() const override;
    // deserialize() subclass details
    void deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) override;
    // run() subclass details
    void run_() override;
    // export_as_text() subclass details
    void export_as_text_( std::ostream &Output ) const override;
// members
    std::string m_message;
};

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
        insert( basic_event *Event );
    inline
    bool
        insert( TEventLauncher *Launcher ) {
            return m_launchers.insert( Launcher ); }
    // returns first event in the queue
    inline
    basic_event *
        begin() {
            return QueryRootEvent; }
    // legacy method, returns pointer to specified event, or null
    basic_event *
        FindEvent( std::string const &Name );
    // legacy method, inserts specified event in the event query
    bool
        AddToQuery( basic_event *Event, TDynamicObject const *Owner, double delay = 0.0 );
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
    using event_sequence = std::deque<basic_event *>;
    using event_map = std::unordered_map<std::string, std::size_t>;
    using eventlauncher_sequence = std::vector<TEventLauncher *>;
// members
    event_sequence m_events;
/*
    // NOTE: disabled until event class refactoring
    event_sequence m_eventqueue;
*/
    // legacy version of the above
    basic_event *QueryRootEvent { nullptr };
    basic_event *m_workevent { nullptr };
    event_map m_eventmap;
    basic_table<TEventLauncher> m_launchers;
    eventlauncher_sequence m_launcherqueue;
};



inline
void
basic_event::group( scene::group_handle Group ) {
    m_group = Group;
}

inline
scene::group_handle
basic_event::group() const {
    return m_group;
}

template <class TableType_>
void
basic_event::init_targets( TableType_ &Repository, std::string const &Targettype, bool const Logerrors ) {

    for( auto &target : m_targets ) {
        std::get<scene::basic_node *>( target ) = Repository.find( std::get<std::string>( target ) );
        if( std::get<scene::basic_node *>( target ) == nullptr ) {
            m_ignored = true; // deaktywacja
            if( Logerrors )
                ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find " + Targettype +" \"" + std::get<std::string>( target ) + "\"" );
        }
    }
}

//---------------------------------------------------------------------------
