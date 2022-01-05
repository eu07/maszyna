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
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "stdafx.h"
#include "MemCell.h"

#include "simulation.h"
#include "Driver.h"
#include "Event.h"
#include "Logs.h"

//---------------------------------------------------------------------------

TMemCell::TMemCell( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {}

void TMemCell::UpdateValues( std::string const &szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask )
{
    if (CheckMask & basic_event::flags::mode_add)
    { // dodawanie wartości
        if( TestFlag( CheckMask, basic_event::flags::text ) )
            szText += szNewText;
        if (TestFlag(CheckMask, basic_event::flags::value1))
            fValue1 += fNewValue1;
        if (TestFlag(CheckMask, basic_event::flags::value2))
            fValue2 += fNewValue2;
    }
    else
    {
        if( TestFlag( CheckMask, basic_event::flags::text ) )
            szText = szNewText;
        if (TestFlag(CheckMask, basic_event::flags::value1))
            fValue1 = fNewValue1;
        if (TestFlag(CheckMask, basic_event::flags::value2))
            fValue2 = fNewValue2;
    }
    if (TestFlag(CheckMask, basic_event::flags::text))
        CommandCheck(); // jeśli zmieniony tekst, próbujemy rozpoznać komendę
}

TCommandType TMemCell::CommandCheck()
{ // rozpoznanie komendy
    if( szText == "SetVelocity" ) // najpopularniejsze
    {
        eCommand = TCommandType::cm_SetVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if( szText == "ShuntVelocity" ) // w tarczach manewrowych
    {
        eCommand = TCommandType::cm_ShuntVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if( szText == "Change_direction" ) // zdarza się
    {
        eCommand = TCommandType::cm_ChangeDirection;
        bCommand = true; // do wysłania
    }
    else if( szText == "OutsideStation" ) // zdarza się
    {
        eCommand = TCommandType::cm_OutsideStation;
        bCommand = false; // tego nie powinno być w komórce
    }
    else if( starts_with( szText, "PassengerStopPoint:" ) ) // porównanie początków
    {
        eCommand = TCommandType::cm_PassengerStopPoint;
        bCommand = false; // tego nie powinno być w komórce
    }
    else if( szText == "SetProximityVelocity" ) // nie powinno tego być
    {
        eCommand = TCommandType::cm_SetProximityVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if( szText == "Emergency_brake" )
    {
        eCommand = TCommandType::cm_EmergencyBrake;
        bCommand = false;
    }
    else if( szText == "CabSignal" ) {
        eCommand = TCommandType::cm_SecuritySystemMagnet;
        bCommand = false;
    }
    else
    {
        eCommand = TCommandType::cm_Unknown; // ciąg nierozpoznany (nie jest komendą)
        bCommand = true; // do wysłania
    }
    return eCommand;
}

bool TMemCell::Load(cParser *parser)
{
    std::string token;
    parser->getTokens( 6, false );
    *parser
        >> m_area.center.x
        >> m_area.center.y
        >> m_area.center.z
        >> szText
        >> fValue1
        >> fValue2;
    parser->getTokens();
    *parser >> token;
    if (token != "none") // gdy różne od "none"
        asTrackName = token; // sprawdzane przez IsEmpty()
    parser->getTokens();
    *parser >> token;
    if (token != "endmemcell")
        Error("endmemcell statement missing");
    CommandCheck();
    return true;
}

void TMemCell::PutCommand( TController *Mech, glm::dvec3 const *Loc ) const
{ // wysłanie zawartości komórki do AI
    if (Mech)
        Mech->PutCommand(szText, fValue1, fValue2, Loc);
}

bool TMemCell::Compare( std::string const &szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask,
    comparison_operator const TextOperator, comparison_operator const Value1Operator, comparison_operator const Value2Operator,
    comparison_pass const Pass ) const {
// porównanie zawartości komórki pamięci z podanymi wartościami
/*
    if( TestFlag( CheckMask, basic_event::flags::text ) ) {
        // porównać teksty
        auto range = szTestText.find( '*' );
        if( range != std::string::npos ) {
            // compare string parts
            if( 0 != szText.compare( 0, range, szTestText, 0, range ) ) {
                return false;
            }
        }
        else {
            // compare full strings
            if( szText != szTestText ) {
                return false;
            }
        }
    }
    // tekst zgodny, porównać resztę
    return ( ( !TestFlag( CheckMask, basic_event::flags::value1 ) || ( fValue1 == fTestValue1 ) )
          && ( !TestFlag( CheckMask, basic_event::flags::value2 ) || ( fValue2 == fTestValue2 ) ) );
*/
    bool checkpassed { false };
    bool checkfailed { false };

    if( TestFlag( CheckMask, basic_event::flags::text ) ) {
        // porównać teksty
        auto range = szTestText.find( '*' );
        auto const result { (
            range == std::string::npos ?
                compare( szText, szTestText, TextOperator ) :
                compare( szText.substr( 0, range ), szTestText.substr( 0, range ), TextOperator ) ) };
        checkpassed |=    result;
        checkfailed |= ( !result );
    }
    if( TestFlag( CheckMask, basic_event::flags::value1 ) ) {
        auto const result { compare( fValue1, fTestValue1, Value1Operator ) };
        checkpassed |=    result;
        checkfailed |= ( !result );
    }
    if( TestFlag( CheckMask, basic_event::flags::value2 ) ) {
        auto const result { compare( fValue2, fTestValue2, Value2Operator ) };
        checkpassed |=    result;
        checkfailed |= ( !result );
    }

    switch( Pass ) {
        case comparison_pass::all:  { return ( checkfailed == false ); }
        case comparison_pass::any:  { return ( checkpassed == true ); }
        case comparison_pass::none: { return ( checkpassed == false ); }
        default:                    { return false; }
    }
};

bool TMemCell::IsVelocity() const
{ // sprawdzenie, czy event odczytu tej komórki ma być do skanowania, czy do kolejkowania
    if (eCommand == TCommandType::cm_SetVelocity)
        return true;
    if (eCommand == TCommandType::cm_ShuntVelocity)
        return true;
    return (eCommand == TCommandType::cm_SetProximityVelocity);
};

void TMemCell::StopCommandSent()
{ //
    if (!bCommand)
        return;
    bCommand = false;
    if( OnSent ) {
        // jeśli jest event
        simulation::Events.AddToQuery( OnSent, nullptr );
    }
};

void TMemCell::AssignEvents(basic_event *e)
{ // powiązanie eventu
    OnSent = e;
};

std::string TMemCell::Values() const {

    return { "[" + Text() + "] [" + to_string( Value1(), 2 ) + "] [" + to_string( Value2(), 2 ) + "]" };
}

void TMemCell::LogValues() const {

    WriteLog( "Memcell " + name() + ": " + Values() );
}

// serialize() subclass details, sends content of the subclass to provided stream
void
TMemCell::serialize_( std::ostream &Output ) const {

    // TODO: implement
}
// deserialize() subclass details, restores content of the subclass from provided stream
void
TMemCell::deserialize_( std::istream &Input ) {

    // TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void
TMemCell::export_as_text_( std::ostream &Output ) const {
    // header
    Output << "memcell ";
    // location
    Output
        << location().x << ' '
        << location().y << ' '
        << location().z << ' '
    // cell data
        << szText << ' '
        << fValue1 << ' '
        << fValue2 << ' '
    // associated track
        << ( Track ? Track->name() : asTrackName.empty() ? "none" : asTrackName ) << ' '
    // footer
        << "endmemcell"
        << "\n";
}




// legacy method, initializes traction after deserialization from scenario file
void
memory_table::InitCells() {

    for( auto *cell : m_items ) {
        // Ra: eventy komórek pamięci, wykonywane po wysłaniu komendy do zatrzymanego pojazdu
        cell->AssignEvents( simulation::Events.FindEvent( cell->name() + ":sent" ) );
        // bind specified path with the memory cell
        if( false == cell->asTrackName.empty() ) {
            cell->Track = simulation::Paths.find( cell->asTrackName );
            if( cell->Track == nullptr ) {
                ErrorLog( "Bad memcell: track \"" + cell->asTrackName + "\" referenced in memcell \"" + cell->name() + "\" doesn't exist" );
            }
        }
    }
}

// legacy method, sends content of all cells to the log
void
memory_table::log_all() {

    for( auto *cell : m_items ) {
        cell->LogValues();
    }
}
