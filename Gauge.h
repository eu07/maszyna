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
#include "sound.h"

typedef enum
{ // typ ruchu
    gt_Unknown, // na razie nie znany
    gt_Rotate, // obrót
    gt_Move, // przesunięcie równoległe
    gt_Wiper, // obrót trzech kolejnych submodeli o ten sam kąt (np. wycieraczka, drzwi
    // harmonijkowe)
    gt_Digital // licznik cyfrowy, np. kilometrów
} TGaugeType;

// animowany wskaźnik, mogący przyjmować wiele stanów pośrednich
class TGauge { 

  private:
    TGaugeType eType { gt_Unknown }; // typ ruchu
    double fFriction { 0.0 }; // hamowanie przy zliżaniu się do zadanej wartości
    double fDesiredValue { 0.0 }; // wartość docelowa
    double fValue { 0.0 }; // wartość obecna
    double fOffset { 0.0 }; // wartość początkowa ("0")
    double fScale { 1.0 }; // wartość końcowa ("1")
    char cDataType; // typ zmiennej parametru: f-float, d-double, i-int
    union {
        // wskaźnik na parametr pokazywany przez animację
        float *fData;
        double *dData { nullptr };
        int *iData;
    };
    PSound m_soundfxincrease { nullptr }; // sound associated with increasing control's value
    PSound m_soundfxdecrease { nullptr }; // sound associated with decreasing control's value
    std::map<int, PSound> m_soundfxvalues; // sounds associated with specific values
// methods
    // imports member data pair from the config file
    bool
        Load_mapping( cParser &Input );
    // plays specified sound
    void
        play( PSound Sound );

  public:
    TGauge() = default;
    inline
    void Clear() { *this = TGauge(); }
    void Init(TSubModel *NewSubModel, TGaugeType eNewTyp, double fNewScale = 1, double fNewOffset = 0, double fNewFriction = 0, double fNewValue = 0);
    bool Load(cParser &Parser, TModel3d *md1, TModel3d *md2 = nullptr, double mul = 1.0);
    void PermIncValue(double fNewDesired);
    void IncValue(double fNewDesired);
    void DecValue(double fNewDesired);
    void UpdateValue(double fNewDesired, PSound Fallbacksound = nullptr );
    void PutValue(double fNewDesired);
    double GetValue() const;
    void Update();
    void Render();
    void AssignFloat(float *fValue);
    void AssignDouble(double *dValue);
    void AssignInt(int *iValue);
    void UpdateValue();
    TSubModel *SubModel; // McZapkie-310302: zeby mozna bylo sprawdzac czy zainicjowany poprawnie
};

//---------------------------------------------------------------------------
