//---------------------------------------------------------------------------

#ifndef groundH
#define groundH

#include "dumb3d.h"
#include "ResourceManager.h"
#include "VBO.h"
#include "Classes.h"

using namespace Math3D;

//Ra: zmniejszone liczby, aby zrobi� tabelk� i zoptymalizowa� wyszukiwanie
const int TP_MODEL=10;
const int TP_MESH=11; //Ra: specjalny obiekt grupoj�cy siatki dla tekstury
const int TP_DUMMYTRACK=12; //Ra: zdublowanie obiektu toru dla rozdzielenia tekstur
const int TP_TERRAIN=13; //Ra: specjalny model dla terenu
const int TP_DYNAMIC=14;
const int TP_SOUND=15;
const int TP_TRACK=16;
const int TP_GEOMETRY=17;
const int TP_MEMCELL=18;
const int TP_EVLAUNCH=19; //MC
const int TP_TRACTION=20;
const int TP_TRACTIONPOWERSOURCE= 21; //MC
//const int TP_ISOLATED=22; //Ra
const int TP_SUBMODEL=22; //Ra: submodele terenu
const int TP_LAST=25; //rozmiar tablicy

struct DaneRozkaz
{//struktura komunikacji z EU07.EXE
 int iSygn; //sygnatura 'EU07'
 int iComm; //rozkaz/status (kod ramki)
 union
 {float fPar[62];
  int iPar[62];
  char cString[248]; //upakowane stringi
 };
};



typedef int TGroundNodeType;

struct TGroundVertex
{
 vector3 Point;
 vector3 Normal;
 float tu,tv;
 void HalfSet(const TGroundVertex &v1,const TGroundVertex &v2)
 {//wyliczenie wsp�rz�dnych i mapowania punktu na �rodku odcinka v1<->v2 
  Point=0.5*(v1.Point+v2.Point);
  Normal=0.5*(v1.Normal+v2.Normal);
  tu=0.5*(v1.tu+v2.tu);
  tv=0.5*(v1.tv+v2.tv);
 }
 void SetByX(const TGroundVertex &v1,const TGroundVertex &v2,double x)
 {//wyliczenie wsp�rz�dnych i mapowania punktu na odcinku v1<->v2
  double i=(x-v1.Point.x)/(v2.Point.x-v1.Point.x); //parametr r�wnania
  double j=1.0-i;
  Point=j*v1.Point+i*v2.Point;
  Normal=j*v1.Normal+i*v2.Normal;
  tu=j*v1.tu+i*v2.tu;
  tv=j*v1.tv+i*v2.tv;
 }
 void SetByZ(const TGroundVertex &v1,const TGroundVertex &v2,double z)
 {//wyliczenie wsp�rz�dnych i mapowania punktu na odcinku v1<->v2
  double i=(z-v1.Point.z)/(v2.Point.z-v1.Point.z); //parametr r�wnania
  double j=1.0-i;
  Point=j*v1.Point+i*v2.Point;
  Normal=j*v1.Normal+i*v2.Normal;
  tu=j*v1.tu+i*v2.tu;
  tv=j*v1.tv+i*v2.tv;
 }
};

class TSubRect; //sektor (aktualnie 200m�200m, ale mo�e by� zmieniony)

class TGroundNode : public Resource
{//obiekt scenerii
private:
public:
 TGroundNodeType iType; //typ obiektu
 union
 {//Ra: wska�niki zale�ne od typu - zrobi� klasy dziedziczone zamiast
  void *Pointer; //do przypisywania NULL
  TSubModel *smTerrain; //modele terenu (kwadratow kilometrowych)
  TAnimModel *Model; //model z animacjami
  TDynamicObject *DynamicObject; //pojazd
  vector3 *Points; //punkty dla linii
  TTrack *pTrack; //trajektoria ruchu
  TGroundVertex *Vertices; //wierzcho�ki dla tr�jk�t�w
  TMemCell *MemCell; //kom�rka pami�ci
  TEventLauncher *EvLaunch; //wyzwalacz zdarze�
  TTraction *Traction; //drut zasilaj�cy
  TTractionPowerSource *TractionPowerSource; //zasilanie drutu (zaniedbane w sceneriach)
  TRealSound *pStaticSound; //d�wi�k przestrzenny
  TGroundNode *nNode; //obiekt renderuj�cy grupowo ma tu wska�nik na list� obiekt�w
 };
 AnsiString asName; //nazwa (nie zawsze ma znaczenie)
 union
 {
  int iNumVerts; //dla tr�jk�t�w
  int iNumPts; //dla linii
  int iCount; //dla terenu
  //int iState; //Ra: nie u�ywane - d�wi�ki zap�tlone
 };
 vector3 pCenter; //wsp�rz�dne �rodka do przydzielenia sektora

 union
 {
  //double fAngle; //k�t obrotu dla modelu
  double fLineThickness; //McZapkie-120702: grubosc linii
  //int Status;  //McZapkie-170303: status dzwieku
 };
 double fSquareRadius; //kwadrat widoczno�ci do
 double fSquareMinRadius; //kwadrat widoczno�ci od
 //TGroundNode *nMeshGroup; //Ra: obiekt grupuj�cy tr�jk�ty w TSubRect dla tekstury
 int iVersion; //wersja siatki (do wykonania rekompilacji)
 //union ?
 GLuint DisplayListID; //numer siatki DisplayLists
 bool PROBLEND;
 int iVboPtr; //indeks w buforze VBO
 GLuint TextureID; //g��wna (jedna) tekstura obiektu
 int iFlags; //tryb przezroczysto�ci: 0x10-nieprz.,0x20-przezroczysty,0x30-mieszany
 int Ambient[4],Diffuse[4],Specular[4]; //o�wietlenie
 bool bVisible;
 TGroundNode *Next; //lista wszystkich w scenerii, ostatni na pocz�tku
 TGroundNode *nNext2; //lista obiekt�w w sektorze
 TGroundNode *nNext3; //lista obiekt�w renderowanych we wsp�lnym cyklu
 __fastcall TGroundNode();
 __fastcall TGroundNode(TGroundNodeType t,int n=0);
 __fastcall ~TGroundNode();
 void __fastcall Init(int n);
 void __fastcall InitCenter(); //obliczenie wsp�rz�dnych �rodka
 void __fastcall InitNormals();

 void __fastcall MoveMe(vector3 pPosition); //przesuwanie (nie dzia�a)

 //bool __fastcall Disable();
 inline TGroundNode* __fastcall Find(const AnsiString &asNameToFind,TGroundNodeType iNodeType)
 {//wyszukiwanie czo�gowe z typem
  if ((iNodeType==iType)&&(asNameToFind==asName))
   return this;
  else
   if (Next) return Next->Find(asNameToFind,iNodeType);
  return NULL;
 };

 void __fastcall Compile(bool many=false);
 void Release();
 bool __fastcall GetTraction();

 void __fastcall RenderHidden();   //obs�uga d�wi�k�w i wyzwalaczy zdarze�
 void __fastcall RenderDL();       //renderowanie nieprzezroczystych w Display Lists
 void __fastcall RenderAlphaDL();  //renderowanie przezroczystych w Display Lists (McZapkie-131202)
 void __fastcall RaRenderVBO();    //renderowanie (nieprzezroczystych) ze wsp�lnego VBO
 void __fastcall RenderVBO();      //renderowanie nieprzezroczystych z w�asnego VBO
 void __fastcall RenderAlphaVBO(); //renderowanie przezroczystych z (w�asnego) VBO
};

class TSubRect : public Resource, public CMesh
{//sektor sk�adowy kwadratu kilometrowego
public:
 int iTracks; //ilo�� tor�w w (tTracks)
 TTrack **tTracks; //tory do renderowania pojazd�w
protected:
 TTrack *tTrackAnim; //obiekty do przeliczenia animacji
 TGroundNode *nRootMesh; //obiekty renderuj�ce wg tekstury (wt�rne, lista po nNext2)
 TGroundNode *nMeshed;   //lista obiekt�w dla kt�rych istniej� obiekty renderuj�ce grupowo
public:
 TGroundNode *nRootNode;        //wszystkie obiekty w sektorze, z wyj�tkiem renderuj�cych i pojazd�w (nNext2)
 TGroundNode *nRenderHidden;    //lista obiekt�w niewidocznych, "renderowanych" r�wnie� z ty�u (nNext3)
 TGroundNode *nRenderRect;      //z poziomu sektora - nieprzezroczyste (nNext3)
 TGroundNode *nRenderRectAlpha; //z poziomu sektora - przezroczyste (nNext3)
 TGroundNode *nRenderWires;     //z poziomu sektora - druty i inne linie (nNext3)
 TGroundNode *nRender;          //indywidualnie - nieprzezroczyste (nNext3)
 TGroundNode *nRenderMixed;     //indywidualnie - nieprzezroczyste i przezroczyste (nNext3)
 TGroundNode *nRenderAlpha;     //indywidualnie - przezroczyste (nNext3)
 int iNodeCount; //licznik obiekt�w, do pomijania pustych sektor�w
public:
 void __fastcall LoadNodes(); //utworzenie VBO sektora
public:
 __fastcall TSubRect();
 virtual __fastcall ~TSubRect();
 virtual void Release(); //zwalnianie VBO sektora
 void __fastcall NodeAdd(TGroundNode *Node); //dodanie obiektu do sektora na etapie rozdzielania na sektory
 void __fastcall RaNodeAdd(TGroundNode *Node); //dodanie obiektu do listy renderowania
 void __fastcall Sort(); //optymalizacja obiekt�w w sektorze (sortowanie wg tekstur)
 TTrack* __fastcall FindTrack(vector3 *Point,int &iConnection,TTrack *Exclude);
 bool __fastcall StartVBO(); //ustwienie VBO sektora dla (nRenderRect), (nRenderRectAlpha) i (nRenderWires)
 bool __fastcall RaTrackAnimAdd(TTrack *t); //zg�oszenie toru do animacji
 void __fastcall RaAnimate(); //przeliczenie animacji tor�w
 void __fastcall RenderDL();       //renderowanie nieprzezroczystych w Display Lists
 void __fastcall RenderAlphaDL();  //renderowanie przezroczystych w Display Lists (McZapkie-131202)
 void __fastcall RenderVBO();      //renderowanie nieprzezroczystych z w�asnego VBO
 void __fastcall RenderAlphaVBO(); //renderowanie przezroczystych z (w�asnego) VBO
};

//Ra: trzeba sprawdzi� wydajno�� siatki
const int iNumSubRects=5; //na ile dzielimy kilometr
const int iNumRects=500;
//const double fHalfNumRects=iNumRects/2.0; //po�owa do wyznaczenia �rodka
const int iTotalNumSubRects=iNumRects*iNumSubRects;
const double fHalfTotalNumSubRects=iTotalNumSubRects/2.0;
const double fSubRectSize=1000.0/iNumSubRects;
const double fRectSize=fSubRectSize*iNumSubRects;

class TGroundRect : public TSubRect
{//kwadrat kilometrowy
 //obiekty o niewielkiej ilo�ci wierzcho�k�w b�d� renderowane st�d
 //Ra: 2012-02 dosz�y submodele terenu
private:
 int iLastDisplay; //numer klatki w kt�rej by� ostatnio wy�wietlany
 TSubRect *pSubRects;
 void __fastcall Init() { pSubRects=new TSubRect[iNumSubRects*iNumSubRects]; };
public:
 static int iFrameNumber; //numer kolejny wy�wietlanej klatki
 __fastcall TGroundRect() { pSubRects=NULL; };
 virtual __fastcall ~TGroundRect();

 TSubRect* __fastcall SafeGetRect(int iCol,int iRow)
 {//pobranie wska�nika do ma�ego kwadratu, utworzenie je�li trzeba
  if (!pSubRects) Init(); //utworzenie ma�ych kwadrat�w
  return pSubRects+iRow*iNumSubRects+iCol; //zwr�cenie w�a�ciwego
 };
 TSubRect* __fastcall FastGetRect(int iCol,int iRow)
 {//pobranie wska�nika do ma�ego kwadratu, bez tworzenia je�li nie ma
  return (pSubRects?pSubRects+iRow*iNumSubRects+iCol:NULL);
 };
 void __fastcall Optimize()
 {//optymalizacja obiekt�w w sektorach
  if (pSubRects)
   for (int i=iNumSubRects*iNumSubRects-1;i>=0;--i)
    pSubRects[i].Sort(); //optymalizacja obiekt�w w sektorach
 };
 void __fastcall RenderDL();
 void __fastcall RenderVBO()
 {//renderowanie kwadratu kilometrowego (VBO), je�li jeszcze nie zrobione
  if (iLastDisplay!=iFrameNumber)
  {LoadNodes(); //ewentualne tworzenie siatek
   if (StartVBO())
   {for (TGroundNode* node=nRenderRect;node;node=node->nNext3) //nast�pny tej grupy
     node->RaRenderVBO(); //nieprzezroczyste tr�jk�ty kwadratu kilometrowego
    EndVBO();
    iLastDisplay=iFrameNumber;
   }
  }
 };
};


class TGround
{
 vector3 CameraDirection; //zmienna robocza przy renderowaniu
 int const *iRange; //tabela widoczno�ci
 //TGroundNode *RootNode; //lista wszystkich w�z��w
 TGroundNode *nRootDynamic; //lista pojazd�w
 TGroundRect Rects[iNumRects][iNumRects]; //mapa kwadrat�w kilometrowych
 TEvent *RootEvent; //lista zdarze�
 TEvent *QueryRootEvent,*tmpEvent,*tmp2Event,*OldQRE;
 TSubRect *pRendered[1500]; //lista renderowanych sektor�w
 int iNumNodes;
 vector3 pOrigin;
 vector3 aRotate;
 bool bInitDone;
 TGroundNode *nRootOfType[TP_LAST]; //tablica grupuj�ca obiekty, przyspiesza szukanie
 //TGroundNode *nLastOfType[TP_LAST]; //ostatnia
 TSubRect srGlobal; //zawiera obiekty globalne (na razie wyzwalacze czasowe)
 int hh,mm,srh,srm,ssh,ssm; //ustawienia czasu
 //int tracks,tracksfar; //liczniki tor�w
 TNames *sTracks; //posortowane nazwy tor�w 
public:
 bool bDynamicRemove; //czy uruchomi� procedur� usuwania pojazd�w
 TDynamicObject *LastDyn; //ABu: paskudnie, ale na bardzo szybko moze jakos przejdzie...
 //TTrain *pTrain;
 //double fVDozwolona;
 //bool bTrabil;

 __fastcall TGround();
 __fastcall ~TGround();
 void __fastcall Free();
 bool __fastcall Init(AnsiString asFile,HDC hDC);
 void __fastcall FirstInit();
 bool __fastcall InitEvents();
 bool __fastcall InitTracks();
 bool __fastcall InitLaunchers();
 TTrack* __fastcall FindTrack(vector3 Point,int &iConnection,TGroundNode *Exclude);
 TGroundNode* __fastcall CreateGroundNode();
 TGroundNode* __fastcall AddGroundNode(cParser* parser);
 bool __fastcall AddGroundNode(double x,double z,TGroundNode *Node)
 {
  TSubRect *tmp=GetSubRect(x,z);
  if (tmp)
  {
   tmp->NodeAdd(Node);
   return true;
  }
  else
   return false;
 };
//    bool __fastcall Include(TQueryParserComp *Parser);
 TGroundNode* __fastcall GetVisible( AnsiString asName );
 TGroundNode* __fastcall GetNode( AnsiString asName );
 bool __fastcall AddDynamic(TGroundNode *Node);
 void __fastcall MoveGroundNode(vector3 pPosition);
 bool __fastcall Update(double dt, int iter);
 bool __fastcall AddToQuery(TEvent *Event, TDynamicObject *Node);
 bool __fastcall GetTraction(vector3 pPosition, TDynamicObject *model);
 bool __fastcall RenderDL(vector3 pPosition);
 bool __fastcall RenderAlphaDL(vector3 pPosition);
 bool __fastcall RenderVBO(vector3 pPosition);
 bool __fastcall RenderAlphaVBO(vector3 pPosition);
 bool __fastcall CheckQuery();
//    __fastcall GetRect(double x, double z) { return &(Rects[int(x/fSubRectSize+fHalfNumRects)][int(z/fSubRectSize+fHalfNumRects)]); };
/*
    int __fastcall GetRowFromZ(double z) { return (z/fRectSize+fHalfNumRects); };
    int __fastcall GetColFromX(double x) { return (x/fRectSize+fHalfNumRects); };
    int __fastcall GetSubRowFromZ(double z) { return (z/fSubRectSize+fHalfNumSubRects); };
   int __fastcall GetSubColFromX(double x) { return (x/fSubRectSize+fHalfNumSubRects); };
   */
/*
 inline TGroundNode* __fastcall FindGroundNode(const AnsiString &asNameToFind )
 {
     if (RootNode)
         return (RootNode->Find( asNameToFind ));
     else
         return NULL;
 }
*/
 inline TGroundNode* __fastcall FindDynamic(AnsiString asNameToFind)
 {
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
   if ((Current->asName==asNameToFind))
    return Current;
  return NULL;
 }

 TGroundNode* __fastcall FindGroundNode(AnsiString asNameToFind,TGroundNodeType iNodeType);

//Winger - to smierdzi
/*    inline TGroundNode* __fastcall FindTraction( TGroundNodeType iNodeType )
    {
        TGroundNode *Current;
        TGroundNode *CurrDynObj;
        char trrx, trry, trrz;
        for (Current= RootNode; Current!=NULL; Current= Current->Next)
            if (Current->iType==iNodeType) // && (Current->Points->x )
//              if
                {
                trrx= char(Current->Points->x);
                trry= char(Current->Points->y);
                trrz= char(Current->Points->z);
                if (trrx!=0)
                        {
                        WriteLog("Znalazlem trakcje, qrwa!", trrx + trry + trrz);
                        return Current;
                        }
                }
        return NULL;
    }
*/
 TGroundRect* __fastcall GetRect(double x, double z) { return &Rects[GetColFromX(x)/iNumSubRects][GetRowFromZ(z)/iNumSubRects]; };
 TSubRect* __fastcall GetSubRect(double x, double z) { return GetSubRect(GetColFromX(x),GetRowFromZ(z)); };
 TSubRect* __fastcall FastGetSubRect(double x, double z) { return FastGetSubRect(GetColFromX(x),GetRowFromZ(z)); };
 TSubRect* __fastcall GetSubRect(int iCol, int iRow);
 TSubRect* __fastcall FastGetSubRect(int iCol, int iRow);
 int __fastcall GetRowFromZ(double z) { return (z/fSubRectSize+fHalfTotalNumSubRects); };
 int __fastcall GetColFromX(double x) { return (x/fSubRectSize+fHalfTotalNumSubRects); };
 TEvent* __fastcall FindEvent(const AnsiString &asEventName);
 void __fastcall TrackJoin(TGroundNode *Current);
private:
 void __fastcall OpenGLUpdate(HDC hDC);
 void __fastcall RaTriangleDivider(TGroundNode* node);
 void __fastcall Navigate(String ClassName,UINT Msg,WPARAM wParam,LPARAM lParam);
 bool PROBLEND;
public:
 void __fastcall WyslijEvent(const AnsiString &e,const AnsiString &d);
 int iRendered; //ilo�� renderowanych sektor�w, pobierana przy pokazywniu FPS
 void __fastcall WyslijString(const AnsiString &t,int n);
 void __fastcall WyslijWolny(const AnsiString &t);
 void __fastcall WyslijNamiary(TGroundNode* t);
 void __fastcall RadioStop(vector3 pPosition);
 TDynamicObject* __fastcall DynamicNearest(vector3 pPosition,double distance=20.0,bool mech=false);
 void __fastcall DynamicRemove(TDynamicObject* dyn);
 void __fastcall TerrainRead(const AnsiString &f); 
 void __fastcall TerrainWrite();
};
//---------------------------------------------------------------------------
#endif
