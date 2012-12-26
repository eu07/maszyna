//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Timer.h"
#include "AdvSound.h"

__fastcall TAdvancedSound::TAdvancedSound()
{
//    SoundStart=SoundCommencing=SoundShut= NULL;
    State= ss_Off;
    fTime= 0;
    fStartLength= 0;;
    fShutLength= 0;;
}

__fastcall TAdvancedSound::~TAdvancedSound()
{
}

void __fastcall TAdvancedSound::Free()
{
}

void __fastcall TAdvancedSound::Init(char *NameOn, char *Name, char *NameOff, double DistanceAttenuation, vector3 pPosition)
{
    SoundStart.Init(NameOn,DistanceAttenuation,pPosition.x,pPosition.y,pPosition.z,true);
    SoundCommencing.Init(Name,DistanceAttenuation,pPosition.x,pPosition.y,pPosition.z,true);
    SoundShut.Init(NameOff,DistanceAttenuation,pPosition.x,pPosition.y,pPosition.z,true);
    fStartLength= SoundStart.GetWaveTime();
    fShutLength= SoundShut.GetWaveTime();
    SoundStart.AM=1.0;
    SoundStart.AA=0.0;
    SoundStart.FM=1.0;
    SoundStart.FA=0.0;
    SoundCommencing.AM=1.0;
    SoundCommencing.AA=0.0;
    SoundCommencing.FM=1.0;
    SoundCommencing.FA=0.0;
    defAM=1.0;
    defFM=1.0;
    SoundShut.AM=1.0;
    SoundShut.AA=0.0;
    SoundShut.FM=1.0;
    SoundShut.FA=0.0;
}

void __fastcall TAdvancedSound::Load(TQueryParserComp *Parser, vector3 pPosition)
{
  AnsiString NameOn= Parser->GetNextSymbol().LowerCase();
  AnsiString Name= Parser->GetNextSymbol().LowerCase();
  AnsiString NameOff= Parser->GetNextSymbol().LowerCase();
  double DistanceAttenuation= Parser->GetNextSymbol().ToDouble();
  Init(NameOn.c_str(),Name.c_str(),NameOff.c_str(),DistanceAttenuation,pPosition);
}

void __fastcall TAdvancedSound::TurnOn(bool ListenerInside, vector3 NewPosition)
{
    if ((State==ss_Off) && (SoundStart.AM>0))
    {
        SoundStart.ResetPosition();
        SoundCommencing.ResetPosition();
        SoundStart.Play(1,0,ListenerInside,NewPosition);
//        SoundStart->SetVolume(-10000);
        State= ss_Starting;
        fTime= 0;
    }
}

void __fastcall TAdvancedSound::TurnOff(bool ListenerInside, vector3 NewPosition)
{
    if ((State==ss_Commencing || State==ss_Starting)  && (SoundShut.AM>0))
    {
        SoundStart.Stop();
        SoundCommencing.Stop();
        SoundShut.ResetPosition();
        SoundShut.Play(1,0,ListenerInside,NewPosition);
        State= ss_ShuttingDown;
        fTime= fShutLength;
//        SoundShut->SetVolume(0);
    }
}

void __fastcall TAdvancedSound::Update(bool ListenerInside, vector3 NewPosition)
{
    if ((State==ss_Commencing)  && (SoundCommencing.AM>0))
    {
//        SoundCommencing->SetFrequency();
        SoundCommencing.Play(1,DSBPLAY_LOOPING,ListenerInside,NewPosition);
    }
    else
    if (State==ss_Starting)
    {
        fTime+= Timer::GetDeltaTime();
//        SoundStart->SetVolume(-1000*(4-fTime)/4);
        if (fTime>=fStartLength)
        {
            State= ss_Commencing;
            SoundCommencing.ResetPosition();
            SoundCommencing.Play(1,DSBPLAY_LOOPING,ListenerInside,NewPosition);
            SoundStart.Stop();

        }
        else
           SoundStart.Play(1,0,ListenerInside,NewPosition);
    }
    else
    if (State==ss_ShuttingDown)
    {
        fTime-= Timer::GetDeltaTime();
//        SoundShut->SetVolume(-1000*(4-fTime)/4);
        if (fTime<=0)
        {
            State= ss_Off;
            SoundShut.Stop();
        }
        else
         SoundShut.Play(1,0,ListenerInside,NewPosition);
    }
}

void __fastcall TAdvancedSound::UpdateAF(double A, double F, bool ListenerInside, vector3 NewPosition)
{   //update, ale z amplituda i czestotliwoscia
    if ((State==ss_Commencing)  && (SoundCommencing.AM>0))
    {
        SoundCommencing.Play(A,DSBPLAY_LOOPING,ListenerInside,NewPosition);
    }
    else
    if (State==ss_Starting)
    {
        fTime+= Timer::GetDeltaTime();
//        SoundStart->SetVolume(-1000*(4-fTime)/4);
        if (fTime>=fStartLength)
        {
            State= ss_Commencing;
            SoundCommencing.ResetPosition();
            SoundCommencing.Play(A,DSBPLAY_LOOPING,ListenerInside,NewPosition);
            SoundStart.Stop();
        }
        else
           SoundStart.Play(A,0,ListenerInside,NewPosition);
    }
    else
    if (State==ss_ShuttingDown)
    {
        fTime-= Timer::GetDeltaTime();
//        SoundShut->SetVolume(-1000*(4-fTime)/4);
        if (fTime<=0)
        {
            State= ss_Off;
            SoundShut.Stop();
        }
        else
         SoundShut.Play(A,0,ListenerInside,NewPosition);
    }
    SoundCommencing.AdjFreq(F,Timer::GetDeltaTime());
}

//---------------------------------------------------------------------------
#pragma package(smart_init)
