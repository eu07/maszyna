//---------------------------------------------------------------------------

#ifndef groundH
#define groundH

#include    "system.hpp"
#include    "classes.hpp"


//#include "Track.h"
#include "dumb3d.h"
#include "Geometry.h"
#include "QueryParserComp.hpp"
#include "AnimModel.h"
//#include "Semaphore.h"
#include "DynObj.h"
#include "Train.h"
#include "Sound.h"
#include "MemCell.h"
#include "Traction.h"
#include "EvLaunch.h"
#include "TractionPower.h"
#include "mtable.hpp"
//#include "Geom.h"

#include "parser.h" //Tolaris-010603
#include "ResourceManager.h"
#include "VBO.h"

const int TP_MODEL= 1000;
const int TP_SEMAPHORE= 1002;
const int TP_DYNAMIC= 1004;
const int TP_SOUND= 1005;
const int TP_TRACK= 1006;
const int TP_GEOMETRY= 1007;
const int TP_MEMCELL= 1008;
const int TP_EVLAUNCH= 1009; //MC
const int TP_TRACTION= 1010;
const int TP_TRACTIONPOWERSOURCE= 1011; //MC

typedef int TGroundNodeType;

struct TGroundVertex
{
    vector3 Point;
    vector3 Normal;
    float tu,tv;
};

class TGroundNode;

class TSubRect;

class TGroundNode : public Resource
{
private:
public:
    TDynamicObject *NearestDynObj;
    double DistToDynObj;

    TGroundNodeType iType; //typ obiektu
    union
    {
        void *Pointer;
        TAnimModel *Model;
        TDynamicObject *DynamicObject;
        vector3 *Points; //punkty dla linii
        TTrack *pTrack;
        TGroundVertex *Vertices; //wierzcho�ki dla tr�jk�t�w
        TMemCell *MemCell;
        TEventLauncher *EvLaunch;
        TTraction *Traction;
        TTractionPowerSource *TractionPowerSource;
        TRealSound *pStaticSound;
//        TGeometry *pGeometry;
    };
    AnsiString asName;
    union
    {
        int iNumVerts; //dla tr�jk�t�w
        int iNumPts; //dla linii
        //int iState; //Ra: nie u�ywane - d�wi�ki zap�tlone
    };
    vector3 pCenter; //�rodek do przydzielenia sektora

    double fAngle;
    double fSquareRadius; //kwadrat widoczno�ci do
    double fSquareMinRadius; //kwadrat widoczno�ci od
    TGroundNode *pTriGroup; //Ra: obiekt grupuj�cy tr�jk�ty w TSubRect (ogranicza ilo�� DisplayList)
    GLuint DisplayListID; //numer siatki
    //CMesh *pVBO; //dane siatki VBO
    int iVboPtr; //indeks w buforze VBO
    GLuint TextureID; //jedna tekstura na obiekt
    bool TexAlpha;
    float fLineThickness; //McZapkie-120702: grubosc linii
//    int Status;  //McZapkie-170303: status dzwieku
    int Ambient[4],Diffuse[4],Specular[4]; //o�wietlenie
    bool bVisible;
    bool bStatic; //czy nie jest pojazdem
    bool bAllocated; //Ra: zawsze true
    TGroundNode *Next; //lista wszystkich, ostatni na ko�cu
    TGroundNode *Next2; //lista w sektorze
    static TSubRect *pOwner; //tymczasowo w�a�ciciel
    __fastcall TGroundNode();
    __fastcall ~TGroundNode();
    bool __fastcall Init(int n);
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
    bool __fastcall Render();
    bool __fastcall RenderAlpha(); //McZapkie-131202: dwuprzebiegowy rendering
    void __fastcall RenderVBO();
};
TSubRect *TGroundNode::pOwner=NULL; //tymczasowo w�a�ciciel

class TSubRect : public Resource
{
private:
 int m_nVertexCount;         // Ilo�� wierzcho�k�w
 // Dane siatki
 CVert *m_pVertices;         // Dane wierzcho�k�w
 CVec *m_pNormals;         // Dane wierzcho�k�w
 CTexCoord *m_pTexCoords;         // Wsp�rz�dne tekstur
 // Nazwy dla obiekt�w VBO
 unsigned int m_nVBOVertices;         // Nazwa VBO z wierzcho�kami
 unsigned int m_nVBONormals;
 unsigned int m_nVBOTexCoords;         // Nazwa VBO z koordynatami tekstur
 unsigned int m_nType;
 void __fastcall BuildVBOs(); //zamiana tablic na VBO
public:
 void __fastcall LoadNodes();
private:
public:
    //CMesh *pVBO; //dane siatki VBO
    TGroundNode *pRootNode;
    TGroundNode *pTriGroup; //Ra: obiekt grupuj�cy tr�jk�ty (ogranicza ilo�� DisplayList)
    __fastcall TSubRect();
    __fastcall ~TSubRect();
    void __fastcall AddNode(TGroundNode *Node)
    {//przyczepienie obiektu do sektora, kwalifikacja tr�jk�t�w do ��czenia
     Node->Next2=pRootNode;
     pRootNode=Node;
/* //Ra: na razie wy��czone do test�w VBO
     if ((Node->iType==GL_TRIANGLE_STRIP)||(Node->iType==GL_TRIANGLE_FAN)||(Node->iType==GL_TRIANGLES))
      if (Node->fSquareMinRadius==0.0) //znikaj�ce z bliska nie mog� by� optymalizowane
       if (Node->fSquareRadius>=160000.0) //tak od 400m to ju� normalne tr�jk�ty musz� by�
        if (!Node->TexAlpha) //i nieprzezroczysty
        {if (pTriGroup) //je�eli by� ju� jaki� grupuj�cy
         {if (pTriGroup->fSquareRadius>Node->fSquareRadius) //i mia� wi�kszy zasi�g
           Node->fSquareRadius=pTriGroup->fSquareRadius; //zwi�kszenie zakresu widoczno�ci grupuj�cego
          pTriGroup->pTriGroup=Node; //poprzedniemu doczepiamy nowy
         }
         Node->pTriGroup=Node; //nowy lider ma si� sam wy�wietla� - wska�nik na siebie
         pTriGroup=Node; //zapami�tanie lidera
        }
*/
    };
//    __fastcall Render() { if (pRootNode) pRootNode->Render(); };
 void Release();
 bool __fastcall StartVBO();
 void __fastcall EndVBO();
};

const int iNumSubRects= 10; //Ra: trzeba sprawdzi� wydajno�� siatki

class TGroundRect
{
private:
    TSubRect *pSubRects;
    void __fastcall Init() { pSubRects= new TSubRect[iNumSubRects*iNumSubRects]; };

public:
    __fastcall TGroundRect() { pSubRects=NULL; };
    __fastcall ~TGroundRect() { SafeDeleteArray(pSubRects); };

    TSubRect* __fastcall SafeGetRect( int iCol, int iRow) { if (!pSubRects) Init();  return pSubRects+iRow*iNumSubRects+iCol; };
    TSubRect* __fastcall FastGetRect( int iCol, int iRow) { return ( pSubRects ? pSubRects+iRow*iNumSubRects+iCol : NULL ); };
};

const int iNumRects= 500;
const double fHalfNumRects= iNumRects/2;

const int iTotalNumSubRects= iNumRects*iNumSubRects;
const double fHalfTotalNumSubRects= iTotalNumSubRects/2;

const double fSubRectSize= 100.0f;
const double fRectSize= fSubRectSize*iNumSubRects;


class TGround
{
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
    TSubRect* __fastcall GetSubRect(double x, double z) { return GetSubRect(GetColFromX(x),GetRowFromZ(z)); };
    TSubRect* __fastcall FastGetSubRect(double x, double z) { return FastGetSubRect(GetColFromX(x),GetRowFromZ(z)); };
    TSubRect* __fastcall GetSubRect(int iCol, int iRow);
    TSubRect* __fastcall FastGetSubRect(int iCol, int iRow);
    int __fastcall GetRowFromZ(double z) { return (z/fSubRectSize+fHalfTotalNumSubRects); };
    int __fastcall GetColFromX(double x) { return (x/fSubRectSize+fHalfTotalNumSubRects); };
    TEvent* __fastcall FindEvent(AnsiString asEventName);
    void __fastcall TrackJoin(TGroundNode *Current);
private:
    TGroundNode *RootNode; //lista w�z��w
//    TGroundNode *FirstVisible,*LastVisible;
    TGroundNode *RootDynamic; //lista pojazd�w

    TGroundRect Rects[iNumRects][iNumRects]; //mapa kwadrat�w kilometrowych

    TEvent *RootEvent; //lista zdarze�
    TEvent *QueryRootEvent,*tmpEvent,*tmp2Event,*OldQRE;

    void __fastcall OpenGLUpdate(HDC hDC);
//    TWorld World;

    int iNumNodes;
    vector3 pOrigin;
    vector3 aRotate;
    bool bInitDone;
};
//---------------------------------------------------------------------------
#endif
