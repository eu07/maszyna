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
#include "Texture.h"

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
    TSwitchExtension(TTrack *owner, int const what);
    ~TSwitchExtension();
    std::shared_ptr<TSegment> Segments[6]; // dwa tory od punktu 1, pozosta³e dwa od 2? Ra 140101: 6 po³¹czeñ dla skrzy¿owañ
    // TTrack *trNear[4]; //tory do³¹czone do punktów 1, 2, 3 i 4
    // dotychczasowe [2]+[2] wskaŸniki zamieniæ na nowe [4]
    TTrack *pNexts[2]; // tory do³¹czone do punktów 2 i 4
    TTrack *pPrevs[2]; // tory do³¹czone do punktów 1 i 3
    int iNextDirection[2]; // to te¿ z [2]+[2] przerobiæ na [4]
    int iPrevDirection[2];
    int CurrentIndex = 0; // dla zwrotnicy
    double fOffset = 0.0,
           fDesiredOffset = 0.0; // aktualne i docelowe położenie napędu iglic
    double fOffsetSpeed = 0.1; // prędkość liniowa ruchu iglic
    double fOffsetDelay = 0.05; // opóźnienie ruchu drugiej iglicy względem pierwszej // dodatkowy ruch drugiej iglicy po zablokowaniu pierwszej na opornicy
    union
    {
        struct
        { // zmienne potrzebne tylko dla zwrotnicy
            double fOffset1,
                   fOffset2; // przesunięcia iglic - 0=na wprost
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
    bool bMovement = false; // czy w trakcie animacji
    int iLeftVBO = 0,
        iRightVBO = 0; // indeksy iglic w VBO
    TSubRect *pOwner = nullptr; // sektor, któremu trzeba zgłosić animację
    TTrack *pNextAnim = nullptr; // następny tor do animowania
    TEvent *evPlus = nullptr,
           *evMinus = nullptr; // zdarzenia sygnalizacji rozprucia
    float fVelocity = -1.0; // maksymalne ograniczenie prędkości (ustawianej eventem)
    vector3 vTrans; // docelowa translacja przesuwnicy
  private:
};

#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
const int iMaxNumDynamics = 40; // McZapkie-100303
#endif

class TIsolated
{ // obiekt zbierający zajętości z kilku odcinków
    int iAxles = 0; // ilość osi na odcinkach obsługiwanych przez obiekt
    TIsolated *pNext = nullptr; // odcinki izolowane są trzymane w postaci listy jednikierunkowej
    static TIsolated *pRoot; // początek listy
  public:
    std::string asName; // nazwa obiektu, baza do nazw eventów
    TEvent *evBusy = nullptr; // zdarzenie wyzwalane po zajęciu grupy
    TEvent *evFree = nullptr; // zdarzenie wyzwalane po całkowitym zwolnieniu zajętości grupy
    TMemCell *pMemCell = nullptr; // automatyczna komórka pamięci, która współpracuje z odcinkiem izolowanym
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
    TTrack *trNext = nullptr; // odcinek od strony punktu 2 - to powinno być w segmencie
    TTrack *trPrev = nullptr; // odcinek od strony punktu 1
    // McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
    texture_manager::size_type TextureID1 = 0; // tekstura szyn albo nawierzchni
    texture_manager::size_type TextureID2 = 0; // tekstura automatycznej podsypki albo pobocza
    float fTexLength = 4.0; // długość powtarzania tekstury w metrach
    float fTexRatio1 = 1.0; // proporcja boków tekstury nawierzchni (żeby zaoszczędzić na rozmiarach tekstur...)
    float fTexRatio2 = 1.0; // proporcja boków tekstury chodnika (żeby zaoszczędzić na rozmiarach tekstur...)
    float fTexHeight1 = 0.6; // wysokość brzegu względem trajektorii
    float fTexWidth = 0.9; // szerokość boku
    float fTexSlope = 0.9;
    double fRadiusTable[ 2 ]; // dwa promienie, drugi dla zwrotnicy
    int iTrapezoid = 0; // 0-standard, 1-przechyłka, 2-trapez, 3-oba
    GLuint DisplayListID = 0;
    TIsolated *pIsolated = nullptr; // obwód izolowany obsługujący zajęcia/zwolnienia grupy torów
    TGroundNode *
        pMyNode = nullptr; // Ra: proteza, żeby tor znał swoją nazwę TODO: odziedziczyć TTrack z TGroundNode
  public:
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    int iNumDynamics = 0;
    TDynamicObject *Dynamics[iMaxNumDynamics];
#else
    typedef std::deque<TDynamicObject *> dynamics_sequence;
    dynamics_sequence Dynamics;
#endif
    int iEvents = 0; // Ra: flaga informująca o obecności eventów
    TEvent *evEventall0 = nullptr; // McZapkie-140302: wyzwalany gdy pojazd stoi
    TEvent *evEventall1 = nullptr;
    TEvent *evEventall2 = nullptr;
    TEvent *evEvent0 = nullptr; // McZapkie-280503: wyzwalany tylko gdy headdriver
    TEvent *evEvent1 = nullptr;
    TEvent *evEvent2 = nullptr;
    std::string asEventall0Name; // nazwy eventów
	std::string asEventall1Name;
	std::string asEventall2Name;
	std::string asEvent0Name;
	std::string asEvent1Name;
	std::string asEvent2Name;
    int iNextDirection = 0; // 0:Point1, 1:Point2, 3:do odchylonego na zwrotnicy
    int iPrevDirection = 0; // domyślnie wirtualne odcinki dołączamy stroną od Point1
    TTrackType eType = tt_Normal; // domyślnie zwykły
    int iCategoryFlag = 1; // 0x100 - usuwanie pojazów // 1-tor, 2-droga, 4-rzeka, 8-samolot?
    float fTrackWidth = 1.435; // szerokość w punkcie 1 // rozstaw toru, szerokość nawierzchni
    float fTrackWidth2 = 0.0; // szerokość w punkcie 2 (głównie drogi i rzeki)
    float fFriction = 0.15; // współczynnik tarcia
    float fSoundDistance = -1.0;
    int iQualityFlag = 20;
    int iDamageFlag = 0;
    TEnvironmentType eEnvironment = e_flat; // dźwięk i oświetlenie
    bool bVisible = true; // czy rysowany
    int iAction = 0; // czy modyfikowany eventami (specjalna obsługa przy skanowaniu)
    float fOverhead = -1.0; // można normalnie pobierać prąd (0 dla jazdy bezprądowej po danym odcinku, >0-z opuszczonym i ograniczeniem prędkości)
  private:
    double fVelocity = -1.0; // ograniczenie prędkości // prędkość dla AI (powyżej rośnie prawdopowobieństwo wykolejenia)
  public:
    // McZapkie-100502:
    double fTrackLength = 100.0; // długość z wpisu, nigdzie nie używana
    double fRadius = 0.0; // promień, dla zwrotnicy kopiowany z tabeli
    bool ScannedFlag = false; // McZapkie: do zaznaczania kolorem torów skanowanych przez AI
    TTraction *hvOverhead = nullptr; // drut zasilający do szybkiego znalezienia (nie używany)
    TGroundNode *nFouling[ 2 ]; // współrzędne ukresu albo oporu kozła
    TTrack *trColides = nullptr; // tor kolizyjny, na którym trzeba sprawdzać pojazdy pod kątem zderzenia

    TTrack(TGroundNode *g);
    ~TTrack();
    void Init();
    static TTrack * Create400m(int what, double dx);
    TTrack * NullCreate(int dir);
    inline bool IsEmpty()
    {
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
        return (iNumDynamics <= 0);
#else
        return Dynamics.empty();
#endif
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
