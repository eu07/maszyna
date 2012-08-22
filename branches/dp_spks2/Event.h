//---------------------------------------------------------------------------

#ifndef EventH
#define EventH

#include "Classes.h"
#include "dumb3d.h"
using namespace Math3D;

typedef enum { tp_Unknown, tp_Sound, tp_SoundPos, tp_Exit,
               tp_Disable, tp_Velocity, tp_Animation, tp_Lights,
               tp_UpdateValues, tp_GetValues, tp_PutValues,
               tp_Switch, tp_DynVel, tp_TrackVel, tp_Multiple,
               tp_AddValues, tp_Ignored, tp_CopyValues, tp_WhoIs,
               tp_LogValues, tp_Visible
             }  TEventType;

const int conditional_memstring=1;
const int conditional_memval1=2;
const int conditional_memval2=4;
const int conditional_memadd=8; //dodanie do poprzedniej zawarto�ci
const int conditional_trackoccupied=0x100; //do u�ywania z & nie moze by� ujemne
const int conditional_trackfree=0x200;
const int conditional_propability=0x400;

union TParam
{
    void *asPointer;
    TMemCell *asMemCell;
    TGroundNode *asGroundNode;
    TTrack *asTrack;
    TAnimModel *asModel;
    TAnimContainer *asAnimContainer;
    TTrain *asTrain;
    TDynamicObject *asDynamic;
    TEvent *asEvent;
    bool asBool;
    double asdouble;
    int asInt;
    TRealSound *asRealSound;
    char *asText;
};

class TEvent
{
private:

public:
    AnsiString asName;
    bool bEnabled; //false gdy ma nie by� dodawany do kolejki (skanowanie sygna��w)
    bool bLaunched;
    bool bIsHistory;
    TEvent *Next;
    TEvent *Next2;
    TEventType Type;
    double fStartTime;
    double fDelay;
    TDynamicObject *Activator;

    TParam Params[13]; //McZapkie-070502 //Ra: zamieni� to na union/struct

    AnsiString asNodeName;
//McZapkie-100302 - dodalem zeby zapamietac nazwe toru
//    AnsiString asNodeName2;

    __fastcall TEvent();
    __fastcall ~TEvent();
    void __fastcall Init();
    void __fastcall Load(cParser* parser,vector3 *org);
    void __fastcall AddToQuery(TEvent *Event);
 AnsiString __fastcall CommandGet();
 double __fastcall ValueGet(int n);
 vector3 __fastcall PositionGet();
};

//---------------------------------------------------------------------------
#endif
