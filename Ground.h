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

#include "Classes.h"
#include "material.h"
#include "dumb3d.h"
#include "Names.h"
#include "lightarray.h"
#include "simulation.h"

typedef int TGroundNodeType;
// Ra: zmniejszone liczby, aby zrobić tabelkę i zoptymalizować wyszukiwanie
const int TP_MODEL = 10;
/*
const int TP_MESH = 11; // Ra: specjalny obiekt grupujący siatki dla tekstury
const int TP_DUMMYTRACK = 12; // Ra: zdublowanie obiektu toru dla rozdzielenia tekstur
*/
const int TP_TERRAIN = 13; // Ra: specjalny model dla terenu
const int TP_DYNAMIC = 14;
const int TP_SOUND = 15;
const int TP_TRACK = 16;
/*
const int TP_GEOMETRY=17;
*/
const int TP_MEMCELL = 18;
const int TP_EVLAUNCH = 19; // MC
const int TP_TRACTION = 20;
const int TP_TRACTIONPOWERSOURCE = 21; // MC
/*
const int TP_ISOLATED=22; //Ra
*/
const int TP_SUBMODEL = 22; // Ra: submodele terenu
const int TP_LAST = 25; // rozmiar tablicy

struct TGroundVertex
{
    glm::dvec3 position;
    glm::vec3 normal;
    glm::vec2 texture;

    void HalfSet(const TGroundVertex &v1, const TGroundVertex &v2)
    { // wyliczenie współrzędnych i mapowania punktu na środku odcinka v1<->v2
        interpolate_( v1, v2, 0.5 );
    }
    void SetByX(const TGroundVertex &v1, const TGroundVertex &v2, double x)
    { // wyliczenie współrzędnych i mapowania punktu na odcinku v1<->v2
        interpolate_( v1, v2, ( x - v1.position.x ) / ( v2.position.x - v1.position.x ) );
    }
    void SetByZ(const TGroundVertex &v1, const TGroundVertex &v2, double z)
    { // wyliczenie współrzędnych i mapowania punktu na odcinku v1<->v2
        interpolate_( v1, v2, ( z - v1.position.z ) / ( v2.position.z - v1.position.z ) );
    }
    void interpolate_( const TGroundVertex &v1, const TGroundVertex &v2, double factor ) {
        position = interpolate( v1.position, v2.position, factor );
        normal = interpolate( v1.normal, v2.normal, static_cast<float>( factor ) );
        texture = interpolate( v1.texture, v2.texture, static_cast<float>( factor ) );
    }
};

// ground node holding single, unique piece of 3d geometry. TBD, TODO: unify this with basic 3d model node
struct piece_node {
    std::vector<TGroundVertex> vertices;
    geometry_handle geometry { 0, 0 }; // geometry prepared for drawing
};

// obiekt scenerii
class TGroundNode {

    friend class opengl_renderer;

public:
    TGroundNode *nNext; // lista wszystkich w scenerii, ostatni na początku
    TGroundNode *nNext2; // lista obiektów w sektorze
    TGroundNode *nNext3; // lista obiektów renderowanych we wspólnym cyklu
    std::string asName; // nazwa (nie zawsze ma znaczenie)
    TGroundNodeType iType; // typ obiektu
    union
    { // Ra: wskażniki zależne od typu - zrobić klasy dziedziczone zamiast
        void *Pointer; // do przypisywania NULL
        TSubModel *smTerrain; // modele terenu (kwadratow kilometrowych)
        TAnimModel *Model; // model z animacjami
        TDynamicObject *DynamicObject; // pojazd
        piece_node *Piece; // non-instanced piece of geometry
        TTrack *pTrack; // trajektoria ruchu
        TMemCell *MemCell; // komórka pamięci
        TEventLauncher *EvLaunch; // wyzwalacz zdarzeń
        TTraction *hvTraction; // drut zasilający
        TTractionPowerSource *psTractionPowerSource; // zasilanie drutu (zaniedbane w sceneriach)
        TTextSound *tsStaticSound; // dźwięk przestrzenny
        TGroundNode *nNode; // obiekt renderujący grupowo ma tu wskaźnik na listę obiektów
    };
    Math3D::vector3 pCenter; // współrzędne środka do przydzielenia sektora
    float m_radius { 0.0f }; // bounding radius of geometry stored in the node. TODO: reuse bounding_area struct for radius and center
    glm::dvec3 m_rootposition; // position of the ground (sub)rectangle holding the node, in the 3d world
    // visualization-related data
    // TODO: wrap these in a struct, when cleaning objects up
    double fSquareMinRadius; // kwadrat widoczności od
    double fSquareRadius; // kwadrat widoczności do
    union
    {
        int iNumVerts; // dla trójkątów
        int iNumPts; // dla linii
        int iCount; // dla terenu
    };
    int iFlags; // tryb przezroczystości: 0x10-nieprz.,0x20-przezroczysty,0x30-mieszany
    material_handle m_material; // główna (jedna) tekstura obiektu
    glm::vec3
        Ambient{ 0.2f },
        Diffuse{ 1.0f },
        Specular{ 0.0f }; // oświetlenie
    double fLineThickness; // McZapkie-120702: grubosc linii
    bool bVisible;

    TGroundNode();
    TGroundNode(TGroundNodeType t);
    ~TGroundNode();

    void InitNormals();
    void RenderHidden(); // obsługa dźwięków i wyzwalaczy zdarzeń
};

class TSubRect : public CMesh
{ // sektor składowy kwadratu kilometrowego
  public:
    scene::bounding_area m_area;
    unsigned int m_framestamp { 0 }; // id of last rendered gfx frame
    int iTracks = 0; // ilość torów w (tTracks)
    TTrack **tTracks = nullptr; // tory do renderowania pojazdów
  protected:
    TTrack *tTrackAnim = nullptr; // obiekty do przeliczenia animacji
  public:
    TGroundNode *nRootNode = nullptr; // wszystkie obiekty w sektorze, z wyjątkiem renderujących i pojazdów (nNext2)
    TGroundNode *nRenderHidden = nullptr; // lista obiektów niewidocznych, "renderowanych" również z tyłu (nNext3)
    TGroundNode *nRenderRect = nullptr; // z poziomu sektora - nieprzezroczyste (nNext3)
    TGroundNode *nRenderRectAlpha = nullptr; // z poziomu sektora - przezroczyste (nNext3)
    TGroundNode *nRenderWires = nullptr; // z poziomu sektora - druty i inne linie (nNext3)
    TGroundNode *nRender = nullptr; // indywidualnie - nieprzezroczyste (nNext3)
    TGroundNode *nRenderMixed = nullptr; // indywidualnie - nieprzezroczyste i przezroczyste (nNext3)
    TGroundNode *nRenderAlpha = nullptr; // indywidualnie - przezroczyste (nNext3)
#ifdef EU07_SCENERY_EDITOR
    std::deque< TGroundNode* > m_memcells; // collection of memcells present in the sector
#endif
    int iNodeCount = 0; // licznik obiektów, do pomijania pustych sektorów
  public:
    void LoadNodes(); // utworzenie VBO sektora
  public:
    virtual ~TSubRect();
    virtual void NodeAdd(TGroundNode *Node); // dodanie obiektu do sektora na etapie rozdzielania na sektory
    void Sort(); // optymalizacja obiektów w sektorze (sortowanie wg tekstur)
    TTrack * FindTrack(vector3 *Point, int &iConnection, TTrack *Exclude);
    TTraction * FindTraction(glm::dvec3 const &Point, int &iConnection, TTraction *Exclude);
#ifdef EU07_USE_OLD_GROUNDCODE
    bool RaTrackAnimAdd(TTrack *t); // zgłoszenie toru do animacji
    void RaAnimate( unsigned int const Framestamp ); // przeliczenie animacji torów
#endif
    void RenderSounds(); // dźwięki pojazdów z niewidocznych sektorów
};

// Ra: trzeba sprawdzić wydajność siatki
const int iNumSubRects = 5; // na ile dzielimy kilometr
const int iNumRects = 500;
const int iTotalNumSubRects = iNumRects * iNumSubRects;
const double fHalfTotalNumSubRects = iTotalNumSubRects / 2.0;
const double fSubRectSize = 1000.0 / iNumSubRects;
const double fRectSize = fSubRectSize * iNumSubRects;

class TGroundRect : public TSubRect
{ // kwadrat kilometrowy
    // obiekty o niewielkiej ilości wierzchołków będą renderowane stąd
    // Ra: 2012-02 doszły submodele terenu
    friend class opengl_renderer;

private:
    TSubRect *pSubRects { nullptr };

    void Init();

public:
    virtual ~TGroundRect();
    // pobranie wskaźnika do małego kwadratu, utworzenie jeśli trzeba
    inline
    TSubRect * SafeGetSubRect(int iCol, int iRow) {
        if( !pSubRects ) {
            // utworzenie małych kwadratów
            Init();
        }
        return pSubRects + iRow * iNumSubRects + iCol; // zwrócenie właściwego
    };
    // pobranie wskaźnika do małego kwadratu, bez tworzenia jeśli nie ma
    inline
    TSubRect *FastGetSubRect(int iCol, int iRow) const {
        return (
            pSubRects ?
                pSubRects + iRow * iNumSubRects + iCol :
                nullptr); };
    void NodeAdd( TGroundNode *Node ); // dodanie obiektu do sektora na etapie rozdzielania na sektory
    // compares two provided nodes, returns true if their content can be merged
    bool mergeable( TGroundNode const &Left, TGroundNode const &Right ) const;
    // optymalizacja obiektów w sektorach
    inline
    void Optimize() {
        if( pSubRects ) {
            for( int i = iNumSubRects * iNumSubRects - 1; i >= 0; --i ) {
                // optymalizacja obiektów w sektorach
                pSubRects[ i ].Sort(); } } };

    TGroundNode *nTerrain { nullptr }; // model terenu z E3D - użyć nRootMesh?
};

class TGround
{
    friend class opengl_renderer;

    TGroundRect Rects[ iNumRects ][ iNumRects ]; // mapa kwadratów kilometrowych
    TSubRect srGlobal; // zawiera obiekty globalne (na razie wyzwalacze czasowe)
    TGroundNode *nRootDynamic = nullptr; // lista pojazdów
    TGroundNode *nRootOfType[ TP_LAST ]; // tablica grupująca obiekty, przyspiesza szukanie
#ifdef EU07_USE_OLD_GROUNDCODE
    TEvent *RootEvent = nullptr; // lista zdarzeń
    TEvent *QueryRootEvent = nullptr,
           *tmpEvent = nullptr;
    typedef std::unordered_map<std::string, TEvent *> event_map;
    event_map m_eventmap;
    TNames<TGroundNode *> m_nodemap;
#endif
    vector3 pOrigin;
    vector3 aRotate;
    bool bInitDone = false;

  private: // metody prywatne
#ifdef EU07_USE_OLD_GROUNDCODE
      bool EventConditon(TEvent *e);
#endif

  public:
    bool bDynamicRemove = false; // czy uruchomić procedurę usuwania pojazdów

    TGround();
    ~TGround();
    void Free();
#ifdef EU07_USE_OLD_GROUNDCODE
    bool Init( std::string File );
    void FirstInit();
    void InitTracks();
    void InitTraction();
    bool InitEvents();
    bool InitLaunchers();
    TTrack * FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude);
    TTraction * FindTraction(glm::dvec3 const &Point, int &iConnection, TGroundNode *Exclude);
    TTraction * TractionNearestFind(glm::dvec3 &p, int dir, TGroundNode *n);
#endif
    TGroundNode * AddGroundNode(cParser *parser);
#ifdef EU07_USE_OLD_GROUNDCODE
    void UpdatePhys(double dt, int iter); // aktualizacja fizyki stałym krokiem
    bool Update(double dt, int iter); // aktualizacja przesunięć zgodna z FPS
    void Update_Hidden(); // updates invisible elements of the scene
    bool GetTraction(TDynamicObject *model);
    bool AddToQuery( TEvent *Event, TDynamicObject *Node );
    bool CheckQuery();
    TGroundNode * DynamicFindAny(std::string const &Name);
    TGroundNode * DynamicFind(std::string const &Name);
    TGroundNode * FindGroundNode(std::string const &asNameToFind, TGroundNodeType const iNodeType);
#endif
    TGroundRect * GetRect( double x, double z );
    TSubRect * GetSubRect( int iCol, int iRow );
    inline
    TSubRect * GetSubRect(double x, double z) {
        return GetSubRect(GetColFromX(x), GetRowFromZ(z)); };
    TSubRect * FastGetSubRect( int iCol, int iRow );
    inline
    TSubRect * FastGetSubRect( double x, double z ) {
        return FastGetSubRect( GetColFromX( x ), GetRowFromZ( z ) ); };
    inline
    int GetRowFromZ(double z) {
        return (int)(z / fSubRectSize + fHalfTotalNumSubRects); };
    inline
    int GetColFromX(double x) {
        return (int)(x / fSubRectSize + fHalfTotalNumSubRects); };
#ifdef EU07_USE_OLD_GROUNDCODE
    TEvent * FindEvent(const std::string &asEventName);
    void TrackJoin(TGroundNode *Current);
private:
    // convert tp_terrain model to a series of triangle nodes
    void convert_terrain( TGroundNode const *Terrain );
    void convert_terrain( TSubModel const *Submodel );
    void RaTriangleDivider(TGroundNode *node);
#endif
public:
#ifdef EU07_USE_OLD_GROUNDCODE
    void DynamicList( bool all = false );
    void TrackBusyList();
    void IsolatedBusyList();
    void IsolatedBusy( const std::string t );
	void RadioStop(vector3 pPosition);
    TDynamicObject * DynamicNearest(vector3 pPosition, double distance = 20.0, bool mech = false);
    TDynamicObject * CouplerNearest(vector3 pPosition, double distance = 20.0, bool mech = false);
#endif
    void DynamicRemove(TDynamicObject *dyn);
    void TerrainRead(std::string const &f);
    void TerrainWrite();
    void Silence(vector3 gdzie);
};

//---------------------------------------------------------------------------
