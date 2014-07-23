//---------------------------------------------------------------------------
#ifndef RealSoundH
#define RealSoundH

#include "Sound.h"
#include "Geometry.h"

class TRealSound
{
protected:
 PSound pSound;
 char* Nazwa;  //dla celow odwszawiania
 double fDistance,fPreviousDistance;  //dla liczenia Dopplera
 float fFrequency; //czêstotliwoœæ samplowania pliku
 int iDoppler; //Ra 2014-07: mo¿liwoœæ wy³¹czenia efektu Dopplera np. dla œpiewu ptaków
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
 void __fastcall TRealSound::Init(char *SoundName,double SoundAttenuation,double X,double Y,double Z,bool Dynamic,bool freqmod=false,double rmin=0.0);
 double __fastcall TRealSound::ListenerDistance(vector3 ListenerPosition);
 void __fastcall TRealSound::Play(double Volume,int Looping,bool ListenerInside,vector3 NewPosition);
 void __fastcall TRealSound::Stop();
 void __fastcall TRealSound::AdjFreq(double Freq,double dt);
 void __fastcall TRealSound::SetPan(int Pan); 
 double TRealSound::GetWaveTime(); //McZapkie TODO: dorobic dla roznych bps
 int TRealSound::GetStatus();
 void __fastcall TRealSound::ResetPosition();
 //void __fastcall TRealSound::FreqReset(float f=22050.0) {fFrequency=f;};
};

class TTextSound : public TRealSound
{//dŸwiêk ze stenogramem
 AnsiString asText;
 float fTime; //czas trwania
public:
 void __fastcall Init(char *SoundName,double SoundAttenuation,double X,double Y,double Z,bool Dynamic,bool freqmod=false,double rmin=0.0);
 void __fastcall Play(double Volume,int Looping,bool ListenerInside,vector3 NewPosition);
};

//---------------------------------------------------------------------------
#endif
