//---------------------------------------------------------------------------

#ifndef FadeSoundH
#define FadeSoundH

#include "Sound.h"
#include "AdvSound.h"

class TFadeSound
{
    PSound Sound;
    float fFade;
    float dt, fTime;
    TSoundState State;

  public:
    TFadeSound();
    ~TFadeSound();
    void Init(char *Name, float fNewFade);
    void TurnOn();
    void TurnOff();
    bool Playing() { return (State == ss_Commencing || State == ss_Starting); };
    void Free();
    void Update();
    void Volume(long vol);
};

//---------------------------------------------------------------------------
#endif
