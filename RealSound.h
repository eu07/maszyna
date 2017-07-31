/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef RealSoundH
#define RealSoundH

#include <string>
#include "Sound.h"
#include "Geometry.h"

class TRealSound
{
  protected:
    PSound pSound = nullptr;
//  char *Nazwa; // dla celow odwszawiania NOTE: currently not used anywhere
    double fDistance = 0.0,
           fPreviousDistance = 0.0; // dla liczenia Dopplera
    float fFrequency = 22050.0; // częstotliwość samplowania pliku
    int iDoppler = 0; // Ra 2014-07: możliwość wyłączenia efektu Dopplera np. dla śpiewu ptaków
  public:
    vector3 vSoundPosition; // polozenie zrodla dzwieku
    double dSoundAtt = -1.0; // odleglosc polowicznego zaniku dzwieku
    double AM = 0.0; // mnoznik amplitudy
    double AA = 0.0; // offset amplitudy
    double FM = 0.0; // mnoznik czestotliwosci
    double FA = 0.0; // offset czestotliwosci
    bool bLoopPlay = false; // czy zapętlony dźwięk jest odtwarzany
    TRealSound() = default;
	TRealSound( std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z, bool Dynamic,
		bool freqmod = false, double rmin = 0.0);
    ~TRealSound();
    void Free();
    void Init(std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z, bool Dynamic,
              bool freqmod = false, double rmin = 0.0);
    double ListenerDistance(vector3 ListenerPosition);
    void Play(double Volume, int Looping, bool ListenerInside, vector3 NewPosition);
    void Start();
    void Stop();
    void AdjFreq(double Freq, double dt);
    void SetPan(int Pan);
    double GetWaveTime(); // McZapkie TODO: dorobic dla roznych bps
    int GetStatus();
    void ResetPosition();
    // void FreqReset(float f=22050.0) {fFrequency=f;};
    bool Empty() { return ( pSound == nullptr ); }
};

class TTextSound : public TRealSound
{ // dźwięk ze stenogramem
    std::string asText;
    float fTime; // czas trwania
  public:
    TTextSound(std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z,
               bool Dynamic, bool freqmod = false, double rmin = 0.0);
    void Init(std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z,
              bool Dynamic, bool freqmod = false, double rmin = 0.0);
    void Play(double Volume, int Looping, bool ListenerInside, vector3 NewPosition);
};

class TSynthSound
{ // klasa generująca sygnał odjazdu (Rp12, Rp13), potem rozbudować o pracę manewrowego...
    int iIndex[44]; // indeksy początkowe, gdy mamy kilka wariantów dźwięków składowych
    // 0..9 - cyfry 0..9
    // 10..19 - liczby 10..19
    // 21..29 - dziesiątki (*21==*10?)
    // 31..39 - setki 100,200,...,800,900
    // 40 - "tysiąc"
    // 41 - "tysiące"
    // 42 - indeksy początkowe dla "odjazd"
    // 43 - indeksy początkowe dla "gotów"
    PSound *sSound; // posortowana tablica dźwięków, rozmiar zależny od liczby znalezionych plików
    // a może zamiast wielu plików/dźwięków zrobić jeden połączony plik i posługiwać się czasem
    // od..do?
};

//---------------------------------------------------------------------------
#endif
