/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include "opengl/glew.h"
#include "ResourceManager.h"
#include "Segment.h"

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
// Ra: opracować alternatywny system cieni/świateł z definiowaniem koloru oświetlenia w halach

class TTrack;
class TGroundNode;
class TSubRect;
class TTraction;

class TSwitchExtension
{ // dodatkowe dane do toru, który jest zwrotnicą
  public:
    TSwitchExtension(TTrack *owner, int what);
    ~TSwitchExtension();
    std::shared_ptr<TSegment> Segments[6]; // dwa tory od punktu 1, pozosta³e dwa od 2? Ra 140101: 6 po³¹czeñ dla
    // skrzy¿owañ
    // TTrack *trNear[4]; //tory do³¹czone do punktów 1, 2, 3 i 4
    // dotychczasowe [2]+[2] wskaŸniki zamieniæ na nowe [4]
    TTrack *pNexts[2]; // tory do³¹czone do punktów 2 i 4
    TTrack *pPrevs[2]; // tory do³¹czone do punktów 1 i 3
    int iNextDirection[2]; // to te¿ z [2]+[2] przerobiæ na [4]
    int iPrevDirection[2];
    int CurrentIndex; // dla zwrotnicy
    double fOffset, fDesiredOffset; // aktualne i docelowe położenie napędu iglic
    double fOffsetSpeed; // prędkość liniowa ruchu iglic
    double fOffsetDelay; // opóźnienie ruchu drugiej iglicy względem pierwszej
    union
    {
        struct
        { // zmienne potrzebne tylko dla zwrotnicy
            double fOffset1, fOffset2; // przesunięcia iglic - 0=na wprost
            bool RightSwitch; // czy zwrotnica w prawo
        };
        struct
        { // zmienne potrzebne tylko dla obrotnicy/przesuwnicy
            TGroundNode *pMyNode; // dla obrotnicy do wtórnego podłączania torów
            // TAnimContainer *pAnim; //animator modelu dla obrotnicy
            TAnimModel *pModel; // na razie model
        };
        struct
        { // zmienne dla skrzyżowania
            vector3 *vPoints; // tablica wierzchołków nawierzchni, generowana przez pobocze
            int iPoints; // liczba faktycznie użytych wierzchołków nawierzchni
            bool bPoints; // czy utworzone?
            int iRoads; // ile dróg się spotyka?
        };
    };
    bool bMovement; // czy w trakcie animacji
    int iLeftVBO, iRightVBO; // indeksy iglic w VBO
    TSubRect *pOwner; // sektor, któremu trzeba zgłosić animację
    TTrack *pNextAnim; // następny tor do animowania
    TEvent *evPlus, *evMinus; // zdarzenia sygnalizacji rozprucia
    float fVelocity; // maksymalne ograniczenie prędkości (ustawianej eventem)
    vector3 vTrans; // docelowa translacja przesuwnicy
  private:
};

const int iMaxNumDynamics = 40; // McZapkie-100303

class TIsolated
{ // obiekt zbierający zajętości z kilku odcinków
    int iAxles; // ilość osi na odcinkach obsługiwanych przez obiekt
    TIsolated *pNext; // odcinki izolowane są trzymane w postaci listy jednikierunkowej
    static TIsolated *pRoot; // początek listy
  public:
    std::string asName; // nazwa obiektu, baza do nazw eventów
    TEvent *evBusy; // zdarzenie wyzwalane po zajęciu grupy
    TEvent *evFree; // zdarzenie wyzwalane po całkowitym zwolnieniu zajętości grupy
    TMemCell *pMemCell; // automatyczna komórka pamięci, która współpracuje z odcinkiem izolowanym
    TIsolated();
    TIsolated(const std::string &n, TIsolated *i);
    ~TIsolated();
    static TIsolated * Find(
        const std::string &n); // znalezienie obiektu albo utworzenie nowego
    void Modify(int i, TDynamicObject *o); // dodanie lub odjęcie osi
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
	std::shared_ptr<TSwitchExtension> SwitchExtension; // dodatkowe dane do toru, który jest zwrotnicą
    std::shared_ptr<TSegment> Segment;
    TTrack *trNext; // odcinek od strony punktu 2 - to powinno być w segmencie
    TTrack *trPrev; // odcinek od strony punktu 1
    // McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
    GLuint TextureID1; // tekstura szyn albo nawierzchni
    GLuint TextureID2; // tekstura automatycznej podsypki albo pobocza
    float fTexLength; // długość powtarzania tekstury w metrach
    float fTexRatio1; // proporcja rozmiarów tekstury dla nawierzchni drogi
    float fTexRatio2; // proporcja rozmiarów tekstury dla chodnika
    float fTexHeight1; // wysokość brzegu względem trajektorii
    float fTexWidth; // szerokość boku
    float fTexSlope;
    double fRadiusTable[2]; // dwa promienie, drugi dla zwrotnicy
    int iTrapezoid; // 0-standard, 1-przechyłka, 2-trapez, 3-oba
    GLuint DisplayListID;
    TIsolated *pIsolated; // obwód izolowany obsługujący zajęcia/zwolnienia grupy torów
    TGroundNode *
        pMyNode; // Ra: proteza, żeby tor znał swoją nazwę TODO: odziedziczyć TTrack z TGroundNode
  public:
    int iNumDynamics;
    TDynamicObject *Dynamics[iMaxNumDynamics];
    int iEvents; // Ra: flaga informująca o obecności eventów
    TEvent *evEventall0; // McZapkie-140302: wyzwalany gdy pojazd stoi
    TEvent *evEventall1;
    TEvent *evEventall2;
    TEvent *evEvent0; // McZapkie-280503: wyzwalany tylko gdy headdriver
    TEvent *evEvent1;
    TEvent *evEvent2;
    std::string asEventall0Name; // nazwy eventów
	std::string asEventall1Name;
	std::string asEventall2Name;
	std::string asEvent0Name;
	std::string asEvent1Name;
	std::string asEvent2Name;
    int iNextDirection; // 0:Point1, 1:Point2, 3:do odchylonego na zwrotnicy
    int iPrevDirection;
    TTrackType eType;
    int iCategoryFlag; // 0x100 - usuwanie pojazów
    float fTrackWidth; // szerokość w punkcie 1
    float fTrackWidth2; // szerokość w punkcie 2 (głównie drogi i rzeki)
    float fFriction; // współczynnik tarcia
    float fSoundDistance;
    int iQualityFlag;
    int iDamageFlag;
    TEnvironmentType eEnvironment; // dźwięk i oświetlenie
    bool bVisible; // czy rysowany
    int iAction; // czy modyfikowany eventami (specjalna obsługa przy skanowaniu)
    float fOverhead; // informacja o stanie sieci: 0-jazda bezprądowa, >0-z opuszczonym i
    // ograniczeniem prędkości
  private:
    double fVelocity; // prędkość dla AI (powyżej rośnie prawdopowobieństwo wykolejenia)
  public:
    // McZapkie-100502:
    double fTrackLength; // długość z wpisu, nigdzie nie używana
    double fRadius; // promień, dla zwrotnicy kopiowany z tabeli
    bool ScannedFlag; // McZapkie: do zaznaczania kolorem torów skanowanych przez AI
    TTraction *hvOverhead; // drut zasilający do szybkiego znalezienia (nie używany)
    TGroundNode *nFouling[2]; // współrzędne ukresu albo oporu kozła
    TTrack *trColides; // tor kolizyjny, na którym trzeba sprawdzać pojazdy pod kątem zderzenia

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
	inline std::shared_ptr<TSegment> CurrentSegment()
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
    void RaArrayFill(CVertNormTex *Vert, const CVertNormTex *Start); // wypełnianie VBO
    void RaRenderVBO(int iPtr); // renderowanie z VBO sektora
    void RenderDyn(); // renderowanie nieprzezroczystych pojazdów (oba tryby)
    void RenderDynAlpha(); // renderowanie przezroczystych pojazdów (oba tryby)
    void RenderDynSounds(); // odtwarzanie dźwięków pojazdów jest niezależne od ich
    // wyświetlania

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
    }; // dodanie lub odjęcie osi
    std::string IsolatedName();
    bool IsolatedEventsAssign(TEvent *busy, TEvent *free);
    double WidthTotal();
    GLuint TextureGet(int i)
    {
        return i ? TextureID1 : TextureID2;
    };
    bool IsGroupable();
    int TestPoint(vector3 *Point);
    void MovedUp1(double dh);
    std::string NameGet();
    void VelocitySet(float v);
    float VelocityGet();
    void ConnectionsLog();

  private:
    void EnvironmentSet();
    void EnvironmentReset();
};

//---------------------------------------------------------------------------
