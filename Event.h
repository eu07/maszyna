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
#include "dumb3d.h"
#include "Names.h"
#include "EvLaunch.h"

enum TEventType {
    tp_Unknown,
    tp_Sound,
    tp_SoundPos,
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
//    tp_Ignored, // NOTE: refactored to separate flag
    tp_CopyValues,
    tp_WhoIs,
    tp_LogValues,
    tp_Visible,
    tp_Voltage,
    tp_Message,
    tp_Friction,
    tp_Lua
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
const int conditional_anyelse = 0x0FF0000; // do sprawdzania, czy są odwrócone warunki
const int conditional_trackoccupied = 0x1000000; // jeśli tor zajęty
const int conditional_trackfree = 0x2000000; // jeśli tor wolny
const int conditional_propability = 0x4000000; // zależnie od generatora lizcb losowych
const int conditional_memcompare = 0x8000000; // porównanie zawartości

union TParam
{
    void *asPointer;
    TMemCell *asMemCell;
    editor::basic_node *asEditorNode;
    glm::dvec3 const *asLocation;
    TTrack *asTrack;
    TAnimModel *asModel;
    TAnimContainer *asAnimContainer;
    TTrain *asTrain;
    TDynamicObject *asDynamic;
    TEvent *asEvent;
    bool asBool;
    double asdouble;
    int asInt;
    sound_source *tsTextSound;
    char *asText;
    TCommandType asCommand;
    TTractionPowerSource *psPower;
};

class TEvent // zmienne: ev*
{ // zdarzenie
  private:
    void Conditions(cParser *parser, std::string s);

  public:
    std::string asName;
    bool m_ignored { false }; // replacement for tp_ignored
    bool bEnabled = false; // false gdy ma nie być dodawany do kolejki (skanowanie sygnałów)
    int iQueued = 0; // ile razy dodany do kolejki
    TEvent *evNext = nullptr; // następny w kolejce
    TEventType Type = tp_Unknown;
    double fStartTime = 0.0;
    double fDelay = 0.0;
    TDynamicObject *Activator = nullptr;
    TParam Params[13]; // McZapkie-070502 //Ra: zamienić to na union/struct

	// this is already mess, so one more hack won't make it much worse...
	// stores TParam union and magic flag (eg. else flag)
	std::vector<std::pair<TParam, int>> ext_params;

    unsigned int iFlags = 0; // zamiast Params[8] z flagami warunku
    std::string asNodeName; // McZapkie-100302 - dodalem zeby zapamietac nazwe toru
    TEvent *evJoined = nullptr; // kolejny event z tą samą nazwą - od wersji 378
    double fRandomDelay = 0.0; // zakres dodatkowego opóźnienia // standardowo nie będzie dodatkowego losowego opóźnienia
public:
    // metody
    TEvent(std::string const &m = "");
    ~TEvent();
    void Load(cParser *parser, Math3D::vector3 const &org);
    static void AddToQuery( TEvent *Event, TEvent *&Start );
    std::string CommandGet();
    TCommandType Command();
    double ValueGet(int n);
    glm::dvec3 PositionGet() const;
    bool StopCommand();
    void StopCommandSent();
    void Append(TEvent *e);
};

class event_manager {

public:
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
        AddToQuery( TEvent *Event, TDynamicObject *Owner, double delay = 0.0 );
    // legacy method, executes queued events
    bool
        CheckQuery();
    // legacy method, initializes events after deserialization from scenario file
    void
        InitEvents();
    // legacy method, initializes event launchers after deserialization from scenario file
    void
        InitLaunchers();

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

//---------------------------------------------------------------------------
