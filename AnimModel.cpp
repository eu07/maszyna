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

std::list<std::weak_ptr<TAnimContainer>> TAnimModel::acAnimList;

TAnimContainer::TAnimContainer()
{
    vRotateAngles = Math3D::vector3(0.0f, 0.0f, 0.0f); // aktualne kąty obrotu
    vDesiredAngles = Math3D::vector3(0.0f, 0.0f, 0.0f); // docelowe kąty obrotu
    fRotateSpeed = 0.0;
    vTranslation = Math3D::vector3(0.0f, 0.0f, 0.0f); // aktualne przesunięcie
    vTranslateTo = Math3D::vector3(0.0f, 0.0f, 0.0f); // docelowe przesunięcie
    fTranslateSpeed = 0.0;
    fAngleSpeed = 0.0;
    pSubModel = NULL;
	iAnim = 0; // położenie początkowe
	evDone = NULL; // powiadamianie o zakończeniu animacji
}

bool TAnimContainer::Init(TSubModel *pNewSubModel)
{
    fRotateSpeed = 0.0f;
    pSubModel = pNewSubModel;
    return (pSubModel != NULL);
}

void TAnimContainer::SetRotateAnim( Math3D::vector3 vNewRotateAngles, double fNewRotateSpeed)
{
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
			TAnimModel::acAnimList.push_back(shared_from_this());
			iAnim |= 0x80000000; // dodany do listy
        }
    }
}

void TAnimContainer::SetTranslateAnim( Math3D::vector3 vNewTranslate, double fNewSpeed)
{
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
			TAnimModel::acAnimList.push_back(shared_from_this());
            iAnim |= 0x80000000; // dodany do listy
        }
    }
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

bool TAnimModel::Init(std::string const &asName, std::string const &asReplacableTexture)
{
    if( asReplacableTexture.substr( 0, 1 ) == "*" ) {
        // od gwiazdki zaczynają się teksty na wyświetlaczach
        asText = asReplacableTexture.substr( 1, asReplacableTexture.length() - 1 ); // zapamiętanie tekstu
    }
    else if( asReplacableTexture != "none" ) {
        m_materialdata.assign( asReplacableTexture );
    }

// TODO: redo the random timer initialization
//    fBlinkTimer = Random() * ( fOnTime + fOffTime );

    pModel = TModelsManager::GetModel( asName );
    return ( pModel != nullptr );
}

bool
TAnimModel::is_keyword( std::string const &Token ) const {

    return ( Token == "endmodel" )
        || ( Token == "lights" )
        || ( Token == "lightcolors" )
        || ( Token == "angles" )
        || ( Token == "notransition" );
}

bool TAnimModel::Load(cParser *parser, bool ter)
{ // rozpoznanie wpisu modelu i ustawienie świateł
	std::string name = parser->getToken<std::string>();
	std::string texture = parser->getToken<std::string>(false);
    replace_slashes( name );
    replace_slashes( texture );
    if (!Init( name, texture ))
    {
        if (name != "notload")
        { // gdy brak modelu
            if (ter) // jeśli teren
            {
				if( ends_with( name, ".t3d" ) ) {
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
                && ( false == is_keyword( token ) ) ) {

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
                && ( false == is_keyword( token ) ) ) {

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

        if( token == "angles" ) {
            parser->getTokens( 3 );
            *parser
                >> vAngle[ 0 ]
                >> vAngle[ 1 ]
                >> vAngle[ 2 ];
        }

        if( token == "notransition" ) {
            m_transition = false;
        }

    } while( ( false == token.empty() )
          && ( token != "endmodel" ) );

    return true;
}

std::shared_ptr<TAnimContainer> TAnimModel::AddContainer(std::string const &Name)
{ // dodanie sterowania submodelem dla egzemplarza
    if (!pModel)
		return nullptr;
    TSubModel *tsb = pModel->GetFromName(Name);
    if (tsb)
    {
		auto tmp = std::make_shared<TAnimContainer>();
        tmp->Init(tsb);
		m_animlist.push_back(tmp);
		return tmp;
    }
	return nullptr;
}

std::shared_ptr<TAnimContainer> TAnimModel::GetContainer(std::string const &Name)
{ // szukanie/dodanie sterowania submodelem dla egzemplarza
	if (true == Name.empty())
		return (!m_animlist.empty()) ? m_animlist.front() : nullptr; // pobranie pierwszego (dla obrotnicy)

	for (auto entry : m_animlist) {
		if (entry->NameGet() == Name)
			return entry;
	}

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
            auto const transitiontime { (
                m_transition ?
                    std::min(
                        1.f,
                        std::min( fOnTime, fOffTime ) * 0.9f ) :
                    0.01f ) };

            switch( mode ) {
                case ls_Off: {
                    // reduce to zero
                    timer = std::max<float>( 0.f, timer - timedelta );
                    break;
                }
                case ls_On: {
                    // increase to max value
                    timer = std::min<float>( transitiontime, timer + timedelta );
                    break;
                }
                default: {
                    break;
                }
            }
            opacity = timer / transitiontime;
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
            auto const transitiontime { (
                m_transition ?
                    std::min(
                        1.f,
                        std::min( ontime, offtime ) * 0.9f ) :
                    0.01f ) };

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
	for (auto entry : m_animlist) {
		if (!entry->evDone) // jeśli jest bez eventu
			entry->UpdateModel(); // przeliczenie animacji każdego submodelu
	}

    m_framestamp = Framestamp;
}

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

	for (auto entry : m_animlist) {
		entry->PrepareModel();
	}
}

int TAnimModel::Flags()
{ // informacja dla TGround, czy ma być w Render, RenderAlpha, czy RenderMixed
    int i = pModel ? pModel->Flags() : 0; // pobranie flag całego modelu
    if( m_materialdata.replacable_skins[ 1 ] > 0 ) // jeśli ma wymienną teksturę 0
        i |= (i & 0x01010001) * ((m_materialdata.textures_alpha & 1) ? 0x20 : 0x10);
    return i;
}

//---------------------------------------------------------------------------

int TAnimModel::TerrainCount()
{ // zliczanie kwadratów kilometrowych (główna linia po Next) do tworznia tablicy
    return pModel ? pModel->TerrainCount() : 0;
}

TSubModel * TAnimModel::TerrainSquare(int n)
{ // pobieranie wskaźników do pierwszego submodelu
    return pModel ? pModel->TerrainSquare(n) : 0;
}

//---------------------------------------------------------------------------
void TAnimModel::LightSet(int const n, float const v)
{ // ustawienie światła (n) na wartość (v)
    if( n >= iMaxNumLights ) {
        return; // przekroczony zakres
    }
    lsLights[ n ] = v;
}

std::optional<std::tuple<float, float, std::optional<glm::vec3>> > TAnimModel::LightGet(const int n)
{
	if (n >= iMaxNumLights)
		return std::nullopt;
	if (!LightsOn[n] && !LightsOff[n])
		return std::nullopt;

	std::optional<glm::vec3> color;

	if (m_lightcolors[n].r >= 0.0f)
		color.emplace(m_lightcolors[n]);

	if (!color && LightsOn[n])
		color = LightsOn[n]->GetDiffuse();

	return std::make_tuple(lsLights[n], m_lightopacities[n], color);
}

void TAnimModel::SkinSet( int const Index, material_handle const Material ) {

    m_materialdata.replacable_skins[ clamp( Index, 1, 4 ) ] = Material;
}

void TAnimModel::AnimUpdate(double dt)
{ // wykonanie zakolejkowanych animacji, nawet gdy modele nie są aktualnie wyświetlane
	acAnimList.remove_if([](std::weak_ptr<TAnimContainer> ptr)
	{
		std::shared_ptr<TAnimContainer> container = ptr.lock();
		if (!container)
			return true;

		container->UpdateModel(); // na razie bez usuwania z listy, bo głównie obrotnica na nią wchodzi
		return false;
	});
}

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
            GfxRenderer->Material( m_materialdata.replacable_skins[ 1 ] ).name :
            "none" ) };
    if( texturefile.find( szTexturePath ) == 0 ) {
        // don't include 'textures/' in the path
        texturefile.erase( 0, std::string{ szTexturePath }.size() );
    }
    if( contains( texturefile, ' ' ) ) {
        Output << "\"" << texturefile << "\"" << ' ';
    }
    else {
        Output << texturefile << ' ';
    }
    // light submodels activation configuration
    if( iNumLights > 0 ) {
        Output << "lights ";
        for( int lightidx = 0; lightidx < iNumLights; ++lightidx ) {
            Output << lsLights[ lightidx ] << ' ';
        }
    }
    // potential light transition switch
    if( false == m_transition ) {
        Output << "notransition" << ' ';
    }
    // footer
    Output
        << "endmodel"
        << "\n";
}

//---------------------------------------------------------------------------
