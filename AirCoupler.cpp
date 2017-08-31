/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "AirCoupler.h"

TAirCoupler::TAirCoupler()
{
    Clear();
}

TAirCoupler::~TAirCoupler()
{
}

int TAirCoupler::GetStatus()
{ // zwraca 1, jeśli istnieje model prosty, 2 gdy skośny
    int x = 0;
    if (pModelOn)
        x = 1;
    if (pModelxOn)
        x = 2;
    return x;
}

void TAirCoupler::Clear()
{ // zerowanie wskaźników
    pModelOn = NULL;
    pModelOff = NULL;
    pModelxOn = NULL;
    bOn = false;
    bxOn = false;
}

void TAirCoupler::Init(std::string const &asName, TModel3d *pModel)
{ // wyszukanie submodeli
    if (!pModel)
        return; // nie ma w czym szukać
    pModelOn = pModel->GetFromName( asName + "_on" ); // połączony na wprost
    pModelOff = pModel->GetFromName( asName + "_off" ); // odwieszony
    pModelxOn = pModel->GetFromName( asName + "_xon" ); // połączony na skos
}

void TAirCoupler::Load(cParser *Parser, TModel3d *pModel)
{
	std::string name = Parser->getToken<std::string>();
	if( pModel ) {

		Init( name, pModel );
	}
    else
    {
        pModelOn = NULL;
        pModelxOn = NULL;
        pModelOff = NULL;
    }
}

void TAirCoupler::Update()
{
    //  if ((pModelOn!=NULL) && (pModelOn!=NULL))
    {
        if (pModelOn)
            pModelOn->iVisible = bOn;
        if (pModelOff)
            pModelOff->iVisible = !(bOn || bxOn);
        if (pModelxOn)
            pModelxOn->iVisible = bxOn;
    }
}

//---------------------------------------------------------------------------
