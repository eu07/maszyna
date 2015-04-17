/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef groundH
#define groundH

#include "dumb3d.h"
#include "ResourceManager.h"
#include "VBO.h"
#include "Classes.h"

using namespace Math3D;

// Ra: zmniejszone liczby, aby zrobiæ tabelkê i zoptymalizowaæ wyszukiwanie
const int TP_MODEL = 10;
const int TP_MESH = 11; // Ra: specjalny obiekt grupoj¹cy siatki dla tekstury
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

typedef int TGroundNodeType;

struct TGroundVertex
{
    vector3 Point;
    vector3 Normal;
    float tu, tv;
    void HalfSet(const TGroundVertex &v1, const TGroundVertex &v2)
    { // wyliczenie wspó³rzêdnych i mapowania punktu na œrodku odcinka v1<->v2
        Point = 0.5 * (v1.Point + v2.Point);
        Normal = 0.5 * (v1.Normal + v2.Normal);
        tu = 0.5 * (v1.tu + v2.tu);
        tv = 0.5 * (v1.tv + v2.tv);
    }
    void SetByX(const TGroundVertex &v1, const TGroundVertex &v2, double x)
    { // wyliczenie wspó³rzêdnych i mapowania punktu na odcinku v1<->v2
        double i = (x - v1.Point.x) / (v2.Point.x - v1.Point.x); // parametr równania
        double j = 1.0 - i;
        Point = j * v1.Point + i * v2.Point;
        Normal = j * v1.Normal + i * v2.Normal;
        tu = j * v1.tu + i * v2.tu;
        tv = j * v1.tv + i * v2.tv;
    }
    void SetByZ(const TGroundVertex &v1, const TGroundVertex &v2, double z)
    { // wyliczenie wspó³rzêdnych i mapowania punktu na odcinku v1<->v2
        double i = (z - v1.Point.z) / (v2.Point.z - v1.Point.z); // parametr równania
        double j = 1.0 - i;
        Point = j * v1.Point + i * v2.Point;
        Normal = j * v1.Normal + i * v2.Normal;
        tu = j * v1.tu + i * v2.tu;
        tv = j * v1.tv + i * v2.tv;
    }
};

class TSubRect; // sektor (aktualnie 200m×200m, ale mo¿e byæ zmieniony)

class TGroundNode : public Resource
{ // obiekt scenerii
  private:
  public:
    TGroundNodeType iType; // typ obiektu
    union
    { // Ra: wska¿niki zale¿ne od typu - zrobiæ klasy dziedziczone zamiast
        void *Pointer; // do przypisywania NULL
        TSubModel *smTerrain; // modele terenu (kwadratow kilometrowych)
        TAnimModel *Model; // model z animacjami
        TDynamicObject *DynamicObject; // pojazd
        vector3 *Points; // punkty dla linii
        TTrack *pTrack; // trajektoria ruchu
        TGroundVertex *Vertices; // wierzcho³ki dla trójk¹tów
        TMemCell *MemCell; // komórka pamiêci
        TEventLauncher *EvLaunch; // wyzwalacz zdarzeñ
        TTraction *hvTraction; // drut zasilaj¹cy
        TTractionPowerSource *psTractionPowerSource; // zasilanie drutu (zaniedbane w sceneriach)
        TTextSound *tsStaticSound; // dŸwiêk przestrzenny
        TGroundNode *nNode; // obiekt renderuj¹cy grupowo ma tu wskaŸnik na listê obiektów
    };
    AnsiString asName; // nazwa (nie zawsze ma znaczenie)
    union
    {
        int iNumVerts; // dla trójk¹tów
        int iNumPts; // dla linii
        int iCount; // dla terenu
        // int iState; //Ra: nie u¿ywane - dŸwiêki zapêtlone
    };
    vector3 pCenter; // wspó³rzêdne œrodka do przydzielenia sektora

    union
    {
        // double fAngle; //k¹t obrotu dla modelu
        double fLineThickness; // McZapkie-120702: grubosc linii
        // int Status;  //McZapkie-170303: status dzwieku
    };
    double fSquareRadius; // kwadrat widocznoœci do
    double fSquareMinRadius; // kwadrat widocznoœci od
    // TGroundNode *nMeshGroup; //Ra: obiekt grupuj¹cy trójk¹ty w TSubRect dla tekstury
    int iVersion; // wersja siatki (do wykonania rekompilacji)
    // union ?
    GLuint DisplayListID; // numer siatki DisplayLists
    bool PROBLEND;
    int iVboPtr; // indeks w buforze VBO
    GLuint TextureID; // g³ówna (jedna) tekstura obiektu
    int iFlags; // tryb przezroczystoœci: 0x10-nieprz.,0x20-przezroczysty,0x30-mieszany
    int Ambient[4], Diffuse[4], Specular[4]; // oœwietlenie
    bool bVisible;
    TGroundNode *nNext; // lista wszystkich w scenerii, ostatni na pocz¹tku
    TGroundNode *nNext2; // lista obiektów w sektorze
    TGroundNode *nNext3; // lista obiektów renderowanych we wspólnym cyklu
    TGroundNode();
    TGroundNode(TGroundNodeType t, int n = 0);
    ~TGroundNode();
    void Init(int n);
    void InitCenter(); // obliczenie wspó³rzêdnych œrodka
    void InitNormals();

    void MoveMe(vector3 pPosition); // przesuwanie (nie dzia³a)

    // bool Disable();
    inline TGroundNode *__fastcall Find(const AnsiString &asNameToFind, TGroundNodeType iNodeType)
    { // wyszukiwanie czo³gowe z typem
        if ((iNodeType == iType) && (asNameToFind == asName))
            return this;
        else if (nNext)
            return nNext->Find(asNameToFind, iNodeType);
        return NULL;
    };

    void Compile(bool many = false);
    void Release();
    bool GetTraction();

    void RenderHidden(); // obs³uga dŸwiêków i wyzwalaczy zdarzeñ
    void RenderDL(); // renderowanie nieprzezroczystych w Display Lists
    void RenderAlphaDL(); // renderowanie przezroczystych w Display Lists
                                     // (McZapkie-131202)
    void RaRenderVBO(); // renderowanie (nieprzezroczystych) ze wspólnego VBO
    void RenderVBO(); // renderowanie nieprzezroczystych z w³asnego VBO
    void RenderAlphaVBO(); // renderowanie przezroczystych z (w³asnego) VBO
};

class TSubRect : public Resource, public CMesh
{ // sektor sk³adowy kwadratu kilometrowego
  public:
    int iTracks; // iloœæ torów w (tTracks)
    TTrack **tTracks; // tory do renderowania pojazdów
  protected:
    TTrack *tTrackAnim; // obiekty do przeliczenia animacji
    TGroundNode *nRootMesh; // obiekty renderuj¹ce wg tekstury (wtórne, lista po nNext2)
    TGroundNode *nMeshed; // lista obiektów dla których istniej¹ obiekty renderuj¹ce grupowo
  public:
    TGroundNode *
        nRootNode; // wszystkie obiekty w sektorze, z wyj¹tkiem renderuj¹cych i pojazdów (nNext2)
    TGroundNode *
        nRenderHidden; // lista obiektów niewidocznych, "renderowanych" równie¿ z ty³u (nNext3)
    TGroundNode *nRenderRect; // z poziomu sektora - nieprzezroczyste (nNext3)
    TGroundNode *nRenderRectAlpha; // z poziomu sektora - przezroczyste (nNext3)
    TGroundNode *nRenderWires; // z poziomu sektora - druty i inne linie (nNext3)
    TGroundNode *nRender; // indywidualnie - nieprzezroczyste (nNext3)
    TGroundNode *nRenderMixed; // indywidualnie - nieprzezroczyste i przezroczyste (nNext3)
    TGroundNode *nRenderAlpha; // indywidualnie - przezroczyste (nNext3)
    int iNodeCount; // licznik obiektów, do pomijania pustych sektorów
  public:
    void LoadNodes(); // utworzenie VBO sektora
  public:
    TSubRect();
    virtual ~TSubRect();
    virtual void Release(); // zwalnianie VBO sektora
    void NodeAdd(
        TGroundNode *Node); // dodanie obiektu do sektora na etapie rozdzielania na sektory
    void RaNodeAdd(TGroundNode *Node); // dodanie obiektu do listy renderowania
    void Sort(); // optymalizacja obiektów w sektorze (sortowanie wg tekstur)
    TTrack *__fastcall FindTrack(vector3 *Point, int &iConnection, TTrack *Exclude);
    TTraction *__fastcall FindTraction(vector3 *Point, int &iConnection, TTraction *Exclude);
    bool StartVBO(); // ustwienie VBO sektora dla (nRenderRect), (nRenderRectAlpha) i
                                // (nRenderWires)
    bool RaTrackAnimAdd(TTrack *t); // zg³oszenie toru do animacji
    void RaAnimate(); // przeliczenie animacji torów
    void RenderDL(); // renderowanie nieprzezroczystych w Display Lists
    void RenderAlphaDL(); // renderowanie przezroczystych w Display Lists
                                     // (McZapkie-131202)
    void RenderVBO(); // renderowanie nieprzezroczystych z w³asnego VBO
    void RenderAlphaVBO(); // renderowanie przezroczystych z (w³asnego) VBO
    void RenderSounds(); // dŸwiêki pojazdów z niewidocznych sektorów
};

// Ra: trzeba sprawdziæ wydajnoœæ siatki
const int iNumSubRects = 5; // na ile dzielimy kilometr
const int iNumRects = 500;
// const double fHalfNumRects=iNumRects/2.0; //po³owa do wyznaczenia œrodka
const int iTotalNumSubRects = iNumRects * iNumSubRects;
const double fHalfTotalNumSubRects = iTotalNumSubRects / 2.0;
const double fSubRectSize = 1000.0 / iNumSubRects;
const double fRectSize = fSubRectSize * iNumSubRects;

class TGroundRect : public TSubRect
{ // kwadrat kilometrowy
    // obiekty o niewielkiej iloœci wierzcho³ków bêd¹ renderowane st¹d
    // Ra: 2012-02 dosz³y submodele terenu
  private:
    int iLastDisplay; // numer klatki w której by³ ostatnio wyœwietlany
    TSubRect *pSubRects;
    void Init() { pSubRects = new TSubRect[iNumSubRects * iNumSubRects]; };

  public:
    static int iFrameNumber; // numer kolejny wyœwietlanej klatki
    TGroundNode *nTerrain; // model terenu z E3D - u¿yæ nRootMesh?
    TGroundRect();
    virtual ~TGroundRect();

    TSubRect *__fastcall SafeGetRect(int iCol, int iRow)
    { // pobranie wskaŸnika do ma³ego kwadratu, utworzenie jeœli trzeba
        if (!pSubRects)
            Init(); // utworzenie ma³ych kwadratów
        return pSubRects + iRow * iNumSubRects + iCol; // zwrócenie w³aœciwego
    };
    TSubRect *__fastcall FastGetRect(int iCol, int iRow)
    { // pobranie wskaŸnika do ma³ego kwadratu, bez tworzenia jeœli nie ma
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
    int const *iRange; // tabela widocznoœci
    // TGroundNode *nRootNode; //lista wszystkich wêz³ów
    TGroundNode *nRootDynamic; // lista pojazdów
    TGroundRect Rects[iNumRects][iNumRects]; // mapa kwadratów kilometrowych
    TEvent *RootEvent; // lista zdarzeñ
    TEvent *QueryRootEvent, *tmpEvent, *tmp2Event, *OldQRE;
    TSubRect *pRendered[1500]; // lista renderowanych sektorów
    int iNumNodes;
    vector3 pOrigin;
    vector3 aRotate;
    bool bInitDone;
    TGroundNode *nRootOfType[TP_LAST]; // tablica grupuj¹ca obiekty, przyspiesza szukanie
    // TGroundNode *nLastOfType[TP_LAST]; //ostatnia
    TSubRect srGlobal; // zawiera obiekty globalne (na razie wyzwalacze czasowe)
    int hh, mm, srh, srm, ssh, ssm; // ustawienia czasu
    // int tracks,tracksfar; //liczniki torów
    TNames *sTracks; // posortowane nazwy torów i eventów
  private: // metody prywatne
    bool EventConditon(TEvent *e);

  public:
    bool bDynamicRemove; // czy uruchomiæ procedurê usuwania pojazdów
    TDynamicObject *LastDyn; // ABu: paskudnie, ale na bardzo szybko moze jakos przejdzie...
    // TTrain *pTrain;
    // double fVDozwolona;
    // bool bTrabil;

    TGround();
    ~TGround();
    void Free();
    bool Init(AnsiString asFile, HDC hDC);
    void FirstInit();
    void InitTracks();
    void InitTraction();
    bool InitEvents();
    bool InitLaunchers();
    TTrack *__fastcall FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude);
    TTraction *__fastcall FindTraction(vector3 *Point, int &iConnection, TGroundNode *Exclude);
    TTraction *__fastcall TractionNearestFind(vector3 &p, int dir, TGroundNode *n);
    // TGroundNode* CreateGroundNode();
    TGroundNode *__fastcall AddGroundNode(cParser *parser);
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
    TGroundNode *__fastcall GetNode(AnsiString asName);
    bool AddDynamic(TGroundNode *Node);
    void MoveGroundNode(vector3 pPosition);
    void UpdatePhys(double dt, int iter); // aktualizacja fizyki sta³ym krokiem
    bool Update(double dt, int iter); // aktualizacja przesuniêæ zgodna z FPS
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
    TGroundNode *__fastcall DynamicFindAny(AnsiString asNameToFind);
    TGroundNode *__fastcall DynamicFind(AnsiString asNameToFind);
    void DynamicList(bool all = false);
    TGroundNode *__fastcall FindGroundNode(AnsiString asNameToFind, TGroundNodeType iNodeType);
    TGroundRect *__fastcall GetRect(double x, double z)
    {
        return &Rects[GetColFromX(x) / iNumSubRects][GetRowFromZ(z) / iNumSubRects];
    };
    TSubRect *__fastcall GetSubRect(double x, double z)
    {
        return GetSubRect(GetColFromX(x), GetRowFromZ(z));
    };
    TSubRect *__fastcall FastGetSubRect(double x, double z)
    {
        return FastGetSubRect(GetColFromX(x), GetRowFromZ(z));
    };
    TSubRect *__fastcall GetSubRect(int iCol, int iRow);
    TSubRect *__fastcall FastGetSubRect(int iCol, int iRow);
    int GetRowFromZ(double z) { return (z / fSubRectSize + fHalfTotalNumSubRects); };
    int GetColFromX(double x) { return (x / fSubRectSize + fHalfTotalNumSubRects); };
    TEvent *__fastcall FindEvent(const AnsiString &asEventName);
    TEvent *__fastcall FindEventScan(const AnsiString &asEventName);
    void TrackJoin(TGroundNode *Current);

  private:
    void OpenGLUpdate(HDC hDC);
    void RaTriangleDivider(TGroundNode *node);
    void Navigate(String ClassName, UINT Msg, WPARAM wParam, LPARAM lParam);
    bool PROBLEND;

  public:
    void WyslijEvent(const AnsiString &e, const AnsiString &d);
    int iRendered; // iloœæ renderowanych sektorów, pobierana przy pokazywniu FPS
    void WyslijString(const AnsiString &t, int n);
    void WyslijWolny(const AnsiString &t);
    void WyslijNamiary(TGroundNode *t);
    void WyslijParam(int nr, int fl);
    void RadioStop(vector3 pPosition);
    TDynamicObject *__fastcall DynamicNearest(vector3 pPosition, double distance = 20.0,
                                              bool mech = false);
    TDynamicObject *__fastcall CouplerNearest(vector3 pPosition, double distance = 20.0,
                                              bool mech = false);
    void DynamicRemove(TDynamicObject *dyn);
    void TerrainRead(const AnsiString &f);
    void TerrainWrite();
    void TrackBusyList();
    void IsolatedBusyList();
    void IsolatedBusy(const AnsiString t);
    void Silence(vector3 gdzie);
};
//---------------------------------------------------------------------------
#endif
