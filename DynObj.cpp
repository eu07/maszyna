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

#include "logs.h"
#include "MdlMngr.h"
#include "Timer.h"
#include "Usefull.h"
// McZapkie-260202
#include "Globals.h"
#include "Texture.h"
#include "AirCoupler.h"

#include "TractionPower.h"
#include "Ground.h" //bo Global::pGround->bDynamicRemove
#include "Event.h"
#include "Driver.h"
#include "Camera.h" //bo likwidujemy trzęsienie
#include "Console.h"
#include "Traction.h"

// Ra: taki zapis funkcjonuje lepiej, ale może nie jest optymalny
#define vWorldFront Math3D::vector3(0, 0, 1)
#define vWorldUp Math3D::vector3(0, 1, 0)
#define vWorldLeft CrossProduct(vWorldUp, vWorldFront)

// Ra: bo te poniżej to się powielały w każdym module odobno
// vector3 vWorldFront=vector3(0,0,1);
// vector3 vWorldUp=vector3(0,1,0);
// vector3 vWorldLeft=CrossProduct(vWorldUp,vWorldFront);

#define M_2PI 6.283185307179586476925286766559;
const float maxrot = (M_PI / 3.0); // 60°

//---------------------------------------------------------------------------
void TAnimPant::AKP_4E()
{ // ustawienie wymiarów dla pantografu AKP-4E
    vPos = vector3(0, 0, 0); // przypisanie domyśnych współczynników do pantografów
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
    fWidthExtra = 0.381; //(2.032m-1.027)/2
    // poza obszarem roboczym jest aproksymacja łamaną o 5 odcinkach
    fHeightExtra[0] = 0.0; //+0.0762
    fHeightExtra[1] = -0.01; //+0.1524
    fHeightExtra[2] = -0.03; //+0.2286
    fHeightExtra[3] = -0.07; //+0.3048
    fHeightExtra[4] = -0.15; //+0.3810
};
//---------------------------------------------------------------------------
int TAnim::TypeSet(int i, int fl)
{ // ustawienie typu animacji i zależnej od
    // niego ilości animowanych submodeli
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
    default:
        iFlags = 0;
    }
    yUpdate = nullptr;
    return iFlags & 15; // ile wskaźników rezerwować dla danego typu animacji
};
TAnim::TAnim()
{ // potrzebne to w ogóle?
    iFlags = -1; // nieznany typ - destruktor nic nie usuwa
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
    case 0x60: // 6-tłok i rozrząd
        break;
    }
};
void TAnim::Parovoz(){
    // animowanie tłoka i rozrządu parowozu
};
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
    float eq = 0, am = 0;

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

void TDynamicObject::ABuSetModelShake(vector3 mShake)
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
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), *pAnim->dWheelAngle);
};

void TDynamicObject::UpdateBoogie(TAnim *pAnim)
{ // animacja wózka
    pAnim->smAnimated->SetRotate(float3(1, 0, 0), *pAnim->dWheelAngle);
};

void TDynamicObject::UpdateDoorTranslate(TAnim *pAnim)
{ // animacja drzwi - przesuw
    // WriteLog("Dla drzwi nr:", i);
    // WriteLog("Wspolczynnik", DoorSpeedFactor[i]);
    // Ra: te współczynniki są bez sensu, bo modyfikują wektor przesunięcia
    // w efekcie drzwi otwierane na zewnątrz będą odlatywac dowolnie daleko :)
    // ograniczyłem zakres ruchu funkcją max
    if (pAnim->smAnimated)
    {
        if (pAnim->iNumber & 1)
            pAnim->smAnimated->SetTranslate(
                vector3(0, 0, Min0R(dDoorMoveR * pAnim->fSpeed, dDoorMoveR)));
        else
            pAnim->smAnimated->SetTranslate(
                vector3(0, 0, Min0R(dDoorMoveL * pAnim->fSpeed, dDoorMoveL)));
    }
};

void TDynamicObject::UpdateDoorRotate(TAnim *pAnim)
{ // animacja drzwi - obrót
    if (pAnim->smAnimated)
    { // if (MoverParameters->DoorOpenMethod==2) //obrotowe
        // albo dwójłomne (trzeba kombinowac
        // submodelami i ShiftL=90,R=180)
        if (pAnim->iNumber & 1)
            pAnim->smAnimated->SetRotate(float3(1, 0, 0), dDoorMoveR);
        else
            pAnim->smAnimated->SetRotate(float3(1, 0, 0), dDoorMoveL);
    }
};

void TDynamicObject::UpdateDoorFold(TAnim *pAnim)
{ // animacja drzwi - obrót
    if (pAnim->smAnimated)
    { // if (MoverParameters->DoorOpenMethod==2) //obrotowe
        // albo dwójłomne (trzeba kombinowac
        // submodelami i ShiftL=90,R=180)
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
            // SubModel->SetRotate(float3(0,1,0),fValue*360.0);
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
};

void TDynamicObject::UpdateDoorPlug(TAnim *pAnim)
{ // animacja drzwi - odskokprzesuw
    if (pAnim->smAnimated)
    {
        if (pAnim->iNumber & 1)
            pAnim->smAnimated->SetTranslate(
                vector3(Min0R(dDoorMoveR * 2, MoverParameters->DoorMaxPlugShift), 0,
                        Max0R(0, Min0R(dDoorMoveR * pAnim->fSpeed, dDoorMoveR) -
                                     MoverParameters->DoorMaxPlugShift * 0.5f)));
        else
            pAnim->smAnimated->SetTranslate(
                vector3(Min0R(dDoorMoveL * 2, MoverParameters->DoorMaxPlugShift), 0,
                        Max0R(0, Min0R(dDoorMoveL * pAnim->fSpeed, dDoorMoveL) -
                                     MoverParameters->DoorMaxPlugShift * 0.5f)));
    }
};

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

// ABu 29.01.05 przeklejone z render i renderalpha: *********************
void __inline TDynamicObject::ABuLittleUpdate(double ObjSqrDist)
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

    if (ObjSqrDist < 160000) // gdy bliżej niż 400m
    {
        for (int i = 0; i < iAnimations; ++i) // wykonanie kolejnych animacji
            if (ObjSqrDist < pAnimations[i].fMaxDist)
                if (pAnimations[i].yUpdate) // jeśli zdefiniowana funkcja
                    pAnimations[ i ].yUpdate( &pAnimations[i] ); // aktualizacja animacji (położenia submodeli
/*
                    pAnimations[i].yUpdate(pAnimations +
                                           i); // aktualizacja animacji (położenia submodeli
*/
        if( ObjSqrDist < 2500 ) // gdy bliżej niż 50m
        {
            // ABu290105: rzucanie pudlem
            // te animacje wymagają bananów w modelach!
            mdModel->GetSMRoot()->SetTranslate(modelShake);
            if (mdKabina)
                mdKabina->GetSMRoot()->SetTranslate(modelShake);
            if (mdLoad)
                mdLoad->GetSMRoot()->SetTranslate(modelShake + vFloor);
            if (mdLowPolyInt)
                mdLowPolyInt->GetSMRoot()->SetTranslate(modelShake);
            if (mdPrzedsionek)
                mdPrzedsionek->GetSMRoot()->SetTranslate(modelShake);
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
                btCoupler1.TurnOn();
                btnOn = true;
            }
            // else btCoupler1.TurnOff();
            if ((TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_coupler)) &&
                (MoverParameters->Couplers[1].Render))
            {
                btCoupler2.TurnOn();
                btnOn = true;
            }
            // else btCoupler2.TurnOff();
            //********************************************************************************
            // przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie
            // 'render' - juz
            // nie
            // przewody powietrzne, yB: decyzja na podstawie polaczen w t3d
            if (Global::bnewAirCouplers)
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
                // else
                //{
                // btCPneumatic1.TurnOff();
                // btCPneumatic1r.TurnOff();
                //}

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
                // else
                //{
                // btCPneumatic2.TurnOff();
                // btCPneumatic2r.TurnOff();
                //}

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
                // else
                //{
                // btPneumatic1.TurnOff();
                // btPneumatic1r.TurnOff();
                //}

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
                // else
                //{
                // btPneumatic2.TurnOff();
                // btPneumatic2r.TurnOff();
                //}
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
                // else
                //{
                // btCPneumatic1.TurnOff();
                // btCPneumatic1r.TurnOff();
                //}

                if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_pneumatic))
                {
                    if (MoverParameters->Couplers[1].Render)
                        btCPneumatic2.TurnOn();
                    else
                        btCPneumatic2r.TurnOn();
                    btnOn = true;
                }
                // else
                //{
                // btCPneumatic2.TurnOff();
                // btCPneumatic2r.TurnOff();
                //}

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
                // else
                //{
                // btPneumatic1.TurnOff();
                // btPneumatic1r.TurnOff();
                //}

                if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_scndpneumatic))
                {
                    if (MoverParameters->Couplers[1].Render)
                        btPneumatic2.TurnOn();
                    else
                        btPneumatic2r.TurnOn();
                    btnOn = true;
                }
                // else
                //{
                // btPneumatic2.TurnOff();
                // btPneumatic2r.TurnOff();
                //}
            }
            //*************************************************************/// koniec
            // wezykow
            // uginanie zderzakow
            for (int i = 0; i < 2; i++)
            {
                double dist = MoverParameters->Couplers[i].Dist / 2.0;
                if (smBuforLewy[i])
                    if (dist < 0)
                        smBuforLewy[i]->SetTranslate(vector3(dist, 0, 0));
                if (smBuforPrawy[i])
                    if (dist < 0)
                        smBuforPrawy[i]->SetTranslate(vector3(dist, 0, 0));
            }
        }

        // Winger 160204 - podnoszenie pantografow

        // przewody sterowania ukrotnionego
        if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_controll))
        {
            btCCtrl1.TurnOn();
            btnOn = true;
        }
        // else btCCtrl1.TurnOff();
        if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_controll))
        {
            btCCtrl2.TurnOn();
            btnOn = true;
        }
        // else btCCtrl2.TurnOff();
        // McZapkie-181103: mostki przejsciowe
        if (TestFlag(MoverParameters->Couplers[0].CouplingFlag, ctrain_passenger))
        {
            btCPass1.TurnOn();
            btnOn = true;
        }
        // else btCPass1.TurnOff();
        if (TestFlag(MoverParameters->Couplers[1].CouplingFlag, ctrain_passenger))
        {
            btCPass2.TurnOn();
            btnOn = true;
        }
        // else btCPass2.TurnOff();
        if (MoverParameters->Battery)
        { // sygnaly konca pociagu
            if (btEndSignals1.Active())
            {
                if (TestFlag(iLights[0], 2) || TestFlag(iLights[0], 32))
                {
                    btEndSignals1.TurnOn();
                    btnOn = true;
                }
                // else btEndSignals1.TurnOff();
            }
            else
            {
                if (TestFlag(iLights[0], 2))
                {
                    btEndSignals11.TurnOn();
                    btnOn = true;
                }
                // else btEndSignals11.TurnOff();
                if (TestFlag(iLights[0], 32))
                {
                    btEndSignals13.TurnOn();
                    btnOn = true;
                }
                // else btEndSignals13.TurnOff();
            }
            if (btEndSignals2.Active())
            {
                if (TestFlag(iLights[1], 2) || TestFlag(iLights[1], 32))
                {
                    btEndSignals2.TurnOn();
                    btnOn = true;
                }
                // else btEndSignals2.TurnOff();
            }
            else
            {
                if (TestFlag(iLights[1], 2))
                {
                    btEndSignals21.TurnOn();
                    btnOn = true;
                }
                // else btEndSignals21.TurnOff();
                if (TestFlag(iLights[1], 32))
                {
                    btEndSignals23.TurnOn();
                    btnOn = true;
                }
                // else btEndSignals23.TurnOff();
            }
        }
        // tablice blaszane:
        if (TestFlag(iLights[0], 64))
        {
            btEndSignalsTab1.TurnOn();
            btnOn = true;
        }
        // else btEndSignalsTab1.TurnOff();
        if (TestFlag(iLights[1], 64))
        {
            btEndSignalsTab2.TurnOn();
            btnOn = true;
        }
        // else btEndSignalsTab2.TurnOff();
        // McZapkie-181002: krecenie wahaczem (korzysta z kata obrotu silnika)
        if (iAnimType[ANIM_LEVERS])
            for (int i = 0; i < 4; ++i)
                if (smWahacze[i])
                    smWahacze[i]->SetRotate(float3(1, 0, 0),
                                            fWahaczeAmp * cos(MoverParameters->eAngle));
        
		if (Mechanik)
        { // rysowanie figurki mechanika
		/*
			if (smMechanik0) // mechanik od strony sprzęgu 0
                if (smMechanik1) // jak jest drugi, to pierwszego jedynie pokazujemy
                    smMechanik0->iVisible = MoverParameters->ActiveCab > 0;
                else
                { // jak jest tylko jeden, to do drugiej kabiny go obracamy
                    smMechanik0->iVisible = (MoverParameters->ActiveCab != 0);
                    smMechanik0->SetRotate(
                        float3(0, 0, 1),
                        MoverParameters->ActiveCab >= 0 ? 0 : 180); // obrót względem osi Z
                }
            if (smMechanik1) // mechanik od strony sprzęgu 1
                smMechanik1->iVisible = MoverParameters->ActiveCab < 0;
		*/
		if (MoverParameters->ActiveCab > 0)
			{
			btMechanik1.TurnOn();
				btnOn = true;
			}
		if (MoverParameters->ActiveCab < 0)
			{
			btMechanik2.TurnOn();
				btnOn = true;
			}
        }
        // ABu: Przechyly na zakretach
        // Ra: przechyłkę załatwiamy na etapie przesuwania modelu
        // if (ObjSqrDist<80000) ABuModelRoll(); //przechyłki od 400m
    }
    if (MoverParameters->Battery)
    { // sygnały czoła pociagu //Ra: wyświetlamy bez
        // ograniczeń odległości, by były widoczne z
        // daleka
        if (TestFlag(iLights[0], 1))
        {
            btHeadSignals11.TurnOn();
            btnOn = true;
        }
        // else btHeadSignals11.TurnOff();
        if (TestFlag(iLights[0], 4))
        {
            btHeadSignals12.TurnOn();
            btnOn = true;
        }
        // else btHeadSignals12.TurnOff();
        if (TestFlag(iLights[0], 16))
        {
            btHeadSignals13.TurnOn();
            btnOn = true;
        }
        // else btHeadSignals13.TurnOff();
        if (TestFlag(iLights[1], 1))
        {
            btHeadSignals21.TurnOn();
            btnOn = true;
        }
        // else btHeadSignals21.TurnOff();
        if (TestFlag(iLights[1], 4))
        {
            btHeadSignals22.TurnOn();
            btnOn = true;
        }
        // else btHeadSignals22.TurnOff();
        if (TestFlag(iLights[1], 16))
        {
            btHeadSignals23.TurnOn();
            btnOn = true;
        }
        // else btHeadSignals23.TurnOff();
    }
}
// ABu 29.01.05 koniec przeklejenia *************************************

double ABuAcos(const vector3 &calc_temp)
{ // Odpowiednik funkcji Arccos, bo cos
    // mi tam nie dzialalo.
    return atan2(-calc_temp.x, calc_temp.z); // Ra: tak prościej
}

TDynamicObject * TDynamicObject::ABuFindNearestObject(TTrack *Track,
                                                                TDynamicObject *MyPointer,
                                                                int &CouplNr)
{ // zwraca wskaznik do obiektu znajdujacego sie na torze
    // (Track), którego sprzęg jest najblizszy
    // kamerze
    // służy np. do łączenia i rozpinania sprzęgów
    // WE: Track      - tor, na ktorym odbywa sie poszukiwanie
    //    MyPointer  - wskaznik do obiektu szukajacego
    // WY: CouplNr    - który sprzęg znalezionego obiektu jest bliższy kamerze

    // Uwaga! Jesli CouplNr==-2 to szukamy njblizszego obiektu, a nie sprzegu!!!

#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    if ((Track->iNumDynamics) > 0)
    { // o ile w ogóle jest co przeglądać na tym torze
        // vector3 poz; //pozycja pojazdu XYZ w scenerii
        // vector3 kon; //wektor czoła względem środka pojazdu wzglęem początku toru
        vector3 tmp; // wektor pomiędzy kamerą i sprzęgiem
        double dist; // odległość
        for (int i = 0; i < Track->iNumDynamics; i++)
        {
            if (CouplNr == -2)
            { // wektor [kamera-obiekt] - poszukiwanie obiektu
                tmp = Global::GetCameraPosition() - Track->Dynamics[i]->vPosition;
                dist = tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z; // odległość do kwadratu
                if (dist < 100.0) // 10 metrów
                    return Track->Dynamics[i];
            }
            else // jeśli (CouplNr) inne niz -2, szukamy sprzęgu
            { // wektor [kamera-sprzeg0], potem [kamera-sprzeg1]
                // Powinno byc wyliczone, ale nie zaszkodzi drugi raz:
                //(bo co, jesli nie wykonuje sie obrotow wozkow?) - Ra: ale zawsze są
                // liczone
                // współrzędne sprzęgów
                // Track->Dynamics[i]->modelRot.z=ABuAcos(Track->Dynamics[i]->Axle0.pPosition-Track->Dynamics[i]->Axle1.pPosition);
                // poz=Track->Dynamics[i]->vPosition; //pozycja środka pojazdu
                // kon=vector3( //położenie przodu względem środka
                // -((0.5*Track->Dynamics[i]->MoverParameters->Dim.L)*sin(Track->Dynamics[i]->modelRot.z)),
                // 0, //yyy... jeśli duże pochylenie i długi pojazd, to może być problem
                // +((0.5*Track->Dynamics[i]->MoverParameters->Dim.L)*cos(Track->Dynamics[i]->modelRot.z))
                //);
                tmp =
                    Global::GetCameraPosition() -
                    Track->Dynamics[i]->vCoulpler[0]; // Ra: pozycje sprzęgów też są zawsze liczone
                dist = tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z; // odległość do kwadratu
                if (dist < 25.0) // 5 metrów
                {
                    CouplNr = 0;
                    return Track->Dynamics[i];
                }
                tmp = Global::GetCameraPosition() - Track->Dynamics[i]->vCoulpler[1];
                dist = tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z; // odległość do kwadratu
                if (dist < 25.0) // 5 metrów
                {
                    CouplNr = 1;
                    return Track->Dynamics[i];
                }
            }
        }
        return NULL;
    }
#else
    for( auto dynamic : Track->Dynamics ) {

        if( CouplNr == -2 ) {
            // wektor [kamera-obiekt] - poszukiwanie obiektu
            if( LengthSquared3( Global::GetCameraPosition() - dynamic->vPosition ) < 100.0 ) {
                // 10 metrów
                return dynamic;
            }
        }
        else {
            // jeśli (CouplNr) inne niz -2, szukamy sprzęgu
            if( LengthSquared3( Global::GetCameraPosition() - dynamic->vCoulpler[ 0 ] ) < 25.0 ) {
                // 5 metrów
                CouplNr = 0;
                return dynamic;
            }
            if( LengthSquared3( Global::GetCameraPosition() - dynamic->vCoulpler[ 1 ] ) < 25.0 ) {
                // 5 metrów
                CouplNr = 1;
                return dynamic;
            }
        }
    }
    // empty track or nothing found
    return nullptr;
#endif
}

TDynamicObject * TDynamicObject::ABuScanNearestObject(TTrack *Track, double ScanDir,
                                                                double ScanDist, int &CouplNr)
{ // skanowanie toru w poszukiwaniu obiektu najblizszego
    // kamerze
    // double MyScanDir=ScanDir;  //Moja orientacja na torze.  //Ra: nie używane
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
    if ((NewTrack != OldTrack) && OldTrack)
    {
        OldTrack->RemoveDynamicObject(this);
        NewTrack->AddDynamicObject(this);
    }
    iAxleFirst = 0; // pojazd powiązany z przednią osią - Axle0
}

// Ra: w poniższej funkcji jest problem ze sprzęgami
TDynamicObject * TDynamicObject::ABuFindObject(TTrack *Track, int ScanDir,
                                                         BYTE &CouplFound, double &dist)
{ // Zwraca wskaźnik najbliższego obiektu znajdującego się
    // na torze w określonym kierunku, ale tylko wtedy, kiedy
    // obiekty mogą się zderzyć, tzn. nie mijają się.

    // WE: Track      - tor, na ktorym odbywa sie poszukiwanie,
    //    MyPointer  - wskaznik do obiektu szukajacego. //Ra: zamieniłem na "this"
    //    ScanDir    - kierunek szukania na torze (+1:w stronę Point2, -1:w stronę
    //    Point1)
    //    MyScanDir  - kierunek szukania obiektu szukajacego (na jego torze); Ra:
    //    nie potrzebne
    //    MyCouplFound - nr sprzegu obiektu szukajacego; Ra: nie potrzebne

    // WY: wskaznik do znalezionego obiektu.
    //    CouplFound - nr sprzegu znalezionego obiektu
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    if (Track->iNumDynamics > 0)
#else
    if( false == Track->Dynamics.empty() )
#endif
    { // sens szukania na tym torze jest tylko, gdy są na nim pojazdy
        double ObjTranslation; // pozycja najblizszego obiektu na torze
        double MyTranslation; // pozycja szukającego na torze
        double MinDist = Track->Length(); // najmniejsza znaleziona odleglość
        // (zaczynamy od długości toru)
        double TestDist; // robocza odległość od kolejnych pojazdów na danym odcinku
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
        int iMinDist = -1; // indeks wykrytego obiektu
#else
        TDynamicObject *collider = nullptr;
#endif
        // if (Track->iNumDynamics>1)
        // iMinDist+=0; //tymczasowo pułapka
        if (MyTrack == Track) // gdy szukanie na tym samym torze
            MyTranslation = RaTranslationGet(); // położenie wózka względem Point1 toru
        else // gdy szukanie na innym torze
            if (ScanDir > 0)
            MyTranslation = 0; // szukanie w kierunku Point2 (od zera) - jesteśmy w Point1
        else
            MyTranslation = MinDist; // szukanie w kierunku Point1 (do zera) - jesteśmy w Point2
        if (ScanDir >= 0)
        { // jeśli szukanie w kierunku Point2
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
            for( int i = 0; i < Track->iNumDynamics; i++ )
            { // pętla po pojazdach
                if (Track->Dynamics[i] != this) // szukający się nie liczy
                {
                    TestDist = (Track->Dynamics[i]->RaTranslationGet()) -
                               MyTranslation; // odległogłość tamtego od szukającego
                    if ((TestDist > 0) && (TestDist <= MinDist))
                    { // gdy jest po właściwej stronie i bliżej
                        // niż jakiś wcześniejszy
                        CouplFound = (Track->Dynamics[i]->RaDirectionGet() > 0) ?
                                         1 :
                                         0; // to, bo (ScanDir>=0)
                        if (Track->iCategoryFlag & 254) // trajektoria innego typu niż tor kolejowy
                        { // dla torów nie ma sensu tego sprawdzać, rzadko co jedzie po
                            // jednej
                            // szynie i się mija
                            // Ra: mijanie samochodów wcale nie jest proste
                            // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
                            // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
                            // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
                            double RelOffsetH; // wzajemna odległość poprzeczna
                            if (CouplFound) // my na tym torze byśmy byli w kierunku Point2
                                // dla CouplFound=1 są zwroty zgodne - istotna różnica
                                // przesunięć
                                RelOffsetH = (MoverParameters->OffsetTrackH -
                                              Track->Dynamics[i]->MoverParameters->OffsetTrackH);
                            else
                                // dla CouplFound=0 są zwroty przeciwne - przesunięcia sumują
                                // się
                                RelOffsetH = (MoverParameters->OffsetTrackH +
                                              Track->Dynamics[i]->MoverParameters->OffsetTrackH);
                            if (RelOffsetH < 0)
                                RelOffsetH = -RelOffsetH;
                            if (RelOffsetH + RelOffsetH >
                                MoverParameters->Dim.W + Track->Dynamics[i]->MoverParameters->Dim.W)
                                continue; // odległość większa od połowy sumy szerokości -
                            // kolizji
                            // nie będzie
                            // jeśli zahaczenie jest niewielkie, a jest miejsce na poboczu, to
                            // zjechać na pobocze
                        }
                        iMinDist = i; // potencjalna kolizja
                        MinDist = TestDist; // odleglość pomiędzy aktywnymi osiami pojazdów
                    }
                }
            }
#else
            for( auto dynamic : Track->Dynamics ) {
                // pętla po pojazdach
                if( dynamic == this ) {
                    // szukający się nie liczy
                    continue;
                }
                
                TestDist = ( dynamic->RaTranslationGet() ) - MyTranslation; // odległogłość tamtego od szukającego
                if( ( TestDist > 0 ) && ( TestDist <= MinDist ) ) { // gdy jest po właściwej stronie i bliżej
                    // niż jakiś wcześniejszy
                    CouplFound = ( dynamic->RaDirectionGet() > 0 ) ? 1 : 0; // to, bo (ScanDir>=0)
                    if( Track->iCategoryFlag & 254 ) {
                        // trajektoria innego typu niż tor kolejowy
                        // dla torów nie ma sensu tego sprawdzać, rzadko co jedzie po jednej szynie i się mija
                        // Ra: mijanie samochodów wcale nie jest proste
                        // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
                        // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
                        // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
                        double RelOffsetH; // wzajemna odległość poprzeczna
                        if( CouplFound ) {
                            // my na tym torze byśmy byli w kierunku Point2
                            // dla CouplFound=1 są zwroty zgodne - istotna różnica przesunięć
                            RelOffsetH = ( MoverParameters->OffsetTrackH - dynamic->MoverParameters->OffsetTrackH );
                        }
                        else {
                            // dla CouplFound=0 są zwroty przeciwne - przesunięcia sumują się
                            RelOffsetH = ( MoverParameters->OffsetTrackH + dynamic->MoverParameters->OffsetTrackH );
                        }
                        if( RelOffsetH < 0 ) {
                            RelOffsetH = -RelOffsetH;
                        }
                        if( RelOffsetH + RelOffsetH > MoverParameters->Dim.W + dynamic->MoverParameters->Dim.W ) {
                            // odległość większa od połowy sumy szerokości - kolizji nie będzie
                            continue;
                        }
                        // jeśli zahaczenie jest niewielkie, a jest miejsce na poboczu, to
                        // zjechać na pobocze
                    }
                    collider = dynamic; // potencjalna kolizja
                    MinDist = TestDist; // odleglość pomiędzy aktywnymi osiami pojazdów
                }
                
            }
#endif
        }
        else //(ScanDir<0)
        {
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
            for( int i = 0; i < Track->iNumDynamics; i++ )
            {
                if (Track->Dynamics[i] != this)
                {
                    TestDist = MyTranslation -
                               (Track->Dynamics[i]->RaTranslationGet()); //???-przesunięcie wózka
                    // względem Point1 toru
                    if ((TestDist > 0) && (TestDist < MinDist))
                    {
                        CouplFound = (Track->Dynamics[i]->RaDirectionGet() > 0) ?
                                         0 :
                                         1; // odwrotnie, bo (ScanDir<0)
                        if (Track->iCategoryFlag & 254) // trajektoria innego typu niż tor kolejowy
                        { // dla torów nie ma sensu tego sprawdzać, rzadko co jedzie po
                            // jednej
                            // szynie i się mija
                            // Ra: mijanie samochodów wcale nie jest proste
                            // Przesunięcie względne pojazdów. Wyznaczane, żeby sprawdzić,
                            // czy pojazdy faktycznie się zderzają (mogą być przesunięte
                            // w/m siebie tak, że nie zachodzą na siebie i wtedy sie mijają).
                            double RelOffsetH; // wzajemna odległość poprzeczna
                            if (CouplFound) // my na tym torze byśmy byli w kierunku Point1
                                // dla CouplFound=1 są zwroty zgodne - istotna różnica
                                // przesunięć
                                RelOffsetH = (MoverParameters->OffsetTrackH -
                                              Track->Dynamics[i]->MoverParameters->OffsetTrackH);
                            else
                                // dla CouplFound=0 są zwroty przeciwne - przesunięcia sumują
                                // się
                                RelOffsetH = (MoverParameters->OffsetTrackH +
                                              Track->Dynamics[i]->MoverParameters->OffsetTrackH);
                            if (RelOffsetH < 0)
                                RelOffsetH = -RelOffsetH;
                            if (RelOffsetH + RelOffsetH >
                                MoverParameters->Dim.W + Track->Dynamics[i]->MoverParameters->Dim.W)
                                continue; // odległość większa od połowy sumy szerokości -
                            // kolizji
                            // nie będzie
                        }
                        iMinDist = i; // potencjalna kolizja
                        MinDist = TestDist; // odleglość pomiędzy aktywnymi osiami pojazdów
                    }
                }
            }
#else
            for( auto dynamic : Track->Dynamics ) {

                if( dynamic == this ) { continue; }

                TestDist = MyTranslation - ( dynamic->RaTranslationGet() ); //???-przesunięcie wózka względem Point1 toru
                if( ( TestDist > 0 ) && ( TestDist < MinDist ) ) {
                    CouplFound = ( dynamic->RaDirectionGet() > 0 ) ? 0 : 1; // odwrotnie, bo (ScanDir<0)
                    if( Track->iCategoryFlag & 254 ) // trajektoria innego typu niż tor kolejowy
                    { // dla torów nie ma sensu tego sprawdzać, rzadko co jedzie po jednej szynie i się mija
                        // Ra: mijanie samochodów wcale nie jest proste
                        // Przesunięcie względne pojazdów. Wyznaczane, żeby sprawdzić,
                        // czy pojazdy faktycznie się zderzają (mogą być przesunięte
                        // w/m siebie tak, że nie zachodzą na siebie i wtedy sie mijają).
                        double RelOffsetH; // wzajemna odległość poprzeczna
                        if( CouplFound ) {
                            // my na tym torze byśmy byli w kierunku Point1
                            // dla CouplFound=1 są zwroty zgodne - istotna różnica przesunięć
                            RelOffsetH = ( MoverParameters->OffsetTrackH - dynamic->MoverParameters->OffsetTrackH );
                        }
                        else {
                            // dla CouplFound=0 są zwroty przeciwne - przesunięcia sumują się
                            RelOffsetH = ( MoverParameters->OffsetTrackH + dynamic->MoverParameters->OffsetTrackH );
                        }
                        if( RelOffsetH < 0 ) {
                            RelOffsetH = -RelOffsetH;
                        }
                        if( RelOffsetH + RelOffsetH > MoverParameters->Dim.W + dynamic->MoverParameters->Dim.W ) {
                            // odległość większa od połowy sumy szerokości - kolizji nie będzie
                            continue;
                        }
                    }
                    collider = dynamic; // potencjalna kolizja
                    MinDist = TestDist; // odleglość pomiędzy aktywnymi osiami pojazdów
                }
            }
#endif
        }
        dist += MinDist; // doliczenie odległości przeszkody albo długości odcinka do przeskanowanej odległości
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
        return ( iMinDist >= 0 ) ? Track->Dynamics[ iMinDist ] : NULL;
#else
        return collider;
#endif
    }
    dist += Track->Length(); // doliczenie długości odcinka do przeskanowanej
    // odległości
    return nullptr; // nie ma pojazdów na torze, to jest NULL
}

int TDynamicObject::DettachStatus(int dir)
{ // sprawdzenie odległości sprzęgów
    // rzeczywistych od strony (dir):
    // 0=przód,1=tył
    // Ra: dziwne, że ta funkcja nie jest używana
    if (!MoverParameters->Couplers[dir].CouplingFlag)
        return 0; // jeśli nic nie podłączone, to jest OK
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
    if (MoverParameters->Couplers[dir].CouplingFlag) // odczepianie, o ile coś podłączone
        MoverParameters->Dettach(dir);
    return MoverParameters->Couplers[dir]
        .CouplingFlag; // sprzęg po rozłączaniu (czego się nie da odpiąć
}

void TDynamicObject::CouplersDettach(double MinDist, int MyScanDir)
{ // funkcja rozłączajaca podłączone sprzęgi,
    // jeśli odległość przekracza (MinDist)
    // MinDist - dystans minimalny, dla ktorego mozna rozłączać
    if (MyScanDir > 0)
    {
        if (PrevConnected) // pojazd od strony sprzęgu 0
        {
            if (MoverParameters->Couplers[0].CoupleDist >
                MinDist) // sprzęgi wirtualne zawsze przekraczają
            {
                if ((PrevConnectedNo ? PrevConnected->NextConnected :
                                       PrevConnected->PrevConnected) == this)
                { // Ra: nie rozłączamy znalezionego, jeżeli nie do nas
                    // podłączony (może jechać w
                    // innym kierunku)
                    PrevConnected->MoverParameters->Couplers[PrevConnectedNo].Connected = NULL;
                    if (PrevConnectedNo == 0)
                    {
                        PrevConnected->PrevConnectedNo = 2; // sprzęg 0 nie podłączony
                        PrevConnected->PrevConnected = NULL;
                    }
                    else if (PrevConnectedNo == 1)
                    {
                        PrevConnected->NextConnectedNo = 2; // sprzęg 1 nie podłączony
                        PrevConnected->NextConnected = NULL;
                    }
                }
                // za to zawsze odłączamy siebie
                PrevConnected = NULL;
                PrevConnectedNo = 2; // sprzęg 0 nie podłączony
                MoverParameters->Couplers[0].Connected = NULL;
            }
        }
    }
    else
    {
        if (NextConnected) // pojazd od strony sprzęgu 1
        {
            if (MoverParameters->Couplers[1].CoupleDist >
                MinDist) // sprzęgi wirtualne zawsze przekraczają
            {
                if ((NextConnectedNo ? NextConnected->NextConnected :
                                       NextConnected->PrevConnected) == this)
                { // Ra: nie rozłączamy znalezionego, jeżeli nie do nas
                    // podłączony (może jechać w
                    // innym kierunku)
                    NextConnected->MoverParameters->Couplers[NextConnectedNo].Connected = NULL;
                    if (NextConnectedNo == 0)
                    {
                        NextConnected->PrevConnectedNo = 2; // sprzęg 0 nie podłączony
                        NextConnected->PrevConnected = NULL;
                    }
                    else if (NextConnectedNo == 1)
                    {
                        NextConnected->NextConnectedNo = 2; // sprzęg 1 nie podłączony
                        NextConnected->NextConnected = NULL;
                    }
                }
                NextConnected = NULL;
                NextConnectedNo = 2; // sprzęg 1 nie podłączony
                MoverParameters->Couplers[1].Connected = NULL;
            }
        }
    }
}

void TDynamicObject::ABuScanObjects(int ScanDir, double ScanDist)
{ // skanowanie toru w poszukiwaniu kolidujących pojazdów
    // ScanDir - określa kierunek poszukiwania zależnie od zwrotu prędkości
    // pojazdu
    // ScanDir=1 - od strony Coupler0, ScanDir=-1 - od strony Coupler1
    int MyScanDir = ScanDir; // zapamiętanie kierunku poszukiwań na torze
    // początkowym, względem sprzęgów
    TTrackFollower *FirstAxle = (MyScanDir > 0 ? &Axle0 : &Axle1); // można by to trzymać w trainset
    TTrack *Track = FirstAxle->GetTrack(); // tor na którym "stoi" skrajny wózek
    // (może być inny niż tor pojazdu)
    if (FirstAxle->GetDirection() < 0) // czy oś jest ustawiona w stronę Point1?
        ScanDir = -ScanDir; // jeśli tak, to kierunek szukania będzie przeciwny
    // (teraz względem
    // toru)
    BYTE MyCouplFound; // numer sprzęgu do podłączenia w obiekcie szukajacym
    MyCouplFound = (MyScanDir < 0) ? 1 : 0;
    BYTE CouplFound; // numer sprzęgu w znalezionym obiekcie (znaleziony wypełni)
    TDynamicObject *FoundedObj; // znaleziony obiekt
    double ActDist = 0; // przeskanowana odleglość; odległość do zawalidrogi
    FoundedObj = ABuFindObject(Track, ScanDir, CouplFound,
                               ActDist); // zaczynamy szukać na tym samym torze

    /*
     if (FoundedObj) //jak coś znajdzie, to śledzimy
     {//powtórzenie wyszukiwania tylko do zastawiania pułepek podczas testów
      if (ABuGetDirection()<0) ScanDir=ScanDir; //ustalenie kierunku względem toru
      FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound);
     }
    */

    if (DebugModeFlag)
        if (FoundedObj) // kod służący do logowania błędów
            if (CouplFound == 0)
            {
                if (FoundedObj->PrevConnected)
                    if (FoundedObj->PrevConnected != this) // odświeżenie tego samego się nie liczy
                        WriteLog("0! Coupler warning on " + asName + ":" +
                                 to_string(MyCouplFound) + " - " + FoundedObj->asName +
                                 ":0 connected to " + FoundedObj->PrevConnected->asName + ":" +
                                 to_string(FoundedObj->PrevConnectedNo));
            }
            else
            {
                if (FoundedObj->NextConnected)
                    if (FoundedObj->NextConnected != this) // odświeżenie tego samego się nie liczy
                        WriteLog("0! Coupler warning on " + asName + ":" +
                                 to_string(MyCouplFound) + " - " + FoundedObj->asName +
                                 ":1 connected to " + FoundedObj->NextConnected->asName + ":" +
                                 to_string(FoundedObj->NextConnectedNo));
            }

    if (FoundedObj == NULL) // jeśli nie ma na tym samym, szukamy po okolicy
    { // szukanie najblizszego toru z jakims obiektem
        // praktycznie przeklejone z TraceRoute()...
        // double CurrDist=0; //aktualna dlugosc toru
        if (ScanDir >= 0) // uwzględniamy kawalek przeanalizowanego wcześniej toru
            ActDist = Track->Length() - FirstAxle->GetTranslation(); // odległość osi od Point2 toru
        else
            ActDist = FirstAxle->GetTranslation(); // odległość osi od Point1 toru
        while (ActDist < ScanDist)
        {
            // ActDist+=CurrDist; //odległość już przeanalizowana
            if (ScanDir > 0) // w kierunku Point2 toru
            {
                if (Track ? Track->iNextDirection :
                            false) // jeśli następny tor jest podpięty od Point2
                    ScanDir = -ScanDir; // to zmieniamy kierunek szukania na tym torze
                Track = Track->CurrentNext(); // potem dopiero zmieniamy wskaźnik
            }
            else // w kierunku Point1
            {
                if (Track ? !Track->iPrevDirection :
                            true) // jeśli poprzedni tor nie jest podpięty od Point2
                    ScanDir = -ScanDir; // to zmieniamy kierunek szukania na tym torze
                Track = Track->CurrentPrev(); // potem dopiero zmieniamy wskaźnik
            }
            if (Track)
            { // jesli jest kolejny odcinek toru
                // CurrDist=Track->Length(); //doliczenie tego toru do przejrzanego
                // dystandu
                FoundedObj = ABuFindObject(Track, ScanDir, CouplFound,
                                           ActDist); // przejrzenie pojazdów tego toru
                if (FoundedObj)
                {
                    // ActDist=ScanDist; //wyjście z pętli poszukiwania
                    break;
                }
            }
            else // jeśli toru nie ma, to wychodzimy
            {
                ActDist = ScanDist + 1.0; // koniec przeglądania torów
                break;
            }
        }
    } // Koniec szukania najbliższego toru z jakimś obiektem.
    // teraz odczepianie i jeśli coś się znalazło, doczepianie.
    if (MyScanDir > 0 ? PrevConnected : NextConnected)
        if ((MyScanDir > 0 ? PrevConnected : NextConnected) != FoundedObj)
            CouplersDettach(1.0, MyScanDir); // odłączamy, jeśli dalej niż metr
    // i łączenie sprzęgiem wirtualnym
    if (FoundedObj)
    { // siebie można bezpiecznie podłączyć jednostronnie do
        // znalezionego
        MoverParameters->Attach(MyCouplFound, CouplFound, FoundedObj->MoverParameters,
                                ctrain_virtual);
        // MoverParameters->Couplers[MyCouplFound].Render=false; //wirtualnego nie
        // renderujemy
        if (MyCouplFound == 0)
        {
            PrevConnected = FoundedObj; // pojazd od strony sprzęgu 0
            PrevConnectedNo = CouplFound;
        }
        else
        {
            NextConnected = FoundedObj; // pojazd od strony sprzęgu 1
            NextConnectedNo = CouplFound;
        }
        if (FoundedObj->MoverParameters->Couplers[CouplFound].CouplingFlag == ctrain_virtual)
        { // Ra: wpinamy się wirtualnym tylko jeśli znaleziony
            // ma wirtualny sprzęg
            FoundedObj->MoverParameters->Attach(CouplFound, MyCouplFound, this->MoverParameters,
                                                ctrain_virtual);
            if (CouplFound == 0) // jeśli widoczny sprzęg 0 znalezionego
            {
                if (DebugModeFlag)
                    if (FoundedObj->PrevConnected)
                        if (FoundedObj->PrevConnected != this)
                            WriteLog("1! Coupler warning on " + asName + ":" +
                                     to_string(MyCouplFound) + " - " + FoundedObj->asName +
                                     ":0 connected to " + FoundedObj->PrevConnected->asName + ":" +
                                     to_string(FoundedObj->PrevConnectedNo));
                FoundedObj->PrevConnected = this;
                FoundedObj->PrevConnectedNo = MyCouplFound;
            }
            else // jeśli widoczny sprzęg 1 znalezionego
            {
                if (DebugModeFlag)
                    if (FoundedObj->NextConnected)
                        if (FoundedObj->NextConnected != this)
                            WriteLog("1! Coupler warning on " + asName + ":" +
                                     to_string(MyCouplFound) + " - " + FoundedObj->asName +
                                     ":1 connected to " + FoundedObj->NextConnected->asName + ":" +
                                     to_string(FoundedObj->NextConnectedNo));
                FoundedObj->NextConnected = this;
                FoundedObj->NextConnectedNo = MyCouplFound;
            }
        }
        // Ra: jeśli dwa samochody się mijają na odcinku przed zawrotką, to
        // odległość między nimi
        // nie może być liczona w linii prostej!
        fTrackBlock = MoverParameters->Couplers[MyCouplFound]
                          .CoupleDist; // odległość do najbliższego pojazdu w linii prostej
        if (Track->iCategoryFlag > 1) // jeśli samochód
            if (ActDist > MoverParameters->Dim.L +
                              FoundedObj->MoverParameters->Dim
                                  .L) // przeskanowana odległość większa od długości pojazdów
                // else if (ActDist<ScanDist) //dla samochodów musi być uwzględniona
                // droga do
                // zawrócenia
                fTrackBlock = ActDist; // ta odległość jest wiecej warta
        // if (fTrackBlock<500.0)
        // WriteLog("Collision of "+AnsiString(fTrackBlock)+"m detected by
        // "+asName+":"+AnsiString(MyCouplFound)+" with "+FoundedObj->asName);
    }
    else // nic nie znalezione, to nie ma przeszkód
        fTrackBlock = 10000.0;
}
//----------ABu: koniec skanowania pojazdow

TDynamicObject::TDynamicObject()
{
    modelShake = vector3(0, 0, 0);
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
    bBrakeAcc = false;
    NextConnected = PrevConnected = NULL;
    NextConnectedNo = PrevConnectedNo = 2; // ABu: Numery sprzegow. 2=nie podłączony
    CouplCounter = 50; // będzie sprawdzać na początku
    asName = "";
    bEnabled = true;
    MyTrack = NULL;
    // McZapkie-260202
    dRailLength = 25.0;
    for (int i = 0; i < MaxAxles; i++)
        dRailPosition[i] = 0.0;
    for (int i = 0; i < MaxAxles; i++)
        dWheelsPosition[i] = 0.0; // będzie wczytane z MMD
    iAxles = 0;
    dWheelAngle[0] = 0.0;
    dWheelAngle[1] = 0.0;
    dWheelAngle[2] = 0.0;
    // Winger 160204 - pantografy
    // PantVolume = 3.5;
    NoVoltTime = 0;
    dDoorMoveL = 0.0;
    dDoorMoveR = 0.0;
    // for (int i=0;i<8;i++)
    //{
    // DoorSpeedFactor[i]=random(150);
    // DoorSpeedFactor[i]=(DoorSpeedFactor[i]+100)/100;
    //}
    mdModel = NULL;
    mdKabina = NULL;
    ReplacableSkinID[0] = 0;
    ReplacableSkinID[1] = 0;
    ReplacableSkinID[2] = 0;
    ReplacableSkinID[3] = 0;
    ReplacableSkinID[4] = 0;
    iAlpha = 0x30300030; // tak gdy tekstury wymienne nie mają przezroczystości
    // smWiazary[0]=smWiazary[1]=NULL;
    smWahacze[0] = smWahacze[1] = smWahacze[2] = smWahacze[3] = NULL;
    fWahaczeAmp = 0;
    smBrakeMode = NULL;
    smLoadMode = NULL;
    mdLoad = NULL;
    mdLowPolyInt = NULL;
    mdPrzedsionek = NULL;
    //smMechanik0 = smMechanik1 = NULL;
    smBuforLewy[0] = smBuforLewy[1] = NULL;
    smBuforPrawy[0] = smBuforPrawy[1] = NULL;
    enginevolume = 0;
    smBogie[0] = smBogie[1] = NULL;
    bogieRot[0] = bogieRot[1] = vector3(0, 0, 0);
    modelRot = vector3(0, 0, 0);
    eng_vol_act = 0.8;
    eng_dfrq = 0;
    eng_frq_act = 1;
    eng_turbo = 0;
    cp1 = cp2 = sp1 = sp2 = 0;
    iDirection = 1; // stoi w kierunku tradycyjnym (0, gdy jest odwrócony)
    iAxleFirst = 0; // numer pierwszej osi w kierunku ruchu (przełączenie
    // następuje, gdy osie sa na
    // tym samym torze)
    iInventory = 0; // flagi bitowe posiadanych submodeli (zaktualizuje się po
    // wczytaniu MMD)
    RaLightsSet(0, 0); // początkowe zerowanie stanu świateł
    // Ra: domyślne ilości animacji dla zgodności wstecz (gdy brak ilości podanych
    // w MMD)
	// ustawienie liczby modeli animowanych podczas konstruowania obiektu a nie na 0
	// prowadzi prosto do wysypów jeśli źle zdefiniowane mmd
    iAnimType[ANIM_WHEELS] = 0; // 0-osie (8)
    iAnimType[ANIM_DOORS] = 0; // 1-drzwi (8)
    iAnimType[ANIM_LEVERS] = 0; // 2-wahacze (4) - np. nogi konia
    iAnimType[ANIM_BUFFERS] = 0; // 3-zderzaki (4)
    iAnimType[ANIM_BOOGIES] = 0; // 4-wózki (2)
    iAnimType[ANIM_PANTS] = 0; // 5-pantografy (2)
    iAnimType[ANIM_STEAMS] = 0; // 6-tłoki (napęd parowozu)
    iAnimations = 0; // na razie nie ma żadnego
/*
    pAnimations = NULL;
*/
    pAnimated = NULL;
    fShade = 0.0; // standardowe oświetlenie na starcie
    iHornWarning = 1; // numer syreny do użycia po otrzymaniu sygnału do jazdy
    asDestination = "none"; // stojący nigdzie nie jedzie
    pValveGear = NULL; // Ra: tymczasowo
    iCabs = 0; // maski bitowe modeli kabin
    smBrakeSet = NULL; // nastawa hamulca (wajcha)
    smLoadSet = NULL; // nastawa ładunku (wajcha)
    smWiper = NULL; // wycieraczka (poniekąd też wajcha)
    fScanDist = 300.0; // odległość skanowania, zwiększana w trybie łączenia
    ctOwner = NULL; // na początek niczyj
    iOverheadMask = 0; // maska przydzielana przez AI pojazdom posiadającym
    // pantograf, aby wymuszały
    // jazdę bezprądową
    tmpTraction.TractionVoltage = 0; // Ra 2F1H: prowizorka, trzeba przechować
    // napięcie, żeby nie wywalało WS pod
    // izolatorem
    fAdjustment = 0.0; // korekcja odległości pomiędzy wózkami (np. na łukach)
}

TDynamicObject::~TDynamicObject()
{ // McZapkie-250302 - zamykanie logowania
    // parametrow fizycznych
    SafeDelete(Mechanik);
    SafeDelete(MoverParameters);
    // Ra: wyłączanie dźwięków powinno być dodane w ich destruktorach, ale się
    // sypie
    /* to też się sypie
     for (int i=0;i<MaxAxles;++i)
      rsStukot[i].Stop();   //dzwieki poszczegolnych osi
     rsSilnik.Stop();
     rsWentylator.Stop();
     rsPisk.Stop();
     rsDerailment.Stop();
     sPantUp.Stop();
     sPantDown.Stop();
     sBrakeAcc.Stop(); //dzwiek przyspieszacza
     rsDiesielInc.Stop();
     rscurve.Stop();
    */
/*
    delete[] pAnimations; // obiekty obsługujące animację
*/
    delete[] pAnimated; // lista animowanych submodeli
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
                     std::string TrainName, // nazwa składu, np. "PE2307" albo Vmax, jeśli pliku
                     // nie ma a są cyfry
                     float Load, // ilość ładunku
                     std::string LoadType, // nazwa ładunku
                     bool Reversed, // true, jeśli ma stać odwrotnie w składzie
                     std::string MoreParams // dodatkowe parametry wczytywane w postaci tekstowej
                     )
{ // Ustawienie początkowe pojazdu
    iDirection = (Reversed ? 0 : 1); // Ra: 0, jeśli ma być wstawiony jako obrócony tyłem
    asBaseDir = "dynamic\\" + BaseDir + "\\"; // McZapkie-310302
    asName = Name;
    std::string asAnimName = ""; // zmienna robocza do wyszukiwania osi i wózków
    // Ra: zmieniamy znaczenie obsady na jednoliterowe, żeby dosadzić kierownika
    if (DriverType == "headdriver")
        DriverType = "1"; // sterujący kabiną +1
    else if (DriverType == "reardriver")
        DriverType = "2"; // sterujący kabiną -1
    // else if (DriverType=="connected") DriverType="c"; //tego trzeba się pozbyć
    // na rzecz
    // ukrotnienia
    else if (DriverType == "passenger")
        DriverType = "p"; // to do przemyślenia
    else if (DriverType == "nobody")
        DriverType = ""; // nikt nie siedzi
    int Cab = 0; // numer kabiny z obsadą (nie można zająć obu)
    if (DriverType == "1") // od przodu składu
        Cab = 1; // iDirection?1:-1; //iDirection=1 gdy normalnie, =0 odwrotnie
    else if (DriverType == "2") // od tyłu składu
        Cab = -1; // iDirection?-1:1;
    else if (DriverType == "p")
    {
        if (Random(6) < 3)
            Cab = 1;
        else
            Cab = -1; // losowy przydział kabiny
    }
    /* to nie ma uzasadnienia
     else
     {//obsada nie rozpoznana
      Cab=0;  //McZapkie-010303: w przyszlosci dac tez pomocnika, palacza,
     konduktora itp.
      Error("Unknown DriverType description: "+DriverType);
      DriverType="nobody";
     }
    */
    // utworzenie parametrów fizyki
    MoverParameters =
        new TMoverParameters(iDirection ? fVel : -fVel, Type_Name, asName, Load, LoadType, Cab);
    iLights = MoverParameters->iLights; // wskaźnik na stan własnych świateł
    // (zmienimy dla rozrządczych EZT)
    // McZapkie: TypeName musi byc nazwą CHK/MMD pojazdu
    if (!MoverParameters->LoadFIZ(asBaseDir))
    { // jak wczytanie CHK się nie uda, to błąd
        if (ConversionError == -8)
            ErrorLog("Missed file: " + BaseDir + "\\" + Type_Name + ".fiz");
        Error("Cannot load dynamic object " + asName + " from:\r\n" + BaseDir + "\\" + Type_Name +
              ".fiz\r\nError " + to_string(ConversionError) + " in line " + to_string(LineCount));
        return 0.0; // zerowa długość to brak pojazdu
    }
    bool driveractive = (fVel != 0.0); // jeśli prędkość niezerowa, to aktywujemy ruch
    if (!MoverParameters->CheckLocomotiveParameters(
            driveractive,
            (fVel > 0 ? 1 : -1) * Cab *
                (iDirection ? 1 : -1))) // jak jedzie lub obsadzony to gotowy do drogi
    {
        Error("Parameters mismatch: dynamic object " + asName + " from\n" + BaseDir + "\\" +
              Type_Name);
        return 0.0; // zerowa długość to brak pojazdu
    }
    // ustawienie pozycji hamulca
    MoverParameters->LocalBrakePos = 0;
    if (driveractive)
    {
        if (Cab == 0)
            MoverParameters->BrakeCtrlPos =
                floor(MoverParameters->Handle->GetPos(bh_NP));
        else
            MoverParameters->BrakeCtrlPos = floor(MoverParameters->Handle->GetPos(bh_RP));
    }
    else
        MoverParameters->BrakeCtrlPos =
            floor(MoverParameters->Handle->GetPos(bh_NP));

    MoverParameters->BrakeLevelSet(
        MoverParameters->BrakeCtrlPos); // poprawienie hamulca po ewentualnym
    // przestawieniu przez Pascal

    // dodatkowe parametry yB
    MoreParams += "."; // wykonuje o jedną iterację za mało, więc trzeba mu dodać
    // kropkę na koniec
    int kropka = MoreParams.find("."); // znajdź kropke
    std::string ActPar; // na parametry
    while (kropka != std::string::npos) // jesli sa kropki jeszcze
    {
        int dlugosc = MoreParams.length();
        ActPar = ToUpper(MoreParams.substr(0, kropka)); // pierwszy parametr;
        MoreParams = MoreParams.substr(kropka + 1, dlugosc - kropka); // reszta do dalszej
        // obrobki
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
                MoverParameters->BrakeStatus |= 128; // wylacz
            }
            if (ActPar.find('0') != std::string::npos) // wylaczanie na sztywno
            {
                MoverParameters->BrakeStatus |= 128; // wylacz
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->BrakeReleaser(1); // odluznij automatycznie
            }
            if (ActPar.find('E') != std::string::npos) // oprozniony
            {
                MoverParameters->Hamulec->ForceEmptiness();
                MoverParameters->BrakeReleaser(1); // odluznij automatycznie
                MoverParameters->Pipe->CreatePress(0);
                MoverParameters->Pipe2->CreatePress(0);
            }
            if (ActPar.find('Q') != std::string::npos) // oprozniony
            {
                //    MoverParameters->Hamulec->ForceEmptiness(); //TODO: sprawdzic,
                //    dlaczego
                //    pojawia sie blad przy uzyciu tej linijki w lokomotywie
                MoverParameters->BrakeReleaser(1); // odluznij automatycznie
                MoverParameters->Pipe->CreatePress(0.0);
                MoverParameters->PipePress = 0.0;
                MoverParameters->Pipe2->CreatePress(0.0);
                MoverParameters->ScndPipePress = 0.0;
                MoverParameters->PantVolume = 1;
                MoverParameters->PantPress = 0;
                MoverParameters->CompressedVolume = 0;
            }

            if (ActPar.find('1') != std::string::npos) // wylaczanie 10%
            {
                if (Random(10) < 1) // losowanie 1/10
                {
                    MoverParameters->BrakeStatus |= 128; // wylacz
                    MoverParameters->Hamulec->ForceEmptiness();
                    MoverParameters->BrakeReleaser(1); // odluznij automatycznie
                }
            }
            if (ActPar.find('X') != std::string::npos) // agonalny wylaczanie 20%, usrednienie przekladni
            {
                if (Random(100) < 20) // losowanie 20/100
                {
                    MoverParameters->BrakeStatus |= 128; // wylacz
                    MoverParameters->Hamulec->ForceEmptiness();
                    MoverParameters->BrakeReleaser(1); // odluznij automatycznie
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
/*        else if (ActPar.substr(0, 1) == "") // tu mozna wpisac inny prefiks i inne rzeczy
        {
            // jakies inne prefiksy
        }
*/
    } // koniec while kropka

    if (MoverParameters->CategoryFlag & 2) // jeśli samochód
    { // ustawianie samochodow na poboczu albo na środku drogi
        if (Track->fTrackWidth < 3.5) // jeśli droga wąska
            MoverParameters->OffsetTrackH = 0.0; // to stawiamy na środku, niezależnie od stanu
        // ruchu
        else if (driveractive) // od 3.5m do 8.0m jedzie po środku pasa, dla
            // szerszych w odległości
            // 1.5m
            MoverParameters->OffsetTrackH =
                Track->fTrackWidth <= 8.0 ? -Track->fTrackWidth * 0.25 : -1.5;
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
    // w wagonie tez niech jedzie
    // if (MoverParameters->MainCtrlPosNo>0 &&
    // if (MoverParameters->CabNo!=0)
    if (DriverType != "")
    { // McZapkie-040602: jeśli coś siedzi w pojeździe
        if (Name == Global::asHumanCtrlVehicle) // jeśli pojazd wybrany do prowadzenia
        {
            if (DebugModeFlag ? false : MoverParameters->EngineType !=
                                            Dumb) // jak nie Debugmode i nie jest dumbem
                Controller = Humandriver; // wsadzamy tam sterującego
            else // w przeciwnym razie trzeba włączyć pokazywanie kabiny
                bDisplayCab = true;
        }
        // McZapkie-151102: rozkład jazdy czytany z pliku *.txt z katalogu w którym
        // jest sceneria
        if (DriverType == "1" || DriverType == "2")
        { // McZapkie-110303: mechanik i rozklad tylko gdy jest obsada
            // MoverParameters->ActiveCab=MoverParameters->CabNo; //ustalenie aktywnej
            // kabiny
            // (rozrząd)
            Mechanik = new TController(Controller, this, Aggressive);
            if (TrainName.empty()) // jeśli nie w składzie
            {
                Mechanik->DirectionInitial(); // załączenie rozrządu (wirtualne kabiny) itd.
                Mechanik->PutCommand(
                    "Timetable:", iDirection ? -fVel : fVel, 0,
                    NULL); // tryb pociągowy z ustaloną prędkością (względem sprzęgów)
            }
            // if (TrainName!="none")
            // Mechanik->PutCommand("Timetable:"+TrainName,fVel,0,NULL);
        }
        else if (DriverType == "p")
        { // obserwator w charakterze pasażera
            // Ra: to jest niebezpieczne, bo w razie co będzie pomagał hamulcem
            // bezpieczeństwa
            Mechanik = new TController(Controller, this, Easyman, false);
        }
    }
    // McZapkie-250202
    iAxles = (MaxAxles < MoverParameters->NAxles) ? MaxAxles : MoverParameters->NAxles; // ilość osi
    // wczytywanie z pliku nazwatypu.mmd, w tym model
    LoadMMediaFile(asBaseDir, Type_Name, asReplacableSkin);
    // McZapkie-100402: wyszukiwanie submodeli sprzegów
    btCoupler1.Init("coupler1", mdModel, false); // false - ma być wyłączony
    btCoupler2.Init("coupler2", mdModel, false);
    btCPneumatic1.Init("cpneumatic1", mdModel);
    btCPneumatic2.Init("cpneumatic2", mdModel);
    btCPneumatic1r.Init("cpneumatic1r", mdModel);
    btCPneumatic2r.Init("cpneumatic2r", mdModel);
    btPneumatic1.Init("pneumatic1", mdModel);
    btPneumatic2.Init("pneumatic2", mdModel);
    btPneumatic1r.Init("pneumatic1r", mdModel);
    btPneumatic2r.Init("pneumatic2r", mdModel);
    btCCtrl1.Init("cctrl1", mdModel, false);
    btCCtrl2.Init("cctrl2", mdModel, false);
    btCPass1.Init("cpass1", mdModel, false);
    btCPass2.Init("cpass2", mdModel, false);
    // sygnaly
    // ABu 060205: Zmiany dla koncowek swiecacych:
    btEndSignals11.Init("endsignal13", mdModel, false);
    btEndSignals21.Init("endsignal23", mdModel, false);
    btEndSignals13.Init("endsignal12", mdModel, false);
    btEndSignals23.Init("endsignal22", mdModel, false);
    iInventory |= btEndSignals11.Active() ? 0x01 : 0; // informacja, czy ma poszczególne światła
    iInventory |= btEndSignals21.Active() ? 0x02 : 0;
    iInventory |= btEndSignals13.Active() ? 0x04 : 0;
    iInventory |= btEndSignals23.Active() ? 0x08 : 0;
    // ABu: to niestety zostawione dla kompatybilnosci modeli:
    btEndSignals1.Init("endsignals1", mdModel, false);
    btEndSignals2.Init("endsignals2", mdModel, false);
    btEndSignalsTab1.Init("endtab1", mdModel, false);
    btEndSignalsTab2.Init("endtab2", mdModel, false);
    iInventory |= btEndSignals1.Active() ? 0x10 : 0;
    iInventory |= btEndSignals2.Active() ? 0x20 : 0;
    iInventory |= btEndSignalsTab1.Active() ? 0x40 : 0; // tabliczki blaszane
    iInventory |= btEndSignalsTab2.Active() ? 0x80 : 0;
    // ABu Uwaga! tu zmienic w modelu!
    btHeadSignals11.Init("headlamp13", mdModel, false); // lewe
    btHeadSignals12.Init("headlamp11", mdModel, false); // górne
    btHeadSignals13.Init("headlamp12", mdModel, false); // prawe
    btHeadSignals21.Init("headlamp23", mdModel, false);
    btHeadSignals22.Init("headlamp21", mdModel, false);
    btHeadSignals23.Init("headlamp22", mdModel, false);
	btMechanik1.Init("mechanik1", mdLowPolyInt, false);
	btMechanik2.Init("mechanik2", mdLowPolyInt, false);
    TurnOff(); // resetowanie zmiennych submodeli
    // wyszukiwanie zderzakow
    if (mdModel) // jeśli ma w czym szukać
        for (int i = 0; i < 2; i++)
        {
            asAnimName = std::string("buffer_left0") + to_string(i + 1);
            smBuforLewy[i] = mdModel->GetFromName(asAnimName.c_str());
            if (smBuforLewy[i])
                smBuforLewy[i]->WillBeAnimated(); // ustawienie flagi animacji
            asAnimName = std::string("buffer_right0") + to_string(i + 1);
            smBuforPrawy[i] = mdModel->GetFromName(asAnimName.c_str());
            if (smBuforPrawy[i])
                smBuforPrawy[i]->WillBeAnimated();
        }
    for (int i = 0; i < iAxles; i++) // wyszukiwanie osi (0 jest na końcu, dlatego dodajemy
        // długość?)
        dRailPosition[i] =
            (Reversed ? -dWheelsPosition[i] : (dWheelsPosition[i] + MoverParameters->Dim.L)) +
            fDist;
    // McZapkie-250202 end.
    Track->AddDynamicObject(this); // wstawiamy do toru na pozycję 0, a potem przesuniemy
    // McZapkie: zmieniono na ilosc osi brane z chk
    // iNumAxles=(MoverParameters->NAxles>3 ? 4 : 2 );
    iNumAxles = 2;
    // McZapkie-090402: odleglosc miedzy czopami skretu lub osiami
    fAxleDist = Max0R(MoverParameters->BDist, MoverParameters->ADist);
    if (fAxleDist < 0.2)
        fAxleDist = 0.2; //żeby się dało wektory policzyć
    if (fAxleDist > MoverParameters->Dim.L - 0.2) // nie mogą być za daleko
        fAxleDist = MoverParameters->Dim.L - 0.2; // bo będzie "walenie w mur"
    double fAxleDistHalf = fAxleDist * 0.5;
    // WriteLog("Dynamic "+Type_Name+" of length "+MoverParameters->Dim.L+" at
    // "+AnsiString(fDist));
    // if (Cab) //jeśli ma obsadę - zgodność wstecz, jeśli tor startowy ma Event0
    // if (Track->Event0) //jeśli tor ma Event0
    //  if (fDist>=0.0) //jeśli jeśli w starych sceneriach początek składu byłby
    //  wysunięty na ten
    //  tor
    //   if (fDist<=0.5*MoverParameters->Dim.L+0.2) //ale nie jest wysunięty
    //    fDist+=0.5*MoverParameters->Dim.L+0.2; //wysunąć go na ten tor
    // przesuwanie pojazdu tak, aby jego początek był we wskazanym miejcu
    fDist -= 0.5 * MoverParameters->Dim.L; // dodajemy pół długości pojazdu, bo
    // ustawiamy jego środek (zliczanie na
    // minus)
    switch (iNumAxles)
    { // Ra: pojazdy wstawiane są na tor początkowy, a potem
    // przesuwane
    case 2: // ustawianie osi na torze
        Axle0.Init(Track, this, iDirection ? 1 : -1);
        Axle0.Move((iDirection ? fDist : -fDist) + fAxleDistHalf, false);
        Axle1.Init(Track, this, iDirection ? 1 : -1);
        Axle1.Move((iDirection ? fDist : -fDist) - fAxleDistHalf,
                   false); // false, żeby nie generować eventów
        // Axle2.Init(Track,this,iDirection?1:-1);
        // Axle2.Move((iDirection?fDist:-fDist)-fAxleDistHalft+0.01),false);
        // Axle3.Init(Track,this,iDirection?1:-1);
        // Axle3.Move((iDirection?fDist:-fDist)+fAxleDistHalf-0.01),false);
        break;
    case 4:
        Axle0.Init(Track, this, iDirection ? 1 : -1);
        Axle0.Move((iDirection ? fDist : -fDist) + (fAxleDistHalf + MoverParameters->ADist * 0.5),
                   false);
        Axle1.Init(Track, this, iDirection ? 1 : -1);
        Axle1.Move((iDirection ? fDist : -fDist) - (fAxleDistHalf + MoverParameters->ADist * 0.5),
                   false);
        // Axle2.Init(Track,this,iDirection?1:-1);
        // Axle2.Move((iDirection?fDist:-fDist)-(fAxleDistHalf-MoverParameters->ADist*0.5),false);
        // Axle3.Init(Track,this,iDirection?1:-1);
        // Axle3.Move((iDirection?fDist:-fDist)+(fAxleDistHalf-MoverParameters->ADist*0.5),false);
        break;
    }
    Move(0.0001); // potrzebne do wyliczenia aktualnej pozycji; nie może być zero,
    // bo nie przeliczy
    // pozycji
    // teraz jeszcze trzeba przypisać pojazdy do nowego toru, bo przesuwanie
    // początkowe osi nie
    // zrobiło tego
    ABuCheckMyTrack(); // zmiana toru na ten, co oś Axle0 (oś z przodu)
    TLocation loc; // Ra: ustawienie pozycji do obliczania sprzęgów
    loc.X = -vPosition.x;
    loc.Y = vPosition.z;
    loc.Z = vPosition.y;
    MoverParameters->Loc = loc; // normalnie przesuwa ComputeMovement() w Update()
    // pOldPos4=Axle1.pPosition; //Ra: nie używane
    // pOldPos1=Axle0.pPosition;
    // ActualTrack= GetTrack(); //McZapkie-030303
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
    // ABu: zainicjowanie zmiennej, zeby nic sie nie ruszylo
    // w pierwszej klatce, potem juz liczona prawidlowa wartosc masy
    MoverParameters->ComputeConstans();
    /*Ra: to nie działa - Event0 musi być wykonywany ciągle
    if (fVel==0.0) //jeśli stoi
     if (MoverParameters->CabNo!=0) //i ma kogoś w kabinie
      if (Track->Event0) //a jest w tym torze event od stania
       RaAxleEvent(Track->Event0); //dodanie eventu stania do kolejki
    */
    vFloor = vector3(0, 0, MoverParameters->Floor); // wektor podłogi dla wagonów, przesuwa ładunek
    return MoverParameters->Dim.L; // długość większa od zera oznacza OK; 2mm docisku?
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
        bEnabled &= Axle0.Move(fDistance, !iAxleFirst); // oś z przodu pojazdu
        bEnabled &= Axle1.Move(fDistance /*-fAdjustment*/, iAxleFirst); // oś z tyłu pojazdu
    }
    else if (fDistance < 0.0)
    { // gdy ruch w stronę sprzęgu 1, doliczyć korektę do osi 0
        bEnabled &= Axle1.Move(fDistance, iAxleFirst); // oś z tyłu pojazdu prusza się pierwsza
        bEnabled &= Axle0.Move(fDistance /*-fAdjustment*/, !iAxleFirst); // oś z przodu pojazdu
    }
    else // gf: bez wywolania Move na postoju nie ma event0
    {
        bEnabled &= Axle1.Move(fDistance, iAxleFirst); // oś z tyłu pojazdu prusza się pierwsza
        bEnabled &= Axle0.Move(fDistance, !iAxleFirst); // oś z przodu pojazdu
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
        // normalizacja potrzebna z powodu
        // pochylenia (vFront)
        vUp = CrossProduct(vFront, vLeft); // wektor w górę, będzie jednostkowy
        modelRot.z = atan2(-vFront.x, vFront.z); // kąt obrotu pojazdu [rad]; z ABuBogies()
        double a = ((Axle1.GetRoll() + Axle0.GetRoll())); // suma przechyłek
        if (a != 0.0)
        { // wyznaczanie przechylenia tylko jeśli jest przechyłka
            // można by pobrać wektory normalne z toru...
            mMatrix.Identity(); // ta macierz jest potrzebna głównie do wyświetlania
            mMatrix.Rotation(a * 0.5, vFront); // obrót wzdłuż osi o przechyłkę
            vUp = mMatrix * vUp; // wektor w górę pojazdu (przekręcenie na przechyłce)
            // vLeft=mMatrix*DynamicObject->vLeft;
            // vUp=CrossProduct(vFront,vLeft); //wektor w górę
            // vLeft=Normalize(CrossProduct(vWorldUp,vFront)); //wektor w lewo
            vLeft = Normalize(CrossProduct(vUp, vFront)); // wektor w lewo
            // vUp=CrossProduct(vFront,vLeft); //wektor w górę
        }
        mMatrix.Identity(); // to też można by od razu policzyć, ale potrzebne jest
        // do wyświetlania
        mMatrix.BasisChange(vLeft, vUp, vFront); // przesuwanie jest jednak rzadziej niż
        // renderowanie
        mMatrix = Inverse(mMatrix); // wyliczenie macierzy dla pojazdu (potrzebna
        // tylko do wyświetlania?)
        // if (MoverParameters->CategoryFlag&2)
        { // przesunięcia są używane po wyrzuceniu pociągu z toru
            vPosition.x += MoverParameters->OffsetTrackH * vLeft.x; // dodanie przesunięcia w bok
            vPosition.z +=
                MoverParameters->OffsetTrackH * vLeft.z; // vLeft jest wektorem poprzecznym
            // if () na przechyłce będzie dodatkowo zmiana wysokości samochodu
            vPosition.y += MoverParameters->OffsetTrackV; // te offsety są liczone przez moverparam
        }
        // Ra: skopiowanie pozycji do fizyki, tam potrzebna do zrywania sprzęgów
        // MoverParameters->Loc.X=-vPosition.x; //robi to {Fast}ComputeMovement()
        // MoverParameters->Loc.Y= vPosition.z;
        // MoverParameters->Loc.Z= vPosition.y;
        // obliczanie pozycji sprzęgów do liczenia zderzeń
        vector3 dir = (0.5 * MoverParameters->Dim.L) * vFront; // wektor sprzęgu
        vCoulpler[0] = vPosition + dir; // współrzędne sprzęgu na początku
        vCoulpler[1] = vPosition - dir; // współrzędne sprzęgu na końcu
        MoverParameters->vCoulpler[0] = vCoulpler[0]; // tymczasowo kopiowane na inny poziom
        MoverParameters->vCoulpler[1] = vCoulpler[1];
        // bCameraNear=
        // if (bCameraNear) //jeśli istotne są szczegóły (blisko kamery)
        { // przeliczenie cienia
            TTrack *t0 = Axle0.GetTrack(); // już po przesunięciu
            TTrack *t1 = Axle1.GetTrack();
            if ((t0->eEnvironment == e_flat) && (t1->eEnvironment == e_flat)) // może być
                // e_bridge...
                fShade = 0.0; // standardowe oświetlenie
            else
            { // jeżeli te tory mają niestandardowy stopień zacienienia
                // (e_canyon, e_tunnel)
                if (t0->eEnvironment == t1->eEnvironment)
                {
                    switch (t0->eEnvironment)
                    { // typ zmiany oświetlenia
                    case e_canyon:
                        fShade = 0.65;
                        break; // zacienienie w kanionie
                    case e_tunnel:
                        fShade = 0.20;
                        break; // zacienienie w tunelu
                    }
                }
                else // dwa różne
                { // liczymy proporcję
                    double d = Axle0.GetTranslation(); // aktualne położenie na torze
                    if (Axle0.GetDirection() < 0)
                        d = t0->fTrackLength - d; // od drugiej strony liczona długość
                    d /= fAxleDist; // rozsataw osi procentowe znajdowanie się na torze
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
    MoverParameters->Attach(iDirection, Object->iDirection ^ 1, Object->MoverParameters, iType,
                            true);
    MoverParameters->Couplers[iDirection].Render = false;
    Object->MoverParameters->Attach(Object->iDirection ^ 1, iDirection, MoverParameters, iType,
                                    true);
    Object->MoverParameters->Couplers[Object->iDirection ^ 1].Render =
        true; // rysowanie sprzęgu w dołączanym
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
    if (MoverParameters->TrainType & dt_EZT) // w przypadku łączenia członów,
        // światła w rozrządczym zależą od
        // stanu w silnikowym
        if (MoverParameters->Couplers[iDirection].AllowedFlag &
            ctrain_depot) // gdy sprzęgi łączone warsztatowo (powiedzmy)
            if ((MoverParameters->Power < 1.0) &&
                (Object->MoverParameters->Power > 1.0)) // my nie mamy mocy, ale ten drugi ma
                iLights = Object->MoverParameters->iLights; // to w tym z mocą będą światła
            // załączane, a w tym bez tylko widoczne
            else if ((MoverParameters->Power > 1.0) &&
                     (Object->MoverParameters->Power < 1.0)) // my mamy moc, ale ten drugi nie ma
                Object->iLights = MoverParameters->iLights; // to w tym z mocą będą światła
    // załączane, a w tym bez tylko widoczne
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
    if (dt > 0)
        MoverParameters->ComputeTotalForce(dt, dt1,
                                           FullVer); // wywalenie WS zależy od ustawienia kierunku
    return true;
}

void TDynamicObject::LoadUpdate()
{ // przeładowanie modelu ładunku
    // Ra: nie próbujemy wczytywać modeli miliony razy podczas renderowania!!!
    if ((mdLoad == NULL) && (MoverParameters->Load > 0))
    {
        std::string asLoadName =
            asBaseDir + MoverParameters->LoadType + ".t3d"; // zapamiętany katalog pojazdu
        // asLoadName=MoverParameters->LoadType;
        // if (MoverParameters->LoadType!=AnsiString("passengers"))
        Global::asCurrentTexturePath = asBaseDir; // bieżąca ścieżka do tekstur to dynamic/...
        mdLoad = TModelsManager::GetModel(asLoadName.c_str()); // nowy ładunek
        Global::asCurrentTexturePath =
            std::string(szTexturePath); // z powrotem defaultowa sciezka do tekstur
        // Ra: w MMD można by zapisać położenie modelu ładunku (np. węgiel) w
        // zależności od
        // załadowania
    }
    else if (MoverParameters->Load == 0)
        mdLoad = NULL; // nie ma ładunku
    // if ((mdLoad==NULL)&&(MoverParameters->Load>0))
    // {
    //  mdLoad=NULL; //Ra: to jest tu bez sensu - co autor miał na myśli?
    // }
    MoverParameters->LoadStatus &= 3; // po zakończeniu będzie równe zero
};

/*
double ComputeRadius(double p1x, double p1z, double p2x, double p2z,
                                double p3x, double p3z, double p4x, double p4z)
{

    double v1z= p1x-p2x;
    double v1x= p1z-p2z;
    double v4z= p3x-p4x;
    double v4x= p3z-p4z;
    double A1= p2z-p1z;
    double B1= p1x-p2x;
    double C1= -p1z*B1-p1x*A1;
    double A2= p4z-p3z;
    double B2= p3x-p4x;
    double C2= -p3z*B1-p3x*A1;
    double y= (A1*C2/A2-C1)/(B1-A1*B2/A2);
    double x= (-B2*y-C2)/A2;
}
*/
double TDynamicObject::ComputeRadius(vector3 p1, vector3 p2, vector3 p3, vector3 p4)
{
    //    vector3 v1

    //    TLine l1= TLine(p1,p1-p2);
    //    TLine l4= TLine(p4,p4-p3);
    //    TPlane p1= l1.GetPlane();
    //    vector3 pt;
    //    CrossPoint(pt,l4,p1);
    double R = 0.0;
    vector3 p12 = p1 - p2;
    vector3 p34 = p3 - p4;
    p12 = CrossProduct(p12, vector3(0.0, 0.1, 0.0));
    p12 = Normalize(p12);
    p34 = CrossProduct(p34, vector3(0.0, 0.1, 0.0));
    p34 = Normalize(p34);
    if (fabs(p1.x - p2.x) > 0.01)
    {
        if (fabs(p12.x - p34.x) > 0.001)
            R = (p1.x - p4.x) / (p34.x - p12.x);
    }
    else
    {
        if (fabs(p12.z - p34.z) > 0.001)
            R = (p1.z - p4.z) / (p34.z - p12.z);
    }
    return (R);
}

/*
double TDynamicObject::ComputeRadius()
{
  double L=0;
  double d=0;
  d=sqrt(SquareMagnitude(Axle0.pPosition-Axle1.pPosition));
  L=Axle1.GetLength(Axle1.pPosition,Axle1.pPosition-Axle2.pPosition,Axle0.pPosition-Axle3.pPosition,Axle0.pPosition);

  double eps=0.01;
  double R= 0;
  double L_d;
  if ((L>0) || (d>0))
   {
     L_d= L-d;
     if (L_d>eps)
      {
        R=L*sqrt(L/(24*(L_d)));
      }
   }
  return R;
}
*/

/* Ra: na razie nie potrzebne
void TDynamicObject::UpdatePos()
{
  MoverParameters->Loc.X= -vPosition.x;
  MoverParameters->Loc.Y=  vPosition.z;
  MoverParameters->Loc.Z=  vPosition.y;
}
*/

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
    if (dt == 0)
        return true; // Ra: pauza
    if (!MoverParameters->PhysicActivation &&
        !MechInside) // to drugie, bo będąc w maszynowym blokuje się fizyka
        return true; // McZapkie: wylaczanie fizyki gdy nie potrzeba
    if (!MyTrack)
        return false; // pojazdy postawione na torach portalowych mają MyTrack==NULL
    if (!bEnabled)
        return false; // a normalnie powinny mieć bEnabled==false

    // Ra: przeniosłem - no już lepiej tu, niż w wyświetlaniu!
    // if ((MoverParameters->ConverterFlag==false) &&
    // (MoverParameters->TrainType!=dt_ET22))
    // Ra: to nie może tu być, bo wyłącza sprężarkę w rozrządczym EZT!
    // if
    // ((MoverParameters->ConverterFlag==false)&&(MoverParameters->CompressorPower!=0))
    // MoverParameters->CompressorFlag=false;
    // if (MoverParameters->CompressorPower==2)
    // MoverParameters->CompressorAllow=MoverParameters->ConverterFlag;

    // McZapkie-260202
    if ((MoverParameters->EnginePowerSource.SourceType == CurrentCollector) &&
        (MoverParameters->Power > 1.0)) // aby rozrządczy nie opuszczał silnikowemu
        if ((MechInside) || (MoverParameters->TrainType == dt_EZT))
        {
            // if
            // ((!MoverParameters->PantCompFlag)&&(MoverParameters->CompressedVolume>=2.8))
            // MoverParameters->PantVolume=MoverParameters->CompressedVolume;
            if (MoverParameters->PantPress < (MoverParameters->TrainType == dt_EZT ? 2.4 : 3.5))
            { // 3.5 wg
                // http://www.transportszynowy.pl/eu06-07pneumat.php
                //"Wyłączniki ciśnieniowe odbieraków prądu wyłączają sterowanie
                // wyłącznika szybkiego
                // oraz uniemożliwiają podniesienie odbieraków prądu, gdy w instalacji
                // rozrządu
                // ciśnienie spadnie poniżej wartości 3,5 bara."
                // Ra 2013-12: Niebugocław mówi, że w EZT podnoszą się przy 2.5
                // if (!MoverParameters->PantCompFlag)
                // MoverParameters->PantVolume=MoverParameters->CompressedVolume;
                MoverParameters->PantFront(false); // opuszczenie pantografów przy niskim ciśnieniu
                MoverParameters->PantRear(false); // to idzie w ukrotnieniu, a nie powinno...
            }
            // Winger - automatyczne wylaczanie malej sprezarki
            else if (MoverParameters->PantPress >= 4.8)
                MoverParameters->PantCompFlag = false;
        } // Ra: do Mover to trzeba przenieść, żeby AI też mogło sobie podpompować

    double dDOMoveLen;

    TLocation l;
    l.X = -vPosition.x; // przekazanie pozycji do fizyki
    l.Y = vPosition.z;
    l.Z = vPosition.y;
    TRotation r;
    r.Rx = r.Ry = r.Rz = 0;
    // McZapkie: parametry powinny byc pobierane z toru

    // TTrackShape ts;
    // ts.R=MyTrack->fRadius;
    // if (ABuGetDirection()<0) ts.R=-ts.R;
    //    ts.R=MyTrack->fRadius; //ujemne promienie są już zamienione przy
    //    wczytywaniu
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
    tp.friction = MyTrack->fFriction * Global::fFriction;
    tp.CategoryFlag = MyTrack->iCategoryFlag & 15;
    tp.DamageFlag = MyTrack->iDamageFlag;
    tp.QualityFlag = MyTrack->iQualityFlag;
    if ((MoverParameters->Couplers[0].CouplingFlag > 0) &&
        (MoverParameters->Couplers[1].CouplingFlag > 0))
    {
        MoverParameters->InsideConsist = true;
    }
    else
    {
        MoverParameters->InsideConsist = false;
    }
    // napiecie sieci trakcyjnej
    // Ra 15-01: przeliczenie poboru prądu powinno być robione wcześniej, żeby na
    // tym etapie były
    // znane napięcia
    // TTractionParam tmpTraction;
    // tmpTraction.TractionVoltage=0;
    if (MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
    { // dla EZT tylko silnikowy
        // if (Global::bLiveTraction)
        { // Ra 2013-12: to niżej jest chyba trochę bez sensu
            double v = MoverParameters->PantRearVolt;
            if (v == 0.0)
            {
                v = MoverParameters->PantFrontVolt;
                if (v == 0.0)
                    if ((MoverParameters->TrainType & (dt_EZT | dt_ET40 | dt_ET41 | dt_ET42)) &&
                        MoverParameters->EngineType !=
                            ElectricInductionMotor) // dwuczłony mogą mieć sprzęg WN
                        v = MoverParameters->GetTrainsetVoltage(); // ostatnia szansa
            }
            if (v != 0.0)
            { // jeśli jest zasilanie
                NoVoltTime = 0;
                tmpTraction.TractionVoltage = v;
            }
            else
            {
                /*
                      if (MoverParameters->Vel>0.1f) //jeśli jedzie
                       if (NoVoltTime==0.0) //tylko przy pierwszym zaniku napięcia
                        if (MoverParameters->PantFrontUp||MoverParameters->PantRearUp)
                        //if
                   ((pants[0].fParamPants->PantTraction>1.0)||(pants[1].fParamPants->PantTraction>1.0))
                        {//wspomagacz usuwania problemów z siecią
                         if (!Global::iPause)
                         {//Ra: tymczasowa teleportacja do miejsca, gdzie brakuje prądu
                          Global::SetCameraPosition(vPosition+vector3(0,0,5)); //nowa
                   pozycja dla
                   generowania obiektów
                          Global::pCamera->Init(vPosition+vector3(0,0,5),Global::pFreeCameraInitAngle[0]);
                   //przestawienie
                         }
                         Global:l::pGround->Silence(Global::pCamera->Pos); //wyciszenie
                   wszystkiego
                   z poprzedniej pozycji
                          Globa:iPause|=1; //tymczasowe zapauzowanie, gdy problem z
                   siecią
                        }
                */
                NoVoltTime = NoVoltTime + dt;
                if (NoVoltTime > 0.2) // jeśli brak zasilania dłużej niż 0.2 sekundy (25km/h pod
                // izolatorem daje 0.15s)
                { // Ra 2F1H: prowizorka, trzeba przechować napięcie, żeby nie wywalało
                    // WS pod
                    // izolatorem
                    if (MoverParameters->Vel > 0.5) // jeśli jedzie
                        if (MoverParameters->PantFrontUp ||
                            MoverParameters->PantRearUp) // Ra 2014-07: doraźna blokada logowania
                            // zimnych lokomotyw - zrobić to trzeba
                            // inaczej
                            // if (NoVoltTime>0.02) //tu można ograniczyć czas rozłączenia
                            // if (DebugModeFlag) //logowanie nie zawsze
                            if ((MoverParameters->Mains) &&
                               ((MoverParameters->EngineType != ElectricInductionMotor)
                                || (MoverParameters->GetTrainsetVoltage() < 0.1f)))
                            { // Ra 15-01: logować tylko, jeśli WS załączony
                              // yB 16-03: i nie jest to asynchron zasilany z daleka 
                                // if (MoverParameters->PantFrontUp&&pants)
                                // Ra 15-01: bezwzględne współrzędne pantografu nie są dostępne,
                                // więc lepiej się tego nie zaloguje
                                ErrorLog("Voltage loss: by " + MoverParameters->Name + " at " +
                                         to_string(vPosition.x, 2, 7) + " " +
                                         to_string(vPosition.y, 2, 7) + " " +
                                         to_string(vPosition.z, 2, 7) + ", time " +
                                         to_string(NoVoltTime, 2, 7));
                                // if (MoverParameters->PantRearUp)
                                // if (iAnimType[ANIM_PANTS]>1)
                                //  if (pants[1])
                                //   ErrorLog("Voltage loss: by "+MoverParameters->Name+" at
                                //   "+FloatToStrF(vPosition.x,ffFixed,7,2)+"
                                //   "+FloatToStrF(vPosition.y,ffFixed,7,2)+"
                                //   "+FloatToStrF(vPosition.z,ffFixed,7,2)+", time
                                //   "+FloatToStrF(NoVoltTime,ffFixed,7,2));
                            }
                    // Ra 2F1H: nie było sensu wpisywać tu zera po upływie czasu, bo
                    // zmienna była
                    // tymczasowa, a napięcie zerowane od razu
                    tmpTraction.TractionVoltage = 0; // Ra 2013-12: po co tak?
                    // pControlled->MainSwitch(false); //może tak?
                }
            }
        }
        // else //Ra: nie no, trzeba podnieść pantografy, jak nie będzie drutu, to
        // będą miały prąd
        // po osiągnięciu 1.4m
        // tmpTraction.TractionVoltage=0.95*MoverParameters->EnginePowerSource.MaxVoltage;
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
        if ((Mechanik->Primary()) &&
            (MoverParameters->EngineType == ElectricInductionMotor)) // jesli glowny i z
        // asynchronami, to
        // niech steruje
        { // hamulcem lacznie dla calego pociagu/ezt
			bool kier = (DirectionGet() * MoverParameters->ActiveCab > 0);
			float FED = 0;
            int np = 0;
            float masa = 0;
            float FrED = 0;
            float masamax = 0;
            float FmaxPN = 0;
			float FfulED = 0;
			float FmaxED = 0;
            float Fzad = 0;
            float FzadED = 0;
            float FzadPN = 0;
			float Frj = 0;
			float amax = 0;
			float osie = 0;
			// 1. ustal wymagana sile hamowania calego pociagu
            //   - opoznienie moze byc ustalane na podstawie charakterystyki
            //   - opoznienie moze byc ustalane na podstawie mas i cisnien granicznych
            //

            // 2. ustal mozliwa do realizacji sile hamowania ED
            //   - w szczegolnosci powinien brac pod uwage rozne sily hamowania
            for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
				(kier ? p = p->NextC(4) : p = p->PrevC(4)))
            {
                np++;
                masamax += p->MoverParameters->MBPM +
                           (p->MoverParameters->MBPM > 1 ? 0 : p->MoverParameters->Mass) +
                           p->MoverParameters->Mred;
                float Nmax = ((p->MoverParameters->P2FTrans * p->MoverParameters->MaxBrakePress[0] -
                               p->MoverParameters->BrakeCylSpring) *
                                  p->MoverParameters->BrakeCylMult[0] -
                              p->MoverParameters->BrakeSlckAdj) *
                             p->MoverParameters->BrakeCylNo * p->MoverParameters->BrakeRigEff;
                FmaxPN += Nmax * p->MoverParameters->Hamulec->GetFC(
                              Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA),
                              p->MoverParameters->Vmax) *
                          1000; // sila hamowania pn
                FmaxED += ((p->MoverParameters->Mains) && (p->MoverParameters->ActiveDir != 0) &&
					(p->MoverParameters->eimc[eimc_p_Fh] * p->MoverParameters->NPoweredAxles >
                                                           0) ?
                               p->MoverParameters->eimc[eimc_p_Fh] * 1000 :
                               0); // chwilowy max ED -> do rozdzialu sil
                FED -= Min0R(p->MoverParameters->eimv[eimv_Fmax], 0) *
                       1000; // chwilowy max ED -> do rozdzialu sil
				FfulED = Min0R(p->MoverParameters->eimv[eimv_Fful], 0) *
					1000; // chwilowy max ED -> do rozdzialu sil
				FrED -= Min0R(p->MoverParameters->eimv[eimv_Fr], 0) *
                        1000; // chwilowo realizowane ED -> do pneumatyki
				Frj += Max0R(p->MoverParameters->eimv[eimv_Fr], 0) *
					1000;// chwilowo realizowany napęd -> do utrzymującego
				masa += p->MoverParameters->TotalMass;
				osie += p->MoverParameters->NAxles;
			}
            amax = FmaxPN / masamax;
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
            Fzad = amax * MoverParameters->LocalBrakeRatio() * masa;
            if ((MoverParameters->ScndS) &&
                (MoverParameters->Vel > MoverParameters->eimc[eimc_p_Vh1]) && (FmaxED > 0))
            {
                Fzad = Min0R(MoverParameters->LocalBrakeRatio() * FmaxED, FfulED);
            }
            if (((MoverParameters->ShuntMode) && (Frj < 0.0015 * masa)) ||
                (MoverParameters->V * MoverParameters->DirAbsolute < -0.2))
            {
                Fzad = Max0R(MoverParameters->StopBrakeDecc * masa, Fzad);
            }

            if (MoverParameters->BrakeHandle == MHZ_EN57?MoverParameters->BrakeOpModeFlag & bom_MED:MoverParameters->EpFuse)
              FzadED = Min0R(Fzad, FmaxED);
            else
              FzadED = 0;
            FzadPN = Fzad - FrED;
            //np = 0;
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
				(kier > 0 ? p = p->NextC(4) : p = p->PrevC(4)))
			{
                float Nmax = ((p->MoverParameters->P2FTrans * p->MoverParameters->MaxBrakePress[0] -
                               p->MoverParameters->BrakeCylSpring) *
                                  p->MoverParameters->BrakeCylMult[0] -
                              p->MoverParameters->BrakeSlckAdj) *
                             p->MoverParameters->BrakeCylNo * p->MoverParameters->BrakeRigEff;
                FmaxEP[i] = Nmax *
                            p->MoverParameters->Hamulec->GetFC(
                                Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA),
                                p->MoverParameters->Vmax) *
                            1000; // sila hamowania pn

                PrzekrF[i] = false;
                FzED[i] = (FmaxED > 0 ? FzadED / FmaxED : 0);
                p->MoverParameters->AnPos =
                    (MoverParameters->ScndS ? MoverParameters->LocalBrakeRatio() : FzED[i]);
                FzEP[i] = FzadPN * p->MoverParameters->NAxles / osie;
                i++;
                p->MoverParameters->ShuntMode = MoverParameters->ShuntMode;
                p->MoverParameters->ShuntModeAllow = MoverParameters->ShuntModeAllow;
            }
            while (test)
            {
                test = false;
                i = 0;
                float przek = 0;
                for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0, 4); p;
                     (kier > 0 ? p = p->NextC(4) : p = p->PrevC(4)))
                {
                    if ((FzEP[i] > 0.01) &&
                        (FzEP[i] >
                         p->MoverParameters->TotalMass * p->MoverParameters->eimc[eimc_p_eped] +
                             Min0R(p->MoverParameters->eimv[eimv_Fr], 0) * 1000) &&
                        (!PrzekrF[i]))
                    {
                        float przek1 = -Min0R(p->MoverParameters->eimv[eimv_Fr], 0) * 1000 +
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
                    i++;
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
                    i++;
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
                float FmaxPoj = Nmax * 
					p->MoverParameters->Hamulec->GetFC(
						Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA),
						p->MoverParameters->Vel) *
					1000; // sila hamowania pn
				p->MoverParameters->LocalBrakePosA = (p->MoverParameters->SlippingWheels ? 0 : FzEP[i] / FmaxPoj);
				if (p->MoverParameters->LocalBrakePosA>0.009)
					if (p->MoverParameters->P2FTrans * p->MoverParameters->BrakeCylMult[0] *
						p->MoverParameters->MaxBrakePress[0] != 0)
					{
						float x = (p->MoverParameters->BrakeSlckAdj / p->MoverParameters->BrakeCylMult[0] +
							p->MoverParameters->BrakeCylSpring) / (p->MoverParameters->P2FTrans *
								p->MoverParameters->MaxBrakePress[0]);
						p->MoverParameters->LocalBrakePosA = x + (1 - x) * p->MoverParameters->LocalBrakePosA;
					}
					else
						p->MoverParameters->LocalBrakePosA = p->MoverParameters->LocalBrakePosA;
				else
					p->MoverParameters->LocalBrakePosA = 0;
				i++;
			}
			/*            ////ALGORYTM 1 - KAZDEMU PO ROWNO
			for (TDynamicObject *p = GetFirstDynamic(MoverParameters->ActiveCab < 0 ? 1 : 0); p;
			(iDirection > 0 ? p = p->NextC(4) : p = p->PrevC(4)))
			{

			float Nmax = ((p->MoverParameters->P2FTrans * p->MoverParameters->MaxBrakePress[0] -
			p->MoverParameters->BrakeCylSpring) *
			p->MoverParameters->BrakeCylMult[0] -
			p->MoverParameters->BrakeSlckAdj) *
			p->MoverParameters->BrakeCylNo * p->MoverParameters->BrakeRigEff;
			float FmaxPoj = Nmax *
			p->MoverParameters->Hamulec->GetFC(
                            Nmax / (p->MoverParameters->NAxles * p->MoverParameters->NBpA),
                            p->MoverParameters->Vel) *
                        1000; // sila hamowania pn
			//         Fpoj=(FED>0?-FzadED*p->MoverParameters->eimv[eimv_Fmax]*1000/FED:0);
			//         p->MoverParameters->AnPos=(p->MoverParameters->eimc[eimc_p_Fh]>1?0.001f*Fpoj/(p->MoverParameters->eimc[eimc_p_Fh]):0);
			p->MoverParameters->AnPos = (FmaxED > 0 ? FzadED / FmaxED : 0);
			// Fpoj = FzadPN * Min0R(p->MoverParameters->TotalMass / masa, 1);
			// p->MoverParameters->LocalBrakePosA =
			//     (p->MoverParameters->SlippingWheels ? 0 : Min0R(Max0R(Fpoj / FmaxPoj, 0), 1));
			p->MoverParameters->LocalBrakePosA = (p->MoverParameters->SlippingWheels ? 0 : FzadPN / FmaxPN);
			}    */

			MED[0][0] = masa*0.001;
			MED[0][1] = amax;
			MED[0][2] = Fzad*0.001;
			MED[0][3] = FmaxPN*0.001;
			MED[0][4] = FmaxED*0.001;
			MED[0][5] = FrED*0.001;
			MED[0][6] = FzadPN*0.001;
			MED[0][7] = nPrzekrF;

			delete[] PrzekrF;
			delete[] FzED;
			delete[] FzEP;
			delete[] FmaxEP;
        }

        // yB: cos (AI) tu jest nie kompatybilne z czyms (hamulce)
        //   if (Controller!=Humandriver)
        //    if (Mechanik->LastReactionTime>0.5)
        //     {
        //      MoverParameters->BrakeCtrlPos=0;
        //      Mechanik->LastReactionTime=0;
        //     }

        Mechanik->UpdateSituation(dt1); // przebłyski świadomości AI
    }

    // fragment "z EXE Kursa"
    if (MoverParameters->Mains) // nie wchodzić w funkcję bez potrzeby
        if ((!MoverParameters->Battery) && (Controller == Humandriver) &&
            (MoverParameters->EngineType != DieselEngine) &&
            (MoverParameters->EngineType != WheelsDriven))
        { // jeśli bateria wyłączona, a nie diesel ani drezyna
            // reczna
            if (MoverParameters->MainSwitch(false)) // wyłączyć zasilanie
                MoverParameters->EventFlag = true;
        }
    if (MoverParameters->TrainType == dt_ET42)
    { // powinny być wszystkie dwuczłony oraz EZT
        /*
             //Ra: to jest bez sensu, bo wyłącza WS przy przechodzeniu przez
           "wewnętrzne" kabiny (z
           powodu ActiveCab)
             //trzeba to zrobić inaczej, np. dla członu A sprawdzać, czy jest B
             //albo sprawdzać w momencie załączania WS i zmiany w sprzęgach
             if
           (((TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0)&&(NextConnected->MoverParameters->TrainType!=dt_ET42))||((TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)&&(PrevConnected->MoverParameters->TrainType!=dt_ET42)))
             {//sprawdzenie, czy z tyłu kabiny mamy drugi człon
              if (MoverParameters->MainSwitch(false))
               MoverParameters->EventFlag=true;
             }
             if
           ((!(TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0))||(!(TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)))
             {
              if (MoverParameters->MainSwitch(false))
               MoverParameters->EventFlag=true;
             }
        */
    }

    // McZapkie-260202 - dMoveLen przyda sie przy stukocie kol
    dDOMoveLen =
        GetdMoveLen() + MoverParameters->ComputeMovement(dt, dt1, ts, tp, tmpTraction, l, r);
    // yB: zeby zawsze wrzucalo w jedna strone zakretu
    MoverParameters->AccN *= -ABuGetDirection();
    // if (dDOMoveLen!=0.0) //Ra: nie może być, bo blokuje Event0
    Move(dDOMoveLen);
    if (!bEnabled) // usuwane pojazdy nie mają toru
    { // pojazd do usunięcia
        Global::pGround->bDynamicRemove = true; // sprawdzić
        return false;
    }
    Global::ABuDebug = dDOMoveLen / dt1;
    ResetdMoveLen();
    // McZapkie-260202
    // tupot mew, tfu, stukot kol:
    DWORD stat;
    // taka prowizorka zeby sciszyc stukot dalekiej lokomotywy
    double ObjectDist;
    double vol = 0;
    // double freq; //Ra: nie używane
    ObjectDist = SquareMagnitude(Global::pCameraPosition - vPosition);
    // McZapkie-270202
    if (MyTrack->fSoundDistance != -1)
    {
        if (ObjectDist < rsStukot[0].dSoundAtt * rsStukot[0].dSoundAtt * 15.0)
        {
            vol = (20.0 + MyTrack->iDamageFlag) / 21;
            if (MyTrack->eEnvironment == e_tunnel)
            {
                vol *= 1.1;
                // freq=1.02;
            }
            else if (MyTrack->eEnvironment == e_bridge)
            {
                vol *= 1.2;
                // freq=0.99;                             //MC: stukot w zaleznosci od
                // tego gdzie
                // jest tor
            }
            if (MyTrack->fSoundDistance != dRailLength)
            {
                dRailLength = MyTrack->fSoundDistance;
                for (int i = 0; i < iAxles; i++)
                {
                    dRailPosition[i] = dWheelsPosition[i] + MoverParameters->Dim.L;
                }
            }
            if (dRailLength != -1)
            {
                if (abs(MoverParameters->V) > 0)
                {
                    for (int i = 0; i < iAxles; i++)
                    {
                        dRailPosition[i] -= dDOMoveLen * Sign(dDOMoveLen);
                        if (dRailPosition[i] < 0)
                        {
                            // McZapkie-040302
                            if (i == iAxles - 1)
                            {
                                rsStukot[0].Stop();
                                MoverParameters->AccV +=
                                    0.5 * GetVelocity() / (1 + MoverParameters->Vmax);
                            }
                            else
                            {
                                rsStukot[i + 1].Stop();
                            }
                            rsStukot[i].Play(vol, 0, MechInside,
                                             vPosition); // poprawic pozycje o uklad osi
                            if (i == 1)
                                MoverParameters->AccV -=
                                    0.5 * GetVelocity() / (1 + MoverParameters->Vmax);
                            dRailPosition[i] += dRailLength;
                        }
                    }
                }
            }
        }
    }
    // McZapkie-260202 end

    // yB: przyspieszacz (moze zadziala, ale dzwiek juz jest)
    int flag = MoverParameters->Hamulec->GetSoundFlag();
    if ((bBrakeAcc) && (TestFlag(flag, sf_Acc)) && (ObjectDist < 2500))
    {
        sBrakeAcc->SetVolume(-ObjectDist * 3 - (FreeFlyModeFlag ? 0 : 2000));
        sBrakeAcc->Play(0, 0, 0);
        sBrakeAcc->SetPan(10000 * sin(ModCamRot));
    }
    if ((rsUnbrake.AM != 0) && (ObjectDist < 5000))
    {
        if ((TestFlag(flag, sf_CylU)) &&
            ((MoverParameters->BrakePress * MoverParameters->MaxBrakePress[3]) > 0.05))
        {
            vol = Min0R(
                0.2 +
                    1.6 * sqrt((MoverParameters->BrakePress > 0 ? MoverParameters->BrakePress : 0) /
                               MoverParameters->MaxBrakePress[3]),
                1);
            vol = vol + (FreeFlyModeFlag ? 0 : -0.5) - ObjectDist / 5000;
            rsUnbrake.SetPan(10000 * sin(ModCamRot));
            rsUnbrake.Play(vol, DSBPLAY_LOOPING, MechInside, GetPosition());
        }
        else
            rsUnbrake.Stop();
    }

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
    if ((MoverParameters->TrainType == dt_ET40) || (MoverParameters->TrainType == dt_EP05))
    { // dla ET40 i EU05 automatyczne cofanie nastawnika - i tak
        // nie będzie to działać dobrze...
        /* if
           ((MoverParameters->MainCtrlPos>MoverParameters->MainCtrlActualPos)&&(abs(MoverParameters->Im)>MoverParameters->IminHi))
           {
            MoverParameters->DecMainCtrl(1);
           } */
        if ((!Console::Pressed(Global::Keys[k_IncMainCtrl])) &&
            (MoverParameters->MainCtrlPos > MoverParameters->MainCtrlActualPos))
        {
            MoverParameters->DecMainCtrl(1);
        }
        if ((!Console::Pressed(Global::Keys[k_DecMainCtrl])) &&
            (MoverParameters->MainCtrlPos < MoverParameters->MainCtrlActualPos))
        {
            MoverParameters->IncMainCtrl(1); // Ra 15-01: a to nie miało być tylko cofanie?
        }
    }

    if (MoverParameters->Vel != 0)
    { // McZapkie-050402: krecenie kolami:
        dWheelAngle[0] += 114.59155902616464175359630962821 * MoverParameters->V * dt1 /
                          MoverParameters->WheelDiameterL; // przednie toczne
        dWheelAngle[1] += MoverParameters->nrot * dt1 * 360.0; // napędne
        dWheelAngle[2] += 114.59155902616464175359630962821 * MoverParameters->V * dt1 /
                          MoverParameters->WheelDiameterT; // tylne toczne
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
        double fCurrent = (MoverParameters->DynamicBrakeFlag && MoverParameters->ResistorsFlag ?
                               0 :
                               MoverParameters->Itot) +
                          MoverParameters->TotalCurrent; // prąd pobierany przez pojazd - bez
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
                if (Global::bLiveTraction ? false :
                                            !p->hvPowerWire) // jeśli nie ma drutu, może pooszukiwać
                    MoverParameters->PantFrontVolt =
                        (p->PantWys >= 1.2) ? 0.95 * MoverParameters->EnginePowerSource.MaxVoltage :
                                              0.0;
                else if (MoverParameters->PantFrontUp ? (PantDiff < 0.01) :
                                                        false) // tolerancja niedolegania
                {
                    if ((MoverParameters->PantFrontVolt == 0.0) &&
                        (MoverParameters->PantRearVolt == 0.0))
                        sPantUp.Play(vol, 0, MechInside, vPosition);
                    if (p->hvPowerWire) // TODO: wyliczyć trzeba prąd przypadający na
                    // pantograf i
                    // wstawić do GetVoltage()
                    {
                        MoverParameters->PantFrontVolt =
                            p->hvPowerWire->VoltageGet(MoverParameters->Voltage, fPantCurrent);
                        fCurrent -= fPantCurrent; // taki prąd płynie przez powyższy pantograf
                    }
                    else
                        MoverParameters->PantFrontVolt = 0.0;
                }
                else
                    MoverParameters->PantFrontVolt = 0.0;
                break;
            case 1:
                if (Global::bLiveTraction ? false :
                                            !p->hvPowerWire) // jeśli nie ma drutu, może pooszukiwać
                    MoverParameters->PantRearVolt =
                        (p->PantWys >= 1.2) ? 0.95 * MoverParameters->EnginePowerSource.MaxVoltage :
                                              0.0;
                else if (MoverParameters->PantRearUp ? (PantDiff < 0.01) : false)
                {
                    if ((MoverParameters->PantRearVolt == 0.0) &&
                        (MoverParameters->PantFrontVolt == 0.0))
                        sPantUp.Play(vol, 0, MechInside, vPosition);
                    if (p->hvPowerWire) // TODO: wyliczyć trzeba prąd przypadający na
                    // pantograf i
                    // wstawić do GetVoltage()
                    {
                        MoverParameters->PantRearVolt =
                            p->hvPowerWire->VoltageGet(MoverParameters->Voltage, fPantCurrent);
                        fCurrent -= fPantCurrent; // taki prąd płynie przez powyższy pantograf
                    }
                    else
                        MoverParameters->PantRearVolt = 0.0;
                }
                else
                    MoverParameters->PantRearVolt = 0.0;
                break;
            } // pozostałe na razie nie obsługiwane
            if (MoverParameters->PantPress >
                (MoverParameters->TrainType == dt_EZT ? 2.5 : 3.3)) // Ra 2013-12:
                // Niebugocław
                // mówi, że w EZT
                // podnoszą się
                // przy 2.5
                pantspeedfactor = 0.015 * (MoverParameters->PantPress) *
                                  dt1; // z EXE Kursa  //Ra: wysokość zależy od ciśnienia !!!
            else
                pantspeedfactor = 0.0;
            if (pantspeedfactor < 0)
                pantspeedfactor = 0;
            k = p->fAngleL;
            if (i ? MoverParameters->PantRearUp :
                    MoverParameters->PantFrontUp) // jeśli ma być podniesiony
            {
                if (PantDiff > 0.001) // jeśli nie dolega do drutu
                { // jeśli poprzednia wysokość jest mniejsza niż pożądana, zwiększyć kąt
                    // dolnego
                    // ramienia zgodnie z ciśnieniem
                    if (pantspeedfactor >
                        0.55 * PantDiff) // 0.55 to około pochodna kąta po wysokości
                        k += 0.55 * PantDiff; // ograniczenie "skoku" w danej klatce
                    else
                        k += pantspeedfactor; // dolne ramię
                    // jeśli przekroczono kąt graniczny, zablokować pantograf (wymaga
                    // interwencji
                    // pociągu sieciowego)
                }
                else if (PantDiff < -0.001)
                { // drut się obniżył albo pantograf
                    // podniesiony za wysoko
                    // jeśli wysokość jest zbyt duża, wyznaczyć zmniejszenie kąta
                    // jeśli zmniejszenie kąta jest zbyt duże, przejść do trybu łamania
                    // pantografu
                    // if (PantFrontDiff<-0.05) //skok w dół o 5cm daje złąmanie
                    // pantografu
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
                    p->PantWys = p->fLenL1 * sin(k) + p->fLenU1 * sin(p->fAngleU) +
                                 p->fHeight; // wysokość całości
                }
            }
        } // koniec pętli po pantografach
        if ((MoverParameters->PantFrontSP == false) && (MoverParameters->PantFrontUp == false))
        {
            sPantDown.Play(vol, 0, MechInside, vPosition);
            MoverParameters->PantFrontSP = true;
        }
        if ((MoverParameters->PantRearSP == false) && (MoverParameters->PantRearUp == false))
        {
            sPantDown.Play(vol, 0, MechInside, vPosition);
            MoverParameters->PantRearSP = true;
        }
        if (MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
        { // Winger 240404 - wylaczanie sprezarki i
            // przetwornicy przy braku napiecia
            if (tmpTraction.TractionVoltage == 0)
            { // to coś wyłączało dźwięk silnika w ST43!
                MoverParameters->ConverterFlag = false;
                MoverParameters->CompressorFlag = false; // Ra: to jest wątpliwe - wyłączenie
                // sprężarki powinno być w jednym miejscu!
            }
        }
    }
    else if (MoverParameters->EnginePowerSource.SourceType == InternalSource)
        if (MoverParameters->EnginePowerSource.PowerType == SteamPower)
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
    if ((dDoorMoveL < MoverParameters->DoorMaxShiftL) && (MoverParameters->DoorLeftOpened))
	{
		rsDoorOpen.Play(vol, 0, MechInside, vPosition);
        dDoorMoveL += dt1 * 0.5 * MoverParameters->DoorOpenSpeed;
	}
    if ((dDoorMoveL > 0) && (!MoverParameters->DoorLeftOpened))
    {
		rsDoorClose.Play(vol, 0, MechInside, vPosition);
        dDoorMoveL -= dt1 * MoverParameters->DoorCloseSpeed;
        if (dDoorMoveL < 0)
            dDoorMoveL = 0;
    }
    if ((dDoorMoveR < MoverParameters->DoorMaxShiftR) && (MoverParameters->DoorRightOpened))
	{
		rsDoorOpen.Play(vol, 0, MechInside, vPosition);
        dDoorMoveR += dt1 * 0.5 * MoverParameters->DoorOpenSpeed;
	}
    if ((dDoorMoveR > 0) && (!MoverParameters->DoorRightOpened))
    {
		rsDoorClose.Play(vol, 0, MechInside, vPosition);
        dDoorMoveR -= dt1 * MoverParameters->DoorCloseSpeed;
        if (dDoorMoveR < 0)
            dDoorMoveR = 0;
    }

    // ABu-160303 sledzenie toru przed obiektem: *******************************
    // Z obserwacji: v>0 -> Coupler 0; v<0 ->coupler1 (Ra: prędkość jest związana
    // z pojazdem)
    // Rozroznienie jest tutaj, zeby niepotrzebnie nie skakac do funkcji. Nie jest
    // uzaleznione
    // od obecnosci AI, zeby uwzglednic np. jadace bez lokomotywy wagony.
    // Ra: można by przenieść na poziom obiektu reprezentującego skład, aby nie
    // sprawdzać środkowych
    if (CouplCounter > 25) // licznik, aby nie robić za każdym razem
    { // poszukiwanie czegoś do zderzenia się
        fTrackBlock = 10000.0; // na razie nie ma przeszkód (na wypadek nie
        // uruchomienia skanowania)
        // jeśli nie ma zwrotnicy po drodze, to tylko przeliczyć odległość?
        if (MoverParameters->V > 0.03) //[m/s] jeśli jedzie do przodu (w kierunku Coupler 0)
        {
            if (MoverParameters->Couplers[0].CouplingFlag ==
                ctrain_virtual) // brak pojazdu podpiętego?
            {
                ABuScanObjects(1, fScanDist); // szukanie czegoś do podłączenia
                // WriteLog(asName+" - block 0: "+AnsiString(fTrackBlock));
            }
        }
        else if (MoverParameters->V < -0.03) //[m/s] jeśli jedzie do tyłu (w kierunku Coupler 1)
            if (MoverParameters->Couplers[1].CouplingFlag ==
                ctrain_virtual) // brak pojazdu podpiętego?
            {
                ABuScanObjects(-1, fScanDist);
                // WriteLog(asName+" - block 1: "+AnsiString(fTrackBlock));
            }
        CouplCounter = Random(20); // ponowne sprawdzenie po losowym czasie
    }
    if (MoverParameters->Vel > 0.1) //[km/h]
        ++CouplCounter; // jazda sprzyja poszukiwaniu połączenia
    else
    {
        CouplCounter = 25; // a bezruch nie, ale trzeba zaktualizować odległość, bo
        // zawalidroga może
        // sobie pojechać
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
    if (MoverParameters->LoadStatus)
        LoadUpdate(); // zmiana modelu ładunku
	
	return true; // Ra: chyba tak?
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

    TLocation l;
    l.X = -vPosition.x;
    l.Y = vPosition.z;
    l.Z = vPosition.y;
    TRotation r;
    r.Rx = r.Ry = r.Rz = 0.0;

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

    if (MoverParameters->LoadStatus)
        LoadUpdate(); // zmiana modelu ładunku
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
    btCCtrl1.TurnOff();
    btCCtrl2.TurnOff();
    btCPass1.TurnOff();
    btCPass2.TurnOff();
    btEndSignals11.TurnOff();
    btEndSignals13.TurnOff();
    btEndSignals21.TurnOff();
    btEndSignals23.TurnOff();
    btEndSignals1.TurnOff();
    btEndSignals2.TurnOff();
    btEndSignalsTab1.TurnOff();
    btEndSignalsTab2.TurnOff();
    btHeadSignals11.TurnOff();
    btHeadSignals12.TurnOff();
    btHeadSignals13.TurnOff();
    btHeadSignals21.TurnOff();
    btHeadSignals22.TurnOff();
    btHeadSignals23.TurnOff();
	btMechanik1.TurnOff();
	btMechanik2.TurnOff();
};

void TDynamicObject::Render()
{ // rysowanie elementów nieprzezroczystych
    // youBy - sprawdzamy, czy jest sens renderowac
    double modelrotate;
    vector3 tempangle;
    // zmienne
    renderme = false;
    // przeklejka
    double ObjSqrDist = SquareMagnitude(Global::pCameraPosition - vPosition);
    // koniec przeklejki
    if (ObjSqrDist < 500) // jak jest blisko - do 70m
        modelrotate = 0.01; // mały kąt, żeby nie znikało
    else
    { // Global::pCameraRotation to kąt bewzględny w świecie (zero - na
        // północ)
        tempangle = (vPosition - Global::pCameraPosition); // wektor od kamery
        modelrotate = ABuAcos(tempangle); // określenie kąta
        // if (modelrotate>M_PI) modelrotate-=(2*M_PI);
        modelrotate += Global::pCameraRotation;
    }
    if (modelrotate > M_PI)
        modelrotate -= (2 * M_PI);
    if (modelrotate < -M_PI)
        modelrotate += (2 * M_PI);
    ModCamRot = modelrotate;

    modelrotate = abs(modelrotate);

    if (modelrotate < maxrot)
        renderme = true;

    if (renderme)
    {
        TSubModel::iInstance = (int)this; //żeby nie robić cudzych animacji
        // AnsiString asLoadName="";
        double ObjSqrDist = SquareMagnitude(Global::pCameraPosition - vPosition);
        ABuLittleUpdate(ObjSqrDist); // ustawianie zmiennych submodeli dla wspólnego modelu

// Cone(vCoulpler[0],modelRot.z,0);
// Cone(vCoulpler[1],modelRot.z,1);

// ActualTrack= GetTrack(); //McZapkie-240702

#if RENDER_CONE
        { // Ra: testowe renderowanie pozycji wózków w postaci ostrosłupów, wymaga
            // GLUT32.DLL
            double dir = RadToDeg(atan2(vLeft.z, vLeft.x));
            Axle0.Render(0);
            Axle1.Render(1); // bogieRot[0]
            // if (PrevConnected) //renderowanie połączenia
        }
#endif

        glPushMatrix();
        // vector3 pos= vPosition;
        // double ObjDist= SquareMagnitude(Global::pCameraPosition-pos);
        if (this == Global::pUserDynamic)
        { // specjalne ustawienie, aby nie trzęsło
            if (Global::bSmudge)
            { // jak jest widoczna smuga, to pojazd renderować po
                // wyrenderowaniu smugi
                glPopMatrix(); // a to trzeba zebrać przed wyjściem
                return;
            }
            // if (Global::pWorld->) //tu trzeba by ustawić animacje na modelu
            // zewnętrznym
            glLoadIdentity(); // zacząć od macierzy jedynkowej
            Global::pCamera->SetCabMatrix(vPosition); // specjalne ustawienie kamery
        }
        else
            glTranslated(vPosition.x, vPosition.y,
                         vPosition.z); // standardowe przesunięcie względem początku scenerii
        glMultMatrixd(mMatrix.getArray());
        if (fShade > 0.0)
        { // Ra: zmiana oswietlenia w tunelu, wykopie
            GLfloat ambientLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            GLfloat diffuseLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            GLfloat specularLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            // trochę problem z ambientem w wykopie...
            for (int li = 0; li < 3; li++)
            {
                ambientLight[li] = Global::ambientDayLight[li] * fShade;
                diffuseLight[li] = Global::diffuseDayLight[li] * fShade;
                specularLight[li] = Global::specularDayLight[li] * fShade;
            }
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
            glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
        }
        if (Global::bUseVBO)
        { // wersja VBO
            if (mdLowPolyInt)
                if (FreeFlyModeFlag ? true : !mdKabina || !bDisplayCab)
                    mdLowPolyInt->RaRender(ObjSqrDist, ReplacableSkinID, iAlpha);
            mdModel->RaRender(ObjSqrDist, ReplacableSkinID, iAlpha);
            if (mdLoad) // renderowanie nieprzezroczystego ładunku
                mdLoad->RaRender(ObjSqrDist, ReplacableSkinID, iAlpha);
            if (mdPrzedsionek)
                mdPrzedsionek->RaRender(ObjSqrDist, ReplacableSkinID, iAlpha);
        }
        else
        { // wersja Display Lists
            if (mdLowPolyInt)
                if (FreeFlyModeFlag ? true : !mdKabina || !bDisplayCab)
                    mdLowPolyInt->Render(ObjSqrDist, ReplacableSkinID, iAlpha);
            mdModel->Render(ObjSqrDist, ReplacableSkinID, iAlpha);
            if (mdLoad) // renderowanie nieprzezroczystego ładunku
                mdLoad->Render(ObjSqrDist, ReplacableSkinID, iAlpha);
            if (mdPrzedsionek)
                mdPrzedsionek->Render(ObjSqrDist, ReplacableSkinID, iAlpha);
        }

        // Ra: czy ta kabina tu ma sens?
        // Ra: czy nie renderuje się dwukrotnie?
        // Ra: dlaczego jest zablokowana w przezroczystych?
        if (mdKabina) // jeśli ma model kabiny
            if ((mdKabina != mdModel) && bDisplayCab && FreeFlyModeFlag)
            { // rendering kabiny gdy jest oddzielnym modelem i
                // ma byc wyswietlana
                // ABu: tylko w trybie FreeFly, zwykly tryb w world.cpp
                // Ra: świetła są ustawione dla zewnętrza danego pojazdu
                // oswietlenie kabiny
                GLfloat ambientCabLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
                GLfloat diffuseCabLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
                GLfloat specularCabLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
                for (int li = 0; li < 3; li++)
                {
                    ambientCabLight[li] = Global::ambientDayLight[li] * 0.9;
                    diffuseCabLight[li] = Global::diffuseDayLight[li] * 0.5;
                    specularCabLight[li] = Global::specularDayLight[li] * 0.5;
                }
                switch (MyTrack->eEnvironment)
                {
                case e_canyon:
                {
                    for (int li = 0; li < 3; li++)
                    {
                        diffuseCabLight[li] *= 0.6;
                        specularCabLight[li] *= 0.7;
                    }
                }
                break;
                case e_tunnel:
                {
                    for (int li = 0; li < 3; li++)
                    {
                        ambientCabLight[li] *= 0.3;
                        diffuseCabLight[li] *= 0.1;
                        specularCabLight[li] *= 0.2;
                    }
                }
                break;
                }
                glLightfv(GL_LIGHT0, GL_AMBIENT, ambientCabLight);
                glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseCabLight);
                glLightfv(GL_LIGHT0, GL_SPECULAR, specularCabLight);
                if (Global::bUseVBO)
                    mdKabina->RaRender(ObjSqrDist, 0);
                else
                    mdKabina->Render(ObjSqrDist, 0);
                glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);
                glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);
                glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);
            }
        if (fShade != 0.0) // tylko jeśli było zmieniane
        { // przywrócenie standardowego oświetlenia
            glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);
            glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);
        }
        glPopMatrix();
        if (btnOn)
            TurnOff(); // przywrócenie domyślnych pozycji submodeli
    } // yB - koniec mieszania z grafika
};

void TDynamicObject::RenderSounds()
{ // przeliczanie dźwięków, bo będzie
    // słychać bez wyświetlania sektora z
    // pojazdem
    // McZapkie-010302: ulepszony dzwiek silnika
    double freq;
    double vol = 0;
    double dt = Timer::GetDeltaTime();

    //    double sounddist;
    //    sounddist=SquareMagnitude(Global::pCameraPosition-vPosition);

    if (MoverParameters->Power > 0)
    {
        if ((rsSilnik.AM != 0) && ((MoverParameters->Mains) || (MoverParameters->EngineType ==
                                                                DieselEngine))) // McZapkie-280503:
        // zeby dla dumb
        // dzialal silnik na
        // jalowych obrotach
        {
            if ((fabs(MoverParameters->enrot) > 0.01) ||
                (MoverParameters->EngineType == Dumb)) //&& (MoverParameters->EnginePower>0.1))
            {
                freq = rsSilnik.FM * fabs(MoverParameters->enrot) + rsSilnik.FA;
                if (MoverParameters->EngineType == Dumb)
                    freq = freq -
                           0.2 * MoverParameters->EnginePower / (1 + MoverParameters->Power * 1000);
                rsSilnik.AdjFreq(freq, dt);
                if (MoverParameters->EngineType == DieselEngine)
                {
                    if (MoverParameters->enrot > 0)
                    {
                        if (MoverParameters->EnginePower > 0)
                            vol = rsSilnik.AM * MoverParameters->dizel_fill + rsSilnik.AA;
                        else
                            vol =
                                rsSilnik.AM * fabs(MoverParameters->enrot / MoverParameters->dizel_nmax) +
                                rsSilnik.AA * 0.9;
                    }
                    else
                        vol = 0;
                }
                else if (MoverParameters->EngineType == DieselElectric)
                    vol = rsSilnik.AM *
                              (MoverParameters->EnginePower / 1000 / MoverParameters->Power) +
                          0.2 * (MoverParameters->enrot * 60) /
                              (MoverParameters->DElist[MoverParameters->MainCtrlPosNo].RPM) +
                          rsSilnik.AA;
                else if (MoverParameters->EngineType == ElectricInductionMotor)
                    vol = rsSilnik.AM *
                              (MoverParameters->EnginePower + fabs(MoverParameters->enrot * 2)) +
                          rsSilnik.AA;
                else
                    vol = rsSilnik.AM * (MoverParameters->EnginePower / 1000 +
                                         fabs(MoverParameters->enrot) * 60.0) +
                          rsSilnik.AA;
                //            McZapkie-250302 - natezenie zalezne od obrotow i mocy
                if ((vol < 1) && (MoverParameters->EngineType == ElectricSeriesMotor) &&
                    (MoverParameters->EnginePower < 100))
                {
                    float volrnd =
                        Random(100) * MoverParameters->enrot / (1 + MoverParameters->nmax);
                    if (volrnd < 2)
                        vol = vol + volrnd / 200.0;
                }
                switch (MyTrack->eEnvironment)
                {
                case e_tunnel:
                {
                    vol += 0.1;
                }
                break;
                case e_canyon:
                {
                    vol += 0.05;
                }
                break;
                }
                if ((MoverParameters->DynamicBrakeFlag) && (MoverParameters->EnginePower > 0.1) &&
                    (MoverParameters->EngineType ==
                     ElectricSeriesMotor)) // Szociu - 29012012 - jeżeli uruchomiony
                    // jest  hamulec
                    // elektrodynamiczny, odtwarzany jest dźwięk silnika
                    vol += 0.8;

                if (enginevolume > 0.0001)
                    if (MoverParameters->EngineType != DieselElectric)
                    {
                        rsSilnik.Play(enginevolume, DSBPLAY_LOOPING, MechInside, GetPosition());
                    }
                    else
                    {
                        sConverter.UpdateAF(vol, freq, MechInside, GetPosition());

                        float fincvol;
                        fincvol = 0;
                        if ((MoverParameters->ConverterFlag) &&
                            (MoverParameters->enrot * 60 > MoverParameters->DElist[0].RPM))
                        {
                            fincvol = (MoverParameters->DElist[MoverParameters->MainCtrlPos].RPM -
                                       (MoverParameters->enrot * 60));
                            fincvol /= (0.05 * MoverParameters->DElist[0].RPM);
                        };
                        if (fincvol > 0.02)
                            rsDiesielInc.Play(fincvol, DSBPLAY_LOOPING, MechInside, GetPosition());
                        else
                            rsDiesielInc.Stop();
                    }
            }
            else
                rsSilnik.Stop();
        }
        enginevolume = (enginevolume + vol) / 2;
        if (enginevolume < 0.01)
            rsSilnik.Stop();
        if ((MoverParameters->EngineType == ElectricSeriesMotor) ||
            (MoverParameters->EngineType == ElectricInductionMotor) && rsWentylator.AM != 0)
        {
            if (MoverParameters->RventRot > 0.1)
            {
                freq = rsWentylator.FM * MoverParameters->RventRot + rsWentylator.FA;
                rsWentylator.AdjFreq(freq, dt);
                if (MoverParameters->EngineType == ElectricInductionMotor)
                    vol =
                        rsWentylator.AM * sqrt(fabs(MoverParameters->dizel_fill)) + rsWentylator.AA;
                else
                    vol = rsWentylator.AM * MoverParameters->RventRot + rsWentylator.AA;
                rsWentylator.Play(vol, DSBPLAY_LOOPING, MechInside, GetPosition());
            }
            else
                rsWentylator.Stop();
        }
        if (MoverParameters->TrainType == dt_ET40)
        {
            if (MoverParameters->Vel > 0.1)
            {
                freq = rsPrzekladnia.FM * (MoverParameters->Vel) + rsPrzekladnia.FA;
                rsPrzekladnia.AdjFreq(freq, dt);
                vol = rsPrzekladnia.AM * (MoverParameters->Vel) + rsPrzekladnia.AA;
                rsPrzekladnia.Play(vol, DSBPLAY_LOOPING, MechInside, GetPosition());
            }
            else
                rsPrzekladnia.Stop();
        }
    }

    // youBy: dzwiek ostrych lukow i ciasnych zwrotek

    if ((ts.R * ts.R > 1) && (MoverParameters->Vel > 0))
        vol = MoverParameters->AccN * MoverParameters->AccN;
    else
        vol = 0;
    //       vol+=(50000/ts.R*ts.R);

    if (vol > 0.001)
    {
        rscurve.Play(2 * vol, DSBPLAY_LOOPING, MechInside, GetPosition());
    }
    else
        rscurve.Stop();

    // McZapkie-280302 - pisk mocno zacisnietych hamulcow - trzeba jeszcze
    // zabezpieczyc przed
    // brakiem deklaracji w mmedia.dta
    if (rsPisk.AM != 0)
    {
        if ((MoverParameters->Vel > (rsPisk.GetStatus() != 0 ? 0.01 : 0.5)) &&
            (!MoverParameters->SlippingWheels) && (MoverParameters->UnitBrakeForce > rsPisk.AM))
        {
            vol = MoverParameters->UnitBrakeForce / (rsPisk.AM + 1) + rsPisk.AA;
            rsPisk.Play(vol, DSBPLAY_LOOPING, MechInside, GetPosition());
        }
        else
            rsPisk.Stop();
    }
	
	if (MoverParameters->SandDose) // Dzwiek piasecznicy
		sSand.TurnOn(MechInside, GetPosition());
	else
		sSand.TurnOff(MechInside, GetPosition());
	sSand.Update(MechInside, GetPosition());
	if (MoverParameters->Hamulec->GetStatus() & b_rls) // Dzwiek odluzniacza
		sReleaser.TurnOn(MechInside, GetPosition());
	else
		sReleaser.TurnOff(MechInside, GetPosition());
	//sReleaser.Update(MechInside, GetPosition());
	double releaser_vol = 1;
	if (MoverParameters->BrakePress < 0.1)
		releaser_vol = MoverParameters->BrakePress * 10;
	sReleaser.UpdateAF(releaser_vol, 1, MechInside, GetPosition());
    // if ((MoverParameters->ConverterFlag==false) &&
    // (MoverParameters->TrainType!=dt_ET22))
    // if
    // ((MoverParameters->ConverterFlag==false)&&(MoverParameters->CompressorPower!=0))
    // MoverParameters->CompressorFlag=false; //Ra: wywalić to stąd, tu tylko dla
    // wyświetlanych!
    // Ra: no to już wiemy, dlaczego pociągi jeżdżą lepiej, gdy się na nie patrzy!
    // if (MoverParameters->CompressorPower==2)
    // MoverParameters->CompressorAllow=MoverParameters->ConverterFlag;

    // McZapkie! - dzwiek compressor.wav tylko gdy dziala sprezarka
    if (MoverParameters->VeselVolume != 0)
    {
        if (MoverParameters->CompressorFlag)
            sCompressor.TurnOn(MechInside, GetPosition());
        else
            sCompressor.TurnOff(MechInside, GetPosition());
        sCompressor.Update(MechInside, GetPosition());
    }
    if (MoverParameters->PantCompFlag) // Winger 160404 - dzwiek malej sprezarki
        sSmallCompressor.TurnOn(MechInside, GetPosition());
    else
        sSmallCompressor.TurnOff(MechInside, GetPosition());
    sSmallCompressor.Update(MechInside, GetPosition());

    // youBy - przenioslem, bo diesel tez moze miec turbo
    if ((MoverParameters->MainCtrlPos) >=
        (MoverParameters->TurboTest)) // hunter-250312: dlaczego zakomentowane?
    // Ra: bo nie działało dobrze
    {
        // udawanie turbo:  (6.66*(eng_vol-0.85))
        if (eng_turbo > 6.66 * (enginevolume - 0.8) + 0.2 * dt)
            eng_turbo = eng_turbo - 0.2 * dt; // 0.125
        else if (eng_turbo < 6.66 * (enginevolume - 0.8) - 0.4 * dt)
            eng_turbo = eng_turbo + 0.4 * dt; // 0.333
        else
            eng_turbo = 6.66 * (enginevolume - 0.8);

        sTurbo.TurnOn(MechInside, GetPosition());
        // sTurbo.UpdateAF(eng_turbo,0.7+(eng_turbo*0.6),MechInside,GetPosition());
        sTurbo.UpdateAF(3 * eng_turbo - 1, 0.4 + eng_turbo * 0.4, MechInside, GetPosition());
        //    eng_vol_act=enginevolume;
        // eng_frq_act=eng_frq;
    }
    else
        sTurbo.TurnOff(MechInside, GetPosition());

    if (MoverParameters->TrainType == dt_PseudoDiesel)
    {
        // ABu: udawanie woodwarda dla lok. spalinowych
        // jesli silnik jest podpiety pod dzwiek przetwornicy
        if (MoverParameters->ConverterFlag) // NBMX dzwiek przetwornicy
        {
            sConverter.TurnOn(MechInside, GetPosition());
        }
        else
            sConverter.TurnOff(MechInside, GetPosition());

        // glosnosc zalezy od stosunku mocy silnika el. do mocy max
        double eng_vol;
        if (MoverParameters->Power > 1)
            // 0.85+0.000015*(...)
            eng_vol = 0.8 + 0.00002 * (MoverParameters->EnginePower / MoverParameters->Power);
        else
            eng_vol = 1;

        eng_dfrq = eng_dfrq + (eng_vol_act - eng_vol);
        if (eng_dfrq > 0)
        {
            eng_dfrq = eng_dfrq - 0.025 * dt;
            if (eng_dfrq < 0.025 * dt)
                eng_dfrq = 0;
        }
        else if (eng_dfrq < 0)
        {
            eng_dfrq = eng_dfrq + 0.025 * dt;
            if (eng_dfrq > -0.025 * dt)
                eng_dfrq = 0;
        }
        double defrot;
        if (MoverParameters->MainCtrlPos != 0)
        {
            double CtrlPos = MoverParameters->MainCtrlPos;
            double CtrlPosNo = MoverParameters->MainCtrlPosNo;
            // defrot=1+0.4*(CtrlPos/CtrlPosNo);
            defrot = 1 + 0.5 * (CtrlPos / CtrlPosNo);
        }
        else
            defrot = 1;

        if (eng_frq_act < defrot)
        {
            // if (MoverParameters->MainCtrlPos==1) eng_frq_act=eng_frq_act+0.1*dt;
            eng_frq_act = eng_frq_act + 0.4 * dt; // 0.05
            if (eng_frq_act > defrot - 0.4 * dt)
                eng_frq_act = defrot;
        }
        else if (eng_frq_act > defrot)
        {
            eng_frq_act = eng_frq_act - 0.1 * dt; // 0.05
            if (eng_frq_act < defrot + 0.1 * dt)
                eng_frq_act = defrot;
        }
        sConverter.UpdateAF(eng_vol_act, eng_frq_act + eng_dfrq, MechInside, GetPosition());
        // udawanie turbo:  (6.66*(eng_vol-0.85))
        if (eng_turbo > 6.66 * (eng_vol - 0.8) + 0.2 * dt)
            eng_turbo = eng_turbo - 0.2 * dt; // 0.125
        else if (eng_turbo < 6.66 * (eng_vol - 0.8) - 0.4 * dt)
            eng_turbo = eng_turbo + 0.4 * dt; // 0.333
        else
            eng_turbo = 6.66 * (eng_vol - 0.8);

        sTurbo.TurnOn(MechInside, GetPosition());
        // sTurbo.UpdateAF(eng_turbo,0.7+(eng_turbo*0.6),MechInside,GetPosition());
        sTurbo.UpdateAF(3 * eng_turbo - 1, 0.4 + eng_turbo * 0.4, MechInside, GetPosition());
        eng_vol_act = eng_vol;
        // eng_frq_act=eng_frq;
    }
    else
    {
        if (MoverParameters->ConverterFlag) // NBMX dzwiek przetwornicy
            sConverter.TurnOn(MechInside, GetPosition());
        else
            sConverter.TurnOff(MechInside, GetPosition());
        sConverter.Update(MechInside, GetPosition());
    }
    if (MoverParameters->WarningSignal > 0)
    {
        if (TestFlag(MoverParameters->WarningSignal, 1))
            sHorn1.TurnOn(MechInside, GetPosition());
        else
            sHorn1.TurnOff(MechInside, GetPosition());
        if (TestFlag(MoverParameters->WarningSignal, 2))
            sHorn2.TurnOn(MechInside, GetPosition());
        else
            sHorn2.TurnOff(MechInside, GetPosition());
    }
    else
    {
        sHorn1.TurnOff(MechInside, GetPosition());
        sHorn2.TurnOff(MechInside, GetPosition());
    }
    if (MoverParameters->DoorClosureWarning)
    {
        if (MoverParameters->DepartureSignal) // NBMX sygnal odjazdu, MC: pod warunkiem ze jest
            // zdefiniowane w chk
            sDepartureSignal.TurnOn(MechInside, GetPosition());
        else
            sDepartureSignal.TurnOff(MechInside, GetPosition());
        sDepartureSignal.Update(MechInside, GetPosition());
    }
    sHorn1.Update(MechInside, GetPosition());
    sHorn2.Update(MechInside, GetPosition());
    // McZapkie: w razie wykolejenia
    if (MoverParameters->EventFlag)
    {
        if (TestFlag(MoverParameters->DamageFlag, dtrain_out) && GetVelocity() > 0)
            rsDerailment.Play(1, 0, true, GetPosition());
        if (GetVelocity() == 0)
            rsDerailment.Stop();
    }
    /* //Ra: dwa razy?
     if (MoverParameters->EventFlag)
     {
      if (TestFlag(MoverParameters->DamageFlag,dtrain_out) && GetVelocity()>0)
        rsDerailment.Play(1,0,true,GetPosition());
      if (GetVelocity()==0)
        rsDerailment.Stop();
     }
    */
};

void TDynamicObject::RenderAlpha()
{ // rysowanie elementów półprzezroczystych
    if (renderme)
    {
        TSubModel::iInstance = (int)this; //żeby nie robić cudzych animacji
        double ObjSqrDist = SquareMagnitude(Global::pCameraPosition - vPosition);
        ABuLittleUpdate(ObjSqrDist); // ustawianie zmiennych submodeli dla wspólnego modelu
        glPushMatrix();
        if (this == Global::pUserDynamic)
        { // specjalne ustawienie, aby nie trzęsło
            if (Global::bSmudge)
            { // jak smuga, to rysować po smudze
                glPopMatrix(); // to trzeba zebrać przed wyściem
                return;
            }
            glLoadIdentity(); // zacząć od macierzy jedynkowej
            Global::pCamera->SetCabMatrix(vPosition); // specjalne ustawienie kamery
        }
        else
            glTranslated(vPosition.x, vPosition.y,
                         vPosition.z); // standardowe przesunięcie względem początku scenerii
        glMultMatrixd(mMatrix.getArray());
        if (fShade > 0.0)
        { // Ra: zmiana oswietlenia w tunelu, wykopie
            GLfloat ambientLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            GLfloat diffuseLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            GLfloat specularLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            // trochę problem z ambientem w wykopie...
            for (int li = 0; li < 3; li++)
            {
                ambientLight[li] = Global::ambientDayLight[li] * fShade;
                diffuseLight[li] = Global::diffuseDayLight[li] * fShade;
                specularLight[li] = Global::specularDayLight[li] * fShade;
            }
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
            glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
        }
        if (Global::bUseVBO)
        { // wersja VBO
            if (mdLowPolyInt)
                if (FreeFlyModeFlag ? true : !mdKabina || !bDisplayCab)
                    mdLowPolyInt->RaRenderAlpha(ObjSqrDist, ReplacableSkinID, iAlpha);
            mdModel->RaRenderAlpha(ObjSqrDist, ReplacableSkinID, iAlpha);
            if (mdLoad)
                mdLoad->RaRenderAlpha(ObjSqrDist, ReplacableSkinID, iAlpha);
            // if (mdPrzedsionek) //Ra: przedsionków tu wcześniej nie było - włączyć?
            // mdPrzedsionek->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
        }
        else
        { // wersja Display Lists
            if (mdLowPolyInt)
                if (FreeFlyModeFlag ? true : !mdKabina || !bDisplayCab)
                    mdLowPolyInt->RenderAlpha(ObjSqrDist, ReplacableSkinID, iAlpha);
            mdModel->RenderAlpha(ObjSqrDist, ReplacableSkinID, iAlpha);
            if (mdLoad)
                mdLoad->RenderAlpha(ObjSqrDist, ReplacableSkinID, iAlpha);
            // if (mdPrzedsionek) //Ra: przedsionków tu wcześniej nie było - włączyć?
            // mdPrzedsionek->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
        }
        /* skoro false to można wyciąc
            //ABu: Tylko w trybie freefly
            if (false)//((mdKabina!=mdModel) && bDisplayCab && FreeFlyModeFlag)
            {
        //oswietlenie kabiny
              GLfloat  ambientCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
              GLfloat  diffuseCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
              GLfloat  specularCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
              for (int li=0; li<3; li++)
               {
                 ambientCabLight[li]= Global::ambientDayLight[li]*0.9;
                 diffuseCabLight[li]= Global::diffuseDayLight[li]*0.5;
                 specularCabLight[li]= Global::specularDayLight[li]*0.5;
               }
              switch (MyTrack->eEnvironment)
              {
               case e_canyon:
                {
                  for (int li=0; li<3; li++)
                   {
                     diffuseCabLight[li]*= 0.6;
                     specularCabLight[li]*= 0.8;
                   }
                }
               break;
               case e_tunnel:
                {
                  for (int li=0; li<3; li++)
                   {
                     ambientCabLight[li]*= 0.3;
                     diffuseCabLight[li]*= 0.1;
                     specularCabLight[li]*= 0.2;
                   }
                }
               break;
              }
        // dorobic swiatlo od drugiej strony szyby

              glLightfv(GL_LIGHT0,GL_AMBIENT,ambientCabLight);
              glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseCabLight);
              glLightfv(GL_LIGHT0,GL_SPECULAR,specularCabLight);

              mdKabina->RenderAlpha(ObjSqrDist,0);
        //smierdzi
        // mdModel->RenderAlpha(SquareMagnitude(Global::pCameraPosition-pos),0);

              glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
              glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
              glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
            }
        */
        if (fShade != 0.0) // tylko jeśli było zmieniane
        { // przywrócenie standardowego oświetlenia
            glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);
            glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);
        }
        glPopMatrix();
        if (btnOn)
            TurnOff(); // przywrócenie domyślnych pozycji submodeli
    }
    return;
} // koniec renderalpha

// McZapkie-250202
// wczytywanie pliku z danymi multimedialnymi (dzwieki)
void TDynamicObject::LoadMMediaFile(std::string BaseDir, std::string TypeName,
                                    std::string ReplacableSkin)
{
    double dSDist;
    // asBaseDir=BaseDir;
    Global::asCurrentDynamicPath = BaseDir;
    std::string asFileName = BaseDir + TypeName + ".mmd";
    std::string asLoadName;
    if( false == MoverParameters->LoadType.empty() ) {
        asLoadName = BaseDir + MoverParameters->LoadType + ".t3d";
    }

    std::string asAnimName;
    bool Stop_InternalData = false;
    pants = NULL; // wskaźnik pierwszego obiektu animującego dla pantografów
	cParser parser( TypeName + ".mmd", cParser::buffer_FILE, BaseDir );
	std::string token;
    int i;
    do {
		token = "";
		parser.getTokens(); parser >> token;

		if( token == "models:") {
			// modele i podmodele
            iMultiTex = 0; // czy jest wiele tekstur wymiennych?
			parser.getTokens();
			parser >> asModel;
            if( asModel[asModel.size() - 1] == '#' ) // Ra 2015-01: nie podoba mi siê to
            { // model wymaga wielu tekstur wymiennych
                iMultiTex = 1;
                asModel.erase( asModel.length() - 1 );
            }
            if ((i = asModel.find(',')) != std::string::npos)
            { // Ra 2015-01: może szukać przecinka w
                // nazwie modelu, a po przecinku była by
                // liczba
                // tekstur?
                if (i < asModel.length())
                    iMultiTex = asModel[i + 1] - '0';
                if (iMultiTex < 0)
                    iMultiTex = 0;
                else if (iMultiTex > 1)
                    iMultiTex = 1; // na razie ustawiamy na 1
            }
            asModel = BaseDir + asModel; // McZapkie 2002-07-20: dynamics maja swoje
            // modele w dynamics/basedir
            Global::asCurrentTexturePath = BaseDir; // biezaca sciezka do tekstur to dynamic/...
            mdModel = TModelsManager::GetModel(asModel, true);
            assert( mdModel != nullptr ); // TODO: handle this more gracefully than all going to shit
            if (ReplacableSkin != "none")
            { // tekstura wymienna jest raczej jedynie w "dynamic\"
                ReplacableSkin =
                    Global::asCurrentTexturePath + ReplacableSkin; // skory tez z dynamic/...
					std::string x = TextureTest(Global::asCurrentTexturePath + "nowhere"); // na razie prymitywnie
					if (!x.empty())
						ReplacableSkinID[4] = TTexturesManager::GetTextureID(NULL, NULL, Global::asCurrentTexturePath + "nowhere", 9);
					/*
                if ((i = ReplacableSkin.Pos("|")) > 0) // replacable dzielone
                {
                    iMultiTex = -1;
                    ReplacableSkinID[-iMultiTex] = TTexturesManager::GetTextureID(
                        NULL, NULL, ReplacableSkin.SubString(1, i - 1).c_str(),
                        Global::iDynamicFiltering);
                    ReplacableSkin.Delete(1, i); // usunięcie razem z pionową kreską
                    ReplacableSkin = Global::asCurrentTexturePath +
                                     ReplacableSkin; // odtworzenie początku ścieżki
                    // sprawdzić, ile jest i ustawić iMultiTex na liczbę podanych tekstur
                    if (!ReplacableSkin.IsEmpty())
                    { // próba wycięcia drugiej nazwy
                        iMultiTex = -2; // skoro zostało coś po kresce, to są co najmniej dwie
                        if ((i = ReplacableSkin.Pos("|")) == 0) // gdy nie ma już kreski
                            ReplacableSkinID[-iMultiTex] = TTexturesManager::GetTextureID(
                                NULL, NULL, ReplacableSkin.SubString(1, i - 1).c_str(),
                                Global::iDynamicFiltering);
                        else
                        { // jak jest kreska, to wczytać drugą i próbować trzecią
                            ReplacableSkinID[-iMultiTex] = TTexturesManager::GetTextureID(
                                NULL, NULL, ReplacableSkin.SubString(1, i - 1).c_str(),
                                Global::iDynamicFiltering);
                            ReplacableSkin.Delete(1, i); // usunięcie razem z pionową kreską
                            ReplacableSkin = Global::asCurrentTexturePath +
                                             ReplacableSkin; // odtworzenie początku ścieżki
                            if (!ReplacableSkin.IsEmpty())
                            { // próba wycięcia trzeciej nazwy
                                iMultiTex =
                                    -3; // skoro zostało coś po kresce, to są co najmniej trzy
                                if ((i = ReplacableSkin.Pos("|")) == 0) // gdy nie ma już kreski
                                    ReplacableSkinID[-iMultiTex] = TTexturesManager::GetTextureID(
                                        NULL, NULL, ReplacableSkin.SubString(1, i - 1).c_str(),
                                        Global::iDynamicFiltering);
                                else
                                { // jak jest kreska, to wczytać trzecią i próbować czwartą
                                    ReplacableSkinID[-iMultiTex] = TTexturesManager::GetTextureID(
                                        NULL, NULL, ReplacableSkin.SubString(1, i - 1).c_str(),
                                        Global::iDynamicFiltering);
                                    ReplacableSkin.Delete(1, i); // usunięcie razem z pionową kreską
                                    ReplacableSkin = Global::asCurrentTexturePath +
                                                     ReplacableSkin; // odtworzenie początku ścieżki
                                    if (!ReplacableSkin.IsEmpty())
                                    { // próba wycięcia trzeciej nazwy
                                        iMultiTex = -4; // skoro zostało coś po kresce, to są co
                                        // najmniej cztery
                                        ReplacableSkinID[-iMultiTex] =
                                            TTexturesManager::GetTextureID(
                                                NULL, NULL,
                                                ReplacableSkin.SubString(1, i - 1).c_str(),
                                                Global::iDynamicFiltering);
                                        // więcej na razie nie zadziała, a u tak trzeba to do modeli
                                        // przenieść
                                    }
                                }
                            }
                        }
                    }
                }
				*/
                if (iMultiTex > 0)
                { // jeśli model ma 4 tekstury
                    ReplacableSkinID[1] = TTexturesManager::GetTextureID(
                        NULL, NULL, ReplacableSkin + ",1", Global::iDynamicFiltering);
                    if (ReplacableSkinID[1])
                    { // pierwsza z zestawu znaleziona
                        ReplacableSkinID[2] = TTexturesManager::GetTextureID(
                            NULL, NULL, ReplacableSkin + ",2", Global::iDynamicFiltering);
                        if (ReplacableSkinID[2])
                        {
                            iMultiTex = 2; // już są dwie
                            ReplacableSkinID[3] = TTexturesManager::GetTextureID(
                                NULL, NULL, ReplacableSkin + ",3",
                                Global::iDynamicFiltering);
                            if (ReplacableSkinID[3])
                            {
                                iMultiTex = 3; // a teraz nawet trzy
                                ReplacableSkinID[4] = TTexturesManager::GetTextureID(
                                    NULL, NULL, ReplacableSkin + ",4",
                                    Global::iDynamicFiltering);
                                if (ReplacableSkinID[4])
                                    iMultiTex = 4; // jak są cztery, to blokujemy podmianę tekstury
                                // rozkładem
                            }
                        }
                    }
                    else
                    { // zestaw nie zadziałał, próbujemy normanie
                        iMultiTex = 0;
                        ReplacableSkinID[1] = TTexturesManager::GetTextureID(
                            NULL, NULL, ReplacableSkin, Global::iDynamicFiltering);
                    }
                }
                else
                    ReplacableSkinID[1] = TTexturesManager::GetTextureID(
                        NULL, NULL, ReplacableSkin, Global::iDynamicFiltering);
                if (TTexturesManager::GetAlpha(ReplacableSkinID[1]))
                    iAlpha = 0x31310031; // tekstura -1 z kanałem alfa - nie renderować w cyklu
                // nieprzezroczystych
                else
                    iAlpha = 0x30300030; // wszystkie tekstury nieprzezroczyste - nie
                // renderować w
                // cyklu przezroczystych
                if (ReplacableSkinID[2])
                    if (TTexturesManager::GetAlpha(ReplacableSkinID[2]))
                        iAlpha |= 0x02020002; // tekstura -2 z kanałem alfa - nie renderować
                // w cyklu
                // nieprzezroczystych
                if (ReplacableSkinID[3])
                    if (TTexturesManager::GetAlpha(ReplacableSkinID[3]))
                        iAlpha |= 0x04040004; // tekstura -3 z kanałem alfa - nie renderować
                // w cyklu
                // nieprzezroczystych
                if (ReplacableSkinID[4])
                    if (TTexturesManager::GetAlpha(ReplacableSkinID[4]))
                        iAlpha |= 0x08080008; // tekstura -4 z kanałem alfa - nie renderować
                // w cyklu
                // nieprzezroczystych
            }
            // Winger 040304 - ladowanie przedsionkow dla EZT
            if (MoverParameters->TrainType == dt_EZT)
            {
                asModel = "przedsionki.t3d";
                asModel = BaseDir + asModel;
                mdPrzedsionek = TModelsManager::GetModel(asModel, true);
            }
            if (!MoverParameters->LoadAccepted.empty())
                // if (MoverParameters->LoadAccepted!=AnsiString("")); // &&
                // MoverParameters->LoadType!=AnsiString("passengers"))
                if (MoverParameters->EnginePowerSource.SourceType == CurrentCollector)
                { // wartość niby "pantstate" - nazwa dla formalności, ważna jest ilość
                    if (MoverParameters->Load == 1)
                        MoverParameters->PantFront(true);
                    else if (MoverParameters->Load == 2)
                        MoverParameters->PantRear(true);
                    else if (MoverParameters->Load == 3)
                    {
                        MoverParameters->PantFront(true);
                        MoverParameters->PantRear(true);
                    }
                    else if (MoverParameters->Load == 4)
                        MoverParameters->DoubleTr = -1;
                    else if (MoverParameters->Load == 5)
                    {
                        MoverParameters->DoubleTr = -1;
                        MoverParameters->PantRear(true);
                    }
                    else if (MoverParameters->Load == 6)
                    {
                        MoverParameters->DoubleTr = -1;
                        MoverParameters->PantFront(true);
                    }
                    else if (MoverParameters->Load == 7)
                    {
                        MoverParameters->DoubleTr = -1;
                        MoverParameters->PantFront(true);
                        MoverParameters->PantRear(true);
                    }
                }
                else // Ra: tu wczytywanie modelu ładunku jest w porządku
                {
                    if( false == asLoadName.empty() ) {
                        mdLoad = TModelsManager::GetModel( asLoadName, true ); // ladunek
                    }
                }
            Global::asCurrentTexturePath = szTexturePath; // z powrotem defaultowa sciezka do tekstur
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
						int co = 0, ile = -1;
                        iAnimations = 0;
                        do
                        { // kolejne liczby to ilość animacj, -1 to znacznik końca
							parser.getTokens( 1, false );
                            parser >> ile; // ilość danego typu animacji
                            // if (co==ANIM_PANTS)
                            // if (!Global::bLoadTraction)
                            //  if (!DebugModeFlag) //w debugmode pantografy mają "niby działać"
                            //   ile=0; //wyłączenie animacji pantografów
                            if (co < ANIM_TYPES)
                                if (ile >= 0)
                                {
                                    iAnimType[co] = ile; // zapamiętanie
                                    iAnimations += ile; // ogólna ilość animacji
                                }
                            ++co;
                        } while (ile >= 0); //-1 to znacznik końca

						while( co < ANIM_TYPES ) {
							iAnimType[ co++ ] = 0; // zerowanie pozostałych
                        }
						parser.getTokens(); parser >> token; // NOTE: should this be here? seems at best superfluous
                    }
                    // WriteLog("Total animations: "+AnsiString(iAnimations));
                }
/*
                if( nullptr == pAnimations )
*/
                if( true == pAnimations.empty() )
                { // Ra: tworzenie tabeli animacji, jeśli jeszcze nie było
                    if (!iAnimations) // jeśli nie podano jawnie, ile ma być animacji
                        iAnimations = 28; // tyle było kiedyś w każdym pojeździe (2 wiązary wypadły)
                    /* //pojazd może mieć pantograf do innych celów niż napęd
                    if (MoverParameters->EnginePowerSource.SourceType!=CurrentCollector)
                    {//nie będzie pantografów, to się trochę uprości
                     iAnimations-=iAnimType[ANIM_PANTS]; //domyślnie były 2 pantografy
                     iAnimType[ANIM_PANTS]=0;
                    }
                    */
/*
                    pAnimations = new TAnim[iAnimations];
*/
                    pAnimations.resize( iAnimations );
                    int i, j, k = 0, sm = 0;
                    for (j = 0; j < ANIM_TYPES; ++j)
                        for (i = 0; i < iAnimType[j]; ++i)
                        {
                            if (j == ANIM_PANTS) // zliczamy poprzednie animacje
                                if (!pants)
                                    if (iAnimType[ANIM_PANTS]) // o ile jakieś pantografy są (a domyślnie są)
                                        pants = &pAnimations[k]; // zapamiętanie na potrzeby wyszukania submodeli
/*
                                        pants = pAnimations + k; // zapamiętanie na potrzeby wyszukania submodeli
*/
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

                if(token == "lowpolyinterior:") {
					// ABu: wnetrze lowpoly
					parser.getTokens();
					parser >> asModel;
                    asModel = BaseDir + asModel; // McZapkie-200702 - dynamics maja swoje modele w dynamic/basedir
                    Global::asCurrentTexturePath = BaseDir; // biezaca sciezka do tekstur to dynamic/...
                    mdLowPolyInt = TModelsManager::GetModel(asModel, true);
                    // Global::asCurrentTexturePath=AnsiString(szTexturePath); //kiedyś uproszczone wnętrze mieszało tekstury nieba
                }

				if( token == "brakemode:" ) {
                // Ra 15-01: gałka nastawy hamulca
					parser.getTokens();
					parser >> asAnimName;
                    smBrakeMode = mdModel->GetFromName(asAnimName.c_str());
                    // jeszcze wczytać kąty obrotu dla poszczególnych ustawień
                }

				if( token == "loadmode:" ) {
                // Ra 15-01: gałka nastawy hamulca
					parser.getTokens();
					parser >> asAnimName;
                    smLoadMode = mdModel->GetFromName(asAnimName.c_str());
                    // jeszcze wczytać kąty obrotu dla poszczególnych ustawień
                }

                else if (token == "animwheelprefix:") {
					// prefiks kręcących się kół
                    int i, j, k, m;
					parser.getTokens( 1, false ); parser >> token;
                    for (i = 0; i < iAnimType[ANIM_WHEELS]; ++i) // liczba osi
                    { // McZapkie-050402: wyszukiwanie kol o nazwie str*
                        asAnimName = token + std::to_string(i + 1);
                        pAnimations[i].smAnimated = mdModel->GetFromName(asAnimName.c_str()); // ustalenie submodelu
                        if (pAnimations[i].smAnimated)
                        { //++iAnimatedAxles;
                            pAnimations[i].smAnimated->WillBeAnimated(); // wyłączenie optymalizacji transformu
/*                          pAnimations[i].yUpdate = UpdateAxle; // animacja osi
*/							pAnimations[ i ].yUpdate = std::bind( &TDynamicObject::UpdateAxle, this, std::placeholders::_1 );
                            pAnimations[i].fMaxDist = 50 * MoverParameters->WheelDiameter; // nie kręcić w większej odległości
                            pAnimations[i].fMaxDist *= pAnimations[i].fMaxDist * MoverParameters->WheelDiameter; // 50m do kwadratu, a średnica do trzeciej
                            pAnimations[i].fMaxDist *= Global::fDistanceFactor; // współczynnik przeliczeniowy jakości ekranu
                        }
                    }
                    // Ra: ustawianie indeksów osi
                    for (i = 0; i < iAnimType[ANIM_WHEELS]; ++i) // ilość osi (zabezpieczenie przed błędami w CHK)
                        pAnimations[i].dWheelAngle = dWheelAngle + 1; // domyślnie wskaźnik na napędzające
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
                                pAnimations[i++].dWheelAngle = dWheelAngle + 1; // obrót osi napędzających
                                --k; // następna będzie albo taka sama, albo bierzemy kolejny znak
                                m = 2; // następujące toczne będą miały inną średnicę
                            }
                            else if ((k >= '1') && (k <= '9'))
                            {
                                pAnimations[i++].dWheelAngle = dWheelAngle + m; // obrót osi tocznych
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
				if( token == "animpantrd1prefix:" ) {
                // prefiks ramion dolnych 1
					parser.getTokens(); parser >> token;
                    float4x4 m; // macierz do wyliczenia pozycji i wektora ruchu pantografu
                    TSubModel *sm;
                    if (pants)
                        for (int i = 0; i < iAnimType[ANIM_PANTS]; i++)
                        { // Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
                            asAnimName = token + std::to_string(i + 1);
                            sm = mdModel->GetFromName(asAnimName.c_str());
                            pants[i].smElement[0] = sm; // jak NULL, to nie będzie animowany
                            if (sm)
                            { // w EP09 wywalało się tu z powodu NULL
                                sm->WillBeAnimated();
                                sm->ParentMatrix(&m); // pobranie macierzy transformacji
                                // m(3)[1]=m[3][1]+0.054; //w górę o wysokość ślizgu (na razie tak)
                                if ((mdModel->Flags() & 0x8000) == 0) // jeśli wczytano z T3D
                                    m.InitialRotate(); // może być potrzebny dodatkowy obrót, jeśli wczytano z T3D, tzn. przed wykonaniem Init()
                                pants[i].fParamPants->vPos.z =
                                    m[3][0]; // przesunięcie w bok (asymetria)
                                pants[i].fParamPants->vPos.y =
                                    m[3][1]; // przesunięcie w górę odczytane z modelu
                                if ((sm = pants[i].smElement[0]->ChildGet()) != NULL)
                                { // jeśli ma potomny, można policzyć długość (odległość potomnego od osi obrotu)
                                    m = float4x4(*sm->GetMatrix()); // wystarczyłby wskaźnik, nie trzeba kopiować
                                    // może trzeba: pobrać macierz dolnego ramienia, wyzerować przesunięcie, przemnożyć przez macierz górnego
                                    pants[i].fParamPants->fHoriz = -fabs(m[3][1]);
                                    pants[i].fParamPants->fLenL1 =
                                        hypot(m[3][1], m[3][2]); // po osi OX nie potrzeba
                                    pants[i].fParamPants->fAngleL0 =
                                        atan2(fabs(m[3][2]), fabs(m[3][1]));
                                    // if (pants[i].fParamPants->fAngleL0<M_PI_2)
                                    // pants[i].fParamPants->fAngleL0+=M_PI; //gdyby w odwrotną stronę wyszło
                                    // if
                                    // ((pants[i].fParamPants->fAngleL0<0.03)||(pants[i].fParamPants->fAngleL0>0.09))
                                    // //normalnie ok. 0.05
                                    // pants[i].fParamPants->fAngleL0=pants[i].fParamPants->fAngleL;
                                    pants[i].fParamPants->fAngleL = pants[i].fParamPants->fAngleL0; // początkowy kąt dolnego
                                    // ramienia
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
                                        if ((mdModel->Flags() & 0x8000) == 0) // jeśli wczytano z T3D
                                            m.InitialRotate(); // może być potrzebny dodatkowy obrót, jeśli wczytano z T3D, tzn. przed wykonaniem Init()
                                        float det = Det(m);
                                        if (std::fabs(det - 1.0) < 0.001) // dopuszczamy 1 promil błędu na skalowaniu ślizgu
                                        { // skalowanie jest w normie, można pobrać wymiary z modelu
                                            pants[i].fParamPants->fHeight =
                                                sm->MaxY(m); // przeliczenie maksimum wysokości wierzchołków względem macierzy
                                            pants[i].fParamPants->fHeight -=
                                                m[3][1]; // odjęcie wysokości pivota ślizgu
                                            pants[i].fParamPants->vPos.x =
                                                m[3][2]; // przy okazji odczytać z modelu pozycję w długości
                                            // ErrorLog("Model OK: "+asModel+",
                                            // height="+pants[i].fParamPants->fHeight);
                                            // ErrorLog("Model OK: "+asModel+",
                                            // pos.x="+pants[i].fParamPants->vPos.x);
                                        }
                                        else
                                        { // gdy ktoś przesadził ze skalowaniem
                                            pants[i].fParamPants->fHeight =
                                                0.0; // niech będzie odczyt z pantfactors:
                                            ErrorLog("Bad model: " + asModel + ", scale of " +
                                                     (sm->pName) + " is " +
                                                     std::to_string(100.0 * det) + "%");
                                        }
                                    }
                                }
                            }
                            else
                                ErrorLog("Bad model: " + asFileName + " - missed submodel " +
                                         asAnimName); // brak ramienia
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
							sm = mdModel->GetFromName( asAnimName.c_str() );
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
								ErrorLog( "Bad model: " + asFileName + " - missed submodel " +
								asAnimName ); // brak ramienia
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
							pants[ i ].smElement[ 2 ] = mdModel->GetFromName( asAnimName.c_str() );
							pants[ i ].smElement[ 2 ]->WillBeAnimated();
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
							pants[ i ].smElement[ 3 ] = mdModel->GetFromName( asAnimName.c_str() );
							pants[ i ].smElement[ 3 ]->WillBeAnimated();
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
							pants[ i ].smElement[ 4 ] = mdModel->GetFromName( asAnimName.c_str() );
							pants[ i ].smElement[ 4 ]->WillBeAnimated();
/*							pants[ i ].yUpdate = UpdatePant;
*/							pants[ i ].yUpdate = std::bind( &TDynamicObject::UpdatePant, this, std::placeholders::_1 );
							pants[ i ].fMaxDist = 300 * 300; // nie podnosić w większej odległości
							pants[ i ].iNumber = i;
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
						for( int i = 0; i < iAnimType[ ANIM_PANTS ]; ++i ) { //  przepisanie współczynników do pantografów (na razie
                            // nie będzie lepiej)
							pants[ i ].fParamPants->fAngleL =
                                pants[i].fParamPants->fAngleL0; // początkowy kąt dolnego ramienia
							pants[ i ].fParamPants->fAngleU =
                                pants[i].fParamPants->fAngleU0; // początkowy kąt
                            // pants[i].fParamPants->PantWys=1.22*sin(pants[i].fParamPants->fAngleL)+1.755*sin(pants[i].fParamPants->fAngleU);
                            // //wysokość początkowa
                            // pants[i].fParamPants->PantWys=1.176289*sin(pants[i].fParamPants->fAngleL)+1.724482197*sin(pants[i].fParamPants->fAngleU);
                            // //wysokość początkowa
							if( pants[ i ].fParamPants->fHeight == 0.0 ) // gdy jest nieprawdopodobna wartość (np. nie znaleziony ślizg)
                            { // gdy pomiary modelu nie udały się, odczyt podanych parametrów z MMD
								pants[ i ].fParamPants->vPos.x = ( i & 1 ) ? pant2x : pant1x;
								pants[ i ].fParamPants->fHeight =
									( i & 1 ) ? pant2h :
                                              pant1h; // wysokość ślizgu jest zapisana w MMD
                            }
							pants[ i ].fParamPants->PantWys =
								pants[ i ].fParamPants->fLenL1 * sin( pants[ i ].fParamPants->fAngleL ) +
								pants[ i ].fParamPants->fLenU1 * sin( pants[ i ].fParamPants->fAngleU ) +
                                pants[i].fParamPants->fHeight; // wysokość początkowa
                            // pants[i].fParamPants->vPos.y=panty-panth-pants[i].fParamPants->PantWys;
                            // //np. 4.429-0.097=4.332=~4.335
                            // pants[i].fParamPants->vPos.z=0; //niezerowe dla pantografów
                            // asymetrycznych
							pants[ i ].fParamPants->PantTraction = pants[ i ].fParamPants->PantWys;
							pants[ i ].fParamPants->fWidth =
                                0.5 *
                                MoverParameters->EnginePowerSource.CollectorParameters
                                    .CSW; // połowa szerokości ślizgu; jest w "Power: CSW="
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
                        SteamPower; // Ra: po chamsku, ale z CHK nie działa
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
                    for (int i = 1; i <= 4; i++)
                    { // McZapkie-050402: wyszukiwanie max 4 wahaczy o nazwie str*
                        asAnimName = token + std::to_string(i);
                        smWahacze[i - 1] = mdModel->GetFromName(asAnimName.c_str());
                        smWahacze[i - 1]->WillBeAnimated();
                    }
					parser.getTokens(); parser >> token;
					if( token == "pendulumamplitude:" ) {
						parser.getTokens( 1, false );
						parser >> fWahaczeAmp;
					}
                }
                /*
				else if (str == AnsiString("engineer:"))
                { // nazwa submodelu maszynisty
                    str = Parser->GetNextSymbol();
                    smMechanik0 = mdModel->GetFromName(str.c_str());
                    if (!smMechanik0)
                    { // jak nie ma bez numerka, to może jest z
                        // numerkiem?
                        smMechanik0 = mdModel->GetFromName(AnsiString(str + "1").c_str());
                        smMechanik1 = mdModel->GetFromName(AnsiString(str + "2").c_str());
                    }
                    // aby dało się go obracać, musi mieć włączoną animację w T3D!
                    // if (!smMechanik1) //jeśli drugiego nie ma
                    // if (smMechanik0) //a jest pierwszy
                    //  smMechanik0->WillBeAnimated(); //to będziemy go obracać
                }
				*/

				else if( token == "animdoorprefix:" ) {
                 // nazwa animowanych drzwi
                    int i, j, k, m;
					parser.getTokens(1, false); parser >> token;
                    for (i = 0, j = 0; i < ANIM_DOORS; ++i)
                        j += iAnimType[i]; // zliczanie wcześniejszych animacji
                    for (i = 0; i < iAnimType[ANIM_DOORS]; ++i) // liczba drzwi
                    { // NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
                        asAnimName = token + std::to_string(i + 1);
                        pAnimations[i + j].smAnimated =
                            mdModel->GetFromName(asAnimName.c_str()); // ustalenie submodelu
                        if (pAnimations[i + j].smAnimated)
                        { //++iAnimatedDoors;
                            pAnimations[i + j].smAnimated->WillBeAnimated(); // wyłączenie optymalizacji transformu
                            switch (MoverParameters->DoorOpenMethod)
                            { // od razu zapinamy potrzebny typ animacji
                            case 1:
/*                              pAnimations[i + j].yUpdate = UpdateDoorTranslate;
*/								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorTranslate, this, std::placeholders::_1 );
                                break;
                            case 2:
/*                              pAnimations[i + j].yUpdate = UpdateDoorRotate;
*/								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorRotate, this, std::placeholders::_1 );
                                break;
                            case 3:
/*                              pAnimations[i + j].yUpdate = UpdateDoorFold;
*/								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorFold, this, std::placeholders::_1 );
                                break; // obrót 3 kolejnych submodeli
							case 4:
/*								pAnimations[i + j].yUpdate = UpdateDoorPlug;
*/								pAnimations[ i + j ].yUpdate = std::bind( &TDynamicObject::UpdateDoorPlug, this, std::placeholders::_1 );
								break;
							default:
								break;
							}
                            pAnimations[i + j].iNumber = i; // parzyste działają inaczej niż nieparzyste
                            pAnimations[i + j].fMaxDist = 300 * 300; // drzwi to z daleka widać
                            pAnimations[i + j].fSpeed = Random(150); // oryginalny koncept z DoorSpeedFactor
                            pAnimations[i + j].fSpeed = (pAnimations[i + j].fSpeed + 100) / 100;
                            // Ra: te współczynniki są bez sensu, bo modyfikują wektor przesunięcia
                        }
                    }
                }

			} while( ( token != "" )
	              && ( token != "endmodels" ) );

            }

		else if( token == "sounds:" ) {
			// dzwieki
			do {
				token = "";
				parser.getTokens(); parser >> token;
				if( token == "wheel_clatter:" ){
					// polozenia osi w/m srodka pojazdu
					parser.getTokens( 1, false );
					parser >> dSDist;
					for( int i = 0; i < iAxles; i++ ) {
						parser.getTokens( 1, false );
						parser >> dWheelsPosition[ i ];
						parser.getTokens();
						parser >> token;
						if( token != "end" ) {
							rsStukot[ i ].Init( token, dSDist, GetPosition().x,
								GetPosition().y + dWheelsPosition[ i ], GetPosition().z,
								true );
        }
                    }
					if( token != "end" ) {
						// TODO: double-check if this if() and/or retrieval makes sense here
						parser.getTokens( 1, false ); parser >> token;
                }
                }

				else if( ( token == "engine:" )
					  && ( MoverParameters->Power > 0 ) ) {
					// plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsSilnik.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true, true );
					if( rsSilnik.GetWaveTime() == 0 ) {
						ErrorLog( "Missed sound: \"" + token + "\" for " + asFileName );
                }
					parser.getTokens( 1, false );
					parser >> rsSilnik.AM;
					if( MoverParameters->EngineType == DieselEngine ) {

						rsSilnik.AM /= ( MoverParameters->Power + MoverParameters->nmax * 60 );
					}
					else if( MoverParameters->EngineType == DieselElectric ) {

						rsSilnik.AM /= ( MoverParameters->Power * 3 );
					}
					else {

						rsSilnik.AM /= ( MoverParameters->Power + MoverParameters->nmax * 60 + MoverParameters->Power + MoverParameters->Power );
					}
					parser.getTokens( 3, false );
					parser
						>> rsSilnik.AA
						>> rsSilnik.FM // MoverParameters->nmax;
						>> rsSilnik.FA;
				}

				else if( ( token == "ventilator:" )
					  && ( ( MoverParameters->EngineType == ElectricSeriesMotor )
					    || ( MoverParameters->EngineType == ElectricInductionMotor ) ) ) {
					// plik z dzwiekiem wentylatora, mnozniki i ofsety amp. i czest.
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsWentylator.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true, true );
					parser.getTokens( 4, false );
					parser
						>> rsWentylator.AM
						>> rsWentylator.AA
						>> rsWentylator.FM
						>> rsWentylator.FA;
					rsWentylator.AM /= MoverParameters->RVentnmax;
					rsWentylator.FM /= MoverParameters->RVentnmax;
				}

				else if( ( token == "transmission:" )
					  && ( MoverParameters->EngineType == ElectricSeriesMotor ) ) {
					// plik z dzwiekiem, mnozniki i ofsety amp. i czest.
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsPrzekladnia.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rsPrzekladnia.AM = 0.029;
                    rsPrzekladnia.AA = 0.1;
                    rsPrzekladnia.FM = 0.005;
                    rsPrzekladnia.FA = 1.0;
                }

				else if( token == "brake:"  ){
					// plik z piskiem hamulca,  mnozniki i ofsety amplitudy.
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsPisk.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
					rsPisk.AM = parser.getToken<double>();
					rsPisk.AA = parser.getToken<double>() * ( 105 - Random( 10 ) ) / 100;
                    rsPisk.FM = 1.0;
                    rsPisk.FA = 0.0;
                }

				else if( token == "brakeacc:" ) {
					// plik z przyspieszaczem (upust po zlapaniu hamowania)
                    //         sBrakeAcc.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
					parser.getTokens( 1, false ); parser >> token;
					sBrakeAcc = TSoundsManager::GetFromName( token.c_str(), true );
                    bBrakeAcc = true;
                    //         sBrakeAcc.AM=1.0;
                    //         sBrakeAcc.AA=0.0;
                    //         sBrakeAcc.FM=1.0;
                    //         sBrakeAcc.FA=0.0;
                }

				else if( token == "unbrake:" ) {
					// plik z piskiem hamulca, mnozniki i ofsety amplitudy.
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsUnbrake.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rsUnbrake.AM = 1.0;
                    rsUnbrake.AA = 0.0;
                    rsUnbrake.FM = 1.0;
                    rsUnbrake.FA = 0.0;
                }

				else if( token == "derail:"  ) {
					// dzwiek przy wykolejeniu
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsDerailment.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rsDerailment.AM = 1.0;
                    rsDerailment.AA = 0.0;
                    rsDerailment.FM = 1.0;
                    rsDerailment.FA = 0.0;
                }

				else if( token == "dieselinc:" ) {
					// dzwiek przy wlazeniu na obroty woodwarda
					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rsDiesielInc.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rsDiesielInc.AM = 1.0;
                    rsDiesielInc.AA = 0.0;
                    rsDiesielInc.FM = 1.0;
                    rsDiesielInc.FA = 0.0;
                }

				else if( token == "curve:" ) {

					double attenuation;
					parser.getTokens( 2, false );
					parser
						>> token
						>> attenuation;
					rscurve.Init(
						token, attenuation,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rscurve.AM = 1.0;
                    rscurve.AA = 0.0;
                    rscurve.FM = 1.0;
                    rscurve.FA = 0.0;
                }

				else if( token == "horn1:" ) {
					// pliki z trabieniem
					sHorn1.Load( parser, GetPosition() );
                }

				else if( token == "horn2:" ) {
					// pliki z trabieniem wysokoton.
					sHorn2.Load( parser, GetPosition() );
					if( iHornWarning ) {
                        iHornWarning = 2; // numer syreny do użycia po otrzymaniu sygnału do jazdy
                }
				}

				else if( token == "departuresignal:" ) {
					// pliki z sygnalem odjazdu
					sDepartureSignal.Load( parser, GetPosition() );
                }

				else if( token == "pantographup:" ) {
					// pliki dzwiekow pantografow
					parser.getTokens( 1, false ); parser >> token;
					sPantUp.Init(
						token, 50,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    sPantUp.AM = 50000;
					sPantUp.AA = -1 * ( 105 - Random( 10 ) ) / 100;
                    sPantUp.FM = 1.0;
                    sPantUp.FA = 0.0;
                }

				else if( token == "pantographdown:" ) {
					// pliki dzwiekow pantografow
					parser.getTokens( 1, false ); parser >> token;
					sPantDown.Init(
						token, 50,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    sPantDown.AM = 50000;
					sPantDown.AA = -1 * ( 105 - Random( 10 ) ) / 100;
                    sPantDown.FM = 1.0;
                    sPantDown.FA = 0.0;
                }

				else if( token == "compressor:" ) {
					// pliki ze sprezarka
					sCompressor.Load( parser, GetPosition() );
                }

				else if( token == "converter:" ) {
					// pliki z przetwornica
					// if (MoverParameters->EngineType==DieselElectric) //będzie modulowany?
					sConverter.Load( parser, GetPosition() );
                }

				else if( token == "turbo:" ) {
					// pliki z turbogeneratorem
					sTurbo.Load( parser, GetPosition() );
                }

				else if( token == "small-compressor:" ) {
					// pliki z przetwornica
					sSmallCompressor.Load( parser, GetPosition() );
                }

				else if( token == "dooropen:" ) {

					parser.getTokens( 1, false ); parser >> token;
					rsDoorOpen.Init(
						token, 50,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rsDoorOpen.AM = 50000;
					rsDoorOpen.AA = -1 * ( 105 - Random( 10 ) ) / 100;
                    rsDoorOpen.FM = 1.0;
                    rsDoorOpen.FA = 0.0;
                }

				else if( token == "doorclose:" ) {

					parser.getTokens( 1, false ); parser >> token;
					rsDoorClose.Init(
						token, 50,
						GetPosition().x, GetPosition().y, GetPosition().z,
						true );
                    rsDoorClose.AM = 50000;
					rsDoorClose.AA = -1 * ( 105 - Random( 10 ) ) / 100;
                    rsDoorClose.FM = 1.0;
                    rsDoorClose.FA = 0.0;
                }

				else if( token == "sand:" ) {
					// pliki z piasecznica
					sSand.Load( parser, GetPosition() );
                }

				else if( token == "releaser:" ) {
					// pliki z odluzniaczem
					sReleaser.Load( parser, GetPosition() );
                }

			} while( ( token != "" )
				  && ( token != "endsounds" ) );

			}

        else if (token == "internaldata:") {
			// dalej nie czytaj
			do {
				// zbieranie informacji o kabinach
				token = "";
				parser.getTokens(); parser >> token;
                if(token == "cab0model:")
                {
					parser.getTokens(); parser >> token;
					if( token != "none" ) { iCabs = 2; }
                }
                else if (token == "cab1model:")
                {
					parser.getTokens(); parser >> token;
					if( token != "none" ) { iCabs = 1; }
                }
                else if (token == "cab2model:")
                {
					parser.getTokens(); parser >> token;
					if( token != "none" ) { iCabs = 4; }
            }

			} while( token != "" );

            Stop_InternalData = true;
        }

	} while( ( token != "" ) 
	      && ( false == Stop_InternalData ) );

    if (mdModel)
        mdModel->Init(); // obrócenie modelu oraz optymalizacja, również zapisanie
    // binarnego
    if (mdLoad)
        mdLoad->Init();
    if (mdPrzedsionek)
        mdPrzedsionek->Init();
    if (mdLowPolyInt)
        mdLowPolyInt->Init();
    // sHorn2.CopyIfEmpty(sHorn1); ///żeby jednak trąbił też drugim
    Global::asCurrentTexturePath = szTexturePath; // kiedyś uproszczone wnętrze mieszało tekstury nieba
}

//---------------------------------------------------------------------------
void TDynamicObject::RadioStop()
{ // zatrzymanie pojazdu
    if (Mechanik) // o ile ktoś go prowadzi
        if (MoverParameters->SecuritySystem.RadioStop &&
            MoverParameters->Radio) // jeśli pojazd ma RadioStop i jest on aktywny
            Mechanik->PutCommand("Emergency_brake", 1.0, 1.0, &vPosition, stopRadio);
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
    if (rear == 2 + 32 + 64)
    { // jeśli koniec pociągu, to trzeba ustalić, czy
        // jest tam czynna lokomotywa
        // EN57 może nie mieć końcówek od środka członu
        if (MoverParameters->Power > 1.0) // jeśli ma moc napędową
            if (!MoverParameters->ActiveDir) // jeśli nie ma ustawionego kierunku
            { // jeśli ma zarówno światła jak i końcówki, ustalić, czy jest w stanie
                // aktywnym
                // np. lokomotywa na zimno będzie mieć końcówki a nie światła
                rear = 64; // tablice blaszane
                // trzeba to uzależnić od "załączenia baterii" w pojeździe
            }
        if (rear == 2 + 32 + 64) // jeśli nadal obydwie możliwości
            if (iInventory &
                (iDirection ? 0x2A : 0x15)) // czy ma jakieś światła czerowone od danej strony
                rear = 2 + 32; // dwa światła czerwone
            else
                rear = 64; // tablice blaszane
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
    CouplCounter = 20; //żeby normalnie skanować kolizje, to musi ruszyć z miejsca
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
        vector3 p1, p2;
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
    if (d->MoverParameters->TrainType & dt_EZT) // na razie dotyczy to EZT
        if (d->NextConnected ? d->MoverParameters->Couplers[1].AllowedFlag & ctrain_depot : false)
        { // gdy jest człon od sprzęgu 1, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if ((d->MoverParameters->Power < 1.0) && (d->NextConnected->MoverParameters->Power >
                                                      1.0)) // my nie mamy mocy, ale ten drugi ma
                d = d->NextConnected; // będziemy sterować tym z mocą
        }
        else if (d->PrevConnected ? d->MoverParameters->Couplers[0].AllowedFlag & ctrain_depot :
                                    false)
        { // gdy jest człon od sprzęgu 0, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if ((d->MoverParameters->Power < 1.0) && (d->PrevConnected->MoverParameters->Power >
                                                      1.0)) // my nie mamy mocy, ale ten drugi ma
                d = d->PrevConnected; // będziemy sterować tym z mocą
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

std::string TDynamicObject::TextureTest(std::string const &name)
{ // Ra 2015-01: sprawdzenie dostępności tekstury o podanej nazwie
	std::vector<std::string> extensions = { ".dds", ".tga", ".bmp" };
	for( auto const &extension : extensions ) {
		if( true == FileExists( name + extension ) ) {
			return name + extension;
        }
    }
    return ""; // nie znaleziona
};

void TDynamicObject::DestinationSet(std::string to, std::string numer)
{ // ustawienie stacji
    // docelowej oraz wymiennej
    // tekstury 4, jeśli
    // istnieje plik
    // w zasadzie, to każdy wagon mógłby mieć inną stację docelową
    // zwłaszcza w towarowych, pod kątem zautomatyzowania maewrów albo pracy górki
    // ale to jeszcze potrwa, zanim będzie możliwe, na razie można wpisać stację z
    // rozkładu
    if (abs(iMultiTex) >= 4)
        return; // jak są 4 tekstury wymienne, to nie zmieniać rozkładem
	numer = Global::Bezogonkow(numer);
    asDestination = to;
    to = Global::Bezogonkow(to); // do szukania pliku obcinamy ogonki
    std::string x = TextureTest(asBaseDir + numer + "@" + MoverParameters->TypeName);
	if (!x.empty())
    {
        ReplacableSkinID[4] = TTexturesManager::GetTextureID( NULL, NULL, x, 9); // rozmywania 0,1,4,5 nie nadają się
        return;
    }
	x = TextureTest(asBaseDir + numer );
	if (!x.empty())
    {
        ReplacableSkinID[4] = TTexturesManager::GetTextureID( NULL, NULL, x, 9); // rozmywania 0,1,4,5 nie nadają się
        return;
    }
    if (to.empty())
        to = "nowhere";
    x = TextureTest(asBaseDir + to + "@" + MoverParameters->TypeName); // w pierwszej kolejności z nazwą FIZ/MMD
    if (!x.empty())
    {
        ReplacableSkinID[4] = TTexturesManager::GetTextureID( NULL, NULL, x, 9); // rozmywania 0,1,4,5 nie nadają się
        return;
    }
    x = TextureTest(asBaseDir + to); // na razie prymitywnie
    if (!x.empty())
        ReplacableSkinID[4] = TTexturesManager::GetTextureID( NULL, NULL, x, 9); // rozmywania 0,1,4,5 nie nadają się
    else
		{
        x = TextureTest(asBaseDir + "nowhere"); // jak nie znalazł dedykowanej, to niech daje nowhere
		if (!x.empty())
			ReplacableSkinID[4] = TTexturesManager::GetTextureID(NULL, NULL, x, 9);
		}
    // Ra 2015-01: żeby zalogować błąd, trzeba by mieć pewność, że model używa
    // tekstury nr 4
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
