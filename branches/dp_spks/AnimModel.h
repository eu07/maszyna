//---------------------------------------------------------------------------

#ifndef AnimModelH
#define AnimModelH

#include "Model3d.h"
//#include "parser.h"

const int iMaxNumLights=8;

//typy stanu �wiate�
typedef enum {
 ls_Off=0,   //zgaszone
 ls_On=1,    //zapalone
 ls_Blink=2, //migaj�ce
 ls_Dark=3   //Ra: zapalajce si� automatycznie, gdy zrobi si� ciemno
} TLightState;

class TAnimContainer
{//opakowanie submodelu, okre�laj�ce animacj� egzemplarza - obs�ugiwane jako lista
private:
 vector3 vRotateAngles;
 vector3 vDesiredAngles;
 double fRotateSpeed;
 vector3 vTranslation;
 vector3 vTranslateTo;
 double fTranslateSpeed;
 TSubModel *pSubModel;
 int iAnim; //animacja: 1-obr�t, 2-przesuw
public:
 TAnimContainer *pNext;
 __fastcall TAnimContainer();
 __fastcall ~TAnimContainer();
 bool __fastcall Init(TSubModel *pNewSubModel);
 //std::string inline __fastcall GetName() { return std::string(pSubModel?pSubModel->asName.c_str():""); };
 std::string inline __fastcall GetName() { return std::string(pSubModel?pSubModel->pName:""); };
 //void __fastcall SetRotateAnim(vector3 vNewRotateAxis, double fNewDesiredAngle, double fNewRotateSpeed, bool bResetAngle=false);
 void __fastcall SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed);
 void __fastcall SetTranslateAnim(vector3 vNewTranslate, double fNewSpeed);
 void __fastcall UpdateModel();
 bool __fastcall InMovement(); //czy w trakcie animacji?
 double _fastcall AngleGet() {return vRotateAngles.z;}; //jednak ostatnia, T3D ma inny uk�ad
 void __fastcall WillBeAnimated() {if (pSubModel) pSubModel->WillBeAnimated();};
};

class TAnimModel
{//opakowanie modelu, okre�laj�ce stan egzemplarza
private:
 TAnimContainer *pRoot; //pojemniki steruj�ce, tylko dla aniomowanych submodeli
 TModel3d *pModel;
 double fBlinkTimer;
 int iNumLights;
 TSubModel *LightsOn[iMaxNumLights]; //Ra: te wska�niki powinny by� w ramach TModel3d
 TSubModel *LightsOff[iMaxNumLights];
 vector3 vAngle; //bazowe obroty egzemplarza wzgl�dem osi
 int iTexAlpha; //�eby nie sprawdza� za ka�dym razem, dla 4 wymiennych tekstur
public:
 TLightState lsLights[iMaxNumLights];
 GLuint ReplacableSkinId[5]; //McZapkie-020802: zmienialne skory
 __fastcall TAnimModel();
 __fastcall ~TAnimModel();
 bool __fastcall Init(TModel3d *pNewModel);
 bool __fastcall Init(AnsiString asName,AnsiString asReplacableTexture);
 bool __fastcall Load(cParser *parser, bool ter=false);
 TAnimContainer* __fastcall AddContainer(char *pName);
 TAnimContainer* __fastcall GetContainer(char *pName);
 void __fastcall Render(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall RenderAlpha(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall RaRender(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall RaRenderAlpha(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall Render(vector3* vPosition);
 void __fastcall RenderAlpha(vector3* vPosition);
 void __fastcall RaRender(vector3* vPosition);
 void __fastcall RaRenderAlpha(vector3* vPosition);
 void __fastcall RaPrepare();
 int __fastcall Flags();
 void __fastcall RaAnglesSet(double a,double b,double c)
 {vAngle.x=a; vAngle.y=b; vAngle.z=c;};
 bool __fastcall TerrainLoaded();
 int __fastcall TerrainCount();
 TSubModel* __fastcall TerrainSquare(int n);
};

//---------------------------------------------------------------------------
#endif