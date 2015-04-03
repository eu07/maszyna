//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Timer.h"
#include "FadeSound.h"

__fastcall TFadeSound::TFadeSound()
{
    Sound = NULL;
    fFade = 0;
    dt = 0;
    fTime = 0;
}

__fastcall TFadeSound::~TFadeSound() { Free(); }

void TFadeSound::Free() {}

void TFadeSound::Init(char *Name, float fNewFade)
{
    Sound = TSoundsManager::GetFromName(Name, false);
    if (Sound)
        Sound->SetVolume(0);
    fFade = fNewFade;
    fTime = 0;
}

void TFadeSound::TurnOn()
{
    State = ss_Starting;
    Sound->Play(0, 0, DSBPLAY_LOOPING);
    fTime = fFade;
}

void TFadeSound::TurnOff() { State = ss_ShuttingDown; }

void TFadeSound::Update()
{

    if (State == ss_Starting)
    {
        fTime += Timer::GetDeltaTime();
        //        SoundStart->SetVolume(-1000*(4-fTime)/4);
        if (fTime >= fFade)
        {
            fTime = fFade;
            State = ss_Commencing;
            Sound->SetVolume(-2000 * (fFade - fTime) / fFade);
            Sound->SetFrequency(44100 - 500 + 500 * (fTime) / fFade);
        }
        else if (Timer::GetSoundTimer())
        {
            Sound->SetVolume(-2000 * (fFade - fTime) / fFade);
            Sound->SetFrequency(44100 - 500 + 500 * (fTime) / fFade);
        }
    }
    else if (State == ss_ShuttingDown)
    {
        fTime -= Timer::GetDeltaTime();

        if (fTime <= 0)
        {
            State = ss_Off;
            fTime = 0;
            Sound->Stop();
        }
        if (Timer::GetSoundTimer())
        { // DSBVOLUME_MIN
            Sound->SetVolume(-2000 * (fFade - fTime) / fFade);
            Sound->SetFrequency(44100 - 500 + 500 * fTime / fFade);
        }
    }
}
void TFadeSound::Volume(long vol)
{
    float glos = 1;
    Sound->SetVolume(vol * glos);
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
