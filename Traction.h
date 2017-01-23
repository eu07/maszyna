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
#include "opengl/glew.h"
#include "VBO.h"
#include "dumb3d.h"

using namespace Math3D;

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
    TTractionPowerSource *psPowered; // ustawione tylko dla bezpośrednio zasilanego przęsła
    TTraction *hvNext[2]; //łączenie drutów w sieć
    int iNext[2]; // do którego końca się łączy
    int iLast; // ustawiony bit 0, jeśli jest ostatnim drutem w sekcji; bit1 - przedostatni
  public:
    GLuint uiDisplayList;
    vector3 pPoint1, pPoint2, pPoint3, pPoint4;
    vector3 vParametric; // współczynniki równania parametrycznego odcinka
    double fHeightDifference; //,fMiddleHeight;
    // int iCategory,iMaterial,iDamageFlag;
    // float fU,fR,fMaxI,fWireThickness;
    int iNumSections;
    int iLines; // ilosc linii dla VBO
    float NominalVoltage;
    float MaxCurrent;
    float fResistivity; //[om/m], przeliczone z [om/km]
    DWORD Material; // 1: Cu, 2: Al
    float WireThickness;
    DWORD DamageFlag; // 1: zasniedziale, 128: zerwana
    int Wires;
    float WireOffset;
    std::string asPowerSupplyName; // McZapkie: nazwa podstacji trakcyjnej
    TTractionPowerSource *
        psSection; // zasilacz (opcjonalnie może to być pulpit sterujący EL2 w hali!)
    std::string asParallel; // nazwa przęsła, z którym może być bieżnia wspólna
    TTraction *hvParallel; // jednokierunkowa i zapętlona lista przęseł ewentualnej bieżni wspólnej
    float fResistance[2]; // rezystancja zastępcza do punktu zasilania (0: przęsło zasilane, <0: do
    // policzenia)
    int iTries;
    // bool bVisible;
    // DWORD dwFlags;

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
    void RaArrayFill(CVertNormTex *Vert);
    void RenderVBO(float mgn, int iPtr);
    int TestPoint(vector3 *Point);
    void Connect(int my, TTraction *with, int to);
    void Init();
    bool WhereIs();
    void ResistanceCalc(int d = -1, double r = 0, TTractionPowerSource *ps = NULL);
    void PowerSet(TTractionPowerSource *ps);
    double VoltageGet(double u, double i);
};
//---------------------------------------------------------------------------
