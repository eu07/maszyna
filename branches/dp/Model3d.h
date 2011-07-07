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

typedef enum
{smt_Unknown,       //nieznany
 smt_Mesh,          //siatka
 smt_Point,
 smt_FreeSpotLight, //punkt œwietlny
 smt_Text,          //generator tekstu
 smt_Stars          //wiele punktów œwietlnych
} TSubModelType;

typedef enum //rodzaj animacji
{at_None, //brak
 at_Rotate, //obrót wzglêdem wektora o k¹t
 at_RotateXYZ, //obrót wzglêdem osi o k¹ty
 at_Translate, //przesuniêcie
 at_SecondsJump, //sekundy z przeskokiem
 at_MinutesJump, //minuty z przeskokiem
 at_HoursJump, //godziny z przeskokiem 12h/360°
 at_Hours24Jump, //godziny z przeskokiem 24h/360°
 at_Seconds, //sekundy p³ynnie
 at_Minutes, //minuty p³ynnie
 at_Hours, //godziny p³ynnie 12h/360°
 at_Hours24, //godziny p³ynnie 24h/360°
 at_Billboard, //obrót w pionie do kamery
 at_Wind, //ruch pod wp³ywem wiatru
 at_Sky, //animacja nieba
 at_Undefined //animacja chwilowo nieokreœlona
} TAnimType;

class TModel3d;

class TSubModel
{//klasa submodelu - pojedyncza siatka, punkt œwietlny albo grupa punktów
private:
 TSubModelType eType;
 GLuint TextureID;
 int iFlags; //flagi informacyjne
 //bit  0: =1 faza rysowania zale¿y od wymiennej tekstury
 //bit  1: =1 rysowany w fazie nieprzezroczystych
 //bit  2: =1 rysowany w fazie przezroczystych
 //bit  7: =1 ta sama tekstura, co poprzedni albo nadrzêdny
 //bit 13: =1 wystarczy przesuniêcie zamiast mno¿enia macierzy (trzy jedynki)
 //bit 14: =1 wymagane przechowanie macierzy (transform niejedynkowy lub animacje)
 bool TexAlpha;        //McZapkie-141202: zeby bylo wiadomo czy sortowac ze wzgledu na przezroczystosc
 float fLight; //próg jasnoœci œwiat³a do zadzia³ania selfillum
 float f4Ambient[4];
 float f4Diffuse[4];
 float f4Specular[4];
 GLuint uiDisplayList;
 double Transparency;
 bool bWire;
 double fWireSize;
 double fSquareMaxDist;
 double fSquareMinDist;
 //McZapkie-050702: parametry dla swiatla:
 double fNearAttenStart;
 double fNearAttenEnd;
 bool bUseNearAtten;      //te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
 int iFarAttenDecay;      //ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
 double fFarDecayRadius;  //normalizacja j.w.
 double fcosFalloffAngle; //cosinus kata stozka pod ktorym widac swiatlo
 double fcosHotspotAngle; //cosinus kata stozka pod ktorym widac aureole i zwiekszone natezenie swiatla
 double fCosViewAngle;    //cos kata pod jakim sie teraz patrzy
 int Index;
 matrix4x4 Matrix;
 //ABu: te same zmienne, ale zdublowane dla Render i RenderAlpha,
 //bo sie chrzanilo przemieszczanie obiektow.
 //Ra: ju¿ siê nie chrzani
 double f_Angle;
 vector3 v_RotateAxis;
 vector3 v_Angles;
 vector3 v_TransVector;
 TSubModel *Next;
 TSubModel *Child;
 //vector3 HitBoxPts[6];
 int __fastcall SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, vector3 *pt, GLVERTEX *Vertices);
 int iNumVerts; //potrzebne do VBO
 int iVboPtr;
 GLVERTEX *Vertices; //do VBO
 int iAnimOwner;
 void __fastcall RaAnimation(TAnimType a);
public:
 TAnimType b_Anim,b_aAnim; //kody animacji oddzielnie, bo zerowane
 bool Visible;
 std::string Name;
 static int iInstance;
 __fastcall TSubModel();
 __fastcall ~TSubModel();
 void __fastcall FirstInit();
 int __fastcall Load(cParser& Parser, TModel3d *Model,int Pos);
 void __fastcall AddChild(TSubModel *SubModel);
 void __fastcall AddNext(TSubModel *SubModel);
 void __fastcall SetRotate(vector3 vNewRotateAxis, double fNewAngle);
 void __fastcall SetRotateXYZ(vector3 vNewAngles);
 void __fastcall SetTranslate(vector3 vNewTransVector);
 TSubModel* __fastcall GetFromName(std::string search);
 void __fastcall Render(GLuint ReplacableSkinId,bool bAlpha);
 void __fastcall RenderAlpha(GLuint ReplacableSkinId,bool bAlpha);
 void __fastcall RaRender(GLuint ReplacableSkinId,bool bAlpha);
 void __fastcall RaRenderAlpha(GLuint ReplacableSkinId,bool bAlpha);
 inline matrix4x4* __fastcall GetMatrix() { return &Matrix; };
 matrix4x4* __fastcall GetTransform();
 inline void __fastcall Hide() { Visible= false; };
 void __fastcall RaArrayFill(CVertNormTex *Vert);
 void __fastcall Render();
 int __fastcall Flags();
 void __fastcall WillBeAnimated() {iFlags|=0x4000;};
 void __fastcall InitialRotate(bool doit);
};

class TModel3d : public CMesh
{
private:
 //TMaterial *Materials;
 //int MaterialsCount; //Ra: nie u¿ywane
 //bool TractionPart; //Ra: nie u¿ywane
 TSubModel *Root;
 int iFlags; //Ra: czy submodele maj¹ przezroczyste tekstury
public:
 inline TSubModel* __fastcall GetSMRoot() {return(Root);};
 //int SubModelsCount; //Ra: nie u¿ywane
 //double Radius; //Ra: nie u¿ywane
 __fastcall TModel3d();
 __fastcall TModel3d(char *FileName);
 __fastcall ~TModel3d();
 TSubModel* __fastcall GetFromName(const char *sName);
 //TMaterial* __fastcall GetMaterialFromName(char *sName);
 bool __fastcall AddTo(const char *Name, TSubModel *SubModel);
 void __fastcall LoadFromTextFile(char *FileName);
 bool __fastcall LoadFromFile(char *FileName);
 void __fastcall SaveToFile(char *FileName);
 void __fastcall BreakHierarhy();
 //renderowanie specjalne
 void __fastcall Render(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RenderAlpha(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRender(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRenderAlpha(double fSquareDistance,GLuint ReplacableSkinId=0,bool bAlpha=false);
 //jeden k¹t obrotu
 void __fastcall Render(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RenderAlpha(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRender(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRenderAlpha(vector3 pPosition,double fAngle=0,GLuint ReplacableSkinId=0,bool bAlpha=false);
 //trzy k¹ty obrotu
 void __fastcall Render(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RenderAlpha(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRender(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 void __fastcall RaRenderAlpha(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId=0,bool bAlpha=false);
 //inline int __fastcall GetSubModelsCount() { return (SubModelsCount); };
 int __fastcall Flags() {return iFlags;};
};

//typedef TModel3d *PModel3d;
//---------------------------------------------------------------------------
#endif
