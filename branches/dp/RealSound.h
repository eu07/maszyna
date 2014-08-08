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
 float fFrequency; //cz�stotliwo�� samplowania pliku
 int iDoppler; //Ra 2014-07: mo�liwo�� wy��czenia efektu Dopplera np. dla �piewu ptak�w
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
{//d�wi�k ze stenogramem
 AnsiString asText;
 float fTime; //czas trwania
public:
 void __fastcall Init(char *SoundName,double SoundAttenuation,double X,double Y,double Z,bool Dynamic,bool freqmod=false,double rmin=0.0);
 void __fastcall Play(double Volume,int Looping,bool ListenerInside,vector3 NewPosition);
};

class TSynthSound
{//klasa generuj�ca sygna� odjazdu (Rp12, Rp13), potem rozbudowa� o prac� manewrowego...
 int iIndex[44]; //indeksy pocz�tkowe, gdy mamy kilka wariant�w d�wi�k�w sk�adowych
 //0..9 - cyfry 0..9
 //10..19 - liczby 10..19
 //21..29 - dziesi�tki (*21==*10?)
 //31..39 - setki 100,200,...,800,900
 //40 - "tysi�c"
 //41 - "tysi�ce"
 //42 - indeksy pocz�tkowe dla "odjazd"
 //43 - indeksy pocz�tkowe dla "got�w"
 PSound *sSound; //posortowana tablica d�wi�k�w, rozmiar zale�ny od liczby znalezionych plik�w
 //a mo�e zamiast wielu plik�w/d�wi�k�w zrobi� jeden po��czony plik i pos�ugiwa� si� czasem od..do? 
};


//---------------------------------------------------------------------------
#endif
