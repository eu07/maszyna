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
#include <vector>
#include <deque>

#include "classes.h"
#include "Segment.h"
#include "material.h"
#include "scenenode.h"
#include "names.h"

namespace scene {
class basic_cell;
}

enum TTrackType {
    tt_Unknown,
    tt_Normal,
    tt_Switch,
    tt_Table,
    tt_Cross,
    tt_Tributary
};
// McZapkie-100502

enum TEnvironmentType {
    e_unknown = -1,
    e_flat = 0,
    e_mountains,
    e_canyon,
    e_tunnel,
    e_bridge,
    e_bank
};
// Ra: opracować alternatywny system cieni/świateł z definiowaniem koloru oświetlenia w halach

class basic_event;
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
    TTrack *pNexts[2]; // tory do³¹czone do punktów 2 i 4
    TTrack *pPrevs[2]; // tory do³¹czone do punktów 1 i 3
    int iNextDirection[2]; // to te¿ z [2]+[2] przerobiæ na [4]
    int iPrevDirection[2];
    int CurrentIndex = 0; // dla zwrotnicy
    float
        fOffset{ 0.f },
        fDesiredOffset{ 0.f }; // aktualne i docelowe położenie napędu iglic
    float fOffsetSpeed = 0.1f; // prędkość liniowa ruchu iglic
    float fOffsetDelay = 0.05f; // opóźnienie ruchu drugiej iglicy względem pierwszej // dodatkowy ruch drugiej iglicy po zablokowaniu pierwszej na opornicy
    union
    {
        struct
        { // zmienne potrzebne tylko dla zwrotnicy
            float fOffset1,
                  fOffset2; // przesunięcia iglic - 0=na wprost
            bool RightSwitch; // czy zwrotnica w prawo
        };
        struct
        { // zmienne potrzebne tylko dla obrotnicy/przesuwnicy
          // TAnimContainer *pAnim; //animator modelu dla obrotnicy
            TAnimModel *pModel; // na razie model
        };
        struct
        { // zmienne dla skrzyżowania
            int iRoads; // ile dróg się spotyka?
            glm::vec3 *vPoints; // tablica wierzchołków nawierzchni, generowana przez pobocze
            bool bPoints; // czy utworzone?
        };
    };
    bool bMovement = false; // czy w trakcie animacji
    scene::basic_cell *pOwner = nullptr; // TODO: convert this to observer pattern
    TTrack *pNextAnim = nullptr; // następny tor do animowania
    basic_event *evPlus = nullptr,
           *evMinus = nullptr; // zdarzenia sygnalizacji rozprucia
    float fVelocity = -1.0; // maksymalne ograniczenie prędkości (ustawianej eventem)
    Math3D::vector3 vTrans; // docelowa translacja przesuwnicy
};

class TIsolated
{ // obiekt zbierający zajętości z kilku odcinków
public:
    // constructors
    TIsolated(const std::string &n, TIsolated *i);
    // methods
    static void DeleteAll();
    static TIsolated * Find(const std::string &n); // znalezienie obiektu albo utworzenie nowego
    bool AssignEvents();
    void Modify(int i, TDynamicObject *o); // dodanie lub odjęcie osi
    inline
    bool
        Busy() {
            return (iAxles > 0); };
    inline
    static TIsolated *
        Root() {
            return (pRoot); };
    inline
    TIsolated *
        Next() {
            return (pNext); };
    inline
    void
        parent( TIsolated *Parent ) {
            pParent = Parent; }
    inline
    TIsolated *
        parent() const {
            return pParent; }
    // members
    std::string asName; // nazwa obiektu, baza do nazw eventów
    basic_event *evBusy { nullptr }; // zdarzenie wyzwalane po zajęciu grupy
    basic_event *evFree { nullptr }; // zdarzenie wyzwalane po całkowitym zwolnieniu zajętości grupy
    TMemCell *pMemCell { nullptr }; // automatyczna komórka pamięci, która współpracuje z odcinkiem izolowanym
private:
    // members
    int iAxles { 0 }; // ilość osi na odcinkach obsługiwanych przez obiekt
    TIsolated *pNext { nullptr }; // odcinki izolowane są trzymane w postaci listy jednikierunkowej
    TIsolated *pParent { nullptr }; // optional parent piece, collecting data from its children
    static TIsolated *pRoot; // początek listy
};

// trajektoria ruchu - opakowanie
class TTrack : public scene::basic_node {

    friend opengl_renderer;
    // NOTE: temporary arrangement
    friend itemproperties_panel;

private:
    TIsolated * pIsolated = nullptr; // obwód izolowany obsługujący zajęcia/zwolnienia grupy torów
	std::shared_ptr<TSwitchExtension> SwitchExtension; // dodatkowe dane do toru, który jest zwrotnicą
    std::shared_ptr<TSegment> Segment;
    TTrack * trNext = nullptr; // odcinek od strony punktu 2 - to powinno być w segmencie
    TTrack * trPrev = nullptr; // odcinek od strony punktu 1

    // McZapkie-070402: dodalem zmienne opisujace rozmiary tekstur
    int iTrapezoid = 0; // 0-standard, 1-przechyłka, 2-trapez, 3-oba
    double fRadiusTable[ 2 ] = { 0.0, 0.0 }; // dwa promienie, drugi dla zwrotnicy
    float fVerticalRadius { 0.f }; // y-axis track radius (currently not supported)
    float fTexLength = 4.0f; // długość powtarzania tekstury w metrach
    float fTexRatio1 = 1.0f; // proporcja boków tekstury nawierzchni (żeby zaoszczędzić na rozmiarach tekstur...)
    float fTexRatio2 = 1.0f; // proporcja boków tekstury chodnika (żeby zaoszczędzić na rozmiarach tekstur...)
    float fTexHeight1 = 0.6f; // wysokość brzegu względem trajektorii
    float fTexHeightOffset = 0.f; // potential adjustment to account for the path roll
    float fTexWidth = 0.9f; // szerokość boku
    float fTexSlope = 0.9f;

    glm::dvec3 m_origin;
    material_handle m_material1 = 0; // tekstura szyn albo nawierzchni
    material_handle m_material2 = 0; // tekstura automatycznej podsypki albo pobocza
    using geometryhandle_sequence = std::vector<gfx::geometry_handle>;
    geometryhandle_sequence Geometry1; // geometry chunks textured with texture 1
    geometryhandle_sequence Geometry2; // geometry chunks textured with texture 2

    std::vector<segment_data> m_paths; // source data for owned paths

public:
    using dynamics_sequence = std::deque<TDynamicObject *>;
    using event_sequence = std::vector<std::pair<std::string, basic_event *> >;

    dynamics_sequence Dynamics;
    event_sequence
        m_events0all,
        m_events1all,
        m_events2all,
        m_events0,
        m_events1,
        m_events2;
    bool m_events { false }; // Ra: flaga informująca o obecności eventów

    int iNextDirection = 0; // 0:Point1, 1:Point2, 3:do odchylonego na zwrotnicy
    int iPrevDirection = 0; // domyślnie wirtualne odcinki dołączamy stroną od Point1
    TTrackType eType = tt_Normal; // domyślnie zwykły
    int iCategoryFlag = 1; // 0x100 - usuwanie pojazów // 1-tor, 2-droga, 4-rzeka, 8-samolot?
    float fTrackWidth = 1.435f; // szerokość w punkcie 1 // rozstaw toru, szerokość nawierzchni
    float fTrackWidth2 = 0.0f; // szerokość w punkcie 2 (głównie drogi i rzeki)
    float fFriction = 0.15f; // współczynnik tarcia
    float fSoundDistance = -1.0f;
    int iQualityFlag = 20;
    int iDamageFlag = 0;
    TEnvironmentType eEnvironment = e_flat; // dźwięk i oświetlenie
    int iAction = 0; // czy modyfikowany eventami (specjalna obsługa przy skanowaniu)
    float fOverhead = -1.0; // można normalnie pobierać prąd (0 dla jazdy bezprądowej po danym odcinku, >0-z opuszczonym i ograniczeniem prędkości)
private:
    double fVelocity = -1.0; // ograniczenie prędkości // prędkość dla AI (powyżej rośnie prawdopowobieństwo wykolejenia)
public:
    // McZapkie-100502:
//    double fTrackLength = 100.0; // długość z wpisu, nigdzie nie używana
    double fRadius = 0.0; // promień, dla zwrotnicy kopiowany z tabeli
    bool ScannedFlag = false; // McZapkie: do zaznaczania kolorem torów skanowanych przez AI
    TGroundNode *nFouling[ 2 ] = { nullptr, nullptr }; // współrzędne ukresu albo oporu kozła

    explicit TTrack( scene::node_data const &Nodedata );
    virtual ~TTrack();

    void Init();
    static bool sort_by_material( TTrack const *Left, TTrack const *Right );
    static TTrack * Create400m(int what, double dx);
    TTrack * NullCreate(int dir);
    inline bool IsEmpty() {
        return Dynamics.empty(); };
    void ConnectPrevPrev(TTrack *pNewPrev, int typ);
    void ConnectPrevNext(TTrack *pNewPrev, int typ);
    void ConnectNextPrev(TTrack *pNewNext, int typ);
    void ConnectNextNext(TTrack *pNewNext, int typ);
    inline double Length() const {
        return Segment->GetLength(); };
	inline std::shared_ptr<TSegment> CurrentSegment() const {
        return Segment; };
    inline TTrack *CurrentNext() const {
        return trNext; };
    inline TTrack *CurrentPrev() const {
        return trPrev; };
    TTrack *Connected(int s, double &d) const;
    bool SetConnections(int i);
    bool Switch(int i, float const t = -1.f, float const d = -1.f);
    bool SwitchForced(int i, TDynamicObject *o);
    int CrossSegment(int from, int into);
    inline int GetSwitchState() {
        return (
            SwitchExtension ?
                SwitchExtension->CurrentIndex :
                -1); };
    // returns number of different routes possible to take from given point
    // TODO: specify entry point, number of routes for switches can vary
    inline
    int
        RouteCount() const {
        return (
            SwitchExtension != nullptr ?
                SwitchExtension->iRoads - 1 :
                1 ); }
    void Load(cParser *parser, glm::dvec3 const &pOrigin);
    bool AssignEvents();
    bool AssignForcedEvents(basic_event *NewEventPlus, basic_event *NewEventMinus);
    void QueueEvents( event_sequence const &Events, TDynamicObject const *Owner );
    void QueueEvents( event_sequence const &Events, TDynamicObject const *Owner, double const Delaylimit );
    bool CheckDynamicObject(TDynamicObject *Dynamic);
    bool AddDynamicObject(TDynamicObject *Dynamic);
    bool RemoveDynamicObject(TDynamicObject *Dynamic);

    // set origin point
    void
        origin( glm::dvec3 Origin ) {
            m_origin = Origin; }
    // retrieves list of the track's end points
    std::vector<glm::dvec3>
        endpoints() const;

    void create_geometry( gfx::geometrybank_handle const &Bank ); // wypełnianie VBO
    void RenderDynSounds(); // odtwarzanie dźwięków pojazdów jest niezależne od ich wyświetlania

    void RaOwnerSet( scene::basic_cell *o ) {
        if( SwitchExtension ) { SwitchExtension->pOwner = o; } };
    bool InMovement(); // czy w trakcie animacji?
    void RaAssign( TAnimModel *am, basic_event *done, basic_event *joined );
    void RaAnimListAdd(TTrack *t);
    TTrack * RaAnimate();

    void RadioStop();
    void AxleCounter(int i, TDynamicObject *o) {
        if (pIsolated)
            pIsolated->Modify(i, o); }; // dodanie lub odjęcie osi
    std::string IsolatedName();
    double WidthTotal();
    bool IsGroupable();
    int TestPoint( Math3D::vector3 *Point);
    void MovedUp1(float const dh);
    void VelocitySet(float v);
    double VelocityGet();
    void ConnectionsLog();

private:
    // radius() subclass details, calculates node's bounding radius
    float radius_();
    // serialize() subclass details, sends content of the subclass to provided stream
    void serialize_( std::ostream &Output ) const;
    // deserialize() subclass details, restores content of the subclass from provided stream
    void deserialize_( std::istream &Input );
    // export() subclass details, sends basic content of the class in legacy (text) format to provided stream
    void export_as_text_( std::ostream &Output ) const;

};



// collection of virtual tracks and roads present in the scene
class path_table : public basic_table<TTrack> {

public:
    ~path_table();
    // legacy method, initializes tracks after deserialization from scenario file
    void
        InitTracks();
    // legacy method, sends list of occupied paths over network
    void
        TrackBusyList() const;
    // legacy method, sends list of occupied path sections over network
    void
        IsolatedBusyList() const;
    // legacy method, sends state of specified path section over network
    void
        IsolatedBusy( std::string const &Name ) const;
};

//---------------------------------------------------------------------------
