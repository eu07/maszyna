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
#include "Globals.h"
#include "logs.h"
#include "mctools.h"
#include "TractionPower.h"

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

// legacy constructor
TTraction::TTraction( std::string Name ) {

    m_name = Name;
}

glm::dvec3 LoadPoint( cParser &Input ) {
    // pobranie współrzędnych punktu
    glm::dvec3 point;
    Input.getTokens( 3 );
    Input
        >> point.x
        >> point.y
        >> point.z;
    return point;
}

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
    m_location = interpolate( pPoint2, pPoint1, 0.5 );
}

std::size_t
#ifdef EU07_USE_OLD_GROUNDCODE
TTraction::create_geometry( geometrybank_handle const &Bank, glm::dvec3 const &Origin ) {
#else
TTraction::create_geometry( geometrybank_handle const &Bank ) {
#endif
    if( m_geometry != null_handle ) {
        return GfxRenderer.Vertices( m_geometry ).size() / 2;
    }

#ifdef EU07_USE_OLD_GROUNDCODE
    if( Bank != 0 ) {
        m_origin = Origin;
    }
#endif

    vertex_array vertices;

    double ddp = std::hypot( pPoint2.x - pPoint1.x, pPoint2.z - pPoint1.z );
    if( Wires == 2 )
        WireOffset = 0;
    // jezdny
    basic_vertex startvertex, endvertex;
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
        for( int i = 0; i < iNumSections - 1; ++i ) {
            float flo, flo1;
            flo = ( Wires == 4 ? 0.25f * fHeightDifference : 0 );
            flo1 = ( Wires == 4 ? +0.05 : 0 );
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
                        pt4.z - ( -pPoint2.x / ddp + pPoint1.x / ddp ) * WireOffset - m_origin.z );
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
    if (!hvNext[0])
        if( glm::all( glm::epsilonEqual( Point, pPoint1, 0.025 ) ) )
            return 0;
    if (!hvNext[1])
        if( glm::all( glm::epsilonEqual( Point, pPoint2, 0.025 ) ) )
            return 1;
    return -1;
};

void TTraction::Connect(int my, TTraction *with, int to)
{ //łączenie segmentu (with) od strony (my) do jego (to)
    if (my)
    { // do mojego Point2
        hvNext[1] = with;
        iNext[1] = to;
    }
    else
    { // do mojego Point1
        hvNext[0] = with;
        iNext[0] = to;
    }
    if (to)
    { // do jego Point2
        with->hvNext[1] = this;
        with->iNext[1] = my;
    }
    else
    { // do jego Point1
        with->hvNext[0] = this;
        with->iNext[0] = my;
    }
    if (hvNext[0]) // jeśli z obu stron podłączony
        if (hvNext[1])
            iLast = 0; // to nie jest ostatnim
    if (with->hvNext[0]) // temu też, bo drugi raz łączenie się nie nie wykona
        if (with->hvNext[1])
            with->iLast = 0; // to nie jest ostatnim
};

bool TTraction::WhereIs()
{ // ustalenie przedostatnich przęseł
    if (iLast)
        return (iLast == 1); // ma już ustaloną informację o położeniu
    if (hvNext[0] ? hvNext[0]->iLast == 1 : false) // jeśli poprzedni jest ostatnim
        iLast = 2; // jest przedostatnim
    else if (hvNext[1] ? hvNext[1]->iLast == 1 : false) // jeśli następny jest ostatnim
        iLast = 2; // jest przedostatnim
    return (iLast == 1); // ostatnie będą dostawać zasilanie
};

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
    double res = (i != 0.0) ? (u / i) : 10000.0;
    if (psPowered)
        return psPowered->CurrentGet(res) *
               res; // yB: dla zasilanego nie baw się w gwiazdy, tylko bierz bezpośrednio
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
    if( false == DebugModeFlag ) {
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
        // w zaleźności od koloru swiatła
        color.r *= Global::DayLight.ambient[ 0 ];
        color.g *= Global::DayLight.ambient[ 1 ];
        color.b *= Global::DayLight.ambient[ 2 ];
    }
    else {
        // tymczasowo pokazanie zasilanych odcinków
        switch( PowerState ) {

            case 1: {
                // czerwone z podłączonym zasilaniem 1
                color.r = 1.0f;
                color.g = 0.0f;
                color.b = 0.0f;
                break;
            }
            case 2: {
                // zielone z podłączonym zasilaniem 2
                color.r = 0.0f;
                color.g = 1.0f;
                color.b = 0.0f;
                break;
            }
            case 3: {
                //żółte z podłączonym zasilaniem z obu stron
                color.r = 1.0f;
                color.g = 1.0f;
                color.b = 0.0f;
                break;
            }
            case 4: {
                // niebieskie z podłączonym zasilaniem
                color.r = 0.5f;
                color.g = 0.5f;
                color.b = 1.0f;
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



// legacy method, initializes traction after deserialization from scenario file
void
traction_table::InitTraction() {
/*
 //łączenie drutów ze sobą oraz z torami i eventami
//    TGroundNode *nCurrent, *nTemp;
//    TTraction *tmp; // znalezione przęsło
    int iConnection;
    std::string name;
    for( auto *traction : m_items ) {
        // podłączenie do zasilacza, żeby można było sumować prąd kilku pojazdów
        // a jednocześnie z jednego miejsca zmieniać napięcie eventem
        // wykonywane najpierw, żeby można było logować podłączenie 2 zasilaczy do jednego drutu
        // izolator zawieszony na przęśle jest ma być osobnym odcinkiem drutu o długości ok. 1m,
        // podłączonym do zasilacza o nazwie "*" (gwiazka); "none" nie będzie odpowiednie
        nTemp = FindGroundNode(traction->asPowerSupplyName, TP_TRACTIONPOWERSOURCE);
        if (nTemp) // jak zasilacz znaleziony
            traction->PowerSet(nTemp->psTractionPowerSource); // to podłączyć do przęsła
        else if (traction->asPowerSupplyName != "*") // gwiazdka dla przęsła z izolatorem
            if (traction->asPowerSupplyName != "none") // dopuszczamy na razie brak podłączenia?
            { // logowanie błędu i utworzenie zasilacza o domyślnej zawartości
                ErrorLog("Missed TractionPowerSource: " + traction->asPowerSupplyName);
                nTemp = new TGroundNode();
                nTemp->iType = TP_TRACTIONPOWERSOURCE;
                nTemp->asName = traction->asPowerSupplyName;
                nTemp->psTractionPowerSource = new TTractionPowerSource(nTemp);
                nTemp->psTractionPowerSource->Init(traction->NominalVoltage, traction->MaxCurrent);
                nTemp->nNext = nRootOfType[nTemp->iType]; // ostatni dodany dołączamy na końcu
                // nowego
                nRootOfType[nTemp->iType] = nTemp; // ustawienie nowego na początku listy
            }
    }

    for( auto *traction : m_items ) {

        if (!traction->hvNext[0]) // tylko jeśli jeszcze nie podłączony
        {
            tmp = FindTraction(traction->pPoint1, iConnection, nCurrent);
            switch (iConnection)
            {
            case 0:
                traction->Connect(0, tmp, 0);
                break;
            case 1:
                traction->Connect(0, tmp, 1);
                break;
            }
            if (traction->hvNext[0]) // jeśli został podłączony
                if (traction->psSection && tmp->psSection) // tylko przęsło z izolatorem może nie
                    // mieć zasilania, bo ma 2, trzeba
                    // sprawdzać sąsiednie
                    if (traction->psSection !=
                        tmp->psSection) // połączone odcinki mają różne zasilacze
                    { // to może być albo podłączenie podstacji lub kabiny sekcyjnej do sekcji, albo
                        // błąd
                        if (traction->psSection->bSection && !tmp->psSection->bSection)
                        { //(tmp->psSection) jest podstacją, a (Traction->psSection) nazwą sekcji
                            tmp->PowerSet(traction->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else if (!traction->psSection->bSection && tmp->psSection->bSection)
                        { //(Traction->psSection) jest podstacją, a (tmp->psSection) nazwą sekcji
                            traction->PowerSet(tmp->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else // jeśli obie to sekcje albo obie podstacje, to będzie błąd
                            ErrorLog("Bad power: at " +
                                     to_string(traction->pPoint1.x, 2, 6) + " " +
                                     to_string(traction->pPoint1.y, 2, 6) + " " +
                                     to_string(traction->pPoint1.z, 2, 6));
                    }
        }
        if (!traction->hvNext[1]) // tylko jeśli jeszcze nie podłączony
        {
            tmp = FindTraction(traction->pPoint2, iConnection, nCurrent);
            switch (iConnection)
            {
            case 0:
                traction->Connect(1, tmp, 0);
                break;
            case 1:
                traction->Connect(1, tmp, 1);
                break;
            }
            if (traction->hvNext[1]) // jeśli został podłączony
                if (traction->psSection && tmp->psSection) // tylko przęsło z izolatorem może nie
                    // mieć zasilania, bo ma 2, trzeba
                    // sprawdzać sąsiednie
                    if (traction->psSection != tmp->psSection)
                    { // to może być albo podłączenie podstacji lub kabiny sekcyjnej do sekcji, albo
                        // błąd
                        if (traction->psSection->bSection && !tmp->psSection->bSection)
                        { //(tmp->psSection) jest podstacją, a (Traction->psSection) nazwą sekcji
                            tmp->PowerSet(traction->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else if (!traction->psSection->bSection && tmp->psSection->bSection)
                        { //(Traction->psSection) jest podstacją, a (tmp->psSection) nazwą sekcji
                            traction->PowerSet(tmp->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else // jeśli obie to sekcje albo obie podstacje, to będzie błąd
                            ErrorLog("Bad power: at " +
                                     to_string(traction->pPoint2.x, 2, 6) + " " +
                                     to_string(traction->pPoint2.y, 2, 6) + " " +
                                     to_string(traction->pPoint2.z, 2, 6));
                    }
        }
    }

    iConnection = 0; // teraz będzie licznikiem końców
    for( auto *traction : m_items ) {
        // operacje mające na celu wykrywanie bieżni wspólnych i łączenie przęseł naprążania
        if (traction->WhereIs()) // oznakowanie przedostatnich przęseł
        { // poszukiwanie bieżni wspólnej dla przedostatnich przęseł, również w celu połączenia
            // zasilania
            // to się nie sprawdza, bo połączyć się mogą dwa niezasilane odcinki jako najbliższe
            // sobie
            // nCurrent->hvTraction->hvParallel=TractionNearestFind(nCurrent->pCenter,0,nCurrent);
            // //szukanie najbliższego przęsła
            // trzeba by zliczać końce, a potem wpisać je do tablicy, aby sukcesywnie podłączać do
            // zasilaczy
            traction->iTries = 5; // oznaczanie końcowych
            ++iConnection;
        }
        if (traction->fResistance[0] == 0.0)
        {
            traction->ResistanceCalc(); // obliczanie przęseł w segmencie z bezpośrednim zasilaniem
            // ErrorLog("Section "+nCurrent->hvTraction->asPowerSupplyName+" connected"); //jako
            // niby błąd będzie bardziej widoczne
            traction->iTries = 0; // nie potrzeba mu szukać zasilania
        }
        // if (!Traction->hvParallel) //jeszcze utworzyć pętle z bieżni wspólnych
    }
    int zg = 0; // zgodność kierunku przęseł, tymczasowo iterator do tabeli końców
    // końców jest ok. 10 razy mniej niż wszystkich przęseł (Quark: 216)
    TGroundNode **nEnds = new TGroundNode *[iConnection];
    for( auto *traction : m_items ) {
        //łączenie bieżni wspólnych, w tym oznaczanie niepodanych jawnie
        if (!traction->asParallel.empty()) // będzie wskaźnik na inne przęsło
            if( ( traction->asParallel == "none" )
             || ( traction->asParallel == "*" ) ) {
                // jeśli nieokreślone
                traction->iLast = 2; // jakby przedostatni - niech po prostu szuka (iLast już przeliczone)
            }
            else if (!traction->hvParallel) // jeśli jeszcze nie został włączony w kółko
            {
                auto *nTemp = find( traction->asParallel );
                if (nTemp)
                { // o ile zostanie znalezione przęsło o takiej nazwie
                    if (!nTemp->hvParallel) // jeśli tamten jeszcze nie ma wskaźnika bieżni wspólnej
                        traction->hvParallel = nTemp; // wpisać siebie i dalej dać mu wskaźnik zwrotny
                    else // a jak ma, to albo dołączyć się do kółeczka
                        traction->hvParallel = nTemp->hvParallel; // przjąć dotychczasowy wskaźnik od niego
                    nTemp->hvParallel = traction; // i na koniec ustawienie wskaźnika zwrotnego
                }
                if (!traction->hvParallel)
                    ErrorLog("Missed overhead: " + traction->asParallel); // logowanie braku
            }
        if (traction->iTries > 0) // jeśli zaznaczony do podłączenia
            // if (!nCurrent->hvTraction->psPower[0]||!nCurrent->hvTraction->psPower[1])
            if (zg < iConnection) // zabezpieczenie
                nEnds[zg++] = nCurrent; // wypełnianie tabeli końców w celu szukania im połączeń
    }
    while( zg < iConnection ) {
        // zapełnienie do końca tablicy, jeśli by jakieś końce wypadły
        nEnds[ zg++ ] = nullptr;
    }
    zg = 1; // nieefektywny przebieg kończy łączenie
    while (zg)
    { // ustalenie zastępczej rezystancji dla każdego przęsła
        zg = 0; // flaga podłączonych przęseł końcowych: -1=puste wskaźniki, 0=coś zostało,
        // 1=wykonano łączenie
        for (int i = 0; i < iConnection; ++i)
            if (nEnds[i]) // załatwione będziemy zerować
            { // każdy przebieg to próba podłączenia końca segmentu naprężania do innego zasilanego
                // przęsła
                if (nEnds[i]->hvTraction->hvNext[0])
                { // jeśli końcowy ma ciąg dalszy od strony 0 (Point1), szukamy odcinka najbliższego
                    // do Point2
                    if (TractionNearestFind(nEnds[i]->hvTraction->pPoint2, 0,
                                            nEnds[i])) // poszukiwanie przęsła
                    {
                        nEnds[i] = NULL;
                        zg = 1; // jak coś zostało podłączone, to może zasilanie gdzieś dodatkowo
                        // dotrze
                    }
                }
                else if (nEnds[i]->hvTraction->hvNext[1])
                { // jeśli końcowy ma ciąg dalszy od strony 1 (Point2), szukamy odcinka najbliższego
                    // do Point1
                    if (TractionNearestFind(nEnds[i]->hvTraction->pPoint1, 1,
                                            nEnds[i])) // poszukiwanie przęsła
                    {
                        nEnds[i] = NULL;
                        zg = 1; // jak coś zostało podłączone, to może zasilanie gdzieś dodatkowo
                        // dotrze
                    }
                }
                else
                { // gdy koniec jest samotny, to na razie nie zostanie podłączony (nie powinno
                    // takich być)
                    nEnds[i] = NULL;
                }
            }
    }
    delete[] nEnds; // nie potrzebne już
*/
}
