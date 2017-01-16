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

#ifndef AnimModelH
#define AnimModelH

#include "Model3d.h"

const int iMaxNumLights = 8;

// typy stanu œwiate³
typedef enum
{
    ls_Off = 0, // zgaszone
    ls_On = 1, // zapalone
    ls_Blink = 2, // migaj¹ce
    ls_Dark = 3 // Ra: zapalajce siê automatycznie, gdy zrobi siê ciemno
} TLightState;

class TAnimVocaloidFrame
{ // ramka animacji typu Vocaloid Motion Data z programu MikuMikuDance
  public:
    char cBone[15]; // nazwa koœci, mo¿e byæ po japoñsku
    int iFrame; // numer ramki
    float3 f3Vector; // przemieszczenie
    float4 qAngle; // kwaternion obrotu
    char cBezier[64]; // krzywe Béziera do interpolacji dla x,y,z i obrotu
};

class TEvent;

class TAnimContainer
{ // opakowanie submodelu, okreœlaj¹ce animacjê egzemplarza - obs³ugiwane jako lista
    friend class TAnimModel;

  private:
    vector3 vRotateAngles; // dla obrotów Eulera
    vector3 vDesiredAngles;
    double fRotateSpeed;
    vector3 vTranslation;
    vector3 vTranslateTo;
    double fTranslateSpeed; // mo¿e tu daæ wektor?
    float4 qCurrent; // aktualny interpolowany
    float4 qStart; // pozycja pocz¹tkowa (0 dla interpolacji)
    float4 qDesired; // pozycja koñcowa (1 dla interpolacji)
    float fAngleCurrent; // parametr interpolacyjny: 0=start, 1=docelowy
    float fAngleSpeed; // zmiana parametru interpolacji w sekundach
    TSubModel *pSubModel;
    float4x4 *mAnim; // macierz do animacji kwaternionowych
    // dla kinematyki odwróconej u¿ywane s¹ kwaterniony
    float fLength; // d³ugoœæ koœci dla IK
    int iAnim; // animacja: +1-obrót Eulera, +2-przesuw, +4-obrót kwaternionem, +8-IK
    //+0x80000000: animacja z eventem, wykonywana poza wyœwietlaniem
    //+0x100: pierwszy stopieñ IK - obróciæ w stronê pierwszego potomnego (dziecka)
    //+0x200: drugi stopieñ IK - dostosowaæ do pozycji potomnego potomnego (wnuka)
    union
    { // mog¹ byæ animacje klatkowe ró¿nego typu, wskaŸniki u¿ywa AnimModel
        TAnimVocaloidFrame *pMovementData; // wskaŸnik do klatki
    };
    TEvent *evDone; // ewent wykonywany po zakoñczeniu animacji, np. zapór, obrotnicy
  public:
    TAnimContainer *pNext;
    TAnimContainer *acAnimNext; // lista animacji z eventem, które musz¹ byæ przeliczane równie¿ bez
    // wyœwietlania
    TAnimContainer();
    ~TAnimContainer();
    bool Init(TSubModel *pNewSubModel);
    // std::string inline GetName() { return
    // std::string(pSubModel?pSubModel->asName.c_str():""); };
    // std::string inline GetName() { return std::string(pSubModel?pSubModel->pName:"");
    // };
    char * NameGet()
    {
        return (pSubModel ? pSubModel->pName : NULL);
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
    }; // jednak ostatnia, T3D ma inny uk³ad
    vector3 _fastcall TransGet()
    {
        return vector3(-vTranslation.x, vTranslation.z, vTranslation.y);
    }; // zmiana, bo T3D ma inny uk³ad
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
    double fCurrent; // klatka animacji wyœwietlona w poprzedniej klatce renderingu
    double fLast; // klatka koñcz¹ca animacjê
    int iMovements;
    TAnimAdvanced();
    ~TAnimAdvanced();
    int SortByBone();
};

class TAnimModel
{ // opakowanie modelu, okreœlaj¹ce stan egzemplarza
  private:
    TAnimContainer *pRoot; // pojemniki steruj¹ce, tylko dla aniomowanych submodeli
    TModel3d *pModel;
    double fBlinkTimer;
    int iNumLights;
    TSubModel *LightsOn[iMaxNumLights]; // Ra: te wskaŸniki powinny byæ w ramach TModel3d
    TSubModel *LightsOff[iMaxNumLights];
    vector3 vAngle; // bazowe obroty egzemplarza wzglêdem osi
    int iTexAlpha; //¿eby nie sprawdzaæ za ka¿dym razem, dla 4 wymiennych tekstur
    std::string asText; // tekst dla wyœwietlacza znakowego
    TAnimAdvanced *pAdvanced;
    void Advanced();
    TLightState lsLights[iMaxNumLights];
    float fDark; // poziom zapalanie œwiat³a (powinno byæ chyba powi¹zane z danym œwiat³em?)
    float fOnTime, fOffTime; // by³y sta³ymi, teraz mog¹ byæ zmienne dla ka¿dego egzemplarza
  private:
    void RaAnimate(); // przeliczenie animacji egzemplarza
    void RaPrepare(); // ustawienie animacji egzemplarza na wzorcu
  public:
    GLuint ReplacableSkinId[5]; // McZapkie-020802: zmienialne skory
    static TAnimContainer *acAnimList; // lista animacji z eventem, które musz¹ byæ przeliczane
    // równie¿ bez wyœwietlania
    TAnimModel();
    ~TAnimModel();
    bool Init(TModel3d *pNewModel);
    bool Init(std::string const &asName, std::string const &asReplacableTexture);
    bool Load(cParser *parser, bool ter = false);
    TAnimContainer * AddContainer(char *pName);
    TAnimContainer * GetContainer(char *pName);
/*  void RenderDL(vector3 pPosition = vector3(0, 0, 0), double fAngle = 0);
    void RenderAlphaDL(vector3 pPosition = vector3(0, 0, 0), double fAngle = 0);
    void RenderVBO(vector3 pPosition = vector3(0, 0, 0), double fAngle = 0);
    void RenderAlphaVBO(vector3 pPosition = vector3(0, 0, 0), double fAngle = 0);
*/  void RenderDL(vector3 *vPosition);
    void RenderAlphaDL(vector3 *vPosition);
    void RenderVBO(vector3 *vPosition);
    void RenderAlphaVBO(vector3 *vPosition);
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
    void TerrainRenderVBO(int n);
    void AnimationVND(void *pData, double a, double b, double c, double d);
    void LightSet(int n, float v);
    static void AnimUpdate(double dt);
};


//---------------------------------------------------------------------------
#endif
