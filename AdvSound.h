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

#include "Sound.h"
#include "RealSound.h"
#include "QueryParserComp.hpp"

typedef enum
{
    ss_Off,
    ss_Starting,
    ss_Commencing,
    ss_ShuttingDown
} TSoundState;

class TAdvancedSound
{ // klasa dŸwiêków maj¹cych pocz¹tek, dowolnie d³ugi œrodek oraz zakoñczenie (np. Rp1)
    TRealSound SoundStart;
    TRealSound SoundCommencing;
    TRealSound SoundShut;
    TSoundState State;
    double fTime;
    double fStartLength;
    double fShutLength;
    double defAM;
    double defFM;

  public:
    TAdvancedSound();
    ~TAdvancedSound();
    void Init(char *NameOn, char *Name, char *NameOff, double DistanceAttenuation,
              vector3 pPosition);
    void Load(TQueryParserComp *Parser, vector3 pPosition);
    void TurnOn(bool ListenerInside, vector3 NewPosition);
    void TurnOff(bool ListenerInside, vector3 NewPosition);
    void Free();
    void Update(bool ListenerInside, vector3 NewPosition);
    void UpdateAF(double A, double F, bool ListenerInside, vector3 NewPosition);
    void CopyIfEmpty(TAdvancedSound &s);
};

//---------------------------------------------------------------------------
#endif
