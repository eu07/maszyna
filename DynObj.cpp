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
            { ".mat", ".dds", ".tga", ".png", ".bmp" } ) };

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
        if (coupler_nr == 0)
        { // jeżeli szukamy od sprzęgu 0
            if (temp->PrevConnected) // jeśli mamy coś z przodu
            {
                if (temp->PrevConnectedNo == 0) // jeśli pojazd od strony sprzęgu 0 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->PrevConnected; // ten jest od strony 0
            }
            else
                return temp; // jeśli jednak z przodu nic nie ma
        }
        else
        {
            if (temp->NextConnected)
            {
                if (temp->NextConnectedNo == 1) // jeśli pojazd od strony sprzęgu 1 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->NextConnected; // ten pojazd jest od strony 1
            }
            else
                return temp; // jeśli jednak z tyłu nic nie ma
        }
    }
    return NULL; // to tylko po wyczerpaniu pętli
};

//---------------------------------------------------------------------------
float TDynamicObject::GetEPP()
{ // szukanie skrajnego połączonego pojazdu w
    // pociagu
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
        if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag & 2) != 2)
            break; // nic nie ma już dalej podłączone
        if (coupler_nr == 0)
        { // jeżeli szukamy od sprzęgu 0
            if (temp->PrevConnected) // jeśli mamy coś z przodu
            {
                if (temp->PrevConnectedNo == 0) // jeśli pojazd od strony sprzęgu 0 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->PrevConnected; // ten jest od strony 0
            }
            else
                break; // jeśli jednak z przodu nic nie ma
        }
        else
        {
            if (temp->NextConnected)
            {
                if (temp->NextConnectedNo == 1) // jeśli pojazd od strony sprzęgu 1 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->NextConnected; // ten pojazd jest od strony 1
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
        if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag & 2) != 2)
            break; // nic nie ma już dalej podłączone
        if (coupler_nr == 0)
        { // jeżeli szukamy od sprzęgu 0
            if (temp->PrevConnected) // jeśli mamy coś z przodu
            {
                if (temp->PrevConnectedNo == 0) // jeśli pojazd od strony sprzęgu 0 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->PrevConnected; // ten jest od strony 0
            }
            else
                break; // jeśli jednak z przodu nic nie ma
        }
        else
        {
            if (temp->NextConnected)
            {
                if (temp->NextConnectedNo == 1) // jeśli pojazd od strony sprzęgu 1 jest odwrócony
                    coupler_nr = 1 - coupler_nr; // to zmieniamy kierunek sprzęgu
                temp = temp->NextConnected; // ten pojazd jest od strony 1
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

/*
TDynamicObject* TDynamicObject::GetFirstCabDynamic(int cpl_type)
{//ZiomalCl: szukanie skrajnego obiektu z kabiną
 TDynamicObject* temp=this;
 int coupler_nr=cpl_type;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zapętlenia składu
 {
  if (!temp)
   return NULL; //Ra: zabezpieczenie przed ewentaulnymi błędami sprzęgów
  if (temp->MoverParameters->CabNo!=0&&temp->MoverParameters->SandCapacity!=0)
    return temp; //nic nie ma już dalej podłączone
  if (temp->MoverParameters->Couplers[coupler_nr].CouplingFlag==0)
   return NULL;
  if (coupler_nr==0)
  {//jeżeli szukamy od sprzęgu 0
   if (temp->PrevConnectedNo==0) //jeśli pojazd od strony sprzęgu 0 jest
odwrócony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzęgu
   if (temp->PrevConnected)
    temp=temp->PrevConnected; //ten jest od strony 0
  }
  else
  {
   if (temp->NextConnectedNo==1) //jeśli pojazd od strony sprzęgu 1 jest
odwrócony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzęgu
   if (temp->NextConnected)
    temp=temp->NextConnected; //ten pojazd jest od strony 1
  }
 }
 return NULL; //to tylko po wyczerpaniu pętli
};
*/

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
    if ((x == 1) && (y == 1))
        z = 3; // dwa proste
    if ((x == 2) && (y == 0))
        z = 1; // lewy skośny, brak prawego
    if ((x == 0) && (y == 2))
        z = 2; // brak lewego, prawy skośny

    return z;
}

void TDynamicObject::SetPneumatic(bool front, bool red)
{
	int x = 0,
		ten = 0,
		tamten = 0;
    ten = GetPneumatic(front, red); // 1=lewy skos,2=prawy skos,3=dwa proste
    if (front)
        if (PrevConnected) // pojazd od strony sprzęgu 0
            tamten = PrevConnected->GetPneumatic((PrevConnectedNo == 0 ? true : false), red);
    if (!front)
        if (NextConnected) // pojazd od strony sprzęgu 1
            tamten = NextConnected->GetPneumatic((NextConnectedNo == 0 ? true : false), red);
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
            if (MoverParameters->Couplers[front ? 0 : 1].Render)
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

void TDynamicObject::UpdateDoorTranslate(TAnim *pAnim)
{ // animacja drzwi - przesuw
    if (pAnim->smAnimated) {

        if( pAnim->iNumber & 1 ) {
            pAnim->smAnimated->SetTranslate(
                Math3D::vector3{
                    0.0,
                    0.0,
                    dDoorMoveR } );
        }
        else {
            pAnim->smAnimated->SetTranslate(
                Math3D::vector3{
                    0.0,
                    0.0,
                    dDoorMoveL } );
        }
    }
};

void TDynamicObject::UpdateDoorRotate(TAnim *pAnim)
{ // animacja drzwi - obrót
    if (pAnim->smAnimated)
    {
        if (pAnim->iNumber & 1)
            pAnim->smAnimated->SetRotate(float3(1, 0, 0), dDoorMoveR);
        else
            pAnim->smAnimated->SetRotate(float3(1, 0, 0), dDoorMoveL);
    }
};

void TDynamicObject::UpdateDoorFold(TAnim *pAnim)
{ // animacja drzwi - obrót
    if (pAnim->smAnimated)
    {
        if (pAnim->iNumber & 1)
        {
            pAnim->smAnimated->SetRotate(float3(0, 0, 1), dDoorMoveR);
            TSubModel *sm = pAnim->smAnimated->ChildGet(); // skrzydło mniejsze
            if (sm)
            {
                sm->SetRotate(float3(0, 0, 1), -dDoorMoveR - dDoorMoveR); // skrzydło większe
                sm = sm->ChildGet();
                if (sm)
                    sm->SetRotate(float3(0, 1, 0), dDoorMoveR); // podnóżek?
            }
        }
        else
        {
            pAnim->smAnimated->SetRotate(float3(0, 0, 1), dDoorMoveL);
            TSubModel *sm = pAnim->smAnimated->ChildGet(); // skrzydło mniejsze
            if (sm)
            {
                sm->SetRotate(float3(0, 0, 1), -dDoorMoveL - dDoorMoveL); // skrzydło większe
                sm = sm->ChildGet();
                if (sm)
                    sm->SetRotate(float3(0, 1, 0), dDoorMoveL); // podnóżek?
            }
        }
    }
};

void TDynamicObject::UpdateDoorPlug(TAnim *pAnim)
{ // animacja drzwi - odskokprzesuw
    if (pAnim->smAnimated) {

        if( pAnim->iNumber & 1 ) {
            pAnim->smAnimated->SetTranslate(
                Math3D::vector3 {
                    std::min(
                        dDoorMoveR * 2,
                        MoverParameters->DoorMaxPlugShift ),
                    0.0,
                    std::max(
                        0.0,
                        dDoorMoveR - MoverParameters->DoorMaxPlugShift * 0.5f ) } );
        }
        else {
            pAnim->smAnimated->SetTranslate(
                Math3D::vector3 {
                    std::min(
                        dDoorMoveL * 2,
                        MoverParameters->DoorMaxPlugShift ),
                    0.0,
                    std::max(
                        0.0,
                        dDoorMoveL - MoverParameters->DoorMaxPlugShift * 0.5f ) } );
        }
    }
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

    if( pAnim->iNumber & 1 ) {
        pAnim->smAnimated->SetTranslate(
            Math3D::vector3{
                interpolate( 0.0, MoverParameters->PlatformMaxShift, dDoorstepMoveR ),
                0.0,
                0.0 } );
    }
    else {
        pAnim->smAnimated->SetTranslate(
            Math3D::vector3{
                interpolate( 0.0, MoverParameters->PlatformMaxShift, dDoorstepMoveL ),
                0.0,
                0.0 } );
    }
}

// doorstep animation, rotate
void TDynamicObject::UpdatePlatformRotate( TAnim *pAnim ) {

    if( pAnim->smAnimated == nullptr ) { return; }

    if( pAnim->iNumber & 1 )
        pAnim->smAnimated->SetRotate(
            float3( 0, 1, 0 ),
            interpolate( 0.0, MoverParameters->PlatformMaxShift, dDoorstepMoveR ) );
    else
        pAnim->smAnimated->SetRotate(
            float3( 0, 1, 0 ),
            interpolate( 0.0, MoverParameters->PlatformMaxShift, dDoorstepMoveL ) );
}

// mirror animation, rotate
void TDynamicObject::UpdateMirror( TAnim *pAnim ) {

    if( pAnim->smAnimated == nullptr ) { return; }

    // only animate the mirror if it's located on the same end of the vehicle as the active cab
    auto const isactive { (
        MoverParameters->ActiveCab > 0 ? ( ( pAnim->iNumber >> 4 ) == side::front ? 1.0 : 0.0 ) :
        MoverParameters->ActiveCab < 0 ? ( ( pAnim->iNumber >> 4 ) == side::rear  ? 1.0 : 0.0 ) :
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
            // ... but skip cab sections, their lighting ignores battery state
            auto const sectionname { section.compartment->pName };
            if( sectionname.find( "cab" ) == 0 ) { continue; }

            section.light_level = 0.0f;
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
                section.light_level = ( Random() < 0.75 ? 0.75f : 0.15f );
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
            // ABu-240105: Dodatkowy warunek: if (...).Render, zeby rysowal tylko
            // jeden
            // z polaczonych sprzegow
            if ((TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_coupler)) &&
                (MoverParameters->Couplers[0].Render))
            {
                btCoupler1.Turn( true );
                btnOn = true;
            }
            // else btCoupler1.TurnOff();
            if ((TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_coupler)) &&
                (MoverParameters->Couplers[1].Render))
            {
                btCoupler2.Turn( true );
                btnOn = true;
            }
            // else btCoupler2.TurnOff();
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

                if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_pneumatic))
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

                if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_pneumatic))
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
                if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_scndpneumatic))
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

                if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_scndpneumatic))
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
                if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_pneumatic))
                {
                    if (MoverParameters->Couplers[0].Render)
                        btCPneumatic1.TurnOn();
                    else
                        btCPneumatic1r.TurnOn();
                    btnOn = true;
                }

                if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_pneumatic))
                {
                    if (MoverParameters->Couplers[1].Render)
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
            for (int i = 0; i < 2; i++)
            {
                double dist = MoverParameters->Couplers[i].Dist / 2.0;
                if (smBuforLewy[i])
                    if (dist < 0)
                        smBuforLewy[i]->SetTranslate( Math3D::vector3(dist, 0, 0));
                if (smBuforPrawy[i])
                    if (dist < 0)
                        smBuforPrawy[i]->SetTranslate( Math3D::vector3(dist, 0, 0));
            }
        } // vehicle within 50m

        // Winger 160204 - podnoszenie pantografow

        // przewody sterowania ukrotnionego
        if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_controll))
        {
            btCCtrl1.Turn( true );
            btnOn = true;
        }
        // else btCCtrl1.TurnOff();
        if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_controll))
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
        if (MoverParameters->Battery || MoverParameters->ConverterFlag)
        { // sygnaly konca pociagu
            if (btEndSignals1.Active())
            {
                if (TestFlag(iLights[0], 2) || TestFlag(iLights[0], 32))
                {
                    btEndSignals1.Turn( true );
                    btnOn = true;
                }
                // else btEndSignals1.TurnOff();
            }
            else
            {
                if (TestFlag(iLights[0], 2))
                {
                    btEndSignals11.Turn( true );
                    btnOn = true;
                }
                // else btEndSignals11.TurnOff();
                if (TestFlag(iLights[0], 32))
                {
                    btEndSignals13.Turn( true );
                    btnOn = true;
                }
                // else btEndSignals13.TurnOff();
            }
            if (btEndSignals2.Active())
            {
                if (TestFlag(iLights[1], 2) || TestFlag(iLights[1], 32))
                {
                    btEndSignals2.Turn( true );
                    btnOn = true;
                }
                // else btEndSignals2.TurnOff();
            }
            else
            {
                if (TestFlag(iLights[1], 2))
                {
                    btEndSignals21.Turn( true );
                    btnOn = true;
                }
                // else btEndSignals21.TurnOff();
                if (TestFlag(iLights[1], 32))
                {
                    btEndSignals23.Turn( true );
                    btnOn = true;
                }
                // else btEndSignals23.TurnOff();
            }
        }
        // tablice blaszane:
        if (TestFlag(iLights[side::front], light::rearendsignals))
        {
            btEndSignalsTab1.Turn( true );
            btnOn = true;
        }
        // else btEndSignalsTab1.TurnOff();
        if (TestFlag(iLights[side::rear], light::rearendsignals))
        {
            btEndSignalsTab2.Turn( true );
            btnOn = true;
        }
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
         && ( ( Mechanik->GetAction() != TAction::actSleep )
           || ( MoverParameters->Battery ) ) ) {
            // rysowanie figurki mechanika
            btMechanik1.Turn( MoverParameters->ActiveCab > 0 );
            btMechanik2.Turn( MoverParameters->ActiveCab < 0 );
            if( MoverParameters->ActiveCab != 0 ) {
                btnOn = true;
            }
        }

    } // vehicle within 400m

    if( MoverParameters->Battery || MoverParameters->ConverterFlag )
    { // sygnały czoła pociagu //Ra: wyświetlamy bez
        // ograniczeń odległości, by były widoczne z
        // daleka
        if (TestFlag(iLights[0], 1))
        {
            btHeadSignals11.Turn( true );
            btnOn = true;
        }
        // else btHeadSignals11.TurnOff();
        if (TestFlag(iLights[0], 4))
        {
            btHeadSignals12.Turn( true );
            btnOn = true;
        }
        // else btHeadSignals12.TurnOff();
        if (TestFlag(iLights[0], 16))
        {
            btHeadSignals13.Turn( true );
            btnOn = true;
        }
        // else btHeadSignals13.TurnOff();
        if (TestFlag(iLights[1], 1))
        {
            btHeadSignals21.Turn( true );
            btnOn = true;
        }
        // else btHeadSignals21.TurnOff();
        if (TestFlag(iLights[1], 4))
        {
            btHeadSignals22.Turn( true );
            btnOn = true;
        }
        // else btHeadSignals22.TurnOff();
        if (TestFlag(iLights[1], 16))
        {
            btHeadSignals23.Turn( true );
            btnOn = true;
        }
        // else btHeadSignals23.TurnOff();
    }
    // interior light levels
    auto sectionlightcolor { glm::vec4( 1.f ) };
    for( auto const &section : Sections ) {
        /*
        sectionlightcolor = glm::vec4( InteriorLight, section.light_level );
        */
        sectionlightcolor = glm::vec4(
            ( ( ( section.light_level == 0.f ) || ( Global.fLuminance > section.compartment->fLight ) ) ?
                glm::vec3( 240.f / 255.f ) : // TBD: save and restore initial submodel diffuse instead of enforcing one?
                InteriorLight ), // TODO: per-compartment (type) light color
            section.light_level );
        section.compartment->SetLightLevel( sectionlightcolor, true );
        if( section.load != nullptr ) {
            section.load->SetLightLevel( sectionlightcolor, true );
        }
    }
    // load chunks visibility
    for( auto const &section : SectionLoadVisibility ) {
        section.submodel->iVisible = section.visible;
        if( false == section.visible ) {
            // if the section root isn't visible we can skip meddling with its children
            continue;
        }
        // if the section root is visible set the state of section chunks
        auto *sectionchunk { section.submodel->ChildGet() };
        auto visiblechunkcount { section.visible_chunks };
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

TDynamicObject * TDynamicObject::ABuFindNearestObject(TTrack *Track, TDynamicObject *MyPointer, int &CouplNr)
{
    // zwraca wskaznik do obiektu znajdujacego sie na torze (Track), którego sprzęg jest najblizszy kamerze
    // służy np. do łączenia i rozpinania sprzęgów
    // WE: Track      - tor, na ktorym odbywa sie poszukiwanie
    //    MyPointer  - wskaznik do obiektu szukajacego
    // WY: CouplNr    - który sprzęg znalezionego obiektu jest bliższy kamerze

    // Uwaga! Jesli CouplNr==-2 to szukamy njblizszego obiektu, a nie sprzegu!!!
    for( auto dynamic : Track->Dynamics ) {

        if( CouplNr == -2 ) {
            // wektor [kamera-obiekt] - poszukiwanie obiektu
            if( Math3D::LengthSquared3( Global.pCamera.Pos - dynamic->vPosition ) < 100.0 ) {
                // 10 metrów
                return dynamic;
            }
        }
        else {
            // jeśli (CouplNr) inne niz -2, szukamy sprzęgu
            if( Math3D::LengthSquared3( Global.pCamera.Pos - dynamic->vCoulpler[ 0 ] ) < 25.0 ) {
                // 5 metrów
                CouplNr = 0;
                return dynamic;
            }
            if( Math3D::LengthSquared3( Global.pCamera.Pos - dynamic->vCoulpler[ 1 ] ) < 25.0 ) {
                // 5 metrów
                CouplNr = 1;
                return dynamic;
            }
        }
    }
    // empty track or nothing found
    return nullptr;
}

TDynamicObject * TDynamicObject::ABuScanNearestObject(TTrack *Track, double ScanDir, double ScanDist, int &CouplNr)
{ // skanowanie toru w poszukiwaniu obiektu najblizszego kamerze
    if (ABuGetDirection() < 0)
        ScanDir = -ScanDir;
    TDynamicObject *FoundedObj;
    FoundedObj =
        ABuFindNearestObject(Track, this, CouplNr); // zwraca numer sprzęgu znalezionego pojazdu
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
                FoundedObj = ABuFindNearestObject(Track, this, CouplNr);
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
TDynamicObject::ABuFindObject( int &Foundcoupler, double &Distance, TTrack const *Track, int const Direction, int const Mycoupler )
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
    }
    // sprzęg po rozłączaniu (czego się nie da odpiąć
    return MoverParameters->Couplers[dir].CouplingFlag;
}

void
TDynamicObject::couple( int const Side ) {

    if( MoverParameters->Couplers[ Side ].Connected == nullptr ) { return; }

    if( MoverParameters->Couplers[ Side ].CouplingFlag == coupling::faux ) {
        // najpierw hak
        if( ( MoverParameters->Couplers[ Side ].Connected->Couplers[ Side ].AllowedFlag
            & MoverParameters->Couplers[ Side ].AllowedFlag
            & coupling::coupler ) == coupling::coupler ) {
            if( MoverParameters->Attach(
                    Side, 2,
                    MoverParameters->Couplers[ Side ].Connected,
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
        if( ( MoverParameters->Couplers[ Side ].Connected->Couplers[ Side ].AllowedFlag
            & MoverParameters->Couplers[ Side ].AllowedFlag
            & coupling::brakehose ) == coupling::brakehose ) {
            if( MoverParameters->Attach(
                    Side, 2,
                    MoverParameters->Couplers[ Side ].Connected,
                    ( MoverParameters->Couplers[ Side ].CouplingFlag | coupling::brakehose ) ) ) {
                SetPneumatic( Side != 0, true );
                if( Side == side::front ) {
                    PrevConnected->SetPneumatic( Side != 0, true );
                }
                else {
                    NextConnected->SetPneumatic( Side != 0, true );
                }
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::mainhose ) ) {
        // zasilajacy
        if( ( MoverParameters->Couplers[ Side ].Connected->Couplers[ Side ].AllowedFlag
            & MoverParameters->Couplers[ Side ].AllowedFlag
            & coupling::mainhose ) == coupling::mainhose ) {
            if( MoverParameters->Attach(
                    Side, 2,
                    MoverParameters->Couplers[ Side ].Connected,
                    ( MoverParameters->Couplers[ Side ].CouplingFlag | coupling::mainhose ) ) ) {
                SetPneumatic( Side != 0, false );
                if( Side == side::front ) {
                    PrevConnected->SetPneumatic( Side != 0, false );
                }
                else {
                    NextConnected->SetPneumatic( Side != 0, false );
                }
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::control ) ) {
        // ukrotnionko
        if( ( MoverParameters->Couplers[ Side ].Connected->Couplers[ Side ].AllowedFlag
            & MoverParameters->Couplers[ Side ].AllowedFlag
            & coupling::control ) == coupling::control ) {
            if( MoverParameters->Attach(
                    Side, 2,
                    MoverParameters->Couplers[ Side ].Connected,
                    ( MoverParameters->Couplers[ Side ].CouplingFlag | coupling::control ) ) ) {
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::gangway ) ) {
        // mostek
        if( ( MoverParameters->Couplers[ Side ].Connected->Couplers[ Side ].AllowedFlag
            & MoverParameters->Couplers[ Side ].AllowedFlag
            & coupling::gangway ) == coupling::gangway ) {
            if( MoverParameters->Attach(
                    Side, 2,
                    MoverParameters->Couplers[ Side ].Connected,
                    ( MoverParameters->Couplers[ Side ].CouplingFlag | coupling::gangway ) ) ) {
                // one coupling type per key press
                return;
            }
        }
    }
    if( false == TestFlag( MoverParameters->Couplers[ Side ].CouplingFlag, coupling::heating ) ) {
        // heating
        if( ( MoverParameters->Couplers[ Side ].Connected->Couplers[ Side ].AllowedFlag
            & MoverParameters->Couplers[ Side ].AllowedFlag
            & coupling::heating ) == coupling::heating ) {
            if( MoverParameters->Attach(
                    Side, 2,
                    MoverParameters->Couplers[ Side ].Connected,
                    ( MoverParameters->Couplers[ Side ].CouplingFlag | coupling::heating ) ) ) {
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

void TDynamicObject::CouplersDettach(double MinDist, int MyScanDir) {
    // funkcja rozłączajaca podłączone sprzęgi, jeśli odległość przekracza (MinDist)
    // MinDist - dystans minimalny, dla ktorego mozna rozłączać
    if (MyScanDir > 0) {
        // pojazd od strony sprzęgu 0
        if( ( PrevConnected != nullptr )
         && ( MoverParameters->Couplers[ side::front ].CoupleDist > MinDist ) ) {
            // sprzęgi wirtualne zawsze przekraczają
            if( ( PrevConnectedNo == side::front ?
                    PrevConnected->PrevConnected :
                    PrevConnected->NextConnected )
                == this ) {
                // Ra: nie rozłączamy znalezionego, jeżeli nie do nas podłączony
                // (może jechać w innym kierunku)
                PrevConnected->MoverParameters->Couplers[PrevConnectedNo].Connected = nullptr;
                if( PrevConnectedNo == side::front ) {
                    // sprzęg 0 nie podłączony
                    PrevConnected->PrevConnectedNo = 2;
                    PrevConnected->PrevConnected = nullptr;
                }
                else if( PrevConnectedNo == side::rear ) {
                    // sprzęg 1 nie podłączony
                    PrevConnected->NextConnectedNo = 2;
                    PrevConnected->NextConnected = nullptr;
                }
            }
            // za to zawsze odłączamy siebie
            PrevConnected = nullptr;
            PrevConnectedNo = 2; // sprzęg 0 nie podłączony
            MoverParameters->Couplers[ side::front ].Connected = nullptr;
        }
    }
    else {
        // pojazd od strony sprzęgu 1
        if( ( NextConnected != nullptr )
         && ( MoverParameters->Couplers[ side::rear ].CoupleDist > MinDist ) ) {
            // sprzęgi wirtualne zawsze przekraczają
            if( ( NextConnectedNo == side::front ?
                    NextConnected->PrevConnected :
                    NextConnected->NextConnected )
                == this) {
                // Ra: nie rozłączamy znalezionego, jeżeli nie do nas podłączony
                // (może jechać w innym kierunku)
                NextConnected->MoverParameters->Couplers[ NextConnectedNo ].Connected = nullptr;
                if( NextConnectedNo == side::front ) {
                    // sprzęg 0 nie podłączony
                    NextConnected->PrevConnectedNo = 2;
                    NextConnected->PrevConnected = nullptr;
                }
                else if( NextConnectedNo == side::rear ) {
                    // sprzęg 1 nie podłączony
                    NextConnected->NextConnectedNo = 2;
                    NextConnected->NextConnected = nullptr;
                }
            }
            // za to zawsze odłączamy siebie
            NextConnected = nullptr;
            NextConnectedNo = 2; // sprzęg 1 nie podłączony
            MoverParameters->Couplers[1].Connected = nullptr;
        }
    }
}

void TDynamicObject::ABuScanObjects( int Direction, double Distance )
{ // skanowanie toru w poszukiwaniu kolidujących pojazdów
    // ScanDir - określa kierunek poszukiwania zależnie od zwrotu prędkości
    // pojazdu
    // ScanDir=1 - od strony Coupler0, ScanDir=-1 - od strony Coupler1
    auto const initialdirection = Direction; // zapamiętanie kierunku poszukiwań na torze początkowym, względem sprzęgów

    TTrack const *track = RaTrackGet();
    if( RaDirectionGet() < 0 ) {
        // czy oś jest ustawiona w stronę Point1?
        Direction = -Direction;
    }

    // (teraz względem toru)
    int const mycoupler = ( initialdirection < 0 ? 1 : 0 ); // numer sprzęgu do podłączenia w obiekcie szukajacym
    int foundcoupler { -1 }; // numer sprzęgu w znalezionym obiekcie (znaleziony wypełni)
    double distance = 0; // przeskanowana odleglość; odległość do zawalidrogi
    TDynamicObject *foundobject = ABuFindObject( foundcoupler, distance, track, Direction, mycoupler ); // zaczynamy szukać na tym samym torze

    if( foundobject == nullptr ) {
        // jeśli nie ma na tym samym, szukamy po okolicy szukanie najblizszego toru z jakims obiektem
        // praktycznie przeklejone z TraceRoute()...
        if (Direction >= 0) // uwzględniamy kawalek przeanalizowanego wcześniej toru
            distance = track->Length() - RaTranslationGet(); // odległość osi od Point2 toru
        else
            distance = RaTranslationGet(); // odległość osi od Point1 toru

        while (distance < Distance) {
            if (Direction > 0) {
                // w kierunku Point2 toru
                if( track ?
                        track->iNextDirection :
                        false ) {
                    // jeśli następny tor jest podpięty od Point2
                    Direction = -Direction; // to zmieniamy kierunek szukania na tym torze
                }
                track = track->CurrentNext(); // potem dopiero zmieniamy wskaźnik
            }
            else {
                // w kierunku Point1
                if( track ?
                        !track->iPrevDirection :
                        true ) {
                    // jeśli poprzedni tor nie jest podpięty od Point2
                    Direction = -Direction; // to zmieniamy kierunek szukania na tym torze
                }
                track = track->CurrentPrev(); // potem dopiero zmieniamy wskaźnik
            }
            if (track) {
                // jesli jest kolejny odcinek toru
                foundobject = ABuFindObject(foundcoupler, distance, track, Direction, mycoupler); // przejrzenie pojazdów tego toru
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

    // teraz odczepianie i jeśli coś się znalazło, doczepianie.
    auto connectedobject = (
        initialdirection > 0 ?
            PrevConnected :
            NextConnected );
    if( ( connectedobject != nullptr )
     && ( connectedobject != foundobject ) ) {
        // odłączamy, jeśli dalej niż metr i łączenie sprzęgiem wirtualnym
        CouplersDettach( 1.0, initialdirection );
    }

    if (foundobject) {
        // siebie można bezpiecznie podłączyć jednostronnie do znalezionego
        MoverParameters->Attach( mycoupler, foundcoupler, foundobject->MoverParameters, coupling::faux );
        // MoverParameters->Couplers[MyCouplFound].Render=false; //wirtualnego nie renderujemy
        if( mycoupler == side::front ) {
            PrevConnected = foundobject; // pojazd od strony sprzęgu 0
            PrevConnectedNo = foundcoupler;
        }
        else {
            NextConnected = foundobject; // pojazd od strony sprzęgu 1
            NextConnectedNo = foundcoupler;
        }

        if( foundobject->MoverParameters->Couplers[ foundcoupler ].CouplingFlag == coupling::faux ) {
            // Ra: wpinamy się wirtualnym tylko jeśli znaleziony ma wirtualny sprzęg
            if( ( foundcoupler == side::front ?
                    foundobject->PrevConnected :
                    foundobject->NextConnected )
                != this ) {
                // but first break existing connection of the target,
                // otherwise we risk leaving the target's connected vehicle with active one-side connection
                foundobject->CouplersDettach(
                    1.0,
                    ( foundcoupler == side::front ?
                         1 :
                        -1 ) );
            }
            foundobject->MoverParameters->Attach( foundcoupler, mycoupler, this->MoverParameters, coupling::faux );

            if( foundcoupler == side::front ) {
                // jeśli widoczny sprzęg 0 znalezionego
                if( ( DebugModeFlag )
                 && ( foundobject->PrevConnected )
                 && ( foundobject->PrevConnected != this ) ) {
                    WriteLog( "ScanObjects(): formed virtual link between \"" + asName + "\" (coupler " + to_string( mycoupler ) + ") and \"" + foundobject->asName + "\" (coupler " + to_string( foundcoupler ) + ")" );
                }
                foundobject->PrevConnected = this;
                foundobject->PrevConnectedNo = mycoupler;
            }
            else {
                // jeśli widoczny sprzęg 1 znalezionego
                if( ( DebugModeFlag )
                 && ( foundobject->NextConnected )
                 && ( foundobject->NextConnected != this ) ) {
                    WriteLog( "ScanObjects(): formed virtual link between \"" + asName + "\" (coupler " + to_string( mycoupler ) + ") and \"" + foundobject->asName + "\" (coupler " + to_string( foundcoupler ) + ")" );
                }
                foundobject->NextConnected = this;
                foundobject->NextConnectedNo = mycoupler;
            }
        }

        // NOTE: the distance we get is approximated as it's measured between active axles, not vehicle ends
        fTrackBlock = distance;
        if( distance < 100.0 ) {
            // at short distances start to calculate range between couplers directly
            // odległość do najbliższego pojazdu w linii prostej
            fTrackBlock = MoverParameters->Couplers[ mycoupler ].CoupleDist;
        }
        if( ( false == TestFlag( track->iCategoryFlag, 1 ) )
         && ( distance > 50.0 ) ) {
            // Ra: jeśli dwa samochody się mijają na odcinku przed zawrotką, to odległość między nimi nie może być liczona w linii prostej!
            // NOTE: the distance is approximated, and additionally less accurate for cars heading in opposite direction
            fTrackBlock = distance - ( 0.5 * ( MoverParameters->Dim.L + foundobject->MoverParameters->Dim.L ) );
        }
    }
    else {
        // nic nie znalezione, to nie ma przeszkód
        fTrackBlock = 10000.0;
    }

#ifdef EU07_DEBUG_COLLISIONS
    // debug collider scans
    if( ( PrevConnected != nullptr )
     && ( PrevConnected == NextConnected ) ) {
        ErrorLog( "Bad coupling: " + asName + " has the same vehicle detected attached to both couplers" );
    }
#endif
}
//----------ABu: koniec skanowania pojazdow

TDynamicObject::TDynamicObject() {
    modelShake = Math3D::vector3(0, 0, 0);
    fTrackBlock = 10000.0; // brak przeszkody na drodze
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
    NextConnected = PrevConnected = NULL;
    NextConnectedNo = PrevConnectedNo = 2; // ABu: Numery sprzegow. 2=nie podłączony
    bEnabled = true;
    MyTrack = NULL;
    // McZapkie-260202
    dWheelAngle[0] = 0.0;
    dWheelAngle[1] = 0.0;
    dWheelAngle[2] = 0.0;
    // Winger 160204 - pantografy
    // PantVolume = 3.5;
    NoVoltTime = 0;
    dDoorMoveL = 0.0;
    dDoorMoveR = 0.0;
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
        DriverType = "p"; // to do przemyślenia
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
    iLights = MoverParameters->iLights; // wskaźnik na stan własnych świateł
    // McZapkie: TypeName musi byc nazwą CHK/MMD pojazdu
    if (!MoverParameters->LoadFIZ(asBaseDir))
    { // jak wczytanie CHK się nie uda, to błąd
        if (ConversionError == 666)
            ErrorLog( "Bad vehicle: failed do locate definition file \"" + BaseDir + "/" + Type_Name + ".fiz" + "\"" );
        else {
            ErrorLog( "Bad vehicle: failed to load definition from file \"" + BaseDir + "/" + Type_Name + ".fiz\" (error " + to_string( ConversionError ) + ")" );
        }
        return 0.0; // zerowa długość to brak pojazdu
    }
    // load the cargo now that we know whether the vehicle will allow it
    MoverParameters->AssignLoad( LoadType, Load );

    bool driveractive = (fVel != 0.0); // jeśli prędkość niezerowa, to aktywujemy ruch
    if (!MoverParameters->CheckLocomotiveParameters(
            driveractive,
            (fVel > 0 ? 1 : -1) * Cab *
                (iDirection ? 1 : -1))) // jak jedzie lub obsadzony to gotowy do drogi
    {
        Error("Parameters mismatch: dynamic object " + asName + " from \"" + BaseDir + "/" + Type_Name + "\"" );
        return 0.0; // zerowa długość to brak pojazdu
    }
    // ustawienie pozycji hamulca
    MoverParameters->LocalBrakePosA = 0.0;
    if (driveractive)
    {
        if (Cab == 0)
            MoverParameters->BrakeCtrlPos =
                static_cast<int>( std::floor(MoverParameters->Handle->GetPos(bh_NP)) );
        else
            MoverParameters->BrakeCtrlPos = static_cast<int>( std::floor(MoverParameters->Handle->GetPos(bh_RP)) );
    }
    else
        MoverParameters->BrakeCtrlPos =
            static_cast<int>( std::floor(MoverParameters->Handle->GetPos(bh_NP)) );

    MoverParameters->BrakeLevelSet(
        MoverParameters->BrakeCtrlPos); // poprawienie hamulca po ewentualnym
    // przestawieniu przez Pascal

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
            if (ActPar.find('G') != std::string::npos)
            {
                MoverParameters->BrakeDelaySwitch(bdelay_G);
            }
			if( ActPar.find( 'P' ) != std::string::npos )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_P);
            }
			if( ActPar.find( 'R' ) != std::string::npos )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_R);
            }
			if( ActPar.find( 'M' ) != std::string::npos )
            {
                MoverParameters->BrakeDelaySwitch(bdelay_R);
                MoverParameters->BrakeDelaySwitch(bdelay_R + bdelay_M);
            }
            // wylaczanie hamulca
            if (ActPar.find("<>") != std::string::npos) // wylaczanie na probe hamowania naglego
            {
                MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
            }
            if (ActPar.find('0') != std::string::npos) // wylaczanie na sztywno
            {
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
            }
            if (ActPar.find('E') != std::string::npos) // oprozniony
            {
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->Pipe->CreatePress(0);
                MoverParameters->Pipe2->CreatePress(0);
            }
            if (ActPar.find('Q') != std::string::npos) // oprozniony
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

            if (ActPar.find('1') != std::string::npos) // wylaczanie 10%
            {
                if (Random(10) < 1) // losowanie 1/10
                {
                    MoverParameters->Hamulec->ForceEmptiness();
                    MoverParameters->Hamulec->SetBrakeStatus( MoverParameters->Hamulec->GetBrakeStatus() | b_dmg ); // wylacz
                }
            }
            if (ActPar.find('X') != std::string::npos) // agonalny wylaczanie 20%, usrednienie przekladni
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
            if (ActPar.find('T') != std::string::npos) // prozny
            {
                MoverParameters->DecBrakeMult();
                MoverParameters->DecBrakeMult();
            } // dwa razy w dol
            if (ActPar.find('H') != std::string::npos) // ladowny I (dla P-Ł dalej prozny)
            {
                MoverParameters->IncBrakeMult();
                MoverParameters->IncBrakeMult();
                MoverParameters->DecBrakeMult();
            } // dwa razy w gore i obniz
            if (ActPar.find('F') != std::string::npos) // ladowny II
            {
                MoverParameters->IncBrakeMult();
                MoverParameters->IncBrakeMult();
            } // dwa razy w gore
            if (ActPar.find('N') != std::string::npos) // parametr neutralny
            {
            }
        } // koniec hamulce
        else if( ( ActPar.size() >= 3 )
              && ( ActPar[ 0 ] == 'W' ) ) {
            // wheel
            ActPar.erase( 0, 1 );

            auto fixedflatsize { 0 };
            auto randomflatsize { 0 };
            auto flatchance { 100 };

            while( false == ActPar.empty() ) {
                // TODO: convert this whole copy and paste mess to something more elegant one day
                switch( ActPar[ 0 ] ) {
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
              && ( ActPar[ 0 ] == 'T' ) ) {
            // temperature
            ActPar.erase( 0, 1 );

            auto setambient { false };

            while( false == ActPar.empty() ) {
                switch( ActPar[ 0 ] ) {
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
    btCoupler1.Init( "coupler1", mdModel, false ); // false - ma być wyłączony
    btCoupler2.Init( "coupler2", mdModel, false);
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
    // ABu 060205: Zmiany dla koncowek swiecacych:
    btEndSignals11.Init( "endsignal13", mdModel, false);
    btEndSignals21.Init( "endsignal23", mdModel, false);
    btEndSignals13.Init( "endsignal12", mdModel, false);
    btEndSignals23.Init( "endsignal22", mdModel, false);
    iInventory[ side::front ] |= btEndSignals11.Active() ? light::redmarker_left : 0; // informacja, czy ma poszczególne światła
    iInventory[ side::front ] |= btEndSignals13.Active() ? light::redmarker_right : 0;
    iInventory[ side::rear ] |= btEndSignals21.Active() ? light::redmarker_left : 0;
    iInventory[ side::rear ] |= btEndSignals23.Active() ? light::redmarker_right : 0;
    // ABu: to niestety zostawione dla kompatybilnosci modeli:
    btEndSignals1.Init( "endsignals1", mdModel, false);
    btEndSignals2.Init( "endsignals2", mdModel, false);
    btEndSignalsTab1.Init( "endtab1", mdModel, false);
    btEndSignalsTab2.Init( "endtab2", mdModel, false);
    iInventory[ side::front ] |= btEndSignals1.Active() ? ( light::redmarker_left | light::redmarker_right ) : 0;
    iInventory[ side::front ] |= btEndSignalsTab1.Active() ? light::rearendsignals : 0; // tabliczki blaszane
    iInventory[ side::rear ] |= btEndSignals2.Active() ? ( light::redmarker_left | light::redmarker_right ) : 0;
    iInventory[ side::rear ] |= btEndSignalsTab2.Active() ? light::rearendsignals : 0;
    // ABu Uwaga! tu zmienic w modelu!
    btHeadSignals11.Init( "headlamp13", mdModel, false); // lewe
    btHeadSignals12.Init( "headlamp11", mdModel, false); // górne
    btHeadSignals13.Init( "headlamp12", mdModel, false); // prawe
    btHeadSignals21.Init( "headlamp23", mdModel, false);
    btHeadSignals22.Init( "headlamp21", mdModel, false);
    btHeadSignals23.Init( "headlamp22", mdModel, false);
    iInventory[ side::front ] |= btHeadSignals11.Active() ? light::headlight_left : 0;
    iInventory[ side::front ] |= btHeadSignals12.Active() ? light::headlight_upper : 0;
    iInventory[ side::front ] |= btHeadSignals13.Active() ? light::headlight_right : 0;
    iInventory[ side::rear ] |= btHeadSignals21.Active() ? light::headlight_left : 0;
    iInventory[ side::rear ] |= btHeadSignals22.Active() ? light::headlight_upper : 0;
    iInventory[ side::rear ] |= btHeadSignals23.Active() ? light::headlight_right : 0;
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
            Sections.push_back( { submodel, nullptr, 0.0f } );
            LowPolyIntCabs[ 0 ] = submodel;
        }
        if( ( submodel = mdLowPolyInt->GetFromName( "cab1" ) ) != nullptr ) {
            Sections.push_back( { submodel, nullptr, 0.0f } );
            LowPolyIntCabs[ 1 ] = submodel;
        }
        if( ( submodel = mdLowPolyInt->GetFromName( "cab2" ) ) != nullptr ) {
            Sections.push_back( { submodel, nullptr, 0.0f } );
            LowPolyIntCabs[ 2 ] = submodel;
        }
        // passenger car compartments
        std::vector<std::string> nameprefixes = { "corridor", "korytarz", "compartment", "przedzial" };
        for( auto const &nameprefix : nameprefixes ) {
            init_sections( mdLowPolyInt, nameprefix );
        }
    }
    // 'external_load' is an optional special section in the main model, pointing to submodel of external load
    if( mdModel ) {
        init_sections( mdModel, "external_load" );
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
    for( auto &axle : m_axlesounds ) {
        // wyszukiwanie osi (0 jest na końcu, dlatego dodajemy długość?)
        axle.distance = (
            Reversed ?
                 -axle.offset :
                ( axle.offset + MoverParameters->Dim.L ) ) + fDist;
    }
    // McZapkie-250202 end.
    Track->AddDynamicObject(this); // wstawiamy do toru na pozycję 0, a potem przesuniemy
    // McZapkie: zmieniono na ilosc osi brane z chk
    // iNumAxles=(MoverParameters->NAxles>3 ? 4 : 2 );
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
TDynamicObject::init_sections( TModel3d const *Model, std::string const &Nameprefix ) {

    std::string sectionname;
    auto sectioncount = 0;
    auto sectionindex = 0;
    TSubModel *sectionsubmodel { nullptr };

    do {
        sectionname =
            Nameprefix + (
            sectionindex < 10 ?
                "0" + std::to_string( sectionindex ) :
                      std::to_string( sectionindex ) );
        sectionsubmodel = Model->GetFromName( sectionname );
        if( sectionsubmodel != nullptr ) {
            Sections.push_back( {
                sectionsubmodel,
                nullptr, // pointers to load sections are generated afterwards
                0.0f } );
            ++sectioncount;
        }
        ++sectionindex;
    } while( ( sectionsubmodel != nullptr )
          || ( sectionindex < 2 ) ); // chain can start from prefix00 or prefix01

    return sectioncount;
}

void
TDynamicObject::create_controller( std::string const Type, bool const Trainset ) {

    if( Type == "" ) { return; }

    if( asName == Global.asHumanCtrlVehicle ) {
        // jeśli pojazd wybrany do prowadzenia
        if( MoverParameters->EngineType != TEngineType::Dumb ) {
            // wsadzamy tam sterującego
            Controller = Humandriver;
        }
        else {
            // w przeciwnym razie trzeba włączyć pokazywanie kabiny
            bDisplayCab = true;
        }
    }
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
        // if (fabs(fAdjustment)>0.02) //jeśli jest zbyt dużo, to rozłożyć na kilka
        // przeliczeń
        // (wygasza drgania?)
        //{//parę centymetrów trzeba by już skorygować; te błędy mogą się też
        // generować na ostrych
        //łukach
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
        // Ra: skopiowanie pozycji do fizyki, tam potrzebna do zrywania sprzęgów
        // MoverParameters->Loc.X=-vPosition.x; //robi to {Fast}ComputeMovement()
        // MoverParameters->Loc.Y= vPosition.z;
        // MoverParameters->Loc.Z= vPosition.y;
        // obliczanie pozycji sprzęgów do liczenia zderzeń
        auto dir = (0.5 * MoverParameters->Dim.L) * vFront; // wektor sprzęgu
        vCoulpler[side::front] = vPosition + dir; // współrzędne sprzęgu na początku
        vCoulpler[side::rear] = vPosition - dir; // współrzędne sprzęgu na końcu
        MoverParameters->vCoulpler[side::front] = vCoulpler[side::front]; // tymczasowo kopiowane na inny poziom
        MoverParameters->vCoulpler[side::rear]  = vCoulpler[side::rear];
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

void TDynamicObject::AttachPrev(TDynamicObject *Object, int iType)
{ // Ra: doczepia Object na końcu
    // składu (nazwa funkcji może być
    // myląca)
    // Ra: używane tylko przy wczytywaniu scenerii
    /*
    //Ra: po wstawieniu pojazdu do scenerii nie miał on ustawionej pozycji, teraz
    już ma
    TLocation loc;
    loc.X=-vPosition.x;
    loc.Y=vPosition.z;
    loc.Z=vPosition.y;
    MoverParameters->Loc=loc; //Ra: do obliczania sprzęgów, na starcie nie są
    przesunięte
    loc.X=-Object->vPosition.x;
    loc.Y=Object->vPosition.z;
    loc.Z=Object->vPosition.y;
    Object->MoverParameters->Loc=loc; //ustawienie dodawanego pojazdu
    */
    MoverParameters->Attach(iDirection, Object->iDirection ^ 1, Object->MoverParameters, iType, true, false);
    MoverParameters->Couplers[iDirection].Render = false;
    Object->MoverParameters->Attach(Object->iDirection ^ 1, iDirection, MoverParameters, iType, true, false);
    Object->MoverParameters->Couplers[Object->iDirection ^ 1].Render = true; // rysowanie sprzęgu w dołączanym
    if (iDirection)
    { //łączenie standardowe
        NextConnected = Object; // normalnie doczepiamy go sobie do sprzęgu 1
        NextConnectedNo = Object->iDirection ^ 1;
    }
    else
    { //łączenie odwrotne
        PrevConnected = Object; // doczepiamy go sobie do sprzęgu 0, gdy stoimy odwrotnie
        PrevConnectedNo = Object->iDirection ^ 1;
    }
    if (Object->iDirection)
    { // dołączany jest normalnie ustawiany
        Object->PrevConnected = this; // on ma nas z przodu
        Object->PrevConnectedNo = iDirection;
    }
    else
    { // dołączany jest odwrotnie ustawiany
        Object->NextConnected = this; // on ma nas z tyłu
        Object->NextConnectedNo = iDirection;
    }
/*
    // NOTE: this appears unnecessary and only messes things for the programmable lights function, which walks along
    // whole trainset and expects each module to point to its own lights. Disabled, TBD, TODO: test for side-effects and delete if there's none
    if (MoverParameters->TrainType & dt_EZT) // w przypadku łączenia członów,
        // światła w rozrządczym zależą od stanu w silnikowym
        if (MoverParameters->Couplers[iDirection].AllowedFlag & ctrain_depot) // gdy sprzęgi łączone warsztatowo (powiedzmy)
            if ((MoverParameters->Power < 1.0) && (Object->MoverParameters->Power > 1.0)) // my nie mamy mocy, ale ten drugi ma
                iLights = Object->MoverParameters->iLights; // to w tym z mocą będą światła załączane, a w tym bez tylko widoczne
            else if ((MoverParameters->Power > 1.0) &&
                     (Object->MoverParameters->Power < 1.0)) // my mamy moc, ale ten drugi nie ma
                Object->iLights = MoverParameters->iLights; // to w tym z mocą będą światła załączane, a w tym bez tylko widoczne
*/
    return;
    // SetPneumatic(1,1); //Ra: to i tak się nie wykonywało po return
    // SetPneumatic(1,0);
    // SetPneumatic(0,1);
    // SetPneumatic(0,0);
}

bool TDynamicObject::UpdateForce(double dt, double dt1, bool FullVer)
{
    if (!bEnabled)
        return false;
    if( dt > 0 ) {
        // wywalenie WS zależy od ustawienia kierunku
        MoverParameters->ComputeTotalForce( dt, dt1, FullVer );
    }
    return true;
}

// initiates load change by specified amounts, with a platform on specified side
void TDynamicObject::LoadExchange( int const Disembark, int const Embark, int const Platform ) {

    if( ( MoverParameters->DoorOpenCtrl == control_t::passenger )
     || ( MoverParameters->DoorOpenCtrl == control_t::mixed ) ) {
        // jeśli jedzie do tyłu, to drzwi otwiera odwrotnie
        auto const lewe = ( DirectionGet() > 0 ) ? 1 : 2;
        auto const prawe = 3 - lewe;
        if( Platform & lewe ) {
            MoverParameters->DoorLeft( true, range_t::local );
        }
        if( Platform & prawe ) {
            MoverParameters->DoorRight( true, range_t::local );
        }
    }
    m_exchange.unload_count += Disembark;
    m_exchange.load_count += Embark;
    m_exchange.platforms = Platform;
    m_exchange.time = 0.0;
}

// calculates time needed to complete current load change
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
        exchangespeedfactor += ( MoverParameters->DoorLeftOpened ? 1.f : 0.f );
    }
    if( m_exchange.platforms & prawe ) {
        exchangespeedfactor += ( MoverParameters->DoorRightOpened ? 1.f : 0.f );
    }

    return exchangespeedfactor;
}

// update state of load exchange operation
void TDynamicObject::update_exchange( double const Deltatime ) {

    // TODO: move offset calculation after initial check, when the loading system is all unified
    update_load_offset();

    if( ( m_exchange.unload_count < 0.01 ) && ( m_exchange.load_count < 0.01 ) ) { return; }

    if( ( MoverParameters->Vel < 2.0 )
     && ( ( true == MoverParameters->DoorLeftOpened )
       || ( true == MoverParameters->DoorRightOpened ) ) ) {

        m_exchange.time += Deltatime;
        while( ( m_exchange.unload_count > 0.01 )
            && ( m_exchange.time >= 1.0 ) ) {
            
            m_exchange.time -= 1.0;
            auto const exchangesize = std::min( m_exchange.unload_count, MoverParameters->UnLoadSpeed * LoadExchangeSpeed() );
            m_exchange.unload_count -= exchangesize;
            MoverParameters->LoadStatus = 1;
            MoverParameters->LoadAmount = std::max( 0.f, MoverParameters->LoadAmount - exchangesize );
            update_load_visibility();
        }
        if( m_exchange.unload_count < 0.01 ) {
            // finish any potential unloading operation before adding new load
            // don't load more than can fit
            m_exchange.load_count = std::min( m_exchange.load_count, MoverParameters->MaxLoad - MoverParameters->LoadAmount );
            while( ( m_exchange.load_count > 0.01 )
                && ( m_exchange.time >= 1.0 ) ) {

                m_exchange.time -= 1.0;
                auto const exchangesize = std::min( m_exchange.load_count, MoverParameters->LoadSpeed * LoadExchangeSpeed() );
                m_exchange.load_count -= exchangesize;
                MoverParameters->LoadStatus = 2;
                MoverParameters->LoadAmount = std::min( MoverParameters->MaxLoad, MoverParameters->LoadAmount + exchangesize ); // std::max not strictly needed but, eh
                update_load_visibility();
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

        MoverParameters->LoadStatus = 0;
        // if the exchange is completed (or canceled) close the door, if applicable
        if( ( MoverParameters->DoorCloseCtrl == control_t::passenger )
         || ( MoverParameters->DoorCloseCtrl == control_t::mixed ) ) {

            if( ( MoverParameters->Vel > 2.0 )
             || ( Random() < (
                    // remotely controlled door are more likely to be left open
                    MoverParameters->DoorCloseCtrl == control_t::passenger ?
                        0.75 :
                        0.50 ) ) ) {

                MoverParameters->DoorLeft( false, range_t::local );
                MoverParameters->DoorRight( false, range_t::local );
            }
        }
    }
}

void TDynamicObject::LoadUpdate() {
    // przeładowanie modelu ładunku
    // Ra: nie próbujemy wczytywać modeli miliony razy podczas renderowania!!!
    if( ( mdLoad == nullptr )
     && ( MoverParameters->LoadAmount > 0 ) ) {

        if( false == MoverParameters->LoadType.name.empty() ) {
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
        // Ra: w MMD można by zapisać położenie modelu ładunku (np. węgiel) w zależności od załadowania
    }
    else if( MoverParameters->LoadAmount == 0 ) {
        // nie ma ładunku
//        MoverParameters->AssignLoad( "" );
        mdLoad = nullptr;
        // erase bindings between lowpoly sections and potential load chunks placed inside them
        update_load_sections();
    }
    MoverParameters->LoadStatus &= 3; // po zakończeniu będzie równe zero
}

void
TDynamicObject::update_load_sections() {

    SectionLoadVisibility.clear();

    for( auto &section : Sections ) {

        section.load = (
            mdLoad != nullptr ?
                mdLoad->GetFromName( section.compartment->pName ) :
                nullptr );

        if( ( section.load != nullptr )
         && ( section.load->count_children() > 0 ) ) {
            SectionLoadVisibility.push_back( { section.load, false } );
        }
    }
    shuffle_load_sections();
}

void
TDynamicObject::update_load_visibility() {
/*
    if( Random() < 0.25 ) {
        shuffle_load_sections();
    }
*/
    auto loadpercentage { (
        MoverParameters->MaxLoad == 0.f ?
            0.0 :
            100.0 * MoverParameters->LoadAmount / MoverParameters->MaxLoad ) };
    auto const sectionloadpercentage { (
        SectionLoadVisibility.empty() ?
            0.0 :
            100.0 / SectionLoadVisibility.size() ) };
    // set as many sections as we can, given overall load percentage and how much of full percentage is covered by each chunk
    std::for_each(
        std::begin( SectionLoadVisibility ), std::end( SectionLoadVisibility ),
        [&]( section_visibility &section ) {
            section.visible = ( loadpercentage > 0.0 );
            section.visible_chunks = 0;
            auto const sectionchunkcount { section.submodel->count_children() };
            auto const sectionchunkloadpercentage{ (
                sectionchunkcount == 0 ?
                    0.0 :
                    sectionloadpercentage / sectionchunkcount ) };
            auto *sectionchunk { section.submodel->ChildGet() };
            while( sectionchunk != nullptr ) {
                if( loadpercentage > 0.0 ) {
                    ++section.visible_chunks;
                    loadpercentage -= sectionchunkloadpercentage;
                }
                sectionchunk = sectionchunk->NextGet();
            } } );
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
TDynamicObject::shuffle_load_sections() {

    std::shuffle( std::begin( SectionLoadVisibility ), std::end( SectionLoadVisibility ), Global.random_engine );
    // shift chunks assigned to corridors to the end of the list, so they show up last
    std::stable_partition(
        std::begin( SectionLoadVisibility ), std::end( SectionLoadVisibility ),
        []( section_visibility const &section ) {
            return (
                ( section.submodel->pName.find( "compartment" ) == 0 )
             || ( section.submodel->pName.find( "przedzial" )   == 0 ) ); } );
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

bool TDynamicObject::Update(double dt, double dt1)
{
    if (dt1 == 0)
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
        //     ts.R=fabs(0.5*MoverParameters->BDist/sin(ts.R*0.5));
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
    tp.friction = MyTrack->fFriction * Global.fFriction * Global.FrictionWeatherFactor;
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
    // 

    // napiecie sieci trakcyjnej
    // Ra 15-01: przeliczenie poboru prądu powinno być robione wcześniej, żeby na
    // tym etapie były
    // znane napięcia
    // TTractionParam tmpTraction;
    // tmpTraction.TractionVoltage=0;
    if (MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector)
    { // dla EZT tylko silnikowy
        // if (Global.bLiveTraction)
        { // Ra 2013-12: to niżej jest chyba trochę bez sensu
            double v = MoverParameters->PantRearVolt;
            if (v == 0.0) {
                v = MoverParameters->PantFrontVolt;
                if( v == 0.0 ) {
                    if( MoverParameters->TrainType & ( dt_EZT | dt_ET40 | dt_ET41 | dt_ET42 ) ) {
                        // dwuczłony mogą mieć sprzęg WN
                        v = MoverParameters->GetTrainsetVoltage(); // ostatnia szansa
                    }
                }
            }
            if (v != 0.0)
            { // jeśli jest zasilanie
                NoVoltTime = 0;
                tmpTraction.TractionVoltage = v;
            }
            else {
                NoVoltTime += dt1;
                if( NoVoltTime > 0.2 ) {
                    // jeśli brak zasilania dłużej niż 0.2 sekundy (25km/h pod izolatorem daje 0.15s)
                    // Ra 2F1H: prowizorka, trzeba przechować napięcie, żeby nie wywalało WS pod izolatorem
                    if( MoverParameters->Vel > 0.5 ) {
                        // jeśli jedzie
                        // Ra 2014-07: doraźna blokada logowania zimnych lokomotyw - zrobić to trzeba inaczej
                        if( MoverParameters->PantFrontUp
                         || MoverParameters->PantRearUp ) {

                            if( ( MoverParameters->Mains )
                             && ( MoverParameters->GetTrainsetVoltage() < 0.1f ) ) {
                                   // Ra 15-01: logować tylko, jeśli WS załączony
                                   // yB 16-03: i nie jest to asynchron zasilany z daleka 
                                   // Ra 15-01: bezwzględne współrzędne pantografu nie są dostępne,
                                   // więc lepiej się tego nie zaloguje
                                ErrorLog(
                                    "Bad traction: " + MoverParameters->Name
                                    + " lost power for " + to_string( NoVoltTime, 2 ) + " sec. at "
                                    + to_string( glm::dvec3{ vPosition } ) );
                            }
                        }
                    }
                    // Ra 2F1H: nie było sensu wpisywać tu zera po upływie czasu, bo zmienna była
                    // tymczasowa, a napięcie zerowane od razu
                    tmpTraction.TractionVoltage = 0; // Ra 2013-12: po co tak?
                }
            }
        }
    }
    else
        tmpTraction.TractionVoltage = 0.95 * MoverParameters->EnginePowerSource.MaxVoltage;
    tmpTraction.TractionFreq = 0;
    tmpTraction.TractionMaxCurrent = 7500; // Ra: chyba za dużo? powinno wywalać przy 1500
    tmpTraction.TractionResistivity = 0.3;

    // McZapkie: predkosc w torze przekazac do TrackParam
    // McZapkie: Vel ma wymiar [km/h] (absolutny), V ma wymiar [m/s], taka
    // przyjalem notacje
    tp.Velmax = MyTrack->VelocityGet();

    if (Mechanik)
    { // Ra 2F3F: do Driver.cpp to przenieść?
        MoverParameters->EqvtPipePress = GetEPP(); // srednie cisnienie w PG
        if( ( Mechanik->Primary() )
         && ( MoverParameters->EngineType == TEngineType::ElectricInductionMotor ) ) {
            // jesli glowny i z asynchronami, to niech steruje hamulcem lacznie dla calego pociagu/ezt
			auto const kier = (DirectionGet() * MoverParameters->ActiveCab > 0);
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
			// 1. ustal wymagana sile hamowania calego pociagu
            //   - opoznienie moze byc ustalane na podstawie charakterystyki
            //   - opoznienie moze byc ustalane na podstawie mas i cisnien granicznych
            //

            // 2. ustal mozliwa do realizacji sile hamowania ED
            //   - w szczegolnosci powinien brac pod uwage rozne sily hamowania
            for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
				(kier ? p = p->NextC(4) : p = p->PrevC(4)))
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
                FmaxED += ((p->MoverParameters->Mains) && (p->MoverParameters->ActiveDir != 0) &&
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

            if ((MoverParameters->Vel < 0.5) && (MoverParameters->BrakePress > 0.2) ||
                (dDoorMoveL > 0.001) || (dDoorMoveR > 0.001))
            {
                MoverParameters->ShuntMode = true;
            }
            if (MoverParameters->ShuntMode)
            {
                MoverParameters->ShuntModeAllow = (dDoorMoveL < 0.001) && (dDoorMoveR < 0.001) &&
                                                  (MoverParameters->LocalBrakeRatio() < 0.01);
            }
            if ((MoverParameters->Vel > 1) && (dDoorMoveL < 0.001) && (dDoorMoveR < 0.001))
            {
                MoverParameters->ShuntMode = false;
                MoverParameters->ShuntModeAllow = (MoverParameters->BrakePress > 0.2) &&
                                                  (MoverParameters->LocalBrakeRatio() < 0.01);
            }
            auto Fzad = amax * MoverParameters->LocalBrakeRatio() * masa;
			if ((MoverParameters->BrakeCtrlPos == MoverParameters->Handle->GetPos(bh_EB))
				&& (MoverParameters->eimc[eimc_p_abed] < 0.001))
				Fzad = amax * masa; //pętla bezpieczeństwa - pełne służbowe
            if ((MoverParameters->ScndS) &&
                (MoverParameters->Vel > MoverParameters->eimc[eimc_p_Vh1]) && (FmaxED > 0))
            {
                Fzad = std::min(MoverParameters->LocalBrakeRatio() * FmaxED, FfulED);
            }
            if (((MoverParameters->ShuntMode) && (Frj < 0.0015 * masa)) ||
                (MoverParameters->V * MoverParameters->DirAbsolute < -0.2))
            {
                Fzad = std::max(MoverParameters->StopBrakeDecc * masa, Fzad);
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
            if( ( MoverParameters->EpFuse && (MoverParameters->BrakeHandle != TBrakeHandle::MHZ_EN57))
             || ( ( MoverParameters->BrakeHandle == TBrakeHandle::MHZ_EN57 )
               && ( MoverParameters->BrakeOpModeFlag & bom_MED ) ) ) {
                FzadED = std::min( Fzad, FmaxED );
            }
			if ((MoverParameters->BrakeCtrlPos == MoverParameters->Handle->GetPos(bh_EB))
				&& (MoverParameters->eimc[eimc_p_abed] < 0.001)) 
				FzadED = 0; //pętla bezpieczeństwa - bez ED
            auto const FzadPN = Fzad - FrED;
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
            float Fpoj = 0; // MoverParameters->ActiveCab < 0
            ////ALGORYTM 2 - KAZDEMU PO ROWNO, ale nie wiecej niz eped * masa
            // 1. najpierw daj kazdemu tyle samo
            int i = 0;
			for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
				p = (kier == true ? p->NextC(4) : p->PrevC(4)) )
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
                    (MoverParameters->ScndS ? MoverParameters->LocalBrakeRatio() : FzED[i]);
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
                for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
                     p = (kier == true ? p->NextC(4) : p->PrevC(4)) )
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
                for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
                     (true == kier ? p = p->NextC(4) : p = p->PrevC(4)))
                {
                    if (!PrzekrF[i])
                    {
                        FzEP[i] += przek;
                    }
                    ++i;
                }
            }
            i = 0;
            for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
                 (true == kier ? p = p->NextC(4) : p = p->PrevC(4)))
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
				for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
					(true == kier ? p = p->NextC(4) : p = p->PrevC(4)))
				{
					MEDLogFile << "\t" << p->MoverParameters->BrakePress;
				}
				MEDLogFile << "\n";
				if (floor(MEDLogTime + dt1) > floor(MEDLogTime))
				{
					MEDLogFile.flush();
				}
				MEDLogTime += dt1;

				if ((MoverParameters->Vel < 0.1) || (MoverParameters->MainCtrlPos > 0))
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
        }

        Mechanik->UpdateSituation(dt1); // przebłyski świadomości AI
    }

    // fragment "z EXE Kursa"
    if( MoverParameters->Mains ) { // nie wchodzić w funkcję bez potrzeby
        if( ( false == MoverParameters->Battery )
         && ( false == MoverParameters->ConverterFlag ) // added alternative power source. TODO: more generic power check
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
    TRotation r { 0.0, 0.0, 0.0 };
    // McZapkie-260202 - dMoveLen przyda sie przy stukocie kol
    dDOMoveLen = GetdMoveLen() + MoverParameters->ComputeMovement(dt, dt1, ts, tp, tmpTraction, l, r);
    if( Mechanik )
        Mechanik->MoveDistanceAdd( dDOMoveLen ); // dodanie aktualnego przemieszczenia

    glm::dvec3 old_pos = vPosition;
    Move(dDOMoveLen);

    m_future_movement = (glm::dvec3(vPosition) - old_pos) / dt1 * Timer::GetDeltaTime();

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
            dRailLength = MyTrack->fSoundDistance;
            for( auto &axle : m_axlesounds ) {
                axle.distance = axle.offset + MoverParameters->Dim.L;
            }
        }
        if( dRailLength != -1 ) {
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

                auto axleindex { 0 };
                for( auto &axle : m_axlesounds ) {
                    axle.distance -= dDOMoveLen * Sign( dDOMoveLen );
                    if( axle.distance < 0 ) {
                        axle.distance += dRailLength;
                        if( MoverParameters->Vel > 2.5 ) {
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

        m_future_wheels_angle = (glm::dvec3(dWheelAngle[0], dWheelAngle[1], dWheelAngle[2]) - old_wheels) / dt1 * Timer::GetDeltaTime();

        if (dWheelAngle[0] > 360.0)
            dWheelAngle[0] -= 360.0; // a w drugą stronę jak się kręcą?
        if (dWheelAngle[1] > 360.0)
            dWheelAngle[1] -= 360.0;
        if (dWheelAngle[2] > 360.0)
            dWheelAngle[2] -= 360.0;
    }

    if (pants) // pantograf może być w wagonie kuchennym albo pojeździe rewizyjnym
    // (np. SR61)
    { // przeliczanie kątów dla pantografów
        double k; // tymczasowy kąt
        double PantDiff;
        TAnimPant *p; // wskaźnik do obiektu danych pantografu
        double fCurrent = (
            ( MoverParameters->DynamicBrakeFlag && MoverParameters->ResistorsFlag ) ?
                0 :
                MoverParameters->Itot )
            + MoverParameters->TotalCurrent; // prąd pobierany przez pojazd - bez
        // sensu z tym (TotalCurrent)
        // TotalCurrent to bedzie prad nietrakcyjny (niezwiazany z napedem)
        // fCurrent+=fabs(MoverParameters->Voltage)*1e-6; //prąd płynący przez
        // woltomierz,
        // rozładowuje kondensator orgromowy 4µF
        double fPantCurrent = fCurrent; // normalnie cały prąd przez jeden pantograf
        if (pants)
            if (iAnimType[ANIM_PANTS] > 1) // a jeśli są dwa pantografy //Ra 1014-11:
                // proteza, trzeba zrobić sensowniej
                if (pants[0].fParamPants->hvPowerWire &&
                    pants[1].fParamPants->hvPowerWire) // i oba podłączone do drutów
                    fPantCurrent = fCurrent * 0.5; // to dzielimy prąd równo na oba (trochę bez
        // sensu, ale lepiej tak niż podwoić prąd)
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
                else if( ( true == MoverParameters->PantFrontUp )
                      && ( PantDiff < 0.01 ) ) // tolerancja niedolegania
                {
                    if (p->hvPowerWire) {
                        auto const lastvoltage { MoverParameters->PantFrontVolt };
                        // TODO: wyliczyć trzeba prąd przypadający na pantograf i wstawić do GetVoltage()
                        MoverParameters->PantFrontVolt = p->hvPowerWire->VoltageGet( MoverParameters->Voltage, fPantCurrent );
                        fCurrent -= fPantCurrent; // taki prąd płynie przez powyższy pantograf
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
                else if ( ( true == MoverParameters->PantRearUp )
                       && ( PantDiff < 0.01 ) )
                {
                    if (p->hvPowerWire) {
                        auto const lastvoltage { MoverParameters->PantRearVolt };
                        // TODO: wyliczyć trzeba prąd przypadający na pantograf i wstawić do GetVoltage()
                        MoverParameters->PantRearVolt = p->hvPowerWire->VoltageGet( MoverParameters->Voltage, fPantCurrent );
                        fCurrent -= fPantCurrent; // taki prąd płynie przez powyższy pantograf
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
            if( ( false == MoverParameters->Battery )
             && ( false == MoverParameters->ConverterFlag ) ) {
                pantspeedfactor = 0.0;
            }
            pantspeedfactor = std::max( 0.0, pantspeedfactor );
            k = p->fAngleL;
            if( ( pantspeedfactor > 0.0 )
             && ( i ?
                    MoverParameters->PantRearUp :
                    MoverParameters->PantFrontUp ) )// jeśli ma być podniesiony
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
        if ((MoverParameters->PantFrontSP == false) && (MoverParameters->PantFrontUp == false))
        {
            for( auto &pantograph : m_pantographsounds ) {
                if( pantograph.sPantDown.offset().z > 0 ) {
                    // limit to pantographs located in the front half of the vehicle
                    pantograph.sPantDown.play( sound_flags::exclusive );
                }
            }
            MoverParameters->PantFrontSP = true;
        }
        if ((MoverParameters->PantRearSP == false) && (MoverParameters->PantRearUp == false))
        {
            for( auto &pantograph : m_pantographsounds ) {
                if( pantograph.sPantDown.offset().z < 0 ) {
                    // limit to pantographs located in the rear half of the vehicle
                    pantograph.sPantDown.play( sound_flags::exclusive );
                }
            }
            MoverParameters->PantRearSP = true;
        }
/*
        // NOTE: disabled because it's both redundant and doesn't take into account alternative power sources
        // converter and compressor will (should) turn off during their individual checks, in the mover's (fast)computemovement() calls
        if (MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
        { // Winger 240404 - wylaczanie sprezarki i
            // przetwornicy przy braku napiecia
            if (tmpTraction.TractionVoltage == 0)
            { // to coś wyłączało dźwięk silnika w ST43!
                MoverParameters->ConverterFlag = false;
                MoverParameters->CompressorFlag = false; // Ra: to jest wątpliwe - wyłączenie sprężarki powinno być w jednym miejscu!
            }
        }
*/
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

    // NBMX Obsluga drzwi, MC: zuniwersalnione
    // TODO: fully generalized door assembly
    if( ( dDoorMoveL < MoverParameters->DoorMaxShiftL )
     && ( true == MoverParameters->DoorLeftOpened ) ) {
        // open left door
        if( ( MoverParameters->TrainType == dt_EZT )
         || ( MoverParameters->TrainType == dt_DMU ) ) {
            // multi-unit vehicles typically open door only after unfolding the doorstep
            if( ( MoverParameters->PlatformMaxShift == 0.0 ) // no wait if no doorstep
             || ( MoverParameters->PlatformOpenMethod == 2 ) // no wait for rotating doorstep
             || ( dDoorstepMoveL == 1.0 ) ) {
                dDoorMoveL = std::min(
                    MoverParameters->DoorMaxShiftL,
                    dDoorMoveL + MoverParameters->DoorOpenSpeed * dt1 );
            }
        }
        else {
            dDoorMoveL = std::min(
                MoverParameters->DoorMaxShiftL,
                dDoorMoveL + MoverParameters->DoorOpenSpeed * dt1 );
        }
        DoorDelayL = 0.f;
    }
    if( ( dDoorMoveL > 0.0 )
     && ( false == MoverParameters->DoorLeftOpened ) ) {
        // close left door
        DoorDelayL += dt1;
        if( DoorDelayL > MoverParameters->DoorCloseDelay ) {
            dDoorMoveL -= dt1 * MoverParameters->DoorCloseSpeed;
            dDoorMoveL = std::max( dDoorMoveL, 0.0 );
        }
    }
    if( ( dDoorMoveR < MoverParameters->DoorMaxShiftR )
     && ( true == MoverParameters->DoorRightOpened ) ) {
        // open right door
        if( ( MoverParameters->TrainType == dt_EZT )
         || ( MoverParameters->TrainType == dt_DMU ) ) {
            // multi-unit vehicles typically open door only after unfolding the doorstep
            if( ( MoverParameters->PlatformMaxShift == 0.0 ) // no wait if no doorstep
             || ( MoverParameters->PlatformOpenMethod == 2 ) // no wait for rotating doorstep
             || ( dDoorstepMoveR == 1.0 ) ) {
                dDoorMoveR = std::min(
                    MoverParameters->DoorMaxShiftR,
                    dDoorMoveR + MoverParameters->DoorOpenSpeed * dt1 );
            }
        }
        else {
            dDoorMoveR = std::min(
                MoverParameters->DoorMaxShiftR,
                dDoorMoveR + MoverParameters->DoorOpenSpeed * dt1 );
        }
        DoorDelayR = 0.f;
    }
    if( ( dDoorMoveR > 0.0 )
     && ( false == MoverParameters->DoorRightOpened ) ) {
        // close right door
        DoorDelayR += dt1;
        if( DoorDelayR > MoverParameters->DoorCloseDelay ) {
            dDoorMoveR -= dt1 * MoverParameters->DoorCloseSpeed;
            dDoorMoveR = std::max( dDoorMoveR, 0.0 );
        }
    }
    // doorsteps
    if( ( dDoorstepMoveL < 1.0 )
     && ( true == MoverParameters->DoorLeftOpened ) ) {
        // unfold left doorstep
        dDoorstepMoveL = std::min(
            1.0,
            dDoorstepMoveL + MoverParameters->PlatformSpeed * dt1 );
    }
    if( ( dDoorstepMoveL > 0.0 )
     && ( false == MoverParameters->DoorLeftOpened )
     && ( DoorDelayL > MoverParameters->DoorCloseDelay ) ) {
        // fold left doorstep
        if( ( MoverParameters->TrainType == dt_EZT )
         || ( MoverParameters->TrainType == dt_DMU ) ) {
            // multi-unit vehicles typically fold the doorstep only after closing the door
            if( dDoorMoveL == 0.0 ) {
                dDoorstepMoveL = std::max(
                    0.0,
                    dDoorstepMoveL - MoverParameters->PlatformSpeed * dt1 );
            }
        }
        else {
            dDoorstepMoveL = std::max(
                0.0,
                dDoorstepMoveL - MoverParameters->PlatformSpeed * dt1 );
        }
    }
    if( ( dDoorstepMoveR < 1.0 )
     && ( true == MoverParameters->DoorRightOpened ) ) {
        // unfold right doorstep
        dDoorstepMoveR = std::min(
            1.0,
            dDoorstepMoveR + MoverParameters->PlatformSpeed * dt1 );
    }
    if( ( dDoorstepMoveR > 0.0 )
     && ( false == MoverParameters->DoorRightOpened )
     && ( DoorDelayR > MoverParameters->DoorCloseDelay ) ) {
        // fold right doorstep
        if( ( MoverParameters->TrainType == dt_EZT )
         || ( MoverParameters->TrainType == dt_DMU ) ) {
            // multi-unit vehicles typically fold the doorstep only after closing the door
            if( dDoorMoveR == 0.0 ) {
                dDoorstepMoveR = std::max(
                    0.0,
                    dDoorstepMoveR - MoverParameters->PlatformSpeed * dt1 );
            }
        }
        else {
            dDoorstepMoveR = std::max(
                0.0,
                dDoorstepMoveR - MoverParameters->PlatformSpeed * dt1 );
        }
    }
    // mirrors
    if( MoverParameters->Vel > 5.0 ) {
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
         && ( true == MoverParameters->DoorLeftOpened ) ) {
            dMirrorMoveL = std::min(
                1.0,
                dMirrorMoveL + 1.0 * dt1 );
        }
        if( ( dMirrorMoveR < 1.0 )
         && ( true == MoverParameters->DoorRightOpened ) ) {
            dMirrorMoveR = std::min(
                1.0,
                dMirrorMoveR + 1.0 * dt1 );
        }
    }

    // compartment lights
    // if the vehicle has a controller, we base the light state on state of the controller otherwise we check the vehicle itself
    if( ( ctOwner != nullptr ?
            ctOwner->Controlling()->Battery != SectionLightsActive :
            SectionLightsActive == true ) ) { // without controller lights are off. NOTE: this likely mess up the EMU
        toggle_lights();
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

glm::dvec3 TDynamicObject::get_future_movement() const
{
    return m_future_movement;
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
    TRotation r { 0.0, 0.0, 0.0 };
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
    btCoupler1.Turn( false );
    btCoupler2.Turn( false );
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
    btEndSignals11.Turn( false );
    btEndSignals13.Turn( false );
    btEndSignals21.Turn( false );
    btEndSignals23.Turn( false );
    btEndSignals1.Turn( false );
    btEndSignals2.Turn( false );
    btEndSignalsTab1.Turn( false );
    btEndSignalsTab2.Turn( false );
    btHeadSignals11.Turn( false );
    btHeadSignals12.Turn( false );
    btHeadSignals13.Turn( false );
    btHeadSignals21.Turn( false );
    btHeadSignals22.Turn( false );
    btHeadSignals23.Turn( false );
	btMechanik1.Turn( false );
	btMechanik2.Turn( false );
    btShutters1.Turn( false );
    btShutters2.Turn( false );
};

// przeliczanie dźwięków, bo będzie słychać bez wyświetlania sektora z pojazdem
void TDynamicObject::RenderSounds() {

    if( Global.iPause != 0 ) { return; }

    if( ( m_startjoltplayed )
     && ( ( std::abs( MoverParameters->AccSVBased ) < 0.01 )
       || ( GetVelocity() < 0.01 ) ) ) {
        // if the vehicle comes to a stop set the movement jolt to play when it starts moving again
        m_startjoltplayed = false;
    }

    double const dt{ Timer::GetDeltaRenderTime() };
    double volume{ 0.0 };
    double frequency{ 1.0 };

    m_powertrainsounds.render( *MoverParameters, dt );

    // NBMX dzwiek przetwornicy
    if( MoverParameters->ConverterFlag ) {
        frequency = (
            MoverParameters->EngineType == TEngineType::ElectricSeriesMotor ?
            ( MoverParameters->RunningTraction.TractionVoltage / MoverParameters->NominalVoltage ) * MoverParameters->RList[ MoverParameters->RlistSize ].Mn :
            1.0 );
        frequency = sConverter.m_frequencyoffset + sConverter.m_frequencyfactor * frequency;
        sConverter
            .pitch( clamp( frequency, 0.5, 1.25 ) ) // arbitrary limits )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sConverter.stop();
    }

    if( MoverParameters->CompressorSpeed > 0.0 ) {
        // McZapkie! - dzwiek compressor.wav tylko gdy dziala sprezarka
        if( MoverParameters->CompressorFlag ) {
            sCompressor.play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            sCompressor.stop();
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
    if( ( true == MoverParameters->Heating )
     && ( std::abs( MoverParameters->enrot ) > 0.01 ) ) {
        // TBD: check whether heating should depend on 'engine rotations' for electric vehicles
        sHeater
            .pitch( true == sHeater.is_combined() ?
                    std::abs( MoverParameters->enrot ) * 60.f * 0.01f :
                    1.f )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sHeater.stop();
    }

    // brake system and braking sounds:

    // brake cylinder piston
    auto const brakepressureratio { std::max( 0.0, MoverParameters->BrakePress ) / std::max( 1.0, MoverParameters->MaxBrakePress[ 3 ] ) };
    if( m_lastbrakepressure != -1.f ) {
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

    // air release
    if( m_lastbrakepressure != -1.f ) {
        // calculate rate of pressure drop in brake cylinder, once it's been initialized
        auto const brakepressuredifference{ m_lastbrakepressure - MoverParameters->BrakePress };
        m_brakepressurechange = interpolate<float>( m_brakepressurechange, brakepressuredifference / dt, 0.005f );
    }
    m_lastbrakepressure = MoverParameters->BrakePress;
    // ensure some basic level of volume and scale it up depending on pressure in the cylinder; scale this by the air release rate
    volume = 20 * m_brakepressurechange * ( 0.25 + 0.75 * brakepressureratio );
    if( volume > 0.075f ) {
        rsUnbrake
            .gain( volume )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        // don't stop the sound too abruptly
        volume = std::max( 0.0, rsUnbrake.gain() - 0.2 * dt );
        rsUnbrake.gain( volume );
        if( volume < 0.05 ) {
            rsUnbrake.stop();
        }
    }

    // Dzwiek odluzniacza
    if( MoverParameters->Hamulec->GetStatus() & b_rls ) {
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
    if( MoverParameters->DoorClosureWarning ) {
        for( auto &door : m_doorsounds ) {
            // TBD, TODO: per-location door state triggers?
            if( ( MoverParameters->DepartureSignal )
/*
             || ( ( MoverParameters->DoorCloseCtrl = control::autonomous )
               && ( ( ( false == MoverParameters->DoorLeftOpened )  && ( dDoorMoveL > 0.0 ) )
                 || ( ( false == MoverParameters->DoorRightOpened ) && ( dDoorMoveR > 0.0 ) ) ) )
*/
                 ) {
                // for the autonomous doors play the warning automatically whenever a door is closing
                // MC: pod warunkiem ze jest zdefiniowane w chk
                door.sDepartureSignal.play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                door.sDepartureSignal.stop();
            }
        }
    }
    // NBMX Obsluga drzwi, MC: zuniwersalnione
    // TODO: fully generalized door assembly
    if( true == MoverParameters->DoorLeftOpened ) {
        // open left door
        // door sounds
        // due to potential wait for the doorstep we play the sound only during actual animation
        if( ( dDoorMoveL > 0.0 )
         && ( dDoorMoveL < MoverParameters->DoorMaxShiftL ) ) {
            for( auto &door : m_doorsounds ) {
                if( door.rsDoorClose.offset().x > 0.f ) {
                    // determine left side doors from their offset
                    door.rsDoorOpen.play( sound_flags::exclusive );
                    door.rsDoorClose.stop();
                }
            }
        }
        // doorstep sounds
        if( dDoorstepMoveL < 1.0 ) {
            for( auto &door : m_doorsounds ) {
                if( door.step_close.offset().x > 0.f ) {
                    door.step_open.play( sound_flags::exclusive );
                    door.step_close.stop();
                }
            }
        }
    }
    if( false == MoverParameters->DoorLeftOpened ) {
        // close left door
        // door sounds can start playing before the door begins moving
        if( dDoorMoveL > 0.0 ) {
            for( auto &door : m_doorsounds ) {
                if( door.rsDoorClose.offset().x > 0.f ) {
                    // determine left side doors from their offset
                    door.rsDoorClose.play( sound_flags::exclusive );
                    door.rsDoorOpen.stop();
                }
            }
        }
        // doorstep sounds are played only when the doorstep is moving
        if( ( dDoorstepMoveL > 0.0 )
         && ( dDoorstepMoveL < 1.0 ) ) {
            for( auto &door : m_doorsounds ) {
                if( door.step_close.offset().x > 0.f ) {
                    // determine left side doors from their offset
                    door.step_close.play( sound_flags::exclusive );
                    door.step_open.stop();
                }
            }
        }
    }

    if( true == MoverParameters->DoorRightOpened ) {
        // open right door
        // door sounds
        // due to potential wait for the doorstep we play the sound only during actual animation
        if( ( dDoorMoveR > 0.0 )
         && ( dDoorMoveR < MoverParameters->DoorMaxShiftR ) ) {
            for( auto &door : m_doorsounds ) {
                if( door.rsDoorClose.offset().x < 0.f ) {
                    // determine right side doors from their offset
                    door.rsDoorOpen.play( sound_flags::exclusive );
                    door.rsDoorClose.stop();
                }
            }
        }
        // doorstep sounds
        if( dDoorstepMoveR < 1.0 ) {
            for( auto &door : m_doorsounds ) {
                if( door.step_close.offset().x < 0.f ) {
                    door.step_open.play( sound_flags::exclusive );
                    door.step_close.stop();
                }
            }
        }
    }
    if( false == MoverParameters->DoorRightOpened ) {
        // close right door
        // door sounds can start playing before the door begins moving
        if( dDoorMoveR > 0.0 ) {
            for( auto &door : m_doorsounds ) {
                if( door.rsDoorClose.offset().x < 0.f ) {
                    // determine left side doors from their offset
                    door.rsDoorClose.play( sound_flags::exclusive );
                    door.rsDoorOpen.stop();
                }
            }
        }
        // doorstep sounds are played only when the doorstep is moving
        if( ( dDoorstepMoveR > 0.0 )
         && ( dDoorstepMoveR < 1.0 ) ) {
            for( auto &door : m_doorsounds ) {
                if( door.step_close.offset().x < 0.f ) {
                    // determine left side doors from their offset
                    door.step_close.play( sound_flags::exclusive );
                    door.step_open.stop();
                }
            }
        }
    }
    // door locks
    if( MoverParameters->DoorBlockedFlag() != m_doorlocks ) {
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
    if( TestFlag( MoverParameters->WarningSignal, 1 ) ) {
        sHorn1.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sHorn1.stop();
    }
    if( TestFlag( MoverParameters->WarningSignal, 2 ) ) {
        sHorn2.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sHorn2.stop();
    }
    if( TestFlag( MoverParameters->WarningSignal, 4 ) ) {
        sHorn3.play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        sHorn3.stop();
    }
    // szum w czasie jazdy
    if( ( GetVelocity() > 0.5 )
     && ( false == m_bogiesounds.empty() )
     && ( // compound test whether the vehicle belongs to user-driven consist (as these don't emit outer noise in cab view)
            FreeFlyModeFlag ? true : // in external view all vehicles emit outer noise
            // Global.pWorld->train() == nullptr ? true : // (can skip this check, with no player train the external view is a given)
            ctOwner == nullptr ? true : // standalone vehicle, can't be part of user-driven train
            ctOwner != simulation::Train->Dynamic()->ctOwner ? true : // confirmed isn't a part of the user-driven train
            Global.CabWindowOpen ? true : // sticking head out we get to hear outer noise
            false ) ) {

        auto const &bogiesound { m_bogiesounds.front() };
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
            for( auto &bogiesound : m_bogiesounds ) {
                bogiesound
                    .pitch( frequency ) // arbitrary limits to prevent the pitch going out of whack
                    .gain( volume )
                    .play( sound_flags::exclusive | sound_flags::looping );
            }
        }
        else {
            // stop all noise instances
            for( auto &bogiesound : m_bogiesounds ) {
                bogiesound.stop();
            }
        }
    }
    else {
        // don't play the optional ending sound if the listener switches views
        for( auto &bogiesound : m_bogiesounds ) {
            bogiesound.stop( false == FreeFlyModeFlag );
        }
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
    if( ( ts.R * ts.R > 1 )
     && ( MoverParameters->Vel > 5.0 ) ) {
        // scale volume with curve radius and vehicle speed
        volume =
            MoverParameters->AccN * MoverParameters->AccN
            * interpolate(
                0.0, 1.0,
                clamp(
                    MoverParameters->Vel / 40.0,
                    0.0, 1.0 ) )
            + ( MyTrack->eType == tt_Switch ? 0.25 : 0.0 );
    }
    else {
        volume = 0;
    }
    if( volume > 0.05 ) {
        rscurve
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

    // McZapkie! - to wazne - SoundFlag wystawiane jest przez moje moduly
    // gdy zachodza pewne wydarzenia komentowane dzwiekiem.
    if( TestFlag( MoverParameters->SoundFlag, sound::pneumatic ) ) {
        // pneumatic relay
        dsbPneumaticRelay
            .gain(
                true == TestFlag( MoverParameters->SoundFlag, sound::loud ) ?
                    1.0f :
                    0.8f )
            .play();
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
                    .gain( 0.65f )
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
                    .gain( 0.65f )
                    .play( sound_flags::exclusive );
            }
        }

        // TODO: dedicated sound for each connection type
        // until then, play legacy placeholders:
        if( ( coupler.sounds & ( sound::attachcoupler | sound::attachcontrol | sound::attachgangway ) ) != 0 ) {
            m_couplersounds[ couplerindex ].dsbCouplerAttach.play();
        }
        if( ( coupler.sounds & ( sound::attachbrakehose | sound::attachmainhose | sound::attachheating ) ) != 0 ) {
            m_couplersounds[ couplerindex ].dsbCouplerDetach.play();
        }
        if( true == TestFlag( coupler.sounds, sound::detachall ) ) {
            // TODO: dedicated disconnect sounds
            m_couplersounds[ couplerindex ].dsbCouplerAttach.play();
            m_couplersounds[ couplerindex ].dsbCouplerDetach.play();
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
            if( GetVelocity() > 0 ) {
                rsDerailment.play();
            }
            else {
                rsDerailment.stop();
            }
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

// McZapkie-250202
// wczytywanie pliku z danymi multimedialnymi (dzwieki)
void TDynamicObject::LoadMMediaFile( std::string const &TypeName, std::string const &ReplacableSkin ) {

    Global.asCurrentDynamicPath = asBaseDir;
    std::string asFileName = asBaseDir + TypeName + ".mmd";
    std::string asAnimName;
    bool Stop_InternalData = false;
    pants = NULL; // wskaźnik pierwszego obiektu animującego dla pantografów
	cParser parser( TypeName + ".mmd", cParser::buffer_FILE, asBaseDir );
    if( false == parser.ok() ) {
        ErrorLog( "Failed to load appearance data for vehicle " + MoverParameters->Name );
        return;
    }
	std::string token;
    do {
		token = "";
		parser.getTokens(); parser >> token;

		if( ( token == "models:" )
         || ( token == "\xef\xbb\xbfmodels:" ) ) { // crude way to handle utf8 bom potentially appearing before the first token
			// modele i podmodele
            m_materialdata.multi_textures = 0; // czy jest wiele tekstur wymiennych?
			parser.getTokens();
			parser >> asModel;
            replace_slashes( asModel );
            if( asModel[asModel.size() - 1] == '#' ) // Ra 2015-01: nie podoba mi siê to
            { // model wymaga wielu tekstur wymiennych
                m_materialdata.multi_textures = 1;
                asModel.erase( asModel.length() - 1 );
            }
            // name can contain leading slash, erase it to avoid creation of double slashes when the name is combined with current directory
            if( asModel[ 0 ] == '/' ) {
                asModel.erase( 0, 1 );
            }
            std::size_t i = asModel.find( ',' );
            if ( i != std::string::npos )
            { // Ra 2015-01: może szukać przecinka w nazwie modelu, a po przecinku była by liczba tekstur?
                if( i < asModel.length() ) {
                    m_materialdata.multi_textures = asModel[ i + 1 ] - '0';
                }
                m_materialdata.multi_textures = clamp( m_materialdata.multi_textures, 0, 1 ); // na razie ustawiamy na 1
            }
            asModel = asBaseDir + asModel; // McZapkie 2002-07-20: dynamics maja swoje modele w dynamics/basedir
            Global.asCurrentTexturePath = asBaseDir; // biezaca sciezka do tekstur to dynamic/...
            mdModel = TModelsManager::GetModel(asModel, true);
            if (ReplacableSkin != "none")
            {
                if (m_materialdata.multi_textures > 0) {
                    // jeśli model ma 4 tekstury
                    // check for the pipe method first
                    if( ReplacableSkin.find( '|' ) != std::string::npos ) {
                        cParser nameparser( ReplacableSkin );
                        nameparser.getTokens( 4, true, "|" );
                        int skinindex = 0;
                        std::string texturename; nameparser >> texturename;
                        while( ( texturename != "" ) && ( skinindex < 4 ) ) {
							std::replace(texturename.begin(), texturename.end(), '\\', '/');
                            m_materialdata.replacable_skins[ skinindex + 1 ] = GfxRenderer.Fetch_Material( texturename );
                            ++skinindex;
                            texturename = ""; nameparser >> texturename;
                        }
                        m_materialdata.multi_textures = skinindex;
                    }
                    else {
                        // otherwise try the basic approach
                        int skinindex = 0;
                        do {
                            material_handle material = GfxRenderer.Fetch_Material( ReplacableSkin + "," + std::to_string( skinindex + 1 ), true );
                            if( material == null_handle ) {
                                break;
                            }
                            m_materialdata.replacable_skins[ skinindex + 1 ] = material;
                            ++skinindex;
                        } while( skinindex < 4 );
                        m_materialdata.multi_textures = skinindex;
                        if( m_materialdata.multi_textures == 0 ) {
                            // zestaw nie zadziałał, próbujemy normanie
                            m_materialdata.replacable_skins[ 1 ] = GfxRenderer.Fetch_Material( ReplacableSkin );
                        }
                    }
                }
                else {
                    m_materialdata.replacable_skins[ 1 ] = GfxRenderer.Fetch_Material( ReplacableSkin );
                }

                // potentially set blank destination texture
                DestinationSet( {}, {} );

                if( GfxRenderer.Material( m_materialdata.replacable_skins[ 1 ] ).get_or_guess_opacity() == 0.0f ) {
                    // tekstura -1 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
                    m_materialdata.textures_alpha = 0x31310031;
                }
                else {
                    // wszystkie tekstury nieprzezroczyste - nie renderować w cyklu przezroczystych
                    m_materialdata.textures_alpha = 0x30300030;
                }

                if( ( m_materialdata.replacable_skins[ 2 ] )
                 && ( GfxRenderer.Material( m_materialdata.replacable_skins[ 2 ] ).get_or_guess_opacity() == 0.0f ) ) {
                    // tekstura -2 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
                    m_materialdata.textures_alpha |= 0x02020002;
                }
                if( ( m_materialdata.replacable_skins[ 3 ] )
                 && ( GfxRenderer.Material( m_materialdata.replacable_skins[ 3 ] ).get_or_guess_opacity() == 0.0f ) ) {
                    // tekstura -3 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
                    m_materialdata.textures_alpha |= 0x04040004;
                }
                if( ( m_materialdata.replacable_skins[ 4 ] )
                 && ( GfxRenderer.Material( m_materialdata.replacable_skins[ 4 ] ).get_or_guess_opacity() == 0.0f ) ) {
                    // tekstura -4 z kanałem alfa - nie renderować w cyklu nieprzezroczystych
                    m_materialdata.textures_alpha |= 0x08080008;
                }
            }
            if( false == MoverParameters->LoadAttributes.empty() ) {
                // Ra: tu wczytywanie modelu ładunku jest w porządku
                mdLoad = LoadMMediaFile_mdload( MoverParameters->LoadType.name );
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
                    if( asModel[ 0 ] == '/' ) {
                        // filename can potentially begin with a slash, and we don't need it
                        asModel.erase( 0, 1 );
                    }
                    asModel = asBaseDir + asModel; // McZapkie-200702 - dynamics maja swoje modele w dynamic/basedir
                    Global.asCurrentTexturePath = asBaseDir; // biezaca sciezka do tekstur to dynamic/...
                    mdLowPolyInt = TModelsManager::GetModel(asModel, true);
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
                            pAnimations[i].fMaxDist = 50 * MoverParameters->WheelDiameter; // nie kręcić w większej odległości
                            pAnimations[i].fMaxDist *= pAnimations[i].fMaxDist * MoverParameters->WheelDiameter; // 50m do kwadratu, a średnica do trzeciej
                            pAnimations[i].fMaxDist *= Global.fDistanceFactor; // współczynnik przeliczeniowy jakości ekranu
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
                            switch (MoverParameters->DoorOpenMethod)
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
                            pAnimations[i + j].iNumber = i; // parzyste działają inaczej niż nieparzyste
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
                            switch (MoverParameters->PlatformOpenMethod)
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
                            pAnimations[i + j].iNumber = i; // parzyste działają inaczej niż nieparzyste
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
                                    side::front :
                                    side::rear ) << 4 )
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
                            std::atof( token.c_str() ),
                            { sound_placement::external, static_cast<float>( dSDist ) } };
                        axle.clatter.deserialize( parser, sound_type::single );
                        axle.clatter.owner( this );
                        axle.clatter.offset( { 0, 0, -axle.offset } ); // vehicle faces +Z in 'its' space, for axle locations negative value means ahead of centre
                        m_axlesounds.emplace_back( axle );
                    }
                    // arrange the axles in case they're listed out of order
                    std::sort(
                        std::begin( m_axlesounds ), std::end( m_axlesounds ),
                        []( axle_sounds const &Left, axle_sounds const &Right ) {
                            return ( Left.offset < Right.offset ); } );
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
				}

                else if( token == "dieselinc:" ) {
                    // dzwiek przy wlazeniu na obroty woodwarda
                    m_powertrainsounds.engine_revving.deserialize( parser, sound_type::single, sound_parameters::range );
                    m_powertrainsounds.engine_revving.owner( this );
                }

                else if( token == "oilpump:" ) {
                    // plik z dzwiekiem wentylatora, mnozniki i ofsety amp. i czest.
                    m_powertrainsounds.oil_pump.deserialize( parser, sound_type::single );
                    m_powertrainsounds.oil_pump.owner( this );
                }

                else if( ( token == "tractionmotor:" )
                      && ( MoverParameters->Power > 0 ) ) {
                    // plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
                    sound_source motortemplate { sound_placement::external };
                    motortemplate.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    motortemplate.owner( this );

                    auto const amplitudedivisor = static_cast<float>( MoverParameters->nmax * 60 + MoverParameters->Power * 3 );
                    motortemplate.m_amplitudefactor /= amplitudedivisor;

                    if( true == m_powertrainsounds.motors.empty() ) {
                        // fallback for cases without specified motor locations, convert sound template to a single sound source
                        m_powertrainsounds.motors.emplace_back( motortemplate );
                    }
                    else {
                        // apply configuration to all defined motors
                        for( auto &motor : m_powertrainsounds.motors ) {
                            // combine potential x- and y-axis offsets of the sound template with z-axis offsets of individual motors
                            auto motoroffset { motortemplate.offset() };
                            motoroffset.z = motor.offset().z;
                            motor = motortemplate;
                            motor.offset( motoroffset );
                        }
                    }
                }

                else if( token == "motorblower:" ) {

                    sound_source blowertemplate { sound_placement::engine };
                    blowertemplate.deserialize( parser, sound_type::single, sound_parameters::range | sound_parameters::amplitude | sound_parameters::frequency );
                    blowertemplate.owner( this );

                    auto const amplitudedivisor = static_cast<float>(
                        MoverParameters->MotorBlowers[ side::front ].speed > 0.f ?
                            MoverParameters->MotorBlowers[ side::front ].speed * MoverParameters->nmax * 60 + MoverParameters->Power * 3 :
                            MoverParameters->MotorBlowers[ side::front ].speed * -1 );
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
                    m_powertrainsounds.radiator_fan.deserialize( parser, sound_type::single );
                    m_powertrainsounds.radiator_fan.owner( this );
                }

				else if( token == "transmission:" ) {
					// plik z dzwiekiem, mnozniki i ofsety amp. i czest.
                    // NOTE, fixed default parameters, legacy system leftover
                    m_powertrainsounds.transmission.m_amplitudefactor = 0.029;
                    m_powertrainsounds.transmission.m_amplitudeoffset = 0.1;
                    m_powertrainsounds.transmission.m_frequencyfactor = 0.005;
                    m_powertrainsounds.transmission.m_frequencyoffset = 1.0;

                    m_powertrainsounds.transmission.deserialize( parser, sound_type::single, sound_parameters::range );
                    m_powertrainsounds.transmission.owner( this );
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

				else if( token == "brakeacc:" ) {
					// plik z przyspieszaczem (upust po zlapaniu hamowania)
                    sBrakeAcc.deserialize( parser, sound_type::single );
                    sBrakeAcc.owner( this );

                    bBrakeAcc = true;
                }

				else if( token == "unbrake:" ) {
					// plik z piskiem hamulca, mnozniki i ofsety amplitudy.
                    rsUnbrake.deserialize( parser, sound_type::single, sound_parameters::range );
                    rsUnbrake.owner( this );
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
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.lock.offset() };
                        door.sDepartureSignal = soundtemplate;
                        door.sDepartureSignal.offset( dooroffset );
                    }
                }

				else if( token == "dooropen:" ) {
                    sound_source soundtemplate { sound_placement::general, 25.f };
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
                    sound_source soundtemplate { sound_placement::general, 25.f };
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
                    sound_source soundtemplate { sound_placement::general, 12.5f };
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
                    sound_source soundtemplate { sound_placement::general, 12.5f };
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
                    sound_source soundtemplate { sound_placement::general, 20.f };
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
                    sound_source soundtemplate { sound_placement::general, 20.f };
                    soundtemplate.deserialize( parser, sound_type::single );
                    soundtemplate.owner( this );
                    for( auto &door : m_doorsounds ) {
                        // apply configuration to all defined doors, but preserve their individual offsets
                        auto const dooroffset { door.step_close.offset() };
                        door.step_close = soundtemplate;
                        door.step_close.offset( dooroffset );
                    }
                }

                else if( token == "unloading:" ) {
                    m_exchangesounds.unloading.deserialize( parser, sound_type::single );
                    m_exchangesounds.unloading.owner( this );
                }

                else if( token == "loading:" ) {
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
                    sound_source noisetemplate { sound_placement::external, EU07_SOUND_RUNNINGNOISECUTOFFRANGE };
                    noisetemplate.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency, MoverParameters->Vmax );
                    noisetemplate.owner( this );

                    noisetemplate.m_amplitudefactor /= ( 1 + MoverParameters->Vmax );
                    noisetemplate.m_frequencyfactor /= ( 1 + MoverParameters->Vmax );

                    if( true == m_bogiesounds.empty() ) {
                        // fallback for cases without specified noise locations, convert sound template to a single sound source
                        m_bogiesounds.emplace_back( noisetemplate );
                    }
                    else {
                        // apply configuration to all defined bogies
                        for( auto &bogie : m_bogiesounds ) {
                            // combine potential x- and y-axis offsets of the sound template with z-axis offsets of individual motors
                            auto bogieoffset { noisetemplate.offset() };
                            bogieoffset.z = bogie.offset().z;
                            bogie = noisetemplate;
                            bogie.offset( bogieoffset );
                        }
                    }
                }

                else if( token == "wheelflat:" ) {
                    // szum podczas jazdy:
                    m_wheelflat.deserialize( parser, sound_type::single, sound_parameters::frequency );
                    m_wheelflat.owner( this );
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
                            door.sDepartureSignal.offset( location );
                            door.rsDoorClose.offset( location );
                            door.rsDoorOpen.offset( location );
                            door.lock.offset( location );
                            door.unlock.offset( location );
                            door.step_close.offset( location );
                            door.step_open.offset( location );
                            m_doorsounds.emplace_back( door );
                        }
                        if( ( sides == "both" )
                         || ( sides == "right" ) ) {
                            // ...and right
                            auto const location { glm::vec3 { MoverParameters->Dim.W * -0.5f, MoverParameters->Dim.H * 0.5f, offset } };
                            door.sDepartureSignal.offset( location );
                            door.rsDoorClose.offset( location );
                            door.rsDoorOpen.offset( location );
                            door.lock.offset( location );
                            door.unlock.offset( location );
                            door.step_close.offset( location );
                            door.step_open.offset( location );
                            m_doorsounds.emplace_back( door );
                        }
                    }
                }

                else if( token == "tractionmotors:" ) {
                    // a list of offsets along vehicle's z-axis; followed with "end"
                    while( ( ( token = parser.getToken<std::string>() ) != "" )
                        && ( token != "end" ) ) {
                        // vehicle faces +Z in 'its' space, for motor locations negative value means ahead of centre
                        auto const offset { std::atof( token.c_str() ) * -1.f };
                        // NOTE: we skip setting owner of the sounds, it'll be done during individual sound deserialization
                        sound_source motor { sound_placement::external }; // generally traction motor
                        sound_source motorblower { sound_placement::engine }; // associated motor blowers
                        // add entry to the list
                        auto const location { glm::vec3 { 0.f, 0.f, offset } };
                        motor.offset( location );
                        m_powertrainsounds.motors.emplace_back( motor );
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
                        m_bogiesounds.emplace_back( bogienoise );
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
                }
                else if( token == "engageslippery:" ) {
                    // tarcie tarcz sprzegla:
                    m_powertrainsounds.rsEngageSlippery.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency );
                    m_powertrainsounds.rsEngageSlippery.owner( this );

                    m_powertrainsounds.rsEngageSlippery.m_frequencyfactor /= ( 1 + MoverParameters->nmax );
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
                // braking sounds
                else if( token == "brakesound:" ) {
                    // hamowanie zwykle:
                    rsBrake.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency );
                    rsBrake.owner( this );
                    // NOTE: can't pre-calculate amplitude normalization based on max brake force, as this varies depending on vehicle speed
                    rsBrake.m_frequencyfactor /= ( 1 + MoverParameters->Vmax );
                }
                else if( token == "slipperysound:" ) {
                    // sanie:
                    rsSlippery.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                    rsSlippery.owner( this );

                    rsSlippery.m_amplitudefactor /= ( 1 + MoverParameters->Vmax );
                }
                // coupler sounds
                else if( token == "couplerattach:" ) {
                    // laczenie:
                    sound_source couplerattach { sound_placement::external };
                    couplerattach.deserialize( parser, sound_type::single );
                    couplerattach.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbCouplerAttach = couplerattach;
                    }
                }
                else if( token == "couplerdetach:" ) {
                    // rozlaczanie:
                    sound_source couplerdetach { sound_placement::external };
                    couplerdetach.deserialize( parser, sound_type::single );
                    couplerdetach.owner( this );
                    for( auto &couplersounds : m_couplersounds ) {
                        couplersounds.dsbCouplerDetach = couplerdetach;
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

                else if( token == "jointcabs:" ) {
                    parser.getTokens();
                    parser >> JointCabs;
                }

            } while( token != "" );

        } // internaldata:

	} while( token != "" );

    if( !iAnimations ) {
        // if the animations weren't defined the model is likely to be non-functional. warrants a warning.
        ErrorLog( "Animations tag is missing from the .mmd file \"" + asFileName + "\"" );
    }

    // assign default samples to sound emitters which weren't included in the config file
    // engine
    if( MoverParameters->Power > 0 ) {
        if( true == m_powertrainsounds.dsbWejscie_na_bezoporow.empty() ) {
            // hunter-111211: domyslne, gdy brak
            m_powertrainsounds.dsbWejscie_na_bezoporow.deserialize( "wejscie_na_bezoporow.wav", sound_type::single );
            m_powertrainsounds.dsbWejscie_na_bezoporow.owner( this );
        }
        if( true == m_powertrainsounds.motor_parallel.empty() ) {
            m_powertrainsounds.motor_parallel.deserialize( "wescie_na_drugi_uklad.wav", sound_type::single );
            m_powertrainsounds.motor_parallel.owner( this );
        }
    }
    // braking sounds
    if( true == rsUnbrake.empty() ) {
        rsUnbrake.deserialize( "[1007]estluz.wav", sound_type::single );
        rsUnbrake.owner( this );
    }
    // couplers
    for( auto &couplersounds : m_couplersounds ) {
        if( true == couplersounds.dsbCouplerAttach.empty() ) {
            couplersounds.dsbCouplerAttach.deserialize( "couplerattach.wav", sound_type::single );
            couplersounds.dsbCouplerAttach.owner( this );
        }
        if( true == couplersounds.dsbCouplerDetach.empty() ) {
            couplersounds.dsbCouplerDetach.deserialize( "couplerdetach.wav", sound_type::single );
            couplersounds.dsbCouplerDetach.owner( this );
        }
        if( true == couplersounds.dsbCouplerStretch.empty() ) {
            couplersounds.dsbCouplerStretch.deserialize( "en57_couplerstretch.wav", sound_type::single );
            couplersounds.dsbCouplerStretch.owner( this );
        }
        if( true == couplersounds.dsbBufferClamp.empty() ) {
            couplersounds.dsbBufferClamp.deserialize( "en57_bufferclamp.wav", sound_type::single );
            couplersounds.dsbBufferClamp.owner( this );
        }
    }
    // other sounds
    if( true == m_wheelflat.empty() ) {
        m_wheelflat.deserialize( "lomotpodkucia.wav 0.23 0.0", sound_type::single, sound_parameters::frequency );
        m_wheelflat.owner( this );
    }
    if( true == rscurve.empty() ) {
        // hunter-111211: domyslne, gdy brak
        rscurve.deserialize( "curve.wav", sound_type::single );
        rscurve.owner( this );
    }


    if (mdModel)
        mdModel->Init(); // obrócenie modelu oraz optymalizacja, również zapisanie binarnego
    if (mdLoad)
        mdLoad->Init();
    if (mdLowPolyInt)
        mdLowPolyInt->Init();

    Global.asCurrentTexturePath = szTexturePath; // kiedyś uproszczone wnętrze mieszało tekstury nieba
    Global.asCurrentDynamicPath = "";

    // position sound emitters which weren't defined in the config file
    // engine sounds, centre of the vehicle
    auto const enginelocation { glm::vec3 {0.f, MoverParameters->Dim.H * 0.5f, 0.f } };
    m_powertrainsounds.position( enginelocation );
    // other engine compartment sounds
    auto const nullvector { glm::vec3() };
    std::vector<sound_source *> enginesounds = {
        &sConverter, &sCompressor, &sSmallCompressor, &sHeater
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
    m_couplersounds[ side::front ].dsbCouplerAttach.offset( frontcoupleroffset );
    m_couplersounds[ side::front ].dsbCouplerDetach.offset( frontcoupleroffset );
    m_couplersounds[ side::front ].dsbCouplerStretch.offset( frontcoupleroffset );
    m_couplersounds[ side::front ].dsbCouplerStretch_loud.offset( frontcoupleroffset );
    m_couplersounds[ side::front ].dsbBufferClamp.offset( frontcoupleroffset );
    m_couplersounds[ side::front ].dsbBufferClamp_loud.offset( frontcoupleroffset );
    auto const rearcoupleroffset { glm::vec3{ 0.f, 1.f, MoverParameters->Dim.L * -0.5f } };
    m_couplersounds[ side::rear ].dsbCouplerAttach.offset( rearcoupleroffset );
    m_couplersounds[ side::rear ].dsbCouplerDetach.offset( rearcoupleroffset );
    m_couplersounds[ side::rear ].dsbCouplerStretch.offset( rearcoupleroffset );
    m_couplersounds[ side::rear ].dsbCouplerStretch_loud.offset( rearcoupleroffset );
    m_couplersounds[ side::rear ].dsbBufferClamp.offset( rearcoupleroffset );
    m_couplersounds[ side::rear ].dsbBufferClamp_loud.offset( rearcoupleroffset );
}

TModel3d *
TDynamicObject::LoadMMediaFile_mdload( std::string const &Name ) const {

    if( Name.empty() ) { return nullptr; }

    // try first specialized version of the load model, vehiclename_loadname
    TModel3d *loadmodel { nullptr };
    auto const specializedloadfilename { asBaseDir + MoverParameters->TypeName + "_" + Name };
    if( ( true == FileExists( specializedloadfilename + ".e3d" ) )
     || ( true == FileExists( specializedloadfilename + ".t3d" ) ) ) {
        loadmodel = TModelsManager::GetModel( specializedloadfilename, true );
    }
    if( loadmodel == nullptr ) {
        // if this fails, try generic load model
        loadmodel = TModelsManager::GetModel( asBaseDir + Name, true );
    }
    return loadmodel;
}

//---------------------------------------------------------------------------
void TDynamicObject::RadioStop()
{ // zatrzymanie pojazdu
    if( Mechanik ) {
        // o ile ktoś go prowadzi
        if( ( MoverParameters->SecuritySystem.RadioStop )
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
		MoverParameters->ConvOvldFlag = true;
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
            if (!MoverParameters->ActiveDir) // jeśli nie ma ustawionego kierunku
            { // jeśli ma zarówno światła jak i końcówki, ustalić, czy jest w stanie
                // aktywnym
                // np. lokomotywa na zimno będzie mieć końcówki a nie światła
                rear = light::rearendsignals; // tablice blaszane
                // trzeba to uzależnić od "załączenia baterii" w pojeździe
            }
        if( rear == ( light::redmarker_left | light::redmarker_right | light::rearendsignals ) ) // jeśli nadal obydwie możliwości
            if( iInventory[
                ( iDirection ?
                    side::rear :
                    side::front ) ] & ( light::redmarker_left | light::redmarker_right ) ) {
                // czy ma jakieś światła czerowone od danej strony
                rear = ( light::redmarker_left | light::redmarker_right ); // dwa światła czerwone
            }
            else {
                rear = light::rearendsignals; // tablice blaszane
            }
    }
    if (iDirection) // w zależności od kierunku pojazdu w składzie
    { // jesli pojazd stoi sprzęgiem 0 w stronę czoła
        if (head >= 0)
            iLights[0] = head;
        if (rear >= 0)
            iLights[1] = rear;
    }
    else
    { // jak jest odwrócony w składzie (-1), to zapalamy odwrotnie
        if (head >= 0)
            iLights[1] = head;
        if (rear >= 0)
            iLights[0] = rear;
    }
};

int TDynamicObject::DirectionSet(int d)
{ // ustawienie kierunku w składzie (wykonuje AI)
    iDirection = d > 0 ? 1 : 0; // d:1=zgodny,-1=przeciwny; iDirection:1=zgodny,0=przeciwny;
    if (MyTrack)
    { // podczas wczytywania wstawiane jest AI, ale może jeszcze nie
        // być toru
        // AI ustawi kierunek ponownie po uruchomieniu silnika
        if (iDirection) // jeśli w kierunku Coupler 0
        {
            if (MoverParameters->Couplers[0].CouplingFlag ==
                ctrain_virtual) // brak pojazdu podpiętego?
                ABuScanObjects(1, 300); // szukanie czegoś do podłączenia
        }
        else if (MoverParameters->Couplers[1].CouplingFlag ==
                 ctrain_virtual) // brak pojazdu podpiętego?
            ABuScanObjects(-1, 300);
    }
    return 1 - (iDirection ? NextConnectedNo : PrevConnectedNo); // informacja o położeniu
    // następnego
};

TDynamicObject * TDynamicObject::PrevAny()
{ // wskaźnik na poprzedni,
    // nawet wirtualny
    return iDirection ? PrevConnected : NextConnected;
};
TDynamicObject * TDynamicObject::Prev()
{
    if (MoverParameters->Couplers[iDirection ^ 1].CouplingFlag)
        return iDirection ? PrevConnected : NextConnected;
    return NULL; // gdy sprzęg wirtualny, to jakby nic nie było
};
TDynamicObject * TDynamicObject::Next()
{
    if (MoverParameters->Couplers[iDirection].CouplingFlag)
        return iDirection ? NextConnected : PrevConnected;
    return NULL; // gdy sprzęg wirtualny, to jakby nic nie było
};
TDynamicObject * TDynamicObject::PrevC(int C)
{
	if (MoverParameters->Couplers[iDirection ^ 1].CouplingFlag & C)
		return iDirection ? PrevConnected : NextConnected;
	return NULL; // gdy sprzęg wirtualny, to jakby nic nie było
};
TDynamicObject * TDynamicObject::NextC(int C)
{
    if (MoverParameters->Couplers[iDirection].CouplingFlag & C)
        return iDirection ? NextConnected : PrevConnected;
    return NULL; // gdy sprzęg inny, to jakby nic nie było
};
double TDynamicObject::NextDistance(double d)
{ // ustalenie odległości do
    // następnego pojazdu, potrzebne
    // do wstecznego skanowania
    if (!MoverParameters->Couplers[iDirection].Connected)
        return d; // jeśli nic nie ma, zwrócenie domyślnej wartości
    if ((d <= 0.0) || (MoverParameters->Couplers[iDirection].CoupleDist < d))
        return MoverParameters->Couplers[iDirection].Dist;
    else
        return d;
};

TDynamicObject * TDynamicObject::Neightbour(int &dir)
{ // ustalenie następnego (1) albo poprzedniego (0) w składzie bez
    // względu na prawidłowość
    // iDirection
    int d = dir; // zapamiętanie kierunku
    dir = 1 - (dir ? NextConnectedNo : PrevConnectedNo); // nowa wartość
    return (d ? (MoverParameters->Couplers[1].CouplingFlag ? NextConnected : NULL) :
                (MoverParameters->Couplers[0].CouplingFlag ? PrevConnected : NULL));
};

void TDynamicObject::CoupleDist()
{ // obliczenie odległości sprzęgów
    if (MyTrack ? (MyTrack->iCategoryFlag & 1) :
                  true) // jeśli nie ma przypisanego toru, to liczyć jak dla kolei
    { // jeśli jedzie po szynach (również unimog), liczenie kul wystarczy
        MoverParameters->SetCoupleDist();
    }
    else
    { // na drodze trzeba uwzględnić wektory ruchu
        double d0 = MoverParameters->Couplers[0].CoupleDist;
        // double d1=MoverParameters->Couplers[1].CoupleDist; //sprzęg z tyłu
        // samochodu można olać,
        // dopóki nie jeździ na wstecznym
        Math3D::vector3 p1, p2;
        double d, w; // dopuszczalny dystans w poprzek
        MoverParameters->SetCoupleDist(); // liczenie standardowe
        if (MoverParameters->Couplers[0].Connected) // jeśli cokolwiek podłączone
            if (MoverParameters->Couplers[0].CouplingFlag == 0) // jeśli wirtualny
                if (MoverParameters->Couplers[0].CoupleDist < 300.0) // i mniej niż 300m
                { // przez MoverParameters->Couplers[0].Connected nie da się dostać do
                    // DynObj, stąd
                    // prowizorka
                    // WriteLog("Collision of
                    // "+AnsiString(MoverParameters->Couplers[0].CoupleDist)+"m detected
                    // by
                    // "+asName+":0.");
                    w = 0.5 * (MoverParameters->Couplers[0].Connected->Dim.W +
                               MoverParameters->Dim.W); // minimalna odległość minięcia
                    d = -DotProduct(vLeft, vCoulpler[0]); // odległość prostej ruchu od początku
                    // układu współrzędnych
                    d = fabs(
                        DotProduct(vLeft,
                                   ((TMoverParameters *)(MoverParameters->Couplers[0].Connected))
                                       ->vCoulpler[MoverParameters->Couplers[0].ConnectedNr]) +
                        d);
                    // WriteLog("Distance "+AnsiString(d)+"m from "+asName+":0.");
                    if (d > w)
                        MoverParameters->Couplers[0].CoupleDist =
                            (d0 < 10 ? 50 : d0); // przywrócenie poprzedniej
                }
        if (MoverParameters->Couplers[1].Connected) // jeśli cokolwiek podłączone
            if (MoverParameters->Couplers[1].CouplingFlag == 0) // jeśli wirtualny
                if (MoverParameters->Couplers[1].CoupleDist < 300.0) // i mniej niż 300m
                {
                    // WriteLog("Collision of
                    // "+AnsiString(MoverParameters->Couplers[1].CoupleDist)+"m detected
                    // by
                    // "+asName+":1.");
                    w = 0.5 * (MoverParameters->Couplers[1].Connected->Dim.W +
                               MoverParameters->Dim.W); // minimalna odległość minięcia
                    d = -DotProduct(vLeft, vCoulpler[1]); // odległość prostej ruchu od początku
                    // układu współrzędnych
                    d = fabs(
                        DotProduct(vLeft,
                                   ((TMoverParameters *)(MoverParameters->Couplers[1].Connected))
                                       ->vCoulpler[MoverParameters->Couplers[1].ConnectedNr]) +
                        d);
                    // WriteLog("Distance "+AnsiString(d)+"m from "+asName+":1.");
                    if (d > w)
                        MoverParameters->Couplers[0].CoupleDist =
                            (d0 < 10 ? 50 : d0); // przywrócenie poprzedniej
                }
    }
};

TDynamicObject * TDynamicObject::ControlledFind()
{ // taka proteza:
    // chcę podłączyć
    // kabinę EN57
    // bezpośrednio z
    // silnikowym, aby
    // nie robić tego
    // przez
    // ukrotnienie
    // drugi silnikowy i tak musi być ukrotniony, podobnie jak kolejna jednostka
    // lepiej by było przesyłać komendy sterowania, co jednak wymaga przebudowy
    // transmisji komend
    // (LD)
    // problem się robi ze światłami, które będą zapalane w silnikowym, ale muszą
    // świecić się w
    // rozrządczych
    // dla EZT światłą czołowe będą "zapalane w silnikowym", ale widziane z
    // rozrządczych
    // również wczytywanie MMD powinno dotyczyć aktualnego członu
    // problematyczna może być kwestia wybranej kabiny (w silnikowym...)
    // jeśli silnikowy będzie zapięty odwrotnie (tzn. -1), to i tak powinno
    // jeździć dobrze
    // również hamowanie wykonuje się zaworem w członie, a nie w silnikowym...
    TDynamicObject *d = this; // zaczynamy od aktualnego
    if( d->MoverParameters->TrainType & dt_EZT ) {
        // na razie dotyczy to EZT
        if( ( d->NextConnected != nullptr )
         && ( true == TestFlag( d->MoverParameters->Couplers[ 1 ].AllowedFlag, coupling::permanent ) ) ) {
            // gdy jest człon od sprzęgu 1, a sprzęg łączony warsztatowo (powiedzmy)
            if( ( d->MoverParameters->Power < 1.0 )
             && ( d->NextConnected->MoverParameters->Power > 1.0 ) ) {
                // my nie mamy mocy, ale ten drugi ma
                d = d->NextConnected; // będziemy sterować tym z mocą
            }
        }
        else if( ( d->PrevConnected != nullptr )
              && ( true == TestFlag( d->MoverParameters->Couplers[ 0 ].AllowedFlag, coupling::permanent ) ) ) {
            // gdy jest człon od sprzęgu 0, a sprzęg łączony warsztatowo (powiedzmy)
            if( ( d->MoverParameters->Power < 1.0 )
             && ( d->PrevConnected->MoverParameters->Power > 1.0 ) ) {
                // my nie mamy mocy, ale ten drugi ma
                d = d->PrevConnected; // będziemy sterować tym z mocą
            }
        }
    }
    return d;
};
//---------------------------------------------------------------------------

void TDynamicObject::ParamSet(int what, int into)
{ // ustawienie lokalnego parametru (what) na stan (into)
    switch (what & 0xFF00)
    {
    case 0x0100: // to np. są drzwi, bity 0..7 określają numer 1..254 albo maskę
        // dla 8 różnych
        if (what & 1) // na razie mamy lewe oraz prawe, czyli używamy maskę 1=lewe,
            // 2=prawe, 3=wszystkie
            if (MoverParameters->DoorLeftOpened)
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
            if (MoverParameters->DoorRightOpened)
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

void TDynamicObject::DestinationSet(std::string to, std::string numer)
{ // ustawienie stacji docelowej oraz wymiennej tekstury 4, jeśli istnieje plik
    // w zasadzie, to każdy wagon mógłby mieć inną stację docelową
    // zwłaszcza w towarowych, pod kątem zautomatyzowania maewrów albo pracy górki
    // ale to jeszcze potrwa, zanim będzie możliwe, na razie można wpisać stację z
    // rozkładu
    if( std::abs( m_materialdata.multi_textures ) >= 4 ) {
        // jak są 4 tekstury wymienne, to nie zmieniać rozkładem
        return;
    }
	numer = Bezogonkow(numer);
    asDestination = to;
    to = Bezogonkow(to); // do szukania pliku obcinamy ogonki
    if( true == to.empty() ) {
        to = "nowhere";
    }
    // destination textures are kept in the vehicle's directory so we point the current texture path there
    auto const currenttexturepath { Global.asCurrentTexturePath };
    Global.asCurrentTexturePath = asBaseDir;
    // now see if we can find any version of the texture
    std::vector<std::string> destinations = {
        numer + '@' + MoverParameters->TypeName,
        numer,
        to + '@' + MoverParameters->TypeName,
        to,
        "nowhere" + '@' + MoverParameters->TypeName,
        "nowhere" };

    for( auto const &destination : destinations ) {

        auto material = TextureTest( ToLower( destination ) );
        if( false == material.empty() ) {
            m_materialdata.replacable_skins[ 4 ] = GfxRenderer.Fetch_Material( material );
            break;
        }
    }
    // whether we got anything, restore previous texture path
    Global.asCurrentTexturePath = currenttexturepath;
};

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
            ctOwner->iOverheadZero |=
                iOverheadMask; // ustawienie bitu - ma jechać bez pobierania prądu
            ctOwner->iOverheadDown |= iOverheadMask; // ustawienie bitu - ma opuścić pantograf
        }
        else
        { // jazda bezprądowa z podniesionym pantografem
            ctOwner->iOverheadZero |=
                iOverheadMask; // ustawienie bitu - ma jechać bez pobierania prądu
            ctOwner->iOverheadDown &= ~iOverheadMask; // zerowanie bitu - może podnieść pantograf
        }
    }
};

// returns type of the nearest functional power source present in the trainset
TPowerSource
TDynamicObject::ConnectedEnginePowerSource( TDynamicObject const *Caller ) const {

    // if there's engine in the current vehicle, that's good enough...
    if( MoverParameters->EnginePowerSource.SourceType != TPowerSource::NotDefined ) {
        return MoverParameters->EnginePowerSource.SourceType;
    }
    // ...otherwise check rear first...
    // NOTE: the order should be reversed in flipped vehicles, but we ignore this out of laziness
    if( ( nullptr != NextConnected )
     && ( NextConnected != Caller )
     && ( ( MoverParameters->Couplers[1].CouplingFlag & ctrain_controll ) == ctrain_controll ) ) {

        auto source = NextConnected->ConnectedEnginePowerSource( this );
        if( source != TPowerSource::NotDefined ) {

            return source;
            }
        }
    // ...then rear...
    if( ( nullptr != PrevConnected )
        && ( PrevConnected != Caller )
        && ( ( MoverParameters->Couplers[ 0 ].CouplingFlag & ctrain_controll ) == ctrain_controll ) ) {

        auto source = PrevConnected->ConnectedEnginePowerSource( this );
        if( source != TPowerSource::NotDefined ) {

            return source;
        }
    }
    // ...if we're still here, report lack of power source
    return MoverParameters->EnginePowerSource.SourceType;
}

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
    if( false == Global.iSlowMotion ) { // musi być pełna prędkość

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

        auto const iVel { std::min( GetVelocity(), 150.0 ) };
        if( iVel > 0.5 ) {
            // acceleration-driven base shake
            shakevector += Math3D::vector3(
                -MoverParameters->AccN * Timedelta * 5.0, // highlight side sway
                -MoverParameters->AccVert * Timedelta,
                -MoverParameters->AccSVBased * Timedelta * 1.25 ); // accent acceleration/deceleration
        }

        auto shake { 1.25 * ShakeSpring.ComputateForces( shakevector, ShakeState.offset ) };

        if( Random( iVel ) > 25.0 ) {
            // extra shake at increased velocity
            shake += ShakeSpring.ComputateForces(
                Math3D::vector3(
                ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * BaseShake.jolt_scale.x,
                ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * BaseShake.jolt_scale.y,
                ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * BaseShake.jolt_scale.z )
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
        &engine, &engine_ignition, &engine_shutdown, &engine_revving, &engine_turbo, &oil_pump, &radiator_fan, &radiator_fan_aux,
        &transmission, &rsEngageSlippery
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
                          + 0.75 * ( Vehicle.enrot * 60 ) / ( Vehicle.DElist[ Vehicle.MainCtrlPosNo ].RPM ) );
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
                        auto const idlerevolutionsthreshold { 1.01 * (
                            Vehicle.EngineType == TEngineType::DieselElectric ?
                                Vehicle.DElist[ 0 ].RPM :
                                Vehicle.dizel_nmin * 60 ) };

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
    engine_volume = interpolate( engine_volume, volume, 0.25 );
    if( engine_volume < 0.05 ) {
        engine.stop();
    }

    // youBy - przenioslem, bo diesel tez moze miec turbo
    if( Vehicle.TurboTest > 0 ) {
        // udawanie turbo:
        auto const goalpitch { std::max( 0.025, ( engine_volume + engine_turbo.m_frequencyoffset ) * engine_turbo.m_frequencyfactor ) };
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

    // motor sounds
    volume = 0.0;
    if( ( true == Vehicle.Mains )
     && ( false == motors.empty() ) ) {

        if( std::abs( Vehicle.enrot ) > 0.01 ) {

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

                    auto const volumevariation { Random( 100 ) * Vehicle.enrot / ( 1 + Vehicle.nmax ) };
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

            if( motor_volume >= 0.05 ) {
                // apply calculated parameters to all motor instances
                for( auto &motor : motors ) {
                    motor
                        .pitch( frequency )
                        .gain( motor_volume )
                        .play( sound_flags::exclusive | sound_flags::looping );
                }
            }
        }
        else {
            // stop all motor instances
            for( auto &motor : motors ) {
                motor.stop();
            }
        }
    }
    motor_volume = interpolate( motor_volume, volume, 0.25 );
    if( motor_volume < 0.05 ) {
        // stop all motor instances
        for( auto &motor : motors ) {
            motor.stop();
        }
    }
    // motor blowers
    if( false == motorblowers.empty() ) {
        for( auto &blowersound : motorblowers ) {
            // match the motor blower and the sound source based on whether they're located in the front or the back of the vehicle
            auto const &blower { Vehicle.MotorBlowers[ ( blowersound.offset().z > 0 ? side::front : side::rear ) ] };
            if( blower.revolutions > 1 ) {

                blowersound
                    .pitch(
                        true == blowersound.is_combined() ?
                            blower.revolutions * 0.01f :
                            blowersound.m_frequencyoffset + blowersound.m_frequencyfactor * blower.revolutions )
                    .gain( blowersound.m_amplitudeoffset + blowersound.m_amplitudefactor * blower.revolutions )
                    .play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                blowersound.stop();
            }
        }
    }

    // inverter sounds
    if( Vehicle.EngineType == TEngineType::ElectricInductionMotor ) {
        if( Vehicle.InverterFrequency > 0.1 ) {

            volume = inverter.m_amplitudeoffset + inverter.m_amplitudefactor * std::sqrt( std::abs( Vehicle.dizel_fill ) );

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
        vehicle->CoupleDist();
    }
    if( Iterationcount > 1 ) {
        // ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
        for( int iteration = 0; iteration < ( Iterationcount - 1 ); ++iteration ) {
            for( auto *vehicle : m_items ) {
                vehicle->UpdateForce( Deltatime, Deltatime, false );
            }
            for( auto *vehicle : m_items ) {
                vehicle->FastUpdate( Deltatime );
            }
        }
    }

    auto const totaltime { Deltatime * Iterationcount }; // całkowity czas

    for( auto *vehicle : m_items ) {
        vehicle->UpdateForce( Deltatime, totaltime, true );
    }
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
        auto pantograph { Vehicle->pants[ pantographindex ].fParamPants };
        if( true == (
                pantographindex == side::front ?
                    Vehicle->MoverParameters->PantFrontUp :
                    Vehicle->MoverParameters->PantRearUp ) ) {
            // jeśli pantograf podniesiony
            auto const pant0 { position + ( vLeft * pantograph->vPos.z ) + ( vUp * pantograph->vPos.y ) + ( vFront * pantograph->vPos.x ) };
            if( pantograph->hvPowerWire != nullptr ) {
                // jeżeli znamy drut z poprzedniego przebiegu
                for( int attempts = 0; attempts < 30; ++attempts ) {
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
                SafeDelete( simulation::Train );
            }
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
