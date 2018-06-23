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
#include "EvLaunch.h"

#include "Globals.h"
#include "Logs.h"
#include "Event.h"
#include "MemCell.h"
#include "Timer.h"
#include "parser.h"
#include "Console.h"
#include "World.h"
#include "utilities.h"

//---------------------------------------------------------------------------

// encodes expected key in a short, where low byte represents the actual key,
// and the high byte holds modifiers: 0x1 = shift, 0x2 = ctrl, 0x4 = alt
int vk_to_glfw_key( int const Keycode ) {

#ifdef _WIN32
    auto const code = VkKeyScan( Keycode );
#else
	auto const code = (short int)Keycode;
#endif
    char key = code & 0xff;
    char shiftstate = ( code & 0xff00 ) >> 8;

    if( (key >= 'A') && (key <= 'Z') ) {
        key = GLFW_KEY_A + key - 'A';
    }
    else if( ( key >= '0' ) && ( key <= '9' ) ) {
        key = GLFW_KEY_0 + key - '0';
    }
    return key + ( shiftstate << 8 );
}

bool TEventLauncher::Load(cParser *parser)
{ // wczytanie wyzwalacza zdarzeń
    std::string token;
    parser->getTokens();
    *parser >> dRadius; // promień działania
    if (dRadius > 0.0)
        dRadius *= dRadius; // do kwadratu, pod warunkiem, że nie jest ujemne
    parser->getTokens(); // klawisz sterujący
    *parser >> token;
    if (token != "none")
    {
        if( token.size() == 1 )
            iKey = vk_to_glfw_key( token[ 0 ] );
        else
            iKey = vk_to_glfw_key(stol_def( token, 0 )); // a jak więcej, to jakby numer klawisza jest
    }
    parser->getTokens();
    *parser >> DeltaTime;
    if (DeltaTime < 0)
        DeltaTime = -DeltaTime; // dla ujemnego zmieniamy na dodatni
    else if (DeltaTime > 0)
    { // wartość dodatnia oznacza wyzwalanie o określonej godzinie
        iMinute = int(DeltaTime) % 100; // minuty są najmłodszymi cyframi dziesietnymi
        iHour = int(DeltaTime - iMinute) / 100; // godzina to setki
        DeltaTime = 0; // bez powtórzeń
        // potentially shift the provided time by requested offset
        auto const timeoffset { static_cast<int>( Global.ScenarioTimeOffset * 60 ) };
        if( timeoffset != 0 ) {
            auto const adjustedtime { clamp_circular( iHour * 60 + iMinute + timeoffset, 24 * 60 ) };
            iHour = ( adjustedtime / 60 ) % 24;
            iMinute = adjustedtime % 60;
        }
        WriteLog(
            "EventLauncher at "
            + std::to_string( iHour ) + ":"
            + ( iMinute < 10 ? "0" : "" ) + to_string( iMinute ) ); // wyświetlenie czasu
    }
    parser->getTokens();
    *parser >> token;
    asEvent1Name = token; // pierwszy event
    parser->getTokens();
    *parser >> token;
    asEvent2Name = token; // drugi event
    if ((asEvent2Name == "end") || (asEvent2Name == "condition"))
    { // drugiego eventu może nie być, bo są z tym problemy, ale ciii...
		token = asEvent2Name; // rozpoznane słowo idzie do dalszego przetwarzania
        asEvent2Name = "none"; // a drugiego eventu nie ma
    }
    else
    { // gdy są dwa eventy
        parser->getTokens();
        *parser >> token;
        //str = AnsiString(token.c_str());
    }
    if (token == "condition")
    { // obsługa wyzwalania warunkowego
        parser->getTokens();
        *parser >> token;
		asMemCellName = token;
        parser->getTokens();
        *parser >> token;
        szText = token;
        if (token != "*") //*=nie brać command pod uwagę
            iCheckMask |= conditional_memstring;
        parser->getTokens();
        *parser >> token;
        if (token != "*") //*=nie brać wartości 1. pod uwagę
        {
            iCheckMask |= conditional_memval1;
            fVal1 = atof(token.c_str());
        }
        else
            fVal1 = 0;
        parser->getTokens();
        *parser >> token;
        if (token.compare("*") != 0) //*=nie brać wartości 2. pod uwagę
        {
            iCheckMask |= conditional_memval2;
            fVal2 = atof(token.c_str());
        }
        else
            fVal2 = 0;
        parser->getTokens(); // słowo zamykające
        *parser >> token;
    }
    return true;
}

bool TEventLauncher::check_conditions()
{ //"renderowanie" wyzwalacza
    auto bCond { false };
    if (iKey != 0) {
        if( iKey > 255 ) {
            // key and modifier
            auto const modifier = ( iKey & 0xff00 ) >> 8;
            bCond = ( Console::pressed( iKey & 0xff ) )
                 && ( ( modifier & 1 ) ? Global.shiftState : true )
                 && ( ( modifier & 2 ) ? Global.ctrlState : true );
        }
        else {
            // just key
            bCond = ( Console::pressed( iKey & 0xff ) ); // czy klawisz wciśnięty
        }
    }
    if (DeltaTime > 0)
    {
        if (UpdatedTime > DeltaTime)
        {
            UpdatedTime = 0; // naliczanie od nowa
            bCond = true;
        }
        else
            UpdatedTime += Timer::GetDeltaTime(); // aktualizacja naliczania czasu
    }
    else
    { // jeśli nie cykliczny, to sprawdzić czas
        if (simulation::Time.data().wHour == iHour)
        {
            if (simulation::Time.data().wMinute == iMinute)
            { // zgodność czasu uruchomienia
                if (UpdatedTime < 10)
                {
                    UpdatedTime = 20; // czas do kolejnego wyzwolenia?
                    bCond = true;
                }
            }
        }
        else
            UpdatedTime = 1;
    }
    if (bCond) // jeśli spełniony został warunek
    {
        if ((iCheckMask != 0) && MemCell) // sprawdzanie warunku na komórce pamięci
            bCond = MemCell->Compare(szText, fVal1, fVal2, iCheckMask);
    }
    return bCond; // sprawdzanie dRadius w Ground.cpp
}

// sprawdzenie, czy jest globalnym wyzwalaczem czasu
bool TEventLauncher::IsGlobal() const {

    return ( ( DeltaTime == 0 )
          && ( iHour >= 0 )
          && ( iMinute >= 0 )
          && ( dRadius < 0.0 ) ); // bez ograniczenia zasięgu
}

// radius() subclass details, calculates node's bounding radius
float
TEventLauncher::radius_() {

    return std::sqrt( dRadius );
}

// serialize() subclass details, sends content of the subclass to provided stream
void
TEventLauncher::serialize_( std::ostream &Output ) const {

    // TODO: implement
}
// deserialize() subclass details, restores content of the subclass from provided stream
void
TEventLauncher::deserialize_( std::istream &Input ) {

    // TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void
TEventLauncher::export_as_text_( std::ostream &Output ) const {
    // header
    Output << "eventlauncher ";
    // location
    Output
        << location().x << ' '
        << location().y << ' '
        << location().z << ' ';
    // activation radius
    Output
        << ( dRadius > 0 ? std::sqrt( dRadius ) : dRadius ) << ' ';
    // activation key
    if( iKey != 0 ) {
        auto const key { iKey & 0xff };
        auto const modifier { ( iKey & 0xff00 ) >> 8 };
        if( ( key >= GLFW_KEY_A ) && ( key <= GLFW_KEY_Z ) ) {
            Output << static_cast<char>(( 'A' + key - GLFW_KEY_A + ( ( modifier & 1 ) == 0 ? 32 : 0 ) )) << ' ';
        }
        else if( ( key >= GLFW_KEY_0 ) && ( key <= GLFW_KEY_9 ) ) {
            Output << static_cast<char>(( '0' + key - GLFW_KEY_0 )) << ' ';
        }
    }
    else {
        Output << "none ";
    }
    // activation interval or hour
    if( DeltaTime != 0 ) {
        // cyclical launcher
        Output << -DeltaTime << ' ';
    }
    else {
        // single activation at specified time
        if( ( iHour < 0 )
         && ( iMinute < 0 ) ) {
            Output << DeltaTime << ' ';
        }
        else {
        // NOTE: activation hour might be affected by user-requested time offset
            auto const timeoffset{ static_cast<int>( Global.ScenarioTimeOffset * 60 ) };
            auto const adjustedtime{ clamp_circular( iHour * 60 + iMinute - timeoffset, 24 * 60 ) };
            Output
                << ( adjustedtime / 60 ) % 24
                << ( adjustedtime % 60 )
                << ' ';
        }
    }
    // associated event(s)
    Output << ( asEvent1Name.empty() ? "none" : asEvent1Name ) << ' ';
    Output << ( asEvent2Name.empty() ? "none" : asEvent2Name ) << ' ';
    if( false == asMemCellName.empty() ) {
        // conditional event
        Output
            << "condition "
            << asMemCellName << ' '
            << szText << ' '
            << ( ( iCheckMask & conditional_memval1 ) != 0 ? to_string( fVal1 ) : "*" ) << ' '
            << ( ( iCheckMask & conditional_memval2 ) != 0 ? to_string( fVal2 ) : "*" ) << ' ';
    }
    // footer
    Output
        << "end"
        << "\n";
}

//---------------------------------------------------------------------------
