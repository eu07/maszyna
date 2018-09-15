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
#include "event.h"

#include "simulation.h"
#include "messaging.h"
#include "globals.h"
#include "memcell.h"
#include "track.h"
#include "traction.h"
#include "tractionpower.h"
#include "sound.h"
#include "animmodel.h"
#include "dynobj.h"
#include "driver.h"
#include "timer.h"
#include "logs.h"

TMemCell const *
TEvent::update_data::data_cell() const {

    return static_cast<TMemCell const *>( std::get<scene::basic_node *>( data_source ) );
}
TMemCell *
TEvent::update_data::data_cell() {

    return static_cast<TMemCell *>( std::get<scene::basic_node *>( data_source ) );
}

TEvent::~TEvent() {

    switch (Type)
    { // sprzątanie
    case tp_Animation: // nic
        if( m_animationtype == 4 ) {
            // jeśli z pliku VMD
            SafeDeleteArray( m_animationfiledata ); // zwolnić obszar
        }
        break;
    default:
        break;
    }
    evJoined = nullptr; // nie usuwać podczepionych tutaj

};

void TEvent::Load(cParser *parser, Math3D::vector3 const &org)
{
    std::string token;

    bEnabled = true; // zmieniane na false dla eventów używanych do skanowania sygnałów

    parser->getTokens();
    *parser >> token;
    asName = ToLower(token); // użycie parametrów może dawać wielkie

    parser->getTokens();
    *parser >> token;

    if (token == "updatevalues")
        Type = tp_UpdateValues;
    else if (token == "getvalues")
        Type = tp_GetValues;
    else if (token == "putvalues")
        Type = tp_PutValues;
    else if (token == "sound")
        Type = tp_Sound;
    else if (token == "animation")
        Type = tp_Animation;
    else if (token == "lights")
        Type = tp_Lights;
    else if (token == "visible")
        Type = tp_Visible; // zmiana wyświetlania obiektu
    else if (token == "switch")
        Type = tp_Switch;
    else if (token == "trackvel")
        Type = tp_TrackVel;
    else if (token == "multiple")
        Type = tp_Multiple;
    else if (token == "addvalues")
        Type = tp_AddValues;
    else if (token == "copyvalues")
        Type = tp_CopyValues;
    else if (token == "whois")
        Type = tp_WhoIs;
    else if (token == "logvalues")
        Type = tp_LogValues;
    else if (token == "voltage")
        Type = tp_Voltage; // zmiana napięcia w zasilaczu (TractionPowerSource)
    else if (token == "message")
        Type = tp_Message; // wyświetlenie komunikatu
    else if (token == "friction")
        Type = tp_Friction; // zmiana tarcia na scenerii
    else
        Type = tp_Unknown;

    parser->getTokens();
    *parser >> fDelay;

    parser->getTokens();
    *parser >> token;
/*
    if( token != "none" ) {
        asNodeName = token; // nazwa obiektu powiązanego
    }
*/
    load_targets( token );

    if (asName.substr(0, 5) == "none_")
        m_ignored = true; // Ra: takie są ignorowane

    switch (Type) {
    case tp_AddValues: {
        m_update.flags = update_memadd; // dodawanko
        // NOTE: AddValues and UpdateValues are largely the same
    }
    case tp_UpdateValues: {
        // update data, previously stored in params 0, 1, 2
        parser->getTokens( 1, false ); // case sensitive
        *parser >> m_update.data_text;
        if( m_update.data_text != "*" ) { //"*" - nie brac command pod uwage
            m_update.flags |= update_memstring;
        }
        parser->getTokens();
        if( parser->peek() != "*" ) { //"*" - nie brac val1 pod uwage
            *parser >> m_update.data_value_1;
            m_update.flags |= update_memval1;
        }
        parser->getTokens();
        if( parser->peek() != "*" ) { //"*" - nie brac val2 pod uwage
            *parser >> m_update.data_value_2;
            m_update.flags |= update_memval2;
        }
        // optional blocks
        while( ( false == ( token = parser->getToken<std::string>() ).empty() )
            && ( token != "endevent" ) ) {

            if( token == "condition" ) {
                m_condition.load( *parser );
                // NOTE: load() call leaves a preloaded token if it's not recognized
                *parser >> token;
                if( ( true == token.empty() ) || ( token == "endevent" ) ) { break; }
            }
            if( token == "randomdelay" ) { // losowe opóźnienie
                parser->getTokens();
                *parser >> fRandomDelay; // Ra 2014-03-11
            }
        }
        break;
    }
    case tp_CopyValues: {
        m_update.flags = update_memstring | update_memval1 | update_memval2; // normalnie trzy
        int paramidx { 0 };
        while( ( false == ( token = parser->getToken<std::string>() ).empty() )
            && ( token != "endevent" ) ) {
            switch( ++paramidx ) {
                case 1: { // nazwa drugiej komórki (źródłowej) // previously stored in param 9
                    std::get<std::string>( m_update.data_source ) = token;
                    break;
                }
                case 2: { // maska wartości
                    m_update.flags = stol_def( token, ( update_memstring | update_memval1 | update_memval2 ) );
                    break;
                }
                default: {
                    break;
                }
            }
        }
        break;
    }
    case tp_WhoIs: {
        m_update.flags = update_memstring | update_memval1 | update_memval2; // normalnie trzy
        int paramidx { 0 };
        while( ( false == ( token = parser->getToken<std::string>() ).empty() )
            && ( token != "endevent" ) ) {
            switch( ++paramidx ) {
                case 1: { // maska wartości
                    m_update.flags = stol_def( token, ( update_memstring | update_memval1 | update_memval2 ) );
                    break;
                }
                default: {
                    break;
                }
            }
        }
        break;
    }
    case tp_GetValues:
    case tp_LogValues: {
        parser->getTokens(); //"endevent"
        *parser >> token;
        break;
    }
    case tp_PutValues: {
        parser->getTokens( 3 );
        // location, previously held in param 3, 4, 5
        *parser
            >> m_update.location.x
            >> m_update.location.y
            >> m_update.location.z;
        // przesunięcie
        // tmp->pCenter.RotateY(aRotate.y/180.0*M_PI); //Ra 2014-11: uwzględnienie rotacji
        // TBD, TODO: take into account rotation as well?
        m_update.location += glm::dvec3{ org };

        parser->getTokens( 1, false ); // komendy 'case sensitive'
        *parser >> token;
        // command type, previously held in param 6
        if( token.substr( 0, 19 ) == "PassengerStopPoint:" ) {
            if( token.find( '#' ) != std::string::npos )
                token.erase( token.find( '#' ) ); // obcięcie unikatowości
            win1250_to_ascii( token ); // get rid of non-ascii chars
            m_update.command_type = TCommandType::cm_PassengerStopPoint;
            // nie do kolejki (dla SetVelocity też, ale jak jest do toru dowiązany)
            bEnabled = false;
        }
        else if( token == "SetVelocity" ) {
            m_update.command_type = TCommandType::cm_SetVelocity;
            bEnabled = false;
        }
        else if( token == "RoadVelocity" ) {
            m_update.command_type = TCommandType::cm_RoadVelocity;
            bEnabled = false;
        }
        else if( token == "SectionVelocity" ) {
            m_update.command_type = TCommandType::cm_SectionVelocity;
            bEnabled = false;
        }
        else if( token == "ShuntVelocity" ) {
            m_update.command_type = TCommandType::cm_ShuntVelocity;
            bEnabled = false;
        }
        else if( token == "OutsideStation" ) {
            m_update.command_type = TCommandType::cm_OutsideStation;
            bEnabled = false; // ma być skanowny, aby AI nie przekraczało W5
        }
        else {
            m_update.command_type = TCommandType::cm_Unknown;
        }
        // update data, previously stored in params 0, 1, 2
        m_update.data_text = token;
        parser->getTokens();
        if( parser->peek() != "none" ) {
            *parser >> m_update.data_value_1;
        }
        parser->getTokens();
        if( parser->peek() != "none" ) {
            *parser >> m_update.data_value_2;
        }
        parser->getTokens();
        *parser >> token;
        break;
    }
    case tp_Lights: {
        // TBD, TODO: remove light count limit?
        auto const lightcountlimit { 8 };
        m_lights.resize( lightcountlimit );
        int paramidx { 0 };
        parser->getTokens();
        while( ( false == ( token = parser->peek() ).empty() )
            && ( token != "endevent" ) ) {

            if( paramidx < lightcountlimit ) {
                *parser >> m_lights[ paramidx++ ];
            }
            else {
                ErrorLog( "Bad event: lights event \"" + asName + "\" with more than " + to_string( lightcountlimit ) + " parameters" );
            }
            parser->getTokens();
        }
        while( paramidx < lightcountlimit ) {
            // HACK: mark unspecified lights with magic value
            m_lights[ paramidx++ ] = -2.f;
        }
        break;
    }
    case tp_Visible: // zmiana wyświetlania obiektu
        parser->getTokens();
        // visibility flag, previously held in param 0
        *parser >> m_visible;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Sound:
        parser->getTokens();
        // playback mode, previously held in param 0 // 0: wylaczyc, 1: wlaczyc; -1: wlaczyc zapetlone
        *parser >> m_soundmode;
        parser->getTokens();
        if( parser->peek() != "endevent" ) {
            // optional parameter, radio channel to receive/broadcast the sound, previously held in param 1
            *parser >> m_soundradiochannel;
            parser->getTokens();
        }
        *parser >> token;
        break;
    case tp_Animation:
        parser->getTokens();
        *parser >> token;
        if (token.compare("rotate") == 0)
        { // obrót względem osi
            parser->getTokens();
            // animation submodel, previously held in param 9
            *parser >> m_animationsubmodel;
            // animation type, previously held in param 0
            m_animationtype = 1;
            parser->getTokens( 4 );
            // animation params, previously held in param 1, 2, 3, 4
            *parser
				>> m_animationparams[ 0 ]
				>> m_animationparams[ 1 ]
				>> m_animationparams[ 2 ]
				>> m_animationparams[ 3 ];
        }
        else if (token.compare("translate") == 0)
        { // przesuw o wektor
            parser->getTokens();
            // animation submodel, previously held in param 9
            *parser >> m_animationsubmodel;
            // animation type, previously held in param 0
            m_animationtype = 2;
            parser->getTokens( 4 );
            // animation params, previously held in param 1, 2, 3, 4
            *parser
                >> m_animationparams[ 0 ]
                >> m_animationparams[ 1 ]
                >> m_animationparams[ 2 ]
                >> m_animationparams[ 3 ];
        }
        else if (token.compare("digital") == 0)
        { // licznik cyfrowy
            parser->getTokens();
            // animation submodel, previously held in param 9
            *parser >> m_animationsubmodel;
            // animation type, previously held in param 0
            m_animationtype = 8;
            parser->getTokens( 4 );
            // animation params, previously held in param 1, 2, 3, 4
            *parser
                >> m_animationparams[ 0 ]
                >> m_animationparams[ 1 ]
                >> m_animationparams[ 2 ]
                >> m_animationparams[ 3 ];
        }
        else if (token.substr(token.length() - 4, 4) == ".vmd") // na razie tu, może będzie inaczej
        { // animacja z pliku VMD
			{
                m_animationfilename = token;
				std::ifstream file( szModelPath + m_animationfilename, std::ios::binary | std::ios::ate ); file.unsetf( std::ios::skipws );
				auto size = file.tellg();   // ios::ate already positioned us at the end of the file
				file.seekg( 0, std::ios::beg ); // rewind the caret afterwards
                // animation size, previously held in param 7
                m_animationfilesize = size;
                // animation data, previously held in param 8
                m_animationfiledata = new char[size];
				file.read( m_animationfiledata, size ); // wczytanie pliku
			}
            parser->getTokens();
            // animation submodel, previously held in param 9
            *parser >> m_animationsubmodel;
            // animation type, previously held in param 0
            m_animationtype = 4;
            parser->getTokens( 4 );
            // animation params, previously held in param 1, 2, 3, 4
            *parser
                >> m_animationparams[ 0 ]
                >> m_animationparams[ 1 ]
                >> m_animationparams[ 2 ]
                >> m_animationparams[ 3 ];
        }
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Switch:
        parser->getTokens();
        // switch state, previously held in param 0
        *parser >> m_switchstate;
        parser->getTokens();
        if (parser->peek() != "endevent") {
            // prędkość liniowa ruchu iglic
            // previously held in param 1
            *parser >> m_switchmoverate;
            parser->getTokens();
        }
        if (parser->peek() != "endevent") {
            // dodatkowy ruch drugiej iglicy (zamknięcie nastawnicze)
            // previously held in param 2
            *parser >> m_switchmovedelay;
            parser->getTokens();
        }
        break;
    case tp_TrackVel:
        parser->getTokens();
        // velocity, previously held in param 0
        *parser >> m_velocity;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Multiple: {
        m_condition.has_else = false;
        while( ( false == ( token = parser->getToken<std::string>() ).empty() )
            && ( token != "endevent" ) ) {

            if( token == "condition" ) {
                m_condition.load( *parser );
                // NOTE: load() call leaves a preloaded token if it's not recognized
                *parser >> token;
                if( ( true == token.empty() ) || ( token == "endevent" ) ) { break; }
            }

            if( token == "randomdelay" ) { // losowe opóźnienie
                parser->getTokens();
                *parser >> fRandomDelay; // Ra 2014-03-11
            }
            else if( token == "else" ) {
                // zmiana flagi dla słowa "else"
                m_condition.has_else = !m_condition.has_else;
            }
            else {
                // potentially valid event name
                if( token.substr( 0, 5 ) != "none_" ) {
                    // eventy rozpoczynające się od "none_" są ignorowane
                    m_children.emplace_back( token, nullptr, ( m_condition.has_else == false ) );
                }
                else {
                    WriteLog( "Multi-event \"" + asName + "\" ignored link to event \"" + token + "\"" );
                }
            }
        }
        break;
    }
    case tp_Voltage: // zmiana napięcia w zasilaczu (TractionPowerSource)
        parser->getTokens();
        // voltage, previously held in param 0
        *parser >> m_voltage;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Friction: // zmiana przyczepnosci na scenerii
        parser->getTokens();
        // friction, previously held in param 0
        *parser >> m_friction;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Message: // wyświetlenie komunikatu
        do
        {
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        } while (token != "endevent");
        break;
    case tp_Unknown: // nieznany
        do
        {
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        } while (token != "endevent");
        WriteLog("Bad event: \"" + asName +
                 (Type == tp_Unknown ? "\" has unknown type." : "\" is ignored."));
        break;
    }
};

// prepares event for use
void
TEvent::init() {

    switch( Type ) {

        case tp_AddValues: // sumowanie wartości
        case tp_UpdateValues: { // zmiana wartości
                                // target memory cell(s), previously held in param 5, copy held in param 9 for memcompare
            init_targets( simulation::Memory, "memory cell" );
            // conditional data
            init_conditions();
            break;
        }
        case tp_LogValues: {
            // skojarzenie z memcell
            init_targets( simulation::Memory, "memory cell" );
            break;
        }
        case tp_GetValues: {
            init_targets( simulation::Memory, "memory cell" );
            // custom target initialization code
            for( auto &target : m_targets ) {
                auto *targetcell = static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) );
                if( targetcell == nullptr ) { continue; }
                if( targetcell->IsVelocity() ) {
                    // jeśli odczyt komórki a komórka zawiera komendę SetVelocity albo ShuntVelocity
                    // to event nie będzie dodawany do kolejki
                    bEnabled = false;
                }
            }
            // NOTE: GetValues retrieves data only from first specified memory cell
            // TBD, TODO: allow retrieval from more than one cell?
            m_update.data_source = m_targets.front();
            break;
        }
        case tp_WhoIs: {
            init_targets( simulation::Memory, "memory cell" );
            break;
        }
        case tp_CopyValues: {
            // skopiowanie komórki do innej
            init_targets( simulation::Memory, "memory cell" );
            // source cell
            std::get<scene::basic_node *>( m_update.data_source ) = simulation::Memory.find( std::get<std::string>( m_update.data_source ) );
            if( std::get<scene::basic_node *>( m_update.data_source ) == nullptr ) {
                m_ignored = true; // deaktywacja
                ErrorLog( "Bad event: copyvalues event \"" + asName + "\" cannot find memcell \"" + std::get<std::string>( m_update.data_source ) + "\"" );
            }
            break;
        }
        case tp_Animation: {
            // animacja modelu
            init_targets( simulation::Instances, "model instance" );
            // custom target initialization code
            if( m_animationtype == 4 ) {
                // vmd animations target whole model
                break;
            }
            // locate and set up animated submodels
            m_animationcontainers.clear();
            for( auto &target : m_targets ) {
                auto *targetmodel{ static_cast<TAnimModel *>( std::get<scene::basic_node *>( target ) ) };
                if( targetmodel == nullptr ) { continue; }
                auto *targetcontainer{ targetmodel->GetContainer( m_animationsubmodel ) };
                if( targetcontainer == nullptr ) {
                    m_ignored = true;
                    ErrorLog( "Bad event: animation event \"" + asName + "\" cannot find submodel " + m_animationsubmodel + " in model instance \"" + targetmodel->name() + "\"" );
                    break;
                }
                targetcontainer->WillBeAnimated(); // oflagowanie animacji
                if( targetcontainer->Event() == nullptr ) {
                    // nie szukać, gdy znaleziony
                    targetcontainer->EventAssign( simulation::Events.FindEvent( targetmodel->name() + '.' + m_animationsubmodel + ":done" ) );
                }
                m_animationcontainers.emplace_back( targetcontainer );
            }
            break;
        }
        case tp_Lights: {
            // zmiana świeteł modelu
            init_targets( simulation::Instances, "model instance" );
            break;
        }
        case tp_Visible: {
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
                    ErrorLog( "Bad event: event \"" + asName + "\" [" + type_to_string() + "] cannot find item \"" + std::get<std::string>( target ) + "\"" );
                }
            }
            break;
        }
        case tp_Switch: {
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
            break;
        }
        case tp_Sound: {
            // odtworzenie dźwięku
            for( auto &target : m_sounds ) {
                std::get<sound_source *>( target ) = simulation::Sounds.find( std::get<std::string>( target ) );
                if( std::get<sound_source *>( target ) == nullptr ) {
                    m_ignored = true; // deaktywacja
                    ErrorLog( "Bad event: event \"" + asName + "\" [" + type_to_string() + "] cannot find static sound \"" + std::get<std::string>( target ) + "\"" );
                }
            }
            break;
        }
        case tp_TrackVel: {
            // ustawienie prędkości na torze
            init_targets( simulation::Paths, "track" );
            // custom target initialization code
            for( auto &target : m_targets ) {
                auto *targettrack = static_cast<TTrack *>( std::get<scene::basic_node *>( target ) );
                if( targettrack == nullptr ) { continue; }
                // flaga zmiany prędkości toru jest istotna dla skanowania
                targettrack->iAction |= 0x200;
            }
            break;
        }
        case tp_Multiple: {
            // conditional data
            init_targets( simulation::Memory, "memory cell", false );
            if( m_ignored ) {
                m_condition.flags &= ~( conditional_memstring | conditional_memval1 | conditional_memval2 );
                m_ignored = false;
            }
            init_conditions();
            // child events bindings
            for( auto &childevent : m_children ) {
                std::get<TEvent *>( childevent ) = simulation::Events.FindEvent( std::get<std::string>( childevent ) );
                if( std::get<TEvent *>( childevent ) == nullptr ) {
                    ErrorLog( "Bad event: multi-event \"" + asName + "\" cannot find event \"" + std::get<std::string>( childevent ) + "\"" );
                }
            }
            break;
        }
        case tp_Voltage: {
            // zmiana napięcia w zasilaczu (TractionPowerSource)
            init_targets( simulation::Powergrid, "power source" );
            break;
        }
        case tp_Message: {
            // wyświetlenie komunikatu
            break;
        }
        default: {
            break;
        }
    } // switch
}

bool
TEvent::test_condition() const {

    if( m_condition.flags == 0 ) {
        return true; // bezwarunkowo
    }
    // if there's conditions, check them
    if( m_condition.flags & conditional_propability ) {
        auto const rprobability { static_cast<float>( Random() ) };
        WriteLog( "Test: Random integer - [" + std::to_string( rprobability ) + "] / [" + std::to_string( m_condition.probability ) + "]" );
        if( rprobability > m_condition.probability ) {
            return false;
        }
    }
    if( m_condition.flags & conditional_trackoccupied ) {
        for( auto *track : m_condition.tracks ) {
            if( true == track->IsEmpty() ) {
                return false;
            }
        }
    }
    if( m_condition.flags & conditional_trackfree ) {
        for( auto *track : m_condition.tracks ) {
            if( false == track->IsEmpty() ) {
                return false;
            }
        }
    }
    if( m_condition.flags & ( conditional_memstring | conditional_memval1 | conditional_memval2 ) ) {
        // porównanie wartości
        for( auto &target : m_targets ) {
            auto *targetcell = static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) );
            if( targetcell == nullptr ) {
                ErrorLog( "Event " + asName + " trying conditional_memcompare with nonexistent memcell" );
                continue; // though this is technically error, we treat it as a success to maintain backward compatibility
            }
            auto const comparisonresult =
                targetcell->Compare(
                    m_condition.match_text,
                    m_condition.match_value_1,
                    m_condition.match_value_2,
                    m_condition.flags );

            std::string comparisonlog = "Test: MemCompare - ";

            comparisonlog +=
                   "[" + targetcell->Text() + "]"
                + " [" + to_string( targetcell->Value1(), 2 ) + "]"
                + " [" + to_string( targetcell->Value2(), 2 ) + "]";

            comparisonlog += (
                true == comparisonresult ?
                    " == " :
                    " != " );

            comparisonlog += (
                TestFlag( m_condition.flags, conditional_memstring ) ?
                    "[" + std::string( m_condition.match_text ) + "]" :
                    "[*]" );
            comparisonlog += (
                TestFlag( m_condition.flags, conditional_memval1 ) ?
                    " [" + to_string( m_condition.match_value_1, 2 ) + "]" :
                    " [*]" );
            comparisonlog += (
                TestFlag( m_condition.flags, conditional_memval2 ) ?
                    " [" + to_string( m_condition.match_value_2, 2 ) + "]" :
                    " [*]" );

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
TEvent::run() {

    WriteLog( "EVENT LAUNCHED" + ( Activator ? ( " by " + Activator->asName ) : "" ) + ": " + asName );
    switch (Type) {
        case tp_CopyValues: {
            // skopiowanie wartości z innej komórki
            auto const *datasource = static_cast<TMemCell *>( std::get<scene::basic_node *>( m_update.data_source ) );
            m_update.data_text = datasource->Text();
            m_update.data_value_1 = datasource->Value1();
            m_update.data_value_2 = datasource->Value2();
        }
        case tp_AddValues: // różni się jedną flagą od UpdateValues
        case tp_UpdateValues: {
            if( false == test_condition() ) { break; }
            WriteLog( "Type: " + std::string( Type == tp_AddValues ? "AddValues & Track command" : "UpdateValues" ) + " - ["
                + m_update.data_text + "] ["
                + to_string( m_update.data_value_1, 2 ) + "] ["
                + to_string( m_update.data_value_2, 2 ) + "]" );
            // TODO: dump status of target cells after the operation
            for( auto &target : m_targets ) {
                auto *targetcell = static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) );
                if( targetcell == nullptr ) { continue; }
                targetcell->UpdateValues(
                    m_update.data_text,
                    m_update.data_value_1,
                    m_update.data_value_2,
                    m_update.flags );
                if( targetcell->Track == nullptr ) { continue; }
                // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
                auto const location { targetcell->location() };
                for( auto dynamic : targetcell->Track->Dynamics ) {
                    targetcell->PutCommand(
                        dynamic->Mechanik,
                        &location );
                }
            }
            break;
        }
        case tp_GetValues: {
            WriteLog( "Type: GetValues" );
            if( Activator ) {
                // TODO: re-enable when messaging module is in place
                if( Global.iMultiplayer ) {
                    // potwierdzenie wykonania dla serwera (odczyt semafora już tak nie działa)
                    multiplayer::WyslijEvent( asName, Activator->name() );
                }
                m_update.data_cell()->PutCommand(
                    Activator->Mechanik,
                    &(m_update.data_cell()->location()) );
            }
            break;
        }
        case tp_PutValues: {
            WriteLog( "Type: PutValues - [" +
                m_update.data_text + "] [" +
                to_string( m_update.data_value_1, 2 ) + "] [" +
                to_string( m_update.data_value_2, 2 ) + "]" );
            if (Activator) {
                // zamiana, bo fizyka ma inaczej niż sceneria
                // NOTE: y & z swap, negative x
                TLocation const loc {
                    -m_update.location.x,
                     m_update.location.z,
                     m_update.location.y };
                if( Activator->Mechanik ) {
                    // przekazanie rozkazu do AI
                    Activator->Mechanik->PutCommand(
                        m_update.data_text,
                        m_update.data_value_1,
                        m_update.data_value_2,
                        loc);
                }
                else {
                    // przekazanie do pojazdu
                    Activator->MoverParameters->PutCommand(
                        m_update.data_text,
                        m_update.data_value_1,
                        m_update.data_value_2,
                        loc);
                }
            }
            break;
        }
        case tp_Lights: {
            for( auto &target : m_targets ) {
                auto *targetmodel = static_cast<TAnimModel *>( std::get<scene::basic_node *>( target ) );
                if( targetmodel == nullptr ) { continue; }
                // event effect code
                for( auto i = 0; i < iMaxNumLights; ++i ) {
                    if( m_lights[ i ] == -2.f ) {
                        // processed all supplied values, bail out
                        break;
                    }
                    if( m_lights[ i ] >= 0.f ) {
                        // -1 zostawia bez zmiany
                        targetmodel->LightSet(
                            i,
                            m_lights[ i ] );
                    }
                }
            }
            break;
        }
        case tp_Visible: {
            for( auto &target : m_targets ) {
                auto *targetnode = std::get<scene::basic_node *>( target );
                if( targetnode == nullptr ) { continue; }
                // event effect code
                targetnode->visible( m_visible );
            }
            break;
        }
        case tp_Sound: {
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
            break;
        }
        case tp_Animation: {
            WriteLog( "Type: Animation" );
            if( m_animationtype == 4 ) {
                // vmd mode targets the entire model
                for( auto &target : m_targets ) {
                    auto *targetmodel = static_cast<TAnimModel *>( std::get<scene::basic_node *>( target ) );
                    if( targetmodel == nullptr ) { continue; }
                    // event effect code
                    targetmodel->AnimationVND(
                        m_animationfiledata,
                        m_animationparams[ 0 ], // tu mogą być dodatkowe parametry, np. od-do
                        m_animationparams[ 1 ],
                        m_animationparams[ 2 ],
                        m_animationparams[ 3 ] );
                }
            }
            else {
                // other animation modes target specific submodels
                for( auto *targetcontainer : m_animationcontainers ) {
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
                }
            }
            break;
        }
        case tp_Switch: {
            for( auto &target : m_targets ) {
                auto *targettrack = static_cast<TTrack *>( std::get<scene::basic_node *>( target ) );
                if( targettrack == nullptr ) { continue; }
                // event effect code
                targettrack->Switch(
                    m_switchstate,
                    m_switchmoverate,
                    m_switchmovedelay );
            }
            if( Global.iMultiplayer ) {
                // dajemy znać do serwera o przełożeniu
                multiplayer::WyslijEvent( asName, "" ); // wysłanie nazwy eventu przełączajacego
            }
            // Ra: bardziej by się przydała nazwa toru, ale nie ma do niej stąd dostępu
            break;
        }
        case tp_TrackVel: {
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
            break;
        }
        case tp_Multiple: {
            auto const condition { test_condition() };
            if( ( true == condition )
                || ( true == m_condition.has_else ) ) {
                // warunek spelniony albo było użyte else
                WriteLog("Type: Multi-event");
                for( auto &childevent : m_children ) {
                    auto *childeventdata { std::get<TEvent*>( childevent ) };
                    if( childeventdata == nullptr ) { continue; }
                    if( std::get<bool>( childevent ) != condition ) { continue; }

                    if( childeventdata != this ) {
                        // normalnie dodać
                        simulation::Events.AddToQuery( childeventdata, Activator );
                    }
                    else {
                        // jeśli ma być rekurencja to musi mieć sensowny okres powtarzania
                        if( fDelay >= 5.0 ) {
                            simulation::Events.AddToQuery( this, Activator );
                        }
                    }
                }
                if( Global.iMultiplayer ) {
                    // dajemy znać do serwera o wykonaniu
                    if( false == m_condition.has_else ) {
                        // jednoznaczne tylko, gdy nie było else
                        if( Activator ) {
                            multiplayer::WyslijEvent( asName, Activator->name() );
                        }
                        else {
                            multiplayer::WyslijEvent( asName, "" );
                        }
                    }
                }
            }
            break;
        }
        case tp_WhoIs: {
            for( auto &target : m_targets ) {
                auto *targetcell = static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) );
                if( targetcell == nullptr ) { continue; }
                // event effect code
                if( m_update.flags & update_load ) {
                    // jeśli pytanie o ładunek
                    if( m_update.flags & update_memadd ) {
                        // jeśli typ pojazdu
                        // TODO: define and recognize individual request types
                        auto const owner = (
                            ( ( Activator->Mechanik != nullptr ) && ( Activator->Mechanik->Primary() ) ) ?
                                Activator->Mechanik :
                                Activator->ctOwner );
                        auto const consistbrakelevel = (
                            owner != nullptr ?
                                owner->fReady :
                                -1.0 );
                        auto const collisiondistance = (
                            owner != nullptr ?
                                owner->TrackBlock() :
                                -1.0 );

                        targetcell->UpdateValues(
                            Activator->MoverParameters->TypeName, // typ pojazdu
                            consistbrakelevel,
                            collisiondistance,
                            m_update.flags & ( update_memstring | update_memval1 | update_memval2 ) );

                        WriteLog(
                            "Type: WhoIs (" + to_string( m_update.flags ) + ") - "
                            + "[name: " + Activator->MoverParameters->TypeName + "], "
                            + "[consist brake level: " + to_string( consistbrakelevel, 2 ) + "], "
                            + "[obstacle distance: " + to_string( collisiondistance, 2 ) + " m]" );
                    }
                    else {
                        // jeśli parametry ładunku
                        targetcell->UpdateValues(
                            Activator->MoverParameters->LoadType, // nazwa ładunku
                            Activator->MoverParameters->Load, // aktualna ilość
                            Activator->MoverParameters->MaxLoad, // maksymalna ilość
                            m_update.flags & ( update_memstring | update_memval1 | update_memval2 ) );

                        WriteLog(
                            "Type: WhoIs (" + to_string( m_update.flags ) + ") - "
                            + "[load type: " + Activator->MoverParameters->LoadType + "], "
                            + "[current load: " + to_string( Activator->MoverParameters->Load, 2 ) + "], "
                            + "[max load: " + to_string( Activator->MoverParameters->MaxLoad, 2 ) + "]" );
                    }
                }
                else if( m_update.flags & update_memadd ) { // jeśli miejsce docelowe pojazdu
                    targetcell->UpdateValues(
                        Activator->asDestination, // adres docelowy
                        Activator->DirectionGet(), // kierunek pojazdu względem czoła składu (1=zgodny,-1=przeciwny)
                        Activator->MoverParameters->Power, // moc pojazdu silnikowego: 0 dla wagonu
                        m_update.flags & ( update_memstring | update_memval1 | update_memval2 ) );

                    WriteLog(
                        "Type: WhoIs (" + to_string( m_update.flags ) + ") - "
                        + "[destination: " + Activator->asDestination + "], "
                        + "[direction: " + to_string( Activator->DirectionGet() ) + "], "
                        + "[engine power: " + to_string( Activator->MoverParameters->Power, 2 ) + "]" );
                }
                else if( Activator->Mechanik ) {
                    if( Activator->Mechanik->Primary() ) { // tylko jeśli ktoś tam siedzi - nie powinno dotyczyć pasażera!
                        targetcell->UpdateValues(
                            Activator->Mechanik->TrainName(),
                            Activator->Mechanik->StationCount() - Activator->Mechanik->StationIndex(), // ile przystanków do końca
                            Activator->Mechanik->IsStop() ?
                            1 :
                            0, // 1, gdy ma tu zatrzymanie
                            m_update.flags );
                        WriteLog( "Train detected: " + Activator->Mechanik->TrainName() );
                    }
                }
            }
            break;
        }
        case tp_LogValues: { // zapisanie zawartości komórki pamięci do logu
            if( m_targets.empty() ) {
                // lista wszystkich
                simulation::Memory.log_all();
            }
            else {
                // jeśli była podana nazwa komórki
                for( auto &target : m_targets ) {
                    auto *targetcell = static_cast<TMemCell *>( std::get<scene::basic_node *>( target ) );
                    if( targetcell == nullptr ) { continue; }
                    WriteLog( "Memcell \"" + targetcell->name() + "\": ["
                        + targetcell->Text() + "] ["
                        + to_string( targetcell->Value1(), 2 ) + "] ["
                        + to_string( targetcell->Value2(), 2 ) + "]" );
                }
            }
            break;
        }
        case tp_Voltage: { // zmiana napięcia w zasilaczu (TractionPowerSource)
            WriteLog( "Type: Voltage [" + to_string( m_voltage, 2 ) + "]" );
            for( auto &target : m_targets ) {
                auto *targetpowersource = static_cast<TTractionPowerSource *>( std::get<scene::basic_node *>( target ) );
                if( targetpowersource == nullptr ) { continue; }
                targetpowersource->VoltageSet( m_voltage );
            }
            break;
        }
        case tp_Friction: { // zmiana tarcia na scenerii
            // na razie takie chamskie ustawienie napięcia zasilania
            WriteLog("Type: Friction");
            Global.fFriction = (m_friction);
            break;
        }
        case tp_Message: { // wyświetlenie komunikatu
            break;
        }
    } // switch (tmpEvent->Type)
}

void
TEvent::init_conditions() {

    m_condition.tracks.clear();

    if( m_condition.flags & ( conditional_trackoccupied | conditional_trackfree ) ) {
        for( auto &target : m_targets ) {
            m_condition.tracks.emplace_back( simulation::Paths.find( std::get<std::string>( target ) ) );
            if( m_condition.tracks.back() == nullptr ) {
//                m_ignored = true; // deaktywacja
                ErrorLog( "Bad event: track \"" + std::get<std::string>( target ) + "\" referenced in event \"" + asName + "\" doesn't exist" );
                m_condition.flags &= ~( conditional_trackoccupied | conditional_trackfree ); // zerowanie flag
            }
        }
    }
}

void
TEvent::condition_data::load( cParser &Input  )
{ // przetwarzanie warunków, wspólne dla Multiple i UpdateValues

    std::string token;
    while( ( true == Input.getTokens() )
        && ( false == ( token = Input.peek() ).empty() )
        && ( token != "endevent" )
        && ( token != "randomdelay" ) ) {

        if( token == "trackoccupied" ) {
            flags |= conditional_trackoccupied;
        }
        else if( token == "trackfree" ) {
            flags |= conditional_trackfree;
        }
        else if( token == "propability" ) {
            flags |= conditional_propability;
            Input.getTokens();
            Input >> probability;
        }
        else if( token == "memcompare" ) {
            Input.getTokens( 1, false ); // case sensitive
            if( Input.peek() != "*" ) //"*" - nie brac command pod uwage
            { // zapamiętanie łańcucha do porównania
                Input >> match_text;
                flags |= conditional_memstring;
            }
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac val1 pod uwage
            {
                Input >> match_value_1;
                flags |= conditional_memval1;
            }
            Input.getTokens();
            if( Input.peek() != "*" ) //"*" - nie brac val2 pod uwage
            {
                Input >> match_value_2;
                flags |= conditional_memval2;
            }
        }
    }
}

void TEvent::load_targets( std::string const &Input ) {

    cParser targetparser{ Input };
    std::string target;
    while( false == ( target = targetparser.getToken<std::string>( true, "|," ) ).empty() ) {
        // actual bindings to targets of proper type are created during scenario initialization
        if( target != "none" ) {
            // sound objects don't inherit from scene node, so we can't use the same container
            // TODO: fix this, having sounds as scene nodes (or a node wrapper for one) might have benefits elsewhere
            if( Type != tp_Sound ) {
                m_targets.emplace_back( target, nullptr );
            }
            else {
                m_sounds.emplace_back( target, nullptr );
            }
        }
    }
}

std::string
TEvent::type_to_string() const {

    std::vector<std::string> const types {
        "unknown", "sound", "animation", "lights", "updatevalues",
        "getvalues", "putvalues", "switch", "trackvel", "multiple",
        "addvalues", "copyvalues", "whois", "logvalues", "visible",
        "voltage", "message", "friction" };

    return types[ Type ];
}

// sends basic content of the class in legacy (text) format to provided stream
void
TEvent::export_as_text( std::ostream &Output ) const {

    if( Type == tp_Unknown ) { return; }

    // header
    Output << "event ";
    // name
    Output << asName << ' ';
    // type
    Output << type_to_string() << ' ';
    // delay
    Output << fDelay << ' ';
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
    switch( Type ) {
        case tp_AddValues:
        case tp_UpdateValues: {
            Output
                << ( ( m_update.flags & update_memstring ) == 0 ? "*" :            m_update.data_text ) << ' '
                << ( ( m_update.flags & update_memval1 )   == 0 ? "*" : to_string( m_update.data_value_1 ) ) << ' '
                << ( ( m_update.flags & update_memval2 )   == 0 ? "*" : to_string( m_update.data_value_2 ) ) << ' ';
            break;
        }
        case tp_CopyValues: {
            auto const *datasource = static_cast<TMemCell *>( std::get<scene::basic_node *>( m_update.data_source ) );
            Output
                << ( datasource != nullptr ?
                        datasource->name() :
                        std::get<std::string>( m_update.data_source ) )
                << ' ' << ( m_update.flags & ( update_memstring | update_memval1 | update_memval2 ) ) << ' ';
            break;
        }
        case tp_PutValues: {
            Output
                // location
                << m_update.location.x << ' '
                << m_update.location.y << ' '
                << m_update.location.z << ' '
                // command
                << m_update.data_text << ' '
                << m_update.data_value_1 << ' '
                << m_update.data_value_2 << ' ';
            break;
        }
        case tp_Multiple: {
            for( auto const &childevent : m_children ) {
                if( true == std::get<bool>( childevent ) ) {
                    auto *childeventdata { std::get<TEvent *>( childevent ) };
                    Output
                        << ( childeventdata != nullptr ?
                            childeventdata->asName :
                            std::get<std::string>( childevent ) )
                        << ' ';
                }
            }
            // optional 'else' block
            if( false == m_condition.has_else ) { break; }
            Output << "else ";
            for( auto const &childevent : m_children ) {
                if( false == std::get<bool>( childevent ) ) {
                    auto *childeventdata { std::get<TEvent *>( childevent ) };
                    Output
                        << ( childeventdata != nullptr ?
                            childeventdata->asName :
                            std::get<std::string>( childevent ) )
                        << ' ';
                }
            }
            break;
        }
        case tp_Visible: {
            Output << ( m_visible ? 1 : 0 ) << ' ';
            break;
        }
        case tp_Switch: {
            Output << m_switchstate << ' ';
            if( ( m_switchmoverate < 0.f )
             || ( m_switchmovedelay < 0.f ) ) {
                Output << m_switchmoverate << ' ';
            }
            if( m_switchmovedelay < 0.f ) {
                Output << m_switchmovedelay << ' ';
            }
            break;
        }
        case tp_Lights: {
            auto lightidx { 0 };
            while( ( lightidx < iMaxNumLights )
                && ( m_lights[ lightidx ] > -2.0 ) ) {
                Output << m_lights[ lightidx ] << ' ';
                ++lightidx;
            }
            break;
        }
        case tp_Animation: {
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
                << m_animationparams[0] << ' '
                << m_animationparams[1] << ' '
                << m_animationparams[2] << ' '
                << m_animationparams[3] << ' ';
            break;
        }
        case tp_Sound: {
            // playback mode
            Output << m_soundmode << ' ';
            // optional radio channel
            if( m_soundradiochannel > 0 ) {
                Output << m_soundradiochannel << ' ';
            }
            break;
        }
        case tp_TrackVel:
            Output << m_velocity << ' ';
            break;
        case tp_Voltage:
            Output << m_voltage << ' ';
            break;
        case tp_Friction: {
            Output << m_friction << ' ';
            break;
        }
        case tp_WhoIs: {
            Output
                << ( m_update.flags & ( update_memstring | update_memval1 | update_memval2 ) ) << ' ';
            break;
        }
        default: {
            break;
        }
    }
    // optional conditions
    // NOTE: for flexibility condition check and export is performed for all event types rather than only for these which support it currently
    if( m_condition.flags != 0 ) {
        Output << "condition ";
        if( ( m_condition.flags & conditional_trackoccupied ) != 0 ) {
            Output << "trackoccupied ";
        }
        if( ( m_condition.flags & conditional_trackfree ) != 0 ) {
            Output << "trackfree ";
        }
        if( ( m_condition.flags & conditional_propability ) != 0 ) {
            Output
                << "propability "
                << m_condition.probability << ' ';
        }
        if( ( m_condition.flags & ( conditional_memstring | conditional_memval1 | conditional_memval2 ) ) != 0 ) {
            Output
                << "memcompare "
                << ( ( m_condition.flags & conditional_memstring ) == 0 ? "*" :            m_condition.match_text ) << ' '
                << ( ( m_condition.flags & conditional_memval1 )   == 0 ? "*" : to_string( m_condition.match_value_1 ) ) << ' '
                << ( ( m_condition.flags & conditional_memval2 )   == 0 ? "*" : to_string( m_condition.match_value_2 ) ) << ' ';
        }
    }
    if( fRandomDelay != 0.0 ) {
        Output
            << "randomdelay "
            << fRandomDelay << ' ';
    }
    // footer
    Output
        << "endevent"
        << "\n";
}

void TEvent::AddToQuery( TEvent *Event, TEvent *&Start ) {

    TEvent *target( Start );
    TEvent *previous( nullptr );
    while( ( Event->fStartTime >= target->fStartTime )
        && ( target->evNext != nullptr ) ) {
        previous = target;
        target = target->evNext;
    }
    // the new event will be either before or after the one we located
    if( Event->fStartTime >= target->fStartTime ) {
        assert( target->evNext == nullptr );
        target->evNext = Event;
        // if we have resurrected event land at the end of list, the link from previous run could potentially "add" unwanted events to the queue
        Event->evNext = nullptr;
    }
    else {
        if( previous != nullptr ) {
            previous->evNext = Event;
            Event->evNext = target;
        }
        else {
            // special case, we're inserting our event before the provided start point
            Event->evNext = Start;
            Start = Event;
        }
    }
}

//---------------------------------------------------------------------------

std::string TEvent::CommandGet() const
{ // odczytanie komendy z eventu
    switch (Type)
    { // to się wykonuje również składu jadącego bez obsługi
    case tp_GetValues:
        return m_update.data_cell()->Text();
    case tp_PutValues:
        return m_update.data_text;
    }
    return ""; // inne eventy się nie liczą
};

TCommandType TEvent::Command() const
{ // odczytanie komendy z eventu
    switch (Type)
    { // to się wykonuje również dla składu jadącego bez obsługi
    case tp_GetValues:
        return m_update.data_cell()->Command();
    case tp_PutValues:
        return m_update.command_type; // komenda zakodowana binarnie
    }
    return TCommandType::cm_Unknown; // inne eventy się nie liczą
};

double TEvent::ValueGet(int n) const
{ // odczytanie komendy z eventu
    n &= 1; // tylko 1 albo 2 jest prawidłowy
    switch (Type)
    { // to się wykonuje również składu jadącego bez obsługi
    case tp_GetValues:
        return ( n ? m_update.data_cell()->Value1() : m_update.data_cell()->Value2() );
    case tp_PutValues:
        return ( n ? m_update.data_value_1 : m_update.data_value_2 );
    }
    return 0.0; // inne eventy się nie liczą
};

glm::dvec3 TEvent::PositionGet() const
{ // pobranie współrzędnych eventu
    switch (Type)
    { //
    case tp_GetValues:
        return m_update.data_cell()->location(); // współrzędne podłączonej komórki pamięci
    case tp_PutValues:
        return m_update.location;
    }
    return glm::dvec3(0, 0, 0); // inne eventy się nie liczą
};

bool TEvent::StopCommand() const
{ //
    if (Type == tp_GetValues)
        return m_update.data_cell()->StopCommand(); // info o komendzie z komórki

    return false;
};

void TEvent::StopCommandSent()
{
    if (Type == tp_GetValues)
        m_update.data_cell()->StopCommandSent(); // komenda z komórki została wysłana
};

void TEvent::Append(TEvent *e)
{ // doczepienie kolejnych z tą samą nazwą
    if (evJoined)
        evJoined->Append(e); // rekurencja! - góra kilkanaście eventów będzie potrzebne
    else
    {
        evJoined = e;
        e->bEnabled = true; // ten doczepiony może być tylko kolejkowany
    }
};



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

// legacy method, updates event queues
void
event_manager::update() {

    // process currently queued events
    CheckQuery();
    // test list of global events for possible new additions to the queue
    for( auto *launcher : m_launcherqueue ) {

        if( true == ( launcher->check_activation() && launcher->check_conditions() ) ) {
            // NOTE: we're presuming global events aren't going to use event2
            WriteLog( "Eventlauncher " + launcher->name() );
            if( launcher->Event1 ) {
                AddToQuery( launcher->Event1, nullptr );
            }
        }
    }
}

// adds provided event to the collection. returns: true on success
// TODO: return handle instead of pointer
bool
event_manager::insert( TEvent *Event ) {

    if( Event->Type == tp_Unknown ) { return false; }

    // najpierw sprawdzamy, czy nie ma, a potem dopisujemy
    auto lookup = m_eventmap.find( Event->asName );
    if( lookup != m_eventmap.end() ) {
        // duplicate of already existing event
        auto const size = Event->asName.size();
        // zawsze jeden znak co najmniej jest
        if( Event->asName[ 0 ] == '#' ) {
            // utylizacja duplikatu z krzyżykiem
            return false;
        }
        // tymczasowo wyjątki:
        else if( ( size > 8 )
              && ( Event->asName.substr( 0, 9 ) == "lineinfo:" ) ) {
            // tymczasowa utylizacja duplikatów W5
            return false;
        }
        else if( ( size > 8 )
              && ( Event->asName.substr( size - 8 ) == "_warning" ) ) {
            // tymczasowa utylizacja duplikatu z trąbieniem
            return false;
        }
        else if( ( size > 4 )
              && ( Event->asName.substr( size - 4 ) == "_shp" ) ) {
            // nie podlegają logowaniu
            // tymczasowa utylizacja duplikatu SHP
            return false;
        }

        auto *duplicate = m_events[ lookup->second ];
        if( Global.bJoinEvents ) {
            // doczepka (taki wirtualny multiple bez warunków)
            duplicate->Append( Event );
        }
        else {
            // NOTE: somewhat convoluted way to deal with 'replacing' events without leaving dangling pointers
            // can be cleaned up if pointers to events were replaced with handles
            ErrorLog( "Bad event: encountered duplicated event, \"" + Event->asName + "\"" );
            duplicate->Append( Event ); // doczepka (taki wirtualny multiple bez warunków)
            duplicate->m_ignored = true; // dezaktywacja pierwotnego - taka proteza na wsteczną zgodność
        }
    }

    m_events.emplace_back( Event );
    if( lookup == m_eventmap.end() ) {
        // if it's first event with such name, it's potential candidate for the execution queue
        m_eventmap.emplace( Event->asName, m_events.size() - 1 );
        if( ( Event->m_ignored != true )
         && ( Event->asName.find( "onstart" ) != std::string::npos ) ) {
            // event uruchamiany automatycznie po starcie
            AddToQuery( Event, nullptr );
        }
    }

    return true;
}

// legacy method, returns pointer to specified event, or null
TEvent *
event_manager::FindEvent( std::string const &Name ) {

    if( Name.empty() ) { return nullptr; }

    auto const lookup = m_eventmap.find( Name );
    return (
        lookup != m_eventmap.end() ?
            m_events[ lookup->second ] :
            nullptr );
}

// legacy method, inserts specified event in the event query
bool
event_manager::AddToQuery( TEvent *Event, TDynamicObject const *Owner ) {

    if( false == Event->bEnabled ) { return false; } // jeśli może być dodany do kolejki (nie używany w skanowaniu)
    if( Event->iQueued != 0 )      { return false; } // jeśli nie dodany jeszcze do kolejki
        
    // kolejka eventów jest posortowana względem (fStartTime)
    Event->Activator = Owner;
    if( ( Event->Type == tp_AddValues )
     && ( Event->fDelay == 0.0 ) ) {
        // eventy AddValues trzeba wykonywać natychmiastowo, inaczej kolejka może zgubić jakieś dodawanie
        if( false == Event->m_ignored ) {
            Event->run();
        }
        // jeśli jest kolejny o takiej samej nazwie, to idzie do kolejki (and if there's no joint event it'll be set to null and processing will end here)
        do {
            Event = Event->evJoined;
            // NOTE: we could've received a new event from joint event above, so we need to check conditions just in case and discard the bad events
            // TODO: refactor this arrangement, it's hardly optimal
        } while( ( Event != nullptr )
              && ( ( false == Event->bEnabled )
                || ( Event->iQueued > 0 ) ) );
    }
    if( ( Event != nullptr )
     && ( false == Event->m_ignored ) ) {
        // standardowe dodanie do kolejki
        ++Event->iQueued; // zabezpieczenie przed podwójnym dodaniem do kolejki
        WriteLog( "EVENT ADDED TO QUEUE" + ( Owner ? ( " by " + Owner->asName ) : "" ) + ": " + Event->asName );
        Event->fStartTime = std::abs( Event->fDelay ) + Timer::GetTime(); // czas od uruchomienia scenerii
        if( Event->fRandomDelay > 0.0 ) {
            // doliczenie losowego czasu opóźnienia
            Event->fStartTime += Event->fRandomDelay * Random( 10000 ) * 0.0001;
        }
        if( QueryRootEvent != nullptr ) {
            TEvent::AddToQuery( Event, QueryRootEvent );
        }
        else {
            QueryRootEvent = Event;
            QueryRootEvent->evNext = nullptr;
        }
    }

    return true;
}

// legacy method, executes queued events
bool
event_manager::CheckQuery() {

    while( ( QueryRootEvent != nullptr )
        && ( QueryRootEvent->fStartTime < Timer::GetTime() ) )
    { // eventy są posortowana wg czasu wykonania
        m_workevent = QueryRootEvent; // wyjęcie eventu z kolejki
        if (QueryRootEvent->evJoined) // jeśli jest kolejny o takiej samej nazwie
        { // to teraz on będzie następny do wykonania
            QueryRootEvent = QueryRootEvent->evJoined; // następny będzie ten doczepiony
            QueryRootEvent->evNext = m_workevent->evNext; // pamiętając o następnym z kolejki
            QueryRootEvent->fStartTime = m_workevent->fStartTime; // czas musi być ten sam, bo nie jest aktualizowany
            QueryRootEvent->Activator = m_workevent->Activator; // pojazd aktywujący
            QueryRootEvent->iQueued = 1;
            // w sumie można by go dodać normalnie do kolejki, ale trzeba te połączone posortować wg czasu wykonania
        }
        else // a jak nazwa jest unikalna, to kolejka idzie dalej
            QueryRootEvent = QueryRootEvent->evNext; // NULL w skrajnym przypadku
        if( ( false == m_workevent->m_ignored ) && ( true == m_workevent->bEnabled ) ) {
            // w zasadzie te wyłączone są skanowane i nie powinny się nigdy w kolejce znaleźć
            --m_workevent->iQueued; // teraz moze być ponownie dodany do kolejki
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
        if( event->fDelay < 0 ) { AddToQuery( event, nullptr ); }
    }
}

// legacy method, initializes event launchers after deserialization from scenario file
void
event_manager::InitLaunchers() {

    for( auto *launcher : m_launchers.sequence() ) {

        if( launcher->iCheckMask != 0 ) {
            if( launcher->asMemCellName != "none" ) {
                // jeśli jest powiązana komórka pamięci
                launcher->MemCell = simulation::Memory.find( launcher->asMemCellName );
                if( launcher->MemCell == nullptr ) {
                    ErrorLog( "Bad scenario: event launcher \"" + launcher->name() + "\" cannot find memcell \"" + launcher->asMemCellName + "\"" );
                }
            }
            else {
                launcher->MemCell = nullptr;
            }
        }

        if( launcher->asEvent1Name != "none" ) {
            launcher->Event1 = simulation::Events.FindEvent( launcher->asEvent1Name );
            if( launcher->Event1 == nullptr ) {
                ErrorLog( "Bad scenario: event launcher \"" + launcher->name() + "\" cannot find event \"" + launcher->asEvent1Name + "\"" );
            }
        }

        if( launcher->asEvent2Name != "none" ) {
            launcher->Event2 = simulation::Events.FindEvent( launcher->asEvent2Name );
            if( launcher->Event2 == nullptr ) {
                ErrorLog( "Bad scenario: event launcher \"" + launcher->name() + "\" cannot find event \"" + launcher->asEvent2Name + "\"" );
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
    Output << "// event launchers\n";
    for( auto const *launcher : m_launchers.sequence() ) {
        if( launcher->group() == null_handle ) {
            launcher->export_as_text( Output );
        }
    }
}
