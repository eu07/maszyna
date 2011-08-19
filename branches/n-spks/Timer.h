//---------------------------------------------------------------------------

#ifndef TimerH
#define TimerH

namespace Timer { 


double __fastcall GetTime();

double __fastcall GetDeltaTime();
double __fastcall GetDeltaRenderTime();

double __fastcall GetfSinceStart();

void __fastcall SetDeltaTime(double v);

double __fastcall GetSimulationTime();

void __fastcall SetSimulationTime(double v);

bool __fastcall GetSoundTimer();

double __fastcall GetFPS();

void __fastcall ResetTimers();

void __fastcall UpdateTimers(bool pause);

};

//---------------------------------------------------------------------------
#endif
