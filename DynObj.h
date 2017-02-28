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
#include <functional>

#include "TrkFoll.h"
// McZapkie:
#include "RealSound.h"
#include "AdvSound.h"
#include "Button.h"
#include "AirCoupler.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int const ANIM_TYPES = 7; // Ra: ilość typów animacji
int const ANIM_WHEELS = 0; // koła
int const ANIM_DOORS = 1; // drzwi
int const ANIM_LEVERS = 2; // elementy obracane (wycieraczki, koła skrętne, przestawiacze, klocki ham.)
int const ANIM_BUFFERS = 3; // elementy przesuwane (zderzaki)
int const ANIM_BOOGIES = 4; // wózki (są skręcane w dwóch osiach)
int const ANIM_PANTS = 5; // pantografy
int const ANIM_STEAMS = 6; // napęd parowozu

class TAnim;
//typedef void(__closure *TUpdate)(TAnim *pAnim); // typ funkcji aktualizującej położenie submodeli
typedef std::function<void(TAnim *)> TUpdate; // __closure is Borland-specific extension

// McZapkie-250202
int const MaxAxles = 16; // ABu 280105: zmienione z 8 na 16
// const MaxAnimatedAxles=16; //i to tez.
// const MaxAnimatedDoors=16;  //NBMX  wrzesien 2003
/*
Ra: Utworzyć klasę wyposażenia opcjonalnego, z której będą dziedziczyć klasy drzwi,
pantografów, napędu parowozu i innych ruchomych części pojazdów. Klasy powinny być
pseudo-wirtualne, bo wirtualne mogą obniżać wydajnosść.
Przy wczytywaniu MMD utworzyć tabelę wskaźnikow na te dodatki. Przy wyświetlaniu
pojazdu wykonywać Update() na kolejnych obiektach wyposażenia.
Rozważyć użycie oddzielnych modeli dla niektórych pojazdów (np. lokomotywy), co
zaoszczędziło by czas ustawiania animacji na modelu wspólnym dla kilku pojazdów,
szczególnie dla pojazdów w danej chwili nieruchomych (przy dużym zagęszczeniu
modeli na stacjach na ogół przewaga jest tych nieruchomych).
*/
class TAnimValveGear
{ // współczynniki do animacji parowozu (wartości przykładowe dla Pt47)
    int iValues; // ilość liczb (wersja):
    float fKorbowodR; // długość korby (pół skoku tłoka) [m]: 0.35
    float fKorbowodL; // długość korbowodu [m]: 3.8
    float fDrazekR; // promień mimośrodu (drążka) [m]: 0.18
    float fDrazekL; // dł. drążka mimośrodowego [m]: 2.55889
    float fJarzmoV; // wysokość w pionie osi jarzma od osi koła [m]: 0.751
    float fJarzmoH; // odległość w poziomie osi jarzma od osi koła [m]: 2.550
    float fJarzmoR; // promień jarzma do styku z drążkiem [m]: 0.450
    float fJarzmoA; // kąt mimośrodu względem kąta koła [°]: -96.77416667
    float fWdzidloL; // długość wodzidła [m]: 2.0
    float fWahaczH; // długość wahacza (góra) [m]: 0.14
    float fSuwakH; // wysokość osi suwaka ponad osią koła [m]: 0.62
    float fWahaczL; // długość wahacza (dół) [m]: 0.84
    float fLacznikL; // długość łącznika wahacza [m]: 0.75072
    float fRamieL; // odległość ramienia krzyżulca od osi koła [m]: 0.192
    float fSuwakL; // odległość środka tłoka/suwaka od osi koła [m]: 5.650
    // dołożyć parametry drążka nastawnicy
    // albo nawet zrobić dynamiczną tablicę float[] i w nią pakować wszelkie współczynniki, potem
    // używać indeksów
    // współczynniki mogą być wspólne dla 2-4 tłoków, albo każdy tłok może mieć odrębne
};

class TAnimPant
{ // współczynniki do animacji pantografu
  public:
    vector3 vPos; // Ra: współrzędne punktu zerowego pantografu (X dodatnie dla przedniego)
    double fLenL1; // długość dolnego ramienia 1, odczytana z modelu
    double fLenU1; // długość górnego ramienia 1, odczytana z modelu
    double fLenL2; // długość dolnego ramienia 2, odczytana z modelu
    double fLenU2; // długość górnego ramienia 2, odczytana z modelu
    double fHoriz; // przesunięcie ślizgu w długości pojazdu względem osi obrotu dolnego ramienia
    double fHeight; // wysokość ślizgu ponad oś obrotu, odejmowana od wysokości drutu
    double fWidth; // połowa szerokości roboczej ślizgu, do wykrycia ześlizgnięcia się drutu
    double fAngleL0; // Ra: początkowy kąt dolnego ramienia (odejmowany przy animacji)
    double fAngleU0; // Ra: początkowy kąt górnego ramienia (odejmowany przy animacji)
    double PantTraction; // Winger 170204: wysokość drutu ponad punktem na wysokości vPos.y p.g.s.
    double PantWys; // Ra: aktualna wysokość uniesienia ślizgu do porównania z wysokością drutu
    double fAngleL; // Winger 160204: aktualny kąt ramienia dolnego
    double fAngleU; // Ra: aktualny kąt ramienia górnego
    double NoVoltTime; // czas od utraty kontaktu z drutem
    TTraction *hvPowerWire; // aktualnie podczepione druty, na razie tu
    float fWidthExtra; // dodatkowy rozmiar poziomy poza część roboczą (fWidth)
    float fHeightExtra[5]; //łamana symulująca kształt nabieżnika
    // double fHorizontal; //Ra 2015-01: położenie drutu względem osi pantografu
    void AKP_4E();
};

class TAnim
{ // klasa animowanej części pojazdu (koła, drzwi, pantografy, burty, napęd parowozu, siłowniki
    // itd.)
  public:
    union
    {
        TSubModel *smAnimated; // animowany submodel (jeśli tylko jeden, np. oś)
        TSubModel **smElement; // jeśli animowanych elementów jest więcej (pantograf, napęd
        // parowozu)
        int iShift; // przesunięcie przed przydzieleniem wskaźnika
    };
    union
    { // parametry animacji
        TAnimValveGear *pValveGear; // współczynniki do animacji parowozu
        double *dWheelAngle; // wskaźnik na kąt obrotu osi
        float *fParam; // różne parametry dla animacji
        TAnimPant *fParamPants; // różne parametry dla animacji
    };
    union
    { // wskaźnik na obiekt odniesienia
        double *fDoubleBase; // jakiś double w fizyce
        float *fFloatBase; // jakiś float w fizyce
        int *iIntBase; // jakiś int w fizyce
    };
    // void _fastcall Update(); //wskaźnik do funkcji aktualizacji animacji
    int iFlags; // flagi animacji
    float fMaxDist; // do jakiej odległości wykonywana jest animacja
    float fSpeed; // parametr szybkości animacji
    int iNumber; // numer kolejny obiektu
  public:
    TAnim();
    ~TAnim();
    TUpdate yUpdate; // metoda TDynamicObject aktualizująca animację
    int TypeSet(int i, int fl = 0); // ustawienie typu
    void Parovoz(); // wykonanie obliczeń animacji
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class TDynamicObject { // klasa pojazdu
private: // położenie pojazdu w świecie oraz parametry ruchu
    vector3 vPosition; // Ra: pozycja pojazdu liczona zaraz po przesunięciu
    vector3 vCoulpler[ 2 ]; // współrzędne sprzęgów do liczenia zderzeń czołowych
    vector3 vUp, vFront, vLeft; // wektory jednostkowe ustawienia pojazdu
    int iDirection; // kierunek pojazdu względem czoła składu (1=zgodny,0=przeciwny)
    TTrackShape ts; // parametry toru przekazywane do fizyki
    TTrackParam tp; // parametry toru przekazywane do fizyki
    TTrackFollower Axle0; // oś z przodu (od sprzęgu 0)
    TTrackFollower Axle1; // oś z tyłu (od sprzęgu 1)
    int iAxleFirst; // numer pierwszej osi w kierunku ruchu (oś wiążąca pojazd z torem i wyzwalająca
    // eventy)
    float fAxleDist; // rozstaw wózków albo osi do liczenia proporcji zacienienia
    vector3 modelRot; // obrot pudła względem świata - do przeanalizowania, czy potrzebne!!!
    // bool bCameraNear; //blisko kamer są potrzebne dodatkowe obliczenia szczegółów
    TDynamicObject * ABuFindNearestObject( TTrack *Track, TDynamicObject *MyPointer,
        int &CouplNr );

public: // parametry położenia pojazdu dostępne publicznie
    std::string asTrack; // nazwa toru początkowego; wywalić?
    std::string asDestination; // dokąd pojazd ma być kierowany "(stacja):(tor)"
    matrix4x4 mMatrix; // macierz przekształcenia do renderowania modeli
    TMoverParameters *MoverParameters; // parametry fizyki ruchu oraz przeliczanie
    // TMoverParameters *pControlled; //wskaźnik do sterowanego członu silnikowego
    TDynamicObject *NextConnected; // pojazd podłączony od strony sprzęgu 1 (kabina -1)
    TDynamicObject *PrevConnected; // pojazd podłączony od strony sprzęgu 0 (kabina 1)
    int NextConnectedNo; // numer sprzęgu podłączonego z tyłu
    int PrevConnectedNo; // numer sprzęgu podłączonego z przodu
    double fScanDist; // odległość skanowania torów na obecność innych pojazdów

    TPowerSource ConnectedEnginePowerSource( TDynamicObject const *Caller ) const;

private:
    // returns type of the nearest functional power source present in the trainset
public: // modele składowe pojazdu
    TModel3d *mdModel; // model pudła
    TModel3d *mdLoad; // model zmiennego ładunku
    TModel3d *mdPrzedsionek; // model przedsionków dla EZT - może użyć mdLoad zamiast?
    TModel3d *mdKabina; // model kabiny dla użytkownika; McZapkie-030303: to z train.h
    TModel3d *mdLowPolyInt; // ABu 010305: wnetrze lowpoly
    float3 InteriorLight{ 0.9f * 255.0f / 255.0f, 0.9f * 216.0f / 255.0f, 0.9f * 176.0f / 255.0f }; // tungsten light. TODO: allow definition of light type?
    float InteriorLightLevel{ 0.0f }; // current level of interior lighting
    float fShade; // zacienienie: 0:normalnie, -1:w ciemności, +1:dodatkowe światło (brak koloru?)

  private: // zmienne i metody do animacji submodeli; Ra: sprzatam animacje w pojeździe
  public: // tymczasowo udostępnione do wyszukiwania drutu
      int iAnimType[ ANIM_TYPES ]; // 0-osie,1-drzwi,2-obracane,3-zderzaki,4-wózki,5-pantografy,6-tłoki
  private:
    int iAnimations; // liczba obiektów animujących
/*
    TAnim *pAnimations; // obiekty animujące (zawierają wskaźnik do funkcji wykonującej animację)
*/
    std::vector<TAnim> pAnimations;
    TSubModel **
        pAnimated; // lista animowanych submodeli (może być ich więcej niż obiektów animujących)
    double dWheelAngle[3]; // kąty obrotu kół: 0=przednie toczne, 1=napędzające i wiązary, 2=tylne
    // toczne
    void
    UpdateNone(TAnim *pAnim){}; // animacja pusta (funkcje ustawiania submodeli, gdy blisko kamery)
    void UpdateAxle(TAnim *pAnim); // animacja osi
    void UpdateBoogie(TAnim *pAnim); // animacja wózka
    void UpdateDoorTranslate(TAnim *pAnim); // animacja drzwi - przesuw
    void UpdateDoorRotate(TAnim *pAnim); // animacja drzwi - obrót
    void UpdateDoorFold(TAnim *pAnim); // animacja drzwi - składanie
	void UpdateDoorPlug(TAnim *pAnim);      // animacja drzwi - odskokowo-przesuwne
	void UpdatePant(TAnim *pAnim); // animacja pantografu
    void UpdateLeverDouble(TAnim *pAnim); // animacja gałki zależna od double
    void UpdateLeverFloat(TAnim *pAnim); // animacja gałki zależna od float
    void UpdateLeverInt(TAnim *pAnim); // animacja gałki zależna od int (wartość)
    void UpdateLeverEnum(TAnim *pAnim); // animacja gałki zależna od int (lista kątów)
  private: // Ra: ciąg dalszy animacji, dopiero do ogarnięcia
    // ABuWozki 060504
    vector3 bogieRot[2]; // Obroty wozkow w/m korpusu
    TSubModel *smBogie[2]; // Wyszukiwanie max 2 wozkow
    TSubModel *smWahacze[4]; // wahacze (np. nogi, dźwignia w drezynie)
    TSubModel *smBrakeMode; // Ra 15-01: nastawa hamulca też
    TSubModel *smLoadMode; // Ra 15-01: nastawa próżny/ładowny
    double fWahaczeAmp;
    // Winger 160204 - pantografy
    double pantspeedfactor;
    // animacje typu przesuw
    TSubModel *smBuforLewy[2];
    TSubModel *smBuforPrawy[2];
    TAnimValveGear *pValveGear;
    vector3 vFloor; // podłoga dla ładunku
  public:
    TAnim *pants; // indeks obiektu animującego dla pantografu 0
    double NoVoltTime; // czas od utraty zasilania
    double dDoorMoveL; // NBMX
    double dDoorMoveR; // NBMX
    TSubModel *smBrakeSet; // nastawa hamulca (wajcha)
    TSubModel *smLoadSet; // nastawa ładunku (wajcha)
    TSubModel *smWiper; // wycieraczka (poniekąd też wajcha)
    // Ra: koneic animacji do ogarnięcia

  private:
    void ABuLittleUpdate(double ObjSqrDist);
    bool btnOn; // ABu: czy byly uzywane buttony, jesli tak, to po renderingu wylacz
    // bo ten sam model moze byc jeszcze wykorzystany przez inny obiekt!
    double ComputeRadius(vector3 p1, vector3 p2, vector3 p3, vector3 p4);

    TButton btCoupler1; // sprzegi
    TButton btCoupler2;
    TAirCoupler
        btCPneumatic1; // sprzegi powietrzne //yB - zmienione z Button na AirCoupler - krzyzyki
    TAirCoupler btCPneumatic2;
    TAirCoupler btCPneumatic1r; // ABu: to zeby nie bylo problemow przy laczeniu wagonow,
    TAirCoupler btCPneumatic2r; //     jesli beda polaczone sprzegami 1<->1 lub 0<->0
    TAirCoupler btPneumatic1; // ABu: sprzegi powietrzne zolte
    TAirCoupler btPneumatic2;
    TAirCoupler btPneumatic1r; // ABu: analogicznie jak 4 linijki wyzej
    TAirCoupler btPneumatic2r;

    TButton btCCtrl1; // sprzegi sterowania
    TButton btCCtrl2;
    TButton btCPass1; // mostki przejsciowe
    TButton btCPass2;
    char cp1, sp1, cp2, sp2; // ustawienia węży

    TButton btEndSignals11; // sygnalu konca pociagu
    TButton btEndSignals13;
    TButton btEndSignals21;
    TButton btEndSignals23;
    TButton btEndSignals1; // zeby bylo kompatybilne ze starymi modelami...
    TButton btEndSignals2;
    TButton btEndSignalsTab1; // sygnaly konca pociagu (blachy)
    TButton btEndSignalsTab2;
    TButton btHeadSignals11; // oswietlenie czolowe - przod
    TButton btHeadSignals12;
    TButton btHeadSignals13;
    TButton btHeadSignals21; // oswietlenie czolowe - tyl
    TButton btHeadSignals22;
    TButton btHeadSignals23;
	TButton btMechanik1;
	TButton btMechanik2;
    //TSubModel *smMechanik0; // Ra: mechanik wbudowany w model jako submodel?
    //TSubModel *smMechanik1; // mechanik od strony sprzęgu 1
    double enginevolume; // MC: pomocnicze zeby gladziej silnik buczal

    int iAxles; // McZapkie: to potem mozna skasowac i zastapic iNumAxles
    double dRailLength;
    double dRailPosition[MaxAxles]; // licznik pozycji osi w/m szyny
    double dWheelsPosition[MaxAxles]; // pozycja osi w/m srodka pojazdu
    TRealSound rsStukot[MaxAxles]; // dzwieki poszczegolnych osi //McZapkie-270202
    TRealSound rsSilnik; // McZapkie-010302 - silnik
    TRealSound rsWentylator; // McZapkie-030302
    TRealSound rsPisk; // McZapkie-260302
    TRealSound rsDerailment; // McZapkie-051202
    TRealSound rsPrzekladnia;
    TAdvancedSound sHorn1;
    TAdvancedSound sHorn2;
    TAdvancedSound sCompressor; // NBMX wrzesien 2003
    TAdvancedSound sConverter;
    TAdvancedSound sSmallCompressor;
    TAdvancedSound sDepartureSignal;
    TAdvancedSound sTurbo;
	TAdvancedSound sSand;
	TAdvancedSound sReleaser;

    // Winger 010304
    //    TRealSound rsPanTup; //PSound sPantUp;
    TRealSound sPantUp;
    TRealSound sPantDown;
    TRealSound rsDoorOpen; // Ra: przeniesione z kabiny
    TRealSound rsDoorClose;

    double eng_vol_act;
    double eng_frq_act;
    double eng_dfrq;
    double eng_turbo;
    void ABuBogies();
    void ABuModelRoll();
    vector3 modelShake;

    bool renderme; // yB - czy renderowac
    // TRealSound sBrakeAcc; //dźwięk przyspieszacza
    PSound sBrakeAcc;
    bool bBrakeAcc;
    TRealSound rsUnbrake; // yB - odglos luzowania
    float ModCamRot;
    int iInventory; // flagi bitowe posiadanych submodeli (np. świateł)
    void TurnOff();

  public:
    int iHornWarning; // numer syreny do użycia po otrzymaniu sygnału do jazdy
    bool bEnabled; // Ra: wyjechał na portal i ma być usunięty
  protected:
    // TTrackFollower Axle2; //dwie osie z czterech (te są protected)
    // TTrackFollower Axle3; //Ra: wyłączyłem, bo kąty są liczone w Segment.cpp
    int iNumAxles; // ilość osi
    int CouplCounter;
    std::string asModel;

  public:
    void ABuScanObjects(int ScanDir, double ScanDist);

  protected:
    TDynamicObject * ABuFindObject(TTrack *Track, int ScanDir, BYTE &CouplFound,
                                             double &dist);
    void ABuCheckMyTrack();

  public:
    int *iLights; // wskaźnik na bity zapalonych świateł (własne albo innego członu)
    double fTrackBlock; // odległość do przeszkody do dalszego ruchu (wykrywanie kolizji z innym
    // pojazdem)
    TDynamicObject * PrevAny();
    TDynamicObject * Prev();
    TDynamicObject * Next();
	TDynamicObject * PrevC(int C);
	TDynamicObject * NextC(int C);
    double NextDistance(double d = -1.0);
    void SetdMoveLen(double dMoveLen)
    {
        MoverParameters->dMoveLen = dMoveLen;
    }
    void ResetdMoveLen()
    {
        MoverParameters->dMoveLen = 0;
    }
    double GetdMoveLen()
    {
        return MoverParameters->dMoveLen;
    }

    int GetPneumatic(bool front, bool red);
    void SetPneumatic(bool front, bool red);
    std::string asName;
    std::string GetName()
    {
        return this ? asName : std::string("");
    };

    TRealSound rsDiesielInc; // youBy
    TRealSound rscurve; // youBy
    //    std::ofstream PneuLogFile; //zapis parametrow pneumatycznych
    // youBy - dym
    // TSmoke Smog;
    // float EmR;
    // vector3 smokeoffset;

    TDynamicObject * ABuScanNearestObject(TTrack *Track, double ScanDir, double ScanDist,
                                                    int &CouplNr);
    TDynamicObject * GetFirstDynamic(int cpl_type, int cf = 1);
    // TDynamicObject* GetFirstCabDynamic(int cpl_type);
    void ABuSetModelShake(vector3 mShake);

    // McZapkie-010302
    TController *Mechanik;
    TController *ctOwner; // wskażnik na obiekt zarządzający składem
    bool MechInside;
    // McZapkie-270202
    bool Controller;
    bool bDisplayCab; // czy wyswietlac kabine w train.cpp
    int iCabs; // maski bitowe modeli kabin
    TTrack *MyTrack; // McZapkie-030303: tor na ktorym stoi, ABu
    std::string asBaseDir;
    texture_manager::size_type ReplacableSkinID[5]; // McZapkie:zmienialne nadwozie
    int iAlpha; // maska przezroczystości tekstur
    int iMultiTex; //<0 tekstury wskazane wpisem, >0 tekstury z przecinkami, =0 jedna
    int iOverheadMask; // maska przydzielana przez AI pojazdom posiadającym pantograf, aby wymuszały
    // jazdę bezprądową
    TTractionParam tmpTraction;
    double fAdjustment; // korekcja - docelowo przenieść do TrkFoll.cpp wraz z odległością od
    // poprzedniego
    TDynamicObject();
    ~TDynamicObject();
    double TDynamicObject::Init( // zwraca długość pojazdu albo 0, jeśli błąd
        std::string Name, std::string BaseDir, std::string asReplacableSkin, std::string Type_Name,
        TTrack *Track, double fDist, std::string DriverType, double fVel, std::string TrainName,
        float Load, std::string LoadType, bool Reversed, std::string);
    void AttachPrev(TDynamicObject *Object, int iType = 1);
    bool UpdateForce(double dt, double dt1, bool FullVer);
    void LoadUpdate();
    bool Update(double dt, double dt1);
    bool FastUpdate(double dt);
    void Move(double fDistance);
    void FastMove(double fDistance);
    void Render();
    void RenderAlpha();
    void RenderSounds();
    inline vector3 GetPosition() const
    {
        return vPosition;
    };
    inline vector3 HeadPosition()
    {
        return vCoulpler[iDirection ^ 1];
    }; // pobranie współrzędnych czoła
    inline vector3 RearPosition()
    {
        return vCoulpler[iDirection];
    }; // pobranie współrzędnych tyłu
    inline vector3 AxlePositionGet()
    {
        return iAxleFirst ? Axle1.pPosition : Axle0.pPosition;
    };
    inline vector3 VectorFront() const
    {
        return vFront;
    };
    inline vector3 VectorUp()
    {
        return vUp;
    };
    inline vector3 VectorLeft() const
    {
        return vLeft;
    };
    inline double * Matrix()
    {
        return mMatrix.getArray();
    };
    inline double GetVelocity()
    {
        return MoverParameters->Vel;
    };
    inline double GetLength() const
    {
        return MoverParameters->Dim.L;
    };
    inline double GetWidth() const
    {
        return MoverParameters->Dim.W;
    };
    inline TTrack * GetTrack()
    {
        return (iAxleFirst ? Axle1.GetTrack() : Axle0.GetTrack());
    };
    // void UpdatePos();

    // McZapkie-260202
    void LoadMMediaFile(std::string BaseDir, std::string TypeName, std::string ReplacableSkin);

    inline double ABuGetDirection() // ABu.
    {
        return (Axle1.GetTrack() == MyTrack ? Axle1.GetDirection() : Axle0.GetDirection());
    };
    // inline double ABuGetTranslation() //ABu.
    // {//zwraca przesunięcie wózka względem Point1 toru
    //  return (Axle1.GetTrack()==MyTrack?Axle1.GetTranslation():Axle0.GetTranslation());
    // };
    inline double RaDirectionGet()
    { // zwraca kierunek pojazdu na torze z aktywną osą
        return iAxleFirst ? Axle1.GetDirection() : Axle0.GetDirection();
    };
    inline double RaTranslationGet()
    { // zwraca przesunięcie wózka względem Point1 toru z aktywną osią
        return iAxleFirst ? Axle1.GetTranslation() : Axle0.GetTranslation();
    };
    inline TTrack * RaTrackGet()
    { // zwraca tor z aktywną osią
        return iAxleFirst ? Axle1.GetTrack() : Axle0.GetTrack();
    };
    void CouplersDettach(double MinDist, int MyScanDir);
    void RadioStop();
	void Damage(char flag);
	void RaLightsSet(int head, int rear);
    // void RaAxleEvent(TEvent *e);
    TDynamicObject * FirstFind(int &coupler_nr, int cf = 1);
    float GetEPP(); // wyliczanie sredniego cisnienia w PG
    int DirectionSet(int d); // ustawienie kierunku w składzie
    int DirectionGet()
    {
        return iDirection + iDirection - 1;
    }; // odczyt kierunku w składzie
    int DettachStatus(int dir);
    int Dettach(int dir);
    TDynamicObject * Neightbour(int &dir);
    void CoupleDist();
    TDynamicObject * ControlledFind();
    void ParamSet(int what, int into);
    int RouteWish(TTrack *tr); // zapytanie do AI, po którym segmencie skrzyżowania
    // jechać
    void DestinationSet(std::string to, std::string numer);
    std::string TextureTest(std::string const &name);
    void OverheadTrack(float o);
    double MED[9][8]; // lista zmiennych do debugowania hamulca ED
};

//---------------------------------------------------------------------------
