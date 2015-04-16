/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Button.h"
#include "Console.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

TButton::TButton()
{
    iFeedbackBit = 0;
    Clear();
};

TButton::~TButton(){};

void TButton::Clear(int i)
{
    pModelOn = NULL;
    pModelOff = NULL;
    bOn = false;
    if (i >= 0)
        FeedbackBitSet(i);
    Update(); // kasowanie bitu Feedback, o ile jakiœ ustawiony
};

void TButton::Init(AnsiString asName, TModel3d *pModel, bool bNewOn)
{
    if (!pModel)
        return; // nie ma w czym szukaæ
    pModelOn = pModel->GetFromName(AnsiString(asName + "_on").c_str());
    pModelOff = pModel->GetFromName(AnsiString(asName + "_off").c_str());
    bOn = bNewOn;
    Update();
};

void TButton::Load(TQueryParserComp *Parser, TModel3d *pModel1, TModel3d *pModel2)
{
    AnsiString str = Parser->GetNextSymbol().LowerCase();
    if (pModel1)
    { // poszukiwanie submodeli w modelu
        Init(str, pModel1, false);
        if (pModel2)
            if (!pModelOn && !pModelOff)
                Init(str, pModel2,
                     false); // mo¿e w drugim bêdzie (jak nie w kabinie, to w zewnêtrznym)
    }
    else
    {
        pModelOn = NULL;
        pModelOff = NULL;
    }
};

void TButton::Update()
{
    if (pModelOn)
        pModelOn->iVisible = bOn;
    if (pModelOff)
        pModelOff->iVisible = !bOn;
    if (iFeedbackBit) // je¿eli generuje informacjê zwrotn¹
    {
        if (bOn) // zapalenie
            Console::BitsSet(iFeedbackBit);
        else
            Console::BitsClear(iFeedbackBit);
    }
};
