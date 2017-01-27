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
#include "Globals.h"
#include "Logs.h"
#include "Usefull.h"
#include "Texture.h"
#include "Timer.h"
#include "Ground.h"
#include "parser.h"
#include "Mover.h"
#include "DynObj.h"
#include "AnimModel.h"
#include "MemCell.h"
#include "Event.h"

// 101206 Ra: trapezoidalne drogi i tory
// 110720 Ra: rozprucie zwrotnicy i odcinki izolowane

static const double fMaxOffset = 0.1; // double(0.1f)==0.100000001490116
// const int NextMask[4]={0,1,0,1}; //tor następny dla stanów 0, 1, 2, 3
// const int PrevMask[4]={0,0,1,1}; //tor poprzedni dla stanów 0, 1, 2, 3
const int iLewo4[4] = {5, 3, 4, 6}; // segmenty (1..6) do skręcania w lewo
const int iPrawo4[4] = {-4, -6, -3, -5}; // segmenty (1..6) do skręcania w prawo
const int iProsto4[4] = {1, -1, 2, -2}; // segmenty (1..6) do jazdy prosto
const int iEnds4[13] = {3, 0, 2, 1, 2, 0, -1,
                        1, 3, 2, 0, 3, 1}; // numer sąsiedniego toru na końcu segmentu "-1"
const int iLewo3[4] = {1, 3, 2, 1}; // segmenty do skręcania w lewo
const int iPrawo3[4] = {-2, -1, -3, -2}; // segmenty do skręcania w prawo
const int iProsto3[4] = {1, -1, 2, 1}; // segmenty do jazdy prosto
const int iEnds3[13] = {3, 0, 2, 1, 2, 0, -1,
                        1, 0, 2, 0, 3, 1}; // numer sąsiedniego toru na końcu segmentu "-1"
TIsolated *TIsolated::pRoot = NULL;

TSwitchExtension::TSwitchExtension(TTrack *owner, int const what)
{ // na początku wszystko puste
    pNexts[0] = nullptr; // wskaźniki do kolejnych odcinków ruchu
    pNexts[1] = nullptr;
    pPrevs[0] = nullptr;
    pPrevs[1] = nullptr;
    fOffset1 = fOffset = fDesiredOffset = -fOffsetDelay; // położenie zasadnicze
    fOffset2 = 0.0; // w zasadniczym wewnętrzna iglica dolega
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

TIsolated::TIsolated()
{ // utworznie pustego
    TIsolated("none", NULL);
};

TIsolated::TIsolated(const string &n, TIsolated *i) :
                           asName( n ),   pNext( i )
{
    // utworznie obwodu izolowanego. nothing to do here.
};

TIsolated::~TIsolated(){
    // usuwanie
    /*
     TIsolated *p=pRoot;
     while (pRoot)
     {
      p=pRoot;
      p->pNext=NULL;
      delete p;
     }
    */
};

TIsolated * TIsolated::Find(const string &n)
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

void TIsolated::Modify(int i, TDynamicObject *o)
{ // dodanie lub odjęcie osi
    if (iAxles)
    { // grupa zajęta
        iAxles += i;
        if (!iAxles)
        { // jeśli po zmianie nie ma żadnej osi na odcinku izolowanym
            if (evFree)
                Global::AddToQuery(evFree, o); // dodanie zwolnienia do kolejki
            if (Global::iMultiplayer) // jeśli multiplayer
                Global::pGround->WyslijString(asName, 10); // wysłanie pakietu o zwolnieniu
            if (pMemCell) // w powiązanej komórce
                pMemCell->UpdateValues(NULL, 0, int(pMemCell->Value2()) & ~0xFF,
                                       update_memval2); //"zerujemy" ostatnią wartość
        }
    }
    else
    { // grupa była wolna
        iAxles += i;
        if (iAxles)
        {
            if (evBusy)
                Global::AddToQuery(evBusy, o); // dodanie zajętości do kolejki
            if (Global::iMultiplayer) // jeśli multiplayer
                Global::pGround->WyslijString(asName, 11); // wysłanie pakietu o zajęciu
            if (pMemCell) // w powiązanej komórce
                pMemCell->UpdateValues(NULL, 0, int(pMemCell->Value2()) | 1,
                                       update_memval2); // zmieniamy ostatnią wartość na nieparzystą
        }
    }
};

// tworzenie nowego odcinka ruchu
TTrack::TTrack(TGroundNode *g) :
    pMyNode( g ) // Ra: proteza, żeby tor znał swoją nazwę TODO: odziedziczyć TTrack z TGroundNode
{
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    ::SecureZeroMemory( Dynamics, sizeof( Dynamics ) );
#endif
    fRadiusTable[ 0 ] = 0.0;
    fRadiusTable[ 1 ] = 0.0;
    nFouling[ 0 ] = nullptr;
    nFouling[ 1 ] = nullptr;
}

TTrack::~TTrack()
{ // likwidacja odcinka
	if( eType == tt_Cross ) {
		delete SwitchExtension->vPoints; // skrzyżowanie może mieć punkty
	}

/*    if (eType == tt_Normal)
        delete Segment; // dla zwrotnic nie usuwać tego (kopiowany)
    else
    { // usuwanie dodatkowych danych dla niezwykłych odcinków
        if (eType == tt_Cross)
            delete SwitchExtension->vPoints; // skrzyżowanie może mieć punkty
        SafeDelete(SwitchExtension);
    }
*/
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
        SwitchExtension->vPoints = NULL; // brak tablicy punktów
        SwitchExtension->iPoints = 0;
        SwitchExtension->bPoints = false; // tablica punktów nie wypełniona
        SwitchExtension->iRoads = 4; // domyślnie 4
        break;
    case tt_Normal:
		Segment = std::make_shared<TSegment>( this );
        break;
    case tt_Table: // oba potrzebne
		SwitchExtension = std::make_shared<TSwitchExtension>( this, 1 ); // kopia oryginalnego toru
        Segment = make_shared<TSegment>(this);
        break;
    }
}

TTrack * TTrack::Create400m(int what, double dx)
{ // tworzenie toru do wstawiania taboru podczas konwersji na E3D
    TGroundNode *tmp = new TGroundNode(TP_TRACK); // node
    TTrack *trk = tmp->pTrack;
    trk->bVisible = false; // nie potrzeba pokazywać, zresztą i tak nie ma tekstur
    trk->iCategoryFlag = what; // taki sam typ plus informacja, że dodatkowy
    trk->Init(); // utworzenie segmentu
    trk->Segment->Init(vector3(-dx, 0, 0), vector3(-dx, 0, 400), 0, 0, 0); // prosty
    tmp->pCenter = vector3(-dx, 0, 200); //środek, aby się mogło wyświetlić
    TSubRect *r = Global::pGround->GetSubRect(tmp->pCenter.x, tmp->pCenter.z);
    r->NodeAdd(tmp); // dodanie toru do segmentu
    r->Sort(); //żeby wyświetlał tabor z dodanego toru
    r->Release(); // usunięcie skompilowanych zasobów
    return trk;
};

TTrack * TTrack::NullCreate(int dir)
{ // tworzenie toru wykolejającego od strony (dir), albo pętli dla samochodów
    TGroundNode *tmp = new TGroundNode(TP_TRACK), *tmp2 = NULL; // node
    TTrack *trk = tmp->pTrack; // tor; UWAGA! obrotnica może generować duże ilości tego
    // tmp->iType=TP_TRACK;
    // TTrack* trk=new TTrack(tmp); //tor; UWAGA! obrotnica może generować duże ilości tego
    // tmp->pTrack=trk;
    trk->bVisible = false; // nie potrzeba pokazywać, zresztą i tak nie ma tekstur
    // trk->iTrapezoid=1; //są przechyłki do uwzględniania w rysowaniu
    trk->iCategoryFlag = (iCategoryFlag & 15) | 0x80; // taki sam typ plus informacja, że dodatkowy
    double r1, r2;
    Segment->GetRolls(r1, r2); // pobranie przechyłek na początku toru
    vector3 p1, cv1, cv2, p2; // będziem tworzyć trajektorię lotu
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
            trk->Segment->Init(p1, p2, 5, -RadToDeg(r1),
                               70.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            ConnectPrevPrev(trk, 0);
            break;
        case 1:
            p1 = Segment->FastGetPoint_1();
            p2 = p1 - 450.0 * Normalize(Segment->GetDirection2());
            trk->Segment->Init(p1, p2, 5, RadToDeg(r2),
                               70.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            ConnectNextPrev(trk, 0);
            break;
        case 3: // na razie nie możliwe
            p1 = SwitchExtension->Segments[1]->FastGetPoint_1(); // koniec toru drugiego zwrotnicy
            p2 = p1 -
                 450.0 *
                     Normalize(
                         SwitchExtension->Segments[1]->GetDirection2()); // przedłużenie na wprost
            trk->Segment->Init(p1, p2, 5, RadToDeg(r2),
                               70.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
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
        tmp2 = new TGroundNode(TP_TRACK); // drugi odcinek do zapętlenia
        TTrack *trk2 = tmp2->pTrack;
        trk2->iCategoryFlag =
            (iCategoryFlag & 15) | 0x80; // taki sam typ plus informacja, że dodatkowy
        trk2->bVisible = false;
        trk2->fVelocity = 20.0; // zawracanie powoli
        trk2->fRadius = 20.0; // promień, aby się dodawało do tabelki prędkości i liczyło
        // narastająco
        trk2->Init(); // utworzenie segmentu
        switch (dir)
        { //łączenie z nowym torem
        case 0:
            p1 = Segment->FastGetPoint_0();
            cv1 = -20.0 * Normalize(Segment->GetDirection1()); // pierwszy wektor kontrolny
            p2 = p1 + cv1 + cv1; // 40m
            trk->Segment->Init(p1, p1 + cv1, p2 + vector3(-cv1.z, cv1.y, cv1.x), p2, 2,
                               -RadToDeg(r1),
                               0.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            ConnectPrevPrev(trk, 0);
            trk2->Segment->Init(p1, p1 + cv1, p2 + vector3(cv1.z, cv1.y, -cv1.x), p2, 2,
                                -RadToDeg(r1),
                                0.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk2->iPrevDirection = 0; // zwrotnie do tego samego odcinka
            break;
        case 1:
            p1 = Segment->FastGetPoint_1();
            cv1 = -20.0 * Normalize(Segment->GetDirection2()); // pierwszy wektor kontrolny
            p2 = p1 + cv1 + cv1;
            trk->Segment->Init(p1, p1 + cv1, p2 + vector3(-cv1.z, cv1.y, cv1.x), p2, 2,
                               RadToDeg(r2),
                               0.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            ConnectNextPrev(trk, 0);
            trk2->Segment->Init(p1, p1 + cv1, p2 + vector3(cv1.z, cv1.y, -cv1.x), p2, 2,
                                RadToDeg(r2),
                                0.0); // bo prosty, kontrolne wyliczane przy zmiennej przechyłce
            trk2->iPrevDirection = 1; // zwrotnie do tego samego odcinka
            break;
        }
        trk2->trPrev = this;
        trk->ConnectNextNext(trk2, 1); // połączenie dwóch dodatkowych odcinków punktami 2
        tmp2->pCenter = (0.5 * (p1 + p2)); //środek, aby się mogło wyświetlić
    }
    // trzeba jeszcze dodać do odpowiedniego segmentu, aby się renderowały z niego pojazdy
    tmp->pCenter = (0.5 * (p1 + p2)); //środek, aby się mogło wyświetlić
    if (tmp2)
        tmp2->pCenter = tmp->pCenter; // ten sam środek jest
    // Ra: to poniżej to porażka, ale na razie się nie da inaczej
    TSubRect *r = Global::pGround->GetSubRect(tmp->pCenter.x, tmp->pCenter.z);
    r->NodeAdd(tmp); // dodanie toru do segmentu
    if (tmp2)
        r->NodeAdd(tmp2); // drugiego też
    r->Sort(); //żeby wyświetlał tabor z dodanego toru
    r->Release(); // usunięcie skompilowanych zasobów
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
        if (bVisible)
            if (pTrack->bVisible)
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
        if (bVisible)
            if (pTrack->bVisible)
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

vector3 MakeCPoint(vector3 p, double d, double a1, double a2)
{
    vector3 cp = vector3(0, 0, 1);
    cp.RotateX(DegToRad(a2));
    cp.RotateY(DegToRad(a1));
    cp = cp * d + p;
    return cp;
}

vector3 LoadPoint(cParser *parser)
{ // pobranie współrzędnych punktu
    vector3 p;
    std::string token;
    parser->getTokens(3);
    *parser >> p.x >> p.y >> p.z;
    return p;
}

void TTrack::Load(cParser *parser, vector3 pOrigin, std::string name)
{ // pobranie obiektu trajektorii ruchu
    vector3 pt, vec, p1, p2, cp1, cp2, p3, p4, cp3, cp4; // dodatkowe punkty potrzebne do skrzyżowań
    double a1, a2, r1, r2, r3, r4, d1, d2, a;
    string str;
    bool bCurve;
    int i; //,state; //Ra: teraz już nie ma początkowego stanu zwrotnicy we wpisie
    std::string token;

    parser->getTokens();
    *parser >> token;
    str = token; // typ toru

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
    if (Global::iWriteLogEnabled & 4)
        WriteLog(str);
    parser->getTokens(4);
    *parser >> fTrackLength >> fTrackWidth >> fFriction >> fSoundDistance;
    //    fTrackLength=Parser->GetNextSymbol().ToDouble();                       //track length
    //    100502
    //    fTrackWidth=Parser->GetNextSymbol().ToDouble();                        //track width
    //    fFriction=Parser->GetNextSymbol().ToDouble();                          //friction coeff.
    //    fSoundDistance=Parser->GetNextSymbol().ToDouble();   //snd
    fTrackWidth2 = fTrackWidth; // rozstaw/szerokość w punkcie 2, na razie taka sama
    parser->getTokens(2);
    *parser >> iQualityFlag >> iDamageFlag;
    //    iQualityFlag=Parser->GetNextSymbol().ToInt();   //McZapkie: qualityflag
    //    iDamageFlag=Parser->GetNextSymbol().ToInt();   //damage
    if (iDamageFlag & 128)
        iAction |= 0x80; // flaga wykolejania z powodu uszkodzenia
    parser->getTokens();
    *parser >> token;
    str = token; // environment
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
        Error("Unknown track environment: \"" + str + "\"");
    }
    parser->getTokens();
    *parser >> token;
    bVisible = (token.compare("vis") == 0); // visible
    if (bVisible)
    {
        parser->getTokens();
        *parser >> token;
        str = token; // railtex
        TextureID1 = (str == "none" ? 0 : TTexturesManager::GetTextureID(
                                              szTexturePath, szSceneryPath, str,
                                              (iCategoryFlag & 1) ? Global::iRailProFiltering :
                                                                    Global::iBallastFiltering));
        parser->getTokens();
        *parser >> fTexLength; // tex tile length
        if (fTexLength < 0.01)
            fTexLength = 4; // Ra: zabezpiecznie przed zawieszeniem
        parser->getTokens();
        *parser >> token;
        str = token; // sub || railtex
        TextureID2 = (str == "none" ? 0 : TTexturesManager::GetTextureID(
                                              szTexturePath, szSceneryPath, str,
                                              (eType == tt_Normal) ? Global::iBallastFiltering :
                                                                     Global::iRailProFiltering));
        parser->getTokens(3);
        *parser >> fTexHeight1 >> fTexWidth >> fTexSlope;
        //     fTexHeight=Parser->GetNextSymbol().ToDouble(); //tex sub height
        //     fTexWidth=Parser->GetNextSymbol().ToDouble(); //tex sub width
        //     fTexSlope=Parser->GetNextSymbol().ToDouble(); //tex sub slope width
        if (iCategoryFlag & 4)
            fTexHeight1 = -fTexHeight1; // rzeki mają wysokość odwrotnie niż drogi
    }
    else if (Global::iWriteLogEnabled & 4)
        WriteLog("unvis");
    Init(); // ustawia SwitchExtension
    double segsize = 5.0; // długość odcinka segmentowania
    switch (eType)
    { // Ra: łuki segmentowane co 5m albo 314-kątem foremnym
    case tt_Table: // obrotnica jest prawie jak zwykły tor
        iAction |= 2; // flaga zmiany położenia typu obrotnica
    case tt_Normal:
        p1 = LoadPoint(parser) + pOrigin; // pobranie współrzędnych P1
        parser->getTokens();
        *parser >> r1; // pobranie przechyłki w P1
        cp1 = LoadPoint(parser); // pobranie współrzędnych punktów kontrolnych
        cp2 = LoadPoint(parser);
        p2 = LoadPoint(parser) + pOrigin; // pobranie współrzędnych P2
        parser->getTokens(2);
        *parser >> r2 >> fRadius; // pobranie przechyłki w P1 i promienia
        fRadius = fabs(fRadius); // we wpisie może być ujemny
        if (iCategoryFlag & 1)
        { // zero na główce szyny
            p1.y += 0.18;
            p2.y += 0.18;
            // na przechyłce doliczyć jeszcze pół przechyłki
        }
        if (fRadius != 0) // gdy podany promień
            segsize = Min0R(5.0, 0.2 + fabs(fRadius) * 0.02); // do 250m - 5, potem 1 co 50m

        if ((((p1 + p1 + p2) / 3.0 - p1 - cp1).Length() < 0.02) ||
            (((p1 + p2 + p2) / 3.0 - p2 + cp1).Length() < 0.02))
            cp1 = cp2 = vector3(0, 0, 0); //"prostowanie" prostych z kontrolnymi, dokładność 2cm

        if ((cp1 == vector3(0, 0, 0)) &&
            (cp2 == vector3(0, 0, 0))) // Ra: hm, czasem dla prostego są podane...
            Segment->Init(p1, p2, segsize, r1,
                          r2); // gdy prosty, kontrolne wyliczane przy zmiennej przechyłce
        else
            Segment->Init(p1, cp1 + p1, cp2 + p2, p2, segsize, r1,
                          r2); // gdy łuk (ustawia bCurve=true)
        if ((r1 != 0) || (r2 != 0))
            iTrapezoid = 1; // są przechyłki do uwzględniania w rysowaniu
        if (eType == tt_Table) // obrotnica ma doklejkę
        { // SwitchExtension=new TSwitchExtension(this,1); //dodatkowe zmienne dla obrotnicy
            SwitchExtension->Segments[0]->Init(p1, p2, segsize); // kopia oryginalnego toru
        }
        else if (iCategoryFlag & 2)
            if (TextureID1 && fTexLength)
            { // dla drogi trzeba ustalić proporcje boków nawierzchni
                float w, h;
                glBindTexture(GL_TEXTURE_2D, TextureID1);
                glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
                glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
                if (h != 0.0)
                    fTexRatio1 = w / h; // proporcja boków
                glBindTexture(GL_TEXTURE_2D, TextureID2);
                glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
                glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
                if (h != 0.0)
                    fTexRatio2 = w / h; // proporcja boków
            }
        break;

    case tt_Cross: // skrzyżowanie dróg - 4 punkty z wektorami kontrolnymi
        segsize = 1.0; // specjalne segmentowanie ze względu na małe promienie
    case tt_Tributary: // dopływ
    case tt_Switch: // zwrotnica
        iAction |= 1; // flaga zmiany położenia typu zwrotnica lub skrzyżowanie dróg
        // problemy z animacją iglic powstaje, gdzy odcinek prosty ma zmienną przechyłkę
        // wtedy dzieli się na dodatkowe odcinki (po 0.2m, bo R=0) i animację diabli biorą
        // Ra: na razie nie podejmuję się przerabiania iglic

        // SwitchExtension=new TSwitchExtension(this,eType==tt_Cross?6:2); //zwrotnica ma doklejkę

        p1 = LoadPoint(parser) + pOrigin; // pobranie współrzędnych P1
        parser->getTokens();
        *parser >> r1;
        cp1 = LoadPoint(parser);
        cp2 = LoadPoint(parser);
        p2 = LoadPoint(parser) + pOrigin; // pobranie współrzędnych P2
        parser->getTokens(2);
        *parser >> r2 >> fRadiusTable[0];
        fRadiusTable[0] = fabs(fRadiusTable[0]); // we wpisie może być ujemny
        if (iCategoryFlag & 1)
        { // zero na główce szyny
            p1.y += 0.18;
            p2.y += 0.18;
            // na przechyłce doliczyć jeszcze pół przechyłki?
        }
        if (fRadiusTable[0] > 0)
            segsize = Min0R(5.0, 0.2 + fRadiusTable[0] * 0.02);
        else if (eType != tt_Cross) // dla skrzyżowań muszą być podane kontrolne
        { // jak promień zerowy, to przeliczamy punkty kontrolne
            cp1 = (p1 + p1 + p2) / 3.0 - p1; // jak jest prosty, to się zoptymalizuje
            cp2 = (p1 + p2 + p2) / 3.0 - p2;
            segsize = 5.0;
        } // ułomny prosty
        if (!(cp1 == vector3(0, 0, 0)) && !(cp2 == vector3(0, 0, 0)))
            SwitchExtension->Segments[0]->Init(p1, p1 + cp1, p2 + cp2, p2, segsize, r1, r2);
        else
            SwitchExtension->Segments[0]->Init(p1, p2, segsize, r1, r2);

        p3 = LoadPoint(parser) + pOrigin; // pobranie współrzędnych P3
        parser->getTokens();
        *parser >> r3;
        cp3 = LoadPoint(parser);
        cp4 = LoadPoint(parser);
        p4 = LoadPoint(parser) + pOrigin; // pobranie współrzędnych P4
        parser->getTokens(2);
        *parser >> r4 >> fRadiusTable[1];
        fRadiusTable[1] = fabs(fRadiusTable[1]); // we wpisie może być ujemny
        if (iCategoryFlag & 1)
        { // zero na główce szyny
            p3.y += 0.18;
            p4.y += 0.18;
            // na przechyłce doliczyć jeszcze pół przechyłki?
        }

        if (fRadiusTable[1] > 0)
            segsize = Min0R(5.0, 0.2 + fRadiusTable[1] * 0.02);
        else if (eType != tt_Cross) // dla skrzyżowań muszą być podane kontrolne
        { // jak promień zerowy, to przeliczamy punkty kontrolne
            cp3 = (p3 + p3 + p4) / 3.0 - p3; // jak jest prosty, to się zoptymalizuje
            cp4 = (p3 + p4 + p4) / 3.0 - p4;
            segsize = 5.0;
        } // ułomny prosty

        if (!(cp3 == vector3(0, 0, 0)) && !(cp4 == vector3(0, 0, 0)))
        { // dla skrzyżowania dróg dać odwrotnie końce, żeby brzegi generować lewym
            if (eType != tt_Cross)
                SwitchExtension->Segments[1]->Init(p3, p3 + cp3, p4 + cp4, p4, segsize, r3, r4);
            else
                SwitchExtension->Segments[1]->Init(p4, p4 + cp4, p3 + cp3, p3, segsize, r4,
                                                   r3); // odwrócony
        }
        else
            SwitchExtension->Segments[1]->Init(p3, p4, segsize, r3, r4);
        if (eType == tt_Cross)
        { // Ra 2014-07: dla skrzyżowań będą dodatkowe segmenty
            SwitchExtension->Segments[2]->Init(p2, cp2 + p2, cp4 + p4, p4, segsize, r2,
                                               r4); // z punktu 2 do 4
            if (LengthSquared3(p3 - p1) <
                0.01) // gdy mniej niż 10cm, to mamy skrzyżowanie trzech dróg
                SwitchExtension->iRoads = 3;
            else // dla 4 dróg będą dodatkowe 3 segmenty
            {
                SwitchExtension->Segments[3]->Init(p4, p4 + cp4, p1 + cp1, p1, segsize, r4,
                                                   r1); // z punktu 4 do 1
                SwitchExtension->Segments[4]->Init(p1, p1 + cp1, p3 + cp3, p3, segsize, r1,
                                                   r3); // z punktu 1 do 3
                SwitchExtension->Segments[5]->Init(p3, p3 + cp3, p2 + cp2, p2, segsize, r3,
                                                   r2); // z punktu 3 do 2
            }
        }

        Switch(0); // na stałe w położeniu 0 - nie ma początkowego stanu zwrotnicy we wpisie

        // Ra: zamienić później na iloczyn wektorowy
        {
            vector3 v1, v2;
            double a1, a2;
            v1 = SwitchExtension->Segments[0]->FastGetPoint_1() -
                 SwitchExtension->Segments[0]->FastGetPoint_0();
            v2 = SwitchExtension->Segments[1]->FastGetPoint_1() -
                 SwitchExtension->Segments[1]->FastGetPoint_0();
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
    parser->getTokens();
    *parser >> token;
    str = token;
    while (str != "endtrack")
    {
        if (str == "event0")
        {
            parser->getTokens();
            *parser >> token;
            asEvent0Name = token;
        }
        else if (str == "event1")
        {
            parser->getTokens();
            *parser >> token;
            asEvent1Name = token;
        }
        else if (str == "event2")
        {
            parser->getTokens();
            *parser >> token;
			asEvent2Name = token;
        }
        else if (str == "eventall0")
        {
            parser->getTokens();
            *parser >> token;
			asEventall0Name = token;
        }
        else if (str == "eventall1")
        {
            parser->getTokens();
            *parser >> token;
			asEventall1Name = token;
        }
        else if (str == "eventall2")
        {
            parser->getTokens();
            *parser >> token;
			asEventall2Name = token;
        }
        else if (str == "velocity")
        {
            parser->getTokens();
            *parser >> fVelocity; //*0.28; McZapkie-010602
            if (SwitchExtension) // jeśli tor ruchomy
                if (fabs(fVelocity) >= 1.0) //żeby zero nie ograniczało dożywotnio
                    SwitchExtension->fVelocity = fVelocity; // zapamiętanie głównego ograniczenia; a
            // np. -40 ogranicza tylko na bok
        }
        else if (str == "isolated")
        { // obwód izolowany, do którego tor należy
            parser->getTokens();
            *parser >> token;
            pIsolated = TIsolated::Find(token);
        }
        else if (str == "angle1")
        { // kąt ścięcia końca od strony 1
            parser->getTokens();
            *parser >> a1;
            Segment->AngleSet(0, a1);
        }
        else if (str == "angle2")
        { // kąt ścięcia końca od strony 2
            parser->getTokens();
            *parser >> a2;
            Segment->AngleSet(1, a2);
        }
        else if (str == "fouling1")
        { // wskazanie modelu ukresu w kierunku 1
            parser->getTokens();
            *parser >> token;
            // nFouling[0]=
        }
        else if (str == "fouling2")
        { // wskazanie modelu ukresu w kierunku 2
            parser->getTokens();
            *parser >> token;
            // nFouling[1]=
        }
        else if (str == "overhead")
        { // informacja o stanie sieci: 0-jazda bezprądowa, >0-z opuszczonym i ograniczeniem
            // prędkości
            parser->getTokens();
            *parser >> fOverhead;
            if (fOverhead > 0.0)
                iAction |= 0x40; // flaga opuszczenia pantografu (tor uwzględniany w skanowaniu jako
            // ograniczenie dla pantografujących)
        }
        else if (str == "colides")
        { // informacja o stanie sieci: 0-jazda bezprądowa, >0-z opuszczonym i ograniczeniem
            // prędkości
            parser->getTokens();
            *parser >> token;
            // trColides=; //tor kolizyjny, na którym trzeba sprawdzać pojazdy pod kątem zderzenia
        }
        else
            ErrorLog("Unknown property: \"" + str + "\" in track \"" + name + "\"");
        parser->getTokens();
        *parser >> token;
		str = token;
    }
    // alternatywny zapis nazwy odcinka izolowanego - po znaku "@" w nazwie toru
    if (!pIsolated)
        if ((i = name.find("@")) != string::npos)
            if (i < name.length()) // nie może być puste
            {
                pIsolated = TIsolated::Find(name.substr(i + 1, name.length()));
                name = name.substr(0, i - 1); // usunięcie z nazwy
            }
}

bool TTrack::AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2)
{
    bool bError = false;
    if (!evEvent0)
    {
        if (NewEvent0)
        {
            evEvent0 = NewEvent0;
            asEvent0Name = "";
            iEvents |= 1; // sumaryczna informacja o eventach
        }
        else
        {
            if (!asEvent0Name.empty())
            {
                ErrorLog("Bad track: Event0 \"" + asEvent0Name +
                         "\" does not exist");
                bError = true;
            }
        }
    }
    else
    {
        ErrorLog(
            "Bad track: Event0 cannot be assigned to track, track already has one");
        bError = true;
    }
    if (!evEvent1)
    {
        if (NewEvent1)
        {
            evEvent1 = NewEvent1;
            asEvent1Name = "";
            iEvents |= 2; // sumaryczna informacja o eventach
        }
        else if (!asEvent1Name.empty())
        { // Ra: tylko w logu informacja
            ErrorLog("Bad track: Event1 \"" + asEvent1Name +
                     "\" does not exist");
            bError = true;
        }
    }
    else
    {
        ErrorLog(
            "Bad track: Event1 cannot be assigned to track, track already has one");
        bError = true;
    }
    if (!evEvent2)
    {
        if (NewEvent2)
        {
            evEvent2 = NewEvent2;
            asEvent2Name = "";
            iEvents |= 4; // sumaryczna informacja o eventach
        }
        else if (!asEvent2Name.empty())
        { // Ra: tylko w logu informacja
            ErrorLog("Bad track: Event2 \"" + asEvent2Name +
                     "\" does not exist");
            bError = true;
        }
    }
    else
    {
        ErrorLog(
            "Bad track: Event2 cannot be assigned to track, track already has one");
        bError = true;
    }
    return !bError;
}

bool TTrack::AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2)
{
    bool bError = false;
    if (!evEventall0)
    {
        if (NewEvent0)
        {
            evEventall0 = NewEvent0;
            asEventall0Name = "";
            iEvents |= 8; // sumaryczna informacja o eventach
        }
        else
        {
            if (!asEvent0Name.empty())
            {
                Error("Eventall0 \"" + asEventall0Name +
                      "\" does not exist");
                bError = true;
            }
        }
    }
    else
    {
        Error("Eventall0 cannot be assigned to track, track already has one");
        bError = true;
    }
    if (!evEventall1)
    {
        if (NewEvent1)
        {
            evEventall1 = NewEvent1;
            asEventall1Name = "";
            iEvents |= 16; // sumaryczna informacja o eventach
        }
        else
        {
            if (!asEvent0Name.empty())
            { // Ra: tylko w logu informacja
                WriteLog("Eventall1 \"" + asEventall1Name +
                         "\" does not exist");
                bError = true;
            }
        }
    }
    else
    {
        Error("Eventall1 cannot be assigned to track, track already has one");
        bError = true;
    }
    if (!evEventall2)
    {
        if (NewEvent2)
        {
            evEventall2 = NewEvent2;
            asEventall2Name = "";
            iEvents |= 32; // sumaryczna informacja o eventach
        }
        else
        {
            if (!asEvent0Name.empty())
            { // Ra: tylko w logu informacja
                WriteLog("Eventall2 \"" + asEventall2Name +
                         "\" does not exist");
                bError = true;
            }
        }
    }
    else
    {
        Error("Eventall2 cannot be assigned to track, track already has one");
        bError = true;
    }
    return !bError;
}

bool TTrack::AssignForcedEvents(TEvent *NewEventPlus, TEvent *NewEventMinus)
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

string TTrack::IsolatedName()
{ // podaje nazwę odcinka izolowanego, jesli nie ma on jeszcze przypisanych zdarzeń
    if (pIsolated)
        if (!pIsolated->evBusy && !pIsolated->evFree)
            return pIsolated->asName;
    return "";
};

bool TTrack::IsolatedEventsAssign(TEvent *busy, TEvent *free)
{ // ustawia zdarzenia dla odcinka izolowanego
    if (pIsolated)
    {
        if (busy)
            pIsolated->evBusy = busy;
        if (free)
            pIsolated->evFree = free;
        return true;
    }
    return false;
};

// ABu: przeniesione z Track.h i poprawione!!!
bool TTrack::AddDynamicObject(TDynamicObject *Dynamic)
{ // dodanie pojazdu do trajektorii
    // Ra: tymczasowo wysyłanie informacji o zajętości konkretnego toru
    // Ra: usunąć po upowszechnieniu się odcinków izolowanych
    if (iCategoryFlag & 0x100) // jeśli usuwaczek
    {
        Dynamic->MyTrack = NULL; // trzeba by to uzależnić od kierunku ruchu...
        return true;
    }
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    if (Global::iMultiplayer) // jeśli multiplayer
        if (!iNumDynamics) // pierwszy zajmujący
            if (pMyNode->asName != "none")
                Global::pGround->WyslijString(pMyNode->asName,
                                              8); // przekazanie informacji o zajętości toru
    if (iNumDynamics < iMaxNumDynamics)
    { // jeśli jest miejsce, dajemy na koniec
        Dynamics[iNumDynamics++] = Dynamic;
        Dynamic->MyTrack = this; // ABu: na ktorym torze jesteśmy
        if (Dynamic->iOverheadMask) // jeśli ma pantografy
            Dynamic->OverheadTrack(
                fOverhead); // przekazanie informacji o jeździe bezprądowej na tym odcinku toru
        return true;
    }
    else
    {
        Error("Too many dynamics on track " + pMyNode->asName);
        return false;
    }
#else
    if( Global::iMultiplayer ) {
        // jeśli multiplayer
        if( true == Dynamics.empty() ) {
            // pierwszy zajmujący
            if( pMyNode->asName != "none" ) {
                // przekazanie informacji o zajętości toru
                Global::pGround->WyslijString( pMyNode->asName, 8 );
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
#endif
};

void TTrack::MoveMe(vector3 pPosition)
{ // to nie jest używane
    if (SwitchExtension)
    {
        SwitchExtension->Segments[0]->MoveMe(1 * pPosition);
        SwitchExtension->Segments[1]->MoveMe(1 * pPosition);
        SwitchExtension->Segments[2]->MoveMe(3 * pPosition); // Ra: 3 razy?
        SwitchExtension->Segments[3]->MoveMe(4 * pPosition);
    }
    else
    {
        Segment->MoveMe(pPosition);
    };
    ResourceManager::Unregister(this);
};

const int numPts = 4;
const int nnumPts = 12;
/*
const vector6 szyna[nnumPts]= //szyna - vextor6(x,y,mapowanie tekstury,xn,yn,zn)
{pierwotna szyna, opracował youBy, zmiany w celu uzyskania symetrii
 vector6( 0.111,-0.180,0.00, 1.000, 0.000,0.000),
 vector6( 0.045,-0.155,0.15, 0.707, 0.707,0.000),
 vector6( 0.045,-0.070,0.25, 0.707,-0.707,0.000),
 vector6( 0.071,-0.040,0.35, 0.707,-0.707,0.000), //albo tu 0.073
 vector6( 0.072,-0.010,0.40, 0.707, 0.707,0.000),
 vector6( 0.052,-0.000,0.45, 0.000, 1.000,0.000),
 vector6( 0.020,-0.000,0.55, 0.000, 1.000,0.000),
 vector6( 0.000,-0.010,0.60,-0.707, 0.707,0.000),
 vector6( 0.001,-0.040,0.65,-0.707,-0.707,0.000), //albo tu -0.001
 vector6( 0.027,-0.070,0.75,-0.707,-0.707,0.000), //albo zostanie asymetryczna
 vector6( 0.027,-0.155,0.85,-0.707, 0.707,0.000),
 vector6(-0.039,-0.180,1.00,-1.000, 0.000,0.000)
};
*/
const vector6 szyna[nnumPts] = // szyna - vextor6(x,y,mapowanie tekstury,xn,yn,zn)
    { // tę wersję opracował Tolein (bez pochylenia)
     vector6(0.111, -0.180, 0.00, 1.000, 0.000, 0.000),
     vector6(0.046, -0.150, 0.15, 0.707, 0.707, 0.000),
     vector6(0.044, -0.050, 0.25, 0.707, -0.707, 0.000),
     vector6(0.073, -0.038, 0.35, 0.707, -0.707, 0.000),
     vector6(0.072, -0.010, 0.40, 0.707, 0.707, 0.000),
     vector6(0.052, -0.000, 0.45, 0.000, 1.000, 0.000),
     vector6(0.020, -0.000, 0.55, 0.000, 1.000, 0.000),
     vector6(0.000, -0.010, 0.60, -0.707, 0.707, 0.000),
     vector6(-0.001, -0.038, 0.65, -0.707, -0.707, 0.000),
     vector6(0.028, -0.050, 0.75, -0.707, -0.707, 0.000),
     vector6(0.026, -0.150, 0.85, -0.707, 0.707, 0.000),
     vector6(-0.039, -0.180, 1.00, -1.000, 0.000, 0.000)};

const vector6 iglica[nnumPts] = // iglica - vextor3(x,y,mapowanie tekstury)
    {
     vector6(0.010, -0.180, 0.00, 1.000, 0.000, 0.000),
     vector6(0.010, -0.155, 0.15, 1.000, 0.000, 0.000),
     vector6(0.010, -0.070, 0.25, 1.000, 0.000, 0.000),
     vector6(0.010, -0.040, 0.35, 1.000, 0.000, 0.000),
     vector6(0.010, -0.010, 0.40, 1.000, 0.000, 0.000),
     vector6(0.010, -0.000, 0.45, 0.707, 0.707, 0.000),
     vector6(0.000, -0.000, 0.55, 0.707, 0.707, 0.000),
     vector6(0.000, -0.010, 0.60, -1.000, 0.000, 0.000),
     vector6(0.000, -0.040, 0.65, -1.000, 0.000, 0.000),
     vector6(0.000, -0.070, 0.75, -1.000, 0.000, 0.000),
     vector6(0.000, -0.155, 0.85, -0.707, 0.707, 0.000),
     vector6(-0.040, -0.180, 1.00, -1.000, 0.000,
             0.000) // 1mm więcej, żeby nie nachodziły tekstury?
};

void TTrack::Compile(GLuint tex)
{ // generowanie treści dla Display Lists - model proceduralny
    if (!tex)
    { // jeśli nie podana tekstura, to każdy tor ma wlasne DL
        if (DisplayListID)
            Release(); // zwolnienie zasobów w celu ponownego utworzenia
        if (Global::bManageNodes)
        {
            DisplayListID = glGenLists(1); // otwarcie nowej listy
            glNewList(DisplayListID, GL_COMPILE);
        };
    }
    glColor3f(1.0f, 1.0f, 1.0f); // to tutaj potrzebne?
    // Ra: nie zmieniamy oświetlenia przy kompilowaniu, ponieważ ono się zmienia w czasie!
    // trochę podliczonych zmiennych, co się potem przydadzą
    double fHTW = 0.5 * fabs(fTrackWidth); // połowa szerokości
    double side = fabs(fTexWidth); // szerokść podsypki na zewnątrz szyny albo pobocza
    double slop = fabs(fTexSlope); // szerokość pochylenia
    double rozp = fHTW + side + slop; // brzeg zewnętrzny
    double hypot1 = hypot(slop, fTexHeight1); // rozmiar pochylenia do liczenia normalnych
    if (hypot1 == 0.0)
        hypot1 = 1.0;
    vector3 normal1 = vector3(fTexSlope / hypot1, fTexHeight1 / hypot1, 0.0); // wektor normalny
    double fHTW2, side2, slop2, rozp2, fTexHeight2, hypot2;
    vector3 normal2;
    if (iTrapezoid & 2) // ten bit oznacza, że istnieje odpowiednie pNext
    { // Ra: jest OK
        fHTW2 = 0.5 * fabs(trNext->fTrackWidth); // połowa rozstawu/nawierzchni
        side2 = fabs(trNext->fTexWidth);
        slop2 = fabs(trNext->fTexSlope);
        rozp2 = fHTW2 + side2 + slop2; // szerokość podstawy
        fTexHeight2 = trNext->fTexHeight1;
        hypot2 = hypot(slop2, fTexHeight2);
        if (hypot2 == 0.0)
            hypot2 = 1.0;
        normal2 = vector3(trNext->fTexSlope / hypot2, fTexHeight2 / hypot2, 0.0);
    }
    else // gdy nie ma następnego albo jest nieodpowiednim końcem podpięty
    {
        fHTW2 = fHTW;
        side2 = side;
        slop2 = slop;
        rozp2 = rozp;
        fTexHeight2 = fTexHeight1;
        hypot2 = hypot1;
        normal2 = normal1;
    }
    double roll1, roll2;
    switch (iCategoryFlag & 15)
    {
    case 1: // tor
    {
        Segment->GetRolls(roll1, roll2);
        double sin1 = sin(roll1), cos1 = cos(roll1), sin2 = sin(roll2), cos2 = cos(roll2);
        // zwykla szyna: //Ra: czemu główki są asymetryczne na wysokości 0.140?
        vector6 rpts1[24], rpts2[24], rpts3[24], rpts4[24];
        int i;
        for (i = 0; i < 12; ++i)
        {
            rpts1[i] = vector6((fHTW + szyna[i].x) * cos1 + szyna[i].y * sin1,
                               -(fHTW + szyna[i].x) * sin1 + szyna[i].y * cos1, szyna[i].z,
                               +szyna[i].n.x * cos1 + szyna[i].n.y * sin1,
                               -szyna[i].n.x * sin1 + szyna[i].n.y * cos1, 0.0);
            rpts2[11 - i] = vector6((-fHTW - szyna[i].x) * cos1 + szyna[i].y * sin1,
                                    -(-fHTW - szyna[i].x) * sin1 + szyna[i].y * cos1, szyna[i].z,
                                    -szyna[i].n.x * cos1 + szyna[i].n.y * sin1,
                                    +szyna[i].n.x * sin1 + szyna[i].n.y * cos1, 0.0);
        }
        if (iTrapezoid) // jak trapez albo przechyłki, to oddzielne punkty na końcu
            for (i = 0; i < 12; ++i)
            {
                rpts1[12 + i] = vector6((fHTW2 + szyna[i].x) * cos2 + szyna[i].y * sin2,
                                        -(fHTW2 + szyna[i].x) * sin2 + szyna[i].y * cos2,
                                        szyna[i].z, +szyna[i].n.x * cos2 + szyna[i].n.y * sin2,
                                        -szyna[i].n.x * sin2 + szyna[i].n.y * cos2, 0.0);
                rpts2[23 - i] = vector6((-fHTW2 - szyna[i].x) * cos2 + szyna[i].y * sin2,
                                        -(-fHTW2 - szyna[i].x) * sin2 + szyna[i].y * cos2,
                                        szyna[i].z, -szyna[i].n.x * cos2 + szyna[i].n.y * sin2,
                                        +szyna[i].n.x * sin2 + szyna[i].n.y * cos2, 0.0);
            }
        switch (eType) // dalej zależnie od typu
        {
        case tt_Table: // obrotnica jak zwykły tor, animacja wykonywana w RaAnimate(), tutaj tylko
        // regeneracja siatek
        case tt_Normal:
            if (TextureID2)
                if (tex ? TextureID2 == tex : true) // jeśli pasuje do grupy (tex)
                { // podsypka z podkładami jest tylko dla zwykłego toru
                    vector6
                        bpts1[8]; // punkty głównej płaszczyzny nie przydają się do robienia boków
                    if (fTexLength ==
                        4.0) // jeśli stare mapowanie na profil 0.2 0.5 1.1 (również 6-9-9/noil)
                    { // stare mapowanie z różną gęstością pikseli i oddzielnymi teksturami na każdy
                        // profil
                        if (iTrapezoid) // trapez albo przechyłki
                        { // podsypka z podkladami trapezowata
                            // ewentualnie poprawić mapowanie, żeby środek mapował się na
                            // 1.435/4.671 ((0.3464,0.6536)
                            // bo się tekstury podsypki rozjeżdżają po zmianie proporcji profilu
                            bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.00, normal1.x,
                                               -normal1.y, 0.0); // lewy brzeg
                            bpts1[1] = vector6((fHTW + side) * cos1, -(fHTW + side) * sin1 - 0.18,
                                               0.33, 0.0, 1.0, 0.0); // krawędź załamania
                            bpts1[2] =
                                vector6(-bpts1[1].x, +(fHTW + side) * sin1 - 0.18, 0.67, -normal1.x,
                                        -normal1.y, 0.0); // prawy brzeg początku symetrycznie
                            bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 1.00, -normal1.x,
                                               -normal1.y, 0.0); // prawy skos
                            // przekrój końcowy
                            bpts1[4] = vector6(rozp2, -fTexHeight2 - 0.18, 0.00, normal2.x,
                                               -normal2.y, 0.0); // lewy brzeg
                            bpts1[5] =
                                vector6((fHTW2 + side2) * cos2, -(fHTW2 + side2) * sin2 - 0.18,
                                        0.33, 0.0, 1.0, 0.0); // krawędź załamania
                            bpts1[6] = vector6(-bpts1[5].x, +(fHTW2 + side2) * sin2 - 0.18, 0.67,
                                               0.0, 1.0, 0.0); // prawy brzeg początku symetrycznie
                            bpts1[7] = vector6(-rozp2, -fTexHeight2 - 0.18, 1.00, -normal2.x,
                                               -normal2.y, 0.0); // prawy skos
                        }
                        else
                        {
                            bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.0, +normal1.x,
                                               -normal1.y, 0.0); // lewy brzeg
                            bpts1[1] = vector6(fHTW + side, -0.18, 0.33, +normal1.x, -normal1.y,
                                               0.0); // krawędź załamania
                            bpts1[2] = vector6(-fHTW - side, -0.18, 0.67, -normal1.x, -normal1.y,
                                               0.0); // druga
                            bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 1.0, -normal1.x,
                                               -normal1.y, 0.0); // prawy skos
                        }
                    }
                    else
                    { // mapowanie proporcjonalne do powierzchni, rozmiar w poprzek określa
                        // fTexLength
                        double max = fTexRatio2 * fTexLength; // szerokość proporcjonalna do
                        // długości
                        double map11 =
                            max > 0.0 ? (fHTW + side) / max : 0.25; // załamanie od strony 1
                        double map12 =
                            max > 0.0 ? (fHTW + side + hypot1) / max : 0.5; // brzeg od strony 1
                        if (iTrapezoid) // trapez albo przechyłki
                        { // podsypka z podkladami trapezowata
                            double map21 =
                                max > 0.0 ? (fHTW2 + side2) / max : 0.25; // załamanie od strony 2
                            double map22 = max > 0.0 ? (fHTW2 + side2 + hypot2) / max :
                                                       0.5; // brzeg od strony 2
                            // ewentualnie poprawić mapowanie, żeby środek mapował się na
                            // 1.435/4.671 ((0.3464,0.6536)
                            // bo się tekstury podsypki rozjeżdżają po zmianie proporcji profilu
                            bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.5 - map12, normal1.x,
                                               -normal1.y, 0.0); // lewy brzeg
                            bpts1[1] = vector6((fHTW + side) * cos1, -(fHTW + side) * sin1 - 0.18,
                                               0.5 - map11, 0.0, 1.0, 0.0); // krawędź załamania
                            bpts1[2] =
                                vector6(-bpts1[1].x, +(fHTW + side) * sin1 - 0.18, 0.5 + map11, 0.0,
                                        1.0, 0.0); // prawy brzeg początku symetrycznie
                            bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 0.5 + map12, -normal1.x,
                                               -normal1.y, 0.0); // prawy skos
                            // przekrój końcowy
                            bpts1[4] = vector6(rozp2, -fTexHeight2 - 0.18, 0.5 - map22, normal2.x,
                                               -normal2.y, 0.0); // lewy brzeg
                            bpts1[5] =
                                vector6((fHTW2 + side2) * cos2, -(fHTW2 + side2) * sin2 - 0.18,
                                        0.5 - map21, 0.0, 1.0, 0.0); // krawędź załamania
                            bpts1[6] =
                                vector6(-bpts1[5].x, +(fHTW2 + side2) * sin2 - 0.18, 0.5 + map21,
                                        0.0, 1.0, 0.0); // prawy brzeg początku symetrycznie
                            bpts1[7] = vector6(-rozp2, -fTexHeight2 - 0.18, 0.5 + map22, -normal2.x,
                                               -normal2.y, 0.0); // prawy skos
                        }
                        else
                        {
                            bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.5 - map12, +normal1.x,
                                               -normal1.y, 0.0); // lewy brzeg
                            bpts1[1] = vector6(fHTW + side, -0.18, 0.5 - map11, +normal1.x,
                                               -normal1.y, 0.0); // krawędź załamania
                            bpts1[2] = vector6(-fHTW - side, -0.18, 0.5 + map11, -normal1.x,
                                               -normal1.y, 0.0); // druga
                            bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 0.5 + map12, -normal1.x,
                                               -normal1.y, 0.0); // prawy skos
                        }
                    }
                    if (!tex)
                        glBindTexture(GL_TEXTURE_2D, TextureID2);
                    Segment->RenderLoft(bpts1, iTrapezoid ? -4 : 4, fTexLength);
                }
            if (TextureID1)
                if (tex ? TextureID1 == tex : true) // jeśli pasuje do grupy (tex)
                { // szyny
                    if (!tex)
                        glBindTexture(GL_TEXTURE_2D, TextureID1);
                    Segment->RenderLoft(rpts1, iTrapezoid ? -nnumPts : nnumPts, fTexLength);
                    Segment->RenderLoft(rpts2, iTrapezoid ? -nnumPts : nnumPts, fTexLength);
                }
            break;
        case tt_Switch: // dla zwrotnicy dwa razy szyny
            if (TextureID1) // zwrotnice nie są grupowane, aby prościej było je animować
            { // iglice liczone tylko dla zwrotnic
                // Ra: TODO: oddzielna animacja każdej iglicy, opór na docisku
                vector6 rpts3[24], rpts4[24];
                for (i = 0; i < 12; ++i)
                {
                    rpts3[i] = vector6((fHTW + iglica[i].x) * cos1 + iglica[i].y * sin1,
                                       -(fHTW + iglica[i].x) * sin1 + iglica[i].y * cos1,
                                       iglica[i].z, +iglica[i].n.x * cos1 + iglica[i].n.y * sin1,
                                       -iglica[i].n.x * sin1 + iglica[i].n.y * cos1, 0.0);
                    rpts4[11 - i] =
                        vector6((-fHTW - iglica[i].x) * cos1 + iglica[i].y * sin1,
                                -(-fHTW - iglica[i].x) * sin1 + iglica[i].y * cos1, iglica[i].z,
                                -iglica[i].n.x * cos1 + iglica[i].n.y * sin1,
                                +iglica[i].n.x * sin1 + iglica[i].n.y * cos1, 0.0);
                }
                if (iTrapezoid) // trapez albo przechyłki, to oddzielne punkty na końcu
                    for (i = 0; i < 12; ++i)
                    {
                        rpts3[12 + i] =
                            vector6((fHTW2 + iglica[i].x) * cos2 + iglica[i].y * sin2,
                                    -(fHTW2 + iglica[i].x) * sin2 + iglica[i].y * cos2, iglica[i].z,
                                    +iglica[i].n.x * cos2 + iglica[i].n.y * sin2,
                                    -iglica[i].n.x * sin2 + iglica[i].n.y * cos2, 0.0);
                        rpts4[23 - i] =
                            vector6((-fHTW2 - iglica[i].x) * cos2 + iglica[i].y * sin2,
                                    -(-fHTW2 - iglica[i].x) * sin2 + iglica[i].y * cos2,
                                    iglica[i].z, -iglica[i].n.x * cos2 + iglica[i].n.y * sin2,
                                    +iglica[i].n.x * sin2 + iglica[i].n.y * cos2, 0.0);
                    }
                // McZapkie-130302 - poprawione rysowanie szyn
                if (SwitchExtension->RightSwitch)
                { // zwrotnica prawa
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    SwitchExtension->Segments[0]->RenderLoft(rpts1, nnumPts, fTexLength,
                                                             2); // prawa szyna za iglicą
                    SwitchExtension->Segments[0]->RenderSwitchRail(
                        rpts1, rpts3, nnumPts, fTexLength, 2,
                        SwitchExtension->fOffset2); // prawa iglica
                    SwitchExtension->Segments[0]->RenderLoft(
                        rpts2, nnumPts, fTexLength); // lewa szyna normalnie cała
                    if (TextureID2 != TextureID1) // nie wiadomo, czy OpenGL to optymalizuje
                        glBindTexture(GL_TEXTURE_2D, TextureID2);
                    SwitchExtension->Segments[1]->RenderLoft(
                        rpts1, nnumPts, fTexLength); // prawa szyna normalna cała
                    SwitchExtension->Segments[1]->RenderLoft(rpts2, nnumPts, fTexLength,
                                                             2); // lewa szyna za iglicą
                    SwitchExtension->Segments[1]->RenderSwitchRail(
                        rpts2, rpts4, nnumPts, fTexLength, 2,
                        -fMaxOffset + SwitchExtension->fOffset1); // lewa iglica
                }
                else
                { // lewa kiedyś działała lepiej niż prawa
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    SwitchExtension->Segments[0]->RenderLoft(
                        rpts1, nnumPts, fTexLength); // prawa szyna normalna cała
                    SwitchExtension->Segments[0]->RenderLoft(rpts2, nnumPts, fTexLength,
                                                             2); // lewa szyna za iglicą
                    SwitchExtension->Segments[0]->RenderSwitchRail(
                        rpts2, rpts4, nnumPts, fTexLength, 2,
                        -SwitchExtension->fOffset2); // lewa iglica
                    if (TextureID2 != TextureID1) // nie wiadomo, czy OpenGL to optymalizuje
                        glBindTexture(GL_TEXTURE_2D, TextureID2);
                    SwitchExtension->Segments[1]->RenderLoft(rpts1, nnumPts, fTexLength,
                                                             2); // prawa szyna za iglicą
                    SwitchExtension->Segments[1]->RenderSwitchRail(
                        rpts1, rpts3, nnumPts, fTexLength, 2,
                        fMaxOffset - SwitchExtension->fOffset1); // prawa iglica
                    SwitchExtension->Segments[1]->RenderLoft(
                        rpts2, nnumPts, fTexLength); // lewa szyna normalnie cała
                }
            }
            break;
        }
    } // koniec obsługi torów
    break;
    case 2: // McZapkie-260302 - droga - rendering
        // McZapkie:240702-zmieniony zakres widzialnosci
        switch (eType) // dalej zależnie od typu
        {
        case tt_Normal: // drogi proste, bo skrzyżowania osobno
        {
            vector6 bpts1[4]; // punkty głównej płaszczyzny przydają się do robienia boków
            if (TextureID1 || TextureID2) // punkty się przydadzą, nawet jeśli nawierzchni nie ma
            { // double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
                double max = fTexRatio1 * fTexLength; // test: szerokość proporcjonalna do długości
                double map1 = max > 0.0 ? fHTW / max : 0.5; // obcięcie tekstury od strony 1
                double map2 = max > 0.0 ? fHTW2 / max : 0.5; // obcięcie tekstury od strony 2
                if (iTrapezoid) // trapez albo przechyłki
                { // nawierzchnia trapezowata
                    Segment->GetRolls(roll1, roll2);
                    bpts1[0] = vector6(fHTW * cos(roll1), -fHTW * sin(roll1), 0.5 - map1,
                                       sin(roll1), cos(roll1), 0.0); // lewy brzeg początku
                    bpts1[1] = vector6(-bpts1[0].x, -bpts1[0].y, 0.5 + map1, -sin(roll1),
                                       cos(roll1), 0.0); // prawy brzeg początku symetrycznie
                    bpts1[2] = vector6(fHTW2 * cos(roll2), -fHTW2 * sin(roll2), 0.5 - map2,
                                       sin(roll2), cos(roll2), 0.0); // lewy brzeg końca
                    bpts1[3] = vector6(-bpts1[2].x, -bpts1[2].y, 0.5 + map2, -sin(roll2),
                                       cos(roll2), 0.0); // prawy brzeg początku symetrycznie
                }
                else
                {
                    bpts1[0] = vector6(fHTW, 0.0, 0.5 - map1, 0.0, 1.0, 0.0);
                    bpts1[1] = vector6(-fHTW, 0.0, 0.5 + map1, 0.0, 1.0, 0.0);
                }
            }
            if (TextureID1) // jeśli podana była tekstura, generujemy trójkąty
                if (tex ? TextureID1 == tex : true) // jeśli pasuje do grupy (tex)
                { // tworzenie trójkątów nawierzchni szosy
                    if (!tex)
                        glBindTexture(GL_TEXTURE_2D, TextureID1);
                    Segment->RenderLoft(bpts1, iTrapezoid ? -2 : 2, fTexLength);
                }
            if (TextureID2)
                if (tex ? TextureID2 == tex : true) // jeśli pasuje do grupy (tex)
                { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak
                    // w Midtown Madness 2?)
                    if (!tex)
                        glBindTexture(GL_TEXTURE_2D, TextureID2);
                    vector6 rpts1[6],
                        rpts2[6]; // współrzędne przekroju i mapowania dla prawej i lewej strony
                    if (fTexHeight1 >= 0.0)
                    { // standardowo: od zewnątrz pochylenie, a od wewnątrz poziomo
                        rpts1[0] = vector6(rozp, -fTexHeight1, 0.0); // lewy brzeg podstawy
                        rpts1[1] =
                            vector6(bpts1[0].x + side, bpts1[0].y, 0.5); // lewa krawędź załamania
                        rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                           1.0); // lewy brzeg pobocza (mapowanie może być inne
                        rpts2[0] = vector6(bpts1[1].x, bpts1[1].y, 1.0); // prawy brzeg pobocza
                        rpts2[1] =
                            vector6(bpts1[1].x - side, bpts1[1].y, 0.5); // prawa krawędź załamania
                        rpts2[2] = vector6(-rozp, -fTexHeight1, 0.0); // prawy brzeg podstawy
                        if (iTrapezoid) // trapez albo przechyłki
                        { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                            // odcinka
                            rpts1[3] =
                                vector6(rozp2, -fTexHeight2, 0.0); // lewy brzeg lewego pobocza
                            rpts1[4] =
                                vector6(bpts1[2].x + side2, bpts1[2].y, 0.5); // krawędź załamania
                            rpts1[5] = vector6(bpts1[2].x, bpts1[2].y, 1.0); // brzeg pobocza
                            rpts2[3] = vector6(bpts1[3].x, bpts1[3].y, 1.0);
                            rpts2[4] = vector6(bpts1[3].x - side2, bpts1[3].y, 0.5);
                            rpts2[5] =
                                vector6(-rozp2, -fTexHeight2, 0.0); // prawy brzeg prawego pobocza
                        }
                    }
                    else
                    { // wersja dla chodnika: skos 1:3.75, każdy chodnik innej szerokości
                        // mapowanie propocjonalne do szerokości chodnika
                        // krawężnik jest mapowany od 31/64 do 32/64 lewy i od 32/64 do 33/64 prawy
                        double d =
                            -fTexHeight1 / 3.75; // krawężnik o wysokości 150mm jest pochylony 40mm
                        double max =
                            fTexRatio2 * fTexLength; // test: szerokość proporcjonalna do długości
                        double map1l = max > 0.0 ?
                                           side / max :
                                           0.484375; // obcięcie tekstury od lewej strony punktu 1
                        double map1r = max > 0.0 ?
                                           slop / max :
                                           0.484375; // obcięcie tekstury od prawej strony punktu 1
                        rpts1[0] = vector6(bpts1[0].x + slop, bpts1[0].y - fTexHeight1,
                                           0.515625 + map1r); // prawy brzeg prawego chodnika
                        rpts1[1] = vector6(bpts1[0].x + d, bpts1[0].y - fTexHeight1,
                                           0.515625); // prawy krawężnik u góry
                        rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                           0.515625 - d / 2.56); // prawy krawężnik u dołu
                        rpts2[0] = vector6(bpts1[1].x, bpts1[1].y,
                                           0.484375 + d / 2.56); // lewy krawężnik u dołu
                        rpts2[1] = vector6(bpts1[1].x - d, bpts1[1].y - fTexHeight1,
                                           0.484375); // lewy krawężnik u góry
                        rpts2[2] = vector6(bpts1[1].x - side, bpts1[1].y - fTexHeight1,
                                           0.484375 - map1l); // lewy brzeg lewego chodnika
                        if (iTrapezoid) // trapez albo przechyłki
                        { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                            // odcinka
                            slop2 = fabs((iTrapezoid & 2) ? slop2 :
                                                            slop); // szerokość chodnika po prawej
                            double map2l =
                                max > 0.0 ? side2 / max :
                                            0.484375; // obcięcie tekstury od lewej strony punktu 2
                            double map2r =
                                max > 0.0 ? slop2 / max :
                                            0.484375; // obcięcie tekstury od prawej strony punktu 2
                            rpts1[3] = vector6(bpts1[2].x + slop2, bpts1[2].y - fTexHeight2,
                                               0.515625 + map2r); // prawy brzeg prawego chodnika
                            rpts1[4] = vector6(bpts1[2].x + d, bpts1[2].y - fTexHeight2,
                                               0.515625); // prawy krawężnik u góry
                            rpts1[5] = vector6(bpts1[2].x, bpts1[2].y,
                                               0.515625 - d / 2.56); // prawy krawężnik u dołu
                            rpts2[3] = vector6(bpts1[3].x, bpts1[3].y,
                                               0.484375 + d / 2.56); // lewy krawężnik u dołu
                            rpts2[4] = vector6(bpts1[3].x - d, bpts1[3].y - fTexHeight2,
                                               0.484375); // lewy krawężnik u góry
                            rpts2[5] = vector6(bpts1[3].x - side2, bpts1[3].y - fTexHeight2,
                                               0.484375 - map2l); // lewy brzeg lewego chodnika
                        }
                    }
                    if (iTrapezoid) // trapez albo przechyłki
                    { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                        // odcinka
                        if ((fTexHeight1 >= 0.0) ? true : (slop != 0.0))
                            Segment->RenderLoft(rpts1, -3, fTexLength); // tylko jeśli jest z prawej
                        if ((fTexHeight1 >= 0.0) ? true : (side != 0.0))
                            Segment->RenderLoft(rpts2, -3, fTexLength); // tylko jeśli jest z lewej
                    }
                    else
                    { // pobocza zwykłe, brak przechyłki
                        if ((fTexHeight1 >= 0.0) ? true : (slop != 0.0))
                            Segment->RenderLoft(rpts1, 3, fTexLength);
                        if ((fTexHeight1 >= 0.0) ? true : (side != 0.0))
                            Segment->RenderLoft(rpts2, 3, fTexLength);
                    }
                }
            break;
        }
        case tt_Cross: // skrzyżowanie dróg rysujemy inaczej
        { // ustalenie współrzędnych środka - przecięcie Point1-Point2 z CV4-Point4
            double a[4]; // kąty osi ulic wchodzących
            vector3 p[4]; // punkty się przydadzą do obliczeń
            // na razie połowa odległości pomiędzy Point1 i Point2, potem się dopracuje
            a[0] = a[1] = 0.5; // parametr do poszukiwania przecięcia łuków
            // modyfikować a[0] i a[1] tak, aby trafić na przecięcie odcinka 34
            p[0] = SwitchExtension->Segments[0]->FastGetPoint(
                a[0]); // współrzędne środka pierwszego odcinka
            p[1] = SwitchExtension->Segments[1]->FastGetPoint(a[1]); //-//- drugiego
            // p[2]=p[1]-p[0]; //jeśli różne od zera, przeliczyć a[0] i a[1] i wyznaczyć nowe punkty
            vector3 oxz = p[0]; // punkt mapowania środka tekstury skrzyżowania
            p[0] = SwitchExtension->Segments[0]
                       ->GetDirection1(); // Point1 - pobranie wektorów kontrolnych
            p[1] = SwitchExtension->Segments[1]->GetDirection2(); // Point3 (bo zamienione)
            p[2] = SwitchExtension->Segments[0]->GetDirection2(); // Point2
            p[3] = SwitchExtension->Segments[1]->GetDirection1(); // Point4 (bo zamienione)
            a[0] = atan2(-p[0].x, p[0].z); // kąty stycznych osi dróg
            a[1] = atan2(-p[1].x, p[1].z);
            a[2] = atan2(-p[2].x, p[2].z);
            a[3] = atan2(-p[3].x, p[3].z);
            p[0] = SwitchExtension->Segments[0]
                       ->FastGetPoint_0(); // Point1 - pobranie współrzędnych końców
            p[1] = SwitchExtension->Segments[1]->FastGetPoint_1(); // Point3
            p[2] = SwitchExtension->Segments[0]->FastGetPoint_1(); // Point2
            p[3] = SwitchExtension->Segments[1]
                       ->FastGetPoint_0(); // Point4 - przy trzech drogach pokrywa się z Point1
            // 2014-07: na początek rysować brzegi jak dla łuków
            // punkty brzegu nawierzchni uzyskujemy podczas renderowania boków (bez sensu, ale
            // najszybciej było zrobić)
            int i, j; // ile punktów (może byc różna ilość punktów między drogami)
            if (!SwitchExtension->vPoints)
            { // jeśli tablica punktów nie jest jeszcze utworzona, zliczamy punkty i tworzymy ją
                if (SwitchExtension->iRoads == 3) // mogą być tylko 3 drogi zamiast 4
                    SwitchExtension->iPoints = 5 + SwitchExtension->Segments[0]->RaSegCount() +
                                               SwitchExtension->Segments[1]->RaSegCount() +
                                               SwitchExtension->Segments[2]->RaSegCount();
                else
                    SwitchExtension->iPoints =
                        5 + SwitchExtension->Segments[2]->RaSegCount() +
                        SwitchExtension->Segments[3]->RaSegCount() +
                        SwitchExtension->Segments[4]->RaSegCount() +
                        SwitchExtension->Segments[5]->RaSegCount(); // mogą być tylko 3 drogi
                SwitchExtension->vPoints =
                    new vector3[SwitchExtension->iPoints]; // tablica utworzona z zapasem, ale nie
                // wypełniona współrzędnymi
            }
            vector3 *b =
                SwitchExtension->bPoints ?
                    NULL :
                    SwitchExtension
                        ->vPoints; // zmienna robocza, NULL gdy tablica punktów już jest wypełniona
            vector6 bpts1[4]; // punkty głównej płaszczyzny przydają się do robienia boków
            if (TextureID1 || TextureID2) // punkty się przydadzą, nawet jeśli nawierzchni nie ma
            { // double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
                double max = fTexRatio1 * fTexLength; // test: szerokość proporcjonalna do długości
                double map1 = max > 0.0 ? fHTW / max : 0.5; // obcięcie tekstury od strony 1
                double map2 = max > 0.0 ? fHTW2 / max : 0.5; // obcięcie tekstury od strony 2
                // if (iTrapezoid) //trapez albo przechyłki
                { // nawierzchnia trapezowata
                    Segment->GetRolls(roll1, roll2);
                    bpts1[0] = vector6(fHTW * cos(roll1), -fHTW * sin(roll1), 0.5 - map1,
                                       sin(roll1), cos(roll1), 0.0); // lewy brzeg początku
                    bpts1[1] = vector6(-bpts1[0].x, -bpts1[0].y, 0.5 + map1, -sin(roll1),
                                       cos(roll1), 0.0); // prawy brzeg początku symetrycznie
                    bpts1[2] = vector6(fHTW2 * cos(roll2), -fHTW2 * sin(roll2), 0.5 - map2,
                                       sin(roll2), cos(roll2), 0.0); // lewy brzeg końca
                    bpts1[3] = vector6(-bpts1[2].x, -bpts1[2].y, 0.5 + map2, -sin(roll2),
                                       cos(roll2), 0.0); // prawy brzeg początku symetrycznie
                }
            }
            // najpierw renderowanie poboczy i zapamiętywanie punktów
            // problem ze skrzyżowaniami jest taki, że teren chce się pogrupować wg tekstur, ale
            // zaczyna od nawierzchni
            // sama nawierzchnia nie wypełni tablicy punktów, bo potrzebne są pobocza
            // ale pobocza renderują się później, więc nawierzchnia nie załapuje się na renderowanie
            // w swoim czasie
            // if (TextureID2)
            // if (tex?TextureID2==tex:true) //jeśli pasuje do grupy (tex)
            { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak w
                // Midtown Madness 2?)
                if (TextureID2)
                    if (!tex)
                        glBindTexture(GL_TEXTURE_2D, TextureID2);
                vector6 rpts1[6],
                    rpts2[6]; // współrzędne przekroju i mapowania dla prawej i lewej strony
                // Ra 2014-07: trzeba to przerobić na pętlę i pobierać profile (przynajmniej 2..4) z
                // sąsiednich dróg
                if (fTexHeight1 >= 0.0)
                { // standardowo: od zewnątrz pochylenie, a od wewnątrz poziomo
                    rpts1[0] = vector6(rozp, -fTexHeight1, 0.0); // lewy brzeg podstawy
                    rpts1[1] = vector6(bpts1[0].x + side, bpts1[0].y, 0.5); // lewa krawędź
                    // załamania
                    rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                       1.0); // lewy brzeg pobocza (mapowanie może być inne
                    rpts2[0] = vector6(bpts1[1].x, bpts1[1].y, 1.0); // prawy brzeg pobocza
                    rpts2[1] =
                        vector6(bpts1[1].x - side, bpts1[1].y, 0.5); // prawa krawędź załamania
                    rpts2[2] = vector6(-rozp, -fTexHeight1, 0.0); // prawy brzeg podstawy
                    // if (iTrapezoid) //trapez albo przechyłki
                    { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                        // odcinka
                        rpts1[3] = vector6(rozp2, -fTexHeight2, 0.0); // lewy brzeg lewego pobocza
                        rpts1[4] = vector6(bpts1[2].x + side2, bpts1[2].y, 0.5); // krawędź
                        // załamania
                        rpts1[5] = vector6(bpts1[2].x, bpts1[2].y, 1.0); // brzeg pobocza
                        rpts2[3] = vector6(bpts1[3].x, bpts1[3].y, 1.0);
                        rpts2[4] = vector6(bpts1[3].x - side2, bpts1[3].y, 0.5);
                        rpts2[5] = vector6(-rozp2, -fTexHeight2, 0.0); // prawy brzeg prawego
                        // pobocza
                    }
                }
                else
                { // wersja dla chodnika: skos 1:3.75, każdy chodnik innej szerokości
                    // mapowanie propocjonalne do szerokości chodnika
                    // krawężnik jest mapowany od 31/64 do 32/64 lewy i od 32/64 do 33/64 prawy
                    double d =
                        -fTexHeight1 / 3.75; // krawężnik o wysokości 150mm jest pochylony 40mm
                    double max =
                        fTexRatio2 * fTexLength; // test: szerokość proporcjonalna do długości
                    double map1l = max > 0.0 ?
                                       side / max :
                                       0.484375; // obcięcie tekstury od lewej strony punktu 1
                    double map1r = max > 0.0 ?
                                       slop / max :
                                       0.484375; // obcięcie tekstury od prawej strony punktu 1
                    rpts1[0] = vector6(bpts1[0].x + slop, bpts1[0].y - fTexHeight1,
                                       0.515625 + map1r); // prawy brzeg prawego chodnika
                    rpts1[1] = vector6(bpts1[0].x + d, bpts1[0].y - fTexHeight1,
                                       0.515625); // prawy krawężnik u góry
                    rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                       0.515625 - d / 2.56); // prawy krawężnik u dołu
                    rpts2[0] = vector6(bpts1[1].x, bpts1[1].y,
                                       0.484375 + d / 2.56); // lewy krawężnik u dołu
                    rpts2[1] = vector6(bpts1[1].x - d, bpts1[1].y - fTexHeight1,
                                       0.484375); // lewy krawężnik u góry
                    rpts2[2] = vector6(bpts1[1].x - side, bpts1[1].y - fTexHeight1,
                                       0.484375 - map1l); // lewy brzeg lewego chodnika
                    // if (iTrapezoid) //trapez albo przechyłki
                    { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                        // odcinka
                        slop2 =
                            fabs((iTrapezoid & 2) ? slop2 : slop); // szerokość chodnika po prawej
                        double map2l = max > 0.0 ?
                                           side2 / max :
                                           0.484375; // obcięcie tekstury od lewej strony punktu 2
                        double map2r = max > 0.0 ?
                                           slop2 / max :
                                           0.484375; // obcięcie tekstury od prawej strony punktu 2
                        rpts1[3] = vector6(bpts1[2].x + slop2, bpts1[2].y - fTexHeight2,
                                           0.515625 + map2r); // prawy brzeg prawego chodnika
                        rpts1[4] = vector6(bpts1[2].x + d, bpts1[2].y - fTexHeight2,
                                           0.515625); // prawy krawężnik u góry
                        rpts1[5] = vector6(bpts1[2].x, bpts1[2].y,
                                           0.515625 - d / 2.56); // prawy krawężnik u dołu
                        rpts2[3] = vector6(bpts1[3].x, bpts1[3].y,
                                           0.484375 + d / 2.56); // lewy krawężnik u dołu
                        rpts2[4] = vector6(bpts1[3].x - d, bpts1[3].y - fTexHeight2,
                                           0.484375); // lewy krawężnik u góry
                        rpts2[5] = vector6(bpts1[3].x - side2, bpts1[3].y - fTexHeight2,
                                           0.484375 - map2l); // lewy brzeg lewego chodnika
                    }
                }
                bool render = TextureID2 ? (tex ? TextureID2 == tex : true) :
                                           false; // renderować nie trzeba, ale trzeba wyznaczyć
                // punkty brzegowe nawierzchni
                // if (iTrapezoid) //trapez albo przechyłki
                if (SwitchExtension->iRoads == 4)
                { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0))
                        SwitchExtension->Segments[2]->RenderLoft(
                            rpts2, -3, fTexLength, 0, 1, &b, render); // tylko jeśli jest z lewej
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0))
                        SwitchExtension->Segments[3]->RenderLoft(
                            rpts2, -3, fTexLength, 0, 1, &b, render); // tylko jeśli jest z lewej
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0))
                        SwitchExtension->Segments[4]->RenderLoft(
                            rpts2, -3, fTexLength, 0, 1, &b, render); // tylko jeśli jest z lewej
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0))
                        SwitchExtension->Segments[5]->RenderLoft(
                            rpts2, -3, fTexLength, 0, 1, &b, render); // tylko jeśli jest z lewej
                }
                else // to będzie ewentualnie dla prostego na skrzyżowaniu trzech dróg
                { // punkt 3 pokrywa się z punktem 1, jak w zwrotnicy; połączenie 1->2 nie musi być
                    // prostoliniowe
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0)) // OK
                        SwitchExtension->Segments[2]->RenderLoft(rpts2, -3, fTexLength, 0, 1, &b,
                                                                 render); // z P2 do P4
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0)) // OK
                        SwitchExtension->Segments[1]->RenderLoft(
                            rpts2, -3, fTexLength, 0, 1, &b, render); // z P4 do P3=P1 (odwrócony)
                    if ((fTexHeight1 >= 0.0) ? true : (side != 0.0)) // OK
                        SwitchExtension->Segments[0]->RenderLoft(rpts2, -3, fTexLength, 0, 1, &b,
                                                                 render); // z P1 do P2
                    /*
                    if ((fTexHeight1>=0.0)?true:(slop!=0.0))
                     Segment->RenderLoft(rpts1,3,fTexLength);
                    if ((fTexHeight1>=0.0)?true:(side!=0.0))
                     Segment->RenderLoft(rpts2,3,fTexLength);
                    */
                }
            }
            // renderowanie nawierzchni na końcu
            double sina0 = sin(a[0]), cosa0 = cos(a[0]);
            double u, v;
            if (!SwitchExtension->bPoints) // jeśli tablica nie wypełniona
                if (b) // ale jest wskaźnik do tablicy - może nie być?
                { // coś się gubi w obliczeniach na wskaźnikach
                    i = (int((void *)(b)) - int((void *)(SwitchExtension->vPoints))) /
                        sizeof(vector3); // ustalenie liczby punktów, bo mogło wyjść inaczej niż
                    // policzone z góry
                    if (i > 0)
                    { // jeśli zostało to właśnie utworzone
                        if (SwitchExtension->iPoints > i) // jeśli wyszło mniej niż było miejsca
                            SwitchExtension->iPoints = i; // domknięcie wachlarza
                        else
                            --SwitchExtension->iPoints; // jak tutaj wejdzie, to błąd jest - zrobić
                        // miejsce na powtórzenie pierwszego punktu
                        // na końcu
                        SwitchExtension->vPoints[SwitchExtension->iPoints++] =
                            SwitchExtension->vPoints[0];
                        SwitchExtension->bPoints = true; // tablica punktów została wypełniona
                    }
                }
            if (TextureID1) // jeśli podana tekstura nawierzchni
                if (tex ? TextureID1 == tex : true) // jeśli pasuje do grupy (tex)
                {
                    if (!tex)
                        glBindTexture(GL_TEXTURE_2D, TextureID1);
                    glBegin(GL_TRIANGLE_FAN); // takie kółeczko będzie
                    glNormal3f(0, 1, 0);
                    glTexCoord2f(0.5, 0.5); //środek tekstury na środku skrzyżowania
                    glVertex3f(oxz.x, oxz.y, oxz.z);
                    for (i = SwitchExtension->iPoints - 1; i >= 0; --i)
                    // for (i=0;i<SwitchExtension->iPoints;++i)
                    {
                        glNormal3f(0, 1, 0);
                        u = (SwitchExtension->vPoints[i].x - oxz.x) /
                            fTexLength; // mapowanie we współrzędnych scenerii
                        v = (SwitchExtension->vPoints[i].z - oxz.z) / (fTexRatio1 * fTexLength);
                        glTexCoord2f(cosa0 * u + sina0 * v + 0.5, -sina0 * u + cosa0 * v + 0.5);
                        glVertex3f(SwitchExtension->vPoints[i].x, SwitchExtension->vPoints[i].y,
                                   SwitchExtension->vPoints[i].z);
                    }
                    glEnd();
                }
            break;
        }
        }
        break;
    case 4: // McZapkie-260302 - rzeka- rendering
        // Ra: rzeki na razie bez zmian, przechyłki na pewno nie mają
        // Ra: przemyśleć wyrównanie u góry trawą do czworoboku
        vector6 bpts1[numPts] = {vector6(fHTW, 0.0, 0.0), vector6(fHTW, 0.2, 0.33),
                                 vector6(-fHTW, 0.0, 0.67), vector6(-fHTW, 0.0, 1.0)};
        // Ra: dziwnie ten kształt wygląda
        if (TextureID1)
            if (tex ? TextureID1 == tex : true) // jeśli pasuje do grupy (tex)
            {
                if (!tex)
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                Segment->RenderLoft(bpts1, numPts, fTexLength);
            }
        if (TextureID2)
            if (tex ? TextureID2 == tex : true) // jeśli pasuje do grupy (tex)
            { // brzegi rzeki prawie jak pobocze derogi, tylko inny znak ma wysokość
                // znak jest zmieniany przy wczytywaniu, więc tu musi byc minus fTexHeight
                vector6 rpts1[3] = {vector6(rozp, -fTexHeight1, 0.0),
                                    vector6(fHTW + side, 0.0, 0.5), vector6(fHTW, 0.0, 1.0)};
                vector6 rpts2[3] = {vector6(-fHTW, 0.0, 1.0), vector6(-fHTW - side, 0.0, 0.5),
                                    vector6(-rozp, -fTexHeight1, 0.0)}; // Ra: po kiego 0.1?
                if (!tex)
                    glBindTexture(GL_TEXTURE_2D, TextureID2); // brzeg rzeki
                Segment->RenderLoft(rpts1, 3, fTexLength);
                Segment->RenderLoft(rpts2, 3, fTexLength);
            }
        break;
    }
    if (!tex)
        if (Global::bManageNodes)
            glEndList();
};

void TTrack::Release()
{
    if (DisplayListID)
        glDeleteLists(DisplayListID, 1);
    DisplayListID = 0;
};

void TTrack::Render()
{
    if (bVisible) // Ra: tory są renderowane sektorami i nie ma sensu każdorazowo liczyć odległości
    {
        if (!DisplayListID)
        {
            Compile();
            if (Global::bManageNodes)
                ResourceManager::Register(this);
        };
        SetLastUsage(Timer::GetSimulationTime());
        EnvironmentSet(); // oświetlenie nie może być skompilowane, bo może się zmieniać z czasem
        glCallList(DisplayListID);
        EnvironmentReset(); // ustawienie oświetlenia na zwykłe
        if (InMovement())
            Release(); // zwrotnica w trakcie animacji do odrysowania
    };
//#ifdef _DEBUG
#if 0
 if (DebugModeFlag && ScannedFlag) //McZapkie-230702
 //if (iNumDynamics) //będzie kreska na zajętym torze
 {
  vector3 pos1,pos2,pos3;
  glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);
  glColor3ub(255,0,0);
  glBindTexture(GL_TEXTURE_2D,0);
  glBegin(GL_LINE_STRIP);
      pos1=Segment->FastGetPoint_0();
      pos2=Segment->FastGetPoint(0.5);
      pos3=Segment->FastGetPoint_1();
      glVertex3f(pos1.x,pos1.y,pos1.z);
      glVertex3f(pos2.x,pos2.y+10,pos2.z);
      glVertex3f(pos3.x,pos3.y,pos3.z);
  glEnd();
  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  ScannedFlag=false;
 }
#endif
    // glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
    // glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
    // glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
};

bool TTrack::CheckDynamicObject(TDynamicObject *Dynamic)
{ // sprawdzenie, czy pojazd jest przypisany do toru
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    for (int i = 0; i < iNumDynamics; i++)
        if (Dynamic == Dynamics[i])
            return true;
    return false;
#else
    for( auto dynamic : Dynamics ) {
        if( dynamic == Dynamic ) {
            return true;
        }
    }
    return false;
#endif
};

bool TTrack::RemoveDynamicObject(TDynamicObject *Dynamic)
{ // usunięcie pojazdu z listy przypisanych do toru
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    for (int i = 0; i < iNumDynamics; i++)
    { // sprawdzanie wszystkich po kolei
        if (Dynamic == Dynamics[i])
        { // znaleziony, przepisanie następnych, żeby dziur nie było
            --iNumDynamics;
            for (i; i < iNumDynamics; i++)
                Dynamics[i] = Dynamics[i + 1];
            if (Global::iMultiplayer) // jeśli multiplayer
                if (!iNumDynamics) // jeśli już nie ma żadnego
                    if (pMyNode->asName != "none")
                        Global::pGround->WyslijString(
                            pMyNode->asName, 9); // przekazanie informacji o zwolnieniu toru
            return true;
        }
    }
    Error("Cannot remove dynamic from track");
    return false;
#else
    bool result = false;
    if( *Dynamics.begin() == Dynamic ) {
        // most likely the object getting removed it's at the front...
        Dynamics.pop_front();
        result = true;
    }
    else if( *( --Dynamics.end() ) == Dynamic ) {
        // ...or the back of the queue...
        Dynamics.pop_back();
        result = true;
    }
    else {
        // ... if these fail, check all objects one by one, just in case
        // TODO: check if this is ever needed, remove if it isn't.
        for( auto idx = Dynamics.begin(); idx != Dynamics.end(); ++idx ) {
            if( *idx == Dynamic ) {
                Dynamics.erase( idx );
                result = true;
            }
        }
    }
    if( Global::iMultiplayer ) {
        // jeśli multiplayer
        if( true == Dynamics.empty() ) {
            // jeśli już nie ma żadnego
            if( pMyNode->asName != "none" ) {
                // przekazanie informacji o zwolnieniu toru
                Global::pGround->WyslijString( pMyNode->asName, 9 ); 
            }
        }
    }
    
    return result;
#endif
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
                TAnimContainer *ac =
                    SwitchExtension->pModel ? SwitchExtension->pModel->GetContainer(NULL) : NULL;
                return ac ?
                           (ac->AngleGet() != SwitchExtension->fOffset) ||
                               !(ac->TransGet() == SwitchExtension->vTrans) :
                           false;
                // return true; //jeśli jest taki obiekt
            }
    }
    return false;
};
void TTrack::RaAssign(TGroundNode *gn, TAnimContainer *ac){
    // Ra: wiązanie toru z modelem obrotnicy
    // if (eType==tt_Table) SwitchExtension->pAnim=p;
};
void TTrack::RaAssign(TGroundNode *gn, TAnimModel *am, TEvent *done, TEvent *joined)
{ // Ra: wiązanie toru z modelem obrotnicy
    if (eType == tt_Table)
    {
        SwitchExtension->pModel = am;
        SwitchExtension->pMyNode = gn;
        SwitchExtension->evMinus = done; // event zakończenia animacji (zadanie nowej przedłuża)
        SwitchExtension->evPlus =
            joined; // event potwierdzenia połączenia (gdy nie znajdzie, to się nie połączy)
        if (am)
            if (am->GetContainer(NULL)) // może nie być?
                am->GetContainer(NULL)->EventAssign(done); // zdarzenie zakończenia animacji
    }
};

int TTrack::RaArrayPrepare()
{ // przygotowanie tablic do skopiowania do VBO (zliczanie wierzchołków)
    if (bVisible) // o ile w ogóle widać
        switch (iCategoryFlag & 15)
        {
        case 1: // tor
            if (eType == tt_Switch) // dla zwrotnicy tylko szyny
                return 48 * ((TextureID1 ? SwitchExtension->Segments[0]->RaSegCount() : 0) +
                             (TextureID2 ? SwitchExtension->Segments[1]->RaSegCount() : 0));
            else // dla toru podsypka plus szyny
                return (Segment->RaSegCount()) * ((TextureID1 ? 48 : 0) + (TextureID2 ? 8 : 0));
        case 2: // droga
            if (eType == tt_Cross) // tylko dla skrzyżowania dróg
            { // specjalny sposób obliczania liczby wierzchołków w skrzyżowaniu
                // int n=0; //wierzchołki wewnętrzne do generowania nawierzchni
                // int b=0; //wierzchołki do generowania boków
                if (fTexHeight1 >= 0) // jeśli fTexHeight1<0, to są chodniki i może któregoś nie być
                { // normalne pobocze, na razie się składa z
                    return (Segment->RaSegCount()) * ((TextureID1 ? 4 : 0) + (TextureID2 ? 12 : 0));
                }
                else
                    return (Segment->RaSegCount()) *
                           ((TextureID1 ? 4 : 0) +
                            (TextureID2 ? (fTexWidth != 0.0 ? 6 : 0) + (fTexSlope != 0.0 ? 6 : 0) :
                                          0));
            }
            else // standardowo dla zwykłej drogi
                if (fTexHeight1 >= 0) // jeśli fTexHeight1<0, to są chodniki i może któregoś nie być
                return (Segment->RaSegCount()) *
                       ((TextureID1 ? 4 : 0) + (TextureID2 ? 12 : 0)); // może nie być poziomego!
            else
                return (Segment->RaSegCount()) *
                       ((TextureID1 ? 4 : 0) +
                        (TextureID2 ? (fTexWidth != 0.0 ? 6 : 0) + (fTexSlope != 0.0 ? 6 : 0) : 0));
        case 4: // rzeki do przemyślenia
            return (Segment->RaSegCount()) * ((TextureID1 ? 4 : 0) + (TextureID2 ? 12 : 0));
        }
    return 0;
};

void TTrack::RaArrayFill(CVertNormTex *Vert, const CVertNormTex *Start)
{ // wypełnianie tablic VBO
    // Ra: trzeba rozdzielić szyny od podsypki, aby móc grupować wg tekstur
    double fHTW = 0.5 * fabs(fTrackWidth);
    double side = fabs(fTexWidth); // szerokść podsypki na zewnątrz szyny albo pobocza
    double slop = fabs(fTexSlope); // brzeg zewnętrzny
    double rozp = fHTW + side + slop; // brzeg zewnętrzny
    double hypot1 = hypot(slop, fTexHeight1); // rozmiar pochylenia do liczenia normalnych
    if (hypot1 == 0.0)
        hypot1 = 1.0;
    vector3 normal1 = vector3(fTexSlope / hypot1, fTexHeight1 / hypot1, 0.0); // wektor normalny
    double fHTW2, side2, slop2, rozp2, fTexHeight2, hypot2;
    vector3 normal2;
    if (iTrapezoid & 2) // ten bit oznacza, że istnieje odpowiednie pNext
    { // Ra: jest OK
        fHTW2 = 0.5 * fabs(trNext->fTrackWidth); // połowa rozstawu/nawierzchni
        side2 = fabs(trNext->fTexWidth);
        slop2 = fabs(trNext->fTexSlope); // nie jest używane później
        rozp2 = fHTW2 + side2 + slop2;
        fTexHeight2 = trNext->fTexHeight1;
        hypot2 = hypot(slop2, fTexHeight2);
        if (hypot2 == 0.0)
            hypot2 = 1.0;
        normal2 = vector3(trNext->fTexSlope / hypot2, fTexHeight2 / hypot2, 0.0);
    }
    else // gdy nie ma następnego albo jest nieodpowiednim końcem podpięty
    {
        fHTW2 = fHTW;
        side2 = side;
        slop2 = slop;
        rozp2 = rozp;
        fTexHeight2 = fTexHeight1;
        hypot2 = hypot1;
        normal2 = normal1;
    }
    double roll1, roll2;
    switch (iCategoryFlag & 15)
    {
    case 1: // tor
    {
        if (Segment)
            Segment->GetRolls(roll1, roll2);
        else
            roll1 = roll2 = 0.0; // dla zwrotnic
        double sin1 = sin(roll1), cos1 = cos(roll1), sin2 = sin(roll2), cos2 = cos(roll2);
        // zwykla szyna: //Ra: czemu główki są asymetryczne na wysokości 0.140?
        vector6 rpts1[24], rpts2[24], rpts3[24], rpts4[24];
        int i;
        for (i = 0; i < 12; ++i)
        {
            rpts1[i] = vector6((fHTW + szyna[i].x) * cos1 + szyna[i].y * sin1,
                               -(fHTW + szyna[i].x) * sin1 + szyna[i].y * cos1, szyna[i].z,
                               +szyna[i].n.x * cos1 + szyna[i].n.y * sin1,
                               -szyna[i].n.x * sin1 + szyna[i].n.y * cos1, 0.0);
            rpts2[11 - i] = vector6((-fHTW - szyna[i].x) * cos1 + szyna[i].y * sin1,
                                    -(-fHTW - szyna[i].x) * sin1 + szyna[i].y * cos1, szyna[i].z,
                                    -szyna[i].n.x * cos1 + szyna[i].n.y * sin1,
                                    +szyna[i].n.x * sin1 + szyna[i].n.y * cos1, 0.0);
        }
        if (iTrapezoid) // trapez albo przechyłki, to oddzielne punkty na końcu
            for (i = 0; i < 12; ++i)
            {
                rpts1[12 + i] = vector6((fHTW2 + szyna[i].x) * cos2 + szyna[i].y * sin2,
                                        -(fHTW2 + szyna[i].x) * sin2 + szyna[i].y * cos2,
                                        szyna[i].z, +szyna[i].n.x * cos2 + szyna[i].n.y * sin2,
                                        -szyna[i].n.x * sin2 + szyna[i].n.y * cos2, 0.0);
                rpts2[23 - i] = vector6((-fHTW2 - szyna[i].x) * cos2 + szyna[i].y * sin2,
                                        -(-fHTW2 - szyna[i].x) * sin2 + szyna[i].y * cos2,
                                        szyna[i].z, -szyna[i].n.x * cos2 + szyna[i].n.y * sin2,
                                        +szyna[i].n.x * sin2 + szyna[i].n.y * cos2, 0.0);
            }
        switch (eType) // dalej zależnie od typu
        {
        case tt_Table: // obrotnica jak zwykły tor, tylko animacja dochodzi
            SwitchExtension->iLeftVBO = Vert - Start; // indeks toru obrotnicy
        case tt_Normal:
            if (TextureID2)
            { // podsypka z podkładami jest tylko dla zwykłego toru
                vector6 bpts1[8]; // punkty głównej płaszczyzny nie przydają się do robienia boków
                if (fTexLength == 4.0) // jeśli stare mapowanie
                { // stare mapowanie z różną gęstością pikseli i oddzielnymi teksturami na każdy
                    // profil
                    if (iTrapezoid) // trapez albo przechyłki
                    { // podsypka z podkladami trapezowata
                        // ewentualnie poprawić mapowanie, żeby środek mapował się na 1.435/4.671
                        // ((0.3464,0.6536)
                        // bo się tekstury podsypki rozjeżdżają po zmianie proporcji profilu
                        bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.00, -0.707, 0.707,
                                           0.0); // lewy brzeg
                        bpts1[1] = vector6((fHTW + side) * cos1, -(fHTW + side) * sin1 - 0.18, 0.33,
                                           -0.707, 0.707, 0.0); // krawędź załamania
                        bpts1[2] = vector6(-bpts1[1].x, +(fHTW + side) * sin1 - 0.18, 0.67, 0.707,
                                           0.707, 0.0); // prawy brzeg początku symetrycznie
                        bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 1.00, 0.707, 0.707,
                                           0.0); // prawy skos
                        // końcowy przekrój
                        bpts1[4] = vector6(rozp2, -fTexHeight2 - 0.18, 0.00, -0.707, 0.707,
                                           0.0); // lewy brzeg
                        bpts1[5] = vector6((fHTW2 + side2) * cos2, -(fHTW2 + side2) * sin2 - 0.18,
                                           0.33, -0.707, 0.707, 0.0); // krawędź załamania
                        bpts1[6] = vector6(-bpts1[5].x, +(fHTW2 + side2) * sin2 - 0.18, 0.67, 0.707,
                                           0.707, 0.0); // prawy brzeg początku symetrycznie
                        bpts1[7] = vector6(-rozp2, -fTexHeight2 - 0.18, 1.00, 0.707, 0.707,
                                           0.0); // prawy skos
                    }
                    else
                    {
                        bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.0, -0.707, 0.707,
                                           0.0); // lewy brzeg
                        bpts1[1] = vector6(fHTW + side, -0.18, 0.33, -0.707, 0.707,
                                           0.0); // krawędź załamania
                        bpts1[2] = vector6(-fHTW - side, -0.18, 0.67, 0.707, 0.707, 0.0); // druga
                        bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 1.0, 0.707, 0.707,
                                           0.0); // prawy skos
                    }
                }
                else
                { // mapowanie proporcjonalne do powierzchni, rozmiar w poprzek określa fTexLength
                    double max = fTexRatio2 * fTexLength; // szerokość proporcjonalna do długości
                    double map11 = max > 0.0 ? (fHTW + side) / max : 0.25; // załamanie od strony 1
                    double map12 =
                        max > 0.0 ? (fHTW + side + hypot1) / max : 0.5; // brzeg od strony 1
                    if (iTrapezoid) // trapez albo przechyłki
                    { // podsypka z podkladami trapezowata
                        double map21 =
                            max > 0.0 ? (fHTW2 + side2) / max : 0.25; // załamanie od strony 2
                        double map22 =
                            max > 0.0 ? (fHTW2 + side2 + hypot2) / max : 0.5; // brzeg od strony 2
                        // ewentualnie poprawić mapowanie, żeby środek mapował się na 1.435/4.671
                        // ((0.3464,0.6536)
                        // bo się tekstury podsypki rozjeżdżają po zmianie proporcji profilu
                        bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.5 - map12, normal1.x,
                                           -normal1.y, 0.0); // lewy brzeg
                        bpts1[1] = vector6((fHTW + side) * cos1, -(fHTW + side) * sin1 - 0.18,
                                           0.5 - map11, 0.0, 1.0, 0.0); // krawędź załamania
                        bpts1[2] = vector6(-bpts1[1].x, +(fHTW + side) * sin1 - 0.18, 0.5 + map11,
                                           0.0, 1.0, 0.0); // prawy brzeg początku symetrycznie
                        bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 0.5 + map12, -normal1.x,
                                           -normal1.y, 0.0); // prawy skos
                        // przekrój końcowy
                        bpts1[4] = vector6(rozp2, -fTexHeight2 - 0.18, 0.5 - map22, normal2.x,
                                           -normal2.y, 0.0); // lewy brzeg
                        bpts1[5] = vector6((fHTW2 + side2) * cos2, -(fHTW2 + side2) * sin2 - 0.18,
                                           0.5 - map21, 0.0, 1.0, 0.0); // krawędź załamania
                        bpts1[6] = vector6(-bpts1[5].x, +(fHTW2 + side2) * sin2 - 0.18, 0.5 + map21,
                                           0.0, 1.0, 0.0); // prawy brzeg początku symetrycznie
                        bpts1[7] = vector6(-rozp2, -fTexHeight2 - 0.18, 0.5 + map22, -normal2.x,
                                           -normal2.y, 0.0); // prawy skos
                    }
                    else
                    {
                        bpts1[0] = vector6(rozp, -fTexHeight1 - 0.18, 0.5 - map12, +normal1.x,
                                           -normal1.y, 0.0); // lewy brzeg
                        bpts1[1] = vector6(fHTW + side, -0.18, 0.5 - map11, +normal1.x, -normal1.y,
                                           0.0); // krawędź załamania
                        bpts1[2] = vector6(-fHTW - side, -0.18, 0.5 + map11, -normal1.x, -normal1.y,
                                           0.0); // druga
                        bpts1[3] = vector6(-rozp, -fTexHeight1 - 0.18, 0.5 + map12, -normal1.x,
                                           -normal1.y, 0.0); // prawy skos
                    }
                }
                Segment->RaRenderLoft(Vert, bpts1, iTrapezoid ? -4 : 4, fTexLength);
            }
            if (TextureID1)
            { // szyny - generujemy dwie, najwyżej rysować się będzie jedną
                Segment->RaRenderLoft(Vert, rpts1, iTrapezoid ? -nnumPts : nnumPts, fTexLength);
                Segment->RaRenderLoft(Vert, rpts2, iTrapezoid ? -nnumPts : nnumPts, fTexLength);
            }
            break;
        case tt_Switch: // dla zwrotnicy dwa razy szyny
            if (TextureID1) // Ra: !!!! tu jest do poprawienia
            { // iglice liczone tylko dla zwrotnic
                vector6 rpts3[24], rpts4[24];
                for (i = 0; i < 12; ++i)
                {
                    rpts3[i] =
                        vector6(+(fHTW + iglica[i].x) * cos1 + iglica[i].y * sin1,
                                -(+fHTW + iglica[i].x) * sin1 + iglica[i].y * cos1, iglica[i].z);
                    rpts3[i + 12] =
                        vector6(+(fHTW2 + szyna[i].x) * cos2 + szyna[i].y * sin2,
                                -(+fHTW2 + szyna[i].x) * sin2 + iglica[i].y * cos2, szyna[i].z);
                    rpts4[11 - i] =
                        vector6((-fHTW - iglica[i].x) * cos1 + iglica[i].y * sin1,
                                -(-fHTW - iglica[i].x) * sin1 + iglica[i].y * cos1, iglica[i].z);
                    rpts4[23 - i] =
                        vector6((-fHTW2 - szyna[i].x) * cos2 + szyna[i].y * sin2,
                                -(-fHTW2 - szyna[i].x) * sin2 + iglica[i].y * cos2, szyna[i].z);
                }
                if (SwitchExtension->RightSwitch)
                { // nowa wersja z SPKS, ale odwrotnie lewa/prawa
                    SwitchExtension->iLeftVBO = Vert - Start; // indeks lewej iglicy
                    SwitchExtension->Segments[0]->RaRenderLoft(Vert, rpts3, -nnumPts, fTexLength, 0,
                                                               2, SwitchExtension->fOffset2);
                    SwitchExtension->Segments[0]->RaRenderLoft(Vert, rpts1, nnumPts, fTexLength, 2);
                    SwitchExtension->Segments[0]->RaRenderLoft(Vert, rpts2, nnumPts, fTexLength);
                    SwitchExtension->Segments[1]->RaRenderLoft(Vert, rpts1, nnumPts, fTexLength);
                    SwitchExtension->iRightVBO = Vert - Start; // indeks prawej iglicy
                    SwitchExtension->Segments[1]->RaRenderLoft(Vert, rpts4, -nnumPts, fTexLength, 0,
                                                               2, -fMaxOffset +
                                                                      SwitchExtension->fOffset1);
                    SwitchExtension->Segments[1]->RaRenderLoft(Vert, rpts2, nnumPts, fTexLength, 2);
                }
                else
                { // lewa działa lepiej niż prawa
                    SwitchExtension->Segments[0]->RaRenderLoft(
                        Vert, rpts1, nnumPts, fTexLength); // lewa szyna normalna cała
                    SwitchExtension->iLeftVBO = Vert - Start; // indeks lewej iglicy
                    SwitchExtension->Segments[0]->RaRenderLoft(
                        Vert, rpts4, -nnumPts, fTexLength, 0, 2,
                        -SwitchExtension->fOffset2); // prawa iglica
                    SwitchExtension->Segments[0]->RaRenderLoft(Vert, rpts2, nnumPts, fTexLength,
                                                               2); // prawa szyna za iglicą
                    SwitchExtension->iRightVBO = Vert - Start; // indeks prawej iglicy
                    SwitchExtension->Segments[1]->RaRenderLoft(
                        Vert, rpts3, -nnumPts, fTexLength, 0, 2,
                        fMaxOffset - SwitchExtension->fOffset1); // lewa iglica
                    SwitchExtension->Segments[1]->RaRenderLoft(Vert, rpts1, nnumPts, fTexLength,
                                                               2); // lewa szyna za iglicą
                    SwitchExtension->Segments[1]->RaRenderLoft(
                        Vert, rpts2, nnumPts, fTexLength); // prawa szyna normalnie cała
                }
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
            vector6 bpts1[4]; // punkty głównej płaszczyzny przydają się do robienia boków
            if (TextureID1 || TextureID2) // punkty się przydadzą, nawet jeśli nawierzchni nie ma
            { // double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
                double max = (iCategoryFlag & 4) ?
                                 0.0 :
                                 fTexLength; // test: szerokość dróg proporcjonalna do długości
                double map1 = max > 0.0 ? fHTW / max : 0.5; // obcięcie tekstury od strony 1
                double map2 = max > 0.0 ? fHTW2 / max : 0.5; // obcięcie tekstury od strony 2
                if (iTrapezoid) // trapez albo przechyłki
                { // nawierzchnia trapezowata
                    Segment->GetRolls(roll1, roll2);
                    bpts1[0] = vector6(fHTW * cos(roll1), -fHTW * sin(roll1),
                                       0.5 - map1); // lewy brzeg początku
                    bpts1[1] = vector6(-bpts1[0].x, -bpts1[0].y,
                                       0.5 + map1); // prawy brzeg początku symetrycznie
                    bpts1[2] = vector6(fHTW2 * cos(roll2), -fHTW2 * sin(roll2),
                                       0.5 - map2); // lewy brzeg końca
                    bpts1[3] = vector6(-bpts1[2].x, -bpts1[2].y,
                                       0.5 + map2); // prawy brzeg początku symetrycznie
                }
                else
                {
                    bpts1[0] = vector6(fHTW, 0.0, 0.5 - map1); // zawsze standardowe mapowanie
                    bpts1[1] = vector6(-fHTW, 0.0, 0.5 + map1);
                }
            }
            if (TextureID1) // jeśli podana była tekstura, generujemy trójkąty
            { // tworzenie trójkątów nawierzchni szosy
                Segment->RaRenderLoft(Vert, bpts1, iTrapezoid ? -2 : 2, fTexLength);
            }
            if (TextureID2)
            { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak w
                // Midtown Madness 2?)
                vector6 rpts1[6],
                    rpts2[6]; // współrzędne przekroju i mapowania dla prawej i lewej strony
                if (fTexHeight1 >= 0.0)
                { // standardowo: od zewnątrz pochylenie, a od wewnątrz poziomo
                    rpts1[0] = vector6(rozp, -fTexHeight1, 0.0); // lewy brzeg podstawy
                    rpts1[1] =
                        vector6(bpts1[0].x + side, bpts1[0].y, 0.5), // lewa krawędź załamania
                        rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                           1.0); // lewy brzeg pobocza (mapowanie może być inne
                    rpts2[0] = vector6(bpts1[1].x, bpts1[1].y, 1.0); // prawy brzeg pobocza
                    rpts2[1] =
                        vector6(bpts1[1].x - side, bpts1[1].y, 0.5); // prawa krawędź załamania
                    rpts2[2] = vector6(-rozp, -fTexHeight1, 0.0); // prawy brzeg podstawy
                    if (iTrapezoid) // trapez albo przechyłki
                    { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                        // odcinka
                        rpts1[3] = vector6(rozp2, -fTexHeight2, 0.0); // lewy brzeg lewego pobocza
                        rpts1[4] = vector6(bpts1[2].x + side2, bpts1[2].y, 0.5); // krawędź
                        // załamania
                        rpts1[5] = vector6(bpts1[2].x, bpts1[2].y, 1.0); // brzeg pobocza
                        rpts2[3] = vector6(bpts1[3].x, bpts1[3].y, 1.0);
                        rpts2[4] = vector6(bpts1[3].x - side2, bpts1[3].y, 0.5);
                        rpts2[5] = vector6(-rozp2, -fTexHeight2, 0.0); // prawy brzeg prawego
                        // pobocza
                        Segment->RaRenderLoft(Vert, rpts1, -3, fTexLength);
                        Segment->RaRenderLoft(Vert, rpts2, -3, fTexLength);
                    }
                    else
                    { // pobocza zwykłe, brak przechyłki
                        Segment->RaRenderLoft(Vert, rpts1, 3, fTexLength);
                        Segment->RaRenderLoft(Vert, rpts2, 3, fTexLength);
                    }
                }
                else
                { // wersja dla chodnika: skos 1:3.75, każdy chodnik innej szerokości
                    // mapowanie propocjonalne do szerokości chodnika
                    // krawężnik jest mapowany od 31/64 do 32/64 lewy i od 32/64 do 33/64 prawy
                    double d =
                        -fTexHeight1 / 3.75; // krawężnik o wysokości 150mm jest pochylony 40mm
                    double max =
                        fTexRatio2 * fTexLength; // test: szerokość proporcjonalna do długości
                    double map1l = max > 0.0 ?
                                       side / max :
                                       0.484375; // obcięcie tekstury od lewej strony punktu 1
                    double map1r = max > 0.0 ?
                                       slop / max :
                                       0.484375; // obcięcie tekstury od prawej strony punktu 1
                    double h1r = (slop > d) ? -fTexHeight1 : 0;
                    double h1l = (side > d) ? -fTexHeight1 : 0;
                    rpts1[0] = vector6(bpts1[0].x + slop, bpts1[0].y + h1r,
                                       0.515625 + map1r); // prawy brzeg prawego chodnika
                    rpts1[1] = vector6(bpts1[0].x + d, bpts1[0].y + h1r,
                                       0.515625); // prawy krawężnik u góry
                    rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                       0.515625 - d / 2.56); // prawy krawężnik u dołu
                    rpts2[0] = vector6(bpts1[1].x, bpts1[1].y,
                                       0.484375 + d / 2.56); // lewy krawężnik u dołu
                    rpts2[1] = vector6(bpts1[1].x - d, bpts1[1].y + h1l,
                                       0.484375); // lewy krawężnik u góry
                    rpts2[2] = vector6(bpts1[1].x - side, bpts1[1].y + h1l,
                                       0.484375 - map1l); // lewy brzeg lewego chodnika
                    if (iTrapezoid) // trapez albo przechyłki
                    { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony
                        // odcinka
                        slop2 =
                            fabs((iTrapezoid & 2) ? slop2 : slop); // szerokość chodnika po prawej
                        double map2l = max > 0.0 ?
                                           side2 / max :
                                           0.484375; // obcięcie tekstury od lewej strony punktu 2
                        double map2r = max > 0.0 ?
                                           slop2 / max :
                                           0.484375; // obcięcie tekstury od prawej strony punktu 2
                        double h2r = (slop2 > d) ? -fTexHeight2 : 0;
                        double h2l = (side2 > d) ? -fTexHeight2 : 0;
                        rpts1[3] = vector6(bpts1[2].x + slop2, bpts1[2].y + h2r,
                                           0.515625 + map2r); // prawy brzeg prawego chodnika
                        rpts1[4] = vector6(bpts1[2].x + d, bpts1[2].y + h2r,
                                           0.515625); // prawy krawężnik u góry
                        rpts1[5] = vector6(bpts1[2].x, bpts1[2].y,
                                           0.515625 - d / 2.56); // prawy krawężnik u dołu
                        rpts2[3] = vector6(bpts1[3].x, bpts1[3].y,
                                           0.484375 + d / 2.56); // lewy krawężnik u dołu
                        rpts2[4] = vector6(bpts1[3].x - d, bpts1[3].y + h2l,
                                           0.484375); // lewy krawężnik u góry
                        rpts2[5] = vector6(bpts1[3].x - side2, bpts1[3].y + h2l,
                                           0.484375 - map2l); // lewy brzeg lewego chodnika
                        if (slop != 0.0)
                            Segment->RaRenderLoft(Vert, rpts1, -3, fTexLength);
                        if (side != 0.0)
                            Segment->RaRenderLoft(Vert, rpts2, -3, fTexLength);
                    }
                    else
                    { // pobocza zwykłe, brak przechyłki
                        if (slop != 0.0)
                            Segment->RaRenderLoft(Vert, rpts1, 3, fTexLength);
                        if (side != 0.0)
                            Segment->RaRenderLoft(Vert, rpts2, 3, fTexLength);
                    }
                }
            }
        }
        }
        break;
    case 4: // Ra: rzeki na razie jak drogi, przechyłki na pewno nie mają
        switch (eType) // dalej zależnie od typu
        {
        case tt_Normal: // drogi proste, bo skrzyżowania osobno
        {
            vector6 bpts1[4]; // punkty głównej płaszczyzny przydają się do robienia boków
            if (TextureID1 || TextureID2) // punkty się przydadzą, nawet jeśli nawierzchni nie ma
            { // double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
                double max = (iCategoryFlag & 4) ?
                                 0.0 :
                                 fTexLength; // test: szerokość dróg proporcjonalna do długości
                double map1 = max > 0.0 ? fHTW / max : 0.5; // obcięcie tekstury od strony 1
                double map2 = max > 0.0 ? fHTW2 / max : 0.5; // obcięcie tekstury od strony 2
                if (iTrapezoid) // trapez albo przechyłki
                { // nawierzchnia trapezowata
                    Segment->GetRolls(roll1, roll2);
                    bpts1[0] = vector6(fHTW * cos(roll1), -fHTW * sin(roll1),
                                       0.5 - map1); // lewy brzeg początku
                    bpts1[1] = vector6(-bpts1[0].x, -bpts1[0].y,
                                       0.5 + map1); // prawy brzeg początku symetrycznie
                    bpts1[2] = vector6(fHTW2 * cos(roll2), -fHTW2 * sin(roll2),
                                       0.5 - map2); // lewy brzeg końca
                    bpts1[3] = vector6(-bpts1[2].x, -bpts1[2].y,
                                       0.5 + map2); // prawy brzeg początku symetrycznie
                }
                else
                {
                    bpts1[0] = vector6(fHTW, 0.0, 0.5 - map1); // zawsze standardowe mapowanie
                    bpts1[1] = vector6(-fHTW, 0.0, 0.5 + map1);
                }
            }
            if (TextureID1) // jeśli podana była tekstura, generujemy trójkąty
            { // tworzenie trójkątów nawierzchni szosy
                Segment->RaRenderLoft(Vert, bpts1, iTrapezoid ? -2 : 2, fTexLength);
            }
            if (TextureID2)
            { // pobocze drogi - poziome przy przechyłce (a może krawężnik i chodnik zrobić jak w
                // Midtown Madness 2?)
                vector6 rpts1[6],
                    rpts2[6]; // współrzędne przekroju i mapowania dla prawej i lewej strony
                rpts1[0] = vector6(rozp, -fTexHeight1, 0.0); // lewy brzeg podstawy
                rpts1[1] = vector6(bpts1[0].x + side, bpts1[0].y, 0.5), // lewa krawędź załamania
                    rpts1[2] = vector6(bpts1[0].x, bpts1[0].y,
                                       1.0); // lewy brzeg pobocza (mapowanie może być inne
                rpts2[0] = vector6(bpts1[1].x, bpts1[1].y, 1.0); // prawy brzeg pobocza
                rpts2[1] = vector6(bpts1[1].x - side, bpts1[1].y, 0.5); // prawa krawędź załamania
                rpts2[2] = vector6(-rozp, -fTexHeight1, 0.0); // prawy brzeg podstawy
                if (iTrapezoid) // trapez albo przechyłki
                { // pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
                    rpts1[3] = vector6(rozp2, -fTexHeight2, 0.0); // lewy brzeg lewego pobocza
                    rpts1[4] = vector6(bpts1[2].x + side2, bpts1[2].y, 0.5); // krawędź załamania
                    rpts1[5] = vector6(bpts1[2].x, bpts1[2].y, 1.0); // brzeg pobocza
                    rpts2[3] = vector6(bpts1[3].x, bpts1[3].y, 1.0);
                    rpts2[4] = vector6(bpts1[3].x - side2, bpts1[3].y, 0.5);
                    rpts2[5] = vector6(-rozp2, -fTexHeight2, 0.0); // prawy brzeg prawego pobocza
                    Segment->RaRenderLoft(Vert, rpts1, -3, fTexLength);
                    Segment->RaRenderLoft(Vert, rpts2, -3, fTexLength);
                }
                else
                { // pobocza zwykłe, brak przechyłki
                    Segment->RaRenderLoft(Vert, rpts1, 3, fTexLength);
                    Segment->RaRenderLoft(Vert, rpts2, 3, fTexLength);
                }
            }
        }
        }
        break;
    }
};

void TTrack::RaRenderVBO(int iPtr)
{ // renderowanie z użyciem VBO
    // Ra 2014-07: trzeba wymienić GL_TRIANGLE_STRIP na GL_TRIANGLES i renderować trójkąty sektora
    // dla kolejnych tekstur!
    EnvironmentSet();
    int seg;
    int i;
    switch (iCategoryFlag & 15)
    {
    case 1: // tor
        if (eType == tt_Switch) // dla zwrotnicy tylko szyny
        {
            if (TextureID1)
                if ((seg = SwitchExtension->Segments[0]->RaSegCount()) > 0)
                {
                    glBindTexture(GL_TEXTURE_2D, TextureID1); // szyny +
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 24 * i, 24);
                    iPtr += 24 * seg; // pominięcie lewej szyny
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 24 * i, 24);
                    iPtr += 24 * seg; // pominięcie prawej szyny
                }
            if (TextureID2)
                if ((seg = SwitchExtension->Segments[1]->RaSegCount()) > 0)
                {
                    glBindTexture(GL_TEXTURE_2D, TextureID2); // szyny -
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 24 * i, 24);
                    iPtr += 24 * seg; // pominięcie lewej szyny
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 24 * i, 24);
                }
        }
        else // dla toru podsypka plus szyny
        {
            if ((seg = Segment->RaSegCount()) > 0)
            {
                if (TextureID2)
                {
                    glBindTexture(GL_TEXTURE_2D, TextureID2); // podsypka
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 8 * i, 8);
                    iPtr += 8 * seg; // pominięcie podsypki
                }
                if (TextureID1)
                {
                    glBindTexture(GL_TEXTURE_2D, TextureID1); // szyny
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 24 * i, 24);
                    iPtr += 24 * seg; // pominięcie lewej szyny
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 24 * i, 24);
                }
            }
        }
        break;
    case 2: // droga
        if ((seg = Segment->RaSegCount()) > 0)
        {
            if (TextureID1)
            {
                glBindTexture(GL_TEXTURE_2D, TextureID1); // nawierzchnia
                for (i = 0; i < seg; ++i)
                {
                    glDrawArrays(GL_TRIANGLE_STRIP, iPtr, 4);
                    iPtr += 4;
                }
            }
            if (TextureID2)
            {
                glBindTexture(GL_TEXTURE_2D, TextureID2); // pobocze
                if (fTexHeight1 >= 0.0)
                { // normalna droga z poboczem
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 6 * i, 6);
                    iPtr += 6 * seg; // pominięcie lewego pobocza
                    for (i = 0; i < seg; ++i)
                        glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 6 * i, 6);
                }
                else
                { // z chodnikami o różnych szerokociach
                    if (fTexWidth != 0.0)
                    {
                        for (i = 0; i < seg; ++i)
                            glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 6 * i, 6);
                        iPtr += 6 * seg; // pominięcie lewego pobocza
                    }
                    if (fTexSlope != 0.0)
                        for (i = 0; i < seg; ++i)
                            glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 6 * i, 6);
                }
            }
        }
        break;
    case 4: // rzeki - jeszcze do przemyślenia
        if ((seg = Segment->RaSegCount()) > 0)
        {
            if (TextureID1)
            {
                glBindTexture(GL_TEXTURE_2D, TextureID1); // nawierzchnia
                for (i = 0; i < seg; ++i)
                {
                    glDrawArrays(GL_TRIANGLE_STRIP, iPtr, 4);
                    iPtr += 4;
                }
            }
            if (TextureID2)
            {
                glBindTexture(GL_TEXTURE_2D, TextureID2); // pobocze
                for (i = 0; i < seg; ++i)
                    glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 6 * i, 6);
                iPtr += 6 * seg; // pominięcie lewego pobocza
                for (i = 0; i < seg; ++i)
                    glDrawArrays(GL_TRIANGLE_STRIP, iPtr + 6 * i, 6);
            }
        }
        break;
    }
    EnvironmentReset();
};

void TTrack::EnvironmentSet()
{ // ustawienie zmienionego światła
    glColor3f(1.0f, 1.0f, 1.0f); // Ra: potrzebne to?
    if (eEnvironment)
    { // McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
        GLfloat ambientLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
        GLfloat diffuseLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
        GLfloat specularLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};
        switch (eEnvironment)
        { // modyfikacje oświetlenia zależnie od środowiska
        case e_canyon:
            for (int li = 0; li < 3; li++)
            {
                // ambientLight[li]= Global::ambientDayLight[li]*0.8; //0.7
                diffuseLight[li] = Global::diffuseDayLight[li] * 0.4; // 0.3
                specularLight[li] = Global::specularDayLight[li] * 0.5; // 0.4
            }
            // glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
            glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
            break;
        case e_tunnel:
            for (int li = 0; li < 3; li++)
            {
                ambientLight[li] = Global::ambientDayLight[li] * 0.2;
                diffuseLight[li] = Global::diffuseDayLight[li] * 0.1;
                specularLight[li] = Global::specularDayLight[li] * 0.2;
            }
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
            glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
            break;
        }
    }
};

void TTrack::EnvironmentReset()
{ // przywrócenie domyślnego światła
    switch (eEnvironment)
    { // przywrócenie globalnych ustawień światła, o ile było zmienione
    case e_canyon: // wykop
    case e_tunnel: // tunel
        glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight);
        glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight);
    }
};

void TTrack::RenderDyn()
{ // renderowanie nieprzezroczystych fragmentów pojazdów
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    if (!iNumDynamics)
        return; // po co kombinować, jeśli nie ma pojazdów?
    // EnvironmentSet(); //Ra: pojazdy sobie same teraz liczą cienie
    for (int i = 0; i < iNumDynamics; i++)
        Dynamics[i]->Render(); // sam sprawdza, czy VBO; zmienia kontekst VBO!
    // EnvironmentReset();
#else
    for( auto dynamic : Dynamics ) {
        // sam sprawdza, czy VBO; zmienia kontekst VBO!
        dynamic->Render();
    }
#endif
};

void TTrack::RenderDynAlpha()
{ // renderowanie przezroczystych fragmentów pojazdów
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    if (!iNumDynamics)
        return; // po co kombinować, jeśli nie ma pojazdów?
    // EnvironmentSet(); //Ra: pojazdy sobie same teraz liczą cienie
    for (int i = 0; i < iNumDynamics; i++)
        Dynamics[i]->RenderAlpha(); // sam sprawdza, czy VBO; zmienia kontekst VBO!
    // EnvironmentReset();
#else
    for( auto dynamic : Dynamics ) {
        // sam sprawdza, czy VBO; zmienia kontekst VBO!
        dynamic->RenderAlpha();
    }
#endif
};

void TTrack::RenderDynSounds()
{ // odtwarzanie dźwięków pojazdów jest niezależne od ich wyświetlania
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    for( int i = 0; i < iNumDynamics; i++ )
        Dynamics[i]->RenderSounds();
#else
    for( auto dynamic : Dynamics ) {
        dynamic->RenderSounds();
    }
#endif
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

bool TTrack::Switch(int i, double t, double d)
{ // przełączenie torów z uruchomieniem animacji
    if (SwitchExtension) // tory przełączalne mają doklejkę
        if (eType == tt_Switch)
        { // przekładanie zwrotnicy jak zwykle
            if (t > 0.0) // prędkość liniowa ruchu iglic
                SwitchExtension->fOffsetSpeed = t; // prędkość łatwiej zgrać z animacją modelu
            if (d >= 0.0) // dodatkowy ruch drugiej iglicy (zamknięcie nastawnicze)
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
            if (SwitchExtension->fVelocity <=
                -2) //-1 oznacza maksymalną prędkość, a dalsze ujemne to ograniczenie na bok
                fVelocity = i ? -SwitchExtension->fVelocity : -1;
            if (SwitchExtension->pOwner ? SwitchExtension->pOwner->RaTrackAnimAdd(this) :
                                          true) // jeśli nie dodane do animacji
            { // nie ma się co bawić
                SwitchExtension->fOffset = SwitchExtension->fDesiredOffset;
                RaAnimate(); // przeliczenie położenia iglic; czy zadziała na niewyświetlanym
                // sektorze w VBO?
            }
            return true;
        }
        else if (eType == tt_Table)
        { // blokowanie (0, szukanie torów) lub odblokowanie (1, rozłączenie) obrotnicy
            if (i)
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
            }
            else
            { // 1: ustalenie finalnego położenia (gdy nie było animacji)
                RaAnimate(); // ostatni etap animowania
                // zablokowanie pozycji i połączenie do sąsiednich torów
                Global::pGround->TrackJoin(SwitchExtension->pMyNode);
                if (trNext || trPrev)
                {
                    fVelocity = 6.0; // jazda dozwolona
                    if (trPrev)
                        if (trPrev->fVelocity ==
                            0.0) // ustawienie 0 da możliwość zatrzymania AI na obrotnicy
                            trPrev->VelocitySet(6.0); // odblokowanie dołączonego toru do jazdy
                    if (trNext)
                        if (trNext->fVelocity == 0.0)
                            trNext->VelocitySet(6.0);
                    if (SwitchExtension->evPlus) // w starych sceneriach może nie być
                        Global::AddToQuery(SwitchExtension->evPlus,
                                           NULL); // potwierdzenie wykonania (np. odpala WZ)
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
                        Global::AddToQuery(SwitchExtension->evPlus, o); // dodanie do kolejki
                    break;
                case 1:
                    if (SwitchExtension->evMinus)
                        Global::AddToQuery(SwitchExtension->evMinus, o); // dodanie do kolejki
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
        // WriteLog("Crossing from P"+AnsiString(from+1)+" into stop on "+pMyNode->asName);
        break;
    case 1: // left
        // WriteLog("Crossing from P"+AnsiString(from+1)+" to left on "+pMyNode->asName);
        i = (SwitchExtension->iRoads == 4) ? iLewo4[from] : iLewo3[from];
        break;
    case 2: // right
        // WriteLog("Crossing from P"+AnsiString(from+1)+" to right on "+pMyNode->asName);
        i = (SwitchExtension->iRoads == 4) ? iPrawo4[from] : iPrawo3[from];
        break;
    case 3: // stright
        // WriteLog("Crossing from P"+AnsiString(from+1)+" to straight on "+pMyNode->asName);
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
    if (SwitchExtension)
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
    if (SwitchExtension->pNextAnim)
        SwitchExtension->pNextAnim = SwitchExtension->pNextAnim->RaAnimate();
    bool m = true; // animacja trwa
    if (eType == tt_Switch) // dla zwrotnicy tylko szyny
    {
        double v = SwitchExtension->fDesiredOffset - SwitchExtension->fOffset; // kierunek
        SwitchExtension->fOffset += sign(v) * Timer::GetDeltaTime() * SwitchExtension->fOffsetSpeed;
        // Ra: trzeba dać to do klasy...
        SwitchExtension->fOffset1 = SwitchExtension->fOffset;
        SwitchExtension->fOffset2 = SwitchExtension->fOffset;
        if (SwitchExtension->fOffset1 >= fMaxOffset)
            SwitchExtension->fOffset1 = fMaxOffset; // ograniczenie animacji zewnętrznej iglicy
        if (SwitchExtension->fOffset2 <= 0.00)
            SwitchExtension->fOffset2 = 0.0; // ograniczenie animacji wewnętrznej iglicy
        if (v < 0)
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
        if (Global::bUseVBO)
        { // dla OpenGL 1.4 odświeży się cały sektor, w późniejszych poprawiamy fragment
            if (true == GLEW_VERSION_1_5) // dla OpenGL 1.4 to się nie wykona poprawnie
                if (TextureID1) // Ra: !!!! tu jest do poprawienia
                { // iglice liczone tylko dla zwrotnic
                    vector6 rpts3[24], rpts4[24];
                    double fHTW = 0.5 * fabs(fTrackWidth);
                    double fHTW2 = fHTW; // Ra: na razie niech tak będzie
                    double cos1 = 1.0, sin1 = 0.0, cos2 = 1.0, sin2 = 0.0; // Ra: ...
                    for (int i = 0; i < 12; ++i)
                    {
                        rpts3[i] =
                            vector6((fHTW + iglica[i].x) * cos1 + iglica[i].y * sin1,
                                    -(fHTW + iglica[i].x) * sin1 + iglica[i].y * cos1, iglica[i].z);
                        rpts3[i + 12] =
                            vector6((fHTW2 + szyna[i].x) * cos2 + szyna[i].y * sin2,
                                    -(fHTW2 + szyna[i].x) * sin2 + iglica[i].y * cos2, szyna[i].z);
                        rpts4[11 - i] = vector6((-fHTW - iglica[i].x) * cos1 + iglica[i].y * sin1,
                                                -(-fHTW - iglica[i].x) * sin1 + iglica[i].y * cos1,
                                                iglica[i].z);
                        rpts4[23 - i] =
                            vector6((-fHTW2 - szyna[i].x) * cos2 + szyna[i].y * sin2,
                                    -(-fHTW2 - szyna[i].x) * sin2 + iglica[i].y * cos2, szyna[i].z);
                    }
                    CVertNormTex Vert[2 * 2 * 12]; // na razie 2 segmenty
                    CVertNormTex *v = Vert; // bo RaAnimate() modyfikuje wskaźnik
                    glGetBufferSubData(
                        GL_ARRAY_BUFFER, SwitchExtension->iLeftVBO * sizeof(CVertNormTex),
                        2 * 2 * 12 * sizeof(CVertNormTex), &Vert); // pobranie fragmentu bufora VBO
                    if (SwitchExtension->RightSwitch)
                    { // nowa wersja z SPKS, ale odwrotnie lewa/prawa
                        SwitchExtension->Segments[0]->RaAnimate(v, rpts3, -nnumPts, fTexLength, 0,
                                                                2, SwitchExtension->fOffset2);
                        glBufferSubData(GL_ARRAY_BUFFER,
                                        SwitchExtension->iLeftVBO * sizeof(CVertNormTex),
                                        2 * 2 * 12 * sizeof(CVertNormTex),
                                        &Vert); // wysłanie fragmentu bufora VBO
                        v = Vert;
                        glGetBufferSubData(GL_ARRAY_BUFFER,
                                           SwitchExtension->iRightVBO * sizeof(CVertNormTex),
                                           2 * 2 * 12 * sizeof(CVertNormTex),
                                           &Vert); // pobranie fragmentu bufora VBO
                        SwitchExtension->Segments[1]->RaAnimate(v, rpts4, -nnumPts, fTexLength, 0,
                                                                2, -fMaxOffset +
                                                                       SwitchExtension->fOffset1);
                    }
                    else
                    { // oryginalnie lewa działała lepiej niż prawa
                        SwitchExtension->Segments[0]->RaAnimate(
                            v, rpts4, -nnumPts, fTexLength, 0, 2,
                            -SwitchExtension->fOffset2); // prawa iglica
                        glBufferSubData(GL_ARRAY_BUFFER,
                                        SwitchExtension->iLeftVBO * sizeof(CVertNormTex),
                                        2 * 2 * 12 * sizeof(CVertNormTex),
                                        &Vert); // wysłanie fragmentu bufora VBO
                        v = Vert;
                        glGetBufferSubData(GL_ARRAY_BUFFER,
                                           SwitchExtension->iRightVBO * sizeof(CVertNormTex),
                                           2 * 2 * 12 * sizeof(CVertNormTex),
                                           &Vert); // pobranie fragmentu bufora VBO
                        SwitchExtension->Segments[1]->RaAnimate(
                            v, rpts3, -nnumPts, fTexLength, 0, 2,
                            fMaxOffset - SwitchExtension->fOffset1); // lewa iglica
                    }
                    glBufferSubData(
                        GL_ARRAY_BUFFER, SwitchExtension->iRightVBO * sizeof(CVertNormTex),
                        2 * 2 * 12 * sizeof(CVertNormTex), &Vert); // wysłanie fragmentu bufora VBO
                }
        }
        else // gdy Display List
            Release(); // niszczenie skompilowanej listy, aby się wygenerowała nowa
    }
    else if (eType == tt_Table) // dla obrotnicy - szyny i podsypka
    {
        if (SwitchExtension->pModel &&
            SwitchExtension->CurrentIndex) // 0=zablokowana się nie animuje
        { // trzeba każdorazowo porównywać z kątem modelu
            // SwitchExtension->fOffset1=SwitchExtension->pAnim?SwitchExtension->pAnim->AngleGet():0.0;
            // //pobranie kąta z modelu
            TAnimContainer *ac = SwitchExtension->pModel ?
                                     SwitchExtension->pModel->GetContainer(NULL) :
                                     NULL; // pobranie głównego submodelu
            // if (ac) ac->EventAssign(SwitchExtension->evMinus); //event zakończenia animacji,
            // trochę bez sensu tutaj
            if (ac)
                if ((ac->AngleGet() != SwitchExtension->fOffset) ||
                    !(ac->TransGet() ==
                      SwitchExtension->vTrans)) // czy przemieściło się od ostatniego sprawdzania
                {
                    double hlen = 0.5 * SwitchExtension->Segments[0]->GetLength(); // połowa
                    // długości
                    SwitchExtension->fOffset = ac->AngleGet(); // pobranie kąta z submodelu
                    double sina = -hlen * sin(DegToRad(SwitchExtension->fOffset)),
                           cosa = -hlen * cos(DegToRad(SwitchExtension->fOffset));
                    SwitchExtension->vTrans = ac->TransGet();
                    vector3 middle =
                        SwitchExtension->pMyNode->pCenter +
                        SwitchExtension->vTrans; // SwitchExtension->Segments[0]->FastGetPoint(0.5);
                    Segment->Init(middle + vector3(sina, 0.0, cosa),
                                  middle - vector3(sina, 0.0, cosa), 5.0); // nowy odcinek
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
                    for( int i = 0; i < iNumDynamics; i++ )
                        Dynamics[i]->Move(0.000001); // minimalny ruch, aby przeliczyć pozycję i
#else
                    for( auto dynamic : Dynamics ) {
                        // minimalny ruch, aby przeliczyć pozycję
                        dynamic->Move( 0.000001 );
                    }
#endif
                    // kąty
                    if (Global::bUseVBO)
                    { // dla OpenGL 1.4 odświeży się cały sektor, w późniejszych poprawiamy fragment
                        // aktualizacja pojazdów na torze
                        if (true == GLEW_VERSION_1_5) // dla OpenGL 1.4 to się nie wykona poprawnie
                        {
                            int size =
                                RaArrayPrepare(); // wielkość tabeli potrzebna dla tej obrotnicy
                            CVertNormTex *Vert = new CVertNormTex[size]; // bufor roboczy
                            // CVertNormTex *v=Vert; //zmieniane przez
                            RaArrayFill(Vert,
                                        Vert -
                                            SwitchExtension
                                                ->iLeftVBO); // iLeftVBO powinno zostać niezmienione
                            glBufferSubData(
                                GL_ARRAY_BUFFER, SwitchExtension->iLeftVBO * sizeof(CVertNormTex),
                                size * sizeof(CVertNormTex), Vert); // wysłanie fragmentu bufora VBO
                        }
                    }
                    else // gdy Display List
                        Release(); // niszczenie skompilowanej listy, aby się wygenerowała nowa
                } // animacja trwa nadal
        }
        else
            m = false; // koniec animacji albo w ogóle nie połączone z modelem
    }
    return m ? this : SwitchExtension->pNextAnim; // zwraca obiekt do dalszej animacji
};
//---------------------------------------------------------------------------
void TTrack::RadioStop()
{ // przekazanie pojazdom rozkazu zatrzymania
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
    for (int i = 0; i < iNumDynamics; i++)
        Dynamics[i]->RadioStop();
#else
    for( auto dynamic : Dynamics ) {
        dynamic->RadioStop();
    }
#endif

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

bool Equal(vector3 v1, vector3 *v2)
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

int TTrack::TestPoint(vector3 *Point)
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

void TTrack::MovedUp1(double dh)
{ // poprawienie przechyłki wymaga wydłużenia podsypki
    fTexHeight1 += dh;
};

string TTrack::NameGet()
{ // ustalenie nazwy toru
    if (this)
        if (pMyNode)
            return pMyNode->asName;
    return "none";
};

void TTrack::VelocitySet(float v)
{ // ustawienie prędkości z ograniczeniem do pierwotnej wartości (zapisanej w scenerii)
    if (SwitchExtension ? SwitchExtension->fVelocity >= 0.0 : false)
    { // zwrotnica może mieć odgórne ograniczenie, nieprzeskakiwalne eventem
        if (v > SwitchExtension->fVelocity ? true : v < 0.0)
            return void(fVelocity =
                            SwitchExtension->fVelocity); // maksymalnie tyle, ile było we wpisie
    }
    fVelocity = v; // nie ma ograniczenia
};

float TTrack::VelocityGet()
{ // pobranie dozwolonej prędkości podczas skanowania
    return ((iDamageFlag & 128) ? 0.0f : fVelocity); // tor uszkodzony = prędkość zerowa
};

void TTrack::ConnectionsLog()
{ // wypisanie informacji o połączeniach
    int i;
    WriteLog("--> tt_Cross named " + pMyNode->asName);
    if (eType == tt_Cross)
        for (i = 0; i < 2; ++i)
        {
            if (SwitchExtension->pPrevs[i])
                WriteLog("Point " + std::to_string(i + i + 1) + " -> track " +
                         SwitchExtension->pPrevs[i]->pMyNode->asName + ":" +
                         std::to_string(int(SwitchExtension->iPrevDirection[i])));
            if (SwitchExtension->pNexts[i])
                WriteLog("Point " + std::to_string(i + i + 2) + " -> track " +
                         SwitchExtension->pNexts[i]->pMyNode->asName + ":" +
                         std::to_string(int(SwitchExtension->iNextDirection[i])));
        }
};

TTrack * TTrack::Neightbour(int s, double &d)
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
