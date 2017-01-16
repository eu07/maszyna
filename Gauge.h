/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"

typedef enum
{ // typ ruchu
    gt_Unknown, // na razie nie znany
    gt_Rotate, // obrót
    gt_Move, // przesuniêcie równoleg³e
    gt_Wiper, // obrót trzech kolejnych submodeli o ten sam k¹t (np. wycieraczka, drzwi
    // harmonijkowe)
    gt_Digital // licznik cyfrowy, np. kilometrów
} TGaugeType;

class TGauge // zmienne "gg"
{ // animowany wskaŸnik, mog¹cy przyjmowaæ wiele stanów poœrednich
  private:
    TGaugeType eType; // typ ruchu
    double fFriction; // hamowanie przy zli¿aniu siê do zadanej wartoœci
    double fDesiredValue; // wartoœæ docelowa
    double fValue; // wartoœæ obecna
    double fOffset; // wartoœæ pocz¹tkowa ("0")
    double fScale; // wartoœæ koñcowa ("1")
    double fStepSize; // nie u¿ywane
    char cDataType; // typ zmiennej parametru: f-float, d-double, i-int
    union
    { // wskaŸnik na parametr pokazywany przez animacjê
        float *fData;
        double *dData;
        int *iData;
    };

  public:
    TGauge();
    ~TGauge();
    void Clear();
    void Init(TSubModel *NewSubModel, TGaugeType eNewTyp, double fNewScale = 1,
              double fNewOffset = 0, double fNewFriction = 0, double fNewValue = 0);
    bool Load(cParser &Parser, TModel3d *md1, TModel3d *md2 = NULL, double mul = 1.0);
    void PermIncValue(double fNewDesired);
    void IncValue(double fNewDesired);
    void DecValue(double fNewDesired);
    void UpdateValue(double fNewDesired);
    void PutValue(double fNewDesired);
    float GetValue()
    {
        return fValue;
    };
    void Update();
    void Render();
    void AssignFloat(float *fValue);
    void AssignDouble(double *dValue);
    void AssignInt(int *iValue);
    void UpdateValue();
    TSubModel *SubModel; // McZapkie-310302: zeby mozna bylo sprawdzac czy zainicjowany poprawnie
};

//---------------------------------------------------------------------------
