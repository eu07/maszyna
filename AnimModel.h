/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#pragma once

#include "Model3d.h"
#include "Texture.h"
#include "DynObj.h"

const int iMaxNumLights = 8;

// typy stanu świateł
typedef enum
{
    ls_Off = 0, // zgaszone
    ls_On = 1, // zapalone
    ls_Blink = 2, // migające
    ls_Dark = 3 // Ra: zapalajce się automatycznie, gdy zrobi się ciemno
} TLightState;

class TAnimVocaloidFrame
{ // ramka animacji typu Vocaloid Motion Data z programu MikuMikuDance
  public:
    char cBone[15]; // nazwa kości, może być po japońsku
    int iFrame; // numer ramki
    float3 f3Vector; // przemieszczenie
    float4 qAngle; // kwaternion obrotu
    char cBezier[64]; // krzywe Béziera do interpolacji dla x,y,z i obrotu
};

class TEvent;

class TAnimContainer
{ // opakowanie submodelu, określające animację egzemplarza - obsługiwane jako lista
    friend class TAnimModel;

  private:
    vector3 vRotateAngles; // dla obrotów Eulera
    vector3 vDesiredAngles;
    double fRotateSpeed;
    vector3 vTranslation;
    vector3 vTranslateTo;
    double fTranslateSpeed; // może tu dać wektor?
    float4 qCurrent; // aktualny interpolowany
    float4 qStart; // pozycja początkowa (0 dla interpolacji)
    float4 qDesired; // pozycja końcowa (1 dla interpolacji)
    float fAngleCurrent; // parametr interpolacyjny: 0=start, 1=docelowy
    float fAngleSpeed; // zmiana parametru interpolacji w sekundach
    TSubModel *pSubModel;
    float4x4 *mAnim; // macierz do animacji kwaternionowych
    // dla kinematyki odwróconej używane są kwaterniony
    float fLength; // długość kości dla IK
    int iAnim; // animacja: +1-obrót Eulera, +2-przesuw, +4-obrót kwaternionem, +8-IK
    //+0x80000000: animacja z eventem, wykonywana poza wyświetlaniem
    //+0x100: pierwszy stopień IK - obrócić w stronę pierwszego potomnego (dziecka)
    //+0x200: drugi stopień IK - dostosować do pozycji potomnego potomnego (wnuka)
    union
    { // mogą być animacje klatkowe różnego typu, wskaźniki używa AnimModel
        TAnimVocaloidFrame *pMovementData; // wskaźnik do klatki
    };
    TEvent *evDone; // ewent wykonywany po zakończeniu animacji, np. zapór, obrotnicy
  public:
    TAnimContainer *pNext;
    TAnimContainer *acAnimNext; // lista animacji z eventem, które muszą być przeliczane również bez
    // wyświetlania
    TAnimContainer();
    ~TAnimContainer();
    bool Init(TSubModel *pNewSubModel);
    // std::string inline GetName() { return
    // std::string(pSubModel?pSubModel->asName.c_str():""); };
    // std::string inline GetName() { return std::string(pSubModel?pSubModel->pName:"");
    // };
    std::string NameGet()
    {
        return (pSubModel ? pSubModel->pName : "");
    };
    // void SetRotateAnim(vector3 vNewRotateAxis, double fNewDesiredAngle, double
    // fNewRotateSpeed, bool bResetAngle=false);
    void SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed);
    void SetTranslateAnim(vector3 vNewTranslate, double fNewSpeed);
    void AnimSetVMD(double fNewSpeed);
    void PrepareModel();
    void UpdateModel();
    void UpdateModelIK();
    bool InMovement(); // czy w trakcie animacji?
    double _fastcall AngleGet()
    {
        return vRotateAngles.z;
    }; // jednak ostatnia, T3D ma inny układ
    vector3 _fastcall TransGet()
    {
        return vector3(-vTranslation.x, vTranslation.z, vTranslation.y);
    }; // zmiana, bo T3D ma inny układ
    void WillBeAnimated()
    {
        if (pSubModel)
            pSubModel->WillBeAnimated();
    };
    void EventAssign(TEvent *ev);
    TEvent * Event()
    {
        return evDone;
    };
};

class TAnimAdvanced
{ // obiekt zaawansowanej animacji submodelu
  public:
    TAnimVocaloidFrame *pMovementData;
    unsigned char *pVocaloidMotionData; // plik animacyjny dla egzemplarza (z eventu)
    double fFrequency; // przeliczenie czasu rzeczywistego na klatki animacji
    double fCurrent; // klatka animacji wyświetlona w poprzedniej klatce renderingu
    double fLast; // klatka kończąca animację
    int iMovements;
    TAnimAdvanced();
    ~TAnimAdvanced();
    int SortByBone();
};

// opakowanie modelu, określające stan egzemplarza
class TAnimModel {

    friend class opengl_renderer;

  private:
    TAnimContainer *pRoot; // pojemniki sterujące, tylko dla aniomowanych submodeli
    TModel3d *pModel;
    double fBlinkTimer;
    int iNumLights;
    TSubModel *LightsOn[iMaxNumLights]; // Ra: te wskaźniki powinny być w ramach TModel3d
    TSubModel *LightsOff[iMaxNumLights];
    vector3 vAngle; // bazowe obroty egzemplarza względem osi
    material_data m_materialdata;

    std::string asText; // tekst dla wyświetlacza znakowego
    TAnimAdvanced *pAdvanced;
    void Advanced();
    TLightState lsLights[iMaxNumLights];
    float fDark; // poziom zapalanie światła (powinno być chyba powiązane z danym światłem?)
    float fOnTime, fOffTime; // były stałymi, teraz mogą być zmienne dla każdego egzemplarza
  private:
    void RaAnimate(); // przeliczenie animacji egzemplarza
    void RaPrepare(); // ustawienie animacji egzemplarza na wzorcu
  public:
    static TAnimContainer *acAnimList; // lista animacji z eventem, które muszą być przeliczane również bez wyświetlania
    inline
        material_data const *Material() const { return &m_materialdata; }

    TAnimModel();
    ~TAnimModel();
    bool Init(TModel3d *pNewModel);
    bool Init(std::string const &asName, std::string const &asReplacableTexture);
    bool Load(cParser *parser, bool ter = false);
    TAnimContainer * AddContainer(char *pName);
    TAnimContainer * GetContainer(char *pName);
#ifdef EU07_USE_OLD_RENDERCODE
    void Render( vector3 const &Position );
    void RenderAlpha( vector3 const &Position );
#endif
    int Flags();
    void RaAnglesSet(double a, double b, double c)
    {
        vAngle.x = a;
        vAngle.y = b;
        vAngle.z = c;
    };
    bool TerrainLoaded();
    int TerrainCount();
    TSubModel * TerrainSquare(int n);
#ifdef EU07_USE_OLD_RENDERCODE
    void TerrainRenderVBO(int n);
#endif
    void AnimationVND(void *pData, double a, double b, double c, double d);
    void LightSet(int n, float v);
    static void AnimUpdate(double dt);
};

//---------------------------------------------------------------------------
