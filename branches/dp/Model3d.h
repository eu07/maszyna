//---------------------------------------------------------------------------
#ifndef Model3dH
#define Model3dH

#include "opengl/glew.h"
#include "Parser.h"
#include "dumb3d.h"
using namespace Math3D;
#include "Float3d.h"
#include "VBO.h"

struct GLVERTEX
{
 vector3 Point;
 vector3 Normal;
 float tu,tv;
};

class TStringPack
{
 char *data;
 //+0 - 4 bajty: typ kromki
 //+4 - 4 bajty: d�ugo�� ��cznie z nag��wkiem
 //+8 - obszar �a�cuch�w znakowych, ka�dy zako�czony zerem
 int *index;
 //+0 - 4 bajty: typ kromki
 //+4 - 4 bajty: d�ugo�� ��cznie z nag��wkiem
 //+8 - tabela indeks�w
public:
 char* String(int n);
 char* StringAt(int n) {return data+9+n;};
 __fastcall TStringPack() {data=NULL; index=NULL;};
 void __fastcall Init(char *d) {data=d;};
 void __fastcall InitIndex(int *i) {index=i;};
};

class TMaterialColor
{
public:
    __fastcall TMaterialColor() {};
    __fastcall TMaterialColor(char V)
    {
        r=g=b=V;
    };
 // __fastcall TMaterialColor(char R, char G, char B)
 // {
 //  r=R; g=G; b=B;
 // };
 // __fastcall TMaterialColor(double R, double G, double B)
    __fastcall TMaterialColor(char R, char G, char B)
    {
        r=R; g=G; b=B;
    };

    char r,g,b;
};

/*
struct TMaterial
{
    int ID;
    AnsiString Name;
//McZapkie-240702: lepiej uzywac wartosci float do opisu koloru bo funkcje opengl chyba tego na ogol uzywaja
    float Ambient[4];
    float Diffuse[4];
    float Specular[4];
    float Transparency;
    GLuint TextureID;
};
*/
/*
struct THitBoxContainer
{
    TPlane Planes[6];
    int Index;
    inline void __fastcall Reset() { Planes[0]= TPlane(vector3(0,0,0),0.0f); };
    inline bool __fastcall Inside(vector3 Point)
    {
        bool Hit= true;

        if (Planes[0].Defined())
            for (int i=0; i<6; i++)
            {
                if (Planes[i].GetSide(Point)>0)
                {
                    Hit= false;
                    break;
                };

        }
        else return(false);
        return(Hit);
    };
};
*/

/* Ra: tego nie b�dziemy ju� u�ywa�, bo mo�na wycisn�� wi�cej
typedef enum
{smt_Unknown,       //nieznany
 smt_Mesh,          //siatka
 smt_Point,
 smt_FreeSpotLight, //punkt �wietlny
 smt_Text,          //generator tekstu
 smt_Stars          //wiele punkt�w �wietlnych
} TSubModelType;
*/
//Ra: specjalne typy submodeli, poza tym GL_TRIANGLES itp.
const int TP_ROTATOR=256;
const int TP_FREESPOTLIGHT=257;
const int TP_STARS=258;
const int TP_TEXT=259;

enum TAnimType //rodzaj animacji
{at_None, //brak
 at_Rotate, //obr�t wzgl�dem wektora o k�t
 at_RotateXYZ, //obr�t wzgl�dem osi o k�ty
 at_Translate, //przesuni�cie
 at_SecondsJump, //sekundy z przeskokiem
 at_MinutesJump, //minuty z przeskokiem
 at_HoursJump, //godziny z przeskokiem 12h/360�
 at_Hours24Jump, //godziny z przeskokiem 24h/360�
 at_Seconds, //sekundy p�ynnie
 at_Minutes, //minuty p�ynnie
 at_Hours, //godziny p�ynnie 12h/360�
 at_Hours24, //godziny p�ynnie 24h/360�
 at_Billboard, //obr�t w pionie do kamery
 at_Wind, //ruch pod wp�ywem wiatru
 at_Sky, //animacja nieba
 at_IK=0x100, //odwrotna kinematyka - submodel steruj�cy (np. staw skokowy)
 at_IK11=0x101, //odwrotna kinematyka - submodel nadrz�dny do sterowango (np. stopa)
 at_IK21=0x102, //odwrotna kinematyka - submodel nadrz�dny do sterowango (np. podudzie)
 at_IK22=0x103, //odwrotna kinematyka - submodel nadrz�dny do nadrz�dnego sterowango (np. udo)
 at_Undefined=0x800000FF //animacja chwilowo nieokre�lona
};

class TModel3d;
class TSubModelInfo;

class TSubModel
{//klasa submodelu - pojedyncza siatka, punkt �wietlny albo grupa punkt�w
 //Ra: ta klasa ma mie� wielko�� 256 bajt�w, aby pokry�a si� z formatem binarnym
 //Ra: nie przestawia� zmiennych, bo wczytuj� si� z pliku binarnego!
private:
 TSubModel *Next;
 TSubModel *Child;
 int eType; //Ra: modele binarne daj� wi�cej mo�liwo�ci ni� mesh z�o�ony z tr�jk�t�w
 int iName; //numer �a�cucha z nazw� submodelu, albo -1 gdy anonimowy
public: //chwilowo
 TAnimType b_Anim;
private:
 int iFlags; //flagi informacyjne:
 //bit  0: =1 faza rysowania zale�y od wymiennej tekstury 0
 //bit  1: =1 faza rysowania zale�y od wymiennej tekstury 1
 //bit  2: =1 faza rysowania zale�y od wymiennej tekstury 2
 //bit  3: =1 faza rysowania zale�y od wymiennej tekstury 3
 //bit  4: =1 rysowany w fazie nieprzezroczystych (sta�a tekstura albo brak)
 //bit  5: =1 rysowany w fazie przezroczystych (sta�a tekstura)
 //bit  7: =1 ta sama tekstura, co poprzedni albo nadrz�dny
 //bit  8: =1 wierzcho�ki wy�wietlane z indeks�w
 //bit  9: =1 wczytano z pliku tekstowego (jest w�a�cicielem tablic)
 //bit 13: =1 wystarczy przesuni�cie zamiast mno�enia macierzy (trzy jedynki)
 //bit 14: =1 wymagane przechowanie macierzy (animacje)
 //bit 15: =1 wymagane przechowanie macierzy (transform niejedynkowy)
 union
 {//transform, nie ka�dy submodel musi mie�
  float4x4 *fMatrix; //pojedyncza precyzja wystarcza
  //matrix4x4 *dMatrix; //do testu macierz podw�jnej precyzji
  int iMatrix; //w pliku binarnym jest numer matrycy
 };
 int iNumVerts; //ilo�� wierzcho�k�w (1 dla FreeSpotLight)
 int iVboPtr; //pocz�tek na li�cie wierzcho�k�w albo indeks�w
 int iTexture; //numer nazwy tekstury, -1 wymienna, 0 brak
 float fVisible; //pr�g jasno�ci �wiat�a do za��czenia submodelu
 float fLight; //pr�g jasno�ci �wiat�a do zadzia�ania selfillum
 float f4Ambient[4];
 float f4Diffuse[4]; //float ze wzgl�du na glMaterialfv()
 float f4Specular[4];
 float f4Emision[4];
 float fWireSize; //nie u�ywane, ale wczytywane
 float fSquareMaxDist;
 float fSquareMinDist;
 //McZapkie-050702: parametry dla swiatla:
 float fNearAttenStart;
 float fNearAttenEnd;
 int bUseNearAtten;      //te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
 int iFarAttenDecay;      //ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
 float fFarDecayRadius;  //normalizacja j.w.
 float fCosFalloffAngle; //cosinus k�ta sto�ka pod kt�rym wida� �wiat�o
 float fCosHotspotAngle; //cosinus k�ta sto�ka pod kt�rym wida� aureol� i zwi�kszone nat�enie �wiat�a
 float fCosViewAngle;    //cos kata pod jakim sie teraz patrzy
 //Ra: dalej s� zmienne robocze, mo�na je przestawia� z zachowaniem rozmiaru klasy
 int TextureID; //numer tekstury, -1 wymienna, 0 brak
 int bWire; //nie u�ywane, ale wczytywane
 //short TexAlpha;  //Ra: nie u�ywane ju�
 GLuint uiDisplayList; //roboczy numer listy wy�wietlania
 float Opacity; //nie u�ywane, ale wczytywane
 //ABu: te same zmienne, ale zdublowane dla Render i RenderAlpha,
 //bo sie chrzanilo przemieszczanie obiektow.
 //Ra: ju� si� nie chrzani
 float f_Angle;
 float3 v_RotateAxis;
 float3 v_Angles;
public: //chwilowo
 float3 v_TransVector;
 float8 *Vertices; //roboczy wska�nik - wczytanie T3D do VBO
 int iAnimOwner; //roboczy numer egzemplarza, kt�ry ustawi� animacj�
 TAnimType b_aAnim; //kody animacji oddzielnie, bo zerowane
public:
 float4x4 *mAnimMatrix; //macierz do animacji kwaternionowych (nale�y do AnimContainer)
 char space[8]; //wolne miejsce na przysz�e zmienne (zmniejszy� w miar� potrzeby)
public:
 TSubModel **smLetter; //wska�nik na tablic� submdeli do generoania tekstu (docelowo zapisa� do E3D)
 TSubModel *Parent; //nadrz�dny, np. do wymna�ania macierzy
 int iVisible; //roboczy stan widoczno�ci
 //AnsiString asTexture; //robocza nazwa tekstury do zapisania w pliku binarnym
 //AnsiString asName; //robocza nazwa
 char *pTexture; //robocza nazwa tekstury do zapisania w pliku binarnym
 char *pName; //robocza nazwa
private:
 //int __fastcall SeekFaceNormal(DWORD *Masks, int f,DWORD dwMask,vector3 *pt,GLVERTEX *Vertices);
 int __fastcall SeekFaceNormal(DWORD *Masks,int f,DWORD dwMask,float3 *pt,float8 *Vertices);
 void __fastcall RaAnimation(TAnimType a);
public:
 static int iInstance; //identyfikator egzemplarza, kt�ry aktualnie renderuje model
 static GLuint *ReplacableSkinId;
 static int iAlpha; //maska bitowa dla danego przebiegu
 static double fSquareDist;
 static TModel3d* pRoot;
 static AnsiString* pasText; //tekst dla wy�wietlacza (!!!! do przemy�lenia)
 __fastcall TSubModel();
 __fastcall ~TSubModel();
 void __fastcall FirstInit();
 int __fastcall Load(cParser& Parser, TModel3d *Model,int Pos,bool dynamic);
 void __fastcall ChildAdd(TSubModel *SubModel);
 void __fastcall NextAdd(TSubModel *SubModel);
 TSubModel* __fastcall NextGet() {return Next;};
 TSubModel* __fastcall ChildGet() {return Child;};
 int __fastcall TriangleAdd(TModel3d *m,int tex,int tri);
 float8* __fastcall TrianglePtr(int tex,int pos,int *la,int *ld,int*ls);
 //float8* __fastcall TrianglePtr(const char *tex,int tri);
 //void __fastcall SetRotate(vector3 vNewRotateAxis,float fNewAngle);
 void __fastcall SetRotate(float3 vNewRotateAxis,float fNewAngle);
 void __fastcall SetRotateXYZ(vector3 vNewAngles);
 void __fastcall SetRotateXYZ(float3 vNewAngles);
 void __fastcall SetTranslate(vector3 vNewTransVector);
 void __fastcall SetTranslate(float3 vNewTransVector);
 void __fastcall SetRotateIK1(float3 vNewAngles);
 TSubModel* __fastcall GetFromName(AnsiString search,bool i=true);
 TSubModel* __fastcall GetFromName(char *search,bool i=true);
 void __fastcall RenderDL();
 void __fastcall RenderAlphaDL();
 void __fastcall RenderVBO();
 void __fastcall RenderAlphaVBO();
 //inline matrix4x4* __fastcall GetMatrix() {return dMatrix;};
 inline float4x4* __fastcall GetMatrix() {return fMatrix;};
 //matrix4x4* __fastcall GetTransform() {return Matrix;};
 inline void __fastcall Hide() {iVisible=0;};
 void __fastcall RaArrayFill(CVertNormTex *Vert);
 //void __fastcall Render();
 int __fastcall FlagsCheck();
 void __fastcall WillBeAnimated() {if (this) iFlags|=0x4000;};
 void __fastcall InitialRotate(bool doit);
 void __fastcall DisplayLists();
 void __fastcall Info();
 void __fastcall InfoSet(TSubModelInfo *info);
 void __fastcall BinInit(TSubModel *s,float4x4 *m,float8 *v,TStringPack *t,TStringPack *n=NULL,bool dynamic=false);
 void __fastcall ReplacableSet(GLuint *r,int a)
 {ReplacableSkinId=r; iAlpha=a;};
 void __fastcall TextureNameSet(const char *n);
 void __fastcall NameSet(const char *n);
 //Ra: funkcje do budowania terenu z E3D
 int __fastcall Flags() {return iFlags;};
 void __fastcall UnFlagNext() {iFlags&=0x00FFFFFF;};
 void __fastcall ColorsSet(int *a,int *d,int*s);
 inline float3 Translation1Get()
 {return fMatrix?*(fMatrix->TranslationGet())+v_TransVector:v_TransVector;}
 inline float3 Translation2Get()
 {return *(fMatrix->TranslationGet())+Child->Translation1Get();}
 void __fastcall ParentMatrix(float4x4 *m);
};

class TSubModelInfo
{//klasa z informacjami o submodelach, do tworzenia pliku binarnego
public:
 TSubModel *pSubModel; //wska�nik na submodel
 int iTransform; //numer transformu (-1 gdy brak)
 int iName; //numer nazwy
 int iTexture; //numer tekstury
 int iNameLen; //d�ugo�� nazwy
 int iTextureLen; //d�ugo�� tekstury
 int iNext,iChild; //numer nast�pnego i potomnego
 static int iTotalTransforms; //ilo�� transform�w
 static int iTotalNames; //ilo�� nazw
 static int iTotalTextures; //ilo�� tekstur
 static int iCurrent; //aktualny obiekt
 static TSubModelInfo* pTable; //tabele obiekt�w pomocniczych
 __fastcall TSubModelInfo()
 {pSubModel=NULL;
  iTransform=iName=iTexture=iNext=iChild=-1; //nie ma
  iNameLen=iTextureLen=0;
 }
 void __fastcall Reset()
 {pTable=this; //ustawienie wska�nika tabeli obiekt�w
  iTotalTransforms=iTotalNames=iTotalTextures=iCurrent=0; //zerowanie licznik�w
 }
 __fastcall ~TSubModelInfo() {};
};

class TModel3d : public CMesh
{
private:
 //TMaterial *Materials;
 //int MaterialsCount; //Ra: nie u�ywane
 //bool TractionPart; //Ra: nie u�ywane
 TSubModel *Root; //drzewo submodeli
 int iFlags; //Ra: czy submodele maj� przezroczyste tekstury
public: //Ra: tymczasowo
 int iNumVerts; //ilo�� wierzcho�k�w (gdy nie ma VBO, to m_nVertexCount=0)
private:
 TStringPack Textures; //nazwy tekstur
 TStringPack Names; //nazwy submodeli
 int *iModel; //zawarto�� pliku binarnego
 int iSubModelsCount; //Ra: u�ywane do tworzenia binarnych
 AnsiString asBinary; //nazwa pod kt�r� zapisa� model binarny
public:
 inline TSubModel* __fastcall GetSMRoot() {return(Root);};
 //double Radius; //Ra: nie u�ywane
 __fastcall TModel3d();
 __fastcall TModel3d(char *FileName);
 __fastcall ~TModel3d();
 TSubModel* __fastcall GetFromName(const char *sName);
 //TMaterial* __fastcall GetMaterialFromName(char *sName);
 TSubModel* __fastcall AddToNamed(const char *Name, TSubModel *SubModel);
 void __fastcall AddTo(TSubModel *tmp,TSubModel *SubModel);
 void __fastcall LoadFromTextFile(char *FileName,bool dynamic);
 void __fastcall LoadFromBinFile(char *FileName,bool dynamic);
 bool __fastcall LoadFromFile(char *FileName,bool dynamic);
 void __fastcall SaveToBinFile(char *FileName);
 void __fastcall BreakHierarhy();
 //renderowanie specjalne
 void __fastcall Render(double fSquareDistance,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RenderAlpha(double fSquareDistance,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RaRender(double fSquareDistance,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RaRenderAlpha(double fSquareDistance,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 //jeden k�t obrotu
 void __fastcall Render(vector3 pPosition,double fAngle=0,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RenderAlpha(vector3 pPosition,double fAngle=0,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RaRender(vector3 pPosition,double fAngle=0,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RaRenderAlpha(vector3 pPosition,double fAngle=0,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 //trzy k�ty obrotu
 void __fastcall Render(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RenderAlpha(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RaRender(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 void __fastcall RaRenderAlpha(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId=NULL,int iAlpha=0x30300030);
 //inline int __fastcall GetSubModelsCount() { return (SubModelsCount); };
 int __fastcall Flags() {return iFlags;};
 void __fastcall Init();
 char* __fastcall NameGet() {return Root?Root->pName:NULL;};
 int __fastcall TerrainCount();
 TSubModel* __fastcall TerrainSquare(int n);
 void __fastcall TerrainRenderVBO(int n);
};

//---------------------------------------------------------------------------
#endif
