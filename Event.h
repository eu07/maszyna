/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef EventH
#define EventH

#include "Classes.h"
#include "dumb3d.h"
using namespace Math3D;

typedef enum
{
    tp_Unknown,
    tp_Sound,
    tp_SoundPos,
    tp_Exit,
    tp_Disable,
    tp_Velocity,
    tp_Animation,
    tp_Lights,
    tp_UpdateValues,
    tp_GetValues,
    tp_PutValues,
    tp_Switch,
    tp_DynVel,
    tp_TrackVel,
    tp_Multiple,
    tp_AddValues,
    tp_Ignored,
    tp_CopyValues,
    tp_WhoIs,
    tp_LogValues,
    tp_Visible,
    tp_Voltage,
    tp_Message,
    tp_Friction
} TEventType;

const int update_memstring = 0x0000001; // zmodyfikowaæ tekst (UpdateValues)
const int update_memval1 = 0x0000002; // zmodyfikowaæ pierwsz¹ wartosæ
const int update_memval2 = 0x0000004; // zmodyfikowaæ drug¹ wartosæ
const int update_memadd = 0x0000008; // dodaæ do poprzedniej zawartoœci
const int update_load = 0x0000010; // odczytaæ ³adunek
const int update_only = 0x00000FF; // wartoœæ graniczna
const int conditional_memstring = 0x0000100; // porównanie tekstu
const int conditional_memval1 = 0x0000200; // porównanie pierwszej wartoœci liczbowej
const int conditional_memval2 = 0x0000400; // porównanie drugiej wartoœci
const int conditional_else = 0x0010000; // flaga odwrócenia warunku (przesuwana bitowo)
const int conditional_anyelse = 0x0FF0000; // do sprawdzania, czy s¹ odwrócone warunki
const int conditional_trackoccupied = 0x1000000; // jeœli tor zajêty
const int conditional_trackfree = 0x2000000; // jeœli tor wolny
const int conditional_propability = 0x4000000; // zale¿nie od generatora lizcb losowych
const int conditional_memcompare = 0x8000000; // porównanie zawartoœci

union TParam
{
    void *asPointer;
    TMemCell *asMemCell;
    TGroundNode *nGroundNode;
    TTrack *asTrack;
    TAnimModel *asModel;
    TAnimContainer *asAnimContainer;
    TTrain *asTrain;
    TDynamicObject *asDynamic;
    TEvent *asEvent;
    bool asBool;
    double asdouble;
    int asInt;
    TTextSound *tsTextSound;
    char *asText;
    TCommandType asCommand;
    TTractionPowerSource *psPower;
};

class TEvent // zmienne: ev*
{ // zdarzenie
  private:
    void Conditions(cParser *parser, AnsiString s);

  public:
    std::string asName;
    bool bEnabled; // false gdy ma nie byæ dodawany do kolejki (skanowanie sygna³ów)
    int iQueued; // ile razy dodany do kolejki
    // bool bIsHistory;
    TEvent *evNext; // nastêpny w kolejce
    TEvent *evNext2;
    TEventType Type;
    double fStartTime;
    double fDelay;
    TDynamicObject *Activator;
    TParam Params[13]; // McZapkie-070502 //Ra: zamieniæ to na union/struct
    unsigned int iFlags; // zamiast Params[8] z flagami warunku
    AnsiString asNodeName; // McZapkie-100302 - dodalem zeby zapamietac nazwe toru
    TEvent *evJoined; // kolejny event z t¹ sam¹ nazw¹ - od wersji 378
    double fRandomDelay; // zakres dodatkowego opóŸnienia
  public: // metody
    TEvent(AnsiString m = "");
    ~TEvent();
    void Init();
    void Load(cParser *parser, vector3 *org);
    void AddToQuery(TEvent *e);
    AnsiString CommandGet();
    TCommandType Command();
    double ValueGet(int n);
    vector3 PositionGet();
    bool StopCommand();
    void StopCommandSent();
    void Append(TEvent *e);
};

//---------------------------------------------------------------------------
#endif
