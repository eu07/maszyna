//---------------------------------------------------------------------------

#ifndef AnimModelH
#define AnimModelH

#include "Model3d.h"
#include "parser.h"

const int iMaxNumLights= 8;

typedef enum { ls_Off=0, ls_On=1, ls_Blink=2, ls_Dark=3} TLightState;

class TAnimContainer
{
private:
    vector3 vRotateAngles;
    vector3 vDesiredAngles;
    double fRotateSpeed;
    TSubModel *pSubModel;
public:
    __fastcall TAnimContainer();
    __fastcall ~TAnimContainer();
    bool __fastcall Init(TSubModel *pNewSubModel);
    std::string inline __fastcall GetName() { return (pSubModel?pSubModel->Name : std::string("")); };
//    void __fastcall SetRotateAnim(vector3 vNewRotateAxis, double fNewDesiredAngle, double fNewRotateSpeed, bool bResetAngle=false);
    void __fastcall SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed, bool bResetAngle=false);
    void __fastcall UpdateModel();
    TAnimContainer *pNext;
    bool __fastcall InMovement(); //czy w trakcie animacji?
    double _fastcall AngleGet() {return vRotateAngles.y;}; //jednak œrodkowa
};

class TAnimModel
{
public:
    __fastcall TAnimModel();
    __fastcall ~TAnimModel();
    bool __fastcall Init(TModel3d *pNewModel);
    bool __fastcall Init(AnsiString asName, AnsiString asReplacableTexture);
    bool __fastcall Load(cParser *parser);
    TAnimContainer* __fastcall AddContainer(char *pName);
    TAnimContainer* __fastcall GetContainer(char *pName);
    bool __fastcall Render(vector3 pPosition= vector3(0,0,0), double fAngle= 0);
    bool __fastcall Render(double fSquareDistance);
    bool __fastcall RenderAlpha(vector3 pPosition= vector3(0,0,0), double fAngle= 0);
    bool __fastcall RenderAlpha(double fSquareDistance);
    TLightState lsLights[iMaxNumLights];
    GLuint ReplacableSkinId; //McZapkie-020802: zmienialne skory
private:
    TSubModel *LightsOn[iMaxNumLights];
    TSubModel *LightsOff[iMaxNumLights];
    int iNumLights;
    double fBlinkTimer;
    TModel3d *pModel;
    TAnimContainer *pRoot;
};

//---------------------------------------------------------------------------
#endif
