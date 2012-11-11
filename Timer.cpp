//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Timer.h"
#include "Globals.h"

namespace Timer {

double DeltaTime,DeltaRenderTime;
double fFPS      =0.0f;
double fLastTime =0.0f;
DWORD dwFrames  =0L;
double fSimulationTime=0;
double fSoundTimer=0;
double fSinceStart=0;



double __fastcall GetTime()
{
    return fSimulationTime;
}

double __fastcall GetDeltaTime()
{//czas symulacji (stoi gdy pauza)
 return DeltaTime;
}

double __fastcall GetDeltaRenderTime()
{//czas renderowania (do poruszania siê)
 return DeltaRenderTime;
}

double __fastcall GetfSinceStart()
{
    return fSinceStart;
}

void __fastcall SetDeltaTime(double t)
{
    DeltaTime= t;
}

double __fastcall GetSimulationTime()
{
 return fSimulationTime;
}

void __fastcall SetSimulationTime(double t)
{
    fSimulationTime=t;
}

bool __fastcall GetSoundTimer()
{
 return (fSoundTimer==0.0f);
}


double __fastcall GetFPS()
{
 return fFPS;
}

void __fastcall ResetTimers()
{
 //double CurrentTime=
 GetTickCount();
 DeltaTime=0.1;
 DeltaRenderTime=0;
 fSoundTimer=0;
};

LONGLONG fr,count,oldCount;
//LARGE_INTEGER fr,count;
void __fastcall UpdateTimers(bool pause)
{
 QueryPerformanceFrequency((LARGE_INTEGER*)&fr);
 QueryPerformanceCounter((LARGE_INTEGER*)&count);
 DeltaRenderTime=double(count-oldCount)/double(fr);
 if (!pause)
 {DeltaTime=Global::fSunSpeed*DeltaRenderTime;
  fSoundTimer+=DeltaTime;
  if (fSoundTimer>0.1) fSoundTimer=0;
/*
  double CurrentTime= double(count)/double(fr);//GetTickCount();
  DeltaTime= (CurrentTime-OldTime);
  OldTime= CurrentTime;
*/
  if (DeltaTime>1) DeltaTime=1;
 }
 else
  DeltaTime=0; //wszystko stoi, bo czas nie p³ynie
 oldCount=count;

 // Keep track of the time lapse and frame count
 double fTime=GetTickCount()*0.001f; // Get current time in seconds
 ++dwFrames; //licznik ramek
 //update the frame rate once per second
 if (fTime-fLastTime>1.0f)
 {
  fFPS     =dwFrames/(fTime-fLastTime);
  fLastTime=fTime;
  dwFrames =0L;
 }
 fSimulationTime+=DeltaTime;
};



};

//---------------------------------------------------------------------------

#pragma package(smart_init)
 