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

enum class TGaugeAnimation {
    // typ ruchu
    gt_Unknown, // na razie nie znany
    gt_Rotate, // obrót
    gt_Move, // przesunięcie równoległe
    gt_Wiper, // obrót trzech kolejnych submodeli o ten sam kąt (np. wycieraczka, drzwi harmonijkowe)
    gt_Digital // licznik cyfrowy, np. kilometrów
};

enum class TGaugeType {
    toggle,
    push
};

// animowany wskaźnik, mogący przyjmować wiele stanów pośrednich
class TGauge { 

public:
// methods
    TGauge() = default;
    explicit TGauge( sound_source const &Soundtemplate );

    inline
    void Clear() {
        *this = TGauge(); }
    void Init(TSubModel *Submodel, TGaugeAnimation Type, float Scale = 1, float Offset = 0, float Friction = 0, float Value = 0, float const Endvalue = -1.0, float const Endscale = -1.0, bool const Interpolate = false );
    void Load(cParser &Parser, TDynamicObject const *Owner, double const mul = 1.0);
    void UpdateValue( float fNewDesired );
    void UpdateValue( float fNewDesired, sound_source &Fallbacksound );
    void PutValue(float fNewDesired);
    float GetValue() const;
    float GetDesiredValue() const;
    void Update();
    void AssignFloat(float *fValue);
    void AssignDouble(double *dValue);
    void AssignInt(int *iValue);
    void UpdateValue();
    // returns offset of submodel associated with the button from the model centre
    glm::vec3 model_offset() const;
    TGaugeType type() const;
// members
    TSubModel *SubModel { nullptr }; // McZapkie-310302: zeby mozna bylo sprawdzac czy zainicjowany poprawnie

private:
// methods
// imports member data pair from the config file
    bool
        Load_mapping( cParser &Input );
    void
        UpdateValue( float fNewDesired, sound_source *Fallbacksound );
    float
        GetScaledValue() const;

// members
    TGaugeAnimation m_animation { TGaugeAnimation::gt_Unknown }; // typ ruchu
    TGaugeType m_type { TGaugeType::toggle }; // switch type
    float m_friction { 0.f }; // hamowanie przy zliżaniu się do zadanej wartości
    float m_targetvalue { 0.f }; // wartość docelowa
    float m_value { 0.f }; // wartość obecna
    float m_offset { 0.f }; // wartość początkowa ("0")
    float m_scale { 1.f }; // scale applied to the value at the start of accepted value range
    float m_endscale { -1.f }; // scale applied to the value at the end of accepted value range
    float m_endvalue { -1.f }; // end value of accepted value range
    bool m_interpolatescale { false };
    char m_datatype; // typ zmiennej parametru: f-float, d-double, i-int
    union {
        // wskaźnik na parametr pokazywany przez animację
        float *fData;
        double *dData { nullptr };
        int *iData;
    };
    int m_soundtype { 0 }; // toggle between exclusive and multiple sound generation
    sound_source m_soundtemplate { sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE }; // shared properties for control's sounds
    sound_source m_soundfxincrease { m_soundtemplate }; // sound associated with increasing control's value
    sound_source m_soundfxdecrease { m_soundtemplate }; // sound associated with decreasing control's value
    std::map<int, sound_source> m_soundfxvalues; // sounds associated with specific values

};

//---------------------------------------------------------------------------
