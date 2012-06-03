//---------------------------------------------------------------------------
#ifndef PoliSoundH
#define PoliSoundH

#include "sound.h"
#include "realsound.h"
#include "AdvSound.h"
#include "QueryParserComp.hpp"

//typedef enum {ss_Off, ss_Starting, ss_Commencing, ss_ShuttingDown} TSoundState;

class TPoliSound
{
    TRealSound SoundStart;
    TRealSound SoundCommencing[6];
    TRealSound SoundShut;
    TSoundState State;
    double fTime;
    double fStartLength;
    double fShutLength;
    double defAM;
    double defFM;
    float freqmod[6];
    int max_i;

public:
    __fastcall TPoliSound();
    __fastcall ~TPoliSound();
    void __fastcall Load(TQueryParserComp *Parser, vector3 pPosition);
    void __fastcall TurnOn(bool ListenerInside, vector3 NewPosition);
    void __fastcall TurnOff(bool ListenerInside, vector3 NewPosition);
    void __fastcall Free();
    void __fastcall UpdateAF(double A, double F, bool ListenerInside, vector3 NewPosition);
    void __fastcall CopyIfEmpty(TPoliSound &s);
};

//---------------------------------------------------------------------------
#endif
