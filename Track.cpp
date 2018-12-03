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

#include "Track.h"
#include "simulation.h"
#include "Globals.h"
#include "Event.h"
#include "messaging.h"
#include "DynObj.h"
#include "AnimModel.h"
#include "Track.h"
#include "Timer.h"
#include "Logs.h"
#include "renderer.h"
#include "utilities.h"

// 101206 Ra: trapezoidalne drogi i tory
// 110720 Ra: rozprucie zwrotnicy i odcinki izolowane

static float const fMaxOffset = 0.1f; // double(0.1f)==0.100000001490116
// const int NextMask[4]={0,1,0,1}; //tor następny dla stanów 0, 1, 2, 3
// const int PrevMask[4]={0,0,1,1}; //tor poprzedni dla stanów 0, 1, 2, 3
const int iLewo4[4] = {5, 3, 4, 6}; // segmenty (1..6) do skręcania w lewo
const int iPrawo4[4] = {-4, -6, -3, -5}; // segmenty (1..6) do skręcania w prawo
const int iProsto4[4] = {1, -1, 2, -2}; // segmenty (1..6) do jazdy prosto
const int iEnds4[13] = {3, 0, 2, 1, 2, 0, -1, 1, 3, 2, 0, 3, 1}; // numer sąsiedniego toru na końcu segmentu "-1"
const int iLewo3[4] = {1, 3, 2, 1}; // segmenty do skręcania w lewo
const int iPrawo3[4] = {-2, -1, -3, -2}; // segmenty do skręcania w prawo
const int iProsto3[4] = {1, -1, 2, 1}; // segmenty do jazdy prosto
const int iEnds3[13] = {3, 0, 2, 1, 2, 0, -1, 1, 0, 2, 0, 3, 1}; // numer sąsiedniego toru na końcu segmentu "-1"
TIsolated *TIsolated::pRoot = NULL;

TTrack::profiles_array TTrack::m_profiles;
TTrack::profiles_map TTrack::m_profilesmap;

TSwitchExtension::TSwitchExtension(TTrack *owner, int const what)
{ // na początku wszystko puste
    pNexts[0] = nullptr; // wskaźniki do kolejnych odcinków ruchu
    pNexts[1] = nullptr;
    pPrevs[0] = nullptr;
    pPrevs[1] = nullptr;
    fOffset1 = fOffset = fDesiredOffset = -fOffsetDelay; // położenie zasadnicze
    fOffset2 = 0.f; // w zasadniczym wewnętrzna iglica dolega
    Segments[0] = std::make_shared<TSegment>(owner); // z punktu 1 do 2
    Segments[1] = std::make_shared<TSegment>(owner); // z punktu 3 do 4 (1=3 dla zwrotnic; odwrócony dla skrzyżowań, ewentualnie 1=4)
    Segments[2] = (what >= 3) ?
                      std::make_shared<TSegment>(owner) :
                      nullptr; // z punktu 2 do 4       skrzyżowanie od góry:      wersja "-1":
    Segments[3] = (what >= 4) ?
                      std::make_shared<TSegment>(owner) :
					  nullptr; // z punktu 4 do 1              1       1=4 0 0=3
    Segments[4] = (what >= 5) ?
                      std::make_shared<TSegment>(owner) :
					  nullptr; // z punktu 1 do 3            4 x 3   3 3 x 2   2
    Segments[5] = (what >= 6) ?
                      std::make_shared<TSegment>(owner) :
                      nullptr; // z punktu 3 do 2              2       2            1       1
}
TSwitchExtension::~TSwitchExtension()
{ // nie ma nic do usuwania
}
/*
TIsolated::TIsolated()
{ // utworznie pustego
    TIsolated("none", NULL);
};
*/
TIsolated::TIsolated(std::string const &n, TIsolated *i) :
                                asName( n ),   pNext( i )
{
    // utworznie obwodu izolowanego. nothing to do here.
};

void TIsolated::DeleteAll() {

    while( pRoot ) {
        auto *next = pRoot->Next();
        delete pRoot;
        pRoot = next;
    }
}

TIsolated * TIsolated::Find(std::string const &n)
{ // znalezienie obiektu albo utworzenie nowego
    TIsolated *p = pRoot;
    while (p)
    { // jeśli się znajdzie, to podać wskaźnik
        if (p->asName == n)
            return p;
        p = p->pNext;
    }
    pRoot = new TIsolated(n, pRoot); // BUG: source of a memory leak
    return pRoot;
};

void
TIsolated::AssignEvents() {

    evBusy = simulation::Events.FindEvent( asName + ":busy" );
    evFree = simulation::Events.FindEvent( asName + ":free" );
    evInc = simulation::Events.FindEvent( asName + ":inc" );
    evDec = simulation::Events.FindEvent( asName + ":dec" );
}

void TIsolated::Modify(int i, TDynamicObject *o)
{ // dodanie lub odjęcie osi
    if (iAxles)
    { // grupa zajęta
        iAxles += i;
        if (!iAxles)
        { // jeśli po zmianie nie ma żadnej osi na odcinku izolowanym
            if (evFree)
                simulation::Events.AddToQuery(evFree, o); // dodanie zwolnienia do kolejki
            if (Global.iMultiplayer) // jeśli multiplayer
                multiplayer::WyslijString(asName, 10); // wysłanie pakietu o zwolnieniu
            if (pMemCell) // w powiązanej komórce
                pMemCell->UpdateValues( "", 0, int( pMemCell->Value2() ) & ~0xFF,
                basic_event::flags::value_2 ); //"zerujemy" ostatnią wartość
        }
    }
    else
    { // grupa była wolna
        iAxles += i;
        if (iAxles)
        {
            if (evBusy)
                simulation::Events.AddToQuery(evBusy, o); // dodanie zajętości do kolejki
            if (Global.iMultiplayer) // jeśli multiplayer
                multiplayer::WyslijString(asName, 11); // wysłanie pakietu o zajęciu
            if (pMemCell) // w powiązanej komórce
                pMemCell->UpdateValues( "", 0, int( pMemCell->Value2() ) | 1, basic_event::flags::value_2 ); // zmieniamy ostatnią wartość na nieparzystą
        }
    }

	if (i > 0 && evInc)
		simulation::Events.AddToQuery(evInc, o);
	if (i < 0 && evDec)
		simulation::Events.AddToQuery(evDec, o);

    // pass the event to the parent
    if( pParent != nullptr ) {
        pParent->Modify( i, o );
    }
};

// tworzenie nowego odcinka ruchu
TTrack::TTrack( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {}

TTrack::~TTrack()
{ // likwidacja odcinka
	if( eType == tt_Cross ) {
		delete SwitchExtension->vPoints; // skrzyżowanie może mieć punkty
	}
}

void TTrack::Init()
{ // tworzenie pomocniczych danych
    switch (eType)
    {
    case tt_Switch:
		SwitchExtension = std::make_shared<TSwitchExtension>( this, 2 ); // na wprost i na bok
        break;
    case tt_Cross: // tylko dla skrzyżowania dróg
		SwitchExtension = std::make_shared<TSwitchExtension>( this, 6 ); // 6 po³¹czeñ
        SwitchExtension->vPoints = nullptr; // brak tablicy punktów
        SwitchExtension->bPoints = false; // tablica punktów nie wypełniona
        SwitchExtension->iRoads = 4; // domyślnie 4
        break;
    case tt_Normal:
		Segment = std::make_shared<TSegment>( this );
        break;
    case tt_Table: // oba potrzebne
		SwitchExtension = std::make_shared<TSwitchExtension>( this, 1 ); // kopia oryginalnego toru
        Segment = std::make_shared<TSegment>(this);
        break;
    }
}

bool
TTrack::sort_by_material( TTrack const *Left, TTrack const *Right ) {

    return std::tie( Left->m_material1, Left->m_material2 ) < std::tie( Right->m_material1, Right->m_material2 );
}

TTrack * TTrack::Create400m(int what, double dx)
{ // tworzenie toru do wstawiania taboru podczas konwersji na E3D
    scene::node_data nodedata;
    nodedata.name = "auto_400m"; // track isn't visible so only name is needed
    auto *trk = new TTrack( nodedata );
    trk->m_visible = false; // nie potrzeba pokazywać, zresztą i tak nie ma tekstur
    trk->iCategoryFlag = what; // taki sam typ plus informacja, że dodatkowy
    trk->Init(); // utworzenie segmentu
    trk->Segment->Init( Math3D::vector3( -dx, 0, 0 ), Math3D::vector3( -dx, 0, 400 ), 10.0, 0, 0 ); // prosty
    trk->location( glm::dvec3{ -dx, 0, 200 } ); //środek, aby się mogło wyświetlić
    simulation::Paths.insert( trk );
    simulation::Region->insert( trk );
    return trk;
};

TTrack * TTrack::NullCreate(int dir)
{ // tworzenie toru wykolejającego od strony (dir), albo pętli dla samochodów
    TTrack
        *trk { nullptr },
        *trk2 { nullptr };
    scene::node_data nodedata;
    nodedata.name = "auto_null"; // track isn't visible so only name is needed
    trk = new TTrack( nodedata );
    trk->m_visible = false; // nie potrzeba pokazywać, zresztą i tak nie ma tekstur
    trk->iCategoryFlag = (iCategoryFlag & 15) | 0x80; // taki sam typ plus informacja, że dodatkowy
    float r1, r2;
    Segment->GetRolls(r1, r2); // pobranie przechyłek na początku toru
    Math3D::vector3 p1, cv1, cv2, p2; // będziem tworzyć trajektorię lotu
    if (iCategoryFlag & 1)
    { // tylko dla kolei
        trk->iDamageFlag = 128; // wykolejenie
        trk->fVelocity = 0.0; // koniec jazdy
        trk->Init(); // utworzenie segmentu
        switch (dir)
        { //łączenie z nowym torem
        case 0:
            p1 = Segment->FastGetPoint_0();
            p2 = p1 - 450.0 * Normalize(Segment->GetDirection1());
            // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk->Segment->Init(p1, p2, 5, -RadToDeg(r1), 70.0);
            ConnectPrevPrev(trk, 0);
            break;
        case 1:
            p1 = Segment->FastGetPoint_1();
            p2 = p1 - 450.0 * Normalize(Segment->GetDirection2());
            // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk->Segment->Init(p1, p2, 5, RadToDeg(r2), 70.0);
            ConnectNextPrev(trk, 0);
            break;
        case 3: // na razie nie możliwe
            p1 = SwitchExtension->Segments[1]->FastGetPoint_1(); // koniec toru drugiego zwrotnicy
            p2 = p1 - 450.0 * Normalize( SwitchExtension->Segments[1]->GetDirection2()); // przedłużenie na wprost
            trk->Segment->Init(p1, p2, 5, RadToDeg(r2), 70.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            ConnectNextPrev(trk, 0);
            // trk->ConnectPrevNext(trk,dir);
            SetConnections(1); // skopiowanie połączeń
            Switch(1); // bo się przełączy na 0, a to coś chce się przecież wykoleić na bok
            break; // do drugiego zwrotnicy... nie zadziała?
        }
    }
    else
    { // tworznie pętelki dla samochodów
        trk->fVelocity = 20.0; // zawracanie powoli
        trk->fRadius = 20.0; // promień, aby się dodawało do tabelki prędkości i liczyło narastająco
        trk->Init(); // utworzenie segmentu
        trk2 = new TTrack( nodedata );
        trk2->iCategoryFlag =
            (iCategoryFlag & 15) | 0x80; // taki sam typ plus informacja, że dodatkowy
        trk2->m_visible = false;
        trk2->fVelocity = 20.0; // zawracanie powoli
        trk2->fRadius = 20.0; // promień, aby się dodawało do tabelki prędkości i liczyło narastająco
        trk2->Init(); // utworzenie segmentu
        trk->m_name = m_name + ":loopstart";
        trk2->m_name = m_name + ":loopfinish";
        switch (dir)
        { //łączenie z nowym torem
        case 0:
            p1 = Segment->FastGetPoint_0();
            cv1 = -20.0 * Normalize(Segment->GetDirection1()); // pierwszy wektor kontrolny
            p2 = p1 + cv1 + cv1; // 40m
            // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk->Segment->Init(p1, p1 + cv1, p2 + Math3D::vector3(-cv1.z, cv1.y, cv1.x), p2, 2, -RadToDeg(r1), 0.0);
            ConnectPrevPrev(trk, 0);
            // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk2->Segment->Init(p1, p1 + cv1, p2 + Math3D::vector3(cv1.z, cv1.y, -cv1.x), p2, 2, -RadToDeg(r1), 0.0);
            trk2->iPrevDirection = 0; // zwrotnie do tego samego odcinka
            break;
        case 1:
            p1 = Segment->FastGetPoint_1();
            cv1 = -20.0 * Normalize(Segment->GetDirection2()); // pierwszy wektor kontrolny
            p2 = p1 + cv1 + cv1;
            // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk->Segment->Init(p1, p1 + cv1, p2 + Math3D::vector3(-cv1.z, cv1.y, cv1.x), p2, 2, RadToDeg(r2), 0.0);
            ConnectNextPrev(trk, 0);
            // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk2->Segment->Init(p1, p1 + cv1, p2 + Math3D::vector3(cv1.z, cv1.y, -cv1.x), p2, 2, RadToDeg(r2), 0.0);
            trk2->iPrevDirection = 1; // zwrotnie do tego samego odcinka
            break;
        }
        trk2->trPrev = this;
        trk->ConnectNextNext(trk2, 1); // połączenie dwóch dodatkowych odcinków punktami 2
    }
    // trzeba jeszcze dodać do odpowiedniego segmentu, aby się renderowały z niego pojazdy
    trk->location( glm::dvec3{ 0.5 * ( p1 + p2 ) } ); //środek, aby się mogło wyświetlić
    simulation::Paths.insert( trk );
    simulation::Region->insert( trk );
    if( trk2 ) {
        trk2->location( trk->location() ); // ten sam środek jest
        simulation::Paths.insert( trk2 );
        simulation::Region->insert( trk2 );
    }
    return trk;
};

void TTrack::ConnectPrevPrev(TTrack *pTrack, int typ)
{ //łączenie torów - Point1 własny do Point1 cudzego
    if (pTrack)
    { //(pTrack) może być zwrotnicą, a (this) tylko zwykłym odcinkiem
        trPrev = pTrack;
        iPrevDirection = ((pTrack->eType == tt_Switch) ? 0 : (typ & 2));
        pTrack->trPrev = this;
        pTrack->iPrevDirection = 0;
    }
}
void TTrack::ConnectPrevNext(TTrack *pTrack, int typ)
{ //łaczenie torów - Point1 własny do Point2 cudzego
    if (pTrack)
    {
        trPrev = pTrack;
        iPrevDirection = typ | 1; // 1:zwykły lub pierwszy zwrotnicy, 3:drugi zwrotnicy
        pTrack->trNext = this;
        pTrack->iNextDirection = 0;
        if (m_visible)
            if (pTrack->m_visible)
                if (eType == tt_Normal) // jeśli łączone są dwa normalne
                    if (pTrack->eType == tt_Normal)
                        if ((fTrackWidth !=
                             pTrack->fTrackWidth) // Ra: jeśli kolejny ma inne wymiary
                            ||
                            (fTexHeight1 != pTrack->fTexHeight1) ||
                            (fTexWidth != pTrack->fTexWidth) || (fTexSlope != pTrack->fTexSlope))
                            pTrack->iTrapezoid |= 2; // to rysujemy potworka
    }
}
void TTrack::ConnectNextPrev(TTrack *pTrack, int typ)
{ //łaczenie torów - Point2 własny do Point1 cudzego
    if (pTrack)
    {
        trNext = pTrack;
        iNextDirection = ((pTrack->eType == tt_Switch) ? 0 : (typ & 2));
        pTrack->trPrev = this;
        pTrack->iPrevDirection = 1;
        if (m_visible)
            if (pTrack->m_visible)
                if (eType == tt_Normal) // jeśli łączone są dwa normalne
                    if (pTrack->eType == tt_Normal)
                        if ((fTrackWidth !=
                             pTrack->fTrackWidth) // Ra: jeśli kolejny ma inne wymiary
                            ||
                            (fTexHeight1 != pTrack->fTexHeight1) ||
                            (fTexWidth != pTrack->fTexWidth) || (fTexSlope != pTrack->fTexSlope))
                            iTrapezoid |= 2; // to rysujemy potworka
    }
}
void TTrack::ConnectNextNext(TTrack *pTrack, int typ)
{ //łaczenie torów - Point2 własny do Point2 cudzego
    if (pTrack)
    {
        trNext = pTrack;
        iNextDirection = typ | 1; // 1:zwykły lub pierwszy zwrotnicy, 3:drugi zwrotnicy
        pTrack->trNext = this;
        pTrack->iNextDirection = 1;
    }
}

void TTrack::Load(cParser *parser, glm::dvec3 const &pOrigin)
{ // pobranie obiektu trajektorii ruchu
    Math3D::vector3 pt, vec, p1, p2, cp1, cp2, p3, p4, cp3, cp4; // dodatkowe punkty potrzebne do skrzyżowań
	double a1, a2, r1, r2, r3, r4;
    std::string str;
    size_t i; //,state; //Ra: teraz już nie ma początkowego stanu zwrotnicy we wpisie
    std::string token;

    parser->getTokens();
    *parser >> str; // typ toru

    if (str == "normal")
    {
        eType = tt_Normal;
        iCategoryFlag = 1;
    }
    else if (str == "switch")
    {
        eType = tt_Switch;
        iCategoryFlag = 1;
    }
    else if (str == "turn")
    { // Ra: to jest obrotnica
        eType = tt_Table;
        iCategoryFlag = 1;
    }
    else if (str == "table")
    { // Ra: obrotnica, przesuwnica albo wywrotnica
        eType = tt_Table;
        iCategoryFlag = 1;
    }
    else if (str == "road")
    {
        eType = tt_Normal;
        iCategoryFlag = 2;
    }
    else if (str == "cross")
    { // Ra: to będzie skrzyżowanie dróg
        eType = tt_Cross;
        iCategoryFlag = 2;
    }
    else if (str == "river")
    {
        eType = tt_Normal;
        iCategoryFlag = 4;
    }
    else if (str == "tributary")
    {
        eType = tt_Tributary;
        iCategoryFlag = 4;
    }
    else
        eType = tt_Unknown;
    if (Global.iWriteLogEnabled & 4)
        WriteLog(str);
    parser->getTokens(4);
    float discard {};
    *parser
        >> discard
        >> fTrackWidth
        >> fFriction
        >> fSoundDistance;
    fTrackWidth2 = fTrackWidth; // rozstaw/szerokość w punkcie 2, na razie taka sama
    parser->getTokens(2);
    *parser
        >> iQualityFlag
        >> iDamageFlag;
    if (iDamageFlag & 128)
        iAction |= 0x80; // flaga wykolejania z powodu uszkodzenia
    parser->getTokens();
    *parser >> str; // environment
    if (str == "flat")
        eEnvironment = e_flat;
    else if (str == "mountains" || str == "mountain")
        eEnvironment = e_mountains;
    else if (str == "canyon")
        eEnvironment = e_canyon;
    else if (str == "tunnel")
        eEnvironment = e_tunnel;
    else if (str == "bridge")
        eEnvironment = e_bridge;
    else if (str == "bank")
        eEnvironment = e_bank;
    else
    {
        eEnvironment = e_unknown;
        Error( "Unknown track environment: \"" + str + "\"" );
    }
    parser->getTokens();
    *parser >> token;
    m_visible = (token.compare("vis") == 0); // visible
    if (m_visible)
    {
        parser->getTokens();
        *parser >> str; // railtex
        m_material1 = (
            str == "none" ?
                null_handle :
                GfxRenderer.Fetch_Material( str ) );
        parser->getTokens();
        *parser >> fTexLength; // tex tile length
        if (fTexLength < 0.01)
            fTexLength = 4; // Ra: zabezpiecznie przed zawieszeniem
        parser->getTokens();
        *parser >> str; // sub || railtex
        m_material2 = (
            str == "none" ?
                null_handle :
                GfxRenderer.Fetch_Material( str ) );
        parser->getTokens(3);
        *parser
            >> fTexHeight1
            >> fTexWidth
            >> fTexSlope;

        if (iCategoryFlag & 4)
            fTexHeight1 = -fTexHeight1; // rzeki mają wysokość odwrotnie niż drogi
    }
    else if (Global.iWriteLogEnabled & 4)
        WriteLog("unvis");
    Init(); // ustawia SwitchExtension
    double segsize = 5.0; // długość odcinka segmentowania

    // path data
    // all subtypes contain at least one path
    m_paths.emplace_back();
    m_paths.back().deserialize( *parser, pOrigin );
    switch( eType ) {
        case tt_Switch:
        case tt_Cross:
        case tt_Tributary: {
            // these subtypes contain additional path
            m_paths.emplace_back();
            m_paths.back().deserialize( *parser, pOrigin );
            break;
        }
        default: {
            break;
        }
    }

    switch (eType) {
        // Ra: łuki segmentowane co 5m albo 314-kątem foremnym
    case tt_Table: {
        // obrotnica jest prawie jak zwykły tor
        iAction |= 2; // flaga zmiany położenia typu obrotnica
    }
    case tt_Normal: {
        // pobranie współrzędnych P1
        auto const &path { m_paths[ 0 ] };
        p1 = path.points[ segment_data::point::start ];
        // pobranie współrzędnych punktów kontrolnych
        cp1 = path.points[ segment_data::point::control1 ];
        cp2 = path.points[ segment_data::point::control2 ];
        // pobranie współrzędnych P2
        p2 = path.points[ segment_data::point::end ];
        r1 = path.rolls[ 0 ];
        r2 = path.rolls[ 1 ];
        fRadius = std::abs( path.radius ); // we wpisie może być ujemny

        if (iCategoryFlag & 1)
        { // zero na główce szyny
          // TODO: delay these calculations unti rail profile and thus height is known
            auto const railheight { 0.18 };
            p1.y += railheight;
            p2.y += railheight;
            // na przechyłce doliczyć jeszcze pół przechyłki
        }

        if( ( ( ( p1 + p1 + p2 ) / 3.0 - p1 - cp1 ).Length() < 0.02 )
         || ( ( ( p1 + p2 + p2 ) / 3.0 - p2 + cp1 ).Length() < 0.02 ) ) {
            // "prostowanie" prostych z kontrolnymi, dokładność 2cm
            cp1 = cp2 = Math3D::vector3( 0, 0, 0 );
        }

        if( fRadius != 0 ) {
            // gdy podany promień
            segsize =
                clamp(
                    std::abs( fRadius ) * ( 0.02 / Global.SplineFidelity ),
                    2.0 / Global.SplineFidelity,
                    10.0 / Global.SplineFidelity );
        }
        else {
            // HACK: crude check whether claimed straight is an actual straight piece
            if( ( cp1 == Math3D::vector3() )
             && ( cp2 == Math3D::vector3() ) ) {
                segsize = 10.0; // for straights, 10m per segment works good enough
            }
            else {
                // HACK: divide roughly in 10 segments. 
                segsize =
                    clamp(
                        ( p1 - p2 ).Length() * 0.1,
                        2.0 / Global.SplineFidelity,
                        10.0 / Global.SplineFidelity );
            }
        }

        if( ( cp1 == Math3D::vector3( 0, 0, 0 ) )
         && ( cp2 == Math3D::vector3( 0, 0, 0 ) ) ) {
            // Ra: hm, czasem dla prostego są podane...
            // gdy prosty, kontrolne wyliczane przy zmiennej przechyłce
            Segment->Init( p1, p2, segsize, r1, r2 );
        }
        else {
            // gdy łuk (ustawia bCurve=true)
            Segment->Init( p1, cp1 + p1, cp2 + p2, p2, segsize, r1, r2 );
        }

        if ((r1 != 0) || (r2 != 0))
            iTrapezoid = 1; // są przechyłki do uwzględniania w rysowaniu

        if (eType == tt_Table) // obrotnica ma doklejkę
        { // SwitchExtension=new TSwitchExtension(this,1); //dodatkowe zmienne dla obrotnicy
            SwitchExtension->Segments[0]->Init(p1, p2, segsize, r1, r2 ); // kopia oryginalnego toru
        }
        else if (iCategoryFlag & 2)
            if (m_material1 && fTexLength)
            { // dla drogi trzeba ustalić proporcje boków nawierzchni
                float w = 1.0f, h = 1.0f;
                opengl_material &mat1 = GfxRenderer.Material(m_material1);
                if (mat1.textures[0] != null_handle)
                {
                    w = GfxRenderer.Texture(mat1.textures[0]).width();
                    h = GfxRenderer.Texture(mat1.textures[0]).height();
                }
                if (h != 0.0)
                    fTexRatio1 = w / h; // proporcja boków
                opengl_material &mat2 = GfxRenderer.Material(m_material2);
                if (mat2.textures[0] != null_handle)
                {
                    w = GfxRenderer.Texture(mat2.textures[0]).width();
                    h = GfxRenderer.Texture(mat2.textures[0]).height();
                }
                if (h != 0.0)
                    fTexRatio2 = w / h; // proporcja boków
            }
        break;
    }
    case tt_Cross: {
        // skrzyżowanie dróg - 4 punkty z wektorami kontrolnymi
//        segsize = 1.0; // specjalne segmentowanie ze względu na małe promienie
    }
    case tt_Tributary: // dopływ
    case tt_Switch: { // zwrotnica
        iAction |= 1; // flaga zmiany położenia typu zwrotnica lub skrzyżowanie dróg
        // problemy z animacją iglic powstaje, gdzy odcinek prosty ma zmienną przechyłkę
        // wtedy dzieli się na dodatkowe odcinki (po 0.2m, bo R=0) i animację diabli biorą
        // Ra: na razie nie podejmuję się przerabiania iglic

        // SwitchExtension=new TSwitchExtension(this,eType==tt_Cross?6:2); //zwrotnica ma doklejkę
        auto const &path { m_paths[ 0 ] };
        p1 = path.points[ segment_data::point::start ];
        // pobranie współrzędnych punktów kontrolnych
        cp1 = path.points[ segment_data::point::control1 ];
        cp2 = path.points[ segment_data::point::control2 ];
        // pobranie współrzędnych P2
        p2 = path.points[ segment_data::point::end ];
        r1 = path.rolls[ 0 ];
        r2 = path.rolls[ 1 ];
        fRadiusTable[0] = std::abs( path.radius ); // we wpisie może być ujemny

        if (iCategoryFlag & 1)
        { // zero na główce szyny
          // TODO: delay these calculations unti rail profile and thus height is known
            auto const railheight { 0.18 };
            p1.y += railheight;
            p2.y += railheight;
            // na przechyłce doliczyć jeszcze pół przechyłki?
        }

        if( eType != tt_Cross ) {
            // dla skrzyżowań muszą być podane kontrolne
            if( ( ( ( p1 + p1 + p2 ) / 3.0 - p1 - cp1 ).Length() < 0.02 )
             || ( ( ( p1 + p2 + p2 ) / 3.0 - p2 + cp1 ).Length() < 0.02 ) ) {
                // "prostowanie" prostych z kontrolnymi, dokładność 2cm
                cp1 = cp2 = Math3D::vector3( 0, 0, 0 );
            }
        }

        if( fRadiusTable[ 0 ] != 0 ) {
            // gdy podany promień
            segsize =
                clamp(
                    std::abs( fRadiusTable[ 0 ] ) * ( 0.02 / Global.SplineFidelity ),
                    2.0 / Global.SplineFidelity,
                    10.0 / Global.SplineFidelity );
        }
        else {
            // HACK: crude check whether claimed straight is an actual straight piece
            if( ( cp1 == Math3D::vector3() )
             && ( cp2 == Math3D::vector3() ) ) {
                segsize = 10.0; // for straights, 10m per segment works good enough
            }
            else {
                // HACK: divide roughly in 10 segments. 
                segsize =
                    clamp(
                        ( p1 - p2 ).Length() * 0.1,
                        2.0 / Global.SplineFidelity,
                        10.0 / Global.SplineFidelity );
            }
        }

        if( ( cp1 == Math3D::vector3( 0, 0, 0 ) )
         && ( cp2 == Math3D::vector3( 0, 0, 0 ) ) ) {
            // Ra: hm, czasem dla prostego są podane...
            // gdy prosty, kontrolne wyliczane przy zmiennej przechyłce
            SwitchExtension->Segments[ 0 ]->Init( p1, p2, segsize, r1, r2 );
        }
        else {
            // gdy łuk (ustawia bCurve=true)
            SwitchExtension->Segments[ 0 ]->Init( p1, cp1 + p1, cp2 + p2, p2, segsize, r1, r2 );
        }

        auto const &path2 { m_paths[ 1 ] };
        p3 = path2.points[ segment_data::point::start ];
        // pobranie współrzędnych punktów kontrolnych
        cp3 = path2.points[ segment_data::point::control1 ];
        cp4 = path2.points[ segment_data::point::control2 ];
        // pobranie współrzędnych P2
        p4 = path2.points[ segment_data::point::end ];
        r3 = path2.rolls[ 0 ];
        r4 = path2.rolls[ 1 ];
        fRadiusTable[1] = std::abs( path2.radius ); // we wpisie może być ujemny

        if (iCategoryFlag & 1)
        { // zero na główce szyny
          // TODO: delay these calculations unti rail profile and thus height is known
            auto const railheight{ 0.18 };
            p3.y += railheight;
            p4.y += railheight;
            // na przechyłce doliczyć jeszcze pół przechyłki?
        }

        if( eType != tt_Cross ) {
            // dla skrzyżowań muszą być podane kontrolne
            if( ( ( ( p3 + p3 + p4 ) / 3.0 - p3 - cp3 ).Length() < 0.02 )
             || ( ( ( p3 + p4 + p4 ) / 3.0 - p4 + cp3 ).Length() < 0.02 ) ) {
                // "prostowanie" prostych z kontrolnymi, dokładność 2cm
                cp3 = cp4 = Math3D::vector3( 0, 0, 0 );
            }
        }

        if( fRadiusTable[ 1 ] != 0 ) {
            // gdy podany promień
            segsize =
                clamp(
                    std::abs( fRadiusTable[ 1 ] ) * ( 0.02 / Global.SplineFidelity ),
                    2.0 / Global.SplineFidelity,
                    10.0 / Global.SplineFidelity );
        }
        else {
            // HACK: crude check whether claimed straight is an actual straight piece
            if( ( cp3 == Math3D::vector3() )
             && ( cp4 == Math3D::vector3() ) ) {
                segsize = 10.0; // for straights, 10m per segment works good enough
            }
            else {
                // HACK: divide roughly in 10 segments. 
                segsize =
                    clamp(
                        ( p3 - p4 ).Length() * 0.1,
                        2.0 / Global.SplineFidelity,
                        10.0 / Global.SplineFidelity );
            }
        }

        if( ( cp3 == Math3D::vector3( 0, 0, 0 ) )
         && ( cp4 == Math3D::vector3( 0, 0, 0 ) ) ) {
            // Ra: hm, czasem dla prostego są podane...
            // gdy prosty, kontrolne wyliczane przy zmiennej przechyłce
            SwitchExtension->Segments[ 1 ]->Init( p3, p4, segsize, r3, r4 );
        }
        else {
            if( eType != tt_Cross ) {
                SwitchExtension->Segments[ 1 ]->Init( p3, p3 + cp3, p4 + cp4, p4, segsize, r3, r4 );
            }
            else {
                // dla skrzyżowania dróg dać odwrotnie końce, żeby brzegi generować lewym
                SwitchExtension->Segments[ 1 ]->Init( p4, p4 + cp4, p3 + cp3, p3, segsize, r4, r3 ); // odwrócony
            }
        }

        if (eType == tt_Cross)
        { // Ra 2014-07: dla skrzyżowań będą dodatkowe segmenty
            SwitchExtension->Segments[2]->Init(p2, cp2 + p2, cp4 + p4, p4, segsize, r2, r4); // z punktu 2 do 4
            if (LengthSquared3(p3 - p1) < 0.01) // gdy mniej niż 10cm, to mamy skrzyżowanie trzech dróg
                SwitchExtension->iRoads = 3;
            else // dla 4 dróg będą dodatkowe 3 segmenty
            {
                SwitchExtension->Segments[3]->Init(p4, p4 + cp4, p1 + cp1, p1, segsize, r4, r1); // z punktu 4 do 1
                SwitchExtension->Segments[4]->Init(p1, p1 + cp1, p3 + cp3, p3, segsize, r1, r3); // z punktu 1 do 3
                SwitchExtension->Segments[5]->Init(p3, p3 + cp3, p2 + cp2, p2, segsize, r3, r2); // z punktu 3 do 2
            }
        }

        Switch(0); // na stałe w położeniu 0 - nie ma początkowego stanu zwrotnicy we wpisie

        if( eType == tt_Switch )
        // Ra: zamienić później na iloczyn wektorowy
        {
            Math3D::vector3 v1, v2;
            double a1, a2;
            v1 =  SwitchExtension->Segments[0]->FastGetPoint_1()
                - SwitchExtension->Segments[0]->FastGetPoint_0();
            v2 =  SwitchExtension->Segments[1]->FastGetPoint_1()
                - SwitchExtension->Segments[1]->FastGetPoint_0();
            a1 = atan2(v1.x, v1.z);
            a2 = atan2(v2.x, v2.z);
            a2 = a2 - a1;
            while (a2 > M_PI)
                a2 = a2 - 2 * M_PI;
            while (a2 < -M_PI)
                a2 = a2 + 2 * M_PI;
            SwitchExtension->RightSwitch = a2 < 0; // lustrzany układ OXY...
        }
        break;
        }
    }

    // optional attributes
    parser->getTokens();
    *parser >> token;
    str = token;
    while (str != "endtrack")
    {
        if (str == "event0")
        {
            parser->getTokens();
            *parser >> token;
            m_events0.emplace_back( token, nullptr );
        }
        else if (str == "event1")
        {
            parser->getTokens();
            *parser >> token;
            m_events1.emplace_back( token, nullptr );
        }
        else if (str == "event2")
        {
            parser->getTokens();
            *parser >> token;
            m_events2.emplace_back( token, nullptr );
        }
        else if (str == "eventall0")
        {
            parser->getTokens();
            *parser >> token;
            m_events0all.emplace_back( token, nullptr );
        }
        else if (str == "eventall1")
        {
            parser->getTokens();
            *parser >> token;
            m_events1all.emplace_back( token, nullptr );
        }
        else if (str == "eventall2")
        {
            parser->getTokens();
            *parser >> token;
            m_events2all.emplace_back( token, nullptr );
        }
        else if (str == "velocity")
        {
            parser->getTokens();
            *parser >> fVelocity; //*0.28; McZapkie-010602
            if (SwitchExtension) // jeśli tor ruchomy
                if (std::abs(fVelocity) >= 1.0) //żeby zero nie ograniczało dożywotnio
                    // zapamiętanie głównego ograniczenia; a np. -40 ogranicza tylko na bok
                    SwitchExtension->fVelocity = static_cast<float>(fVelocity);
        }
        else if (str == "isolated")
        { // obwód izolowany, do którego tor należy
            parser->getTokens();
            *parser >> token;
            pIsolated = TIsolated::Find(token);
        }
        else if (str == "angle1")
        { // kąt ścięcia końca od strony 1
            // NOTE: not used/implemented
            parser->getTokens();
            *parser >> a1;
            //Segment->AngleSet(0, a1);
        }
        else if (str == "angle2")
        { // kąt ścięcia końca od strony 2
          // NOTE: not used/implemented
            parser->getTokens();
            *parser >> a2;
            //Segment->AngleSet(1, a2);
        }
        else if (str == "fouling1")
        { // wskazanie modelu ukresu w kierunku 1
          // NOTE: not used/implemented
            parser->getTokens();
            *parser >> token;
            // nFouling[0]=
        }
        else if (str == "fouling2")
        { // wskazanie modelu ukresu w kierunku 2
          // NOTE: not used/implemented
            parser->getTokens();
            *parser >> token;
            // nFouling[1]=
        }
        else if (str == "overhead")
        { // informacja o stanie sieci: 0-jazda bezprądowa, >0-z opuszczonym i ograniczeniem prędkości
            parser->getTokens();
            *parser >> fOverhead;
            if (fOverhead > 0.0)
                iAction |= 0x40; // flaga opuszczenia pantografu (tor uwzględniany w skanowaniu jako
            // ograniczenie dla pantografujących)
        }
        else if( str == "vradius" ) {
            // y-axis track radius
            // NOTE: not used/implemented
            parser->getTokens();
            *parser >> fVerticalRadius;
        }
        else if( str == "trackbed" ) {
            // switch trackbed texture
            auto const trackbedtexture { parser->getToken<std::string>() };
            if( eType == tt_Switch ) {
                SwitchExtension->m_material3 = GfxRenderer.Fetch_Material( trackbedtexture );
            }
        }
        else if( str == "railprofile" ) {
            // rail profile
            auto const railprofile { parser->getToken<std::string>() };
            if( iCategoryFlag == 1 ) {
                m_profile1 = fetch_track_rail_profile( railprofile );
            }
        }
        else
            ErrorLog("Bad track: unknown property: \"" + str + "\" defined for track \"" + m_name + "\"");
        parser->getTokens();
        *parser >> token;
		str = token;
    }
    // alternatywny zapis nazwy odcinka izolowanego - po znaku "@" w nazwie toru
    if (!pIsolated)
        if ((i = m_name.find("@")) != std::string::npos)
            if (i < m_name.length()) // nie może być puste
            {
                pIsolated = TIsolated::Find(m_name.substr(i + 1, m_name.length()));
                m_name = m_name.substr(0, i - 1); // usunięcie z nazwy
            }

    // calculate path location
    location( (
        CurrentSegment()->FastGetPoint_0()
        + CurrentSegment()->FastGetPoint( 0.5 )
        + CurrentSegment()->FastGetPoint_1() )
        / 3.0 );
}

bool TTrack::AssignEvents() {

    bool lookupfail { false };

    std::vector< std::pair< std::string, event_sequence * > > const eventsequences {
        { "event0", &m_events0 }, { "eventall0", &m_events0all },
        { "event1", &m_events1 }, { "eventall1", &m_events1all },
        { "event2", &m_events2 }, { "eventall2", &m_events2all } };

    for( auto &eventsequence : eventsequences ) {
        for( auto &event : *( eventsequence.second ) ) {
            event.second = simulation::Events.FindEvent( event.first );
            if( event.second != nullptr ) {
                m_events = true;
            }
            else {
                ErrorLog( "Bad track: \"" + m_name + "\" can't find assigned event \"" + event.first + "\"" );
                lookupfail = true;
            }
        }
    }

    auto const trackname { name() };

    if( ( Global.iHiddenEvents & 1 )
     && ( false == trackname.empty() ) ) {
        // jeśli podana jest nazwa torów, można szukać eventów skojarzonych przez nazwę
        for( auto &eventsequence : eventsequences ) {
            auto *event = simulation::Events.FindEvent( trackname + ':' + eventsequence.first );
            if( event != nullptr ) {
                // HACK: auto-associated events come with empty lookup string, to avoid including them in the text format export
                eventsequence.second->emplace_back( "", event );
                m_events = true;
            }
        }
    }

    return ( lookupfail == false );
}

bool TTrack::AssignForcedEvents(basic_event *NewEventPlus, basic_event *NewEventMinus)
{ // ustawienie eventów sygnalizacji rozprucia
    if (SwitchExtension)
    {
        if (NewEventPlus)
            SwitchExtension->evPlus = NewEventPlus;
        if (NewEventMinus)
            SwitchExtension->evMinus = NewEventMinus;
        return true;
    }
    return false;
};

void TTrack::QueueEvents( event_sequence const &Events, TDynamicObject const *Owner ) {

    for( auto const &event : Events ) {
        if( event.second != nullptr ) {
            simulation::Events.AddToQuery( event.second, Owner );
        }
    }
}

void TTrack::QueueEvents( event_sequence const &Events, TDynamicObject const *Owner, double const Delaylimit ) {

    for( auto const &event : Events ) {
        if( ( event.second != nullptr )
         && ( event.second->m_delay <= Delaylimit) ) {
            simulation::Events.AddToQuery( event.second, Owner );
        }
    }
}

std::string TTrack::IsolatedName()
{ // podaje nazwę odcinka izolowanego, jesli nie ma on jeszcze przypisanych zdarzeń
    if (pIsolated)
        if (!pIsolated->evBusy && !pIsolated->evFree)
            return pIsolated->asName;
    return "";
};

// ABu: przeniesione z Path.h i poprawione!!!
bool TTrack::AddDynamicObject(TDynamicObject *Dynamic)
{ // dodanie pojazdu do trajektorii
    // Ra: tymczasowo wysyłanie informacji o zajętości konkretnego toru
    // Ra: usunąć po upowszechnieniu się odcinków izolowanych
    if (iCategoryFlag & 0x100) // jeśli usuwaczek
    {
        Dynamic->MyTrack = NULL; // trzeba by to uzależnić od kierunku ruchu...
        return true;
    }
    if( Global.iMultiplayer ) {
        // jeśli multiplayer
        if( true == Dynamics.empty() ) {
            // pierwszy zajmujący
            if( m_name != "none" ) {
                // przekazanie informacji o zajętości toru
                multiplayer::WyslijString( m_name, 8 );
            }
        }
    }
    Dynamics.emplace_back( Dynamic );
    Dynamic->MyTrack = this; // ABu: na ktorym torze jesteśmy
    if( Dynamic->iOverheadMask ) {
        // jeśli ma pantografy
        Dynamic->OverheadTrack( fOverhead ); // przekazanie informacji o jeździe bezprądowej na tym odcinku toru
    }
    return true;
};

const int numPts = 4;

bool TTrack::CheckDynamicObject(TDynamicObject *Dynamic)
{ // sprawdzenie, czy pojazd jest przypisany do toru
    for( auto dynamic : Dynamics ) {
        if( dynamic == Dynamic ) {
            return true;
        }
    }
    return false;
};

bool TTrack::RemoveDynamicObject(TDynamicObject *Dynamic)
{ // usunięcie pojazdu z listy przypisanych do toru
    bool result = false;
    if( *Dynamics.begin() == Dynamic ) {
        // most likely the object getting removed is at the front...
        Dynamics.pop_front();
        result = true;
    }
    else if( *( --Dynamics.end() ) == Dynamic ) {
        // ...or the back of the queue...
        Dynamics.pop_back();
        result = true;
    }
    else {
        // ... if these fail, check all objects one by one
        for( auto idx = Dynamics.begin(); idx != Dynamics.end(); /*iterator advancement is inside the loop*/ ) {
            if( *idx == Dynamic ) {
                idx = Dynamics.erase( idx );
                result = true;
                break; // object are unique, so we can bail out here.
            }
            else {
                ++idx;
            }
        }
    }
    if( Global.iMultiplayer ) {
        // jeśli multiplayer
        if( true == Dynamics.empty() ) {
            // jeśli już nie ma żadnego
            if( m_name != "none" ) {
                // przekazanie informacji o zwolnieniu toru
                multiplayer::WyslijString( m_name, 9 );
            }
        }
    }
    
    return result;
}

bool TTrack::InMovement()
{ // tory animowane (zwrotnica, obrotnica) mają SwitchExtension
    if (SwitchExtension)
    {
        if (eType == tt_Switch)
            return SwitchExtension->bMovement; // ze zwrotnicą łatwiej
        if (eType == tt_Table)
            if (SwitchExtension->pModel)
            {
                if (!SwitchExtension->CurrentIndex)
                    return false; // 0=zablokowana się nie animuje
                // trzeba każdorazowo porównywać z kątem modelu
                TAnimContainer *ac = (
                    SwitchExtension->pModel ?
                        SwitchExtension->pModel->GetContainer() :
                        nullptr );
                return ac ?
                           (ac->AngleGet() != SwitchExtension->fOffset) ||
                               !(ac->TransGet() == SwitchExtension->vTrans) :
                           false;
                // return true; //jeśli jest taki obiekt
            }
    }
    return false;
};

void TTrack::RaAssign( TAnimModel *am, basic_event *done, basic_event *joined )
{ // Ra: wiązanie toru z modelem obrotnicy
    if (eType == tt_Table)
    {
        SwitchExtension->pModel = am;
        SwitchExtension->evMinus = done; // event zakończenia animacji (zadanie nowej przedłuża)
        SwitchExtension->evPlus = joined; // event potwierdzenia połączenia (gdy nie znajdzie, to się nie połączy)
        if( ( am != nullptr ) && ( am->GetContainer() ) ) {// może nie być?
            am->GetContainer()->EventAssign( done ); // zdarzenie zakończenia animacji
        }
    }
};

void TTrack::create_map_geometry(std::vector<gfx::basic_vertex> &Bank)
{
    if (iCategoryFlag != 1)
        return; // only tracks for now

    for (auto &v : endpoints())
        Bank.push_back(gfx::basic_vertex(glm::vec3(v), glm::vec3(0.0f), glm::vec2(0.0f)));
}

// wypełnianie tablic VBO
void TTrack::create_geometry( gfx::geometrybank_handle const &Bank ) {

    switch (iCategoryFlag & 15)
    {
    case 1: // tor
    {
        // zwykla szyna: //Ra: czemu główki są asymetryczne na wysokości 0.140?
        gfx::vertex_array rpts1, rpts2;
        create_track_rail_profile( rpts1, rpts2 );
        switch (eType) // dalej zależnie od typu
        {
        case tt_Table: // obrotnica jak zwykły tor, tylko animacja dochodzi
        case tt_Normal:
            if (m_material2)
            { // podsypka z podkładami jest tylko dla zwykłego toru
                gfx::vertex_array bpts1;
                create_track_bed_profile( bpts1, trPrev, trNext );
                auto const texturelength { texture_length( m_material2 ) };
                gfx::vertex_array vertices;
                Segment->RenderLoft(vertices, m_origin, bpts1, iTrapezoid > 0, texturelength);
                if( ( Bank != 0 ) && ( true == Geometry2.empty() ) ) {
                    Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                }
                if( ( Bank == 0 ) && ( false == Geometry2.empty() ) ) {
                    // special variant, replace existing data for a turntable track
                    GfxRenderer.Replace( vertices, Geometry2[ 0 ], GL_TRIANGLE_STRIP );
                }
            }
            if (m_material1)
            { // szyny - generujemy dwie, najwyżej rysować się będzie jedną
                auto const nnumPts { track_rail_profile( m_profile1.second ).size() / 2 };
                auto const texturelength { texture_length( m_material1 ) };
                gfx::vertex_array vertices;
                if( ( Bank != 0 ) && ( true == Geometry1.empty() ) ) {
                    Segment->RenderLoft( vertices, m_origin, rpts1, iTrapezoid > 0, texturelength );
                    Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                    vertices.clear(); // reuse the scratchpad
                    Segment->RenderLoft( vertices, m_origin, rpts2, iTrapezoid > 0, texturelength );
                    Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                }
                if( ( Bank == 0 ) && ( false == Geometry1.empty() ) ) {
                    // special variant, replace existing data for a turntable track
                    Segment->RenderLoft( vertices, m_origin, rpts1, iTrapezoid > 0, texturelength );
                    GfxRenderer.Replace( vertices, Geometry1[ 0 ], GL_TRIANGLE_STRIP );
                    vertices.clear(); // reuse the scratchpad
                    Segment->RenderLoft( vertices, m_origin, rpts2, iTrapezoid > 0, texturelength );
                    GfxRenderer.Replace( vertices, Geometry1[ 1 ], GL_TRIANGLE_STRIP );
                }
            }
            break;
        case tt_Switch: // dla zwrotnicy dwa razy szyny
            if( m_material1 || m_material2 ) {
                // iglice liczone tylko dla zwrotnic
                gfx::vertex_array rpts3, rpts4;
                create_track_blade_profile( rpts3, rpts4 );
                // TODO, TBD: change all track geometry to triangles, to allow packing data in less, larger buffers
                auto const nnumPts { track_rail_profile( m_profile1.second ).size() / 2 };
                auto const bladelength { static_cast<int>( std::ceil( SwitchExtension->Segments[ 0 ]->RaSegCount() * 0.65 ) ) };
                // positive jointlength: the switch is typically used along the main track, negative: along the diverging track
                // TODO: determine this from names of textures assigned to the tracks
//                auto const jointlength { static_cast<int>( std::ceil( SwitchExtension->Segments[ 0 ]->RaSegCount() * 0.15 ) ) };
                auto const jointlength { 0 }; // temporary until implementation of the above
                if (SwitchExtension->RightSwitch)
                { // nowa wersja z SPKS, ale odwrotnie lewa/prawa
                    gfx::vertex_array vertices;
                    if( m_material1 ) {
                        auto const texturelength { texture_length( m_material1 ) };
                        // left blade
                        // composed from two parts: transition from blade to regular rail, and regular rail
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts3, true, texturelength, 1.0, 0, bladelength / 2, { SwitchExtension->fOffset2, SwitchExtension->fOffset2 / 2 } );
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, bladelength / 2, bladelength, { SwitchExtension->fOffset2 / 2, 0.f } );
                        Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        // fixed parts
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, bladelength );
                        Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        if( jointlength > 0 ) {
                            // part of the diverging rail touched by wheels of vehicle going straight
                            SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, 0, jointlength );
                            Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        // other rail, full length
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength );
                        Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                    }
                    if( m_material2 ) {
                        auto const texturelength { texture_length( m_material2 ) };
                        // right blade
                        // composed from two parts: transition from blade to regular rail, and regular rail
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts4, true, texturelength, 1.0, 0, bladelength / 2, { -fMaxOffset + SwitchExtension->fOffset1, ( -fMaxOffset + SwitchExtension->fOffset1 ) / 2 } );
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, bladelength / 2, bladelength, { ( -fMaxOffset + SwitchExtension->fOffset1 ) / 2, 0.f } );
                        Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        // fixed parts
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, bladelength );
                        Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        // diverging rail, potentially minus part touched by wheels of vehicle going straight
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, jointlength );
                        Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                    }
                }
                else
                { // lewa działa lepiej niż prawa
                    gfx::vertex_array vertices;
                    if( m_material1 ) {
                        auto const texturelength { texture_length( m_material1 ) };
                        // right blade
                        // composed from two parts: transition from blade to regular rail, and regular rail
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts4, true, texturelength, 1.0, 0, bladelength / 2, { -SwitchExtension->fOffset2, -SwitchExtension->fOffset2 / 2 } );
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, bladelength / 2, bladelength, { -SwitchExtension->fOffset2 / 2, 0.f } );
                        Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        // fixed parts
                        // prawa szyna za iglicą
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, bladelength );
                        Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        if( jointlength > 0 ) {
                            // part of the diverging rail touched by wheels of vehicle going straight
                            SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, 0, jointlength );
                            Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        // other rail, full length
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength );
                        Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                    }
                    if( m_material2 ) {
                        auto const texturelength { texture_length( m_material2 ) };
                        // left blade
                        // composed from two parts: transition from blade to regular rail, and regular rail
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts3, true, texturelength, 1.0, 0, bladelength / 2, { fMaxOffset - SwitchExtension->fOffset1, ( fMaxOffset - SwitchExtension->fOffset1 ) / 2 } );
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, bladelength / 2, bladelength, { ( fMaxOffset - SwitchExtension->fOffset1 ) / 2, 0.f } );
                        Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        // fixed parts
                        // lewa szyna za iglicą
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, bladelength );
                        Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                        // diverging rail, potentially minus part touched by wheels of vehicle going straight
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, jointlength );
                        Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                        vertices.clear();
                    }
                }
            }
            // auto-generated switch trackbed
            if( true == Global.CreateSwitchTrackbeds ) {
                gfx::vertex_array vertices;
                create_switch_trackbed( vertices );
                SwitchExtension->Geometry3 = GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP );
                vertices.clear();
            }

            break;
        }
    } // koniec obsługi torów
    break;
    case 2: // McZapkie-260302 - droga - rendering
        switch (eType) // dalej zależnie od typu
        {
        case tt_Normal: // drogi proste, bo skrzyżowania osobno
        {
            gfx::vertex_array bpts1; // punkty głównej płaszczyzny przydają się do robienia boków
            if (m_material1 || m_material2) {
                // punkty się przydadzą, nawet jeśli nawierzchni nie ma
                create_road_profile( bpts1 );
            }
            if (m_material1) // jeśli podana była tekstura, generujemy trójkąty
            { // tworzenie trójkątów nawierzchni szosy
                auto const texturelength { texture_length( m_material1 ) };
                gfx::vertex_array vertices;
                Segment->RenderLoft(vertices, m_origin, bpts1, iTrapezoid > 0, texturelength);
                Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
            }
            if (m_material2)
            { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak w Midtown Madness 2?)
                auto const side{ std::abs( fTexWidth ) }; // szerokść podsypki na zewnątrz szyny albo pobocza
                auto const slop{ std::abs( fTexSlope ) }; // brzeg zewnętrzny
                auto const texturelength { texture_length( m_material2 ) };
                gfx::vertex_array rpts1, rpts2; // współrzędne przekroju i mapowania dla prawej i lewej strony
                create_road_side_profile( rpts1, rpts2, bpts1 );
                gfx::vertex_array vertices;
                if( ( fTexHeight1 >= 0.0 ) || ( slop != 0.0 ) ) {
                    // tylko jeśli jest z prawej
                    Segment->RenderLoft( vertices, m_origin, rpts1, iTrapezoid > 0, texturelength );
                    Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                    vertices.clear();
                }
                if( ( fTexHeight1 >= 0.0 ) || ( side != 0.0 ) ) {
                    // tylko jeśli jest z lewej
                    Segment->RenderLoft( vertices, m_origin, rpts2, iTrapezoid > 0, texturelength );
                    Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                    vertices.clear();
                }
            }
            break;
        }
        case tt_Cross: // skrzyżowanie dróg rysujemy inaczej
        { // ustalenie współrzędnych środka - przecięcie Point1-Point2 z CV4-Point4
            double a[4]; // kąty osi ulic wchodzących
            Math3D::vector3 p[4]; // punkty się przydadzą do obliczeń
            // na razie połowa odległości pomiędzy Point1 i Point2, potem się dopracuje
            a[0] = a[1] = 0.5; // parametr do poszukiwania przecięcia łuków
            // modyfikować a[0] i a[1] tak, aby trafić na przecięcie odcinka 34
            p[0] = SwitchExtension->Segments[0]->FastGetPoint(a[0]); // współrzędne środka pierwszego odcinka
            p[1] = SwitchExtension->Segments[1]->FastGetPoint(a[1]); //-//- drugiego
            // p[2]=p[1]-p[0]; //jeśli różne od zera, przeliczyć a[0] i a[1] i wyznaczyć nowe punkty
            Math3D::vector3 oxz = p[0]; // punkt mapowania środka tekstury skrzyżowania
            p[0] = SwitchExtension->Segments[0]->GetDirection1(); // Point1 - pobranie wektorów kontrolnych
            p[1] = SwitchExtension->Segments[1]->GetDirection2(); // Point3 (bo zamienione)
            p[2] = SwitchExtension->Segments[0]->GetDirection2(); // Point2
            p[3] = SwitchExtension->Segments[1]->GetDirection1(); // Point4 (bo zamienione)
            a[0] = atan2(-p[0].x, p[0].z); // kąty stycznych osi dróg
            a[1] = atan2(-p[1].x, p[1].z);
            a[2] = atan2(-p[2].x, p[2].z);
            a[3] = atan2(-p[3].x, p[3].z);
            p[0] = SwitchExtension->Segments[0]->FastGetPoint_0(); // Point1 - pobranie współrzędnych końców
            p[1] = SwitchExtension->Segments[1]->FastGetPoint_1(); // Point3
            p[2] = SwitchExtension->Segments[0]->FastGetPoint_1(); // Point2
            p[3] = SwitchExtension->Segments[1]->FastGetPoint_0(); // Point4 - przy trzech drogach pokrywa się z Point1
            // 2014-07: na początek rysować brzegi jak dla łuków
            // punkty brzegu nawierzchni uzyskujemy podczas renderowania boków (bez sensu, ale najszybciej było zrobić)
            int pointcount;
            if( SwitchExtension->iRoads == 3 ) {
                // mogą być tylko 3 drogi zamiast 4
                pointcount =
                    SwitchExtension->Segments[ 0 ]->RaSegCount()
                    + SwitchExtension->Segments[ 1 ]->RaSegCount()
                    + SwitchExtension->Segments[ 2 ]->RaSegCount();
            }
            else {
                pointcount =
                    SwitchExtension->Segments[ 2 ]->RaSegCount()
                    + SwitchExtension->Segments[ 3 ]->RaSegCount()
                    + SwitchExtension->Segments[ 4 ]->RaSegCount()
                    + SwitchExtension->Segments[ 5 ]->RaSegCount();
            }
            if (!SwitchExtension->vPoints)
            { // jeśli tablica punktów nie jest jeszcze utworzona, zliczamy punkty i tworzymy ją
                // we'll need to add couple extra points for the complete fan we'll build
                SwitchExtension->vPoints = new glm::vec3[pointcount + SwitchExtension->iRoads];
            }
            glm::vec3 *b =
                SwitchExtension->bPoints ?
                    nullptr :
                    SwitchExtension->vPoints; // zmienna robocza, NULL gdy tablica punktów już jest wypełniona
            gfx::vertex_array bpts1; // punkty głównej płaszczyzny przydają się do robienia boków
            if (m_material1 || m_material2) { // punkty się przydadzą, nawet jeśli nawierzchni nie ma
                create_road_profile( bpts1, true );
            }
            // najpierw renderowanie poboczy i zapamiętywanie punktów
            // problem ze skrzyżowaniami jest taki, że teren chce się pogrupować wg tekstur, ale zaczyna od nawierzchni
            // sama nawierzchnia nie wypełni tablicy punktów, bo potrzebne są pobocza
            // ale pobocza renderują się później, więc nawierzchnia nie załapuje się na renderowanie w swoim czasie
            if( m_material2 ) 
            { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak w Midtown Madness 2?)
                gfx::vertex_array rpts1, rpts2; // współrzędne przekroju i mapowania dla prawej i lewej strony
                create_road_side_profile( rpts1, rpts2, bpts1, true );
                // Ra 2014-07: trzeba to przerobić na pętlę i pobierać profile (przynajmniej 2..4) z sąsiednich dróg
                bool render = ( m_material2 != 0 ); // renderować nie trzeba, ale trzeba wyznaczyć punkty brzegowe nawierzchni
                auto const side{ std::abs( fTexWidth ) }; // szerokść podsypki na zewnątrz szyny albo pobocza
                auto const texturelength{ texture_length( m_material2 ) };
                gfx::vertex_array vertices;
                if (SwitchExtension->iRoads == 4)
                { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
                    if( ( fTexHeight1 >= 0.0 ) || ( side != 0.0 ) ) {
                        SwitchExtension->Segments[ 2 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render );
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        SwitchExtension->Segments[ 3 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render );
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        SwitchExtension->Segments[ 4 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render );
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        SwitchExtension->Segments[ 5 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render );
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                    }
                }
                else {
                    // punkt 3 pokrywa się z punktem 1, jak w zwrotnicy; połączenie 1->2 nie musi być prostoliniowe
                    if( ( fTexHeight1 >= 0.0 ) || ( side != 0.0 ) ) {
                        SwitchExtension->Segments[ 2 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render ); // z P2 do P4
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render ); // z P4 do P3=P1 (odwrócony)
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                        SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts2, true, texturelength, 1.0, 0, 0, {}, &b, render ); // z P1 do P2
                        if( true == render ) {
                            Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                            vertices.clear();
                        }
                    }
                }
            }
            // renderowanie nawierzchni na końcu
            double sina0 = sin(a[0]), cosa0 = cos(a[0]);
            double u, v;
            if( ( false == SwitchExtension->bPoints ) // jeśli tablica nie wypełniona
             && ( b != nullptr ) ) {
                SwitchExtension->bPoints = true; // tablica punktów została wypełniona
            }

            if( m_material1 ) {
                auto const texturelength { texture_length( m_material1 ) };
                gfx::vertex_array vertices;
                // jeśli podana tekstura nawierzchni
                // we start with a vertex in the middle...
                vertices.emplace_back(
                    glm::vec3{
                        oxz.x - m_origin.x,
                        oxz.y - m_origin.y,
                        oxz.z - m_origin.z },
                    glm::vec3{ 0.0f, 1.0f, 0.0f },
                    glm::vec2{ 0.5f, 0.5f } );
                // ...and add one extra vertex to close the fan...
                u = ( SwitchExtension->vPoints[ 0 ].x - oxz.x + m_origin.x ) / texturelength;
                v = ( SwitchExtension->vPoints[ 0 ].z - oxz.z + m_origin.z ) / ( fTexRatio1 * texturelength );
                vertices.emplace_back(
                    glm::vec3 {
                        SwitchExtension->vPoints[ 0 ].x,
                        SwitchExtension->vPoints[ 0 ].y,
                        SwitchExtension->vPoints[ 0 ].z },
                    glm::vec3{ 0.0f, 1.0f, 0.0f },
                    // mapowanie we współrzędnych scenerii
                    glm::vec2{
                         cosa0 * u + sina0 * v + 0.5,
                        -sina0 * u + cosa0 * v + 0.5 } );
                // ...then draw the precalculated rest
                for (int i = pointcount + SwitchExtension->iRoads - 1; i >= 0; --i) {
                    // mapowanie we współrzędnych scenerii
                    u = ( SwitchExtension->vPoints[ i ].x - oxz.x + m_origin.x ) / texturelength;
                    v = ( SwitchExtension->vPoints[ i ].z - oxz.z + m_origin.z ) / ( fTexRatio1 * texturelength );
                    vertices.emplace_back(
                        glm::vec3 {
                            SwitchExtension->vPoints[ i ].x,
                            SwitchExtension->vPoints[ i ].y,
                            SwitchExtension->vPoints[ i ].z },
                        glm::vec3{ 0.0f, 1.0f, 0.0f },
                        // mapowanie we współrzędnych scenerii
                        glm::vec2{
                             cosa0 * u + sina0 * v + 0.5,
                            -sina0 * u + cosa0 * v + 0.5 } );
                }
                Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_FAN ) );
            }
            break;
        } // tt_cross
        } // road
        break;
    case 4: // Ra: rzeki na razie jak drogi, przechyłki na pewno nie mają
        switch (eType) // dalej zależnie od typu
        {
        case tt_Normal: // drogi proste, bo skrzyżowania osobno
        {
            gfx::vertex_array bpts1; // punkty głównej płaszczyzny przydają się do robienia boków
            if (m_material1 || m_material2) { // punkty się przydadzą, nawet jeśli nawierzchni nie ma
                create_road_profile( bpts1 );
            }
            if (m_material1) // jeśli podana była tekstura, generujemy trójkąty
            { // tworzenie trójkątów nawierzchni szosy
                gfx::vertex_array vertices;
                Segment->RenderLoft(vertices, m_origin, bpts1, iTrapezoid > 0, fTexLength);
                Geometry1.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
            }
            if (m_material2)
            { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak w Midtown Madness 2?)
                gfx::vertex_array rpts1, rpts2; // współrzędne przekroju i mapowania dla prawej i lewej strony
                create_road_side_profile( rpts1, rpts2, bpts1 );
                gfx::vertex_array vertices;
                Segment->RenderLoft( vertices, m_origin, rpts1, iTrapezoid > 0, fTexLength );
                Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                vertices.clear();
                Segment->RenderLoft( vertices, m_origin, rpts2, iTrapezoid > 0, fTexLength );
                Geometry2.emplace_back( GfxRenderer.Insert( vertices, Bank, GL_TRIANGLE_STRIP ) );
                vertices.clear();
            }
        }
        }
        break;
    }
    return;
};

void TTrack::RenderDynSounds()
{ // odtwarzanie dźwięków pojazdów jest niezależne od ich wyświetlania
    for( auto dynamic : Dynamics ) {
        dynamic->RenderSounds();
    }
};
//---------------------------------------------------------------------------
bool TTrack::SetConnections(int i)
{ // przepisanie aktualnych połączeń toru do odpowiedniego segmentu
    if (SwitchExtension)
    {
        SwitchExtension->pNexts[i] = trNext;
        SwitchExtension->pPrevs[i] = trPrev;
        SwitchExtension->iNextDirection[i] = iNextDirection;
        SwitchExtension->iPrevDirection[i] = iPrevDirection;
        if (eType == tt_Switch)
        { // zwrotnica jest wyłącznie w punkcie 1, więc tor od strony Prev jest zawsze ten sam
            SwitchExtension->pPrevs[i ^ 1] = trPrev;
            SwitchExtension->iPrevDirection[i ^ 1] = iPrevDirection;
        }
        else if (eType == tt_Cross)
            if (SwitchExtension->iRoads == 3)
            {
            }
        if (i)
            Switch(0); // po przypisaniu w punkcie 4 włączyć stan zasadniczy
        return true;
    }
    Error("Cannot set connections");
    return false;
}

bool TTrack::Switch(int i, float const t, float const d)
{ // przełączenie torów z uruchomieniem animacji
    if (SwitchExtension) // tory przełączalne mają doklejkę
        if (eType == tt_Switch)
        { // przekładanie zwrotnicy jak zwykle
            if (t > 0.f) // prędkość liniowa ruchu iglic
                SwitchExtension->fOffsetSpeed = t; // prędkość łatwiej zgrać z animacją modelu
            if (d >= 0.f) // dodatkowy ruch drugiej iglicy (zamknięcie nastawnicze)
                SwitchExtension->fOffsetDelay = d;
            i &= 1; // ograniczenie błędów !!!!
            SwitchExtension->fDesiredOffset =
                i ? fMaxOffset + SwitchExtension->fOffsetDelay : -SwitchExtension->fOffsetDelay;
            SwitchExtension->CurrentIndex = i;
            Segment = SwitchExtension->Segments[i]; // wybranie aktywnej drogi - potrzebne to?
            trNext = SwitchExtension->pNexts[i]; // przełączenie końców
            trPrev = SwitchExtension->pPrevs[i];
            iNextDirection = SwitchExtension->iNextDirection[i];
            iPrevDirection = SwitchExtension->iPrevDirection[i];
            fRadius = fRadiusTable[i]; // McZapkie: wybor promienia toru
            if( SwitchExtension->fVelocity <= -2 ) {
                //-1 oznacza maksymalną prędkość, a dalsze ujemne to ograniczenie na bok
                fVelocity = i ? -SwitchExtension->fVelocity : -1;
            }
            else {
                fVelocity = SwitchExtension->fVelocity;
            }
            if (SwitchExtension->pOwner ? SwitchExtension->pOwner->RaTrackAnimAdd(this) :
                                          true) // jeśli nie dodane do animacji
            { // nie ma się co bawić
                SwitchExtension->fOffset = SwitchExtension->fDesiredOffset;
                // przeliczenie położenia iglic; czy zadziała na niewyświetlanym sektorze w VBO?
                RaAnimate();
            }
            return true;
        }
        else if (eType == tt_Table)
        { // blokowanie (0, szukanie torów) lub odblokowanie (1, rozłączenie) obrotnicy
            if (i) // NOTE: this condition seems opposite to intention/comment? TODO: investigate this
            { // 0: rozłączenie sąsiednich torów od obrotnicy
                if (trPrev) // jeśli jest tor od Point1 obrotnicy
                    if (iPrevDirection) // 0:dołączony Point1, 1:dołączony Point2
                        trPrev->trNext = NULL; // rozłączamy od Point2
                    else
                        trPrev->trPrev = NULL; // rozłączamy od Point1
                if (trNext) // jeśli jest tor od Point2 obrotnicy
                    if (iNextDirection) // 0:dołączony Point1, 1:dołączony Point2
                        trNext->trNext = NULL; // rozłączamy od Point2
                    else
                        trNext->trPrev = NULL; // rozłączamy od Point1
                trNext = trPrev =
                    NULL; // na końcu rozłączamy obrotnicę (wkaźniki do sąsiadów już niepotrzebne)
                fVelocity = 0.0; // AI, nie ruszaj się!
                if (SwitchExtension->pOwner)
                    SwitchExtension->pOwner->RaTrackAnimAdd(this); // dodanie do listy animacyjnej
                // TODO: unregister path ends in the owner cell
            }
            else
            { // 1: ustalenie finalnego położenia (gdy nie było animacji)
                RaAnimate(); // ostatni etap animowania
                // zablokowanie pozycji i połączenie do sąsiednich torów
                // TODO: register new position of the path endpoints with the region
                simulation::Region->TrackJoin( this );
                if (trNext || trPrev)
                {
                    fVelocity = 6.0; // jazda dozwolona
                    if( ( trPrev )
                     && ( trPrev->fVelocity == 0.0 ) ) {
                        // ustawienie 0 da możliwość zatrzymania AI na obrotnicy
                        trPrev->VelocitySet( 6.0 ); // odblokowanie dołączonego toru do jazdy
                    }
                    if( ( trNext )
                     && ( trNext->fVelocity == 0.0 ) ) {
                        trNext->VelocitySet( 6.0 );
                    }
                    if( SwitchExtension->evPlus ) { // w starych sceneriach może nie być
                        // potwierdzenie wykonania (np. odpala WZ)
                        simulation::Events.AddToQuery( SwitchExtension->evPlus, nullptr );
                    }
                }
            }
            SwitchExtension->CurrentIndex = i; // zapamiętanie stanu zablokowania
            return true;
        }
        else if (eType == tt_Cross)
        { // to jest przydatne tylko do łączenia odcinków
            i &= 1;
            SwitchExtension->CurrentIndex = i;
            Segment = SwitchExtension->Segments[i]; // wybranie aktywnej drogi - potrzebne to?
            trNext = SwitchExtension->pNexts[i]; // przełączenie końców
            trPrev = SwitchExtension->pPrevs[i];
            iNextDirection = SwitchExtension->iNextDirection[i];
            iPrevDirection = SwitchExtension->iPrevDirection[i];
            return true;
        }
    if (iCategoryFlag == 1)
        iDamageFlag = (iDamageFlag & 127) + 128 * (i & 1); // przełączanie wykolejenia
    else
        Error("Cannot switch normal track");
    return false;
};

bool TTrack::SwitchForced(int i, TDynamicObject *o)
{ // rozprucie rozjazdu
    if (SwitchExtension)
        if (eType == tt_Switch)
        { //
            if (i != SwitchExtension->CurrentIndex)
            {
                switch (i)
                {
                case 0:
                    if (SwitchExtension->evPlus)
                        simulation::Events.AddToQuery(SwitchExtension->evPlus, o); // dodanie do kolejki
                    break;
                case 1:
                    if (SwitchExtension->evMinus)
                        simulation::Events.AddToQuery(SwitchExtension->evMinus, o); // dodanie do kolejki
                    break;
                }
                Switch(i); // jeśli się tu nie przełączy, to każdy pojazd powtórzy event rozrprucia
            }
        }
        else if (eType == tt_Cross)
        { // ustawienie wskaźnika na wskazany segment
            Segment = SwitchExtension->Segments[i];
        }
    return true;
};

int TTrack::CrossSegment(int from, int into)
{ // ustawienie wskaźnika na segement w pożądanym kierunku (into) od strony (from)
    // zwraca kod segmentu, z kierunkiem jazdy jako znakiem ±
    int i = 0;
    switch (into)
    {
    case 0: // stop
//        WriteLog( "Stopping in P" + to_string( from + 1 ) + " on " + name() );
        break;
    case 1: // left
//        WriteLog( "Turning left from P" + to_string( from + 1 ) + " on " + name() );
        i = (SwitchExtension->iRoads == 4) ? iLewo4[from] : iLewo3[from];
        break;
    case 2: // right
//        WriteLog( "Turning right from P" + to_string( from + 1 ) + " on " + name() );
        i = (SwitchExtension->iRoads == 4) ? iPrawo4[from] : iPrawo3[from];
        break;
    case 3: // stright
//        WriteLog( "Going straight from P" + to_string( from + 1 ) + " on " + name() );
        i = (SwitchExtension->iRoads == 4) ? iProsto4[from] : iProsto3[from];
        break;
    }
    if (i)
    {
        Segment = SwitchExtension->Segments[abs(i) - 1];
        // WriteLog("Selected segment: "+AnsiString(abs(i)-1));
    }
    return i;
};

void TTrack::RaAnimListAdd(TTrack *t)
{ // dodanie toru do listy animacyjnej
    if ((t != nullptr) && (SwitchExtension != nullptr))
    {
        if (t == this)
            return; // siebie nie dodajemy drugi raz do listy
        if (!t->SwitchExtension)
            return; // nie podlega animacji
        if (SwitchExtension->pNextAnim)
        {
            if (SwitchExtension->pNextAnim == t)
                return; // gdy już taki jest
            else
                SwitchExtension->pNextAnim->RaAnimListAdd(t);
        }
        else
        {
            SwitchExtension->pNextAnim = t;
            t->SwitchExtension->pNextAnim = NULL; // nowo dodawany nie może mieć ogona
        }
    }
};

TTrack * TTrack::RaAnimate()
{ // wykonanie rekurencyjne animacji, wywoływane przed wyświetleniem sektora
    // zwraca wskaźnik toru wymagającego dalszej animacji
    if( SwitchExtension->pNextAnim )
        SwitchExtension->pNextAnim = SwitchExtension->pNextAnim->RaAnimate();
    bool m = true; // animacja trwa
    if (eType == tt_Switch) // dla zwrotnicy tylko szyny
    {
        auto const v = SwitchExtension->fDesiredOffset - SwitchExtension->fOffset; // kierunek
        SwitchExtension->fOffset += sign(v) * Timer::GetDeltaTime() * SwitchExtension->fOffsetSpeed;
        // Ra: trzeba dać to do klasy...
        SwitchExtension->fOffset1 = SwitchExtension->fOffset;
        SwitchExtension->fOffset2 = SwitchExtension->fOffset;
        if (SwitchExtension->fOffset1 > fMaxOffset)
            SwitchExtension->fOffset1 = fMaxOffset; // ograniczenie animacji zewnętrznej iglicy
        if (SwitchExtension->fOffset2 < 0.f)
            SwitchExtension->fOffset2 = 0.f; // ograniczenie animacji wewnętrznej iglicy
        if (v < 0.f)
        { // jak na pierwszy z torów
            if (SwitchExtension->fOffset <= SwitchExtension->fDesiredOffset)
            {
                SwitchExtension->fOffset = SwitchExtension->fDesiredOffset;
                m = false; // koniec animacji
            }
        }
        else
        { // jak na drugi z torów
            if (SwitchExtension->fOffset >= SwitchExtension->fDesiredOffset)
            {
                SwitchExtension->fOffset = SwitchExtension->fDesiredOffset;
                m = false; // koniec animacji
            }
        }
        // skip the geometry update if no geometry for this track was generated yet
        if( ( ( m_material1 != 0 )
           || ( m_material2 != 0 ) )
         && ( ( false == Geometry1.empty() )
           || ( false == Geometry2.empty() ) ) ) {
            // iglice liczone tylko dla zwrotnic
            gfx::vertex_array rpts1, rpts2;
            create_track_rail_profile( rpts1, rpts2 );
            gfx::vertex_array rpts3, rpts4;
            create_track_blade_profile( rpts3, rpts4 );
            gfx::vertex_array vertices;
            auto const bladelength { static_cast<int>( std::ceil( SwitchExtension->Segments[ 0 ]->RaSegCount() * 0.65 ) ) };
            auto const nnumPts { track_rail_profile( m_profile1.second ).size() / 2 };
            if (SwitchExtension->RightSwitch)
            { // nowa wersja z SPKS, ale odwrotnie lewa/prawa
                if( m_material1 ) {
                    auto const texturelength { texture_length( m_material1 ) };
                    // left blade
                    // composed from two parts: transition from blade to regular rail, and regular rail
                    SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts3, true, texturelength, 1.0, 0, bladelength / 2, { SwitchExtension->fOffset2, SwitchExtension->fOffset2 / 2 } );
                    SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, bladelength / 2, bladelength, { SwitchExtension->fOffset2 / 2, 0.f } );
                    GfxRenderer.Replace( vertices, Geometry1[ 0 ], GL_TRIANGLE_STRIP );
                    vertices.clear();
                }
                if( m_material2 ) {
                    auto const texturelength { texture_length( m_material2 ) };
                    // right blade
                    // composed from two parts: transition from blade to regular rail, and regular rail
                    SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts4, true, texturelength, 1.0, 0, bladelength / 2, { -fMaxOffset + SwitchExtension->fOffset1, ( -fMaxOffset + SwitchExtension->fOffset1 ) / 2 } );
                    SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, bladelength / 2, bladelength, { ( -fMaxOffset + SwitchExtension->fOffset1 ) / 2, 0.f } );
                    GfxRenderer.Replace( vertices, Geometry2[ 0 ], GL_TRIANGLE_STRIP );
                    vertices.clear();
                }
            }
            else { // lewa działa lepiej niż prawa
                if( m_material1 ) {
                    auto const texturelength { texture_length( m_material1 ) };
                    // right blade
                    // composed from two parts: transition from blade to regular rail, and regular rail
                    SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts4, true, texturelength, 1.0, 0, bladelength / 2, { -SwitchExtension->fOffset2, -SwitchExtension->fOffset2 / 2 } );
                    SwitchExtension->Segments[ 0 ]->RenderLoft( vertices, m_origin, rpts2, false, texturelength, 1.0, bladelength / 2, bladelength, { -SwitchExtension->fOffset2 / 2, 0.f } );
                    GfxRenderer.Replace( vertices, Geometry1[ 0 ], GL_TRIANGLE_STRIP );
                    vertices.clear();
                }
                if( m_material2 ) {
                    auto const texturelength { texture_length( m_material2 ) };
                    // left blade
                    // composed from two parts: transition from blade to regular rail, and regular rail
                    SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts3, true, texturelength, 1.0, 0, bladelength / 2, { fMaxOffset - SwitchExtension->fOffset1, ( fMaxOffset - SwitchExtension->fOffset1 ) / 2 } );
                    SwitchExtension->Segments[ 1 ]->RenderLoft( vertices, m_origin, rpts1, false, texturelength, 1.0, bladelength / 2, bladelength, { ( fMaxOffset - SwitchExtension->fOffset1 ) / 2, 0.f } );
                    GfxRenderer.Replace( vertices, Geometry2[ 0 ], GL_TRIANGLE_STRIP );
                    vertices.clear();
                }
            }
        }
    }
    else if (eType == tt_Table) {
        // dla obrotnicy - szyny i podsypka
        if (SwitchExtension->pModel &&
            SwitchExtension->CurrentIndex) // 0=zablokowana się nie animuje
        { // trzeba każdorazowo porównywać z kątem modelu
            // //pobranie kąta z modelu
            TAnimContainer *ac = (
                SwitchExtension->pModel ?
                    SwitchExtension->pModel->GetContainer() : // pobranie głównego submodelu
                    nullptr );
            if( ac ) {
                if( ( ac->AngleGet() != SwitchExtension->fOffset )
                || !( ac->TransGet() == SwitchExtension->vTrans ) ) { // czy przemieściło się od ostatniego sprawdzania

                    double hlen = 0.5 * SwitchExtension->Segments[ 0 ]->GetLength(); // połowa długości
                    SwitchExtension->fOffset =
                        SwitchExtension->pModel->Angles().y // take into account orientation of the model
                        + ac->AngleGet(); // pobranie kąta z submodelu
                    double
                        sina = -hlen * std::sin( glm::radians( SwitchExtension->fOffset ) ),
                        cosa = -hlen * std::cos( glm::radians( SwitchExtension->fOffset ) );
                    SwitchExtension->vTrans = ac->TransGet();
                    auto middle = location() + SwitchExtension->vTrans; // SwitchExtension->Segments[0]->FastGetPoint(0.5);
                    Segment->Init(
                        middle + Math3D::vector3( sina, 0.0, cosa ),
                        middle - Math3D::vector3( sina, 0.0, cosa ),
                        10.0 ); // nowy odcinek
                    for( auto dynamic : Dynamics ) {
                        // minimalny ruch, aby przeliczyć pozycję
                        dynamic->Move( 0.000001 );
                    }
                    // NOTE: passing empty handle is a bit of a hack here. could be refactored into something more elegant
                    create_geometry( {} );
                } // animacja trwa nadal
            }
        }
        else
            m = false; // koniec animacji albo w ogóle nie połączone z modelem
    }
    return m ? this : SwitchExtension->pNextAnim; // zwraca obiekt do dalszej animacji
};
//---------------------------------------------------------------------------
void TTrack::RadioStop()
{ // przekazanie pojazdom rozkazu zatrzymania
    for( auto dynamic : Dynamics ) {
        dynamic->RadioStop();
    }
};

double TTrack::WidthTotal()
{ // szerokość z poboczem
    if (iCategoryFlag & 2) // jesli droga
        if (fTexHeight1 >= 0.0) // i ma boki zagięte w dół (chodnik jest w górę)
            return 2.0 * fabs(fTexWidth) +
                   0.5 * fabs(fTrackWidth + fTrackWidth2); // dodajemy pobocze
    return 0.5 * fabs(fTrackWidth + fTrackWidth2); // a tak tylko zwykła średnia szerokość
};

bool TTrack::IsGroupable()
{ // czy wyświetlanie toru może być zgrupwane z innymi
    if ((eType == tt_Switch) || (eType == tt_Table))
        return false; // tory ruchome nie są grupowane
    if ((eEnvironment == e_canyon) || (eEnvironment == e_tunnel))
        return false; // tory ze zmianą światła
    return true;
};

bool Equal( Math3D::vector3 v1, Math3D::vector3 *v2)
{ // sprawdzenie odległości punktów
    // Ra: powinno być do 100cm wzdłuż toru i ze 2cm w poprzek (na prostej może nie być długiego
    // kawałka)
    // Ra: z automatycznie dodawanym stukiem, jeśli dziura jest większa niż 2mm.
    if (fabs(v1.x - v2->x) > 0.02)
        return false; // sześcian zamiast kuli
    if (fabs(v1.z - v2->z) > 0.02)
        return false;
    if (fabs(v1.y - v2->y) > 0.02)
        return false;
    return true;
    // return (SquareMagnitude(v1-*v2)<0.00012); //0.011^2=0.00012
};

int TTrack::TestPoint( Math3D::vector3 *Point)
{ // sprawdzanie, czy tory można połączyć
    switch (eType)
    {
    case tt_Normal: // zwykły odcinek
        if (trPrev == NULL)
            if (Equal(Segment->FastGetPoint_0(), Point))
                return 0;
        if (trNext == NULL)
            if (Equal(Segment->FastGetPoint_1(), Point))
                return 1;
        break;
    case tt_Switch: // zwrotnica
    {
        int state = GetSwitchState(); // po co?
        // Ra: TODO: jak się zmieni na bezpośrednie odwołania do segmentow zwrotnicy,
        // to się wykoleja, ponieważ trNext zależy od przełożenia
        Switch(0);
        if (trPrev == NULL)
            // if (Equal(SwitchExtension->Segments[0]->FastGetPoint_0(),Point))
            if (Equal(Segment->FastGetPoint_0(), Point))
            {
                Switch(state);
                return 2;
            }
        if (trNext == NULL)
            // if (Equal(SwitchExtension->Segments[0]->FastGetPoint_1(),Point))
            if (Equal(Segment->FastGetPoint_1(), Point))
            {
                Switch(state);
                return 3;
            }
        Switch(1); // można by się pozbyć tego przełączania
        if (trPrev == NULL) // Ra: z tym chyba nie potrzeba łączyć
            // if (Equal(SwitchExtension->Segments[1]->FastGetPoint_0(),Point))
            if (Equal(Segment->FastGetPoint_0(), Point))
            {
                Switch(state); // Switch(0);
                return 4;
            }
        if (trNext == NULL) // TODO: to zależy od przełożenia zwrotnicy
            // if (Equal(SwitchExtension->Segments[1]->FastGetPoint_1(),Point))
            if (Equal(Segment->FastGetPoint_1(), Point))
            {
                Switch(state); // Switch(0);
                return 5;
            }
        Switch(state);
    }
    break;
    case tt_Cross: // skrzyżowanie dróg
        // if (trPrev==NULL)
        if (Equal(SwitchExtension->Segments[0]->FastGetPoint_0(), Point))
            return 2;
        // if (trNext==NULL)
        if (Equal(SwitchExtension->Segments[0]->FastGetPoint_1(), Point))
            return 3;
        // if (trPrev==NULL)
        if (Equal(SwitchExtension->Segments[1]->FastGetPoint_0(), Point))
            return 4;
        // if (trNext==NULL)
        if (Equal(SwitchExtension->Segments[1]->FastGetPoint_1(), Point))
            return 5;
        break;
    }
    return -1;
};

// retrieves list of the track's end points
std::vector<glm::dvec3>
TTrack::endpoints() const {

    switch( eType ) {
        case tt_Normal:
        case tt_Table: {
            return {
                glm::dvec3{ Segment->FastGetPoint_0() },
                glm::dvec3{ Segment->FastGetPoint_1() } };
        }
        case tt_Switch:
        case tt_Cross: {
            return {
                glm::dvec3{ SwitchExtension->Segments[ 0 ]->FastGetPoint_0() },
                glm::dvec3{ SwitchExtension->Segments[ 0 ]->FastGetPoint_1() },
                glm::dvec3{ SwitchExtension->Segments[ 1 ]->FastGetPoint_0() },
                glm::dvec3{ SwitchExtension->Segments[ 1 ]->FastGetPoint_1() } };
        }
        default: {
            return{};
        }
    }
}

// radius() subclass details, calculates node's bounding radius
float
TTrack::radius_() {

    auto const points { endpoints() };
    auto radius { 0.f };
    for( auto &point : points ) {
        radius = std::max(
            radius,
            static_cast<float>( glm::length( m_area.center - point ) ) ); // extra margin to prevent driven vehicle from flicking
    }
    return radius;
}

// serialize() subclass details, sends content of the subclass to provided stream
void
TTrack::serialize_( std::ostream &Output ) const {

    // TODO: implement
}
// deserialize() subclass details, restores content of the subclass from provided stream
void
TTrack::deserialize_( std::istream &Input ) {

    // TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void
TTrack::export_as_text_( std::ostream &Output ) const {
    // header
    Output << "track ";
    // type
    Output << (
            eType == tt_Normal ? (
                iCategoryFlag == 1 ? "normal" :
                iCategoryFlag == 2 ? "road" :
                iCategoryFlag == 4 ? "river" :
                "none" ) :
            eType == tt_Switch ? "switch" :
            eType == tt_Cross ? "cross" :
            eType == tt_Table ? "turn" :
            eType == tt_Tributary ? "tributary" :
            "none" )
            << ' ';
    // basic attributes
    Output
        << Length() << ' '
        << fTrackWidth << ' '
        << fFriction << ' '
        << fSoundDistance << ' '
        << iQualityFlag << ' '
        << iDamageFlag << ' ';
    // environment
    Output << (
            eEnvironment == e_flat ? "flat" :
            eEnvironment == e_bridge ? "bridge" :
            eEnvironment == e_tunnel ? "tunnel" :
            eEnvironment == e_bank ? "bank" :
            eEnvironment == e_canyon ? "canyon" :
            eEnvironment == e_mountains ? "mountains" :
            "none" )
           << ' ';
    // visibility
    // NOTE: 'invis' would be less wrong than 'unvis', but potentially incompatible with old 3rd party tools
    Output << ( m_visible ? "vis" : "unvis" ) << ' ';
    if( m_visible ) {
        // texture parameters are supplied only if the path is set as visible
        auto texturefile { (
            m_material1 != null_handle ?
                GfxRenderer.Material( m_material1 ).name :
                "none" ) };
        if( texturefile.find( szTexturePath ) == 0 ) {
            // don't include 'textures/' in the path
            texturefile.erase( 0, std::string{ szTexturePath }.size() );
        }
        Output
            << texturefile << ' '
            << fTexLength << ' ';

        texturefile = (
            m_material2 != null_handle ?
                GfxRenderer.Material( m_material2 ).name :
                "none" );
        if( texturefile.find( szTexturePath ) == 0 ) {
            // don't include 'textures/' in the path
            texturefile.erase( 0, std::string{ szTexturePath }.size() );
        }
        Output << texturefile << ' ';

        Output
            << (fTexHeight1 - fTexHeightOffset ) * ( ( iCategoryFlag & 4 ) ? -1 : 1 ) << ' '
            << fTexWidth << ' '
            << fTexSlope << ' ';
    }
    // path data
    for( auto const &path : m_paths ) {
        Output
            << path.points[ segment_data::point::start ].x << ' '
            << path.points[ segment_data::point::start ].y << ' '
            << path.points[ segment_data::point::start ].z << ' '
            << path.rolls[ 0 ] << ' '

            << path.points[ segment_data::point::control1 ].x << ' '
            << path.points[ segment_data::point::control1 ].y << ' '
            << path.points[ segment_data::point::control1 ].z << ' '

            << path.points[ segment_data::point::control2 ].x << ' '
            << path.points[ segment_data::point::control2 ].y << ' '
            << path.points[ segment_data::point::control2 ].z << ' '

            << path.points[ segment_data::point::end ].x << ' '
            << path.points[ segment_data::point::end ].y << ' '
            << path.points[ segment_data::point::end ].z << ' '
            << path.rolls[ 1 ] << ' '

            << path.radius << ' ';
    }
    // optional attributes
    std::vector< std::pair< std::string, event_sequence const * > > const eventsequences {
        { "event0", &m_events0 }, { "eventall0", &m_events0all },
        { "event1", &m_events1 }, { "eventall1", &m_events1all },
        { "event2", &m_events2 }, { "eventall2", &m_events2all } };

    for( auto &eventsequence : eventsequences ) {
        for( auto &event : *( eventsequence.second ) ) {
            // NOTE: actual event name can be potentially different from its cached name, if it was renamed in the editor
            // therefore on export we pull the name from the event itself, if the binding isn't broken
            Output
                << eventsequence.first << ' '
                << ( event.second != nullptr ?
                        event.second->m_name :
                        event.first )
                << ' ';
        }
    }
    if( ( SwitchExtension )
     && ( SwitchExtension->fVelocity != -1.0 ) ) {
        Output << "velocity " << SwitchExtension->fVelocity << ' ';
    }
    else {
        if( fVelocity != -1.0 ) {
            Output << "velocity " << fVelocity << ' ';
        }
    }
    if( pIsolated ) {
        Output << "isolated " << pIsolated->asName << ' ';
    }
    if( fOverhead != -1.0 ) {
        Output << "overhead " << fOverhead << ' ';
    }
    if( fVerticalRadius != 0.f ) {
        Output << "vradius " << fVerticalRadius << ' ';
    }
    if( ( eType == tt_Switch )
     && ( SwitchExtension->m_material3 != null_handle ) ) {
        auto texturefile { GfxRenderer.Material( m_material2 ).name };
        if( texturefile.find( szTexturePath ) == 0 ) {
            // don't include 'textures/' in the path
            texturefile.erase( 0, std::string{ szTexturePath }.size() );
        }
        Output << "trackbed " << texturefile << ' ';
    }
    // footer
    Output
        << "endtrack"
        << "\n";
}

// locates specified profile in the profile database, potentially loading it from a file
// returns: pair <profile name, profile handle>
std::pair<std::string, int>
TTrack::fetch_track_rail_profile( std::string const &Profile ) {

    auto const railprofilepath { std::string( szModelPath ) + "tory/railprofile_" };
    auto const railkeyprefix { std::string( "rail_" ) };

    if( m_profiles.empty() ) {
        // ensure the default profile is always first in the database
        fetch_default_profiles();
    }
    // try to locate specified rail profile...
    auto const lookup { m_profilesmap.find( railkeyprefix + Profile ) };
    if( lookup != m_profilesmap.end() ) {
        // ...if it works, we're done...
        return { Profile, lookup->second };
    }
    // ... and if it fails try to add the profile to the database from a data file
    auto profilehandle { 0 }; // fallback link to default profile if loading it fails
    auto profiledata { deserialize_profile( railprofilepath + Profile ) };
    if( false == profiledata.empty() ) {
        // if we get the profile data add it to the database and calculate a link to it
        profilehandle = static_cast<int>( m_profiles.size() );
        m_profiles.emplace_back( profiledata );
    }
    m_profilesmap.emplace( railkeyprefix + Profile, profilehandle );
    return { Profile, profilehandle };
}

void
TTrack::fetch_default_profiles() {

    if( false == m_profiles.empty() ) { return; }

    auto const railprofilepath { std::string( szModelPath ) + "tory/railprofile_" };
    auto const railkeyprefix { std::string( "rail_" ) };

    m_profiles.emplace_back( deserialize_profile( railprofilepath + "default" ) );
        if( m_profiles.back().empty() ) {
            // fallback to prevent utter start failure, supply legacy track profile
            m_profiles.back() = {
// szyna - vextor6(x,y,mapowanie tekstury,xn,yn,zn)
// tę wersję opracował Tolein (bez pochylenia)
    {{ 0.111f, -0.180f, 0.f}, { 1.000f,  0.000f, 0.f}, {0.00f, 0.f}},
    {{ 0.046f, -0.150f, 0.f}, { 0.707f,  0.707f, 0.f}, {0.15f, 0.f}},
    {{ 0.044f, -0.050f, 0.f}, { 0.707f, -0.707f, 0.f}, {0.25f, 0.f}},
    {{ 0.073f, -0.038f, 0.f}, { 0.707f, -0.707f, 0.f}, {0.35f, 0.f}},
    {{ 0.072f, -0.010f, 0.f}, { 0.707f,  0.707f, 0.f}, {0.40f, 0.f}},
    {{ 0.052f, -0.000f, 0.f}, { 0.000f,  1.000f, 0.f}, {0.45f, 0.f}},
    {{ 0.020f, -0.000f, 0.f}, { 0.000f,  1.000f, 0.f}, {0.55f, 0.f}},
    {{ 0.000f, -0.010f, 0.f}, {-0.707f,  0.707f, 0.f}, {0.60f, 0.f}},
    {{-0.001f, -0.038f, 0.f}, {-0.707f, -0.707f, 0.f}, {0.65f, 0.f}},
    {{ 0.028f, -0.050f, 0.f}, {-0.707f, -0.707f, 0.f}, {0.75f, 0.f}},
    {{ 0.026f, -0.150f, 0.f}, {-0.707f,  0.707f, 0.f}, {0.85f, 0.f}},
    {{-0.039f, -0.180f, 0.f}, {-1.000f,  0.000f, 0.f}, {1.00f, 0.f}},
// iglica - vextor3(x,y,mapowanie tekstury)
// 1 mm więcej, żeby nie nachodziły tekstury?
    {{ 0.010f, -0.180f, 0.f}, { 1.000f, 0.000f, 0.f}, {0.00f, 0.f}},
    {{ 0.010f, -0.155f, 0.f}, { 1.000f, 0.000f, 0.f}, {0.15f, 0.f}},
    {{ 0.010f, -0.070f, 0.f}, { 1.000f, 0.000f, 0.f}, {0.25f, 0.f}},
    {{ 0.010f, -0.040f, 0.f}, { 1.000f, 0.000f, 0.f}, {0.35f, 0.f}},
    {{ 0.010f, -0.010f, 0.f}, { 1.000f, 0.000f, 0.f}, {0.40f, 0.f}},
    {{ 0.010f, -0.000f, 0.f}, { 0.707f, 0.707f, 0.f}, {0.45f, 0.f}},
    {{ 0.000f, -0.000f, 0.f}, { 0.707f, 0.707f, 0.f}, {0.55f, 0.f}},
    {{ 0.000f, -0.010f, 0.f}, {-1.000f, 0.000f, 0.f}, {0.60f, 0.f}},
    {{ 0.000f, -0.040f, 0.f}, {-1.000f, 0.000f, 0.f}, {0.65f, 0.f}},
    {{ 0.000f, -0.070f, 0.f}, {-1.000f, 0.000f, 0.f}, {0.75f, 0.f}},
    {{ 0.000f, -0.155f, 0.f}, {-0.707f, 0.707f, 0.f}, {0.85f, 0.f}},
    {{-0.040f, -0.180f, 0.f}, {-1.000f, 0.000f, 0.f}, {1.00f, 0.f}} };
        }
    m_profilesmap.emplace( railkeyprefix + "default", 0 );
}

gfx::vertex_array
TTrack::deserialize_profile( std::string const &Profile ) {

    gfx::vertex_array profiledata;

    cParser input { Profile + ".txt", cParser::buffer_FILE };

    while( input.getTokens( 5, true, "\n\r\t ;,{}" ) ) {
        gfx::basic_vertex vertex;
        input
            >> vertex.position.x
            >> vertex.position.y
            >> vertex.normal.x
            >> vertex.normal.y
            >> vertex.texture.s;
        profiledata.emplace_back( vertex );
    }

    return profiledata;
}

gfx::vertex_array const &
TTrack::track_rail_profile( int const Profile ) {

    return m_profiles[ Profile ];
}

float
TTrack::texture_length( material_handle const Material ) {

    if( Material == null_handle ) {
        return fTexLength;
    }
    auto const texturelength { GfxRenderer.Material( Material ).size.y };
    return (
        texturelength < 0.f ?
            fTexLength :
            texturelength );
}

void TTrack::MovedUp1(float const dh)
{ // poprawienie przechyłki wymaga wydłużenia podsypki
    fTexHeight1 += dh;
    fTexHeightOffset += dh;
};

// ustawienie prędkości z ograniczeniem do pierwotnej wartości (zapisanej w scenerii)
void TTrack::VelocitySet(float v) {
    // TBD, TODO: add a variable to preserve potential speed limit set by the track configuration on basic track pieces
    if( ( SwitchExtension )
     && ( SwitchExtension->fVelocity != -1 ) ) {
        // zwrotnica może mieć odgórne ograniczenie, nieprzeskakiwalne eventem
        fVelocity =
            min_speed<float>(
                v,
                ( SwitchExtension->fVelocity > 0 ?
                    SwitchExtension->fVelocity : // positive limit applies to both switch tracks
                    ( SwitchExtension->CurrentIndex == 0 ?
                        -1 : // negative limit applies only to the diverging track
                        -SwitchExtension->fVelocity ) ) );
    }
    else {
        fVelocity = v; // nie ma ograniczenia
    }
};

double TTrack::VelocityGet()
{ // pobranie dozwolonej prędkości podczas skanowania
    return ((iDamageFlag & 128) ? 0.0 : fVelocity); // tor uszkodzony = prędkość zerowa
};

void TTrack::ConnectionsLog()
{ // wypisanie informacji o połączeniach
    int i;
    WriteLog("--> tt_Cross named " + m_name);
    if (eType == tt_Cross)
        for (i = 0; i < 2; ++i)
        {
            if (SwitchExtension->pPrevs[i])
                WriteLog("Point " + std::to_string(i + i + 1) + " -> track " +
                         SwitchExtension->pPrevs[i]->m_name + ":" +
                         std::to_string(int(SwitchExtension->iPrevDirection[i])));
            if (SwitchExtension->pNexts[i])
                WriteLog("Point " + std::to_string(i + i + 2) + " -> track " +
                         SwitchExtension->pNexts[i]->m_name + ":" +
                         std::to_string(int(SwitchExtension->iNextDirection[i])));
        }
};

bool
TTrack::DoubleSlip() const {

    // crude way to discern part of double slip switch:
    // a switch with name ending in _a or _b or _c or _d
    return (
        ( iCategoryFlag == 1 )
     && ( eType == tt_Switch )
     && ( m_name.size() > 2 )
     && ( m_name.back() >= 'a' )
     && ( m_name.back() <= 'd' )
     && ( ( m_name[ m_name.size() - 2 ] == '_' )
       || ( m_name.rfind( '_' ) != std::string::npos ) ) );
}


TTrack * TTrack::Connected(int s, double &d) const
{ // zwraca wskaźnik na sąsiedni tor, w kierunku określonym znakiem (s), odwraca (d) w razie
    // niezgodności kierunku torów
    TTrack *t; // nie zmieniamy kierunku (d), jeśli nie ma toru dalej
    if (eType != tt_Cross)
    { // jeszcze trzeba sprawdzić zgodność
        t = (s > 0) ? trNext : trPrev;
        if (t) // o ile jest na co przejść, zmieniamy znak kierunku na nowym torze
            if (t->eType == tt_Cross)
            { // jeśli wjazd na skrzyżowanie, trzeba ustalić segment, bo od tego zależy zmiana
                // kierunku (d)
                // if (r) //gdy nie podano (r), to nie zmieniać (d)
                // if (s*t->CrossSegment(((s>0)?iNextDirection:iPrevDirection),r)<0)
                //  d=-d;
            }
            else
            {
                if ((s > 0) ? iNextDirection : !iPrevDirection)
                    d = -d; // następuje zmiana kierunku wózka albo kierunku skanowania
                // s=((s>0)?iNextDirection:iPrevDirection)?-1:1; //kierunek toru po zmianie
            }
        return (t); // zwrotnica ma odpowiednio ustawione (trNext)
    }
    switch ((SwitchExtension->iRoads == 4) ? iEnds4[s + 6] :
                                             iEnds3[s + 6]) // numer końca 0..3, -1 to błąd
    { // zjazd ze skrzyżowania
    case 0:
        if (SwitchExtension->pPrevs[0])
            if ((s > 0) == SwitchExtension->iPrevDirection[0])
                d = -d;
        return SwitchExtension->pPrevs[0];
    case 1:
        if (SwitchExtension->pNexts[0])
            if ((s > 0) == SwitchExtension->iNextDirection[0])
                d = -d;
        return SwitchExtension->pNexts[0];
    case 2:
        if (SwitchExtension->pPrevs[1])
            if ((s > 0) == SwitchExtension->iPrevDirection[1])
                d = -d;
        return SwitchExtension->pPrevs[1];
    case 3:
        if (SwitchExtension->pNexts[1])
            if ((s > 0) == SwitchExtension->iNextDirection[1])
                d = -d;
        return SwitchExtension->pNexts[1];
    }
    return NULL;
};

// creates rail profile data for current track
void
TTrack::create_track_rail_profile( gfx::vertex_array &Right, gfx::vertex_array &Left ) {

    auto const fHTW { 0.5f * std::abs( fTrackWidth ) };

    float
        roll1{ 0.f },
        roll2{ 0.f };

    if( Segment ) {
        Segment->GetRolls( roll1, roll2 );
    }

    float const
        sin1 { std::sin( roll1 ) },
        cos1 { std::cos( roll1 ) },
        sin2 { std::sin( roll2 ) },
        cos2 { std::cos( roll2 ) };

    auto const &railprofile { track_rail_profile( m_profile1.second ) };
    // NOTE: rail profile defines both regular rail and switch blade profiles, so we halve total point count
    auto const pointcount { railprofile.size() / 2 };
    Right.resize( pointcount * ( iTrapezoid == 0 ? 1 : 2 ) );
    Left.resize(  pointcount * ( iTrapezoid == 0 ? 1 : 2 ) );

    auto const *szyna { railprofile.data() };

    for( int i = 0; i < pointcount; ++i ) {

        Right[ i ] = {
            // position
            {( fHTW + szyna[ i ].position.x ) * cos1 + szyna[ i ].position.y * sin1,
            -( fHTW + szyna[ i ].position.x ) * sin1 + szyna[ i ].position.y * cos1,
                      szyna[ i ].position.z},
            // normal
            { szyna[ i ].normal.x * cos1 + szyna[ i ].normal.y * sin1,
             -szyna[ i ].normal.x * sin1 + szyna[ i ].normal.y * cos1,
              szyna[ i ].normal.z },
            // texture
            { szyna[ i ].texture.s,
              szyna[ i ].texture.t } };

        Left[ pointcount - 1 - i ] = {
            // position
            {(-fHTW - szyna[ i ].position.x ) * cos1 + szyna[ i ].position.y * sin1,
            -(-fHTW - szyna[ i ].position.x ) * sin1 + szyna[ i ].position.y * cos1,
                      szyna[ i ].position.z},
            // normal
            {-szyna[ i ].normal.x * cos1 + szyna[ i ].normal.y * sin1,
              szyna[ i ].normal.x * sin1 + szyna[ i ].normal.y * cos1,
              szyna[ i ].normal.z },
            // texture
            { szyna[ i ].texture.s,
              szyna[ i ].texture.t } };

        if( iTrapezoid == 0 ) { continue; }

        // trapez albo przechyłki, to oddzielne punkty na końcu
        Right[ pointcount + i ] = {
            // position
            {( fHTW + szyna[ i ].position.x ) * cos2 + szyna[ i ].position.y * sin2,
            -( fHTW + szyna[ i ].position.x ) * sin2 + szyna[ i ].position.y * cos2,
                      szyna[ i ].position.z},
            // normal
            { szyna[ i ].normal.x * cos2 + szyna[ i ].normal.y * sin2,
             -szyna[ i ].normal.x * sin2 + szyna[ i ].normal.y * cos2,
              szyna[ i ].normal.z },
            // texture
            { szyna[ i ].texture.s,
              szyna[ i ].texture.t } };

        Left[ pointcount * 2 - 1 - i ] = {
            // position
            {(-fHTW - szyna[ i ].position.x ) * cos2 + szyna[ i ].position.y * sin2,
            -(-fHTW - szyna[ i ].position.x ) * sin2 + szyna[ i ].position.y * cos2,
                      szyna[ i ].position.z},
            // normal
            {-szyna[ i ].normal.x * cos2 + szyna[ i ].normal.y * sin2,
              szyna[ i ].normal.x * sin2 + szyna[ i ].normal.y * cos2,
              szyna[ i ].normal.z },
            // texture
            { szyna[ i ].texture.s,
              szyna[ i ].texture.t } };
    }
}

// creates switch blades profile data for current track
void
TTrack::create_track_blade_profile( gfx::vertex_array &Right, gfx::vertex_array &Left ) {

    auto const fHTW { 0.5f * std::abs( fTrackWidth ) };
    float const fHTW2 { (
        ( iTrapezoid & 2 ) != 0 ? // ten bit oznacza, że istnieje odpowiednie pNext
            0.5f * std::fabs( trNext->fTrackWidth ) : // połowa rozstawu/nawierzchni
            fHTW ) };

    float
        roll1 { 0.f },
        roll2 { 0.f };

    if( Segment ) {
        Segment->GetRolls( roll1, roll2 );
    }

    float const
        sin1 { std::sin( roll1 ) },
        cos1 { std::cos( roll1 ) },
        sin2 { std::sin( roll2 ) },
        cos2 { std::cos( roll2 ) };

    auto const &railprofile { track_rail_profile( m_profile1.second ) };
    // NOTE: rail profile defines both regular rail and switch blade profiles, so we halve total point count
    auto const pointcount { railprofile.size() / 2 };

    Right.resize( pointcount * 2 );
    Left.resize( pointcount * 2 );

    auto const *szyna { railprofile.data() };
    auto const *iglica { szyna + pointcount };

    glm::vec3 const flipxvalue { -1, 1, 1 };
    for( int i = 0; i < pointcount; ++i ) {

        Right[ i ] = {
            {+( fHTW + iglica[ i ].position.x ) * cos1 + iglica[ i ].position.y * sin1,
             -( fHTW + iglica[ i ].position.x ) * sin1 + iglica[ i ].position.y * cos1,
             0.f},
             {iglica[ i ].normal},
             {iglica[ i ].texture.s, 0.f} };
        Right[ i + pointcount ] = {
            {+( fHTW2 + szyna[ i ].position.x ) * cos2 + szyna[ i ].position.y * sin2,
             -( fHTW2 + szyna[ i ].position.x ) * sin2 + szyna[ i ].position.y * cos2,
             0.f},
             {szyna[ i ].normal},
             {szyna[ i ].texture.s, 0.f} };
        Left[ pointcount - 1 - i ] = {
            { ( -fHTW - iglica[ i ].position.x ) * cos1 + iglica[ i ].position.y * sin1,
             -( -fHTW - iglica[ i ].position.x ) * sin1 + iglica[ i ].position.y * cos1,
             0.f},
             {iglica[ i ].normal * flipxvalue},
             {iglica[ i ].texture.s, 0.f} };
        Left[ pointcount * 2 - 1 - i ] = {
            { ( -fHTW2 - szyna[ i ].position.x ) * cos2 + szyna[ i ].position.y * sin2,
             -( -fHTW2 - szyna[ i ].position.x ) * sin2 + szyna[ i ].position.y * cos2,
             0.f},
             {szyna[ i ].normal * flipxvalue},
             {szyna[ i ].texture.s, 0.f} };
    }
}

// creates trackbed profile data for current track
void
TTrack::create_track_bed_profile( gfx::vertex_array &Output, TTrack const *Previous, TTrack const *Next ) {
    // geometry parameters
    auto * profilesource = (
        eType != tt_Switch ? this :
        Previous && Previous->eType != tt_Switch ? Previous :
        Next && Next->eType != tt_Switch ? Next :
        this );

    auto const texheight1 { profilesource->fTexHeight1 };
    auto const texwidth { profilesource->fTexWidth };
    auto const texslope { profilesource->fTexSlope };

    auto const fHTW { 0.5f * std::abs( fTrackWidth ) };
    auto const side { std::abs( texwidth ) }; // szerokść podsypki na zewnątrz szyny albo pobocza
    auto const slop { std::abs( texslope ) }; // brzeg zewnętrzny
    auto const rozp { fHTW + side + slop }; // brzeg zewnętrzny

    auto hypot1 { std::hypot( slop, texheight1 ) }; // rozmiar pochylenia do liczenia normalnych
    if( hypot1 == 0.f )
        hypot1 = 1.f;

    glm::vec3 normal1 { texheight1 / hypot1, texslope / hypot1, 0.f }; // wektor normalny
    if( glm::length( normal1 ) == 0.f ) {
        // fix normal for vertical surfaces
        normal1 = glm::vec3 { 1.f, 0.f, 0.f };
    }

    glm::vec3 normal2;
    float fHTW2, side2, slop2, rozp2, fTexHeight2, hypot2;
    if( ( Next != nullptr )
     && ( Next->eType != tt_Switch )
     && ( ( iTrapezoid & 2 ) // ten bit oznacza, że istnieje odpowiednie pNext
       || ( eType == tt_Switch ) ) ) { 
        fHTW2 = 0.5f * std::abs(Next->fTrackWidth); // połowa rozstawu/nawierzchni
        side2 = std::abs(Next->fTexWidth);
        slop2 = std::abs(Next->fTexSlope); // nie jest używane później
        rozp2 = fHTW2 + side2 + slop2;
        fTexHeight2 = Next->fTexHeight1;
        hypot2 = std::hypot(slop2, fTexHeight2);
        if( hypot2 == 0.f )
            hypot2 = 1.f;
        normal2 = { fTexHeight2 / hypot2, Next->fTexSlope / hypot2, 0.f };
        if( glm::length( normal2 ) == 0.f ) {
            // fix normal for vertical surfaces
            normal2 = glm::vec3 { 1.f, 0.f, 0.f };
        }
    }
    else {
        // gdy nie ma następnego albo jest nieodpowiednim końcem podpięty
        fHTW2 = fHTW;
        side2 = side;
        slop2 = slop;
        rozp2 = rozp;
        fTexHeight2 = texheight1;
        hypot2 = hypot1;
        normal2 = normal1;
    }

    float
        roll1{ 0.f },
        roll2{ 0.f };

    if( Segment ) {
        Segment->GetRolls( roll1, roll2 );
    }

    float const
        sin1 { std::sin( roll1 ) },
        cos1 { std::cos( roll1 ) },
        sin2 { std::sin( roll2 ) },
        cos2 { std::cos( roll2 ) };

    // profile
    auto const transition { ( iTrapezoid != 0 ) || ( eType == tt_Switch ) };
    auto const pointcount { transition ? 10 : 5 };
    Output.resize( pointcount );
    // potentially retrieve texture length override from the assigned material
    auto const texturelength { texture_length( copy_adjacent_trackbed_material() ) };
    auto const railheight { std::abs( track_rail_profile( m_profile1.second ).front().position.y ) };
    if( texturelength == 4.f ) {
        // stare mapowanie z różną gęstością pikseli i oddzielnymi teksturami na każdy profil
        auto const normalx = std::cos( glm::radians( 75.f ) );
        auto const normaly = std::sin( glm::radians( 75.f ) );
        if( transition ) {
            // trapez albo przechyłki
            // ewentualnie poprawić mapowanie, żeby środek mapował się na 1.435/4.671 ((0.3464,0.6536)
            // bo się tekstury podsypki rozjeżdżają po zmianie proporcji profilu
            Output[ 0 ] = {
                {rozp, -texheight1 - railheight, 0.f},
                {normalx, normaly, 0.f},
                {0.00f, 0.f} }; // lewy brzeg
            Output[ 1 ] = {
                {( fHTW + side ) * cos1, -( fHTW + side ) * sin1 - railheight, 0.f},
                {normalx, normaly, 0.f},
                {0.33f, 0.f} }; // krawędź załamania
            Output[ 2 ] = {
                {0.f, -railheight + 0.01f, 0.f},
                {0.f, 1.f, 0.f},
                {0.5f, 0.f} }; // middle
            Output[ 3 ] = {
                {-Output[ 1 ].position.x, +( fHTW + side ) * sin1 - railheight, 0.f},
                {-normalx, normaly, 0.f},
                {0.67f, 0.f} }; // prawy brzeg początku symetrycznie
            Output[ 4 ] = {
                {-rozp, -texheight1 - railheight, 0.f},
                {-normalx, normaly, 0.f},
                {1.f, 0.f} }; // prawy skos
            // końcowy przekrój
            Output[ 5 ] = {
                {rozp2, -fTexHeight2 - railheight, 0.f},
                {normalx, normaly, 0.f},
                {0.00f, 0.f} }; // lewy brzeg
            Output[ 6 ] = {
                {( fHTW2 + side2 ) * cos2, -( fHTW2 + side2 ) * sin2 - railheight, 0.f},
                {normalx, normaly, 0.f},
                {0.33f, 0.f} }; // krawędź załamania
            Output[ 7 ] = {
                {0.f, -railheight + 0.01f, 0.f},
                {0.f, 1.f, 0.f},
                {0.5f, 0.f} }; // middle
            Output[ 8 ] = {
                {-Output[ 6 ].position.x, +( fHTW2 + side2 ) * sin2 - railheight, 0.f},
                {-normalx, normaly, 0.f},
                {0.67f, 0.f} }; // prawy brzeg początku symetrycznie
            Output[ 9 ] = {
                {-rozp2, -fTexHeight2 - railheight, 0.f},
                {-normalx, normaly, 0.f},
                {1.00f, 0.f} }; // prawy skos
        }
        else {
            Output[ 0 ] = {
                {rozp, -texheight1 - railheight, 0.f},
                {normalx, normaly, 0.f},
                {0.00f, 0.f} }; // lewy brzeg
            Output[ 1 ] = {
                {fHTW + side, -railheight, 0.f},
                {normalx, normaly, 0.f},
                {0.33f, 0.f} }; // krawędź załamania
            Output[ 2 ] = {
                {0.f, -railheight + 0.01f, 0.f},
                {0.f, 1.f, 0.f},
                {0.5f, 0.f} }; // middle
            Output[ 3 ] = {
                {-fHTW - side, -railheight, 0.f},
                {-normalx, normaly, 0.f},
                {0.67f, 0.f} }; // druga
            Output[ 4 ] = {
                {-rozp, -texheight1 - railheight, 0.f},
                {-normalx, normaly, 0.f},
                {1.00f, 0.f} }; // prawy skos
        }
    }
    else {
        // mapowanie proporcjonalne do powierzchni, rozmiar w poprzek określa fTexLength
        auto const max = fTexRatio2 * texturelength; // szerokość proporcjonalna do długości
        auto const map11 = max > 0.f ? (fHTW + side) / max : 0.25f; // załamanie od strony 1
        auto const map12 = max > 0.f ? (fHTW + side + hypot1) / max : 0.5f; // brzeg od strony 1
        if (transition) {
            // trapez albo przechyłki
            auto const map21 = max > 0.f ? (fHTW2 + side2) / max : 0.25f; // załamanie od strony 2
            auto const map22 = max > 0.f ? (fHTW2 + side2 + hypot2) / max : 0.5f; // brzeg od strony 2
            // ewentualnie poprawić mapowanie, żeby środek mapował się na 1.435/4.671
            // ((0.3464,0.6536)
            // bo się tekstury podsypki rozjeżdżają po zmianie proporcji profilu
            Output[ 0 ] = {
                {rozp, -texheight1 - railheight, 0.f},
                {normal1.x, normal1.y, 0.f},
                {0.5f - map12, 0.f} }; // lewy brzeg
            Output[ 1 ] = {
                {( fHTW + side ) * cos1, -( fHTW + side ) * sin1 - railheight, 0.f},
                {normal1.x, normal1.y, 0.f},
                {0.5f - map11 , 0.f} }; // krawędź załamania
            Output[ 2 ] = {
                {0.f, -railheight + 0.01f, 0.f},
                {0.f, 1.f, 0.f},
                {0.5f, 0.f} }; // middle
            Output[ 3 ] = {
                {-Output[ 1 ].position.x, +( fHTW + side ) * sin1 - railheight, 0.f},
                {-normal1.x, normal1.y, 0.f},
                {0.5 + map11, 0.f} }; // prawy brzeg początku symetrycznie
            Output[ 4 ] = {
                {-rozp, -texheight1 - railheight, 0.f},
                {-normal1.x, normal1.y, 0.f},
                {0.5f + map12, 0.f} }; // prawy skos
            // przekrój końcowy
            Output[ 5 ] = {
                {rozp2, -fTexHeight2 - railheight, 0.f},
                {normal2.x, normal2.y, 0.f},
                {0.5f - map22, 0.f} }; // lewy brzeg
            Output[ 6 ] = {
                {( fHTW2 + side2 ) * cos2, -( fHTW2 + side2 ) * sin2 - railheight, 0.f},
                {normal2.x, normal2.y, 0.f},
                {0.5f - map21 , 0.f} }; // krawędź załamania
            Output[ 7 ] = {
                {0.f, -railheight + 0.01f, 0.f},
                {0.f, 1.f, 0.f},
                {0.5f, 0.f} }; // middle
            Output[ 8 ] = {
                {-Output[ 6 ].position.x, +( fHTW2 + side2 ) * sin2 - railheight, 0.f},
                {-normal2.x, normal2.y, 0.f},
                {0.5f + map21, 0.f} }; // prawy brzeg początku symetrycznie
            Output[ 9 ] = {
                {-rozp2, -fTexHeight2 - railheight, 0.f},
                {-normal2.x, normal2.y, 0.f},
                {0.5f + map22, 0.f} }; // prawy skos
        }
        else
        {
            Output[ 0 ] = {
                {rozp, -texheight1 - railheight, 0.f},
                {+normal1.x, normal1.y, 0.f},
                {0.5f - map12, 0.f} }; // lewy brzeg
            Output[ 1 ] = {
                {fHTW + side, - railheight, 0.f},
                {+normal1.x, normal1.y, 0.f},
                {0.5f - map11, 0.f} }; // krawędź załamania
            Output[ 2 ] = {
                {0.f, -railheight + 0.01f, 0.f},
                {0.f, 1.f, 0.f},
                {0.5f, 0.f} }; // middle
            Output[ 3 ] = {
                {-fHTW - side, - railheight, 0.f},
                {-normal1.x, normal1.y, 0.f},
                {0.5f + map11, 0.f} }; // druga
            Output[ 4 ] = {
                {-rozp, -texheight1 - railheight, 0.f},
                {-normal1.x, normal1.y, 0.f},
                {0.5f + map12, 0.f} }; // prawy skos
        }
    }
}

// creates road profile data for current path
void
TTrack::create_road_profile( gfx::vertex_array &Output, bool const Forcetransition ) {

    auto const fHTW { 0.5f * std::abs( fTrackWidth ) };
    float const fHTW2 { (
        ( iTrapezoid & 2 ) != 0 ? // ten bit oznacza, że istnieje odpowiednie pNext
            0.5f * std::fabs( trNext->fTrackWidth ) : // połowa rozstawu/nawierzchni
            fHTW ) };

    glm::vec3 const normalup { 0.f, 1.f, 0.f };

    auto const texturelength { texture_length( m_material1 ) };
    auto const max = fTexRatio1 * texturelength; // test: szerokość proporcjonalna do długości
    auto const map1 = max > 0.f ? fHTW / max : 0.5f; // obcięcie tekstury od strony 1
    auto const map2 = max > 0.f ? fHTW2 / max : 0.5f; // obcięcie tekstury od strony 2

    auto const transition { ( true == Forcetransition ) || ( iTrapezoid != 0 ) };

    auto const pointcount{ transition ? 4 : 2 };
    Output.resize( pointcount );

    if( transition ) {
        // trapez albo przechyłki
        float
            roll1 { 0.f },
            roll2 { 0.f };

        if( Segment ) {
            Segment->GetRolls( roll1, roll2 );
        }

        Output[ 0 ] = {
            {fHTW * std::cos( roll1 ), -fHTW * std::sin( roll1 ), 0.f},
            normalup,
            {0.5f - map1, 0.f} }; // lewy brzeg początku
        Output[ 1 ] = {
            {-Output[ 0 ].position.x, -Output[ 0 ].position.y, 0.f},
            normalup,
            {0.5f + map1, 0.f} }; // prawy brzeg początku symetrycznie
        Output[ 2 ] = {
            {fHTW2 * std::cos( roll2 ), -fHTW2 * std::sin( roll2 ), 0.f},
            normalup,
            {0.5f - map2, 0.f} }; // lewy brzeg końca
        Output[ 3 ] = {
            {-Output[ 2 ].position.x, -Output[ 2 ].position.y, 0.f},
            normalup,
            {0.5f + map2, 0.f} }; // prawy brzeg początku symetrycznie
    }
    else {
        Output[ 0 ] = {
            {fHTW, 0.f, 0.f},
            normalup,
            {0.5f - map1, 0.f} };
        Output[ 1 ] = {
            {-fHTW, 0.f, 0.f},
            normalup,
            {0.5f + map1, 0.f} };
    }
}

void
TTrack::create_road_side_profile( gfx::vertex_array &Right, gfx::vertex_array &Left, gfx::vertex_array const &Road, bool const Forcetransition ) {

    auto const fHTW{ 0.5f * std::abs( fTrackWidth ) };
    auto const side{ std::abs( fTexWidth ) }; // szerokść podsypki na zewnątrz szyny albo pobocza
    auto const slop{ std::abs( fTexSlope ) }; // brzeg zewnętrzny
    auto const rozp{ fHTW + side + slop }; // brzeg zewnętrzny

    float fHTW2, side2, slop2, rozp2, fTexHeight2;
    if( iTrapezoid & 2 ) {
        // ten bit oznacza, że istnieje odpowiednie pNext
        // Ra: jest OK
        fHTW2 = 0.5f * std::fabs( trNext->fTrackWidth ); // połowa rozstawu/nawierzchni
        side2 = std::fabs( trNext->fTexWidth );
        slop2 = std::fabs( trNext->fTexSlope ); // nie jest używane później
        rozp2 = fHTW2 + side2 + slop2;
        fTexHeight2 = trNext->fTexHeight1;
    }
    else {
        // gdy nie ma następnego albo jest nieodpowiednim końcem podpięty
        fHTW2 = fHTW;
        side2 = side;
        slop2 = slop;
        rozp2 = rozp;
        fTexHeight2 = fTexHeight1;
    }

    glm::vec3 const normalup{ 0.f, 1.f, 0.f };

    auto const texturelength{ texture_length( m_material2 ) };

    auto const transition { ( true == Forcetransition ) || ( iTrapezoid != 0 ) };

    auto const pointcount{ transition ? 6 : 3 };
    Right.resize( pointcount );
    Left.resize( pointcount );


    if( fTexHeight1 >= 0.f ) { // standardowo: od zewnątrz pochylenie, a od wewnątrz poziomo
        Right[ 0 ] = {
            {rozp, -fTexHeight1, 0.f},
            { 1.f, 0.f, 0.f },
            {0.f, 0.f} }; // lewy brzeg podstawy
        Right[ 1 ] = {
            {Road[ 0 ].position.x + side, Road[ 0 ].position.y, 0.f},
            normalup,
            {0.5, 0.f} }; // lewa krawędź załamania
        Right[ 2 ] = {
            {Road[ 0 ].position.x, Road[ 0 ].position.y, 0.f},
            normalup,
            {1.f, 0.f} }; // lewy brzeg pobocza (mapowanie może być inne
        Left[ 0 ] = {
            {Road[ 1 ].position.x, Road[ 1 ].position.y, 0.f},
            normalup,
            {1.f, 0.f} }; // prawy brzeg pobocza
        Left[ 1 ] = {
            {Road[ 1 ].position.x - side, Road[ 1 ].position.y, 0.f},
            normalup,
            {0.5f, 0.f} }; // prawa krawędź załamania
        Left[ 2 ] = {
            {-rozp, -fTexHeight1, 0.f},
            { -1.f, 0.f, 0.f },
            {0.f, 0.f} }; // prawy brzeg podstawy
        if( transition ) {
            // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
            Right[ 3 ] = {
                {rozp2, -fTexHeight2, 0.f},
                { 1.f, 0.f, 0.f },
                {0.f, 0.f} }; // lewy brzeg lewego pobocza
            Right[ 4 ] = {
                {Road[ 2 ].position.x + side2, Road[ 2 ].position.y, 0.f},
                normalup,
                {0.5f, 0.f} }; // krawędź załamania
            Right[ 5 ] = {
                {Road[ 2 ].position.x, Road[ 2 ].position.y, 0.f},
                normalup,
                {1.f, 0.f} }; // brzeg pobocza
            Left[ 3 ] = {
                {Road[ 3 ].position.x, Road[ 3 ].position.y, 0.f},
                normalup,
                {1.f, 0.f} };
            Left[ 4 ] = {
                {Road[ 3 ].position.x - side2, Road[ 3 ].position.y, 0.f},
                normalup,
                {0.5f, 0.f} };
            Left[ 5 ] = {
                {-rozp2, -fTexHeight2, 0.f},
                { -1.f, 0.f, 0.f },
                {0.f, 0.f} }; // prawy brzeg prawego pobocza
        }
    }
    else { // wersja dla chodnika: skos 1:3.75, każdy chodnik innej szerokości
        // mapowanie propocjonalne do szerokości chodnika
        // krawężnik jest mapowany od 31/64 do 32/64 lewy i od 32/64 do 33/64 prawy
        auto const d = -fTexHeight1 / 3.75f; // krawężnik o wysokości 150mm jest pochylony 40mm
        auto const max = fTexRatio2 * texturelength; // test: szerokość proporcjonalna do długości
        auto const map1l = (
            max > 0.f ?
                side / max :
                0.484375f ); // obcięcie tekstury od lewej strony punktu 1
        auto const map1r = (
            max > 0.f ?
                slop / max :
                0.484375f ); // obcięcie tekstury od prawej strony punktu 1
        auto const h1r = (
            slop > d ?
                -fTexHeight1 :
                0.f );
        auto const h1l = (
            side > d ?
                -fTexHeight1 :
                0.f );

        Right[ 0 ] = {
            {Road[ 0 ].position.x + slop, Road[ 0 ].position.y + h1r, 0.f},
            normalup,
            {0.515625f + map1r, 0.f} }; // prawy brzeg prawego chodnika
        Right[ 1 ] = {
            {Road[ 0 ].position.x + d, Road[ 0 ].position.y + h1r, 0.f},
            normalup,
            {0.515625f, 0.f} }; // prawy krawężnik u góry
        Right[ 2 ] = {
            {Road[ 0 ].position.x, Road[ 0 ].position.y, 0.f},
            { -1.f, 0.f, 0.f },
            {0.515625f - d / 2.56f, 0.f} }; // prawy krawężnik u dołu
        Left[ 0 ] = {
            {Road[ 1 ].position.x, Road[ 1 ].position.y, 0.f},
            { 1.f, 0.f, 0.f },
            {0.484375f + d / 2.56f, 0.f} }; // lewy krawężnik u dołu
        Left[ 1 ] = {
            {Road[ 1 ].position.x - d, Road[ 1 ].position.y + h1l, 0.f},
            normalup,
            {0.484375f, 0.f} }; // lewy krawężnik u góry
        Left[ 2 ] = {
            {Road[ 1 ].position.x - side, Road[ 1 ].position.y + h1l, 0.f},
            normalup,
            {0.484375f - map1l, 0.f} }; // lewy brzeg lewego chodnika

        if( transition ) {
            // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
            slop2 = (
                std::fabs( ( iTrapezoid & 2 ) ?
                    slop2 :
                    slop ) ); // szerokość chodnika po prawej
            auto const map2l = (
                max > 0.f ?
                    side2 / max :
                    0.484375f ); // obcięcie tekstury od lewej strony punktu 2
            auto const map2r = (
                max > 0.f ?
                    slop2 / max :
                    0.484375f ); // obcięcie tekstury od prawej strony punktu 2
            auto const h2r = (
                slop2 > d ?
                    -fTexHeight2 :
                    0.f );
            auto const h2l = (
                side2 > d ?
                    -fTexHeight2 :
                    0.f );

            Right[ 3 ] = {
                {Road[ 2 ].position.x + slop2, Road[ 2 ].position.y + h2r, 0.f},
                normalup,
                {0.515625f + map2r, 0.f} }; // prawy brzeg prawego chodnika
            Right[ 4 ] = {
                {Road[ 2 ].position.x + d, Road[ 2 ].position.y + h2r, 0.f},
                normalup,
                {0.515625f, 0.f} }; // prawy krawężnik u góry
            Right[ 5 ] = {
                {Road[ 2 ].position.x, Road[ 2 ].position.y, 0.f},
                { -1.f, 0.f, 0.f },
                {0.515625f - d / 2.56f, 0.f} }; // prawy krawężnik u dołu
            Left[ 3 ] = {
                {Road[ 3 ].position.x, Road[ 3 ].position.y, 0.f},
                { 1.f, 0.f, 0.f },
                {0.484375f + d / 2.56f, 0.f} }; // lewy krawężnik u dołu
            Left[ 4 ] = {
                {Road[ 3 ].position.x - d, Road[ 3 ].position.y + h2l, 0.f},
                normalup,
                {0.484375f, 0.f} }; // lewy krawężnik u góry
            Left[ 5 ] = {
                {Road[ 3 ].position.x - side2, Road[ 3 ].position.y + h2l, 0.f},
                normalup,
                {0.484375f - map2l, 0.f} }; // lewy brzeg lewego chodnika
        }
    }
}

void
TTrack::create_switch_trackbed( gfx::vertex_array &Output ) {
    // try to get trackbed material from a regular track connected to the primary path
    if( ( SwitchExtension->m_material3 == null_handle )
     && ( trPrev != nullptr )
     && ( trPrev->eType == tt_Normal ) ) {
        SwitchExtension->m_material3 = trPrev->m_material2;
    }
    if( ( SwitchExtension->m_material3 == null_handle )
     && ( trNext != nullptr )
     && ( trNext->eType == tt_Normal ) ) {
        SwitchExtension->m_material3 = trNext->m_material2;
    }
    // without material don't bother
    if( SwitchExtension->m_material3 == null_handle ) { return; }
    // generate trackbed for each path of the switch...
    auto const texturelength { texture_length( SwitchExtension->m_material3 ) };
    gfx::vertex_array trackbedprofile;
    gfx::vertex_array trackbedvertices1, trackbedvertices2;
    // main trackbed
    create_track_bed_profile( trackbedprofile, SwitchExtension->pPrevs[ 0 ], SwitchExtension->pNexts[ 0 ] );
    SwitchExtension->Segments[ 0 ]->RenderLoft( trackbedvertices1, m_origin, trackbedprofile, true, texturelength );
    // side trackbed
    create_track_bed_profile( trackbedprofile, SwitchExtension->pPrevs[ 1 ], SwitchExtension->pNexts[ 1 ] );
    SwitchExtension->Segments[ 1 ]->RenderLoft( trackbedvertices2, m_origin, trackbedprofile, true, texturelength );
    // ...then combine them into a single geometry sequence
    auto const segmentsize { 10 };
    auto const segmentcount { trackbedvertices1.size() / segmentsize };
    auto *sampler1 { trackbedvertices1.data() };
    auto *sampler2 { trackbedvertices2.data() };
    auto const isright { SwitchExtension->RightSwitch };
    auto const isleft { false == isright };
    auto const samplersoffset { isright ? 2 : 0 };
    auto const geometryoffset { 0.025f };
    for( int segment = 0; segment < segmentcount; ++segment ) {
        // main trackbed
        // lower outer edge to avoid z-fighting
        if( isright ) {
            ( sampler1 + samplersoffset + 0 )->position.y -= geometryoffset;
            ( sampler1 + samplersoffset + 1 )->position.y -= geometryoffset;
        }
        if( isleft ) {
            ( sampler1 + samplersoffset + 6 )->position.y -= geometryoffset;
            ( sampler1 + samplersoffset + 7 )->position.y -= geometryoffset;
        }
        // copy the data
        for( auto pointidx = 0; pointidx < segmentsize; ++pointidx ) {
            Output.emplace_back( *( sampler1 + pointidx ) );
        }
        // side trackbed
        // lower outer edge to avoid z-fighting
        if( isleft ) {
            ( sampler2 - samplersoffset + 2 )->position.y -= geometryoffset;
            ( sampler2 - samplersoffset + 3 )->position.y -= geometryoffset;
        }
        if( isright ) {
            ( sampler2 - samplersoffset + 8 )->position.y -= geometryoffset;
            ( sampler2 - samplersoffset + 9 )->position.y -= geometryoffset;
        }
        // copy the data
        for( auto pointidx = 0; pointidx < segmentsize; ++pointidx ) {
            Output.emplace_back( *( sampler2 + pointidx ) );
        }
        // switch to next segment data
        sampler1 += segmentsize;
        sampler2 += segmentsize;
    }
}

material_handle
TTrack::copy_adjacent_trackbed_material( TTrack const *Exclude ) {

    if( iCategoryFlag != 1 ) { return null_handle; } // tracks only

    auto &material { eType == tt_Switch ? SwitchExtension->m_material3 : m_material2 };

    if( material != null_handle ) { return material; } // already has material

    std::vector<TTrack *> adjacents;
    switch( eType ) {
        case tt_Normal: {
            // for regular tracks don't set the trackbed texture if we aren't sitting next to a part of a double slip
/*
            auto const hasadjacentdoubleslip {
                ( trPrev ? trPrev->DoubleSlip() : false )
             || ( trNext ? trNext->DoubleSlip() : false ) };

            if( true == hasadjacentdoubleslip ) {
                adjacents.emplace_back( trPrev );
                adjacents.emplace_back( trNext );
            }
*/
            auto const hasadjacentswitch {
                ( trPrev && trPrev->eType == tt_Switch )
             || ( trNext && trNext->eType == tt_Switch ) };

//            if( true == hasadjacentdoubleslip ) {
            if( true == hasadjacentswitch ) {
                adjacents.emplace_back( trPrev );
                adjacents.emplace_back( trNext );
            }
            break;
        }
        case tt_Switch: {
            // only check the neighbours of the main track
            adjacents.emplace_back( SwitchExtension->pPrevs[ 0 ] );
            adjacents.emplace_back( SwitchExtension->pNexts[ 0 ] );
            break;
        }
        default: {
            break;
        }
    }

    for( auto *adjacent : adjacents ) {
        if( ( adjacent != nullptr ) && ( adjacent != Exclude ) ) {
            material = adjacent->copy_adjacent_trackbed_material( this );
            if( material != null_handle ) { break; } // got what we wanted
        }
    }

    return material;
}



path_table::~path_table() {

    TIsolated::DeleteAll();
}

// legacy method, initializes tracks after deserialization from scenario file
void
path_table::InitTracks() {

    int connection { -1 };
    TTrack *matchingtrack { nullptr };

#ifdef EU07_IGNORE_LEGACYPROCESSINGORDER
    for( auto *track : m_items ) {
#else
    // NOTE: legacy code peformed item operations last-to-first due to way the items were added to the list
    // this had impact in situations like two possible connection candidates, where only the first one would be used
    for( auto first = std::rbegin(m_items); first != std::rend(m_items); ++first ) {
        auto *track = *first;
#endif
        track->AssignEvents();

        auto const trackname { track->name() };

        switch (track->eType) {

        case tt_Table: {
            // obrotnicę też łączymy na starcie z innymi torami
            // szukamy modelu o tej samej nazwie
            auto *instance = simulation::Instances.find( trackname );
            // wiązanie toru z modelem obrotnicy
            track->RaAssign(
                instance,
                simulation::Events.FindEvent( trackname + ":done" ),
                simulation::Events.FindEvent( trackname + ":joined" ) );
            if( instance == nullptr ) {
                // jak nie ma modelu to pewnie jest wykolejnica, a ta jest domyślnie zamknięta i wykoleja
                break;
            }
            // no break on purpose:
            // "jak coś pójdzie źle, to robimy z tego normalny tor"
        }
        case tt_Normal: {
            // tylko proste są podłączane do rozjazdów, stąd dwa rozjazdy się nie połączą ze sobą
            if( track->CurrentPrev() == nullptr ) {
                // tylko jeśli jeszcze nie podłączony
                std::tie( matchingtrack, connection ) = simulation::Region->find_path( track->CurrentSegment()->FastGetPoint_0(), track );
                switch( connection ) {
                    case -1: // Ra: pierwsza koncepcja zawijania samochodów i statków
                        // if ((Track->iCategoryFlag&1)==0) //jeśli nie jest torem szynowym
                        // Track->ConnectPrevPrev(Track,0); //łączenie końca odcinka do samego siebie
                        break;
                    case 0:
                        track->ConnectPrevPrev( matchingtrack, 0 );
                        break;
                    case 1:
                        track->ConnectPrevNext( matchingtrack, 1 );
                        break;
                    case 2:
                        track->ConnectPrevPrev( matchingtrack, 0 ); // do Point1 pierwszego
                        matchingtrack->SetConnections( 0 ); // zapamiętanie ustawień w Segmencie
                        break;
                    case 3:
                        track->ConnectPrevNext( matchingtrack, 1 ); // do Point2 pierwszego
                        matchingtrack->SetConnections( 0 ); // zapamiętanie ustawień w Segmencie
                        break;
                    case 4:
                        matchingtrack->Switch( 1 );
                        track->ConnectPrevPrev( matchingtrack, 2 ); // do Point1 drugiego
                        matchingtrack->SetConnections( 1 ); // robi też Switch(0)
                        matchingtrack->Switch( 0 );
                        break;
                    case 5:
                        matchingtrack->Switch( 1 );
                        track->ConnectPrevNext( matchingtrack, 3 ); // do Point2 drugiego
                        matchingtrack->SetConnections( 1 ); // robi też Switch(0)
                        matchingtrack->Switch( 0 );
                        break;
                    default:
                        break;
                }
            }
            if( track->CurrentNext() == nullptr ) {
                // tylko jeśli jeszcze nie podłączony
                std::tie( matchingtrack, connection ) = simulation::Region->find_path( track->CurrentSegment()->FastGetPoint_1(), track );
                switch( connection ) {
                    case -1: // Ra: pierwsza koncepcja zawijania samochodów i statków
                        // if ((Track->iCategoryFlag&1)==0) //jeśli nie jest torem szynowym
                        // Track->ConnectNextNext(Track,1); //łączenie końca odcinka do samego siebie
                        break;
                    case 0:
                        track->ConnectNextPrev( matchingtrack, 0 );
                        break;
                    case 1:
                        track->ConnectNextNext( matchingtrack, 1 );
                        break;
                    case 2:
                        track->ConnectNextPrev( matchingtrack, 0 );
                        matchingtrack->SetConnections( 0 ); // zapamiętanie ustawień w Segmencie
                        break;
                    case 3:
                        track->ConnectNextNext( matchingtrack, 1 );
                        matchingtrack->SetConnections( 0 ); // zapamiętanie ustawień w Segmencie
                        break;
                    case 4:
                        matchingtrack->Switch( 1 );
                        track->ConnectNextPrev( matchingtrack, 2 );
                        matchingtrack->SetConnections( 1 ); // robi też Switch(0)
                        // tmp->Switch(0);
                        break;
                    case 5:
                        matchingtrack->Switch( 1 );
                        track->ConnectNextNext( matchingtrack, 3 );
                        matchingtrack->SetConnections( 1 ); // robi też Switch(0)
                        // tmp->Switch(0);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case tt_Switch: {
            // dla rozjazdów szukamy eventów sygnalizacji rozprucia
            track->AssignForcedEvents(
                simulation::Events.FindEvent( trackname + ":forced+" ),
                simulation::Events.FindEvent( trackname + ":forced-" ) );
            break;
        }
        default: {
            break;
        }
        } // switch

        if( ( trackname[ 0 ] == '*' )
         && ( !track->CurrentPrev() && track->CurrentNext() ) ) {
            // możliwy portal, jeśli nie podłączony od strony 1
            // ustawienie flagi portalu
            track->iCategoryFlag |= 0x100;
        }
    }

    if( Global.CreateSwitchTrackbeds ) {
        // do this after all connections are established, otherwise missing switch connections
        // may prevent us from obtaining texture data for basic track from 'across' a switch
        for( auto *track : m_items ) {
            // try to assign missing trackbed materials for switches and tracks neighbouring switches
            switch( track->eType ) {

            case tt_Normal:
            case tt_Switch: {
                track->copy_adjacent_trackbed_material();
                break;
            }
            default: {
                break;
            }
            } // switch
        }
    }

    auto *isolated = TIsolated::Root();
    while( isolated ) {

        isolated->AssignEvents();

        auto *memorycell = simulation::Memory.find( isolated->asName ); // czy jest komóka o odpowiedniej nazwie
        if( memorycell == nullptr ) {
            // utworzenie automatycznej komórki
            // TODO: determine suitable location for this one, create and add world reference node
            scene::node_data nodedata;
            nodedata.name = isolated->asName;
            memorycell = new TMemCell( nodedata ); // to nie musi mieć nazwy, nazwa w wyszukiwarce wystarczy
            // NOTE: autogenerated cells aren't exported; they'll be autogenerated anew when exported file is loaded
            memorycell->is_exportable = false;
            simulation::Memory.insert( memorycell );
        }
        // przypisanie powiązanej komórki
        isolated->pMemCell = memorycell;

        isolated = isolated->Next();
    }

    TTrack::fetch_default_profiles();
}

// legacy method, sends list of occupied paths over network
void
path_table::TrackBusyList() const {
    // wysłanie informacji o wszystkich zajętych odcinkach
    for( auto const *path : m_items ) {
        if( ( false == path->name().empty() ) // musi być nazwa
         && ( false == path->Dynamics.empty() ) ) {
            // zajęty
            multiplayer::WyslijString( path->name(), 8 );
        }
    }
}

// legacy method, sends list of occupied path sections over network
void
path_table::IsolatedBusyList() const {
    // wysłanie informacji o wszystkich odcinkach izolowanych
    TIsolated *Current;
    for( Current = TIsolated::Root(); Current; Current = Current->Next() ) {

        if( Current->Busy() ) { multiplayer::WyslijString( Current->asName, 11 ); }
        else                  { multiplayer::WyslijString( Current->asName, 10 ); }
    }
    multiplayer::WyslijString( "none", 10 ); // informacja o końcu listy
}

// legacy method, sends state of specified path section over network
void
path_table::IsolatedBusy( std::string const &Name ) const {
    // wysłanie informacji o odcinku izolowanym (t)
    // Ra 2014-06: do wyszukania użyć drzewka nazw
    TIsolated *Current;
    for( Current = TIsolated::Root(); Current; Current = Current->Next() ) {
        if( Current->asName == Name ) {
            if( Current->Busy() ) { multiplayer::WyslijString( Current->asName, 11 ); }
            else                  { multiplayer::WyslijString( Current->asName, 10 ); }
            // nie sprawdzaj dalszych
            return;
        }
    }
    multiplayer::WyslijString( Name, 10 ); // wolny (technically not found but, eh)
}
