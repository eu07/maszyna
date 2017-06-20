/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef FadeSoundH
#define FadeSoundH

#include "Sound.h"
#include "AdvSound.h"

class TFadeSound
{
    PSound Sound = nullptr;
    float fFade = 0.0f;
    float dt = 0.0f,
          fTime = 0.0f;
    TSoundState State = ss_Off;

  public:
    TFadeSound();
    ~TFadeSound();
    void Init(std::string const &Name, float fNewFade);
    void TurnOn();
    void TurnOff();
    bool Playing()
    {
        return (State == ss_Commencing || State == ss_Starting);
    };
    void Free();
    void Update();
    void Volume(long vol);
};

//---------------------------------------------------------------------------
#endif
