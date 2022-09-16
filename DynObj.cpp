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
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "stdafx.h"
#include "DynObj.h"

#include "simulation.h"
#include "lightarray.h"
#include "Camera.h"
#include "Train.h"
#include "Driver.h"
#include "Globals.h"
#include "Timer.h"
#include "Logs.h"
#include "Console.h"
#include "Traction.h"
#include "sound.h"
#include "MdlMngr.h"
#include "Model3d.h"
#include "renderer.h"
#include "uitranscripts.h"
#include "messaging.h"
#include "Driver.h"

// Ra: taki zapis funkcjonuje lepiej, ale może nie jest optymalny
#define vWorldFront Math3D::vector3(0, 0, 1)
#define vWorldUp Math3D::vector3(0, 1, 0)
#define vWorldLeft CrossProduct(vWorldUp, vWorldFront)

#define M_2PI 6.283185307179586476925286766559;
const float maxrot = (float)(M_PI / 3.0); // 60°

std::string const TDynamicObject::MED_labels[] = {
    "masa: ", "amax: ", "Fzad: ", "FmPN: ", "FmED: ", "FrED: ", "FzPN: ", "nPrF: "
};

bool TDynamicObject::bDynamicRemove { false };

// helper, locates submodel with specified name in specified 3d model; returns: pointer to the submodel, or null
TSubModel *
GetSubmodelFromName( TModel3d * const Model, std::string const Name ) {

    return (
        Model ?
            Model->GetFromName( Name ) :
            nullptr );
}

// Ra 2015-01: sprawdzenie dostępności tekstury o podanej nazwie
std::string
TextureTest( std::string const &Name ) {

    auto const lookup {
        FileExists(
            { Global.asCurrentTexturePath + Name, Name, szTexturePath + Name },
            { ".mat", ".dds", ".tga", ".ktx", ".png", ".bmp", ".jpg", ".tex" } ) };

    return ( lookup.first + lookup.second );
}

//---------------------------------------------------------------------------
void TAnimPant::AKP_4E()
{ // ustawienie wymiarów dla pantografu AKP-4E
    vPos = Math3D::vector3(0, 0, 0); // przypisanie domyśnych współczynników do pantografów
    fLenL1 = 1.22; // 1.176289 w modelach
    fLenU1 = 1.755; // 1.724482197 w modelach
    fHoriz = 0.535; // 0.54555075 przesunięcie ślizgu w długości pojazdu względem
    // osi obrotu dolnego
    // ramienia
    fHeight = 0.07; // wysokość ślizgu ponad oś obrotu
    fWidth = 0.635; // połowa szerokości ślizgu, 0.635 dla AKP-1 i AKP-4E
    fAngleL0 = DegToRad(2.8547285515689267247882521833308);
    fAngleL = fAngleL0; // początkowy kąt dolnego ramienia
    // fAngleU0=acos((1.22*cos(fAngleL)+0.535)/1.755); //górne ramię
    fAngleU0 = acos((fLenL1 * cos(fAngleL) + fHoriz) / fLenU1); // górne ramię
    fAngleU = fAngleU0; // początkowy kąt
    // PantWys=1.22*sin(fAngleL)+1.755*sin(fAngleU); //wysokość początkowa
    PantWys = fLenL1 * sin(fAngleL) + fLenU1 * sin(fAngleU) + fHeight; // wysokość początkowa
    PantTraction = PantWys;
    hvPowerWire = NULL;
    fWidthExtra = 0.381f; //(2.032m-1.027)/2
    // poza obszarem roboczym jest aproksymacja łamaną o 5 odcinkach
    fHeightExtra[0] = 0.0f; //+0.0762
    fHeightExtra[1] = -0.01f; //+0.1524
    fHeightExtra[2] = -0.03f; //+0.2286
    fHeightExtra[3] = -0.07f; //+0.3048
    fHeightExtra[4] = -0.15f; //+0.3810
};
//---------------------------------------------------------------------------
int TAnim::TypeSet(int i, int fl)
{ // ustawienie typu animacji i zależnej od niego ilości animowanych submodeli
    fMaxDist = -1.0; // normalnie nie pokazywać
    switch (i)
    { // maska 0x000F: ile używa wskaźników na submodele (0 gdy jeden,
    // wtedy bez tablicy)
    // maska 0x00F0:
    // 0-osie,1-drzwi,2-obracane,3-zderzaki,4-wózki,5-pantografy,6-tłoki
    // maska 0xFF00: ile używa liczb float dla współczynników i stanu
    case 0:
        iFlags = 0x000;
        break; // 0-oś
    case 1:
        iFlags = 0x010;
        break; // 1-drzwi
    case 2:
        iFlags = 0x020;
        fParam = fl ? new float[fl] : NULL;
        iFlags += fl << 8;
        break; // 2-wahacz, dźwignia itp.
    case 3:
        iFlags = 0x030;
        break; // 3-zderzak
    case 4:
        iFlags = 0x040;
        break; // 4-wózek
    case 5: // 5-pantograf - 5 submodeli
        iFlags = 0x055;
        fParamPants = new TAnimPant();
        fParamPants->AKP_4E();
        break;
    case 6:
        iFlags = 0x068;
        break; // 6-tłok i rozrząd - 8 submodeli
    case 7:
        iFlags = 0x070;
        break; // doorstep
    case 8:
        iFlags = 0x080;
        break; // mirror
    default:
        iFlags = 0;
    }
    yUpdate = nullptr;
    return iFlags & 15; // ile wskaźników rezerwować dla danego typu animacji
};

TAnim::~TAnim()
{ // usuwanie animacji
    switch (iFlags & 0xF0)
    { // usuwanie struktur, zależnie ile zostało stworzonych
    case 0x20: // 2-wahacz, dźwignia itp.
        delete fParam;
        break;
    case 0x50: // 5-pantograf
        delete fParamPants;
        break;
    default:
        break;
    }
};
/*
void TAnim::Parovoz(){
    // animowanie tłoka i rozrządu parowozu
};
*/


// assigns specified texture or a group of textures to replacable texture slots
void
material_data::assign( std::string const &Replacableskin ) {

    // check for the pipe method first
    if( contains( Replacableskin,  '|' ) ) {
        cParser nameparser( Replacableskin );
        nameparser.getTokens( 4, true, "|" );
        int skinindex = 0;
        std::string texturename; nameparser >> texturename;
        while( ( texturename != "" ) && ( skinindex < 4 ) ) {
            replacable_skins[ skinindex + 1 ] = GfxRenderer->Fetch_Material( texturename );
            ++skinindex;
            texturename = ""; nameparser >> texturename;
        }
        multi_textures = skinindex;
    }
    else {
        // otherwise try the basic approach
        int skinindex = 0;
        do {
            // test quietly for file existence so we don't generate tons of false errors in the log
            // NOTE: this means actual missing files won't get reported which is hardly ideal, but still somewhat better
            auto const material { TextureTest( ToLower( Replacableskin + "," + std::to_string( skinindex + 1 ) ) ) };
            if( true == material.empty() ) { break; }

            replacable_skins[ skinindex + 1 ] = GfxRenderer->Fetch_Material( material );
            ++skinindex;
        } while( skinindex < 4 );
        multi_textures = skinindex;
        if( multi_textures == 0 ) {
            // zestaw nie zadziałał, próbujemy normanie
            replacable_skins[ 1 ] = GfxRenderer->Fetch_Material( Replacableskin );
        }
    }
    if( replacable_skins[ 1 ] == null_handle ) {
        // last ditch attempt, check for single replacable skin texture
        replacable_skins[ 1 ] = GfxRenderer->Fetch_Material( Replacableskin );
    }

    // BUGS! it's not entierly designed whether opacity is property of material or submodel,
    // and code does confusing things with this in various places
    textures_alpha = (
        GfxRenderer->Material( replacable_skins[ 1 ] ).is_translucent() ?
            0x31310031 :  // tekstura -1 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
            0x30300030 ); // wszystkie tekstury nieprzezroczyste - nie renderować w cyklu przezroczystych
    if( GfxRenderer->Material( replacable_skins[ 2 ] ).is_translucent() ) {
        // tekstura -2 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
        textures_alpha |= 0x02020002;
    }
    if( GfxRenderer->Material( replacable_skins[ 3 ] ).is_translucent() ) {
        // tekstura -3 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
        textures_alpha |= 0x04040004;
    }
    if( GfxRenderer->Material( replacable_skins[ 4 ] ).is_translucent() ) {
        // tekstura -4 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
        textures_alpha |= 0x08080008;
    }
}


void TDynamicObject::destination_data::deserialize( cParser &Input ) {

    while( true == deserialize_mapping( Input ) ) {
        ; // all work done by while()
    }
}

bool TDynamicObject::destination_data::deserialize_mapping( cParser &Input ) {
    // token can be a key or block end
    auto const key { Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) };

    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    if( key == "{" ) {
        script = Input.getToken<std::string>();
    }
    else if( key == "update:" ) {
        auto const value { Input.getToken<std::string>() };
        // TODO: implement
    }
    else if( key == "instance:" ) {
        instancing = Input.getToken<std::string>();
    }
    else if( key == "parameters:" ) {
        parameters = Input.getToken<std::string>();
    }
    else if( key == "background:" ) {
        background = Input.getToken<std::string>();
    }

    return true;
}


//---------------------------------------------------------------------------
TDynamicObject * TDynamicObject::FirstFind(int &coupler_nr, int cf)
{ // szukanie skrajnego połączonego pojazdu w pociagu
    // od strony sprzegu (coupler_nr) obiektu (start)
    TDynamicObject *temp = this;
    for (int i = 0; i < 300; i++) // ograniczenie do 300 na wypadek zapętlenia składu
    {
        if (!temp)
            return NULL; // Ra: zabezpieczenie przed ewentaulnymi błędami sprzęgów
        if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag & cf) != cf)
            return temp; // nic nie ma już dalej podłączone sprzęgiem cf
        if (coupler_nr == end::front)
        { // jeżeli szukamy od sprzęgu 0
            if (temp->PrevConnected()) // jeśli mamy coś z przodu
            {
                if (temp->PrevConnectedNo() == end::front) // jeśli pojazd od strony sprzęgu 0 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->PrevConnected(); // ten jest od strony 0
            }
            else
                return temp; // jeśli jednak z przodu nic nie ma
        }
        else
        {
            if (temp->NextConnected())
            {
                if (temp->NextConnectedNo() == end::rear) // jeśli pojazd od strony sprzęgu 1 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->NextConnected(); // ten pojazd jest od strony 1
            }
            else
                return temp; // jeśli jednak z tyłu nic nie ma
        }
    }
    return NULL; // to tylko po wyczerpaniu pętli
};

//---------------------------------------------------------------------------
float TDynamicObject::GetEPP()
{ // szukanie skrajnego połączonego pojazdu w pociagu
    // od strony sprzegu (coupler_nr) obiektu (start)
    TDynamicObject *temp = this;
    int coupler_nr = 0;
    double eq = 0.0;
    double am = 0.0;

    for (int i = 0; i < 300; ++i) // ograniczenie do 300 na wypadek zapętlenia składu
    {
        if (!temp)
            break; // Ra: zabezpieczenie przed ewentaulnymi błędami sprzęgów
        eq += temp->MoverParameters->PipePress * temp->MoverParameters->Dim.L;
        am += temp->MoverParameters->Dim.L;
        if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag & coupling::brakehose) != coupling::brakehose)
            break; // nic nie ma już dalej podłączone
        if (coupler_nr == 0)
        { // jeżeli szukamy od sprzęgu 0
            if (temp->PrevConnected()) // jeśli mamy coś z przodu
            {
                if (temp->PrevConnectedNo() == end::front) // jeśli pojazd od strony sprzęgu 0 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->PrevConnected(); // ten jest od strony 0
            }
            else
                break; // jeśli jednak z przodu nic nie ma
        }
        else
        {
            if (temp->NextConnected())
            {
                if (temp->NextConnectedNo() == end::rear) // jeśli pojazd od strony sprzęgu 1 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->NextConnected(); // ten pojazd jest od strony 1
            }
            else
                break; // jeśli jednak z tyłu nic nie ma
        }
    }

    temp = this;
    coupler_nr = 1;
    for (int i = 0; i < 300; i++) // ograniczenie do 300 na wypadek zapętlenia składu
    {
        if (!temp)
            break; // Ra: zabezpieczenie przed ewentaulnymi błędami sprzęgów
        eq += temp->MoverParameters->PipePress * temp->MoverParameters->Dim.L;
        am += temp->MoverParameters->Dim.L;
        if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag & coupling::brakehose) != coupling::brakehose)
            break; // nic nie ma już dalej podłączone
        if (coupler_nr == 0)
        { // jeżeli szukamy od sprzęgu 0
            if (temp->PrevConnected()) // jeśli mamy coś z przodu
            {
                if (temp->PrevConnectedNo() == end::front) // jeśli pojazd od strony sprzęgu 0 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->PrevConnected(); // ten jest od strony 0
            }
            else
                break; // jeśli jednak z przodu nic nie ma
        }
        else
        {
            if (temp->NextConnected())
            {
                if (temp->NextConnectedNo() == end::rear) // jeśli pojazd od strony sprzęgu 1 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->NextConnected(); // ten pojazd jest od strony 1
            }
            else
                break; // jeśli jednak z tyłu nic nie ma
        }
    }
    eq -= MoverParameters->PipePress * MoverParameters->Dim.L;
    am -= MoverParameters->Dim.L;
    return eq / am;
};

//---------------------------------------------------------------------------
TDynamicObject * TDynamicObject::GetFirstDynamic(int cpl_type, int cf)
{ // Szukanie skrajnego połączonego pojazdu w pociagu
    // od strony sprzegu (cpl_type) obiektu szukajacego
    // Ra: wystarczy jedna funkcja do szukania w obu kierunkach
    return FirstFind(cpl_type, cf); // używa referencji
};

void TDynamicObject::ABuSetModelShake( Math3D::vector3 mShake )
{
    modelShake = mShake;
};

int TDynamicObject::GetPneumatic(bool front, bool red)
{
    int x, y, z; // 1=prosty, 2=skośny
    if (red)
    {
        if (front)
        {
            x = btCPneumatic1.GetStatus();
            y = btCPneumatic1r.GetStatus();
        }
        else
        {
            x = btCPneumatic2.GetStatus();
            y = btCPneumatic2r.GetStatus();
        }
    }
    else if (front)
    {
        x = btPneumatic1.GetStatus();
        y = btPneumatic1r.GetStatus();
    }
    else
    {
        x = btPneumatic2.GetStatus();
        y = btPneumatic2r.GetStatus();
    }
    z = 0; // brak węży?
    if ((x > 0) && (y > 0))
        z = 3; // dwa
    if ((x > 0) && (y == 0))
        z = 1; // lewy, brak prawego
    if ((x == 0) && (y > 0))
        z = 2; // brak lewego, prawy

    return z;
}

void TDynamicObject::SetPneumatic(bool front, bool red)
{
	int x = 0,
		ten = 0,
		tamten = 0;
    ten = GetPneumatic(front, red); // 1=lewy skos,2=prawy skos,3=dwa proste
    if (front)
        if (PrevConnected()) // pojazd od strony sprzęgu 0
            tamten = PrevConnected()->GetPneumatic((PrevConnectedNo() == end::front ? true : false), red);
    if (!front)
        if (NextConnected()) // pojazd od strony sprzęgu 1
            tamten = NextConnected()->GetPneumatic((NextConnectedNo() == end::front ? true : false), red);
    if (ten == tamten) // jeśli układ jest symetryczny
        switch (ten)
        {
        case 1:
            x = 2;
            break; // mamy lewy skos, dać lewe skosy
        case 2:
            x = 3;
            break; // mamy prawy skos, dać prawe skosy
        case 3: // wszystkie cztery na prosto
            if (MoverParameters->Couplers[front ? end::front : end::rear].Render)
                x = 1;
            else
                x = 4;
            break;
        }
    else
    {
        if (ten == 2)
            x = 4;
        if (ten == 1)
            x = 1;
        if (ten == 3)
            if (tamten == 1)
                x = 4;
            else
                x = 1;
    }
    if (front)
    {
        if (red)
            cp1 = x;
        else
            sp1 = x;
    } // który pokazywać z przodu
    else
    {
        if (red)
            cp2 = x;
        else
            sp2 = x;
    } // który pokazywać z tyłu
}

void TDynamicObject::UpdateAxle(TAnim *pAnim)
{ // animacja osi
    size_t wheel_id = pAnim->dWheelAngle;
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), dWheelAngle[wheel_id]);
    pAnim->smAnimated->future_transform = glm::rotate((float)glm::radians(m_future_wheels_angle[wheel_id]), glm::vec3(1.0f, 0.0f, 0.0f));
};

// animacja drzwi - przesuw
void TDynamicObject::UpdateDoorTranslate(TAnim *pAnim) {
    if( pAnim->smAnimated == nullptr ) { return; }

    auto const &door { MoverParameters->Doors.instances[ (
        ( pAnim->iNumber & 1 ) == 0 ?
            side::right :
            side::left ) ] };

    pAnim->smAnimated->SetTranslate(
        Math3D::vector3{
            0.0,
            0.0,
            door.position } );
};

// animacja drzwi - obrót
void TDynamicObject::UpdateDoorRotate(TAnim *pAnim) {

    if( pAnim->smAnimated == nullptr ) { return; }

    auto const &door { MoverParameters->Doors.instances[ (
        ( pAnim->iNumber & 1 ) == 0 ?
            side::right :
            side::left ) ] };

    pAnim->smAnimated->SetRotate(
        float3(1, 0, 0),
        door.position );
};

// animacja drzwi - obrót
void TDynamicObject::UpdateDoorFold(TAnim *pAnim) {

    if( pAnim->smAnimated == nullptr ) { return; }

    auto const &door { MoverParameters->Doors.instances[ (
        ( pAnim->iNumber & 1 ) == 0 ?
            side::right :
            side::left ) ] };

    // skrzydło mniejsze
    pAnim->smAnimated->SetRotate(
        float3(0, 0, 1),
        door.position);

    // skrzydło większe
    auto *sm = pAnim->smAnimated->ChildGet();
    if( sm == nullptr ) { return; }

    sm->SetRotate(
        float3(0, 0, 1),
        -door.position - door.position);

    // podnóżek?
    sm = sm->ChildGet();
    if( sm == nullptr ) { return; }

    sm->SetRotate(
        float3(0, 1, 0),
        door.position);
};

// animacja drzwi - odskokprzesuw
void TDynamicObject::UpdateDoorPlug(TAnim *pAnim) {

    if( pAnim->smAnimated == nullptr ) { return; }

    auto const &door { MoverParameters->Doors.instances[ (
        ( pAnim->iNumber & 1 ) == 0 ?
            side::right :
            side::left ) ] };

    pAnim->smAnimated->SetTranslate(
        Math3D::vector3 {
            std::min(
                door.position * 2.f,
                MoverParameters->Doors.range_out ),
            0.0,
            std::max(
                0.f,
                door.position - MoverParameters->Doors.range_out * 0.5f ) } );
}

void TDynamicObject::UpdatePant(TAnim *pAnim)
{ // animacja pantografu - 4 obracane ramiona, ślizg piąty
    float a, b, c;
    a = RadToDeg(pAnim->fParamPants->fAngleL - pAnim->fParamPants->fAngleL0);
    b = RadToDeg(pAnim->fParamPants->fAngleU - pAnim->fParamPants->fAngleU0);
    c = a + b;
    if (pAnim->smElement[0])
        pAnim->smElement[0]->SetRotate(float3(-1, 0, 0), a); // dolne ramię
    if (pAnim->smElement[1])
        pAnim->smElement[1]->SetRotate(float3(1, 0, 0), a);
    if (pAnim->smElement[2])
        pAnim->smElement[2]->SetRotate(float3(1, 0, 0), c); // górne ramię
    if (pAnim->smElement[3])
        pAnim->smElement[3]->SetRotate(float3(-1, 0, 0), c);
    if (pAnim->smElement[4])
        pAnim->smElement[4]->SetRotate(float3(-1, 0, 0), b); //ślizg
}

// doorstep animation, shift
void TDynamicObject::UpdatePlatformTranslate( TAnim *pAnim ) {

    if( pAnim->smAnimated == nullptr ) { return; }

    auto const &door { MoverParameters->Doors.instances[ (
        ( pAnim->iNumber & 1 ) == 0 ?
            side::right :
            side::left ) ] };

    pAnim->smAnimated->SetTranslate(
        Math3D::vector3{
            interpolate( 0.f, MoverParameters->Doors.step_range, door.step_position ),
            0.0,
            0.0 } );
}

// doorstep animation, rotate
void TDynamicObject::UpdatePlatformRotate( TAnim *pAnim ) {

    if( pAnim->smAnimated == nullptr ) { return; }

    auto const &door { MoverParameters->Doors.instances[ (
        ( pAnim->iNumber & 1 ) == 0 ?
            side::right :
            side::left ) ] };

    pAnim->smAnimated->SetRotate(
        float3( 0, 1, 0 ),
        interpolate( 0.f, MoverParameters->Doors.step_range, door.step_position ) );
}

// mirror animation, rotate
void TDynamicObject::UpdateMirror( TAnim *pAnim ) {

    if( pAnim->smAnimated == nullptr ) { return; }

    // only animate the mirror if it's located on the same end of the vehicle as the active cab
    auto const isactive { (
        MoverParameters->CabOccupied > 0 ? ( ( pAnim->iNumber >> 4 ) == end::front ? 1.0 : 0.0 ) :
        MoverParameters->CabOccupied < 0 ? ( ( pAnim->iNumber >> 4 ) == end::rear  ? 1.0 : 0.0 ) :
        0.0 ) };

    if( pAnim->iNumber & 1 )
        pAnim->smAnimated->SetRotate(
            float3( 0, 1, 0 ),
            interpolate( 0.0, MoverParameters->MirrorMaxShift, dMirrorMoveR * isactive ) );
    else
        pAnim->smAnimated->SetRotate(
            float3( 0, 1, 0 ),
            interpolate( 0.0, MoverParameters->MirrorMaxShift, dMirrorMoveL * isactive ) );
}

/*
void TDynamicObject::UpdateLeverDouble(TAnim *pAnim)
{ // animacja gałki zależna od double
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), pAnim->fSpeed * *pAnim->fDoubleBase);
};
void TDynamicObject::UpdateLeverFloat(TAnim *pAnim)
{ // animacja gałki zależna od float
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), pAnim->fSpeed * *pAnim->fFloatBase);
};
void TDynamicObject::UpdateLeverInt(TAnim *pAnim)
{ // animacja gałki zależna od int
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), pAnim->fSpeed * *pAnim->iIntBase);
};
void TDynamicObject::UpdateLeverEnum(TAnim *pAnim)
{ // ustawienie kąta na
    // wartość wskazaną przez
    // int z tablicy fParam
    // pAnim->fParam[0]; - dodać lepkość
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), pAnim->fParam[*pAnim->iIntBase]);
};
*/
// sets light levels for registered interior sections
void
TDynamicObject::toggle_lights() {

    if( true == SectionLightsActive ) {
        // switch all lights off...
        for( auto &section : Sections ) {
            auto const sectionname { section.compartment->pName };
            if( sectionname.rfind( "cab", 0 ) != 0 ) {
                section.light_level = 0.0;
            }
        }
        SectionLightsActive = false;
    }
    else {
        // set lights with probability depending on the compartment type. TODO: expose this in .mmd file
        for( auto &section : Sections ) {

            auto const sectionname { section.compartment->pName };
            if( ( sectionname.find( "corridor" ) == 0 )
             || ( sectionname.find( "korytarz" ) == 0 ) ) {
                // corridors are lit 100% of time
                section.light_level = 0.75f;
            }
            else if(
                ( sectionname.find( "compartment" ) == 0 )
             || ( sectionname.find( "przedzial" )   == 0 ) ) {
                // compartments are lit with 75% probability
                section.light_level = ( Random() < 0.75 ? 0.75f : 0.10f );
            }
        }
        SectionLightsActive = true;
    }
}

void
TDynamicObject::set_cab_lights( int const Cab, float const Level ) {

    for( auto &section : Sections ) {
        // cab compartments are placed at the beginning of the list, so we can bail out as soon as we find different compartment type
        auto const sectionname { section.compartment->pName };
        if( sectionname.size() < 4 ) { return; }
        if( sectionname.find( "cab" ) != 0 ) { return; }
        if( sectionname[ 3 ] != Cab + '0' ) { continue; } // match the cab with correct index

        section.light_level = Level;
    }
}

// ABu 29.01.05 przeklejone z render i renderalpha: *********************
void TDynamicObject::ABuLittleUpdate(double ObjSqrDist)
{ // ABu290105: pozbierane i uporzadkowane powtarzajace
    // sie rzeczy z Render i RenderAlpha
    // dodatkowy warunek, if (ObjSqrDist<...) zeby niepotrzebnie nie zmianiec w
    // obiektach,
    // ktorych i tak nie widac
    // NBMX wrzesien, MC listopad: zuniwersalnione
    btnOn = false; // czy przywrócić stan domyślny po renderowaniu

    if (mdLoad) // tymczasowo ładunek na poziom podłogi
        if (vFloor.z > 0.0)
            mdLoad->GetSMRoot()->SetTranslate(modelShake + vFloor);

    if (ObjSqrDist < ( 400 * 400 ) ) // gdy bliżej niż 400m
    {
        for( auto &animation : pAnimations ) {
            // wykonanie kolejnych animacji
            if( ( ObjSqrDist < animation.fMaxDist )
             && ( animation.yUpdate ) ) {
                // jeśli zdefiniowana funkcja aktualizacja animacji (położenia submodeli
                animation.yUpdate( &animation );
            }
        }

        if( ( mdModel != nullptr )
         && ( ObjSqrDist < ( 50 * 50 ) ) ) {
            // gdy bliżej niż 50m
            // ABu290105: rzucanie pudlem
            // te animacje wymagają bananów w modelach!
            mdModel->GetSMRoot()->SetTranslate(modelShake);
            if (mdKabina)
                mdKabina->GetSMRoot()->SetTranslate(modelShake);
            if (mdLoad)
                mdLoad->GetSMRoot()->SetTranslate(modelShake + vFloor);
            if (mdLowPolyInt)
                mdLowPolyInt->GetSMRoot()->SetTranslate(modelShake);
            // ABu: koniec rzucania
            // ABu011104: liczenie obrotow wozkow
            ABuBogies();
            // Mczapkie-100402: rysowanie lub nie - sprzegow
            // ABu-240105: Dodatkowy warunek: if (...).Render, zeby rysowal tylko jeden z polaczonych sprzegow
            // display _on if connected with another vehicle and the coupling owner (render flag)
            // display _xon if connected with another vehicle and not the coupling owner
            // display _xon if not connected, but equipped with coupling adapter
            // display _off if not connected, not equipped with coupling adapter or if _xon model is missing
            if( TestFlag( MoverParameters->Couplers[ end::front ].CouplingFlag, coupling::coupler ) ) {
                if( MoverParameters->Couplers[ end::front ].Render ) {
                    btCoupler1.TurnOn();
                }
                else {
                    btCoupler1.TurnxOnWithOffAsFallback();
                }
                btnOn = true;
            }
            else {
                if( true == MoverParameters->Couplers[ end::front ].has_adapter() ) {
                    btCoupler1.TurnxOnWithOffAsFallback();
                    btnOn = true;
                }
            }
            if( TestFlag( MoverParameters->Couplers[ end::rear ].CouplingFlag, coupling::coupler ) ) {
                if( MoverParameters->Couplers[ end::rear ].Render ) {
                    btCoupler2.TurnOn();
                }
                else {
                    btCoupler2.TurnxOnWithOffAsFallback();
                }
                btnOn = true;
            }
            else {
                if( true == MoverParameters->Couplers[ end::rear ].has_adapter() ) {
                    btCoupler2.TurnxOnWithOffAsFallback();
                    btnOn = true;
                }
            }
            //********************************************************************************
            // przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie
            // 'render' - juz
            // nie
            // przewody powietrzne, yB: decyzja na podstawie polaczen w t3d
            if (Global.bnewAirCouplers)
            {
                SetPneumatic(false, false); // wczytywanie z t3d ulozenia wezykow
                SetPneumatic(true, false); // i zapisywanie do zmiennej
                SetPneumatic(true, true); // ktore z nich nalezy
                SetPneumatic(false, true); // wyswietlic w tej klatce

                if (TestFlag(MoverParameters->Couplers[end::front].CouplingFlag, ctrain_pneumatic))
                {
                    switch (cp1)
                    {
                    case 1:
                        btCPneumatic1.TurnOn();
                        break;
                    case 2:
                        btCPneumatic1.TurnxOn();
                        break;
                    case 3:
                        btCPneumatic1r.TurnxOn();
                        break;
                    case 4:
                        btCPneumatic1r.TurnOn();
                        break;
                    }
                    btnOn = true;
                }

                if (TestFlag(MoverParameters->Couplers[end::rear].CouplingFlag, ctrain_pneumatic))
                {
                    switch (cp2)
                    {
                    case 1:
                        btCPneumatic2.TurnOn();
                        break;
                    case 2:
                        btCPneumatic2.TurnxOn();
                        break;
                    case 3:
                        btCPneumatic2r.TurnxOn();
                        break;
                    case 4:
                        btCPneumatic2r.TurnOn();
                        break;
                    }
                    btnOn = true;
                }

                // przewody zasilajace, j.w. (yB)
                if (TestFlag(MoverParameters->Couplers[end::front].CouplingFlag, ctrain_scndpneumatic))
                {
                    switch (sp1)
                    {
                    case 1:
                        btPneumatic1.TurnOn();
                        break;
                    case 2:
                        btPneumatic1.TurnxOn();
                        break;
                    case 3:
                        btPneumatic1r.TurnxOn();
                        break;
                    case 4:
                        btPneumatic1r.TurnOn();
                        break;
                    }
                    btnOn = true;
                }

                if (TestFlag(MoverParameters->Couplers[end::rear].CouplingFlag, ctrain_scndpneumatic))
                {
                    switch (sp2)
                    {
                    case 1:
                        btPneumatic2.TurnOn();
                        break;
                    case 2:
                        btPneumatic2.TurnxOn();
                        break;
                    case 3:
                        btPneumatic2r.TurnxOn();
                        break;
                    case 4:
                        btPneumatic2r.TurnOn();
                        break;
                    }
                    btnOn = true;
                }
            }
            //*********************************************************************************/
            else // po staremu ABu'oewmu
            {
                // przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie
                // 'render'
                if (TestFlag(MoverParameters->Couplers[end::front].CouplingFlag, ctrain_pneumatic))
                {
                    if (MoverParameters->Couplers[end::front].Render)
                        btCPneumatic1.TurnOn();
                    else
                        btCPneumatic1r.TurnOn();
                    btnOn = true;
                }

                if (TestFlag(MoverParameters->Couplers[end::rear].CouplingFlag, ctrain_pneumatic))
                {
                    if (MoverParameters->Couplers[end::rear].Render)
                        btCPneumatic2.TurnOn();
                    else
                        btCPneumatic2r.TurnOn();
                    btnOn = true;
                }

                // przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie
                // 'render'
                // //yB - zasilajace
                if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_scndpneumatic))
                {
                    if (MoverParameters->Couplers[0].Render)
                        btPneumatic1.TurnOn();
                    else
                        btPneumatic1r.TurnOn();
                    btnOn = true;
                }

                if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_scndpneumatic))
                {
                    if (MoverParameters->Couplers[1].Render)
                        btPneumatic2.TurnOn();
                    else
                        btPneumatic2r.TurnOn();
                    btnOn = true;
                }
            }
            //*************************************************************/// koniec
            // wezykow
            // uginanie zderzakow
            for (int i = 0; i < 2; ++i) {

                if( MoverParameters->Couplers[ i ].has_adapter() ) {
                    // HACK: if there's coupler adapter on this side, we presume there's additional distance put between vehicles
                    // which prevents buffers from clashing against each other (or the other vehicle doesn't have buffers to begin with)
                    continue;
                }

                auto const dist { clamp( MoverParameters->Couplers[ i ].Dist / 2.0, -MoverParameters->Couplers[ i ].DmaxB, 0.0 ) };

                if( dist >= 0.0 ) { continue; }

                if( smBuforLewy[ i ] ) {
                    smBuforLewy[ i ]->SetTranslate( Math3D::vector3( dist, 0, 0 ) );
                }
                if( smBuforPrawy[ i ] ) {
                    smBuforPrawy[ i ]->SetTranslate( Math3D::vector3( dist, 0, 0 ) );
                }
            }
        } // vehicle within 50m

        // Winger 160204 - podnoszenie pantografow

        // przewody sterowania ukrotnionego
        if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, coupling::control))
        {
            btCCtrl1.Turn( true );
            btnOn = true;
        }
        // else btCCtrl1.TurnOff();
        if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, coupling::control))
        {
            btCCtrl2.Turn( true );
            btnOn = true;
        }
        // else btCCtrl2.TurnOff();
        // McZapkie-181103: mostki przejsciowe
        if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_passenger))
        {
            btCPass1.Turn( true );
            btnOn = true;
        }
        // else btCPass1.TurnOff();
        if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_passenger))
        {
            btCPass2.Turn( true );
            btnOn = true;
        }
        // else btCPass2.TurnOff();
        if (MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable)
        { // sygnaly konca pociagu
            if (m_endsignals1.Active()) {
                if (TestFlag(MoverParameters->iLights[end::front], ( light::redmarker_left | light::redmarker_right ) ) ) {
                    m_endsignals1.Turn( true );
                    btnOn = true;
                }
            }
            else {
                if (TestFlag(MoverParameters->iLights[end::front], light::redmarker_left)) {
                    m_endsignal13.Turn( true );
                    btnOn = true;
                }
                if (TestFlag(MoverParameters->iLights[end::front], light::redmarker_right)) {
                    m_endsignal12.Turn( true );
                    btnOn = true;
                }
            }
            if (m_endsignals2.Active()) {
                if (TestFlag(MoverParameters->iLights[end::rear], ( light::redmarker_left | light::redmarker_right ) ) ) {
                    m_endsignals2.Turn( true );
                    btnOn = true;
                }
            }
            else {
                if (TestFlag(MoverParameters->iLights[end::rear], light::redmarker_left)) {
                    m_endsignal23.Turn( true );
                    btnOn = true;
                }
                if (TestFlag(MoverParameters->iLights[end::rear], light::redmarker_right)) {
                    m_endsignal22.Turn( true );
                    btnOn = true;
                }
            }
        }
        // tablice blaszane:
        if (TestFlag(MoverParameters->iLights[end::front], light::rearendsignals)) {
            m_endtab1.Turn( true );
            btnOn = true;
        }
        // else btEndSignalsTab1.TurnOff();
        if (TestFlag(MoverParameters->iLights[end::rear], light::rearendsignals)) {
            m_endtab2.Turn( true );
            btnOn = true;
        }
        // destination signs
        update_destinations();
        // else btEndSignalsTab2.TurnOff();
        // McZapkie-181002: krecenie wahaczem (korzysta z kata obrotu silnika)
        if (iAnimType[ANIM_LEVERS])
            for (int i = 0; i < 4; ++i)
                if (smWahacze[i])
                    smWahacze[i]->SetRotate(float3(1, 0, 0),
                                            fWahaczeAmp * cos(MoverParameters->eAngle));

        // cooling shutters
        // NOTE: shutters display _on state when they're closed, _off otherwise
        if( ( true == MoverParameters->dizel_heat.water.config.shutters )
         && ( false == MoverParameters->dizel_heat.zaluzje1 ) ) {
            btShutters1.Turn( true );
            btnOn = true;
        }
        if( ( true == MoverParameters->dizel_heat.water_aux.config.shutters )
         && ( false == MoverParameters->dizel_heat.zaluzje2 ) ) {
            btShutters2.Turn( true );
            btnOn = true;
        }

		if( ( false == bDisplayCab ) // edge case, lowpoly may act as a stand-in for the hi-fi cab, so make sure not to show the driver when inside
         && ( Mechanik != nullptr )
         && ( ( Mechanik->action() != TAction::actSleep )
           /* || ( MoverParameters->Battery ) */ ) ) {
            // rysowanie figurki mechanika
            btMechanik1.Turn( MoverParameters->CabOccupied > 0 );
            btMechanik2.Turn( MoverParameters->CabOccupied < 0 );
            if( MoverParameters->CabOccupied != 0 ) {
                btnOn = true;
            }
        }

    } // vehicle within 400m

    if( MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable )
    { // sygnały czoła pociagu //Ra: wyświetlamy bez
        // ograniczeń odległości, by były widoczne z
        // daleka
        if (TestFlag(MoverParameters->iLights[end::front], light::headlight_left))
        {
            if( DimHeadlights ) {
                m_headlamp13.TurnxOnWithOnAsFallback();
            }
            else {
                m_headlamp13.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::front], light::headlight_upper))
        {
            if( DimHeadlights ) {
                m_headlamp11.TurnxOnWithOnAsFallback();
            }
            else {
                m_headlamp11.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::front], light::headlight_right))
        {
            if( DimHeadlights ) {
                m_headlamp12.TurnxOnWithOnAsFallback();
            }
            else {
                m_headlamp12.TurnOn();
            }
            btnOn = true;
        }
        // else btHeadSignals13.TurnOff();
        if (TestFlag(MoverParameters->iLights[end::rear], light::headlight_left))
        {
            if( DimHeadlights ) {
                m_headlamp23.TurnxOnWithOnAsFallback();
            }
            else {
                m_headlamp23.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::rear], light::headlight_upper))
        {
            if( DimHeadlights ) {
                m_headlamp21.TurnxOnWithOnAsFallback();
            }
            else {
                m_headlamp21.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::rear], light::headlight_right))
        {
            if( DimHeadlights ) {
                m_headlamp22.TurnxOnWithOnAsFallback();
            }
            else {
                m_headlamp22.TurnOn();
            }
            btnOn = true;
        }
        // auxiliary lights
        if (TestFlag(MoverParameters->iLights[end::front], light::auxiliary_left))
        {
            if( DimHeadlights ) {
                m_headsignal13.TurnxOnWithOnAsFallback();
            }
            else {
                m_headsignal13.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::front], light::auxiliary_right))
        {
            if( DimHeadlights ) {
                m_headsignal12.TurnxOnWithOnAsFallback();
            }
            else {
                m_headsignal12.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::rear], light::auxiliary_left))
        {
            if( DimHeadlights ) {
                m_headsignal23.TurnxOnWithOnAsFallback();
            }
            else {
                m_headsignal23.TurnOn();
            }
            btnOn = true;
        }
        if (TestFlag(MoverParameters->iLights[end::rear], light::auxiliary_right))
        {
            if( DimHeadlights ) {
                m_headsignal22.TurnxOnWithOnAsFallback();
            }
            else {
                m_headsignal22.TurnOn();
            }
            btnOn = true;
        }
    }
    // interior light levels
    auto sectionlightcolor { glm::vec4( 1.f ) };
    bool cabsection{ true };
    for( auto const &section : Sections ) {
        if( cabsection ) {
            // check whether we're still processing cab sections
            auto const &sectionname { section.compartment->pName };
            cabsection &= ( ( sectionname.size() >= 4 ) && ( starts_with( sectionname, "cab" ) ) );
        }
        // TODO: add cablight devices
        auto const sectionlightlevel { section.light_level * ( cabsection ? 1.0f : MoverParameters->CompartmentLights.intensity ) };
        sectionlightcolor = glm::vec4(
            ( ( ( sectionlightlevel == 0.f ) || ( Global.fLuminance > section.compartment->fLight ) ) ?
                glm::vec3( 240.f / 255.f ) : // TBD: save and restore initial submodel diffuse instead of enforcing one?
                InteriorLight ), // TODO: per-compartment (type) light color
            sectionlightlevel );
        section.compartment->SetLightLevel( sectionlightcolor, true );
        if( section.load != nullptr ) {
            section.load->SetLightLevel( sectionlightcolor, true );
        }
    }
    // load chunks visibility
    for( auto const &section : Sections ) {
        // section isn't guaranteed to have load model, so check that first
        if( section.load == nullptr ) { continue; }
        section.load->iVisible = ( section.load_chunks_visible > 0 );
        // if the section root isn't visible we can skip meddling with its children
        if( false == section.load->iVisible ) { continue; }
        // if the section root is visible set the state of section chunks
        auto *sectionchunk { section.load->ChildGet() };
        auto visiblechunkcount { section.load_chunks_visible };
        while( sectionchunk != nullptr ) {
            sectionchunk->iVisible = ( visiblechunkcount > 0 );
            --visiblechunkcount;
            sectionchunk = sectionchunk->NextGet();
        }
    }

    // driver cabs visibility
    for( int cabidx = 0; cabidx < LowPolyIntCabs.size(); ++cabidx ) {
        if( LowPolyIntCabs[ cabidx ] == nullptr ) { continue; }
        LowPolyIntCabs[ cabidx ]->iVisible = (
            mdKabina == nullptr ? true : // there's no hi-fi cab
            bDisplayCab == false ? true : // we're in external view
            simulation::Train == nullptr ? true : // not a player-driven vehicle, implies external view
            simulation::Train->Dynamic() != this ? true : // not a player-driven vehicle, implies external view
            JointCabs ? false : // internal view, all cabs share the model so hide them 'all'
            ( simulation::Train->iCabn != cabidx ) ); // internal view, hide occupied cab and show others
    }
}
// ABu 29.01.05 koniec przeklejenia *************************************

TDynamicObject * TDynamicObject::ABuFindNearestObject(glm::vec3 pos, TTrack *Track, TDynamicObject *MyPointer, int &CouplNr)
{
	// zwraca wskaznik do obiektu znajdujacego sie na torze (Track), którego sprzęg jest najblizszy punktowi
    // służy np. do łączenia i rozpinania sprzęgów
    // WE: Track      - tor, na ktorym odbywa sie poszukiwanie
    //    MyPointer  - wskaznik do obiektu szukajacego
    // WY: CouplNr    - który sprzęg znalezionego obiektu jest bliższy kamerze

    // Uwaga! Jesli CouplNr==-2 to szukamy njblizszego obiektu, a nie sprzegu!!!
    for( auto dynamic : Track->Dynamics ) {

        if( CouplNr == -2 ) {
            // wektor [kamera-obiekt] - poszukiwanie obiektu
			if( Math3D::LengthSquared3( pos - dynamic->vPosition ) < 100.0 ) {
                // 10 metrów
                return dynamic;
            }
        }
        else {
            // jeśli (CouplNr) inne niz -2, szukamy sprzęgu
			if( Math3D::LengthSquared3( pos - dynamic->vCoulpler[ 0 ] ) < 25.0 ) {
                // 5 metrów
                CouplNr = 0;
                return dynamic;
            }
			if( Math3D::LengthSquared3( pos - dynamic->vCoulpler[ 1 ] ) < 25.0 ) {
                // 5 metrów
                CouplNr = 1;
                return dynamic;
            }
        }
    }
    // empty track or nothing found
    return nullptr;
}

TDynamicObject * TDynamicObject::ABuScanNearestObject(glm::vec3 pos, TTrack *Track, double ScanDir, double ScanDist, int &CouplNr)
{ // skanowanie toru w poszukiwaniu obiektu najblizszego punktowi
    if (ABuGetDirection() < 0)
        ScanDir = -ScanDir;
    TDynamicObject *FoundedObj;
    FoundedObj =
	    ABuFindNearestObject(pos, Track, this, CouplNr); // zwraca numer sprzęgu znalezionego pojazdu
    if (FoundedObj == NULL)
    {
        double ActDist; // Przeskanowana odleglosc.
        double CurrDist = 0; // Aktualna dlugosc toru.
        if (ScanDir >= 0)
            ActDist =
                Track->Length() - RaTranslationGet(); //???-przesunięcie wózka względem Point1 toru
        else
            ActDist = RaTranslationGet(); // przesunięcie wózka względem Point1 toru
        while (ActDist < ScanDist)
        {
            ActDist += CurrDist;
            if (ScanDir > 0) // do przodu
            {
                if (Track->iNextDirection)
                {
                    Track = Track->CurrentNext();
                    ScanDir = -ScanDir;
                }
                else
                    Track = Track->CurrentNext();
            }
            else // do tyłu
            {
                if (Track->iPrevDirection)
                    Track = Track->CurrentPrev();
                else
                {
                    Track = Track->CurrentPrev();
                    ScanDir = -ScanDir;
                }
            }
            if (Track != NULL)
            { // jesli jest kolejny odcinek toru
                CurrDist = Track->Length();
				FoundedObj = ABuFindNearestObject(pos, Track, this, CouplNr);
                if (FoundedObj != NULL)
                    ActDist = ScanDist;
            }
            else // Jesli nie ma, to wychodzimy.
                ActDist = ScanDist;
        }
    } // Koniec szukania najblizszego toru z jakims obiektem.
    return FoundedObj;
}

// ABu 01.11.04 poczatek wyliczania przechylow pudla **********************
void TDynamicObject::ABuModelRoll()
{ // ustawienie przechyłki pojazdu i jego
    // zawartości
    // Ra: przechyłkę załatwiamy na etapie przesuwania modelu
}

// ABu 06.05.04 poczatek wyliczania obrotow wozkow **********************

void TDynamicObject::ABuBogies()
{ // Obracanie wozkow na zakretach. Na razie
    // uwzględnia tylko zakręty,
    // bez zadnych gorek i innych przeszkod.
    if ((smBogie[0] != NULL) && (smBogie[1] != NULL))
    {
        // modelRot.z=ABuAcos(Axle0.pPosition-Axle1.pPosition); //kąt obrotu pojazdu
        // [rad]
        // bogieRot[0].z=ABuAcos(Axle0.pPosition-Axle3.pPosition);
        bogieRot[0].z = Axle0.vAngles.z;
        bogieRot[0] = RadToDeg(modelRot - bogieRot[0]); // mnożenie wektora przez stałą
        smBogie[0]->SetRotateXYZ(bogieRot[0]);
        // bogieRot[1].z=ABuAcos(Axle2.pPosition-Axle1.pPosition);
        bogieRot[1].z = Axle1.vAngles.z;
        bogieRot[1] = RadToDeg(modelRot - bogieRot[1]);
        smBogie[1]->SetRotateXYZ(bogieRot[1]);
    }
};
// ABu 06.05.04 koniec wyliczania obrotow wozkow ************************

// ABu 16.03.03 sledzenie toru przed obiektem: **************************
void TDynamicObject::ABuCheckMyTrack()
{ // Funkcja przypisujaca obiekt
    // prawidlowej tablicy Dynamics,
    // bo gdzies jest jakis blad i wszystkie obiekty z danego
    // pociagu na poczatku stawiane sa na jednym torze i wpisywane
    // do jednej tablicy. Wykonuje sie tylko raz - po to 'ABuChecked'
    TTrack *OldTrack = MyTrack;
    TTrack *NewTrack = Axle0.GetTrack();
    if( NewTrack != OldTrack ) {
        if( OldTrack ) {
            OldTrack->RemoveDynamicObject( this );
        }
        NewTrack->AddDynamicObject(this);
    }
    iAxleFirst = 0; // pojazd powiązany z przednią osią - Axle0
}

// Ra: w poniższej funkcji jest problem ze sprzęgami
TDynamicObject *
TDynamicObject::ABuFindObject( int &Foundcoupler, double &Distance, TTrack const *Track, int const Direction, int const Mycoupler ) const
{ // Zwraca wskaźnik najbliższego obiektu znajdującego się
    // na torze w określonym kierunku, ale tylko wtedy, kiedy
    // obiekty mogą się zderzyć, tzn. nie mijają się.
    // WE:
    //    Track     - tor, na ktorym odbywa sie poszukiwanie,
    //    Direction - kierunek szukania na torze (+1:w stronę Point2, -1:w stronę Point1)
    //    Mycoupler - nr sprzegu obiektu szukajacego;
    // WY:
    //    wskaznik do znalezionego obiektu.
    //    Foundcoupler - nr sprzegu znalezionego obiektu
    //    Distance - distance to found object

    if( true == Track->Dynamics.empty() ) {
        // sens szukania na tym torze jest tylko, gdy są na nim pojazdy
        Distance += Track->Length(); // doliczenie długości odcinka do przeskanowanej odległości
        return nullptr; // nie ma pojazdów na torze, to jest NULL
    }

    double distance = Track->Length(); // najmniejsza znaleziona odleglość (zaczynamy od długości toru)
    double myposition; // pozycja szukającego na torze
    TDynamicObject *foundobject = nullptr;
    if( MyTrack == Track ) {
        // gdy szukanie na tym samym torze
        myposition = RaTranslationGet(); // położenie wózka względem Point1 toru
    }
    else {
        // gdy szukanie na innym torze
        if( Direction > 0 ) {
            // szukanie w kierunku Point2 (od zera) - jesteśmy w Point1
            myposition = 0;
        }
        else {
            // szukanie w kierunku Point1 (do zera) - jesteśmy w Point2
            myposition = distance;
        }
    }

    double objectposition; // robocza odległość od kolejnych pojazdów na danym odcinku
    for( auto dynamic : Track->Dynamics ) {

        if( dynamic == this ) { continue; } // szukający się nie liczy
/*
        if( ( ( dynamic == PrevConnected ) && ( MoverParameters->Couplers[ TMoverParameters::side::front ].CouplingFlag != coupling::faux ) )
         || ( ( dynamic == NextConnected ) && ( MoverParameters->Couplers[ TMoverParameters::side::rear  ].CouplingFlag != coupling::faux ) )  ){
            // stop-gap check to prevent 'detection' of attached vehicles
            // which seems to be a side-effect of inaccurate location calculation in 'simple' mode?
            // TODO: investigate actual cause
            continue;
        }
*/
        if( Direction > 0 ) {
            // jeśli szukanie w kierunku Point2
            objectposition = ( dynamic->RaTranslationGet() ) - myposition; // odległogłość tamtego od szukającego
            if( ( objectposition > 0 )
             && ( objectposition < distance ) ) {
                // gdy jest po właściwej stronie i bliżej niż jakiś wcześniejszy
                Foundcoupler = ( dynamic->RaDirectionGet() > 0 ) ? 1 : 0; // to, bo (ScanDir>=0)
            }
            else { continue; }
        }
        else {
            objectposition = myposition - ( dynamic->RaTranslationGet() ); //???-przesunięcie wózka względem Point1 toru
            if( ( objectposition > 0 )
             && ( objectposition < distance ) ) {
                Foundcoupler = ( dynamic->RaDirectionGet() > 0 ) ? 0 : 1; // odwrotnie, bo (ScanDir<0)
            }
            else { continue; }
        }

        if( Track->iCategoryFlag & 254 ) {
            // trajektoria innego typu niż tor kolejowy
            // dla torów nie ma sensu tego sprawdzać, rzadko co jedzie po jednej szynie i się mija
            // Ra: mijanie samochodów wcale nie jest proste
            // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
            // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
            // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
            double relativeoffset; // wzajemna odległość poprzeczna
            if( Foundcoupler != Mycoupler ) {
                // facing the same direction
                relativeoffset = std::abs( MoverParameters->OffsetTrackH - dynamic->MoverParameters->OffsetTrackH );
            }
            else {
                relativeoffset = std::abs( MoverParameters->OffsetTrackH + dynamic->MoverParameters->OffsetTrackH );
            }
            if( relativeoffset + relativeoffset > MoverParameters->Dim.W + dynamic->MoverParameters->Dim.W ) {
                // odległość większa od połowy sumy szerokości - kolizji nie będzie
                continue;
            }
            // jeśli zahaczenie jest niewielkie, a jest miejsce na poboczu, to zjechać na pobocze
        }
        foundobject = dynamic; // potencjalna kolizja
        distance = objectposition; // odleglość pomiędzy aktywnymi osiami pojazdów
    }

    Distance += distance; // doliczenie odległości przeszkody albo długości odcinka do przeskanowanej odległości
    return foundobject;
}

int TDynamicObject::DettachStatus(int dir)
{ // sprawdzenie odległości sprzęgów
    // rzeczywistych od strony (dir):
    // 0=przód,1=tył
    // Ra: dziwne, że ta funkcja nie jest używana
    if( MoverParameters->Couplers[ dir ].CouplingFlag == coupling::faux ) {
        return 0; // jeśli nic nie podłączone, to jest OK
    }
    return (MoverParameters->DettachStatus(dir)); // czy jest w odpowiedniej odległości?
}

int TDynamicObject::Dettach(int dir)
{ // rozłączenie sprzęgów rzeczywistych od
    // strony (dir): 0=przód,1=tył
    // zwraca maskę bitową aktualnych sprzegów (0 jeśli rozłączony)
    if (ctOwner)
    { // jeśli pojazd ma przypisany obiekt nadzorujący skład, to póki
        // są wskaźniki
        TDynamicObject *d = this;
        while (d)
        {
            d->ctOwner = NULL; // usuwanie właściciela
            d = d->Prev();
        }
        d = Next();
        while (d)
        {
            d->ctOwner = NULL; // usuwanie właściciela
            d = d->Next(); // i w drugą stronę
        }
    }
    if( MoverParameters->Couplers[ dir ].CouplingFlag ) {
        // odczepianie, o ile coś podłączone
        MoverParameters->Dettach( dir );
/*
        if( true == MoverParameters->Dettach( dir ) ) {
            auto *othervehicle { dir ? NextConnected : PrevConnected };
            auto const othercoupler { dir ? NextConnectedNo() : PrevConnectedNo() };
            ( othercoupler ? othervehicle->NextConnected : othervehicle->PrevConnected ) = nullptr;
            ( dir ? NextConnected : PrevConnected ) = nullptr;
        };
*/
    }
    // sprzęg po rozłączaniu (czego się nie da odpiąć
    return MoverParameters->Couplers[dir].CouplingFlag;
}

void
TDynamicObject::couple( int const Side ) {

    auto const &neighbour { MoverParameters->Neighbours[ Side ] };

    if( neighbour.vehicle == nullptr ) { return; }

    auto const &coupler { MoverParameters->Couplers[ Side ] };
    auto *othervehicle { neighbour.vehicle };
    auto *othervehicleparams{ othervehicle->MoverParameters };
    auto const &othercoupler { othervehicleparams->Couplers[ neighbour.vehicle_end ] };

    if( coupler.CouplingFlag == coupling::faux ) {
        // najpierw hak
        if( ( coupler.AllowedFlag
            & othercoupler.AllowedFlag
            & coupling::coupler ) == coupling::coupler ) {
            if( MoverParameters->Attach(
                    Side, neighbour.vehicle_end,
                    othervehicleparams,
                    coupling::coupler ) ) {
                // one coupling type per key press
                return;
            }
            else {
                WriteLog( "Mechanical coupling failed." );
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::brakehose ) ) {
        // pneumatyka
        if( ( coupler.AllowedFlag
            & othercoupler.AllowedFlag
            & coupling::brakehose ) == coupling::brakehose ) {
            if( MoverParameters->Attach(
                    Side, neighbour.vehicle_end,
                    othervehicleparams,
                    ( coupler.CouplingFlag | coupling::brakehose ) ) ) {
                SetPneumatic( Side != 0, true );
                othervehicle->SetPneumatic( Side != 0, true );
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::mainhose ) ) {
        // zasilajacy
        if( ( coupler.AllowedFlag
            & othercoupler.AllowedFlag
            & coupling::mainhose ) == coupling::mainhose ) {
            if( MoverParameters->Attach(
                    Side, neighbour.vehicle_end,
                    othervehicleparams,
                    ( coupler.CouplingFlag | coupling::mainhose ) ) ) {
                SetPneumatic( Side != 0, false );
                othervehicle->SetPneumatic( Side != 0, false );
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::control ) ) {
        // ukrotnionko
        if( ( ( coupler.AllowedFlag & othercoupler.AllowedFlag & coupling::control ) == coupling::control )
         && ( coupler.control_type == othercoupler.control_type ) ) {
            if( MoverParameters->Attach(
                    Side, neighbour.vehicle_end,
                    othervehicleparams,
                    ( coupler.CouplingFlag | coupling::control ) ) ) {
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::gangway ) ) {
        // mostek
        if( ( coupler.AllowedFlag
            & othercoupler.AllowedFlag
            & coupling::gangway ) == coupling::gangway ) {
            if( MoverParameters->Attach(
                    Side, neighbour.vehicle_end,
                    othervehicleparams,
                    ( coupler.CouplingFlag | coupling::gangway ) ) ) {
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::heating ) ) {
        // heating
        if( ( coupler.AllowedFlag
            & othercoupler.AllowedFlag
            & coupling::heating ) == coupling::heating ) {
            if( MoverParameters->Attach(
                    Side, neighbour.vehicle_end,
                    othervehicleparams,
                    ( coupler.CouplingFlag | coupling::heating ) ) ) {
                // one coupling type per key press
                return;
            }
        }
    }

}

int
TDynamicObject::uncouple( int const Side ) {

    if( ( DettachStatus( Side ) >= 0 )
     || ( true == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::permanent ) ) ) {
        // can't uncouple, return existing coupling state
        return MoverParameters->Couplers[ Side ].CouplingFlag;
    }
    // jeżeli sprzęg niezablokowany, jest co odczepić i się da
    auto const couplingflag { Dettach( Side ) };
    return couplingflag;
}

bool
TDynamicObject::attach_coupler_adapter( int const Side, bool const Enforce ) {

    auto &coupler { MoverParameters->Couplers[ Side ] };
    // sanity check(s)
    if( coupler.type() == TCouplerType::Automatic ) { return false; }
    auto const *neighbour { MoverParameters->Neighbours[ Side ].vehicle };
    if( ( neighbour == nullptr )
     || ( MoverParameters->Neighbours[ Side ].distance > 25.0 ) ) {
        // can only acquire the adapter from a nearby enough vehicle
        return false;
    }
    // TBD: empty struct instead of fallback defaults, to allow vehicles without adapter?
    auto adapterdata {
        coupleradapter_data {
            { 0.085f, 0.95f },
            "tabor/polsprzeg" } };
    if( false == neighbour->m_coupleradapter.model.empty() ) {
        // explicit coupler adapter definition overrides default parameters
        adapterdata = neighbour->m_coupleradapter;
    }
    if( ( MoverParameters->Neighbours[ Side ].distance - adapterdata.position.x < 0.5 )
     && ( false == Enforce ) ) {
        // arbitrary amount of free room required to install the adapter
        // NOTE: this also covers cases with established physical connection
        return false;
    }

    coupler.adapter_type = TCouplerType::Automatic;
    coupler.adapter_length = adapterdata.position.x;
    coupler.adapter_height = adapterdata.position.y;
    // audio flag, visuals update
    coupler.sounds |= sound::attachadapter;
    m_coupleradapters[ Side ] = TModelsManager::GetModel( adapterdata.model );

    return true;
}

bool
TDynamicObject::remove_coupler_adapter( int const Side ) {

    auto &coupler{ MoverParameters->Couplers[ Side ] };

    if( coupler.adapter_type == TCouplerType::NoCoupler ) { return false; }
    // TODO: sanity check(s)
    if( coupler.Connected != nullptr ) {
        // TBD: disallow instead adapter removal if it's coupled with another vehicle?
        uncouple( Side );
    }
    coupler.adapter_type = TCouplerType::NoCoupler;
    coupler.adapter_length = 0.0;
    coupler.adapter_height = 0.0;
    // audio flag, visuals update
    coupler.sounds |= sound::removeadapter;
    m_coupleradapters[ Side ] = nullptr;

    return true;
}

TDynamicObject::TDynamicObject() {
    modelShake = Math3D::vector3(0, 0, 0);
//    fTrackBlock = 10000.0; // brak przeszkody na drodze
    btnOn = false;
    vUp = vWorldUp;
    vFront = vWorldFront;
    vLeft = vWorldLeft;
    iNumAxles = 0;
    MoverParameters = NULL;
    Mechanik = NULL;
    MechInside = false;
    // McZapkie-270202
    Controller = AIdriver;
    bDisplayCab = false; // 030303
//    NextConnected = PrevConnected = NULL;
//    NextConnectedNo = PrevConnectedNo = 2; // ABu: Numery sprzegow. 2=nie podłączony
    bEnabled = true;
    MyTrack = NULL;
    // McZapkie-260202
    dWheelAngle[0] = 0.0;
    dWheelAngle[1] = 0.0;
    dWheelAngle[2] = 0.0;
    // Winger 160204 - pantografy
    // PantVolume = 3.5;
    NoVoltTime = 0;
    mdModel = NULL;
    mdKabina = NULL;
    // smWiazary[0]=smWiazary[1]=NULL;
    smWahacze[0] = smWahacze[1] = smWahacze[2] = smWahacze[3] = NULL;
    fWahaczeAmp = 0;
    smBrakeMode = NULL;
    smLoadMode = NULL;
    mdLoad = NULL;
    mdLowPolyInt = NULL;
    //smMechanik0 = smMechanik1 = NULL;
    smBuforLewy[0] = smBuforLewy[1] = NULL;
    smBuforPrawy[0] = smBuforPrawy[1] = NULL;
    smBogie[0] = smBogie[1] = NULL;
    bogieRot[0] = bogieRot[1] = Math3D::vector3(0, 0, 0);
    modelRot = Math3D::vector3(0, 0, 0);
    cp1 = cp2 = sp1 = sp2 = 0;
    iDirection = 1; // stoi w kierunku tradycyjnym (0, gdy jest odwrócony)
    iAxleFirst = 0; // numer pierwszej osi w kierunku ruchu (przełączenie
    // następuje, gdy osie sa na
    // tym samym torze)
    RaLightsSet(0, 0); // początkowe zerowanie stanu świateł
    // Ra: domyślne ilości animacji dla zgodności wstecz (gdy brak ilości podanych
    // w MMD)
	// ustawienie liczby modeli animowanych podczas konstruowania obiektu a nie na 0
	// prowadzi prosto do wysypów jeśli źle zdefiniowane mmd
    iAnimations = 0; // na razie nie ma żadnego
    pAnimated = NULL;
    fShade = 0.0; // standardowe oświetlenie na starcie
    iHornWarning = 1; // numer syreny do użycia po otrzymaniu sygnału do jazdy
    asDestination = "none"; // stojący nigdzie nie jedzie
    pValveGear = NULL; // Ra: tymczasowo
    iCabs = 0; // maski bitowe modeli kabin
    smBrakeSet = NULL; // nastawa hamulca (wajcha)
    smLoadSet = NULL; // nastawa ładunku (wajcha)
    smWiper = NULL; // wycieraczka (poniekąd też wajcha)
    ctOwner = NULL; // na początek niczyj
    iOverheadMask = 0; // maska przydzielana przez AI pojazdom posiadającym
    // pantograf, aby wymuszały
    // jazdę bezprądową
    tmpTraction.TractionVoltage = 0; // Ra 2F1H: prowizorka, trzeba przechować
    // napięcie, żeby nie wywalało WS pod
    // izolatorem
    fAdjustment = 0.0; // korekcja odległości pomiędzy wózkami (np. na łukach)
}

TDynamicObject::~TDynamicObject() {
    SafeDelete( Mechanik );
    SafeDelete( MoverParameters );
    SafeDeleteArray( pAnimated ); // lista animowanych submodeli
}

void TDynamicObject::place_on_track(TTrack *Track, double fDist, bool Reversed)
{
	for( auto &axle : m_axlesounds ) {
		// wyszukiwanie osi (0 jest na końcu, dlatego dodajemy długość?)
		axle.distance = (
		    Reversed ?
		         -axle.offset :
		        ( axle.offset + MoverParameters->Dim.L ) ) + fDist;
	}
	double fAxleDistHalf = fAxleDist * 0.5;
	// przesuwanie pojazdu tak, aby jego początek był we wskazanym miejcu
	fDist -= 0.5 * MoverParameters->Dim.L; // dodajemy pół długości pojazdu, bo ustawiamy jego środek (zliczanie na minus)
	switch (iNumAxles) {
	    // Ra: pojazdy wstawiane są na tor początkowy, a potem przesuwane
	case 2: // ustawianie osi na torze
		Axle0.Init(Track, this, iDirection ? 1 : -1);
		Axle0.Reset();
		Axle0.Move((iDirection ? fDist : -fDist) + fAxleDistHalf, false);
		Axle1.Init(Track, this, iDirection ? 1 : -1);
		Axle1.Reset();
		Axle1.Move((iDirection ? fDist : -fDist) - fAxleDistHalf, false); // false, żeby nie generować eventów
		break;
	}
	// potrzebne do wyliczenia aktualnej pozycji; nie może być zero, bo nie przeliczy pozycji
	// teraz jeszcze trzeba przypisać pojazdy do nowego toru, bo przesuwanie początkowe osi nie
	// zrobiło tego
	Move( 0.0001 );
	ABuCheckMyTrack(); // zmiana toru na ten, co oś Axle0 (oś z przodu)
}

double
TDynamicObject::Init(std::string Name, // nazwa pojazdu, np. "EU07-424"
                     std::string BaseDir, // z którego katalogu wczytany, np. "PKP/EU07"
                     std::string asReplacableSkin, // nazwa wymiennej tekstury
                     std::string Type_Name, // nazwa CHK/MMD, np. "303E"
                     TTrack *Track, // tor początkowy wstwawienia (początek składu)
                     double fDist, // dystans względem punktu 1
                     std::string DriverType, // typ obsady
                     double fVel, // prędkość początkowa
                     std::string TrainName, // nazwa składu, np. "PE2307" albo Vmax, jeśli pliku nie ma a są cyfry
                     float Load, // ilość ładunku
                     std::string LoadType, // nazwa ładunku
                     bool Reversed, // true, jeśli ma stać odwrotnie w składzie
                     std::string MoreParams // dodatkowe parametry wczytywane w postaci tekstowej
                     )
{ // Ustawienie początkowe pojazdu
    iDirection = (Reversed ? 0 : 1); // Ra: 0, jeśli ma być wstawiony jako obrócony tyłem
    asBaseDir = szDynamicPath + BaseDir + "/"; // McZapkie-310302
    asName = Name;
    std::string asAnimName; // zmienna robocza do wyszukiwania osi i wózków
    // Ra: zmieniamy znaczenie obsady na jednoliterowe, żeby dosadzić kierownika
    if (DriverType == "headdriver")
        DriverType = "1"; // sterujący kabiną +1
    else if (DriverType == "reardriver")
        DriverType = "2"; // sterujący kabiną -1
    else if (DriverType == "passenger")
        DriverType = ""; // legacy type, no longer needed
    else if (DriverType == "nobody")
        DriverType = ""; // nikt nie siedzi

    int Cab = 0; // numer kabiny z obsadą (nie można zająć obu)
    if (DriverType == "1") // od przodu składu
        Cab = 1; // iDirection?1:-1; //iDirection=1 gdy normalnie, =0 odwrotnie
    else if (DriverType == "2") // od tyłu składu
        Cab = -1; // iDirection?-1:1;
/*
    // NOTE: leave passenger in the middle section, this is most likely to be 'passenger' section in MU trains
    else if (DriverType == "p")
    {
        if (Random(6) < 3)
            Cab = 1;
        else
            Cab = -1; // losowy przydział kabiny
    }
*/
    // utworzenie parametrów fizyki
    MoverParameters = new TMoverParameters(iDirection ? fVel : -fVel, Type_Name, asName, Cab);
    // McZapkie: TypeName musi byc nazwą CHK/MMD pojazdu
    if (!MoverParameters->LoadFIZ(asBaseDir))
    { // jak wczytanie CHK się nie uda, to błąd
        if (ConversionError == 666)
            ErrorLog( "Bad vehicle: failed to locate definition file \"" + BaseDir + "/" + Type_Name + ".fiz" + "\"" );
        else {
            ErrorLog( "Bad vehicle: failed to load definition from file \"" + BaseDir + "/" + Type_Name + ".fiz\" (error " + to_string( ConversionError ) + ")" );
        }
        return 0.0; // zerowa długość to brak pojazdu
    }
    // load the cargo now that we know whether the vehicle will allow it
    MoverParameters->AssignLoad( LoadType, Load );
    MoverParameters->ComputeMass();

    bool driveractive = (fVel != 0.0); // jeśli prędkość niezerowa, to aktywujemy ruch
    if (!MoverParameters->CheckLocomotiveParameters(
            driveractive,
            (fVel > 0 ? 1 : -1) * Cab *
                (iDirection ? 1 : -1))) // jak jedzie lub obsadzony to gotowy do drogi
    {
        Error("Parameters mismatch: dynamic object " + asName + " from \"" + BaseDir + "/" + Type_Name + "\"" );
        return 0.0; // zerowa długość to brak pojazdu
    }
    // controller position
    MoverParameters->MainCtrlPos = MoverParameters->MainCtrlNoPowerPos();
    // ustawienie pozycji hamulca
    MoverParameters->LocalBrakePosA = 0.0;
    if (driveractive)
    {
        if (Cab == 0)
            MoverParameters->BrakeCtrlPos =
                static_cast<int>( std::floor(MoverParameters->Handle->GetPos(bh_NP)) );
        else
            MoverParameters->BrakeCtrlPos = static_cast<int>( std::floor(MoverParameters->Handle->GetPos(bh_RP)) );

        // engage independent brake if applicable, if the vehicle is to be on standby
        // NOTE: with more than one driver in the consist this is likely to go awery, since all but one will be sent to sleep
        // TBD, TODO: have the virtual helper release independent brakes during consist check?
        if( DriverType != "" ) {
            if( MoverParameters->LocalBrake != TLocalBrake::ManualBrake ) {
                if( fVel < 1.0 ) {
                    MoverParameters->IncLocalBrakeLevel( LocalBrakePosNo );
					if ( MoverParameters->EIMCtrlEmergency ) {
						MoverParameters->DecLocalBrakeLevel(1);
					}
                }
            }
        }
    }
    else
        MoverParameters->BrakeCtrlPos =
            static_cast<int>( std::floor(MoverParameters->Handle->GetPos(bh_NP)) );

    // poprawienie hamulca po ewentualnym przestawieniu przez Pascal
    // TODO: check if needed, we're not in Pascal anymore, Toto
    MoverParameters->BrakeLevelSet(MoverParameters->BrakeCtrlPos);

    // dodatkowe parametry yB
    MoreParams += "."; // wykonuje o jedną iterację za mało, więc trzeba mu dodać
    // kropkę na koniec
    size_t kropka = MoreParams.find("."); // znajdź kropke
    std::string ActPar; // na parametry
    while (kropka != std::string::npos) // jesli sa kropki jeszcze
    {
        size_t dlugosc = MoreParams.length();
        ActPar = ToUpper(MoreParams.substr(0, kropka)); // pierwszy parametr;
        MoreParams = MoreParams.substr(kropka + 1, dlugosc - kropka); // reszta do dalszej obrobki
        kropka = MoreParams.find(".");

        if (ActPar.substr(0, 1) == "B") // jesli hamulce
        { // sprawdzanie kolejno nastaw
            WriteLog("Wpis hamulca: " + ActPar);
            if ( contains( ActPar, 'G') )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_G);
            }
			if( contains( ActPar, 'P' ) )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_P);
            }
			if( contains( ActPar, 'R' ) )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_R);
            }
			if( contains( ActPar, 'M' ) )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_R);
                MoverParameters->BrakeDelaySwitch(bdelay_R + bdelay_M);
            }
            // wylaczanie hamulca
            if ( contains( ActPar, "<>") ) // wylaczanie na probe hamowania naglego
            {
                MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
            }
            if ( contains( ActPar, '0' ) ) // wylaczanie na sztywno
            {
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
            }
            if ( contains( ActPar, 'E' ) ) // oprozniony
            {
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->Pipe->CreatePress(0);
                MoverParameters->Pipe2->CreatePress(0);
            }
            if ( contains( ActPar, 'Q' ) ) // oprozniony
            {
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->Pipe->CreatePress(0.0);
                MoverParameters->PipePress = 0.0;
                MoverParameters->Pipe2->CreatePress(0.0);
                MoverParameters->ScndPipePress = 0.0;
                MoverParameters->PantVolume = 0.1;
                MoverParameters->PantPress = 0.0;
                MoverParameters->CompressedVolume = 0.0;
            }

            if ( contains( ActPar, '1' ) ) // wylaczanie 10%
            {
                if (Random(10) < 1) // losowanie 1/10
                {
                    MoverParameters->Hamulec->ForceEmptiness();
                    MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
                }
            }
            if ( contains( ActPar, 'X') ) // agonalny wylaczanie 20%, usrednienie przekladni
            {
                if (Random(100) < 20) // losowanie 20/100
                {
                    MoverParameters->Hamulec->ForceEmptiness();
                    MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
                }
                if (MoverParameters->BrakeCylMult[2] * MoverParameters->BrakeCylMult[1] >
                    0.01) // jesli jest nastawiacz mechaniczny PL
                {
                    float rnd = Random(100);
                    if (rnd < 20) // losowanie 20/100         usrednienie
                    {
                        MoverParameters->BrakeCylMult[2] = MoverParameters->BrakeCylMult[1] =
                            (MoverParameters->BrakeCylMult[2] + MoverParameters->BrakeCylMult[1]) /
                            2;
                    }
                    else if (rnd < 70) // losowanie 70/100-20/100    oslabienie
                    {
                        MoverParameters->BrakeCylMult[1] = MoverParameters->BrakeCylMult[1] * 0.50;
                        MoverParameters->BrakeCylMult[2] = MoverParameters->BrakeCylMult[2] * 0.75;
                    }
                    else if (rnd < 80) // losowanie 80/100-70/100    tylko prozny
                    {
                        MoverParameters->BrakeCylMult[2] = MoverParameters->BrakeCylMult[1];
                    }
                    else // tylko ladowny
                    {
                        MoverParameters->BrakeCylMult[1] = MoverParameters->BrakeCylMult[2];
                    }
                }
            }
            // nastawianie ladunku
            if ( contains( ActPar, 'T' ) ) // prozny
            {
                MoverParameters->DecBrakeMult();
                MoverParameters->DecBrakeMult();
            } // dwa razy w dol
            if ( contains( ActPar, 'H' ) ) // ladowny I (dla P-Ł dalej prozny)
            {
                MoverParameters->IncBrakeMult();
                MoverParameters->IncBrakeMult();
                MoverParameters->DecBrakeMult();
            } // dwa razy w gore i obniz
            if ( contains( ActPar, 'F' ) ) // ladowny II
            {
                MoverParameters->IncBrakeMult();
                MoverParameters->IncBrakeMult();
            } // dwa razy w gore
            if ( contains( ActPar, 'N' ) ) // parametr neutralny
            {
            }
        } // koniec hamulce
        else if( ( ActPar.size() >= 3 )
              && ( ActPar.front() == 'W' ) ) {
            // wheel
            ActPar.erase( 0, 1 );

            auto fixedflatsize { 0 };
            auto randomflatsize { 0 };
            auto flatchance { 100 };

            while( false == ActPar.empty() ) {
                // TODO: convert this whole copy and paste mess to something more elegant one day
                switch( ActPar.front() ) {
                    case 'F': {
                        // fixed flat size
                        auto const indexstart { 1 };
                        auto const indexend { ActPar.find_first_not_of( "1234567890", indexstart ) };
                        fixedflatsize = std::atoi( ActPar.substr( indexstart, indexend ).c_str() );
                        ActPar.erase( 0, indexend );
                        break;
                    }
                    case 'R': {
                        // random flat size
                        auto const indexstart { 1 };
                        auto const indexend { ActPar.find_first_not_of( "1234567890", indexstart ) };
                        randomflatsize = std::atoi( ActPar.substr( indexstart, indexend ).c_str() );
                        ActPar.erase( 0, indexend );
                        break;
                    }
                    case 'P': {
                        // random flat probability
                        auto const indexstart { 1 };
                        auto const indexend { ActPar.find_first_not_of( "1234567890", indexstart ) };
                        flatchance = std::atoi( ActPar.substr( indexstart, indexend ).c_str() );
                        ActPar.erase( 0, indexend );
                        break;
                    }
                    case 'H': {
                        // truck hunting
                        auto const indexstart { 1 };
                        auto const indexend { ActPar.find_first_not_of( "1234567890", indexstart ) };
                        auto const huntingchance { std::atoi( ActPar.substr( indexstart, indexend ).c_str() ) };
                        MoverParameters->TruckHunting = ( Random( 0, 100 ) < huntingchance );
                        ActPar.erase( 0, indexend );
                        break;
                    }
                    default: {
                        // unrecognized key
                        ActPar.erase( 0, 1 );
                        break;
                    }
                }
            }
            if( Random( 0, 100 ) <= flatchance ) {
                MoverParameters->WheelFlat += fixedflatsize + Random( 0, randomflatsize );
            }
        } // wheel
        else if( ( ActPar.size() >= 2 )
              && ( ActPar.front() == 'T' ) ) {
            // temperature
            ActPar.erase( 0, 1 );

            auto setambient { false };

            while( false == ActPar.empty() ) {
                switch( ActPar.front() ) {
                    case 'A': {
                        // cold start, set all temperatures to ambient level
                        setambient = true;
                        ActPar.erase( 0, 1 );
                        break;
                    }
                    default: {
                        // unrecognized key
                        ActPar.erase( 0, 1 );
                        break;
                    }
                }
            }
            if( true == setambient ) {
                // TODO: pull ambient temperature from environment data
                MoverParameters->dizel_HeatSet( Global.AirTemperature );
            }
        } // temperature
        else if( ( ActPar.size() >= 2 )
              && ( ActPar.front() == 'L' ) ) {
            // load
            ActPar.erase( 0, 1 );
            // immediately followed by max load override
            // TBD: make it instead an optional sub-parameter?
            {
                auto const indexstart { 0 };
                auto const indexend { ActPar.find_first_not_of( "1234567890", indexstart ) };
                MoverParameters->MaxLoad = std::atoi( ActPar.substr( indexstart, indexend ).c_str() );
                ActPar.erase( 0, indexend );
            }
            while( false == ActPar.empty() ) {
                switch( ActPar.front() ) {
                    default: {
                        // unrecognized key
                        ActPar.erase( 0, 1 );
                        break;
                    }
                }
            }
        } // load
/*        else if (ActPar.substr(0, 1) == "") // tu mozna wpisac inny prefiks i inne rzeczy
        {
            // jakies inne prefiksy
        }
*/
    } // koniec while kropka

    if (MoverParameters->CategoryFlag & 2) // jeśli samochód
    { // ustawianie samochodow na poboczu albo na środku drogi
        if( Track->fTrackWidth < 3.5 ) // jeśli droga wąska
            MoverParameters->OffsetTrackH = 0.0; // to stawiamy na środku, niezależnie od stanu
        // ruchu
        else if( driveractive ) {// od 3.5m do 8.0m jedzie po środku pasa, dla
            // szerszych w odległości
            // 1.5m
            auto const lanefreespace = 0.5 * Track->fTrackWidth - MoverParameters->Dim.W;
            MoverParameters->OffsetTrackH = (
                lanefreespace > 1.5 ?
                -0.5 * MoverParameters->Dim.W - 0.75 : // wide road, keep near centre with safe margin
                ( lanefreespace > 0.1 ?
                    -0.25 * Track->fTrackWidth : // narrow fit, stay on the middle
                    -0.5 * MoverParameters->Dim.W  - 0.075 ) ); // too narrow, just keep some minimal clearance
        }
        else // jak stoi, to kołem na poboczu i pobieramy szerokość razem z
            // poboczem, ale nie z
            // chodnikiem
            MoverParameters->OffsetTrackH =
                -0.5 * (Track->WidthTotal() - MoverParameters->Dim.W) + 0.05;
        iHornWarning = 0; // nie będzie trąbienia po podaniu zezwolenia na jazdę
        if (fDist < 0.0) //-0.5*MoverParameters->Dim.L) //jeśli jest przesunięcie do tyłu
            if (!Track->CurrentPrev()) // a nie ma tam odcinka i trzeba by coś
                // wygenerować
                fDist = -fDist; // to traktujemy, jakby przesunięcie było w drugą stronę
    }

    create_controller( DriverType, !TrainName.empty() );

    ShakeSpring.Init( 125.0 );
    BaseShake = baseshake_config {};
    ShakeState = shake_state {};

    // McZapkie-250202
/*
    iAxles = std::min( MoverParameters->NAxles, MaxAxles ); // ilość osi
*/
    // wczytywanie z pliku nazwatypu.mmd, w tym model
    erase_extension( asReplacableSkin );
    LoadMMediaFile(Type_Name, asReplacableSkin);
    // McZapkie-100402: wyszukiwanie submodeli sprzegów
    btCoupler1.Init( "coupler1", mdModel ); // false - ma być wyłączony
    btCoupler2.Init( "coupler2", mdModel );
    // brake hoses
    btCPneumatic1.Init("cpneumatic1", mdModel);
    btCPneumatic2.Init("cpneumatic2", mdModel);
    btCPneumatic1r.Init("cpneumatic1r", mdModel);
    btCPneumatic2r.Init("cpneumatic2r", mdModel);
    // main hoses
    btPneumatic1.Init("pneumatic1", mdModel);
    btPneumatic2.Init("pneumatic2", mdModel);
    btPneumatic1r.Init("pneumatic1r", mdModel);
    btPneumatic2r.Init("pneumatic2r", mdModel);
    // control cables
    btCCtrl1.Init( "cctrl1", mdModel, false);
    btCCtrl2.Init( "cctrl2", mdModel, false);
    // gangways
    btCPass1.Init( "cpass1", mdModel, false);
    btCPass2.Init( "cpass2", mdModel, false);
    // sygnaly
    m_endsignal12.Init( "endsignal12", mdModel, false );
    m_endsignal13.Init( "endsignal13", mdModel, false );
    m_endsignal22.Init( "endsignal22", mdModel, false );
    m_endsignal23.Init( "endsignal23", mdModel, false );
    m_endsignals1.Init( "endsignals1", mdModel, false );
    m_endsignals2.Init( "endsignals2", mdModel, false );
    m_endtab1.Init( "endtab1", mdModel, false );
    m_endtab2.Init( "endtab2", mdModel, false );
    m_headlamp11.Init( "headlamp11", mdModel ); // górne
    m_headlamp12.Init( "headlamp12", mdModel ); // prawe
    m_headlamp13.Init( "headlamp13", mdModel ); // lewe
    m_headlamp21.Init( "headlamp21", mdModel );
    m_headlamp22.Init( "headlamp22", mdModel );
    m_headlamp23.Init( "headlamp23", mdModel );
    m_headsignal12.Init( "headsignal12", mdModel );
    m_headsignal13.Init( "headsignal13", mdModel );
    m_headsignal22.Init( "headsignal22", mdModel );
    m_headsignal23.Init( "headsignal23", mdModel );
    // informacja, czy ma poszczególne światła
    iInventory[ end::front ] |= m_endsignal12.Active() ? light::redmarker_right : 0;
    iInventory[ end::front ] |= m_endsignal13.Active() ? light::redmarker_left : 0;
    iInventory[ end::rear ]  |= m_endsignal22.Active() ? light::redmarker_right : 0;
    iInventory[ end::rear ]  |= m_endsignal23.Active() ? light::redmarker_left : 0;
    iInventory[ end::front ] |= m_endsignals1.Active() ? ( light::redmarker_left | light::redmarker_right ) : 0;
    iInventory[ end::rear ]  |= m_endsignals2.Active() ? ( light::redmarker_left | light::redmarker_right ) : 0;
    iInventory[ end::front ] |= m_endtab1.Active() ? light::rearendsignals : 0; // tabliczki blaszane
    iInventory[ end::rear ]  |= m_endtab2.Active() ? light::rearendsignals : 0;
    iInventory[ end::front ] |= m_headlamp11.Active() ? light::headlight_upper : 0;
    iInventory[ end::front ] |= m_headlamp12.Active() ? light::headlight_right : 0;
    iInventory[ end::front ] |= m_headlamp13.Active() ? light::headlight_left : 0;
    iInventory[ end::rear ]  |= m_headlamp21.Active() ? light::headlight_upper : 0;
    iInventory[ end::rear ]  |= m_headlamp22.Active() ? light::headlight_right : 0;
    iInventory[ end::rear ]  |= m_headlamp23.Active() ? light::headlight_left : 0;
    iInventory[ end::front ] |= m_headsignal12.Active() ? light::auxiliary_right : 0;
    iInventory[ end::front ] |= m_headsignal13.Active() ? light::auxiliary_left : 0;
    iInventory[ end::rear ]  |= m_headsignal22.Active() ? light::auxiliary_right : 0;
    iInventory[ end::rear ]  |= m_headsignal23.Active() ? light::auxiliary_left : 0;
    btMechanik1.Init( "mechanik1", mdLowPolyInt, false);
	btMechanik2.Init( "mechanik2", mdLowPolyInt, false);
    if( MoverParameters->dizel_heat.water.config.shutters ) {
        btShutters1.Init( "shutters1", mdModel, false );
    }
    if( MoverParameters->dizel_heat.water_aux.config.shutters ) {
        btShutters2.Init( "shutters2", mdModel, false );
    }
    TurnOff(); // resetowanie zmiennych submodeli

    if( mdLowPolyInt != nullptr ) {
        // check the low poly interior for potential compartments of interest, ie ones which can be individually lit
        // TODO: definition of relevant compartments in the .mmd file
        TSubModel *submodel { nullptr };
        if( ( submodel = mdLowPolyInt->GetFromName( "cab0" ) ) != nullptr ) {
            Sections.push_back( { submodel, nullptr, 0, 0.0f } );
            LowPolyIntCabs[ 0 ] = submodel;
        }
        if( ( submodel = mdLowPolyInt->GetFromName( "cab1" ) ) != nullptr ) {
            Sections.push_back( { submodel, nullptr, 0, 0.0f } );
            LowPolyIntCabs[ 1 ] = submodel;
        }
        if( ( submodel = mdLowPolyInt->GetFromName( "cab2" ) ) != nullptr ) {
            Sections.push_back( { submodel, nullptr, 0, 0.0f } );
            LowPolyIntCabs[ 2 ] = submodel;
        }
        // passenger car compartments
        std::vector<std::string> nameprefixes = { "corridor", "korytarz", "compartment", "przedzial" };
        for( auto const &nameprefix : nameprefixes ) {
            init_sections( mdLowPolyInt, nameprefix, MoverParameters->CompartmentLights.start_type == start_t::manual );
        }
    }
    // destination sign
    if( mdModel ) {
        init_destination( mdModel );
    }
    // 'external_load' is an optional special section in the main model, pointing to submodel of external load
    if( mdModel ) {
        init_sections( mdModel, "external_load", false );
    }
    update_load_sections();
    update_load_visibility();
    // wyszukiwanie zderzakow
    if( mdModel ) {
        // jeśli ma w czym szukać
        for( int i = 0; i < 2; i++ ) {
            asAnimName = std::string( "buffer_left0" ) + to_string( i + 1 );
            smBuforLewy[ i ] = mdModel->GetFromName( asAnimName );
            if( smBuforLewy[ i ] )
                smBuforLewy[ i ]->WillBeAnimated(); // ustawienie flagi animacji
            asAnimName = std::string( "buffer_right0" ) + to_string( i + 1 );
            smBuforPrawy[ i ] = mdModel->GetFromName( asAnimName );
            if( smBuforPrawy[ i ] )
                smBuforPrawy[ i ]->WillBeAnimated();
        }
    }
    if( Track->fSoundDistance > 0.f ) {
        for( auto &axle : m_axlesounds ) {
            // wyszukiwanie osi (0 jest na końcu, dlatego dodajemy długość?)
            axle.distance =
                clamp_circular<double>(
                    ( Reversed ?
                        -axle.offset :
                         axle.offset )
                        - 0.5 * MoverParameters->Dim.L
                        + fDist,
                    Track->fSoundDistance );
        }
    }
    // McZapkie-250202 end.
    Track->AddDynamicObject(this); // wstawiamy do toru na pozycję 0, a potem przesuniemy
    // McZapkie: zmieniono na ilosc osi brane z chk
    // iNumAxles=(MoverParameters->NAxles>3 ? 4 : 2 );
    initial_track = MyTrack;
    iNumAxles = 2;
    // McZapkie-090402: odleglosc miedzy czopami skretu lub osiami
    fAxleDist = clamp(
            std::max( MoverParameters->BDist, MoverParameters->ADist ),
            0.2, //żeby się dało wektory policzyć
            MoverParameters->Dim.L - 0.2 ); // nie mogą być za daleko bo będzie "walenie w mur"
    double fAxleDistHalf = fAxleDist * 0.5;
    // przesuwanie pojazdu tak, aby jego początek był we wskazanym miejcu
    fDist -= 0.5 * MoverParameters->Dim.L; // dodajemy pół długości pojazdu, bo ustawiamy jego środek (zliczanie na minus)
    switch (iNumAxles) {
        // Ra: pojazdy wstawiane są na tor początkowy, a potem przesuwane
    case 2: // ustawianie osi na torze
        Axle0.Init(Track, this, iDirection ? 1 : -1);
        Axle0.Move((iDirection ? fDist : -fDist) + fAxleDistHalf, false);
        Axle1.Init(Track, this, iDirection ? 1 : -1);
        Axle1.Move((iDirection ? fDist : -fDist) - fAxleDistHalf, false); // false, żeby nie generować eventów
        break;
    case 4:
        Axle0.Init(Track, this, iDirection ? 1 : -1);
        Axle0.Move((iDirection ? fDist : -fDist) + (fAxleDistHalf + MoverParameters->ADist * 0.5), false);
        Axle1.Init(Track, this, iDirection ? 1 : -1);
        Axle1.Move((iDirection ? fDist : -fDist) - (fAxleDistHalf + MoverParameters->ADist * 0.5), false);
        // Axle2.Init(Track,this,iDirection?1:-1);
        // Axle2.Move((iDirection?fDist:-fDist)-(fAxleDistHalf-MoverParameters->ADist*0.5),false);
        // Axle3.Init(Track,this,iDirection?1:-1);
        // Axle3.Move((iDirection?fDist:-fDist)+(fAxleDistHalf-MoverParameters->ADist*0.5),false);
        break;
    }
    // potrzebne do wyliczenia aktualnej pozycji; nie może być zero, bo nie przeliczy pozycji
    // teraz jeszcze trzeba przypisać pojazdy do nowego toru, bo przesuwanie początkowe osi nie
    // zrobiło tego
    Move( 0.0001 );
    ABuCheckMyTrack(); // zmiana toru na ten, co oś Axle0 (oś z przodu)
    // Ra: ustawienie pozycji do obliczania sprzęgów
    MoverParameters->Loc = {
        -vPosition.x,
         vPosition.z,
         vPosition.y }; // normalnie przesuwa ComputeMovement() w Update()
    // ABuWozki 060504
    if (mdModel) // jeśli ma w czym szukać
    {
        smBogie[0] = mdModel->GetFromName("bogie1"); // Ra: bo nazwy są małymi
        smBogie[1] = mdModel->GetFromName("bogie2");
        if (!smBogie[0])
            smBogie[0] = mdModel->GetFromName("boogie01"); // Ra: alternatywna nazwa
        if (!smBogie[1])
            smBogie[1] = mdModel->GetFromName("boogie02"); // Ra: alternatywna nazwa
        if (smBogie[0])
            smBogie[0]->WillBeAnimated();
        if (smBogie[1])
            smBogie[1]->WillBeAnimated();
    }
    // ABu: zainicjowanie zmiennej, zeby nic sie nie ruszylo w pierwszej klatce,
    // potem juz liczona prawidlowa wartosc masy
    MoverParameters->ComputeConstans();
    // wektor podłogi dla wagonów, przesuwa ładunek
    vFloor = Math3D::vector3(0, 0, MoverParameters->Floor);

    // długość większa od zera oznacza OK; 2mm docisku?
    return MoverParameters->Dim.L;
}

int
TDynamicObject::init_sections( TModel3d const *Model, std::string const &Nameprefix, bool const Overrideselfillum ) {

    auto sectioncount = 0;
    auto sectionindex = 0;
    TSubModel *sectionsubmodel { nullptr };

    do {
        // section names for index < 10 match either prefix0X or prefixX
        // section names above 10 match prefixX
        auto const sectionindexname { std::to_string( sectionindex ) };
        sectionsubmodel = Model->GetFromName( Nameprefix + sectionindexname );
        if( ( sectionsubmodel == nullptr )
         && ( sectionindex < 10 ) ) {
            sectionsubmodel = Model->GetFromName( Nameprefix + "0" + sectionindexname );
        }
        if( sectionsubmodel != nullptr ) {
            // HACK: disable automatic self-illumination threshold, at least until 3d model update
            if( Overrideselfillum ) {
                sectionsubmodel->SetSelfIllum( 2.0f, true, false );
            }
            Sections.push_back( {
                sectionsubmodel,
                nullptr, // pointers to load sections are generated afterwards
                0,
                0.0f } );
            ++sectioncount;
        }
        ++sectionindex;
    } while( ( sectionsubmodel != nullptr )
          || ( sectionindex < 2 ) ); // chain can start from prefix00 or prefix01

    return sectioncount;
}

bool
TDynamicObject::init_destination( TModel3d *Model ) {

    if( Model->GetSMRoot() == nullptr ) { return false; }

    std::tie( DestinationSign.sign, DestinationSign.has_light ) = Model->GetSMRoot()->find_replacable4();

    return DestinationSign.sign != nullptr;
}

void
TDynamicObject::create_controller( std::string const Type, bool const Trainset ) {

    if( Type == "" ) { return; }

    // McZapkie-151102: rozkład jazdy czytany z pliku *.txt z katalogu w którym jest sceneria
    if( ( Type == "1" )
     || ( Type == "2" ) ) {
        // McZapkie-110303: mechanik i rozklad tylko gdy jest obsada
        Mechanik = new TController( Controller, this, Aggressive );

        if( false == Trainset ) {
            // jeśli nie w składzie
            // załączenie rozrządu (wirtualne kabiny) itd.
            Mechanik->DirectionInitial();
            // tryb pociągowy z ustaloną prędkością (względem sprzęgów)
            Mechanik->PutCommand(
                "Timetable:",
                MoverParameters->V * 3.6 * ( iDirection ? -1.0 : 1.0 ),
                0,
                nullptr );
        }
    }
    else if( Type == "p" ) {
        // obserwator w charakterze pasażera
        // Ra: to jest niebezpieczne, bo w razie co będzie pomagał hamulcem bezpieczeństwa
        Mechanik = new TController( Controller, this, Easyman, false );
    }
}

void TDynamicObject::FastMove(double fDistance)
{
    MoverParameters->dMoveLen = MoverParameters->dMoveLen + fDistance;
}

void TDynamicObject::Move(double fDistance)
{ // przesuwanie pojazdu po
    // trajektorii polega na
    // przesuwaniu poszczególnych osi
    // Ra: wartość prędkości 2km/h ma ograniczyć aktywację eventów w przypadku
    // drgań
    if (Axle0.GetTrack() == Axle1.GetTrack()) // przed przesunięciem
    { // powiązanie pojazdu z osią można zmienić tylko wtedy, gdy skrajne osie są
        // na tym samym torze
        if (MoverParameters->Vel >
            2) //|[km/h]| nie ma sensu zmiana osi, jesli pojazd drga na postoju
            iAxleFirst = (MoverParameters->V >= 0.0) ?
                             1 :
                             0; //[m/s] ?1:0 - aktywna druga oś w kierunku jazdy
        // aktualnie eventy aktywuje druga oś, żeby AI nie wyłączało sobie semafora
        // za szybko
    }
    if (fDistance > 0.0)
    { // gdy ruch w stronę sprzęgu 0, doliczyć korektę do osi 1
        bEnabled &= Axle0.Move(fDistance, iAxleFirst == 0); // oś z przodu pojazdu
        bEnabled &= Axle1.Move(fDistance /*-fAdjustment*/, iAxleFirst != 0); // oś z tyłu pojazdu
    }
    else if (fDistance < 0.0)
    { // gdy ruch w stronę sprzęgu 1, doliczyć korektę do osi 0
        bEnabled &= Axle1.Move(fDistance, iAxleFirst != 0); // oś z tyłu pojazdu prusza się pierwsza
        bEnabled &= Axle0.Move(fDistance /*-fAdjustment*/, iAxleFirst == 0); // oś z przodu pojazdu
    }
    else // gf: bez wywolania Move na postoju nie ma event0
    {
        bEnabled &= Axle1.Move(fDistance, iAxleFirst != 0); // oś z tyłu pojazdu prusza się pierwsza
        bEnabled &= Axle0.Move(fDistance, iAxleFirst == 0); // oś z przodu pojazdu
    }
    if (fDistance != 0.0) // nie liczyć ponownie, jeśli stoi
    { // liczenie pozycji pojazdu tutaj, bo jest używane w wielu miejscach
        vPosition = 0.5 * (Axle1.pPosition + Axle0.pPosition); //środek między skrajnymi osiami
        vFront = Axle0.pPosition - Axle1.pPosition; // wektor pomiędzy skrajnymi osiami
        // Ra 2F1J: to nie jest stabilne (powoduje rzucanie taborem) i wymaga
        // dopracowania
        fAdjustment = vFront.Length() - fAxleDist; // na łuku będzie ujemny
        // if (fabs(fAdjustment)>0.02) //jeśli jest zbyt dużo, to rozłożyć na kilka przeliczeń (wygasza drgania?)
        //{//parę centymetrów trzeba by już skorygować; te błędy mogą się też
        // generować na ostrych łukach
        // fAdjustment*=0.5; //w jednym kroku korygowany jest ułamek błędu
        //}
        // else
        // fAdjustment=0.0;
        vFront = Normalize(vFront); // kierunek ustawienia pojazdu (wektor jednostkowy)
        vLeft = Normalize(CrossProduct(vWorldUp, vFront)); // wektor poziomy w lewo,
        // normalizacja potrzebna z powodu pochylenia (vFront)
        vUp = CrossProduct(vFront, vLeft); // wektor w górę, będzie jednostkowy
        modelRot.z = atan2(-vFront.x, vFront.z); // kąt obrotu pojazdu [rad]; z ABuBogies()
        auto const roll { Roll() }; // suma przechyłek
        if (roll != 0.0)
        { // wyznaczanie przechylenia tylko jeśli jest przechyłka
            // można by pobrać wektory normalne z toru...
            mMatrix.Identity(); // ta macierz jest potrzebna głównie do wyświetlania
            mMatrix.Rotation(roll * 0.5, vFront); // obrót wzdłuż osi o przechyłkę
            vUp = mMatrix * vUp; // wektor w górę pojazdu (przekręcenie na przechyłce)
            // vLeft=mMatrix*DynamicObject->vLeft;
            // vUp=CrossProduct(vFront,vLeft); //wektor w górę
            // vLeft=Normalize(CrossProduct(vWorldUp,vFront)); //wektor w lewo
            vLeft = Normalize(CrossProduct(vUp, vFront)); // wektor w lewo
            // vUp=CrossProduct(vFront,vLeft); //wektor w górę
        }
        mMatrix.Identity(); // to też można by od razu policzyć, ale potrzebne jest do wyświetlania
        mMatrix.BasisChange(vLeft, vUp, vFront); // przesuwanie jest jednak rzadziej niż renderowanie
        mMatrix = Inverse(mMatrix); // wyliczenie macierzy dla pojazdu (potrzebna tylko do wyświetlania?)
        // if (MoverParameters->CategoryFlag&2)
        { // przesunięcia są używane po wyrzuceniu pociągu z toru
            vPosition.x += MoverParameters->OffsetTrackH * vLeft.x; // dodanie przesunięcia w bok
            vPosition.z += MoverParameters->OffsetTrackH * vLeft.z; // vLeft jest wektorem poprzecznym
            // if () na przechyłce będzie dodatkowo zmiana wysokości samochodu
            vPosition.y += MoverParameters->OffsetTrackV; // te offsety są liczone przez moverparam
        }
        // obliczanie pozycji sprzęgów do liczenia zderzeń
        auto dir = (0.5 * MoverParameters->Dim.L) * vFront; // wektor sprzęgu
        vCoulpler[end::front] = vPosition + dir; // współrzędne sprzęgu na początku
        vCoulpler[end::rear] = vPosition - dir; // współrzędne sprzęgu na końcu
        // bCameraNear=
        // if (bCameraNear) //jeśli istotne są szczegóły (blisko kamery)
        { // przeliczenie cienia
            TTrack *t0 = Axle0.GetTrack(); // już po przesunięciu
            TTrack *t1 = Axle1.GetTrack();
            if ((t0->eEnvironment == e_flat) && (t1->eEnvironment == e_flat)) // może być e_bridge...
                fShade = 0.0; // standardowe oświetlenie
            else
            { // jeżeli te tory mają niestandardowy stopień zacienienia
                // (e_canyon, e_tunnel)
                if (t0->eEnvironment == t1->eEnvironment)
                {
                    switch (t0->eEnvironment)
                    { // typ zmiany oświetlenia
                    case e_canyon:
                        fShade = 0.65f;
                        break; // zacienienie w kanionie
                    case e_tunnel:
                        fShade = 0.20f;
                        break; // zacienienie w tunelu
                    }
                }
                else // dwa różne
                { // liczymy proporcję
                    double d = Axle0.GetTranslation(); // aktualne położenie na torze
                    if (Axle0.GetDirection() < 0)
                        d = t0->Length() - d; // od drugiej strony liczona długość
                    d /= fAxleDist; // rozsataw osi procentowe znajdowanie się na torze

                    float shadefrom = 1.0f, shadeto = 1.0f;
                    // NOTE, TODO: calculating brightness level is used enough times to warrant encapsulation into a function
                    switch( t0->eEnvironment ) {
                        case e_canyon: { shadeto = 0.65f; break; }
                        case e_tunnel: { shadeto = 0.2f; break; }
                        default: {break; }
                    }
                    switch( t1->eEnvironment ) {
                        case e_canyon: { shadefrom = 0.65f; break; }
                        case e_tunnel: { shadefrom = 0.2f; break; }
                        default: {break; }
                    }
                    fShade = interpolate( shadefrom, shadeto, static_cast<float>( d ) );
/*
                    switch (t0->eEnvironment)
                    { // typ zmiany oświetlenia - zakładam, że
                    // drugi tor ma e_flat
                    case e_canyon:
                        fShade = (d * 0.65) + (1.0 - d);
                        break; // zacienienie w kanionie
                    case e_tunnel:
                        fShade = (d * 0.20) + (1.0 - d);
                        break; // zacienienie w tunelu
                    }
                    switch (t1->eEnvironment)
                    { // typ zmiany oświetlenia - zakładam, że
                    // pierwszy tor ma e_flat
                    case e_canyon:
                        fShade = d + (1.0 - d) * 0.65;
                        break; // zacienienie w kanionie
                    case e_tunnel:
                        fShade = d + (1.0 - d) * 0.20;
                        break; // zacienienie w tunelu
                    }
*/
                }
            }
        }
    }
};

void TDynamicObject::AttachNext(TDynamicObject *Object, int iType)
{ // Ra: doczepia Object na końcu składu (nazwa funkcji może być myląca)
    // Ra: używane tylko przy wczytywaniu scenerii
    auto const vehicleend { iDirection };
    auto const othervehicleend { Object->iDirection ^ 1 };

    MoverParameters->Attach( vehicleend, othervehicleend, Object->MoverParameters, iType, true, false );
    // update neighbour data for both affected vehicles
    update_neighbours();
    Object->update_neighbours();

    // potentially attach automatic coupler adapter to allow the connection
    // HACK: we're doing it after establishin actual connection, as the method needs valid neighbour data
    auto &coupler { MoverParameters->Couplers[ vehicleend ] };
    auto &othercoupler { Object->MoverParameters->Couplers[ ( othervehicleend != 2 ? othervehicleend : coupler.ConnectedNr ) ] };

    if( coupler.type() != othercoupler.type() ) {
        if( othercoupler.type() == TCouplerType::Automatic ) {
            // try to attach adapter to the vehicle
            attach_coupler_adapter(
                vehicleend,
                true );
        }
        else if( coupler.type() == TCouplerType::Automatic ) {
            // try to attach adapter to the other vehicle
            Object->attach_coupler_adapter(
                ( othervehicleend != 2 ? othervehicleend : coupler.ConnectedNr ),
                true );
        }
        // update distance to neighbours on account of potentially attached adapter
        update_neighbours();
        Object->update_neighbours();
    }
    // potentially adjust vehicle position to avoid collision at the simulation start
    if( MoverParameters->Neighbours[ vehicleend ].distance > -0.001 ) { return; }

    Object->Move( MoverParameters->Neighbours[ vehicleend ].distance * Object->DirectionGet() );
    // HACK: manually update vehicle position as it's used by neighbour distance update we do next
    Object->MoverParameters->Loc = {
        -Object->vPosition.x,
         Object->vPosition.z,
         Object->vPosition.y };
    // update neighbour distance data after moving our vehicle
    update_neighbours();
    Object->update_neighbours();
}

bool TDynamicObject::UpdateForce(double dt)
{
    if (!bEnabled)
        return false;
    if( dt > 0 ) {
        // wywalenie WS zależy od ustawienia kierunku
        MoverParameters->ComputeTotalForce( dt );
    }
    return true;
}

// initiates load change by specified amounts, with a platform on specified side
void TDynamicObject::LoadExchange( int const Disembark, int const Embark, int const Platforms ) {
/*
    if( ( MoverParameters->Doors.open_control == control_t::passenger )
     || ( MoverParameters->Doors.open_control == control_t::mixed ) ) {
        // jeśli jedzie do tyłu, to drzwi otwiera odwrotnie
        auto const lewe = ( DirectionGet() > 0 ) ? 1 : 2;
        auto const prawe = 3 - lewe;
        if( Platform & lewe ) {
            MoverParameters->OperateDoors( side::left, true, range_t::local );
        }
        if( Platform & prawe ) {
            MoverParameters->OperateDoors( side::right, true, range_t::local );
        }
    }
*/
    if( Platforms == 0 ) { return; } // edge case, if there's no accessible platforms discard the request

    m_exchange.unload_count += Disembark;
    m_exchange.load_count += Embark;
    m_exchange.platforms = Platforms;
    m_exchange.time = 0.0;
}

// calculates time needed to complete current load change
float TDynamicObject::LoadExchangeTime( int const Platforms ) {

    m_exchange.platforms = Platforms;
    return LoadExchangeTime();
}

float TDynamicObject::LoadExchangeTime() const {

    if( ( m_exchange.unload_count < 0.01 ) && ( m_exchange.load_count < 0.01 ) ) { return 0.f; }

    auto const baseexchangetime { m_exchange.unload_count / MoverParameters->UnLoadSpeed + m_exchange.load_count / MoverParameters->LoadSpeed };
    auto const nominalexchangespeedfactor { ( m_exchange.platforms == 3 ? 2.f : 1.f ) };
    auto const actualexchangespeedfactor { LoadExchangeSpeed() };

    return baseexchangetime / ( actualexchangespeedfactor > 0.f ? actualexchangespeedfactor : nominalexchangespeedfactor );
}

// calculates current load exchange rate
float TDynamicObject::LoadExchangeSpeed() const {
    // platforms (1:left, 2:right, 3:both)
    // with exchange performed on both sides waiting times are halved
    auto exchangespeedfactor { 0.f };
    auto const lewe { ( DirectionGet() > 0 ) ? 1 : 2 };
    auto const prawe { 3 - lewe };
    if( m_exchange.platforms & lewe ) {
        exchangespeedfactor += ( MoverParameters->Doors.instances[side::left].is_open ? 1.f : 0.f );
    }
    if( m_exchange.platforms & prawe ) {
        exchangespeedfactor += ( MoverParameters->Doors.instances[ side::right ].is_open ? 1.f : 0.f );
    }

    return exchangespeedfactor;
}

// update state of load exchange operation
void TDynamicObject::update_exchange( double const Deltatime ) {

    // TODO: move offset calculation after initial check, when the loading system is all unified
    update_load_offset();

    if( ( m_exchange.unload_count < 0.01 ) && ( m_exchange.load_count < 0.01 ) ) { return; }

    if( MoverParameters->Vel < 2.0 ) {

        auto const exchangespeed { LoadExchangeSpeed() };

        if( exchangespeed < ( m_exchange.platforms == 3 ? 2.f : 1.f ) ) {
            // the exchange isn't performed at optimal rate, or at all. try to open viable vehicle doors to speed it up
            auto const lewe { ( DirectionGet() > 0 ) ? 1 : 2 };
            auto const prawe { 3 - lewe };
            if( ( m_exchange.platforms & lewe )
             && ( false == (
                 MoverParameters->Doors.instances[side::left].is_open
              || MoverParameters->Doors.instances[side::left].is_opening ) ) ) {
                // try to open left door
                MoverParameters->OperateDoors( side::left, true, range_t::local );
            }
            if( ( m_exchange.platforms & prawe )
             && ( false == (
                 MoverParameters->Doors.instances[side::right].is_open
              || MoverParameters->Doors.instances[side::right].is_opening ) ) ) {
                // try to open right door
                MoverParameters->OperateDoors( side::right, true, range_t::local );
            }
        }

        if( exchangespeed > 0.f ) {

            m_exchange.time += Deltatime;
            while( ( m_exchange.unload_count > 0.01 )
                && ( m_exchange.time >= 1.0 ) ) {

                m_exchange.time -= 1.0;
                auto const exchangesize = std::min( m_exchange.unload_count, MoverParameters->UnLoadSpeed * exchangespeed );
                m_exchange.unload_count -= exchangesize;
                MoverParameters->LoadStatus = 1;
                MoverParameters->LoadAmount = std::max( 0.f, MoverParameters->LoadAmount - exchangesize );
                MoverParameters->ComputeMass();
                update_load_visibility();
            }
            if( m_exchange.unload_count < 0.01 ) {
                // finish any potential unloading operation before adding new load
                // don't load more than can fit
                m_exchange.load_count = std::min( m_exchange.load_count, MoverParameters->MaxLoad - MoverParameters->LoadAmount );
                while( ( m_exchange.load_count > 0.01 )
                    && ( m_exchange.time >= 1.0 ) ) {

                    m_exchange.time -= 1.0;
                    auto const exchangesize = std::min( m_exchange.load_count, MoverParameters->LoadSpeed * exchangespeed );
                    m_exchange.load_count -= exchangesize;
                    MoverParameters->LoadStatus = 2;
                    MoverParameters->LoadAmount = std::min( MoverParameters->MaxLoad, MoverParameters->LoadAmount + exchangesize ); // std::max not strictly needed but, eh
                    MoverParameters->ComputeMass();
                    update_load_visibility();
                }
            }
        }
    }

    if( MoverParameters->Vel > 2.0 ) {
        // if the vehicle moves too fast cancel the exchange
        m_exchange.unload_count = 0;
        m_exchange.load_count = 0;
    }

    if( ( m_exchange.unload_count < 0.01 )
     && ( m_exchange.load_count < 0.01 ) ) {

        MoverParameters->LoadStatus = 4;
        // if the exchange is completed (or canceled) close the door, if applicable
        if( ( MoverParameters->Doors.close_control == control_t::passenger )
         || ( MoverParameters->Doors.close_control == control_t::mixed ) ) {

            if( ( MoverParameters->Vel > 2.0 )
             || ( Random() < (
                 // remotely controlled door are more likely to be left open
                 MoverParameters->Doors.close_control == control_t::passenger ?
                        0.75 :
                        0.50 ) ) ) {

                MoverParameters->OperateDoors( side::left, false, range_t::local );
                MoverParameters->OperateDoors( side::right, false, range_t::local );
            }
        }
        // if the vehicle was emptied potentially switch load visualization model
        if( MoverParameters->LoadAmount == 0 ) {
            MoverParameters->AssignLoad( "" );
        }
    }
}

void TDynamicObject::LoadUpdate() {

    MoverParameters->LoadStatus &= ( 1 | 2 ); // po zakończeniu będzie równe zero
    // przeładowanie modelu ładunku
    if( MoverParameters->LoadTypeChange ) {
        // whether we succeed or not don't try more than once
        MoverParameters->LoadTypeChange = false;
        // bieżąca ścieżka do tekstur to dynamic/...
        Global.asCurrentTexturePath = asBaseDir;
        mdLoad = LoadMMediaFile_mdload( MoverParameters->LoadType.name );
        // TODO: discern from vehicle component which merely uses vehicle directory and has no animations, so it can be initialized outright
        // and actual vehicles which get their initialization after their animations are set up
        if( mdLoad != nullptr ) {
            mdLoad->Init();
        }
        // update bindings between lowpoly sections and potential load chunks placed inside them
        update_load_sections();
        // z powrotem defaultowa sciezka do tekstur
        Global.asCurrentTexturePath = std::string( szTexturePath );
    }
}

void
TDynamicObject::update_load_sections() {

    SectionLoadOrder.clear();

    for( auto &section : Sections ) {

        section.load = GetSubmodelFromName( mdLoad,  section.compartment->pName );

        if( section.load != nullptr ) {
            // create entry for each load model chunk assigned to the section
            // TBD, TODO: store also pointer to chunk submodel and control its visibility more directly, instead of per-section visibility flag?
            auto loadchunkcount { section.load->count_children() };
            while( loadchunkcount-- ) {
                SectionLoadOrder.push_back( &section );
            }
            // HACK: disable automatic self-illumination threshold, at least until 3d model update
            if( MoverParameters->CompartmentLights.start_type == start_t::manual ) {
                section.load->SetSelfIllum( 2.0f, true, false );
            }
        }
    }
    shuffle_load_order();
}

void
TDynamicObject::update_load_visibility() {
    // start with clean load chunk visibility slate
    for( auto &section : Sections ) {
        section.load_chunks_visible = 0;
    }
    // each entry in load order sequence matches a single chunk of the section it points to
    // the length of load order sequence matches total number of load chunks
    auto const loadpercentage { (
        MoverParameters->MaxLoad == 0.f ?
            0.0 :
            MoverParameters->LoadAmount / MoverParameters->MaxLoad ) };
    auto visiblechunkcount { (
        SectionLoadOrder.empty() ?
            0 :
            static_cast<int>( std::ceil( loadpercentage * SectionLoadOrder.size() - 0.001f ) ) ) };
    for( auto *section : SectionLoadOrder ) {
        if( visiblechunkcount == 0 ) { break; }
        section->load_chunks_visible++;
        --visiblechunkcount;
    }
}

void
TDynamicObject::update_load_offset() {

    if( MoverParameters->LoadType.offset_min == 0.f ) { return; }

    auto const loadpercentage { (
        MoverParameters->MaxLoad == 0.f ?
            0.0 :
            100.0 * MoverParameters->LoadAmount / MoverParameters->MaxLoad ) };

    LoadOffset = interpolate( MoverParameters->LoadType.offset_min, 0.f, clamp( 0.0, 1.0, loadpercentage * 0.01 ) );
}

void
TDynamicObject::shuffle_load_order() {

    std::shuffle( std::begin( SectionLoadOrder ), std::end( SectionLoadOrder ), Global.random_engine );
    // shift chunks assigned to corridors to the end of the list, so they show up last
    std::stable_partition(
        std::begin( SectionLoadOrder ), std::end( SectionLoadOrder ),
        []( vehicle_section const *section ) {
            return (
                ( section->compartment->pName.find( "compartment" ) == 0 )
             || ( section->compartment->pName.find( "przedzial" )   == 0 ) ); } );
    // NOTE: potentially we're left with a mix of corridor and external section loads
    // but that's not necessarily a wrong outcome, so we leave it this way for the time being
}

/*
Ra:
  Powinny być dwie funkcje wykonujące aktualizację fizyki. Jedna wykonująca
krok obliczeń, powtarzana odpowiednią liczbę razy, a druga wykonująca zbiorczą
aktualzację mniej istotnych elementów.
  Ponadto należało by ustalić odległość składów od kamery i jeśli przekracza
ona np. 10km, to traktować składy jako uproszczone, np. bez wnikania w siły
na sprzęgach, opóźnienie działania hamulca itp. Oczywiście musi mieć to pewną
histerezę czasową, aby te tryby pracy nie przełączały się zbyt szybko.
*/

void TDynamicObject::update_destinations() {

    if( DestinationSign.sign == nullptr ) { return; }

    auto const lowvoltagepower { ( MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable ) };

    DestinationSign.sign->fLight = (
        ( ( DestinationSign.has_light ) && ( lowvoltagepower ) ) ?
             2.0 :
            -1.0 );

    // jak są 4 tekstury wymienne, to nie zmieniać rozkładem
    if( std::abs( m_materialdata.multi_textures ) >= 4 ) { return; }
    // TODO: dedicated setting to discern electronic signs, instead of fallback on light presence
    m_materialdata.replacable_skins[ 4 ] = (
        ( ( DestinationSign.destination != null_handle )
       && ( ( false == DestinationSign.has_light ) // physical destination signs remain up until manually changed
         || ( ( lowvoltagepower ) // lcd signs are off without power
           && ( ctOwner != nullptr ) ) ) ) ? // lcd signs are off for carriages without engine, potentially left on a siding
            DestinationSign.destination :
            DestinationSign.destination_off );
}

bool TDynamicObject::Update(double dt, double dt1)
{
	if (dt1 == 0.0)
        return true; // Ra: pauza
    if (!MoverParameters->PhysicActivation &&
        !MechInside) // to drugie, bo będąc w maszynowym blokuje się fizyka
        return true; // McZapkie: wylaczanie fizyki gdy nie potrzeba
    if (!MyTrack)
        return false; // pojazdy postawione na torach portalowych mają MyTrack==NULL
    if (!bEnabled)
        return false; // a normalnie powinny mieć bEnabled==false

    double dDOMoveLen;
    // McZapkie: parametry powinny byc pobierane z toru
    if (Axle0.vAngles.z != Axle1.vAngles.z)
    { // wyliczenie promienia z obrotów osi - modyfikację zgłosił youBy
        ts.R = Axle0.vAngles.z - Axle1.vAngles.z; // różnica może dawać stałą ±M_2PI
        if( ( ts.R > 15000.0 ) || ( ts.R < -15000.0 ) ) {
            // szkoda czasu na zbyt duże promienie, 4km to promień nie wymagający przechyłki
            ts.R = 0.0;
        }
        else {
            // normalizacja
                 if( ts.R >  M_PI ) { ts.R -= M_2PI; }
            else if( ts.R < -M_PI ) { ts.R += M_2PI; }

            if( ts.R != 0.0 ) {
                // sin(0) results in division by zero
                ts.R = -0.5 * MoverParameters->BDist / sin( ts.R * 0.5 );
            }
        }
    }
    else
        ts.R = 0.0;
    // ts.R=ComputeRadius(Axle1.pPosition,Axle2.pPosition,Axle3.pPosition,Axle0.pPosition);
    // Ra: składową pochylenia wzdłużnego mamy policzoną w jednostkowym wektorze
    // vFront
    ts.Len = 1.0; // Max0R(MoverParameters->BDist,MoverParameters->ADist);
    ts.dHtrack = -vFront.y; // Axle1.pPosition.y-Axle0.pPosition.y; //wektor
    // między skrajnymi osiami
    // (!!!odwrotny)
    ts.dHrail = (Axle1.GetRoll() + Axle0.GetRoll()) * 0.5; //średnia przechyłka pudła
    // TTrackParam tp;
    tp.Width = MyTrack->fTrackWidth;
    // McZapkie-250202
    tp.friction = MyTrack->Friction() * Global.fFriction * Global.FrictionWeatherFactor;
    tp.CategoryFlag = MyTrack->iCategoryFlag & 15;
    tp.DamageFlag = MyTrack->iDamageFlag;
    tp.QualityFlag = MyTrack->iQualityFlag;

    // couplers
    if( ( MoverParameters->Couplers[ 0 ].CouplingFlag != coupling::faux )
     && ( MoverParameters->Couplers[ 1 ].CouplingFlag != coupling::faux ) ) {
        MoverParameters->InsideConsist = true;
    }
    else {
        MoverParameters->InsideConsist = false;
    }
    if( TestFlag( MoverParameters->AIFlag, sound::attachcoupler ) ) {
        auto *driver{ ctOwner ? ctOwner : Mechanik };
        if( driver != nullptr ) {
            driver->CheckVehicles( Connect );
        }
        ClearFlag( MoverParameters->AIFlag, sound::attachcoupler );
    }

    // napiecie sieci trakcyjnej
    if (MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector) { // dla EZT tylko silnikowy

        tmpTraction.TractionVoltage = std::max( std::abs( MoverParameters->PantRearVolt ), std::abs( MoverParameters->PantFrontVolt ) );
        // jeśli brak zasilania dłużej niż 0.2 sekundy (25km/h pod izolatorem daje 0.15s)
        // Ra 2F1H: prowizorka, trzeba przechować napięcie, żeby nie wywalało WS pod izolatorem
        if( tmpTraction.TractionVoltage > 0.0) {
            NoVoltTime = 0;
        }
        else {
            NoVoltTime += dt1;
            if( NoVoltTime <= 0.2 ) {
                tmpTraction.TractionVoltage = MoverParameters->PantographVoltage;
            }
            else {
                // jeśli jedzie
                if( MoverParameters->Vel > 0.5 ) {
                    // Ra 2014-07: doraźna blokada logowania zimnych lokomotyw - zrobić to trzeba inaczej
                    if( MoverParameters->Pantographs[end::front].is_active
                     || MoverParameters->Pantographs[end::rear].is_active ) {
                        if( ( MoverParameters->Mains ) // Ra 15-01: logować tylko, jeśli WS załączony
                         && ( MoverParameters->GetTrainsetHighVoltage() < 0.1f ) ) { // yB 16-03: i nie jest to asynchron zasilany z daleka
                            // Ra 15-01: bezwzględne współrzędne pantografu nie są dostępne więc lepiej się tego nie zaloguje
                            ErrorLog(
                                "Bad traction: " + MoverParameters->Name
                                + " lost power for " + to_string( NoVoltTime, 2 ) + " sec. at "
                                + to_string( glm::dvec3{ vPosition } ) );
                        }
                    }
                }
            }
        }
    }
    else {
        tmpTraction.TractionVoltage = 0.95 * MoverParameters->EnginePowerSource.MaxVoltage;
    }
    tmpTraction.TractionFreq = 0;
    tmpTraction.TractionMaxCurrent = 7500; // Ra: chyba za dużo? powinno wywalać przy 1500
    tmpTraction.TractionResistivity = 0.3;

    MoverParameters->PantographVoltage = tmpTraction.TractionVoltage;
    // McZapkie: predkosc w torze przekazac do TrackParam
    // McZapkie: Vel ma wymiar [km/h] (absolutny), V ma wymiar [m/s], taka przyjalem notacje
    tp.Velmax = MyTrack->VelocityGet();

    if (Mechanik)
    { // Ra 2F3F: do Driver.cpp to przenieść?
        if( Mechanik->primary() ) {
            MoverParameters->EqvtPipePress = GetEPP(); // srednie cisnienie w PG
        }
		if ((Mechanik->primary()) &&
            ((MoverParameters->EngineType == TEngineType::DieselEngine) ||
             (MoverParameters->EngineType == TEngineType::DieselElectric))
			&& (MoverParameters->EIMCtrlType > 0)) {
			MoverParameters->CheckEIMIC(dt1);
			if (MoverParameters->SpeedCtrl)
				MoverParameters->CheckSpeedCtrl(dt1);
			MoverParameters->eimic_real = std::min(MoverParameters->eimic,MoverParameters->eimicSpeedCtrl);
			MoverParameters->SendCtrlToNext("EIMIC", MoverParameters->eimic_real, MoverParameters->CabActive);
		}
		if( ( Mechanik->primary() )
         && ( MoverParameters->EngineType == TEngineType::ElectricInductionMotor ) ) {
            // jesli glowny i z asynchronami, to niech steruje hamulcem i napedem lacznie dla calego pociagu/ezt
			auto const kier = (DirectionGet() * MoverParameters->CabOccupied > 0);
            auto FED { 0.0 };
            auto np { 0 };
            auto masa { 0.0 };
            auto FrED { 0.0 };
            auto masamax { 0.0 };
            auto FmaxPN { 0.0 };
			auto FfulED { 0.0 };
			auto FmaxED { 0.0 };
            auto Frj { 0.0 };
            auto osie { 0 };
			// 0a. ustal aktualna nastawe zadania sily napedowej i hamowania
			if( ( MoverParameters->Power < 1 )
             && ( ctOwner != nullptr ) ) {
				MoverParameters->MainCtrlPos = ctOwner->Controlling()->MainCtrlPos*MoverParameters->MainCtrlPosNo / std::max(1, ctOwner->Controlling()->MainCtrlPosNo);
				MoverParameters->SpeedCtrlValue = ctOwner->Controlling()->SpeedCtrlValue;
                MoverParameters->SpeedCtrlUnit.IsActive = ctOwner->Controlling()->SpeedCtrlUnit.IsActive;
			}
			MoverParameters->CheckEIMIC(dt1);
			MoverParameters->CheckSpeedCtrl(dt1);

			auto eimic = Min0R(MoverParameters->eimic, MoverParameters->eimicSpeedCtrl);
			MoverParameters->eimic_real = eimic;
			if (MoverParameters->EIMCtrlType == 2 && MoverParameters->MainCtrlPos == 0)
				eimic = -1.0;
			MoverParameters->SendCtrlToNext("EIMIC", Max0R(0, eimic), MoverParameters->CabActive);
			auto LBR = Max0R(-eimic, 0);
			auto eim_lb = (Mechanik->AIControllFlag || !MoverParameters->LocHandleTimeTraxx ? 0 : MoverParameters->eim_localbrake);

			// 1. ustal wymagana sile hamowania calego pociagu
            //   - opoznienie moze byc ustalane na podstawie charakterystyki
            //   - opoznienie moze byc ustalane na podstawie mas i cisnien granicznych
            //

            // 2. ustal mozliwa do realizacji sile hamowania ED
            //   - w szczegolnosci powinien brac pod uwage rozne sily hamowania
            for (TDynamicObject *p = GetFirstDynamic(MoverParameters->CabOccupied < 0 ? 1 : 0, 4); p;
				(kier ? p = p->Next(4) : p = p->Prev(4)))
            {
                ++np;
                masamax +=
                    p->MoverParameters->MBPM
                    + ( p->MoverParameters->MBPM > 1.0 ?
                        0.0 :
                        p->MoverParameters->Mass )
                    + p->MoverParameters->Mred;
                auto const Nmax = ((p->MoverParameters->P2FTrans * p->MoverParameters->MaxBrakePress[0] -
                               p->MoverParameters->BrakeCylSpring) *
                                  p->MoverParameters->BrakeCylMult[0] -
                              p->MoverParameters->BrakeSlckAdj) *
                             p->MoverParameters->BrakeCylNo * p->MoverParameters->BrakeRigEff;
                FmaxPN += Nmax * p->MoverParameters->Hamulec->GetFC(
                              Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA),
                              p->MoverParameters->MED_Vref) *
                          1000; // sila hamowania pn
                FmaxED += ((p->MoverParameters->Mains) && (p->MoverParameters->DirActive != 0) &&
					(p->MoverParameters->InvertersRatio == 1.0) &&
					(p->MoverParameters->eimc[eimc_p_Fh] * p->MoverParameters->NPoweredAxles >
                                                           0) ?
                               p->MoverParameters->eimc[eimc_p_Fh] * 1000 :
                               0); // chwilowy max ED -> do rozdzialu sil
                FED -= std::min(p->MoverParameters->eimv[eimv_Fmax], 0.0) *
                       1000; // chwilowy max ED -> do rozdzialu sil
				FfulED = std::min(p->MoverParameters->eimv[eimv_Fful], 0.0) *
					1000; // chwilowy max ED -> do rozdzialu sil
				FrED -= std::min(p->MoverParameters->eimv[eimv_Fmax], 0.0) *
                        1000; // chwilowo realizowane ED -> do pneumatyki
				Frj += std::max(p->MoverParameters->eimv[eimv_Fmax], 0.0) *
					1000;// chwilowo realizowany napęd -> do utrzymującego
				masa += p->MoverParameters->TotalMass;
				osie += p->MoverParameters->NAxles;
			}
			double RapidMult = 1.0;
			if (((MoverParameters->BrakeDelays & (bdelay_P + bdelay_R)) == (bdelay_P + bdelay_R))
				&& (MoverParameters->BrakeDelayFlag & bdelay_P))
				RapidMult = MoverParameters->RapidMult;

			auto const amax = RapidMult * std::min(FmaxPN / masamax, MoverParameters->MED_amax);

			Mechanik->fMedAmax = amax;
            auto doorisopen {
                ( false == MoverParameters->Doors.instances[ side::left ].is_closed )
             || ( false == MoverParameters->Doors.instances[ side::right ].is_closed )
             || ( MoverParameters->Doors.permit_needed
               && ( MoverParameters->Doors.instances[ side::left ].open_permit
                 || MoverParameters->Doors.instances[ side::right ].open_permit ) ) };
			doorisopen &= !(MoverParameters->ReleaseParkingBySpringBrakeWhenDoorIsOpen && MoverParameters->SpringBrake.IsActive);

            if ((MoverParameters->Vel < 0.5) && (eimic < 0 || doorisopen || MoverParameters->Hamulec->GetEDBCP()))
            {
                MoverParameters->ShuntMode = true;
            }
            if (MoverParameters->ShuntMode)
            {
                MoverParameters->ShuntModeAllow = ( false == doorisopen ) &&
                                                  (LBR < 0.01);
            }
            if( ( MoverParameters->Vel > 1 ) && ( false == doorisopen ) )
            {
                MoverParameters->ShuntMode = false;
                MoverParameters->ShuntModeAllow = (MoverParameters->BrakePress > 0.2) &&
                                                  (LBR < 0.01);
            }
            auto Fzad = amax * LBR * masa;
			if (((MoverParameters->BrakeCtrlPos == MoverParameters->Handle->GetPos(bh_EB))
				&& (MoverParameters->eimc[eimc_p_abed] < 0.001)) ||
                (MoverParameters->EmergencyValveFlow > 0))
				Fzad = amax * masa; //pętla bezpieczeństwa - pełne służbowe
            if ((MoverParameters->ScndS) &&
                (MoverParameters->Vel > MoverParameters->eimc[eimc_p_Vh1]) && (FmaxED > 0))
            {
                Fzad = std::min(LBR * FmaxED, FfulED);
            }
            if (((MoverParameters->ShuntMode) && (eimic <= 0) || (doorisopen)) /*||
                (MoverParameters->V * MoverParameters->DirAbsolute < -0.2)*/)
            {
                auto const sbd { ( ( MoverParameters->SpringBrake.IsActive && MoverParameters->ReleaseParkingBySpringBrake ) ? 0.0 : MoverParameters->StopBrakeDecc ) };
                Fzad = std::max( Fzad, sbd * masa );
            }
			if ((Fzad > 1) && (!MEDLogFile.is_open()) && (MoverParameters->Vel > 1))
			{
                MEDLogFile.open(
                    "MEDLOGS/" + MoverParameters->Name + "_" + to_string( ++MEDLogCount ) + ".csv",
                    std::ios::in | std::ios::out | std::ios::trunc );
				MEDLogFile << "t\tVel\tMasa\tOsie\tFmaxPN\tFmaxED\tFfulED\tFrED\tFzad\tFzadED\tFzadPN";
				for(int k=1;k<=np;k++)
				{
					MEDLogFile << "\tBP" << k;
				}
				MEDLogFile << "\n";
				MEDLogFile.flush();
				MEDLogInactiveTime = 0;
				MEDLogTime = 0;
			}
            auto FzadED { 0.0 };
            if( ( LBR > MoverParameters->MED_MinBrakeReqED )
             && ( MoverParameters->BrakeHandle == TBrakeHandle::MHZ_EN57 ?
                    ( ( MoverParameters->BrakeOpModeFlag & bom_MED ) != 0 ) :
                    MoverParameters->EpFuse ) ) {
                FzadED = std::min( Fzad, FmaxED );
            }
			/*/ELF - wdrazanie ED po powrocie na utrzymanie hamowania - do usuniecia
			if (MoverParameters->EIMCtrlType == 2 && MoverParameters->MainCtrlPos < 2 && MoverParameters->eimic > -0.999)
			{
				FzadED = std::min(FzadED, MED_oldFED);
			} //*/
			//opoznienie wdrazania ED
			if (FzadED > MED_oldFED)
			{
				if (MoverParameters->MED_ED_DelayTimer <= 0) {
					MoverParameters->MED_ED_DelayTimer += dt1;
					if (MoverParameters->MED_ED_DelayTimer > 0) {

					}
					else {
						FzadED = std::min(FzadED, MED_oldFED);
					}
				}
				else
				{
					FzadED = std::min(FzadED, MED_oldFED);
					MoverParameters->MED_ED_DelayTimer = (FrED > 0 ?
															-MoverParameters->MED_ED_Delay2 :
															-MoverParameters->MED_ED_Delay1);
				}
			}
			if ((MoverParameters->BrakeCtrlPos == MoverParameters->Handle->GetPos(bh_EB))
				&& (MoverParameters->eimc[eimc_p_abed] < 0.001))
				FzadED = 0; //pętla bezpieczeństwa - bez ED
            auto const FzadPN = Fzad - FrED * MoverParameters->MED_FrED_factor;
            //np = 0;
            // BUG: likely memory leak, allocation per inner loop, deleted only once outside
            // TODO: sort this shit out
			bool* PrzekrF = new bool[np];
			float nPrzekrF = 0;
			bool test = true;
			float* FzED = new float[np];
			float* FzEP = new float[np];
			float* FmaxEP = new float[np];
			// 3. ustaw pojazdom sile hamowania ED
            //   - proporcjonalnie do mozliwosci

            // 4. ustal potrzebne dohamowanie pneumatyczne
            //   - od sily zadanej trzeba odjac realizowana przez ED
            // 5. w razie potrzeby wlacz hamulec utrzymujacy
            //   - gdy zahamowany ma ponizej 2 km/h
            // 6. ustaw pojazdom sile hamowania ep
            //   - proporcjonalnie do masy, do liczby osi, rowne cisnienia - jak
            //   bedzie, tak bedzie dobrze
            float Fpoj = 0; // MoverParameters->CabOccupied < 0
            ////ALGORYTM 2 - KAZDEMU PO ROWNO, ale nie wiecej niz eped * masa
            // 1. najpierw daj kazdemu tyle samo
            int i = 0;
			for (TDynamicObject *p = GetFirstDynamic(MoverParameters->CabOccupied < 0 ? 1 : 0, 4); p;
				p = (kier == true ? p->Next(4) : p->Prev(4)) )
			{
                auto const Nmax = ((p->MoverParameters->P2FTrans * p->MoverParameters->MaxBrakePress[0] -
                               p->MoverParameters->BrakeCylSpring) *
                                  p->MoverParameters->BrakeCylMult[0] -
                              p->MoverParameters->BrakeSlckAdj) *
                             p->MoverParameters->BrakeCylNo * p->MoverParameters->BrakeRigEff;
                FmaxEP[i] = Nmax *
                            p->MoverParameters->Hamulec->GetFC(
                                Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA),
                                p->MoverParameters->MED_Vref) *
                            1000; // sila hamowania pn

                PrzekrF[i] = false;
                FzED[i] = (FmaxED > 0 ? FzadED / FmaxED : 0);
                p->MoverParameters->AnPos =
                    (MoverParameters->ScndS ? LBR : FzED[i]);
                FzEP[ i ] = static_cast<float>( FzadPN * p->MoverParameters->NAxles ) / static_cast<float>( osie );
                ++i;
                p->MoverParameters->ShuntMode = MoverParameters->ShuntMode;
                p->MoverParameters->ShuntModeAllow = MoverParameters->ShuntModeAllow;
            }
            while (test)
            {
                test = false;
                i = 0;
                float przek = 0;
                for (TDynamicObject *p = GetFirstDynamic(MoverParameters->CabOccupied < 0 ? 1 : 0, 4); p;
                     p = (kier == true ? p->Next(4) : p->Prev(4)) )
                {
                    if ((FzEP[i] > 0.01) &&
                        (FzEP[i] >
                         p->MoverParameters->TotalMass * p->MoverParameters->eimc[eimc_p_eped] +
                             Min0R(p->MoverParameters->eimv[eimv_Fmax], 0) * 1000) &&
                        (!PrzekrF[i]))
                    {
                        float przek1 = -Min0R(p->MoverParameters->eimv[eimv_Fmax], 0) * 1000 +
                                       FzEP[i] -
                                       p->MoverParameters->TotalMass *
                                           p->MoverParameters->eimc[eimc_p_eped] * 0.999;
                        PrzekrF[i] = true;
                        test = true;
                        nPrzekrF++;
                        przek1 = Min0R(przek1, FzEP[i]);
                        FzEP[i] -= przek1;
                        if (FzEP[i] < 0)
                            FzEP[i] = 0;
                        przek += przek1;
                    }
                    ++i;
                }
                i = 0;
                przek = przek / (np - nPrzekrF);
                for (TDynamicObject *p = GetFirstDynamic(MoverParameters->CabOccupied < 0 ? 1 : 0, 4); p;
                     (true == kier ? p = p->Next(4) : p = p->Prev(4)))
                {
                    if (!PrzekrF[i])
                    {
                        FzEP[i] += przek;
                    }
                    ++i;
                }
            }
            i = 0;
            for (TDynamicObject *p = GetFirstDynamic(MoverParameters->CabOccupied < 0 ? 1 : 0, 4); p;
                 (true == kier ? p = p->Next(4) : p = p->Prev(4)))
            {
                float Nmax = ((p->MoverParameters->P2FTrans * p->MoverParameters->MaxBrakePress[0] -
                               p->MoverParameters->BrakeCylSpring) *
                                  p->MoverParameters->BrakeCylMult[0] -
                              p->MoverParameters->BrakeSlckAdj) *
                             p->MoverParameters->BrakeCylNo * p->MoverParameters->BrakeRigEff;
				if (FrED > 0.1)
					p->MoverParameters->MED_EPVC_CurrentTime = 0;
				else
					p->MoverParameters->MED_EPVC_CurrentTime += dt1;
				bool EPVC = ((p->MoverParameters->MED_EPVC) && ((p->MoverParameters->MED_EPVC_Time < 0) || (p->MoverParameters->MED_EPVC_CurrentTime < p->MoverParameters->MED_EPVC_Time)));
				float VelC = (EPVC ? clamp(p->MoverParameters->Vel, p->MoverParameters->MED_Vmin, p->MoverParameters->MED_Vmax) : p->MoverParameters->MED_Vref);//korekcja EP po prędkości
                float FmaxPoj = Nmax *
					p->MoverParameters->Hamulec->GetFC(
						Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA), VelC) *
					1000; // sila hamowania pn
				p->MoverParameters->LocalBrakePosAEIM = FzEP[i] / FmaxPoj;
				if (p->MoverParameters->LocalBrakePosAEIM>0.009)
					if (p->MoverParameters->P2FTrans * p->MoverParameters->BrakeCylMult[0] *
						p->MoverParameters->MaxBrakePress[0] != 0)
					{
						float x = (p->MoverParameters->BrakeSlckAdj / p->MoverParameters->BrakeCylMult[0] +
							p->MoverParameters->BrakeCylSpring) / (p->MoverParameters->P2FTrans *
								p->MoverParameters->MaxBrakePress[0]);
						p->MoverParameters->LocalBrakePosAEIM = x + (1 - x) * p->MoverParameters->LocalBrakePosAEIM;
					}
					else
						p->MoverParameters->LocalBrakePosAEIM = p->MoverParameters->LocalBrakePosAEIM;
				else
					p->MoverParameters->LocalBrakePosAEIM = 0;
				if (p->MoverParameters->LocHandleTimeTraxx)
				{
					p->MoverParameters->eim_localbrake = eim_lb;
					p->MoverParameters->LocalBrakePosAEIM = std::max(p->MoverParameters->LocalBrakePosAEIM, eim_lb);
				}
				++i;
			}

			MED[0][0] = masa*0.001;
			MED[0][1] = amax;
			MED[0][2] = Fzad*0.001;
			MED[0][3] = FmaxPN*0.001;
			MED[0][4] = FmaxED*0.001;
			MED[0][5] = FrED*0.001;
			MED[0][6] = FzadPN*0.001;
			MED[0][7] = nPrzekrF;

			if (MEDLogFile.is_open())
			{
				MEDLogFile << MEDLogTime << "\t" << MoverParameters->Vel << "\t" << masa*0.001 << "\t" << osie << "\t" << FmaxPN*0.001 << "\t" << FmaxED*0.001 << "\t"
					<< FfulED*0.001 << "\t" << FrED*0.001 << "\t" << Fzad*0.001 << "\t" << FzadED*0.001 << "\t" << FzadPN*0.001;
				for (TDynamicObject *p = GetFirstDynamic(MoverParameters->CabOccupied < 0 ? 1 : 0, 4); p;
					(true == kier ? p = p->Next(4) : p = p->Prev(4)))
				{
					MEDLogFile << "\t" << p->MoverParameters->BrakePress;
				}
				MEDLogFile << "\n";
				if (floor(MEDLogTime + dt1) > floor(MEDLogTime))
				{
					MEDLogFile.flush();
				}
				MEDLogTime += dt1;

				if ((MoverParameters->Vel < 0.1) || (MoverParameters->MainCtrlPowerPos() > 0))
				{
					MEDLogInactiveTime += dt1;
				}
				else
				{
					MEDLogInactiveTime = 0;
				}
				if (MEDLogInactiveTime > 5)
				{
					MEDLogFile.flush();
					MEDLogFile.close();
				}

			}

			delete[] PrzekrF;
			delete[] FzED;
			delete[] FzEP;
			delete[] FmaxEP;

			MED_oldFED = FzadED;
        }

        Mechanik->Update(dt1); // przebłyski świadomości AI
    }

    // fragment "z EXE Kursa"
    if( MoverParameters->Mains ) { // nie wchodzić w funkcję bez potrzeby
        if( ( false == ( MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable ) )
/*
          // NOTE: disabled on account of multi-unit setups, where the unmanned unit wouldn't be affected
            && ( Controller == Humandriver )
*/
            && ( MoverParameters->EngineType != TEngineType::DieselEngine )
            && ( MoverParameters->EngineType != TEngineType::WheelsDriven ) ) { // jeśli bateria wyłączona, a nie diesel ani drezyna reczna
            if( MoverParameters->MainSwitch( false, ( MoverParameters->TrainType == dt_EZT ? range_t::unit : range_t::local ) ) ) {
                // wyłączyć zasilanie
                // NOTE: we turn off entire EMU, but only the affected unit for other multi-unit consists
                MoverParameters->EventFlag = true;
                // drop pantographs
                // NOTE: this isn't universal behaviour
                // TODO: have this dependant on .fiz-driven flag
                // NOTE: moved to pantspeed calculation part a little later in the function. all remarks and todo still apply
/*
                MoverParameters->PantFront( false, ( MoverParameters->TrainType == dt_EZT ? range::unit : range::local ) );
                MoverParameters->PantRear( false, ( MoverParameters->TrainType == dt_EZT ? range::unit : range::local ) );
*/
            }
        }
    }

    // przekazanie pozycji do fizyki
    // NOTE: coordinate system swap
    // TODO: replace with regular glm vectors
    TLocation const l {
        -vPosition.x,
         vPosition.z,
         vPosition.y };
    TRotation const r {
        0.0,
        0.0,
        modelRot.z };
    // McZapkie-260202 - dMoveLen przyda sie przy stukocie kol
    dDOMoveLen = GetdMoveLen() + MoverParameters->ComputeMovement(dt, dt1, ts, tp, tmpTraction, l, r);
    if( Mechanik ) {
        // dodanie aktualnego przemieszczenia
        Mechanik->MoveDistanceAdd( dDOMoveLen );
    }
    if( ( simulation::Train != nullptr )
     && ( simulation::Train->Dynamic() == this ) ) {
        // update distance meter in user-controlled cab
        // TBD: place the meter on mover logic level?
        simulation::Train->add_distance( dDOMoveLen );
    }
    glm::dvec3 old_pos = vPosition;
    Move(dDOMoveLen);

	m_future_movement = (glm::dvec3(vPosition) - old_pos) / dt1 * Timer::GetDeltaRenderTime();

    if (!bEnabled) // usuwane pojazdy nie mają toru
    { // pojazd do usunięcia
        bDynamicRemove = true; // sprawdzić
        return false;
    }
    ResetdMoveLen();

    // McZapkie-260202
    // tupot mew, tfu, stukot kol:
    if( MyTrack->fSoundDistance != -1 ) {

        if( MyTrack->fSoundDistance != dRailLength ) {
            if( dRailLength > 0.0 ) {
                for( auto &axle : m_axlesounds ) {
                    axle.distance = axle.offset;
                }
            }
            dRailLength = MyTrack->fSoundDistance;
        }
        if( dRailLength > 0.0 ) {
            if( MoverParameters->Vel > 0 ) {
                // TODO: track quality and/or environment factors as separate subroutine
                auto volume =
                    interpolate(
                        0.8, 1.2,
                        clamp(
                            MyTrack->iQualityFlag / 20.0,
                            0.0, 1.0 ) );
                switch( MyTrack->eEnvironment ) {
                    case e_tunnel: {
                        volume *= 1.1;
                        break;
                    }
                    case  e_bridge: {
                        volume *= 1.2;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                if( dRailLength > 0.0 ) {
                    auto axleindex { 0 };
                    auto const directioninconsist { (
                        ctOwner == nullptr ?
                            1 :
                            ( ctOwner->Vehicle()->DirectionGet() == DirectionGet() ?
                                1 :
                               -1 ) ) };
                    for( auto &axle : m_axlesounds ) {
                        axle.distance += dDOMoveLen * directioninconsist;
                        if( ( axle.distance < 0 )
                         || ( axle.distance > dRailLength ) ) {
                            axle.distance = clamp_circular( axle.distance, dRailLength );
                            if( MoverParameters->Vel > 0.1 ) {
                                // NOTE: for combined clatter sound we supply 1/100th of actual value, as the sound module converts does the opposite, converting received (typically) 0-1 values to 0-100 range
                                auto const frequency = (
                                    true == axle.clatter.is_combined() ?
                                        MoverParameters->Vel * 0.01 :
                                        1.0 );
                                axle.clatter
                                    .pitch( frequency )
                                    .gain( volume )
                                    .play();
                                // crude bump simulation, drop down on even axles, move back up on the odd ones
                                MoverParameters->AccVert +=
                                    interpolate(
                                        0.01, 0.05,
                                        clamp(
                                            GetVelocity() / ( 1 + MoverParameters->Vmax ),
                                            0.0, 1.0 ) )
                                    * ( ( axleindex % 2 ) != 0 ?
                                         1 :
                                        -1 );
                            }
                        }
                        ++axleindex;
                    }
                }
            }
        }
    }
    // McZapkie-260202 end

    // fragment z EXE Kursa
    /* if (MoverParameters->TrainType==dt_ET42)
         {
         if ((MoverParameters->DynamicBrakeType=dbrake_switch) &&
       ((MoverParameters->BrakePress >
       0.2) || ( MoverParameters->PipePress < 0.36 )))
                      {
                      MoverParameters->StLinFlag=true;
                      }
         else
         if ((MoverParameters->DynamicBrakeType=dbrake_switch) &&
       (MoverParameters->BrakePress <
       0.1))
                      {
                      MoverParameters->StLinFlag=false;

                      }
         }   */

    if (MoverParameters->Vel != 0)
    { // McZapkie-050402: krecenie kolami:
        glm::dvec3 old_wheels = glm::dvec3(dWheelAngle[0], dWheelAngle[1], dWheelAngle[2]);

        dWheelAngle[0] += 114.59155902616464175359630962821 * MoverParameters->V * dt1 /
                          MoverParameters->WheelDiameterL; // przednie toczne
        dWheelAngle[1] += MoverParameters->nrot * dt1 * 360.0; // napędne
        dWheelAngle[2] += 114.59155902616464175359630962821 * MoverParameters->V * dt1 /
                          MoverParameters->WheelDiameterT; // tylne toczne


		m_future_wheels_angle = (glm::dvec3(dWheelAngle[0], dWheelAngle[1], dWheelAngle[2]) - old_wheels) / dt1 * Timer::GetDeltaRenderTime();

        dWheelAngle[0] = clamp_circular( dWheelAngle[0] );
        dWheelAngle[1] = clamp_circular( dWheelAngle[1] );
        dWheelAngle[2] = clamp_circular( dWheelAngle[2] );
    }
    if (pants) // pantograf może być w wagonie kuchennym albo pojeździe rewizyjnym (np. SR61)
    { // przeliczanie kątów dla pantografów
        double k; // tymczasowy kąt
        double PantDiff;
        TAnimPant *p; // wskaźnik do obiektu danych pantografu
        double fCurrent = (
            ( MoverParameters->DynamicBrakeFlag && MoverParameters->ResistorsFlag ) ?
                0 :
                std::abs( MoverParameters->Itot ) * MoverParameters->IsVehicleEIMBrakingFactor() )
            + MoverParameters->TotalCurrent; // prąd pobierany przez pojazd - bez sensu z tym (TotalCurrent)
        // TotalCurrent to bedzie prad nietrakcyjny (niezwiazany z napedem)
        // fCurrent+=fabs(MoverParameters->Voltage)*1e-6; //prąd płynący przez woltomierz, rozładowuje kondensator orgromowy 4µF
/*
        double fPantCurrent = fCurrent; // normalnie cały prąd przez jeden pantograf
        if (iAnimType[ANIM_PANTS] > 1) { // a jeśli są dwa pantografy //Ra 1014-11: proteza, trzeba zrobić sensowniej
            if (pants[0].fParamPants->hvPowerWire && pants[1].fParamPants->hvPowerWire) { // i oba podłączone do drutów
                fPantCurrent = fCurrent * 0.5; // to dzielimy prąd równo na oba (trochę bez sensu, ale lepiej tak niż podwoić prąd)
            }
        }
*/
        // test whether more than one pantograph touches the wire
        // NOTE: we're duplicating lot of code from below
        // TODO: clean this up
        auto activepantographs { 0 };
        for( int idx = 0; idx < iAnimType[ ANIM_PANTS ]; ++idx ) {
            auto const *pantograph { pants[ idx ].fParamPants };
            if( Global.bLiveTraction == false ) {
                if( pantograph->PantWys >= 1.2 ) {
                    ++activepantographs;
                }
            }
            else {
                if( ( pantograph->hvPowerWire != nullptr )
                 && ( true == MoverParameters->Pantographs[ end::front ].is_active )
                 && ( pantograph->PantTraction - pantograph->PantWys < 0.01 ) ) { // tolerancja niedolegania
                    ++activepantographs;
                }
            }
        }
        auto const fPantCurrent { fCurrent / std::max( 1, activepantographs ) };

        for (int i = 0; i < iAnimType[ANIM_PANTS]; ++i)
        { // pętla po wszystkich pantografach
            p = pants[i].fParamPants;
            if (p->PantWys < 0)
            { // patograf został połamany, liczony nie będzie
                if (p->fAngleL > p->fAngleL0)
                    p->fAngleL -= 0.2 * dt1; // nieco szybciej niż jak dla opuszczania
                if (p->fAngleL < p->fAngleL0)
                    p->fAngleL = p->fAngleL0; // kąt graniczny
                if (p->fAngleU < M_PI)
                    p->fAngleU += 0.5 * dt1; // górne się musi ruszać szybciej.
                if (p->fAngleU > M_PI)
                    p->fAngleU = M_PI;
                if (i & 1) // zgłoszono, że po połamaniu potrafi zostać zasilanie
                    MoverParameters->PantRearVolt = 0.0;
                else
                    MoverParameters->PantFrontVolt = 0.0;
                continue; // reszta wtedy nie jest wykonywana
            }
            PantDiff = p->PantTraction - p->PantWys; // docelowy-aktualny
            switch (i) // numer pantografu
            { // trzeba usunąć to rozróżnienie
            case 0:
                if( ( Global.bLiveTraction == false )
                 && ( p->hvPowerWire == nullptr ) ) {
                    // jeśli nie ma drutu, może pooszukiwać
                    MoverParameters->PantFrontVolt =
                        ( p->PantWys >= 1.2 ) ?
                            0.95 * MoverParameters->EnginePowerSource.MaxVoltage :
                            0.0;
                }
                else if( ( true == MoverParameters->Pantographs[ end::front ].is_active )
                      && ( PantDiff < 0.01 ) ) // tolerancja niedolegania
                {
                    if (p->hvPowerWire) {
                        auto const lastvoltage { MoverParameters->PantFrontVolt };
                        // TODO: wyliczyć trzeba prąd przypadający na pantograf i wstawić do GetVoltage()
                        if( lastvoltage == 0.0 ) {
                            // HACK: retrieve the wire voltage for calculations down the road without blowing up the supply
                            MoverParameters->PantFrontVolt = p->hvPowerWire->VoltageGet( MoverParameters->PantographVoltage, 0.0 );
                        }
                        else {
                            MoverParameters->PantFrontVolt = p->hvPowerWire->VoltageGet( MoverParameters->PantographVoltage, fPantCurrent );
                            if( MoverParameters->PantFrontVolt > 0.0 ) {
                                fCurrent -= fPantCurrent; // taki prąd płynie przez powyższy pantograf (unless it doesn't)
                            }
                        }
                        // TODO: refactor reaction to voltage change to mover as sound event for specific pantograph
                        if( ( lastvoltage == 0.0 )
                         && ( MoverParameters->PantFrontVolt > 0.0 ) ) {
                            for( auto &pantograph : m_pantographsounds ) {
                                if( pantograph.sPantUp.offset().z > 0 ) {
                                    // limit to pantographs located in the front half of the vehicle
                                    pantograph.sPantUp.play( sound_flags::exclusive );
                                }
                            }
                        }
                    }
                    else
                        MoverParameters->PantFrontVolt = 0.0;
                }
                else
                    MoverParameters->PantFrontVolt = 0.0;
                ( ( fPantCurrent > 0.0 ) ? MoverParameters->EnergyMeter.first : MoverParameters->EnergyMeter.second ) += MoverParameters->PantRearVolt * fPantCurrent * dt1 / 3600000.0;
                break;
            case 1:
                if( ( false == Global.bLiveTraction )
                 && ( nullptr == p->hvPowerWire ) ) {
                    // jeśli nie ma drutu, może pooszukiwać
                    MoverParameters->PantRearVolt =
                        ( p->PantWys >= 1.2 ) ?
                            0.95 * MoverParameters->EnginePowerSource.MaxVoltage :
                            0.0;
                }
                else if ( ( true == MoverParameters->Pantographs[ end::rear ].is_active )
                       && ( PantDiff < 0.01 ) )
                {
                    if (p->hvPowerWire) {
                        auto const lastvoltage { MoverParameters->PantRearVolt };
                        // TODO: wyliczyć trzeba prąd przypadający na pantograf i wstawić do GetVoltage()
                        if( lastvoltage == 0.0 ) {
                            // HACK: retrieve the wire voltage for calculations down the road without blowing up the supply
                            MoverParameters->PantRearVolt = p->hvPowerWire->VoltageGet( MoverParameters->PantographVoltage, 0.0 );
                        }
                        else {
                            MoverParameters->PantRearVolt = p->hvPowerWire->VoltageGet( MoverParameters->PantographVoltage, fPantCurrent );
                            if( MoverParameters->PantRearVolt > 0.0 ) {
                                fCurrent -= fPantCurrent; // taki prąd płynie przez powyższy pantograf (unless it doesn't)
                            }
                        }
                        // TODO: refactor reaction to voltage change to mover as sound event for specific pantograph
                        if( ( lastvoltage == 0.0 )
                         && ( MoverParameters->PantRearVolt > 0.0 ) ) {
                            for( auto &pantograph : m_pantographsounds ) {
                                if( pantograph.sPantUp.offset().z < 0 ) {
                                    // limit to pantographs located in the rear half of the vehicle
                                    pantograph.sPantUp.play( sound_flags::exclusive );
                                }
                            }
                        }
                    }
                    else
                        MoverParameters->PantRearVolt = 0.0;
                }
                else {
//                    Global.iPause ^= 2;
                    MoverParameters->PantRearVolt = 0.0;
                }
                ( ( fPantCurrent > 0.0 ) ? MoverParameters->EnergyMeter.first : MoverParameters->EnergyMeter.second ) += MoverParameters->PantFrontVolt * fPantCurrent * dt1 / 3600000.0;
                break;
            } // pozostałe na razie nie obsługiwane
            if( MoverParameters->PantPress > (
                    MoverParameters->TrainType == dt_EZT ?
                        2.45 : // Ra 2013-12: Niebugocław mówi, że w EZT podnoszą się przy 2.5
                        3.45 ) ) {
                // z EXE Kursa
                // Ra: wysokość zależy od ciśnienia !!!
                pantspeedfactor = 0.015 * ( MoverParameters->PantPress ) * dt1;
            }
            else {
                pantspeedfactor = 0.0;
            }
            if( false == ( MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable ) ) {
                pantspeedfactor = 0.0;
            }
            pantspeedfactor = std::max( 0.0, pantspeedfactor );
            k = p->fAngleL;
            if( ( pantspeedfactor > 0.0 )
             && ( MoverParameters->Pantographs[i].is_active ) )// jeśli ma być podniesiony
            {
                if (PantDiff > 0.001) // jeśli nie dolega do drutu
                { // jeśli poprzednia wysokość jest mniejsza niż pożądana, zwiększyć kąt dolnego
                    // ramienia zgodnie z ciśnieniem
                    if (pantspeedfactor >
                        0.55 * PantDiff) // 0.55 to około pochodna kąta po wysokości
                        k += 0.55 * PantDiff; // ograniczenie "skoku" w danej klatce
                    else
                        k += pantspeedfactor; // dolne ramię
                    // jeśli przekroczono kąt graniczny, zablokować pantograf
                    // (wymaga interwencji pociągu sieciowego)
                }
                else if (PantDiff < -0.001) {
                    // drut się obniżył albo pantograf podniesiony za wysoko
                    // jeśli wysokość jest zbyt duża, wyznaczyć zmniejszenie kąta
                    // jeśli zmniejszenie kąta jest zbyt duże, przejść do trybu łamania pantografu
                    // if (PantFrontDiff<-0.05) //skok w dół o 5cm daje złąmanie pantografu
                    k += 0.4 * PantDiff; // mniej niż pochodna kąta po wysokości
                } // jeśli wysokość jest dobra, nic więcej nie liczyć
            }
            else
            { // jeśli ma być na dole
                if (k > p->fAngleL0) // jeśli wyżej niż położenie wyjściowe
                    k -= 0.15 * dt1; // ruch w dół
                if (k < p->fAngleL0)
                    k = p->fAngleL0; // położenie minimalne
            }
            if (k != p->fAngleL)
            { //żeby nie liczyć w kilku miejscach ani gdy nie potrzeba
                if (k + p->fAngleU < M_PI)
                { // o ile nie został osiągnięty kąt maksymalny
                    p->fAngleL = k; // zmieniony kąt
                    // wyliczyć kąt górnego ramienia z wzoru (a)cosinusowego
                    //=acos((b*cos()+c)/a)
                    // p->dPantAngleT=acos((1.22*cos(k)+0.535)/1.755); //górne ramię
                    p->fAngleU = acos((p->fLenL1 * cos(k) + p->fHoriz) / p->fLenU1); // górne ramię
                    // wyliczyć aktualną wysokość z wzoru sinusowego
                    // h=a*sin()+b*sin()
                    // wysokość całości
                    p->PantWys = p->fLenL1 * sin(k) + p->fLenU1 * sin(p->fAngleU) + p->fHeight;
                }
            }
        } // koniec pętli po pantografach
        // TBD, TODO: generate sound event during mover update instead?
        if( MoverParameters->Pantographs[end::front].sound_event != MoverParameters->Pantographs[ end::front ].is_active ) {
            if( MoverParameters->Pantographs[ end::front ].is_active ) {
                // pantograph moving up
                // TBD: add a sound?
            }
            else {
                // pantograph dropping
                for( auto &pantograph : m_pantographsounds ) {
                    if( pantograph.sPantDown.offset().z > 0 ) {
                        // limit to pantographs located in the front half of the vehicle
                        pantograph.sPantDown.play( sound_flags::exclusive );
                    }
                }
            }
            MoverParameters->Pantographs[ end::front ].sound_event = MoverParameters->Pantographs[ end::front ].is_active;
        }
        if( MoverParameters->Pantographs[ end::rear ].sound_event != MoverParameters->Pantographs[ end::rear ].is_active ) {
            if( MoverParameters->Pantographs[ end::rear ].is_active ) {
                // pantograph moving up
                // TBD: add a sound?
            }
            else {
                // pantograph dropping
                for( auto &pantograph : m_pantographsounds ) {
                    if( pantograph.sPantDown.offset().z < 0 ) {
                        // limit to pantographs located in the front half of the vehicle
                        pantograph.sPantDown.play( sound_flags::exclusive );
                    }
                }
            }
            MoverParameters->Pantographs[ end::rear ].sound_event = MoverParameters->Pantographs[ end::rear ].is_active;
        }
    }
    else if (MoverParameters->EnginePowerSource.SourceType == TPowerSource::InternalSource)
        if (MoverParameters->EnginePowerSource.PowerType == TPowerType::SteamPower)
        // if (smPatykird1[0])
        { // Ra: animacja rozrządu parowozu, na razie nieoptymalizowane
            /* //Ra: tymczasowo wyłączone ze względu na porządkowanie animacji
               pantografów
               double fi,dx,c2,ka,kc;
               double sin_fi,cos_fi;
               double L1=1.6688888888888889;
               double L2=5.6666666666666667; //2550/450
               double Lc=0.4;
               double L=5.686422222; //2558.89/450
               double G1,G2,G3,ksi,sin_ksi,gam;
               double G1_2,G2_2,G3_2; //kwadraty
               //ruch tłoków oraz korbowodów
               for (int i=0;i<=1;++i)
               {//obie strony w ten sam sposób
                fi=DegToRad(dWheelAngle[1]+(i?pant2x:pant1x)); //kąt obrotu koła dla
               tłoka 1
                sin_fi=sin(fi);
                cos_fi=cos(fi);
                dx=panty*cos_fi+sqrt(panth*panth-panty*panty*sin_fi*sin_fi)-panth;
               //nieoptymalne
                if (smPatykird1[i]) //na razie zabezpieczenie
                 smPatykird1[i]->SetTranslate(float3(dx,0,0));
                ka=-asin(panty/panth)*sin_fi;
                if (smPatykirg1[i]) //na razie zabezpieczenie
                 smPatykirg1[i]->SetRotateXYZ(vector3(RadToDeg(ka),0,0));
                //smPatykirg1[0]->SetRotate(float3(0,1,0),RadToDeg(fi)); //obracamy
                //ruch drążka mimośrodkowego oraz jarzma
                //korzystałem z pliku PDF "mm.pdf" (opis czworoboku
               korbowo-wahaczowego):
                //"MECHANIKA MASZYN. Szkic wykładu i laboratorium komputerowego."
                //Prof. dr hab. inż. Jerzy Zajączkowski, 2007, Politechnika Łódzka
                //L1 - wysokość (w pionie) osi jarzma ponad osią koła
                //L2 - odległość w poziomie osi jarzma od osi koła
                //Lc - długość korby mimośrodu na kole
                //Lr - promień jarzma =1.0 (pozostałe przeliczone proporcjonalnie)
                //L - długość drążka mimośrodowego
                //fi - kąt obrotu koła
                //ksi - kąt obrotu jarzma (od pionu)
                //gam - odchylenie drążka mimośrodowego od poziomu
                //G1=(Lr*Lr+L1*L1+L2*L2+Kc*Lc-L*L-2.0*Lc*L2*cos(fi)+2.0*Lc*L1*sin(fi))/(Lr*Lr);
                //G2=2.0*(L2-Lc*cos(fi))/Lr;
                //G3=2.0*(L1-Lc*sin(fi))/Lr;
                fi=DegToRad(dWheelAngle[1]+(i?pant2x:pant1x)-96.77416667); //kąt
               obrotu koła dla
               tłoka 1
                //1) dla dWheelAngle[1]=0° korba jest w dół, a mimośród w stronę
               jarzma, czyli
               fi=-7°
                //2) dla dWheelAngle[1]=90° korba jest do tyłu, a mimośród w dół,
               czyli fi=83°
                sin_fi=sin(fi);
                cos_fi=cos(fi);
                G1=(1.0+L1*L1+L2*L2+Lc*Lc-L*L-2.0*Lc*L2*cos_fi+2.0*Lc*L1*sin_fi);
                G1_2=G1*G1;
                G2=2.0*(L2-Lc*cos_fi);
                G2_2=G2*G2;
                G3=2.0*(L1-Lc*sin_fi);
                G3_2=G3*G3;
                sin_ksi=(G1*G2-G3*_fm_sqrt(G2_2+G3_2-G1_2))/(G2_2+G3_2); //x1 (minus
               delta)
                ksi=asin(sin_ksi); //kąt jarzma
                if (smPatykirg2[i])
                 smPatykirg2[i]->SetRotateXYZ(vector3(RadToDeg(ksi),0,0)); //obrócenie
               jarzma
                //1) ksi=-23°, gam=
                //2) ksi=10°, gam=
                //gam=acos((L2-sin_ksi-Lc*cos_fi)/L); //kąt od poziomu, liczony
               względem poziomu
                //gam=asin((L1-cos_ksi-Lc*sin_fi)/L); //kąt od poziomu, liczony
               względem pionu
                gam=atan2((L1-cos(ksi)+Lc*sin_fi),(L2-sin_ksi+Lc*cos_fi)); //kąt od
               poziomu
                if (smPatykird2[i]) //na razie zabezpieczenie
                 smPatykird2[i]->SetRotateXYZ(vector3(RadToDeg(-gam-ksi),0,0));
               //obrócenie drążka
               mimośrodowego
               }
            */
        }

	if (MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector
	        && MoverParameters->EnginePowerSource.CollectorParameters.FakePower) {
		MoverParameters->PantRearVolt = 0.95 * MoverParameters->EnginePowerSource.MaxVoltage;
		MoverParameters->PantFrontVolt = 0.95 * MoverParameters->EnginePowerSource.MaxVoltage;
	}

    // mirrors
    if( (MoverParameters->Vel > MoverParameters->MirrorVelClose)
		|| (MoverParameters->CabActive == 0) && (activation::mirrors)
		|| (MoverParameters->MirrorForbidden) ) {
        // automatically fold mirrors when above velocity threshold
        if( dMirrorMoveL > 0.0 ) {
            dMirrorMoveL = std::max(
                0.0,
                dMirrorMoveL - 1.0 * dt1 );
        }
        if( dMirrorMoveR > 0.0 ) {
            dMirrorMoveR = std::max(
                0.0,
                dMirrorMoveR - 1.0 * dt1 );
        }
    }
    else {
        // unfold mirror on the side with open doors, if not moving too fast
        if( ( dMirrorMoveL < 1.0 )
         && ( true == MoverParameters->Doors.instances[side::left].open_permit ) ) {
            dMirrorMoveL = std::min(
                1.0,
                dMirrorMoveL + 1.0 * dt1 );
        }
        if( ( dMirrorMoveR < 1.0 )
         && ( true == MoverParameters->Doors.instances[side::right].open_permit ) ) {
            dMirrorMoveR = std::min(
                1.0,
                dMirrorMoveR + 1.0 * dt1 );
        }
    }

    // compartment lights
    // if the vehicle has a controller, we base the light state on state of the controller otherwise we check the vehicle itself
    if( ( ctOwner  != nullptr ? SectionLightsActive != MoverParameters->CompartmentLights.is_active :
          Mechanik != nullptr ? SectionLightsActive != MoverParameters->CompartmentLights.is_active :
          SectionLightsActive ) ) { // without controller switch the lights off
        toggle_lights();
    }

	if (MoverParameters->InactiveCabPantsCheck)
	{
		pants_up();
		MoverParameters->InactiveCabPantsCheck = false;
	}


    if (MoverParameters->DerailReason > 0)
    {
        switch (MoverParameters->DerailReason)
        {
        case 1:
            ErrorLog("Bad driving: " + asName + " derailed due to end of track");
            break;
        case 2:
            ErrorLog("Bad driving: " + asName + " derailed due to too high speed");
            break;
        case 3:
            ErrorLog("Bad dynamic: " + asName + " derailed due to track width");
            break; // błąd w scenerii
        case 4:
            ErrorLog("Bad dynamic: " + asName + " derailed due to wrong track type");
            break; // błąd w scenerii
        }
        MoverParameters->DerailReason = 0; //żeby tylko raz
    }

    if( MoverParameters->LoadStatus ) {
        LoadUpdate(); // zmiana modelu ładunku
    }
    update_exchange( dt );

	return true; // Ra: chyba tak?
}

void TDynamicObject::pants_up()
{
	TDynamicObject *d = this;
	bool isAnyPantUp = false;
	while (d) {
		for (auto &item : d->MoverParameters->Pantographs)
		{
			isAnyPantUp |= item.is_active;
		}
		d = d->Next(4); // pozostałe też
	}
	d = Prev(4);
	while (d) {
		for (auto &item : d->MoverParameters->Pantographs)
		{
			isAnyPantUp |= item.is_active;
		}
		d = d->Prev(4); // w drugą stronę też
	}
	if (isAnyPantUp)
	{
		d = this;
		while (d) {
			d->MoverParameters->OperatePantographValve(end::front, operation_t::enable, range_t::local);
			d->MoverParameters->OperatePantographValve(end::rear, operation_t::enable, range_t::local);
			d = d->Next(4); // pozostałe też
		}
		d = Prev(4);
		while (d) {
			d->MoverParameters->OperatePantographValve(end::front, operation_t::enable, range_t::local);
			d->MoverParameters->OperatePantographValve(end::rear, operation_t::enable, range_t::local);
			d = d->Prev(4); // w drugą stronę też
		}
	}
}

glm::dvec3 TDynamicObject::get_future_movement() const
{
	return m_future_movement;
}

void TDynamicObject::move_set(double distance)
{
	TDynamicObject *d = this;
	while( d ) {
		d->Move( distance * d->DirectionGet() );
		d = d->Next(); // pozostałe też
	    }
	d = Prev();
	while( d ) {
		d->Move( distance * d->DirectionGet() );
		d = d->Prev(); // w drugą stronę też
	}
}

bool TDynamicObject::FastUpdate(double dt)
{
    if (dt == 0.0)
        return true; // Ra: pauza
    double dDOMoveLen;
    if (!MoverParameters->PhysicActivation)
        return true; // McZapkie: wylaczanie fizyki gdy nie potrzeba

    if (!bEnabled)
        return false;

    // NOTE: coordinate system swap
    // TODO: replace with regular glm vectors
    TLocation const l {
        -vPosition.x,
         vPosition.z,
         vPosition.y };
    TRotation const r {
        0.0,
        0.0,
        modelRot.z };
    // McZapkie: parametry powinny byc pobierane z toru
    // ts.R=MyTrack->fRadius;
    // ts.Len= Max0R(MoverParameters->BDist,MoverParameters->ADist);
    // ts.dHtrack=Axle1.pPosition.y-Axle0.pPosition.y;
    // ts.dHrail=((Axle1.GetRoll())+(Axle0.GetRoll()))*0.5f;
    // tp.Width=MyTrack->fTrackWidth;
    // McZapkie-250202
    // tp.friction= MyTrack->fFriction;
    // tp.CategoryFlag= MyTrack->iCategoryFlag&15;
    // tp.DamageFlag=MyTrack->iDamageFlag;
    // tp.QualityFlag=MyTrack->iQualityFlag;
    dDOMoveLen = MoverParameters->FastComputeMovement(dt, ts, tp, l, r); // ,ts,tp,tmpTraction);
    // Move(dDOMoveLen);
    // ResetdMoveLen();
    FastMove(dDOMoveLen);

    if( MoverParameters->LoadStatus ) {
        LoadUpdate(); // zmiana modelu ładunku
    }
    update_exchange( dt );

    return true; // Ra: chyba tak?
}

// McZapkie-040402: liczenie pozycji uwzgledniajac wysokosc szyn itp.
// vector3 TDynamicObject::GetPosition()
//{//Ra: pozycja pojazdu jest liczona zaraz po przesunięciu
// return vPosition;
//};

void TDynamicObject::TurnOff()
{ // wyłączenie rysowania submodeli zmiennych dla
    // egemplarza pojazdu
    btnOn = false;
    btCoupler1.TurnOff();
    btCoupler2.TurnOff();
    btCPneumatic1.TurnOff();
    btCPneumatic1r.TurnOff();
    btCPneumatic2.TurnOff();
    btCPneumatic2r.TurnOff();
    btPneumatic1.TurnOff();
    btPneumatic1r.TurnOff();
    btPneumatic2.TurnOff();
    btPneumatic2r.TurnOff();
    btCCtrl1.Turn( false );
    btCCtrl2.Turn( false );
    btCPass1.Turn( false );
    btCPass2.Turn( false );
    m_endsignal12.Turn( false );
    m_endsignal13.Turn( false );
    m_endsignal22.Turn( false );
    m_endsignal23.Turn( false );
    m_endsignals1.Turn( false );
    m_endsignals2.Turn( false );
    m_endtab1.Turn( false );
    m_endtab2.Turn( false );
    m_headlamp11.TurnOff();
    m_headlamp12.TurnOff();
    m_headlamp13.TurnOff();
    m_headlamp21.TurnOff();
    m_headlamp22.TurnOff();
    m_headlamp23.TurnOff();
    m_headsignal12.TurnOff();
    m_headsignal13.TurnOff();
    m_headsignal22.TurnOff();
    m_headsignal23.TurnOff();
	btMechanik1.Turn( false );
	btMechanik2.Turn( false );
    btShutters1.Turn( false );
    btShutters2.Turn( false );
};

// przeliczanie dźwięków, bo będzie słychać bez wyświetlania sektora z pojazdem
void TDynamicObject::RenderSounds() {

    if( false == simulation::is_ready ) { return; }
    if( Global.iPause != 0 )            { return; }

    if( ( m_startjoltplayed )
     && ( ( std::abs( MoverParameters->AccSVBased ) < 0.01 )
       || ( GetVelocity() < 0.01 ) ) ) {
        // if the vehicle comes to a stop set the movement jolt to play when it starts moving again
        m_startjoltplayed = false;
    }

    auto const dt{ Timer::GetDeltaRenderTime() };
    m_powertrainsounds.render( *MoverParameters, dt );

    auto volume{ 0.0 };
    auto frequency{ 1.0 };

    // NBMX dzwiek przetwornicy
    if( MoverParameters->ConverterFlag ) {
        if( MoverParameters->EngineType == TEngineType::ElectricSeriesMotor ) {
            auto const voltage { std::max( MoverParameters->GetTrainsetHighVoltage(), MoverParameters->PantographVoltage ) };
            if( voltage > 0.0 ) {
                // NOTE: we do sound modulation here to avoid sudden jump on voltage loss
                frequency = ( voltage / ( MoverParameters->NominalVoltage * MoverParameters->RList[ MoverParameters->RlistSize ].Mn ) );
                frequency *= sConverter.m_frequencyfactor + sConverter.m_frequencyoffset;
                sConverter.pitch( clamp( frequency, 0.75, 1.25 ) ); // arbitrary limits )
            }
        }
        else {
            frequency = sConverter.m_frequencyoffset + sConverter.m_frequencyfactor * frequency;
            sConverter.pitch( clamp( frequency, 0.75, 1.25 ) ); // arbitrary limits )
        }
        sConverter.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sConverter.stop();
    }

    if( MoverParameters->CompressorSpeed > 0.0 ) {
        // McZapkie! - dzwiek compressor.wav tylko gdy dziala sprezarka
        if( MoverParameters->CompressorFlag ) {
            // for compressor coupled with the diesel engine sound pitch is driven by engine revolutions
            if( MoverParameters->CompressorPower == 3 ) {
                // presume the compressor sound is recorded for idle revolutions
                // increase the pitch according to increase of engine revolutions
                auto const enginefactor {
                    clamp( // try to keep the sound pitch in semi-reasonable range
                        MoverParameters->EngineMaxRPM() / MoverParameters->EngineIdleRPM() * MoverParameters->EngineRPMRatio(),
                        0.5, 2.5 ) };
                sCompressor.pitch( enginefactor );
                sCompressorIdle.pitch( enginefactor );
            }
            if( sCompressorIdle.empty() ) {
                // legacy sound path, if there's no dedicated idle sound
                sCompressor.play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                // enhanced sound path, with dedicated sound for idling compressor
                if( MoverParameters->CompressorGovernorLock ) {
                    sCompressor.stop();
                    sCompressorIdle.play( sound_flags::exclusive | sound_flags::looping );
                }
                else {
                    sCompressor.play( sound_flags::exclusive | sound_flags::looping );
                    sCompressorIdle.stop();
                }
            }
        }
        else {
            sCompressor.stop();
            sCompressorIdle.stop();
        }
    }

    // Winger 160404 - dzwiek malej sprezarki
    if( MoverParameters->PantCompFlag ) {
        sSmallCompressor.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sSmallCompressor.stop();
    }

    // heater sound
    {
        auto const isdieselenginepowered { ( MoverParameters->EngineType == TEngineType::DieselElectric ) || ( MoverParameters->EngineType == TEngineType::DieselEngine ) };
        if( ( true == MoverParameters->Heating )
         && ( ( false == isdieselenginepowered )
           || ( std::abs( MoverParameters->enrot ) > 0.01 ) ) ) {
            sHeater
                .pitch( true == sHeater.is_combined() ?
                    std::abs( MoverParameters->enrot ) * 60.f * 0.01f :
                    1.f )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            sHeater.stop();
        }
    }

    // battery sound
    if( MoverParameters->Battery ) {
        m_batterysound.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        m_batterysound.stop();
    }

    // brake system and braking sounds:

    // brake cylinder piston
    auto const brakepressureratio { std::max( 0.0, MoverParameters->BrakePress ) / std::max( 1.0, MoverParameters->MaxBrakePress[ 3 ] ) };
    if( m_lastbrakepressure != -1.f ) {
        // HACK: potentially reset playback of opening bookend sounds
        if( false == m_brakecylinderpistonadvance.is_playing() ) {
            m_brakecylinderpistonadvance.stop();
        }
        if( false == m_brakecylinderpistonrecede.is_playing() ) {
            m_brakecylinderpistonrecede.stop();
        }
        // actual sound playback
        auto const quantizedratio { static_cast<int>( 15 * brakepressureratio ) };
        auto const lastbrakepressureratio { std::max( 0.f, m_lastbrakepressure ) / std::max( 1.0, MoverParameters->MaxBrakePress[ 3 ] ) };
        auto const quantizedratiochange { quantizedratio - static_cast<int>( 15 * lastbrakepressureratio ) };
        if( quantizedratiochange > 0 ) {
            m_brakecylinderpistonadvance
                .pitch(
                    true == m_brakecylinderpistonadvance.is_combined() ?
                        quantizedratio * 0.01f :
                        m_brakecylinderpistonadvance.m_frequencyoffset + m_brakecylinderpistonadvance.m_frequencyfactor * 1.f )
                .play();
        }
        else if( quantizedratiochange < 0 ) {
            m_brakecylinderpistonrecede
                .pitch( true == m_brakecylinderpistonrecede.is_combined() ?
                        quantizedratio * 0.01f :
                        m_brakecylinderpistonrecede.m_frequencyoffset + m_brakecylinderpistonrecede.m_frequencyfactor * 1.f )
                .play();
        }
    }

	// epbrake - epcompact
	if (( MoverParameters->BrakeSystem == TBrakeSystem::ElectroPneumatic ) && ( MoverParameters->LocHandle )) {
		auto const epbrakepressureratio{ std::min( std::max( 0.0, MoverParameters->LocHandle->GetCP() ) / std::max( 1.0, MoverParameters->MaxBrakePress[0] ),
												   m_epbrakepressurechangedectimer > -1.0f ?
													   std::max( MoverParameters->LocalBrakePosAEIM, MoverParameters->Hamulec->GetEDBCP() / MoverParameters->MaxBrakePress[3]) :
														1.0 ) };
		if ( m_lastepbrakepressure != -1.f ) {
			// HACK: potentially reset playback of opening bookend sounds
			if ( false == m_epbrakepressureincrease.is_playing() ) {
				m_epbrakepressureincrease.stop();
			}
			if ( false == m_epbrakepressuredecrease.is_playing() ) {
				m_epbrakepressuredecrease.stop();
			}
			m_epbrakepressurechangeinctimer += dt;
			m_epbrakepressurechangedectimer += dt;
			// actual sound playback
			auto const epquantizedratio{ static_cast<int>( 50 * epbrakepressureratio) };
			auto const lastepbrakepressureratio { std::max( 0.f, m_lastepbrakepressure ) / std::max( 1.0, MoverParameters->MaxBrakePress[0] ) };
			auto const epquantizedratiochange { epquantizedratio - static_cast<int>( 50 * lastepbrakepressureratio ) };
			if ( epquantizedratiochange > 0 && m_epbrakepressurechangeinctimer > 0.05f) {
				m_epbrakepressureincrease
					.pitch(
						true == m_epbrakepressureincrease.is_combined() ?
						epquantizedratio * 0.01f :
						m_epbrakepressureincrease.m_frequencyoffset + m_epbrakepressureincrease.m_frequencyfactor * 1.f )
					.play();
				m_epbrakepressurechangeinctimer = 0;
			}
			else if ( epquantizedratiochange < 0 && m_epbrakepressurechangedectimer > 0.3f) {
				m_epbrakepressuredecrease
					.pitch(true == m_epbrakepressuredecrease.is_combined() ?
						-epquantizedratiochange * 0.01f :
						m_epbrakepressuredecrease.m_frequencyoffset + m_epbrakepressuredecrease.m_frequencyfactor * 1.f )
					.play();
				m_epbrakepressurechangedectimer = 0;
			}
		}
		if ( ( m_epbrakepressurechangeinctimer == 0 ) || ( m_epbrakepressurechangedectimer == 0 ) )
		m_lastepbrakepressure = std::min( MoverParameters->LocHandle->GetCP(),
										  MoverParameters->LocalBrakePosAEIM * std::max( 1.0, MoverParameters->MaxBrakePress[0] ) );
	}

    // emergency brake
    if( MoverParameters->EmergencyValveFlow > 0.025 ) {
        // smooth out air flow rate
        m_emergencybrakeflow = (
            m_emergencybrakeflow == 0.0 ?
                MoverParameters->EmergencyValveFlow :
                interpolate( m_emergencybrakeflow, MoverParameters->EmergencyValveFlow, 0.1 ) );
        // scale volume based on the flow rate and on the pressure in the main pipe
        auto const flowpressure { clamp( m_emergencybrakeflow, 0.0, 1.0 ) + clamp( 0.1 * MoverParameters->PipePress, 0.0, 0.5 ) };
         m_emergencybrake
            .pitch( m_emergencybrake.m_frequencyoffset + 1.0 * m_emergencybrake.m_frequencyfactor )
            .gain( m_emergencybrake.m_amplitudeoffset + clamp( flowpressure, 0.0, 1.0 ) * m_emergencybrake.m_amplitudefactor )
            .play( sound_flags::exclusive | sound_flags::looping );
   }
    else if( MoverParameters->EmergencyValveFlow < 0.015 ) {
        m_emergencybrakeflow = 0.0;
        m_emergencybrake.stop();
    }

    // air release
    if( m_lastbrakepressure != -1.f ) {
        // calculate rate of pressure drop in brake cylinder, once it's been initialized
        auto const brakepressuredifference{ m_lastbrakepressure - MoverParameters->BrakePress };
        m_brakepressurechange = interpolate<float>( m_brakepressurechange, brakepressuredifference / dt, 0.05f );
    }
    m_lastbrakepressure = MoverParameters->BrakePress;
    // ensure some basic level of volume and scale it up depending on pressure in the cylinder; scale this by the air release rate
    volume = rsUnbrake.m_amplitudefactor * m_brakepressurechange * ( 0.25 + 0.75 * brakepressureratio );
    if( ( m_brakepressurechange > 0.05 ) && ( brakepressureratio > 0.05 ) ) {
        rsUnbrake
            .gain( volume )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        // don't stop the sound too abruptly
        volume = std::max( 0.0, rsUnbrake.gain() - 0.5 * dt );
        rsUnbrake.gain( volume );
        if( volume < 0.05 ) {
            rsUnbrake.stop();
        }
    }

    // Dzwiek odluzniacza
    if( MoverParameters->Hamulec->Releaser() ) {
        sReleaser
            .gain(
                clamp<float>(
                    MoverParameters->BrakePress * 1.25f, // arbitrary multiplier
                    0.f, 1.f ) )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sReleaser.stop();
    }

    // slipping wheels
    if( MoverParameters->SlippingWheels ) {

        if( ( MoverParameters->UnitBrakeForce > 100.0 )
            && ( GetVelocity() > 1.0 ) ) {

            auto const velocitydifference{ GetVelocity() / MoverParameters->Vmax };
            rsSlippery
                .gain( rsSlippery.m_amplitudeoffset + rsSlippery.m_amplitudefactor * velocitydifference )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
    }
    else {
        rsSlippery.stop();
    }

    // Dzwiek piasecznicy
    if( MoverParameters->SandDose ) {
        sSand.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sSand.stop();
    }

    // brakes
    auto brakeforceratio{ 0.0 };
    if( //( false == mvOccupied->SlippingWheels ) &&
        ( MoverParameters->UnitBrakeForce > 10.0 )
     && ( MoverParameters->Vel > 0.05 ) ) {

        brakeforceratio =
            clamp(
                MoverParameters->UnitBrakeForce / std::max( 1.0, MoverParameters->BrakeForceR( 1.0, MoverParameters->Vel ) / ( MoverParameters->NAxles * std::max( 1, MoverParameters->NBpA ) ) ),
                0.0, 1.0 );
        rsBrake
            .pitch( rsBrake.m_frequencyoffset + MoverParameters->Vel * rsBrake.m_frequencyfactor )
            .gain( rsBrake.m_amplitudeoffset + std::sqrt( brakeforceratio * interpolate( 0.4, 1.0, ( MoverParameters->Vel / ( 1 + MoverParameters->Vmax ) ) ) ) * rsBrake.m_amplitudefactor )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        rsBrake.stop();
    }

    // yB: przyspieszacz (moze zadziala, ale dzwiek juz jest)
    if( true == bBrakeAcc ) {
        if( true == TestFlag( MoverParameters->Hamulec->GetSoundFlag(), sf_Acc ) ) {
            sBrakeAcc.play( sound_flags::exclusive );
        }
    }

    // McZapkie-280302 - pisk mocno zacisnietych hamulcow
    if( MoverParameters->Vel > 2.5 ) {

        volume = rsPisk.m_amplitudeoffset + interpolate( -1.0, 1.0, brakeforceratio ) * rsPisk.m_amplitudefactor;
        if( volume > 0.075 ) {
            rsPisk
                .pitch(
                    true == rsPisk.is_combined() ?
                        MoverParameters->Vel * 0.01f :
                        rsPisk.m_frequencyoffset + rsPisk.m_frequencyfactor * 1.f )
                .gain( volume )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
    }
    else {
        // don't stop the sound too abruptly
        volume = std::max( 0.0, rsPisk.gain() - ( rsPisk.gain() * 2.5 * dt ) );
        rsPisk.gain( volume );
    }
    if( volume < 0.05 ) {
        rsPisk.stop();
    }

    // spring brake
    if( m_springbrakesounds.state != MoverParameters->SpringBrake.Activate ) {
        m_springbrakesounds.state = MoverParameters->SpringBrake.Activate;
        if( m_springbrakesounds.state ) {
            m_springbrakesounds.activate.play( sound_flags::exclusive );
            m_springbrakesounds.release.stop();
        }
        else {
            m_springbrakesounds.activate.stop();
            m_springbrakesounds.release.play( sound_flags::exclusive );
        }
    }

    // other sounds
    // load exchange
    if( MoverParameters->LoadStatus & 1 ) {
        m_exchangesounds.unloading.play( sound_flags::exclusive );
    }
    else {
        m_exchangesounds.unloading.stop();
    }
    if( MoverParameters->LoadStatus & 2 ) {
        m_exchangesounds.loading.play( sound_flags::exclusive );
    }
    else {
        m_exchangesounds.loading.stop();
    }
    // NBMX sygnal odjazdu
    if( MoverParameters->Doors.has_warning ) {
        auto const lowvoltagepower { MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable };
        for( auto &doorspeaker : m_doorspeakers ) {
            // TBD, TODO: per-location door state triggers?
            if( ( MoverParameters->DepartureSignal )
             && ( lowvoltagepower )
/*
             || ( ( MoverParameters->DoorCloseCtrl = control::autonomous )
               && ( ( ( false == MoverParameters->DoorLeftOpened )  && ( dDoorMoveL > 0.0 ) )
                 || ( ( false == MoverParameters->DoorRightOpened ) && ( dDoorMoveR > 0.0 ) ) ) )
*/
                 ) {
                // for the autonomous doors play the warning automatically whenever a door is closing
                // MC: pod warunkiem ze jest zdefiniowane w chk
                doorspeaker.departure_signal.play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                doorspeaker.departure_signal.stop();
            }
        }
    }
    // announcements
    {
        auto const lowvoltagepower { MoverParameters->Power24vIsAvailable || MoverParameters->Power110vIsAvailable };
        if( lowvoltagepower ) {
            // system is powered up, can play queued announcements
            if( ( false == m_pasystem.announcement_queue.empty() )
             && ( false == m_pasystem.announcement.is_playing() ) ) {
                // pull first sound from the queue
                m_pasystem.announcement = m_pasystem.announcement_queue.front();
                m_pasystem.announcement.owner( this );
                m_pasystem.announcement.range( 0.5 * MoverParameters->Dim.L * -1 );
                if( m_pasystem.soundproofing ) {
                    if( !m_pasystem.announcement.soundproofing() ) {
                        m_pasystem.announcement.soundproofing() = m_pasystem.soundproofing;
                    }
                }
                m_pasystem.announcement.play();
                m_pasystem.announcement_queue.pop_front();
            }
        }
        else {
            m_pasystem.announcement.stop();
            m_pasystem.announcement_queue.clear();
        }
    }
    // NBMX Obsluga drzwi, MC: zuniwersalnione
    std::array<side, 2> const sides { side::right, side::left };
    for( auto const side : sides ) {

        auto const &door { MoverParameters->Doors.instances[ side ] };

        if( true == door.is_opening ) {
            // door sounds
            if( ( false == door.step_unfolding ) // no wait if no doorstep
             || ( MoverParameters->Doors.step_type == 2 ) ) { // no wait for rotating doorstep
                if( door.position < 0.5f ) { // safety measure, to keep slightly too short sounds from repeating
                    for( auto &doorsounds : m_doorsounds ) {
                        if( doorsounds.placement == side ) {
                            // determine left side doors from their offset
                            doorsounds.rsDoorOpen.play( sound_flags::exclusive );
                            doorsounds.rsDoorClose.stop();
                        }
                    }
                }
            }
        }
        if( true == door.is_closing ) {
            // door sounds can start playing before the door begins moving but shouldn't cease once the door closes
            if( door.position > 0.f ) {
                for( auto &doorsounds : m_doorsounds ) {
                    if( doorsounds.placement == side ) {
                        // determine left side doors from their offset
                        doorsounds.rsDoorClose.play( sound_flags::exclusive );
                        doorsounds.rsDoorOpen.stop();
                    }
                }
            }
        }
        // doorstep sounds
        if( door.step_unfolding ) {
            for( auto &doorsounds : m_doorsounds ) {
                if( doorsounds.placement == side ) {
                    doorsounds.step_open.play( sound_flags::exclusive );
                    doorsounds.step_close.stop();
                }
            }
        }
        if( door.step_folding ) {
            if( door.step_position < 1.0f ) { // sanity check, the vehicles may keep the doorstep unfolded until the door close
                for( auto &doorsounds : m_doorsounds ) {
                    if( doorsounds.placement == side ) {
                        // determine left side doors from their offset
                        doorsounds.step_close.play( sound_flags::exclusive );
                        doorsounds.step_open.stop();
                    }
                }
            }
        }
    }
    // door locks
    if( MoverParameters->Doors.is_locked != m_doorlocks ) {
        // toggle state of the locks...
        m_doorlocks = !m_doorlocks;
        // ...and play relevant sounds
        for( auto &door : m_doorsounds ) {
            if( m_doorlocks ) {
                door.lock.play( sound_flags::exclusive );
            }
            else {
                door.unlock.play( sound_flags::exclusive );
            }
        }
    }

    // horns
    {
        // for moving vehicle combine regular horn activation flag with emergency brake horn activation flag, if the brake is active
        auto const warningsignal { (
            ( MoverParameters->Vel > 0.5 ) && ( MoverParameters->AlarmChainFlag ) ?
                MoverParameters->EmergencyBrakeWarningSignal :
                0 )
            | ( MoverParameters->WarningSignal ) };

        if( TestFlag( warningsignal, 1 ) ) {
            sHorn1.play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            sHorn1.stop();
        }
        if( TestFlag( warningsignal, 2 ) ) {
            sHorn2.play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            sHorn2.stop();
        }
        if( TestFlag( warningsignal, 4 ) ) {
            sHorn3.play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            sHorn3.stop();
        }
    }
    // szum w czasie jazdy
    if( ( GetVelocity() > 0.5 )
#ifdef EU07_SOUND_BOGIESOUNDS
     && ( false == m_bogiesounds.empty() )
#endif
     && ( // compound test whether the vehicle belongs to user-driven consist (as these don't emit outer noise in cab view)
            FreeFlyModeFlag ? true : // in external view all vehicles emit outer noise
            // Global.pWorld->train() == nullptr ? true : // (can skip this check, with no player train the external view is a given)
            ctOwner == nullptr ? true : // standalone vehicle, can't be part of user-driven train
            ctOwner != simulation::Train->Dynamic()->ctOwner ? true : // confirmed isn't a part of the user-driven train
            Global.CabWindowOpen ? true : // sticking head out we get to hear outer noise
            false ) ) {
#ifdef EU07_SOUND_BOGIESOUNDS
        auto const &bogiesound { m_bogiesounds.front() };
#else
        auto const &bogiesound { m_outernoise };
#endif
        // frequency calculation
        auto const normalizer { (
            true == bogiesound.is_combined() ?
                MoverParameters->Vmax * 0.01f :
                1.f ) };
        frequency =
            bogiesound.m_frequencyoffset
            + bogiesound.m_frequencyfactor * MoverParameters->Vel * normalizer;

        // volume calculation
        volume =
            bogiesound.m_amplitudeoffset
            + bogiesound.m_amplitudefactor * MoverParameters->Vel;
        if( brakeforceratio > 0.0 ) {
            // hamulce wzmagaja halas
            volume *= 1 + 0.125 * brakeforceratio;
        }
        // scale volume by track quality
        // TODO: track quality and/or environment factors as separate subroutine
        volume *=
            interpolate(
                0.8, 1.2,
                clamp(
                    MyTrack->iQualityFlag / 20.0,
                    0.0, 1.0 ) );
        // for single sample sounds muffle the playback at low speeds
        if( false == bogiesound.is_combined() ) {
            volume *=
                interpolate(
                    0.0, 1.0,
                    clamp(
                        MoverParameters->Vel / 40.0,
                        0.0, 1.0 ) );
        }

        if( volume > 0.05 ) {
            // apply calculated parameters to all motor instances
#ifdef EU07_SOUND_BOGIESOUNDS
            for( auto &bogiesound : m_bogiesounds ) {
                bogiesound
                    .pitch( frequency ) // arbitrary limits to prevent the pitch going out of whack
                    .gain( volume )
                    .play( sound_flags::exclusive | sound_flags::looping );
            }
#else
            m_outernoise
                .pitch( frequency ) // arbitrary limits to prevent the pitch going out of whack
                .gain( volume )
                .play( sound_flags::exclusive | sound_flags::looping );
#endif
        }
        else {
            // stop all noise instances
#ifdef EU07_SOUND_BOGIESOUNDS
            for( auto &bogiesound : m_bogiesounds ) {
                bogiesound.stop();
            }
#else
            m_outernoise.stop();
#endif
        }
    }
    else {
        // don't play the optional ending sound if the listener switches views
#ifdef EU07_SOUND_BOGIESOUNDS
        for( auto &bogiesound : m_bogiesounds ) {
            bogiesound.stop( false == FreeFlyModeFlag );
        }
#else
        m_outernoise.stop( false == FreeFlyModeFlag );
#endif
    }
    // flat spot sound
    if( MoverParameters->CategoryFlag == 1 ) {
        // trains only
        if( ( GetVelocity() > 1.0 )
         && ( MoverParameters->WheelFlat > 5.0 ) ) {
            m_wheelflat
                .pitch( m_wheelflat.m_frequencyoffset + std::abs( MoverParameters->nrot ) * m_wheelflat.m_frequencyfactor )
                .gain( m_wheelflat.m_amplitudeoffset + m_wheelflat.m_amplitudefactor * ( ( 1.0 + ( MoverParameters->Vel / MoverParameters->Vmax ) + clamp( MoverParameters->WheelFlat / 60.0, 0.0, 1.0 ) ) / 3.0 ) )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            m_wheelflat.stop();
        }
    }

    // youBy: dzwiek ostrych lukow i ciasnych zwrotek
    if( ( MoverParameters->Vel > 5.0 )
     && ( ts.R * ts.R > 1.0 )
     && ( std::abs( ts.R ) < 15000.0 ) ) {
        // scale volume with curve radius and vehicle speed
        volume =
            std::abs( MoverParameters->AccN ) // * MoverParameters->AccN
            * interpolate(
                0.5, 1.0,
                clamp(
                    MoverParameters->Vel / 40.0,
                    0.0, 1.0 ) )
            * ( ( ( MyTrack->eType == tt_Switch ) && ( std::abs( ts.R ) < 1500.0 ) ) ? 100.0 : 1.0 );
    }
    else {
        volume = 0;
    }
    if( volume > 0.05 ) {
        rscurve
            .pitch(
                true == rscurve.is_combined() ?
                    MoverParameters->Vel * 0.01f :
                    rscurve.m_frequencyoffset + rscurve.m_frequencyfactor * 1.f )
            .gain( 2.5 * volume )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        rscurve.stop();
    }

    // movement start jolt
    if( false == m_startjoltplayed ) {
        auto const velocity { GetVelocity() };
        if( ( MoverParameters->V > 0.0 ? ( MoverParameters->AccSVBased > 0.1 ) : ( MoverParameters->AccSVBased < 0.1 ) )
         && ( velocity >  1.0 )
         && ( velocity < 15.0 ) ) {
            m_startjolt.play( sound_flags::exclusive );
            m_startjoltplayed = true;
        }
    }

    // McZapkie! - to wazne - SoundFlag wystawiane jest przez moje moduly gdy zachodza pewne wydarzenia komentowane dzwiekiem.
    // pneumatic relay
    if( TestFlag( MoverParameters->SoundFlag, sound::pneumatic ) ) {
        dsbPneumaticRelay
            .gain(
                true == TestFlag( MoverParameters->SoundFlag, sound::loud ) ?
                    1.0f :
                    0.8f )
            .play();
    }
    // door permit
    if( TestFlag( MoverParameters->SoundFlag, sound::doorpermit ) ) {
        // NOTE: current implementation doesn't discern between permit for left/right side,
        // which may be undesired in weird setups with doors only on one side
        // TBD, TODO: rework into dedicated sound event flag for each door location instance?
        for( auto &door : m_doorsounds ) {
            door.permit_granted.play( sound_flags::exclusive );
        }
    }

    // couplers
    int couplerindex { 0 };
    for( auto &couplersounds : m_couplersounds ) {

        auto &coupler { MoverParameters->Couplers[ couplerindex ] };

        if( coupler.sounds == sound::none ) {
            ++couplerindex;
            continue;
        }

        if( true == TestFlag( coupler.sounds, sound::bufferclash ) ) {
            // zderzaki uderzaja o siebie
            if( true == TestFlag( coupler.sounds, sound::loud ) ) {
                // loud clash
                if( false == couplersounds.dsbBufferClamp_loud.empty() ) {
                    // dedicated sound for loud clash
                    couplersounds.dsbBufferClamp_loud
                        .gain( 1.f )
                        .play( sound_flags::exclusive );
                }
                else {
                    // fallback on the standard sound
                    couplersounds.dsbBufferClamp
                        .gain( 1.f )
                        .play( sound_flags::exclusive );
                }
            }
            else {
                // basic clash
                couplersounds.dsbBufferClamp
                    .gain( 1.f )
                    .play( sound_flags::exclusive );
            }
        }

        if( true == TestFlag( coupler.sounds, sound::couplerstretch ) ) {
            // sprzegi sie rozciagaja
            if( true == TestFlag( coupler.sounds, sound::loud ) ) {
                // loud stretch
                if( false == couplersounds.dsbCouplerStretch_loud.empty() ) {
                    // dedicated sound for loud stretch
                    couplersounds.dsbCouplerStretch_loud
                        .gain( 1.f )
                        .play( sound_flags::exclusive );
                }
                else {
                    // fallback on the standard sound
                    couplersounds.dsbCouplerStretch
                        .gain( 1.f )
                        .play( sound_flags::exclusive );
                }
            }
            else {
                // basic clash
                couplersounds.dsbCouplerStretch
                    .gain( 1.f )
                    .play( sound_flags::exclusive );
            }
        }
        // attach/detach sounds
        if( ( coupler.sounds & sound::detach ) == 0 ) {
            // potentially added some couplings
            if( ( coupler.sounds & sound::attachcoupler ) != 0 ) {
                couplersounds.attach_coupler.play();
            }
            if( ( coupler.sounds & sound::attachbrakehose ) != 0 ) {
                couplersounds.attach_brakehose.play();
            }
            if( ( coupler.sounds & sound::attachmainhose ) != 0 ) {
                couplersounds.attach_mainhose.play();
            }
            if( ( coupler.sounds & sound::attachcontrol ) != 0 ) {
                couplersounds.attach_control.play();
            }
            if( ( coupler.sounds & sound::attachgangway ) != 0 ) {
                couplersounds.attach_gangway.play();
            }
            if( ( coupler.sounds & sound::attachheating ) != 0 ) {
                couplersounds.attach_heating.play();
            }
        }
        else {
            // potentially removed some couplings
            if( ( coupler.sounds & sound::attachcoupler ) != 0 ) {
                couplersounds.detach_coupler.play();
            }
            if( ( coupler.sounds & sound::attachbrakehose ) != 0 ) {
                couplersounds.detach_brakehose.play();
            }
            if( ( coupler.sounds & sound::attachmainhose ) != 0 ) {
                couplersounds.detach_mainhose.play();
            }
            if( ( coupler.sounds & sound::attachcontrol ) != 0 ) {
                couplersounds.detach_control.play();
            }
            if( ( coupler.sounds & sound::attachgangway ) != 0 ) {
                couplersounds.detach_gangway.play();
            }
            if( ( coupler.sounds & sound::attachheating ) != 0 ) {
                couplersounds.detach_heating.play();
            }
        }
        if( true == TestFlag( coupler.sounds, sound::attachadapter ) ) {
            couplersounds.dsbAdapterAttach.play();
        }
        if( true == TestFlag( coupler.sounds, sound::removeadapter ) ) {
            couplersounds.dsbAdapterRemove.play();
        }

        ++couplerindex;
        coupler.sounds = 0;
    }

    MoverParameters->SoundFlag = 0;
    // McZapkie! - koniec obslugi dzwiekow z mover.pas

// special events
    if( MoverParameters->EventFlag ) {
        // McZapkie: w razie wykolejenia
        if( true == TestFlag( MoverParameters->DamageFlag, dtrain_out ) ) {
            rsDerailment.play( sound_flags::exclusive );
        }

        MoverParameters->EventFlag = false;
    }
}

// calculates distance between event-starting axle and front of the vehicle
double
TDynamicObject::tracing_offset() const {

    auto const axletoend{ ( GetLength() - fAxleDist ) * 0.5 };
    return (
        iAxleFirst ?
            axletoend :
            axletoend + iDirection * fAxleDist );
}

// TODO: compute and cache radius during vehicle initialization
double
TDynamicObject::radius() const {

    glm::vec3 diagonal(
        static_cast<float>( MoverParameters->Dim.L ),
        static_cast<float>( MoverParameters->Dim.H ),
        static_cast<float>( MoverParameters->Dim.W ) );
    return glm::length( diagonal ) * 0.5f;
}

// McZapkie-250202
// wczytywanie pliku z danymi multimedialnymi (dzwieki)
void TDynamicObject::LoadMMediaFile( std::string const &TypeName, std::string const &ReplacableSkin ) {

    Global.asCurrentDynamicPath = asBaseDir;
    std::string asFileName = asBaseDir + TypeName + ".mmd";
    std::string asAnimName;
    bool Stop_InternalData = false;
    pants = NULL; // wskaźnik pierwszego obiektu animującego dla pantografów
    {
        // preliminary check whether the file exists
        cParser parser( TypeName + ".mmd", cParser::buffer_FILE, asBaseDir );
        if( false == parser.ok() ) {
            ErrorLog( "Failed to load appearance data for vehicle " + MoverParameters->Name );
            return;
        }
    }
    // use #include wrapper to access the appearance data file
    // this allows us to provide the file content with user-defined parameters
    cParser parser(
        "include " + TypeName + ".mmd"
        + " " + asName // (p1)
        + " " + TypeName // (p2)
        + " " + ReplacableSkin // (p3)
        + " end",
        cParser::buffer_TEXT,
        asBaseDir );
	parser.allowRandomIncludes = true;
	std::string token;
    do {
		token = "";
		parser.getTokens(); parser >> token;

		if( token == "models:" ) {
			// modele i podmodele
            m_materialdata.multi_textures = 0; // czy jest wiele tekstur wymiennych?
			parser.getTokens();
			parser >> asModel;
            replace_slashes( asModel );
            if( asModel.back() == '#' ) // Ra 2015-01: nie podoba mi siê to
            { // model wymaga wielu tekstur wymiennych
                m_materialdata.multi_textures = 1;
                asModel.erase( asModel.length() - 1 );
            }
            // name can contain leading slash, erase it to avoid creation of double slashes when the name is combined with current directory
            erase_leading_slashes( asModel );
            /*
            // never really used, may as well get rid of it
            std::size_t i = asModel.find( ',' );
            if ( i != std::string::npos )
            { // Ra 2015-01: może szukać przecinka w nazwie modelu, a po przecinku była by liczba tekstur?
                if( i < asModel.length() ) {
                    m_materialdata.multi_textures = asModel[ i + 1 ] - '0';
                }
                m_materialdata.multi_textures = clamp( m_materialdata.multi_textures, 0, 1 ); // na razie ustawiamy na 1
            }
            */
            asModel = asBaseDir + asModel; // McZapkie 2002-07-20: dynamics maja swoje modele w dynamics/basedir
            Global.asCurrentTexturePath = asBaseDir; // biezaca sciezka do tekstur to dynamic/...
            mdModel = TModelsManager::GetModel(asModel, true);
            if (ReplacableSkin != "none") {
                m_materialdata.assign( ReplacableSkin );
            }
            Global.asCurrentTexturePath = szTexturePath; // z powrotem defaultowa sciezka do tekstur
            do {
				token = "";
				parser.getTokens(); parser >> token;

				if( token == "animations:" ) {
					// Ra: ustawienie ilości poszczególnych animacji - musi być jako pierwsze, inaczej ilości będą domyślne
/*
                    if( nullptr == pAnimations )
*/
                    if( true == pAnimations.empty() )
                    { // jeśli nie ma jeszcze tabeli animacji, można odczytać nowe ilości
                        int co = 0;
                        iAnimations = 0;
                        int ile;
                        do
                        { // kolejne liczby to ilość animacj, -1 to znacznik końca
                            ile = -1;
							parser.getTokens( 1, false );
                            parser >> ile; // ilość danego typu animacji
                            if (ile >= 0)
                            {
                                iAnimType[co] = ile; // zapamiętanie
                                iAnimations += ile; // ogólna ilość animacji
                            }
                            else {
                                iAnimType[co] = 0;
                            }
                            ++co;
                        } while ( (ile >= 0) && (co < ANIM_TYPES) ); //-1 to znacznik końca

                        pAnimations.resize( iAnimations );
                        int i, j, k = 0, sm = 0;
                        for (j = 0; j < ANIM_TYPES; ++j)
                            for (i = 0; i < iAnimType[j]; ++i)
                            {
                                if (j == ANIM_PANTS) // zliczamy poprzednie animacje
                                    if (!pants)
                                        if (iAnimType[ANIM_PANTS]) // o ile jakieś pantografy są (a domyślnie są)
                                            pants = &pAnimations[k]; // zapamiętanie na potrzeby wyszukania submodeli
                                pAnimations[k].iShift = sm; // przesunięcie do przydzielenia wskaźnika
                                sm += pAnimations[k++].TypeSet(j); // ustawienie typu animacji i zliczanie tablicowanych submodeli
                            }
                        if (sm) // o ile są bardziej złożone animacje
                        {
                            pAnimated = new TSubModel *[sm]; // tabela na animowane submodele
                            for (k = 0; k < iAnimations; ++k)
                                pAnimations[k].smElement = pAnimated + pAnimations[k].iShift; // przydzielenie wskaźnika do tabelki
                        }
                    }
                }

                else if(token == "lowpolyinterior:") {
					// ABu: wnetrze lowpoly
					parser.getTokens();
					parser >> asModel;

                    replace_slashes( asModel );
                    erase_leading_slashes( asModel );
                    asModel = asBaseDir + asModel; // McZapkie-200702 - dynamics maja swoje modele w dynamic/basedir
                    Global.asCurrentTexturePath = asBaseDir; // biezaca sciezka do tekstur to dynamic/...
                    mdLowPolyInt = TModelsManager::GetModel(asModel, true);
                }

                else if(token == "coupleradapter:") {
					// coupling adapter data
					parser.getTokens( 3 );
                    parser
                        >> m_coupleradapter.model
                        >> m_coupleradapter.position.x
                        >> m_coupleradapter.position.y;
                    replace_slashes( m_coupleradapter.model );
                    erase_leading_slashes( m_coupleradapter.model );
                }

                else if(token == "attachments:") {
					// additional 3d models attached to main body
                    // content provided as a series of values together enclosed in "{}"
                    // each value is a name of additional 3d model
                    // value can be optionally set of values enclosed in "[]" in which case one value will be picked randomly
                    // TBD: reconsider something more yaml-compliant and/or ability to define offset and rotation
                    while( ( ( token = deserialize_random_set( parser ) ) != "" )
                        && ( token != "}" ) ) {
                        if( token == "{" ) { continue; }
                        replace_slashes( token );
                        Global.asCurrentTexturePath = asBaseDir; // biezaca sciezka do tekstur to dynamic/...
                        auto *attachmentmodel { TModelsManager::GetModel( asBaseDir + token, true ) };
                        if( attachmentmodel != nullptr ) {
                            mdAttachments.emplace_back( attachmentmodel );
                        }
                    }
                }

                else if(token == "loads:") {
					// default load visualization models overrides
                    // content provided as "key: value" pairs together enclosed in "{}"
                    // value can be optionally set of values enclosed in "[]" in which case one value will be picked randomly
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                        && ( token != "}" ) ) {
                        if( token.back() == ':' ) {
                            auto loadmodel { deserialize_random_set( parser ) };
                            replace_slashes( loadmodel );
                            LoadModelOverrides.emplace( token.erase( token.size() - 1 ), loadmodel );
                        }
                    }
                }

				else if( token == "brakemode:" ) {
                // Ra 15-01: gałka nastawy hamulca
					parser.getTokens();
					parser >> asAnimName;
                    smBrakeMode = GetSubmodelFromName( mdModel, asAnimName );
                    // jeszcze wczytać kąty obrotu dla poszczególnych ustawień
                }

				else if( token == "loadmode:" ) {
                // Ra 15-01: gałka nastawy hamulca
					parser.getTokens();
					parser >> asAnimName;
                    smLoadMode = GetSubmodelFromName( mdModel, asAnimName );
                    // jeszcze wczytać kąty obrotu dla poszczególnych ustawień
                }

                else if (token == "animwheelprefix:") {
					// prefiks kręcących się kół
                    int i, k, m;
					unsigned int j;
					parser.getTokens( 1, false ); parser >> token;
                    for (i = 0; i < iAnimType[ANIM_WHEELS]; ++i) // liczba osi
                    { // McZapkie-050402: wyszukiwanie kol o nazwie str*
                        asAnimName = token + std::to_string(i + 1);
                        pAnimations[i].smAnimated = GetSubmodelFromName( mdModel, asAnimName );
                        if (pAnimations[i].smAnimated)
                        { //++iAnimatedAxles;
                            pAnimations[i].smAnimated->WillBeAnimated(); // wyłączenie optymalizacji transformu
							pAnimations[i].yUpdate = std::bind( &TDynamicObject::UpdateAxle, this, std::placeholders::_1 );
							pAnimations[i].fMaxDist = Global.fDistanceFactor * MoverParameters->WheelDiameter * 200;
							pAnimations[i].fMaxDist *= pAnimations[i].fMaxDist;
                        }
                    }
                    // Ra: ustawianie indeksów osi
                    for (i = 0; i < iAnimType[ANIM_WHEELS]; ++i) // ilość osi (zabezpieczenie przed błędami w CHK)
                        pAnimations[i].dWheelAngle = 1; // domyślnie wskaźnik na napędzające
                    i = 0;
                    j = 1;
                    k = 0;
                    m = 0; // numer osi; kolejny znak; ile osi danego typu; która średnica
                    if ((MoverParameters->WheelDiameterL != MoverParameters->WheelDiameter) ||
                        (MoverParameters->WheelDiameterT != MoverParameters->WheelDiameter))
                    { // obsługa różnych średnic, o ile występują
                        while ((i < iAnimType[ANIM_WHEELS]) &&
                               (j <= MoverParameters->AxleArangement.length()))
                        { // wersja ze wskaźnikami jest bardziej elastyczna na nietypowe układy
                            if ((k >= 'A') && (k <= 'J')) // 10 chyba maksimum?
                            {
                                pAnimations[i++].dWheelAngle = 1; // obrót osi napędzających
                                --k; // następna będzie albo taka sama, albo bierzemy kolejny znak
                                m = 2; // następujące toczne będą miały inną średnicę
                            }
                            else if ((k >= '1') && (k <= '9'))
                            {
                                pAnimations[i++].dWheelAngle = m; // obrót osi tocznych
                                --k; // następna będzie albo taka sama, albo bierzemy kolejny znak
                            }
                            else
                                k = MoverParameters->AxleArangement[j++]; // pobranie kolejnego znaku
                        }
                    }
                }
                // else if (str==AnsiString("animrodprefix:")) //prefiks wiazarow dwoch
                // {
                //  str= Parser->GetNextSymbol();
                //  for (int i=1; i<=2; i++)
                //  {//McZapkie-050402: wyszukiwanie max 2 wiazarow o nazwie str*
                //   asAnimName=str+i;
                //   smWiazary[i-1]=mdModel->GetFromName(asAnimName.c_str());
                //   smWiazary[i-1]->WillBeAnimated();
                //  }
                // }

				else if( token == "animpantprefix:" ) {
                 // Ra: pantografy po nowemu mają literki i numerki
                }
                // Pantografy - Winger 160204
				else if( token == "animpantrd1prefix:" ) {
                // prefiks ramion dolnych 1
					parser.getTokens(); parser >> token;
                    float4x4 m; // macierz do wyliczenia pozycji i wektora ruchu pantografu
                    TSubModel *sm;
                    if (pants)
                        for (int i = 0; i < iAnimType[ANIM_PANTS]; i++)
                        { // Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
                            asAnimName = token + std::to_string(i + 1);
                            sm = GetSubmodelFromName( mdModel, asAnimName );
                            pants[i].smElement[0] = sm; // jak NULL, to nie będzie animowany
                            if (sm)
                            { // w EP09 wywalało się tu z powodu NULL
                                sm->WillBeAnimated();
                                sm->ParentMatrix(&m); // pobranie macierzy transformacji
                                // m(3)[1]=m[3][1]+0.054; //w górę o wysokość ślizgu (na razie tak)
                                pants[i].fParamPants->vPos.z = m[3][0]; // przesunięcie w bok (asymetria)
                                pants[i].fParamPants->vPos.y = m[3][1]; // przesunięcie w górę odczytane z modelu
                                if ((sm = pants[i].smElement[0]->ChildGet()) != NULL)
                                { // jeśli ma potomny, można policzyć długość (odległość potomnego od osi obrotu)
                                    m = float4x4(*sm->GetMatrix()); // wystarczyłby wskaźnik, nie trzeba kopiować
                                    // może trzeba: pobrać macierz dolnego ramienia, wyzerować przesunięcie, przemnożyć przez macierz górnego
                                    pants[i].fParamPants->fHoriz = -fabs(m[3][1]);
                                    pants[i].fParamPants->fLenL1 = hypot(m[3][1], m[3][2]); // po osi OX nie potrzeba
                                    pants[i].fParamPants->fAngleL0 = atan2(fabs(m[3][2]), fabs(m[3][1]));
                                    // if (pants[i].fParamPants->fAngleL0<M_PI_2)
                                    // pants[i].fParamPants->fAngleL0+=M_PI; //gdyby w odwrotną stronę wyszło
                                    // if
                                    // ((pants[i].fParamPants->fAngleL0<0.03)||(pants[i].fParamPants->fAngleL0>0.09))
                                    // //normalnie ok. 0.05
                                    // pants[i].fParamPants->fAngleL0=pants[i].fParamPants->fAngleL;
                                    pants[i].fParamPants->fAngleL = pants[i].fParamPants->fAngleL0; // początkowy kąt dolnego ramienia
                                    if ((sm = sm->ChildGet()) != NULL)
                                    { // jeśli dalej jest ślizg, można policzyć długość górnego ramienia
                                        m = float4x4(*sm->GetMatrix()); // wystarczyłby wskaźnik,
                                        // nie trzeba kopiować trzeba by uwzględnić macierz dolnego ramienia, żeby uzyskać kąt do poziomu...
                                        pants[i].fParamPants->fHoriz += fabs(m(3)[1]); // różnica długości rzutów ramion na
                                        // płaszczyznę podstawy (jedna dodatnia, druga ujemna)
                                        pants[i].fParamPants->fLenU1 = hypot( m[3][1], m[3][2] ); // po osi OX nie potrzeba
                                        // pants[i].fParamPants->pantu=acos((1.22*cos(pants[i].fParamPants->fAngleL)+0.535)/1.755); //górne ramię
                                        // pants[i].fParamPants->fAngleU0=acos((1.176289*cos(pants[i].fParamPants->fAngleL)+0.54555075)/1.724482197); //górne ramię
                                        pants[i].fParamPants->fAngleU0 = atan2( fabs(m[3][2]), fabs(m[3][1]) ); // początkowy kąt górnego ramienia, odczytany z modelu
                                        // if (pants[i].fParamPants->fAngleU0<M_PI_2)
                                        // pants[i].fParamPants->fAngleU0+=M_PI; //gdyby w odwrotną stronę wyszło
                                        // if (pants[i].fParamPants->fAngleU0<0)
                                        // pants[i].fParamPants->fAngleU0=-pants[i].fParamPants->fAngleU0;
                                        // if
                                        // ((pants[i].fParamPants->fAngleU0<0.00)||(pants[i].fParamPants->fAngleU0>0.09)) //normalnie ok. 0.07
                                        // pants[i].fParamPants->fAngleU0=acos((pants[i].fParamPants->fLenL1*cos(pants[i].fParamPants->fAngleL)+pants[i].fParamPants->fHoriz)/pants[i].fParamPants->fLenU1);
                                        pants[i].fParamPants->fAngleU = pants[i].fParamPants->fAngleU0; // początkowy kąt
                                        // Ra: ze względu na to, że niektóre modele pantografów są zrąbane, ich mierzenie ma obecnie ograniczony sens
                                        sm->ParentMatrix(&m); // pobranie macierzy transformacji pivota ślizgu względem wstawienia pojazdu
                                        float det = Det(m);
                                        if (std::fabs(det - 1.0) < 0.001) // dopuszczamy 1 promil błędu na skalowaniu ślizgu
                                        { // skalowanie jest w normie, można pobrać wymiary z modelu
                                            pants[i].fParamPants->fHeight = sm->MaxY(m); // przeliczenie maksimum wysokości wierzchołków względem macierzy
                                            pants[i].fParamPants->fHeight -= m[3][1]; // odjęcie wysokości pivota ślizgu
                                            pants[i].fParamPants->vPos.x = m[3][2]; // przy okazji odczytać z modelu pozycję w długości
                                            // ErrorLog("Model OK: "+asModel+",
                                            // height="+pants[i].fParamPants->fHeight);
                                            // ErrorLog("Model OK: "+asModel+",
                                            // pos.x="+pants[i].fParamPants->vPos.x);
                                        }
                                        else
                                        { // gdy ktoś przesadził ze skalowaniem
                                            pants[i].fParamPants->fHeight = 0.0; // niech będzie odczyt z pantfactors:
                                            ErrorLog(
                                                "Bad model: " + asModel + ", scale of " + (sm->pName) + " is " + std::to_string(100.0 * det) + "%",
                                                logtype::model );
                                        }
                                    }
                                }
                            }
                            else {
                                // brak ramienia
                                pants[ i ].fParamPants->fHeight = 0.0; // niech będzie odczyt z pantfactors:
                                ErrorLog( "Bad model: " + asFileName + " - missed submodel " + asAnimName, logtype::model );
                            }
                        }
                }

				else if( token == "animpantrd2prefix:" ) {
                // prefiks ramion dolnych 2
					parser.getTokens(); parser >> token;
                    float4x4 m; // macierz do wyliczenia pozycji i wektora ruchu pantografu
                    TSubModel *sm;
					if( pants ) {
						for( int i = 0; i < iAnimType[ ANIM_PANTS ]; i++ ) {
                            // Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
							asAnimName = token + std::to_string( i + 1 );
                            sm = GetSubmodelFromName( mdModel, asAnimName );
							pants[ i ].smElement[ 1 ] = sm; // jak NULL, to nie będzie animowany
							if( sm ) { // w EP09 wywalało się tu z powodu NULL
                                sm->WillBeAnimated();
								if( pants[ i ].fParamPants->vPos.y == 0.0 ) {
                                    // jeśli pierwsze ramię nie ustawiło tej wartości, próbować drugim
                                    //!!!! docelowo zrobić niezależną animację ramion z każdej strony
                                    m = float4x4(
                                        *sm->GetMatrix()); // skopiowanie, bo będziemy mnożyć
									m( 3 )[ 1 ] =
										m[ 3 ][ 1 ] + 0.054; // w górę o wysokość ślizgu (na razie tak)
									while( sm->Parent ) {
										if( sm->Parent->GetMatrix() )
                                            m = *sm->Parent->GetMatrix() * m;
                                        sm = sm->Parent;
                                    }
									pants[ i ].fParamPants->vPos.z = m[3][0]; // przesunięcie w bok (asymetria)
									pants[ i ].fParamPants->vPos.y = m[3][1]; // przesunięcie w górę odczytane z modelu
                                }
                            }
                            else
								ErrorLog( "Bad model: " + asFileName + " - missed submodel " + asAnimName, logtype::model ); // brak ramienia
						}
                    }
                }

				else if( token == "animpantrg1prefix:" ) {
                 // prefiks ramion górnych 1
					parser.getTokens(); parser >> token;
                    if( pants ) {
                        for( int i = 0; i < iAnimType[ ANIM_PANTS ]; i++ ) {
                            // Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
                            asAnimName = token + std::to_string( i + 1 );
                            pants[ i ].smElement[ 2 ] = GetSubmodelFromName( mdModel, asAnimName );
                            if( pants[ i ].smElement[ 2 ] ) {
                                pants[ i ].smElement[ 2 ]->WillBeAnimated();
                            }
                            else {
                                ErrorLog( "Bad model: " + asFileName + " - missed submodel " + asAnimName, logtype::model ); // brak ramienia
                            }
                        }
                    }
                }

				else if( token == "animpantrg2prefix:" ) {
                 // prefiks ramion górnych 2
                    parser.getTokens(); parser >> token;
                    if( pants ) {
                        for( int i = 0; i < iAnimType[ ANIM_PANTS ]; i++ ) {
                            // Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
                            asAnimName = token + std::to_string( i + 1 );
                            pants[ i ].smElement[ 3 ] = GetSubmodelFromName( mdModel, asAnimName );
                            if( pants[ i ].smElement[ 3 ] ) {
                                pants[ i ].smElement[ 3 ]->WillBeAnimated();
                            }
                            else {
                                ErrorLog( "Bad model: " + asFileName + " - missed submodel " + asAnimName, logtype::model ); // brak ramienia
                            }
                        }
                    }
                }

				else if( token == "animpantslprefix:" ) {
                 // prefiks ślizgaczy
					parser.getTokens(); parser >> token;
                    if( pants ) {
                        for( int i = 0; i < iAnimType[ ANIM_PANTS ]; i++ ) {
                            // Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
                            asAnimName = token + std::to_string( i + 1 );
                            pants[ i ].smElement[ 4 ] = GetSubmodelFromName( mdModel, asAnimName );
                            if( pants[ i ].smElement[ 4 ] ) {
                                pants[ i ].smElement[ 4 ]->WillBeAnimated();
                                pants[ i ].yUpdate = std::bind( &TDynamicObject::UpdatePant, this, std::placeholders::_1 );
                                pants[ i ].fMaxDist = 300 * 300; // nie podnosić w większej odległości
                                pants[ i ].iNumber = i;
                            }
                            else {
                                ErrorLog( "Bad model: " + asFileName + " - missed submodel " + asAnimName, logtype::model ); // brak ramienia
                            }
                        }
                    }
                }

				else if( token == "pantfactors:" ) {
                 // Winger 010304:
                    // parametry pantografow
					double pant1x, pant2x, pant1h, pant2h;
					parser.getTokens( 4, false );
					parser
						>> pant1x
						>> pant2x
						>> pant1h // wysokość pierwszego ślizgu
						>> pant2h;// wysokość drugiego ślizgu
					if( pant1h > 0.5 ) {
                        pant1h = pant2h; // tu może być zbyt duża wartość
					}
                    if ((pant1x < 0) &&
                        (pant2x > 0)) // pierwsza powinna być dodatnia, a druga ujemna
                    {
                        pant1x = -pant1x;
                        pant2x = -pant2x;
                    }
                    if( pants ) {
                        for( int i = 0; i < iAnimType[ ANIM_PANTS ]; ++i ) {
                            // przepisanie współczynników do pantografów (na razie nie będzie lepiej)
                            auto &pantograph { *(pants[ i ].fParamPants) };

                            pantograph.fAngleL = pantograph.fAngleL0; // początkowy kąt dolnego ramienia
                            pantograph.fAngleU = pantograph.fAngleU0; // początkowy kąt
                            // pants[i].fParamPants->PantWys=1.22*sin(pants[i].fParamPants->fAngleL)+1.755*sin(pants[i].fParamPants->fAngleU);
                            // //wysokość początkowa
                            // pants[i].fParamPants->PantWys=1.176289*sin(pants[i].fParamPants->fAngleL)+1.724482197*sin(pants[i].fParamPants->fAngleU);
                            // //wysokość początkowa
                            if( pantograph.fHeight == 0.0 ) // gdy jest nieprawdopodobna wartość (np. nie znaleziony ślizg)
                            { // gdy pomiary modelu nie udały się, odczyt podanych parametrów z MMD
                                pantograph.vPos.x =
                                    ( i & 1 ) ?
                                        pant2x :
                                        pant1x;
                                pantograph.fHeight =
                                    ( i & 1 ) ?
                                        pant2h :
                                        pant1h; // wysokość ślizgu jest zapisana w MMD
                            }
                            pantograph.PantWys =
                                pantograph.fLenL1 * sin( pantograph.fAngleL ) +
                                pantograph.fLenU1 * sin( pantograph.fAngleU ) +
                                pantograph.fHeight; // wysokość początkowa
                            // pants[i].fParamPants->vPos.y=panty-panth-pants[i].fParamPants->PantWys; //np. 4.429-0.097=4.332=~4.335
                            if( pantograph.vPos.y == 0.0 ) {
                                // crude fallback, place the pantograph(s) atop of the vehicle-sized box
                                pantograph.vPos.y = MoverParameters->Dim.H - pantograph.fHeight - pantograph.PantWys;
                            }
                            // pants[i].fParamPants->vPos.z=0; //niezerowe dla pantografów asymetrycznych
                            pantograph.PantTraction = pantograph.PantWys;
                            // połowa szerokości ślizgu; jest w "Power: CSW="
                            pantograph.fWidth = 0.5 * MoverParameters->EnginePowerSource.CollectorParameters.CSW;

                            // create sound emitters for the pantograph
                            m_pantographsounds.emplace_back();
                        }
                    }
                }

                else if (token == "animpistonprefix:") {
					// prefiks tłoczysk - na razie uzywamy modeli pantografów
					parser.getTokens(1, false); parser >> token;
					for( int i = 1; i <= 2; ++i )
                    {
                        // asAnimName=str+i;
                        // smPatykird1[i-1]=mdModel->GetFromName(asAnimName.c_str());
                        // smPatykird1[i-1]->WillBeAnimated();
                    }
                }

				else if( token == "animconrodprefix:" ) {
                 // prefiks korbowodów - na razie używamy modeli pantografów
					parser.getTokens(); parser >> token;
					for( int i = 1; i <= 2; i++ )
                    {
                        // asAnimName=str+i;
                        // smPatykirg1[i-1]=mdModel->GetFromName(asAnimName.c_str());
                        // smPatykirg1[i-1]->WillBeAnimated();
                    }
                }

				else if( token == "pistonfactors:" ) {
                 // Ra: parametry
                    // silnika parowego
                    // (tłoka)
                    /* //Ra: tymczasowo wyłączone ze względu na porządkowanie animacji
                       pantografów
                             pant1x=Parser->GetNextSymbol().ToDouble(); //kąt przesunięcia
                       dla
                       pierwszego tłoka
                             pant2x=Parser->GetNextSymbol().ToDouble(); //kąt przesunięcia
                       dla
                       drugiego tłoka
                             panty=Parser->GetNextSymbol().ToDouble(); //długość korby (r)
                             panth=Parser->GetNextSymbol().ToDouble(); //długoś korbowodu
                       (k)
                    */
                    MoverParameters->EnginePowerSource.PowerType =
                        TPowerType::SteamPower; // Ra: po chamsku, ale z CHK nie działa
                }

				else if( token == "animreturnprefix:" ) {
                 // prefiks drążka mimośrodowego - na razie używamy modeli pantografów
					parser.getTokens(1, false); parser >> token;
					for( int i = 1; i <= 2; i++ )
                    {
                        // asAnimName=str+i;
                        // smPatykird2[i-1]=mdModel->GetFromName(asAnimName.c_str());
                        // smPatykird2[i-1]->WillBeAnimated();
                    }
                }

                else if (token == "animexplinkprefix:"){ // animreturnprefix:
                 // prefiks jarzma - na razie używamy modeli pantografów
					parser.getTokens(1, false); parser >> token;
					for( int i = 1; i <= 2; i++ )
                    {
                        // asAnimName=str+i;
                        // smPatykirg2[i-1]=mdModel->GetFromName(asAnimName.c_str());
                        // smPatykirg2[i-1]->WillBeAnimated();
                    }
                }

				else if( token == "animpendulumprefix:" ) {
                 // prefiks wahaczy
					parser.getTokens(); parser >> token;
                    asAnimName = "";
                    for( int i = 1; i <= 4; ++i ) {
                        // McZapkie-050402: wyszukiwanie max 4 wahaczy o nazwie str*
                        asAnimName = token + std::to_string( i );
                        smWahacze[ i - 1 ] = GetSubmodelFromName( mdModel, asAnimName );
                        if( smWahacze[ i - 1 ] ) {
                            smWahacze[ i - 1 ]->WillBeAnimated();
                        }
                    }
					parser.getTokens(); parser >> token;
					if( token == "pendulumamplitude:" ) {
						parser.getTokens( 1, false );
						parser >> fWahaczeAmp;
					}
                }

				else if( token == "animdoorprefix:" ) {
                    // nazwa animowanych drzwi
					int i, j;
					parser.getTokens(1, false); parser >> token;
                    for (i = 0, j = 0; i < ANIM_DOORS; ++i)
                        j += iAnimType[i]; // zliczanie wcześniejszych animacji
                    for (i = 0; i < iAnimType[ANIM_DOORS]; ++i) // liczba drzwi
                    { // NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
                      // ustalenie submodelu
                        asAnimName = token + std::to_string(i + 1);
                        pAnimations[ i + j ].smAnimated = GetSubmodelFromName( mdModel, asAnimName );
                        if (pAnimations[i + j].smAnimated)
                        { //++iAnimatedDoors;
                            pAnimations[i + j].smAnimated->WillBeAnimated(); // wyłączenie optymalizacji transformu
                            switch( MoverParameters->Doors.type )
                            { // od razu zapinamy potrzebny typ animacji
                            case 1:
								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorTranslate, this, std::placeholders::_1 );
                                break;
                            case 2:
								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorRotate, this, std::placeholders::_1 );
                                break;
                            case 3:
								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorFold, this, std::placeholders::_1 );
                                break; // obrót 3 kolejnych submodeli
							case 4:
								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorPlug, this, std::placeholders::_1 );
								break;
							default:
								break;
							}
                            pAnimations[i + j].iNumber = i + 1; // parzyste działają inaczej niż nieparzyste
                            pAnimations[i + j].fMaxDist = 300 * 300; // drzwi to z daleka widać
/*
                            // NOTE: no longer used
                            pAnimations[i + j].fSpeed = Random(150); // oryginalny koncept z DoorSpeedFactor
                            pAnimations[i + j].fSpeed = (pAnimations[i + j].fSpeed + 100) / 100;
*/
                        }
                    }
                }

                else if( token == "animstepprefix:" ) {
                    // animated doorstep submodel name prefix
					int i, j;
					parser.getTokens(1, false); parser >> token;
                    for (i = 0, j = 0; i < ANIM_DOORSTEPS; ++i)
                        j += iAnimType[i]; // zliczanie wcześniejszych animacji
                    for (i = 0; i < iAnimType[ANIM_DOORSTEPS]; ++i) // liczba drzwi
                    { // NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
                      // ustalenie submodelu
                        asAnimName = token + std::to_string(i + 1);
                        pAnimations[i + j].smAnimated = GetSubmodelFromName( mdModel, asAnimName );
                        if (pAnimations[i + j].smAnimated)
                        { //++iAnimatedDoors;
                            pAnimations[i + j].smAnimated->WillBeAnimated(); // wyłączenie optymalizacji transformu
                            switch( MoverParameters->Doors.step_type )
                            { // od razu zapinamy potrzebny typ animacji
                            case 1: // shift
								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdatePlatformTranslate, this, std::placeholders::_1 );
                                break;
                            case 2: // rotate
								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdatePlatformRotate, this, std::placeholders::_1 );
                                break;
							default:
								break;
							}
                            pAnimations[i + j].iNumber = i + 1; // parzyste działają inaczej niż nieparzyste
                            pAnimations[i + j].fMaxDist = 150 * 150; // drzwi to z daleka widać
/*
                            // NOTE: no longer used
                            pAnimations[i + j].fSpeed = Random(150); // oryginalny koncept z DoorSpeedFactor
                            pAnimations[i + j].fSpeed = (pAnimations[i + j].fSpeed + 100) / 100;
*/
                        }
                    }
                }

                else if( token == "animmirrorprefix:" ) {
                    // animated mirror submodel name prefix
                    int i, j;
                    parser.getTokens( 1, false ); parser >> token;
                    for( i = 0, j = 0; i < ANIM_MIRRORS; ++i )
                        j += iAnimType[ i ]; // zliczanie wcześniejszych animacji
                    for( i = 0; i < iAnimType[ ANIM_MIRRORS ]; ++i ) // liczba drzwi
                    { // NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
                      // ustalenie submodelu
                        asAnimName = token + std::to_string( i + 1 );
                        pAnimations[ i + j ].smAnimated = GetSubmodelFromName( mdModel, asAnimName );
                        if( pAnimations[ i + j ].smAnimated ) {
                            pAnimations[ i + j ].smAnimated->WillBeAnimated(); // wyłączenie optymalizacji transformu
                            // od razu zapinamy potrzebny typ animacji
                            auto const offset { pAnimations[ i + j ].smAnimated->offset() };
                            pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateMirror, this, std::placeholders::_1 );
                            // we don't expect more than 2-4 mirrors, so it should be safe to store submodel location (front/rear) in the higher bits
                            // parzyste działają inaczej niż nieparzyste
                            pAnimations[ i + j ].iNumber =
                                ( ( pAnimations[ i + j ].smAnimated->offset().z > 0 ?
                                    end::front :
                                    end::rear ) << 4 )
                                + i;
                            pAnimations[ i + j ].fMaxDist = 150 * 150; // drzwi to z daleka widać
/*
                            // NOTE: no longer used
                            pAnimations[i + j].fSpeed = Random(150); // oryginalny koncept z DoorSpeedFactor
                            pAnimations[i + j].fSpeed = (pAnimations[i + j].fSpeed + 100) / 100;
*/
                        }
                    }
                }

			} while( ( token != "" )
	              && ( token != "endmodels" ) );

            if( false == MoverParameters->LoadAttributes.empty() ) {
                // Ra: tu wczytywanie modelu ładunku jest w porządku

                // bieżąca ścieżka do tekstur to dynamic/...
                Global.asCurrentTexturePath = asBaseDir;

                mdLoad = LoadMMediaFile_mdload( MoverParameters->LoadType.name );

                // z powrotem defaultowa sciezka do tekstur
                Global.asCurrentTexturePath = std::string( szTexturePath );
            }

        } // models

		else if( token == "sounds:" ) {
			// dzwieki
			do {
				token = "";
				parser.getTokens(); parser >> token;
				if( token == "wheel_clatter:" ){
					// polozenia osi w/m srodka pojazdu
                    double dSDist;
					parser.getTokens( 1, false );
					parser >> dSDist;
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                          && ( token != "end" ) ) {
                        // add another axle entry to the list
                        axle_sounds axle {
                            0,
                            std::atof( token.c_str() ) * -1.0, // for axle locations negative value means ahead of centre but vehicle faces +Z in 'its' space
                            { sound_placement::external, static_cast<float>( dSDist ) } };
                        axle.clatter.deserialize( parser, sound_type::single );
                        axle.clatter.owner( this );
                        axle.clatter.offset( { 0, 0, axle.offset } );
                        m_axlesounds.emplace_back( axle );
                    }
                    // arrange the axles in case they're listed out of order
                    std::sort(
                        std::begin( m_axlesounds ), std::end( m_axlesounds ),
                        []( axle_sounds const &Left, axle_sounds const &Right ) {
                            return ( Left.offset > Right.offset ); } );
                }

				else if( ( token == "engine:" )
					  && ( MoverParameters->Power > 0 ) ) {
					// plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
                    m_powertrainsounds.engine.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    m_powertrainsounds.engine.owner( this );

                    auto const amplitudedivisor = static_cast<float>( (
                        MoverParameters->EngineType == TEngineType::DieselEngine ? 1 :
                        MoverParameters->EngineType == TEngineType::DieselElectric ? 1 :
                        MoverParameters->nmax * 60 + MoverParameters->Power * 3 ) );
                    m_powertrainsounds.engine.m_amplitudefactor /= amplitudedivisor;

				} else if (token == "fakeengine:") {
					m_powertrainsounds.fake_engine.deserialize(parser, sound_type::single);
					m_powertrainsounds.fake_engine.owner(this);
				}

                else if( token == "dieselinc:" ) {
                    // dzwiek przy wlazeniu na obroty woodwarda
                    m_powertrainsounds.engine_revving.deserialize( parser, sound_type::single, sound_parameters::range );
                    m_powertrainsounds.engine_revving.owner( this );
                }

                else if( token == "oilpump:" ) {
                    m_powertrainsounds.oil_pump.deserialize( parser, sound_type::single );
                    m_powertrainsounds.oil_pump.owner( this );
                }

                else if( token == "fuelpump:" ) {
                    m_powertrainsounds.fuel_pump.deserialize( parser, sound_type::single );
                    m_powertrainsounds.fuel_pump.owner( this );
                }

                else if( token == "waterpump:" ) {
                    m_powertrainsounds.water_pump.deserialize( parser, sound_type::single );
                    m_powertrainsounds.water_pump.owner( this );
                }

                else if( token == "waterheater:" ) {
                    m_powertrainsounds.water_heater.deserialize( parser, sound_type::single );
                    m_powertrainsounds.water_heater.owner( this );
                }

                else if( ( token == "tractionmotor:" ) && ( MoverParameters->Power > 0 ) ) {
                    // plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
                    sound_source motortemplate { sound_placement::external };
                    motortemplate.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    auto const amplitudedivisor { static_cast<float>( MoverParameters->nmax * 60 + MoverParameters->Power * 3 ) };
                    motortemplate.m_amplitudefactor /= amplitudedivisor;
                    motortemplate.owner( this );

                    auto &motors { m_powertrainsounds.motors };

                    if( true == motors.empty() ) {
                        // fallback for cases without specified motor locations, convert sound template to a single sound source
                        motors.emplace_back( motortemplate );
                    }
                    else {
                        // apply configuration to all defined motors
                        for( auto &motor : motors ) {
                            // combine potential x- and y-axis offsets of the sound template with z-axis offsets of individual motors
                            auto motoroffset { motortemplate.offset() };
                            motoroffset.z = motor.offset().z;
                            motor = motortemplate;
                            motor.offset( motoroffset );
                            // apply randomized playback start offset for each instance, to reduce potential reverb with identical nearby sources
                            motor.start( LocalRandom( 0.0, 1.0 ) );
                        }
                    }
                }

                else if( ( token == "tractionacmotor:" ) && ( MoverParameters->Power > 0 ) ) {
                    // plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
                    sound_source motortemplate { sound_placement::external };
                    motortemplate.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    motortemplate.owner( this );

                    auto &motors { m_powertrainsounds.acmotors };

                    if( true == motors.empty() ) {
                        // fallback for cases without specified motor locations, convert sound template to a single sound source
                        motors.emplace_back( motortemplate );
                    }
                    else {
                        // apply configuration to all defined motors
                        for( auto &motor : motors ) {
                            // combine potential x- and y-axis offsets of the sound template with z-axis offsets of individual motors
                            auto motoroffset { motortemplate.offset() };
                            motoroffset.z = motor.offset().z;
                            motor = motortemplate;
                            motor.offset( motoroffset );
                            // apply randomized playback start offset for each instance, to reduce potential reverb with identical nearby sources
                            motor.start( LocalRandom( 0.0, 1.0 ) );
                        }
                    }
                }

                else if( token == "motorblower:" ) {

                    sound_source blowertemplate { sound_placement::engine };
                    blowertemplate.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    blowertemplate.owner( this );

                    auto const amplitudedivisor { static_cast<float>(
                        MoverParameters->MotorBlowers[ end::front ].speed > 0 ? MoverParameters->MotorBlowers[ end::front ].speed * MoverParameters->nmax * 60 + MoverParameters->Power * 3 :
                        blowertemplate.has_bookends() ? 1 : // NOTE: for motorblowers with fixed speed if the sound has defined bookends we skip revolutions-based part of frequency/volume adjustments
                        MoverParameters->MotorBlowers[ end::front ].speed * -1 ) };
                    blowertemplate.m_amplitudefactor /= amplitudedivisor;
                    blowertemplate.m_frequencyfactor /= amplitudedivisor;

                    if( true == m_powertrainsounds.motors.empty() ) {
                        // fallback for cases without specified motor locations, convert sound template to a single sound source
                        m_powertrainsounds.motorblowers.emplace_back( blowertemplate );
                    }
                    else {
                        // apply configuration to all defined motor blowers
                        for( auto &blower : m_powertrainsounds.motorblowers ) {
                            // combine potential x- and y-axis offsets of the sound template with z-axis offsets of individual blowers
                            auto bloweroffset { blowertemplate.offset() };
                            bloweroffset.z = blower.offset().z;
                            blower = blowertemplate;
                            blower.offset( bloweroffset );
                            // apply randomized playback start offset for each instance, to reduce potential reverb with identical nearby sources
                            blower.start( LocalRandom( 0.0, 1.0 ) );
                        }
                    }
                }

				else if( token == "inverter:" ) {
					// plik z dzwiekiem wentylatora, mnozniki i ofsety amp. i czest.
                    m_powertrainsounds.inverter.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    m_powertrainsounds.inverter.owner( this );
				}

                else if( token == "ventilator:" ) {
					// plik z dzwiekiem wentylatora, mnozniki i ofsety amp. i czest.
                    m_powertrainsounds.rsWentylator.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    m_powertrainsounds.rsWentylator.owner( this );

                    m_powertrainsounds.rsWentylator.m_amplitudefactor /= MoverParameters->RVentnmax;
                    m_powertrainsounds.rsWentylator.m_frequencyfactor /= MoverParameters->RVentnmax;
				}

                else if( token == "radiatorfan1:" ) {
                    // primary circuit radiator fan
                    m_powertrainsounds.radiator_fan.deserialize( parser, sound_type::single );
                    m_powertrainsounds.radiator_fan.owner( this );
                }

                else if( token == "radiatorfan2" ) {
                    // auxiliary circuit radiator fan
                    m_powertrainsounds.radiator_fan_aux.deserialize( parser, sound_type::single );
                    m_powertrainsounds.radiator_fan_aux.owner( this );
                }

				else if( token == "transmission:" ) {
					// plik z dzwiekiem, mnozniki i ofsety amp. i czest.
                    // NOTE, fixed default parameters, legacy system leftover
                    auto &sound { m_powertrainsounds.transmission };
                    sound.m_amplitudefactor = 0.029;
                    sound.m_amplitudeoffset = 0.1;
                    sound.m_frequencyfactor = 0.005;
                    sound.m_frequencyoffset = 1.0;

                    sound.deserialize( parser, sound_type::single, sound_parameters::range );
                    sound.owner( this );
                }

                else if( token == "brakesound:" ) {
                    // hamowanie zwykle:
                    rsBrake.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency );
                    rsBrake.owner( this );
                    // NOTE: can't pre-calculate amplitude normalization based on max brake force, as this varies depending on vehicle speed
                    rsBrake.m_frequencyfactor /= ( 1 + MoverParameters->Vmax );
                }

				else if( token == "brake:"  ) {
					// plik z piskiem hamulca,  mnozniki i ofsety amplitudy.
                    rsPisk.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude );
                    rsPisk.owner( this );

                    if( rsPisk.m_amplitudefactor > 10.f ) {
                        // HACK: convert old style activation point threshold to the new, regular amplitude adjustment system
                        rsPisk.m_amplitudefactor = 1.f;
                        rsPisk.m_amplitudeoffset = 0.f;
                    }
                    rsPisk.m_amplitudeoffset *= ( 105.f - Random( 10.f ) ) / 100.f;
                }

                else if( token == "brakecylinderinc:" ) {
                    // brake cylinder pressure increase sounds
                    m_brakecylinderpistonadvance.deserialize( parser, sound_type::single );
                    m_brakecylinderpistonadvance.owner( this );
                }

                else if( token == "brakecylinderdec:" ) {
                    // brake cylinder pressure decrease sounds
                    m_brakecylinderpistonrecede.deserialize( parser, sound_type::single );
                    m_brakecylinderpistonrecede.owner( this );
                }

				else if (token == "epbrakeinc:") {
				// brake cylinder pressure increase sounds
					m_epbrakepressureincrease.deserialize(parser, sound_type::single);
					m_epbrakepressureincrease.owner(this);
				}

				else if (token == "epbrakedec:") {
				// brake cylinder pressure decrease sounds
					m_epbrakepressuredecrease.deserialize(parser, sound_type::single);
					m_epbrakepressuredecrease.owner(this);
				}

                else if( token == "emergencybrake:" ) {
					// emergency brake sound
                    m_emergencybrake.deserialize( parser, sound_type::single );
                    m_emergencybrake.owner( this );
                }

				else if( token == "brakeacc:" ) {
					// plik z przyspieszaczem (upust po zlapaniu hamowania)
                    sBrakeAcc.deserialize( parser, sound_type::single );
                    sBrakeAcc.owner( this );

                    bBrakeAcc = true;
                }

				else if( token == "unbrake:" ) {
					// plik z piskiem hamulca, mnozniki i ofsety amplitudy.
					rsUnbrake.m_amplitudefactor = 20.0f;
                    rsUnbrake.deserialize( parser, sound_type::single, sound_parameters::range );
                    rsUnbrake.owner( this );
                }
                // spring brake sounds
                else if( token == "springbrake:" ) {
                    m_springbrakesounds.activate.deserialize( parser, sound_type::single );
                    m_springbrakesounds.activate.owner( this );
                }
                else if( token == "springbrakeoff:" ) {
                    m_springbrakesounds.release.deserialize( parser, sound_type::single );
                    m_springbrakesounds.release.owner( this );
                }

				else if( token == "derail:"  ) {
					// dzwiek przy wykolejeniu
                    rsDerailment.deserialize( parser, sound_type::single, sound_parameters::range );
                    rsDerailment.owner( this );
                }

				else if( token == "curve:" ) {
                    rscurve.deserialize( parser, sound_type::single, sound_parameters::range );
                    rscurve.owner( this );
                }

				else if( token == "horn1:" ) {
					// pliki z trabieniem
                    sHorn1.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sHorn1.owner( this );
                }

				else if( token == "horn2:" ) {
					// pliki z trabieniem wysokoton.
                    sHorn2.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sHorn2.owner( this );
                    // TBD, TODO: move horn selection to ai config file
                    if( iHornWarning ) {
                        iHornWarning = 2; // numer syreny do użycia po otrzymaniu sygnału do jazdy
                    }
				}

                else if( token == "horn3:" ) {
                    sHorn3.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sHorn3.owner( this );
                }

				else if( token == "pantographup:" ) {
					// pliki dzwiekow pantografow
                    sound_source pantographup { sound_placement::external };
                    pantographup.deserialize( parser, sound_type::single );
                    pantographup.owner( this );
                    for( auto &pantograph : m_pantographsounds ) {
                        pantograph.sPantUp = pantographup;
                    }
                }

				else if( token == "pantographdown:" ) {
					// pliki dzwiekow pantografow
                    sound_source pantographdown { sound_placement::external };
                    pantographdown.deserialize( parser, sound_type::single );
                    pantographdown.owner( this );
                    for( auto &pantograph : m_pantographsounds ) {
                        pantograph.sPantDown = pantographdown;
                    }
                }

				else if( token == "compressor:" ) {
					// pliki ze sprezarka
                    sCompressor.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sCompressor.owner( this );
                }

                else if( token == "compressoridle:" ) {
					// pliki ze sprezarka
                    sCompressorIdle.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sCompressorIdle.owner( this );
                }

				else if( token == "converter:" ) {
					// pliki z przetwornica
                    sConverter.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sConverter.owner( this );
                }

                else if( token == "heater:" ) {
                    // train heating device
                    sHeater.deserialize( parser, sound_type::single );
                    sHeater.owner( this );
                }

                else if( token == "battery:" ) {
                    // train heating device
                    m_batterysound.deserialize( parser, sound_type::single );
                    m_batterysound.owner( this );
                }

				else if( token == "turbo:" ) {
					// pliki z turbogeneratorem
                    m_powertrainsounds.engine_turbo.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    m_powertrainsounds.engine_turbo.owner( this );
                    m_powertrainsounds.engine_turbo.gain( 0 );
                }

				else if( token == "small-compressor:" ) {
					// pliki z przetwornica
                    sSmallCompressor.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sSmallCompressor.owner( this );
                }

                else if( token == "departuresignal:" ) {
					// pliki z sygnalem odjazdu
                    sound_source soundtemplate { sound_placement::general, 25.f };
                    soundtemplate.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    soundtemplate.owner( this );
                    for( auto &speaker : m_doorspeakers ) {
                        speaker.departure_signal = soundtemplate;
                        speaker.departure_signal.offset( speaker.offset );
                    }
                }

				else if( token == "dooropen:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.rsDoorOpen.offset() };
                        door.rsDoorOpen = soundtemplate;
                        door.rsDoorOpen.offset( dooroffset );
                    }
                }

				else if( token == "doorclose:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.rsDoorClose.offset() };
                        door.rsDoorClose = soundtemplate;
                        door.rsDoorClose.offset( dooroffset );
                    }
                }

                else if( token == "doorlock:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.lock.offset() };
                        door.lock = soundtemplate;
                        door.lock.offset( dooroffset );
                    }
                }

				else if( token == "doorunlock:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.unlock.offset() };
                        door.unlock = soundtemplate;
                        door.unlock.offset( dooroffset );
                    }
                }

                else if( token == "doorstepopen:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.step_open.offset() };
                        door.step_open = soundtemplate;
                        door.step_open.offset( dooroffset );
                    }
                }

                else if( token == "doorstepclose:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.step_close.offset() };
                        door.step_close = soundtemplate;
                        door.step_close.offset( dooroffset );
                    }
                }

                else if( token == "doorpermit:" ) {
                    sound_source soundtemplate { sound_placement::general };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.permit_granted.offset() };
                        door.permit_granted = soundtemplate;
                        door.permit_granted.offset( dooroffset );
                    }
                }

                else if( token == "unloading:" ) {
                    m_exchangesounds.unloading.range( MoverParameters->Dim.L * 0.5f * -1 );
                    m_exchangesounds.unloading.deserialize( parser, sound_type::single );
                    m_exchangesounds.unloading.owner( this );
                }

                else if( token == "loading:" ) {
                    m_exchangesounds.loading.range( MoverParameters->Dim.L * 0.5f * -1 );
                    m_exchangesounds.loading.deserialize( parser, sound_type::single );
                    m_exchangesounds.loading.owner( this );
                }

				else if( token == "sand:" ) {
                    sSand.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sSand.owner( this );
                }

				else if( token == "releaser:" ) {
					// pliki z odluzniaczem
                    sReleaser.deserialize( parser, sound_type::multipart, sound_parameters::range );
                    sReleaser.owner( this );
                }

                else if( token == "outernoise:" ) {
                    // szum podczas jazdy:
                    sound_source noisetemplate{ sound_placement::external, EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
                    noisetemplate.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency, MoverParameters->Vmax );
                    noisetemplate.owner( this );

                    noisetemplate.m_amplitudefactor /= ( 1 + MoverParameters->Vmax );
                    noisetemplate.m_frequencyfactor /= ( 1 + MoverParameters->Vmax );
#ifdef EU07_SOUND_BOGIESOUNDS
                    if( true == m_bogiesounds.empty() ) {
                        // fallback for cases without specified noise locations, convert sound template to a single sound source
                        m_bogiesounds.emplace_back( noisetemplate );
                    }
                    else {
                        // apply configuration to all defined bogies
                        for( auto &bogie : m_bogiesounds ) {
                            // combine potential x- and y-axis offsets of the sound template with z-axis offsets of individual motors
                            auto bogieoffset{ noisetemplate.offset() };
                            bogieoffset.z = bogie.offset().z;
                            bogie = noisetemplate;
                            bogie.offset( bogieoffset );
                        }
                    }
                    // apply randomized playback start offset for each bogie, to reduce potential reverb with identical nearby sources
                    auto bogieidx( 0 );
                    for( auto &bogie : m_bogiesounds ) {
                        bogie.start( (
                            bogieidx % 2 ?
                                LocalRandom(  0.0, 30.0 ) :
                                LocalRandom( 50.0, 80.0 ) )
                            * 0.01 );
                        ++bogieidx;
                    }
#else
                    m_outernoise = noisetemplate;
                    // apply randomized playback start offset, to reduce potential reverb with identical nearby sources
                    m_outernoise.start( Random( 0.0, 80.0 ) * 0.01 );
#endif
                }

                else if( token == "wheelflat:" ) {
                    // szum podczas jazdy:
                    m_wheelflat.deserialize( parser, sound_type::single, sound_parameters::frequency );
                    m_wheelflat.owner( this );
                }

                else if(token == "announcements:") {
					// announcement sounds
                    // content provided as "key: value" pairs together enclosed in "{}"
                    // value can be optionally set of values enclosed in "[]" in which case one value will be picked randomly
                    std::unordered_map<std::string, announcement_t> const announcements = {
                        { "near_stop:", announcement_t::approaching },
                        { "stop:", announcement_t::current },
                        { "next_stop:", announcement_t::next },
                        { "destination:", announcement_t::destination },
                        { "chime:", announcement_t::chime } };
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                        && ( token != "}" ) ) {
                        if( token.back() == ':' ) {

                            if( token == "soundproofing:" ) {
                                // custom soundproofing in format [ p1, p2, p3, p4, p5, p6 ]
                                parser.getTokens( 6, false, "\n\r\t ,;[]" );
                                std::array<float, 6> soundproofing;
                                parser
                                    >> soundproofing[ 0 ]
                                    >> soundproofing[ 1 ]
                                    >> soundproofing[ 2 ]
                                    >> soundproofing[ 3 ]
                                    >> soundproofing[ 4 ]
                                    >> soundproofing[ 5 ];
                                for( auto & soundproofingelement : soundproofing ) {
                                    if( soundproofingelement != -1.f ) {
                                        soundproofingelement = std::sqrt( clamp( soundproofingelement, 0.f, 1.f ) );
                                    }
                                }
                                m_pasystem.soundproofing = soundproofing;
                                continue;
                            }

                            auto const lookup { announcements.find( token ) };
                            auto const announcementtype { (
                                lookup != announcements.end() ?
                                    lookup->second :
                                    announcement_t::idle ) };
                            // NOTE: we retrieve key value for all keys, not just recognized ones
                            if( announcementtype == announcement_t::idle ) {
                                token = parser.getToken<std::string>();
                                continue;
                            }
/*
                            auto announcementsound { deserialize_random_set( parser ) };
                            replace_slashes( announcementsound );
*/
                            sound_source soundtemplate { sound_placement::engine }; // NOTE: sound range gets filled by pa system
                            soundtemplate.deserialize( parser, sound_type::single );
                            soundtemplate.owner( this );
                            m_pasystem.announcements[ static_cast<int>( announcementtype ) ] = soundtemplate;
                        }
                    }
                    // set provided custom soundproofing to assigned sounds (for sounds without their own custom soundproofing)
                    if( m_pasystem.soundproofing ) {
                        for( auto &announcement : m_pasystem.announcements ) {
                            if( !announcement.soundproofing() ) {
                                announcement.soundproofing() = m_pasystem.soundproofing;
                            }
                        }
                    }
                }

			} while( ( token != "" )
				  && ( token != "endsounds" ) );

        } // sounds:

        else if( token == "locations:" ) {
            do {
                token = "";
                parser.getTokens(); parser >> token;

                if( token == "doors:" ) {
                    // a list of pairs: offset along vehicle's z-axis and sides on which the door is present; followed with "end"
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                        && ( token != "end" ) ) {
                        // vehicle faces +Z in 'its' space, for door locations negative value means ahead of centre
                        auto const offset { std::atof( token.c_str() ) * -1.f };
                        // recognized side specifications are "right", "left" and "both"
                        auto const sides { parser.getToken<std::string>() };
                        // NOTE: we skip setting owner of the sounds, it'll be done during individual sound deserialization
                        door_sounds door;
                        // add entries to the list:
                        if( ( sides == "both" )
                         || ( sides == "left" ) ) {
                            // left...
                            auto const location { glm::vec3 { MoverParameters->Dim.W * 0.5f, MoverParameters->Dim.H * 0.5f, offset } };
                            door.placement = side::left;
                            door.rsDoorClose.offset( location );
                            door.rsDoorOpen.offset( location );
                            door.lock.offset( location );
                            door.unlock.offset( location );
                            door.step_close.offset( location );
                            door.step_open.offset( location );
                            door.permit_granted.offset( location );
                            m_doorsounds.emplace_back( door );
                        }
                        if( ( sides == "both" )
                         || ( sides == "right" ) ) {
                            // ...and right
                            auto const location { glm::vec3 { MoverParameters->Dim.W * -0.5f, 2.f, offset } };
                            door.placement = side::right;
                            door.rsDoorClose.offset( location );
                            door.rsDoorOpen.offset( location );
                            door.lock.offset( location );
                            door.unlock.offset( location );
                            door.step_close.offset( location );
                            door.step_open.offset( location );
                            door.permit_granted.offset( location );
                            m_doorsounds.emplace_back( door );
                        }
                        m_doorspeakers.emplace_back(
                            doorspeaker_sounds {
                                { 0.f, 3.f, offset },
                                {} } );
                    }
                }

                else if( token == "tractionmotors:" ) {
                    // a list of offsets along vehicle's z-axis; followed with "end"
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                        && ( token != "end" ) ) {
                        // vehicle faces +Z in 'its' space, for motor locations negative value means ahead of centre
                        auto const offset { std::atof( token.c_str() ) * -1.f };
                        // NOTE: we skip setting owner of the sounds, it'll be done during individual sound deserialization
                        sound_source motor { sound_placement::external }; // generic traction motor sounds
                        sound_source acmotor { sound_placement::external }; // inverter-specific traction motor sounds
                        sound_source motorblower { sound_placement::engine }; // associated motor blowers
                        // add entry to the list
                        auto const location { glm::vec3 { 0.f, 0.f, offset } };
                        motor.offset( location );
                        m_powertrainsounds.motors.emplace_back( motor );
                        acmotor.offset( location );
                        m_powertrainsounds.acmotors.emplace_back( acmotor );
                        motorblower.offset( location );
                        m_powertrainsounds.motorblowers.emplace_back( motorblower );
                    }
                }

                else if( token == "bogies:" ) {
                    // a list of offsets along vehicle's z-axis; followed with "end"
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                        && ( token != "end" ) ) {
                        // vehicle faces +Z in 'its' space, for bogie locations negative value means ahead of centre
                        auto const offset { std::atof( token.c_str() ) * -1.f };
                        // NOTE: we skip setting owner of the sounds, it'll be done during individual sound deserialization
                        sound_source bogienoise { sound_placement::external, EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
                        // add entry to the list
                        auto const location { glm::vec3 { 0.f, 0.f, offset } };
                        bogienoise.offset( location );
#ifdef EU07_SOUND_BOGIESOUNDS
                        m_bogiesounds.emplace_back( bogienoise );
#endif
                    }
                }

            } while( ( token != "" )
                  && ( token != "endlocations" ) );

        } // locations:

        else if( token == "internaldata:" ) {
            // dalej nie czytaj
            do {
                // zbieranie informacji o kabinach
                token = "";
                parser.getTokens(); parser >> token;
                if( token == "cab0model:" ) {
                    parser.getTokens(); parser >> token;
                    if( token != "none" ) { iCabs |= 0x2; }
                }
                else if( token == "cab1model:" ) {
                    parser.getTokens(); parser >> token;
                    if( token != "none" ) { iCabs |= 0x1; }
                }
                else if( token == "cab2model:" ) {
                    parser.getTokens(); parser >> token;
                    if( token != "none" ) { iCabs |= 0x4; }
                }
                // engine sounds
                else if( token == "ignition:" ) {
                    // odpalanie silnika
                    m_powertrainsounds.engine_ignition.deserialize( parser, sound_type::single );
                    m_powertrainsounds.engine_ignition.owner( this );
                } else if (token == "shutdown:") {
                    m_powertrainsounds.engine_shutdown.deserialize(parser, sound_type::single);
                    m_powertrainsounds.engine_shutdown.owner(this);
				} else if( token == "engageslippery:" ) {
                    // tarcie tarcz sprzegla:
                    m_powertrainsounds.rsEngageSlippery.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency );
                    m_powertrainsounds.rsEngageSlippery.owner( this );

                    m_powertrainsounds.rsEngageSlippery.m_frequencyfactor /= ( 1 + MoverParameters->nmax );
                }
				else if (token == "retarder:") {
					m_powertrainsounds.retarder.deserialize(parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency);
					m_powertrainsounds.retarder.owner(this);
				}
				else if( token == "linebreakerclose:" ) {
                    m_powertrainsounds.linebreaker_close.deserialize( parser, sound_type::single );
                    m_powertrainsounds.linebreaker_close.owner( this );
                }
                else if( token == "linebreakeropen:" ) {
                    m_powertrainsounds.linebreaker_open.deserialize( parser, sound_type::single );
                    m_powertrainsounds.linebreaker_open.owner( this );
                }
                else if( token == "relay:" ) {
                    // styczniki itp:
                    m_powertrainsounds.motor_relay.deserialize( parser, sound_type::single );
                    m_powertrainsounds.motor_relay.owner( this );
                }
                else if( token == "shuntfield:" ) {
                    // styczniki itp:
                    m_powertrainsounds.motor_shuntfield.deserialize( parser, sound_type::single );
                    m_powertrainsounds.motor_shuntfield.owner( this );
                }
                else if( token == "wejscie_na_bezoporow:" ) {
                    // hunter-111211: wydzielenie wejscia na bezoporowa i na drugi uklad do pliku
                    m_powertrainsounds.dsbWejscie_na_bezoporow.deserialize( parser, sound_type::single );
                    m_powertrainsounds.dsbWejscie_na_bezoporow.owner( this );
                }
                else if( token == "wejscie_na_drugi_uklad:" ) {
                    m_powertrainsounds.motor_parallel.deserialize( parser, sound_type::single );
                    m_powertrainsounds.motor_parallel.owner( this );
                }
                else if( token == "pneumaticrelay:" ) {
                    // wylaczniki pneumatyczne:
                    dsbPneumaticRelay.deserialize( parser, sound_type::single );
                    dsbPneumaticRelay.owner( this );
                }
                else if( token == "slipperysound:" ) {
                    // sanie:
                    rsSlippery.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                    rsSlippery.owner( this );

                    rsSlippery.m_amplitudefactor /= ( 1 + MoverParameters->Vmax );
                }
                // coupler sounds
                else if( token == "couplerattach:" ) {
                    sound_source soundtemplate { sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.attach_coupler = soundtemplate;
                    }
                }
                else if( token == "brakehoseattach:" ) {
                    // laczenie:
                    sound_source soundtemplate { sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.attach_brakehose = soundtemplate;
                    }
                }
                else if( token == "mainhoseattach:" ) {
                    // laczenie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.attach_mainhose = soundtemplate;
                    }
                }
                else if( token == "controlattach:" ) {
                    // laczenie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.attach_control = soundtemplate;
                    }
                }
                else if( token == "gangwayattach:" ) {
                    // laczenie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.attach_gangway = soundtemplate;
                    }
                }
                else if( token == "heatingattach:" ) {
                    // laczenie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.attach_heating = soundtemplate;
                    }
                }
                else if( token == "couplerdetach:" ) {
                    // rozlaczanie:
                    sound_source soundtemplate { sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.detach_coupler = soundtemplate;
                    }
                }
                else if( token == "brakehosedetach:" ) {
                    // rozlaczanie:
                    sound_source soundtemplate { sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.detach_brakehose = soundtemplate;
                    }
                }
                else if( token == "mainhosedetach:" ) {
                    // rozlaczanie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.detach_mainhose = soundtemplate;
                    }
                }
                else if( token == "controldetach:" ) {
                    // rozlaczanie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.detach_control = soundtemplate;
                    }
                }
                else if( token == "gangwaydetach:" ) {
                    // rozlaczanie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.detach_gangway = soundtemplate;
                    }
                }
                else if( token == "heatingdetach:" ) {
                    // rozlaczanie:
                    sound_source soundtemplate{ sound_placement::external };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.detach_heating = soundtemplate;
                    }
                }
                else if( token == "couplerstretch:" ) {
                    // coupler stretching
                    sound_source couplerstretch { sound_placement::external };
                    couplerstretch.deserialize( parser, sound_type::single );
                    couplerstretch.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbCouplerStretch = couplerstretch;
                    }
                }
                else if( token == "couplerstretch_loud:" ) {
                    // coupler stretching
                    sound_source couplerstretch { sound_placement::external };
                    couplerstretch.deserialize( parser, sound_type::single );
                    couplerstretch.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbCouplerStretch_loud = couplerstretch;
                    }
                }
                else if( token == "bufferclamp:" ) {
                    // buffers hitting one another
                    sound_source bufferclash { sound_placement::external };
                    bufferclash.deserialize( parser, sound_type::single );
                    bufferclash.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbBufferClamp = bufferclash;
                    }
                }
                else if( token == "bufferclamp_loud:" ) {
                    // buffers hitting one another
                    sound_source bufferclash { sound_placement::external };
                    bufferclash.deserialize( parser, sound_type::single );
                    bufferclash.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbBufferClamp_loud = bufferclash;
                    }
                }
                else if( token == "coupleradapterattach:" ) {
                    // laczenie:
                    sound_source adapterattach { sound_placement::external };
                    adapterattach.deserialize( parser, sound_type::single );
                    adapterattach.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbAdapterAttach = adapterattach;
                    }
                }
                else if( token == "coupleradapterremove:" ) {
                    // rozlaczanie:
                    sound_source adapterremove { sound_placement::external };
                    adapterremove.deserialize( parser, sound_type::single );
                    adapterremove.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbAdapterRemove = adapterremove;
                    }
                }
                else if( token == "startjolt:" ) {
                    // movement start jolt
                    m_startjolt.deserialize( parser, sound_type::single );
                    m_startjolt.owner( this );
                }

                else if (token == "mechspring:")
                {
                    // parametry bujania kamery:
                    double ks, kd;
                    parser.getTokens(2, false);
                    parser
                        >> ks
                        >> kd;
                    ShakeSpring.Init(ks, kd);
                    parser.getTokens(6, false);
                    parser
                        >> BaseShake.jolt_scale.x
                        >> BaseShake.jolt_scale.y
                        >> BaseShake.jolt_scale.z
                        >> BaseShake.jolt_limit
                        >> BaseShake.angle_scale.x
                        >> BaseShake.angle_scale.z;
                }
                else if( token == "enginespring:" ) {
                    parser.getTokens( 5, false );
                    parser
                        >> EngineShake.scale
                        >> EngineShake.fadein_offset
                        >> EngineShake.fadein_factor
                        >> EngineShake.fadeout_offset
                        >> EngineShake.fadeout_factor;
                    // offsets values are provided as rpm for convenience
                    EngineShake.fadein_offset /= 60.f;
                    EngineShake.fadeout_offset /= 60.f;
                }
                else if( token == "huntingspring:" ) {
                    parser.getTokens( 4, false );
                    parser
                        >> HuntingShake.scale
                        >> HuntingShake.frequency
                        >> HuntingShake.fadein_begin
                        >> HuntingShake.fadein_end;
                }

                else if( token == "soundproofing:" ) {
                    for( auto &soundproofingtable : m_soundproofing ) {
                        for( auto &soundproofingelement : soundproofingtable ) {
                            auto const value { parser.getToken<float>( false ) };
                            if( value != -1.f ) {
                                soundproofingelement = std::sqrt( clamp( value, 0.f, 1.f ) );
                            }
                        }
                    }
                }

                else if( token == "jointcabs:" ) {
                    parser.getTokens();
                    parser >> JointCabs;
                }

                else if( token == "cablight:" ) {
                    parser.getTokens( 3, false ); // low power light, ignore
                    parser.getTokens( 3, false ); // base light
                    parser
                        >> InteriorLight.r
                        >> InteriorLight.g
                        >> InteriorLight.b;
                    InteriorLight = glm::clamp( InteriorLight / 255.f, glm::vec3( 0.f ), glm::vec3( 1.f ) );
                    parser.getTokens( 3, false ); // dimmed light, ignore
                }

                else if( token == "pydestinationsign:" ) {
                    DestinationSign.deserialize( parser );
                    // supply vehicle folder as script path if none is provided
                    if( ( false == DestinationSign.script.empty() )
                     && ( substr_path( DestinationSign.script ).empty() ) ) {
                        DestinationSign.script = asBaseDir + DestinationSign.script;
                    }
                }
                // NOTE: legacy key, now expected as optional "background:" parameter in pydestinationsign: { parameter block }
                else if( token == "destinationsignbackground:" ) {
                    parser.getTokens();
                    parser >> DestinationSign.background;
                }

            } while( token != "" );

        } // internaldata:

	} while( token != "" );

    if( !iAnimations ) {
        // if the animations weren't defined the model is likely to be non-functional. warrants a warning.
        ErrorLog( "Animations tag is missing from the .mmd file \"" + asFileName + "\"" );
    }

    if( ReplacableSkin != "none" ) {
        // potentially set blank destination texture
        DestinationSign.destination_off = DestinationFind( ( DestinationSign.background.empty() ? "nowhere" : DestinationSign.background ) );
    }

    // assign default samples to sound emitters which weren't included in the config file
    if( TestFlag( MoverParameters->CategoryFlag, 1 ) ) {
        // rail vehicles:
        // engine
        if( MoverParameters->Power > 0 ) {
            if( true == m_powertrainsounds.dsbWejscie_na_bezoporow.empty() ) {
                // hunter-111211: domyslne, gdy brak
                m_powertrainsounds.dsbWejscie_na_bezoporow.deserialize( "wejscie_na_bezoporow", sound_type::single );
                m_powertrainsounds.dsbWejscie_na_bezoporow.owner( this );
            }
            if( true == m_powertrainsounds.motor_parallel.empty() ) {
                m_powertrainsounds.motor_parallel.deserialize( "wescie_na_drugi_uklad", sound_type::single );
                m_powertrainsounds.motor_parallel.owner( this );
            }
        }
        // braking sounds
        if( true == rsUnbrake.empty() ) {
            rsUnbrake.deserialize( "[1007]estluz", sound_type::single );
            rsUnbrake.owner( this );
        }
        // couplers
        for( auto &couplersounds : m_couplersounds ) {
            if( true == couplersounds.attach_coupler.empty() ) {
                couplersounds.attach_coupler.deserialize( "couplerattach_default", sound_type::single );
                couplersounds.attach_coupler.owner( this );
            }
            if( true == couplersounds.detach_coupler.empty() ) {
                couplersounds.detach_coupler.deserialize( "couplerdetach_default", sound_type::single );
                couplersounds.detach_coupler.owner( this );
            }
            if( true == couplersounds.dsbCouplerStretch.empty() ) {
                couplersounds.dsbCouplerStretch.deserialize( "couplerstretch_default", sound_type::single );
                couplersounds.dsbCouplerStretch.owner( this );
            }
            if( true == couplersounds.dsbBufferClamp.empty() ) {
                couplersounds.dsbBufferClamp.deserialize( "bufferclamp_default", sound_type::single );
                couplersounds.dsbBufferClamp.owner( this );
            }
        }
        // other sounds
        if( true == m_wheelflat.empty() ) {
            m_wheelflat.deserialize( "lomotpodkucia 0.23 0.0", sound_type::single, sound_parameters::frequency );
            m_wheelflat.owner( this );
        }
        if( true == rscurve.empty() ) {
            // hunter-111211: domyslne, gdy brak
            rscurve.deserialize( "curve", sound_type::single );
            rscurve.owner( this );
        }
    }

    if (mdModel)
        mdModel->Init(); // obrócenie modelu oraz optymalizacja, również zapisanie binarnego
    if (mdLoad)
        mdLoad->Init();
    if (mdLowPolyInt)
        mdLowPolyInt->Init();
    for( auto *attachment : mdAttachments ) {
        attachment->Init();
    }

    Global.asCurrentTexturePath = szTexturePath; // kiedyś uproszczone wnętrze mieszało tekstury nieba
    Global.asCurrentDynamicPath = "";

    // position sound emitters which weren't defined in the config file
    // engine sounds, centre of the vehicle
    auto const enginelocation { glm::vec3 {0.f, MoverParameters->Dim.H * 0.5f, 0.f } };
    m_powertrainsounds.position( enginelocation );
    // other engine compartment sounds
    auto const nullvector { glm::vec3() };
    std::vector<sound_source *> enginesounds = {
        &sConverter, &sCompressor, &sCompressorIdle, &sSmallCompressor, &sHeater, &m_batterysound
    };
    for( auto sound : enginesounds ) {
        if( sound->offset() == nullvector ) {
            sound->offset( enginelocation );
        }
    }

    // pantographs
    if( pants != nullptr ) {
        std::size_t pantographindex { 0 };
        for( auto &pantographsounds : m_pantographsounds ) {
            auto const pantographoffset {
                glm::vec3(
                    0.f,
                    pants[ pantographindex ].fParamPants->vPos.y,
                    -pants[ pantographindex ].fParamPants->vPos.x ) };
            pantographsounds.sPantUp.offset( pantographoffset );
            pantographsounds.sPantDown.offset( pantographoffset );
            ++pantographindex;
        }
    }
    // doors
    std::size_t dooranimationfirstindex { 0 };
    for( std::size_t i = 0; i < ANIM_DOORS; ++i ) {
        // zliczanie wcześniejszych animacji
        dooranimationfirstindex += iAnimType[ i ];
    }
    // couplers
    auto const frontcoupleroffset { glm::vec3{ 0.f, 1.f, MoverParameters->Dim.L * 0.5f } };
    m_couplersounds[ end::front ].attach_coupler.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].attach_brakehose.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].attach_mainhose.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].attach_control.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].attach_gangway.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].attach_heating.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].detach_coupler.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].detach_brakehose.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].detach_mainhose.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].detach_control.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].detach_gangway.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].detach_heating.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].dsbCouplerStretch.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].dsbCouplerStretch_loud.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].dsbBufferClamp.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].dsbBufferClamp_loud.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].dsbAdapterAttach.offset( frontcoupleroffset );
    m_couplersounds[ end::front ].dsbAdapterRemove.offset( frontcoupleroffset );
    auto const rearcoupleroffset { glm::vec3{ 0.f, 1.f, MoverParameters->Dim.L * -0.5f } };
    m_couplersounds[ end::rear ].attach_coupler.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].attach_brakehose.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].attach_mainhose.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].attach_control.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].attach_gangway.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].attach_heating.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].detach_coupler.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].detach_brakehose.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].detach_mainhose.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].detach_control.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].detach_gangway.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].detach_heating.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].dsbCouplerStretch.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].dsbCouplerStretch_loud.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].dsbBufferClamp.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].dsbBufferClamp_loud.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].dsbAdapterAttach.offset( rearcoupleroffset );
    m_couplersounds[ end::rear ].dsbAdapterRemove.offset( rearcoupleroffset );
}

TModel3d *
TDynamicObject::LoadMMediaFile_mdload( std::string const &Name ) const {

    auto const loadname { ( Name.empty() ? "none" : Name ) };
    TModel3d *loadmodel { nullptr };

    // check if we don't have model override for this load type
    if ( loadmodel == nullptr )
    {
        auto const lookup { LoadModelOverrides.find( loadname ) };
        if( lookup != LoadModelOverrides.end() ) {
            loadmodel = TModelsManager::GetModel( asBaseDir + lookup->second, true );
        }
    }
    // regular routine if there's no override or it couldn't be loaded
    // try first specialized version of the load model, vehiclename_loadname
    if ( loadmodel == nullptr )
    {
        auto const specializedloadfilename { asBaseDir + MoverParameters->TypeName + "_" + loadname };
        loadmodel = TModelsManager::GetModel( specializedloadfilename, true, false );
    }
    // try generic version of the load model next, loadname
    if ( loadmodel == nullptr )
    {
        auto const genericloadfilename { asBaseDir + loadname };
        loadmodel = TModelsManager::GetModel( genericloadfilename, true, false );
    }

    if ( loadmodel != nullptr )
        loadmodel->GetSMRoot()->WillBeAnimated();

    return loadmodel;
}

//---------------------------------------------------------------------------
void TDynamicObject::RadioStop()
{ // zatrzymanie pojazdu
    if( Mechanik ) {
        // o ile ktoś go prowadzi
		if( ( MoverParameters->SecuritySystem.radiostop_available() )
         && ( MoverParameters->Radio ) ) {
            // jeśli pojazd ma RadioStop i jest on aktywny
            // HACK cast until math types unification
			glm::dvec3 pos = static_cast<glm::dvec3>(vPosition);
            Mechanik->PutCommand( "Emergency_brake", 1.0, 1.0, &pos, stopRadio );
            // add onscreen notification for human driver
            // TODO: do it selectively for the 'local' driver once the multiplayer is in
            if( false == Mechanik->AIControllFlag ) {
                ui::Transcripts.AddLine( "!! RADIO-STOP !!", 0.0, 10.0, false );
            }
        }
    }
};

//---------------------------------------------------------------------------
void TDynamicObject::Damage(char flag)
{
	if (flag & 1)  //różnicówka nie robi nic
	{
		MoverParameters->MainSwitch(false);
		MoverParameters->FuseOff();
	}
	else
	{
	}

	if (flag & 2)  //usterka sterowania
	{
		MoverParameters->StLinFlag = false;
		if (MoverParameters->InitialCtrlDelay<100000000)
			MoverParameters->InitialCtrlDelay += 100000001;
	}
	else
	{
		if (MoverParameters->InitialCtrlDelay>100000000)
			MoverParameters->InitialCtrlDelay -= 100000001;
	}

	if (flag & 4)  //blokada przetwornicy
	{
        if( MoverParameters->ConverterStart != start_t::disabled ) {
            MoverParameters->ConvOvldFlag = true;
        }
	}
	else
	{
	}

	if (flag & 8)  //blokada sprezarki
	{
		if (MoverParameters->MinCompressor>0)
			MoverParameters->MinCompressor -= 100000001;
		if (MoverParameters->MaxCompressor>0)
			MoverParameters->MaxCompressor -= 100000001;
	}
	else
	{
		if (MoverParameters->MinCompressor<0)
			MoverParameters->MinCompressor += 100000001;
		if (MoverParameters->MaxCompressor<0)
			MoverParameters->MaxCompressor += 100000001;
	}

	if (flag & 16)  //blokada wału
	{
		if (MoverParameters->CtrlDelay<100000000)
			MoverParameters->CtrlDelay += 100000001;
		if (MoverParameters->CtrlDownDelay<100000000)
			MoverParameters->CtrlDownDelay += 100000001;
	}
	else
	{
		if (MoverParameters->CtrlDelay>100000000)
			MoverParameters->CtrlDelay -= 100000001;
		if (MoverParameters->CtrlDownDelay>100000000)
			MoverParameters->CtrlDownDelay -= 100000001;
	}

	if (flag & 32)  //hamowanie nagŁe
	{
	}
	else
	{
	}

	MoverParameters->EngDmgFlag = flag;
};

void TDynamicObject::SetLights() {

    auto const isfrontcaboccupied { MoverParameters->CabOccupied * DirectionGet() >= 0 };
	int const automaticmarkers { MoverParameters->CabActive == 0 && ( MoverParameters->InactiveCabFlag & activation::redmarkers )
								? light::redmarker_left + light::redmarker_right : 0 };
    int const front { ( isfrontcaboccupied ? end::front : end::rear ) };
    int const rear { 1 - front };
    auto const lightpos { MoverParameters->LightsPos - 1 };
    auto const frontlights { automaticmarkers > 0 ? automaticmarkers : MoverParameters->Lights[ front ][ lightpos ] };
    auto const rearlights { automaticmarkers > 0 ? automaticmarkers : MoverParameters->Lights[ rear ][ lightpos ] };
    auto *vehicle { GetFirstDynamic( MoverParameters->CabOccupied >= 0 ? end::front : end::rear, coupling::control ) };
    while( vehicle != nullptr ) {
        // set lights on given side if there's no coupling with another vehicle, turn them off otherwise
        auto const *frontvehicle { ( isfrontcaboccupied ? vehicle->Prev( coupling::coupler ) : vehicle->Next( coupling::coupler ) ) };
        auto const *rearvehicle { ( isfrontcaboccupied ? vehicle->Next( coupling::coupler ) : vehicle->Prev( coupling::coupler ) ) };
        vehicle->RaLightsSet(
            ( frontvehicle == nullptr ? frontlights : 0 ),
            ( rearvehicle == nullptr ? rearlights : 0 ) );
        vehicle = ( isfrontcaboccupied ? vehicle->Next( coupling::control ) : vehicle->Prev( coupling::control ) );
    }
};

void TDynamicObject::RaLightsSet(int head, int rear)
{ // zapalenie świateł z przodu i z
    // tyłu, zależne od kierunku
    // pojazdu
    if (!MoverParameters)
        return; // może tego nie być na początku
    if (rear == ( light::redmarker_left | light::redmarker_right | light::rearendsignals ) )
    { // jeśli koniec pociągu, to trzeba ustalić, czy
        // jest tam czynna lokomotywa
        // EN57 może nie mieć końcówek od środka członu
        if (MoverParameters->Power > 1.0) // jeśli ma moc napędową
            if (!MoverParameters->DirActive) // jeśli nie ma ustawionego kierunku
            { // jeśli ma zarówno światła jak i końcówki, ustalić, czy jest w stanie aktywnym
                // np. lokomotywa na zimno będzie mieć końcówki a nie światła
                rear = light::rearendsignals; // tablice blaszane
                // trzeba to uzależnić od "załączenia baterii" w pojeździe
            }
        if( rear == ( light::redmarker_left | light::redmarker_right | light::rearendsignals ) ) // jeśli nadal obydwie możliwości
            if( iInventory[
                ( iDirection ?
                    end::rear :
                    end::front ) ] & ( light::redmarker_left | light::redmarker_right ) ) {
                // czy ma jakieś światła czerowone od danej strony
                rear = ( light::redmarker_left | light::redmarker_right ); // dwa światła czerwone
            }
            else {
                rear = light::rearendsignals; // tablice blaszane
            }
    }
    // w zależności od kierunku pojazdu w składzie
    if( head >= 0 ) {
        auto const vehicleend { iDirection > 0 ? end::front : end::rear };
        MoverParameters->iLights[ vehicleend ] = ( head & iInventory[ vehicleend ] );
    }
    if( rear >= 0 ) {
        auto const vehicleend{ iDirection > 0 ? end::rear : end::front };
        MoverParameters->iLights[ vehicleend ] = ( rear & iInventory[ vehicleend ] );
    }
};

bool TDynamicObject::has_signal_pc1_on() const {

    auto const vehicleend { iDirection > 0 ? end::front : end::rear };
    auto const equippedlights { iInventory[ vehicleend ] };
    auto const pattern { equippedlights & ( light::headlight_left | light::headlight_right | light::headlight_upper ) };
    auto const patternfallback { equippedlights & ( light::auxiliary_left | light::auxiliary_right | light::headlight_upper ) };
    auto const hasauxiliarylights { ( equippedlights & ( light::auxiliary_left | light::auxiliary_right ) ) != 0 };
    auto const activelights { MoverParameters->iLights[ vehicleend ] };

    return ( ( ( pattern != 0 ) && ( activelights == pattern ) )
          || ( ( hasauxiliarylights ) && ( activelights == patternfallback ) )
          || ( ( pattern == 0 ) && ( patternfallback == 0 ) && ( activelights == light::rearendsignals ) ) ); // pc4
}

bool TDynamicObject::has_signal_pc2_on() const {

    auto const vehicleend { iDirection > 0 ? end::front : end::rear };
    auto const equippedlights { iInventory[ vehicleend ] };
    auto const pattern { equippedlights & ( light::redmarker_left | light::headlight_right | light::headlight_upper ) };
    auto const patternfallback { equippedlights & ( light::redmarker_left | light::auxiliary_right | light::headlight_upper ) };
    auto const hasauxiliarylights { ( equippedlights & ( light::auxiliary_left | light::auxiliary_right ) ) != 0 };
    auto const activelights { MoverParameters->iLights[ vehicleend ] };

    return ( ( activelights == pattern )
          || ( hasauxiliarylights && ( activelights == patternfallback ) ) );
}

bool TDynamicObject::has_signal_pc5_on() const {

    auto const vehicleend { iDirection > 0 ? end::rear : end::front };
    auto const equippedlights { iInventory[ vehicleend ] };
    auto const pattern { equippedlights & ( light::redmarker_left | light::redmarker_right ) };
    auto const patternfallback { equippedlights & ( light::rearendsignals ) };
    auto const activelights { MoverParameters->iLights[ vehicleend ] };

    return ( ( ( pattern != 0 ) && ( activelights == pattern ) )
          || ( ( patternfallback != 0 ) && ( activelights == patternfallback ) ) );
}

bool TDynamicObject::has_signal_on( int const Side, int const Pattern ) const {
/*
    auto const vehicleend { iDirection > 0 ? Side : 1 - Side };
    auto const pattern { iInventory[ vehicleend ] & Pattern };

    return ( MoverParameters->iLights[ vehicleend ] == pattern );
*/
    return ( MoverParameters->iLights[ Side ] == ( Pattern & iInventory[ Side ] ) );
}

int TDynamicObject::DirectionSet(int d)
{ // ustawienie kierunku w składzie (wykonuje AI)
    auto const lastdirection { iDirection };
    iDirection = ( d > 0 ) ? 1 : 0; // d:1=zgodny,-1=przeciwny; iDirection:1=zgodny,0=przeciwny;

    if( iDirection != lastdirection ) {
        // direction was flipped, switch recorded servicable platform sides for potentially ongoing load exchange
        auto const left { ( lastdirection > 0 ) ? 1 : 2 };
        auto const right { 3 - left };
        m_exchange.platforms =
            ( ( m_exchange.platforms & left )  != 0 ? right : 0 )
          + ( ( m_exchange.platforms & right ) != 0 ? left : 0 );
    }

    if (MyTrack)
    { // podczas wczytywania wstawiane jest AI, ale może jeszcze nie być toru
        // AI ustawi kierunek ponownie po uruchomieniu silnika
        update_neighbours();
    }
    // informacja o położeniu następnego
    return 1 - ( ( iDirection > 0 ) ? NextConnectedNo() : PrevConnectedNo() );
};

// wskaźnik na poprzedni, nawet wirtualny
TDynamicObject * TDynamicObject::PrevAny() const {
    return MoverParameters->Neighbours[ iDirection ^ 1 ].vehicle;
}
TDynamicObject * TDynamicObject::Prev(int C) const {
	return ( (MoverParameters->Couplers[ iDirection ^ 1 ].CouplingFlag & C) ?
        MoverParameters->Neighbours[ iDirection ^ 1 ].vehicle :
        nullptr );// gdy sprzęg wirtualny, to jakby nic nie było
}
TDynamicObject * TDynamicObject::Next(int C) const {
	return ( (MoverParameters->Couplers[ iDirection ].CouplingFlag & C) ?
        MoverParameters->Neighbours[ iDirection ].vehicle :
        nullptr );// gdy sprzęg wirtualny, to jakby nic nie było
}

// checks whether there's unbroken connection of specified type to specified vehicle
bool
TDynamicObject::is_connected( TDynamicObject const *Vehicle, coupling const Coupling ) const {

    auto *vehicle { this };
    if( vehicle == Vehicle ) {
        // edge case, vehicle is always "connected" with itself
        return true;
    }
    // check ahead, it's more likely the "owner" using this method is located there
    while( ( vehicle = vehicle->Prev( Coupling ) ) != nullptr ) {
        if( vehicle == Vehicle ) {
            return true;
        }
        if( vehicle == this ) {
            // edge case, looping consist
            return false;
        }
    }
    // start anew in the other direction
    vehicle = this;
    while( ( vehicle = vehicle->Next( Coupling ) ) != nullptr ) {
        if( vehicle == Vehicle ) {
            return true;
        }
    }
    // no luck in either direction, give up
    return false;
}

// ustalenie następnego (1) albo poprzedniego (0) w składzie bez względu na prawidłowość iDirection
TDynamicObject *
TDynamicObject::Neighbour(int &dir) {

    auto *neighbour { (
        MoverParameters->Couplers[ dir ].CouplingFlag != coupling::faux ?
            MoverParameters->Neighbours[ dir ].vehicle :
            nullptr ) };
    // nowa wartość
    dir = 1 - MoverParameters->Neighbours[ dir ].vehicle_end;

    return neighbour;
};

// updates potential collision sources
void
TDynamicObject::update_neighbours() {

    for( int end = end::front; end <= end::rear; ++end ) {

        auto &neighbour { MoverParameters->Neighbours[ end ] };
        auto const &coupler { MoverParameters->Couplers[ end ] };

        if( ( coupler.Connected != nullptr )
         && ( neighbour.vehicle != nullptr ) ) {
            // physical connection with another vehicle locks down collision source on this end
//            neighbour.vehicle = coupler.Connected;
//            neighbour.vehicle_end = coupler.ConnectedNr;
            neighbour.distance = TMoverParameters::CouplerDist( MoverParameters, coupler.Connected );
            // take into account potential adapters attached to the couplers
            auto const &othercoupler { neighbour.vehicle->MoverParameters->Couplers[ neighbour.vehicle_end ] };
            neighbour.distance -= coupler.adapter_length;
            neighbour.distance -= othercoupler.adapter_length;
        }
        else {
            // if there's no connected vehicle check for potential collision sources in the vicinity
            // NOTE: we perform a new scan on each update to ensure we always locate the nearest potential source
            neighbour = neighbour_data();
            // 10m ~= 140 km/h at 4 fps + safety margin, potential distance between outer axles of involved vehicles
            auto const scanrange { std::max( 10.0, std::abs( MoverParameters->V ) ) + 40.0 };
            auto const lookup { find_vehicle( end, scanrange ) };

            if( false == std::get<bool>( lookup ) ) { continue; }

            neighbour.vehicle = std::get<TDynamicObject *>( lookup );
            neighbour.vehicle_end = std::get<int>( lookup );
            neighbour.distance = std::get<double>( lookup );

            if( ( neighbour.vehicle )
             && ( neighbour.distance < ( neighbour.vehicle->MoverParameters->CategoryFlag == 2 ? 50 : 100 ) ) ) {
                // at short distances (re)calculate range between couplers directly
                neighbour.distance = TMoverParameters::CouplerDist( MoverParameters, neighbour.vehicle->MoverParameters );
                // take into account potential adapters attached to the couplers
                auto const &othercoupler { neighbour.vehicle->MoverParameters->Couplers[ neighbour.vehicle_end ] };
                neighbour.distance -= coupler.adapter_length;
                neighbour.distance -= othercoupler.adapter_length;
            }
        }
    }
}

// locates potential collision source within specified range, scanning track in specified direction. returns: true if neighbour was located, false otherwise
// NOTE: reuses legacy code. TBD, TODO: review, refactor?
std::tuple<TDynamicObject *, int, double, bool>
TDynamicObject::find_vehicle( int const Direction, double const Distance ) const {

    auto direction { ( Direction == end::front ? 1 : -1 ) };
    auto const initialdirection { direction }; // zapamiętanie kierunku poszukiwań na torze początkowym, względem sprzęgów

    auto const *track { RaTrackGet() };
    if( RaDirectionGet() < 0 ) {
        // czy oś jest ustawiona w stronę Point1?
        direction = -direction;
    }

    // (teraz względem toru)
    auto const mycoupler { ( initialdirection < 0 ? end::rear : end::front ) }; // numer sprzęgu do podłączenia w obiekcie szukajacym
    auto foundcoupler { -1 }; // numer sprzęgu w znalezionym obiekcie (znaleziony wypełni)
    auto distance { 0.0 }; // przeskanowana odleglość; odległość do zawalidrogi
    auto *foundobject { ABuFindObject( foundcoupler, distance, track, direction, mycoupler ) }; // zaczynamy szukać na tym samym torze

    if( foundobject == nullptr ) {
        // jeśli nie ma na tym samym, szukamy po okolicy szukanie najblizszego toru z jakims obiektem
        // praktycznie przeklejone z TraceRoute()...
        if (direction >= 0) // uwzględniamy kawalek przeanalizowanego wcześniej toru
            distance = track->Length() - RaTranslationGet(); // odległość osi od Point2 toru
        else
            distance = RaTranslationGet(); // odległość osi od Point1 toru

        while (distance < Distance) {
            if (direction > 0) {
                // w kierunku Point2 toru
                if( track ?
                        track->iNextDirection :
                        false ) {
                    // jeśli następny tor jest podpięty od Point2
                    direction = -direction; // to zmieniamy kierunek szukania na tym torze
                }
                if( track ) {
                    track = track->CurrentNext(); // potem dopiero zmieniamy wskaźnik
                }
            }
            else {
                // w kierunku Point1
                if( track ?
                        !track->iPrevDirection :
                        true ) {
                    // jeśli poprzedni tor nie jest podpięty od Point2
                    direction = -direction; // to zmieniamy kierunek szukania na tym torze
                }
                if( track ) {
                    track = track->CurrentPrev(); // potem dopiero zmieniamy wskaźnik
                }
            }
            if (track) {
                // jesli jest kolejny odcinek toru
                foundobject = ABuFindObject(foundcoupler, distance, track, direction, mycoupler); // przejrzenie pojazdów tego toru
                if (foundobject) {
                    break;
                }
            }
            else {
                // jeśli toru nie ma, to wychodzimy
                distance = Distance + 1.0; // koniec przeglądania torów
                break;
            }
        }
    } // Koniec szukania najbliższego toru z jakimś obiektem.

    if( foundobject == nullptr ) { return {}; }
/*
    auto const *vehicle { MoverParameters };
    auto const *foundvehicle { foundobject->MoverParameters };
    if( ( false == TestFlag( track->iCategoryFlag, 1 ) )
     && ( distance > 50.0 ) ) {
        // Ra: jeśli dwa samochody się mijają na odcinku przed zawrotką, to odległość między nimi nie może być liczona w linii prostej!
        // NOTE: the distance is approximated, and additionally less accurate for cars heading in opposite direction
        distance -= ( 0.5 * ( vehicle->Dim.L + foundvehicle->Dim.L ) );
    }
    else if( distance < 100.0 ) {
        // at short distances start to calculate range between couplers directly
        // odległość do najbliższego pojazdu w linii prostej
        distance = TMoverParameters::CouplerDist( vehicle, foundvehicle );
    }
*/
    return { foundobject, foundcoupler, distance, true };
}

TDynamicObject * TDynamicObject::FindPowered()
{ // taka proteza:
    // chcę podłączyć kabinę EN57 bezpośrednio z silnikowym, aby nie robić tego przez ukrotnienie
    // drugi silnikowy i tak musi być ukrotniony, podobnie jak kolejna jednostka
    // lepiej by było przesyłać komendy sterowania, co jednak wymaga przebudowy transmisji komend (LD)
    // problem się robi ze światłami, które będą zapalane w silnikowym, ale muszą świecić się w rozrządczych
    // dla EZT światłą czołowe będą "zapalane w silnikowym", ale widziane z rozrządczych
    // również wczytywanie MMD powinno dotyczyć aktualnego członu
    // problematyczna może być kwestia wybranej kabiny (w silnikowym...)
    // jeśli silnikowy będzie zapięty odwrotnie (tzn. -1), to i tak powinno jeździć dobrze
    // również hamowanie wykonuje się zaworem w członie, a nie w silnikowym...
    auto const coupling { (
        ( MoverParameters->TrainType == dt_EZT ) || ( MoverParameters->TrainType == dt_DMU ) ) ?
            coupling::permanent :
            coupling::control };

    auto *lookup {
        find_vehicle(
            coupling,
            []( TDynamicObject * vehicle ) {
                return ( vehicle->MoverParameters->Power > 1.0 ); } ) };

    return( lookup != nullptr ? lookup : this ); // always return valid vehicle for backward compatibility
}

TDynamicObject *
TDynamicObject::FindPantographCarrier() {

    // try first within a single unit, broaden to all vehicles under our control if first attempt fails
    std::array<coupling, 2> const couplings = { coupling::permanent, coupling::control };

    for( auto const coupling : couplings ) {
        auto *result =
            find_vehicle(
                coupling,
                []( TDynamicObject * vehicle ) {
                    return (
                        ( vehicle->MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector )
                     && ( vehicle->MoverParameters->EnginePowerSource.CollectorParameters.CollectorsNo > 0 ) ); } );
        if( result != nullptr ) {
            return result;
        }
    }
    // if we're still here, admit failure
    return nullptr;
}

//---------------------------------------------------------------------------

void TDynamicObject::ParamSet(int what, int into)
{ // ustawienie lokalnego parametru (what) na stan (into)
    switch (what & 0xFF00)
    {
    case 0x0100: // to np. są drzwi, bity 0..7 określają numer 1..254 albo maskę
        // dla 8 różnych
        if (what & 1) // na razie mamy lewe oraz prawe, czyli używamy maskę 1=lewe,
            // 2=prawe, 3=wszystkie
            if( MoverParameters->Doors.instances[side::left].is_open )
            { // są otwarte
                if (!into) // jeśli zamykanie
                {
                    // dźwięk zamykania
                }
            }
            else
            { // są zamknięte
                if (into) // jeśli otwieranie
                {
                    // dźwięk otwierania
                }
            }
        if (what & 2) // prawe działają niezależnie od lewych
            if( MoverParameters->Doors.instances[side::right].is_open )
            { // są otwarte
                if (!into) // jeśli zamykanie
                {
                    // dźwięk zamykania
                }
            }
            else
            { // są zamknięte
                if (into) // jeśli otwieranie
                {
                    // dźwięk otwierania
                }
            }
        break;
    }
};

int TDynamicObject::RouteWish(TTrack *tr)
{ // zapytanie do AI, po którym
    // segmencie (-6..6) jechać na
    // skrzyżowaniu (tr)
    return Mechanik ? Mechanik->CrossRoute(tr) : 0; // wg AI albo prosto
};

void TDynamicObject::DestinationSet(std::string to, std::string numer) {
    // ustawienie stacji docelowej oraz wymiennej tekstury 4, jeśli istnieje plik
    // w zasadzie, to każdy wagon mógłby mieć inną stację docelową
    // zwłaszcza w towarowych, pod kątem zautomatyzowania maewrów albo pracy górki
    // ale to jeszcze potrwa, zanim będzie możliwe, na razie można wpisać stację z
    // rozkładu

    asDestination = to;

    if( std::abs( m_materialdata.multi_textures ) >= 4 ) { return; } // jak są 4 tekstury wymienne, to nie zmieniać rozkładem
    if( DestinationSign.sign == nullptr )                { return; } // no sign submodel, no problem

    // now see if we can find any version of the destination texture
    std::vector<std::string> const destinations = {
        numer, // try dedicated timetable sign first...
        to }; // ...then generic destination sign

    for( auto const &destination : destinations ) {

        DestinationSign.destination = DestinationFind( destination );
        if( DestinationSign.destination != null_handle ) {
            // got what we wanted, we're done here
            return;
        }
    }
    // if we didn't get static texture we might be able to make one
    if( DestinationSign.script.empty() ) { return; } // no script so no way to make the texture
    if( numer == "none" )                { return; } // blank or incomplete/malformed timetable, don't bother

    std::string signrequest {
          "make:"
        + DestinationSign.script
        + "?"
        // timetable include
        + "$timetable=" + (
            ctOwner == nullptr ?
                MoverParameters->Name : // leading vehicle, can point to it directly
                ctOwner->Vehicle()->MoverParameters->Name ) // owned vehicle, safer to point to owner as carriages can have identical names
        // basic instancing string
        // NOTE: underscore doesn't have any magic meaning for the time being, it's just less likely to conflict with regular dictionary keys
        + "&_id1=" + (
            ctOwner != nullptr ? ctOwner->TrainName() :
            Mechanik != nullptr ? Mechanik->TrainName() :
            "none" ) }; // shouldn't get here but, eh
    // TBD, TODO: replace instancing with support for variables in extra parameters string?
    if( false == DestinationSign.instancing.empty() ) {
        signrequest +=
            "&_id2=" + (
                DestinationSign.instancing == "name" ? MoverParameters->Name :
                DestinationSign.instancing == "type" ? MoverParameters->TypeName :
                "none" );
    }
    // optionl extra parameters
    if( false == DestinationSign.parameters.empty() ) {
        signrequest += "&" + DestinationSign.parameters;
    }

    DestinationSign.destination = GfxRenderer->Fetch_Material( signrequest );
}

material_handle TDynamicObject::DestinationFind( std::string Destination ) {

    if( Destination.empty() ) { return null_handle; }

    // destination textures are kept in the vehicle's directory so we point the current texture path there
    auto const currenttexturepath { Global.asCurrentTexturePath };
    Global.asCurrentTexturePath = asBaseDir;

    auto destinationhandle { null_handle };

    if( starts_with( Destination, "make:" ) ) {
        // autogenerated texture
        destinationhandle = GfxRenderer->Fetch_Material( Destination );
    }
    else {
        // regular texture
        Destination = Bezogonkow( Destination ); // do szukania pliku obcinamy ogonki
        // now see if we can find any version of the texture
        std::vector<std::string> const destinations {
            Destination + '@' + MoverParameters->TypeName,
            Destination };

        for( auto const &destination : destinations ) {
            auto material = TextureTest( ToLower( destination ) );
            if( false == material.empty() ) {
                destinationhandle = GfxRenderer->Fetch_Material( material );
                break;
            }
        }
    }
    // whether we got anything, restore previous texture path
    Global.asCurrentTexturePath = currenttexturepath;

    return destinationhandle;
}

void TDynamicObject::announce( announcement_t const Announcement, bool const Chime ) {

    if( m_doorspeakers.empty() ) { return; }

    auto const *driver { (
        ctOwner != nullptr ?
            ctOwner :
            Mechanik ) };
    if( driver == nullptr ) { return; }

    auto const &timetable { driver->TrainTimetable() };
    auto const &announcements { m_pasystem.announcements };
    auto playchime { Chime };

    if( announcements[ static_cast<int>( Announcement ) ].empty() ) {
        goto followup;
    }
    // if the announcement sound was defined queue playback
    {
        sound_source stopnamesound;
        switch( Announcement ) {
            case announcement_t::approaching:
            case announcement_t::next: {
                stopnamesound = timetable.next_stop_sound();
                break;
            }
            case announcement_t::current: {
                stopnamesound = timetable.current_stop_sound();
                break;
            }
            case announcement_t::destination: {
                stopnamesound = timetable.last_stop_sound();
                break;
            }
            default: {
                break;
            }
        }
        if( stopnamesound.empty() ) {
            goto followup;
        }
        // potentially precede the announcement with a chime...
        if( ( true == playchime )
         && ( false == announcements[ static_cast<int>( announcement_t::chime ) ].empty() ) ) {
            m_pasystem.announcement_queue.emplace_back( announcements[ static_cast<int>( announcement_t::chime ) ] );
            playchime = false;
        }
        // ...then play the announcement itself
        m_pasystem.announcement_queue.emplace_back( announcements[ static_cast<int>( Announcement ) ] );
        m_pasystem.announcement_queue.emplace_back( stopnamesound );
    }
followup:
    // potentially follow up with another announcement
    if( Announcement == announcement_t::next ) {
        announce( announcement_t::destination, playchime );
    }
}

void TDynamicObject::OverheadTrack(float o)
{ // ewentualne wymuszanie jazdy
    // bezprądowej z powodu informacji
    // w torze
    if (ctOwner) // jeśli ma obiekt nadzorujący
    { // trzeba zaktualizować mapę flag bitowych jazdy bezprądowej
        if (o < 0.0)
        { // normalna jazda po tym torze
            ctOwner->iOverheadZero &= ~iOverheadMask; // zerowanie bitu - może pobierać prąd
            ctOwner->iOverheadDown &= ~iOverheadMask; // zerowanie bitu - może podnieść pantograf
        }
        else if (o > 0.0)
        { // opuszczenie pantografów
            ctOwner->iOverheadZero |= iOverheadMask; // ustawienie bitu - ma jechać bez pobierania prądu
            ctOwner->iOverheadDown |= iOverheadMask; // ustawienie bitu - ma opuścić pantograf
        }
        else
        { // jazda bezprądowa z podniesionym pantografem
            ctOwner->iOverheadZero |=  iOverheadMask; // ustawienie bitu - ma jechać bez pobierania prądu
            ctOwner->iOverheadDown &= ~iOverheadMask; // zerowanie bitu - może podnieść pantograf
        }
    }
};

void
TDynamicObject::update_shake( double const Timedelta ) {
    // Ra: mechanik powinien być telepany niezależnie od pozycji pojazdu
    // Ra: trzeba zrobić model bujania głową i wczepić go do pojazdu

    // Ra: tu by się przydało uwzględnić rozkład sił:
    // - na postoju horyzont prosto, kabina skosem
    // - przy szybkiej jeździe kabina prosto, horyzont pochylony

    // McZapkie: najpierw policzę pozycję w/m kabiny

    // ABu: rzucamy kabina tylko przy duzym FPS!
    // Mala histereza, zeby bez przerwy nie przelaczalo przy FPS~17
    // Granice mozna ustalic doswiadczalnie. Ja proponuje 14:20
    if( Global.iSlowMotion == 0 ) { // musi być pełna prędkość

        Math3D::vector3 shakevector;
        if( ( MoverParameters->EngineType == TEngineType::DieselElectric )
         || ( MoverParameters->EngineType == TEngineType::DieselEngine ) ) {
            if( std::abs( MoverParameters->enrot ) > 0.0 ) {
                // engine vibration
                shakevector.x +=
                    ( std::sin( MoverParameters->eAngle * 4.0 ) * Timedelta * EngineShake.scale )
                    // fade in with rpm above threshold
                    * clamp(
                        ( MoverParameters->enrot - EngineShake.fadein_offset ) * EngineShake.fadein_factor,
                        0.0, 1.0 )
                    // fade out with rpm above threshold
                    * interpolate(
                        1.0, 0.0,
                        clamp(
                            ( MoverParameters->enrot - EngineShake.fadeout_offset ) * EngineShake.fadeout_factor,
                            0.0, 1.0 ) );
            }
        }

        if( ( HuntingShake.fadein_begin > 0.f )
         && ( true == MoverParameters->TruckHunting ) ) {
            // hunting oscillation
            HuntingAngle = clamp_circular( HuntingAngle + 4.0 * HuntingShake.frequency * Timedelta * MoverParameters->Vel, 360.0 );
            auto const huntingamount =
                interpolate(
                    0.0, 1.0,
                    clamp(
                        ( MoverParameters->Vel - HuntingShake.fadein_begin ) / ( HuntingShake.fadein_end - HuntingShake.fadein_begin ),
                        0.0, 1.0 ) );
            shakevector.x +=
                ( std::sin( glm::radians( HuntingAngle ) ) * Timedelta * HuntingShake.scale )
                * huntingamount;
            IsHunting = ( huntingamount > 0.025 );
        }

		if (FreeFlyModeFlag)
            shakevector *= 0;

        auto const iVel { std::min( GetVelocity(), 150.0 ) };
        if( iVel > 0.5 ) {
            // acceleration-driven base shake
            shakevector += Math3D::vector3(
                -MoverParameters->AccN * Timedelta * 5.0, // highlight side sway
                -MoverParameters->AccVert * Timedelta,
                -MoverParameters->AccSVBased * Timedelta * 1.25 ); // accent acceleration/deceleration
        }

        auto shake { 1.25 * ShakeSpring.ComputateForces( shakevector, ShakeState.offset ) };

        if( LocalRandom( iVel ) > 25.0 ) {
            // extra shake at increased velocity
            shake += ShakeSpring.ComputateForces(
                Math3D::vector3(
                ( LocalRandom( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * BaseShake.jolt_scale.x,
                ( LocalRandom( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * BaseShake.jolt_scale.y,
                ( LocalRandom( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * BaseShake.jolt_scale.z )
//                * (( 200 - DynamicObject->MyTrack->iQualityFlag ) * 0.0075 ) // scale to 75-150% based on track quality
                * 1.25,
                ShakeState.offset );
        }
        shake *= 0.85;

        ShakeState.velocity -= ( shake + ShakeState.velocity * 100 ) * ( BaseShake.jolt_scale.x + BaseShake.jolt_scale.y + BaseShake.jolt_scale.z ) / ( 200 );

        // McZapkie:
        ShakeState.offset += ShakeState.velocity * Timedelta;
        if( std::abs( ShakeState.offset.y ) >  std::abs( BaseShake.jolt_limit ) ) {
            ShakeState.velocity.y = -ShakeState.velocity.y;
        }
    }
    else { // hamowanie rzucania przy spadku FPS
        ShakeState.offset -= ShakeState.offset * std::min( Timedelta, 1.0 ); // po tym chyba potrafią zostać jakieś ułamki, które powodują zjazd
    }
}

std::pair<double, double>
TDynamicObject::shake_angles() const {

    return {
        std::atan( ShakeState.velocity.x * BaseShake.angle_scale.x ),
        std::atan( ShakeState.velocity.z * BaseShake.angle_scale.z ) };
}

void
TDynamicObject::powertrain_sounds::position( glm::vec3 const Location ) {

    auto const nullvector { glm::vec3() };
    std::vector<sound_source *> enginesounds = {
        &inverter,
        &motor_relay, &dsbWejscie_na_bezoporow, &motor_parallel, &motor_shuntfield, &rsWentylator,
        &engine, &engine_ignition, &engine_shutdown, &engine_revving, &engine_turbo, &oil_pump, &fuel_pump, &water_pump, &water_heater, &radiator_fan, &radiator_fan_aux,
        &transmission, &rsEngageSlippery, &retarder
    };
    for( auto sound : enginesounds ) {
        if( sound->offset() == nullvector ) {
            sound->offset( Location );
        }
    }
}

void
TDynamicObject::powertrain_sounds::render( TMoverParameters const &Vehicle, double const Deltatime ) {

    if( Vehicle.Power == 0 ) { return; }

    double frequency { 1.0 };
    double volume { 0.0 };

    // oil pump
    if( true == Vehicle.OilPump.is_active ) {
        oil_pump
            .pitch( oil_pump.m_frequencyoffset + oil_pump.m_frequencyfactor * 1.f )
            .gain( oil_pump.m_amplitudeoffset + oil_pump.m_amplitudefactor * 1.f )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        oil_pump.stop();
    }
    // fuel pump
    if( true == Vehicle.FuelPump.is_active ) {
        fuel_pump
            .pitch( fuel_pump.m_frequencyoffset + fuel_pump.m_frequencyfactor * 1.f )
            .gain( fuel_pump.m_amplitudeoffset + fuel_pump.m_amplitudefactor * 1.f )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        fuel_pump.stop();
    }
    // water pump
    if( true == Vehicle.WaterPump.is_active ) {
        water_pump
            .pitch( water_pump.m_frequencyoffset + water_pump.m_frequencyfactor * 1.f )
            .gain( water_pump.m_amplitudeoffset + water_pump.m_amplitudefactor * 1.f )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        water_pump.stop();
    }
    // water heater
    if( true == Vehicle.WaterHeater.is_active ) {
        water_heater
            .pitch( water_heater.m_frequencyoffset + water_heater.m_frequencyfactor * 1.f )
            .gain( water_heater.m_amplitudeoffset + water_heater.m_amplitudefactor * 1.f )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        water_heater.stop();
    }
    // engine sounds
    // ignition
    if( engine_state_last != Vehicle.Mains ) {

        if( true == Vehicle.Mains ) {
           // TODO: separate engine and main circuit
           // engine activation
            engine_shutdown.stop();
            engine_ignition
                .pitch( engine_ignition.m_frequencyoffset + engine_ignition.m_frequencyfactor * 1.f )
                .gain( engine_ignition.m_amplitudeoffset + engine_ignition.m_amplitudefactor * 1.f )
                .play( sound_flags::exclusive );
            // main circuit activation
            linebreaker_close
                .pitch( linebreaker_close.m_frequencyoffset + linebreaker_close.m_frequencyfactor * 1.f )
                .gain( linebreaker_close.m_amplitudeoffset + linebreaker_close.m_amplitudefactor * 1.f )
                .play();
        }
        else {
            // engine deactivation
            engine_ignition.stop();
            engine_shutdown.pitch( engine_shutdown.m_frequencyoffset + engine_shutdown.m_frequencyfactor * 1.f )
                .gain( engine_shutdown.m_amplitudeoffset + engine_shutdown.m_amplitudefactor * 1.f )
                .play( sound_flags::exclusive );
            // main circuit deactivation
            linebreaker_open
                .pitch( linebreaker_open.m_frequencyoffset + linebreaker_open.m_frequencyfactor * 1.f )
                .gain( linebreaker_open.m_amplitudeoffset + linebreaker_open.m_amplitudefactor * 1.f )
                .play();
        }
        engine_state_last = Vehicle.Mains;
    }
    // main engine sound
    if( true == Vehicle.Mains ) {

        if( ( std::abs( Vehicle.enrot ) > 0.01 )
            // McZapkie-280503: zeby dla dumb dzialal silnik na jalowych obrotach
         || ( Vehicle.EngineType == TEngineType::Dumb ) ) {

            // frequency calculation
            auto const normalizer { (
                true == engine.is_combined() ?
                    // for combined engine sound we calculate sound point in rpm, to make .mmd files setup easier
                    // NOTE: we supply 1/100th of actual value, as the sound module converts does the opposite, converting received (typically) 0-1 values to 0-100 range
                    60.f * 0.01f :
                    // for legacy single-piece sounds the standard frequency calculation is left untouched
                    1.f ) };
            frequency =
                engine.m_frequencyoffset
                + engine.m_frequencyfactor * std::abs( Vehicle.enrot ) * normalizer;

            if( Vehicle.EngineType == TEngineType::Dumb ) {
                frequency -= 0.2 * Vehicle.EnginePower / ( 1 + Vehicle.Power * 1000 );
            }

            // base volume calculation
            switch( Vehicle.EngineType ) {
                // TODO: check calculated values
                case TEngineType::DieselElectric: {

                    volume =
                        engine.m_amplitudeoffset
                        + engine.m_amplitudefactor * (
                            0.25 * ( Vehicle.EnginePower / Vehicle.Power )
                          + 0.75 * Vehicle.EngineRPMRatio() );
                    break;
                }
                case TEngineType::DieselEngine: {
                    if( Vehicle.enrot > 0.0 ) {
                        volume = (
                            Vehicle.EnginePower > 0 ?
                                engine.m_amplitudeoffset + engine.m_amplitudefactor * Vehicle.dizel_fill :
                                engine.m_amplitudeoffset + engine.m_amplitudefactor * std::fabs( Vehicle.enrot / Vehicle.dizel_nmax ) );
                    }
                    break;
                }
                default: {
                    volume =
                        engine.m_amplitudeoffset
                        + engine.m_amplitudefactor * ( Vehicle.EnginePower + std::fabs( Vehicle.enrot ) * 60.0 );
                    break;
                }
            }

            if( engine_volume >= 0.05 ) {

                auto enginerevvolume { 0.f };
                if( ( Vehicle.EngineType == TEngineType::DieselElectric )
                 || ( Vehicle.EngineType == TEngineType::DieselEngine ) ) {
                    // diesel engine revolutions increase; it can potentially decrease volume of base engine sound
                    if( engine_revs_last != -1.f ) {
                        // calculate potential recent increase of engine revolutions
                        auto const revolutionsperminute { Vehicle.enrot * 60 };
                        auto const revolutionsdifference { revolutionsperminute - engine_revs_last };
                        auto const idlerevolutionsthreshold { 1.01 * Vehicle.EngineIdleRPM() };
                        engine_revs_change = std::max( 0.0, engine_revs_change - 2.5 * Deltatime );
                        if( ( revolutionsperminute > idlerevolutionsthreshold )
                         && ( revolutionsdifference > 1.0 * Deltatime ) ) {
                            engine_revs_change = clamp( engine_revs_change + 5.0 * Deltatime, 0.0, 1.25 );
                        }
                        enginerevvolume = 0.8 * engine_revs_change;
                    }
                    engine_revs_last = Vehicle.enrot * 60;

                    auto const enginerevfrequency { (
                        true == engine_revving.is_combined() ?
                            // if the sound contains multiple samples we reuse rpm-based calculation from the engine
                            frequency :
                            engine_revving.m_frequencyoffset + 1.f * engine_revving.m_frequencyfactor ) };

                    if( enginerevvolume > 0.02 ) {
                        engine_revving
                            .pitch( enginerevfrequency )
                            .gain( enginerevvolume )
                            .play( sound_flags::exclusive );
                    }
                    else {
                        engine_revving.stop();
                    }
                } // diesel engines

                // multi-part revving sound pieces replace base engine sound, single revving simply gets mixed with the base
                auto const enginevolume { (
                    ( ( enginerevvolume > 0.02 ) && ( true == engine_revving.is_combined() ) ) ?
                        std::max( 0.0, engine_volume - enginerevvolume ) :
                        engine_volume ) };
                engine
                    .pitch( frequency )
                    .gain( enginevolume )
                    .play( sound_flags::exclusive | sound_flags::looping );
            } // enginevolume > 0.05
        }
        else {
            engine.stop();
        }
    }

	if (Vehicle.Mains) {
		float power = std::clamp(Vehicle.eimv_pr, 0.0, 1.0);

		fake_engine
		        .pitch(fake_engine.m_frequencyoffset + power * fake_engine.m_frequencyfactor)
		        .gain(fake_engine.m_amplitudeoffset + power * fake_engine.m_amplitudefactor)
		        .play(sound_flags::exclusive | sound_flags::looping);
	}
	else {
		fake_engine.stop();
	}

    engine_volume = interpolate( engine_volume, volume, 0.25 );
    if( engine_volume < 0.05 ) {
        engine.stop();
    }

    // youBy - przenioslem, bo diesel tez moze miec turbo
    if( Vehicle.TurboTest > 0 ) {
        // udawanie turbo:
		auto const pitch_diesel { Vehicle.EngineType == TEngineType::DieselEngine ? Vehicle.enrot / Vehicle.dizel_nmax * Vehicle.dizel_fill : 1 };
        auto const goalpitch { std::max( 0.025, ( /*engine_volume **/ pitch_diesel + engine_turbo.m_frequencyoffset ) * engine_turbo.m_frequencyfactor ) };
        auto const goalvolume { (
            ( ( Vehicle.MainCtrlPos >= Vehicle.TurboTest ) && ( Vehicle.enrot > 0.1 ) ) ?
                std::max( 0.0, ( engine_turbo_pitch + engine_turbo.m_amplitudeoffset ) * engine_turbo.m_amplitudefactor ) :
                0.0 ) };
        auto const currentvolume { engine_turbo.gain() };
        auto const changerate { 0.4 * Deltatime };

        engine_turbo_pitch = (
            engine_turbo_pitch > goalpitch ?
                std::max( goalpitch, engine_turbo_pitch - changerate * 0.5 ) :
                std::min( goalpitch, engine_turbo_pitch + changerate ) );

        volume = (
            currentvolume > goalvolume ?
                std::max( goalvolume, currentvolume - changerate ) :
                std::min( goalvolume, currentvolume + changerate ) );

        engine_turbo
            .pitch( 0.4 + engine_turbo_pitch * 0.4 )
            .gain( volume );

        if( volume > 0.05 ) {
            engine_turbo.play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            engine_turbo.stop();
        }
    }

    if( Vehicle.dizel_engage > 0.1 ) {
        if( std::abs( Vehicle.dizel_engagedeltaomega ) > 0.2 ) {
            frequency = rsEngageSlippery.m_frequencyoffset + rsEngageSlippery.m_frequencyfactor * std::fabs( Vehicle.dizel_engagedeltaomega );
            volume = rsEngageSlippery.m_amplitudeoffset + rsEngageSlippery.m_amplitudefactor * ( Vehicle.dizel_engage );
        }
        else {
            frequency = 1.f; // rsEngageSlippery.FA+0.7*rsEngageSlippery.FM*(fabs(mvControlled->enrot)+mvControlled->nmax);
            volume = (
                Vehicle.dizel_engage > 0.2 ?
                    rsEngageSlippery.m_amplitudeoffset + 0.2 * rsEngageSlippery.m_amplitudefactor * ( Vehicle.enrot / Vehicle.nmax ) :
                    0.f );
        }

        rsEngageSlippery
            .pitch( frequency )
            .gain( volume )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        rsEngageSlippery.stop();
    }

	if (Vehicle.hydro_R) {
		float speed = std::abs(Vehicle.hydro_R_n);

		retarder
			.pitch(retarder.m_frequencyoffset + speed * retarder.m_frequencyfactor)
			.gain(retarder.m_amplitudeoffset + Vehicle.hydro_R_Fill * retarder.m_amplitudefactor);

		if ((retarder.gain() > 0.01)&&(speed > 1)&&(Vehicle.hydro_R_ClutchActive)) {
			retarder.play(sound_flags::exclusive | sound_flags::looping);
		}
		else {
			retarder.stop();
		}
	}

    // motor sounds
    volume = 0.0;
    // generic traction motor sounds
    if( false == motors.empty() ) {

        if( std::abs( Vehicle.nrot ) > 0.01 ) {

            auto const &motor { motors.front() };
            // frequency calculation
            auto normalizer { 1.f };
            if( true == motor.is_combined() ) {
                // for combined motor sound we calculate sound point in rpm, to make .mmd files setup easier
                // NOTE: we supply 1/100th of actual value, as the sound module converts does the opposite, converting received (typically) 0-1 values to 0-100 range
                normalizer = 60.f * 0.01f;
            }
            auto const motorrevolutions { std::abs( Vehicle.nrot ) * Vehicle.Transmision.Ratio };
            frequency =
                motor.m_frequencyoffset
                + motor.m_frequencyfactor * motorrevolutions * normalizer;

            // base volume calculation
            switch( Vehicle.EngineType ) {
                case TEngineType::ElectricInductionMotor: {
                    volume =
                        motor.m_amplitudeoffset
                        + motor.m_amplitudefactor * ( Vehicle.EnginePower + motorrevolutions * 2 );
                    break;
                }
                case TEngineType::ElectricSeriesMotor: {
                    volume =
                        motor.m_amplitudeoffset
                        + motor.m_amplitudefactor * ( Vehicle.EnginePower + motorrevolutions * 60.0 );
                    break;
                }
                default: {
                    volume =
                        motor.m_amplitudeoffset
                        + motor.m_amplitudefactor * motorrevolutions * 60.0;
                    break;
                }
            }

            if( Vehicle.EngineType == TEngineType::ElectricSeriesMotor ) {
                // volume variation
                if( ( volume < 1.0 )
                    && ( Vehicle.EnginePower < 100 ) ) {

                    auto const volumevariation { LocalRandom( 100 ) * Vehicle.enrot / ( 1 + Vehicle.nmax ) };
                    if( volumevariation < 2 ) {
                        volume += volumevariation / 200;
                    }
                }

                if( ( Vehicle.DynamicBrakeFlag )
                    && ( Vehicle.EnginePower > 0.1 ) ) {
                    // Szociu - 29012012 - jeżeli uruchomiony jest hamulec elektrodynamiczny, odtwarzany jest dźwięk silnika
                    volume += 0.8;
                }
            }
            // scale motor volume based on whether they're active
            motor_momentum =
                clamp(
                    motor_momentum
                    - 1.0 * Deltatime // smooth out decay
                    + std::abs( Vehicle.Mm ) / 60.0 * Deltatime,
                    0.0, 1.25 );
            volume *= std::max( 0.25f, motor_momentum );
            motor_volume = interpolate( motor_volume, volume, 0.25 );
            if( motor_volume >= 0.05 ) {
                // apply calculated parameters to all motor instances
                for( auto &motor : motors ) {
                    motor
                        .pitch( frequency )
                        .gain( motor_volume )
                        .play( sound_flags::exclusive | sound_flags::looping );
                }
            }
            else {
                // stop all motor instances
                for( auto &motor : motors ) {
                    motor.stop();
                }
            }
        }
        else {
            // stop all motor instances
            motor_volume = 0.0;
            for( auto &motor : motors ) {
                motor.stop();
            }
        }
    }
    // inverter-specific traction motor sounds
    if( false == acmotors.empty() ) {

        if( Vehicle.EngineType == TEngineType::ElectricInductionMotor ) {
            if( Vehicle.InverterFrequency > 0.001 ) {

                for( auto &motor : acmotors ) {
                    motor
                        .pitch( motor.m_frequencyoffset + motor.m_frequencyfactor * Vehicle.InverterFrequency )
                        .gain( motor.m_amplitudeoffset + motor.m_amplitudefactor * std::sqrt( std::abs( Vehicle.eimv_pr ) ) )
                        .play( sound_flags::exclusive | sound_flags::looping );
                }
            }
            else {
                for( auto &motor : acmotors ) {
                    motor.stop();
                }
            }
        }
    }
    // motor blowers
    if( false == motorblowers.empty() ) {
        for( auto &blowersound : motorblowers ) {
            // match the motor blower and the sound source based on whether they're located in the front or the back of the vehicle
            auto const &blower { Vehicle.MotorBlowers[ ( blowersound.offset().z > 0 ? end::front : end::rear ) ] };
            // TODO: for the sounds with provided bookends invoke stop() when the stop is triggered and revolutions start dropping, instead of after full stop
            if( blower.revolutions > 1 ) {
                // NOTE: for motorblowers with fixed speed if the sound has defined bookends we skip revolutions-based part of frequency/volume adjustments
                auto const revolutionmodifier { ( Vehicle.MotorBlowers[ end::front ].speed < 0.f ) && ( blowersound.has_bookends() ) ? 1.f : blower.revolutions };
                blowersound
                    .pitch(
                        true == blowersound.is_combined() ?
                            blower.revolutions * 0.01f :
                            blowersound.m_frequencyoffset + blowersound.m_frequencyfactor * revolutionmodifier )
                    .gain( blowersound.m_amplitudeoffset + blowersound.m_amplitudefactor * revolutionmodifier )
                    .play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                blowersound.stop();
            }
        }
    }
    // inverter sounds
    if( Vehicle.EngineType == TEngineType::ElectricInductionMotor ) {
        if( Vehicle.InverterFrequency > 0.001 ) {

            volume = inverter.m_amplitudeoffset + inverter.m_amplitudefactor * std::sqrt( std::abs( Vehicle.eimv_pr) );

            inverter
                .pitch( inverter.m_frequencyoffset + inverter.m_frequencyfactor * Vehicle.InverterFrequency )
                .gain( volume )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            inverter.stop();
        }
    }
    // ventillator sounds
    if( Vehicle.RventRot > 0.1 ) {

        rsWentylator
            .pitch( rsWentylator.m_frequencyoffset + rsWentylator.m_frequencyfactor * Vehicle.RventRot )
            .gain( rsWentylator.m_amplitudeoffset + rsWentylator.m_amplitudefactor * Vehicle.RventRot )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        // ...otherwise shut down the sound
        rsWentylator.stop();
    }
    // radiator fan sounds
    if( ( Vehicle.EngineType == TEngineType::DieselEngine )
     || ( Vehicle.EngineType == TEngineType::DieselElectric ) ) {

        if( Vehicle.dizel_heat.rpmw > 0.1 ) {
            // NOTE: fan speed tends to max out at ~100 rpm; by default we try to get pitch range of 0.5-1.5 and volume range of 0.5-1.0
            radiator_fan
                .pitch( 0.5 + radiator_fan.m_frequencyoffset + radiator_fan.m_frequencyfactor * Vehicle.dizel_heat.rpmw * 0.01 )
                .gain( 0.5 + radiator_fan.m_amplitudeoffset + 0.5 * radiator_fan.m_amplitudefactor * Vehicle.dizel_heat.rpmw * 0.01 )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            // ...otherwise shut down the sound
            radiator_fan.stop();
        }

        if( Vehicle.dizel_heat.rpmw2 > 0.1 ) {
            // NOTE: fan speed tends to max out at ~100 rpm; by default we try to get pitch range of 0.5-1.5 and volume range of 0.5-1.0
            radiator_fan_aux
                .pitch( 0.5 + radiator_fan_aux.m_frequencyoffset + radiator_fan_aux.m_frequencyfactor * Vehicle.dizel_heat.rpmw2 * 0.01 )
                .gain( 0.5 + radiator_fan_aux.m_amplitudeoffset + 0.5 * radiator_fan_aux.m_amplitudefactor * Vehicle.dizel_heat.rpmw2 * 0.01 )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            // ...otherwise shut down the sound
            radiator_fan_aux.stop();
        }
    }

    // relay sounds
    auto const soundflags { Vehicle.SoundFlag };
    if( TestFlag( soundflags, sound::relay ) ) {
        // przekaznik - gdy bezpiecznik, automatyczny rozruch itp
        if( true == TestFlag( soundflags, sound::shuntfield ) ) {
            // shunt field
            motor_shuntfield
                .pitch(
                    true == motor_shuntfield.is_combined() ?
                        Vehicle.ScndCtrlActualPos * 0.01f :
                        motor_shuntfield.m_frequencyoffset + motor_shuntfield.m_frequencyfactor * 1.f )
                .gain(
                    motor_shuntfield.m_amplitudeoffset + (
                        true == TestFlag( soundflags, sound::loud ) ?
                            1.0f :
                            0.8f )
                        * motor_shuntfield.m_amplitudefactor )
                .play();
        }
        else if( true == TestFlag( soundflags, sound::parallel ) ) {
            // parallel mode
            if( TestFlag( soundflags, sound::loud ) ) {
                dsbWejscie_na_bezoporow.play();
            }
            else {
                motor_parallel.play();
            }
        }
        else {
            // series mode
            motor_relay
                .pitch(
                    true == motor_relay.is_combined() ?
                        Vehicle.MainCtrlActualPos * 0.01f :
                        motor_relay.m_frequencyoffset + motor_relay.m_frequencyfactor * 1.f )
                .gain(
                    motor_relay.m_amplitudeoffset + (
                        true == TestFlag( soundflags, sound::loud ) ?
                            1.0f :
                            0.8f )
                        * motor_relay.m_amplitudefactor )
                .play();
        }
    }

    if( Vehicle.Vel > 0.1 ) {
        transmission
            .pitch( transmission.m_frequencyoffset + transmission.m_frequencyfactor * Vehicle.Vel )
            .gain( transmission.m_amplitudeoffset + transmission.m_amplitudefactor * Vehicle.Vel )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        transmission.stop();
    }
}


// legacy method, calculates changes in simulation state over specified time
void
vehicle_table::update( double Deltatime, int Iterationcount ) {
    // Ra: w zasadzie to trzeba by utworzyć oddzielną listę taboru do liczenia fizyki
    //    na którą by się zapisywały wszystkie pojazdy będące w ruchu
    //    pojazdy stojące nie potrzebują aktualizacji, chyba że np. ktoś im zmieni nastawę hamulca
    //    oddzielną listę można by zrobić na pojazdy z napędem, najlepiej posortowaną wg typu napędu
    for( auto *vehicle : m_items ) {
        if( false == vehicle->bEnabled ) { continue; }
        // Ra: zmienić warunek na sprawdzanie pantografów w jednej zmiennej: czy pantografy i czy podniesione
        if( vehicle->MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
            update_traction( vehicle );
        }
        vehicle->MoverParameters->ComputeConstans();
        vehicle->update_neighbours();
    }
    if( Iterationcount > 1 ) {
        // ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
        for( int iteration = 0; iteration < ( Iterationcount - 1 ); ++iteration ) {
            for( auto *vehicle : m_items ) {
                vehicle->UpdateForce( Deltatime );
            }
            for( auto *vehicle : m_items ) {
                vehicle->FastUpdate( Deltatime );
            }
        }
    }
    for( auto *vehicle : m_items ) {
        vehicle->UpdateForce( Deltatime );
    }

    auto const totaltime { Deltatime * Iterationcount }; // całkowity czas

    for( auto *vehicle : m_items ) {
        // Ra 2015-01: tylko tu przelicza sieć trakcyjną
        vehicle->Update( Deltatime, totaltime );
    }

    // jeśli jest coś do usunięcia z listy, to trzeba na końcu
    erase_disabled();
}

// legacy method, checks for presence and height of traction wire for specified vehicle
void
vehicle_table::update_traction( TDynamicObject *Vehicle ) {

    auto const vFront = glm::make_vec3( Vehicle->VectorFront().getArray() ); // wektor normalny dla płaszczyzny ruchu pantografu
    auto const vUp = glm::make_vec3( Vehicle->VectorUp().getArray() ); // wektor pionu pudła (pochylony od pionu na przechyłce)
    auto const vLeft = glm::make_vec3( Vehicle->VectorLeft().getArray() ); // wektor odległości w bok (odchylony od poziomu na przechyłce)
    auto const position = glm::dvec3 { Vehicle->GetPosition() }; // współrzędne środka pojazdu

    for( int pantographindex = 0; pantographindex < Vehicle->iAnimType[ ANIM_PANTS ]; ++pantographindex ) {
        // pętla po pantografach
        auto *pantograph { Vehicle->pants[ pantographindex ].fParamPants };
        if( true == Vehicle->MoverParameters->Pantographs[ pantographindex ].is_active ) {
            // jeśli pantograf podniesiony
            auto const pant0 { position + ( vLeft * pantograph->vPos.z ) + ( vUp * pantograph->vPos.y ) + ( vFront * pantograph->vPos.x ) };
            if( pantograph->hvPowerWire != nullptr ) {
                // jeżeli znamy drut z poprzedniego przebiegu
                for( int attempts = 0; attempts < 30; ++attempts ) {
                    // sanity check. shouldn't happen in theory, but did happen in practice
                    if( pantograph->hvPowerWire == nullptr ) { break; }

                    // powtarzane aż do znalezienia odpowiedniego odcinka na liście dwukierunkowej
                    if( pantograph->hvPowerWire->iLast & 0x3 ) {
                        // dla ostatniego i przedostatniego przęsła wymuszamy szukanie innego
                        // nie to, że nie ma, ale trzeba sprawdzić inne
                        pantograph->hvPowerWire = nullptr;
                        break;
                    }
                    if( pantograph->hvPowerWire->hvParallel ) {
                        // jeśli przęsło tworzy bieżnię wspólną, to trzeba sprawdzić pozostałe
                        // nie to, że nie ma, ale trzeba sprawdzić inne
                        pantograph->hvPowerWire = nullptr;
                        break;
                    }
                    // obliczamy wyraz wolny równania płaszczyzny (to miejsce nie jest odpowienie)
                    // podstawiamy równanie parametryczne drutu do równania płaszczyzny pantografu
                    // TODO: investigate this routine with reardriver/negative speed, does it picks the right wire?
                    auto const fRaParam =
                        -( glm::dot( pantograph->hvPowerWire->pPoint1, vFront ) - glm::dot( pant0, vFront ) )
                         / glm::dot( pantograph->hvPowerWire->vParametric, vFront );

                    if( fRaParam < -0.001 ) {
                        // histereza rzędu 7cm na 70m typowego przęsła daje 1 promil
                        pantograph->hvPowerWire = pantograph->hvPowerWire->hvNext[ 0 ];
                        continue;
                    }
                    if( fRaParam > 1.001 ) {
                        pantograph->hvPowerWire = pantograph->hvPowerWire->hvNext[ 1 ];
                        continue;
                    }
                    // jeśli t jest w przedziale, wyznaczyć odległość wzdłuż wektorów vUp i vLeft
                    // punkt styku płaszczyzny z drutem (dla generatora łuku el.)
                    auto const vStyk { pantograph->hvPowerWire->pPoint1 + fRaParam * pantograph->hvPowerWire->vParametric };
                    auto const vGdzie { vStyk - pant0 }; // wektor
                    // odległość w pionie musi być w zasięgu ruchu "pionowego" pantografu
                    // musi się mieścić w przedziale ruchu pantografu
                    auto const fVertical { glm::dot( vGdzie, vUp ) };
                    // odległość w bok powinna być mniejsza niż pół szerokości pantografu
                    // to się musi mieścić w przedziale zależnym od szerokości pantografu
                    auto const fHorizontal { std::abs( glm::dot( vGdzie, vLeft ) ) - pantograph->fWidth };
                    // jeśli w pionie albo w bok jest za daleko, to dany drut jest nieużyteczny
                    if( fHorizontal <= 0.0 ) {
                        // koniec pętli, aktualny drut pasuje
                        pantograph->PantTraction = fVertical;
                        break;
                    }
                    else {
                        // the wire is outside contact area and as of now we don't have good detection of parallel sections
                        // as such there's no guaratee there isn't parallel section present.
                        // therefore we don't bother checking if the wire is still within range of guide horns
                        // but simply force area search for potential better option
                        pantograph->hvPowerWire = nullptr;
                        break;
                    }
                }
            }

            if( pantograph->hvPowerWire == nullptr ) {
                // look in the region for a suitable traction piece if we don't already have any
                simulation::Region->update_traction( Vehicle, pantographindex );
            }

            if( ( pantograph->hvPowerWire == nullptr )
             && ( false == Global.bLiveTraction ) ) {
                // jeśli drut nie znaleziony ale można oszukiwać to dajemy coś tam dla picu
                Vehicle->pants[ pantographindex ].fParamPants->PantTraction = 1.4;
            }
        }
        else {
            // pantograph is down
            pantograph->hvPowerWire = nullptr;
        }
    }
}

// legacy method, sends list of vehicles over network
void
vehicle_table::DynamicList( bool const Onlycontrolled ) const {
    // odesłanie nazw pojazdów dostępnych na scenerii (nazwy, szczególnie wagonów, mogą się powtarzać!)
    for( auto const *vehicle : m_items ) {
        if( ( false == Onlycontrolled )
         || ( vehicle->Mechanik != nullptr ) ) {
            // same nazwy pojazdów
            multiplayer::WyslijString( vehicle->asName, 6 );
        }
    }
    // informacja o końcu listy
    multiplayer::WyslijString( "none", 6 );
}

// maintenance; removes from tracks consists with vehicles marked as disabled
bool
vehicle_table::erase_disabled() {

    if( false == TDynamicObject::bDynamicRemove ) { return false; }

    // go through the list and retrieve vehicles scheduled for removal...
    type_sequence disabledvehicles;
    for( auto *vehicle : m_items ) {
        if( false == vehicle->bEnabled ) {
            disabledvehicles.emplace_back( vehicle );
        }
    }
    // ...now propagate removal flag through affected consists...
    for( auto *vehicle : disabledvehicles ) {
        TDynamicObject *coupledvehicle { vehicle };
        while( ( coupledvehicle = coupledvehicle->Next() ) != nullptr ) {
            coupledvehicle->bEnabled = false;
        }
        // (try to) run propagation in both directions, it's simpler than branching based on direction etc
        coupledvehicle = vehicle;
        while( ( coupledvehicle = coupledvehicle->Prev() ) != nullptr ) {
            coupledvehicle->bEnabled = false;
        }
    }
    // ...then actually remove all disabled vehicles...
    auto vehicleiter = std::begin( m_items );
    while( vehicleiter != std::end( m_items ) ) {

        auto *vehicle { *vehicleiter };

        if( true == vehicle->bEnabled ) {
            ++vehicleiter;
        }
        else {
            if( ( vehicle->MyTrack != nullptr )
             && ( true == vehicle->MyTrack->RemoveDynamicObject( vehicle ) ) ) {
                vehicle->MyTrack = nullptr;
            }
            if( ( simulation::Train != nullptr )
             && ( simulation::Train->Dynamic() == vehicle ) ) {
                // clear potential train binding
                // TBD, TODO: kill vehicle sounds
				simulation::Train = nullptr;
            }
			simulation::Trains.purge(vehicle->name());
            // remove potential entries in the light array
            simulation::Lights.remove( vehicle );
/*
            // finally get rid of the vehicle and its record themselves
            // BUG: deleting the vehicle leaves dangling pointers in event->Activator and potentially elsewhere
            // TBD, TODO: either mark 'dead' vehicles with additional flag, or delete the references as well
            SafeDelete( vehicle );
            vehicleiter = m_items.erase( vehicleiter );
*/
            ++vehicleiter; // NOTE: instead of the erase in the disabled section
        }
    }
    // ...and call it a day
    TDynamicObject::bDynamicRemove = false;

    return true;
}
