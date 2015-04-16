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

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Driver.h"
#include "mctools.hpp"
#include "MemCell.h"
#include "Event.h"
#include "parser.h"

#include "Usefull.h"
#include "Globals.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TMemCell::TMemCell(vector3 *p)
{
    fValue1 = fValue2 = 0;
    szText = new char[256]; // musi byæ dla automatycznie tworzonych komórek dla odcinków
                            // izolowanych
    vPosition =
        p ? *p : vector3(0, 0, 0); // ustawienie wspó³rzêdnych, bo do TGroundNode nie ma dostêpu
    bCommand = false; // komenda wys³ana
    OnSent = NULL;
}

TMemCell::~TMemCell() { SafeDeleteArray(szText); }

void TMemCell::Init() {}

void TMemCell::UpdateValues(char *szNewText, double fNewValue1, double fNewValue2,
                                       int CheckMask)
{
    if (CheckMask & update_memadd)
    { // dodawanie wartoœci
        if (TestFlag(CheckMask, update_memstring))
            strcat(szText, szNewText);
        if (TestFlag(CheckMask, update_memval1))
            fValue1 += fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 += fNewValue2;
    }
    else
    {
        if (TestFlag(CheckMask, update_memstring))
            strcpy(szText, szNewText);
        if (TestFlag(CheckMask, update_memval1))
            fValue1 = fNewValue1;
        if (TestFlag(CheckMask, update_memval2))
            fValue2 = fNewValue2;
    }
    if (TestFlag(CheckMask, update_memstring))
        CommandCheck(); // jeœli zmieniony tekst, próbujemy rozpoznaæ komendê
}

TCommandType TMemCell::CommandCheck()
{ // rozpoznanie komendy
    if (strcmp(szText, "SetVelocity") == 0) // najpopularniejsze
    {
        eCommand = cm_SetVelocity;
        bCommand = false; // ta komenda nie jest wysy³ana
    }
    else if (strcmp(szText, "ShuntVelocity") == 0) // w tarczach manewrowych
    {
        eCommand = cm_ShuntVelocity;
        bCommand = false; // ta komenda nie jest wysy³ana
    }
    else if (strcmp(szText, "Change_direction") == 0) // zdarza siê
    {
        eCommand = cm_ChangeDirection;
        bCommand = true; // do wys³ania
    }
    else if (strcmp(szText, "OutsideStation") == 0) // zdarza siê
    {
        eCommand = cm_OutsideStation;
        bCommand = false; // tego nie powinno byæ w komórce
    }
    else if (strncmp(szText, "PassengerStopPoint:", 19) == 0) // porównanie pocz¹tków
    {
        eCommand = cm_PassengerStopPoint;
        bCommand = false; // tego nie powinno byæ w komórce
    }
    else if (strcmp(szText, "SetProximityVelocity") == 0) // nie powinno tego byæ
    {
        eCommand = cm_SetProximityVelocity;
        bCommand = false; // ta komenda nie jest wysy³ana
    }
    else
    {
        eCommand = cm_Unknown; // ci¹g nierozpoznany (nie jest komend¹)
        bCommand = true; // do wys³ania
    }
    return eCommand;
}

bool TMemCell::Load(cParser *parser)
{
    std::string token;
    parser->getTokens(1, false); // case sensitive
    *parser >> token;
    SafeDeleteArray(szText);
    szText = new char[256]; // musi byæ bufor do ³¹czenia tekstów
    strcpy(szText, token.c_str());
    parser->getTokens();
    *parser >> fValue1;
    parser->getTokens();
    *parser >> fValue2;
    parser->getTokens();
    *parser >> token;
    if (token.compare("none") != 0) // gdy ró¿ne od "none"
        asTrackName = AnsiString(token.c_str()); // sprawdzane przez IsEmpty()
    parser->getTokens();
    *parser >> token;
    if (token.compare("endmemcell") != 0)
        Error("endmemcell statement missing");
    CommandCheck();
    return true;
}

void TMemCell::PutCommand(TController *Mech, vector3 *Loc)
{ // wys³anie zawartoœci komórki do AI
    if (Mech)
        Mech->PutCommand(szText, fValue1, fValue2, Loc);
}

bool TMemCell::Compare(char *szTestText, double fTestValue1, double fTestValue2,
                                  int CheckMask)
{ // porównanie zawartoœci komórki pamiêci z podanymi wartoœciami
    if (TestFlag(CheckMask, conditional_memstring))
    { // porównaæ teksty
        char *pos = StrPos(szTestText, "*"); // zwraca wskaŸnik na pozycjê albo NULL
        if (pos)
        { // porównanie fragmentu ³añcucha
            int i = pos - szTestText; // iloœæ porównywanych znaków
            if (i) // jeœli nie jest pierwszym znakiem
                if (AnsiString(szTestText, i) != AnsiString(szText, i))
                    return false; // pocz¹tki o d³ugoœci (i) s¹ ró¿ne
        }
        else if (AnsiString(szTestText) != AnsiString(szText))
            return false; //³¹ñcuchy s¹ ró¿ne
    }
    // tekst zgodny, porównaæ resztê
    return ((!TestFlag(CheckMask, conditional_memval1) || (fValue1 == fTestValue1)) &&
            (!TestFlag(CheckMask, conditional_memval2) || (fValue2 == fTestValue2)));
};

bool TMemCell::Render() { return true; }

bool TMemCell::IsVelocity()
{ // sprawdzenie, czy event odczytu tej komórki ma byæ do skanowania, czy do kolejkowania
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
    if (OnSent) // jeœli jest event
        Global::AddToQuery(OnSent, NULL);
};

void TMemCell::AssignEvents(TEvent *e)
{ // powi¹zanie eventu
    OnSent = e;
};
