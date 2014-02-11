//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "math.h"
#include "RealSound.h"
#include "Globals.h"
#include "Timer.h"
#include "Logs.h"
#include "McZapkie\mctools.hpp"

__fastcall TRealSound::TRealSound()
{
 pSound=NULL;
 dSoundAtt=-1;
 AM=0.0;
 AA=0.0;
 FM=0.0;
 FA=0.0;
 vSoundPosition.x=0;
 vSoundPosition.y=0;
 vSoundPosition.z=0;
 fDistance=fPreviousDistance=0.0;
 fFrequency=22050.0; //czêstotliwoœæ samplowania pliku
}

__fastcall TRealSound::~TRealSound()
{
 //if (this) if (pSound) pSound->Stop();
}

void __fastcall TRealSound::Free()
{
}

void __fastcall TRealSound::Init(char *SoundName, double DistanceAttenuation, double X, double Y, double Z,bool Dynamic,bool freqmod)
{
 //Nazwa=SoundName; //to tak raczej nie zadzia³a, (SoundName) jest tymczasowe
 pSound=TSoundsManager::GetFromName(SoundName,Dynamic,&fFrequency);
 if (pSound)
 {
  if (freqmod)
   if (fFrequency!=22050.0)
   {//dla modulowanych nie mo¿e byæ zmiany mno¿nika, bo czêstotliwoœæ w nag³ówku by³¹ ignorowana, a mog³a byæ inna ni¿ 22050
    fFrequency=22050.0;
    ErrorLog("Bad sound: "+AnsiString(SoundName)+", as modulated, should have 22.05kHz in header");
   }
  AM=1.0;
  pSound->SetVolume(DSBVOLUME_MIN);
 }
 else
 {//nie ma dŸwiêku, to jest wysyp
  AM=0;
  ErrorLog("Missed file: "+AnsiString(SoundName));
 }
 if (DistanceAttenuation>0.0)
 {
  dSoundAtt=DistanceAttenuation*DistanceAttenuation;
  vSoundPosition.x=X;
  vSoundPosition.y=Y;
  vSoundPosition.z=Z;
 }
 else
  dSoundAtt=-1;
};


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
 long int vol;
 double dS;
// double Distance;
 DWORD stat;
 if ((Global::bSoundEnabled)&&(AM!=0))
 {
  if (Volume>1.0)
   Volume=1.0;
  fPreviousDistance=fDistance;
  fDistance=0.0; //??
  if (dSoundAtt>0.0)
  {
   vSoundPosition=NewPosition;
   dS=dSoundAtt; //*dSoundAtt; //bo odleglosc podawana w kwadracie
   fDistance=ListenerDistance(Global::pCameraPosition);
   if (ListenerInside) //os³abianie dŸwiêków z odleg³oœci¹
    Volume=Volume*dS/(dS+fDistance);
   else
    Volume=Volume*dS/(dS+2*fDistance); //podwójne dla ListenerInside=false
  }
  if (FreeFlyModeFlag) //gdy swobodne latanie
   fPreviousDistance=fDistance; //to efektu Dopplera nie bêdzie
  //McZapkie-010302 - babranie tylko z niezbyt odleglymi dŸwiêkami
  if ((dSoundAtt==-1)||(fDistance<20.0*dS))
  {
//   vol=2*Volume+1;
//   if (vol<1) vol=1;
//   vol=10000*(log(vol)-1);
//   vol=10000*(vol-1);
   //int glos=1;
   //Volume=Volume*glos; //Ra: whatta hella is this
   if (Volume<0.0) Volume=0.0;
   vol=-5000.0+5000.0*Volume;
   if (vol>=0)
    vol=-1;
   if (Timer::GetSoundTimer()||!Looping) //Ra: po co to jest?
    pSound->SetVolume(vol); //Attenuation, in hundredths of a decibel (dB).
   pSound->GetStatus(&stat);
   if (!(stat&DSBSTATUS_PLAYING))
    pSound->Play(0,0,Looping);
  }
  else //wylacz dzwiek bo daleko
  {
/* Ra: stara wersja, ale podobno lepsza */
   pSound->GetStatus(&stat);
   if (stat&DSBSTATUS_PLAYING)
    pSound->Stop();
//*/
/* Ra: wy³¹czy³em, bo podobno jest gorzej ni¿ wczeœniej
   //ZiomalCl: dŸwiêk po wy³¹czeniu sam siê nie w³¹czy, gdy wrócimy w rejon odtwarzania
   pSound->SetVolume(DSBVOLUME_MIN); //dlatego lepiej go wyciszyæ na czas oddalenia siê
   pSound->GetStatus(&stat);
   if (!(stat&DSBSTATUS_PLAYING))
    pSound->Play(0,0,Looping); //ZiomalCl: w³¹czenie odtwarzania rownie¿ i tu, gdy¿ jesli uruchamiamy dŸwiêk poza promieniem, nie uruchomi siê on w ogóle
*/
  }
 }
};

void __fastcall TRealSound::Stop()
{
 DWORD stat;
 if (pSound)
  if ((Global::bSoundEnabled)&&(AM!=0))
  {
   pSound->GetStatus(&stat);
   if (stat&DSBSTATUS_PLAYING)
    pSound->Stop();
  }
};

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
     df=fFrequency*df; //TODO - brac czestotliwosc probkowania z wav
     pSound->SetFrequency(( df<DSBFREQUENCY_MIN ? DSBFREQUENCY_MIN : (df>DSBFREQUENCY_MAX ? DSBFREQUENCY_MAX : df) ) );
    }
  }
}

double TRealSound::GetWaveTime() //McZapkie: na razie tylko dla 22KHz/8bps
{//u¿ywana do pomiaru czasu dla dŸwiêków z pocz¹tkiem i koñcem
 if (!pSound) return 0.0;
 double WaveTime;
 DSBCAPS caps;
 caps.dwSize=sizeof(caps);
 pSound->GetCaps(&caps);
 WaveTime=caps.dwBufferBytes;
 return WaveTime/fFrequency;  // wielkosc w bajtach przez czestotliwosc probkowania
}

void __fastcall TRealSound::SetPan(int Pan)
{
  pSound->SetPan(Pan);
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
 if (pSound) //Ra: znowu jakiœ badziew!
  pSound->SetCurrentPosition(0);
}

void __fastcall TTextSound::Init(char *SoundName,double SoundAttenuation,double X,double Y,double Z,bool Dynamic,bool freqmod)
{//dodatkowo doczytuje plik tekstowy
 TRealSound::Init(SoundName,SoundAttenuation,X,Y,Z,Dynamic,freqmod);
 AnsiString txt=AnsiString(SoundName);
 txt.Delete(txt.Length()-3,4); //obciêcie rozszerzenia
 txt+="-pl.wav"; //na razie po polsku
 if (FileExists(txt))
 {//wczytanie
  TFileStream *ts=new TFileStream(txt,fmOpenRead);
  asText=AnsiString::StringOfChar(' ',ts->Size);
  ts->Read(asText.c_str(),ts->Size);
  delete ts;
 }
};

//---------------------------------------------------------------------------
#pragma package(smart_init)

