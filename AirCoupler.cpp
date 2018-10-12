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

#include "Model3d.h"
#include "parser.h"

AirCoupler::AirCoupler()
{
    Clear();
}

AirCoupler::~AirCoupler()
{
}

/**
 * \return 1 when \(straight\) TModel3d \c only ModelOn exists
 * \return 2 when \(slanted\) TModel3d \c ModelxOn exists
 * \return 0 when neither of them exist
 */
int AirCoupler::GetStatus()
{
    if (ModelxOn)
		return 2;
    if (ModelOn)
		return 1;
    return 0;
}

/**
 * Reset pointers and variables.
 */
void AirCoupler::Clear()
{
    ModelOn = NULL;
    ModelOff = NULL;
    ModelxOn = NULL;
    On = false;
    xOn = false;
}

/**
 * Looks for submodels in the model and updates pointers.
 */
void AirCoupler::Init(std::string const &asName, TModel3d *Model)
{
    if (!Model)
        return;
    ModelOn = Model->GetFromName(asName + "_on"); // Straight connect.
    ModelOff = Model->GetFromName(asName + "_off"); // Not connected. Hung up.
    ModelxOn = Model->GetFromName(asName + "_xon"); // Slanted connect.
}
/**
 * Gets name of submodel \(from cParser \b *Parser\),
 * looks for it in the TModel3d \b *Model and update pointers.
 * If submodel is not found, reset pointers.
*/
void AirCoupler::Load(cParser *Parser, TModel3d *Model)
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

// Update submodels visibility.
void AirCoupler::Update()
{
    if (ModelOn)
        ModelOn->iVisible = On;
    if (ModelOff)
        ModelOff->iVisible = !(On || xOn);
    if (ModelxOn)
        ModelxOn->iVisible = xOn;
}
