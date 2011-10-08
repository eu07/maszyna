//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

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

#include "math.h"
#include "RealSound.h"
#include "Globals.h"
#include "Timer.h"

__fastcall TRealSound::TRealSound()
{
  pSound= NULL;
  dSoundAtt=-1;
  AM=0.0;
  AA=0.0;
  FM=0.0;
  FA=0.0;
  vSoundPosition.x=0;
  vSoundPosition.y=0;
  vSoundPosition.z=0;
  fDistance=fPreviousDistance=0.0;
}

__fastcall TRealSound::~TRealSound()
{
}

void __fastcall TRealSound::Free()
{
}

void __fastcall TRealSound::Init(char *SoundName, double DistanceAttenuation, double X, double Y, double Z)
{
//    Nazwa= SoundName;
    pSound= TSoundsManager::GetFromName(SoundName);
    if (pSound!=NULL)
    {
     AM= 1.0;
     pSound->SetVolume(DSBVOLUME_MIN);
    }
    else
     AM=0;
    if (DistanceAttenuation>0)
     {
      dSoundAtt=DistanceAttenuation;
      vSoundPosition.x=X;
      vSoundPosition.y=Y;
      vSoundPosition.z=Z;
     }
    else
     {
      dSoundAtt=-1;
     }
}


double __fastcall TRealSound::ListenerDistance(vector3 ListenerPosition)
{
   if (dSoundAtt==-1)
    { return 0.0; }
   else
    { return SquareMagnitude(ListenerPosition-vSoundPosition); }
}

void __fastcall TRealSound::Play(double Volume, int Looping, bool ListenerInside, vector3 NewPosition)
{
 if (!pSound) return;
 double vol;
 double dS;
// double Distance;
 DWORD stat;
 if ((Global::bSoundEnabled) && (AM!=0))
 {
   if (Volume>1)
    Volume= 1;
   fPreviousDistance=fDistance;
   fDistance= 0;
   if (dSoundAtt!=-1)
    {
      vSoundPosition= NewPosition;
      dS= dSoundAtt*dSoundAtt; //bo odleglosc podawana w kwadracie
      fDistance= ListenerDistance(Global::pCameraPosition);
      if (!ListenerInside)     //oslabianie dzwiekow
        Volume= Volume*dS/(dS+2*fDistance);
      else
        Volume= Volume*dS/(dS+fDistance);
    }
  //McZapkie-010302 - babranie tylko z niezbyt odleglymi dzwiekami
   if (dSoundAtt==-1 || fDistance<20.0*dS)
    {
//     vol=2*Volume+1; 
//     if (vol<1) vol=1;
//     vol=10000*(log(vol)-1);
//     vol=10000*(vol-1);
     int glos=1;
     vol=Volume*glos;
     if (vol<0) vol=0;
     vol=-5000+5000*vol;
     if (vol>0)
      vol=0;
     if (Timer::GetSoundTimer())
      pSound->SetVolume(vol);
     pSound->GetStatus(&stat);
      if (!(stat&DSBSTATUS_PLAYING))
       pSound->Play( 0, 0, Looping );
    }
   else //wylacz dzwiek bo daleko
    {
     pSound->GetStatus(&stat);
      if (stat&DSBSTATUS_PLAYING)
       pSound->Stop();
    }
 }
}

void __fastcall TRealSound::Stop()
{
DWORD stat;
 if ((Global::bSoundEnabled) && (AM!=0))
 {
  pSound->GetStatus(&stat);
  if (stat&DSBSTATUS_PLAYING)
   pSound->Stop();
 }
}

void __fastcall TRealSound::AdjFreq(double Freq, double dt) //McZapkie TODO: dorobic tu efekt Dopplera
//Freq moze byc liczba dodatnia mniejsza od 1 lub wieksza od 1
{
 float df, Vlist, Vsrc;
 if ((Global::bSoundEnabled) && (AM!=0))
  {
   if (dt>0)
//efekt Dopplera
    {
     Vlist=(sqrt(fPreviousDistance)-sqrt(fDistance))/dt;
     df= Freq*(1+Vlist/299.8);
    }
   else
    df=Freq; 
   if (Timer::GetSoundTimer())
    {
     df=22050.0*df; //TODO - brac czestotliwosc probkowania z wav
     pSound->SetFrequency(( df<DSBFREQUENCY_MIN ? DSBFREQUENCY_MIN : (df>DSBFREQUENCY_MAX ? DSBFREQUENCY_MAX : df) ) );
    }
  }
}

double TRealSound::GetWaveTime() //McZapkie: na razie tylko dla 22KHz/8bps
{
 if (!pSound) return 0.0;
 double WaveTime=0;
 DSBCAPS caps;
 caps.dwSize= sizeof(caps);
 pSound->GetCaps(&caps);
 WaveTime= caps.dwBufferBytes;
 return WaveTime/22000;  // wielkosc w bajtach przez czestotliwosc probkowania
}

int TRealSound::GetStatus()
{
DWORD stat;
 if ((Global::bSoundEnabled) && (AM!=0))
  {
    pSound->GetStatus(&stat);
    return stat;
  }
 else
  return 0; 
}

void __fastcall TRealSound::ResetPosition()
{
  pSound->SetCurrentPosition(0);
}

//---------------------------------------------------------------------------
#pragma package(smart_init)
