/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "stdafx.h"
#include "RealSound.h"
#include "Globals.h"
#include "Logs.h"
//#include "math.h"
#include "Timer.h"
#include "mczapkie/mctools.h"
#include "usefull.h"

TRealSound::TRealSound(std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z, bool Dynamic,
	bool freqmod, double rmin)
{
	Init(SoundName, SoundAttenuation, X, Y, Z, Dynamic, freqmod, rmin);
}

TRealSound::~TRealSound()
{
    // if (this) if (pSound) pSound->Stop();
}

void TRealSound::Free()
{
}

void TRealSound::Init(std::string const &SoundName, double DistanceAttenuation, double X, double Y, double Z,
                      bool Dynamic, bool freqmod, double rmin)
{
    // Nazwa=SoundName; //to tak raczej nie zadziała, (SoundName) jest tymczasowe
    pSound = TSoundsManager::GetFromName(SoundName, Dynamic, &fFrequency);
    if (pSound)
    {
        if (freqmod)
            if (fFrequency != 22050.0)
            { // dla modulowanych nie może być zmiany mnożnika, bo częstotliwość w nagłówku byłą
                // ignorowana, a mogła być inna niż 22050
                fFrequency = 22050.0;
                ErrorLog("Bad sound: " + SoundName + ", as modulated, should have 22.05kHz in header");
            }
        AM = 1.0;
        pSound->SetVolume(DSBVOLUME_MIN);
    }
    else
    { // nie ma dźwięku, to jest wysyp
        AM = 0;
        ErrorLog("Missed sound: " + SoundName);
    }
    if (DistanceAttenuation > 0.0)
    {
        dSoundAtt = DistanceAttenuation * DistanceAttenuation;
        vSoundPosition.x = X;
        vSoundPosition.y = Y;
        vSoundPosition.z = Z;
        if (rmin < 0)
            iDoppler = 1; // wyłączenie efektu Dopplera, np. dla dźwięku ptaków
    }
    else
        dSoundAtt = -1;
};

double TRealSound::ListenerDistance(vector3 ListenerPosition)
{
    if (dSoundAtt == -1)
    {
        return 0.0;
    }
    else
    {
        return SquareMagnitude(ListenerPosition - vSoundPosition);
    }
}

void TRealSound::Play(double Volume, int Looping, bool ListenerInside, vector3 NewPosition)
{
    if (!pSound)
        return;
    long int vol;
	double dS = 0.0;
    // double Distance;
    DWORD stat;
    if ((Global::bSoundEnabled) && (AM != 0))
    {
        if (Volume > 1.0)
            Volume = 1.0;
        fPreviousDistance = fDistance;
        fDistance = 0.0; //??
        if (dSoundAtt > 0.0)
        {
            vSoundPosition = NewPosition;
            dS = dSoundAtt; //*dSoundAtt; //bo odleglosc podawana w kwadracie
            fDistance = ListenerDistance(Global::pCameraPosition);
            if (ListenerInside) // osłabianie dźwięków z odległością
                Volume = Volume * dS / (dS + fDistance);
            else
                Volume = Volume * dS / (dS + 2 * fDistance); // podwójne dla ListenerInside=false
        }
        if (iDoppler) //
        { // Ra 2014-07: efekt Dopplera nie zawsze jest wskazany
            // if (FreeFlyModeFlag) //gdy swobodne latanie - nie sprawdza się to
            fPreviousDistance = fDistance; // to efektu Dopplera nie będzie
        }
        if (Looping) // dźwięk zapętlony można wyłączyć i zostanie włączony w miarę potrzeby
            bLoopPlay = true; // dźwięk wyłączony
        // McZapkie-010302 - babranie tylko z niezbyt odleglymi dźwiękami
        if ((dSoundAtt == -1) || (fDistance < 20.0 * dS))
        {
            //   vol=2*Volume+1;
            //   if (vol<1) vol=1;
            //   vol=10000*(log(vol)-1);
            //   vol=10000*(vol-1);
            // int glos=1;
            // Volume=Volume*glos; //Ra: whatta hella is this
            if (Volume < 0.0)
                Volume = 0.0;
            vol = -5000.0 + 5000.0 * Volume;
            if (vol >= 0)
                vol = -1;
            if (Timer::GetSoundTimer() || !Looping) // Ra: po co to jest?
                pSound->SetVolume(vol); // Attenuation, in hundredths of a decibel (dB).
            pSound->GetStatus(&stat);
            if (!(stat & DSBSTATUS_PLAYING))
                pSound->Play(0, 0, Looping);
        }
        else // wylacz dzwiek bo daleko
        { // Ra 2014-09: oddalanie się nie może być powodem do wyłączenie dźwięku
            /*
            // Ra: stara wersja, ale podobno lepsza
               pSound->GetStatus(&stat);
               if (bLoopPlay) //jeśli zapętlony, to zostanie ponownie włączony, o ile znajdzie się
            bliżej
                if (stat&DSBSTATUS_PLAYING)
                 pSound->Stop();
            // Ra: wyłączyłem, bo podobno jest gorzej niż wcześniej
               //ZiomalCl: dźwięk po wyłączeniu sam się nie włączy, gdy wrócimy w rejon odtwarzania
               pSound->SetVolume(DSBVOLUME_MIN); //dlatego lepiej go wyciszyć na czas oddalenia się
               pSound->GetStatus(&stat);
               if (!(stat&DSBSTATUS_PLAYING))
                pSound->Play(0,0,Looping); //ZiomalCl: włączenie odtwarzania rownież i tu, gdyż
            jesli uruchamiamy dźwięk poza promieniem, nie uruchomi się on w ogóle
            */
        }
    }
};

void TRealSound::Start(){
    // włączenie dźwięku

};

void TRealSound::Stop()
{
    DWORD stat;
    if (pSound)
        if ((Global::bSoundEnabled) && (AM != 0))
        {
            bLoopPlay = false; // dźwięk wyłączony
            pSound->GetStatus(&stat);
            if (stat & DSBSTATUS_PLAYING)
                pSound->Stop();
        }
};

void TRealSound::AdjFreq(double Freq, double dt) // McZapkie TODO: dorobic tu efekt Dopplera
// Freq moze byc liczba dodatnia mniejsza od 1 lub wieksza od 1
{
	float df, Vlist;
    if ((Global::bSoundEnabled) && (AM != 0) && (pSound != nullptr))
    {
        if (dt > 0)
        // efekt Dopplera
        {
            Vlist = (sqrt(fPreviousDistance) - sqrt(fDistance)) / dt;
            df = Freq * (1 + Vlist / 299.8);
        }
        else
            df = Freq;
        if (Timer::GetSoundTimer())
        {
            df = fFrequency * df; // TODO - brac czestotliwosc probkowania z wav
            pSound->SetFrequency( clamp( df, static_cast<float>(DSBFREQUENCY_MIN), static_cast<float>(DSBFREQUENCY_MAX) ) );
        }
    }
}

double TRealSound::GetWaveTime() // McZapkie: na razie tylko dla 22KHz/8bps
{ // używana do pomiaru czasu dla dźwięków z początkiem i końcem
    if (!pSound)
        return 0.0;
    double WaveTime;
    DSBCAPS caps;
    caps.dwSize = sizeof(caps);
    pSound->GetCaps(&caps);
    WaveTime = caps.dwBufferBytes;
    return WaveTime /
           fFrequency; //(pSound->);  // wielkosc w bajtach przez czestotliwosc probkowania
}

void TRealSound::SetPan(int Pan)
{
    pSound->SetPan(Pan);
}

int TRealSound::GetStatus()
{
    DWORD stat;
    if ((Global::bSoundEnabled) && (AM != 0))
    {
        pSound->GetStatus(&stat);
        return stat;
    }
    else
        return 0;
}

void TRealSound::ResetPosition()
{
    if (pSound) // Ra: znowu jakiś badziew!
        pSound->SetCurrentPosition(0);
}

TTextSound::TTextSound(std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z,
	bool Dynamic, bool freqmod, double rmin)
	: TRealSound(SoundName, SoundAttenuation, X, Y, Z, Dynamic, freqmod, rmin)
{
	Init(SoundName, SoundAttenuation, X, Y, Z, Dynamic, freqmod, rmin);
}

void TTextSound::Init(std::string const &SoundName, double SoundAttenuation, double X, double Y, double Z,
                      bool Dynamic, bool freqmod, double rmin)
{ // dodatkowo doczytuje plik tekstowy
    //TRealSound::Init(SoundName, SoundAttenuation, X, Y, Z, Dynamic, freqmod, rmin);
    fTime = GetWaveTime();
    std::string txt(SoundName);
	txt.erase( txt.rfind( '.' ) ); // obcięcie rozszerzenia
    for (size_t i = txt.length(); i > 0; --i)
        if (txt[i] == '/')
            txt[i] = '\\'; // bo nie rozumi
    txt += "-" + Global::asLang + ".txt"; // już może być w różnych językach
    if (!FileExists(txt))
        txt = "sounds\\" + txt; //ścieżka może nie być podana
    if (FileExists(txt))
    { // wczytanie
/*      TFileStream *ts = new TFileStream(txt, fmOpenRead);
        asText = AnsiString::StringOfChar(' ', ts->Size);
        ts->Read(asText.c_str(), ts->Size);
        delete ts;
*/		std::ifstream inputfile( txt );
		asText.assign( std::istreambuf_iterator<char>( inputfile ), std::istreambuf_iterator<char>() );
	}
};
void TTextSound::Play(double Volume, int Looping, bool ListenerInside, vector3 NewPosition)
{
    if (false == asText.empty())
    { // jeśli ma powiązany tekst
        DWORD stat;
        pSound->GetStatus(&stat);
        if (!(stat & DSBSTATUS_PLAYING)) {
            // jeśli nie jest aktualnie odgrywany
            Global::tranTexts.Add( asText, fTime, true );
        }
    }
    TRealSound::Play(Volume, Looping, ListenerInside, NewPosition);
};

//---------------------------------------------------------------------------
