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
#include "parser.h"
#include "Model3d.h"
#include "Console.h"
#include "Logs.h"
#include "renderer.h"

void TButton::clear(int i)
{
	pModelOn = nullptr;
	pModelOff = nullptr;
	state = false;
	if (i >= 0) {
		feedbackBitSet(i);
	}
	update(); // kasowanie bitu Feedback, o ile jakiś ustawiony
};

void TButton::init(const std::string& asName, TModel3d* pModel, bool bNewOn)
{
	if (pModel) {
		pModelOn = pModel->GetFromName(asName + "_on");
		pModelOff = pModel->GetFromName(asName + "_off");
		state = bNewOn;
		update();
	}
};

void TButton::load(cParser& parser, const TDynamicObject* owner, TModel3d* pModel1, TModel3d* pModel2)
{
	std::string submodelname;

	parser.getTokens();
	if (parser.peek() != "{") {
		// old fixed size config
		parser >> submodelname;
	} else {
		// new, block type config
		// TODO: rework the base part into yaml-compatible flow style mapping
		submodelname = parser.getToken<std::string>(false);
		while (loadMapping(parser)) {
		} // all work done by while()
	}

	// bind defined sounds with the button owner
	soundFxIncrease.owner(owner);
	soundFxDecrease.owner(owner);

	if (pModel1) {
		// poszukiwanie submodeli w modelu
		init(submodelname, pModel1, false);
	}
	if (!pModelOn && !pModelOff && pModel2) {
		// poszukiwanie submodeli w modelu
		init(submodelname, pModel2, false);
	}

	if (!pModelOn && !pModelOff) {
		// if we failed to locate even one state submodel, cry
		ErrorLog("Bad model: failed to locate sub-model \"" + submodelname + "\" in 3d model \"" + pModel1->NameGet() + "\"", logtype::model);
	}

	// pass submodel location to defined sounds
	auto const nulloffset{ glm::vec3{} };
	auto const offset{ modelOffset() };
	if (soundFxIncrease.offset() == nulloffset) {
		soundFxIncrease.offset(offset);
	}
	if (soundFxDecrease.offset() == nulloffset) {
		soundFxDecrease.offset(offset);
	}
}

bool TButton::loadMapping(cParser& input)
{
	// token can be a key or block end
	std::string const key{ input.getToken<std::string>(true, "\n\r\t  ,;") };
	if (key.empty() || (key == "}")) {
		return false;
	}
	// if not block end then the key is followed by assigned value or sub-block
	if (key == "soundinc:") {
		soundFxIncrease.deserialize(input, sound_type::single);
	} else if (key == "sounddec:") {
		soundFxDecrease.deserialize(input, sound_type::single);
	}

	return true; // return value marks a key: value pair was extracted, nothing about whether it's recognized
}

// returns offset of submodel associated with the button from the model centre
glm::vec3 TButton::modelOffset() const
{
	auto const submodel{ (pModelOn ? pModelOn : (pModelOff ? pModelOff : nullptr)) };

	return (submodel ? submodel->offset(std::numeric_limits<float>::max()) : glm::vec3());
}

void TButton::turn(const bool state)
{
	if (state != this->state) {
		this->state = state;
		play();
		update();
	}
}

void TButton::update()
{
	if (bData && (*bData != state)) {
		state = (*bData);
		play();
	}

	if (pModelOn) {
		pModelOn->iVisible = state;
	}
	if (pModelOff) {
		pModelOff->iVisible = !state;
	}

#ifdef _WIN32
	if (iFeedbackBit) {
		// jeżeli generuje informację zwrotną
		if (state) { // zapalenie
			Console::BitsSet(iFeedbackBit);
		} else {
			Console::BitsClear(iFeedbackBit);
		}
	}
#endif
};

void TButton::assignBool(const bool* bValue)
{
	bData = bValue;
}

void TButton::play()
{
	if (state) {
		soundFxIncrease.play();
	} else {
		soundFxDecrease.play();
	}
}
