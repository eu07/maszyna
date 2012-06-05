//---------------------------------------------------------------------------
#ifndef PoliSoundH
#define PoliSoundH

#include "sound.h"
#include "realsound.h"
#include "AdvSound.h"
#include "QueryParserComp.hpp"

//typedef enum {ss_Off, ss_Starting, ss_Commencing, ss_ShuttingDown} TSoundState; - zdefiniowane w realsound

class TPoliSound
{
    TRealSound SoundStart;       //poczatkowy
    TRealSound *SoundCommencing; //rozne srodki
    TRealSound SoundShut;        //koncowy
    TSoundState State;           //stan
    double fTime;                //czas
    double fStartLength;         //czas poczatku
    double fShutLength;          //czas konca
    float soft;                  //wygladzanie przejscia
    float *freqmod;              //szybkosci
    int max_i;                   //ilosc

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

class TPoliSoundShort //to samo, tylko bez poczatku i konca
{
    TRealSound *SoundCommencing; //rozne srodki
    TSoundState State;           //stan
    float soft;                  //wygladzanie przejscia
    float *freqmod;              //szybkosci
    int max_i;                   //ilosc

public:
    __fastcall TPoliSoundShort();
    __fastcall ~TPoliSoundShort();
    void __fastcall Load(TQueryParserComp *Parser, vector3 pPosition);
    void __fastcall Stop();
    void __fastcall Free();
    void __fastcall Play(double A, double F, bool ListenerInside, vector3 NewPosition);
    void __fastcall CopyIfEmpty(TPoliSoundShort &s);
};

//---------------------------------------------------------------------------
#endif
