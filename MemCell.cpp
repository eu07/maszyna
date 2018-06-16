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
#include "Logs.h"

TMemCell::TMemCell( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {}

void TMemCell::UpdateValues( std::string const &szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask )
{
    if (CheckMask & update_memadd)
    { // dodawanie wartości
        if( TestFlag( CheckMask, update_memstring ) )
            szText += szNewText;
        if (TestFlag(CheckMask, update_memval1))
            fValue1 += fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 += fNewValue2;
    }
    else
    {
        if( TestFlag( CheckMask, update_memstring ) )
            szText = szNewText;
        if (TestFlag(CheckMask, update_memval1))
            fValue1 = fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 = fNewValue2;
    }
    if (TestFlag(CheckMask, update_memstring))
        CommandCheck(); // jeśli zmieniony tekst, próbujemy rozpoznać komendę
}

TCommandType TMemCell::CommandCheck()
{ // rozpoznanie komendy
    if( szText == "SetVelocity" ) // najpopularniejsze
    {
        eCommand = cm_SetVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if( szText == "ShuntVelocity" ) // w tarczach manewrowych
    {
        eCommand = cm_ShuntVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else if( szText == "Change_direction" ) // zdarza się
    {
        eCommand = cm_ChangeDirection;
        bCommand = true; // do wysłania
    }
    else if( szText == "OutsideStation" ) // zdarza się
    {
        eCommand = cm_OutsideStation;
        bCommand = false; // tego nie powinno być w komórce
    }
    else if( szText.compare( 0, 19, "PassengerStopPoint:" ) == 0 ) // porównanie początków
    {
        eCommand = cm_PassengerStopPoint;
        bCommand = false; // tego nie powinno być w komórce
    }
    else if( szText == "SetProximityVelocity" ) // nie powinno tego być
    {
        eCommand = cm_SetProximityVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
    else
    {
        eCommand = cm_Unknown; // ciąg nierozpoznany (nie jest komendą)
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

void TMemCell::PutCommand( TController *Mech, glm::dvec3 const *Loc )
{ // wysłanie zawartości komórki do AI
    if (Mech)
        Mech->PutCommand(szText, fValue1, fValue2, Loc);
}

bool TMemCell::Compare( std::string const &szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask ) {
    // porównanie zawartości komórki pamięci z podanymi wartościami
    if( TestFlag( CheckMask, conditional_memstring ) ) {
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
    return ( ( !TestFlag( CheckMask, conditional_memval1 ) || ( fValue1 == fTestValue1 ) )
          && ( !TestFlag( CheckMask, conditional_memval2 ) || ( fValue2 == fTestValue2 ) ) );
};

bool TMemCell::IsVelocity() const
{ // sprawdzenie, czy event odczytu tej komórki ma być do skanowania, czy do kolejkowania
    if (eCommand == cm_SetVelocity)
        return true;
    if (eCommand == cm_ShuntVelocity)
        return true;
    return (eCommand == cm_SetProximityVelocity);
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

void TMemCell::AssignEvents(TEvent *e)
{ // powiązanie eventu
    OnSent = e;
};

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
        << ( asTrackName.empty() ? "none" : asTrackName ) << ' '
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
    }
}

// legacy method, sends content of all cells to the log
void
memory_table::log_all() {

    for( auto *cell : m_items ) {

        WriteLog( "Memcell \"" + cell->name() + "\": ["
            + cell->Text() + "] ["
            + to_string( cell->Value1(), 2 ) + "] ["
            + to_string( cell->Value2(), 2 ) + "]" );
    }
}
