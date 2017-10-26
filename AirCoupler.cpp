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

// zwraca 1, jeśli istnieje model prosty, 2 gdy skośny
/**
 * Returns 0 when straight model exists,
 * 2 when oblique model exists, and 0 when neither of them exist.
 */
int TAirCoupler::GetStatus()
{
    if (ModelOn) return 1;
    if (ModelxOn) return 2;
    return 0;
}

// zerowanie wskaźników
/**
 * Reset pointers.
 */
void TAirCoupler::Clear()
{
    ModelOn = NULL;
    ModelOff = NULL;
    ModelxOn = NULL;
    On = false;
    xOn = false;
}
// wyszukanie submodeli
/**
 * Looks for submodels.
 */
void TAirCoupler::Init(std::string const &asName, TModel3d *Model)
{
    if (!Model)
        return; // nie ma w czym szukać
    // polaczony na wprost
    /** Straight connect. */
    ModelOn = Model->GetFromName(asName + "_on");
    // odwieszony
    /** Not connected. Hung up. */
    ModelOff = Model->GetFromName(asName + "_off");
    // polaczony na skos
    /** Oblique connect. */
    ModelxOn = Model->GetFromName(asName + "_xon");
}

void TAirCoupler::Load(cParser *Parser, TModel3d *Model)
{
	std::string name = Parser->getToken<std::string>();
    if(Model)
    {
		Init(name, Model);
	}
    else
    {
        ModelOn = NULL;
        ModelxOn = NULL;
        ModelOff = NULL;
    }
}

void TAirCoupler::Update()
{
    //  if ((pModelOn!=NULL) && (pModelOn!=NULL)) // legacy bullshitt alert
    {
        if (ModelOn)
            ModelOn->iVisible = On;
        if (ModelOff)
            ModelOff->iVisible = !(On || xOn);
        if (ModelxOn)
            ModelxOn->iVisible = xOn;
    }
}
