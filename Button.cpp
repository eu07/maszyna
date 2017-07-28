/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Button.h"
#include "Console.h"
#include "Logs.h"

//---------------------------------------------------------------------------

TButton::TButton()
{
    iFeedbackBit = 0;
    bData = NULL;
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
    Update(); // kasowanie bitu Feedback, o ile jakiś ustawiony
};

void TButton::Init(std::string const &asName, TModel3d *pModel, bool bNewOn)
{
    if (!pModel)
        return; // nie ma w czym szukać
    pModelOn = pModel->GetFromName( (asName + "_on").c_str() );
    pModelOff = pModel->GetFromName( (asName + "_off").c_str() );
    bOn = bNewOn;
    Update();
};

void TButton::Load(cParser &Parser, TModel3d *pModel1, TModel3d *pModel2)
{
	std::string const token = Parser.getToken<std::string>();
    if (pModel1)
    { // poszukiwanie submodeli w modelu
        Init(token, pModel1, false);
        if (pModel2)
            if (!pModelOn && !pModelOff)
                Init(token, pModel2, false); // może w drugim będzie (jak nie w kabinie,
        // to w zewnętrznym)
    }
    else
    {
        pModelOn = NULL;
        pModelOff = NULL;

        ErrorLog( "Failed to locate sub-model \"" + token + "\" in 3d model \"" + pModel1->NameGet() + "\"" );
    }
};

void TButton::Update()
{
    if (bData != NULL)
        bOn = (*bData);
    if (pModelOn)
        pModelOn->iVisible = bOn;
    if (pModelOff)
        pModelOff->iVisible = !bOn;
    if (iFeedbackBit) // jeżeli generuje informację zwrotną
    {
        if (bOn) // zapalenie
            Console::BitsSet(iFeedbackBit);
        else
            Console::BitsClear(iFeedbackBit);
    }
};

void TButton::AssignBool(bool *bValue)
{
    bData = bValue;
}
