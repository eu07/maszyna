/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Timer.h"
#include "Globals.h"

namespace Timer
{

double DeltaTime, DeltaRenderTime;
double fFPS{ 0.0f };
double fLastTime{ 0.0f };
DWORD dwFrames{ 0 };
double fSimulationTime{ 0.0 };
double fSoundTimer{ 0.0 };
double fSinceStart{ 0.0 };

double GetTime()
{
    return fSimulationTime;
}

double GetDeltaTime()
{ // czas symulacji (stoi gdy pauza)
    return DeltaTime;
}

double GetDeltaRenderTime()
{ // czas renderowania (do poruszania się)
    return DeltaRenderTime;
}

double GetfSinceStart()
{
    return fSinceStart;
}

void SetDeltaTime(double t)
{
    DeltaTime = t;
}

bool GetSoundTimer()
{ // Ra: być może, by dźwięki nie modyfikowały się zbyt często, po 0.1s zeruje się ten licznik
    return (fSoundTimer == 0.0f);
}

double GetFPS()
{
    return fFPS;
}

void ResetTimers()
{
    UpdateTimers( Global::iPause != 0 );
    DeltaTime = 0.1;
    DeltaRenderTime = 0.0;
    fSoundTimer = 0.0;
};

uint64_t fr, count, oldCount;

void UpdateTimers(bool pause)
{
#ifdef _WIN32
    QueryPerformanceFrequency((LARGE_INTEGER *)&fr);
    QueryPerformanceCounter((LARGE_INTEGER *)&count);
#elif __linux__
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	count = (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
	fr = 1000000000;
#endif
    DeltaRenderTime = double(count - oldCount) / double(fr);
    if (!pause)
    {
        DeltaTime = Global::fTimeSpeed * DeltaRenderTime;
        fSoundTimer += DeltaTime;
        if (fSoundTimer > 0.1)
            fSoundTimer = 0.0;

        if (DeltaTime > 1.0)
            DeltaTime = 1.0;
    }
    else
        DeltaTime = 0.0; // wszystko stoi, bo czas nie płynie

    oldCount = count;
    // Keep track of the time lapse and frame count
#if __linux__
	double fTime = (double)(count / 1000000000);
#elif _WIN32_WINNT >= _WIN32_WINNT_VISTA
    double fTime = ::GetTickCount64() * 0.001f; // Get current time in seconds
#elif _WIN32
    double fTime = ::GetTickCount() * 0.001f; // Get current time in seconds
#endif
    ++dwFrames; // licznik ramek
    // update the frame rate once per second
    if (fTime - fLastTime > 1.0f)
    {
        fFPS = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames = 0L;
    }
    fSimulationTime += DeltaTime;
};

}; // namespace timer

//---------------------------------------------------------------------------
