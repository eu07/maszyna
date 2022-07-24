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

#include "Classes.h"
#include "material.h"
#include "MOVER.h"
#include "TrkFoll.h"
#include "Button.h"
#include "AirCoupler.h"
#include "Texture.h"
#include "sound.h"
#include "Spring.h"

#define EU07_SOUND_BOGIESOUNDS

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int const ANIM_WHEELS = 0; // koła
int const ANIM_DOORS = 1; // drzwi
int const ANIM_LEVERS = 2; // elementy obracane (wycieraczki, koła skrętne, przestawiacze, klocki ham.)
int const ANIM_BUFFERS = 3; // elementy przesuwane (zderzaki)
int const ANIM_BOOGIES = 4; // wózki (są skręcane w dwóch osiach)
int const ANIM_PANTS = 5; // pantografy
int const ANIM_STEAMS = 6; // napęd parowozu
int const ANIM_DOORSTEPS = 7;
int const ANIM_MIRRORS = 8;
int const ANIM_TYPES = 9; // Ra: ilość typów animacji

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
    Math3D::vector3 vPos; // Ra: współrzędne punktu zerowego pantografu (X dodatnie dla przedniego)
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
// constructor
    TAnim() = default;
// destructor
    ~TAnim();
// methods
    int TypeSet( int i, int fl = 0 ); // ustawienie typu
// members
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
        int dWheelAngle; // wskaźnik na kąt obrotu osi
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
    int iFlags{ 0 }; // flagi animacji
    float fMaxDist; // do jakiej odległości wykonywana jest animacja NOTE: square of actual distance
    float fSpeed; // parametr szybkości animacji
    int iNumber; // numer kolejny obiektu

    TUpdate yUpdate; // metoda TDynamicObject aktualizująca animację
/*
    void Parovoz(); // wykonanie obliczeń animacji
*/
};

//---------------------------------------------------------------------------

enum class announcement_t : int {
    idle = 0,
    approaching,
    current,
    next,
    destination,
    chime,
    end
};

// parameters for the material object, as currently used by various simulator models
struct material_data {

    int textures_alpha{ 0x30300030 }; // maska przezroczystości tekstur. default: tekstury wymienne nie mają przezroczystości
    material_handle replacable_skins[ 5 ] = { null_handle, null_handle, null_handle, null_handle, null_handle }; // McZapkie:zmienialne nadwozie
    int multi_textures{ 0 }; //<0 tekstury wskazane wpisem, >0 tekstury z przecinkami, =0 jedna

    // assigns specified texture or a group of textures to replacable texture slots
    void assign( std::string const &Replacableskin );
};

class TDynamicObject { // klasa pojazdu

    friend opengl_renderer;
    friend opengl33_renderer;

public:
    static bool bDynamicRemove; // moved from ground

private: // położenie pojazdu w świecie oraz parametry ruchu
    Math3D::vector3 vPosition; // Ra: pozycja pojazdu liczona zaraz po przesunięciu
    Math3D::vector3 vCoulpler[ 2 ]; // współrzędne sprzęgów do liczenia zderzeń czołowych
    Math3D::vector3 vUp, vFront, vLeft; // wektory jednostkowe ustawienia pojazdu
    int iDirection; // kierunek pojazdu względem czoła składu (1=zgodny,0=przeciwny)
    TTrackShape ts; // parametry toru przekazywane do fizyki
    TTrackParam tp; // parametry toru przekazywane do fizyki
    TTrackFollower Axle0; // oś z przodu (od sprzęgu 0)
    TTrackFollower Axle1; // oś z tyłu (od sprzęgu 1)
    int iAxleFirst; // numer pierwszej osi w kierunku ruchu (oś wiążąca pojazd z torem i wyzwalająca eventy)
    float fAxleDist; // rozstaw wózków albo osi do liczenia proporcji zacienienia
    Math3D::vector3 modelRot; // obrot pudła względem świata - do przeanalizowania, czy potrzebne!!!
    TDynamicObject * ABuFindNearestObject(glm::vec3 pos, TTrack *Track, TDynamicObject *MyPointer, int &CouplNr );
    
    glm::dvec3 m_future_movement;
    glm::dvec3 m_future_wheels_angle;

public:
    // parametry położenia pojazdu dostępne publicznie
    std::string asTrack; // nazwa toru początkowego; wywalić?
    std::string asDestination; // dokąd pojazd ma być kierowany "(stacja):(tor)"
	Math3D::matrix4x4 mMatrix; // macierz przekształcenia do renderowania modeli
    TMoverParameters *MoverParameters; // parametry fizyki ruchu oraz przeliczanie
    inline TDynamicObject *NextConnected() { return MoverParameters->Neighbours[ end::rear ].vehicle; }; // pojazd podłączony od strony sprzęgu 1 (kabina -1)
    inline TDynamicObject *PrevConnected() { return MoverParameters->Neighbours[ end::front ].vehicle; }; // pojazd podłączony od strony sprzęgu 0 (kabina 1)
    inline TDynamicObject *NextConnected() const { return MoverParameters->Neighbours[ end::rear ].vehicle; }; // pojazd podłączony od strony sprzęgu 1 (kabina -1)
    inline TDynamicObject *PrevConnected() const { return MoverParameters->Neighbours[ end::front ].vehicle; }; // pojazd podłączony od strony sprzęgu 0 (kabina 1)
    inline int NextConnectedNo() const { return MoverParameters->Neighbours[ end::rear ].vehicle_end; }
    inline int PrevConnectedNo() const { return MoverParameters->Neighbours[ end::front ].vehicle_end; }
//    double fTrackBlock; // odległość do przeszkody do dalszego ruchu (wykrywanie kolizji z innym pojazdem)

    // modele składowe pojazdu
    TModel3d *mdModel; // model pudła
    TModel3d *mdLoad; // model zmiennego ładunku
    TModel3d *mdKabina; // model kabiny dla użytkownika; McZapkie-030303: to z train.h
    TModel3d *mdLowPolyInt; // ABu 010305: wnetrze lowpoly
    std::array<TSubModel *, 3> LowPolyIntCabs {}; // pointers to low fidelity version of individual driver cabs
    bool JointCabs{ false }; // flag for vehicles with multiple virtual 'cabs' sharing location and 3d model(s)
    std::vector<TModel3d *> mdAttachments; // additional models attached to main body
    struct destination_data {
        TSubModel *sign { nullptr }; // submodel mapped with replacable texture -4
        bool has_light { false }; // the submodel was originally configured with self-illumination attribute
        material_handle destination { null_handle }; // most recently assigned non-blank destination texture
        material_handle destination_off { null_handle }; // blank destination sign
        std::string background; // potential default background texture override
        std::string script; // potential python script used to generate texture data
        int update_rate { 0 }; // -1: per stop, 0: none, >0: fps // TBD, TODO: implement?
        std::string instancing; // potential method to generate more than one texture per timetable
        std::string parameters; // potential extra parameters supplied by mmd file
    // methods
        void deserialize( cParser &Input );
    private:
        bool deserialize_mapping( cParser &Input );
    } DestinationSign;
    float fShade; // zacienienie: 0:normalnie, -1:w ciemności, +1:dodatkowe światło (brak koloru?)
    float LoadOffset { 0.f };
    std::unordered_map<std::string, std::string> LoadModelOverrides; // potential overrides of default load visualization models
    glm::vec3 InteriorLight { 0.9f * 255.f / 255.f, 0.9f * 216.f / 255.f, 0.9f * 176.f / 255.f }; // tungsten light. TODO: allow definition of light type?
    float InteriorLightLevel { 0.0f }; // current level of interior lighting
    struct vehicle_section {
        TSubModel *compartment;
        TSubModel *load;
        int load_chunks_visible;
        float light_level;
    };
    std::vector<vehicle_section> Sections; // table of recognized vehicle sections
    bool SectionLightsActive { false }; // flag indicating whether section lights were set.
    std::vector<vehicle_section *> SectionLoadOrder; // helper, activation/deactivation load chunk sequence

private:
    // zmienne i metody do animacji submodeli; Ra: sprzatam animacje w pojeździe
    material_data m_materialdata;

public:
    inline
    material_data const
        *Material() const {
            return &m_materialdata; }
    // tymczasowo udostępnione do wyszukiwania drutu
    std::array<int, ANIM_TYPES> iAnimType{ 0 }; // 0-osie,1-drzwi,2-obracane,3-zderzaki,4-wózki,5-pantografy,6-tłoki
private:
    int iAnimations; // liczba obiektów animujących
    std::vector<TAnim> pAnimations;
    TSubModel ** pAnimated; // lista animowanych submodeli (może być ich więcej niż obiektów animujących)
    double dWheelAngle[3]; // kąty obrotu kół: 0=przednie toczne, 1=napędzające i wiązary, 2=tylne toczne
/*
    void UpdateNone(TAnim *pAnim){}; // animacja pusta (funkcje ustawiania submodeli, gdy blisko kamery)
*/
    void UpdateAxle(TAnim *pAnim); // animacja osi
    void UpdateDoorTranslate(TAnim *pAnim); // animacja drzwi - przesuw
    void UpdateDoorRotate(TAnim *pAnim); // animacja drzwi - obrót
    void UpdateDoorFold(TAnim *pAnim); // animacja drzwi - składanie
	void UpdateDoorPlug(TAnim *pAnim);      // animacja drzwi - odskokowo-przesuwne
	void UpdatePant(TAnim *pAnim); // animacja pantografu
    void UpdatePlatformTranslate(TAnim *pAnim); // doorstep animation, shift
    void UpdatePlatformRotate(TAnim *pAnim); // doorstep animation, rotate
    void UpdateMirror(TAnim *pAnim); // mirror animation
/*
    void UpdateLeverDouble(TAnim *pAnim); // animacja gałki zależna od double
    void UpdateLeverFloat(TAnim *pAnim); // animacja gałki zależna od float
    void UpdateLeverInt(TAnim *pAnim); // animacja gałki zależna od int (wartość)
    void UpdateLeverEnum(TAnim *pAnim); // animacja gałki zależna od int (lista kątów)
*/
    void toggle_lights(); // switch light levels for registered interior sections
  private: // Ra: ciąg dalszy animacji, dopiero do ogarnięcia
    // ABuWozki 060504
    Math3D::vector3 bogieRot[2]; // Obroty wozkow w/m korpusu
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
    Math3D::vector3 vFloor; // podłoga dla ładunku
  public:
    TAnim *pants; // indeks obiektu animującego dla pantografu 0
    double NoVoltTime; // czas od utraty zasilania
    double dMirrorMoveL{ 0.0 };
    double dMirrorMoveR{ 0.0 };
    TSubModel *smBrakeSet; // nastawa hamulca (wajcha)
    TSubModel *smLoadSet; // nastawa ładunku (wajcha)
    TSubModel *smWiper; // wycieraczka (poniekąd też wajcha)
    // Ra: koneic animacji do ogarnięcia

private:
// types
    struct exchange_data {
        float unload_count { 0.f }; // amount to unload
        float load_count { 0.f }; // amount to load
        int platforms { 0 }; // platforms which may take part in the exchange
        float time { 0.f }; // time spent on the operation
    };

    struct coupleradapter_data {
        glm::vec2 position; // adapter placement; offset from vehicle end and height
        std::string model; // 3d model of the adapter
    };

    struct coupler_sounds {
        // TBD: change to an array with index-based access for easier initialization?
        sound_source attach_coupler { sound_placement::external };
        sound_source attach_brakehose { sound_placement::external };
        sound_source attach_mainhose { sound_placement::external };
        sound_source attach_control { sound_placement::external };
        sound_source attach_gangway { sound_placement::external };
        sound_source attach_heating { sound_placement::external };
        sound_source detach_coupler { sound_placement::external };
        sound_source detach_brakehose { sound_placement::external };
        sound_source detach_mainhose { sound_placement::external };
        sound_source detach_control { sound_placement::external };
        sound_source detach_gangway { sound_placement::external };
        sound_source detach_heating { sound_placement::external };
        sound_source dsbCouplerStretch { sound_placement::external }; // moved from cab
        sound_source dsbCouplerStretch_loud { sound_placement::external };
        sound_source dsbBufferClamp { sound_placement::external }; // moved from cab
        sound_source dsbBufferClamp_loud { sound_placement::external };
        sound_source dsbAdapterAttach { sound_placement::external };
        sound_source dsbAdapterRemove { sound_placement::external };
    };

    struct pantograph_sounds {
        // TODO: split pantograph sound into one for contact of arm with the wire, and electric arc sound
        sound_source sPantUp { sound_placement::external };
        sound_source sPantDown { sound_placement::external };
    };

    struct door_sounds {
        sound_source rsDoorOpen { sound_placement::general }; // Ra: przeniesione z kabiny
        sound_source rsDoorClose { sound_placement::general };
        sound_source lock { sound_placement::general };
        sound_source unlock { sound_placement::general };
        sound_source step_open { sound_placement::general };
        sound_source step_close { sound_placement::general };
        sound_source permit_granted { sound_placement::general };
        side placement {};
    };

    struct exchange_sounds {
        sound_source loading { sound_placement::general };
        sound_source unloading { sound_placement::general };
    };

    struct axle_sounds {
        double distance; // distance to rail joint
        double offset; // axle offset from centre of the vehicle
        sound_source clatter; // clatter emitter
    };

    struct powertrain_sounds {
        sound_source inverter { sound_placement::engine };
        std::vector<sound_source> motorblowers;
        std::vector<sound_source> motors; // generic traction motor sounds
        std::vector<sound_source> acmotors; // inverter-specific traction motor sounds
//        bool dcmotors { true }; // traction dc motor(s)
        double motor_volume { 0.0 }; // MC: pomocnicze zeby gladziej silnik buczal
        float motor_momentum { 0.f }; // recent change in motor revolutions
        sound_source motor_relay { sound_placement::engine };
        sound_source dsbWejscie_na_bezoporow { sound_placement::engine }; // moved from cab
        sound_source motor_parallel { sound_placement::engine }; // moved from cab
        sound_source motor_shuntfield { sound_placement::engine };
        sound_source linebreaker_close { sound_placement::engine };
        sound_source linebreaker_open { sound_placement::engine };
        sound_source rsWentylator { sound_placement::engine }; // McZapkie-030302
        sound_source engine { sound_placement::engine }; // generally diesel engine
		sound_source fake_engine { sound_placement::engine };
        sound_source engine_ignition { sound_placement::engine }; // moved from cab
        sound_source engine_shutdown { sound_placement::engine };
        bool engine_state_last { false }; // helper, cached previous state of the engine
        double engine_volume { 0.0 }; // MC: pomocnicze zeby gladziej silnik buczal
        sound_source engine_revving { sound_placement::engine }; // youBy
        float engine_revs_last { -1.f }; // helper, cached rpm of the engine
        float engine_revs_change { 0.f }; // recent change in engine revolutions
        sound_source engine_turbo { sound_placement::engine };
        double engine_turbo_pitch { 1.0 };
        sound_source oil_pump { sound_placement::engine };
        sound_source fuel_pump { sound_placement::engine };
        sound_source water_pump { sound_placement::engine };
        sound_source water_heater { sound_placement::engine };
        sound_source radiator_fan { sound_placement::engine };
        sound_source radiator_fan_aux { sound_placement::engine };
        sound_source transmission { sound_placement::engine };
        sound_source rsEngageSlippery { sound_placement::engine }; // moved from cab
		sound_source retarder { sound_placement::engine };

        void position( glm::vec3 const Location );
		void render( TMoverParameters const &Vehicle, double const Deltatime );
    };
    // single source per door (pair) on the centreline
    struct doorspeaker_sounds {
        glm::vec3 offset;
        sound_source departure_signal;
    };
    // single source per vehicle
    struct pasystem_sounds {
        std::array<sound_source, static_cast<int>( announcement_t::end )> announcements;
        std::optional< std::array<float, 6> > soundproofing; 
        sound_source announcement;
        std::deque<sound_source> announcement_queue; // fifo queue
    };
    struct springbrake_sounds {
        sound_source activate { sound_placement::external };
        sound_source release { sound_placement::external };
        bool state { false };
    };


// methods
    void ABuLittleUpdate(double ObjSqrDist);
    void ABuBogies();
    void ABuModelRoll();
    void TurnOff();
    // update state of load exchange operation
    void update_exchange( double const Deltatime );

// members
    AirCoupler btCoupler1; // sprzegi
    AirCoupler btCoupler2;
    std::array<TModel3d *, 2> m_coupleradapters = { nullptr, nullptr };
    AirCoupler btCPneumatic1; // sprzegi powietrzne //yB - zmienione z Button na AirCoupler - krzyzyki
    AirCoupler btCPneumatic2;
    AirCoupler btCPneumatic1r; // ABu: to zeby nie bylo problemow przy laczeniu wagonow,
    AirCoupler btCPneumatic2r; //     jesli beda polaczone sprzegami 1<->1 lub 0<->0
    AirCoupler btPneumatic1; // ABu: sprzegi powietrzne zolte
    AirCoupler btPneumatic2;
    AirCoupler btPneumatic1r; // ABu: analogicznie jak 4 linijki wyzej
    AirCoupler btPneumatic2r;

    TButton btCCtrl1; // sprzegi sterowania
    TButton btCCtrl2;
    TButton btCPass1; // mostki przejsciowe
    TButton btCPass2;
    char cp1, sp1, cp2, sp2; // ustawienia węży

    TButton m_endsignal12; // sygnalu konca pociagu
    TButton m_endsignal13;
    TButton m_endsignal22;
    TButton m_endsignal23;
    TButton m_endsignals1; // zeby bylo kompatybilne ze starymi modelami...
    TButton m_endsignals2;
    TButton m_endtab1; // sygnaly konca pociagu (blachy)
    TButton m_endtab2;
    AirCoupler m_headlamp11; // oswietlenie czolowe - przod
    AirCoupler m_headlamp12;
    AirCoupler m_headlamp13;
    AirCoupler m_headlamp21; // oswietlenie czolowe - tyl
    AirCoupler m_headlamp22;
    AirCoupler m_headlamp23;
    AirCoupler m_headsignal12;
    AirCoupler m_headsignal13;
    AirCoupler m_headsignal22;
    AirCoupler m_headsignal23;
    TButton btMechanik1;
	TButton btMechanik2;
    TButton btShutters1; // cooling shutters for primary water circuit
    TButton btShutters2; // cooling shutters for auxiliary water circuit

    double dRailLength { 0.0 };
    std::vector<axle_sounds> m_axlesounds;
    // engine sounds
    powertrain_sounds m_powertrainsounds;
    sound_source sConverter { sound_placement::engine };
    sound_source sCompressor { sound_placement::engine }; // NBMX wrzesien 2003
    sound_source sCompressorIdle { sound_placement::engine };
    sound_source sSmallCompressor { sound_placement::engine };
    sound_source sHeater { sound_placement::engine };
    sound_source m_batterysound { sound_placement::engine };
    // braking sounds
    sound_source dsbPneumaticRelay { sound_placement::external };
    sound_source rsBrake { sound_placement::external, EU07_SOUND_BRAKINGCUTOFFRANGE }; // moved from cab
    sound_source sBrakeAcc { sound_placement::external };
    bool bBrakeAcc { false };
    sound_source rsPisk { sound_placement::external, EU07_SOUND_BRAKINGCUTOFFRANGE }; // McZapkie-260302
    sound_source rsUnbrake { sound_placement::external }; // yB - odglos luzowania
    sound_source m_brakecylinderpistonadvance { sound_placement::external };
    sound_source m_brakecylinderpistonrecede { sound_placement::external };
    float m_lastbrakepressure { -1.f }; // helper, cached level of pressure in the brake cylinder
    float m_brakepressurechange { 0.f }; // recent change of pressure in the brake cylinder
	sound_source m_epbrakepressureincrease{ sound_placement::external };
	sound_source m_epbrakepressuredecrease{ sound_placement::external };
	float m_lastepbrakepressure{ -1.f }; // helper, cached level of pressure in the brake cylinder
	float m_epbrakepressurechange{ 0.f }; // recent change of pressure in the brake cylinder
	float m_epbrakepressurechangeinctimer{ 0.f }; // last time of change of pressure in the brake cylinder - increase
	float m_epbrakepressurechangedectimer{ 0.f }; // last time of change of pressure in the brake cylinder - decrease
    sound_source m_emergencybrake { sound_placement::engine };
    double m_emergencybrakeflow{ 0.f };
    sound_source sReleaser { sound_placement::external };
    springbrake_sounds m_springbrakesounds;
    sound_source rsSlippery { sound_placement::external, EU07_SOUND_BRAKINGCUTOFFRANGE }; // moved from cab
    sound_source sSand { sound_placement::external };
    // moving part and other external sounds
    sound_source m_startjolt { sound_placement::general }; // movement start jolt, played once on initial acceleration at slow enough speed
    bool m_startjoltplayed { false };
    std::array<coupler_sounds, 2> m_couplersounds; // always front and rear
    std::vector<pantograph_sounds> m_pantographsounds; // typically 2 but can be less (or more?)
    std::vector<door_sounds> m_doorsounds; // can expect symmetrical arrangement, but don't count on it
    bool m_doorlocks { false }; // sound helper, current state of door locks
    sound_source sHorn1 { sound_placement::external, 5 * EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
    sound_source sHorn2 { sound_placement::external, 5 * EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
    sound_source sHorn3 { sound_placement::external, 5 * EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
#ifdef EU07_SOUND_BOGIESOUNDS
    std::vector<sound_source> m_bogiesounds; // TBD, TODO: wrapper for all bogie-related sounds (noise, brakes, squeal etc)
#else
    sound_source m_outernoise { sound_placement::external, EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
#endif
    sound_source m_wheelflat { sound_placement::external, EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
    sound_source rscurve { sound_placement::external, EU07_SOUND_RUNNINGNOISECUTOFFRANGE }; // youBy
    sound_source rsDerailment { sound_placement::external, 2 * EU07_SOUND_RUNNINGNOISECUTOFFRANGE }; // McZapkie-051202

    exchange_data m_exchange; // state of active load exchange procedure, if any
    exchange_sounds m_exchangesounds; // sounds associated with the load exchange

    std::vector<doorspeaker_sounds> m_doorspeakers;
    pasystem_sounds m_pasystem;

    std::array<
        std::array<float, 6> // listener: rear cab, engine, front cab, window, attached camera, free camera
        , 5> m_soundproofing = {{
            {{ EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_SOME, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_STRONG }}, // internal sounds
            {{ EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_SOME, EU07_SOUNDPROOFING_SOME, EU07_SOUNDPROOFING_SOME }}, // engine sounds
            {{ EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_SOME, EU07_SOUNDPROOFING_SOME, EU07_SOUNDPROOFING_NONE }}, // external sound
            {{ EU07_SOUNDPROOFING_VERYSTRONG, EU07_SOUNDPROOFING_VERYSTRONG, EU07_SOUNDPROOFING_VERYSTRONG, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_STRONG, EU07_SOUNDPROOFING_NONE }}, // external ambient sound
            {{ EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_NONE, EU07_SOUNDPROOFING_NONE }}, // custom sounds
        }};

    coupleradapter_data m_coupleradapter;

    bool renderme; // yB - czy renderowac
    float ModCamRot;
    int iInventory[ 2 ] { 0, 0 }; // flagi bitowe posiadanych submodeli (np. świateł)
	bool btnOn; // ABu: czy byly uzywane buttony, jesli tak, to po renderingu wylacz
                // bo ten sam model moze byc jeszcze wykorzystany przez inny obiekt!

  public:
    int iHornWarning; // numer syreny do użycia po otrzymaniu sygnału do jazdy
    bool bEnabled; // Ra: wyjechał na portal i ma być usunięty
  protected:
    int iNumAxles; // ilość osi
    std::string asModel;

  private:
    TDynamicObject *ABuFindObject( int &Foundcoupler, double &Distance, TTrack const *Track, int const Direction, int const Mycoupler ) const;
    void ABuCheckMyTrack();

  public:
    bool DimHeadlights{ false }; // status of the headlight dimming toggle. NOTE: single toggle for all lights is a simplification. TODO: separate per-light switches
    // checks whether there's unbroken connection of specified type to specified vehicle
    bool is_connected( TDynamicObject const *Vehicle, coupling const Coupling = coupling::coupler ) const;
	TDynamicObject * PrevAny() const;
	TDynamicObject * Prev(int C = -1) const;
	TDynamicObject * Next(int C = -1) const;
    void SetdMoveLen(double dMoveLen) {
        MoverParameters->dMoveLen = dMoveLen; }
    void ResetdMoveLen() {
        MoverParameters->dMoveLen = 0; }
    double GetdMoveLen() {
        return MoverParameters->dMoveLen; }

    int GetPneumatic(bool front, bool red);
    void SetPneumatic(bool front, bool red);
    std::string asName;
	const std::string &name() const {
		return asName; }
    std::string asBaseDir;

    //    std::ofstream PneuLogFile; //zapis parametrow pneumatycznych
    // youBy - dym
    // TSmoke Smog;
    // float EmR;
    // vector3 smokeoffset;

	TDynamicObject * ABuScanNearestObject(glm::vec3 pos, TTrack *Track, double ScanDir, double ScanDist,
	                                                int &CouplNr);
    TDynamicObject * GetFirstDynamic(int cpl_type, int cf = 1);
    void ABuSetModelShake( Math3D::vector3 mShake);

    // McZapkie-010302
    TController *Mechanik;
    TController *ctOwner; // wskażnik na obiekt zarządzający składem
    bool MechInside;
    // McZapkie-270202
    bool Controller;
    bool bDisplayCab; // czy wyswietlac kabine w train.cpp
    int iCabs; // maski bitowe modeli kabin
    TTrack *MyTrack; // McZapkie-030303: tor na ktorym stoi, ABu
    int iOverheadMask; // maska przydzielana przez AI pojazdom posiadającym pantograf, aby wymuszały jazdę bezprądową
    TTractionParam tmpTraction;
    double fAdjustment; // korekcja - docelowo przenieść do TrkFoll.cpp wraz z odległością od poprzedniego

	TTrack *initial_track = nullptr;

    TDynamicObject();
    ~TDynamicObject();
	void place_on_track(TTrack *Track, double fDist, bool Reversed);
    // zwraca długość pojazdu albo 0, jeśli błąd
    double Init(
        std::string Name, std::string BaseDir, std::string asReplacableSkin, std::string Type_Name,
        TTrack *Track, double fDist, std::string DriverType, double fVel, std::string TrainName,
        float Load, std::string LoadType, bool Reversed, std::string);
    int init_sections( TModel3d const *Model, std::string const &Nameprefix, bool const Overrideselfillum );
    bool init_destination( TModel3d *Model );
    void create_controller( std::string const Type, bool const Trainset );
    void AttachNext(TDynamicObject *Object, int iType = 1);
    bool UpdateForce(double dt);
    // initiates load change by specified amounts, with a platform on specified side
    void LoadExchange( int const Disembark, int const Embark, int const Platforms );
    // calculates time needed to complete current load change, using specified platforms
    float LoadExchangeTime( int const Platforms );
    // calculates time needed to complete current load change, using previously specified platforms
    float LoadExchangeTime() const;
    // calculates current load exchange factor, where 1 = nominal rate, higher = faster
    float LoadExchangeSpeed() const; // TODO: make private when cleaning up
    void LoadUpdate();
    void update_load_sections();
    void update_load_visibility();
    void update_load_offset();
    void shuffle_load_order();
    void update_destinations();
    bool Update(double dt, double dt1);
    bool FastUpdate(double dt);
    void Move(double fDistance);
    void FastMove(double fDistance);
    void RenderSounds();
    inline Math3D::vector3 GetPosition() const {
        return vPosition; };
    // converts location from vehicle coordinates frame to world frame
    inline Math3D::vector3 GetWorldPosition( Math3D::vector3 const &Location ) const {
        return vPosition + mMatrix * Location; }
    // pobranie współrzędnych czoła
    inline Math3D::vector3 HeadPosition() const {
        return vCoulpler[iDirection ^ 1]; };
    // pobranie współrzędnych tyłu
    inline Math3D::vector3 RearPosition() const {
        return vCoulpler[iDirection]; };
    inline Math3D::vector3 CouplerPosition( end const End ) const {
        return vCoulpler[ End ]; }
    inline Math3D::vector3 AxlePositionGet() {
        return iAxleFirst ?
            Axle1.pPosition :
            Axle0.pPosition; };
    inline double Roll() {
        return ( ( Axle1.GetRoll() + Axle0.GetRoll() ) ); }
/*
    // TODO: check if scanning takes into account direction when selecting axle
    // if it does, replace the version above
    // if it doesn't, fix it so it does
    inline Math3D::vector3 AxlePositionGet() {
        return (
            iDirection ?
                ( iAxleFirst ? Axle1.pPosition : Axle0.pPosition ) :
                ( iAxleFirst ? Axle0.pPosition : Axle1.pPosition ) ); }
*/
    inline Math3D::vector3 VectorFront() const {
        return vFront; };
    inline Math3D::vector3 VectorUp() const {
        return vUp; };
    inline Math3D::vector3 VectorLeft() const {
        return vLeft; };
    inline double const * Matrix() const {
        return mMatrix.readArray(); };
    inline double * Matrix() {
        return mMatrix.getArray(); };
    inline double GetVelocity() const {
        return MoverParameters->Vel; };
    inline double GetLength() const {
        return MoverParameters->Dim.L; };
    inline double GetWidth() const {
        return MoverParameters->Dim.W; };
    double radius() const;
    // calculates distance between event-starting axle and front of the vehicle
    double tracing_offset() const;
    inline TTrack * GetTrack() {
        return (iAxleFirst ?
            Axle1.GetTrack() :
            Axle0.GetTrack()); };

    // McZapkie-260202
    void LoadMMediaFile(std::string const &TypeName, std::string const &ReplacableSkin);
    TModel3d *LoadMMediaFile_mdload( std::string const &Name ) const;

    inline double ABuGetDirection() const { // ABu.
        return (Axle1.GetTrack() == MyTrack ? Axle1.GetDirection() : Axle0.GetDirection()); };
    // zwraca kierunek pojazdu na torze z aktywną osą
    inline double RaDirectionGet() const {
        return iAxleFirst ?
            Axle1.GetDirection() :
            Axle0.GetDirection(); };
    // zwraca przesunięcie wózka względem Point1 toru z aktywną osią
    inline double RaTranslationGet() const {
        return iAxleFirst ?
            Axle1.GetTranslation() :
            Axle0.GetTranslation(); };
    // zwraca tor z aktywną osią
    inline TTrack const * RaTrackGet() const {
        return iAxleFirst ?
            Axle1.GetTrack() :
            Axle0.GetTrack(); };

    void couple( int const Side );
    int uncouple( int const Side );
    bool attach_coupler_adapter( int const Side, bool const Enforce = false );
    bool remove_coupler_adapter( int const Side );
    void RadioStop();
	void Damage(char flag);
	void pants_up();
    void SetLights();
    void RaLightsSet(int head, int rear);
    int LightList( end const Side ) const { return iInventory[ Side ]; }
    bool has_signal_pc1_on() const;
    bool has_signal_pc2_on() const;
    bool has_signal_pc5_on() const;
    bool has_signal_on( int const Side, int const Pattern ) const;
    void set_cab_lights( int const Cab, float const Level );
    TDynamicObject * FirstFind(int &coupler_nr, int cf = 1);
    float GetEPP(); // wyliczanie sredniego cisnienia w PG
    int DirectionSet(int d); // ustawienie kierunku w składzie
    // odczyt kierunku w składzie; returns 1 if true, -1 otherwise
    int DirectionGet() const {
        return iDirection + iDirection - 1; };
    int DettachStatus(int dir);
    int Dettach(int dir);
    TDynamicObject * Neighbour(int &dir);
    // updates potential collision sources
    void update_neighbours();
    // locates potential collision source within specified range, scanning its route in specified direction
    auto find_vehicle( int const Direction, double const Range ) const -> std::tuple<TDynamicObject *, int, double, bool>;
    // locates potential vehicle connected with specific coupling type and satisfying supplied predicate
    template <typename Predicate_>
    auto find_vehicle( coupling const Coupling, Predicate_ const Predicate ) -> TDynamicObject *;
    TDynamicObject * FindPowered();
    TDynamicObject * FindPantographCarrier();
    template <typename UnaryFunction_>
    void for_each( coupling const Coupling, UnaryFunction_ const Function );
    void ParamSet(int what, int into);
    // zapytanie do AI, po którym segmencie skrzyżowania jechać
    int RouteWish(TTrack *tr);
    void DestinationSet(std::string to, std::string numer);
    material_handle DestinationFind( std::string Destination );
    void OverheadTrack(float o);
    glm::dvec3 get_future_movement() const;
	void move_set(double distance);
    // playes specified announcement, potentially preceding it with a chime
    void announce( announcement_t const Announcement, bool const Chime = true );
    // returns soundproofing for specified sound type and listener location
    float soundproofing( int const Placement, int const Listener ) const {
        return m_soundproofing[ Placement - 1 ][ Listener + 1 ]; }

    double MED[9][8]; // lista zmiennych do debugowania hamulca ED
    static std::string const MED_labels[ 8 ];
	std::ofstream MEDLogFile; // zapis parametrów hamowania
	double MEDLogTime = 0;
	double MEDLogInactiveTime = 0;
	int MEDLogCount = 0;
	double MED_oldFED = 0;

// vehicle shaking calculations
// TBD, TODO: make an object out of it
public:
// methods
    void update_shake( double const Timedelta );
    std::pair<double, double> shake_angles() const;
// members
    struct baseshake_config {
        Math3D::vector3 angle_scale { 0.05, 0.0, 0.1 }; // roll, yaw, pitch
        Math3D::vector3 jolt_scale { 0.2, 0.2, 0.1 };
        double jolt_limit { 0.15 };
    } BaseShake;
    struct engineshake_config {
        float scale { 2.f };
        float fadein_offset { 1.5f }; // 90 rpm
        float fadein_factor { 0.3f };
        float fadeout_offset { 10.f }; // 600 rpm
        float fadeout_factor { 0.5f };
    } EngineShake;
    struct huntingshake_config {
        float scale { 1.f };
        float frequency { 1.f };
        float fadein_begin { 0.f }; // effect start speed in km/h
        float fadein_end { 0.f }; // full effect speed in km/h
    } HuntingShake;
    float HuntingAngle { 0.f }; // crude approximation of hunting oscillation; current angle of sine wave
    bool IsHunting { false };
    TSpring ShakeSpring;
    struct shake_state {
        Math3D::vector3 velocity {}; // current shaking vector
        Math3D::vector3 offset {}; // overall shake-driven offset from base position
    } ShakeState;

	Math3D::vector3 modelShake;
};



class vehicle_table : public basic_table<TDynamicObject> {

public:
    // legacy method, calculates changes in simulation state over specified time
    void
        update( double dt, int iter );
    // legacy method, checks for presence and height of traction wire for specified vehicle
    void
        update_traction( TDynamicObject *Vehicle );
    // legacy method, sends list of vehicles over network
    void
        DynamicList( bool const Onlycontrolled = false ) const;

private:
    // maintenance; removes from tracks consists with vehicles marked as disabled
    bool
        erase_disabled();
};


template <typename Predicate_>
auto
TDynamicObject::find_vehicle( coupling const Coupling, Predicate_ const Predicate ) -> TDynamicObject * {

    if( Predicate( this ) ) {
        return this; }
    // try first to look towards the rear
    auto *vehicle { this };
    while( ( vehicle = vehicle->Next( Coupling ) ) != nullptr ) {
        if( Predicate( vehicle ) ) {
            return vehicle; } }
    // if we didn't yet find a suitable vehicle try in the other direction
    vehicle = this;
    while( ( vehicle = vehicle->Prev( Coupling ) ) != nullptr ) {
        if( Predicate( vehicle ) ) {
            return vehicle; } }
    // if we still don't have a match give up
    return nullptr;
}

template <typename UnaryFunction_>
void
TDynamicObject::for_each( coupling const Coupling, UnaryFunction_ const Function ) {

    Function( this );
    // walk first towards the rear
    auto *vehicle { this };
    while( ( vehicle = vehicle->Next( Coupling ) ) != nullptr ) {
        Function( vehicle ); }
    // then towards the front
    vehicle = this;
    while( ( vehicle = vehicle->Prev( Coupling ) ) != nullptr ) {
        Function( vehicle ); }
}

//---------------------------------------------------------------------------
