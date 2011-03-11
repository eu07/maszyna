//---------------------------------------------------------------------------

#ifndef groundH
#define groundH

#include "system.hpp"
#include "classes.hpp"


#include "dumb3d.h"
#include "Geometry.h"
#include "QueryParserComp.hpp"
#include "AnimModel.h"
#include "DynObj.h"
#include "Train.h"
#include "Sound.h"
#include "MemCell.h"
#include "Traction.h"
#include "EvLaunch.h"
#include "TractionPower.h"
#include "mtable.hpp"

#include "parser.h" //Tolaris-010603
#include "ResourceManager.h"
#include "VBO.h"

const int TP_MODEL= 1000;
const int TP_SEMAPHORE= 1002; //Ra: ju¿ nie u¿ywane
const int TP_DYNAMIC= 1004;
const int TP_SOUND= 1005;
const int TP_TRACK= 1006;
const int TP_GEOMETRY= 1007;
const int TP_MEMCELL= 1008;
const int TP_EVLAUNCH= 1009; //MC
const int TP_TRACTION= 1010;
const int TP_TRACTIONPOWERSOURCE= 1011; //MC
const int TP_ISOLATED=1012; //Ra

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
 {Point=0.5*(v1.Point+v2.Point);
  Normal=0.5*(v1.Normal+v2.Normal);
  tu=0.5*(v1.tu+v2.tu);
  tv=0.5*(v1.tv+v2.tv);
 }
};

class TGroundNode;

class TSubRect;

class TGroundNode : public Resource
{
private:
public:
    //TDynamicObject *NearestDynObj;
    //double DistToDynObj;

    TGroundNodeType iType; //typ obiektu
    union
    {
     void *Pointer; //do przypisywania NULL
     TAnimModel *Model;
     TDynamicObject *DynamicObject;
     vector3 *Points; //punkty dla linii
     TTrack *pTrack;
     TGroundVertex *Vertices; //wierzcho³ki dla trójk¹tów
     TMemCell *MemCell;
     TEventLauncher *EvLaunch;
     TTraction *Traction;
     TTractionPowerSource *TractionPowerSource;
     TRealSound *pStaticSound;
    };
    AnsiString asName;
    union
    {
     int iNumVerts; //dla trójk¹tów
     int iNumPts; //dla linii
     //int iState; //Ra: nie u¿ywane - dŸwiêki zapêtlone
    };
    vector3 pCenter; //œrodek do przydzielenia sektora

    union
    {
     double fAngle; //k¹t obrotu dla modelu
     double fLineThickness; //McZapkie-120702: grubosc linii
     //int Status;  //McZapkie-170303: status dzwieku
    };
    double fSquareRadius; //kwadrat widocznoœci do
    double fSquareMinRadius; //kwadrat widocznoœci od
    TGroundNode *pTriGroup; //Ra: obiekt grupuj¹cy trójk¹ty w TSubRect (ogranicza iloœæ DisplayList)
    GLuint DisplayListID; //numer siatki
    int iVboPtr; //indeks w buforze VBO
    GLuint TextureID; //jedna tekstura na obiekt
    int iFlags; //tryb przezroczystoœci: 2-nieprz.,4-przezroczysty,6-mieszany
    int Ambient[4],Diffuse[4],Specular[4]; //oœwietlenie
    bool bVisible;
    bool bStatic; //czy nie jest pojazdem - do zredukowania
    bool bAllocated; //Ra: zawsze true
    TGroundNode *Next; //lista wszystkich w scenerii, ostatni na pocz¹tku
    TGroundNode *pNext2; //lista w sektorze
    TGroundNode *pNext3; //lista obiektów podobnych, renderowanych grupowo
    __fastcall TGroundNode();
    __fastcall ~TGroundNode();
    void __fastcall Init(int n);
    void __fastcall InitCenter();
    void __fastcall InitNormals();

    void __fastcall MoveMe(vector3 pPosition);

    //bool __fastcall Disable();
    inline TGroundNode* __fastcall Find(const AnsiString &asNameToFind)
    {
        if (asNameToFind==asName) return this; else if (Next) return Next->Find(asNameToFind);
        return NULL;
    };
    inline TGroundNode* __fastcall Find(const AnsiString &asNameToFind, TGroundNodeType iNodeType )
    {
        if ((iNodeType==iType) && (asNameToFind==asName))
            return this;
        else
            if (Next) return Next->Find(asNameToFind,iNodeType);
        return NULL;
    };

    void __fastcall Compile();
    void Release();

    bool __fastcall GetTraction();
    void __fastcall RenderHidden();
    void __fastcall Render();
    void __fastcall RenderAlpha(); //McZapkie-131202: dwuprzebiegowy rendering
    void __fastcall RaRenderVBO();
    void __fastcall RaRender();
    void __fastcall RaRenderAlpha(); //McZapkie-131202: dwuprzebiegowy rendering
};
//TSubRect *TGroundNode::pOwner=NULL; //tymczasowo w³aœciciel

class TSubRect : public Resource, public CMesh
{
private:
 TGroundNode *pTriGroup; //Ra: obiekt grupuj¹cy trójk¹ty (ogranicza iloœæ DisplayList)
 TTrack *pTrackAnim; //obiekty do przeliczenia animacji
public:
 TGroundNode *pRootNode; //lista wszystkich obiektów w sektorze
 TGroundNode *pRenderHidden; //lista obiektów niewidocznych, "renderowanych" równie¿ z ty³u
 TGroundNode *pRenderVBO;      //lista grup renderowanych ze wspólnego VBO
 TGroundNode *pRenderAlphaVBO; //lista grup renderowanych ze wspólnego VBO
 TGroundNode *pRender;      //z w³asnych VBO - nieprzezroczyste
 TGroundNode *pRenderMixed; //z w³asnych VBO - nieprzezroczyste i przezroczyste
 TGroundNode *pRenderAlpha; //z w³asnych VBO - przezroczyste
 TGroundNode *pRenderWires; //z w³asnych VBO - druty
 void __fastcall LoadNodes();
public:
 __fastcall TSubRect();
 virtual __fastcall ~TSubRect();
 void __fastcall RaAddNode(TGroundNode *Node);
 void __fastcall AddNode(TGroundNode *Node);
 //void __fastcall RaGroupAdd(TGroundNode *Node) {if (pTriGroup) Node->pTriGroup=pTriGroup; else pTriGroup=Node;};
 //__fastcall Render() { if (pRootNode) pRootNode->Render(); };
 bool __fastcall StartVBO();
 virtual void Release();
 bool __fastcall RaTrackAnimAdd(TTrack *t);
 void __fastcall RaAnimate();
};

//Ra: trzeba sprawdziæ wydajnoœæ siatki
const int iNumSubRects=5; //na ile dzielimy kilometr
const int iNumRects=500;
const double fHalfNumRects=iNumRects/2.0; //po³owa do wyznaczenia œrodka
const int iTotalNumSubRects=iNumRects*iNumSubRects;
const double fHalfTotalNumSubRects=iTotalNumSubRects/2.0;
const double fSubRectSize=1000.0/iNumSubRects;
const double fRectSize=fSubRectSize*iNumSubRects;

class TGroundRect : public TSubRect
{//obiekty o niewielkiej iloœci wierzcho³ków bêd¹ renderowane st¹d
private:
 int iLastDisplay; //numer klatki w której by³ ostatnio wyœwietlany
 TSubRect *pSubRects;
 void __fastcall Init() { pSubRects= new TSubRect[iNumSubRects*iNumSubRects]; };
public:
 static int iFrameNumber; //numer kolejny wyœwietlanej klatki
 __fastcall TGroundRect() { pSubRects=NULL; };
 virtual __fastcall ~TGroundRect() { SafeDeleteArray(pSubRects); };

 TSubRect* __fastcall SafeGetRect(int iCol,int iRow)
 {//pobranie wskaŸnika do ma³ego kwadratu, utworzenie jeœli trzeba
  if (!pSubRects) Init(); //utworzenie ma³ych kwadratów
  return pSubRects+iRow*iNumSubRects+iCol; //zwrócenie w³aœciwego
 };
 TSubRect* __fastcall FastGetRect( int iCol, int iRow)
 {//pobranie wskaŸnika do ma³ego kwadratu, bez tworzenia jeœli nie ma
  return (pSubRects?pSubRects+iRow*iNumSubRects+iCol:NULL);
 };
 void __fastcall Render()
 {//renderowanie kwadratu kilometrowego, jeœli jeszcze nie zrobione
  if (iLastDisplay!=iFrameNumber)
  {for (TGroundNode* node=pRender;node!=NULL;node=node->pNext3)
    node->Render(); //nieprzezroczyste obiekty (pojazdy z automatu)
   iLastDisplay=iFrameNumber;
  }
 };
};



class TGround
{
 vector3 CameraDirection; //zmienna robocza przy renderowaniu
 int const *iRange; //tabela widocznoœci
public:
    TDynamicObject *LastDyn; //ABu: paskudnie, ale na bardzo szybko moze jakos przejdzie...
    TTrain *pTrain;
//    double fVDozwolona;
  //  bool bTrabil;

    __fastcall TGround();
    __fastcall ~TGround();
    void __fastcall Free();
    bool __fastcall Init(AnsiString asFile);
    bool __fastcall InitEvents();
    bool __fastcall InitTracks();
    bool __fastcall InitLaunchers();
    TGroundNode* __fastcall FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude);
    TGroundNode* __fastcall CreateGroundNode();
    TGroundNode* __fastcall AddGroundNode(cParser* parser);
    bool __fastcall AddGroundNode(double x, double z, TGroundNode *Node)
    {
        TSubRect *tmp= GetSubRect(x,z);
        if (tmp)
        {
            tmp->AddNode(Node);
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
    bool __fastcall Render(vector3 pPosition);
    bool __fastcall RenderAlpha(vector3 pPosition);
    bool __fastcall RaRender(vector3 pPosition);
    bool __fastcall RaRenderAlpha(vector3 pPosition);
    bool __fastcall CheckQuery();
//    __fastcall GetRect(double x, double z) { return &(Rects[int(x/fSubRectSize+fHalfNumRects)][int(z/fSubRectSize+fHalfNumRects)]); };
/*
    int __fastcall GetRowFromZ(double z) { return (z/fRectSize+fHalfNumRects); };
    int __fastcall GetColFromX(double x) { return (x/fRectSize+fHalfNumRects); };
    int __fastcall GetSubRowFromZ(double z) { return (z/fSubRectSize+fHalfNumSubRects); };
   int __fastcall GetSubColFromX(double x) { return (x/fSubRectSize+fHalfNumSubRects); };
   */
    inline TGroundNode* __fastcall FindGroundNode(const AnsiString &asNameToFind )
    {
        if (RootNode)
            return (RootNode->Find( asNameToFind ));
        else
            return NULL;
    }

    inline TGroundNode* __fastcall FindDynamic( AnsiString asNameToFind )
    {
        for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
            if ((Current->asName==asNameToFind))
                return Current;
        return NULL;
    }

    inline TGroundNode* __fastcall FindGroundNode( AnsiString asNameToFind, TGroundNodeType iNodeType )
    {
        TGroundNode *Current;
        for (Current= RootNode; Current!=NULL; Current= Current->Next)
            if ((Current->iType==iNodeType) && (Current->asName==asNameToFind))
                return Current;
        return NULL;
    }

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
    TGroundNode *RootNode; //lista wêz³ów
//    TGroundNode *FirstVisible,*LastVisible;
    TGroundNode *RootDynamic; //lista pojazdów

    TGroundRect Rects[iNumRects][iNumRects]; //mapa kwadratów kilometrowych

    TEvent *RootEvent; //lista zdarzeñ
    TEvent *QueryRootEvent,*tmpEvent,*tmp2Event,*OldQRE;
    TSubRect *pRendered[16*iNumSubRects*iNumSubRects+8*iNumSubRects+1]; //lista renderowanych sektorów
    int iRendered;

    void __fastcall OpenGLUpdate(HDC hDC);
//    TWorld World;

    int iNumNodes;
    vector3 pOrigin;
    vector3 aRotate;
    bool bInitDone;
    void __fastcall RaTriangleDivider(TGroundNode* node);
 void __fastcall Navigate(String ClassName,UINT Msg,WPARAM wParam,LPARAM lParam);
 void __fastcall WyslijEvent(const AnsiString &e,const AnsiString &d);
public:
 void __fastcall WyslijString(const AnsiString &t,int n);
 void __fastcall WyslijWolny(const AnsiString &t);
};
//---------------------------------------------------------------------------
#endif
