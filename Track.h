/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef TrackH
#define TrackH

#include "Segment.h"
#include "ResourceManager.h"
#include "opengl/glew.h"
#include <system.hpp>
#include "Classes.h"
#include <string>

using namespace std;

class TEvent;

typedef enum
{
    tt_Unknown,
    tt_Normal,
    tt_Switch,
    tt_Table,
    tt_Cross,
    tt_Tributary
} TTrackType;
// McZapkie-100502
typedef enum
{
    e_unknown = -1,
    e_flat = 0,
    e_mountains,
    e_canyon,
    e_tunnel,
    e_bridge,
    e_bank
} TEnvironmentType;
// Ra: opracowaæ alternatywny system cieni/œwiate³ z definiowaniem koloru oœwietlenia w halach

class TTrack;
class TGroundNode;
class TSubRect;
class TTraction;

class TSwitchExtension
{ // dodatkowe dane do toru, który jest zwrotnic¹
  public:
    TSwitchExtension(TTrack *owner, int what);
    ~TSwitchExtension();
    TSegment *Segments[6]; // dwa tory od punktu 1, pozosta³e dwa od 2? Ra 140101: 6 po³¹czeñ dla
    // skrzy¿owañ
    // TTrack *trNear[4]; //tory do³¹czone do punktów 1, 2, 3 i 4
    // dotychczasowe [2]+[2] wskaŸniki zamieniæ na nowe [4]
    TTrack *pNexts[2]; // tory do³¹czone do punktów 2 i 4
    TTrack *pPrevs[2]; // tory do³¹czone do punktów 1 i 3
    int iNextDirection[2]; // to te¿ z [2]+[2] przerobiæ na [4]
    int iPrevDirection[2];
    int CurrentIndex; // dla zwrotnicy
    double fOffset, fDesiredOffset; // aktualne i docelowe po³o¿enie napêdu iglic
    double fOffsetSpeed; // prêdkoœæ liniowa ruchu iglic
    double fOffsetDelay; // opóŸnienie ruchu drugiej iglicy wzglêdem pierwszej
    union
    {
        struct
        { // zmienne potrzebne tylko dla zwrotnicy
            double fOffset1, fOffset2; // przesuniêcia iglic - 0=na wprost
            bool RightSwitch; // czy zwrotnica w prawo
        };
        struct
        { // zmienne potrzebne tylko dla obrotnicy/przesuwnicy
            TGroundNode *pMyNode; // dla obrotnicy do wtórnego pod³¹czania torów
            // TAnimContainer *pAnim; //animator modelu dla obrotnicy
            TAnimModel *pModel; // na razie model
        };
        struct
        { // zmienne dla skrzy¿owania
            vector3 *vPoints; // tablica wierzcho³ków nawierzchni, generowana przez pobocze
            int iPoints; // liczba faktycznie u¿ytych wierzcho³ków nawierzchni
            bool bPoints; // czy utworzone?
            int iRoads; // ile dróg siê spotyka?
        };
    };
    bool bMovement; // czy w trakcie animacji
    int iLeftVBO, iRightVBO; // indeksy iglic w VBO
    TSubRect *pOwner; // sektor, któremu trzeba zg³osiæ animacjê
    TTrack *pNextAnim; // nastêpny tor do animowania
    TEvent *evPlus, *evMinus; // zdarzenia sygnalizacji rozprucia
    float fVelocity; // maksymalne ograniczenie prêdkoœci (ustawianej eventem)
    vector3 vTrans; // docelowa translacja przesuwnicy
  private:
};

const int iMaxNumDynamics = 40; // McZapkie-100303

class TIsolated
{ // obiekt zbieraj¹cy zajêtoœci z kilku odcinków
    int iAxles; // iloœæ osi na odcinkach obs³ugiwanych przez obiekt
    TIsolated *pNext; // odcinki izolowane s¹ trzymane w postaci listy jednikierunkowej
    static TIsolated *pRoot; // pocz¹tek listy
  public:
    std::string asName; // nazwa obiektu, baza do nazw eventów
    TEvent *evBusy; // zdarzenie wyzwalane po zajêciu grupy
    TEvent *evFree; // zdarzenie wyzwalane po ca³kowitym zwolnieniu zajêtoœci grupy
    TMemCell *pMemCell; // automatyczna komórka pamiêci, która wspó³pracuje z odcinkiem izolowanym
    TIsolated();
    TIsolated(const string &n, TIsolated *i);
    ~TIsolated();
    static TIsolated * Find(
        const string &n); // znalezienie obiektu albo utworzenie nowego
    void Modify(int i, TDynamicObject *o); // dodanie lub odjêcie osi
    bool Busy()
    {
        return (iAxles > 0);
    };
    static TIsolated * Root()
    {
        return (pRoot);
    };
    TIsolated * Next()
    {
        return (pNext);
    };
};

class TTrack : public Resource
{ // trajektoria ruchu - opakowanie
  private:
    TSwitchExtension *SwitchExtension; // dodatkowe dane do toru, który jest zwrotnic¹
    TSegment *Segment;
    TTrack *trNext; // odcinek od strony punktu 2 - to powinno byæ w segmencie
    TTrack *trPrev; // odcinek od strony punktu 1
    // McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
    GLuint TextureID1; // tekstura szyn albo nawierzchni
    GLuint TextureID2; // tekstura automatycznej podsypki albo pobocza
    float fTexLength; // d³ugoœæ powtarzania tekstury w metrach
    float fTexRatio1; // proporcja rozmiarów tekstury dla nawierzchni drogi
    float fTexRatio2; // proporcja rozmiarów tekstury dla chodnika
    float fTexHeight1; // wysokoœæ brzegu wzglêdem trajektorii
    float fTexWidth; // szerokoœæ boku
    float fTexSlope;
    double fRadiusTable[2]; // dwa promienie, drugi dla zwrotnicy
    int iTrapezoid; // 0-standard, 1-przechy³ka, 2-trapez, 3-oba
    GLuint DisplayListID;
    TIsolated *pIsolated; // obwód izolowany obs³uguj¹cy zajêcia/zwolnienia grupy torów
    TGroundNode *
        pMyNode; // Ra: proteza, ¿eby tor zna³ swoj¹ nazwê TODO: odziedziczyæ TTrack z TGroundNode
  public:
    int iNumDynamics;
    TDynamicObject *Dynamics[iMaxNumDynamics];
    int iEvents; // Ra: flaga informuj¹ca o obecnoœci eventów
    TEvent *evEventall0; // McZapkie-140302: wyzwalany gdy pojazd stoi
    TEvent *evEventall1;
    TEvent *evEventall2;
    TEvent *evEvent0; // McZapkie-280503: wyzwalany tylko gdy headdriver
    TEvent *evEvent1;
    TEvent *evEvent2;
    string asEventall0Name; // nazwy eventów
    string asEventall1Name;
    string asEventall2Name;
    string asEvent0Name;
    string asEvent1Name;
    string asEvent2Name;
    int iNextDirection; // 0:Point1, 1:Point2, 3:do odchylonego na zwrotnicy
    int iPrevDirection;
    TTrackType eType;
    int iCategoryFlag; // 0x100 - usuwanie pojazów
    float fTrackWidth; // szerokoœæ w punkcie 1
    float fTrackWidth2; // szerokoœæ w punkcie 2 (g³ównie drogi i rzeki)
    float fFriction; // wspó³czynnik tarcia
    float fSoundDistance;
    int iQualityFlag;
    int iDamageFlag;
    TEnvironmentType eEnvironment; // dŸwiêk i oœwietlenie
    bool bVisible; // czy rysowany
    int iAction; // czy modyfikowany eventami (specjalna obs³uga przy skanowaniu)
    float fOverhead; // informacja o stanie sieci: 0-jazda bezpr¹dowa, >0-z opuszczonym i
    // ograniczeniem prêdkoœci
  private:
    double fVelocity; // prêdkoœæ dla AI (powy¿ej roœnie prawdopowobieñstwo wykolejenia)
  public:
    // McZapkie-100502:
    double fTrackLength; // d³ugoœæ z wpisu, nigdzie nie u¿ywana
    double fRadius; // promieñ, dla zwrotnicy kopiowany z tabeli
    bool ScannedFlag; // McZapkie: do zaznaczania kolorem torów skanowanych przez AI
    TTraction *hvOverhead; // drut zasilaj¹cy do szybkiego znalezienia (nie u¿ywany)
    TGroundNode *nFouling[2]; // wspó³rzêdne ukresu albo oporu koz³a
    TTrack *trColides; // tor kolizyjny, na którym trzeba sprawdzaæ pojazdy pod k¹tem zderzenia

    TTrack(TGroundNode *g);
    ~TTrack();
    void Init();
    static TTrack * Create400m(int what, double dx);
    TTrack * NullCreate(int dir);
    inline bool IsEmpty()
    {
        return (iNumDynamics <= 0);
    };
    void ConnectPrevPrev(TTrack *pNewPrev, int typ);
    void ConnectPrevNext(TTrack *pNewPrev, int typ);
    void ConnectNextPrev(TTrack *pNewNext, int typ);
    void ConnectNextNext(TTrack *pNewNext, int typ);
    inline double Length()
    {
        return Segment->GetLength();
    };
    inline TSegment * CurrentSegment()
    {
        return Segment;
    };
    inline TTrack * CurrentNext()
    {
        return (trNext);
    };
    inline TTrack * CurrentPrev()
    {
        return (trPrev);
    };
    TTrack * Neightbour(int s, double &d);
    bool SetConnections(int i);
    bool Switch(int i, double t = -1.0, double d = -1.0);
    bool SwitchForced(int i, TDynamicObject *o);
    int CrossSegment(int from, int into);
    inline int GetSwitchState()
    {
        return (SwitchExtension ? SwitchExtension->CurrentIndex : -1);
    };
    void Load(cParser *parser, vector3 pOrigin, std::string name);
    bool AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
    bool AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2);
    bool AssignForcedEvents(TEvent *NewEventPlus, TEvent *NewEventMinus);
    bool CheckDynamicObject(TDynamicObject *Dynamic);
    bool AddDynamicObject(TDynamicObject *Dynamic);
    bool RemoveDynamicObject(TDynamicObject *Dynamic);
    void MoveMe(vector3 pPosition);

    void Release();
    void Compile(GLuint tex = 0);

    void Render(); // renderowanie z Display Lists
    int RaArrayPrepare(); // zliczanie rozmiaru dla VBO sektroa
    void RaArrayFill(CVertNormTex *Vert, const CVertNormTex *Start); // wype³nianie VBO
    void RaRenderVBO(int iPtr); // renderowanie z VBO sektora
    void RenderDyn(); // renderowanie nieprzezroczystych pojazdów (oba tryby)
    void RenderDynAlpha(); // renderowanie przezroczystych pojazdów (oba tryby)
    void RenderDynSounds(); // odtwarzanie dŸwiêków pojazdów jest niezale¿ne od ich
    // wyœwietlania

    void RaOwnerSet(TSubRect *o)
    {
        if (SwitchExtension)
            SwitchExtension->pOwner = o;
    };
    bool InMovement(); // czy w trakcie animacji?
    void RaAssign(TGroundNode *gn, TAnimContainer *ac);
    void RaAssign(TGroundNode *gn, TAnimModel *am, TEvent *done, TEvent *joined);
    void RaAnimListAdd(TTrack *t);
    TTrack * RaAnimate();

    void RadioStop();
    void AxleCounter(int i, TDynamicObject *o)
    {
        if (pIsolated)
            pIsolated->Modify(i, o);
    }; // dodanie lub odjêcie osi
    string IsolatedName();
    bool IsolatedEventsAssign(TEvent *busy, TEvent *free);
    double WidthTotal();
    GLuint TextureGet(int i)
    {
        return i ? TextureID1 : TextureID2;
    };
    bool IsGroupable();
    int TestPoint(vector3 *Point);
    void MovedUp1(double dh);
    string NameGet();
    void VelocitySet(float v);
    float VelocityGet();
    void ConnectionsLog();

  private:
    void EnvironmentSet();
    void EnvironmentReset();
};

//---------------------------------------------------------------------------
#endif
