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
#include "world.h"
#include "globals.h"
#include "timer.h"
#include "logs.h"

TEvent::TEvent( std::string const &m ) :
                       asNodeName( m )
{
    if( false == m.empty() ) {
        // utworzenie niejawnego odczytu komórki pamięci w torze
        Type = tp_GetValues;
    }
    for( int i = 0; i < 13; ++i ) {
        Params[ i ].asPointer = nullptr;
    }
};

TEvent::~TEvent() {

    switch (Type)
    { // sprzątanie
    case tp_Multiple:
        if (iFlags & conditional_memstring) // o ile jest łańcuch do porównania w memcompare
            SafeDeleteArray(Params[10].asText);
        break;
    case tp_UpdateValues:
    case tp_AddValues:
        SafeDeleteArray(Params[0].asText);
        if (iFlags & conditional_memstring) // o ile jest łańcuch do porównania w memcompare
            SafeDeleteArray(Params[10].asText);
        break;
    case tp_Animation: // nic
        // SafeDeleteArray(Params[9].asText); //nie usuwać - nazwa jest zamieniana na wskaźnik do submodelu
        if (Params[0].asInt == 4) // jeśli z pliku VMD
            SafeDeleteArray( Params[8].asPointer ); // zwolnić obszar
        break;
    case tp_GetValues: // nic
        break;
	case tp_PutValues: // params[0].astext stores the token
		SafeDeleteArray( Params[ 0 ].asText );
		break;
    default:
        break;
    }
    evJoined = nullptr; // nie usuwać podczepionych tutaj

};

void TEvent::Conditions(cParser *parser, std::string s)
{ // przetwarzanie warunków, wspólne dla Multiple i UpdateValues
    if (s == "condition")
    { // jesli nie "endevent"
        std::string token, str;
        parser->getTokens();
        *parser >> token;
		str = token;
        if (str == "trackoccupied")
            iFlags |= conditional_trackoccupied;
        else if (str == "trackfree")
            iFlags |= conditional_trackfree;
        else if (str == "propability")
        {
            iFlags |= conditional_propability;
            parser->getTokens();
            *parser >> Params[10].asdouble;
        }
        else if (str == "memcompare")
        {
            iFlags |= conditional_memcompare;
            parser->getTokens(1, false); // case sensitive
            *parser >> token;
			str = token;
            if (str != "*") //"*" - nie brac command pod uwage
            { // zapamiętanie łańcucha do porównania
                Params[10].asText = new char[str.size() + 1];
                strcpy(Params[10].asText, str.c_str());
                iFlags |= conditional_memstring;
            }
            parser->getTokens();
            *parser >> token;
			str = token;
            if (str != "*") //"*" - nie brac val1 pod uwage
            {
                Params[11].asdouble = atof(str.c_str());
                iFlags |= conditional_memval1;
            }
            parser->getTokens();
            *parser >> token;
            str = token;
            if (str != "*") //"*" - nie brac val2 pod uwage
            {
                Params[12].asdouble = atof(str.c_str());
                iFlags |= conditional_memval2;
            }
        }
        parser->getTokens();
        *parser >> token;
        s = token; // ewentualnie dalej losowe opóźnienie
    }
    if (s == "randomdelay")
    { // losowe opóźnienie
        std::string token;
        parser->getTokens();
        *parser >> fRandomDelay; // Ra 2014-03-11
        parser->getTokens();
        *parser >> token; // endevent
    }
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
    // str = AnsiString(token.c_str());

    if (token == "exit")
        Type = tp_Exit;
    else if (token == "updatevalues")
        Type = tp_UpdateValues;
    else if (token == "getvalues")
        Type = tp_GetValues;
    else if (token == "putvalues")
        Type = tp_PutValues;
    else if (token == "disable")
        Type = tp_Disable;
    else if (token == "sound")
        Type = tp_Sound;
    else if (token == "velocity")
        Type = tp_Velocity;
    else if (token == "animation")
        Type = tp_Animation;
    else if (token == "lights")
        Type = tp_Lights;
    else if (token == "visible")
        Type = tp_Visible; // zmiana wyświetlania obiektu
    else if (token == "switch")
        Type = tp_Switch;
    else if (token == "dynvel")
        Type = tp_DynVel;
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

    if (token != "none")
        asNodeName = token; // nazwa obiektu powiązanego

    if (asName.substr(0, 5) == "none_")
        m_ignored = true; // Ra: takie są ignorowane

    switch (Type)
    {
    case tp_AddValues:
        iFlags = update_memadd; // dodawanko
    case tp_UpdateValues:
        // if (Type==tp_UpdateValues) iFlags=0; //co modyfikować
        parser->getTokens(1, false); // case sensitive
        *parser >> token;
        Params[0].asText = new char[token.size() + 1];
        strcpy(Params[0].asText, token.c_str());
        if (token != "*") // czy ma zostać bez zmian?
            iFlags |= update_memstring;
        parser->getTokens();
        *parser >> token;
        if (token != "*") // czy ma zostać bez zmian?
        {
            Params[1].asdouble = atof(token.c_str());
            iFlags |= update_memval1;
        }
        else
            Params[1].asdouble = 0;
        parser->getTokens();
        *parser >> token;
        if (token != "*") // czy ma zostać bez zmian?
        {
            Params[2].asdouble = atof(token.c_str());
            iFlags |= update_memval2;
        }
        else
            Params[2].asdouble = 0;
        parser->getTokens();
        *parser >> token;
        Conditions(parser, token); // sprawdzanie warunków
        break;
    case tp_CopyValues: {
        Params[9].asText = nullptr;
        iFlags = update_memstring | update_memval1 | update_memval2; // normalanie trzy
        int paramidx { 0 };
        parser->getTokens();
        *parser >> token; // nazwa drugiej komórki (źródłowej)
        while (token.compare("endevent") != 0) {

            switch (++paramidx)
            { // znaczenie kolejnych parametrów
            case 1: // nazwa drugiej komórki (źródłowej)
                Params[9].asText = new char[token.size() + 1]; // usuwane i zamieniane na wskaźnik
                strcpy(Params[9].asText, token.c_str());
                break;
            case 2: // maska wartości
                iFlags = stol_def(token, (update_memstring | update_memval1 | update_memval2));
                break;
            }
            parser->getTokens();
            *parser >> token;
        }
        break;
    }
    case tp_WhoIs: {
        iFlags = update_memstring | update_memval1 | update_memval2; // normalanie trzy
        int paramidx { 0 };
        parser->getTokens();
        *parser >> token; // nazwa drugiej komórki (źródłowej)
        while( token.compare( "endevent" ) != 0 ) {
            switch( ++paramidx ) { // znaczenie kolejnych parametrów
                case 1: // maska wartości
                    iFlags = stol_def( token, ( update_memstring | update_memval1 | update_memval2 ) );
                    break;
                default:
                    break;
            }
            parser->getTokens();
            *parser >> token;
        }
        break;
    }
    case tp_GetValues:
    case tp_LogValues:
        parser->getTokens(); //"endevent"
        *parser >> token;
        break;
    case tp_PutValues:
        parser->getTokens(3);
        *parser >> Params[3].asdouble >> Params[4].asdouble >> Params[5].asdouble; // położenie
        // X,Y,Z
        if ( !(org == Math3D::vector3()) )
        { // przesunięcie
            // tmp->pCenter.RotateY(aRotate.y/180.0*M_PI); //Ra 2014-11: uwzględnienie rotacji
            Params[3].asdouble += org.x; // współrzędne w scenerii
            Params[4].asdouble += org.y;
            Params[5].asdouble += org.z;
        }
        // Params[12].asInt=0;
        parser->getTokens(1, false); // komendy 'case sensitive'
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token.substr(0, 19) == "PassengerStopPoint:")
        {
            if (token.find('#') != std::string::npos)
				token.erase(token.find('#')); // obcięcie unikatowości
            win1250_to_ascii( token ); // get rid of non-ascii chars
            Params[6].asCommand = cm_PassengerStopPoint;
            // nie do kolejki (dla SetVelocity też, ale jak jest do toru dowiązany)
            bEnabled = false;
        }
        else if (token == "SetVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_SetVelocity;
        }
        else if (token == "RoadVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_RoadVelocity;
        }
        else if (token == "SectionVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_SectionVelocity;
        }
        else if (token == "ShuntVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_ShuntVelocity;
        }
        else if (token == "OutsideStation")
        {
            bEnabled = false; // ma być skanowny, aby AI nie przekraczało W5
            Params[6].asCommand = cm_OutsideStation;
        }
        else
            Params[6].asCommand = cm_Unknown;
        Params[0].asText = new char[token.size() + 1];
        strcpy(Params[0].asText, token.c_str());
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token == "none")
            Params[1].asdouble = 0.0;
        else
            try
            {
                Params[1].asdouble = atof(token.c_str());
            }
            catch (...)
            {
                Params[1].asdouble = 0.0;
                WriteLog("Error: number expected in PutValues event, found: " + token);
            }
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token == "none")
            Params[2].asdouble = 0.0;
        else
            try
            {
                Params[2].asdouble = atof(token.c_str());
            }
            catch (...)
            {
                Params[2].asdouble = 0.0;
                WriteLog("Error: number expected in PutValues event, found: " + token);
            }
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Lights: {
        int paramidx { 0 };
        do {
            parser->getTokens();
            *parser >> token;
            if( token.compare( "endevent" ) != 0 ) {

                if( paramidx < 8 ) {
                    Params[ paramidx ].asdouble = atof( token.c_str() ); // teraz może mieć ułamek
                    ++paramidx;
                }
                else {
                    ErrorLog( "Bad event: lights event \"" + asName + "\" with more than 8 parameters" );
                }
            }
        } while( token.compare( "endevent" ) != 0 );
        while( paramidx < 8 ) {
            // HACK: mark unspecified lights with magic value
            Params[ paramidx ].asdouble = -2.0;
            ++paramidx;
        }
        break;
    }
    case tp_Visible: // zmiana wyświetlania obiektu
        parser->getTokens();
        *parser >> token;
        Params[0].asInt = atoi(token.c_str());
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Velocity:
        parser->getTokens();
        *parser >> token;
        Params[0].asdouble = atof(token.c_str()) * ( 1000.0 / 3600.0 );
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Sound:
        parser->getTokens();
        *parser >> Params[0].asInt; // 0: wylaczyc, 1: wlaczyc; -1: wlaczyc zapetlone
        Params[1].asdouble = 0.0;
        parser->getTokens();
        if( parser->peek() != "endevent" ) {
            // optional parameter, radio channel to receive/broadcast the sound
            *parser >> Params[1].asdouble;
            parser->getTokens();
        }
        *parser >> token;
        break;
    case tp_Exit:
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Disable:
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Animation:
        parser->getTokens();
        *parser >> token;
        Params[0].asInt = 0; // na razie nieznany typ
        if (token.compare("rotate") == 0)
        { // obrót względem osi
            parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 1;
            parser->getTokens(4);
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        else if (token.compare("translate") == 0)
        { // przesuw o wektor
            parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 2;
            parser->getTokens(4);
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        else if (token.compare("digital") == 0)
        { // licznik cyfrowy
            parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 8;
            parser->getTokens(4); // jaki ma być sens tych parametrów?
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        else if (token.substr(token.length() - 4, 4) == ".vmd") // na razie tu, może będzie inaczej
        { // animacja z pliku VMD
			{
				std::ifstream file( szModelPath + token, std::ios::binary | std::ios::ate ); file.unsetf( std::ios::skipws );
				auto size = file.tellg();   // ios::ate already positioned us at the end of the file
				file.seekg( 0, std::ios::beg ); // rewind the caret afterwards
				Params[ 7 ].asInt = size;
				Params[8].asPointer = new char[size];
				file.read( static_cast<char*>(Params[ 8 ].asPointer), size ); // wczytanie pliku
			}
			parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 4; // rodzaj animacji
            parser->getTokens(4);
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Switch:
        parser->getTokens();
        *parser >> Params[0].asInt;
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token != "endevent")
        {
            Params[1].asdouble = atof(token.c_str()); // prędkość liniowa ruchu iglic
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        }
        else
            Params[1].asdouble = -1.0; // użyć domyślnej
        if (token != "endevent")
        {
            Params[2].asdouble =
                atof(token.c_str()); // dodatkowy ruch drugiej iglicy (zamknięcie nastawnicze)
            parser->getTokens();
            *parser >> token;
        }
        else
            Params[2].asdouble = -1.0; // użyć domyślnej
        break;
    case tp_DynVel:
        parser->getTokens();
        *parser >> Params[0].asdouble; // McZapkie-090302 *0.28;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_TrackVel:
        parser->getTokens();
        *parser >> Params[0].asdouble; // McZapkie-090302 *0.28;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Multiple: {
        bool conditionalelse { false }; // flaga dla else
        parser->getTokens();
        *parser >> token;

        while( ( token != "endevent" )
            && ( token != "condition" )
            && ( token != "randomdelay" ) ) {

            if( token != "else" ) {
                if( token.substr( 0, 5 ) != "none_" ) {
                    // eventy rozpoczynające się od "none_" są ignorowane
                    m_children.emplace_back( token, nullptr, ( conditionalelse == false ) );
                }
                else {
                    WriteLog( "Multi-event \"" + asName + "\" ignored link to event \"" + token + "\"" );
                }
            }
            else {
                // zmiana flagi dla słowa "else"
                conditionalelse = !conditionalelse;
                m_conditionalelse = true;
            }
            parser->getTokens();
            *parser >> token;
        }
        Conditions(parser, token); // sprawdzanie warunków
        break;
    }
    case tp_Voltage: // zmiana napięcia w zasilaczu (TractionPowerSource)
    case tp_Friction: // zmiana przyczepnosci na scenerii
        parser->getTokens();
        *parser >> Params[0].asdouble; // Ra 2014-01-27
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
//    case tp_Ignored: // ignorowany
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

// sends basic content of the class in legacy (text) format to provided stream
void
TEvent::export_as_text( std::ostream &Output ) const {

    if( Type == tp_Unknown ) { return; }

    // header
    Output << "event ";
    // name
    Output << asName << ' ';
    // type
    std::vector<std::string> const types {
        "unknown", "sound", "exit", "disable", "velocity", "animation", "lights",
        "updatevalues", "getvalues", "putvalues", "switch", "dynvel", "trackvel",
        "multiple", "addvalues", "copyvalues", "whois", "logvalues", "visible",
        "voltage", "message", "friction" };
    Output << types[ Type ] << ' ';
    // delay
    Output << fDelay << ' ';
    // associated node
    Output << ( asNodeName.empty() ? "none" : asNodeName ) << ' ';
    // type-specific attributes
    switch( Type ) {
        case tp_AddValues:
        case tp_UpdateValues: {
            Output
                << ( ( iFlags & update_memstring ) == 0 ? "*" :            Params[ 0 ].asText ) << ' '
                << ( ( iFlags & update_memval1 )   == 0 ? "*" : to_string( Params[ 1 ].asdouble ) ) << ' '
                << ( ( iFlags & update_memval2 )   == 0 ? "*" : to_string( Params[ 2 ].asdouble ) ) << ' ';
            break;
        }
        case tp_CopyValues: {
            // NOTE: there's no way to get the original parameter value if it doesn't point to existing memcell
            Output
                << ( Params[ 9 ].asMemCell == nullptr ? "none" : Params[ 9 ].asMemCell->name() ) << ' '
                << ( iFlags & ( update_memstring | update_memval1 | update_memval2 ) ) << ' ';
            break;
        }
        case tp_PutValues: {
            Output
                // location
                << Params[ 3 ].asdouble << ' '
                << Params[ 4 ].asdouble << ' '
                << Params[ 5 ].asdouble << ' '
                // command
                << Params[ 0 ].asText << ' '
                << Params[ 1 ].asdouble << ' '
                << Params[ 2 ].asdouble << ' ';
            break;
        }
        case tp_Multiple: {
            for( auto const &childevent : m_children ) {
                if( std::get<TEvent *>( childevent ) == nullptr ) { continue; }
                if( true == std::get<bool>( childevent ) ) {
                    Output << std::get<std::string>( childevent ) << ' ';
                }
            }
            // optional 'else' block
            if( true == m_conditionalelse ) {
                Output << "else ";
                for( auto const &childevent : m_children ) {
                    if( std::get<TEvent *>( childevent ) == nullptr ) { continue; }
                    if( false == std::get<bool>( childevent ) ) {
                        Output << std::get<std::string>( childevent ) << ' ';
                    }
                }
            }
            break;
        }
        case tp_Visible: {
            Output << Params[ 0 ].asInt << ' ';
            break;
        }
        case tp_Switch: {
            Output << Params[ 0 ].asInt << ' ';
            if( ( Params[ 1 ].asdouble != -1.0 )
            ||  ( Params[ 2 ].asdouble != -1.0 ) ) {
                Output << Params[ 1 ].asdouble << ' ';
            }
            if( Params[ 2 ].asdouble != -1.0 ) {
                Output << Params[ 2 ].asdouble << ' ';
            }
            break;
        }
        case tp_Lights: {
            auto lightidx { 0 };
            while( ( lightidx < iMaxNumLights )
                && ( Params[ lightidx ].asdouble > -2.0 ) ) {
                Output << Params[ lightidx ].asdouble << ' ';
                ++lightidx;
            }
            break;
        }
        case tp_Animation: {
            // animation type
            Output << (
                Params[ 0 ].asInt == 1 ? "rotate" :
                Params[ 0 ].asInt == 2 ? "translate" :
                // NOTE: .vmd animation doesn't preserve file name, can't be exported. TODO: fix this
                Params[ 0 ].asInt == 8 ? "digital" :
                "none" )
                << ' ';
            // submodel
            Output << (
                Params[ 9 ].asAnimContainer != nullptr ?
                    Params[ 9 ].asAnimContainer->NameGet() :
                    "none" ) << ' ';
            // animation parameters
            Output
                << Params[ 1 ].asdouble << ' '
                << Params[ 2 ].asdouble << ' '
                << Params[ 3 ].asdouble << ' '
                << Params[ 4 ].asdouble << ' ';
            break;
        }
        case tp_Sound: {
            // playback mode
            Output << Params[ 0 ].asInt << ' ';
            // optional radio channel
            if( Params[ 1 ].asdouble > 0.0 ) {
                Output << Params[ 1 ].asdouble << ' ';
            }
            break;
        }
        case tp_DynVel:
        case tp_TrackVel:
        case tp_Voltage:
        case tp_Friction: {
            Output << Params[ 0 ].asdouble << ' ';
            break;
        }
        case tp_Velocity: {
            Output << Params[ 0 ].asdouble * ( 3600.0 / 1000.0 ) << ' ';
            break;
        }
        case tp_WhoIs: {
            Output
                << ( iFlags & ( update_memstring | update_memval1 | update_memval2 ) ) << ' ';
            break;
        }
        default: {
            break;
        }
    }
    // optional conditions
    // NOTE: for flexibility condition check and export is performed for all event types rather than only for these which support it currently
    auto const isconditional { ( conditional_trackoccupied | conditional_trackfree | conditional_propability | conditional_memcompare ) };
    if( ( ( iFlags & isconditional ) != 0 ) ) {
        Output << "condition ";
        if( ( iFlags & conditional_trackoccupied ) != 0 ) {
            Output << "trackoccupied ";
        }
        else if( ( iFlags & conditional_trackfree ) != 0 ) {
            Output << "trackfree ";
        }
        else if( ( iFlags & conditional_propability ) != 0 ) {
            Output
                << "propability "
                << Params[ 10 ].asdouble << ' ';
        }
        else if( ( iFlags & conditional_memcompare ) != 0 ) {
            Output
                << "memcompare "
                << ( ( iFlags & conditional_memstring ) == 0 ? "*" :            Params[ 10 ].asText ) << ' '
                << ( ( iFlags & conditional_memval1 )   == 0 ? "*" : to_string( Params[ 11 ].asdouble ) ) << ' '
                << ( ( iFlags & conditional_memval2 )   == 0 ? "*" : to_string( Params[ 12 ].asdouble ) ) << ' ';
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

std::string TEvent::CommandGet()
{ // odczytanie komendy z eventu
    switch (Type)
    { // to się wykonuje również składu jadącego bez obsługi
    case tp_GetValues:
        return std::string(Params[9].asMemCell->Text());
    case tp_PutValues:
        return std::string(Params[0].asText);
    }
    return ""; // inne eventy się nie liczą
};

TCommandType TEvent::Command()
{ // odczytanie komendy z eventu
    switch (Type)
    { // to się wykonuje również dla składu jadącego bez obsługi
    case tp_GetValues:
        return Params[9].asMemCell->Command();
    case tp_PutValues:
        return Params[6].asCommand; // komenda zakodowana binarnie
    }
    return cm_Unknown; // inne eventy się nie liczą
};

double TEvent::ValueGet(int n)
{ // odczytanie komendy z eventu
    n &= 1; // tylko 1 albo 2 jest prawidłowy
    switch (Type)
    { // to się wykonuje również składu jadącego bez obsługi
    case tp_GetValues:
        return n ? Params[9].asMemCell->Value1() : Params[9].asMemCell->Value2();
    case tp_PutValues:
        return Params[2 - n].asdouble;
    }
    return 0.0; // inne eventy się nie liczą
};

glm::dvec3 TEvent::PositionGet() const
{ // pobranie współrzędnych eventu
    switch (Type)
    { //
    case tp_GetValues:
        return Params[9].asMemCell->location(); // współrzędne podłączonej komórki pamięci
    case tp_PutValues:
        return glm::dvec3(Params[3].asdouble, Params[4].asdouble, Params[5].asdouble);
    }
    return glm::dvec3(0, 0, 0); // inne eventy się nie liczą
};

bool TEvent::StopCommand()
{ //
    if (Type == tp_GetValues)
        return Params[9].asMemCell->StopCommand(); // info o komendzie z komórki
    return false;
};

void TEvent::StopCommandSent()
{
    if (Type == tp_GetValues)
        Params[9].asMemCell->StopCommandSent(); // komenda z komórki została wysłana
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

    if( false == Event->bEnabled ) { return false; }
    if( Event->iQueued != 0 )      { return false; }
    // jeśli może być dodany do kolejki (nie używany w skanowaniu)
    // jeśli nie dodany jeszcze do kolejki
        
    // kolejka eventów jest posortowana względem (fStartTime)
    Event->Activator = Owner;
    if( ( Event->Type == tp_AddValues )
     && ( Event->fDelay == 0.0 ) ) {
        // eventy AddValues trzeba wykonywać natychmiastowo, inaczej kolejka może zgubić jakieś dodawanie
        // Ra: kopiowanie wykonania tu jest bez sensu, lepiej by było wydzielić funkcję
        // wykonującą eventy i ją wywołać
        if( ( false == Event->m_ignored )
         && ( true == EventConditon( Event ) ) ) { // teraz mogą być warunki do tych eventów

            Event->Params[ 5 ].asMemCell->UpdateValues(
                Event->Params[ 0 ].asText, Event->Params[ 1 ].asdouble,
                Event->Params[ 2 ].asdouble, Event->iFlags );

            if( Event->Params[ 6 ].asTrack ) {
                // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
                for( auto dynamic : Event->Params[ 6 ].asTrack->Dynamics ) {
                    Event->Params[ 5 ].asMemCell->PutCommand(
                        dynamic->Mechanik,
                        Event->Params[ 4 ].asLocation );
                }
                //if (DebugModeFlag)
                WriteLog(
                    "EVENT EXECUTED" + ( Owner ? ( " by " + Owner->asName ) : "" ) + ": AddValues & Track command - ["
                    + std::string{ Event->Params[ 0 ].asText } + "] ["
                    + to_string( Event->Params[ 1 ].asdouble, 2 ) + "] ["
                    + to_string( Event->Params[ 2 ].asdouble, 2 ) + " ]" );
            }
            //else if (DebugModeFlag)
            WriteLog(
                "EVENT EXECUTED" + ( Owner ? ( " by " + Owner->asName ) : "" ) + ": AddValues - ["
                + std::string( Event->Params[ 0 ].asText ) + "] ["
                + to_string( Event->Params[ 1 ].asdouble, 2 ) + "] ["
                + to_string( Event->Params[ 2 ].asdouble, 2 ) + "]" );
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

    TLocation loc;
    int i;
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
            WriteLog( "EVENT LAUNCHED" + ( m_workevent->Activator ? ( " by " + m_workevent->Activator->asName ) : "" ) + ": " + m_workevent->asName );
            switch (m_workevent->Type)
            {
            case tp_CopyValues: // skopiowanie wartości z innej komórki
                m_workevent->Params[5].asMemCell->UpdateValues(
                    m_workevent->Params[9].asMemCell->Text(),
                    m_workevent->Params[9].asMemCell->Value1(),
                    m_workevent->Params[9].asMemCell->Value2(),
                    m_workevent->iFlags // flagi określają, co ma być skopiowane
                    );
            // break; //żeby się wysłało do torów i nie było potrzeby na AddValues * 0 0
            case tp_AddValues: // różni się jedną flagą od UpdateValues
            case tp_UpdateValues:
                if (EventConditon(m_workevent))
                { // teraz mogą być warunki do tych eventów
                    if (m_workevent->Type != tp_CopyValues) // dla CopyValues zrobiło się wcześniej
                        m_workevent->Params[5].asMemCell->UpdateValues(
                            m_workevent->Params[0].asText,
                            m_workevent->Params[1].asdouble,
                            m_workevent->Params[2].asdouble,
                            m_workevent->iFlags);
                    if (m_workevent->Params[6].asTrack)
                    { // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla
                        // wszystkich 'dynamic' na danym torze
                        for( auto dynamic : m_workevent->Params[ 6 ].asTrack->Dynamics ) {
                            m_workevent->Params[ 5 ].asMemCell->PutCommand(
                                dynamic->Mechanik,
                                m_workevent->Params[ 4 ].asLocation );
                        }
                        //if (DebugModeFlag)
                        WriteLog("Type: UpdateValues & Track command - [" +
                            m_workevent->Params[5].asMemCell->Text() + "] [" +
                            to_string( m_workevent->Params[ 5 ].asMemCell->Value1(), 2 ) + "] [" +
                            to_string( m_workevent->Params[ 5 ].asMemCell->Value2(), 2 ) + "]" );
                    }
                    else //if (DebugModeFlag) 
                        WriteLog("Type: UpdateValues - [" +
                            m_workevent->Params[5].asMemCell->Text() + "] [" +
                            to_string( m_workevent->Params[ 5 ].asMemCell->Value1(), 2 ) + "] [" +
                            to_string( m_workevent->Params[ 5 ].asMemCell->Value2(), 2 ) + "]" );
                }
                break;
            case tp_GetValues: {
                if( m_workevent->Activator ) {
                    // TODO: re-enable when messaging module is in place
                    if( Global.iMultiplayer ) {
                        // potwierdzenie wykonania dla serwera (odczyt semafora już tak nie działa)
                        multiplayer::WyslijEvent( m_workevent->asName, m_workevent->Activator->name() );
                    }
                    m_workevent->Params[ 9 ].asMemCell->PutCommand(
                        m_workevent->Activator->Mechanik,
                        m_workevent->Params[ 8 ].asLocation );
                }
                WriteLog( "Type: GetValues" );
                break;
            }
            case tp_PutValues: {
                if (m_workevent->Activator) {
                    // zamiana, bo fizyka ma inaczej niż sceneria
                    loc.X = -m_workevent->Params[3].asdouble;
                    loc.Y =  m_workevent->Params[5].asdouble;
                    loc.Z =  m_workevent->Params[4].asdouble;
                    if( m_workevent->Activator->Mechanik ) {
                        // przekazanie rozkazu do AI
                        m_workevent->Activator->Mechanik->PutCommand(
                            m_workevent->Params[0].asText, m_workevent->Params[1].asdouble,
                            m_workevent->Params[2].asdouble, loc);
                    }
                    else {
                        // przekazanie do pojazdu
                        m_workevent->Activator->MoverParameters->PutCommand(
                            m_workevent->Params[0].asText, m_workevent->Params[1].asdouble,
                            m_workevent->Params[2].asdouble, loc);
                    }
                    WriteLog("Type: PutValues - [" +
                        std::string(m_workevent->Params[0].asText) + "] [" +
                        to_string( m_workevent->Params[ 1 ].asdouble, 2 ) + "] [" +
                        to_string( m_workevent->Params[ 2 ].asdouble, 2 ) + "]" );
                }
                break;
            }
            case tp_Lights: {
                if( m_workevent->Params[ 9 ].asModel ) {
                    for( i = 0; i < iMaxNumLights; ++i ) {
                        if( m_workevent->Params[ i ].asdouble == -2.0 ) {
                            // processed all supplied values, bail out
                            break;
                        }
                        if( m_workevent->Params[ i ].asdouble >= 0 ) {
                            // -1 zostawia bez zmiany
                            m_workevent->Params[ 9 ].asModel->LightSet(
                                i,
                                m_workevent->Params[ i ].asdouble );
                        }
                    }
                }
                break;
            }
            case tp_Visible: {
                if( m_workevent->Params[ 9 ].asSceneNode )
                    m_workevent->Params[ 9 ].asSceneNode->visible( m_workevent->Params[ 0 ].asInt > 0 );
                break;
            }
            case tp_Velocity: {
                Error( "Not implemented yet :(" );
                break;
            }
            case tp_Exit: {
                // wyłączenie takie samo jak sekwencja F10 -> Y
                MessageBox(
                    0,
                    ExchangeCharInString( m_workevent->asNodeName, '_', ' ' ).c_str(),
                    " THE END ",
                    MB_OK );
                return false;
            }
            case tp_Sound: {
                if( m_workevent->Params[ 9 ].tsTextSound == nullptr ) {
                    break;
                }
                switch( m_workevent->Params[ 0 ].asInt ) {
                    // trzy możliwe przypadki:
                    case 0: {
                        m_workevent->Params[ 9 ].tsTextSound->stop();
                        break;
                    }
                    case 1: {
                        if( m_workevent->Params[ 1 ].asdouble > 0.0 ) {
                            Global.pWorld->radio_message(
                                m_workevent->Params[ 9 ].tsTextSound,
                                static_cast<int>( m_workevent->Params[ 1 ].asdouble ) );
                        }
                        else {
                            m_workevent->Params[ 9 ].tsTextSound->play( sound_flags::exclusive | sound_flags::event );
                        }
                        break;
                    }
                    case -1: {
                        m_workevent->Params[ 9 ].tsTextSound->play( sound_flags::exclusive | sound_flags::looping | sound_flags::event );
                        break;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            case tp_Disable:
                Error("Not implemented yet :(");
                break;
            case tp_Animation: {
                switch( m_workevent->Params[ 0 ].asInt ) {
                    case 1: {
                        m_workevent->Params[ 9 ].asAnimContainer->SetRotateAnim(
                            Math3D::vector3 {
                                m_workevent->Params[ 1 ].asdouble,
                                m_workevent->Params[ 2 ].asdouble,
                                m_workevent->Params[ 3 ].asdouble },
                            m_workevent->Params[ 4 ].asdouble );
                        break;
                    }
                    case 2: {
                        m_workevent->Params[ 9 ].asAnimContainer->SetTranslateAnim(
                            Math3D::vector3 {
                                m_workevent->Params[ 1 ].asdouble,
                                m_workevent->Params[ 2 ].asdouble,
                                m_workevent->Params[ 3 ].asdouble },
                            m_workevent->Params[ 4 ].asdouble );
                        break;
                    }
                    case 4: {
                        m_workevent->Params[ 9 ].asModel->AnimationVND(
                            m_workevent->Params[ 8 ].asPointer,
                            m_workevent->Params[ 1 ].asdouble, // tu mogą być dodatkowe parametry, np. od-do
                            m_workevent->Params[ 2 ].asdouble,
                            m_workevent->Params[ 3 ].asdouble,
                            m_workevent->Params[ 4 ].asdouble );
                        break;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            case tp_Switch: {
                if( m_workevent->Params[ 9 ].asTrack ) {
                    m_workevent->Params[ 9 ].asTrack->Switch(
                        m_workevent->Params[ 0 ].asInt,
                        m_workevent->Params[ 1 ].asdouble,
                        m_workevent->Params[ 2 ].asdouble );
                }
                if( Global.iMultiplayer ) {
                    // dajemy znać do serwera o przełożeniu
                    multiplayer::WyslijEvent( m_workevent->asName, "" ); // wysłanie nazwy eventu przełączajacego
                }
                // Ra: bardziej by się przydała nazwa toru, ale nie ma do niej stąd dostępu
                break;
            }
            case tp_TrackVel:
                if (m_workevent->Params[9].asTrack) {
                    // prędkość na zwrotnicy może być ograniczona z góry we wpisie, większej się nie ustawi eventem
                    m_workevent->Params[9].asTrack->VelocitySet(m_workevent->Params[0].asdouble);
                    // wyświetlana jest ta faktycznie ustawiona
                    WriteLog( "Type: TrackVel - ["
                        + to_string( m_workevent->Params[ 0 ].asdouble, 2 ) + "]"
                        + ( DebugModeFlag ?
                            ", actual [ " + to_string( m_workevent->Params[ 9 ].asTrack->VelocityGet(), 2 ) + "]" :
                            "" ) );
                }
                break;
            case tp_DynVel:
                Error("Event \"DynVel\" is obsolete");
                break;
            case tp_Multiple: {
                auto const condition { EventConditon( m_workevent ) };
                if( ( true == condition )
                 || ( true == m_workevent->m_conditionalelse ) ) {
                    // warunek spelniony albo było użyte else
                    WriteLog("Type: Multi-event");
                    for( auto &childevent : m_workevent->m_children ) {
                        auto *childeventdata { std::get<TEvent*>( childevent ) };
                        if( childeventdata == nullptr ) { continue; }
                        if( std::get<bool>( childevent ) != condition ) { continue; }

                        if( childeventdata != m_workevent ) {
                            // normalnie dodać
                            AddToQuery( childeventdata, m_workevent->Activator );
                        }
                        else {
                            // jeśli ma być rekurencja to musi mieć sensowny okres powtarzania
                            if( m_workevent->fDelay >= 5.0 ) {
                                AddToQuery( m_workevent, m_workevent->Activator );
                            }
                        }
                    }
                    if( Global.iMultiplayer ) {
                        // dajemy znać do serwera o wykonaniu
                        if( false == m_workevent->m_conditionalelse ) {
                            // jednoznaczne tylko, gdy nie było else
                            if( m_workevent->Activator ) {
                                multiplayer::WyslijEvent( m_workevent->asName, m_workevent->Activator->name() );
                            }
                            else {
                                multiplayer::WyslijEvent( m_workevent->asName, "" );
                            }
                        }
                    }
                }
                break;
            }
            case tp_WhoIs: {
                // pobranie nazwy pociągu do komórki pamięci
                if (m_workevent->iFlags & update_load) {
                    // jeśli pytanie o ładunek
                    if( m_workevent->iFlags & update_memadd ) {
                        // jeśli typ pojazdu
                        // TODO: define and recognize individual request types
                        auto const owner = (
                            ( ( m_workevent->Activator->Mechanik != nullptr ) && ( m_workevent->Activator->Mechanik->Primary() ) ) ?
                                m_workevent->Activator->Mechanik :
                                m_workevent->Activator->ctOwner );
                        auto const consistbrakelevel = (
                            owner != nullptr ?
                                owner->fReady :
                                -1.0 );
                        auto const collisiondistance = (
                            owner != nullptr ?
                                owner->TrackBlock() :
                                -1.0 );

                        m_workevent->Params[ 9 ].asMemCell->UpdateValues(
                            m_workevent->Activator->MoverParameters->TypeName, // typ pojazdu
                            consistbrakelevel,
                            collisiondistance,
                            m_workevent->iFlags & ( update_memstring | update_memval1 | update_memval2 ) );

                        WriteLog(
                              "Type: WhoIs (" + to_string( m_workevent->iFlags ) + ") - "
                            + "[name: " + m_workevent->Activator->MoverParameters->TypeName + "], "
                            + "[consist brake level: " + to_string( consistbrakelevel, 2 ) + "], "
                            + "[obstacle distance: " + to_string( collisiondistance, 2 ) + " m]" );
                    }
                    else {
                        // jeśli parametry ładunku
                        m_workevent->Params[ 9 ].asMemCell->UpdateValues(
                            m_workevent->Activator->MoverParameters->LoadType, // nazwa ładunku
                            m_workevent->Activator->MoverParameters->Load, // aktualna ilość
                            m_workevent->Activator->MoverParameters->MaxLoad, // maksymalna ilość
                            m_workevent->iFlags & ( update_memstring | update_memval1 | update_memval2 ) );

                        WriteLog(
                              "Type: WhoIs (" + to_string( m_workevent->iFlags ) + ") - "
                            + "[load type: " + m_workevent->Activator->MoverParameters->LoadType + "], "
                            + "[current load: " + to_string( m_workevent->Activator->MoverParameters->Load, 2 ) + "], "
                            + "[max load: " + to_string( m_workevent->Activator->MoverParameters->MaxLoad, 2 ) + "]" );
                    }
                }
                else if (m_workevent->iFlags & update_memadd)
                { // jeśli miejsce docelowe pojazdu
                    m_workevent->Params[ 9 ].asMemCell->UpdateValues(
                        m_workevent->Activator->asDestination, // adres docelowy
                        m_workevent->Activator->DirectionGet(), // kierunek pojazdu względem czoła składu (1=zgodny,-1=przeciwny)
                        m_workevent->Activator->MoverParameters->Power, // moc pojazdu silnikowego: 0 dla wagonu
                        m_workevent->iFlags & (update_memstring | update_memval1 | update_memval2));

                        WriteLog(
                              "Type: WhoIs (" + to_string( m_workevent->iFlags ) + ") - "
                            + "[destination: " + m_workevent->Activator->asDestination + "], "
                            + "[direction: " + to_string( m_workevent->Activator->DirectionGet() ) + "], "
                            + "[engine power: " + to_string( m_workevent->Activator->MoverParameters->Power, 2 ) + "]" );
                }
                else if (m_workevent->Activator->Mechanik)
                    if (m_workevent->Activator->Mechanik->Primary())
                    { // tylko jeśli ktoś tam siedzi - nie powinno dotyczyć pasażera!
                        m_workevent->Params[ 9 ].asMemCell->UpdateValues(
							m_workevent->Activator->Mechanik->TrainName(),
                            m_workevent->Activator->Mechanik->StationCount() - m_workevent->Activator->Mechanik->StationIndex(), // ile przystanków do końca
                            m_workevent->Activator->Mechanik->IsStop() ?
                                1 :
                                0, // 1, gdy ma tu zatrzymanie
                            m_workevent->iFlags);
                        WriteLog("Train detected: " + m_workevent->Activator->Mechanik->TrainName());
                    }
                break;
            }
            case tp_LogValues: {
                // zapisanie zawartości komórki pamięci do logu
                if( m_workevent->Params[ 9 ].asMemCell ) {
                    // jeśli była podana nazwa komórki
                    WriteLog( "Memcell \"" + m_workevent->asNodeName + "\": ["
                        + m_workevent->Params[ 9 ].asMemCell->Text() + "] ["
                        + to_string( m_workevent->Params[ 9 ].asMemCell->Value1(), 2 ) + "] ["
                        + to_string( m_workevent->Params[ 9 ].asMemCell->Value2(), 2 ) + "]" );
                }
                else {
                    // lista wszystkich
                    simulation::Memory.log_all();
                }
                break;
            }
            case tp_Voltage: // zmiana napięcia w zasilaczu (TractionPowerSource)
                if (m_workevent->Params[9].psPower)
                { // na razie takie chamskie ustawienie napięcia zasilania
                    WriteLog("Type: Voltage");
                    m_workevent->Params[9].psPower->VoltageSet(m_workevent->Params[0].asdouble);
                }
            case tp_Friction: // zmiana tarcia na scenerii
            { // na razie takie chamskie ustawienie napięcia zasilania
                WriteLog("Type: Friction");
                Global.fFriction = (m_workevent->Params[0].asdouble);
            }
            break;
            case tp_Message: // wyświetlenie komunikatu
                break;
            } // switch (tmpEvent->Type)
        } // if (tmpEvent->bEnabled)
    } // while
    return true;
}

// legacy method, initializes events after deserialization from scenario file
void
event_manager::InitEvents() {

    //łączenie eventów z pozostałymi obiektami
    for( auto *event : m_events ) {

        switch( event->Type ) {

        case tp_AddValues: // sumowanie wartości
        case tp_UpdateValues: { // zmiana wartości
            auto *cell = simulation::Memory.find( event->asNodeName ); // nazwa komórki powiązanej z eventem
            if( cell != nullptr ) { // McZapkie-100302
                if( event->iFlags & ( conditional_trackoccupied | conditional_trackfree ) ) {
                    // jeśli chodzi o zajetosc toru (tor może być inny, niż wpisany w komórce)
                    // nazwa toru ta sama, co nazwa komórki
                    event->Params[ 9 ].asTrack = simulation::Paths.find( event->asNodeName );
                    if( event->Params[ 9 ].asTrack == nullptr ) {
                        ErrorLog( "Bad event: track \"" + event->asNodeName + "\" referenced in event \"" + event->asName + "\" doesn't exist" );
                    }
                }
                event->Params[ 4 ].asLocation = &( cell->location() );
                event->Params[ 5 ].asMemCell = cell; // komórka do aktualizacji
                if( event->iFlags & ( conditional_memcompare ) ) {
                    // komórka do badania warunku
                    event->Params[ 9 ].asMemCell = cell;
                }
                if( false == cell->asTrackName.empty() ) {
                    // tor powiązany z komórką powiązaną z eventem
                    // tu potrzebujemy wskaźnik do komórki w (tmp)
                    event->Params[ 6 ].asTrack = simulation::Paths.find( cell->asTrackName );
                    if( event->Params[ 6 ].asTrack == nullptr ) {
                        ErrorLog( "Bad memcell: track \"" + cell->asTrackName + "\" referenced in memcell \"" + cell->name() + "\" doesn't exist" );
                    }
                }
                else {
                    event->Params[ 6 ].asTrack = nullptr;
                }
            }
            else {
                // nie ma komórki, to nie będzie działał poprawnie
                event->m_ignored = true; // deaktywacja
                ErrorLog( "Bad event: event \"" + event->asName + "\" cannot find memcell \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_LogValues: {
            // skojarzenie z memcell
            if( event->asNodeName.empty() ) { // brak skojarzenia daje logowanie wszystkich
                event->Params[ 9 ].asMemCell = nullptr;
                break;
            }
        }
        case tp_GetValues:
        case tp_WhoIs: {
            auto *cell = simulation::Memory.find( event->asNodeName );
            if( cell != nullptr ) {
                event->Params[ 8 ].asLocation = &( cell->location() );
                event->Params[ 9 ].asMemCell = cell;
                if( ( event->Type == tp_GetValues )
                 && ( cell->IsVelocity() ) ) {
                    // jeśli odczyt komórki a komórka zawiera komendę SetVelocity albo ShuntVelocity
                    // to event nie będzie dodawany do kolejki
                    event->bEnabled = false;
                }
            }
            else {
                // nie ma komórki, to nie będzie działał poprawnie
                event->m_ignored = true; // deaktywacja
                ErrorLog( "Bad event: event \"" + event->asName + "\" cannot find memcell \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_CopyValues: {
            // skopiowanie komórki do innej
            auto *cell = simulation::Memory.find( event->asNodeName ); // komórka docelowa
            if( cell != nullptr ) {
                event->Params[ 4 ].asLocation = &( cell->location() );
                event->Params[ 5 ].asMemCell = cell; // komórka docelowa
                if( false == cell->asTrackName.empty() ) {
                    // tor powiązany z komórką powiązaną z eventem
                    // tu potrzebujemy wskaźnik do komórki w (tmp)
                    event->Params[ 6 ].asTrack = simulation::Paths.find( cell->asTrackName );
                    if( event->Params[ 6 ].asTrack == nullptr ) {
                        ErrorLog( "Bad memcell: track \"" + cell->asTrackName + "\" referenced in memcell \"" + cell->name() + "\" doesn't exists" );
                    }
                }
                else {
                    event->Params[ 6 ].asTrack = nullptr;
                }
            }
            else {
                ErrorLog( "Bad event: copyvalues event \"" + event->asName + "\" cannot find memcell \"" + event->asNodeName + "\"" );
            }
            std::string const cellastext { event->Params[ 9 ].asText };
            cell = simulation::Memory.find( event->Params[ 9 ].asText ); // komórka źódłowa
            SafeDeleteArray( event->Params[ 9 ].asText ); // usunięcie nazwy komórki
            if( cell != nullptr ) {
                event->Params[ 8 ].asLocation = &( cell->location() );
                event->Params[ 9 ].asMemCell = cell; // komórka źródłowa
            }
            else {
                ErrorLog( "Bad event: copyvalues event \"" + event->asName + "\" cannot find memcell \"" + cellastext + "\"" );
            }
            break;
        }
        case tp_Animation: {
            // animacja modelu
            // retrieve target name parameter
            std::string const cellastext = event->Params[ 9 ].asText;
            SafeDeleteArray( event->Params[ 9 ].asText );
            // egzemplarz modelu do animowania
            auto *instance = simulation::Instances.find( event->asNodeName );
            if( instance != nullptr ) {
                if( event->Params[ 0 ].asInt == 4 ) {
                    // model dla całomodelowych animacji
                    event->Params[ 9 ].asModel = instance;
                }
                else {
                    // standardowo przypisanie submodelu
                    event->Params[ 9 ].asAnimContainer = instance->GetContainer( cellastext ); // submodel
                    if( event->Params[ 9 ].asAnimContainer ) {
                        event->Params[ 9 ].asAnimContainer->WillBeAnimated(); // oflagowanie animacji
                        if( event->Params[ 9 ].asAnimContainer->Event() == nullptr ) {
                            // nie szukać, gdy znaleziony
                            event->Params[ 9 ].asAnimContainer->EventAssign(
                                FindEvent( event->asNodeName + "." + cellastext + ":done" ) );
                        }
                    }
                }
            }
            else {
                ErrorLog( "Bad event: animation event \"" + event->asName + "\" cannot find model instance \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_Lights: {
            // zmiana świeteł modelu
            auto *instance = simulation::Instances.find( event->asNodeName );
            if( instance != nullptr )
                event->Params[ 9 ].asModel = instance;
            else
                ErrorLog( "Bad event: lights event \"" + event->asName + "\" cannot find model instance \"" + event->asNodeName + "\"" );
            break;
        }
        case tp_Visible: {
            // ukrycie albo przywrócenie obiektu
            scene::basic_node *node = simulation::Instances.find( event->asNodeName ); // najpierw model
            if( node == nullptr ) {
                // albo tory?
                node = simulation::Paths.find( event->asNodeName );
            }
            if( node == nullptr ) {
                // może druty?
                node = simulation::Traction.find( event->asNodeName );
            }
            if( node != nullptr )
                event->Params[ 9 ].asSceneNode = node;
            else {
                event->m_ignored = true;
                ErrorLog( "Bad event: visibility event \"" + event->asName + "\" cannot find item \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_Switch: {
            // przełożenie zwrotnicy albo zmiana stanu obrotnicy
            auto *track = simulation::Paths.find( event->asNodeName );
            if( track != nullptr ) {
                // dowiązanie toru
                if( track->iAction == 0 ) {
                    // jeśli nie jest zwrotnicą ani obrotnicą to będzie się zmieniał stan uszkodzenia
                    track->iAction |= 0x100;
                }
                event->Params[ 9 ].asTrack = track;
                if( ( event->Params[ 0 ].asInt == 0 )
                 && ( event->Params[ 2 ].asdouble >= 0.0 ) ) {
                    // jeśli przełącza do stanu 0 & jeśli jest zdefiniowany dodatkowy ruch iglic
                    // przesłanie parametrów
                    event->Params[ 9 ].asTrack->Switch(
                        event->Params[ 0 ].asInt,
                        event->Params[ 1 ].asdouble,
                        event->Params[ 2 ].asdouble );
                }
            }
            else {
                ErrorLog( "Bad event: switch event \"" + event->asName + "\" cannot find track \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_Sound: {
            // odtworzenie dźwięku
            auto *sound = simulation::Sounds.find( event->asNodeName );
            if( sound != nullptr )
                event->Params[ 9 ].tsTextSound = sound;
            else
                ErrorLog( "Bad event: sound event \"" + event->asName + "\" cannot find static sound \"" + event->asNodeName + "\"" );
            break;
        }
        case tp_TrackVel: {
            // ustawienie prędkości na torze
            if( false == event->asNodeName.empty() ) {
                auto *track = simulation::Paths.find( event->asNodeName );
                if( track != nullptr ) {
                    // flaga zmiany prędkości toru jest istotna dla skanowania
                    track->iAction |= 0x200;
                    event->Params[ 9 ].asTrack = track;
                }
                else {
                    ErrorLog( "Bad event: track velocity event \"" + event->asName + "\" cannot find track \"" + event->asNodeName + "\"" );
                }
            }
            break;
        }
        case tp_DynVel: {
            // komunikacja z pojazdem o konkretnej nazwie
            if( event->asNodeName == "activator" )
                event->Params[ 9 ].asDynamic = nullptr;
            else {
                auto *vehicle = simulation::Vehicles.find( event->asNodeName );
                if( vehicle != nullptr )
                    event->Params[ 9 ].asDynamic = vehicle;
                else
                    ErrorLog( "Bad event: vehicle velocity event \"" + event->asName + "\" cannot find vehicle \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_Multiple: {
            if( false == event->asNodeName.empty() ) {
                if( event->iFlags & ( conditional_trackoccupied | conditional_trackfree ) ) {
                    // jeśli chodzi o zajetosc toru
                    event->Params[ 9 ].asTrack = simulation::Paths.find( event->asNodeName );
                    if( event->Params[ 9 ].asTrack == nullptr ) {
                        ErrorLog( "Bad event: multi-event \"" + event->asName + "\" cannot find track \"" + event->asNodeName + "\"" );
                        event->iFlags &= ~( conditional_trackoccupied | conditional_trackfree ); // zerowanie flag
                    }
                }
                else if( event->iFlags & ( conditional_memstring | conditional_memval1 | conditional_memval2 ) ) {
                    // jeśli chodzi o komorke pamieciową
                    event->Params[ 9 ].asMemCell = simulation::Memory.find( event->asNodeName );
                    if( event->Params[ 9 ].asMemCell == nullptr ) {
                        ErrorLog( "Bad event: multi-event \"" + event->asName + "\" cannot find memory cell \"" + event->asNodeName + "\"" );
                        event->iFlags &= ~( conditional_memstring | conditional_memval1 | conditional_memval2 );
                    }
                }
            }
            for( auto &childevent : event->m_children ) {
                std::get<TEvent *>( childevent ) = FindEvent( std::get<std::string>( childevent ) );
                if( std::get<TEvent *>( childevent ) == nullptr ) {
                    ErrorLog( "Bad event: multi-event \"" + event->asName + "\" cannot find event \"" + std::get<std::string>( childevent ) + "\"" );
                }
            }
            break;
        }
        case tp_Voltage: {
            // zmiana napięcia w zasilaczu (TractionPowerSource)
            if( false == event->asNodeName.empty() ) {
                auto *powersource = simulation::Powergrid.find( event->asNodeName ); // podłączenie zasilacza
                if( powersource != nullptr )
                    event->Params[ 9 ].psPower = powersource;
                else
                    ErrorLog( "Bad event: voltage event \"" + event->asName + "\" cannot find power source \"" + event->asNodeName + "\"" );
            }
            break;
        }
        case tp_Message: {
            // wyświetlenie komunikatu
            break;
        }

        } // switch

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
        event->export_as_text( Output );
    }
    Output << "// event launchers\n";
    for( auto const *launcher : m_launchers.sequence() ) {
        launcher->export_as_text( Output );
    }
}

// legacy method, verifies condition for specified event
bool
event_manager::EventConditon( TEvent *Event ) {

    if (Event->iFlags <= update_only)
        return true; // bezwarunkowo

    if (Event->iFlags & conditional_trackoccupied)
        return (!Event->Params[9].asTrack->IsEmpty());
    else if (Event->iFlags & conditional_trackfree)
        return (Event->Params[9].asTrack->IsEmpty());
    else if (Event->iFlags & conditional_propability)
    {
        double rprobability = Random();
        WriteLog( "Random integer: " + std::to_string( rprobability ) + " / " + std::to_string( Event->Params[ 10 ].asdouble ) );
        return (Event->Params[10].asdouble > rprobability);
    }
    else if( Event->iFlags & conditional_memcompare ) {
        // porównanie wartości
        if( nullptr == Event->Params[9].asMemCell ) {

            ErrorLog( "Event " + Event->asName + " trying conditional_memcompare with nonexistent memcell" );
            return true; // though this is technically error, we report success to maintain backward compatibility
        }
        auto const comparisonresult =
            m_workevent->Params[ 9 ].asMemCell->Compare(
                ( Event->Params[ 10 ].asText != nullptr ?
                    Event->Params[ 10 ].asText :
                    "" ),
                Event->Params[ 11 ].asdouble,
                Event->Params[ 12 ].asdouble,
                Event->iFlags );

        std::string comparisonlog = "Type: MemCompare - ";

        comparisonlog +=
               "[" + Event->Params[ 9 ].asMemCell->Text() + "]"
            + " [" + to_string( Event->Params[ 9 ].asMemCell->Value1(), 2 ) + "]"
            + " [" + to_string( m_workevent->Params[ 9 ].asMemCell->Value2(), 2 ) + "]";

        comparisonlog += (
            true == comparisonresult ?
                " == " :
                " != " );

        comparisonlog += (
            TestFlag( Event->iFlags, conditional_memstring ) ?
                "[" + std::string( m_workevent->Params[ 10 ].asText ) + "]" :
                "[*]" );
        comparisonlog += (
            TestFlag( m_workevent->iFlags, conditional_memval1 ) ?
                " [" + to_string( m_workevent->Params[ 11 ].asdouble, 2 ) + "]" :
                " [*]" );
        comparisonlog += (
            TestFlag( m_workevent->iFlags, conditional_memval2 ) ?
                " [" + to_string( m_workevent->Params[ 12 ].asdouble, 2 ) + "]" :
                " [*]" );

        WriteLog( comparisonlog );
        return comparisonresult;
    }
    // unrecognized request
    return false;
}
