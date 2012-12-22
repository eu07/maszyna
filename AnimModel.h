//---------------------------------------------------------------------------

#ifndef AnimModelH
#define AnimModelH

#include "Model3d.h"
#include "parser.h"

const int iMaxNumLights=8;

//typy stanu œwiate³
typedef enum {
 ls_Off=0,   //zgaszone
 ls_On=1,    //zapalone
 ls_Blink=2, //migaj¹ce
 ls_Dark=3   //Ra: zapalajce siê automatycznie, gdy zrobi siê ciemno
} TLightState;

class TAnimContainer
{//opakowanie submodelu, okreœlaj¹ce animacjê egzemplarza - obs³ugiwane jako lista
private:
 vector3 vRotateAngles;
 vector3 vDesiredAngles;
 double fRotateSpeed;
 vector3 vTranslation;
 vector3 vTranslateTo;
 double fTranslateSpeed;
 TSubModel *pSubModel;
 int iAnim; //animacja: 1-obrót, 2-przesuw
public:
 TAnimContainer *pNext;
 __fastcall TAnimContainer();
 __fastcall ~TAnimContainer();
 bool __fastcall Init(TSubModel *pNewSubModel);
 std::string inline __fastcall GetName() { return (pSubModel?pSubModel->Name : std::string("")); };
 //void __fastcall SetRotateAnim(vector3 vNewRotateAxis, double fNewDesiredAngle, double fNewRotateSpeed, bool bResetAngle=false);
 void __fastcall SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed);
 void __fastcall SetTranslateAnim(vector3 vNewTranslate, double fNewSpeed);
 void __fastcall UpdateModel();
 bool __fastcall InMovement(); //czy w trakcie animacji?
 double _fastcall AngleGet() {return vRotateAngles.z;}; //jednak ostatnia, T3D ma inny uk³ad
};

class TAnimModel
{//opakowanie modelu, okreœlaj¹ce stan egzemplarza
private:
 TAnimContainer *pRoot;
 TModel3d *pModel;
 double fBlinkTimer;
 int iNumLights;
 TSubModel *LightsOn[iMaxNumLights];
 TSubModel *LightsOff[iMaxNumLights];
public:
 TLightState lsLights[iMaxNumLights];
 GLuint ReplacableSkinId; //McZapkie-020802: zmienialne skory
 __fastcall TAnimModel();
 __fastcall ~TAnimModel();
 bool __fastcall Init(TModel3d *pNewModel);
 bool __fastcall Init(AnsiString asName,AnsiString asReplacableTexture);
 bool __fastcall Load(cParser *parser);
 TAnimContainer* __fastcall AddContainer(char *pName);
 TAnimContainer* __fastcall GetContainer(char *pName);
 void __fastcall Render(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall RenderAlpha(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall RaRender(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 void __fastcall RaRenderAlpha(vector3 pPosition=vector3(0,0,0),double fAngle=0);
 //void __fastcall Render(double fSquareDistance);
 //void __fastcall RenderAlpha(double fSquareDistance);
 void __fastcall RaPrepare();
 bool bTexAlpha; //¿eby nie sprawdzaæ za ka¿dym razem
 int __fastcall Flags();   
};

//---------------------------------------------------------------------------
#endif
