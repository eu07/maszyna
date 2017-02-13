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
#include "Texture.h"

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

TTraction::TTraction()
{
    hvNext[ 0 ] = nullptr;
    hvNext[ 1 ] = nullptr;
    psPower[ 0 ] = nullptr;
    psPower[ 1 ] = nullptr; // na początku zasilanie nie podłączone
    iNext[ 0 ] = 0;
    iNext[ 1 ] = 0;
    fResistance[ 0 ] = -1.0;
    fResistance[ 1 ] = -1.0; // trzeba dopiero policzyć
}

TTraction::~TTraction()
{
    if (!Global::bUseVBO)
        glDeleteLists(uiDisplayList, 1);
}

void TTraction::Optimize()
{
    if (Global::bUseVBO)
        return;
    uiDisplayList = glGenLists(1);
    glNewList(uiDisplayList, GL_COMPILE);

    TextureManager.Bind(0);
    //    glColor3ub(0,0,0); McZapkie: to do render

    //    glPushMatrix();
    //    glTranslatef(pPosition.x,pPosition.y,pPosition.z);

    if (Wires != 0)
    {
        // Dlugosc odcinka trakcji 'Winger
        double ddp = hypot(pPoint2.x - pPoint1.x, pPoint2.z - pPoint1.z);

        if (Wires == 2)
            WireOffset = 0;
        // Przewoz jezdny 1 'Marcin
        glBegin(GL_LINE_STRIP);
        glVertex3f(pPoint1.x - (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset, pPoint1.y,
                   pPoint1.z - (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset);
        glVertex3f(pPoint2.x - (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset, pPoint2.y,
                   pPoint2.z - (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset);
        glEnd();
        // Nie wiem co 'Marcin
        Math3D::vector3 pt1, pt2, pt3, pt4, v1, v2;
        v1 = pPoint4 - pPoint3;
        v2 = pPoint2 - pPoint1;
        float step = 0;
        if (iNumSections > 0)
            step = 1.0f / (float)iNumSections;
        float f = step;
        float mid = 0.5;
        float t;

        // Przewod nosny 'Marcin
        if (Wires != 1)
        {
            glBegin(GL_LINE_STRIP);
            glVertex3f(pPoint3.x, pPoint3.y, pPoint3.z);
            for (int i = 0; i < iNumSections - 1; i++)
            {
                pt3 = pPoint3 + v1 * f;
                t = (1 - fabs(f - mid) * 2);
                if ((Wires < 4) || ((i != 0) && (i != iNumSections - 2)))
                    glVertex3f(pt3.x, pt3.y - sqrt(t) * fHeightDifference, pt3.z);
                f += step;
            }
            glVertex3f(pPoint4.x, pPoint4.y, pPoint4.z);
            glEnd();
        }

        // Drugi przewod jezdny 'Winger
        if (Wires > 2)
        {
            glBegin(GL_LINE_STRIP);
            glVertex3f(pPoint1.x + (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset, pPoint1.y,
                       pPoint1.z + (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset);
            glVertex3f(pPoint2.x + (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset, pPoint2.y,
                       pPoint2.z + (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset);
            glEnd();
        }

        f = step;

        if (Wires == 4)
        {
            glBegin(GL_LINE_STRIP);
            glVertex3f(pPoint3.x, pPoint3.y - 0.65f * fHeightDifference, pPoint3.z);
            for (int i = 0; i < iNumSections - 1; i++)
            {
                pt3 = pPoint3 + v1 * f;
                t = (1 - fabs(f - mid) * 2);
                glVertex3f(
                    pt3.x,
                    pt3.y - sqrt(t) * fHeightDifference -
                        ((i == 0) || (i == iNumSections - 2) ? 0.25f * fHeightDifference : +0.05),
                    pt3.z);
                f += step;
            }
            glVertex3f(pPoint4.x, pPoint4.y - 0.65f * fHeightDifference, pPoint4.z);
            glEnd();
        }

        f = step;

        // Przewody pionowe (wieszaki) 'Marcin, poprawki na 2 przewody jezdne 'Winger
        if (Wires != 1)
        {
            glBegin(GL_LINES);
            for (int i = 0; i < iNumSections - 1; i++)
            {
                float flo, flo1;
                flo = (Wires == 4 ? 0.25f * fHeightDifference : 0);
                flo1 = (Wires == 4 ? +0.05 : 0);
                pt3 = pPoint3 + v1 * f;
                pt4 = pPoint1 + v2 * f;
                t = (1 - fabs(f - mid) * 2);
                if ((i % 2) == 0)
                {
                    glVertex3f(pt3.x, pt3.y - sqrt(t) * fHeightDifference -
                                          ((i == 0) || (i == iNumSections - 2) ? flo : flo1),
                               pt3.z);
                    glVertex3f(pt4.x - (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset, pt4.y,
                               pt4.z - (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset);
                }
                else
                {
                    glVertex3f(pt3.x, pt3.y - sqrt(t) * fHeightDifference -
                                          ((i == 0) || (i == iNumSections - 2) ? flo : flo1),
                               pt3.z);
                    glVertex3f(pt4.x + (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset, pt4.y,
                               pt4.z + (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset);
                }
                if ((Wires == 4) && ((i == 1) || (i == iNumSections - 3)))
                {
                    glVertex3f(pt3.x, pt3.y - sqrt(t) * fHeightDifference - 0.05, pt3.z);
                    glVertex3f(pt3.x, pt3.y - sqrt(t) * fHeightDifference, pt3.z);
                }
                // endif;
                f += step;
            }
            glEnd();
        }
        glEndList();
    }
}
/*
void TTraction::InitCenter(vector3 Angles, vector3 pOrigin)
{
    pPosition= (pPoint2+pPoint1)*0.5f;
    fSquaredRadius= SquareMagnitude((pPoint2-pPoint1)*0.5f);
} */

void TTraction::RenderDL(float mgn) // McZapkie: mgn to odleglosc od obserwatora
{
    // McZapkie: ustalanie przezroczystosci i koloru linii:
    if (Wires != 0 && !TestFlag(DamageFlag, 128)) // rysuj jesli sa druty i nie zerwana
    {
        // glDisable(GL_LIGHTING); //aby nie używało wektorów normalnych do kolorowania
        glColor4f(0, 0, 0, 1); // jak nieznany kolor to czarne nieprzezroczyste
        if (!Global::bSmoothTraction)
            glDisable(GL_LINE_SMOOTH); // na liniach kiepsko wygląda - robi gradient
        float linealpha = 5000 * WireThickness / (mgn + 1.0); //*WireThickness
        if (linealpha > 1.2)
            linealpha = 1.2; // zbyt grube nie są dobre
        glLineWidth(linealpha);
        if (linealpha > 1.0)
            linealpha = 1.0;
        // McZapkie-261102: kolor zalezy od materialu i zasniedzenia
        float r, g, b;
        switch (Material)
        { // Ra: kolory podzieliłem przez 2, bo po zmianie ambient za jasne były
        // trzeba uwzględnić kierunek świecenia Słońca - tylko ze Słońcem widać kolor
        case 1:
            if (TestFlag(DamageFlag, 1))
            {
                r = 0.00000;
                g = 0.32549;
                b = 0.2882353; // zielona miedź
            }
            else
            {
                r = 0.35098;
                g = 0.22549;
                b = 0.1; // czerwona miedź
            }
            break;
        case 2:
            if (TestFlag(DamageFlag, 1))
            {
                r = 0.10;
                g = 0.10;
                b = 0.10; // czarne Al
            }
            else
            {
                r = 0.25;
                g = 0.25;
                b = 0.25; // srebrne Al
            }
            break;
        // tymczasowo pokazanie zasilanych odcinków
        case 4:
            r = 0.5;
            g = 0.5;
            b = 1.0;
            break; // niebieskie z podłączonym zasilaniem
        case 5:
            r = 1.0;
            g = 0.0;
            b = 0.0;
            break; // czerwone z podłączonym zasilaniem 1
        case 6:
            r = 0.0;
            g = 1.0;
            b = 0.0;
            break; // zielone z podłączonym zasilaniem 2
        case 7:
            r = 1.0;
            g = 1.0;
            b = 0.0;
            break; //żółte z podłączonym zasilaniem z obu stron
        }
        if (DebugModeFlag)
            if (hvParallel)
            { // jeśli z bieżnią wspólną, to dodatkowo przyciemniamy
                r *= 0.6;
                g *= 0.6;
                b *= 0.6;
            }
        r *= Global::ambientDayLight[0]; // w zaleźności od koloru swiatła
        g *= Global::ambientDayLight[1];
        b *= Global::ambientDayLight[2];
        if (linealpha > 1.0)
            linealpha = 1.0; // trzeba ograniczyć do <=1
        glColor4f(r, g, b, linealpha);
        if (!uiDisplayList)
            Optimize(); // generowanie DL w miarę potrzeby
        glCallList(uiDisplayList);
        glLineWidth(1.0);
        glEnable(GL_LINE_SMOOTH);
        // glEnable(GL_LIGHTING); //bez tego się modele nie oświetlają
    }
}

int TTraction::RaArrayPrepare()
{ // przygotowanie tablic do skopiowania do VBO (zliczanie wierzchołków)
    // if (bVisible) //o ile w ogóle widać
    switch (Wires)
    {
    case 1:
        iLines = 2;
        break;
    case 2:
        iLines = iNumSections ? 4 * (iNumSections)-2 + 2 : 4;
        break;
    case 3:
        iLines = iNumSections ? 4 * (iNumSections)-2 + 4 : 6;
        break;
    case 4:
        iLines = iNumSections ? 4 * (iNumSections)-2 + 6 : 8;
        break;
    default:
        iLines = 0;
    }
    // else iLines=0;
    return iLines;
};

void TTraction::RaArrayFill(CVertNormTex *Vert)
{ // wypełnianie tablic VBO
    CVertNormTex *old = Vert;
    double ddp = hypot(pPoint2.x - pPoint1.x, pPoint2.z - pPoint1.z);
    if (Wires == 2)
        WireOffset = 0;
    // jezdny
    Vert->x = pPoint1.x - (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset;
    Vert->y = pPoint1.y;
    Vert->z = pPoint1.z - (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset;
    ++Vert;
    Vert->x = pPoint2.x - (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset;
    Vert->y = pPoint2.y;
    Vert->z = pPoint2.z - (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset;
    ++Vert;
    // Nie wiem co 'Marcin
    Math3D::vector3 pt1, pt2, pt3, pt4, v1, v2;
    v1 = pPoint4 - pPoint3;
    v2 = pPoint2 - pPoint1;
    float step = 0;
    if (iNumSections > 0)
        step = 1.0f / (float)iNumSections;
    float f = step;
    float mid = 0.5;
    float t;
    // Przewod nosny 'Marcin
    if (Wires > 1)
    { // lina nośna w kawałkach
        Vert->x = pPoint3.x;
        Vert->y = pPoint3.y;
        Vert->z = pPoint3.z;
        ++Vert;
        for (int i = 0; i < iNumSections - 1; i++)
        {
            pt3 = pPoint3 + v1 * f;
            t = (1 - fabs(f - mid) * 2);
            Vert->x = pt3.x;
            Vert->y = pt3.y - sqrt(t) * fHeightDifference;
            Vert->z = pt3.z;
            ++Vert;
            Vert->x = pt3.x; // drugi raz, bo nie jest line_strip
            Vert->y = pt3.y - sqrt(t) * fHeightDifference;
            Vert->z = pt3.z;
            ++Vert;
            f += step;
        }
        Vert->x = pPoint4.x;
        Vert->y = pPoint4.y;
        Vert->z = pPoint4.z;
        ++Vert;
    }
    // Drugi przewod jezdny 'Winger
    if (Wires == 3)
    {
        Vert->x = pPoint1.x + (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset;
        Vert->y = pPoint1.y;
        Vert->z = pPoint1.z + (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset;
        ++Vert;
        Vert->x = pPoint2.x + (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset;
        Vert->y = pPoint2.y;
        Vert->z = pPoint2.z + (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset;
        ++Vert;
    }
    f = step;
    // Przewody pionowe (wieszaki) 'Marcin, poprawki na 2 przewody jezdne 'Winger
    if (Wires > 1)
    {
        for (int i = 0; i < iNumSections - 1; i++)
        {
            pt3 = pPoint3 + v1 * f;
            pt4 = pPoint1 + v2 * f;
            t = (1 - fabs(f - mid) * 2);
            Vert->x = pt3.x;
            Vert->y = pt3.y - sqrt(t) * fHeightDifference;
            Vert->z = pt3.z;
            ++Vert;
            if ((i % 2) == 0)
            {
                Vert->x = pt4.x - (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset;
                Vert->y = pt4.y;
                Vert->z = pt4.z - (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset;
            }
            else
            {
                Vert->x = pt4.x + (pPoint2.z / ddp - pPoint1.z / ddp) * WireOffset;
                Vert->y = pt4.y;
                Vert->z = pt4.z + (-pPoint2.x / ddp + pPoint1.x / ddp) * WireOffset;
            }
            ++Vert;
            f += step;
        }
    }
    if ((Vert - old) != iLines)
        WriteLog("!!! Wygenerowano punktów " + std::to_string(Vert - old) + ", powinno być " +
                 std::to_string(iLines));
};

void TTraction::RenderVBO(float mgn, int iPtr)
{ // renderowanie z użyciem VBO
    if (Wires != 0 && !TestFlag(DamageFlag, 128)) // rysuj jesli sa druty i nie zerwana
    {
        TextureManager.Bind(0);
        glDisable(GL_LIGHTING); // aby nie używało wektorów normalnych do kolorowania
        glColor4f(0, 0, 0, 1); // jak nieznany kolor to czarne nieprzezroczyste
        if (!Global::bSmoothTraction)
            glDisable(GL_LINE_SMOOTH); // na liniach kiepsko wygląda - robi gradient
        float linealpha = 5000 * WireThickness / (mgn + 1.0); //*WireThickness
        if (linealpha > 1.2)
            linealpha = 1.2; // zbyt grube nie są dobre
        glLineWidth(linealpha);
        // McZapkie-261102: kolor zalezy od materialu i zasniedzenia
        float r, g, b;
        switch (Material)
        { // Ra: kolory podzieliłem przez 2, bo po zmianie ambient za jasne były
        // trzeba uwzględnić kierunek świecenia Słońca - tylko ze Słońcem widać kolor
        case 1:
            if (TestFlag(DamageFlag, 1))
            {
                r = 0.00000;
                g = 0.32549;
                b = 0.2882353; // zielona miedź
            }
            else
            {
                r = 0.35098;
                g = 0.22549;
                b = 0.1; // czerwona miedź
            }
            break;
        case 2:
            if (TestFlag(DamageFlag, 1))
            {
                r = 0.10;
                g = 0.10;
                b = 0.10; // czarne Al
            }
            else
            {
                r = 0.25;
                g = 0.25;
                b = 0.25; // srebrne Al
            }
            break;
        // tymczasowo pokazanie zasilanych odcinków
        case 4:
            r = 0.5;
            g = 0.5;
            b = 1.0;
            break; // niebieskie z podłączonym zasilaniem
        case 5:
            r = 1.0;
            g = 0.0;
            b = 0.0;
            break; // czerwone z podłączonym zasilaniem 1
        case 6:
            r = 0.0;
            g = 1.0;
            b = 0.0;
            break; // zielone z podłączonym zasilaniem 2
        case 7:
            r = 1.0;
            g = 1.0;
            b = 0.0;
            break; //żółte z podłączonym zasilaniem z obu stron
        }
        r = r * Global::ambientDayLight[0]; // w zaleznosci od koloru swiatla
        g = g * Global::ambientDayLight[1];
        b = b * Global::ambientDayLight[2];
        if (linealpha > 1.0)
            linealpha = 1.0; // trzeba ograniczyć do <=1
        glColor4f(r, g, b, linealpha);
        glDrawArrays(GL_LINES, iPtr, iLines);
        glLineWidth(1.0);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_LIGHTING); // bez tego się modele nie oświetlają
    }
};

int TTraction::TestPoint(Math3D::vector3 *Point)
{ // sprawdzanie, czy przęsła można połączyć
    if (!hvNext[0])
        if (pPoint1.Equal(Point))
            return 0;
    if (!hvNext[1])
        if (pPoint2.Equal(Point))
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
        // double r; //sumaryczna rezystancja
        if (DebugModeFlag) // tylko podczas testów
            Material = 4; // pokazanie, że to przęsło ma podłączone zasilanie
        while (t ? !t->psPower[d] : false) // jeśli jest jakiś kolejny i nie ma ustalonego zasilacza
        { // ustawienie zasilacza i policzenie rezystancji zastępczej
            if (DebugModeFlag) // tylko podczas testów
                if (t->Material != 4) // przęsła zasilającego nie modyfikować
                {
                    if (t->Material < 4)
                        t->Material = 4; // tymczasowo, aby zmieniła kolor
                    t->Material |= d ? 2 : 1; // kolor zależny od strony, z której jest zasilanie
                }
            t->psPower[d] = ps; // skopiowanie wskaźnika zasilacza od danej strony
            t->fResistance[d] = r; // wpisanie rezystancji w kierunku tego zasilacza
            r += t->fResistivity * Length3(t->vParametric); // doliczenie oporu kolejnego odcinka
            p = t; // zapamiętanie dotychczasowego
            t = p->hvNext[d ^ 1]; // podążanie w tę samą stronę
            d = p->iNext[d ^ 1];
            // w przypadku zapętlenia sieci może się zawiesić?
        }
    }
    else
    { // podążanie w obu kierunkach, można by rekurencją, ale szkoda zasobów
        r = 0.5 * fResistivity *
            Length3(vParametric); // powiedzmy, że w zasilanym przęśle jest połowa
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
    double u0, u1, i0, i1;
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
