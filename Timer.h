//---------------------------------------------------------------------------

#ifndef TimerH
#define TimerH

namespace Timer
{

double GetTime();

double GetDeltaTime();
double GetDeltaRenderTime();

double GetfSinceStart();

void SetDeltaTime(double v);

double GetSimulationTime();

void SetSimulationTime(double v);

bool GetSoundTimer();

double GetFPS();

void ResetTimers();

void UpdateTimers(bool pause);
};

//---------------------------------------------------------------------------
#endif
