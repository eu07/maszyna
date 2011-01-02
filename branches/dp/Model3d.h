//---------------------------------------------------------------------------
#ifndef Model3dH
#define Model3dH

#include "opengl/glew.h"
#include "geometry.h"
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
//    __fastcall TMaterialColor(char R, char G, char B)
  //  {
    //    r=R; g=G; b=B;
//    };
//    __fastcall TMaterialColor(double R, double G, double B)
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

typedef enum { smt_Unknown, smt_Mesh, smt_Point, smt_FreeSpotLight, smt_Text} TSubModelType;

typedef enum { at_None, at_Rotate, at_RotateXYZ, at_Translate } TAnimType;

class TModel3d;

class TSubModel
{
private:
      TSubModelType eType;
      GLuint TextureID;
      bool TexAlpha;        //McZapkie-141202: zeby bylo wiadomo czy sortowac ze wzgledu na przezroczystosc
      bool bLight; //selfillum
      float f4Ambient[4];
      float f4Diffuse[4];
      float f4Specular[4];

//      bool TexHash;
      //GLuint uiDisplayList;
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

      double f_Angle, f_aAngle;
      vector3 v_RotateAxis, v_aRotateAxis;
      vector3 v_Angles, v_aAngles;
      double f_DesiredAngle, f_aDesiredAngle;
      double f_RotateSpeed, f_aRotateSpeed;
      vector3 v_TransVector, v_aTransVector;
      vector3 v_DesiredTransVector, v_aDesiredTransVector;
      double f_TranslateSpeed, f_aTranslateSpeed;


      TSubModel *Next;
      TSubModel *Child;
  //    vector3 HitBoxPts[6];
      int __fastcall SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, vector3 pt, GLVERTEX *Vertices);

      int iNumVerts; //potrzebne do VBO
      int iVboPtr;
      GLVERTEX *Vertices; //do VBO
public:

      TAnimType b_Anim, b_aAnim;

      bool Visible;
      std::string Name;

      __fastcall TSubModel();
      __fastcall FirstInit();
      __fastcall ~TSubModel();
      int __fastcall Load(cParser& Parser, int NIndex, TModel3d *Model,int Pos);
      void __fastcall AddChild(TSubModel *SubModel);
      void __fastcall AddNext(TSubModel *SubModel);
      void __fastcall SetRotate(vector3 vNewRotateAxis, double fNewAngle);
      void __fastcall SetRotateXYZ(vector3 vNewAngles);
      void __fastcall SetTranslate(vector3 vNewTransVector);
      TSubModel* __fastcall GetFromName(std::string search);
      void __fastcall Render(GLuint ReplacableSkinId);
      void __fastcall RenderAlpha(GLuint ReplacableSkinId);      
      inline matrix4x4* __fastcall GetMatrix() { return &Matrix; };
      matrix4x4* __fastcall GetTransform();
      inline void __fastcall Hide() { Visible= false; };
      void  __fastcall RaArraysFill(CVert *Vert,CVec *Norm,CTexCoord *Tex);
} ;

class TModel3d : public CMesh
{
private:
//    TMaterial *Materials;
    int MaterialsCount;
    bool TractionPart;
    TSubModel *Root;
public:
    inline TSubModel* __fastcall GetSMRoot() {return(Root);};
    int SubModelsCount;
    double Radius;
    __fastcall TModel3d();
    __fastcall TModel3d(char *FileName);
    __fastcall ~TModel3d();
    TSubModel* __fastcall GetFromName(const char *sName);
//    TMaterial* __fastcall GetMaterialFromName(char *sName);
    bool __fastcall AddTo(const char *Name, TSubModel *SubModel);
    void __fastcall LoadFromTextFile(char *FileName);
    bool __fastcall LoadFromFile(char *FileName);
    void __fastcall SaveToFile(char *FileName);
    void __fastcall BreakHierarhy();
    void __fastcall Render( vector3 pPosition, double fAngle= 0, GLuint ReplacableSkinId= 0);
    void __fastcall Render( double fSquareDistance, GLuint ReplacableSkinId= 0);
    void __fastcall RenderAlpha( vector3 pPosition, double fAngle= 0, GLuint ReplacableSkinId= 0);
    void __fastcall RenderAlpha( double fSquareDistance, GLuint ReplacableSkinId= 0);
    inline int __fastcall GetSubModelsCount() { return (SubModelsCount); };
};

typedef TModel3d *PModel3d;
//---------------------------------------------------------------------------
#endif
