//---------------------------------------------------------------------------

#ifndef TractionH
#define TractionH

#include <gl/gl.h>
#include <gl/glu.h>
#include "opengl/glut.h"

#include "Geometry.h"
#include "AnimModel.h"


class TTraction //: public TNode
{
private:
    vector3 vUp,vFront,vLeft;
    matrix4x4 mMatrix;
public:
    GLuint uiDisplayList;
    vector3 pPoint1,pPoint2,pPoint3,pPoint4;
    double fHeightDifference;//,fMiddleHeight;
  //  int iCategory,iMaterial,iDamageFlag;
//    float fU,fR,fMaxI,fWireThickness;
    int iNumSections;
    float NominalVoltage;
    float MaxCurrent;
    float Resistivity;
    DWORD Material;   //1: Cu, 2: Al
    float WireThickness;
    DWORD DamageFlag; //1: zasniedziale, 128: zerwana
    int Wires;
    float WireOffset;
    AnsiString asPowerSupplyName; //McZapkie: nazwa podstacji trakcyjnej
//    TModel3d *mdPole;
//    GLuint ReplacableSkinID;  //McZapkie:zmienialna tekstura slupa
//    int PoleSide;             //przy automatycznym rysowaniu slupow: lewy/prawy
//    bool bVisible;
//    DWORD dwFlags;

    void __fastcall TTraction::Optimize();

    TTraction();
    virtual ~TTraction();

//    virtual void __fastcall InitCenter(vector3 Angles, vector3 pOrigin);
//    virtual bool __fastcall Hit(double x, double z, vector3 &hitPoint, vector3 &hitDirection) { return TNode::Hit(x,z,hitPoint,hitDirection); };
  //  virtual bool __fastcall Move(double dx, double dy, double dz) { return false; };
//    virtual void __fastcall SelectedRender();
    virtual void __fastcall Render(float mgn);

};
//---------------------------------------------------------------------------
#endif