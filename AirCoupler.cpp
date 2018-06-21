/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "AirCoupler.h"

AirCoupler::AirCoupler()
{
	clear();
}

AirCoupler::~AirCoupler() {}

int AirCoupler::getStatus()
{
	if (modelxOn) {
		return 2;
	}
	if (modelOn) {
		return 1;
	}
	return 0;
}

/**
 * Reset pointers and variables.
 */
void AirCoupler::clear()
{
	modelOn = nullptr;
	modelOff = nullptr;
	modelxOn = nullptr;
	on = false;
	xOn = false;
}

/**
 * Looks for submodels in the model and updates pointers.
 */
void AirCoupler::init(const std::string& asName, TModel3d* model)
{
	if (model) {
		modelOn = model->GetFromName(asName + "_on"); // Straight connect.
		modelOff = model->GetFromName(asName + "_off"); // Not connected. Hung up.
		modelxOn = model->GetFromName(asName + "_xon"); // Slanted connect.
	}
}
/**
 * Gets name of submodel \(from cParser \b *Parser\),
 * looks for it in the TModel3d \b *Model and update pointers.
 * If submodel is not found, reset pointers.
*/
void AirCoupler::load(cParser* parser, TModel3d* model)
{
	std::string name = parser->getToken<std::string>();
	if (model) {
		init(name, model);
	} else {
		modelOn = nullptr;
		modelxOn = nullptr;
		modelOff = nullptr;
	}
}

// Update submodels visibility.
void AirCoupler::update()
{
	if (modelOn) {
		modelOn->iVisible = on;
	}
	if (modelOff) {
		modelOff->iVisible = !(on || xOn);
	}
	if (modelxOn) {
		modelxOn->iVisible = xOn;
	}
}
