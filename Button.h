/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "Classes.h"
#include "sound.h"

// animacja dwustanowa, włącza jeden z dwóch submodeli (jednego z nich może nie być)
class TButton
{
public:
	TButton() = default;
	void clear(const int i = -1);
	inline void feedbackBitSet(const int i)
	{
		iFeedbackBit = 1 << i;
	};
	void turn(const bool state);
	inline bool getValue() const
	{
		return (state ? 1 : 0);
	}
	inline bool active()
	{
		return ((pModelOn != nullptr) || (pModelOff != nullptr));
	}
	void update();
	void init(const std::string& asName, TModel3d* pModel, bool bNewOn = false);
	void load(cParser& parser, const TDynamicObject* owner, TModel3d* pModel1, TModel3d* pModel2 = nullptr);
	void assignBool(const bool* bValue);
	// returns offset of submodel associated with the button from the model centre
	glm::vec3 modelOffset() const;
	inline uint8_t b()
	{
		return state ? 1 : 0;
	};

private:
	// imports member data pair from the config file
	bool loadMapping(cParser& input);
	// plays the sound associated with current state
	void play();

	TSubModel *pModelOn{ nullptr }, *pModelOff{ nullptr }; // submodel dla stanu załączonego i wyłączonego
	bool state{ false };
	const bool* bData{ nullptr };
	int iFeedbackBit{ 0 }; // Ra: bit informacji zwrotnej, do wyprowadzenia na pulpit
	sound_source soundFxIncrease{ sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE }; // sound associated with increasing control's value
	sound_source soundFxDecrease{ sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE }; // sound associated with decreasing control's value
};
