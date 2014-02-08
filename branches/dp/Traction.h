//---------------------------------------------------------------------------

#ifndef TractionH
#define TractionH

#include "opengl/glew.h"
#include "dumb3d.h"
#include "VBO.h"

using namespace Math3D;

class TTractionPowerSource; 

class TTraction 
{//drut zasilaj�cy, dla wska�nik�w u�ywa� przedrostka "hv"
private:
 vector3 vUp,vFront,vLeft;
 matrix4x4 mMatrix;
 //matryca do wyliczania pozycji drutu w zale�no�ci od [X,Y,k�t] w scenerii:
 // - x: odleg�o�� w bok (czy odbierak si� nie zsun��)
 // - y: przyjmuje warto�� <0,1>, je�li pod drutem (wzd�u�)
 // - z: wysoko�� bezwzgl�dna drutu w danym miejsu
public: //na razie
 TTraction *hvNext[2]; //��czenie drut�w w sie�
 int iNext[2]; //do kt�rego ko�ca si� ��czy
 int iLast; //ustawiony bit 0, je�li jest ostatnim drutem w sekcji; bit1 - przedostatni
public:
 GLuint uiDisplayList;
 vector3 pPoint1,pPoint2,pPoint3,pPoint4;
 vector3 vParametric; //wsp�czynniki r�wnania parametrycznego odcinka
 double fHeightDifference;//,fMiddleHeight;
 //int iCategory,iMaterial,iDamageFlag;
 //float fU,fR,fMaxI,fWireThickness;
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
 TTractionPowerSource *psPower; //zasilacz (opcjonalnie mo�e to by� pulpit steruj�cy w hali!)
 AnsiString asParallel; //nazwa prz�s�a, z kt�rym mo�e by� bie�nia wsp�lna
 TTraction *hvParallel; //jednokierunkowa i zap�tlona lista prz�se� ewentualnej bie�ni wsp�lnej
 //bool bVisible;
 //DWORD dwFlags;

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
 void __fastcall WhereIs();
};
//---------------------------------------------------------------------------
#endif
