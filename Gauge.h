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
    gt_Move, // przesunięcie równoległe
    gt_Wiper, // obrót trzech kolejnych submodeli o ten sam kąt (np. wycieraczka, drzwi
    // harmonijkowe)
    gt_Digital // licznik cyfrowy, np. kilometrów
} TGaugeType;

class TGauge // zmienne "gg"
{ // animowany wskaźnik, mogący przyjmować wiele stanów pośrednich
  private:
    TGaugeType eType; // typ ruchu
    double fFriction; // hamowanie przy zliżaniu się do zadanej wartości
    double fDesiredValue; // wartość docelowa
    double fValue; // wartość obecna
    double fOffset; // wartość początkowa ("0")
    double fScale; // wartość końcowa ("1")
    double fStepSize; // nie używane
    char cDataType; // typ zmiennej parametru: f-float, d-double, i-int
    union
    { // wskaźnik na parametr pokazywany przez animację
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
