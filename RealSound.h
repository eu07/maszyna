//---------------------------------------------------------------------------
#ifndef RealSoundH
#define RealSoundH

#include "sound.h"
#include "Geometry.h"

class TRealSound
{
private:
 PSound pSound;
 char* Nazwa;  //dla celow odwszawiania
 double fDistance,fPreviousDistance;  //dla liczenia Dopplera
 float fFrequency; //czêstotliwoœæ samplowania pliku
public:
 vector3 vSoundPosition;      //polozenie zrodla dzwieku
 double dSoundAtt;  //odleglosc polowicznego zaniku dzwieku
 double AM;         //mnoznik amplitudy
 double AA;         //offset amplitudy
 double FM;         //mnoznik czestotliwosci
 double FA;         //offset czestotliwosci
 __fastcall TRealSound::TRealSound();
 __fastcall TRealSound::~TRealSound();
 void __fastcall TRealSound::Free();
 void __fastcall TRealSound::Init(char *SoundName,double SoundAttenuation,double X,double Y,double Z,bool Dynamic);
 double __fastcall TRealSound::ListenerDistance(vector3 ListenerPosition);
 void __fastcall TRealSound::Play(double Volume,int Looping,bool ListenerInside,vector3 NewPosition);
 void __fastcall TRealSound::Stop();
 void __fastcall TRealSound::AdjFreq(double Freq,double dt);
 void __fastcall TRealSound::SetPan(int Pan); 
 double TRealSound::GetWaveTime(); //McZapkie TODO: dorobic dla roznych bps
 int TRealSound::GetStatus();
 void __fastcall TRealSound::ResetPosition();
};

//---------------------------------------------------------------------------
#endif
