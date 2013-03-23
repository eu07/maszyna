//---------------------------------------------------------------------------

#ifndef TractionH
#define TractionH

#include "opengl/glew.h"
#include "dumb3d.h"
#include "VBO.h"

using namespace Math3D;

class TTraction 
{
private:
 vector3 vUp,vFront,vLeft;
 matrix4x4 mMatrix;
 //matryca do wyliczania pozycji drutu w zale¿noœci od [X,Y,k¹t] w scenerii:
 // - x: odleg³oœæ w bok (czy odbierak siê nie zsun¹³)
 // - y: przyjmuje wartoœæ <0,1>, jeœli pod drutem (wzd³u¿)
 // - z: wysokoœæ bezwzglêdna drutu w danym miejsu
public: //na razie
 TTraction *pPrev,*pNext; //³¹czenie drutów w sieæ
 int iPrev,iNext; //do którego koñca siê ³¹czy
public:
 GLuint uiDisplayList;
 vector3 pPoint1,pPoint2,pPoint3,pPoint4;
 vector3 vParametric; //wspó³czynniki równania parametrycznego odcinka
 double fHeightDifference;//,fMiddleHeight;
  //  int iCategory,iMaterial,iDamageFlag;
//    float fU,fR,fMaxI,fWireThickness;
    int iNumSections;
    int iLines; //ilosc linii dla VBO
    float NominalVoltage;
    float MaxCurrent;
    float Resistivity;
    DWORD Material;   //1: Cu, 2: Al
    float WireThickness;
    DWORD DamageFlag; //1: zasniedziale, 128: zerwana
    int Wires;
    float WireOffset;
    AnsiString asPowerSupplyName; //McZapkie: nazwa podstacji trakcyjnej
//    bool bVisible;
//    DWORD dwFlags;

    void __fastcall Optimize();

    TTraction();
    ~TTraction();

//    virtual void __fastcall InitCenter(vector3 Angles, vector3 pOrigin);
//    virtual bool __fastcall Hit(double x, double z, vector3 &hitPoint, vector3 &hitDirection) { return TNode::Hit(x,z,hitPoint,hitDirection); };
  //  virtual bool __fastcall Move(double dx, double dy, double dz) { return false; };
//    virtual void __fastcall SelectedRender();
 void __fastcall RenderDL(float mgn);
 int __fastcall RaArrayPrepare();
 void __fastcall RaArrayFill(CVertNormTex *Vert);
 void __fastcall RenderVBO(float mgn,int iPtr);
 int __fastcall TestPoint(vector3 *Point);
 void __fastcall Connect(int my,TTraction *with,int to);
 void __fastcall Init();
};
//---------------------------------------------------------------------------
#endif
