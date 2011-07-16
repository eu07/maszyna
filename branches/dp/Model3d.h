//---------------------------------------------------------------------------
#ifndef Model3dH
#define Model3dH

#include "opengl/glew.h"
#include "Parser.h"
#include "dumb3d.h"
#include "VBO.h"
using namespace Math3D;

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
 char* String(int n);
 char* StringAt(int n) {return data+9+n;};
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
const int TP_UNKNOWN=2000;
const int TP_FREESPOTLIGHT=2001;
const int TP_STARS=2002;
const int TP_TEXT=2003;

typedef enum //rodzaj animacji
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
 at_Undefined //animacja chwilowo nieokre�lona
} TAnimType;

class TModel3d;

class TSubModel
{//klasa submodelu - pojedyncza siatka, punkt �wietlny albo grupa punkt�w
 //Ra: ta klasa ma mie� wielko�� 320 bajt�w, aby pokry�a si� z formatem binarnym
private:
 //TSubModelType eType;
 int eType; //Ra: modele binarne daj� wi�cej mo�liwo�ci ni� mesh z�o�ony z tr�jk�t�w
 int iFlags; //flagi informacyjne
 //bit  0: =1 faza rysowania zale�y od wymiennej tekstury
 //bit  1: =1 rysowany w fazie nieprzezroczystych
 //bit  2: =1 rysowany w fazie przezroczystych
 //bit  7: =1 ta sama tekstura, co poprzedni albo nadrz�dny
 //bit 13: =1 wystarczy przesuni�cie zamiast mno�enia macierzy (trzy jedynki)
 //bit 14: =1 wymagane przechowanie macierzy (animacje)
 //bit 15: =1 wymagane przechowanie macierzy (transform niejedynkowy)
 GLuint TextureID; //numer tekstury, -1 wymienna, 0 brak
 bool TexAlpha;        //McZapkie-141202: zeby bylo wiadomo czy sortowac ze wzgledu na przezroczystosc
 float fLight; //pr�g jasno�ci �wiat�a do zadzia�ania selfillum
 float f4Ambient[4];
 float f4Diffuse[4]; //float ze wzgl�du na glMaterialfv()
 float f4Specular[4];
 GLuint uiDisplayList;
 double Transparency; //nie u�ywane, ale wczytywane
 bool bWire; //nie u�ywane, ale wczytywane
 double fWireSize; //nie u�ywane, ale wczytywane
 double fSquareMaxDist;
 double fSquareMinDist;
 //McZapkie-050702: parametry dla swiatla:
 double fNearAttenStart;
 double fNearAttenEnd;
 bool bUseNearAtten;      //te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
 int iFarAttenDecay;      //ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
 double fFarDecayRadius;  //normalizacja j.w.
 double fcosFalloffAngle; //cosinus k�ta sto�ka pod kt�rym wida� �wiat�o
 double fcosHotspotAngle; //cosinus k�ta sto�ka pod kt�rym wida� aureol� i zwi�kszone nat�enie �wiat�a
 double fCosViewAngle;    //cos kata pod jakim sie teraz patrzy
 //int Index;
 union
 {matrix4x4 *dMatrix; //transform, nie ka�dy submodel musi mie�
  float *fMatrix;
  int iMatrix; //w pliku binarnym jest numer matrycy
 };
 //ABu: te same zmienne, ale zdublowane dla Render i RenderAlpha,
 //bo sie chrzanilo przemieszczanie obiektow.
 //Ra: ju� si� nie chrzani
 float f_Angle;
 vector3 v_RotateAxis;
 vector3 v_Angles;
 vector3 v_TransVector;
 TSubModel *Next;
 TSubModel *Child;
 //vector3 HitBoxPts[6];
 int iNumVerts; //potrzebne do VBO
 int iVboPtr;
 GLVERTEX *Vertices; //do VBO
 int iAnimOwner;
 TAnimType b_Anim,b_aAnim; //kody animacji oddzielnie, bo zerowane
 char space[32];
public:
 bool Visible;
 std::string Name;
private:
 int __fastcall SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, vector3 *pt, GLVERTEX *Vertices);
 void __fastcall RaAnimation(TAnimType a);
public:
 static int iInstance;
 __fastcall TSubModel();
 __fastcall ~TSubModel();
 void __fastcall FirstInit();
 int __fastcall Load(cParser& Parser, TModel3d *Model,int Pos);
 void __fastcall AddChild(TSubModel *SubModel);
 void __fastcall AddNext(TSubModel *SubModel);
 void __fastcall SetRotate(vector3 vNewRotateAxis,float fNewAngle);
 void __fastcall SetRotateXYZ(vector3 vNewAngles);
 void __fastcall SetTranslate(vector3 vNewTransVector);
 TSubModel* __fastcall GetFromName(std::string search);
 void __fastcall Render(GLuint ReplacableSkinId,bool bAlpha);
 void __fastcall RenderAlpha(GLuint ReplacableSkinId,bool bAlpha);
 void __fastcall RaRender(GLuint ReplacableSkinId,bool bAlpha);
 void __fastcall RaRenderAlpha(GLuint ReplacableSkinId,bool bAlpha);
 inline matrix4x4* __fastcall GetMatrix() { return dMatrix; };
 //matrix4x4* __fastcall GetTransform() {return Matrix;};
 inline void __fastcall Hide() { Visible= false; };
 void __fastcall RaArrayFill(CVertNormTex *Vert);
 void __fastcall Render();
 int __fastcall Flags();
 void __fastcall WillBeAnimated() {if (this) iFlags|=0x4000;};
 void __fastcall InitialRotate(bool doit);
};

class TModel3d : public CMesh
{
private:
 //TMaterial *Materials;
 //int MaterialsCount; //Ra: nie u�ywane
 //bool TractionPart; //Ra: nie u�ywane
 TSubModel *Root; //drzewo submodeli
 int iFlags; //Ra: czy submodele maj� przezroczyste tekstury
 int iNumVerts; //ilo�� wierzcho�k�w (gdy nie ma VBO, to m_nVertexCount=0)
 TStringPack *Textures; //nazwy tekstur
 TStringPack *Names; //nazwy submodeli
public:
 inline TSubModel* __fastcall GetSMRoot() {return(Root);};
 //int SubModelsCount; //Ra: nie u�ywane
 //double Radius; //Ra: nie u�ywane
 __fastcall TModel3d();
 __fastcall TModel3d(char *FileName);
 __fastcall ~TModel3d();
 TSubModel* __fastcall GetFromName(const char *sName);
 //TMaterial* __fastcall GetMaterialFromName(char *sName);
 bool __fastcall AddTo(const char *Name, TSubModel *SubModel);
 void __fastcall LoadFromTextFile(char *FileName,bool dynamic);
 void __fastcall LoadFromBinFile(char *FileName);
 void __fastcall LoadFromFile(char *FileName,bool dynamic);
 void __fastcall SaveToBinFile(char *FileName);
 void __fastcall BreakHierarhy();
 //renderowanie specjalne
 void __fastcall Render(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RenderAlpha(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRender(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRenderAlpha(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 //jeden k�t obrotu
 void __fastcall Render(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RenderAlpha(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRender(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRenderAlpha(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 //trzy k�ty obrotu
 void __fastcall Render(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RenderAlpha(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRender(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRenderAlpha(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 //inline int __fastcall GetSubModelsCount() { return (SubModelsCount); };
 int __fastcall Flags() {return iFlags;};
 void __fastcall Init();
};

//typedef TModel3d *PModel3d;
//---------------------------------------------------------------------------
#endif
