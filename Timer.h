/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

namespace Timer
{

double GetTime();

double GetDeltaTime();
double GetDeltaRenderTime();

double GetfSinceStart();

void SetDeltaTime(double v);

bool GetSoundTimer();

double GetFPS();

void ResetTimers();

void UpdateTimers(bool pause);
};

//---------------------------------------------------------------------------
