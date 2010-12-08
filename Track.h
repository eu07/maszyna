//---------------------------------------------------------------------------

#ifndef TrackH
#define TrackH

//#include "DynObj.h"
#include "Segment.h"
//#include "QueryParserComp.hpp"
#include "parser.h"
#include "Event.h"
#include "Flags.h"
#include "ResourceManager.h"

class TEvent;

typedef enum { tt_Unknown, tt_Normal, tt_Switch, tt_Turn, tt_Cross } TTrackType;
//McZapkie-100502
typedef enum { e_unknown, e_flat, e_mountains, e_canyon, e_tunnel, e_bridge, e_bank } TEnvironmentType;

class TTrack;

const double fMaxOffset= 0.1f;

class TSwitchExtension
{//dodatkowe dane do toru, który jest zwrotnic¹
public:
    __fastcall TSwitchExtension();
    __fastcall ~TSwitchExtension();
//    vector3 p00,p01,p02,p03,p04,p05,p06,p07,p08,p09,p10,p11;
    TSegment Segments[4]; //dwa tory zwrotnicy, a pozosta³e dwa?
    TTrack *pNexts[2];
    TTrack *pPrevs[2];
    bool bNextSwitchDirection[2];
    bool bPrevSwitchDirection[2];
    int CurrentIndex;
    double fOffset1, fOffset2, fDesiredOffset1, fDesiredOffset2;
private:
};

const int iMaxNumDynamics= 40; //McZapkie-100303

const int NextMask[4]= {0,1,0,1};
const int PrevMask[4]= {0,0,1,1};

class TTrack: public Resource
{
private:
    TSwitchExtension *SwitchExtension; //dodatkowe dane do toru, który jest zwrotnic¹
//    TFlags32 Flags;
//    TSegment Segments[2];
    TSegment *Segment;
    TTrack *pNext; //odcinek od strony punktu 2
//    TTrack *pNext1;
    TTrack *pPrev; //odcinek od strony punktu 1
//    TTrack *pNext2;
//McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
    GLuint TextureID1; //tekstura szyn
    float fTexLength;
    GLuint TextureID2; //tekstura automatycznej podsypki
    float fTexHeight; //wysokoœ brzegu wzglêdem trajektorii
    float fTexWidth;
    float fTexSlope;
    vector3 *HelperPts;
    double fRadiusTable[2]; //dwa promienie, drugi dla zwrotnicy
    int iTrapezoid; //0-standard, 1-przechy³ka, 2-trapez, 3-oba
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
    inline bool __fastcall Switch(int i)
    {
        if (SwitchExtension)
        {
            SwitchExtension->fDesiredOffset1= fMaxOffset*double(NextMask[i]);
            SwitchExtension->fDesiredOffset2= fMaxOffset*double(PrevMask[i]);
            SwitchExtension->CurrentIndex= i;
            Segment= SwitchExtension->Segments+i;
            pNext= SwitchExtension->pNexts[NextMask[i]];
            pPrev= SwitchExtension->pPrevs[PrevMask[i]];
            bNextSwitchDirection= SwitchExtension->bNextSwitchDirection[NextMask[i]];
            bPrevSwitchDirection= SwitchExtension->bPrevSwitchDirection[PrevMask[i]];
            //McZapkie: wybor promienia toru:
            fRadius= fRadiusTable[i];
            return true;
        }
        Error("Cannot switch normal track");
        return false;
    };
    inline int __fastcall GetSwitchState() { return (SwitchExtension?SwitchExtension->CurrentIndex:-1); };
    void __fastcall Load(cParser *parser, vector3 pOrigin);
    bool __fastcall AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
    bool __fastcall AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
    bool __fastcall CheckDynamicObject(TDynamicObject *Dynamic)
    {
        for (int i=0; i<iNumDynamics; i++)
            if (Dynamic==Dynamics[i])
                return true;
        return false;
    };
    bool __fastcall AddDynamicObject(TDynamicObject *Dynamic);
    bool __fastcall RemoveDynamicObject(TDynamicObject *Dynamic)
    {
        for (int i=0; i<iNumDynamics; i++)
        {
            if (Dynamic==Dynamics[i])
            {
                iNumDynamics--;
                for (i; i<iNumDynamics; i++)
                    Dynamics[i]= Dynamics[i+1];
                return true;

            }
        }
        Error("Cannot remove dynamic from track");
        return false;
    }

    void MoveMe(vector3 pPosition);

    void Release();
    void Compile();

    bool __fastcall Render();
    bool __fastcall RenderAlpha();

private:
    GLuint DisplayListID;

};

//---------------------------------------------------------------------------
#endif
