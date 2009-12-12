//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Timer.h"
#include "FadeSound.h"


__fastcall TFadeSound::TFadeSound()
{
    Sound= NULL;
    fFade= 0;
    dt= 0;
    fTime= 0;
}


__fastcall TFadeSound::~TFadeSound()
{
    Free();
}

__fastcall TFadeSound::Free()
{
}

void __fastcall TFadeSound::Init(char *Name, float fNewFade)
{
    Sound= TSoundsManager::GetFromName(Name);
    if (Sound)
        Sound->SetVolume(0);
    fFade= fNewFade;
    fTime= 0;
}

void __fastcall TFadeSound::TurnOn()
{
    State= ss_Starting;
    Sound->Play(0,0,DSBPLAY_LOOPING);
    fTime= fFade;

}

void __fastcall TFadeSound::TurnOff()
{
    State= ss_ShuttingDown;
}

void __fastcall TFadeSound::Update()
{

    if (State==ss_Starting)
    {
        fTime+= Timer::GetDeltaTime();
//        SoundStart->SetVolume(-1000*(4-fTime)/4);
        if (fTime>=fFade)
        {
            fTime= fFade;
            State= ss_Commencing;
            Sound->SetVolume(-2000*(fFade-fTime)/fFade);
            Sound->SetFrequency(44100-500+500*(fTime)/fFade);
        }
        else
        if (Timer::GetSoundTimer())
        {
            Sound->SetVolume(-2000*(fFade-fTime)/fFade);
            Sound->SetFrequency(44100-500+500*(fTime)/fFade);
        }
    }
    else
    if (State==ss_ShuttingDown)
    {
        fTime-= Timer::GetDeltaTime();

        if (fTime<=0)
        {
            State= ss_Off;
            fTime= 0;
            Sound->Stop();
        }
        if (Timer::GetSoundTimer())
        {                            //DSBVOLUME_MIN
            Sound->SetVolume(-2000*(fFade-fTime)/fFade);
            Sound->SetFrequency(44100-500+500*fTime/fFade);
        }
    }
}
void __fastcall TFadeSound::Volume(long vol)
{
 float glos= 1;
 Sound->SetVolume(vol*glos);
}

//---------------------------------------------------------------------------

#pragma package(smart_init)


