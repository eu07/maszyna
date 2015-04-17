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

#include "Sound.h"
#include "Geometry.h"

class TRealSound
{
  protected:
    PSound pSound;
    char *Nazwa; // dla celow odwszawiania
    double fDistance, fPreviousDistance; // dla liczenia Dopplera
    float fFrequency; // czêstotliwoœæ samplowania pliku
    int iDoppler; // Ra 2014-07: mo¿liwoœæ wy³¹czenia efektu Dopplera np. dla œpiewu ptaków
  public:
    vector3 vSoundPosition; // polozenie zrodla dzwieku
    double dSoundAtt; // odleglosc polowicznego zaniku dzwieku
    double AM; // mnoznik amplitudy
    double AA; // offset amplitudy
    double FM; // mnoznik czestotliwosci
    double FA; // offset czestotliwosci
    bool bLoopPlay; // czy zapêtlony dŸwiêk jest odtwarzany
    TRealSound();
    ~TRealSound();
    void Free();
    void Init(char *SoundName, double SoundAttenuation, double X, double Y, double Z,
                         bool Dynamic, bool freqmod = false, double rmin = 0.0);
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
};

class TTextSound : public TRealSound
{ // dŸwiêk ze stenogramem
    AnsiString asText;
    float fTime; // czas trwania
  public:
    void Init(char *SoundName, double SoundAttenuation, double X, double Y, double Z,
                         bool Dynamic, bool freqmod = false, double rmin = 0.0);
    void Play(double Volume, int Looping, bool ListenerInside, vector3 NewPosition);
};

class TSynthSound
{ // klasa generuj¹ca sygna³ odjazdu (Rp12, Rp13), potem rozbudowaæ o pracê manewrowego...
    int iIndex[44]; // indeksy pocz¹tkowe, gdy mamy kilka wariantów dŸwiêków sk³adowych
    // 0..9 - cyfry 0..9
    // 10..19 - liczby 10..19
    // 21..29 - dziesi¹tki (*21==*10?)
    // 31..39 - setki 100,200,...,800,900
    // 40 - "tysi¹c"
    // 41 - "tysi¹ce"
    // 42 - indeksy pocz¹tkowe dla "odjazd"
    // 43 - indeksy pocz¹tkowe dla "gotów"
    PSound *sSound; // posortowana tablica dŸwiêków, rozmiar zale¿ny od liczby znalezionych plików
    // a mo¿e zamiast wielu plików/dŸwiêków zrobiæ jeden po³¹czony plik i pos³ugiwaæ siê czasem
    // od..do?
};

//---------------------------------------------------------------------------
#endif
