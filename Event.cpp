/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "stdafx.h"
#include "Event.h"

#include "simulation.h"
#include "simulationtime.h"
#include "simulationsounds.h"
#include "messaging.h"
#include "Globals.h"
#include "MemCell.h"
#include "Track.h"
#include "Traction.h"
#include "TractionPower.h"
#include "sound.h"
#include "AnimModel.h"
#include "DynObj.h"
#include "Driver.h"
#include "renderer.h"
#include "Timer.h"
#include "Logs.h"
#include "widgets/map_objects.h"

void
basic_event::event_conditions::bind( basic_event::node_sequence *Nodes ) {

    memcompare_cells = Nodes;
}

void
basic_event::event_conditions::init() {

    tracks.clear();

    if( flags & ( flags::track_busy | flags::track_free ) ) {
        for( auto &target : *memcompare_cells ) {
            tracks.emplace_back( simulation::Paths.find( std::get<std::string>( target ) ) );
            if( tracks.back() == nullptr ) {
                // legacy compatibility behaviour, instead of disabling the event we disable the memory cell comparison test
//                m_ignored = true; // deaktywacja
//                ErrorLog( "Bad event: track \"" + std::get<std::string>( target ) + "\" referenced in event \"" + asName + "\" doesn't exist" );
                flags &= ~( flags::track_busy | flags::track_free ); // zerowanie flag
            }
        }
    }
}

bool
basic_event::event_conditions::test() const {

    if( flags == 0 ) {
        return true; // bezwarunkowo
    }
    // if there's conditions, check them
    if( flags & flags::probability ) {
        auto const randomroll { static_cast<float>( Random() ) };
        WriteLog(
            "Test: Random integer - ["
            + std::to_string( randomroll ) + "] / [" + std::to_string( probability )
            + "] - "
            + ( randomroll > probability ? "Pass" : "Fail" ) );
        if( randomroll > probability ) {
            return false;
        }
    }
    if( flags & flags::track_busy ) {
        auto trackbusyresult { true };
        std::string trackbusylog { "Test: Track busy - " };
        for( auto *track : tracks ) {
            if( true == track->IsEmpty() ) {
                trackbusylog += "[" + track->name() + "]";
                trackbusyresult = false;
                break;
            }
            else {
                auto const &vehicles { track->Dynamics };
                if( trackbusylog.back() == ']' ) {
                    trackbusylog += ", ";
                }
                trackbusylog += "[" + vehicles.front()->asName + "] @ [" + track->name() + "]";
            }
        }

        WriteLog(
            trackbusylog
            + " - "
            + ( trackbusyresult ? "Pass" : "Fail" ) );

        if( false == trackbusyresult ) {
            return false;
        }
    }
    if( flags & flags::track_free ) {
        auto trackfreeresult { true };
        std::string trackfreelog{ "Test: Track free - " };
        for( auto *track : tracks ) {
            if( false == track->IsEmpty() ) {
                auto const &vehicles { track->Dynamics };
                trackfreelog += "[" + vehicles.front()->asName + "] @ [" + track->name() + "] - ";
                trackfreeresult = false;
                break;
            }
        }

        WriteLog(
            trackfreelog
            + ( trackfreeresult ? "Pass" : "Fail" ) );

        if( false == trackfreeresult ) {
            return false;
        }
    }
    if( flags & ( flags::text | flags::value1 | flags::value2 ) ) {
        // porównanie wartości
        for( auto &cellwrapper : *memcompare_cells ) {
            auto *cell { static_cast<TMemCell *>( std::get<scene::basic_node *>( cellwrapper ) ) };
            if( cell == nullptr ) {
//                ErrorLog( "Event " + asName + " trying conditional_memcompare with nonexistent memcell" );
                continue; // though this is technically error, we treat it as a success to maintain backward compatibility
            }
            auto const comparisonresult =
                cell->Compare(
                    memcompare_text, memcompare_value1, memcompare_value2,
                    flags,
                    memcompare_text_operator, memcompare_value1_operator, memcompare_value2_operator,
                    memcompare_pass );

            auto const combiner { (
                memcompare_pass == comparison_pass::all ? " && " :
                memcompare_pass == comparison_pass::any ? " || " :
                memcompare_pass == comparison_pass::none ? " !! " :
                " ?? " ) };

            std::string comparisonlog = "Test: MemCompare - " + cell->name() + " - ";

            comparisonlog +=
                ( TestFlag( flags, flags::text ) ?
                    "[" + cell->Text() + "] " + to_string( memcompare_text_operator ) + " [" + memcompare_text + "]" :
                    "[*]" )
                + combiner;

            comparisonlog +=
                ( TestFlag( flags, flags::value1 ) ?
                    "[" + to_string( cell->Value1(), 2 ) + "] " + to_string( memcompare_value1_operator ) + " [" + to_string( memcompare_value1, 2 ) + "]" :
                    "[*]" )
                + combiner;

            comparisonlog +=
                ( TestFlag( flags, flags::value2 ) ?
                    "[" + to_string( cell->Value2(), 2 ) + "] " + to_string( memcompare_value2_operator ) + " [" + to_string( memcompare_value2, 2 ) + "]" :
                    "[*]" )
                + " - ";

            comparisonlog += ( comparisonresult ? "Pass" : "Fail" );

            WriteLog( comparisonlog );

            if( false == comparisonresult ) {
                return false;
            }
        }
    }
    // if we made it here it means we passed all checks
    return true;
}

void
basic_event::event_conditions::deserialize( cParser &Input ) {
    // przetwarzanie warunków, wspólne dla Multiple i UpdateValues

    std::string token;
    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( false == basic_event::is_keyword( token ) ) ) {

        if( token == "trackoccupied" ) {
            flags |= flags::track_busy;
        }
        else if( token == "trackfree" ) {
            flags |= flags::track_free;
        }
        else if( ( token == "propability" ) || ( token == "probability" )) { //remove propability in few years after changing old scenery scripts 01.2021
            flags |= flags::probability;
            Input.getTokens();
            Input >> probability;
        }
        else if( token == "memcompare" ) {
            Input.getTokens( 1, false ); // case sensitive
            if( Input.peek() != "*" ) //"*" - nie brac command pod uwage
            { // zapamiętanie łańcucha do porównania
                Input >> memcompare_text;
                flags |= flags::text;
            }
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac val1 pod uwage
            {
                Input >> memcompare_value1;
                flags |= flags::value1;
            }
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac val2 pod uwage
            {
                Input >> memcompare_value2;
                flags |= flags::value2;
            }
        }
        else if( token == "memcompareex" ) {
            memcompare_pass = comparison_pass_from_string( Input.getToken<std::string>() );
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac pod uwage
            { // two tokens, operator followed by comparison value
                std::string operatorstring;
                Input >> operatorstring;
                memcompare_text_operator = comparison_operator_from_string( operatorstring );
                Input.getTokens( 1, false ); // case sensitive
                Input >> memcompare_text;
                flags |= flags::text;
            }
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac pod uwage
            { // two tokens, operator followed by comparison value
                std::string operatorstring;
                Input >> operatorstring;
                memcompare_value1_operator = comparison_operator_from_string( operatorstring );
                Input.getTokens();
                Input >> memcompare_value1;
                flags |= flags::value1;
            }
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac pod uwage
            { // two tokens, operator followed by comparison value
                std::string operatorstring;
                Input >> operatorstring;
                memcompare_value2_operator = comparison_operator_from_string( operatorstring );
                Input.getTokens();
                Input >> memcompare_value2;
                flags |= flags::value2;
            }
        }
    }
}

// sends basic content of the class in legacy (text) format to provided stream
void
basic_event::event_conditions::export_as_text( std::ostream &Output ) const {

    if( flags != 0 ) {
        Output << "condition ";
        if( ( flags & flags::track_busy ) != 0 ) {
            Output << "trackoccupied ";
        }
        if( ( flags & flags::track_free ) != 0 ) {
            Output << "trackfree ";
        }
        if( ( flags & flags::probability ) != 0 ) {
            Output
                << "probability "
                << probability << ' ';
        }
        if( ( flags & ( flags::text | flags::value1 | flags::value2 ) ) != 0 ) {
            // NOTE: export doesn't preserve original memcompare condition, these are all upgraded to memcompareex format for simplicity
            Output
                << "memcompareex "
                << ( ( flags & flags::text )   == 0 ? "*" :            memcompare_text     + ' ' + to_string( memcompare_text_operator ) ) << ' '
                << ( ( flags & flags::value1 ) == 0 ? "*" : to_string( memcompare_value1 ) + ' ' + to_string( memcompare_value1_operator ) ) << ' '
                << ( ( flags & flags::value2 ) == 0 ? "*" : to_string( memcompare_value2 ) + ' ' + to_string( memcompare_value2_operator ) ) << ' ';
        }
    }
}

basic_event::~basic_event() {

    m_sibling = nullptr; // nie usuwać podczepionych tutaj
}

void
basic_event::deserialize( cParser &Input, scene::scratch_data &Scratchpad ) {

    std::string token;

    Input.getTokens();
    Input >> m_delay;

    Input.getTokens();
    Input >> token;
    deserialize_targets( token );

    if( starts_with( m_name, "none_" ) ) {
        m_ignored = true; // Ra: takie są ignorowane
    }

    deserialize_( Input, Scratchpad );
    // subclass method is expected to leave next token past its own data preloaded on its exit
    while( ( false == ( token = Input.peek() ).empty() )
        && ( token != "endevent" ) ) {

        if( token == "randomdelay" ) { // losowe opóźnienie
            Input.getTokens();
            Input >> m_delayrandom; // Ra 2014-03-11
        }
        if( token == "departuredelay" ) { // timetable-based delay
            Input.getTokens();
            Input >> m_delaydeparture;
        }
        Input.getTokens();
    }
}

void
basic_event::deserialize_targets( std::string const &Input ) {

    cParser targetparser { Input };
    std::string target;
    while( false == ( target = targetparser.getToken<std::string>( true, "|," ) ).empty() ) {
        // actual bindings to targets of proper type are created during scenario initialization
        if( target != "none" ) {
            m_targets.emplace_back( target, nullptr );
        }
    }
}

void
basic_event::run() {

    WriteLog( "EVENT LAUNCHED" + ( m_activator ? ( " by " + m_activator->asName ) : "" ) + ": " + m_name );
    run_();
}

// sends basic content of the class in legacy (text) format to provided stream
void
basic_event::export_as_text( std::ostream &Output ) const {
    // header
    Output << "event ";
    // name
    Output << m_name << ' ';
    // type
    Output << type() << ' ';
    // delay
    Output << m_delay << ' ';
    // target node(s)
    if( m_targets.empty() ) {
        Output << "none";
    }
    else {
        auto targetidx { 0 };
        for( auto &target : m_targets ) {
            auto *targetnode { std::get<scene::basic_node *>( target ) };
            Output
                << ( targetnode != nullptr ? targetnode->name() : std::get<std::string>( target ) )
                << ( ++targetidx < m_targets.size() ? '|' : ' ' );
        }
    }
    // type-specific attributes
    export_as_text_( Output );

    if( m_delayrandom != 0.0 ) {
        Output
            << "randomdelay "
            << m_delayrandom << ' ';
    }
    if( false == std::isnan( m_delayrandom ) ) {
        Output
            << "departuredelay "
            << m_delaydeparture << ' ';
    }
    // footer
    Output
        << "endevent"
        << "\n";
}

void
basic_event::append( basic_event *Event ) {
    // doczepienie kolejnych z tą samą nazwą
    // TODO: remove recursion
    if( m_sibling )
        m_sibling->append( Event ); // rekurencja! - góra kilkanaście eventów będzie potrzebne
    else {
        m_sibling = Event;
        Event->m_passive = false; // ten doczepiony może być tylko kolejkowany
    }
};

// returns: true if the event should be executed immediately
bool
basic_event::is_instant() const {

    return false;
}

// sends content of associated data cell to specified vehicle controller
void
basic_event::send_command( TController &Controller ) {

    return;
}

// returns: true if associated data cell contains a command for vehicle controller
bool
basic_event::is_command() const {

    return false;
};

std::string
basic_event::input_text() const {
    // odczytanie komendy z eventu
    return "";
};

TCommandType
basic_event::input_command() const {
    // odczytanie komendy z eventu
    return TCommandType::cm_Unknown;
};

double
basic_event::input_value( int Index ) const {
    // odczytanie komendy z eventu
    return 0.0;
};

glm::dvec3
basic_event::input_location() const {
    // pobranie współrzędnych eventu
    return glm::dvec3( 0, 0, 0 );
};

bool
basic_event::is_keyword( std::string const &Token ) {
    // TODO: convert to array lookup if keyword list gets longer
    return ( Token == "endevent" )
        || ( Token == "randomdelay" )
        || ( Token == "departuredelay" );
}



TMemCell const *
input_event::input_data::data_cell() const {

    return static_cast<TMemCell const *>( std::get<scene::basic_node *>( data_source ) );
}
TMemCell *
input_event::input_data::data_cell() {

    return static_cast<TMemCell *>( std::get<scene::basic_node *>( data_source ) );
}



// prepares event for use
void
updatevalues_event::init() {
    // sumowanie wartości // zmiana wartości
    init_targets( simulation::Memory, "memory cell" );
    // conditional data
    m_conditions.bind( &m_targets );
    m_conditions.init();
}

// event type string
std::string
updatevalues_event::type() const {

    return (
        ( m_input.flags & flags::mode_add ) == 0 ?
            "updatevalues" :
            "addvalues" );
}

// deserialize() subclass details
void
updatevalues_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens( 1, false ); // case sensitive
    Input >> m_input.data_text;
    if( m_input.data_text != "*" ) { //"*" - nie brac command pod uwage
        m_input.flags |= flags::text;
    }
    Input.getTokens();
    if( Input.peek() != "*" ) { //"*" - nie brac val1 pod uwage
        Input >> m_input.data_value_1;
        m_input.flags |= flags::value1;
    }
    Input.getTokens();
    if( Input.peek() != "*" ) { //"*" - nie brac val2 pod uwage
        Input >> m_input.data_value_2;
        m_input.flags |= flags::value2;
    }
    Input.getTokens();
    // optional blocks
    std::string token;
    while( ( false == ( token = Input.peek() ).empty() )
        && ( false == is_keyword( token ) ) ) {

        if( token == "condition" ) {
            m_conditions.deserialize( Input );
            // NOTE: condition deserialization leaves preloaded next token
        }
    }
}

// run() subclass details
// TODO: update and copy values run_ methods are largely identical, refactor to a single helper
void
updatevalues_event::run_() {

    if( false == m_conditions.test() ) { return; }

    WriteLog( "Type: " + std::string( ( m_input.flags & flags::mode_add ) ? "AddValues" : "UpdateValues" ) + " & Track command - ["
        + ( ( m_input.flags & flags::text ) ? m_input.data_text : "X" ) + "] ["
        + ( ( m_input.flags & flags::value1 ) ? to_string( m_input.data_value_1, 2 ) : "X" ) + "] ["
        + ( ( m_input.flags & flags::value2 ) ? to_string( m_input.data_value_2, 2 ) : "X" ) + "]" );
    for( auto &target : m_targets ) {
        auto *targetcell { static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) ) };
        if( targetcell == nullptr ) { continue; }
        targetcell->UpdateValues(
            m_input.data_text,
            m_input.data_value_1,
            m_input.data_value_2,
            m_input.flags );
        WriteLog( " Memcell: " + targetcell->name() + " - " + targetcell->Values() );
//        targetcell->LogValues();
        if( targetcell->Track == nullptr ) { continue; }
        // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
        auto const location { targetcell->location() };
        for( auto vehicle : targetcell->Track->Dynamics ) {
            if( vehicle->Mechanik ) {
                WriteLog( " Vehicle: [" + vehicle->name() + "]" );
                targetcell->PutCommand(
                    vehicle->Mechanik,
                    &location );
            }
        }
    }

	map::Objects.poi_dirty = true; // it could potentially change map icons
}

// export_as_text() subclass details
void
updatevalues_event::export_as_text_( std::ostream &Output ) const {

    Output
        << ( ( m_input.flags & flags::text )   == 0 ? "*" : m_input.data_text ) << ' '
        << ( ( m_input.flags & flags::value1 ) == 0 ? "*" : to_string( m_input.data_value_1 ) ) << ' '
        << ( ( m_input.flags & flags::value2 ) == 0 ? "*" : to_string( m_input.data_value_2 ) ) << ' ';

    m_conditions.export_as_text( Output );
}

// returns: true if the event should be executed immediately
bool
updatevalues_event::is_instant() const {

    return ( ( ( m_input.flags & flags::mode_add ) != 0 ) && ( m_delay == 0.0 ) );
}



// prepares event for use
void
getvalues_event::init() {

    init_targets( simulation::Memory, "memory cell" );
    // custom target initialization code
    for( auto &target : m_targets ) {
        auto *targetcell { static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) ) };
        if( targetcell == nullptr ) { continue; }
        if( targetcell->IsVelocity() ) {
            // jeśli odczyt komórki a komórka zawiera komendę SetVelocity albo ShuntVelocity
            // to event nie będzie dodawany do kolejki
            m_passive = true;
        }
    }
    if( m_targets.empty() ) {
        m_ignored = true;
        return;
    }
    // NOTE: GetValues retrieves data only from first specified memory cell
    // TBD, TODO: allow retrieval from more than one cell?
    m_input.data_source = m_targets.front();
}

// event type string
std::string
getvalues_event::type() const {

    return "getvalues";
}

// deserialize() subclass details
void
getvalues_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // nothing to do here, just preload next token
    Input.getTokens();
}

// run() subclass details
void
getvalues_event::run_() {

    auto const *cell { m_input.data_cell() };

    WriteLog( "Type: GetValues - "
        + cell->name() + " - ["
        + cell->Text() + "] ["
        + to_string( cell->Value1(), 2 ) + "] ["
        + to_string( cell->Value2(), 2 ) + "]" );

    if( m_activator == nullptr ) { return; }

    cell->PutCommand(
        m_activator->Mechanik,
        &( cell->location() ) );

    // potwierdzenie wykonania dla serwera (odczyt semafora już tak nie działa)
    if( Global.iMultiplayer ) {
        multiplayer::WyslijEvent( m_name, m_activator->name() );
    }
}

// export_as_text() subclass details
void
getvalues_event::export_as_text_( std::ostream &Output ) const {
    // nothing to do here
}

// sends content of associated data cell to specified vehicle controller
void
getvalues_event::send_command( TController &Controller ) {

    Controller.PutCommand( input_text(), input_value( 1 ), input_value( 2 ), nullptr );
    m_input.data_cell()->StopCommandSent(); // komenda z komórki została wysłana
}
// returns: true if associated data cell contains a command for vehicle controller
bool
getvalues_event::is_command() const {
    // info o komendzie z komórki
    return m_input.data_cell()->StopCommand();
}

// input data access
std::string
getvalues_event::input_text() const {

    return m_input.data_cell()->Text();
}

TCommandType
getvalues_event::input_command() const {

    return m_input.data_cell()->Command();
}

double
getvalues_event::input_value( int Index ) const {

    Index &= 1; // tylko 1 albo 2 jest prawidłowy
    return ( Index == 1 ? m_input.data_cell()->Value1() : m_input.data_cell()->Value2() );
}

glm::dvec3
getvalues_event::input_location() const {

    return m_input.data_cell()->location(); // współrzędne podłączonej komórki pamięci
}



// prepares event for use
void
putvalues_event::init() {
    // nothing to do here
}

// event type string
std::string
putvalues_event::type() const {

    return "putvalues";
}

// deserialize() subclass details
void
putvalues_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens( 3 );
    // location, previously held in param 3, 4, 5
    Input
        >> m_input.location.x
        >> m_input.location.y
        >> m_input.location.z;
    // przesunięcie
    // tmp->pCenter.RotateY(aRotate.y/180.0*M_PI); //Ra 2014-11: uwzględnienie rotacji
    // TBD, TODO: take into account rotation as well?
    if( false == Scratchpad.location.offset.empty() ) {
        m_input.location += Scratchpad.location.offset.top();
    }

    std::string token;
    Input.getTokens( 1, false ); // komendy 'case sensitive'
    Input >> token;
    // command type, previously held in param 6
    if( starts_with( token, "PassengerStopPoint:" ) ) {
        if( contains( token, '#' ) ) {
            token.erase( token.find( '#' ) ); // obcięcie unikatowości
        }
        win1250_to_ascii( token ); // get rid of non-ascii chars
        m_input.command_type = TCommandType::cm_PassengerStopPoint;
        // nie do kolejki (dla SetVelocity też, ale jak jest do toru dowiązany)
        m_passive = true;
    }
    else if( token == "SetVelocity" ) {
        m_input.command_type = TCommandType::cm_SetVelocity;
        m_passive = true;
    }
    else if( token == "RoadVelocity" ) {
        m_input.command_type = TCommandType::cm_RoadVelocity;
        m_passive = true;
    }
    else if( token == "SectionVelocity" ) {
        m_input.command_type = TCommandType::cm_SectionVelocity;
        m_passive = true;
    }
    else if( token == "ShuntVelocity" ) {
        m_input.command_type = TCommandType::cm_ShuntVelocity;
        m_passive = true;
    }
    else if( token == "OutsideStation" ) {
        m_input.command_type = TCommandType::cm_OutsideStation;
        m_passive = true; // ma być skanowny, aby AI nie przekraczało W5
    }
    else if( token == "CabSignal" ) {
        m_input.command_type = TCommandType::cm_SecuritySystemMagnet;
        m_passive = true;
    }
    else {
        m_input.command_type = TCommandType::cm_Unknown;
    }
    // update data, previously stored in params 0, 1, 2
    m_input.data_text = token;
    Input.getTokens();
    if( Input.peek() != "none" ) {
        Input >> m_input.data_value_1;
    }
    Input.getTokens();
    if( Input.peek() != "none" ) {
        Input >> m_input.data_value_2;
    }
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
putvalues_event::run_() {

    WriteLog(
        "Type: PutValues - ["
        + m_input.data_text + "] ["
        + to_string( m_input.data_value_1, 2 ) + "] ["
        + to_string( m_input.data_value_2, 2 ) + "]" );

    if( m_activator == nullptr ) { return; }
    // zamiana, bo fizyka ma inaczej niż sceneria
    // NOTE: y & z swap, negative x
    TLocation const loc {
        -m_input.location.x,
         m_input.location.z,
         m_input.location.y };

    if( m_activator->Mechanik ) {
        // przekazanie rozkazu do AI
        WriteLog( " Vehicle: [" + m_activator->Mechanik->Vehicle()->name() + "]" );
        m_activator->Mechanik->PutCommand(
            m_input.data_text,
            m_input.data_value_1,
            m_input.data_value_2,
            loc );
    }
    else if( ( m_activator->ctOwner )
          && ( is_command_for_owner( m_input ) ) ) {
        // send the command to consist owner,
        // we're acting on presumption there's hardly ever need to issue command to unmanned vehicle
        // and the intended recipient moved between vehicles after the event was queued
        WriteLog( " Vehicle: [" + m_activator->ctOwner->Vehicle()->name() + "]" );
        m_activator->ctOwner->PutCommand(
            m_input.data_text,
            m_input.data_value_1,
            m_input.data_value_2,
            loc );
    }
    else {
        // przekazanie do pojazdu
        WriteLog( " Vehicle: [" + m_activator->name() +"]" );
        m_activator->MoverParameters->PutCommand(
            m_input.data_text,
            m_input.data_value_1,
            m_input.data_value_2,
            loc );
    }
}

// export_as_text() subclass details
void
putvalues_event::export_as_text_( std::ostream &Output ) const {

    Output
        // location
        << m_input.location.x << ' '
        << m_input.location.y << ' '
        << m_input.location.z << ' '
        // command
        << m_input.data_text << ' '
        << m_input.data_value_1 << ' '
        << m_input.data_value_2 << ' ';
}

//determines whether provided input should be passed to consist owner
bool
putvalues_event::is_command_for_owner( input_data const &Input ) const {

    if( starts_with( Input.data_text, "Load=" ) )   { return false; }
    if( starts_with( Input.data_text, "UnLoad=" ) ) { return false; }
    // TBD, TODO: add other exceptions

    return true;
}

// input data access
std::string
putvalues_event::input_text() const {

    return m_input.data_text;
}

TCommandType
putvalues_event::input_command() const {

    return m_input.command_type; // komenda zakodowana binarnie
}

double
putvalues_event::input_value( int Index ) const {

    Index &= 1; // tylko 1 albo 2 jest prawidłowy
    return ( Index == 1 ? m_input.data_value_1 : m_input.data_value_2 );
}

glm::dvec3
putvalues_event::input_location() const {

    return m_input.location;
}



// prepares event for use
void
copyvalues_event::init() {
    // skopiowanie komórki do innej
    init_targets( simulation::Memory, "memory cell" );
    // source cell
    std::get<scene::basic_node *>( m_input.data_source ) = simulation::Memory.find( std::get<std::string>( m_input.data_source ) );
    if( std::get<scene::basic_node *>( m_input.data_source ) == nullptr ) {
        m_ignored = true; // deaktywacja
        ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find memory cell \"" + std::get<std::string>( m_input.data_source ) + "\"" );
    }
}

// event type string
std::string
copyvalues_event::type() const {

    return "copyvalues";
}

// deserialize() subclass details
void
copyvalues_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    m_input.flags = ( flags::text | flags::value1 | flags::value2 ); // normalnie trzy

    std::string token;
    int paramidx { 0 };

    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( false == is_keyword( token ) ) ) {

        Input >> token;
        switch( ++paramidx ) {
            case 1: { // nazwa drugiej komórki (źródłowej) // previously stored in param 9
                std::get<std::string>( m_input.data_source ) = token;
                break;
            }
            case 2: { // maska wartości
                m_input.flags = stol_def( token, ( flags::text | flags::value1 | flags::value2 ) );
                break;
            }
            default: {
                break;
            }
        }
    }
}

// run() subclass details
// TODO: update and copy values run_ methods are largely identical, refactor to a single helper
void
copyvalues_event::run_() {
    // skopiowanie wartości z innej komórki
    auto const *datasource { static_cast<TMemCell *>( std::get<scene::basic_node *>( m_input.data_source ) ) };
    m_input.data_text = datasource->Text();
    m_input.data_value_1 = datasource->Value1();
    m_input.data_value_2 = datasource->Value2();

    WriteLog( "Type: CopyValues - ["
        + ( ( m_input.flags & flags::text ) ? m_input.data_text : "X" ) + "] ["
        + ( ( m_input.flags & flags::value1 ) ? to_string( m_input.data_value_1, 2 ) : "X" ) + "] ["
        + ( ( m_input.flags & flags::value2 ) ? to_string( m_input.data_value_2, 2 ) : "X" ) + "]" );
    // TODO: dump status of target cells after the operation
    for( auto &target : m_targets ) {
        auto *targetcell { static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) ) };
        if( targetcell == nullptr ) { continue; }
        targetcell->UpdateValues(
            m_input.data_text,
            m_input.data_value_1,
            m_input.data_value_2,
            m_input.flags );
        WriteLog( " Memcell: " + targetcell->name() + " - " + targetcell->Values() );
//        targetcell->LogValues();
        if( targetcell->Track == nullptr ) { continue; }
        // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
        auto const location { targetcell->location() };
        for( auto vehicle : targetcell->Track->Dynamics ) {
            if( vehicle->Mechanik ) {
                WriteLog( " Vehicle: [" + vehicle->name() + "]" );
                targetcell->PutCommand(
                    vehicle->Mechanik,
                    &location );
            }
        }
    }
}

// export_as_text() subclass details
void
copyvalues_event::export_as_text_( std::ostream &Output ) const {

    auto const *datasource { static_cast<TMemCell *>( std::get<scene::basic_node *>( m_input.data_source ) ) };
    Output
        << ( datasource != nullptr ?
                datasource->name() :
                std::get<std::string>( m_input.data_source ) )
        << ' ' << ( m_input.flags & ( flags::text | flags::value1 | flags::value2 ) ) << ' ';
}



// prepares event for use
void
whois_event::init() {

    init_targets( simulation::Memory, "memory cell" );
}

// event type string
std::string
whois_event::type() const {

    return "whois";
}

// deserialize() subclass details
void
whois_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    m_input.flags = ( flags::text | flags::value1 | flags::value2 ); // normalnie trzy

    std::string token;
    int paramidx { 0 };

    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( false == is_keyword( token ) ) ) {

        Input >> token;
        switch( ++paramidx ) {
            case 1: { // maska wartości
                m_input.flags = stol_def( token, ( flags::text | flags::value1 | flags::value2 ) );
                break;
            }
            default: {
                break;
            }
        }
    }
}

// run() subclass details
void 
whois_event::run_() {

    for( auto &target : m_targets ) {
        auto *targetcell { static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) ) };
        if( targetcell == nullptr ) { continue; }
        // event effect code
        // +40: next station name, unused, stop at next station (duplicate of +0 2nd numeric value)
        // +32: vehicle name
        // +24: vehicle type, consist brake level, obstacle distance
        // +16: load type, load amount, max load amount
        // +8: destination, direction, engine power
        // +0: train name, station count, stop on next station
        if( m_input.flags & flags::whois_name ) {
            // +32 or +40
            // next station name
            if( m_input.flags & flags::mode_alt ) {
                auto const *owner { (
                    ( ( m_activator->Mechanik != nullptr ) && ( m_activator->Mechanik->primary() ) ) ?
                        m_activator->Mechanik :
                        m_activator->ctOwner ) };
                auto const nextstop { (
                    owner != nullptr ?
                        owner->TrainTimetable().NextStop() :
                        "none" ) };
                auto const isstop { (
                    ( ( owner != nullptr ) && ( owner->IsStop() ) ) ?
                        1 :
                        0 ) }; // 1, gdy ma tu zatrzymanie

                targetcell->UpdateValues(
                    nextstop, // next station name
                    0, // unused
                    isstop, // stop at next station or passthrough
                    m_input.flags & ( flags::text | flags::value1 | flags::value2 ) );

                WriteLog(
                    "Type: WhoIs (" + to_string( m_input.flags ) + ") - "
                    + "[next station: " + nextstop + "], "
                    + "[X], "
                    + "[stop at next station: " + ( isstop != 0 ? "yes" : "no" ) + "]" );
            }
            // vehicle name
            else {
                targetcell->UpdateValues(
                    m_activator->asName, // vehicle name
                    0, // unused
                    0, // unused
                    m_input.flags & ( flags::text | flags::value1 | flags::value2 ) );

                WriteLog(
                    "Type: WhoIs (" + to_string( m_input.flags ) + ") - "
                    + "[name: " + m_activator->asName + "], "
                    + "[X], "
                    + "[X]" );
            }
        }
        else if( m_input.flags & flags::whois_load ) {
            // +16 or +24
            // jeśli pytanie o ładunek
            if( m_input.flags & flags::mode_alt ) {
                // jeśli typ pojazdu
                // TODO: define and recognize individual request types
                auto const owner { (
                    ( ( m_activator->Mechanik != nullptr ) && ( m_activator->Mechanik->primary() ) ) ?
                        m_activator->Mechanik :
                        m_activator->ctOwner ) };
                auto const consistbrakelevel { (
                    owner != nullptr ?
                        owner->fReady :
                        -1.0 ) };
                auto const collisiondistance { (
                    owner != nullptr ?
                        owner->TrackObstacle() :
                        -1.0 ) };

                targetcell->UpdateValues(
                    m_activator->MoverParameters->TypeName, // typ pojazdu
                    consistbrakelevel,
                    collisiondistance,
                    m_input.flags & ( flags::text | flags::value1 | flags::value2 ) );

                WriteLog(
                    "Type: WhoIs (" + to_string( m_input.flags ) + ") - "
                    + "[type: " + m_activator->MoverParameters->TypeName + "], "
                    + "[consist brake level: " + to_string( consistbrakelevel, 2 ) + "], "
                    + "[obstacle distance: " + to_string( collisiondistance, 2 ) + " m]" );
            }
            else {
                // jeśli parametry ładunku
                targetcell->UpdateValues(
                    m_activator->MoverParameters->LoadType.name, // nazwa ładunku
                    m_activator->MoverParameters->LoadAmount, // aktualna ilość
                    m_activator->MoverParameters->MaxLoad, // maksymalna ilość
                    m_input.flags & ( flags::text | flags::value1 | flags::value2 ) );

                WriteLog(
                    "Type: WhoIs (" + to_string( m_input.flags ) + ") - "
                    + "[load type: " + m_activator->MoverParameters->LoadType.name + "], "
                    + "[current load: " + to_string( m_activator->MoverParameters->LoadAmount, 2 ) + "], "
                    + "[max load: " + to_string( m_activator->MoverParameters->MaxLoad, 2 ) + "]" );
            }
        }
        // +8
        else if( m_input.flags & flags::mode_alt ) { // jeśli miejsce docelowe pojazdu
            targetcell->UpdateValues(
                m_activator->asDestination, // adres docelowy
                m_activator->DirectionGet(), // kierunek pojazdu względem czoła składu (1=zgodny,-1=przeciwny)
                m_activator->MoverParameters->Power, // moc pojazdu silnikowego: 0 dla wagonu
                m_input.flags & ( flags::text | flags::value1 | flags::value2 ) );

            WriteLog(
                "Type: WhoIs (" + to_string( m_input.flags ) + ") - "
                + "[destination: " + m_activator->asDestination + "], "
                + "[direction: " + to_string( m_activator->DirectionGet() ) + "], "
                + "[engine power: " + to_string( m_activator->MoverParameters->Power, 2 ) + "]" );
        }
        // +0
        else if( m_activator->Mechanik ) {
            if( m_activator->Mechanik->primary() ) { // tylko jeśli ktoś tam siedzi - nie powinno dotyczyć pasażera!
                targetcell->UpdateValues(
                    m_activator->Mechanik->TrainName(),
                    m_activator->Mechanik->StationCount() - m_activator->Mechanik->StationIndex(), // ile przystanków do końca
                    m_activator->Mechanik->IsStop() ?
                        1 :
                        0, // 1, gdy ma tu zatrzymanie
                    m_input.flags & ( flags::text | flags::value1 | flags::value2 ) );
                WriteLog(
                    "Type: WhoIs (" + to_string( m_input.flags ) + ") - "
                    + "[train: " + m_activator->Mechanik->TrainName() + "], "
                    + "[stations left: " + to_string( m_activator->Mechanik->StationCount() - m_activator->Mechanik->StationIndex() ) + "], "
                    + "[stop at next station: " + ( m_activator->Mechanik->IsStop() ? "yes" : "no") + "]" );
            }
        }
    }
}

// export_as_text() subclass details
void
whois_event::export_as_text_( std::ostream &Output ) const {

    Output << ( m_input.flags & ( flags::text | flags::value1 | flags::value2 ) ) << ' ';
}



// prepares event for use
void
logvalues_event::init() {

    init_targets( simulation::Memory, "memory cell" );
}

// event type string
std::string
logvalues_event::type() const {

    return "logvalues";
}

// deserialize() subclass details
void
logvalues_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // nothing to do here, just preload next token
    Input.getTokens();
}

// run() subclass details
void
logvalues_event::run_() {
    // zapisanie zawartości komórki pamięci do logu
    if( m_targets.empty() ) {
        // lista wszystkich
        simulation::Memory.log_all();
    }
    else {
        // jeśli była podana nazwa komórki
        for( auto &target : m_targets ) {
            auto *targetcell { static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) ) };
            if( targetcell == nullptr ) { continue; }
            targetcell->LogValues();
        }
    }
}

// export_as_text() subclass details
void
logvalues_event::export_as_text_( std::ostream &Output ) const {

}



// prepares event for use
void
multi_event::init() {

    auto const conditiontchecksmemcell { ( m_conditions.flags & ( flags::text | flags::value1 | flags::value2 ) ) != 0 };
    // not all multi-events have memory cell checks, for the ones which don't we can keep quiet about it
    init_targets( simulation::Memory, "memory cell", conditiontchecksmemcell );
    if( m_ignored ) {
        // legacy compatibility behaviour, instead of disabling the event we disable the memory cell comparison test
        m_conditions.flags &= ~( flags::text | flags::value1 | flags::value2 );
        m_ignored = false;
    }
    // conditional data
    m_conditions.bind( &m_targets );
    m_conditions.init();
    // child events bindings
    for( auto &childevent : m_children ) {
        std::get<basic_event *>( childevent ) = simulation::Events.FindEvent( std::get<std::string>( childevent ) );
        if( std::get<basic_event *>( childevent ) == nullptr ) {
            ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find event \"" + std::get<std::string>( childevent ) + "\"" );
        }
    }
}

std::vector<std::string> multi_event::dump_children_names() const {
    std::vector<std::string> result;
    for (auto const &childevent : m_children) {
        result.push_back(std::get<std::string>(childevent));
    }
    return result;
}

// event type string
std::string
multi_event::type() const {

    return "multiple";
}

// deserialize() subclass details
void
multi_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    m_conditions.has_else = false;

    std::string token;
    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( false == is_keyword( token ) ) ) {

        if( token == "condition" ) {
            m_conditions.deserialize( Input );
            // NOTE: condition block comes last so we can bail out afterwards
            break;
        }
        else if( token == "else" ) {
            // zmiana flagi dla słowa "else"
            m_conditions.has_else = !m_conditions.has_else;
        }
        else {
            // potentially valid event name
            if( starts_with( token, "none_" ) ) {
                // eventy rozpoczynające się od "none_" są ignorowane
                WriteLog( "Multi-event \"" + m_name + "\" ignored link to event \"" + token + "\"" );
            }
            else {
                m_children.emplace_back( token, nullptr, ( m_conditions.has_else == false ) );
            }
        }
    }
}

// run() subclass details
void
multi_event::run_() {

    auto const conditiontest { m_conditions.test() };
    if( conditiontest
     || m_conditions.has_else ) {
           // warunek spelniony albo było użyte else
        WriteLog( "Type: Multi-event" );
        for( auto &childwrapper : m_children ) {
            auto *childevent { std::get<basic_event*>( childwrapper ) };
            if( childevent == nullptr ) { continue; }
            if( std::get<bool>( childwrapper ) != conditiontest ) { continue; }

            if( childevent != this ) {
                // normalnie dodać
                simulation::Events.AddToQuery( childevent, m_activator );
            }
            else {
                // jeśli ma być rekurencja to musi mieć sensowny okres powtarzania
                if( m_delay >= 5.0 ) {
                    simulation::Events.AddToQuery( this, m_activator );
                }
            }
        }
        if( Global.iMultiplayer ) {
            // dajemy znać do serwera o wykonaniu
            if( false == m_conditions.has_else ) {
                // jednoznaczne tylko, gdy nie było else
                if( m_activator ) {
                    multiplayer::WyslijEvent( m_name, m_activator->name() );
                }
                else {
                    multiplayer::WyslijEvent( m_name, "" );
                }
            }
        }
    }
}

// export_as_text() subclass details
void
multi_event::export_as_text_( std::ostream &Output ) const {

    for( auto const &childevent : m_children ) {
        if( true == std::get<bool>( childevent ) ) {
            auto *childeventdata { std::get<basic_event *>( childevent ) };
            Output
                << ( childeventdata != nullptr ?
                    childeventdata->m_name :
                    std::get<std::string>( childevent ) )
                << ' ';
        }
    }
    // optional 'else' block
    if( true == m_conditions.has_else ) {
        Output << "else ";
        for( auto const &childevent : m_children ) {
            if( false == std::get<bool>( childevent ) ) {
                auto *childeventdata{ std::get<basic_event *>( childevent ) };
                Output
                    << ( childeventdata != nullptr ?
                        childeventdata->m_name :
                        std::get<std::string>( childevent ) )
                    << ' ';
            }
        }
    }

    m_conditions.export_as_text( Output );
}



// prepares event for use
void
sound_event::init() {
    // odtworzenie dźwięku
    for( auto &target : m_sounds ) {
        std::get<sound_source *>( target ) = simulation::Sounds.find( std::get<std::string>( target ) );
        if( std::get<sound_source *>( target ) == nullptr ) {
            m_ignored = true; // deaktywacja
            ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find static sound \"" + std::get<std::string>( target ) + "\"" );
        }
    }
}

// event type string
std::string
sound_event::type() const {

    return "sound";
}

// deserialize() subclass details
void
sound_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens();
    // playback mode, previously held in param 0 // 0: wylaczyc, 1: wlaczyc; -1: wlaczyc zapetlone
    Input >> m_soundmode;
    Input.getTokens();
    if( false == is_keyword( Input.peek() ) ) {
        // optional parameter, radio channel to receive/broadcast the sound, previously held in param 1
        Input >> m_soundradiochannel;
        Input.getTokens(); // preload next token
    }
}

// run() subclass details
void
sound_event::run_() {

    WriteLog(
        "Type: Sound - [" + std::string( m_soundmode == 1 ? "play" : m_soundmode == -1 ? "loop" : "stop" ) + "]"
        + ( m_soundradiochannel > 0 ? " [channel " + to_string( m_soundradiochannel ) + "]" : "" ) );
    for( auto &target : m_sounds ) {
        auto *targetsound = std::get<sound_source *>( target );
        if( targetsound == nullptr ) { continue; }
        // event effect code
        switch( m_soundmode ) {
            // trzy możliwe przypadki:
            case 0: {
                targetsound->stop();
                break;
            }
            case 1: {
                if( m_soundradiochannel > 0 ) {
                    simulation::radio_message(
                        targetsound,
                        m_soundradiochannel );
                }
                else {
                    targetsound->play( sound_flags::exclusive | sound_flags::event );
                }
                break;
            }
            case -1: {
                targetsound->play( sound_flags::exclusive | sound_flags::looping | sound_flags::event );
                break;
            }
            default: {
                break;
            }
        }
    }
}

// export_as_text() subclass details
void
sound_event::export_as_text_( std::ostream &Output ) const {

    // playback mode
    Output << m_soundmode << ' ';
    // optional radio channel
    if( m_soundradiochannel > 0 ) {
        Output << m_soundradiochannel << ' ';
    }
}

void
sound_event::deserialize_targets( std::string const &Input ) {
    // sound objects don't inherit from scene node, so we can't use the same container
    // TODO: fix this, having sounds as scene nodes (or a node wrapper for one) might have benefits elsewhere
    cParser targetparser{ Input };
    std::string target;
    while( false == ( target = targetparser.getToken<std::string>( true, "|," ) ).empty() ) {
        // actual bindings to targets of proper type are created during scenario initialization
        if( target != "none" ) {
            m_sounds.emplace_back( target, nullptr );
        }
    }
}



TMemCell const *
texture_event::input_data::data_cell() const {

    return static_cast<TMemCell const *>( std::get<scene::basic_node *>( data_source ) );
}
TMemCell *
texture_event::input_data::data_cell() {

    return static_cast<TMemCell *>( std::get<scene::basic_node *>( data_source ) );
}



// prepares event for use
void
texture_event::init() {
    // target models
    init_targets( simulation::Instances, "model instance" );
    // optional input data memory cell
    TMemCell *inputcell { nullptr };
    auto const inputcellname { std::get<std::string>( m_input.data_source ) };
    if( inputcellname != "none" ) {
        inputcell = simulation::Memory.find( inputcellname );
        if( inputcell == nullptr ) {
            ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find memory cell \"" + inputcellname + "\"" );
        }
    }
    std::get<scene::basic_node *>( m_input.data_source ) = inputcell;
}

// event type string
std::string
texture_event::type() const {

    return "texture";
}

// deserialize() subclass details
void
texture_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens( 3 );
    Input
        >> m_skinindex
        >> m_skin
        >> std::get<std::string>( m_input.data_source );

    // validate input
    if( m_skinindex < 0 ) {
        // intercept potential error, index specified using .t3d format convention
        m_skinindex *= -1;
    }
    m_skinindex = clamp( m_skinindex, 1, 4 ); // TODO: define-based upper bound in case of future extension

    Input.getTokens(); // preload next token
}

// run() subclass details
void
texture_event::run_() {

    material_handle material { null_handle };

    if( m_input.data_cell() == nullptr ) {
        // straightforward variant without parameters, we can pass expression directly
        material = GfxRenderer->Fetch_Material( m_skin );
    }
    else {
        // variant with memory cell: pass the cell content as parameters and generate filename
        cParser expression(
            m_skin, cParser::buffer_TEXT,
            "", true,
            {   m_input.data_cell()->Text(),
                to_string( m_input.data_cell()->Value1(), 0 ), // memory cell values are passed as ints
                to_string( m_input.data_cell()->Value2(), 0 ) } );
        material = GfxRenderer->Fetch_Material( expression.getToken<std::string>( false, "\n\r" ) );
    }
    // assign received material to target models
    for( auto &target : m_targets ) {
        auto *targetmodel = static_cast<TAnimModel *>( std::get<scene::basic_node *>( target ) );
        if( targetmodel == nullptr ) { continue; }
        // event effect code
        targetmodel->SkinSet( m_skinindex, material );
    }
}

// export_as_text() subclass details
void
texture_event::export_as_text_( std::ostream &Output ) const {

    // playback mode
    Output
        << m_skinindex << ' '
        << m_skin << ' '
        << std::get<std::string>( m_input.data_source ) << ' ';
}



// destructor
animation_event::~animation_event() {

    if( m_animationtype == 4 ) {
        // jeśli z pliku VMD
        SafeDeleteArray( m_animationfiledata ); // zwolnić obszar
    }
}

// prepares event for use
void
animation_event::init() {
    // animacja modelu
    init_targets( simulation::Instances, "model instance" );
    // custom target initialization code
    if( m_animationtype == 4 ) {
        // vmd animations target whole model
        return;
    }
    // locate and set up animated submodels
    m_animationcontainers.clear();
    for( auto &target : m_targets ) {
        auto *targetmodel { static_cast<TAnimModel *>( std::get<scene::basic_node *>( target ) ) };
        if( targetmodel == nullptr ) { continue; }
		auto targetcontainer{ targetmodel->GetContainer( m_animationsubmodel ) };
		if( !targetcontainer ) {
            m_ignored = true;
            ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find submodel " + m_animationsubmodel + " in model instance \"" + targetmodel->name() + "\"" );
            break;
        }
        targetcontainer->WillBeAnimated(); // oflagowanie animacji
        if( targetcontainer->Event() == nullptr ) {
            // nie szukać, gdy znaleziony
            targetcontainer->EventAssign( simulation::Events.FindEvent( targetmodel->name() + '.' + m_animationsubmodel + ":done" ) );
        }
        m_animationcontainers.emplace_back( targetcontainer );
    }
}

// event type string
std::string
animation_event::type() const {

    return "animation";
}

// deserialize() subclass details
void
animation_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    std::string token;

    Input.getTokens();
    Input >> token;
    if( token == "rotate" ) { // obrót względem osi
        Input.getTokens();
        // animation submodel, previously held in param 9
        Input >> m_animationsubmodel;
        // animation type, previously held in param 0
        m_animationtype = 1;
        Input.getTokens( 4 );
        // animation params, previously held in param 1, 2, 3, 4
        Input
            >> m_animationparams[ 0 ]
            >> m_animationparams[ 1 ]
            >> m_animationparams[ 2 ]
            >> m_animationparams[ 3 ];
    }
    else if( token == "translate" ) { // przesuw o wektor
        Input.getTokens();
        // animation submodel, previously held in param 9
        Input >> m_animationsubmodel;
        // animation type, previously held in param 0
        m_animationtype = 2;
        Input.getTokens( 4 );
        // animation params, previously held in param 1, 2, 3, 4
        Input
            >> m_animationparams[ 0 ]
            >> m_animationparams[ 1 ]
            >> m_animationparams[ 2 ]
            >> m_animationparams[ 3 ];
    }
    else if( token == "digital" ) { // licznik cyfrowy
        Input.getTokens();
        // animation submodel, previously held in param 9
        Input >> m_animationsubmodel;
        // animation type, previously held in param 0
        m_animationtype = 8;
        Input.getTokens( 4 );
        // animation params, previously held in param 1, 2, 3, 4
        Input
            >> m_animationparams[ 0 ]
            >> m_animationparams[ 1 ]
            >> m_animationparams[ 2 ]
            >> m_animationparams[ 3 ];
    }
    else if( ends_with( token, ".vmd" ) ) // na razie tu, może będzie inaczej
    { // animacja z pliku VMD
        {
            m_animationfilename = token;
            std::ifstream file( szModelPath + m_animationfilename, std::ios::binary | std::ios::ate ); file.unsetf( std::ios::skipws );
            auto size = file.tellg();   // ios::ate already positioned us at the end of the file
            file.seekg( 0, std::ios::beg ); // rewind the caret afterwards
            // animation size, previously held in param 7
            m_animationfilesize = size;
            // animation data, previously held in param 8
            m_animationfiledata = new char[ size ];
            file.read( m_animationfiledata, size ); // wczytanie pliku
        }
        Input.getTokens();
        // animation submodel, previously held in param 9
        Input >> m_animationsubmodel;
        // animation type, previously held in param 0
        m_animationtype = 4;
        Input.getTokens( 4 );
        // animation params, previously held in param 1, 2, 3, 4
        Input
            >> m_animationparams[ 0 ]
            >> m_animationparams[ 1 ]
            >> m_animationparams[ 2 ]
            >> m_animationparams[ 3 ];
    }
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
animation_event::run_() {

    WriteLog( "Type: Animation" );
	// animation modes target specific submodels
	m_animationcontainers.remove_if([this](std::weak_ptr<TAnimContainer> ptr)
	{
		auto targetcontainer = ptr.lock();
		if (!targetcontainer)
			return true;

		switch( m_animationtype ) {
		    case 1: { // rotate
			    targetcontainer->SetRotateAnim(
				    glm::make_vec3( m_animationparams.data() ),
				    m_animationparams[ 3 ] );
				break;
		    }
		    case 2: { // translate
			    targetcontainer->SetTranslateAnim(
				    glm::make_vec3( m_animationparams.data() ),
				    m_animationparams[ 3 ] );
				break;
		    }
			// TODO: implement digital mode
		    default: {
			    break;
		    }
		}
		return false;
	});
}

// export_as_text() subclass details
void
animation_event::export_as_text_( std::ostream &Output ) const {

    // animation type
    Output << (
        m_animationtype == 1 ? "rotate" :
        m_animationtype == 2 ? "translate" :
        m_animationtype == 4 ? m_animationfilename :
        m_animationtype == 8 ? "digital" :
        "none" )
        << ' ';
    // submodel
    Output << m_animationsubmodel << ' ';
    // animation parameters
    Output
        << m_animationparams[ 0 ] << ' '
        << m_animationparams[ 1 ] << ' '
        << m_animationparams[ 2 ] << ' '
        << m_animationparams[ 3 ] << ' ';
}



// prepares event for use
void
lights_event::init() {
    // zmiana świeteł modelu
    init_targets( simulation::Instances, "model instance" );
}

// event type string
std::string
lights_event::type() const {

    return "lights";
}

// deserialize() subclass details
void
lights_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    // TBD, TODO: remove light count limit?
    auto const lightcountlimit { 8 };
    m_lights.resize( lightcountlimit );
    int lightidx { 0 };

    std::string token;
    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( false == is_keyword( token ) ) ) {

        if( lightidx < lightcountlimit ) {
            Input >> m_lights[ lightidx++ ];
        }
        else {
            ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") with more than " + to_string( lightcountlimit ) + " parameters" );
        }
    }
    while( lightidx < lightcountlimit ) {
        // HACK: mark unspecified lights with magic value
        m_lights[ lightidx++ ] = std::numeric_limits<float>::quiet_NaN();
    }
}

// run() subclass details
void
lights_event::run_() {

    for( auto &target : m_targets ) {
        auto *targetmodel = static_cast<TAnimModel *>( std::get<scene::basic_node *>( target ) );
        if( targetmodel == nullptr ) { continue; }
        // event effect code
        for( auto lightidx = 0; lightidx < iMaxNumLights; ++lightidx ) {
            if( std::isnan( m_lights[ lightidx ] ) ) {
                // processed all supplied values, bail out
                break;
            }
            if( m_lights[ lightidx ] != -1.f ) {
                // -1 zostawia bez zmiany
                targetmodel->LightSet(
                    lightidx,
                    m_lights[ lightidx ] );
            }
        }
    }
}

// export_as_text() subclass details
void
lights_event::export_as_text_( std::ostream &Output ) const {

    auto lightidx{ 0 };
    while( ( lightidx < iMaxNumLights )
        && ( false == std::isnan( m_lights[ lightidx ] ) ) ) {
        Output << m_lights[ lightidx ] << ' ';
        ++lightidx;
    }
}



// prepares event for use
void
switch_event::init() {
    // przełożenie zwrotnicy albo zmiana stanu obrotnicy
    init_targets( simulation::Paths, "track" );
    // custom target initialization code
    for( auto &target : m_targets ) {
        auto *targettrack = static_cast<TTrack *>( std::get<scene::basic_node *>( target ) );
        if( targettrack == nullptr ) { continue; }
        // dowiązanie toru
        if( targettrack->iAction == 0 ) {
            // jeśli nie jest zwrotnicą ani obrotnicą to będzie się zmieniał stan uszkodzenia
            targettrack->iAction |= 0x100;
        }
        if( ( m_switchstate == 0 )
            && ( m_switchmovedelay >= 0.0 ) ) {
            // jeśli przełącza do stanu 0 & jeśli jest zdefiniowany dodatkowy ruch iglic
            // przesłanie parametrów
            targettrack->Switch(
                m_switchstate,
                m_switchmoverate,
                m_switchmovedelay );
        }
    }
}

// event type string
std::string
switch_event::type() const {

    return "switch";
}

// deserialize() subclass details
void
switch_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens();
    // switch state, previously held in param 0
    Input >> m_switchstate;
    Input.getTokens();
    if( false == is_keyword( Input.peek() ) ) {
        // prędkość liniowa ruchu iglic
        // previously held in param 1
        Input >> m_switchmoverate;
        Input.getTokens();
    }
    if( false == is_keyword( Input.peek() ) ) {
        // dodatkowy ruch drugiej iglicy (zamknięcie nastawnicze)
        // previously held in param 2
        Input >> m_switchmovedelay;
        Input.getTokens();
    }
}

// run() subclass details
void
switch_event::run_() {

    for( auto &target : m_targets ) {
        auto *targettrack { static_cast<TTrack *>( std::get<scene::basic_node *>( target ) ) };
        if( targettrack == nullptr ) { continue; }
        // event effect code
        targettrack->Switch(
            m_switchstate,
            m_switchmoverate,
            m_switchmovedelay );
    }
    if( Global.iMultiplayer ) {
        // dajemy znać do serwera o przełożeniu
        multiplayer::WyslijEvent( m_name, "" ); // wysłanie nazwy eventu przełączajacego
    }
    // Ra: bardziej by się przydała nazwa toru, ale nie ma do niej stąd dostępu
}

// export_as_text() subclass details
void
switch_event::export_as_text_( std::ostream &Output ) const {

    Output << m_switchstate << ' ';
    if( ( m_switchmoverate  < 0.f )
     || ( m_switchmovedelay < 0.f ) ) {
        Output << m_switchmoverate << ' ';
    }
    if( m_switchmovedelay < 0.f ) {
        Output << m_switchmovedelay << ' ';
    }
}



// prepares event for use
void
track_event::init() {
    // ustawienie prędkości na torze
    init_targets( simulation::Paths, "track" );
    // custom target initialization code
    for( auto &target : m_targets ) {
        auto *targettrack = static_cast<TTrack *>( std::get<scene::basic_node *>( target ) );
        if( targettrack == nullptr ) { continue; }
        // flaga zmiany prędkości toru jest istotna dla skanowania
        targettrack->iAction |= 0x200;
    }
}

// event type string
std::string
track_event::type() const {

    return "trackvel";
}

// deserialize() subclass details
void
track_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens();
    Input >> m_velocity;
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
track_event::run_() {

    WriteLog( "Type: TrackVel - [" + to_string( m_velocity, 2 ) + "]" );
    for( auto &target : m_targets ) {
        auto *targettrack = static_cast<TTrack *>( std::get<scene::basic_node *>( target ) );
        if( targettrack == nullptr ) { continue; }
        // event effect code
        targettrack->VelocitySet( m_velocity );
        if( DebugModeFlag ) {
            WriteLog( "actual track velocity for " + targettrack->name() + " [" + to_string( targettrack->VelocityGet(), 2 ) + "]" );
        }
    }
}

// export_as_text() subclass details
void
track_event::export_as_text_( std::ostream &Output ) const {

    Output << m_velocity << ' ';
}



// prepares event for use
void
voltage_event::init() {
    // zmiana napięcia w zasilaczu (TractionPowerSource)
    init_targets( simulation::Powergrid, "power source" );
}

// event type string
std::string
voltage_event::type() const {

    return "voltage";
}

// deserialize() subclass details
void
voltage_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // zmiana napięcia w zasilaczu (TractionPowerSource)
    Input.getTokens();
    Input >> m_voltage;
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
voltage_event::run_() {
    // zmiana napięcia w zasilaczu (TractionPowerSource)
    WriteLog( "Type: Voltage [" + to_string( m_voltage, 2 ) + "]" );
    for( auto &target : m_targets ) {
        auto *targetpowersource = static_cast<TTractionPowerSource *>( std::get<scene::basic_node *>( target ) );
        if( targetpowersource == nullptr ) { continue; }
        // na razie takie chamskie ustawienie napięcia zasilania
        targetpowersource->VoltageSet( m_voltage );
    }
}

// export_as_text() subclass details
void
voltage_event::export_as_text_( std::ostream &Output ) const {

    Output << m_voltage << ' ';
}



// prepares event for use
void
visible_event::init() {
    // ukrycie albo przywrócenie obiektu
    for( auto &target : m_targets ) {
        auto &targetnode{ std::get<scene::basic_node *>( target ) };
        auto &targetname{ std::get<std::string>( target ) };
        // najpierw model
        targetnode = simulation::Instances.find( targetname );
        if( targetnode == nullptr ) {
            // albo tory?
            targetnode = simulation::Paths.find( targetname );
        }
        if( targetnode == nullptr ) {
            // może druty?
            targetnode = simulation::Traction.find( targetname );
        }
        if( targetnode == nullptr ) {
            m_ignored = true; // deaktywacja
            ErrorLog( "Bad event: \"" + m_name + "\" (type: " + type() + ") can't find item \"" + std::get<std::string>( target ) + "\"" );
        }
    }
}

// event type string
std::string
visible_event::type() const {

    return "visible";
}

// deserialize() subclass details
void
visible_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // zmiana wyświetlania obiektu
    Input.getTokens();
    Input >> m_visible;
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
visible_event::run_() {

    for( auto &target : m_targets ) {
        auto *targetnode = std::get<scene::basic_node *>( target );
        if( targetnode == nullptr ) { continue; }
        // event effect code
        targetnode->visible( m_visible );
    }
}

// export_as_text() subclass details
void
visible_event::export_as_text_( std::ostream &Output ) const {

    Output << ( m_visible ? 1 : 0 ) << ' ';
}



// prepares event for use
void
friction_event::init() {
    // nothing to do here
}

// event type string
std::string
friction_event::type() const {

    return "friction";
}

// deserialize() subclass details
void
friction_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // zmiana przyczepnosci na scenerii
    Input.getTokens();
    Input >> m_friction;
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
friction_event::run_() {
    // zmiana tarcia na scenerii
    WriteLog( "Type: Friction" );
    Global.fFriction = ( m_friction );
}

// export_as_text() subclass details
void
friction_event::export_as_text_( std::ostream &Output ) const {

    Output << m_friction << ' ';
}

#ifdef WITH_LUA
lua_event::lua_event(lua::eventhandler_t func) {
    lua_func = func;
}

// prepares event for use
void
lua_event::init() {
    // nothing to do here
}

// event type string
std::string
lua_event::type() const {
    return "lua";
}

// deserialize() subclass details
void
lua_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // preload next token
    Input.getTokens();
}

// run() subclass details
void
lua_event::run_() {
	try {
	    if (lua_func)
	        lua_func(this, m_activator);
	} catch (...) {
		ErrorLog(simulation::Lua.get_error());
	}
}

// export_as_text() subclass details
void
lua_event::export_as_text_( std::ostream &Output ) const {
    // nothing to do here
}

bool lua_event::is_instant() const {
	return m_delay == 0.0 && m_delayrandom == 0.0;
}
#endif

// prepares event for use
void
message_event::init() {
    // nothing to do here
}

// event type string
std::string
message_event::type() const {

    return "message";
}

// deserialize() subclass details
void
message_event::deserialize_( cParser &Input, scene::scratch_data &Scratchpad ) {
    // wyświetlenie komunikatu
    std::string token;
    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( false == is_keyword( token ) ) ) {
        m_message += ( m_message.empty() ? "" : " " ) + token;
    }
}

// run() subclass details
void
message_event::run_() {
    // TODO: implement
}

// export_as_text() subclass details
void
message_event::export_as_text_( std::ostream &Output ) const {

    Output << '\"' << m_message << '\"' << ' ';
}

//---------------------------------------------------------------------------

basic_event *
make_event( cParser &Input, scene::scratch_data &Scratchpad ) {

    auto const name = ToLower( Input.getToken<std::string>() );
    auto const type = Input.getToken<std::string>();

    basic_event *event { nullptr };

         if( type == "addvalues" )    { event = new updatevalues_event(); }
    else if( type == "updatevalues" ) { event = new updatevalues_event(); }
    else if( type == "copyvalues" )   { event = new copyvalues_event(); }
    else if( type == "getvalues" )    { event = new getvalues_event(); }
    else if( type == "putvalues" )    { event = new putvalues_event(); }
    else if( type == "whois" )        { event = new whois_event(); }
    else if( type == "logvalues" )    { event = new logvalues_event(); }
    else if( type == "multiple" )     { event = new multi_event(); }
    else if( type == "switch" )       { event = new switch_event(); }
    else if( type == "trackvel" )     { event = new track_event(); }
    else if( type == "sound" )        { event = new sound_event(); }
    else if( type == "texture" )      { event = new texture_event(); }
    else if( type == "animation" )    { event = new animation_event(); }
    else if( type == "lights" )       { event = new lights_event(); }
    else if( type == "voltage" )      { event = new voltage_event(); }
    else if( type == "visible" )      { event = new visible_event(); }
    else if( type == "friction" )     { event = new friction_event(); }
    else if( type == "message" )      { event = new message_event(); }

    if( event == nullptr ) {
        ErrorLog( "Bad event: unrecognized type \"" + type + "\" specified for event \"" + name + "\"." );
        return event;
    }

    event->m_name = name;
    if( type == "addvalues" ) {
        static_cast<updatevalues_event*>( event )->m_input.flags = basic_event::flags::mode_add;
    }
    return event;
}

//---------------------------------------------------------------------------

event_manager::~event_manager() {

    for( auto *event : m_events ) {
        delete event;
    }
}

// adds specified event launcher to the list of global launchers
void
event_manager::queue( TEventLauncher *Launcher ) {

    m_launcherqueue.emplace_back( Launcher );
}

// inserts in the event query events assigned to event launchers capable of receiving specified radio message sent from specified location
void
event_manager::queue_receivers( radio_message const Message, glm::dvec3 const &Location ) {

    for( auto *launcher : m_radiodrivenlaunchers.sequence() ) {
        if( ( launcher->key() == Message )
         && ( ( launcher->dRadius < 0 )
           || ( glm::length2( launcher->location() - Location ) < launcher->dRadius ) )
         && ( true == launcher->check_conditions() ) ) {
            // NOTE: only execution of event1 is supported for radio messages
            // TBD, TODO: consider ability/way to execute event2
            simulation::Events.AddToQuery( launcher->Event1, nullptr );
        }
    }
}

// legacy method, updates event queues
void
event_manager::update() {

    // process currently queued events
    CheckQuery();
    // test list of global events for possible new additions to the queue
    for( auto *launcher : m_launcherqueue ) {
		if (launcher->check_conditions() && launcher->Event1) {
			// NOTE: we're presuming global events aren't going to use event2

			if (launcher->check_activation()) {
				WriteLog( "Eventlauncher: " + launcher->name() );
				AddToQuery( launcher->Event1, nullptr );
			}

			if (launcher->check_activation_key()) {
				WriteLog( "Eventlauncher: " + launcher->name() );
				m_relay.post(user_command::queueevent, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &launcher->Event1->name());
			}
		}
    }
}

// adds provided event to the collection. returns: true on success
// TBD, TODO: return handle instead of pointer
bool
event_manager::insert( basic_event *Event ) {
    // najpierw sprawdzamy, czy nie ma, a potem dopisujemy
    auto lookup = m_eventmap.find( Event->m_name );
    if( lookup != m_eventmap.end() ) {
        // duplicate of already existing event
        auto const size = Event->m_name.size();
        // zawsze jeden znak co najmniej jest
        if( Event->m_name[ 0 ] == '#' ) {
            // utylizacja duplikatu z krzyżykiem
            return false;
        }
        // tymczasowo wyjątki:
        else if( ends_with( Event->m_name, "lineinfo:" ) ) {
            // tymczasowa utylizacja duplikatów W5
            return false;
        }
        else if( ends_with( Event->m_name, "_warning" ) ) {
            // tymczasowa utylizacja duplikatu z trąbieniem
            return false;
        }
        else if( ends_with( Event->m_name, "_shp" ) ) {
            // nie podlegają logowaniu
            // tymczasowa utylizacja duplikatu SHP
            return false;
        }

        auto *duplicate = m_events[ lookup->second ];
        if( Global.bJoinEvents ) {
            // doczepka (taki wirtualny multiple bez warunków)
            duplicate->append( Event );
        }
        else {
            // NOTE: somewhat convoluted way to deal with 'replacing' events without leaving dangling pointers
            // can be cleaned up if pointers to events were replaced with handles
            ErrorLog( "Bad scenario: duplicate event name \"" + Event->m_name + "\"" );
            duplicate->append( Event ); // doczepka (taki wirtualny multiple bez warunków)
            duplicate->m_ignored = true; // dezaktywacja pierwotnego - taka proteza na wsteczną zgodność
        }
    }

    m_events.emplace_back( Event );
    if( lookup == m_eventmap.end() ) {
        // if it's first event with such name, it's potential candidate for the execution queue
        m_eventmap.emplace( Event->m_name, m_events.size() - 1 );
        if( ( Event->m_ignored != true )
         && ( contains( Event->m_name, "onstart" ) ) ) {
            // event uruchamiany automatycznie po starcie
            AddToQuery( Event, nullptr );
        }
    }

    return true;
}

basic_event * event_manager::FindEventById(uint32_t id)
{
	if (id < m_events.size())
		return m_events[id];
	else
		return nullptr;
}

uint32_t event_manager::GetEventId(const basic_event *ev) {
	return GetEventId(ev->m_name);
}

uint32_t event_manager::GetEventId(const std::string &Name)
{
	if (Name.empty())
		return -1;

	auto const lookup = m_eventmap.find(Name);
	return lookup != m_eventmap.end() ? lookup->second : -1;
}

// legacy method, returns pointer to specified event, or null
basic_event *
event_manager::FindEvent( std::string const &Name )
{
	return FindEventById(GetEventId(Name));
}

// legacy method, inserts specified event in the event query
bool
event_manager::AddToQuery( basic_event *Event, TDynamicObject const *Owner, double delay ) {

    if( Event->m_passive )     { return false; } // jeśli może być dodany do kolejki (nie używany w skanowaniu)
    if( Event->m_inqueue > 0 ) { return false; } // jeśli nie dodany jeszcze do kolejki
        
    // kolejka eventów jest posortowana względem (fStartTime)
    Event->m_activator = Owner;
    if( Event->is_instant() ) {
        // eventy AddValues trzeba wykonywać natychmiastowo, inaczej kolejka może zgubić jakieś dodawanie
        if( false == Event->m_ignored ) {
            Event->run();
        }
        // jeśli jest kolejny o takiej samej nazwie, to idzie do kolejki (and if there's no joint event it'll be set to null and processing will end here)
        do {
            Event = Event->m_sibling;
            // NOTE: we could've received a new event from joint event above, so we need to check conditions just in case and discard the bad events
            // TODO: refactor this arrangement, it's hardly optimal
        } while( ( Event != nullptr )
              && ( ( Event->m_passive )
                || ( Event->m_inqueue > 0 ) ) );
    }
    if( ( Event != nullptr )
     && ( false == Event->m_ignored ) ) {
        // standardowe dodanie do kolejki
        ++(Event->m_inqueue); // zabezpieczenie przed podwójnym dodaniem do kolejki
        WriteLog( "EVENT ADDED TO QUEUE" + ( Owner ? ( " by " + Owner->asName ) : "" ) + ": " + Event->m_name );
        Event->m_launchtime = delay + std::abs( Event->m_delay ) + Timer::GetTime(); // czas od uruchomienia scenerii
        if( Event->m_delayrandom > 0.0 ) {
            // doliczenie losowego czasu opóźnienia
            Event->m_launchtime += Event->m_delayrandom * Random();
        }
        if( ( Owner != nullptr )
         && ( false == std::isnan( Event->m_delaydeparture ) ) ) {
            auto const *timetableowner { (
                ( ( Owner->Mechanik != nullptr ) && ( Owner->Mechanik->primary() ) ) ?
                    Owner->Mechanik :
                    Owner->ctOwner ) };
            if( timetableowner != nullptr ) {
                auto const &timetable { timetableowner->TrainTimetable() };
                auto const &time { simulation::Time.data() };
                Event->m_launchtime +=
                    timetable.seconds_until_departure( time.wHour, time.wMinute + time.wSecond * 0.0167 )
                    + Event->m_delaydeparture;
            }
        }
        // NOTE: sanity check, as departure-based delay math can potentially produce negative overall delay
        Event->m_launchtime = std::max( Event->m_launchtime, 0.0 );
        if( QueryRootEvent != nullptr ) {
            basic_event *target { QueryRootEvent };
            basic_event *previous { nullptr };
            while( ( Event->m_launchtime >= target->m_launchtime )
                && ( target->m_next != nullptr ) ) {
                previous = target;
                target = target->m_next;
            }
            // the new event will be either before or after the one we located
            if( Event->m_launchtime >= target->m_launchtime ) {
                assert( target->m_next == nullptr );
                target->m_next = Event;
                // if we have resurrected event land at the end of list, the link from previous run could potentially "add" unwanted events to the queue
                Event->m_next = nullptr;
            }
            else {
                if( previous != nullptr ) {
                    previous->m_next = Event;
                    Event->m_next = target;
                }
                else {
                    // special case, we're inserting our event at the very start
                    Event->m_next = QueryRootEvent;
                    QueryRootEvent = Event;
                }
            }
        }
        else {
            QueryRootEvent = Event;
            QueryRootEvent->m_next = nullptr;
        }
    }

    return true;
}

// legacy method, executes queued events
bool
event_manager::CheckQuery() {

    while( ( QueryRootEvent != nullptr )
        && ( QueryRootEvent->m_launchtime < Timer::GetTime() ) )
    { // eventy są posortowana wg czasu wykonania
        m_workevent = QueryRootEvent; // wyjęcie eventu z kolejki
        if (QueryRootEvent->m_sibling) // jeśli jest kolejny o takiej samej nazwie
        { // to teraz on będzie następny do wykonania
            QueryRootEvent = QueryRootEvent->m_sibling; // następny będzie ten doczepiony
            QueryRootEvent->m_next = m_workevent->m_next; // pamiętając o następnym z kolejki
            QueryRootEvent->m_launchtime = m_workevent->m_launchtime; // czas musi być ten sam, bo nie jest aktualizowany
            QueryRootEvent->m_activator = m_workevent->m_activator; // pojazd aktywujący
            QueryRootEvent->m_inqueue = 1;
            // w sumie można by go dodać normalnie do kolejki, ale trzeba te połączone posortować wg czasu wykonania
        }
        else // a jak nazwa jest unikalna, to kolejka idzie dalej
            QueryRootEvent = QueryRootEvent->m_next; // NULL w skrajnym przypadku
        if( ( false == m_workevent->m_ignored ) && ( false == m_workevent->m_passive ) ) {
            // w zasadzie te wyłączone są skanowane i nie powinny się nigdy w kolejce znaleźć
            --(m_workevent->m_inqueue); // teraz moze być ponownie dodany do kolejki
            m_workevent->run();
        } // if (tmpEvent->bEnabled)
    } // while
    return true;
}

// legacy method, initializes events after deserialization from scenario file
void
event_manager::InitEvents() {
    //łączenie eventów z pozostałymi obiektami
    for( auto *event : m_events ) {
        event->init();
        if( event->m_delay < 0 ) { AddToQuery( event, nullptr ); }
    }
}

// legacy method, initializes event launchers after deserialization from scenario file
void
event_manager::InitLaunchers() {

    std::vector<basic_table<TEventLauncher> *> launchertables {
        &m_inputdrivenlaunchers,
        &m_radiodrivenlaunchers
    };

    for( auto *launchertable : launchertables ) {
        for( auto *launcher : launchertable->sequence() ) {

            if( launcher->iCheckMask != 0 ) {
                if( launcher->asMemCellName != "none" ) {
                    // jeśli jest powiązana komórka pamięci
                    launcher->MemCell = simulation::Memory.find( launcher->asMemCellName );
                    if( launcher->MemCell == nullptr ) {
                        ErrorLog( "Bad scenario: event launcher \"" + launcher->name() + "\" can't find memcell \"" + launcher->asMemCellName + "\"" );
                    }
                }
                else {
                    launcher->MemCell = nullptr;
                }
            }

            if( launcher->asEvent1Name != "none" ) {
                launcher->Event1 = simulation::Events.FindEvent( launcher->asEvent1Name );
                if( launcher->Event1 == nullptr ) {
                    ErrorLog( "Bad scenario: event launcher \"" + launcher->name() + "\" can't find event \"" + launcher->asEvent1Name + "\"" );
                }
            }

            if( launcher->asEvent2Name != "none" ) {
                launcher->Event2 = simulation::Events.FindEvent( launcher->asEvent2Name );
                if( launcher->Event2 == nullptr ) {
                    ErrorLog( "Bad scenario: event launcher \"" + launcher->name() + "\" can't find event \"" + launcher->asEvent2Name + "\"" );
                }
            }
        }
    }
}

// sends basic content of the class in legacy (text) format to provided stream
void
event_manager::export_as_text( std::ostream &Output ) const {

    Output << "// events\n";
    for( auto const *event : m_events ) {
        if( event->group() == null_handle ) {
            event->export_as_text( Output );
        }
    }
    Output << "// event launchers, basic\n";
    for( auto const *launcher : m_inputdrivenlaunchers.sequence() ) {
        if( launcher->group() == null_handle ) {
            launcher->export_as_text( Output );
        }
    }
    Output << "// event launchers, radio driven\n";
    for( auto const *launcher : m_radiodrivenlaunchers.sequence() ) {
        if( launcher->group() == null_handle ) {
            launcher->export_as_text( Output );
        }
    }
}

std::vector<TEventLauncher*> event_manager::find_eventlaunchers(glm::vec2 center, float radius) const {
	std::vector<TEventLauncher *> results;

	for (auto &launcher : m_inputdrivenlaunchers.sequence()) {
		glm::dvec3 location = launcher->location();
		if (glm::distance2(glm::vec2(location.x, location.z), center) < radius)
			results.push_back(launcher);
	}

	return results;
}
