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
//Ra: opracowa� alternatywny system cieni/�wiate� z definiowaniem koloru o�wietlenia w halach

class TTrack;
class TGroundNode;
class TSubRect;
class TTraction;

class TSwitchExtension
{//dodatkowe dane do toru, kt�ry jest zwrotnic�
public:
 __fastcall TSwitchExtension(TTrack *owner);
 __fastcall ~TSwitchExtension();
 TSegment *Segments[4]; //dwa tory od punktu 1, pozosta�e dwa od 2?
 TTrack *pNexts[2];
 TTrack *pPrevs[2];
 bool iNextDirection[2];
 bool iPrevDirection[2];
 int CurrentIndex; //dla zwrotnicy
 double fOffset1, fDesiredOffset1; //ruch od strony punktu 1
 union
 {double fOffset2, fDesiredOffset2; //ruch od strony punktu 2 nie obs�ugiwany
  TGroundNode *pMyNode; //dla obrotnicy do wt�rnego pod��czania tor�w
 };
 union
 {bool RightSwitch; //czy zwrotnica w prawo
  //TAnimContainer *pAnim; //animator modelu dla obrotnicy
  TAnimModel *pModel; //na razie model
 };
 bool bMovement; //czy w trakcie animacji
 int iLeftVBO,iRightVBO; //indeksy iglic w VBO
 TSubRect *pOwner; //sektor, kt�remu trzeba zg�osi� animacj�
 TTrack *pNextAnim; //nast�pny tor do animowania
 TEvent *EventPlus,*EventMinus; //zdarzenia sygnalizacji rozprucia
private:
};

const int iMaxNumDynamics=40; //McZapkie-100303

class TIsolated
{//obiekt zbieraj�cy zaj�to�ci z kilku odcink�w
 int iAxles; //ilo�� osi na odcinkach obs�ugiwanych przez obiekt
 TIsolated *pNext;
 static TIsolated *pRoot;
public:
 AnsiString asName; //nazwa obiektu, baza do nazw event�w
 TEvent *eBusy; //zdarzenie wyzwalane po zaj�ciu grupy
 TEvent *eFree; //zdarzenie wyzwalane po ca�kowitym zwolnieniu zaj�to�ci grupy
 __fastcall TIsolated();
 __fastcall TIsolated(const AnsiString &n,TIsolated *i);
 __fastcall ~TIsolated();
 static TIsolated* __fastcall Find(const AnsiString &n); //znalezienie obiektu albo utworzenie nowego
 void __fastcall Modify(int i,TDynamicObject *o); //dodanie lub odj�cie osi
};

class TTrack : public Resource
{//trajektoria ruchu - opakowanie
private:
 TSwitchExtension *SwitchExtension; //dodatkowe dane do toru, kt�ry jest zwrotnic�
 TSegment *Segment;
 TTrack *pNext; //odcinek od strony punktu 2 - to powinno by� w segmencie
 TTrack *pPrev; //odcinek od strony punktu 1
 //McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
 GLuint TextureID1; //tekstura szyn albo nawierzchni
 GLuint TextureID2; //tekstura automatycznej podsypki albo pobocza
 float fTexLength; //d�ugo�� powtarzania tekstury w metrach
 float fTexRatio1; //proporcja rozmiar�w tekstury dla nawierzchni drogi
 float fTexRatio2; //proporcja rozmiar�w tekstury dla chodnika
 float fTexHeight1; //wysoko�� brzegu wzgl�dem trajektorii
 float fTexWidth; //szeroko�� boku
 float fTexSlope;
 double fRadiusTable[2]; //dwa promienie, drugi dla zwrotnicy
 int iTrapezoid; //0-standard, 1-przechy�ka, 2-trapez, 3-oba
 GLuint DisplayListID;
 TIsolated *pIsolated; //obw�d izolowany obs�uguj�cy zaj�cia/zwolnienia grupy tor�w
 TGroundNode *pMyNode; //Ra: proteza, �eby tor zna� swoj� nazw� TODO: odziedziczy� TTrack z TGroundNode
public:
 int iNumDynamics;
 TDynamicObject *Dynamics[iMaxNumDynamics];
 int iEvents; //Ra: flaga informuj�ca o obecno�ci event�w
 TEvent *Eventall0;  //McZapkie-140302: wyzwalany gdy pojazd stoi
 TEvent *Eventall1;
 TEvent *Eventall2;
 TEvent *Event0;  //McZapkie-280503: wyzwalany tylko gdy headdriver
 TEvent *Event1;
 TEvent *Event2;
 TEvent *EventBusy; //Ra: wyzwalane, gdy zajmowany; nazwa automatyczna
 TEvent *EventFree; //Ra: wyzwalane, gdy zwalniany; nazwa automatyczna
 AnsiString asEventall0Name; //nazwy event�w
 AnsiString asEventall1Name;
 AnsiString asEventall2Name;
 AnsiString asEvent0Name;
 AnsiString asEvent1Name;
 AnsiString asEvent2Name;
 int iNextDirection; //0:Point1, 1:Point2, 3:do odchylonego na zwrotnicy
 int iPrevDirection;
 TTrackType eType;
 int iCategoryFlag; //0x100 - usuwanie pojaz�w
 float fTrackWidth; //szeroko�� w punkcie 1
 float fTrackWidth2; //szeroko�� w punkcie 2 (g��wnie drogi i rzeki)
 float fFriction; //wsp�czynnik tarcia
 float fSoundDistance;
 int iQualityFlag;
 int iDamageFlag;
 TEnvironmentType eEnvironment; //d�wi�k i o�wietlenie
 bool bVisible; //czy rysowany
 double fVelocity; //pr�dko�� dla AI (powy�ej ro�nie prawdopowobie�stwo wykolejenia)
 //McZapkie-100502:
 double fTrackLength; //d�ugo�� z wpisu, nigdzie nie u�ywana
 double fRadius; //promie�, dla zwrotnicy kopiowany z tabeli
 bool ScannedFlag; //McZapkie: do zaznaczania kolorem tor�w skanowanych przez AI
 TTraction *pTraction; //drut zasilaj�cy

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
 void  __fastcall RaArrayFill(CVertNormTex *Vert,const CVertNormTex *Start); //wype�nianie VBO
 void  __fastcall RaRenderVBO(int iPtr); //renderowanie z VBO sektora
 void __fastcall RenderDyn(); //renderowanie nieprzezroczystych pojazd�w (oba tryby)
 void __fastcall RenderDynAlpha(); //renderowanie przezroczystych pojazd�w (oba tryby)

 void __fastcall RaOwnerSet(TSubRect *o)
 {if (SwitchExtension) SwitchExtension->pOwner=o;};
 bool __fastcall InMovement(); //czy w trakcie animacji?
 void __fastcall RaAssign(TGroundNode *gn,TAnimContainer *ac);
 void __fastcall RaAssign(TGroundNode *gn,TAnimModel *am);
 void __fastcall RaAnimListAdd(TTrack *t);
 TTrack* __fastcall RaAnimate();

 void __fastcall RadioStop();
 void __fastcall AxleCounter(int i,TDynamicObject *o)
 {if (pIsolated) pIsolated->Modify(i,o);}; //dodanie lub odj�cie osi
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
