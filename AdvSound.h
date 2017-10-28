/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef AdvSoundH
#define AdvSoundH

#include "RealSound.h"
#include "parser.h"

enum TSoundState {

    ss_Off,
    ss_Starting,
    ss_Commencing,
    ss_ShuttingDown
};

class TAdvancedSound
{ // klasa dźwięków mających początek, dowolnie długi środek oraz zakończenie (np. Rp1)
    TRealSound SoundStart;
    TRealSound SoundCommencing;
    TRealSound SoundShut;
    TSoundState State = ss_Off;
    double fTime = 0.0;
    double fStartLength = 0.0;
    double fShutLength = 0.0;
    double defAM = 0.0;
    double defFM = 0.0;

  public:
    TAdvancedSound() = default;
    ~TAdvancedSound();
	void Init( std::string const &NameOn, std::string const &Name, std::string const &NameOff, double DistanceAttenuation, Math3D::vector3 const &pPosition);
    void Load(cParser &Parser, Math3D::vector3 const &pPosition);
    void TurnOn(bool ListenerInside, Math3D::vector3 NewPosition);
    void TurnOff(bool ListenerInside, Math3D::vector3 NewPosition);
    void Update(bool ListenerInside, Math3D::vector3 NewPosition);
    void UpdateAF(double A, double F, bool ListenerInside, Math3D::vector3 NewPosition);
    void CopyIfEmpty(TAdvancedSound &s);
};

//---------------------------------------------------------------------------
#endif
