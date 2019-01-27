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
#include "Traction.h"

#include "simulation.h"
#include "Globals.h"
#include "TractionPower.h"
#include "Logs.h"
#include "renderer.h"
#include "utilities.h"

//---------------------------------------------------------------------------
/*

=== Koncepcja dwustronnego zasilania sekcji sieci trakcyjnej, Ra 2014-02 ===
0. Każde przęsło sieci może mieć wpisaną nazwę zasilacza, a także napięcie
nominalne i maksymalny prąd, które stanowią redundancję do danych zasilacza.
Rezystancja może się zmieniać, materiał i grubość drutu powinny być wspólny
dla segmentu. Podanie punktów umożliwia łączenie przęseł w listy dwukierunkowe,
co usprawnia wyszukiwania kolejnych przeseł podczas jazdy. Dla bieżni wspólnej
powinna być podana nazwa innego przęsła w parametrze "parallel". Wskaźniki
na przęsła bieżni wspólnej mają być układane w jednokierunkowy pierścień.
1. Problemem jest ustalenie topologii sekcji dla linii dwutorowych. Nad każdym
torem powinna znajdować się oddzielna sekcja sieci, aby mogła zostać odłączona
w przypadku zwarcia. Sekcje nad równoległymi torami są również łączone
równolegle przez kabiny sekcyjne, co zmniejsza płynące prądy i spadki napięć.
2. Drugim zagadnieniem jest zasilanie sekcji jednocześnie z dwóch stron, czyli
sekcja musi mieć swoją nazwę oraz wskazanie dwóch zasilaczy ze wskazaniem
geograficznym ich położenia. Dodatkową trudnością jest brak połączenia
pomiędzy segmentami naprężania. Podsumowując, każdy segment naprężania powinien
mieć przypisanie do sekcji zasilania, a dodatkowo skrajne segmenty powinny
wskazywać dwa różne zasilacze.
3. Zasilaczem sieci może być podstacja, która w wersji 3kV powinna generować
pod obciążeniem napięcie maksymalne rzędu 3600V, a spadek napięcia następuje
na jej rezystancji wewnętrznej oraz na przewodach trakcyjnych. Zasilaczem może
być również kabina sekcyjna, która jest zasilana z podstacji poprzez przewody
trakcyjne.
4. Dla uproszczenia można przyjąć, że zasilanie pojazdu odbywać się będzie z
dwóch sąsiednich podstacji, pomiędzy którymi może być dowolna liczba kabin
sekcyjnych. W przypadku wyłączenia jednej z tych podstacji, zasilanie może
być pobierane z kolejnej. Łącznie należy rozważać 4 najbliższe podstacje,
przy czym do obliczeń można przyjmować 2 z nich.
5. Przęsła sieci są łączone w listę dwukierunkową, więc wystarczy nazwę
sekcji wpisać w jednym z nich, wpisanie w każdym nie przeszkadza.
Alternatywnym sposobem łączenia segmentów naprężania może być wpisywanie
nazw przęseł jako "parallel", co może być uciążliwe dla autorów scenerii.
W skrajnych przęsłach należałoby dodatkowo wpisać nazwę zasilacza, będzie
to jednocześnie wskazanie przęsła, do którego podłączone są przewody
zasilające. Konieczne jest odróżnienie nazwy sekcji od nazwy zasilacza, co
można uzyskać różnicując ich nazwy albo np. specyficznie ustawiając wartość
prądu albo napięcia przęsła.
6. Jeśli dany segment naprężania jest wspólny dla dwóch sekcji zasilania,
to jedno z przęseł musi mieć nazwę "*" (gwiazdka), co będzie oznaczało, że
ma zamontowany izolator. Dla uzyskania efektów typu łuk elektryczny, należało
by wskazać położenie izolatora i jego długość (ew. typ).
7. Również w parametrach zasilacza należało by określić, czy jest podstacją,
czy jedynie kabiną sekcyjną. Różnić się one będą fizyką działania.
8. Dla zbudowanej topologii sekcji i zasilaczy należało by zbudować dynamiczny
schemat zastępczy. Dynamika polega na wyłączaniu sekcji ze zwarciem oraz
przeciążonych podstacji. Musi być też możliwość wyłączenia sekcji albo
podstacji za pomocą eventu.
9. Dla każdej sekcji musi być tworzony obiekt, wskazujący na podstacje
zasilające na końcach, stan włączenia, zwarcia, przepięcia. Do tego obiektu
musi wskazywać każde przęsło z aktywnym zasilaniem.

          z.1                  z.2             z.3
   -=-a---1*1---c-=---c---=-c--2*2--e---=---e-3-*-3--g-=-
   -=-b---1*1---d-=---d---=-d--2*2--f---=---e-3-*-3--h-=-

   nazwy sekcji (@): a,b,c,d,e,f,g,h
   nazwy zasilaczy (#): 1,2,3
   przęsło z izolatorem: *
   przęsła bez wskazania nazwy sekcji/zasilacza: -
   segment napręzania: =-x-=
   segment naprężania z izolatorem: =---@---#*#---@---=
   segment naprężania bez izolatora: =--------@------=

Obecnie brak nazwy sekcji nie jest akceptowany i każde przęsło musi mieć wpisaną
jawnie nazwę sekcji, ewentualnie nazwę zasilacza (zostanie zastąpiona wskazaniem
sekcji z sąsiedniego przęsła).
*/

TTraction::TTraction( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {}

void
TTraction::Load( cParser *parser, glm::dvec3 const &pOrigin ) {

    parser->getTokens( 4 );
    *parser
        >> asPowerSupplyName
        >> NominalVoltage
        >> MaxCurrent
        >> fResistivity;
    if( fResistivity == 0.01f ) {
        // tyle jest w sceneriach [om/km]
        // taka sensowniejsza wartość za http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
        fResistivity = 0.075f;
    }
    fResistivity *= 0.001f; // teraz [om/m]
    // Ra 2014-02: a tutaj damy symbol sieci i jej budowę, np.:
    // SKB70-C, CuCd70-2C, KB95-2C, C95-C, C95-2C, YC95-2C, YpC95-2C, YC120-2C
    // YpC120-2C, YzC120-2C, YwsC120-2C, YC150-C150, YC150-2C150, C150-C150
    // C120-2C, 2C120-2C, 2C120-2C-1, 2C120-2C-2, 2C120-2C-3, 2C120-2C-4
    auto const material = parser->getToken<std::string>();
    // 1=miedziana, rysuje się na zielono albo czerwono
    // 2=aluminiowa, rysuje się na czarno
         if( material == "none" ) { Material = 0; }
    else if( material == "al" )   { Material = 2; }
    else                          { Material = 1; }
    parser->getTokens( 2 );
    *parser
        >> WireThickness
        >> DamageFlag;
    pPoint1 = LoadPoint( *parser ) + pOrigin;
    pPoint2 = LoadPoint( *parser ) + pOrigin;
    pPoint3 = LoadPoint( *parser ) + pOrigin;
    pPoint4 = LoadPoint( *parser ) + pOrigin;
    auto const minheight { parser->getToken<double>() };
    fHeightDifference = ( pPoint3.y - pPoint1.y + pPoint4.y - pPoint2.y ) * 0.5 - minheight;
    auto const segmentlength { parser->getToken<double>() };
    iNumSections = (
        segmentlength ?
            glm::length( ( pPoint1 - pPoint2 ) ) / segmentlength :
            0 );
    parser->getTokens( 2 );
    *parser
        >> Wires
        >> WireOffset;
    m_visible = ( parser->getToken<std::string>() == "vis" );

    std::string token { parser->getToken<std::string>() };
    if( token == "parallel" ) {
        // jawne wskazanie innego przęsła, na które może przestawić się pantograf
        parser->getTokens();
        *parser >> asParallel;
    }
    while( ( false == token.empty() )
        && ( token != "endtraction" ) ) {

        token = parser->getToken<std::string>();
    }

    Init(); // przeliczenie parametrów

    // calculate traction location
    location( interpolate( pPoint2, pPoint1, 0.5 ) );
}

// retrieves list of the track's end points
std::vector<glm::dvec3>
TTraction::endpoints() const {

    return { pPoint1, pPoint2 };
}

std::size_t
TTraction::create_geometry( gfx::geometrybank_handle const &Bank ) {
    if( m_geometry != null_handle ) {
        return GfxRenderer.Vertices( m_geometry ).size() / 2;
    }

    gfx::vertex_array vertices;

    double ddp = std::hypot( pPoint2.x - pPoint1.x, pPoint2.z - pPoint1.z );
    if( Wires == 2 )
        WireOffset = 0;
    // jezdny
    gfx::basic_vertex startvertex, endvertex;
    startvertex.position =
        glm::vec3(
            pPoint1.x - ( pPoint2.z / ddp - pPoint1.z / ddp ) * WireOffset - m_origin.x,
            pPoint1.y - m_origin.y,
            pPoint1.z - ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
    endvertex.position =
        glm::vec3(
            pPoint2.x - ( pPoint2.z / ddp - pPoint1.z / ddp ) * WireOffset - m_origin.x,
            pPoint2.y - m_origin.y,
            pPoint2.z - ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
    vertices.emplace_back( startvertex );
    vertices.emplace_back( endvertex );
    // Nie wiem co 'Marcin
    glm::dvec3 pt1, pt2, pt3, pt4, v1, v2;
    v1 = pPoint4 - pPoint3;
    v2 = pPoint2 - pPoint1;
    float step = 0;
    if( iNumSections > 0 )
        step = 1.0f / (float)iNumSections;
    double f = step;
    float mid = 0.5;
    float t;
    // Przewod nosny 'Marcin
    if( Wires > 1 ) { // lina nośna w kawałkach
        startvertex.position =
            glm::vec3(
                pPoint3.x - m_origin.x,
                pPoint3.y - m_origin.y,
                pPoint3.z - m_origin.z );
        for( int i = 0; i < iNumSections - 1; ++i ) {
            pt3 = pPoint3 + v1 * f;
            t = ( 1 - std::fabs( f - mid ) * 2 );
            if( ( Wires < 4 )
             || ( ( i != 0 )
               && ( i != iNumSections - 2 ) ) ) {
                endvertex.position =
                    glm::vec3(
                        pt3.x - m_origin.x,
                        pt3.y - std::sqrt( t ) * fHeightDifference - m_origin.y,
                        pt3.z - m_origin.z );
                vertices.emplace_back( startvertex );
                vertices.emplace_back( endvertex );
                startvertex = endvertex;
            }
            f += step;
        }
        endvertex.position =
            glm::vec3(
                pPoint4.x - m_origin.x,
                pPoint4.y - m_origin.y,
                pPoint4.z - m_origin.z );
        vertices.emplace_back( startvertex );
        vertices.emplace_back( endvertex );
    }
    // Drugi przewod jezdny 'Winger
    if( Wires > 2 ) {
        startvertex.position =
            glm::vec3(
                pPoint1.x + ( pPoint2.z / ddp - pPoint1.z / ddp ) * WireOffset - m_origin.x,
                pPoint1.y - m_origin.y,
                pPoint1.z + ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
        endvertex.position =
            glm::vec3(
                pPoint2.x + ( pPoint2.z / ddp - pPoint1.z / ddp ) * WireOffset - m_origin.x,
                pPoint2.y - m_origin.y,
                pPoint2.z + ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
        vertices.emplace_back( startvertex );
        vertices.emplace_back( endvertex );
    }

    f = step;

    if( Wires == 4 ) {
        startvertex.position =
            glm::vec3(
                pPoint3.x - m_origin.x,
                pPoint3.y - 0.65f * fHeightDifference - m_origin.y,
                pPoint3.z - m_origin.z );
        for( int i = 0; i < iNumSections - 1; ++i ) {
            pt3 = pPoint3 + v1 * f;
            t = ( 1 - std::fabs( f - mid ) * 2 );
            endvertex.position =
                glm::vec3(
                    pt3.x - m_origin.x,
                    pt3.y - std::sqrt( t ) * fHeightDifference - (
                        ( ( i == 0 )
                       || ( i == iNumSections - 2 ) ) ?
                            0.25f * fHeightDifference :
                            0.05 )
                        - m_origin.y,
                    pt3.z - m_origin.z );
            vertices.emplace_back( startvertex );
            vertices.emplace_back( endvertex );
            startvertex = endvertex;
            f += step;
        }
        endvertex.position =
            glm::vec3(
                pPoint4.x - m_origin.x,
                pPoint4.y - 0.65f * fHeightDifference - m_origin.y,
                pPoint4.z - m_origin.z );
        vertices.emplace_back( startvertex );
        vertices.emplace_back( endvertex );
    }
    f = step;

    // Przewody pionowe (wieszaki) 'Marcin, poprawki na 2 przewody jezdne 'Winger
    if( Wires > 1 ) {
        auto const flo { static_cast<float>( Wires == 4 ? 0.25f * fHeightDifference : 0 ) };
        auto const flo1 { static_cast<float>( Wires == 4 ? +0.05 : 0 ) };
        for( int i = 0; i < iNumSections - 1; ++i ) {
            pt3 = pPoint3 + v1 * f;
            pt4 = pPoint1 + v2 * f;
            t = ( 1 - std::fabs( f - mid ) * 2 );
            if( ( i % 2 ) == 0 ) {
                startvertex.position =
                    glm::vec3(
                        pt3.x - m_origin.x,
                        pt3.y - std::sqrt( t ) * fHeightDifference - ( ( i == 0 ) || ( i == iNumSections - 2 ) ? flo : flo1 ) - m_origin.y,
                        pt3.z - m_origin.z );
                endvertex.position =
                    glm::vec3(
                        pt4.x - ( pPoint2.z / ddp - pPoint1.z / ddp ) * WireOffset - m_origin.x,
                        pt4.y - m_origin.y,
                        pt4.z - ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
                vertices.emplace_back( startvertex );
                vertices.emplace_back( endvertex );
            }
            else {
                startvertex.position =
                    glm::vec3(
                        pt3.x - m_origin.x,
                        pt3.y - std::sqrt( t ) * fHeightDifference - ( ( i == 0 ) || ( i == iNumSections - 2 ) ? flo : flo1 ) - m_origin.y,
                        pt3.z - m_origin.z );
                endvertex.position =
                    glm::vec3(
                        pt4.x + ( pPoint2.z / ddp - pPoint1.z / ddp ) * WireOffset - m_origin.x,
                        pt4.y - m_origin.y,
                        pt4.z + ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
                vertices.emplace_back( startvertex );
                vertices.emplace_back( endvertex );
            }
            if( ( ( Wires == 4 )
             && ( ( i == 1 )
               || ( i == iNumSections - 3 ) ) ) ) {
                startvertex.position =
                    glm::vec3(
                        pt3.x - m_origin.x,
                        pt3.y - std::sqrt( t ) * fHeightDifference - 0.05 - m_origin.y,
                        pt3.z - m_origin.z );
                endvertex.position =
                    glm::vec3(
                        pt3.x - m_origin.x,
                        pt3.y - std::sqrt( t ) * fHeightDifference - m_origin.y,
                        pt3.z - m_origin.z );
                vertices.emplace_back( startvertex );
                vertices.emplace_back( endvertex );
            }
            f += step;
        }
    }

    auto const elementcount = vertices.size() / 2;
    m_geometry = GfxRenderer.Insert( vertices, Bank, GL_LINES );

    return elementcount;
}

int TTraction::TestPoint(glm::dvec3 const &Point)
{ // sprawdzanie, czy przęsła można połączyć
    if( ( hvNext[ 0 ] == nullptr )
     && ( glm::all( glm::epsilonEqual( Point, pPoint1, 0.025 ) ) ) ) {
        return 0;
    }
    if( ( hvNext[ 1 ] == nullptr )
     && ( glm::all( glm::epsilonEqual( Point, pPoint2, 0.025 ) ) ) ) {
        return 1;
    }
    return -1;
};

//łączenie segmentu (with) od strony (my) do jego (to)
void
TTraction::Connect(int my, TTraction *with, int to) {

    hvNext[ my ] = with;
    iNext[ my ] = to;

    if( ( hvNext[ 0 ] != nullptr )
     && ( hvNext[ 1 ] != nullptr ) ) {
        // jeśli z obu stron podłączony to nie jest ostatnim
        iLast = 0;
    }

    with->hvNext[ to ] = this;
    with->iNext[ to ] = my;

    if( ( with->hvNext[ 0 ] != nullptr )
     && ( with->hvNext[ 1 ] != nullptr ) ) {
        // temu też, bo drugi raz łączenie się nie nie wykona
        with->iLast = 0;
    }
}

// ustalenie przedostatnich przęseł
bool TTraction::WhereIs() { 

    if( iLast ) {
        // ma już ustaloną informację o położeniu
        return ( iLast & 1 );
    }
    for( int endindex = 0; endindex < 2; ++endindex ) {
        if( hvNext[ endindex ] == nullptr ) {
            // no neighbour piece means this one is last
            iLast |= 1;
        }
        else if( hvNext[ endindex ]->hvNext[ 1 - iNext[ endindex ] ] == nullptr ) {
            // otherwise if that neighbour has no further connection on the opposite end then this piece is second-last
            iLast |= 2;
        }
    }
    return (iLast & 1); // ostatnie będą dostawać zasilanie
}

void TTraction::Init()
{ // przeliczenie parametrów
    vParametric = pPoint2 - pPoint1; // wektor mnożników parametru dla równania parametrycznego
};

void TTraction::ResistanceCalc(int d, double r, TTractionPowerSource *ps)
{ //(this) jest przęsłem zasilanym, o rezystancji (r), policzyć rezystancję zastępczą sąsiednich
    if (d >= 0)
    { // podążanie we wskazanym kierunku
        TTraction *t = hvNext[d], *p;
        if (ps)
            psPower[d ^ 1] = ps; // podłączenie podanego
        else
            ps = psPower[d ^ 1]; // zasilacz od przeciwnej strony niż idzie analiza
        d = iNext[d]; // kierunek
        PowerState = 4;
        while( ( t != nullptr )
            && ( t->psPower[d] == nullptr ) ) // jeśli jest jakiś kolejny i nie ma ustalonego zasilacza
        { // ustawienie zasilacza i policzenie rezystancji zastępczej
            if( t->PowerState != 4 ) {
                // przęsła zasilającego nie modyfikować
                if( t->psPowered != nullptr ) {

                    t->PowerState = 4;
                }
                else {
                    // kolor zależny od strony, z której jest zasilanie
                    t->PowerState |= d ? 2 : 1;
                }
            }
            t->psPower[d] = ps; // skopiowanie wskaźnika zasilacza od danej strony
            t->fResistance[d] = r; // wpisanie rezystancji w kierunku tego zasilacza
            r += t->fResistivity * glm::length(t->vParametric); // doliczenie oporu kolejnego odcinka
            p = t; // zapamiętanie dotychczasowego
            t = p->hvNext[d ^ 1]; // podążanie w tę samą stronę
            d = p->iNext[d ^ 1];
            // w przypadku zapętlenia sieci może się zawiesić?
        }
    }
    else
    { // podążanie w obu kierunkach, można by rekurencją, ale szkoda zasobów
        r = 0.5 * fResistivity * glm::length(vParametric); // powiedzmy, że w zasilanym przęśle jest połowa
        if (fResistance[0] == 0.0)
            ResistanceCalc(0, r); // do tyłu (w stronę Point1)
        if (fResistance[1] == 0.0)
            ResistanceCalc(1, r); // do przodu (w stronę Point2)
    }
};

void TTraction::PowerSet(TTractionPowerSource *ps)
{ // podłączenie przęsła do zasilacza
    if (ps->bSection)
        psSection = ps; // ustalenie sekcji zasilania
    else
    { // ustalenie punktu zasilania (nie ma jeszcze połączeń między przęsłami)
        psPowered = ps; // ustawienie bezpośredniego zasilania dla przęsła
        psPower[0] = psPower[1] = ps; // a to chyba nie jest dobry pomysł, bo nawet zasilane przęsło
        // powinno mieć wskazania na inne
        fResistance[0] = fResistance[1] = 0.0; // a liczy się tylko rezystancja zasilacza
    }
};

double TTraction::VoltageGet(double u, double i)
{ // pobranie napięcia na przęśle po podłączeniu do niego rezystancji (res) - na razie jest to prąd
    if (!psSection)
        if (!psPowered)
            return NominalVoltage; // jak nie ma zasilacza, to napięcie podane w przęśle
    // na początek można założyć, że wszystkie podstacje mają to samo napięcie i nie płynie prąd
    // pomiędzy nimi
    // dla danego przęsła mamy 3 źródła zasilania
    // 1. zasilacz psPower[0] z rezystancją fResistance[0] oraz jego wewnętrzną
    // 2. zasilacz psPower[1] z rezystancją fResistance[1] oraz jego wewnętrzną
    // 3. zasilacz psPowered z jego wewnętrzną rezystancją dla przęseł zasilanych bezpośrednio
    double res = (
        (i != 0.0) ?
            (u / i) :
            10000.0 );
    if( psPowered != nullptr ) {
        // yB: dla zasilanego nie baw się w gwiazdy, tylko bierz bezpośrednio
        return (
            ( res != 0.0 ) ?
                psPowered->CurrentGet( res ) * res :
                0.0 );
    }
    double r0t, r1t, r0g, r1g;
    double i0, i1;
    r0t = fResistance[0]; //średni pomysł, ale lepsze niż nic
    r1t = fResistance[1]; // bo nie uwzględnia spadków z innych pojazdów
    if (psPower[0] && psPower[1])
    { // gdy przęsło jest zasilane z obu stron - mamy trójkąt: res, r0t, r1t
        // yB: Gdy wywali podstacja, to zaczyna się robić nieciekawie - napięcie w sekcji na jednym
        // końcu jest równe zasilaniu,
        // yB: a na drugim końcu jest równe 0. Kolejna sprawa to rozróżnienie uszynienia sieci na
        // podstacji/odłączniku (czyli
        // yB: potencjał masy na sieci) od braku zasilania (czyli odłączenie źródła od sieci i brak
        // jego wpływu na napięcie).
        if ((r0t > 0.0) && (r1t > 0.0))
        { // rezystancje w mianowniku nie mogą być zerowe
            r0g = res + r0t + (res * r0t) / r1t; // przeliczenie z trójkąta na gwiazdę
            r1g = res + r1t + (res * r1t) / r0t;
            // pobierane są prądy dla każdej rezystancji, a suma jest mnożona przez rezystancję
            // pojazdu w celu uzyskania napięcia
            i0 = psPower[0]->CurrentGet(r0g); // oddzielnie dla sprawdzenia
            i1 = psPower[1]->CurrentGet(r1g);
            return (i0 + i1) * res;
        }
        else if (r0t >= 0.0)
            return psPower[0]->CurrentGet(res + r0t) * res;
        else if (r1t >= 0.0)
            return psPower[1]->CurrentGet(res + r1t) * res;
        else
            return 0.0; // co z tym zrobić?
    }
    else if (psPower[0] && (r0t >= 0.0))
    { // jeśli odcinek podłączony jest tylko z jednej strony
        return psPower[0]->CurrentGet(res + r0t) * res;
    }
    else if (psPower[1] && (r1t >= 0.0))
        return psPower[1]->CurrentGet(res + r1t) * res;
    return 0.0; // gdy nie podłączony wcale?
};

glm::vec3
TTraction::wire_color() const {

    glm::vec3 color;
    if( !DebugModeFlag || GfxRenderer.settings.force_normal_traction_render )
    {
        switch( Material ) { // Ra: kolory podzieliłem przez 2, bo po zmianie ambient za jasne były
                             // trzeba uwzględnić kierunek świecenia Słońca - tylko ze Słońcem widać kolor
            case 1: {
                if( TestFlag( DamageFlag, 1 ) ) {
                    color.r = 0.00000f;
                    color.g = 0.32549f;
                    color.b = 0.2882353f; // zielona miedź
                }
                else {
                    color.r = 0.35098f;
                    color.g = 0.22549f;
                    color.b = 0.1f; // czerwona miedź
                }
                break;
            }
            case 2: {
                if( TestFlag( DamageFlag, 1 ) ) {
                    color.r = 0.10f;
                    color.g = 0.10f;
                    color.b = 0.10f; // czarne Al
                }
                else {
                    color.r = 0.25f;
                    color.g = 0.25f;
                    color.b = 0.25f; // srebrne Al
                }
                break;
            }
            default: {break; }
        }
		color *= 0.2;
        // w zaleźności od koloru swiatła
        color.r *= Global.DayLight.ambient[ 0 ];
        color.g *= Global.DayLight.ambient[ 1 ];
        color.b *= Global.DayLight.ambient[ 2 ];
        color *= 0.35f;
    }
    else {
        // tymczasowo pokazanie zasilanych odcinków
        switch( PowerState ) {

            case 1: {
                // cyan
                color = glm::vec3 { 0.f, 174.f / 255.f, 239.f / 255.f };
                break;
            }
            case 2: {
                // yellow
                color = glm::vec3 { 240.f / 255.f, 228.f / 255.f, 0.f };
                break;
            }
            case 3: {
                // green
                color = glm::vec3 { 0.f, 239.f / 255.f, 118.f / 255.f };
                break;
            }
            case 4: {
                // white for powered, red for ends
                color = (
                    psPowered != nullptr ?
                        glm::vec3{ 239.f / 255.f, 239.f / 255.f, 239.f / 255.f } :
                        glm::vec3{ 239.f / 255.f, 128.f / 255.f, 128.f / 255.f } );
                break;
            }
            default: { break; }
        }
        if( hvParallel ) { // jeśli z bieżnią wspólną, to dodatkowo przyciemniamy
            color.r *= 0.6f;
            color.g *= 0.6f;
            color.b *= 0.6f;
        }
    }
    return color;
}

// radius() subclass details, calculates node's bounding radius
float
TTraction::radius_() {

    auto const points { endpoints() };
    auto radius { 0.f };
    for( auto &point : points ) {
        radius = std::max(
            radius,
            static_cast<float>( glm::length( m_area.center - point ) ) );
    }
    return radius;
}

// serialize() subclass details, sends content of the subclass to provided stream
void
TTraction::serialize_( std::ostream &Output ) const {

    // TODO: implement
}
// deserialize() subclass details, restores content of the subclass from provided stream
void
TTraction::deserialize_( std::istream &Input ) {

    // TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void
TTraction::export_as_text_( std::ostream &Output ) const {
    // header
    Output << "traction ";
    // basic attributes
    Output
        << asPowerSupplyName << ' '
        << NominalVoltage << ' '
        << MaxCurrent << ' '
        << ( fResistivity * 1000 ) << ' '
        << (
            Material == 2 ? "al" :
            Material == 0 ? "none" :
            "cu" ) << ' '
        << WireThickness << ' '
        << DamageFlag << ' ';
    // path data
    Output
        << pPoint1.x << ' ' << pPoint1.y << ' ' << pPoint1.z << ' '
        << pPoint2.x << ' ' << pPoint2.y << ' ' << pPoint2.z << ' '
        << pPoint3.x << ' ' << pPoint3.y << ' ' << pPoint3.z << ' '
        << pPoint4.x << ' ' << pPoint4.y << ' ' << pPoint4.z << ' ';
    // minimum height
    Output << ( ( pPoint3.y - pPoint1.y + pPoint4.y - pPoint2.y ) * 0.5 - fHeightDifference ) << ' ';
    // segment length
    Output << static_cast<int>( iNumSections ? glm::length( pPoint1 - pPoint2 ) / iNumSections : 0.0 ) << ' ';
    // wire data
    Output
        << Wires << ' '
        << WireOffset << ' ';
    // visibility
    // NOTE: 'invis' would be less wrong than 'unvis', but potentially incompatible with old 3rd party tools
    Output << ( m_visible ? "vis" : "unvis" ) << ' ';
    // optional attributes
    if( false == asParallel.empty() ) {
        Output << "parallel " << asParallel << ' ';
    }
    // footer
    Output
        << "endtraction"
        << "\n";
}



// legacy method, initializes traction after deserialization from scenario file
void
traction_table::InitTraction() {

 //łączenie drutów ze sobą oraz z torami i eventami
//    TGroundNode *nCurrent, *nTemp;
//    TTraction *tmp; // znalezione przęsło

    int connection { -1 };
    TTraction *matchingtraction { nullptr };

    for( auto *traction : m_items ) {
        // podłączenie do zasilacza, żeby można było sumować prąd kilku pojazdów
        // a jednocześnie z jednego miejsca zmieniać napięcie eventem
        // wykonywane najpierw, żeby można było logować podłączenie 2 zasilaczy do jednego drutu
        // izolator zawieszony na przęśle jest ma być osobnym odcinkiem drutu o długości ok. 1m,
        // podłączonym do zasilacza o nazwie "*" (gwiazka); "none" nie będzie odpowiednie
        auto *powersource = simulation::Powergrid.find( traction->asPowerSupplyName );
        if( powersource ) {
            // jak zasilacz znaleziony to podłączyć do przęsła
            traction->PowerSet( powersource );
        }
        else {
            if( ( traction->asPowerSupplyName != "*" ) // gwiazdka dla przęsła z izolatorem
             && ( traction->asPowerSupplyName != "none" ) ) { // dopuszczamy na razie brak podłączenia?
                // logowanie błędu i utworzenie zasilacza o domyślnej zawartości
                ErrorLog( "Bad scenario: traction piece connected to nonexistent power source \"" + traction->asPowerSupplyName + "\"" );
                scene::node_data nodedata;
                nodedata.name = traction->asPowerSupplyName;
                powersource = new TTractionPowerSource( nodedata );
                powersource->Init( traction->NominalVoltage, traction->MaxCurrent );
                simulation::Powergrid.insert( powersource );
            }
        }
    }

#ifdef EU07_IGNORE_LEGACYPROCESSINGORDER
    for( auto *traction : m_items ) {
#else
    // NOTE: legacy code peformed item operations last-to-first due to way the items were added to the list
    // this had impact in situations like two possible connection candidates, where only the first one would be used
    for( auto first = std::rbegin(m_items); first != std::rend(m_items); ++first ) {
        auto *traction = *first;
#endif
        if( traction->hvNext[ 0 ] == nullptr ) {
            // tylko jeśli jeszcze nie podłączony
            std::tie( matchingtraction, connection ) = simulation::Region->find_traction( traction->pPoint1, traction );
            switch (connection) {
                case 0:
                case 1: {
                    traction->Connect( 0, matchingtraction, connection );
                    break;
                }
                default: {
                    break;
                }
            }
            if( traction->hvNext[ 0 ] ) {
                // jeśli został podłączony
                if( ( traction->psSection != nullptr )
                 && ( matchingtraction->psSection != nullptr ) ) {
                    // tylko przęsło z izolatorem może nie mieć zasilania, bo ma 2, trzeba sprawdzać sąsiednie
                    if( traction->psSection != matchingtraction->psSection ) {
                        // połączone odcinki mają różne zasilacze
                        // to może być albo podłączenie podstacji lub kabiny sekcyjnej do sekcji, albo błąd
                        if( ( true == traction->psSection->bSection )
                         && ( false == matchingtraction->psSection->bSection ) ) {
                            //(tmp->psSection) jest podstacją, a (Traction->psSection) nazwą sekcji
                            matchingtraction->PowerSet( traction->psSection ); // zastąpienie wskazaniem sekcji
                        }
                        else if( ( false == traction->psSection->bSection )
                              && ( true == matchingtraction->psSection->bSection ) ) {
                            //(Traction->psSection) jest podstacją, a (tmp->psSection) nazwą sekcji
                            traction->PowerSet( matchingtraction->psSection ); // zastąpienie wskazaniem sekcji
                        }
                        else {
                            // jeśli obie to sekcje albo obie podstacje, to będzie błąd
                            ErrorLog( "Bad scenario: faulty traction power connection at location " + to_string( traction->pPoint1 ) );
                        }
                    }
                }
            }
        }
        if( traction->hvNext[ 1 ] == nullptr ) {
            // tylko jeśli jeszcze nie podłączony
            std::tie( matchingtraction, connection ) = simulation::Region->find_traction( traction->pPoint2, traction );
            switch (connection) {
                case 0:
                case 1: {
                    traction->Connect( 1, matchingtraction, connection );
                    break;
                }
                default: {
                    break;
                }
            }
            if( traction->hvNext[ 1 ] ) {
                // jeśli został podłączony
                if( ( traction->psSection != nullptr )
                 && ( matchingtraction->psSection != nullptr ) ) {
                    // tylko przęsło z izolatorem może nie mieć zasilania, bo ma 2, trzeba sprawdzać sąsiednie
                    if( traction->psSection != matchingtraction->psSection ) {
                        // to może być albo podłączenie podstacji lub kabiny sekcyjnej do sekcji, albo błąd
                        if( ( true == traction->psSection->bSection )
                         && ( false == matchingtraction->psSection->bSection ) ) {
                            //(tmp->psSection) jest podstacją, a (Traction->psSection) nazwą sekcji
                            matchingtraction->PowerSet( traction->psSection ); // zastąpienie wskazaniem sekcji
                        }
                        else if( ( false == traction->psSection->bSection )
                              && ( true == matchingtraction->psSection->bSection ) ) {
                            //(Traction->psSection) jest podstacją, a (tmp->psSection) nazwą sekcji
                            traction->PowerSet( matchingtraction->psSection ); // zastąpienie wskazaniem sekcji
                        }
                        else {
                            // jeśli obie to sekcje albo obie podstacje, to będzie błąd
                            ErrorLog( "Bad scenario: faulty traction power connection at location " + to_string( traction->pPoint2 ) );
                        }
                    }
                }
            }
        }
    }

    auto endcount { 0 };
    for( auto *traction : m_items ) {
        // operacje mające na celu wykrywanie bieżni wspólnych i łączenie przęseł naprążania
        if( traction->WhereIs() ) {
            // true for outer pieces of the traction section
            traction->iTries = 5; // oznaczanie końcowych
            ++endcount;
        }
        if (traction->fResistance[0] == 0.0) {
            // obliczanie przęseł w segmencie z bezpośrednim zasilaniem
            traction->ResistanceCalc();
            traction->iTries = 0; // nie potrzeba mu szukać zasilania
        }
    }

    std::vector<TTraction *> ends; ends.reserve( endcount );
    for( auto *traction : m_items ) {
        //łączenie bieżni wspólnych, w tym oznaczanie niepodanych jawnie
        if( false == traction->asParallel.empty() ) {
            // będzie wskaźnik na inne przęsło
            if( ( traction->asParallel == "none" )
             || ( traction->asParallel == "*" ) ) {
                // jeśli nieokreślone
                traction->iLast |= 2; // jakby przedostatni - niech po prostu szuka (iLast już przeliczone)
            }
            else if( traction->hvParallel == nullptr ) {
                // jeśli jeszcze nie został włączony w kółko
                auto *nTemp = find( traction->asParallel );
                if( nTemp != nullptr ) {
                    // o ile zostanie znalezione przęsło o takiej nazwie
                    if( nTemp->hvParallel == nullptr ) {
                        // jeśli tamten jeszcze nie ma wskaźnika bieżni wspólnej
                        traction->hvParallel = nTemp; // wpisać siebie i dalej dać mu wskaźnik zwrotny
                    }
                    else {
                        // a jak ma, to albo dołączyć się do kółeczka
                        traction->hvParallel = nTemp->hvParallel; // przjąć dotychczasowy wskaźnik od niego
                    }
                    nTemp->hvParallel = traction; // i na koniec ustawienie wskaźnika zwrotnego
                }
                if( traction->hvParallel == nullptr ) {
                    ErrorLog( "Missed overhead: " + traction->asParallel ); // logowanie braku
                }
            }
        }
        if( traction->iTries == 5 ) {
            // jeśli zaznaczony do podłączenia
            // wypełnianie tabeli końców w celu szukania im połączeń
            ends.emplace_back( traction );
        }
    }

    bool connected; // nieefektywny przebieg kończy łączenie
    do {
        // ustalenie zastępczej rezystancji dla każdego przęsła
        // flaga podłączonych przęseł końcowych: -1=puste wskaźniki, 0=coś zostało, 1=wykonano łączenie
        connected = false;
        for( auto &end : ends ) {
            // załatwione będziemy zerować
            if( end == nullptr ) { continue; }
            // każdy przebieg to próba podłączenia końca segmentu naprężania do innego zasilanego przęsła
            if( end->hvNext[ 0 ] != nullptr ) {
                // jeśli końcowy ma ciąg dalszy od strony 0 (Point1), szukamy odcinka najbliższego do Point2
                std::tie( matchingtraction, connection ) = simulation::Region->find_traction( end->pPoint2, end, 0 );
                if( matchingtraction != nullptr ) {
                    // jak znalezione przęsło z zasilaniem, to podłączenie "równoległe"
                    end->ResistanceCalc( 0, matchingtraction->fResistance[ connection ], matchingtraction->psPower[ connection ] );
                    // jak coś zostało podłączone, to może zasilanie gdzieś dodatkowo dotrze
                    connected = true;
                    end = nullptr;
                }
            }
            else if( end->hvNext[ 1 ] != nullptr ) {
                // jeśli końcowy ma ciąg dalszy od strony 1 (Point2), szukamy odcinka najbliższego do Point1
                std::tie( matchingtraction, connection ) = simulation::Region->find_traction( end->pPoint1, end, 1 );
                if( matchingtraction != nullptr ) {
                    // jak znalezione przęsło z zasilaniem, to podłączenie "równoległe"
                    end->ResistanceCalc( 1, matchingtraction->fResistance[ connection ], matchingtraction->psPower[ connection ] );
                    // jak coś zostało podłączone, to może zasilanie gdzieś dodatkowo dotrze
                    connected = true;
                    end = nullptr;
                }
            }
            else {
                // gdy koniec jest samotny, to na razie nie zostanie podłączony (nie powinno takich być)
                end = nullptr;
            }
        }
    } while( true == connected );
}
