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
#include "VBO.h"
#include "Classes.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "dumb3d.h"
#include "Names.h"

using namespace Math3D;

// Ra: zmniejszone liczby, aby zrobić tabelkę i zoptymalizować wyszukiwanie
const int TP_MODEL = 10;
const int TP_MESH = 11; // Ra: specjalny obiekt grupujący siatki dla tekstury
const int TP_DUMMYTRACK = 12; // Ra: zdublowanie obiektu toru dla rozdzielenia tekstur
const int TP_TERRAIN = 13; // Ra: specjalny model dla terenu
const int TP_DYNAMIC = 14;
const int TP_SOUND = 15;
const int TP_TRACK = 16;
// const int TP_GEOMETRY=17;
const int TP_MEMCELL = 18;
const int TP_EVLAUNCH = 19; // MC
const int TP_TRACTION = 20;
const int TP_TRACTIONPOWERSOURCE = 21; // MC
// const int TP_ISOLATED=22; //Ra
const int TP_SUBMODEL = 22; // Ra: submodele terenu
const int TP_LAST = 25; // rozmiar tablicy

struct DaneRozkaz
{ // struktura komunikacji z EU07.EXE
    int iSygn; // sygnatura 'EU07'
    int iComm; // rozkaz/status (kod ramki)
    union
    {
        float fPar[62];
        int iPar[62];
        char cString[248]; // upakowane stringi
    };
};

struct DaneRozkaz2
{              // struktura komunikacji z EU07.EXE
	int iSygn; // sygnatura 'EU07'
	int iComm; // rozkaz/status (kod ramki)
	union
	{
		float fPar[496];
		int iPar[496];
		char cString[1984]; // upakowane stringi
	};
};

typedef int TGroundNodeType;

struct TGroundVertex
{
    vector3 Point;
    vector3 Normal;
    float tu, tv;
    void HalfSet(const TGroundVertex &v1, const TGroundVertex &v2)
    { // wyliczenie współrzędnych i mapowania punktu na środku odcinka v1<->v2
        Point = 0.5 * (v1.Point + v2.Point);
        Normal = 0.5 * (v1.Normal + v2.Normal);
        tu = 0.5 * (v1.tu + v2.tu);
        tv = 0.5 * (v1.tv + v2.tv);
    }
    void SetByX(const TGroundVertex &v1, const TGroundVertex &v2, double x)
    { // wyliczenie współrzędnych i mapowania punktu na odcinku v1<->v2
        double i = (x - v1.Point.x) / (v2.Point.x - v1.Point.x); // parametr równania
        double j = 1.0 - i;
        Point = j * v1.Point + i * v2.Point;
        Normal = j * v1.Normal + i * v2.Normal;
        tu = j * v1.tu + i * v2.tu;
        tv = j * v1.tv + i * v2.tv;
    }
    void SetByZ(const TGroundVertex &v1, const TGroundVertex &v2, double z)
    { // wyliczenie współrzędnych i mapowania punktu na odcinku v1<->v2
        double i = (z - v1.Point.z) / (v2.Point.z - v1.Point.z); // parametr równania
        double j = 1.0 - i;
        Point = j * v1.Point + i * v2.Point;
        Normal = j * v1.Normal + i * v2.Normal;
        tu = j * v1.tu + i * v2.tu;
        tv = j * v1.tv + i * v2.tv;
    }
};

class TSubRect; // sektor (aktualnie 200m×200m, ale może być zmieniony)

class TGroundNode : public Resource
{ // obiekt scenerii
  private:
  public:
    TGroundNodeType iType; // typ obiektu
    union
    { // Ra: wskażniki zależne od typu - zrobić klasy dziedziczone zamiast
        void *Pointer; // do przypisywania NULL
        TSubModel *smTerrain; // modele terenu (kwadratow kilometrowych)
        TAnimModel *Model; // model z animacjami
        TDynamicObject *DynamicObject; // pojazd
        vector3 *Points; // punkty dla linii
        TTrack *pTrack; // trajektoria ruchu
        TGroundVertex *Vertices; // wierzchołki dla trójkątów
        TMemCell *MemCell; // komórka pamięci
        TEventLauncher *EvLaunch; // wyzwalacz zdarzeń
        TTraction *hvTraction; // drut zasilający
        TTractionPowerSource *psTractionPowerSource; // zasilanie drutu (zaniedbane w sceneriach)
        TTextSound *tsStaticSound; // dźwięk przestrzenny
        TGroundNode *nNode; // obiekt renderujący grupowo ma tu wskaźnik na listę obiektów
    };
    std::string asName; // nazwa (nie zawsze ma znaczenie)
    union
    {
        int iNumVerts; // dla trójkątów
        int iNumPts; // dla linii
        int iCount; // dla terenu
        // int iState; //Ra: nie używane - dźwięki zapętlone
    };
    vector3 pCenter; // współrzędne środka do przydzielenia sektora

    union
    {
        // double fAngle; //kąt obrotu dla modelu
        double fLineThickness; // McZapkie-120702: grubosc linii
        // int Status;  //McZapkie-170303: status dzwieku
    };
    double fSquareRadius; // kwadrat widoczności do
    double fSquareMinRadius; // kwadrat widoczności od
    // TGroundNode *nMeshGroup; //Ra: obiekt grupujący trójkąty w TSubRect dla tekstury
    int iVersion; // wersja siatki (do wykonania rekompilacji)
    // union ?
    GLuint DisplayListID; // numer siatki DisplayLists
    bool PROBLEND;
    int iVboPtr; // indeks w buforze VBO
    texture_manager::size_type TextureID; // główna (jedna) tekstura obiektu
    int iFlags; // tryb przezroczystości: 0x10-nieprz.,0x20-przezroczysty,0x30-mieszany
    int Ambient[4], Diffuse[4], Specular[4]; // oświetlenie
    bool bVisible;
    TGroundNode *nNext; // lista wszystkich w scenerii, ostatni na początku
    TGroundNode *nNext2; // lista obiektów w sektorze
    TGroundNode *nNext3; // lista obiektów renderowanych we wspólnym cyklu
    TGroundNode();
    TGroundNode(TGroundNodeType t, int n = 0);
    ~TGroundNode();
    void Init(int n);
    void InitCenter(); // obliczenie współrzędnych środka
    void InitNormals();

    void MoveMe(vector3 pPosition); // przesuwanie (nie działa)

    // bool Disable();
    inline TGroundNode * Find(const std::string &asNameToFind, TGroundNodeType iNodeType)
    { // wyszukiwanie czołgowe z typem
        if ((iNodeType == iType) && (asNameToFind == asName))
            return this;
        else if (nNext)
            return nNext->Find(asNameToFind, iNodeType);
        return NULL;
    };

    void Compile(bool many = false);
    void Release();
    bool GetTraction();

    void RenderHidden(); // obsługa dźwięków i wyzwalaczy zdarzeń
    void RenderDL(); // renderowanie nieprzezroczystych w Display Lists
    void RenderAlphaDL(); // renderowanie przezroczystych w Display Lists
    // (McZapkie-131202)
    void RaRenderVBO(); // renderowanie (nieprzezroczystych) ze wspólnego VBO
    void RenderVBO(); // renderowanie nieprzezroczystych z własnego VBO
    void RenderAlphaVBO(); // renderowanie przezroczystych z (własnego) VBO
};

class TSubRect : public Resource, public CMesh
{ // sektor składowy kwadratu kilometrowego
  public:
    int iTracks = 0; // ilość torów w (tTracks)
    TTrack **tTracks = nullptr; // tory do renderowania pojazdów
  protected:
    TTrack *tTrackAnim = nullptr; // obiekty do przeliczenia animacji
    TGroundNode *nRootMesh = nullptr; // obiekty renderujące wg tekstury (wtórne, lista po nNext2)
    TGroundNode *nMeshed = nullptr; // lista obiektów dla których istnieją obiekty renderujące grupowo
  public:
    TGroundNode *
        nRootNode = nullptr; // wszystkie obiekty w sektorze, z wyjątkiem renderujących i pojazdów (nNext2)
    TGroundNode *
        nRenderHidden = nullptr; // lista obiektów niewidocznych, "renderowanych" również z tyłu (nNext3)
    TGroundNode *nRenderRect = nullptr; // z poziomu sektora - nieprzezroczyste (nNext3)
    TGroundNode *nRenderRectAlpha = nullptr; // z poziomu sektora - przezroczyste (nNext3)
    TGroundNode *nRenderWires = nullptr; // z poziomu sektora - druty i inne linie (nNext3)
    TGroundNode *nRender = nullptr; // indywidualnie - nieprzezroczyste (nNext3)
    TGroundNode *nRenderMixed = nullptr; // indywidualnie - nieprzezroczyste i przezroczyste (nNext3)
    TGroundNode *nRenderAlpha = nullptr; // indywidualnie - przezroczyste (nNext3)
    int iNodeCount = 0; // licznik obiektów, do pomijania pustych sektorów
  public:
    void LoadNodes(); // utworzenie VBO sektora
  public:
//    TSubRect() = default;
    virtual ~TSubRect();
    virtual void Release(); // zwalnianie VBO sektora
    void NodeAdd(TGroundNode *Node); // dodanie obiektu do sektora na etapie rozdzielania na sektory
    void RaNodeAdd(TGroundNode *Node); // dodanie obiektu do listy renderowania
    void Sort(); // optymalizacja obiektów w sektorze (sortowanie wg tekstur)
    TTrack * FindTrack(vector3 *Point, int &iConnection, TTrack *Exclude);
    TTraction * FindTraction(vector3 *Point, int &iConnection, TTraction *Exclude);
    bool StartVBO(); // ustwienie VBO sektora dla (nRenderRect), (nRenderRectAlpha) i
    // (nRenderWires)
    bool RaTrackAnimAdd(TTrack *t); // zgłoszenie toru do animacji
    void RaAnimate(); // przeliczenie animacji torów
    void RenderDL(); // renderowanie nieprzezroczystych w Display Lists
    void RenderAlphaDL(); // renderowanie przezroczystych w Display Lists
    // (McZapkie-131202)
    void RenderVBO(); // renderowanie nieprzezroczystych z własnego VBO
    void RenderAlphaVBO(); // renderowanie przezroczystych z (własnego) VBO
    void RenderSounds(); // dźwięki pojazdów z niewidocznych sektorów
};

// Ra: trzeba sprawdzić wydajność siatki
const int iNumSubRects = 5; // na ile dzielimy kilometr
const int iNumRects = 500;
// const double fHalfNumRects=iNumRects/2.0; //połowa do wyznaczenia środka
const int iTotalNumSubRects = iNumRects * iNumSubRects;
const double fHalfTotalNumSubRects = iTotalNumSubRects / 2.0;
const double fSubRectSize = 1000.0 / iNumSubRects;
const double fRectSize = fSubRectSize * iNumSubRects;

class TGroundRect : public TSubRect
{ // kwadrat kilometrowy
    // obiekty o niewielkiej ilości wierzchołków będą renderowane stąd
    // Ra: 2012-02 doszły submodele terenu
  private:
    int iLastDisplay; // numer klatki w której był ostatnio wyświetlany
    TSubRect *pSubRects;
    void Init()
    {
        pSubRects = new TSubRect[iNumSubRects * iNumSubRects];
    };

  public:
    static int iFrameNumber; // numer kolejny wyświetlanej klatki
    TGroundNode *nTerrain; // model terenu z E3D - użyć nRootMesh?
    TGroundRect();
    virtual ~TGroundRect();

    TSubRect * SafeGetRect(int iCol, int iRow)
    { // pobranie wskaźnika do małego kwadratu, utworzenie jeśli trzeba
        if (!pSubRects)
            Init(); // utworzenie małych kwadratów
        return pSubRects + iRow * iNumSubRects + iCol; // zwrócenie właściwego
    };
    TSubRect * FastGetRect(int iCol, int iRow)
    { // pobranie wskaźnika do małego kwadratu, bez tworzenia jeśli nie ma
        return (pSubRects ? pSubRects + iRow * iNumSubRects + iCol : NULL);
    };
    void Optimize()
    { // optymalizacja obiektów w sektorach
        if (pSubRects)
            for (int i = iNumSubRects * iNumSubRects - 1; i >= 0; --i)
                pSubRects[i].Sort(); // optymalizacja obiektów w sektorach
    };
    void RenderDL();
    void RenderVBO();
};

class TGround
{
    vector3 CameraDirection; // zmienna robocza przy renderowaniu
    int const *iRange = nullptr; // tabela widoczności
    // TGroundNode *nRootNode; //lista wszystkich węzłów
    TGroundNode *nRootDynamic = nullptr; // lista pojazdów
    TGroundRect Rects[iNumRects][iNumRects]; // mapa kwadratów kilometrowych
    TEvent *RootEvent = nullptr; // lista zdarzeń
    TEvent *QueryRootEvent = nullptr,
           *tmpEvent = nullptr,
           *tmp2Event = nullptr,
           *OldQRE = nullptr;
    TSubRect *pRendered[1500]; // lista renderowanych sektorów
    int iNumNodes = 0;
    vector3 pOrigin;
    vector3 aRotate;
    bool bInitDone = false;
    TGroundNode *nRootOfType[TP_LAST]; // tablica grupująca obiekty, przyspiesza szukanie
    // TGroundNode *nLastOfType[TP_LAST]; //ostatnia
    TSubRect srGlobal; // zawiera obiekty globalne (na razie wyzwalacze czasowe)
    int hh = 0,
        mm = 0,
        srh = 0,
        srm = 0,
        ssh = 0,
        ssm = 0; // ustawienia czasu
    // int tracks,tracksfar; //liczniki torów
#ifdef EU07_USE_OLD_TNAMES_CLASS
    TNames *sTracks = nullptr; // posortowane nazwy torów i eventów
#else
    typedef std::unordered_map<std::string, TEvent *> event_map;
//    typedef std::unordered_map<std::string, TGroundNode *> groundnode_map;
    event_map m_eventmap;
//    groundnode_map m_memcellmap, m_modelmap, m_trackmap;
    TNames<TGroundNode *> m_trackmap;
#endif
  private: // metody prywatne
    bool EventConditon(TEvent *e);

  public:
    bool bDynamicRemove = false; // czy uruchomić procedurę usuwania pojazdów
    TDynamicObject *LastDyn = nullptr; // ABu: paskudnie, ale na bardzo szybko moze jakos przejdzie...
    // TTrain *pTrain;
    // double fVDozwolona;
    // bool bTrabil;

    TGround();
    ~TGround();
    void Free();
    bool Init(std::string asFile, HDC hDC);
    void FirstInit();
    void InitTracks();
    void InitTraction();
    bool InitEvents();
    bool InitLaunchers();
    TTrack * FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude);
    TTraction * FindTraction(vector3 *Point, int &iConnection, TGroundNode *Exclude);
    TTraction * TractionNearestFind(vector3 &p, int dir, TGroundNode *n);
    // TGroundNode* CreateGroundNode();
    TGroundNode * AddGroundNode(cParser *parser);
    bool AddGroundNode(double x, double z, TGroundNode *Node)
    {
        TSubRect *tmp = GetSubRect(x, z);
        if (tmp)
        {
            tmp->NodeAdd(Node);
            return true;
        }
        else
            return false;
    };
    //    bool Include(TQueryParserComp *Parser);
    // TGroundNode* GetVisible(AnsiString asName);
    TGroundNode * GetNode(std::string asName);
    bool AddDynamic(TGroundNode *Node);
    void MoveGroundNode(vector3 pPosition);
    void UpdatePhys(double dt, int iter); // aktualizacja fizyki stałym krokiem
    bool Update(double dt, int iter); // aktualizacja przesunięć zgodna z FPS
    bool AddToQuery(TEvent *Event, TDynamicObject *Node);
    bool GetTraction(TDynamicObject *model);
    bool RenderDL(vector3 pPosition);
    bool RenderAlphaDL(vector3 pPosition);
    bool RenderVBO(vector3 pPosition);
    bool RenderAlphaVBO(vector3 pPosition);
    bool CheckQuery();
    //    GetRect(double x, double z) { return
    //    &(Rects[int(x/fSubRectSize+fHalfNumRects)][int(z/fSubRectSize+fHalfNumRects)]); };
    /*
        int GetRowFromZ(double z) { return (z/fRectSize+fHalfNumRects); };
        int GetColFromX(double x) { return (x/fRectSize+fHalfNumRects); };
        int GetSubRowFromZ(double z) { return (z/fSubRectSize+fHalfNumSubRects); };
       int GetSubColFromX(double x) { return (x/fSubRectSize+fHalfNumSubRects); };
       */
    /*
     inline TGroundNode* FindGroundNode(const AnsiString &asNameToFind )
     {
         if (RootNode)
             return (RootNode->Find( asNameToFind ));
         else
             return NULL;
     }
    */
    TGroundNode * DynamicFindAny(std::string asNameToFind);
    TGroundNode * DynamicFind(std::string asNameToFind);
    void DynamicList(bool all = false);
    TGroundNode * FindGroundNode(std::string asNameToFind, TGroundNodeType iNodeType);
    TGroundRect * GetRect(double x, double z)
    {
        return &Rects[GetColFromX(x) / iNumSubRects][GetRowFromZ(z) / iNumSubRects];
    };
    TSubRect * GetSubRect(double x, double z)
    {
        return GetSubRect(GetColFromX(x), GetRowFromZ(z));
    };
    TSubRect * FastGetSubRect(double x, double z)
    {
        return FastGetSubRect(GetColFromX(x), GetRowFromZ(z));
    };
    TSubRect * GetSubRect(int iCol, int iRow);
    TSubRect * FastGetSubRect(int iCol, int iRow);
    int GetRowFromZ(double z)
    {
        return (z / fSubRectSize + fHalfTotalNumSubRects);
    };
    int GetColFromX(double x)
    {
        return (x / fSubRectSize + fHalfTotalNumSubRects);
    };
    TEvent * FindEvent(const std::string &asEventName);
    TEvent * FindEventScan(const std::string &asEventName);
    void TrackJoin(TGroundNode *Current);

  private:
    void OpenGLUpdate(HDC hDC);
    void RaTriangleDivider(TGroundNode *node);
    void Navigate(std::string const &ClassName, UINT Msg, WPARAM wParam, LPARAM lParam);
    bool PROBLEND;

  public:
    void WyslijEvent(const std::string &e, const std::string &d);
    int iRendered; // ilość renderowanych sektorów, pobierana przy pokazywniu FPS
    void WyslijString(const std::string &t, int n);
    void WyslijWolny(const std::string &t);
    void WyslijNamiary(TGroundNode *t);
    void WyslijParam(int nr, int fl);
	void WyslijUszkodzenia(const std::string &t, char fl);
	void WyslijPojazdy(int nr); // -> skladanie wielu pojazdow
	void WyslijObsadzone(); // -> skladanie wielu pojazdow    
	void RadioStop(vector3 pPosition);
    TDynamicObject * DynamicNearest(vector3 pPosition, double distance = 20.0,
                                              bool mech = false);
    TDynamicObject * CouplerNearest(vector3 pPosition, double distance = 20.0,
                                              bool mech = false);
    void DynamicRemove(TDynamicObject *dyn);
    void TerrainRead(std::string const &f);
    void TerrainWrite();
    void TrackBusyList();
    void IsolatedBusyList();
    void IsolatedBusy(const std::string t);
    void Silence(vector3 gdzie);
};
//---------------------------------------------------------------------------
