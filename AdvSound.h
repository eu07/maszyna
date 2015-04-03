//---------------------------------------------------------------------------
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
    __fastcall TAdvancedSound();
    __fastcall ~TAdvancedSound();
    void __fastcall Init(char *NameOn, char *Name, char *NameOff, double DistanceAttenuation,
                         vector3 pPosition);
    void __fastcall Load(TQueryParserComp *Parser, vector3 pPosition);
    void __fastcall TurnOn(bool ListenerInside, vector3 NewPosition);
    void __fastcall TurnOff(bool ListenerInside, vector3 NewPosition);
    void __fastcall Free();
    void __fastcall Update(bool ListenerInside, vector3 NewPosition);
    void __fastcall UpdateAF(double A, double F, bool ListenerInside, vector3 NewPosition);
    void __fastcall CopyIfEmpty(TAdvancedSound &s);
};

//---------------------------------------------------------------------------
#endif
