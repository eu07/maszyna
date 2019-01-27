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

#include "stdafx.h"
#include "AnimModel.h"

#include "renderer.h"
#include "MdlMngr.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Event.h"
#include "Globals.h"
#include "Timer.h"
#include "Logs.h"
#include "renderer.h"

TAnimContainer *TAnimModel::acAnimList = NULL;

TAnimAdvanced::TAnimAdvanced(){};

TAnimAdvanced::~TAnimAdvanced(){
    // delete[] pVocaloidMotionData; //plik został zmodyfikowany
};

int TAnimAdvanced::SortByBone()
{ // sortowanie pliku animacji w celu optymalniejszego wykonania
    // rekordy zostają ułożone wg kolejnych ramek dla każdej kości
    // ułożenie kości alfabetycznie nie jest niezbędne, ale upraszcza sortowanie bąbelkowe
    TAnimVocaloidFrame buf; // bufor roboczy (przydało by się pascalowe Swap()
    int i, j, k, swaps = 0, last = iMovements - 1, e;
    for (i = 0; i < iMovements; ++i)
        for (j = 0; j < 15; ++j)
            if (pMovementData[i].cBone[j] == '\0') // jeśli znacznik końca
                for (++j; j < 15; ++j)
                    pMovementData[i].cBone[j] = '\0'; // zerowanie bajtów za znacznikiem końca
    for (i = 0; i < last; ++i) // do przedostatniego
    { // dopóki nie posortowane
        j = i; // z którym porównywać
        k = i; // z którym zamienić (nie trzeba zamieniać, jeśli ==i)
        while (++j < iMovements)
        {
            e = strcmp(pMovementData[k].cBone,
                       pMovementData[j].cBone); // numery trzeba porównywać inaczej
            if (e > 0)
                k = j; // trzeba zamienić - ten pod j jest mniejszy
            else if (!e)
                if (pMovementData[k].iFrame > pMovementData[j].iFrame)
                    k = j; // numer klatki pod j jest mniejszy
        }
        if (k > i)
        { // jeśli trzeba przestawić
            // buf=pMovementData[i];
            // pMovementData[i]=pMovementData[k];
            // pMovementData[k]=buf;
            memcpy(&buf, pMovementData + i, sizeof(TAnimVocaloidFrame));
            memcpy(pMovementData + i, pMovementData + k, sizeof(TAnimVocaloidFrame));
            memcpy(pMovementData + k, &buf, sizeof(TAnimVocaloidFrame));
            ++swaps;
        }
    }
    return swaps;
};

TAnimContainer::TAnimContainer()
{
    pNext = NULL;
    vRotateAngles = Math3D::vector3(0.0f, 0.0f, 0.0f); // aktualne kąty obrotu
    vDesiredAngles = Math3D::vector3(0.0f, 0.0f, 0.0f); // docelowe kąty obrotu
    fRotateSpeed = 0.0;
    vTranslation = Math3D::vector3(0.0f, 0.0f, 0.0f); // aktualne przesunięcie
    vTranslateTo = Math3D::vector3(0.0f, 0.0f, 0.0f); // docelowe przesunięcie
    fTranslateSpeed = 0.0;
    fAngleSpeed = 0.0;
    pSubModel = NULL;
    iAnim = 0; // położenie początkowe
    pMovementData = NULL; // nie ma zaawansowanej animacji
    mAnim = NULL; // nie ma macierzy obrotu dla submodelu
    evDone = NULL; // powiadamianie o zakończeniu animacji
    acAnimNext = NULL; // na razie jest poza listą
}

TAnimContainer::~TAnimContainer()
{
    SafeDelete(pNext);
    delete mAnim; // AnimContainer jest właścicielem takich macierzy
}

bool TAnimContainer::Init(TSubModel *pNewSubModel)
{
    fRotateSpeed = 0.0f;
    pSubModel = pNewSubModel;
    return (pSubModel != NULL);
}

void TAnimContainer::SetRotateAnim( Math3D::vector3 vNewRotateAngles, double fNewRotateSpeed)
{
    if (!this)
        return; // wywoływane z eventu, gdy brak modelu
    vDesiredAngles = vNewRotateAngles;
    fRotateSpeed = fNewRotateSpeed;
    iAnim |= 1;
    /* //Ra 2014-07: jeśli model nie jest renderowany, to obliczyć czas animacji i dodać event
     wewnętrzny
     //można by też ustawić czas początku animacji zamiast pobierać czas ramki i liczyć różnicę
    */
    if (evDone)
    { // dołączyć model do listy aniomowania, żeby animacje były przeliczane również bez
        // wyświetlania
        if (iAnim >= 0)
        { // jeśli nie jest jeszcze na liście animacyjnej
            acAnimNext = TAnimModel::acAnimList; // pozostałe doklić sobie jako ogon
            TAnimModel::acAnimList = this; // a wstawić się na początek
            iAnim |= 0x80000000; // dodany do listy
        }
    }
}

void TAnimContainer::SetTranslateAnim( Math3D::vector3 vNewTranslate, double fNewSpeed)
{
    if (!this)
        return; // wywoływane z eventu, gdy brak modelu
    vTranslateTo = vNewTranslate;
    fTranslateSpeed = fNewSpeed;
    iAnim |= 2;
    /* //Ra 2014-07: jeśli model nie jest renderowany, to obliczyć czas animacji i dodać event
     wewnętrzny
     //można by też ustawić czas początku animacji zamiast pobierać czas ramki i liczyć różnicę
    */
    if (evDone)
    { // dołączyć model do listy aniomowania, żeby animacje były przeliczane również bez
        // wyświetlania
        if (iAnim >= 0)
        { // jeśli nie jest jeszcze na liście animacyjnej
            acAnimNext = TAnimModel::acAnimList; // pozostałe doklić sobie jako ogon
            TAnimModel::acAnimList = this; // a wstawić się na początek
            iAnim |= 0x80000000; // dodany do listy
        }
    }
}

void TAnimContainer::AnimSetVMD(double fNewSpeed)
{
    if (!this)
        return; // wywoływane z eventu, gdy brak modelu
    // skala do ustalenia, "cal" japoński (sun) to nieco ponad 3cm
    // X-w lewo, Y-w górę, Z-do tyłu
    // minimalna wysokość to -7.66, a nadal musi być ponad podłogą
    // if (pMovementData->iFrame>0) return; //tylko pierwsza ramka
    vTranslateTo =
        Math3D::vector3(
            0.1 * pMovementData->f3Vector.x,
            0.1 * pMovementData->f3Vector.z,
            0.1 * pMovementData->f3Vector.y);
    if (LengthSquared3(vTranslateTo) > 0.0 ? true : LengthSquared3(vTranslation) > 0.0)
    { // jeśli ma być przesunięte albo jest przesunięcie
        iAnim |= 2; // wyłączy się samo
        if (fNewSpeed > 0.0)
            fTranslateSpeed = fNewSpeed; // prędkość jest mnożnikiem, nie podlega skalowaniu
        else // za późno na animacje, trzeba przestawić
            vTranslation = vTranslateTo;
    }
    // if ((qCurrent.w<1.0)||(pMovementData->qAngle.w<1.0))
    { // jeśli jest jakiś obrót
        if (!mAnim)
        {
            mAnim = new float4x4(); // będzie potrzebna macierz animacji
            mAnim->Identity(); // jedynkowanie na początek
        }
        iAnim |= 4; // animacja kwaternionowa
        qStart = qCurrent; // potrzebna początkowa do interpolacji
        //---+ - też niby dobrze, ale nie tak trąca włosy na początku (macha w dół)
        //-+-+ - dłoń ma w górze zamiast na pasie w pozycji początkowej
        //+--+ - głowa do tyłu (broda w górę) w pozycji początkowej
        //--++ - pozycja początkowa dobra, trąca u góry, ale z rękami jakoś nie tak, kółko w
        // przeciwną stronę
        //++++ - kładzie się brzuchem do góry
        //-+++ - ręce w górze na początku, zamiast w dół, łokieć jakby w przeciwną stronę
        //+-++ - nie podnosi ręki do głowy
        //++-+ - dłoń ma w górze zamiast na pasie
        qDesired = Normalize(float4(-pMovementData->qAngle.x, -pMovementData->qAngle.z,
                                    -pMovementData->qAngle.y,
                                    pMovementData->qAngle.w)); // tu trzeba będzie osie zamienić
        if (fNewSpeed > 0.0)
        {
            fAngleSpeed = fNewSpeed; // wtedy animować za pomocą interpolacji
            fAngleCurrent = 0.0; // początek interpolacji
        }
        else
        { // za późno na animację, można tylko przestawić w docelowe miejsce
            fAngleSpeed = 0.0;
            fAngleCurrent = 1.0; // interpolacja zakończona
            qCurrent = qDesired;
        }
    }
    // if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak główna kość
    // if (!strcmp(pSubModel->pName,"Ť¶‚Â‚Ü?ć‚h‚j")) //IK lewej stopy
    // WriteLog(AnsiString(pMovementData->iFrame)+": "+AnsiString(pMovementData->f3Vector.x)+"
    // "+AnsiString(pMovementData->f3Vector.y)+" "+AnsiString(pMovementData->f3Vector.z));
}

// przeliczanie animacji wykonać tylko raz na model
void TAnimContainer::UpdateModel() {

    if (pSubModel) // pozbyć się tego - sprawdzać wcześniej
    {
        if (fTranslateSpeed != 0.0)
        {
            auto dif = vTranslateTo - vTranslation; // wektor w kierunku docelowym
            double l = LengthSquared3(dif); // długość wektora potrzebnego przemieszczenia
            if (l >= 0.0001)
            { // jeśli do przemieszczenia jest ponad 1cm
                auto s = Math3D::SafeNormalize(dif); // jednostkowy wektor kierunku
                s = s *
                    (fTranslateSpeed *
                     Timer::GetDeltaTime()); // przemieszczenie w podanym czasie z daną prędkością
                if (LengthSquared3(s) < l) //żeby nie jechało na drugą stronę
                    vTranslation += s;
                else
                    vTranslation = vTranslateTo; // koniec animacji, "koniec animowania" uruchomi
                // się w następnej klatce
            }
            else
            { // koniec animowania
                vTranslation = vTranslateTo;
                fTranslateSpeed = 0.0; // wyłączenie przeliczania wektora
                if (LengthSquared3(vTranslation) <= 0.0001) // jeśli jest w punkcie początkowym
                    iAnim &= ~2; // wyłączyć zmianę pozycji submodelu
                if( evDone ) {
                    // wykonanie eventu informującego o zakończeniu
                    simulation::Events.AddToQuery( evDone, nullptr );
                }
            }
        }
        if (fRotateSpeed != 0.0)
        {
            bool anim = false;
            auto dif = vDesiredAngles - vRotateAngles;
            double s;
            s = std::abs( fRotateSpeed ) * sign(dif.x) * Timer::GetDeltaTime();
            if (fabs(s) >= fabs(dif.x))
                vRotateAngles.x = vDesiredAngles.x;
            else
            {
                vRotateAngles.x += s;
                anim = true;
            }
            s = std::abs( fRotateSpeed ) * sign(dif.y) * Timer::GetDeltaTime();
            if (fabs(s) >= fabs(dif.y))
                vRotateAngles.y = vDesiredAngles.y;
            else
            {
                vRotateAngles.y += s;
                anim = true;
            }
            s = std::abs( fRotateSpeed ) * sign(dif.z) * Timer::GetDeltaTime();
            if (fabs(s) >= fabs(dif.z))
                vRotateAngles.z = vDesiredAngles.z;
            else
            {
                vRotateAngles.z += s;
                anim = true;
            }
            // HACK: negative speed allows to work around legacy behaviour, where desired angle > 360 meant permanent rotation
            if( fRotateSpeed > 0.0 ) {
                while( vRotateAngles.x >= 360 )
                    vRotateAngles.x -= 360;
                while( vRotateAngles.x <= -360 )
                    vRotateAngles.x += 360;
                while( vRotateAngles.y >= 360 )
                    vRotateAngles.y -= 360;
                while( vRotateAngles.y <= -360 )
                    vRotateAngles.y += 360;
                while( vRotateAngles.z >= 360 )
                    vRotateAngles.z -= 360;
                while( vRotateAngles.z <= -360 )
                    vRotateAngles.z += 360;
            }

            if( ( vRotateAngles.x == 0.0 )
             && ( vRotateAngles.y == 0.0 )
             && ( vRotateAngles.z == 0.0 ) ) {
                iAnim &= ~1; // kąty są zerowe
            }
            if (!anim)
            { // nie potrzeba przeliczać już
                fRotateSpeed = 0.0;
                if( evDone ) {
                    // wykonanie eventu informującego o zakończeniu
                    simulation::Events.AddToQuery( evDone, nullptr );
                }
            }
        }
        if( fAngleSpeed != 0.f ) {
            // NOTE: this is angle- not quaternion-based rotation TBD, TODO: switch to quaternion rotations?
            fAngleCurrent += fAngleSpeed * Timer::GetDeltaTime(); // aktualny parametr interpolacji
        }
    }
};

void TAnimContainer::PrepareModel()
{ // tutaj zostawić tylko ustawienie submodelu, przeliczanie ma być w UpdateModel()
    if (pSubModel) // pozbyć się tego - sprawdzać wcześniej
    {
        // nanoszenie animacji na wzorzec
        if (iAnim & 1) // zmieniona pozycja względem początkowej
            pSubModel->SetRotateXYZ(vRotateAngles); // ustawia typ animacji
        if (iAnim & 2) // zmieniona pozycja względem początkowej
            pSubModel->SetTranslate(vTranslation);
        if (iAnim & 4) // zmieniona pozycja względem początkowej
        {
            if (fAngleSpeed > 0.0f)
            {
                if (fAngleCurrent >= 1.0f)
                { // interpolacja zakończona, ustawienie na pozycję końcową
                    qCurrent = qDesired;
                    fAngleSpeed = 0.0; // wyłączenie przeliczania wektora
                    if( evDone ) {
                        // wykonanie eventu informującego o zakończeniu
                        simulation::Events.AddToQuery( evDone, nullptr );
                    }
                }
                else
                { // obliczanie pozycji pośredniej
                    // normalizacja jest wymagana do interpolacji w następnej animacji
                    qCurrent = Normalize(
                        Slerp(qStart, qDesired, fAngleCurrent)); // interpolacja sferyczna kąta
                    // qCurrent=Slerp(qStart,qDesired,fAngleCurrent); //interpolacja sferyczna kąta
                    if (qCurrent.w ==
                        1.0) // rozpoznać brak obrotu i wyłączyć w iAnim w takim przypadku
                        iAnim &= ~4; // kąty są zerowe
                }
            }
            mAnim->Quaternion(&qCurrent); // wypełnienie macierzy (wymaga normalizacji?)
            pSubModel->mAnimMatrix = mAnim; // użyczenie do submodelu (na czas renderowania!)
        }
    }
    // if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak główna kość
    // WriteLog(AnsiString(pMovementData->iFrame)+": "+AnsiString(iAnim)+"
    // "+AnsiString(vTranslation.x)+" "+AnsiString(vTranslation.y)+" "+AnsiString(vTranslation.z));
}

void TAnimContainer::UpdateModelIK()
{ // odwrotna kinematyka wyliczana dopiero po ustawieniu macierzy w submodelach
    if (pSubModel) // pozbyć się tego - sprawdzać wcześniej
    {
        if ((pSubModel->b_Anim == TAnimType::at_IK)
          ||(pSubModel->b_Anim == TAnimType::at_IK22))
        { // odwrotna kinematyka
            float3 d, k;
            TSubModel *ch = pSubModel->ChildGet();
            switch (pSubModel->b_Anim)
            {
            case TAnimType::at_IK11: // stopa: ustawić w kierunku czubka (pierwszy potomny)
                d = ch->Translation1Get(); // wektor względem aktualnego układu (nie uwzględnia
                // obrotu)
                k = float3(RadToDeg(atan2(d.z, hypot(d.x, d.y))), 0.0,
                           -RadToDeg(atan2(d.y, d.x))); // proste skierowanie na punkt
                pSubModel->SetRotateIK1(k);
                // if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak główna kość
                // WriteLog("--> "+AnsiString(k.x)+" "+AnsiString(k.y)+" "+AnsiString(k.z));
                // Ra: to już jest dobrze, może być inna ćwiartka i znak
                break;
            case TAnimType::at_IK22: // udo: ustawić w kierunku pierwszej potomnej pierwszej potomnej (kostki)
                // pozycję kostki należy określić względem kości centralnej (+biodro może być
                // pochylone)
                // potem wyliczyć ewentualne odchylenie w tej i następnej
                // w sumie to proste, jak wyznaczenie kątów w trójkącie o znanej długości boków...
                d = ch->Translation2Get(); // wektor względem aktualnego układu (nie uwzględnia
                // obrotu)
                // if ()
                { // kość IK jest dalej niż pozycja spoczynkowa
                    k = float3(RadToDeg(atan2(d.z, hypot(d.x, d.y))), 0.0,
                               -RadToDeg(atan2(d.y, d.x))); // proste skierowanie na punkt
                    pSubModel->SetRotateIK1(k);
                }
                break;
            }
        }
    }
}

bool TAnimContainer::InMovement()
{ // czy trwa animacja - informacja dla obrotnicy
    return (fRotateSpeed != 0.0) || (fTranslateSpeed != 0.0);
}

void TAnimContainer::EventAssign(basic_event *ev)
{ // przypisanie eventu wykonywanego po zakończeniu animacji
    evDone = ev;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

TAnimModel::TAnimModel( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {

    m_lightcolors.fill( glm::vec3{ -1.f } );
    m_lightopacities.fill( 1.f );
}

TAnimModel::~TAnimModel()
{
    SafeDelete(pAdvanced); // nie ma zaawansowanej animacji
    SafeDelete(pRoot);
}

bool TAnimModel::Init(std::string const &asName, std::string const &asReplacableTexture)
{
    if( asReplacableTexture.substr( 0, 1 ) == "*" ) {
        // od gwiazdki zaczynają się teksty na wyświetlaczach
        asText = asReplacableTexture.substr( 1, asReplacableTexture.length() - 1 ); // zapamiętanie tekstu
    }
    else if( asReplacableTexture != "none" ) {
        m_materialdata.replacable_skins[ 1 ] = GfxRenderer.Fetch_Material( asReplacableTexture );
    }
    if( ( m_materialdata.replacable_skins[ 1 ] != null_handle )
     && ( GfxRenderer.Material( m_materialdata.replacable_skins[ 1 ] ).get_or_guess_opacity() == 0.0f ) ) {
        // tekstura z kanałem alfa - nie renderować w cyklu nieprzezroczystych
        m_materialdata.textures_alpha = 0x31310031;
    }
    else{
        // tekstura nieprzezroczysta - nie renderować w cyklu przezroczystych
        m_materialdata.textures_alpha = 0x30300030;
    }

// TODO: redo the random timer initialization
//    fBlinkTimer = Random() * ( fOnTime + fOffTime );

    pModel = TModelsManager::GetModel( asName );
    return ( pModel != nullptr );
}

bool TAnimModel::Load(cParser *parser, bool ter)
{ // rozpoznanie wpisu modelu i ustawienie świateł
	std::string name = parser->getToken<std::string>();
    std::string texture = parser->getToken<std::string>( false ); // tekstura (zmienia na małe)
    replace_slashes( name );
    replace_slashes( texture );
    if (!Init( name, texture ))
    {
        if (name != "notload")
        { // gdy brak modelu
            if (ter) // jeśli teren
            {
				if( name.substr( name.rfind( '.' ) ) == ".t3d" ) {
					name[ name.length() - 3 ] = 'e';
				}
#ifdef EU07_USE_OLD_TERRAINCODE
                Global.asTerrainModel = name;
                WriteLog("Terrain model \"" + name + "\" will be created.");
#endif
            }
            else
                ErrorLog("Missed file: " + name);
        }
    }
    else
    { // wiązanie świateł, o ile model wczytany
        LightsOn[0] = pModel->GetFromName("Light_On00");
        LightsOn[1] = pModel->GetFromName("Light_On01");
        LightsOn[2] = pModel->GetFromName("Light_On02");
        LightsOn[3] = pModel->GetFromName("Light_On03");
        LightsOn[4] = pModel->GetFromName("Light_On04");
        LightsOn[5] = pModel->GetFromName("Light_On05");
        LightsOn[6] = pModel->GetFromName("Light_On06");
        LightsOn[7] = pModel->GetFromName("Light_On07");
        LightsOff[0] = pModel->GetFromName("Light_Off00");
        LightsOff[1] = pModel->GetFromName("Light_Off01");
        LightsOff[2] = pModel->GetFromName("Light_Off02");
        LightsOff[3] = pModel->GetFromName("Light_Off03");
        LightsOff[4] = pModel->GetFromName("Light_Off04");
        LightsOff[5] = pModel->GetFromName("Light_Off05");
        LightsOff[6] = pModel->GetFromName("Light_Off06");
        LightsOff[7] = pModel->GetFromName("Light_Off07");
    }
    for (int i = 0; i < iMaxNumLights; ++i)
        if (LightsOn[i] || LightsOff[i]) // Ra: zlikwidowałem wymóg istnienia obu
            iNumLights = i + 1;

    std::string token;
    do {
        token = parser->getToken<std::string>();

        if( token == "lights" ) {
            auto i{ 0 };
            while( ( false == ( token = parser->getToken<std::string>() ).empty() )
                && ( token != "lightcolors" )
                && ( token != "endmodel" ) ) {

                if( i < iNumLights ) {
                    // stan światła jest liczbą z ułamkiem
                    LightSet( i, std::stof( token ) );
                }
                ++i;
            }
        }

        if( token == "lightcolors" ) {
            auto i{ 0 };
            while( ( false == ( token = parser->getToken<std::string>() ).empty() )
                && ( token != "lights" )
                && ( token != "endmodel" ) ) {

                if( ( i < iNumLights )
                 && ( token != "-1" ) ) { // -1 leaves the default color intact
                    auto const lightcolor { std::stoi( token, 0, 16 ) };
                    m_lightcolors[i] = {
                        ( ( lightcolor >> 16 ) & 0xff ) / 255.f,
                        ( ( lightcolor >> 8 )  & 0xff ) / 255.f,
                        ( ( lightcolor )       & 0xff ) / 255.f };
                }
                ++i;
            }
        }
    } while( ( false == token.empty() )
          && ( token != "endmodel" ) );

    return true;
}

TAnimContainer * TAnimModel::AddContainer(std::string const &Name)
{ // dodanie sterowania submodelem dla egzemplarza
    if (!pModel)
        return NULL;
    TSubModel *tsb = pModel->GetFromName(Name);
    if (tsb)
    {
        TAnimContainer *tmp = new TAnimContainer();
        tmp->Init(tsb);
        tmp->pNext = pRoot;
        pRoot = tmp;
        return tmp;
    }
    return NULL;
}

TAnimContainer * TAnimModel::GetContainer(std::string const &Name)
{ // szukanie/dodanie sterowania submodelem dla egzemplarza
    if (true == Name.empty())
        return pRoot; // pobranie pierwszego (dla obrotnicy)
    TAnimContainer *pCurrent;
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
        // if (pCurrent->GetName()==pName)
		if (Name == pCurrent->NameGet())
            return pCurrent;
    return AddContainer(Name);
}

// przeliczenie animacji - jednorazowo na klatkę
void TAnimModel::RaAnimate( unsigned int const Framestamp ) {
    
    if( Framestamp == m_framestamp ) { return; }

    auto const timedelta { Timer::GetDeltaTime() };

    // interpretacja ułamka zależnie od typu
    // case ls_Off: ustalenie czasu migotania, t<1s (f>1Hz), np. 0.1 => t=0.1 (f=10Hz)
    // case ls_On: ustalenie wypełnienia ułamkiem, np. 1.25 => zapalony przez 1/4 okresu
    // case ls_Blink: ustalenie częstotliwości migotania, f<1Hz (t>1s), np. 2.2 => f=0.2Hz (t=5s)
    float modeintegral, modefractional;
    for( int idx = 0; idx < iNumLights; ++idx ) {

        modefractional = std::modf( std::abs( lsLights[ idx ] ), &modeintegral );

        if( modeintegral >= ls_Dark ) {
            // light threshold modes don't use timers
            continue;
        }
        auto const mode { static_cast<int>( modeintegral ) };
            
        auto &opacity { m_lightopacities[ idx ] };
        auto &timer { m_lighttimers[ idx ] };
        if( ( modeintegral < ls_Blink ) && ( modefractional < 0.01f ) ) {
            // simple flip modes
            switch( mode ) {
                case ls_Off: {
                    // reduce to zero
                    timer = std::max<float>( 0.f, timer - timedelta );
                    break;
                }
                case ls_On: {
                    // increase to max value
                    timer = std::min<float>( fTransitionTime, timer + timedelta );
                    break;
                }
                default: {
                    break;
                }
            }
            opacity = timer / fTransitionTime;
        }
        else {
            // blink modes
            auto const ontime { (
                ( mode == ls_Blink ) ? ( ( modefractional < 0.01f ) ? fOnTime : ( 1.f / modefractional ) * 0.5f ) :
                ( mode == ls_Off ) ? modefractional * 0.5f :
                ( mode == ls_On ) ? modefractional * ( fOnTime + fOffTime ) :
                fOnTime ) }; // fallback
            auto const offtime { (
                ( mode == ls_Blink ) ? ( ( modefractional < 0.01f ) ? fOffTime : ontime ) :
                ( mode == ls_Off ) ? ontime :
                ( mode == ls_On ) ? ( fOnTime + fOffTime ) - ontime :
                fOffTime ) }; // fallback
            auto const transitiontime {
                std::min(
                    1.f,
                    std::min( ontime, offtime ) * 0.9f ) };

            timer = clamp_circular<float>( timer + timedelta * ( lsLights[ idx ] > 0.f ? 1.f : -1.f ), ontime + offtime );
            // set opacity depending on blink stage
            if( timer < ontime ) {
                // blink on
                opacity = clamp( timer / transitiontime, 0.f, 1.f );
            }
            else {
                // blink off
                opacity = 1.f - clamp( ( timer - ontime ) / transitiontime, 0.f, 1.f );
            }
        }
    }

    // Ra 2F1I: to by można pomijać dla modeli bez animacji, których jest większość
    TAnimContainer *pCurrent;
    for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext)
        if (!pCurrent->evDone) // jeśli jest bez eventu
            pCurrent->UpdateModel(); // przeliczenie animacji każdego submodelu
    // if () //tylko dla modeli z IK !!!!
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext) // albo osobny łańcuch
        pCurrent->UpdateModelIK(); // przeliczenie odwrotnej kinematyki

    m_framestamp = Framestamp;
};

void TAnimModel::RaPrepare()
{ // ustawia światła i animacje we wzorcu modelu przed renderowaniem egzemplarza
    bool state; // stan światła
    for (int i = 0; i < iNumLights; ++i)
    {
        auto const lightmode { static_cast<int>( std::abs( lsLights[ i ] ) ) };
        switch( lightmode ) {
            case ls_On:
            case ls_Off:
            case ls_Blink: {
                if (LightsOn[i]) {
                    LightsOn[i]->iVisible = ( m_lightopacities[i] > 0.f );
                    LightsOn[i]->SetVisibilityLevel( m_lightopacities[i], true, false );
                }
                if (LightsOff[i]) {
                    LightsOff[i]->iVisible = ( m_lightopacities[i] < 1.f );
                    LightsOff[i]->SetVisibilityLevel( 1.f, true, false );
                }
                break;
            }
            case ls_Dark: {
                // zapalone, gdy ciemno
                state = (
                    Global.fLuminance - std::max( 0.f, Global.Overcast - 1.f ) <= (
                        lsLights[ i ] == static_cast<float>( ls_Dark ) ?
                            DefaultDarkThresholdLevel :
                            ( lsLights[ i ] - static_cast<float>( ls_Dark ) ) ) );
                break;
            }
            case ls_Home: {
                // like ls_dark but off late at night
                auto const simulationhour { simulation::Time.data().wHour };
                state = (
                    Global.fLuminance - std::max( 0.f, Global.Overcast - 1.f ) <= (
                        lsLights[ i ] == static_cast<float>( ls_Home ) ?
                            DefaultDarkThresholdLevel :
                            ( lsLights[ i ] - static_cast<float>( ls_Home ) ) ) );
                // force the lights off between 1-5am
                state = state && (( simulationhour < 1 ) || ( simulationhour >= 5 ));
                break;
            }
            default: {
                break;
            }
        }
        if( lightmode >= ls_Dark ) {
            // crude as hell but for test will do :x
            if (LightsOn[i]) {
                LightsOn[i]->iVisible = state;
                // TODO: set visibility for the entire submodel's children as well
                LightsOn[i]->fVisible = m_lightopacities[i];
            }
            if (LightsOff[i])
                LightsOff[i]->iVisible = !state;
        }
        // potentially modify freespot colors
        if( LightsOn[i] ) {
            LightsOn[i]->SetDiffuseOverride( m_lightcolors[i], true);
        }
    }
    TSubModel::iInstance = reinterpret_cast<std::uintptr_t>( this ); //żeby nie robić cudzych animacji
    TSubModel::pasText = &asText; // przekazanie tekstu do wyświetlacza (!!!! do przemyślenia)
    if (pAdvanced) // jeśli jest zaawansowana animacja
        Advanced(); // wykonać co tam trzeba
    TAnimContainer *pCurrent;
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
        pCurrent->PrepareModel(); // ustawienie animacji egzemplarza dla każdego submodelu
    // if () //tylko dla modeli z IK !!!!
    // for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext) //albo osobny łańcuch
    //  pCurrent->UpdateModelIK(); //przeliczenie odwrotnej kinematyki
}

int TAnimModel::Flags()
{ // informacja dla TGround, czy ma być w Render, RenderAlpha, czy RenderMixed
    int i = pModel ? pModel->Flags() : 0; // pobranie flag całego modelu
    if( m_materialdata.replacable_skins[ 1 ] > 0 ) // jeśli ma wymienną teksturę 0
        i |= (i & 0x01010001) * ((m_materialdata.textures_alpha & 1) ? 0x20 : 0x10);
    return i;
};

//---------------------------------------------------------------------------

int TAnimModel::TerrainCount()
{ // zliczanie kwadratów kilometrowych (główna linia po Next) do tworznia tablicy
    return pModel ? pModel->TerrainCount() : 0;
};
TSubModel * TAnimModel::TerrainSquare(int n)
{ // pobieranie wskaźników do pierwszego submodelu
    return pModel ? pModel->TerrainSquare(n) : 0;
};

//---------------------------------------------------------------------------

void TAnimModel::Advanced()
{ // wykonanie zaawansowanych animacji na submodelach
    pAdvanced->fCurrent +=
        pAdvanced->fFrequency * Timer::GetDeltaTime(); // aktualna ramka zmiennoprzecinkowo
    int frame = floor(pAdvanced->fCurrent); // numer klatki jako int
    TAnimContainer *pCurrent;
    if (pAdvanced->fCurrent >= pAdvanced->fLast)
    { // animacja została zakończona
        delete pAdvanced;
        pAdvanced = NULL; // dalej już nic
        for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
            if (pCurrent->pMovementData) // jeśli obsługiwany tabelką animacji
                pCurrent->pMovementData = NULL; // usuwanie wskaźników
    }
    else
    { // coś trzeba poanimować - wszystkie animowane submodele są w tym łańcuchu
        for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
            if (pCurrent->pMovementData) // jeśli obsługiwany tabelką animacji
                if (frame >= pCurrent->pMovementData->iFrame) // koniec czekania
                    if (!strcmp(pCurrent->pMovementData->cBone,
                                (pCurrent->pMovementData + 1)->cBone))
                    { // jak kolejna ramka dotyczy tego samego submodelu, ustawić animację do
                        // kolejnej ramki
                        ++pCurrent->pMovementData; // kolejna klatka
                        pCurrent->AnimSetVMD(
                            pAdvanced->fFrequency /
                            (double(pCurrent->pMovementData->iFrame) - pAdvanced->fCurrent));
                    }
                    else
                        pCurrent->pMovementData =
                            NULL; // inna nazwa, animowanie zakończone w aktualnym położeniu
    }
};

void TAnimModel::AnimationVND(void *pData, double a, double b, double c, double d)
{ // rozpoczęcie wykonywania animacji z podanego pliku
    // tabela w pliku musi być posortowana wg klatek dla kolejnych kości!
    // skrócone nagranie ma 3:42 = 222 sekundy, animacja kończy się na klatce 6518
    // daje to 29.36 (~=30) klatek na sekundę
    // w opisach jest podawane 24 albo 36 jako standard => powiedzmy, parametr (d) to FPS animacji
    delete pAdvanced; // usunięcie ewentualnego poprzedniego
    pAdvanced = NULL; // gdyby się nie udało rozpoznać pliku
    if (std::string(static_cast<char *>(pData)) == "Vocaloid Motion Data 0002")
    {
        pAdvanced = new TAnimAdvanced();
        pAdvanced->pVocaloidMotionData = static_cast<unsigned char *>(pData); // podczepienie pliku danych
        pAdvanced->iMovements = *((int *)(((char *)pData) + 50)); // numer ostatniej klatki
        pAdvanced->pMovementData = (TAnimVocaloidFrame *)(((char *)pData) + 54); // rekordy animacji
        // WriteLog(sizeof(TAnimVocaloidFrame));
        pAdvanced->fFrequency = d;
        pAdvanced->fCurrent = 0.0; // aktualna ramka
        pAdvanced->fLast = 0.0; // ostatnia ramka
        /*
          if (0) //jeśli włączone sortowanie plików VMD (trochę się przeciąga)
           if (pAdvanced->SortByBone()) //próba posortowania
           {//zapisać posortowany plik, jeśli dokonano zmian
            TFileStream *fs=new TFileStream("models/1.vmd",fmCreate);
            fs->Write(pData,2198342); //2948728);
            delete fs;
           }
        */

/*      int i, j, k, idx;
*/      std::string name;
        TAnimContainer *pSub;
        for (int i = 0; i < pAdvanced->iMovements; ++i)
        {
            if (strcmp(pAdvanced->pMovementData[i].cBone, name.c_str()))
            { // jeśli pozycja w tabelce nie była wyszukiwana w submodelach
                pSub = GetContainer(pAdvanced->pMovementData[i].cBone); // szukanie
                if (pSub) // znaleziony
                {
                    pSub->pMovementData = pAdvanced->pMovementData + i; // gotów do animowania
                    pSub->AnimSetVMD(0.0); // usuawienie pozycji początkowej (powinna być zerowa,
                    // inaczej będzie skok)
                }
                name = std::string(pAdvanced->pMovementData[i].cBone); // nowa nazwa do pomijania
            }
            if (pAdvanced->fLast < pAdvanced->pMovementData[i].iFrame)
                pAdvanced->fLast = pAdvanced->pMovementData[i].iFrame;
        }
        /*
          for (i=0;i<pAdvanced->iMovements;++i)
          if
          (AnsiString(pAdvanced->pMovementData[i+1].cBone)!=AnsiString(pAdvanced->pMovementData[i].cBone))
          {//generowane dla ostatniej klatki danej kości
           name="";
           for (j=0;j<15;j++)
            name+=IntToHex((unsigned char)pAdvanced->pMovementData[i].cBone[j],2);
           WriteLog(name+","
            +AnsiString(pAdvanced->pMovementData[i].cBone)+","
            +AnsiString(idx)+"," //indeks
            +AnsiString(i+1-idx)+"," //ile pozycji animacji
            +AnsiString(k)+"," //pierwsza klatka
            +AnsiString(pAdvanced->pMovementData[i].iFrame)+"," //ostatnia klatka
            +AnsiString(pAdvanced->pMovementData[i].f3Vector.x)+","
            +AnsiString(pAdvanced->pMovementData[i].f3Vector.y)+","
            +AnsiString(pAdvanced->pMovementData[i].f3Vector.z)+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[0])+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[1])+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[2])+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[3])

            );
           idx=i+1;
           k=pAdvanced->pMovementData[i+1].iFrame; //pierwsza klatka następnego
          }
          else
           if (pAdvanced->pMovementData[i].iFrame>0)
            if ((k>pAdvanced->pMovementData[i].iFrame)||(k==0))
             k=pAdvanced->pMovementData[i].iFrame; //pierwsza niezerowa ramka
        */
        /*
          for (i=0;i<pAdvanced->iMovements;++i)
           if (AnsiString(pAdvanced->pMovementData[i].cBone)=="\x89\x45\x90\x65\x8E\x77\x82\x4F")
           {name="";
            for (j=0;j<15;j++)
             name+=IntToHex((unsigned char)pAdvanced->pMovementData[i].cBone[j],2);
            WriteLog(name+","
             +AnsiString(i)+"," //pozycja w tabeli
             +AnsiString(pAdvanced->pMovementData[i].iFrame)+"," //pierwsza klatka
            );
           }
        */
    }
};

//---------------------------------------------------------------------------
void TAnimModel::LightSet(int const n, float const v)
{ // ustawienie światła (n) na wartość (v)
    if( n >= iMaxNumLights ) {
        return; // przekroczony zakres
    }
    lsLights[ n ] = v;
};

void TAnimModel::AnimUpdate(double dt)
{ // wykonanie zakolejkowanych animacji, nawet gdy modele nie są aktualnie wyświetlane
    TAnimContainer *p = TAnimModel::acAnimList;
    while( p ) {

        p->UpdateModel();
        p = p->acAnimNext; // na razie bez usuwania z listy, bo głównie obrotnica na nią wchodzi
    }
};

// radius() subclass details, calculates node's bounding radius
float
TAnimModel::radius_() {

    return (
        pModel ?
            pModel->bounding_radius() :
            0.f );
}

// serialize() subclass details, sends content of the subclass to provided stream
void
TAnimModel::serialize_( std::ostream &Output ) const {
    
    // TODO: implement
}
// deserialize() subclass details, restores content of the subclass from provided stream
void
TAnimModel::deserialize_( std::istream &Input ) {

    // TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void
TAnimModel::export_as_text_( std::ostream &Output ) const {
    // header
    Output << "model ";
    // location and rotation
    Output
        << location().x << ' '
        << location().y << ' '
        << location().z << ' '
        << vAngle.y << ' ';
    // 3d shape
    auto modelfile { (
        pModel ?
            pModel->NameGet() + ".t3d" : // rainsted requires model file names to include an extension
            "none" ) };
    if( modelfile.find( szModelPath ) == 0 ) {
        // don't include 'models/' in the path
        modelfile.erase( 0, std::string{ szModelPath }.size() );
    }
    Output << modelfile << ' ';
    // texture
    auto texturefile { (
        m_materialdata.replacable_skins[ 1 ] != null_handle ?
            GfxRenderer.Material( m_materialdata.replacable_skins[ 1 ] ).name :
            "none" ) };
    if( texturefile.find( szTexturePath ) == 0 ) {
        // don't include 'textures/' in the path
        texturefile.erase( 0, std::string{ szTexturePath }.size() );
    }
    Output << texturefile << ' ';
    // light submodels activation configuration
    if( iNumLights > 0 ) {
        Output << "lights ";
        for( int lightidx = 0; lightidx < iNumLights; ++lightidx ) {
            Output << lsLights[ lightidx ] << ' ';
        }
    }
    // footer
    Output
        << "endmodel"
        << "\n";
}

//---------------------------------------------------------------------------
