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

#include "Globals.h"
#include "Logs.h"
#include "Usefull.h"
#include "Driver.h"
#include "Event.h"
#include "parser.h"

//---------------------------------------------------------------------------

TMemCell::TMemCell(vector3 *p)
{
    fValue1 = fValue2 = 0;
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    szText = new char[256]; // musi być dla automatycznie tworzonych komórek dla odcinków izolowanych
#endif
    vPosition =
        p ? *p : vector3(0, 0, 0); // ustawienie współrzędnych, bo do TGroundNode nie ma dostępu
    bCommand = false; // komenda wysłana
    OnSent = NULL;
}
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
TMemCell::~TMemCell()
{
    SafeDeleteArray(szText);
}
#endif
void TMemCell::Init()
{
}
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
void TMemCell::UpdateValues(char const *szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask)
#else
void TMemCell::UpdateValues( std::string const &szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask )
#endif
{
    if (CheckMask & update_memadd)
    { // dodawanie wartości
        if( TestFlag( CheckMask, update_memstring ) )
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
            strcat( szText, szNewText );
#else
            szText += szNewText;
#endif
        if (TestFlag(CheckMask, update_memval1))
            fValue1 += fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 += fNewValue2;
    }
    else
    {
        if( TestFlag( CheckMask, update_memstring ) )
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
            strcpy( szText, szNewText );
#else
            szText = szNewText;
#endif
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
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    if (strcmp(szText, "SetVelocity") == 0) // najpopularniejsze
#else
    if( szText == "SetVelocity" ) // najpopularniejsze
#endif
    {
        eCommand = cm_SetVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    else if( strcmp( szText, "ShuntVelocity" ) == 0 ) // w tarczach manewrowych
#else
    else if( szText == "ShuntVelocity" ) // w tarczach manewrowych
#endif
    {
        eCommand = cm_ShuntVelocity;
        bCommand = false; // ta komenda nie jest wysyłana
    }
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    else if( strcmp( szText, "Change_direction" ) == 0 ) // zdarza się
#else
    else if( szText == "Change_direction" ) // zdarza się
#endif
    {
        eCommand = cm_ChangeDirection;
        bCommand = true; // do wysłania
    }
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    else if( strcmp( szText, "OutsideStation" ) == 0 ) // zdarza się
#else
    else if( szText == "OutsideStation" ) // zdarza się
#endif
    {
        eCommand = cm_OutsideStation;
        bCommand = false; // tego nie powinno być w komórce
    }
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    else if( strncmp( szText, "PassengerStopPoint:", 19 ) == 0 ) // porównanie początków
#else
    else if( szText.compare( 0, 19, "PassengerStopPoint:" ) == 0 ) // porównanie początków
#endif
    {
        eCommand = cm_PassengerStopPoint;
        bCommand = false; // tego nie powinno być w komórce
    }
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    else if( strcmp( szText, "SetProximityVelocity" ) == 0 ) // nie powinno tego być
#else
    else if( szText == "SetProximityVelocity" ) // nie powinno tego być
#endif
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
    parser->getTokens(1, false); // case sensitive
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    *parser >> token;
    SafeDeleteArray(szText);
    szText = new char[256]; // musi być bufor do łączenia tekstów
    strcpy(szText, token.c_str());
#else
    *parser >> szText;
#endif
    parser->getTokens( 2, false );
    *parser
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

void TMemCell::PutCommand(TController *Mech, vector3 *Loc)
{ // wysłanie zawartości komórki do AI
    if (Mech)
        Mech->PutCommand(szText, fValue1, fValue2, Loc);
}

bool TMemCell::Compare(char const *szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask)
{ // porównanie zawartości komórki pamięci z podanymi wartościami
    if (TestFlag(CheckMask, conditional_memstring))
    { // porównać teksty
        std::string
			match( szTestText );
		auto range = match.find( '*' );
		if( range != std::string::npos ) {
			// compare string parts
			if( 0 != szText.compare( 0, range, match, 0, range ) ) {
				return false;
			}
		}
		else {
			// compare full strings
			if( szText != match ) {
				return false;
			}
		}
	}
    // tekst zgodny, porównać resztę
    return ((!TestFlag(CheckMask, conditional_memval1) || (fValue1 == fTestValue1)) &&
            (!TestFlag(CheckMask, conditional_memval2) || (fValue2 == fTestValue2)));
};

#ifndef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
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
    return ( ( !TestFlag( CheckMask, conditional_memval1 ) || ( fValue1 == fTestValue1 ) ) &&
        ( !TestFlag( CheckMask, conditional_memval2 ) || ( fValue2 == fTestValue2 ) ) );
};
#endif

bool TMemCell::Render()
{
    return true;
}

bool TMemCell::IsVelocity()
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
    if (OnSent) // jeśli jest event
        Global::AddToQuery(OnSent, NULL);
};

void TMemCell::AssignEvents(TEvent *e)
{ // powiązanie eventu
    OnSent = e;
};
