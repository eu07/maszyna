/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef Model3dH
#define Model3dH

#include "opengl/glew.h"
#include "Parser.h"
#include "dumb3d.h"
#include "Float3d.h"
#include "VBO.h"
#include "Texture.h"

using namespace Math3D;

struct GLVERTEX
{
    vector3 Point;
    vector3 Normal;
    float tu, tv;
};

class TStringPack
{
    char *data;
    //+0 - 4 bajty: typ kromki
    //+4 - 4 bajty: długość łącznie z nagłówkiem
    //+8 - obszar łańcuchów znakowych, każdy zakończony zerem
    int *index;
    //+0 - 4 bajty: typ kromki
    //+4 - 4 bajty: długość łącznie z nagłówkiem
    //+8 - tabela indeksów
  public:
    char *String(int n);
    char *StringAt(int n)
    {
        return data + 9 + n;
    };
    TStringPack()
    {
        data = NULL;
        index = NULL;
    };
    void Init(char *d)
    {
        data = d;
    };
    void InitIndex(int *i)
    {
        index = i;
    };
};

class TMaterialColor
{
  public:
    TMaterialColor(){};
    TMaterialColor(char V)
    {
        r = g = b = V;
    };
    // TMaterialColor(double R, double G, double B)
    TMaterialColor(char R, char G, char B)
    {
        r = R;
        g = G;
        b = B;
    };

    char r, g, b;
};

/*
struct TMaterial
{
    int ID;
    AnsiString Name;
//McZapkie-240702: lepiej uzywac wartosci float do opisu koloru bo funkcje opengl chyba tego na ogol
uzywaja
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
    inline void Reset() { Planes[0]= TPlane(vector3(0,0,0),0.0f); };
    inline bool Inside(vector3 Point)
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

/* Ra: tego nie będziemy już używać, bo można wycisnąć więcej
typedef enum
{smt_Unknown,       //nieznany
 smt_Mesh,          //siatka
 smt_Point,
 smt_FreeSpotLight, //punkt świetlny
 smt_Text,          //generator tekstu
 smt_Stars          //wiele punktów świetlnych
} TSubModelType;
*/
// Ra: specjalne typy submodeli, poza tym GL_TRIANGLES itp.
const int TP_ROTATOR = 256;
const int TP_FREESPOTLIGHT = 257;
const int TP_STARS = 258;
const int TP_TEXT = 259;

enum TAnimType // rodzaj animacji
{
    at_None, // brak
    at_Rotate, // obrót względem wektora o kąt
    at_RotateXYZ, // obrót względem osi o kąty
    at_Translate, // przesunięcie
    at_SecondsJump, // sekundy z przeskokiem
    at_MinutesJump, // minuty z przeskokiem
    at_HoursJump, // godziny z przeskokiem 12h/360°
    at_Hours24Jump, // godziny z przeskokiem 24h/360°
    at_Seconds, // sekundy płynnie
    at_Minutes, // minuty płynnie
    at_Hours, // godziny płynnie 12h/360°
    at_Hours24, // godziny płynnie 24h/360°
    at_Billboard, // obrót w pionie do kamery
    at_Wind, // ruch pod wpływem wiatru
    at_Sky, // animacja nieba
    at_IK = 0x100, // odwrotna kinematyka - submodel sterujący (np. staw skokowy)
    at_IK11 = 0x101, // odwrotna kinematyka - submodel nadrzędny do sterowango (np. stopa)
    at_IK21 = 0x102, // odwrotna kinematyka - submodel nadrzędny do sterowango (np. podudzie)
    at_IK22 = 0x103, // odwrotna kinematyka - submodel nadrzędny do nadrzędnego sterowango (np. udo)
    at_Digital = 0x200, // dziesięciocyfrowy licznik mechaniczny (z cylindrami)
    at_DigiClk = 0x201, // zegar cyfrowy jako licznik na dziesięciościanach
    at_Undefined = 0x800000FF // animacja chwilowo nieokreślona
};

class TModel3d;
class TSubModelInfo;

class TSubModel
{ // klasa submodelu - pojedyncza siatka, punkt świetlny albo grupa punktów
    // Ra: ta klasa ma mieć wielkość 256 bajtów, aby pokryła się z formatem binarnym
    // Ra: nie przestawiać zmiennych, bo wczytują się z pliku binarnego!
  private:
    TSubModel *Next;
    TSubModel *Child;
    int eType; // Ra: modele binarne dają więcej możliwości niż mesh złożony z trójkątów
    int iName; // numer łańcucha z nazwą submodelu, albo -1 gdy anonimowy
  public: // chwilowo
    TAnimType b_Anim;

  private:
    int iFlags; // flagi informacyjne:
    // bit  0: =1 faza rysowania zależy od wymiennej tekstury 0
    // bit  1: =1 faza rysowania zależy od wymiennej tekstury 1
    // bit  2: =1 faza rysowania zależy od wymiennej tekstury 2
    // bit  3: =1 faza rysowania zależy od wymiennej tekstury 3
    // bit  4: =1 rysowany w fazie nieprzezroczystych (stała tekstura albo brak)
    // bit  5: =1 rysowany w fazie przezroczystych (stała tekstura)
    // bit  7: =1 ta sama tekstura, co poprzedni albo nadrzędny
    // bit  8: =1 wierzchołki wyświetlane z indeksów
    // bit  9: =1 wczytano z pliku tekstowego (jest właścicielem tablic)
    // bit 13: =1 wystarczy przesunięcie zamiast mnożenia macierzy (trzy jedynki)
    // bit 14: =1 wymagane przechowanie macierzy (animacje)
    // bit 15: =1 wymagane przechowanie macierzy (transform niejedynkowy)
    union
    { // transform, nie każdy submodel musi mieć
        float4x4 *fMatrix; // pojedyncza precyzja wystarcza
        // matrix4x4 *dMatrix; //do testu macierz podwójnej precyzji
        int iMatrix; // w pliku binarnym jest numer matrycy
    };
    int iNumVerts; // ilość wierzchołków (1 dla FreeSpotLight)
    int iVboPtr; // początek na liście wierzchołków albo indeksów
    int iTexture; // numer nazwy tekstury, -1 wymienna, 0 brak
    float fVisible; // próg jasności światła do załączenia submodelu
    float fLight; // próg jasności światła do zadziałania selfillum
    float f4Ambient[4];
    float f4Diffuse[4]; // float ze względu na glMaterialfv()
    float f4Specular[4];
    float f4Emision[4];
    float fWireSize; // nie używane, ale wczytywane
    float fSquareMaxDist;
    float fSquareMinDist;
    // McZapkie-050702: parametry dla swiatla:
    float fNearAttenStart;
    float fNearAttenEnd;
    bool bUseNearAtten; // te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
    int iFarAttenDecay; // ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
    float fFarDecayRadius; // normalizacja j.w.
    float fCosFalloffAngle; // cosinus kąta stożka pod którym widać światło
    float fCosHotspotAngle; // cosinus kąta stożka pod którym widać aureolę i zwiększone natężenie
    // światła
    float fCosViewAngle; // cos kata pod jakim sie teraz patrzy
    // Ra: dalej są zmienne robocze, można je przestawiać z zachowaniem rozmiaru klasy
    texture_manager::size_type TextureID; // numer tekstury, -1 wymienna, 0 brak
    bool bWire; // nie używane, ale wczytywane
    // short TexAlpha;  //Ra: nie używane już
    GLuint uiDisplayList; // roboczy numer listy wyświetlania
    float Opacity; // nie używane, ale wczytywane
    // ABu: te same zmienne, ale zdublowane dla Render i RenderAlpha,
    // bo sie chrzanilo przemieszczanie obiektow.
    // Ra: już się nie chrzani
    float f_Angle;
    float3 v_RotateAxis;
    float3 v_Angles;

  public: // chwilowo
    float3 v_TransVector;
    float8 *Vertices; // roboczy wskaźnik - wczytanie T3D do VBO
    int iAnimOwner; // roboczy numer egzemplarza, który ustawił animację
    TAnimType b_aAnim; // kody animacji oddzielnie, bo zerowane
  public:
    float4x4 *mAnimMatrix; // macierz do animacji kwaternionowych (należy do AnimContainer)
    char space[8]; // wolne miejsce na przyszłe zmienne (zmniejszyć w miarę potrzeby)
  public:
    TSubModel **
        smLetter; // wskaźnik na tablicę submdeli do generoania tekstu (docelowo zapisać do E3D)
    TSubModel *Parent; // nadrzędny, np. do wymnażania macierzy
    int iVisible; // roboczy stan widoczności
    // AnsiString asTexture; //robocza nazwa tekstury do zapisania w pliku binarnym
    // AnsiString asName; //robocza nazwa
    char *pTexture; // robocza nazwa tekstury do zapisania w pliku binarnym
    char *pName; // robocza nazwa
  private:
    // int SeekFaceNormal(DWORD *Masks, int f,DWORD dwMask,vector3 *pt,GLVERTEX
    // *Vertices);
    int SeekFaceNormal(unsigned int *Masks, int f, unsigned int dwMask, float3 *pt, float8 *Vertices);
    void RaAnimation(TAnimType a);

  public:
    static int iInstance; // identyfikator egzemplarza, który aktualnie renderuje model
    static texture_manager::size_type *ReplacableSkinId;
    static int iAlpha; // maska bitowa dla danego przebiegu
    static double fSquareDist;
    static TModel3d *pRoot;
    static std::string *pasText; // tekst dla wyświetlacza (!!!! do przemyślenia)
    TSubModel();
    ~TSubModel();
    void FirstInit();
    int Load(cParser &Parser, TModel3d *Model, int Pos, bool dynamic);
    void ChildAdd(TSubModel *SubModel);
    void NextAdd(TSubModel *SubModel);
    TSubModel * NextGet()
    {
        return Next;
    };
    TSubModel * ChildGet()
    {
        return Child;
    };
    int TriangleAdd(TModel3d *m, texture_manager::size_type tex, int tri);
    float8 * TrianglePtr(int tex, int pos, int *la, int *ld, int *ls);
    // float8* TrianglePtr(const char *tex,int tri);
    // void SetRotate(vector3 vNewRotateAxis,float fNewAngle);
    void SetRotate(float3 vNewRotateAxis, float fNewAngle);
    void SetRotateXYZ(vector3 vNewAngles);
    void SetRotateXYZ(float3 vNewAngles);
    void SetTranslate(vector3 vNewTransVector);
    void SetTranslate(float3 vNewTransVector);
    void SetRotateIK1(float3 vNewAngles);
    TSubModel * GetFromName(std::string const &search, bool i = true);
    TSubModel * GetFromName(char const *search, bool i = true);
    void RenderDL();
    void RenderAlphaDL();
    void RenderVBO();
    void RenderAlphaVBO();
    // inline matrix4x4* GetMatrix() {return dMatrix;};
    inline float4x4 * GetMatrix()
    {
        return fMatrix;
    };
    // matrix4x4* GetTransform() {return Matrix;};
    inline void Hide()
    {
        iVisible = 0;
    };
    void RaArrayFill(CVertNormTex *Vert);
    // void Render();
    int FlagsCheck();
    void WillBeAnimated()
    {
        if (this)
            iFlags |= 0x4000;
    };
    void InitialRotate(bool doit);
    void DisplayLists();
    void Info();
    void InfoSet(TSubModelInfo *info);
    void BinInit(TSubModel *s, float4x4 *m, float8 *v, TStringPack *t, TStringPack *n = NULL,
                 bool dynamic = false);
    void ReplacableSet(texture_manager::size_type *r, int a)
    {
        ReplacableSkinId = r;
        iAlpha = a;
    };
    void TextureNameSet(const char *n);
    void NameSet(const char *n);
    // Ra: funkcje do budowania terenu z E3D
    int Flags()
    {
        return iFlags;
    };
    void UnFlagNext()
    {
        iFlags &= 0x00FFFFFF;
    };
    void ColorsSet(int *a, int *d, int *s);
    inline float3 Translation1Get()
    {
        return fMatrix ? *(fMatrix->TranslationGet()) + v_TransVector : v_TransVector;
    }
    inline float3 Translation2Get()
    {
        return *(fMatrix->TranslationGet()) + Child->Translation1Get();
    }
    int GetTextureId()
    {
        return TextureID;
    }
    void ParentMatrix(float4x4 *m);
    float MaxY(const float4x4 &m);
    void AdjustDist();
};

class TSubModelInfo
{ // klasa z informacjami o submodelach, do tworzenia pliku binarnego
  public:
    TSubModel *pSubModel; // wskaźnik na submodel
    int iTransform; // numer transformu (-1 gdy brak)
    int iName; // numer nazwy
    int iTexture; // numer tekstury
    int iNameLen; // długość nazwy
    int iTextureLen; // długość tekstury
    int iNext, iChild; // numer następnego i potomnego
    static int iTotalTransforms; // ilość transformów
    static int iTotalNames; // ilość nazw
    static int iTotalTextures; // ilość tekstur
    static int iCurrent; // aktualny obiekt
    static TSubModelInfo *pTable; // tabele obiektów pomocniczych
    TSubModelInfo()
    {
        pSubModel = NULL;
        iTransform = iName = iTexture = iNext = iChild = -1; // nie ma
        iNameLen = iTextureLen = 0;
    }
    void Reset()
    {
        pTable = this; // ustawienie wskaźnika tabeli obiektów
        iTotalTransforms = iTotalNames = iTotalTextures = iCurrent = 0; // zerowanie liczników
    }
    ~TSubModelInfo(){};
};

class TModel3d : public CMesh
{
  private:
    // TMaterial *Materials;
    // int MaterialsCount; //Ra: nie używane
    // bool TractionPart; //Ra: nie używane
    TSubModel *Root; // drzewo submodeli
    int iFlags; // Ra: czy submodele mają przezroczyste tekstury
  public: // Ra: tymczasowo
    int iNumVerts; // ilość wierzchołków (gdy nie ma VBO, to m_nVertexCount=0)
  private:
    TStringPack Textures; // nazwy tekstur
    TStringPack Names; // nazwy submodeli
    int *iModel; // zawartość pliku binarnego
    int iSubModelsCount; // Ra: używane do tworzenia binarnych
    std::string asBinary; // nazwa pod którą zapisać model binarny
  public:
    inline TSubModel * GetSMRoot()
    {
        return (Root);
    };
    // double Radius; //Ra: nie używane
    TModel3d();
    TModel3d(char *FileName);
    ~TModel3d();
    TSubModel * GetFromName(const char *sName);
    // TMaterial* GetMaterialFromName(char *sName);
    TSubModel * AddToNamed(const char *Name, TSubModel *SubModel);
    void AddTo(TSubModel *tmp, TSubModel *SubModel);
    void LoadFromTextFile(std::string const &FileName, bool dynamic);
    void LoadFromBinFile(std::string const &FileName, bool dynamic);
    bool LoadFromFile(std::string const &FileName, bool dynamic);
    void SaveToBinFile(char const *FileName);
    void BreakHierarhy();
    // renderowanie specjalne
    void Render(double fSquareDistance, texture_manager::size_type *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
    void RenderAlpha(double fSquareDistance, texture_manager::size_type *ReplacableSkinId = NULL,
                     int iAlpha = 0x30300030);
    void RaRender(double fSquareDistance, texture_manager::size_type *ReplacableSkinId = NULL, int iAlpha = 0x30300030);
    void RaRenderAlpha(double fSquareDistance, texture_manager::size_type *ReplacableSkinId = NULL,
                       int iAlpha = 0x30300030);
    // jeden kąt obrotu
    void Render(vector3 pPosition, double fAngle = 0, texture_manager::size_type *ReplacableSkinId = NULL,
                int iAlpha = 0x30300030);
    void RenderAlpha(vector3 pPosition, double fAngle = 0, texture_manager::size_type *ReplacableSkinId = NULL,
                     int iAlpha = 0x30300030);
    void RaRender(vector3 pPosition, double fAngle = 0, texture_manager::size_type *ReplacableSkinId = NULL,
                  int iAlpha = 0x30300030);
    void RaRenderAlpha(vector3 pPosition, double fAngle = 0, texture_manager::size_type *ReplacableSkinId = NULL,
                       int iAlpha = 0x30300030);
    // trzy kąty obrotu
    void Render( vector3 *vPosition, vector3 *vAngle, texture_manager::size_type *ReplacableSkinId = NULL,
                int iAlpha = 0x30300030);
    void RenderAlpha( vector3 *vPosition, vector3 *vAngle, texture_manager::size_type *ReplacableSkinId = NULL,
                     int iAlpha = 0x30300030);
    void RaRender( vector3 *vPosition, vector3 *vAngle, texture_manager::size_type *ReplacableSkinId = NULL,
                  int iAlpha = 0x30300030);
    void RaRenderAlpha( vector3 *vPosition, vector3 *vAngle, texture_manager::size_type *ReplacableSkinId = NULL,
                       int iAlpha = 0x30300030);
    // inline int GetSubModelsCount() { return (SubModelsCount); };
    int Flags()
    {
        return iFlags;
    };
    void Init();
    char * NameGet()
    {
        return Root ? Root->pName : NULL;
    };
    int TerrainCount();
    TSubModel * TerrainSquare(int n);
    void TerrainRenderVBO(int n);
};

//---------------------------------------------------------------------------
#endif
