//---------------------------------------------------------------------------

#ifndef TrackH
#define TrackH

#include "Segment.h"
#include "parser.h"
#include "Event.h"
#include "Flags.h"
#include "ResourceManager.h"

class TEvent;

typedef enum { tt_Unknown, tt_Normal, tt_Switch, tt_Turn, tt_Cross } TTrackType;
//McZapkie-100502
typedef enum { e_unknown, e_flat, e_mountains, e_canyon, e_tunnel, e_bridge, e_bank } TEnvironmentType;

class TTrack;
class TGroundNode;

static const double fMaxOffset=0.1f;

class TSwitchExtension
{//dodatkowe dane do toru, który jest zwrotnic¹
public:
    __fastcall TSwitchExtension();
    __fastcall ~TSwitchExtension();
    TSegment Segments[4]; //dwa tory od punktu 1, pozosta³e dwa od 2?
    TTrack *pNexts[2];
    TTrack *pPrevs[2];
    bool bNextSwitchDirection[2];
    bool bPrevSwitchDirection[2];
    int CurrentIndex; //dla zwrotnicy
    double fOffset1, fDesiredOffset1; //ruch od strony punktu 1
    union
    {double fOffset2, fDesiredOffset2; //ruch od strony punktu 2 nie obs³ugiwany
     TGroundNode *pMyNode; //dla obrotnicy do wtórnego pod³¹czania torów
    };
    union
    {bool RightSwitch; //czy zwrotnica w prawo
     //TAnimContainer *pAnim; //animator modelu dla obrotnicy
     TAnimModel *pModel; //na razie model
    };
    bool bMovement; //czy w trakcie animacji
private:
};

const int iMaxNumDynamics= 40; //McZapkie-100303

const int NextMask[4]= {0,1,0,1}; //tor nastêpny dla stanów 0, 1, 2, 3
const int PrevMask[4]= {0,0,1,1}; //tor poprzedni dla stanów 0, 1, 2, 3

class TTrack: public Resource
{
private:
    TSwitchExtension *SwitchExtension; //dodatkowe dane do toru, który jest zwrotnic¹
    TSegment *Segment;
    TTrack *pNext; //odcinek od strony punktu 2
    TTrack *pPrev; //odcinek od strony punktu 1
//McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
    GLuint TextureID1; //tekstura szyn
    float fTexLength;
    GLuint TextureID2; //tekstura automatycznej podsypki
    float fTexHeight; //wysokoœ brzegu wzglêdem trajektorii
    float fTexWidth;
    float fTexSlope;
    //vector3 *HelperPts; //Ra: nie u¿ywane, na razie niech zostanie
    double fRadiusTable[2]; //dwa promienie, drugi dla zwrotnicy
    int iTrapezoid; //0-standard, 1-przechy³ka, 2-trapez, 3-oba
private:
    GLuint DisplayListID;
public:
    int iNumDynamics;
    TDynamicObject *Dynamics[iMaxNumDynamics];
    TEvent *Eventall0;  //McZapkie-140302: wyzwalany gdy pojazd stoi
    TEvent *Eventall1;
    TEvent *Eventall2;
    TEvent *Event0;  //McZapkie-280503: wyzwalany tylko gdy headdriver
    TEvent *Event1;
    TEvent *Event2;
    TEvent *EventBusy; //Ra: wyzwalane, gdy zajmowany; nazwa automatyczna
    TEvent *EventFree; //Ra: wyzwalane, gdy zwalniany; nazwa automatyczna
    AnsiString asEventall0Name; //nazwy eventów
    AnsiString asEventall1Name;
    AnsiString asEventall2Name;
    AnsiString asEvent0Name;
    AnsiString asEvent1Name;
    AnsiString asEvent2Name;
    bool bNextSwitchDirection;
    bool bPrevSwitchDirection;
    TTrackType eType;
    int iCategoryFlag;
    float fTrackWidth; //szerokoœæ w punkcie 1
    float fTrackWidth2; //szerokoœæ w punkcie 2 (g³ównie drogi i rzeki)
    float fFriction; //wspó³czynnik tarcia
    float fSoundDistance;
    int iQualityFlag;
    int iDamageFlag;
    TEnvironmentType eEnvironment; //dŸwiêk i oœwietlenie
    bool bVisible; //czy rysowany
    double fVelocity; //prêdkoœæ dla AI (powy¿ej roœnie prawdopowobieñstwo wykolejenia)
//McZapkie-100502:
    double fTrackLength; //d³ugoœæ z wpisu, nigdzie nie u¿ywana
    double fRadius; //promieñ, dla zwrotnicy kopiowany z tabeli
    bool ScannedFlag; //McZapkie: to dla testu
    __fastcall TTrack();
    __fastcall ~TTrack();
    void __fastcall Init();
    inline bool __fastcall IsEmpty() { return (iNumDynamics<=0); };
    void __fastcall ConnectPrevPrev(TTrack *pNewPrev);
    void __fastcall ConnectPrevNext(TTrack *pNewPrev);
    void __fastcall ConnectNextPrev(TTrack *pNewNext);
    void __fastcall ConnectNextNext(TTrack *pNewNext);
    inline double __fastcall Length() { return Segment->GetLength(); };
    inline TSegment* __fastcall CurrentSegment() { return Segment; };
    inline TTrack* __fastcall CurrentNext() { return (pNext); };
    inline TTrack* __fastcall CurrentPrev() { return (pPrev); };
    inline bool __fastcall SetConnections(int i)
    {
        if (SwitchExtension)
        {
            SwitchExtension->pNexts[NextMask[i]]= pNext;
            SwitchExtension->pPrevs[PrevMask[i]]= pPrev;
            SwitchExtension->bNextSwitchDirection[NextMask[i]]= bNextSwitchDirection;
            SwitchExtension->bPrevSwitchDirection[PrevMask[i]]= bPrevSwitchDirection;
            if (eType==tt_Switch)
            {
                SwitchExtension->pPrevs[PrevMask[i+2]]= pPrev;
                SwitchExtension->bPrevSwitchDirection[PrevMask[i+2]]= bPrevSwitchDirection;
            }
            Switch(0);
            return true;
        }
        Error("Cannot set connections");
        return false;
    }
    bool __fastcall Switch(int i);
    inline int __fastcall GetSwitchState() { return (SwitchExtension?SwitchExtension->CurrentIndex:-1); };
    void __fastcall Load(cParser *parser, vector3 pOrigin);
    bool __fastcall AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
    bool __fastcall AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
    bool __fastcall CheckDynamicObject(TDynamicObject *Dynamic);
    bool __fastcall AddDynamicObject(TDynamicObject *Dynamic);
    bool __fastcall RemoveDynamicObject(TDynamicObject *Dynamic);
    void __fastcall MoveMe(vector3 pPosition);

    void Release();
    void Compile();

    bool __fastcall Render();
    bool __fastcall RenderAlpha();
    bool __fastcall InMovement(); //czy w trakcie animacji?

    void __fastcall Assign(TGroundNode *gn,TAnimContainer *ac);
    void __fastcall Assign(TGroundNode *gn,TAnimModel *am);
};

//---------------------------------------------------------------------------
#endif
