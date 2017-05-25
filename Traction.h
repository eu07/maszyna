/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include "GL/glew.h"
#include "VBO.h"
#include "dumb3d.h"

class TTractionPowerSource;

class TTraction
{ // drut zasilający, dla wskaźników używać przedrostka "hv"
  private:
    // vector3 vUp,vFront,vLeft;
    // matrix4x4 mMatrix;
    // matryca do wyliczania pozycji drutu w zależności od [X,Y,kąt] w scenerii:
    // - x: odległość w bok (czy odbierak się nie zsunął)
    // - y: przyjmuje wartość <0,1>, jeśli pod drutem (wzdłuż)
    // - z: wysokość bezwzględna drutu w danym miejsu
  public: // na razie
    TTractionPowerSource *psPower[2]; // najbliższe zasilacze z obu kierunków
    TTractionPowerSource *psPowered = nullptr; // ustawione tylko dla bezpośrednio zasilanego przęsła
    TTraction *hvNext[2]; //łączenie drutów w sieć
    int iNext[2]; // do którego końca się łączy
    int iLast = 1; //że niby ostatni drut // ustawiony bit 0, jeśli jest ostatnim drutem w sekcji; bit1 - przedostatni
  public:
    GLuint uiDisplayList = 0;
    Math3D::vector3 pPoint1, pPoint2, pPoint3, pPoint4;
    Math3D::vector3 vParametric; // współczynniki równania parametrycznego odcinka
    double fHeightDifference = 0.0; //,fMiddleHeight;
    // int iCategory,iMaterial,iDamageFlag;
    // float fU,fR,fMaxI,fWireThickness;
    int iNumSections = 0;
    int iLines = 0; // ilosc linii dla VBO
    float NominalVoltage = 0.0;
    float MaxCurrent = 0.0;
    float fResistivity = 0.0; //[om/m], przeliczone z [om/km]
    DWORD Material = 0; // 1: Cu, 2: Al
    float WireThickness = 0.0;
    DWORD DamageFlag = 0; // 1: zasniedziale, 128: zerwana
    int Wires = 2;
    float WireOffset = 0.0;
    std::string asPowerSupplyName; // McZapkie: nazwa podstacji trakcyjnej
    TTractionPowerSource *psSection = nullptr; // zasilacz (opcjonalnie może to być pulpit sterujący EL2 w hali!)
    std::string asParallel; // nazwa przęsła, z którym może być bieżnia wspólna
    TTraction *hvParallel = nullptr; // jednokierunkowa i zapętlona lista przęseł ewentualnej bieżni wspólnej
    float fResistance[2]; // rezystancja zastępcza do punktu zasilania (0: przęsło zasilane, <0: do policzenia)
    int iTries = 0;
    int PowerState{ 0 }; // type of incoming power, if any

    void Optimize();

    TTraction();
    ~TTraction();

    //    virtual void InitCenter(vector3 Angles, vector3 pOrigin);
    //    virtual bool Hit(double x, double z, vector3 &hitPoint, vector3 &hitDirection)
    //    { return TNode::Hit(x,z,hitPoint,hitDirection); };
    //  virtual bool Move(double dx, double dy, double dz) { return false; };
    //    virtual void SelectedRender();
    void RenderDL(float mgn);
    int RaArrayPrepare();
    int RaArrayFill( CVertNormTex *Vert, Math3D::vector3 const &Origin );
    void RenderVBO(float mgn, int iPtr);
    int TestPoint(Math3D::vector3 *Point);
    void Connect(int my, TTraction *with, int to);
    void Init();
    bool WhereIs();
    void ResistanceCalc(int d = -1, double r = 0, TTractionPowerSource *ps = NULL);
    void PowerSet(TTractionPowerSource *ps);
    double VoltageGet(double u, double i);
private:
    void wire_color( float &Red, float &Green, float &Blue ) const;
};
//---------------------------------------------------------------------------
