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

const int conditional_memstring     =        1; //por�wnanie tekstu
const int conditional_memval1       =        2; //por�wnanie pierwszej warto�ci liczbowej
const int conditional_memval2       =        4; //por�wnanie drugiej warto�ci
const int conditional_trackoccupied =    0x100; //je�li tor zaj�ty
const int conditional_trackfree     =    0x200; //je�li tor wolny
const int conditional_propability   =    0x400; //zale�nie od generatora lizcb losowych
const int conditional_memcompare    =    0x800; //por�wnanie zawarto�ci
const int conditional_else          =  0x10000; //flaga odwr�cenia warunku (przesuwana bitowo)
const int conditional_anyelse       = 0xFF0000; //do sprawdzania, czy s� odwr�cone warunki
const int update_memstring          =0x1000000; //zmodyfikowa� tekst (UpdateValues)
const int update_memval1            =0x2000000; //zmodyfikowa� pierwsz� wartos�
const int update_memval2            =0x4000000; //zmodyfikowa� drug� wartos�
const int update_memadd             =0x8000000; //doda� do poprzedniej zawarto�ci

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
 void __fastcall Conditions(cParser* parser,AnsiString s);
public:
 AnsiString asName;
 bool bEnabled; //false gdy ma nie by� dodawany do kolejki (skanowanie sygna��w)
 int iQueued; //ile razy dodany do kolejki
 //bool bIsHistory;
 TEvent *Next; //nast�pny w kolejce
 TEvent *Next2;
 TEventType Type;
 double fStartTime;
 double fDelay;
 TDynamicObject *Activator;
 TParam Params[13]; //McZapkie-070502 //Ra: zamieni� to na union/struct
 int iFlags; //zamiast Params[8] z flagami warunku
 AnsiString asNodeName; //McZapkie-100302 - dodalem zeby zapamietac nazwe toru
 TEvent *eJoined; //kolejny event z t� sam� nazw� - od wersji 378
public: //metody
 __fastcall TEvent();
 __fastcall ~TEvent();
 void __fastcall Init();
 void __fastcall Load(cParser* parser,vector3 *org);
 void __fastcall AddToQuery(TEvent *Event);
 AnsiString __fastcall CommandGet();
 double __fastcall ValueGet(int n);
 vector3 __fastcall PositionGet();
 bool __fastcall StopCommand();
 void __fastcall StopCommandSent();
 void __fastcall Append(TEvent *e);
};

//---------------------------------------------------------------------------
#endif
