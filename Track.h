//---------------------------------------------------------------------------

#ifndef TrackH
#define TrackH

#include "Segment.h"
#include "ResourceManager.h"
#include "opengl/glew.h"
#include <system.hpp>
#include "Classes.h"


class TEvent;

typedef enum { tt_Unknown, tt_Normal, tt_Switch, tt_Turn, tt_Cross, tt_Tributary } TTrackType;
//McZapkie-100502
typedef enum {e_unknown=-1, e_flat=0, e_mountains, e_canyon, e_tunnel, e_bridge, e_bank} TEnvironmentType;
//Ra: opracowaæ alternatywny system cieni/œwiate³ z definiowaniem koloru oœwietlenia w halach

class TTrack;
class TGroundNode;
class TSubRect;
class TTraction;

class TSwitchExtension
{//dodatkowe dane do toru, który jest zwrotnic¹
public:
 __fastcall TSwitchExtension(TTrack *owner);
 __fastcall ~TSwitchExtension();
 TSegment *Segments[4]; //dwa tory od punktu 1, pozosta³e dwa od 2?
 TTrack *pNexts[2];
 TTrack *pPrevs[2];
 bool iNextDirection[2];
 bool iPrevDirection[2];
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
 int iLeftVBO,iRightVBO; //indeksy iglic w VBO
 TSubRect *pOwner; //sektor, któremu trzeba zg³osiæ animacjê
 TTrack *pNextAnim; //nastêpny tor do animowania
 TEvent *EventPlus,*EventMinus; //zdarzenia sygnalizacji rozprucia
private:
};

const int iMaxNumDynamics=40; //McZapkie-100303

class TIsolated
{//obiekt zbieraj¹cy zajêtoœci z kilku odcinków
 int iAxles; //iloœæ osi na odcinkach obs³ugiwanych przez obiekt
 TIsolated *pNext;
 static TIsolated *pRoot;
public:
 AnsiString asName; //nazwa obiektu, baza do nazw eventów
 TEvent *eBusy; //zdarzenie wyzwalane po zajêciu grupy
 TEvent *eFree; //zdarzenie wyzwalane po ca³kowitym zwolnieniu zajêtoœci grupy
 __fastcall TIsolated();
 __fastcall TIsolated(const AnsiString &n,TIsolated *i);
 __fastcall ~TIsolated();
 static TIsolated* __fastcall Find(const AnsiString &n); //znalezienie obiektu albo utworzenie nowego
 void __fastcall Modify(int i,TDynamicObject *o); //dodanie lub odjêcie osi
};

class TTrack : public Resource
{//trajektoria ruchu - opakowanie
private:
 TSwitchExtension *SwitchExtension; //dodatkowe dane do toru, który jest zwrotnic¹
 TSegment *Segment;
 TTrack *pNext; //odcinek od strony punktu 2 - to powinno byæ w segmencie
 TTrack *pPrev; //odcinek od strony punktu 1
 //McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
 GLuint TextureID1; //tekstura szyn albo nawierzchni
 GLuint TextureID2; //tekstura automatycznej podsypki albo pobocza
 float fTexLength; //d³ugoœæ powtarzania tekstury w metrach
 float fTexRatio1; //proporcja rozmiarów tekstury dla nawierzchni drogi
 float fTexRatio2; //proporcja rozmiarów tekstury dla chodnika
 float fTexHeight1; //wysokoœæ brzegu wzglêdem trajektorii
 float fTexWidth; //szerokoœæ boku
 float fTexSlope;
 double fRadiusTable[2]; //dwa promienie, drugi dla zwrotnicy
 int iTrapezoid; //0-standard, 1-przechy³ka, 2-trapez, 3-oba
 GLuint DisplayListID;
 TIsolated *pIsolated; //obwód izolowany obs³uguj¹cy zajêcia/zwolnienia grupy torów
 TGroundNode *pMyNode; //Ra: proteza, ¿eby tor zna³ swoj¹ nazwê TODO: odziedziczyæ TTrack z TGroundNode
public:
 int iNumDynamics;
 TDynamicObject *Dynamics[iMaxNumDynamics];
 int iEvents; //Ra: flaga informuj¹ca o obecnoœci eventów
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
 int iNextDirection; //0:Point1, 1:Point2, 3:do odchylonego na zwrotnicy
 int iPrevDirection;
 TTrackType eType;
 int iCategoryFlag; //0x100 - usuwanie pojazów
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
 bool ScannedFlag; //McZapkie: do zaznaczania kolorem torów skanowanych przez AI
 TTraction *pTraction; //drut zasilaj¹cy

 __fastcall TTrack(TGroundNode *g);
 __fastcall ~TTrack();
 void __fastcall Init();
 TTrack* __fastcall NullCreate(int dir);
 inline bool __fastcall IsEmpty() { return (iNumDynamics<=0); };
 void __fastcall ConnectPrevPrev(TTrack *pNewPrev,int typ);
 void __fastcall ConnectPrevNext(TTrack *pNewPrev,int typ);
 void __fastcall ConnectNextPrev(TTrack *pNewNext,int typ);
 void __fastcall ConnectNextNext(TTrack *pNewNext,int typ);
 inline double __fastcall Length() { return Segment->GetLength(); };
 inline TSegment* __fastcall CurrentSegment() { return Segment; };
 inline TTrack* __fastcall CurrentNext() {return (pNext);};
 inline TTrack* __fastcall CurrentPrev() {return (pPrev);};
 bool __fastcall SetConnections(int i);
 bool __fastcall Switch(int i);
 bool __fastcall SwitchForced(int i,TDynamicObject *o);
 inline int __fastcall GetSwitchState() { return (SwitchExtension?SwitchExtension->CurrentIndex:-1); };
 void __fastcall Load(cParser *parser, vector3 pOrigin,AnsiString name);
 bool __fastcall AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
 bool __fastcall AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
 bool __fastcall AssignForcedEvents(TEvent *NewEventPlus, TEvent *NewEventMinus);
 bool __fastcall CheckDynamicObject(TDynamicObject *Dynamic);
 bool __fastcall AddDynamicObject(TDynamicObject *Dynamic);
 bool __fastcall RemoveDynamicObject(TDynamicObject *Dynamic);
 void __fastcall MoveMe(vector3 pPosition);

 void Release();
 void __fastcall Compile(GLuint tex=0);

 void __fastcall Render(); //renderowanie z Display Lists
 int __fastcall RaArrayPrepare(); //zliczanie rozmiaru dla VBO sektroa
 void  __fastcall RaArrayFill(CVertNormTex *Vert,const CVertNormTex *Start); //wype³nianie VBO
 void  __fastcall RaRenderVBO(int iPtr); //renderowanie z VBO sektora
 void __fastcall RenderDyn(); //renderowanie nieprzezroczystych pojazdów (oba tryby)
 void __fastcall RenderDynAlpha(); //renderowanie przezroczystych pojazdów (oba tryby)

 void __fastcall RaOwnerSet(TSubRect *o)
 {if (SwitchExtension) SwitchExtension->pOwner=o;};
 bool __fastcall InMovement(); //czy w trakcie animacji?
 void __fastcall RaAssign(TGroundNode *gn,TAnimContainer *ac);
 void __fastcall RaAssign(TGroundNode *gn,TAnimModel *am);
 void __fastcall RaAnimListAdd(TTrack *t);
 TTrack* __fastcall RaAnimate();

 void __fastcall RadioStop();
 void __fastcall AxleCounter(int i,TDynamicObject *o)
 {if (pIsolated) pIsolated->Modify(i,o);}; //dodanie lub odjêcie osi
 AnsiString __fastcall IsolatedName();
 bool __fastcall IsolatedEventsAssign(TEvent *busy, TEvent *free);
 double __fastcall WidthTotal();
 GLuint TextureGet(int i) {return i?TextureID1:TextureID2;};
 bool __fastcall IsGroupable();
 int __fastcall TestPoint(vector3 *Point);
 void __fastcall MovedUp1(double dh);
private:
 void __fastcall EnvironmentSet();
 void __fastcall EnvironmentReset();
};

//---------------------------------------------------------------------------
#endif
