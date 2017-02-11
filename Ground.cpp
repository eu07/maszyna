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
#include "Ground.h"

#include "opengl/glew.h"

#include "Globals.h"
#include "Logs.h"
#include "usefull.h"
#include "Timer.h"
#include "Texture.h"
#include "Event.h"
#include "EvLaunch.h"
#include "TractionPower.h"
#include "Traction.h"
#include "Track.h"
#include "RealSound.h"
#include "AnimModel.h"
#include "MemCell.h"
#include "mtable.h"
#include "DynObj.h"
#include "Data.h"
#include "parser.h" //Tolaris-010603
#include "Driver.h"
#include "Console.h"
#include "Names.h"

#define _PROBLEND 1
//---------------------------------------------------------------------------

bool bCondition; // McZapkie: do testowania warunku na event multiple
string LogComment;

//---------------------------------------------------------------------------
// Obiekt renderujący siatkę jest sztucznie tworzonym obiektem pomocniczym,
// grupującym siatki obiektów dla danej tekstury. Obiektami składowymi mogą
// byc trójkąty terenu, szyny, podsypki, a także proste modele np. słupy.
// Obiekty składowe dodane są do listy TSubRect::nMeshed z listą zrobioną na
// TGroundNode::nNext3, gdzie są posortowane wg tekstury. Obiekty renderujące
// są wpisane na listę TSubRect::nRootMesh (TGroundNode::nNext2) oraz na
// odpowiednie listy renderowania, gdzie zastępują obiekty składowe (nNext3).
// Problematyczne są tory/drogi/rzeki, gdzie używane sa 2 tekstury. Dlatego
// tory są zdublowane jako TP_TRACK oraz TP_DUMMYTRACK. Jeśli tekstura jest
// tylko jedna (np. zwrotnice), nie jest używany TP_DUMMYTRACK.
//---------------------------------------------------------------------------
TGroundNode::TGroundNode()
{ // nowy obiekt terenu - pusty
    iType = GL_POINTS;
    Vertices = NULL;
    nNext = nNext2 = NULL;
    pCenter = vector3(0, 0, 0);
    iCount = 0; // wierzchołków w trójkącie
    // iNumPts=0; //punktów w linii
    TextureID = 0;
    iFlags = 0; // tryb przezroczystości nie zbadany
    DisplayListID = 0;
    Pointer = NULL; // zerowanie wskaźnika kontekstowego
    bVisible = false; // czy widoczny
    fSquareRadius = 10000 * 10000;
    fSquareMinRadius = 0;
    asName = "";
    // Color= TMaterialColor(1);
    // fAngle=0; //obrót dla modelu
    fLineThickness=1.0; //mm dla linii
    for (int i = 0; i < 3; i++)
    {
        Ambient[i] = Global::whiteLight[i] * 255;
        Diffuse[i] = Global::whiteLight[i] * 255;
        Specular[i] = Global::noLight[i] * 255;
    }
    nNext3 = NULL; // nie wyświetla innych
    iVboPtr = -1; // indeks w VBO sektora (-1: nie używa VBO)
    iVersion = 0; // wersja siatki
}

TGroundNode::~TGroundNode()
{
    // if (iFlags&0x200) //czy obiekt został utworzony?
    switch (iType)
    {
    case TP_MEMCELL:
        SafeDelete(MemCell);
        break;
    case TP_EVLAUNCH:
        SafeDelete(EvLaunch);
        break;
    case TP_TRACTION:
        SafeDelete(hvTraction);
        break;
    case TP_TRACTIONPOWERSOURCE:
        SafeDelete(psTractionPowerSource);
        break;
    case TP_TRACK:
        SafeDelete(pTrack);
        break;
    case TP_DYNAMIC:
        SafeDelete(DynamicObject);
        break;
    case TP_MODEL:
        if (iFlags & 0x200) // czy model został utworzony?
            delete Model;
        Model = NULL;
        break;
	case TP_SOUND:
		SafeDelete(tsStaticSound);
		break;
    case TP_TERRAIN:
    { // pierwsze nNode zawiera model E3D, reszta to trójkąty
        for (int i = 1; i < iCount; ++i)
            nNode->Vertices =
                NULL; // zerowanie wskaźników w kolejnych elementach, bo nie są do usuwania
        delete[] nNode; // usunięcie tablicy i pierwszego elementu
    }
    case TP_SUBMODEL: // dla formalności, nie wymaga usuwania
        break;
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        SafeDeleteArray(Points);
        break;
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLES:
        SafeDeleteArray(Vertices);
        break;
    }
}

void TGroundNode::Init(int n)
{ // utworzenie tablicy wierzchołków
    bVisible = false;
    iNumVerts = n;
    Vertices = new TGroundVertex[iNumVerts];
}

TGroundNode::TGroundNode(TGroundNodeType t, int n)
{ // utworzenie obiektu
    TGroundNode(); // domyślne ustawienia
    iNumVerts = n;
    if (iNumVerts)
        Vertices = new TGroundVertex[iNumVerts];
    iType = t;
    switch (iType)
    { // zależnie od typu
    case TP_TRACK:
        pTrack = new TTrack(this);
        break;
    }
}

void TGroundNode::InitCenter()
{ // obliczenie środka ciężkości obiektu
    for (int i = 0; i < iNumVerts; i++)
        pCenter += Vertices[i].Point;
    pCenter /= iNumVerts;
}

void TGroundNode::InitNormals()
{ // obliczenie wektorów normalnych
    vector3 v1, v2, v3, v4, v5, n1, n2, n3, n4;
    int i;
    float tu, tv;
    switch (iType)
    {
    case GL_TRIANGLE_STRIP:
        v1 = Vertices[0].Point - Vertices[1].Point;
        v2 = Vertices[1].Point - Vertices[2].Point;
        n1 = SafeNormalize(CrossProduct(v1, v2));
        if (Vertices[0].Normal == vector3(0, 0, 0))
            Vertices[0].Normal = n1;
        v3 = Vertices[2].Point - Vertices[3].Point;
        n2 = SafeNormalize(CrossProduct(v3, v2));
        if (Vertices[1].Normal == vector3(0, 0, 0))
            Vertices[1].Normal = (n1 + n2) * 0.5;

        for (i = 2; i < iNumVerts - 2; i += 2)
        {
            v4 = Vertices[i - 1].Point - Vertices[i].Point;
            v5 = Vertices[i].Point - Vertices[i + 1].Point;
            n3 = SafeNormalize(CrossProduct(v3, v4));
            n4 = SafeNormalize(CrossProduct(v5, v4));
            if (Vertices[i].Normal == vector3(0, 0, 0))
                Vertices[i].Normal = (n1 + n2 + n3) / 3;
            if (Vertices[i + 1].Normal == vector3(0, 0, 0))
                Vertices[i + 1].Normal = (n2 + n3 + n4) / 3;
            n1 = n3;
            n2 = n4;
            v3 = v5;
        }
        if (Vertices[i].Normal == vector3(0, 0, 0))
            Vertices[i].Normal = (n1 + n2) / 2;
        if (Vertices[i + 1].Normal == vector3(0, 0, 0))
            Vertices[i + 1].Normal = n2;
        break;
    case GL_TRIANGLE_FAN:

        break;
    case GL_TRIANGLES:
        for (i = 0; i < iNumVerts; i += 3)
        {
            v1 = Vertices[i + 0].Point - Vertices[i + 1].Point;
            v2 = Vertices[i + 1].Point - Vertices[i + 2].Point;
            n1 = SafeNormalize(CrossProduct(v1, v2));
            if (Vertices[i + 0].Normal == vector3(0, 0, 0))
                Vertices[i + 0].Normal = (n1);
            if (Vertices[i + 1].Normal == vector3(0, 0, 0))
                Vertices[i + 1].Normal = (n1);
            if (Vertices[i + 2].Normal == vector3(0, 0, 0))
                Vertices[i + 2].Normal = (n1);
            tu = floor(Vertices[i + 0].tu);
            tv = floor(Vertices[i + 0].tv);
            Vertices[i + 1].tv -= tv;
            Vertices[i + 2].tv -= tv;
            Vertices[i + 0].tv -= tv;
            Vertices[i + 1].tu -= tu;
            Vertices[i + 2].tu -= tu;
            Vertices[i + 0].tu -= tu;
        }
        break;
    }
}

void TGroundNode::MoveMe(vector3 pPosition)
{ // przesuwanie obiektów scenerii o wektor w celu redukcji trzęsienia
    pCenter += pPosition;
    switch (iType)
    {
    case TP_TRACTION:
        hvTraction->pPoint1 += pPosition;
        hvTraction->pPoint2 += pPosition;
        hvTraction->pPoint3 += pPosition;
        hvTraction->pPoint4 += pPosition;
        hvTraction->Optimize();
        break;
    case TP_MODEL:
    case TP_DYNAMIC:
    case TP_MEMCELL:
    case TP_EVLAUNCH:
        break;
    case TP_TRACK:
        pTrack->MoveMe(pPosition);
        break;
    case TP_SOUND: // McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
        tsStaticSound->vSoundPosition += pPosition;
        break;
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        for (int i = 0; i < iNumPts; i++)
            Points[i] += pPosition;
        ResourceManager::Unregister(this);
        break;
    default:
        for (int i = 0; i < iNumVerts; i++)
            Vertices[i].Point += pPosition;
        ResourceManager::Unregister(this);
    }
}

void TGroundNode::RaRenderVBO()
{ // renderowanie z domyslnego bufora VBO
    glColor3ub(Diffuse[0], Diffuse[1], Diffuse[2]);
    if (TextureID)
        glBindTexture(GL_TEXTURE_2D, TextureID); // Ustaw aktywną teksturę
    glDrawArrays(iType, iVboPtr, iNumVerts); // Narysuj naraz wszystkie trójkąty
}

void TGroundNode::RenderVBO()
{ // renderowanie obiektu z VBO - faza nieprzezroczystych
    double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
    if ((mgn > fSquareRadius || (mgn < fSquareMinRadius)) &&
        (iType != TP_EVLAUNCH)) // McZapkie-070602: nie rysuj odleglych obiektow ale sprawdzaj
        // wyzwalacz zdarzen
        return;
    int i, a;
    switch (iType)
    {
    case TP_TRACTION:
        return;
    case TP_TRACK:
        if (iNumVerts)
            pTrack->RaRenderVBO(iVboPtr);
        return;
    case TP_MODEL:
        Model->RenderVBO(&pCenter);
        return;
    // case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
    // if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING)==DSBPLAY_LOOPING)
    // {
    //  pStaticSound->Play(1,DSBPLAY_LOOPING,true,pStaticSound->vSoundPosition);
    //  pStaticSound->AdjFreq(1.0,Timer::GetDeltaTime());
    // }
    // return; //Ra: TODO sprawdzić, czy dźwięki nie są tylko w RenderHidden
    case TP_MEMCELL:
        return;
    case TP_EVLAUNCH:
        if (EvLaunch->Render())
            if ((EvLaunch->dRadius < 0) || (mgn < EvLaunch->dRadius))
            {
                if (Global::shiftState && EvLaunch->Event2 != NULL)
                    Global::AddToQuery(EvLaunch->Event2, NULL);
                else if (EvLaunch->Event1 != NULL)
                    Global::AddToQuery(EvLaunch->Event1, NULL);
            }
        return;
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        if (iNumPts)
        {
            float linealpha = 255000 * fLineThickness / (mgn + 1.0);
            if (linealpha > 255)
                linealpha = 255;
            float r, g, b;
            r = floor(Diffuse[0] * Global::ambientDayLight[0]); // w zaleznosci od koloru swiatla
            g = floor(Diffuse[1] * Global::ambientDayLight[1]);
            b = floor(Diffuse[2] * Global::ambientDayLight[2]);
            glColor4ub(r, g, b, linealpha); // przezroczystosc dalekiej linii
            // glDisable(GL_LIGHTING); //nie powinny świecić
            glDrawArrays(iType, iVboPtr, iNumPts); // rysowanie linii
            // glEnable(GL_LIGHTING);
        }
        return;
    default:
        if (iVboPtr >= 0)
            RaRenderVBO();
    };
    return;
};

void TGroundNode::RenderAlphaVBO()
{ // renderowanie obiektu z VBO - faza przezroczystych
    double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
    float r, g, b;
    if (mgn < fSquareMinRadius)
        return;
    if (mgn > fSquareRadius)
        return;
    int i, a;
#ifdef _PROBLEND
    if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
    {
        glDisable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.45); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    };
#endif
    switch (iType)
    {
    case TP_TRACTION:
        if (bVisible)
        {
#ifdef _PROBLEND
            glEnable(GL_BLEND);
            glAlphaFunc(GL_GREATER, 0.04);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
            hvTraction->RenderVBO(mgn, iVboPtr);
        }
        return;
    case TP_MODEL:
#ifdef _PROBLEND
        glEnable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.04);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
        Model->RenderAlphaVBO(&pCenter);
        return;
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        if (iNumPts)
        {
            float linealpha = 255000 * fLineThickness / (mgn + 1.0);
            if (linealpha > 255)
                linealpha = 255;
            r = Diffuse[0] * Global::ambientDayLight[0]; // w zaleznosci od koloru swiatla
            g = Diffuse[1] * Global::ambientDayLight[1];
            b = Diffuse[2] * Global::ambientDayLight[2];
            glColor4ub(r, g, b, linealpha); // przezroczystosc dalekiej linii
            // glDisable(GL_LIGHTING); //nie powinny świecić
            glDrawArrays(iType, iVboPtr, iNumPts); // rysowanie linii
// glEnable(GL_LIGHTING);
#ifdef _PROBLEND
            glEnable(GL_BLEND);
            glAlphaFunc(GL_GREATER, 0.04);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
        }
#ifdef _PROBLEND
        glEnable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.04);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
        return;
    default:
        if (iVboPtr >= 0)
        {
            RaRenderVBO();
#ifdef _PROBLEND
            glEnable(GL_BLEND);
            glAlphaFunc(GL_GREATER, 0.04);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
            return;
        }
    }
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    return;
}

void TGroundNode::Compile(bool many)
{ // tworzenie skompilowanej listy w wyświetlaniu DL
    if (!many)
    { // obsługa pojedynczej listy
        if (DisplayListID)
            Release();
        if (Global::bManageNodes)
        {
            DisplayListID = glGenLists(1);
            glNewList(DisplayListID, GL_COMPILE);
            iVersion = Global::iReCompile; // aktualna wersja siatek (do WireFrame)
        }
    }
    if ((iType == GL_LINES) || (iType == GL_LINE_STRIP) || (iType == GL_LINE_LOOP))
    {
#ifdef USE_VERTEX_ARRAYS
        glVertexPointer(3, GL_DOUBLE, sizeof(vector3), &Points[0].x);
#endif
        glBindTexture(GL_TEXTURE_2D, 0);
#ifdef USE_VERTEX_ARRAYS
        glDrawArrays(iType, 0, iNumPts);
#else
        glBegin(iType);
        for (int i = 0; i < iNumPts; i++)
            glVertex3dv(&Points[i].x);
        glEnd();
#endif
    }
    else if (iType == GL_TRIANGLE_STRIP || iType == GL_TRIANGLE_FAN || iType == GL_TRIANGLES)
    { // jak nie linie, to trójkąty
        TGroundNode *tri = this;
        do
        { // pętla po obiektach w grupie w celu połączenia siatek
#ifdef USE_VERTEX_ARRAYS
            glVertexPointer(3, GL_DOUBLE, sizeof(TGroundVertex), &tri->Vertices[0].Point.x);
            glNormalPointer(GL_DOUBLE, sizeof(TGroundVertex), &tri->Vertices[0].Normal.x);
            glTexCoordPointer(2, GL_FLOAT, sizeof(TGroundVertex), &tri->Vertices[0].tu);
#endif
            glColor3ub(tri->Diffuse[0], tri->Diffuse[1], tri->Diffuse[2]);
            glBindTexture(GL_TEXTURE_2D, Global::bWireFrame ? 0 : tri->TextureID);
#ifdef USE_VERTEX_ARRAYS
            glDrawArrays(Global::bWireFrame ? GL_LINE_LOOP : tri->iType, 0, tri->iNumVerts);
#else
            glBegin(Global::bWireFrame ? GL_LINE_LOOP : tri->iType);
            for (int i = 0; i < tri->iNumVerts; i++)
            {
                glNormal3d(tri->Vertices[i].Normal.x, tri->Vertices[i].Normal.y,
                           tri->Vertices[i].Normal.z);
                glTexCoord2f(tri->Vertices[i].tu, tri->Vertices[i].tv);
                glVertex3dv(&tri->Vertices[i].Point.x);
            };
            glEnd();
#endif
            /*
               if (tri->pTriGroup) //jeśli z grupy
               {tri=tri->pNext2; //następny w sektorze
                while (tri?!tri->pTriGroup:false) tri=tri->pNext2; //szukamy kolejnego należącego do
               grupy
               }
               else
            */
            tri = NULL; // a jak nie, to koniec
        } while (tri);
    }
    else if (iType == TP_MESH)
    { // grupa ze wspólną teksturą - wrzucanie do wspólnego Display List
        if (TextureID)
            glBindTexture(GL_TEXTURE_2D, TextureID); // Ustaw aktywną teksturę
        TGroundNode *n = nNode;
        while (n ? n->TextureID == TextureID : false)
        { // wszystkie obiekty o tej samej testurze
            switch (n->iType)
            { // poszczególne typy różnie się tworzy
            case TP_TRACK:
            case TP_DUMMYTRACK:
                n->pTrack->Compile(TextureID); // dodanie trójkątów dla podanej tekstury
                break;
            }
            n = n->nNext3; // następny z listy
        }
    }
    if (!many)
        if (Global::bManageNodes)
            glEndList();
};

void TGroundNode::Release()
{
    if (DisplayListID)
        glDeleteLists(DisplayListID, 1);
    DisplayListID = 0;
};

void TGroundNode::RenderHidden()
{ // renderowanie obiektów niewidocznych
    double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
    switch (iType)
    {
    case TP_SOUND: // McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
        if ((tsStaticSound->GetStatus() & DSBSTATUS_PLAYING) == DSBPLAY_LOOPING)
        {
            tsStaticSound->Play(1, DSBPLAY_LOOPING, true, tsStaticSound->vSoundPosition);
            tsStaticSound->AdjFreq(1.0, Timer::GetDeltaTime());
        }
        return;
    case TP_EVLAUNCH:
        if (EvLaunch->Render())
            if ((EvLaunch->dRadius < 0) || (mgn < EvLaunch->dRadius))
            {
                WriteLog("Eventlauncher " + asName);
                if (Global::shiftState && (EvLaunch->Event2))
                    Global::AddToQuery(EvLaunch->Event2, NULL);
                else if (EvLaunch->Event1)
                    Global::AddToQuery(EvLaunch->Event1, NULL);
            }
        return;
    }
};

void TGroundNode::RenderDL()
{ // wyświetlanie obiektu przez Display List
    switch (iType)
    { // obiekty renderowane niezależnie od odległości
    case TP_SUBMODEL:
        TSubModel::fSquareDist = 0;
        return smTerrain->RenderDL();
    }
    // if (pTriGroup) if (pTriGroup!=this) return; //wyświetla go inny obiekt
    double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
    if ((mgn > fSquareRadius) || (mgn < fSquareMinRadius)) // McZapkie-070602: nie rysuj odleglych
        // obiektow ale sprawdzaj wyzwalacz
        // zdarzen
        return;
    int i, a;
    switch (iType)
    {
    case TP_TRACK:
        return pTrack->Render();
    case TP_MODEL:
        return Model->RenderDL(&pCenter);
    }
    // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
    // if ((iNumVerts&&(iFlags&0x10))||(iNumPts&&(fLineThickness<0)))
    if ((iFlags & 0x10) || (fLineThickness < 0))
    {
        if (!DisplayListID || (iVersion != Global::iReCompile)) // Ra: wymuszenie rekompilacji
        {
            Compile();
            if (Global::bManageNodes)
                ResourceManager::Register(this);
        };

        if ((iType == GL_LINES) || (iType == GL_LINE_STRIP) || (iType == GL_LINE_LOOP))
        // if (iNumPts)
        { // wszelkie linie są rysowane na samym końcu
            float r, g, b;
            r = Diffuse[0] * Global::ambientDayLight[0]; // w zaleznosci od koloru swiatla
            g = Diffuse[1] * Global::ambientDayLight[1];
            b = Diffuse[2] * Global::ambientDayLight[2];
            glColor4ub(r, g, b, 1.0);
            glCallList(DisplayListID);
            // glColor4fv(Diffuse); //przywrócenie koloru
            // glColor3ub(Diffuse[0],Diffuse[1],Diffuse[2]);
        }
        // GL_TRIANGLE etc
        else
            glCallList(DisplayListID);
        SetLastUsage(Timer::GetSimulationTime());
    };
};

void TGroundNode::RenderAlphaDL()
{
    // SPOSOB NA POZBYCIE SIE RAMKI DOOKOLA TEXTURY ALPHA DLA OBIEKTOW ZAGNIEZDZONYCH W SCN JAKO
    // NODE

    // W GROUND.H dajemy do klasy TGroundNode zmienna bool PROBLEND to samo robimy w klasie TGround
    // nastepnie podczas wczytywania textury dla TRIANGLES w TGround::AddGroundNode
    // sprawdzamy czy w nazwie jest @ i wg tego
    // ustawiamy PROBLEND na true dla wlasnie wczytywanego trojkata (kazdy trojkat jest osobnym
    // nodem)
    // nastepnie podczas renderowania w bool TGroundNode::RenderAlpha()
    // na poczatku ustawiamy standardowe GL_GREATER = 0.04
    // pozniej sprawdzamy czy jest wlaczony PROBLEND dla aktualnie renderowanego noda TRIANGLE,
    // wlasciwie dla kazdego node'a
    // i jezeli tak to odpowiedni GL_GREATER w przeciwnym wypadku standardowy 0.04

    // if (pTriGroup) if (pTriGroup!=this) return; //wyświetla go inny obiekt
    double mgn = SquareMagnitude(pCenter - Global::pCameraPosition);
    float r, g, b;
    if (mgn < fSquareMinRadius)
        return;
    if (mgn > fSquareRadius)
        return;
    int i, a;
    switch (iType)
    {
    case TP_TRACTION:
        if (bVisible)
            hvTraction->RenderDL(mgn);
        return;
    case TP_MODEL:
        Model->RenderAlphaDL(&pCenter);
        return;
    case TP_TRACK:
        // pTrack->RenderAlpha();
        return;
    };

    // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
    if ((iNumVerts && (iFlags & 0x20)) || (iNumPts && (fLineThickness > 0)))
    {
#ifdef _PROBLEND
        if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
        {
            glDisable(GL_BLEND);
            glAlphaFunc(GL_GREATER, 0.45); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
        };
#endif
        if (!DisplayListID) //||Global::bReCompile) //Ra: wymuszenie rekompilacji
        {
            Compile();
            if (Global::bManageNodes)
                ResourceManager::Register(this);
        };

        // GL_LINE, GL_LINE_STRIP, GL_LINE_LOOP
        if (iNumPts)
        {
            float linealpha = 255000 * fLineThickness / (mgn + 1.0);
            if (linealpha > 255)
                linealpha = 255;
            r = Diffuse[0] * Global::ambientDayLight[0]; // w zaleznosci od koloru swiatla
            g = Diffuse[1] * Global::ambientDayLight[1];
            b = Diffuse[2] * Global::ambientDayLight[2];
            glColor4ub(r, g, b, linealpha); // przezroczystosc dalekiej linii
            glCallList(DisplayListID);
        }
        // GL_TRIANGLE etc
        else
            glCallList(DisplayListID);
        SetLastUsage(Timer::GetSimulationTime());
    };
#ifdef _PROBLEND
    if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
    {
        glEnable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.04);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    };
#endif
}

//------------------------------------------------------------------------------
//------------------ Podstawowy pojemnik terenu - sektor -----------------------
//------------------------------------------------------------------------------
TSubRect::~TSubRect()
{
    if (Global::bManageNodes) // Ra: tu się coś sypie
        ResourceManager::Unregister(this); // wyrejestrowanie ze sprzątacza
    // TODO: usunąć obiekty z listy (nRootMesh), bo są one tworzone dla sektora
	delete[] tTracks;
}

void TSubRect::NodeAdd(TGroundNode *Node)
{ // przyczepienie obiektu do sektora, wstępna kwalifikacja na listy renderowania
    if (!this)
        return; // zabezpiecznie przed obiektami przekraczającymi obszar roboczy
    // Ra: sortowanie obiektów na listy renderowania:
    // nRenderHidden    - lista obiektów niewidocznych, "renderowanych" również z tyłu
    // nRenderRect      - lista grup renderowanych z sektora
    // nRenderRectAlpha - lista grup renderowanych z sektora z przezroczystością
    // nRender          - lista grup renderowanych z własnych VBO albo DL
    // nRenderAlpha     - lista grup renderowanych z własnych VBO albo DL z przezroczystością
    // nRenderWires     - lista grup renderowanych z własnych VBO albo DL - druty i linie
    // nMeshed          - obiekty do pogrupowania wg tekstur
    GLuint t; // pomocniczy kod tekstury
    switch (Node->iType)
    {
    case TP_SOUND: // te obiekty są sprawdzanie niezależnie od kierunku patrzenia
    case TP_EVLAUNCH:
        Node->nNext3 = nRenderHidden;
        nRenderHidden = Node; // do listy koniecznych
        break;
    case TP_TRACK: // TODO: tory z cieniem (tunel, canyon) też dać bez łączenia?
        ++iTracks; // jeden tor więcej
        Node->pTrack->RaOwnerSet(this); // do którego sektora ma zgłaszać animację
        // if (Global::bUseVBO?false:!Node->pTrack->IsGroupable())
        if (Global::bUseVBO ? true :
                              !Node->pTrack->IsGroupable()) // TODO: tymczasowo dla VBO wyłączone
            RaNodeAdd(
                Node); // tory ruchome nie są grupowane przy Display Lists (wymagają odświeżania DL)
        else
        { // tory nieruchome mogą być pogrupowane wg tekstury, przy VBO wszystkie
            Node->TextureID = Node->pTrack->TextureGet(0); // pobranie tekstury do sortowania
            t = Node->pTrack->TextureGet(1);
            if (Node->TextureID) // jeżeli jest pierwsza
            {
                if (t && (Node->TextureID != t))
                { // jeśli są dwie różne tekstury, dodajemy drugi obiekt dla danego toru
                    TGroundNode *n = new TGroundNode(); // BUG: source of a memory leak here
                    n->iType = TP_DUMMYTRACK; // obiekt renderujący siatki dla tekstury
                    n->TextureID = t;
                    n->pTrack = Node->pTrack; // wskazuje na ten sam tor
                    n->pCenter = Node->pCenter;
                    n->fSquareRadius = Node->fSquareRadius;
                    n->fSquareMinRadius = Node->fSquareMinRadius;
                    n->iFlags = Node->iFlags;
                    n->nNext2 = nRootMesh;
                    nRootMesh = n; // podczepienie do listy, żeby usunąć na końcu
                    n->nNext3 = nMeshed;
                    nMeshed = n;
                }
            }
            else
                Node->TextureID = t; // jest tylko druga tekstura
            if (Node->TextureID)
            {
                Node->nNext3 = nMeshed;
                nMeshed = Node;
            } // do podzielenia potem
        }
        break;
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLES:
        // Node->nNext3=nMeshed; nMeshed=Node; //do podzielenia potem
        if (Node->iFlags & 0x20) // czy jest przezroczyste?
        {
            Node->nNext3 = nRenderRectAlpha;
            nRenderRectAlpha = Node;
        } // DL: do przezroczystych z sektora
        else if (Global::bUseVBO)
        {
            Node->nNext3 = nRenderRect;
            nRenderRect = Node;
        } // VBO: do nieprzezroczystych z sektora
        else
        {
            Node->nNext3 = nRender;
            nRender = Node;
        } // DL: do nieprzezroczystych wszelakich
        /*
           //Ra: na razie wyłączone do testów VBO
           //if
           ((Node->iType==GL_TRIANGLE_STRIP)||(Node->iType==GL_TRIANGLE_FAN)||(Node->iType==GL_TRIANGLES))
            if (Node->fSquareMinRadius==0.0) //znikające z bliska nie mogą być optymalizowane
             if (Node->fSquareRadius>=160000.0) //tak od 400m to już normalne trójkąty muszą być
             //if (Node->iFlags&0x10) //i nieprzezroczysty
             {if (pTriGroup) //jeżeli był już jakiś grupujący
              {if (pTriGroup->fSquareRadius>Node->fSquareRadius) //i miał większy zasięg
                Node->fSquareRadius=pTriGroup->fSquareRadius; //zwiększenie zakresu widoczności
           grupującego
               pTriGroup->pTriGroup=Node; //poprzedniemu doczepiamy nowy
              }
              Node->pTriGroup=Node; //nowy lider ma się sam wyświetlać - wskaźnik na siebie
              pTriGroup=Node; //zapamiętanie lidera
             }
        */
        break;
    case TP_TRACTION:
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP: // te renderowane na końcu, żeby nie łapały koloru nieba
        Node->nNext3 = nRenderWires;
        nRenderWires = Node; // lista drutów
        break;
    case TP_MODEL: // modele zawsze wyświetlane z własnego VBO
        // jeśli model jest prosty, można próbować zrobić wspólną siatkę (słupy)
        if ((Node->iFlags & 0x20200020) == 0) // czy brak przezroczystości?
        {
            Node->nNext3 = nRender;
            nRender = Node;
        } // do nieprzezroczystych
        else if ((Node->iFlags & 0x10100010) == 0) // czy brak nieprzezroczystości?
        {
            Node->nNext3 = nRenderAlpha;
            nRenderAlpha = Node;
        } // do przezroczystych
        else // jak i take i takie, to będzie dwa razy renderowane...
        {
            Node->nNext3 = nRenderMixed;
            nRenderMixed = Node;
        } // do mieszanych
        // Node->nNext3=nMeshed; //dopisanie do listy sortowania
        // nMeshed=Node;
        break;
    case TP_MEMCELL:
    case TP_TRACTIONPOWERSOURCE: // a te w ogóle pomijamy
        //  case TP_ISOLATED: //lista torów w obwodzie izolowanym - na razie ignorowana
        break;
    case TP_DYNAMIC:
        return; // tych nie dopisujemy wcale
    }
    Node->nNext2 = nRootNode; // dopisanie do ogólnej listy
    nRootNode = Node;
    ++iNodeCount; // licznik obiektów
}

void TSubRect::RaNodeAdd(TGroundNode *Node)
{ // finalna kwalifikacja na listy renderowania, jeśli nie obsługiwane grupowo
    switch (Node->iType)
    {
    case TP_TRACK:
        if (Global::bUseVBO)
        {
            Node->nNext3 = nRenderRect;
            nRenderRect = Node;
        } // VBO: do nieprzezroczystych z sektora
        else
        {
            Node->nNext3 = nRender;
            nRender = Node;
        } // DL: do nieprzezroczystych
        break;
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLES:
        if (Node->iFlags & 0x20) // czy jest przezroczyste?
        {
            Node->nNext3 = nRenderRectAlpha;
            nRenderRectAlpha = Node;
        } // DL: do przezroczystych z sektora
        else if (Global::bUseVBO)
        {
            Node->nNext3 = nRenderRect;
            nRenderRect = Node;
        } // VBO: do nieprzezroczystych z sektora
        else
        {
            Node->nNext3 = nRender;
            nRender = Node;
        } // DL: do nieprzezroczystych wszelakich
        break;
    case TP_MODEL: // modele zawsze wyświetlane z własnego VBO
        if ((Node->iFlags & 0x20200020) == 0) // czy brak przezroczystości?
        {
            Node->nNext3 = nRender;
            nRender = Node;
        } // do nieprzezroczystych
        else if ((Node->iFlags & 0x10100010) == 0) // czy brak nieprzezroczystości?
        {
            Node->nNext3 = nRenderAlpha;
            nRenderAlpha = Node;
        } // do przezroczystych
        else // jak i take i takie, to będzie dwa razy renderowane...
        {
            Node->nNext3 = nRenderMixed;
            nRenderMixed = Node;
        } // do mieszanych
        break;
    case TP_MESH: // grupa ze wspólną teksturą
        //{Node->nNext3=nRenderRect; nRenderRect=Node;} //do nieprzezroczystych z sektora
        {
            Node->nNext3 = nRender;
            nRender = Node;
        } // do nieprzezroczystych
        break;
    case TP_SUBMODEL: // submodele terenu w kwadracie kilometrowym idą do nRootMesh
        // WriteLog("nRootMesh was "+AnsiString(nRootMesh?"not null ":"null
        // ")+IntToHex(int(this),8));
        Node->nNext3 = nRootMesh; // przy VBO musi być inaczej
        nRootMesh = Node;
        break;
    }
}

void TSubRect::Sort()
{ // przygotowanie sektora do renderowania
    TGroundNode **n0, *n1, *n2; // wskaźniki robocze
    delete[] tTracks; // usunięcie listy
    tTracks =
        iTracks ? new TTrack *[iTracks] : NULL; // tworzenie tabeli torów do renderowania pojazdów
    if (tTracks)
    { // wypełnianie tabeli torów
        int i = 0;
        for (n1 = nRootNode; n1; n1 = n1->nNext2) // kolejne obiekty z sektora
            if (n1->iType == TP_TRACK)
                tTracks[i++] = n1->pTrack;
    }
    // sortowanie obiektów w sektorze na listy renderowania
    if (!nMeshed)
        return; // nie ma nic do sortowania
    bool sorted = false;
    while (!sorted)
    { // sortowanie bąbelkowe obiektów wg tekstury
        sorted = true; // zakładamy posortowanie
        n0 = &nMeshed; // wskaźnik niezbędny do zamieniania obiektów
        n1 = nMeshed; // lista obiektów przetwarzanych na statyczne siatki
        while (n1)
        { // sprawdzanie stanu posortowania obiektów i ewentualne zamiany
            n2 = n1->nNext3; // kolejny z tej listy
            if (n2) // jeśli istnieje
                if (n1->TextureID > n2->TextureID)
                { // zamiana elementów miejscami
                    *n0 = n2; // drugi będzie na początku
                    n1->nNext3 = n2->nNext3; // ten zza drugiego będzie za pierwszym
                    n2->nNext3 = n1; // a za drugim będzie pierwszy
                    sorted = false; // potrzebny kolejny przebieg
                }
            n0 = &(n1->nNext3);
            n1 = n2;
        };
    }
    // wyrzucenie z listy obiektów pojedynczych (nie ma z czym ich grupować)
    // nawet jak są pojedyncze, to i tak lepiej, aby były w jednym Display List
    /*
        else
        {//dodanie do zwykłej listy renderowania i usunięcie z grupowego
         *n0=n2; //drugi będzie na początku
         RaNodeAdd(n1); //nie ma go z czym zgrupować; (n1->nNext3) zostanie nadpisane
         n1=n2; //potrzebne do ustawienia (n0)
        }
    */
    //...
    // przeglądanie listy i tworzenie obiektów renderujących dla danej tekstury
    GLuint t = 0; // pomocniczy kod tekstury
    n1 = nMeshed; // lista obiektów przetwarzanych na statyczne siatki
    while (n1)
    { // dla każdej tekstury powinny istnieć co najmniej dwa obiekty, ale dla DL nie ma to znaczenia
        if (t < n1->TextureID) // jeśli (n1) ma inną teksturę niż poprzednie
        { // można zrobić obiekt renderujący
            t = n1->TextureID;
			n2 = new TGroundNode(); // BUG: source of a memory leak here
            n2->nNext2 = nRootMesh;
            nRootMesh = n2; // podczepienie na początku listy
            nRootMesh->iType = TP_MESH; // obiekt renderujący siatki dla tekstury
            nRootMesh->TextureID = t;
            nRootMesh->nNode = n1; // pierwszy element z listy
            nRootMesh->pCenter = n1->pCenter;
            nRootMesh->fSquareRadius = 1e8; // widać bez ograniczeń
            nRootMesh->fSquareMinRadius = 0.0;
            nRootMesh->iFlags = 0x10;
            RaNodeAdd(nRootMesh); // dodanie do odpowiedniej listy renderowania
        }
        n1 = n1->nNext3; // kolejny z tej listy
    };
}

TTrack * TSubRect::FindTrack(vector3 *Point, int &iConnection, TTrack *Exclude)
{ // szukanie toru, którego koniec jest najbliższy (*Point)
    TTrack *Track;
    for (int i = 0; i < iTracks; ++i)
        if (tTracks[i] != Exclude) // można użyć tabelę torów, bo jest mniejsza
        {
            iConnection = tTracks[i]->TestPoint(Point);
            if (iConnection >= 0)
                return tTracks[i]; // szukanie TGroundNode nie jest potrzebne
        }
    /*
     TGroundNode *Current;
     for (Current=nRootNode;Current;Current=Current->Next)
      if ((Current->iType==TP_TRACK)&&(Current->pTrack!=Exclude)) //można użyć tabelę torów
       {
        iConnection=Current->pTrack->TestPoint(Point);
        if (iConnection>=0) return Current;
       }
    */
    return NULL;
};

bool TSubRect::RaTrackAnimAdd(TTrack *t)
{ // aktywacja animacji torów w VBO (zwrotnica, obrotnica)
    if (m_nVertexCount < 0)
        return true; // nie ma animacji, gdy nie widać
    if (tTrackAnim)
        tTrackAnim->RaAnimListAdd(t);
    else
        tTrackAnim = t;
    return false; // będzie animowane...
}

void TSubRect::RaAnimate()
{ // wykonanie animacji
    if (!tTrackAnim)
        return; // nie ma nic do animowania
    if (Global::bUseVBO)
    { // odświeżenie VBO sektora
        if (true == GLEW_VERSION_1_5) // modyfikacje VBO są dostępne od OpenGL 1.5
            glBindBuffer(GL_ARRAY_BUFFER, m_nVBOVertices);
        else // dla OpenGL 1.4 z GL_ARB_vertex_buffer_object odświeżenie całego sektora
            Release(); // opróżnienie VBO sektora, aby się odświeżył z nowymi ustawieniami
    }
    tTrackAnim = tTrackAnim->RaAnimate(); // przeliczenie animacji kolejnego
};

TTraction * TSubRect::FindTraction(vector3 *Point, int &iConnection, TTraction *Exclude)
{ // szukanie przęsła w sektorze, którego koniec jest najbliższy (*Point)
    TGroundNode *Current;
    for (Current = nRenderWires; Current; Current = Current->nNext3)
        if ((Current->iType == TP_TRACTION) && (Current->hvTraction != Exclude))
        {
            iConnection = Current->hvTraction->TestPoint(Point);
            if (iConnection >= 0)
                return Current->hvTraction;
        }
    return NULL;
};

void TSubRect::LoadNodes()
{ // utworzenie siatek VBO dla wszystkich node w sektorze
    if (m_nVertexCount >= 0)
        return; // obiekty były już sprawdzone
    m_nVertexCount = 0; //-1 oznacza, że nie sprawdzono listy obiektów
    if (!nRootNode)
        return;
    TGroundNode *n = nRootNode;
    while (n)
    {
        switch (n->iType)
        {
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLES:
            n->iVboPtr = m_nVertexCount; // nowy początek
            m_nVertexCount += n->iNumVerts;
            break;
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
            n->iVboPtr = m_nVertexCount; // nowy początek
            m_nVertexCount +=
                n->iNumPts; // miejsce w tablicach normalnych i teksturowania się zmarnuje...
            break;
        case TP_TRACK:
            n->iVboPtr = m_nVertexCount; // nowy początek
            n->iNumVerts = n->pTrack->RaArrayPrepare(); // zliczenie wierzchołków
            m_nVertexCount += n->iNumVerts;
            break;
        case TP_TRACTION:
            n->iVboPtr = m_nVertexCount; // nowy początek
            n->iNumVerts = n->hvTraction->RaArrayPrepare(); // zliczenie wierzchołków
            m_nVertexCount += n->iNumVerts;
            break;
        }
        n = n->nNext2; // następny z sektora
    }
    if (!m_nVertexCount)
        return; // jeśli nie ma obiektów do wyświetlenia z VBO, to koniec
    if (Global::bUseVBO)
    { // tylko liczenie wierzchołów, gdy nie ma VBO
        MakeArray(m_nVertexCount);
        n = nRootNode;
        int i;
        while (n)
        {
            if (n->iVboPtr >= 0)
                switch (n->iType)
                {
                case GL_TRIANGLE_STRIP:
                case GL_TRIANGLE_FAN:
                case GL_TRIANGLES:
                    for (i = 0; i < n->iNumVerts; ++i)
                    { // Ra: trójkąty można od razu wczytywać do takich tablic... to może poczekać
                        m_pVNT[n->iVboPtr + i].x = n->Vertices[i].Point.x;
                        m_pVNT[n->iVboPtr + i].y = n->Vertices[i].Point.y;
                        m_pVNT[n->iVboPtr + i].z = n->Vertices[i].Point.z;
                        m_pVNT[n->iVboPtr + i].nx = n->Vertices[i].Normal.x;
                        m_pVNT[n->iVboPtr + i].ny = n->Vertices[i].Normal.y;
                        m_pVNT[n->iVboPtr + i].nz = n->Vertices[i].Normal.z;
                        m_pVNT[n->iVboPtr + i].u = n->Vertices[i].tu;
                        m_pVNT[n->iVboPtr + i].v = n->Vertices[i].tv;
                    }
                    break;
                case GL_LINES:
                case GL_LINE_STRIP:
                case GL_LINE_LOOP:
                    for (i = 0; i < n->iNumPts; ++i)
                    {
                        m_pVNT[n->iVboPtr + i].x = n->Points[i].x;
                        m_pVNT[n->iVboPtr + i].y = n->Points[i].y;
                        m_pVNT[n->iVboPtr + i].z = n->Points[i].z;
                        // miejsce w tablicach normalnych i teksturowania się marnuje...
                    }
                    break;
                case TP_TRACK:
                    if (n->iNumVerts) // bo tory zabezpieczające są niewidoczne
                        n->pTrack->RaArrayFill(m_pVNT + n->iVboPtr, m_pVNT);
                    break;
                case TP_TRACTION:
                    if (n->iNumVerts) // druty mogą być niewidoczne...?
                        n->hvTraction->RaArrayFill(m_pVNT + n->iVboPtr);
                    break;
                }
            n = n->nNext2; // następny z sektora
        }
        BuildVBOs();
    }
    if (Global::bManageNodes)
        ResourceManager::Register(this); // dodanie do automatu zwalniającego pamięć
}

bool TSubRect::StartVBO()
{ // początek rysowania elementów z VBO w sektorze
    SetLastUsage(Timer::GetSimulationTime()); // te z tyłu będą niepotrzebnie zwalniane
    return CMesh::StartVBO();
};

void TSubRect::Release()
{ // wirtualne zwolnienie zasobów przez sprzątacz albo destruktor
    if (Global::bUseVBO)
        CMesh::Clear(); // usuwanie buforów
};

void TSubRect::RenderDL()
{ // renderowanie nieprzezroczystych (DL)
    TGroundNode *node;
    RaAnimate(); // przeliczenia animacji torów w sektorze
    for (node = nRender; node; node = node->nNext3)
        node->RenderDL(); // nieprzezroczyste obiekty (oprócz pojazdów)
    for (node = nRenderMixed; node; node = node->nNext3)
        node->RenderDL(); // nieprzezroczyste z mieszanych modeli
    for (int j = 0; j < iTracks; ++j)
        tTracks[j]->RenderDyn(); // nieprzezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderAlphaDL()
{ // renderowanie przezroczystych modeli oraz pojazdów (DL)
    TGroundNode *node;
    for (node = nRenderMixed; node; node = node->nNext3)
        node->RenderAlphaDL(); // przezroczyste z mieszanych modeli
    for (node = nRenderAlpha; node; node = node->nNext3)
        node->RenderAlphaDL(); // przezroczyste modele
    // for (node=tmp->nRender;node;node=node->nNext3)
    // if (node->iType==TP_TRACK)
    //  node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
    for (int j = 0; j < iTracks; ++j)
        tTracks[j]->RenderDynAlpha(); // przezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderVBO()
{ // renderowanie nieprzezroczystych (VBO)
    TGroundNode *node;
    RaAnimate(); // przeliczenia animacji torów w sektorze
    LoadNodes(); // czemu tutaj?
    if (StartVBO())
    {
        for (node = nRenderRect; node; node = node->nNext3)
            if (node->iVboPtr >= 0)
                node->RenderVBO(); // nieprzezroczyste obiekty terenu
        EndVBO();
    }
    for (node = nRender; node; node = node->nNext3)
        node->RenderVBO(); // nieprzezroczyste obiekty (oprócz pojazdów)
    for (node = nRenderMixed; node; node = node->nNext3)
        node->RenderVBO(); // nieprzezroczyste z mieszanych modeli
    for (int j = 0; j < iTracks; ++j)
        tTracks[j]->RenderDyn(); // nieprzezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderAlphaVBO()
{ // renderowanie przezroczystych modeli oraz pojazdów (VBO)
    TGroundNode *node;
    for (node = nRenderMixed; node; node = node->nNext3)
        node->RenderAlphaVBO(); // przezroczyste z mieszanych modeli
    for (node = nRenderAlpha; node; node = node->nNext3)
        node->RenderAlphaVBO(); // przezroczyste modele
    // for (node=tmp->nRender;node;node=node->nNext3)
    // if (node->iType==TP_TRACK)
    //  node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
    for (int j = 0; j < iTracks; ++j)
        tTracks[j]->RenderDynAlpha(); // przezroczyste fragmenty pojazdów na torach
};

void TSubRect::RenderSounds()
{ // aktualizacja dźwięków w pojazdach sektora (sektor może nie być wyświetlany)
    for (int j = 0; j < iTracks; ++j)
        tTracks[j]->RenderDynSounds(); // dźwięki pojazdów idą niezależnie od wyświetlania
};
//---------------------------------------------------------------------------
//------------------ Kwadrat kilometrowy ------------------------------------
//---------------------------------------------------------------------------
int TGroundRect::iFrameNumber = 0; // licznik wyświetlanych klatek

TGroundRect::TGroundRect()
{
    pSubRects = NULL;
    nTerrain = NULL;
};

TGroundRect::~TGroundRect()
{
    SafeDeleteArray(pSubRects);
};

void TGroundRect::RenderDL()
{ // renderowanie kwadratu kilometrowego (DL), jeśli jeszcze nie zrobione
    if (iLastDisplay != iFrameNumber)
    { // tylko jezeli dany kwadrat nie był jeszcze renderowany
        // for (TGroundNode* node=pRender;node;node=node->pNext3)
        // node->Render(); //nieprzezroczyste trójkąty kwadratu kilometrowego
        if (nRender)
        { //łączenie trójkątów w jedną listę - trochę wioska
            if (!nRender->DisplayListID || (nRender->iVersion != Global::iReCompile))
            { // jeżeli nie skompilowany, kompilujemy wszystkie trójkąty w jeden
                nRender->fSquareRadius = 5000.0 * 5000.0; // aby agregat nigdy nie znikał
                nRender->DisplayListID = glGenLists(1);
                glNewList(nRender->DisplayListID, GL_COMPILE);
                nRender->iVersion = Global::iReCompile; // aktualna wersja siatek
                for (TGroundNode *node = nRender; node; node = node->nNext3) // następny tej grupy
                    node->Compile(true);
                glEndList();
            }
            nRender->RenderDL(); // nieprzezroczyste trójkąty kwadratu kilometrowego
        }
        if (nRootMesh)
            nRootMesh->RenderDL();
        iLastDisplay = iFrameNumber; // drugi raz nie potrzeba
    }
};

void TGroundRect::RenderVBO()
{ // renderowanie kwadratu kilometrowego (VBO), jeśli jeszcze nie zrobione
    if (iLastDisplay != iFrameNumber)
    { // tylko jezeli dany kwadrat nie był jeszcze renderowany
        LoadNodes(); // ewentualne tworzenie siatek
        if (StartVBO())
        {
            for (TGroundNode *node = nRenderRect; node; node = node->nNext3) // następny tej grupy
                node->RaRenderVBO(); // nieprzezroczyste trójkąty kwadratu kilometrowego
            EndVBO();
            iLastDisplay = iFrameNumber;
        }
        if (nTerrain)
            nTerrain->smTerrain->iVisible = iFrameNumber; // ma się wyświetlić w tej ramce
    }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void TGround::MoveGroundNode(vector3 pPosition)
{ // Ra: to wymaga gruntownej reformy
    /*
     TGroundNode *Current;
     for (Current=RootNode;Current!=NULL;Current=Current->Next)
      Current->MoveMe(pPosition);

     TGroundRect *Rectx=new TGroundRect; //kwadrat kilometrowy
     for(int i=0;i<iNumRects;i++)
      for(int j=0;j<iNumRects;j++)
       Rects[i][j]=*Rectx; //kopiowanie zawartości do każdego kwadratu
     delete Rectx;
     for (Current=RootNode;Current!=NULL;Current=Current->Next)
     {//rozłożenie obiektów na mapie
      if (Current->iType!=TP_DYNAMIC)
      {//pojazdów to w ogóle nie dotyczy
       if ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true //~czy trójkąt?
        :(Current->iFlags&0x20)?true //~czy teksturę ma nieprzezroczystą?
         //:(Current->iNumVerts!=3)?true //~czy tylko jeden trójkąt?
         :(Current->fSquareMinRadius!=0.0)?true //~czy widoczny z bliska?
          :(Current->fSquareRadius<=90000.0)) //~czy widoczny z daleka?
        GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
       else //dodajemy do kwadratu kilometrowego
        GetRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
      }
     }
     for (Current=RootDynamic;Current!=NULL;Current=Current->Next)
     {
      Current->pCenter+=pPosition;
      Current->DynamicObject->UpdatePos();
     }
     for (Current=RootDynamic;Current!=NULL;Current=Current->Next)
      Current->DynamicObject->MoverParameters->Physic_ReActivation();
    */
}

std::vector<TGroundVertex> TempVerts;
/*
TGroundVertex TempVerts[ 10000 ]; // tu wczytywane s¹ trójk¹ty
*/
BYTE TempConnectionType[ 200 ]; // Ra: sprzêgi w sk³adzie; ujemne, gdy odwrotnie

TGround::TGround()
{
    Global::pGround = this;

    for( int i = 0; i < TP_LAST; ++i ) {
        nRootOfType[ i ] = nullptr; // zerowanie tablic wyszukiwania
    }
#ifdef EU07_USE_OLD_TNAMES_CLASS
    sTracks = new TNames(); // nazwy torów - na razie tak
#endif
    ::SecureZeroMemory( TempConnectionType, sizeof( TempConnectionType ) );
    ::SecureZeroMemory( pRendered, sizeof( pRendered ) );
}

TGround::~TGround()
{
    Free();
}

void TGround::Free()
{
    TEvent *tmp;
    for (TEvent *Current = RootEvent; Current;)
    {
        tmp = Current;
        Current = Current->evNext2;
        delete tmp;
    }
    TGroundNode *tmpn;
    for (int i = 0; i < TP_LAST; ++i)
    {
        for (TGroundNode *Current = nRootOfType[i]; Current;)
        {
            tmpn = Current;
            Current = Current->nNext;
            delete tmpn;
        }
        nRootOfType[i] = NULL;
    }
    for (TGroundNode *Current = nRootDynamic; Current;)
    {
        tmpn = Current;
        Current = Current->nNext;
        delete tmpn;
    }
    iNumNodes = 0;
    // RootNode=NULL;
    nRootDynamic = NULL;
#ifdef EU07_USE_OLD_TNAMES_CLASS
    delete sTracks;
#endif
}

TGroundNode * TGround::DynamicFindAny(std::string asNameToFind)
{ // wyszukanie pojazdu o podanej nazwie, szukanie po wszystkich (użyć drzewa!)
    for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
        if ((Current->asName == asNameToFind))
            return Current;
    return NULL;
};

TGroundNode * TGround::DynamicFind(std::string asNameToFind)
{ // wyszukanie pojazdu z obsadą o podanej nazwie (użyć drzewa!)
    for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
        if (Current->DynamicObject->Mechanik)
            if ((Current->asName == asNameToFind))
                return Current;
    return NULL;
};

void TGround::DynamicList(bool all)
{ // odesłanie nazw pojazdów dostępnych na scenerii (nazwy, szczególnie wagonów, mogą się
    // powtarzać!)
    for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
        if (all || Current->DynamicObject->Mechanik)
            WyslijString(Current->asName, 6); // same nazwy pojazdów
    WyslijString("none", 6); // informacja o końcu listy
};

TGroundNode * TGround::FindGroundNode(std::string asNameToFind, TGroundNodeType iNodeType)
{ // wyszukiwanie obiektu o podanej nazwie i konkretnym typie
    if ((iNodeType == TP_TRACK) || (iNodeType == TP_MEMCELL) || (iNodeType == TP_MODEL))
    { // wyszukiwanie w drzewie binarnym
#ifdef EU07_USE_OLD_TNAMES_CLASS
        return (TGroundNode *)sTracks->Find(iNodeType, asNameToFind.c_str());
#else
/*
        switch( iNodeType ) {

            case TP_TRACK: {
                auto const lookup = m_trackmap.find( asNameToFind );
                return
                    lookup != m_trackmap.end() ?
                    lookup->second :
                    nullptr;
            }
            case TP_MODEL: {
                auto const lookup = m_modelmap.find( asNameToFind );
                return
                    lookup != m_modelmap.end() ?
                    lookup->second :
                    nullptr;
            }
            case TP_MEMCELL: {
                auto const lookup = m_memcellmap.find( asNameToFind );
                return
                    lookup != m_memcellmap.end() ?
                    lookup->second :
                    nullptr;
            }
        }
        return nullptr;
*/
        return m_trackmap.Find( iNodeType, asNameToFind );
#endif
    }
    // standardowe wyszukiwanie liniowe
    TGroundNode *Current;
    for (Current = nRootOfType[iNodeType]; Current; Current = Current->nNext)
        if (Current->asName == asNameToFind)
            return Current;
    return NULL;
}

double fTrainSetVel = 0;
double fTrainSetDir = 0;
double fTrainSetDist = 0; // odległość składu od punktu 1 w stronę punktu 2
string asTrainSetTrack = "";
int iTrainSetConnection = 0;
bool bTrainSet = false;
string asTrainName = "";
int iTrainSetWehicleNumber = 0;
TGroundNode *nTrainSetNode = NULL; // poprzedni pojazd do łączenia
TGroundNode *nTrainSetDriver = NULL; // pojazd, któremu zostanie wysłany rozkład

void TGround::RaTriangleDivider(TGroundNode *node)
{ // tworzy dodatkowe trójkąty i zmiejsza podany
    // to jest wywoływane przy wczytywaniu trójkątów
    // dodatkowe trójkąty są dodawane do głównej listy trójkątów
    // podział trójkątów na sektory i kwadraty jest dokonywany później w FirstInit
    if (node->iType != GL_TRIANGLES)
        return; // tylko pojedyncze trójkąty
    if (node->iNumVerts != 3)
        return; // tylko gdy jeden trójkąt
    double x0 = 1000.0 * floor(0.001 * node->pCenter.x) - 200.0;
    double x1 = x0 + 1400.0;
    double z0 = 1000.0 * floor(0.001 * node->pCenter.z) - 200.0;
    double z1 = z0 + 1400.0;
    if ((node->Vertices[0].Point.x >= x0) && (node->Vertices[0].Point.x <= x1) &&
        (node->Vertices[0].Point.z >= z0) && (node->Vertices[0].Point.z <= z1) &&
        (node->Vertices[1].Point.x >= x0) && (node->Vertices[1].Point.x <= x1) &&
        (node->Vertices[1].Point.z >= z0) && (node->Vertices[1].Point.z <= z1) &&
        (node->Vertices[2].Point.x >= x0) && (node->Vertices[2].Point.x <= x1) &&
        (node->Vertices[2].Point.z >= z0) && (node->Vertices[2].Point.z <= z1))
        return; // trójkąt wystający mniej niż 200m z kw. kilometrowego jest do przyjęcia
    // Ra: przerobić na dzielenie na 2 trójkąty, podział w przecięciu z siatką kilometrową
    // Ra: i z rekurencją będzie dzielić trzy trójkąty, jeśli będzie taka potrzeba
    int divide = -1; // bok do podzielenia: 0=AB, 1=BC, 2=CA; +4=podział po OZ; +8 na x1/z1
    double min = 0, mul; // jeśli przechodzi przez oś, iloczyn będzie ujemny
    x0 += 200.0;
    x1 -= 200.0; // przestawienie na siatkę
    z0 += 200.0;
    z1 -= 200.0;
    mul = (node->Vertices[0].Point.x - x0) * (node->Vertices[1].Point.x - x0); // AB na wschodzie
    if (mul < min)
        min = mul, divide = 0;
    mul = (node->Vertices[1].Point.x - x0) * (node->Vertices[2].Point.x - x0); // BC na wschodzie
    if (mul < min)
        min = mul, divide = 1;
    mul = (node->Vertices[2].Point.x - x0) * (node->Vertices[0].Point.x - x0); // CA na wschodzie
    if (mul < min)
        min = mul, divide = 2;
    mul = (node->Vertices[0].Point.x - x1) * (node->Vertices[1].Point.x - x1); // AB na zachodzie
    if (mul < min)
        min = mul, divide = 8;
    mul = (node->Vertices[1].Point.x - x1) * (node->Vertices[2].Point.x - x1); // BC na zachodzie
    if (mul < min)
        min = mul, divide = 9;
    mul = (node->Vertices[2].Point.x - x1) * (node->Vertices[0].Point.x - x1); // CA na zachodzie
    if (mul < min)
        min = mul, divide = 10;
    mul = (node->Vertices[0].Point.z - z0) * (node->Vertices[1].Point.z - z0); // AB na południu
    if (mul < min)
        min = mul, divide = 4;
    mul = (node->Vertices[1].Point.z - z0) * (node->Vertices[2].Point.z - z0); // BC na południu
    if (mul < min)
        min = mul, divide = 5;
    mul = (node->Vertices[2].Point.z - z0) * (node->Vertices[0].Point.z - z0); // CA na południu
    if (mul < min)
        min = mul, divide = 6;
    mul = (node->Vertices[0].Point.z - z1) * (node->Vertices[1].Point.z - z1); // AB na północy
    if (mul < min)
        min = mul, divide = 12;
    mul = (node->Vertices[1].Point.z - z1) * (node->Vertices[2].Point.z - z1); // BC na północy
    if (mul < min)
        min = mul, divide = 13;
    mul = (node->Vertices[2].Point.z - z1) * (node->Vertices[0].Point.z - z1); // CA na północy
    if (mul < min)
        divide = 14;
    // tworzymy jeden dodatkowy trójkąt, dzieląc jeden bok na przecięciu siatki kilometrowej
    TGroundNode *ntri; // wskaźnik na nowy trójkąt
    ntri = new TGroundNode(); // a ten jest nowy
    ntri->iType = GL_TRIANGLES; // kopiowanie parametrów, przydałby się konstruktor kopiujący
    ntri->Init(3);
    ntri->TextureID = node->TextureID;
    ntri->iFlags = node->iFlags;
    for (int j = 0; j < 4; ++j)
    {
        ntri->Ambient[j] = node->Ambient[j];
        ntri->Diffuse[j] = node->Diffuse[j];
        ntri->Specular[j] = node->Specular[j];
    }
    ntri->asName = node->asName;
    ntri->fSquareRadius = node->fSquareRadius;
    ntri->fSquareMinRadius = node->fSquareMinRadius;
    ntri->bVisible = node->bVisible; // a są jakieś niewidoczne?
    ntri->nNext = nRootOfType[GL_TRIANGLES];
    nRootOfType[GL_TRIANGLES] = ntri; // dopisanie z przodu do listy
    iNumNodes++;
    switch (divide & 3)
    { // podzielenie jednego z boków, powstaje wierzchołek D
    case 0: // podział AB (0-1) -> ADC i DBC
        ntri->Vertices[2] = node->Vertices[2]; // wierzchołek C jest wspólny
        ntri->Vertices[1] = node->Vertices[1]; // wierzchołek B przechodzi do nowego
        // node->Vertices[1].HalfSet(node->Vertices[0],node->Vertices[1]); //na razie D tak
        if (divide & 4)
            node->Vertices[1].SetByZ(node->Vertices[0], node->Vertices[1], (divide & 8) ? z1 : z0);
        else
            node->Vertices[1].SetByX(node->Vertices[0], node->Vertices[1], (divide & 8) ? x1 : x0);
        ntri->Vertices[0] = node->Vertices[1]; // wierzchołek D jest wspólny
        break;
    case 1: // podział BC (1-2) -> ABD i ADC
        ntri->Vertices[0] = node->Vertices[0]; // wierzchołek A jest wspólny
        ntri->Vertices[2] = node->Vertices[2]; // wierzchołek C przechodzi do nowego
        // node->Vertices[2].HalfSet(node->Vertices[1],node->Vertices[2]); //na razie D tak
        if (divide & 4)
            node->Vertices[2].SetByZ(node->Vertices[1], node->Vertices[2], (divide & 8) ? z1 : z0);
        else
            node->Vertices[2].SetByX(node->Vertices[1], node->Vertices[2], (divide & 8) ? x1 : x0);
        ntri->Vertices[1] = node->Vertices[2]; // wierzchołek D jest wspólny
        break;
    case 2: // podział CA (2-0) -> ABD i DBC
        ntri->Vertices[1] = node->Vertices[1]; // wierzchołek B jest wspólny
        ntri->Vertices[2] = node->Vertices[2]; // wierzchołek C przechodzi do nowego
        // node->Vertices[2].HalfSet(node->Vertices[2],node->Vertices[0]); //na razie D tak
        if (divide & 4)
            node->Vertices[2].SetByZ(node->Vertices[2], node->Vertices[0], (divide & 8) ? z1 : z0);
        else
            node->Vertices[2].SetByX(node->Vertices[2], node->Vertices[0], (divide & 8) ? x1 : x0);
        ntri->Vertices[0] = node->Vertices[2]; // wierzchołek D jest wspólny
        break;
    }
    // przeliczenie środków ciężkości obu
    node->pCenter =
        (node->Vertices[0].Point + node->Vertices[1].Point + node->Vertices[2].Point) / 3.0;
    ntri->pCenter =
        (ntri->Vertices[0].Point + ntri->Vertices[1].Point + ntri->Vertices[2].Point) / 3.0;
    RaTriangleDivider(node); // rekurencja, bo nawet na TD raz nie wystarczy
    RaTriangleDivider(ntri);
};

TGroundNode * TGround::AddGroundNode(cParser *parser)
{ // wczytanie wpisu typu "node"
    // parser->LoadTraction=Global::bLoadTraction; //Ra: tu nie potrzeba powtarzać
	string str, str1, str2, str3, str4, Skin, DriverType, asNodeName;
    int nv, ti, i, n;
    double tf, r, rmin, tf1, tf2, tf3, tf4, l, dist, mgn;
    int int1, int2;
    bool bError = false, curve;
    vector3 pt, front, up, left, pos, tv;
    matrix4x4 mat2, mat1, mat;
    GLuint TexID;
    TGroundNode *tmp1;
    TTrack *Track;
    TTextSound *tmpsound;
    std::string token;
    parser->getTokens(2);
    *parser >> r >> rmin;
    parser->getTokens();
    *parser >> token;
    asNodeName = token;
    parser->getTokens();
    *parser >> token;
	str = token;
    //str = AnsiString(token.c_str());
    TGroundNode *tmp, *tmp2;
    tmp = new TGroundNode();
    tmp->asName = (asNodeName == "none" ? string("") : asNodeName);
    if (r >= 0)
        tmp->fSquareRadius = r * r;
    tmp->fSquareMinRadius = rmin * rmin;
    if (str == "triangles")
        tmp->iType = GL_TRIANGLES;
    else if (str == "triangle_strip")
        tmp->iType = GL_TRIANGLE_STRIP;
    else if (str == "triangle_fan")
        tmp->iType = GL_TRIANGLE_FAN;
    else if (str == "lines")
        tmp->iType = GL_LINES;
    else if (str == "line_strip")
        tmp->iType = GL_LINE_STRIP;
    else if (str == "line_loop")
        tmp->iType = GL_LINE_LOOP;
    else if (str == "model")
        tmp->iType = TP_MODEL;
    // else if (str=="terrain")             tmp->iType=TP_TERRAIN; //tymczasowo do odwołania
    else if (str == "dynamic")
        tmp->iType = TP_DYNAMIC;
    else if (str == "sound")
        tmp->iType = TP_SOUND;
    else if (str == "track")
        tmp->iType = TP_TRACK;
    else if (str == "memcell")
        tmp->iType = TP_MEMCELL;
    else if (str == "eventlauncher")
        tmp->iType = TP_EVLAUNCH;
    else if (str == "traction")
        tmp->iType = TP_TRACTION;
    else if (str == "tractionpowersource")
        tmp->iType = TP_TRACTIONPOWERSOURCE;
    // else if (str=="isolated")            tmp->iType=TP_ISOLATED;
    else
        bError = true;
    // WriteLog("-> node "+str+" "+tmp->asName);
    if (bError)
    {
        Error("Scene parse error near " + str);
        for (int i = 0; i < 60; ++i)
        { // Ra: skopiowanie dalszej części do logu - taka prowizorka, lepsza niż nic
            parser->getTokens(); // pobranie linijki tekstu nie działa
            *parser >> token;
            WriteLog(token);
        }
        // if (tmp==RootNode) RootNode=NULL;
        delete tmp;
        return NULL;
    }
    switch (tmp->iType)
    {
    case TP_TRACTION:
        tmp->hvTraction = new TTraction();
        parser->getTokens();
        *parser >> token;
        tmp->hvTraction->asPowerSupplyName = token; // nazwa zasilacza
        parser->getTokens(3);
        *parser >> tmp->hvTraction->NominalVoltage >> tmp->hvTraction->MaxCurrent >>
            tmp->hvTraction->fResistivity;
        if (tmp->hvTraction->fResistivity == 0.01) // tyle jest w sceneriach [om/km]
            tmp->hvTraction->fResistivity = 0.075; // taka sensowniejsza wartość za
        // http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
        tmp->hvTraction->fResistivity *= 0.001; // teraz [om/m]
        parser->getTokens();
        *parser >> token;
        // Ra 2014-02: a tutaj damy symbol sieci i jej budowę, np.:
        // SKB70-C, CuCd70-2C, KB95-2C, C95-C, C95-2C, YC95-2C, YpC95-2C, YC120-2C
        // YpC120-2C, YzC120-2C, YwsC120-2C, YC150-C150, YC150-2C150, C150-C150
        // C120-2C, 2C120-2C, 2C120-2C-1, 2C120-2C-2, 2C120-2C-3, 2C120-2C-4
        if (token.compare("none") == 0)
            tmp->hvTraction->Material = 0;
        else if (token.compare("al") == 0)
            tmp->hvTraction->Material = 2; // 1=aluminiowa, rysuje się na czarno
        else
            tmp->hvTraction->Material = 1; // 1=miedziana, rysuje się na zielono albo czerwono
        parser->getTokens();
        *parser >> tmp->hvTraction->WireThickness;
        parser->getTokens();
        *parser >> tmp->hvTraction->DamageFlag;
        parser->getTokens(3);
        *parser >> tmp->hvTraction->pPoint1.x >> tmp->hvTraction->pPoint1.y >>
            tmp->hvTraction->pPoint1.z;
        tmp->hvTraction->pPoint1 += pOrigin;
        parser->getTokens(3);
        *parser >> tmp->hvTraction->pPoint2.x >> tmp->hvTraction->pPoint2.y >>
            tmp->hvTraction->pPoint2.z;
        tmp->hvTraction->pPoint2 += pOrigin;
        parser->getTokens(3);
        *parser >> tmp->hvTraction->pPoint3.x >> tmp->hvTraction->pPoint3.y >>
            tmp->hvTraction->pPoint3.z;
        tmp->hvTraction->pPoint3 += pOrigin;
        parser->getTokens(3);
        *parser >> tmp->hvTraction->pPoint4.x >> tmp->hvTraction->pPoint4.y >>
            tmp->hvTraction->pPoint4.z;
        tmp->hvTraction->pPoint4 += pOrigin;
        parser->getTokens();
        *parser >> tf1;
        tmp->hvTraction->fHeightDifference =
            (tmp->hvTraction->pPoint3.y - tmp->hvTraction->pPoint1.y + tmp->hvTraction->pPoint4.y -
             tmp->hvTraction->pPoint2.y) *
                0.5f -
            tf1;
        parser->getTokens();
        *parser >> tf1;
        if (tf1 > 0)
            tmp->hvTraction->iNumSections =
                (tmp->hvTraction->pPoint1 - tmp->hvTraction->pPoint2).Length() / tf1;
        else
            tmp->hvTraction->iNumSections = 0;
        parser->getTokens();
        *parser >> tmp->hvTraction->Wires;
        parser->getTokens();
        *parser >> tmp->hvTraction->WireOffset;
        parser->getTokens();
        *parser >> token;
        tmp->bVisible = (token.compare("vis") == 0);
        parser->getTokens();
        *parser >> token;
        if (token.compare("parallel") == 0)
        { // jawne wskazanie innego przęsła, na które może przestawić się pantograf
            parser->getTokens();
            *parser >> token; // wypadało by to zapamiętać...
            tmp->hvTraction->asParallel = token;
            parser->getTokens();
            *parser >> token; // a tu już powinien być koniec
        }
        if (token.compare("endtraction") != 0)
            Error("ENDTRACTION delimiter missing! " + str2 + " found instead.");
        tmp->hvTraction->Init(); // przeliczenie parametrów
        // if (Global::bLoadTraction)
        // tmp->hvTraction->Optimize(); //generowanie DL dla wszystkiego przy wczytywaniu?
        tmp->pCenter = (tmp->hvTraction->pPoint2 + tmp->hvTraction->pPoint1) * 0.5f;
        // if (!Global::bLoadTraction) SafeDelete(tmp); //Ra: tak być nie może, bo NULL to błąd
        break;
    case TP_TRACTIONPOWERSOURCE:
        parser->getTokens(3);
        *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
        tmp->pCenter += pOrigin;
        tmp->psTractionPowerSource = new TTractionPowerSource(tmp);
        tmp->psTractionPowerSource->Load(parser);
        break;
    case TP_MEMCELL:
        parser->getTokens(3);
        *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
        tmp->pCenter.RotateY(aRotate.y / 180.0 * M_PI); // Ra 2014-11: uwzględnienie rotacji
        tmp->pCenter += pOrigin;
        tmp->MemCell = new TMemCell(&tmp->pCenter);
        tmp->MemCell->Load(parser);
        if (!tmp->asName.empty()) // jest pusta gdy "none"
        { // dodanie do wyszukiwarki
#ifdef EU07_USE_OLD_TNAMES_CLASS
            if (sTracks->Update(TP_MEMCELL, tmp->asName.c_str(),
                                tmp)) // najpierw sprawdzić, czy już jest
            { // przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność
                // wsteczna)
                ErrorLog("Duplicated memcell: " + tmp->asName); // to zgłaszać duplikat
            }
            else
                sTracks->Add(TP_MEMCELL, tmp->asName.c_str(), tmp); // nazwa jest unikalna
#else
            if( false == m_trackmap.Add( TP_MEMCELL, tmp->asName, tmp ) ) {
                // przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność wsteczna)
                ErrorLog( "Duplicated memcell: " + tmp->asName ); // to zgłaszać duplikat
            }
#endif
        }
        break;
    case TP_EVLAUNCH:
        parser->getTokens(3);
        *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
        tmp->pCenter.RotateY(aRotate.y / 180.0 * M_PI); // Ra 2014-11: uwzględnienie rotacji
        tmp->pCenter += pOrigin;
        tmp->EvLaunch = new TEventLauncher();
        tmp->EvLaunch->Load(parser);
        break;
    case TP_TRACK:
        tmp->pTrack = new TTrack(tmp);
        if (Global::iWriteLogEnabled & 4)
            if (!tmp->asName.empty())
                WriteLog(tmp->asName);
        tmp->pTrack->Load(parser, pOrigin,
                          tmp->asName); // w nazwie może być nazwa odcinka izolowanego
        if (!tmp->asName.empty()) // jest pusta gdy "none"
        { // dodanie do wyszukiwarki
#ifdef EU07_USE_OLD_TNAMES_CLASS
            if (sTracks->Update(TP_TRACK, tmp->asName.c_str(),
                                tmp)) // najpierw sprawdzić, czy już jest
            { // przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność
                // wsteczna)
                if (tmp->pTrack->iCategoryFlag & 1) // jeśli jest zdublowany tor kolejowy
                    ErrorLog("Duplicated track: " + tmp->asName); // to zgłaszać duplikat
            }
            else
                sTracks->Add(TP_TRACK, tmp->asName.c_str(), tmp); // nazwa jest unikalna
#else
            if( false == m_trackmap.Add( TP_TRACK, tmp->asName, tmp ) ) {
                // przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność wsteczna)
                ErrorLog( "Duplicated track: " + tmp->asName ); // to zgłaszać duplikat
            }
#endif
        }
        tmp->pCenter = (tmp->pTrack->CurrentSegment()->FastGetPoint_0() +
                        tmp->pTrack->CurrentSegment()->FastGetPoint(0.5) +
                        tmp->pTrack->CurrentSegment()->FastGetPoint_1()) /
                       3.0;
        break;
    case TP_SOUND:
        parser->getTokens(3);
        *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
        tmp->pCenter.RotateY(aRotate.y / 180.0 * M_PI); // Ra 2014-11: uwzględnienie rotacji
        tmp->pCenter += pOrigin;
        parser->getTokens();
        *parser >> token;
		str = token;
		//str = AnsiString(token.c_str());
        tmp->tsStaticSound = new TTextSound(str, sqrt(tmp->fSquareRadius), tmp->pCenter.x, tmp->pCenter.y, tmp->pCenter.z, false, rmin);
        if (rmin < 0.0)
            rmin =
                0.0; // przywrócenie poprawnej wartości, jeśli służyła do wyłączenia efektu Dopplera

        //            tmp->pDirectSoundBuffer=TSoundsManager::GetFromName(str.c_str());
        //            tmp->iState=(Parser->GetNextSymbol().LowerCase()=="loop"?DSBPLAY_LOOPING:0);
        parser->getTokens();
        *parser >> token;
        break;
    case TP_DYNAMIC:
        tmp->DynamicObject = new TDynamicObject();
        // tmp->DynamicObject->Load(Parser);
        parser->getTokens();
        *parser >> token;
        str1 = token; // katalog
        // McZapkie: doszedl parametr ze zmienialna skora
        parser->getTokens();
        *parser >> token;
        Skin = token; // tekstura wymienna
        parser->getTokens();
        *parser >> token;
        str3 = token; // McZapkie-131102: model w MMD
        if (bTrainSet)
        { // jeśli pojazd jest umieszczony w składzie
            str = asTrainSetTrack;
            parser->getTokens();
            *parser >> tf1; // Ra: -1 oznacza odwrotne wstawienie, normalnie w składzie 0
            parser->getTokens();
            *parser >> token;
            DriverType = token; // McZapkie:010303 - w przyszlosci rozne
            // konfiguracje mechanik/pomocnik itp
            tf3 = fTrainSetVel; // prędkość
            parser->getTokens();
            *parser >> token;
            str4 = token;
            int2 = str4.find("."); // yB: wykorzystuje tutaj zmienna, ktora potem bedzie ladunkiem
            if (int2 != string::npos) // yB: jesli znalazl kropke, to ja przetwarza jako parametry
            {
                int dlugosc = str4.length();
                int1 = atoi(str4.substr(0, int2).c_str()); // niech sprzegiem bedzie do kropki cos
                str4 = str4.substr(int2 + 1, dlugosc - int2);
            }
            else
            {
                int1 = atoi(str4.c_str());
                str4 = "";
            }
            int2 = 0; // zeruje po wykorzystaniu
            //    *parser >> int1; //yB: nastawy i takie tam TUTAJ!!!!!
            if (int1 < 0)
                int1 = (-int1) |
                       ctrain_depot; // sprzęg zablokowany (pojazdy nierozłączalne przy manewrach)
            if (tf1 != -1.0)
                if (fabs(tf1) > 0.5) // maksymalna odległość między sprzęgami - do przemyślenia
                    int1 = 0; // likwidacja sprzęgu, jeśli odległość zbyt duża - to powinno być
            // uwzględniane w fizyce sprzęgów...
            TempConnectionType[iTrainSetWehicleNumber] = int1; // wartość dodatnia
        }
        else
        { // pojazd wstawiony luzem
            fTrainSetDist = 0; // zerowanie dodatkowego przesunięcia
            asTrainName = ""; // puste oznacza jazdę pojedynczego bez rozkładu, "none" jest dla
            // składu (trainset)
            parser->getTokens();
            *parser >> token;
            str = token; // track
            parser->getTokens();
            *parser >> tf1; // Ra: -1 oznacza odwrotne wstawienie
            parser->getTokens();
            *parser >> token;
            DriverType = token; // McZapkie:010303: obsada
            parser->getTokens();
            *parser >>
                tf3; // prędkość, niektórzy wpisują tu "3" jako sprzęg, żeby nie było tabliczki
            iTrainSetWehicleNumber = 0;
            TempConnectionType[iTrainSetWehicleNumber] = 3; // likwidacja tabliczki na końcu?
        }
        parser->getTokens();
        *parser >> int2; // ilość ładunku
        if (int2 > 0)
        { // jeżeli ładunku jest więcej niż 0, to rozpoznajemy jego typ
            parser->getTokens();
            *parser >> token;
            str2 = token; // LoadType
            if (str2 == "enddynamic") // idiotoodporność: ładunek bez podanego typu
            {
                str2 = "";
                int2 = 0; // ilość bez typu się nie liczy jako ładunek
            }
        }
        else
            str2 = ""; // brak ladunku

        tmp1 = FindGroundNode(str, TP_TRACK); // poszukiwanie toru
        if (tmp1 ? tmp1->pTrack != NULL : false)
        { // jeśli tor znaleziony
            Track = tmp1->pTrack;
            if (!iTrainSetWehicleNumber) // jeśli pierwszy pojazd
                if (Track->evEvent0) // jeśli tor ma Event0
                    if (fabs(fTrainSetVel) <= 1.0) // a skład stoi
                        if (fTrainSetDist >= 0.0) // ale może nie sięgać na owy tor
                            if (fTrainSetDist < 8.0) // i raczej nie sięga
                                fTrainSetDist =
                                    8.0; // przesuwamy około pół EU07 dla wstecznej zgodności
            // WriteLog("Dynamic shift: "+AnsiString(fTrainSetDist));
            /* //Ra: to jednak robi duże problemy - przesunięcie w dynamic jest przesunięciem do
               tyłu, odwrotnie niż w trainset
                if (!iTrainSetWehicleNumber) //dla pierwszego jest to przesunięcie (ujemne = do
               tyłu)
                 if (tf1!=-1.0) //-1 wyjątkowo oznacza odwrócenie
                  tf1=-tf1; //a dla kolejnych odległość między sprzęgami (ujemne = wbite)
            */
            tf3 = tmp->DynamicObject->Init(asNodeName, str1, Skin, str3, Track,
                                           (tf1 == -1.0 ? fTrainSetDist : fTrainSetDist - tf1),
                                           DriverType, tf3, asTrainName, int2, str2, (tf1 == -1.0),
                                           str4);
            if (tf3 != 0.0) // zero oznacza błąd
            {
                fTrainSetDist -=
                    tf3; // przesunięcie dla kolejnego, minus bo idziemy w stronę punktu 1
                tmp->pCenter = tmp->DynamicObject->GetPosition();
/* // NOTE: the ctrain_depot flag is used to mark merged together parts of modular trains
   //       clearing it here breaks this connection, so i'm disabling this piece of code.
   //       if it has some actual purpose and disabling it breaks that, a different solution has to be found
   //       either for modular trains, or whatever it is this code does.
                if (TempConnectionType[iTrainSetWehicleNumber]) // jeśli jest sprzęg
                    if (tmp->DynamicObject->MoverParameters->Couplers[tf1 == -1.0 ? 0 : 1]
                            .AllowedFlag &
                        ctrain_depot) // jesli zablokowany
                        TempConnectionType[iTrainSetWehicleNumber] |= ctrain_depot; // będzie blokada
*/
                iTrainSetWehicleNumber++;
            }
            else
            { // LastNode=NULL;
                delete tmp;
                tmp = NULL; // nie może być tu return, bo trzeba pominąć jeszcze enddynamic
            }
        }
        else
        { // gdy tor nie znaleziony
            ErrorLog("Missed track: dynamic placed on \"" + tmp->DynamicObject->asTrack + "\"");
            delete tmp;
            tmp = NULL; // nie może być tu return, bo trzeba pominąć jeszcze enddynamic
        }
        parser->getTokens();
        *parser >> token;
        if (token.compare("destination") == 0)
        { // dokąd wagon ma jechać, uwzględniane przy manewrach
            parser->getTokens();
            *parser >> token;
            if (tmp)
                tmp->DynamicObject->asDestination = token;
            *parser >> token;
        }
        if (token.compare("enddynamic") != 0)
            Error("enddynamic statement missing");
        break;
    case TP_MODEL:
        if (rmin < 0)
        {
            tmp->iType = TP_TERRAIN;
            tmp->fSquareMinRadius = 0; // to w ogóle potrzebne?
        }
        parser->getTokens(3);
        *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
        parser->getTokens();
        *parser >> tf1;
        // OlO_EU&KAKISH-030103: obracanie punktow zaczepien w modelu
        tmp->pCenter.RotateY(aRotate.y / 180.0 * M_PI);
        // McZapkie-260402: model tez ma wspolrzedne wzgledne
        tmp->pCenter += pOrigin;
        // tmp->fAngle+=aRotate.y; // /180*M_PI
        /*
           if (tmp->iType==TP_MODEL)
           {//jeśli standardowy model
        */
        tmp->Model = new TAnimModel();
        tmp->Model->RaAnglesSet(aRotate.x, tf1 + aRotate.y,
                                aRotate.z); // dostosowanie do pochylania linii
        if (tmp->Model->Load(
                parser, tmp->iType == TP_TERRAIN)) // wczytanie modelu, tekstury i stanu świateł...
            tmp->iFlags =
                tmp->Model->Flags() | 0x200; // ustalenie, czy przezroczysty; flaga usuwania
        else if (tmp->iType != TP_TERRAIN)
        { // model nie wczytał się - ignorowanie node
            delete tmp;
            tmp = NULL; // nie może być tu return
            break; // nie może być tu return?
        }
        /*
           }
           else if (tmp->iType==TP_TERRAIN)
           {//nie potrzeba nakładki animującej submodele
            *parser >> token;
            tmp->pModel3D=TModelsManager::GetModel(token.c_str(),false);
            do //Ra: z tym to trochę bez sensu jest
            {parser->getTokens();
             *parser >> token;
             str=AnsiString(token.c_str());
            } while (str!="endterrains");
           }
        */
        if (tmp->iType == TP_TERRAIN)
        { // jeśli model jest terenem, trzeba utworzyć dodatkowe obiekty
            // po wczytaniu model ma już utworzone DL albo VBO
            Global::pTerrainCompact = tmp->Model; // istnieje co najmniej jeden obiekt terenu
            tmp->iCount = Global::pTerrainCompact->TerrainCount() + 1; // zliczenie submodeli
            tmp->nNode = new TGroundNode[tmp->iCount]; // sztuczne node dla kwadratów
            tmp->nNode[0].iType = TP_MODEL; // pierwszy zawiera model (dla delete)
            tmp->nNode[0].Model = Global::pTerrainCompact;
            tmp->nNode[0].iFlags = 0x200; // nie wyświetlany, ale usuwany
            for (i = 1; i < tmp->iCount; ++i)
            { // a reszta to submodele
                tmp->nNode[i].iType = TP_SUBMODEL; //
                tmp->nNode[i].smTerrain = Global::pTerrainCompact->TerrainSquare(i - 1);
                tmp->nNode[i].iFlags = 0x10; // nieprzezroczyste; nie usuwany
                tmp->nNode[i].bVisible = true;
                tmp->nNode[i].pCenter = tmp->pCenter; // nie przesuwamy w inne miejsce
                // tmp->nNode[i].asName=
            }
        }
        else if (!tmp->asName.empty()) // jest pusta gdy "none"
        { // dodanie do wyszukiwarki
#ifdef EU07_USE_OLD_TNAMES_CLASS
            if (sTracks->Update(TP_MODEL, tmp->asName.c_str(),
                                tmp)) // najpierw sprawdzić, czy już jest
            { // przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność
                // wsteczna)
                ErrorLog("Duplicated model: " + tmp->asName); // to zgłaszać duplikat
            }
            else
                sTracks->Add(TP_MODEL, tmp->asName.c_str(), tmp); // nazwa jest unikalna
#else
            if( false == m_trackmap.Add( TP_MODEL, tmp->asName, tmp ) ) {
                // przy zdublowaniu wskaźnik zostanie podmieniony w drzewku na późniejszy (zgodność wsteczna)
                ErrorLog( "Duplicated model: " + tmp->asName ); // to zgłaszać duplikat
            }
#endif
        }
        // str=Parser->GetNextSymbol().LowerCase();
        break;
    // case TP_GEOMETRY :
    case GL_TRIANGLES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
        parser->getTokens();
        *parser >> token;
        // McZapkie-050702: opcjonalne wczytywanie parametrow materialu (ambient,diffuse,specular)
        if (token.compare("material") == 0)
        {
            parser->getTokens();
            *parser >> token;
            while (token.compare("endmaterial") != 0)
            {
                if (token.compare("ambient:") == 0)
                {
                    parser->getTokens();
                    *parser >> tmp->Ambient[0];
                    parser->getTokens();
                    *parser >> tmp->Ambient[1];
                    parser->getTokens();
                    *parser >> tmp->Ambient[2];
                }
                else if (token.compare("diffuse:") == 0)
                { // Ra: coś jest nie tak, bo w jednej linijce nie działa
                    parser->getTokens();
                    *parser >> tmp->Diffuse[0];
                    parser->getTokens();
                    *parser >> tmp->Diffuse[1];
                    parser->getTokens();
                    *parser >> tmp->Diffuse[2];
                }
                else if (token.compare("specular:") == 0)
                {
                    parser->getTokens();
                    *parser >> tmp->Specular[0];
                    parser->getTokens();
                    *parser >> tmp->Specular[1];
                    parser->getTokens();
                    *parser >> tmp->Specular[2];
                }
                else
                    Error("Scene material failure!");
                parser->getTokens();
                *parser >> token;
            }
        }
        if (token.compare("endmaterial") == 0)
        {
            parser->getTokens();
            *parser >> token;
        }
        str = token;
#ifdef _PROBLEND
        // PROBLEND Q: 13122011 - Szociu: 27012012
        PROBLEND = true; // domyslnie uruchomione nowe wyświetlanie
        tmp->PROBLEND = true; // odwolanie do tgroundnode, bo rendering jest w tej klasie
        if (str.find("@") != string::npos) // sprawdza, czy w nazwie tekstury jest znak "@"
        {
            PROBLEND = false; // jeśli jest, wyswietla po staremu
            tmp->PROBLEND = false;
        }
#endif
        tmp->TextureID = TTexturesManager::GetTextureID(szTexturePath, szSceneryPath, str.c_str());
        tmp->iFlags = TTexturesManager::GetAlpha(tmp->TextureID) ? 0x220 : 0x210; // z usuwaniem
        if (((tmp->iType == GL_TRIANGLES) && (tmp->iFlags & 0x10)) ?
                Global::pTerrainCompact->TerrainLoaded() :
                false)
        { // jeśli jest tekstura nieprzezroczysta, a teren załadowany, to pomijamy trójkąty
            do
            { // pomijanie trójkątów
                parser->getTokens();
                *parser >> token;
            } while (token.compare("endtri") != 0);
            // delete tmp; //nie ma co tego trzymać
            // tmp=NULL; //to jest błąd
        }
        else
        {
/*
            i = 0;
*/
            TempVerts.clear();
            TGroundVertex vertex;
            do
            {
/*
                if (i < 9999) // 3333 trójkąty
                { // liczba wierzchołków nie jest nieograniczona
*/
/*
                    parser->getTokens(3);
                    *parser >> TempVerts[i].Point.x >> TempVerts[i].Point.y >> TempVerts[i].Point.z;
                    parser->getTokens(3);
                    *parser >> TempVerts[i].Normal.x >> TempVerts[i].Normal.y >>
                        TempVerts[i].Normal.z;
*/
                parser->getTokens( 8, false );
                *parser
                    >> vertex.Point.x
                    >> vertex.Point.y
                    >> vertex.Point.z
                    >> vertex.Normal.x
                    >> vertex.Normal.y
                    >> vertex.Normal.z
                    >> vertex.tu
                    >> vertex.tv;
                /*
                         str=Parser->GetNextSymbol().LowerCase();
                         if (str==AnsiString("x"))
                             TempVerts[i].tu=(TempVerts[i].Point.x+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
                         else
                         if (str==AnsiString("y"))
                             TempVerts[i].tu=(TempVerts[i].Point.y+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
                         else
                         if (str==AnsiString("z"))
                             TempVerts[i].tu=(TempVerts[i].Point.z+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
                         else
                             TempVerts[i].tu=str.ToDouble();;

                         str=Parser->GetNextSymbol().LowerCase();
                         if (str==AnsiString("x"))
                             TempVerts[i].tv=(TempVerts[i].Point.x+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
                         else
                         if (str==AnsiString("y"))
                             TempVerts[i].tv=(TempVerts[i].Point.y+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
                         else
                         if (str==AnsiString("z"))
                             TempVerts[i].tv=(TempVerts[i].Point.z+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
                         else
                             TempVerts[i].tv=str.ToDouble();;
                    */
/*
                    parser->getTokens(2);
                    *parser >> TempVerts[i].tu >> TempVerts[i].tv;
*/
                    //    tf=Parser->GetNextSymbol().ToDouble();
                    //          TempVerts[i].tu=tf;
                    //        tf=Parser->GetNextSymbol().ToDouble();
                    //      TempVerts[i].tv=tf;
/*
                    TempVerts[i].Point.RotateZ(aRotate.z / 180 * M_PI);
                    TempVerts[i].Point.RotateX(aRotate.x / 180 * M_PI);
                    TempVerts[i].Point.RotateY(aRotate.y / 180 * M_PI);
                    TempVerts[i].Normal.RotateZ(aRotate.z / 180 * M_PI);
                    TempVerts[i].Normal.RotateX(aRotate.x / 180 * M_PI);
                    TempVerts[i].Normal.RotateY(aRotate.y / 180 * M_PI);
                    TempVerts[i].Point += pOrigin;
                    tmp->pCenter += TempVerts[i].Point;
*/
                vertex.Point.RotateZ( aRotate.z / 180 * M_PI );
                vertex.Point.RotateX( aRotate.x / 180 * M_PI );
                vertex.Point.RotateY( aRotate.y / 180 * M_PI );
                vertex.Normal.RotateZ( aRotate.z / 180 * M_PI );
                vertex.Normal.RotateX( aRotate.x / 180 * M_PI );
                vertex.Normal.RotateY( aRotate.y / 180 * M_PI );
                vertex.Point += pOrigin;
                tmp->pCenter += vertex.Point;

                TempVerts.emplace_back( vertex );
/*
                }
                else if (i == 9999)
                    ErrorLog("Bad triangles: too many verices");
                i++;
*/
                parser->getTokens();
                *parser >> token;

                //   }

            } while (token.compare("endtri") != 0);
/*
            nv = i;
*/
            nv = TempVerts.size();
            tmp->Init(nv); // utworzenie tablicy wierzchołków
            tmp->pCenter /= (nv > 0 ? nv : 1);

            //   memcpy(tmp->Vertices,TempVerts,nv*sizeof(TGroundVertex));

            r = 0;
            for (int i = 0; i < nv; i++)
            {
                tmp->Vertices[i] = TempVerts[i];
                tf = SquareMagnitude(tmp->Vertices[i].Point - tmp->pCenter);
                if (tf > r)
                    r = tf;
            }

            //   tmp->fSquareRadius=2000*2000+r;
            tmp->fSquareRadius += r;
            RaTriangleDivider(tmp); // Ra: dzielenie trójkątów jest teraz całkiem wydajne
        } // koniec wczytywania trójkątów
        break;
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP: {
            parser->getTokens( 3 );
            *parser >> tmp->Diffuse[ 0 ] >> tmp->Diffuse[ 1 ] >> tmp->Diffuse[ 2 ];
            //   tmp->Diffuse[0]=Parser->GetNextSymbol().ToDouble()/255;
            //   tmp->Diffuse[1]=Parser->GetNextSymbol().ToDouble()/255;
            //   tmp->Diffuse[2]=Parser->GetNextSymbol().ToDouble()/255;
            parser->getTokens();
            *parser >> tmp->fLineThickness;
            TempVerts.clear();
            TGroundVertex vertex;
            i = 0;
            parser->getTokens();
            *parser >> token;
            do {
                str = token;
/*
                TempVerts[ i ].Point.x = atof( str.c_str() );
                parser->getTokens( 2 );
                *parser >> TempVerts[ i ].Point.y >> TempVerts[ i ].Point.z;
                TempVerts[ i ].Point.RotateZ( aRotate.z / 180 * M_PI );
                TempVerts[ i ].Point.RotateX( aRotate.x / 180 * M_PI );
                TempVerts[ i ].Point.RotateY( aRotate.y / 180 * M_PI );
                TempVerts[ i ].Point += pOrigin;
                tmp->pCenter += TempVerts[ i ].Point;
*/
                vertex.Point.x = std::atof( str.c_str() );
                parser->getTokens( 2 );
                *parser
                    >> vertex.Point.y
                    >> vertex.Point.z;
                vertex.Point.RotateZ( aRotate.z / 180 * M_PI );
                vertex.Point.RotateX( aRotate.x / 180 * M_PI );
                vertex.Point.RotateY( aRotate.y / 180 * M_PI );
                vertex.Point += pOrigin;
                tmp->pCenter += vertex.Point;
                TempVerts.emplace_back( vertex );

                ++i;
                parser->getTokens();
                *parser >> token;
            } while( token.compare( "endline" ) != 0 );
            nv = i;
            //   tmp->Init(nv);
            tmp->Points = new vector3[ nv ];
            tmp->iNumPts = nv;
            tmp->pCenter /= ( nv > 0 ? nv : 1 );
            for( int i = 0; i < nv; i++ )
                tmp->Points[ i ] = TempVerts[ i ].Point;
            break;
        }
    }
    return tmp;
}

TSubRect * TGround::FastGetSubRect(int iCol, int iRow)
{
    int br, bc, sr, sc;
    br = iRow / iNumSubRects;
    bc = iCol / iNumSubRects;
    sr = iRow - br * iNumSubRects;
    sc = iCol - bc * iNumSubRects;
    if ((br < 0) || (bc < 0) || (br >= iNumRects) || (bc >= iNumRects))
        return NULL;
    return (Rects[br][bc].FastGetRect(sc, sr));
}

TSubRect * TGround::GetSubRect(int iCol, int iRow)
{ // znalezienie małego kwadratu mapy
    int br, bc, sr, sc;
    br = iRow / iNumSubRects; // współrzędne kwadratu kilometrowego
    bc = iCol / iNumSubRects;
    sr = iRow - br * iNumSubRects; // współrzędne wzglęne małego kwadratu
    sc = iCol - bc * iNumSubRects;
    if ((br < 0) || (bc < 0) || (br >= iNumRects) || (bc >= iNumRects))
        return NULL; // jeśli poza mapą
    return (Rects[br][bc].SafeGetRect(sc, sr)); // pobranie małego kwadratu
}

TEvent * TGround::FindEvent(const string &asEventName)
{
#ifdef EU07_USE_OLD_TNAMES_CLASS
    return (TEvent *)sTracks->Find(0, asEventName.c_str()); // wyszukiwanie w drzewie
#else
    auto const lookup = m_eventmap.find( asEventName );
    return
        lookup != m_eventmap.end() ?
        lookup->second :
        nullptr;
#endif
    /* //powolna wyszukiwarka
     for (TEvent *Current=RootEvent;Current;Current=Current->Next2)
     {
      if (Current->asName==asEventName)
       return Current;
     }
     return NULL;
    */
}

TEvent * TGround::FindEventScan(const string &asEventName)
{ // wyszukanie eventu z opcją utworzenia niejawnego dla komórek skanowanych
#ifdef EU07_USE_OLD_TNAMES_CLASS
    TEvent *e = (TEvent *)sTracks->Find( 0, asEventName.c_str() ); // wyszukiwanie w drzewie eventów
#else
    auto const lookup = m_eventmap.find( asEventName );
    auto e =
        lookup != m_eventmap.end() ?
        lookup->second :
        nullptr;
#endif
    if (e)
        return e; // jak istnieje, to w porządku
    if (asEventName.rfind(":scan") != std::string::npos) // jeszcze może być event niejawny
    { // no to szukamy komórki pamięci o nazwie zawartej w evencie
        string n = asEventName.substr(0, asEventName.length() - 5); // do dwukropka
#ifdef EU07_USE_OLD_TNAMES_CLASS
        if( sTracks->Find( TP_MEMCELL, n.c_str() ) ) // jeśli jest takowa komórka pamięci
#else
        if( m_trackmap.Find( TP_MEMCELL, n ) != nullptr ) // jeśli jest takowa komórka pamięci
#endif
            e = new TEvent(n); // utworzenie niejawnego eventu jej odczytu
    }
    return e; // utworzony albo się nie udało
}

void TGround::FirstInit()
{ // ustalanie zależności na scenerii przed wczytaniem pojazdów
    if (bInitDone)
        return; // Ra: żeby nie robiło się dwa razy
    bInitDone = true;
    WriteLog("InitNormals");
    int i, j;
    for (i = 0; i < TP_LAST; ++i)
    {
        for (TGroundNode *Current = nRootOfType[i]; Current; Current = Current->nNext)
        {
            Current->InitNormals();
            if (Current->iType != TP_DYNAMIC)
            { // pojazdów w ogóle nie dotyczy dodawanie do mapy
                if (i == TP_EVLAUNCH ? Current->EvLaunch->IsGlobal() : false)
                    srGlobal.NodeAdd(Current); // dodanie do globalnego obiektu
                else if (i == TP_TERRAIN)
                { // specjalne przetwarzanie terenu wczytanego z pliku E3D
                    TGroundRect *gr;
                    for (j = 1; j < Current->iCount; ++j)
                    { // od 1 do końca są zestawy trójkątów
                        std::string xxxzzz = Current->nNode[j].smTerrain->pName; // pobranie nazwy
                        gr = GetRect(
                            ( std::stoi( xxxzzz.substr( 0, 3 )) - 500 ) * 1000,
                            ( std::stoi( xxxzzz.substr( 3, 3 )) - 500 ) * 1000 );
                        if (Global::bUseVBO)
                            gr->nTerrain = Current->nNode + j; // zapamiętanie
                        else
                            gr->RaNodeAdd(&Current->nNode[j]);
                    }
                }
                //    else if
                //    ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true
                //    //~czy trójkąt?
                else if ((Current->iType != GL_TRIANGLES) ?
                             true //~czy trójkąt?
                             :
                             (Current->iFlags & 0x20) ?
                             true //~czy teksturę ma nieprzezroczystą?
                             :
                             (Current->fSquareMinRadius != 0.0) ?
                             true //~czy widoczny z bliska?
                             :
                             (Current->fSquareRadius <= 90000.0)) //~czy widoczny z daleka?
                    GetSubRect(Current->pCenter.x, Current->pCenter.z)->NodeAdd(Current);
                else // dodajemy do kwadratu kilometrowego
                    GetRect(Current->pCenter.x, Current->pCenter.z)->NodeAdd(Current);
            }
            // if (Current->iType!=TP_DYNAMIC)
            // GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
        }
    }
    for (i = 0; i < iNumRects; ++i)
        for (j = 0; j < iNumRects; ++j)
            Rects[i][j].Optimize(); // optymalizacja obiektów w sektorach
    WriteLog("InitNormals OK");
    WriteLog("InitTracks");
    InitTracks(); //łączenie odcinków ze sobą i przyklejanie eventów
    WriteLog("InitTracks OK");
    WriteLog("InitTraction");
    InitTraction(); //łączenie drutów ze sobą
    WriteLog("InitTraction OK");
    WriteLog("InitEvents");
    InitEvents();
    WriteLog("InitEvents OK");
    WriteLog("InitLaunchers");
    InitLaunchers();
    WriteLog("InitLaunchers OK");
    WriteLog("InitGlobalTime");
    // ABu 160205: juz nie TODO :)
    Mtable::GlobalTime = std::make_shared<TMTableTime>( hh, mm, srh, srm, ssh, ssm ); // McZapkie-300302: inicjacja czasu rozkladowego - TODO: czytac z trasy!
    WriteLog("InitGlobalTime OK");
    // jeszcze ustawienie pogody, gdyby nie było w scenerii wpisów
    glClearColor(Global::AtmoColor[0], Global::AtmoColor[1], Global::AtmoColor[2],
                 0.0); // Background Color
    if (Global::fFogEnd > 0)
    {
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, Global::FogColor); // set fog color
        glFogf(GL_FOG_START, Global::fFogStart); // fog start depth
        glFogf(GL_FOG_END, Global::fFogEnd); // fog end depth
        glEnable(GL_FOG);
    }
    else
        glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, Global::lightPos); // daylight position
    glLightfv(GL_LIGHT0, GL_AMBIENT, Global::ambientDayLight); // kolor wszechobceny
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Global::diffuseDayLight); // kolor padający
    glLightfv(GL_LIGHT0, GL_SPECULAR, Global::specularDayLight); // kolor odbity
    // musi być tutaj, bo wcześniej nie mieliśmy wartości światła
/*
    if (Global::fMoveLight >= 0.0) // albo tak, albo niech ustala minimum ciemności w nocy
    {
*/
        Global::fLuminance = // obliczenie luminacji "światła w ciemności"
            +0.150 * Global::ambientDayLight[0] // R
            + 0.295 * Global::ambientDayLight[1] // G
            + 0.055 * Global::ambientDayLight[2]; // B
        if (Global::fLuminance > 0.1) // jeśli miało by być za jasno
            for (int i = 0; i < 3; i++)
                Global::ambientDayLight[i] *=
                    0.1 / Global::fLuminance; // ograniczenie jasności w nocy
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Global::ambientDayLight);
/*
    }
    else if (Global::bDoubleAmbient) // Ra: wcześniej było ambient dawane na obydwa światła
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Global::ambientDayLight);
*/
    glEnable(GL_LIGHTING);
    WriteLog("FirstInit is done");
};

bool TGround::Init(std::string asFile)
{ // główne wczytywanie scenerii
    if (ToLower(asFile).substr(0, 7) == "scenery")
        asFile = asFile.erase(0, 8); // Ra: usunięcie niepotrzebnych znaków - zgodność wstecz z 2003
    WriteLog("Loading scenery from " + asFile);
    Global::pGround = this;
    // pTrain=NULL;
    pOrigin = aRotate = vector3(0, 0, 0); // zerowanie przesunięcia i obrotu
    string str = "";
    // TFileStream *fs;
    // int size;
    std::string subpath = Global::asCurrentSceneryPath; //   "scenery/";
    cParser parser(asFile, cParser::buffer_FILE, subpath, Global::bLoadTraction);
    std::string token;

    /*
        TFileStream *fs;
        fs=new TFileStream(asFile , fmOpenRead	| fmShareCompat	);
        AnsiString str="";
        int size=fs->Size;
        str.SetLength(size);
        fs->Read(str.c_str(),size);
        str+="";
        delete fs;
        TQueryParserComp *Parser;
        Parser=new TQueryParserComp(NULL);
        Parser->TextToParse=str;
    //    Parser->LoadStringToParse(asFile);
        Parser->First();
        AnsiString Token,asFileName;
    */
    const int OriginStackMaxDepth = 100; // rozmiar stosu dla zagnieżdżenia origin
    int OriginStackTop = 0;
    vector3 OriginStack[OriginStackMaxDepth]; // stos zagnieżdżenia origin

    double tf;
    int ParamCount, ParamPos;

    // ABu: Jezeli nie ma definicji w scenerii to ustawiane ponizsze wartosci:
    hh = 10; // godzina startu
    mm = 30; // minuty startu
    srh = 6; // godzina wschodu slonca
    srm = 0; // minuty wschodu slonca
    ssh = 20; // godzina zachodu slonca
    ssm = 0; // minuty zachodu slonca
    TGroundNode *LastNode = NULL; // do użycia w trainset
    iNumNodes = 0;
    token = "";
    parser.getTokens();
    parser >> token;
    int refresh = 0;

    while (token != "") //(!Parser->EndOfFile)
    {
        if (refresh == 50)
        { // SwapBuffers(hDC); //Ra: bez ogranicznika za bardzo spowalnia :( a u niektórych miga
            refresh = 0;
            Global::DoEvents();
        }
        else
            ++refresh;
        str = token;
        if (str == "node")
        {
            LastNode = AddGroundNode(&parser); // rozpoznanie węzła
            if (LastNode)
            { // jeżeli przetworzony poprawnie
                if (LastNode->iType == GL_TRIANGLES)
                {
                    if (!LastNode->Vertices)
                        SafeDelete(LastNode); // usuwamy nieprzezroczyste trójkąty terenu
                }
                else if (Global::bLoadTraction ? false : LastNode->iType == TP_TRACTION)
                    SafeDelete(LastNode); // usuwamy druty, jeśli wyłączone
                if (LastNode) // dopiero na koniec dopisujemy do tablic
                    if (LastNode->iType != TP_DYNAMIC)
                    { // jeśli nie jest pojazdem
                        LastNode->nNext = nRootOfType[LastNode->iType]; // ostatni dodany dołączamy
                        // na końcu nowego
                        nRootOfType[LastNode->iType] =
                            LastNode; // ustawienie nowego na początku listy
                        iNumNodes++;
                    }
                    else
                    { // jeśli jest pojazdem
                        // if (!bInitDone) FirstInit(); //jeśli nie było w scenerii
                        if (LastNode->DynamicObject->Mechanik) // ale może być pasażer
                            if (LastNode->DynamicObject->Mechanik
                                    ->Primary()) // jeśli jest głównym (pasażer nie jest)
                                nTrainSetDriver =
                                    LastNode; // pojazd, któremu zostanie wysłany rozkład
                        LastNode->nNext = nRootDynamic;
                        nRootDynamic = LastNode; // dopisanie z przodu do listy
                        // if (bTrainSet && (LastNode?(LastNode->iType==TP_DYNAMIC):false))
                        if (nTrainSetNode) // jeżeli istnieje wcześniejszy TP_DYNAMIC
                            nTrainSetNode->DynamicObject->AttachPrev(
                                LastNode->DynamicObject,
                                TempConnectionType[iTrainSetWehicleNumber - 2]);
                        nTrainSetNode = LastNode; // ostatnio wczytany
                        if (TempConnectionType[iTrainSetWehicleNumber - 1] ==
                            0) // jeśli sprzęg jest zerowy, to wysłać rozkład do składu
                        { // powinien też tu wchodzić, gdy pojazd bez trainset
                            if (nTrainSetDriver) // pojazd, któremu zostanie wysłany rozkład
                            { // wysłanie komendy "Timetable" ustawia odpowiedni tryb jazdy
                                nTrainSetDriver->DynamicObject->Mechanik->DirectionInitial();
                                nTrainSetDriver->DynamicObject->Mechanik->PutCommand(
                                    "Timetable:" + asTrainName, fTrainSetVel, 0, NULL);
                                nTrainSetDriver =
                                    NULL; // a przy "endtrainset" już wtedy nie potrzeba
                            }
                        }
                    }
            }
            else
            {
                Error("Scene parse error near " + token);
                // break;
            }
        }
        else if (str == "trainset")
        {
            iTrainSetWehicleNumber = 0;
            nTrainSetNode = NULL;
            nTrainSetDriver = NULL; // pojazd, któremu zostanie wysłany rozkład
            bTrainSet = true;
            parser.getTokens();
            parser >> token;
            asTrainName = token; // McZapkie: rodzaj+nazwa pociagu w SRJP
            parser.getTokens();
            parser >> token;
            asTrainSetTrack = token; //ścieżka startowa
            parser.getTokens(2);
            parser >> fTrainSetDist >> fTrainSetVel; // przesunięcie i prędkość
        }
        else if (str == "endtrainset")
        { // McZapkie-110103: sygnaly konca pociagu ale tylko dla pociagow rozkladowych
            if (nTrainSetNode) // trainset bez dynamic się sypał
            { // powinien też tu wchodzić, gdy pojazd bez trainset
                if (nTrainSetDriver) // pojazd, któremu zostanie wysłany rozkład
                { // wysłanie komendy "Timetable" ustawia odpowiedni tryb jazdy
                    nTrainSetDriver->DynamicObject->Mechanik->DirectionInitial();
                    nTrainSetDriver->DynamicObject->Mechanik->PutCommand("Timetable:" + asTrainName,
                                                                         fTrainSetVel, 0, NULL);
                }
            }
            if (LastNode) // ostatni wczytany obiekt
                if (LastNode->iType ==
                    TP_DYNAMIC) // o ile jest pojazdem (na ogół jest, ale kto wie...)
                    if (iTrainSetWehicleNumber ? !TempConnectionType[iTrainSetWehicleNumber - 1] :
                                                 false) // jeśli ostatni pojazd ma sprzęg 0
                        LastNode->DynamicObject->RaLightsSet(-1, 2 + 32 + 64); // to założymy mu
            // końcówki blaszane
            // (jak AI się
            // odpali, to sobie
            // poprawi)
            bTrainSet = false;
            fTrainSetVel = 0;
            // iTrainSetConnection=0;
            nTrainSetNode = nTrainSetDriver = NULL;
            iTrainSetWehicleNumber = 0;
        }
        else if (str == "event")
        {
            TEvent *tmp = new TEvent();
            tmp->Load(&parser, &pOrigin);
            if (tmp->Type == tp_Unknown)
                delete tmp;
            else
            { // najpierw sprawdzamy, czy nie ma, a potem dopisujemy
                TEvent *found = FindEvent(tmp->asName);
                if (found)
                { // jeśli znaleziony duplikat
                    auto const size = tmp->asName.size();
                    if( tmp->asName[0] == '#' ) // zawsze jeden znak co najmniej jest
                    {
                        delete tmp;
                        tmp = nullptr;
                    } // utylizacja duplikatu z krzyżykiem
                    else if( ( size > 8 )
                          && ( tmp->asName.substr( 0, 9 ) == "lineinfo:" ))
                        // tymczasowo wyjątki
                    {
                        delete tmp;
                        tmp = nullptr;
                    } // tymczasowa utylizacja duplikatów W5
                    else if( ( size > 8 )
                          && ( tmp->asName.substr( size - 8 ) == "_warning"))
                        // tymczasowo wyjątki
                    {
                        delete tmp;
                        tmp = nullptr;
                    } // tymczasowa utylizacja duplikatu z trąbieniem
                    else if( ( size > 4 )
                          && ( tmp->asName.substr( size - 4 ) == "_shp" ))
                          // nie podlegają logowaniu
                    {
                        delete tmp;
                        tmp = NULL;
                    } // tymczasowa utylizacja duplikatu SHP
                    if (tmp) // jeśli nie został zutylizowany
                        if (Global::bJoinEvents)
                            found->Append(tmp); // doczepka (taki wirtualny multiple bez warunków)
                        else
                        {
                            ErrorLog("Duplicated event: " + tmp->asName);
                            found->Append(tmp); // doczepka (taki wirtualny multiple bez warunków)
                            found->Type = tp_Ignored; // dezaktywacja pierwotnego - taka proteza na
                            // wsteczną zgodność
                            // SafeDelete(tmp); //bezlitośnie usuwamy wszelkie duplikaty, żeby nie
                            // zaśmiecać drzewka
                        }
                }
                if ( nullptr != tmp )
                { // jeśli nie duplikat
                    tmp->evNext2 = RootEvent; // lista wszystkich eventów (m.in. do InitEvents)
                    RootEvent = tmp;
                    if (!found)
                    { // jeśli nazwa wystąpiła, to do kolejki i wyszukiwarki dodawany jest tylko
                        // pierwszy
                        if (RootEvent->Type != tp_Ignored)
                            if (RootEvent->asName.find(
                                    "onstart") != string::npos) // event uruchamiany automatycznie po starcie
                                AddToQuery(RootEvent, NULL); // dodanie do kolejki
#ifdef EU07_USE_OLD_TNAMES_CLASS
                        sTracks->Add(0, tmp->asName.c_str(), tmp); // dodanie do wyszukiwarki
#else
                        m_eventmap.emplace( tmp->asName, tmp ); // dodanie do wyszukiwarki
#endif
                    }
                }
            }
        }
        //     else
        //     if (str==AnsiString("include"))  //Tolaris to zrobil wewnatrz parsera
        //     {
        //         Include(Parser);
        //     }
        else if (str == "rotate")
        {
            // parser.getTokens(3);
            // parser >> aRotate.x >> aRotate.y >> aRotate.z; //Ra: to potrafi dawać błędne
            // rezultaty
            parser.getTokens();
            parser >> aRotate.x;
            parser.getTokens();
            parser >> aRotate.y;
            parser.getTokens();
            parser >> aRotate.z;
            // WriteLog("*** rotate "+AnsiString(aRotate.x)+" "+AnsiString(aRotate.y)+"
            // "+AnsiString(aRotate.z));
        }
        else if (str == "origin")
        {
            //      str=Parser->GetNextSymbol().LowerCase();
            //      if (str=="begin")
            {
                if (OriginStackTop >= OriginStackMaxDepth - 1)
                {
                    MessageBox(0, "Origin stack overflow ", "Error", MB_OK);
                    break;
                }
                parser.getTokens(3);
                parser >> OriginStack[OriginStackTop].x >> OriginStack[OriginStackTop].y >>
                    OriginStack[OriginStackTop].z;
                pOrigin += OriginStack[OriginStackTop]; // sumowanie całkowitego przesunięcia
                OriginStackTop++; // zwiększenie wskaźnika stosu
            }
        }
        else if (str == "endorigin")
        {
            //      else
            //    if (str=="end")
            {
                if (OriginStackTop <= 0)
                {
                    MessageBox(0, "Origin stack underflow ", "Error", MB_OK);
                    break;
                }

                OriginStackTop--; // zmniejszenie wskaźnika stosu
                pOrigin -= OriginStack[OriginStackTop];
            }
        }
        else if (str == "atmo") // TODO: uporzadkowac gdzie maja byc parametry mgly!
        { // Ra: ustawienie parametrów OpenGL przeniesione do FirstInit
            WriteLog("Scenery atmo definition");
            parser.getTokens(3);
            parser >> Global::AtmoColor[0] >> Global::AtmoColor[1] >> Global::AtmoColor[2];
            parser.getTokens(2);
            parser >> Global::fFogStart >> Global::fFogEnd;
            if (Global::fFogEnd > 0.0)
            { // ostatnie 3 parametry są opcjonalne
                parser.getTokens(3);
                parser >> Global::FogColor[0] >> Global::FogColor[1] >> Global::FogColor[2];
            }
            parser.getTokens();
            parser >> token;
            while (token.compare("endatmo") != 0)
            { // a kolejne parametry są pomijane
                parser.getTokens();
                parser >> token;
            }
        }
        else if (str == "time")
        {
            WriteLog("Scenery time definition");
            char temp_in[9];
            char temp_out[9];
            int i, j;
            parser.getTokens();
            parser >> temp_in;
            for (j = 0; j <= 8; j++)
                temp_out[j] = ' ';
            for (i = 0; temp_in[i] != ':'; i++)
                temp_out[i] = temp_in[i];
            hh = atoi(temp_out);
            for (j = 0; j <= 8; j++)
                temp_out[j] = ' ';
            for (j = i + 1; j <= 8; j++)
                temp_out[j - (i + 1)] = temp_in[j];
            mm = atoi(temp_out);

            parser.getTokens();
            parser >> temp_in;
            for (j = 0; j <= 8; j++)
                temp_out[j] = ' ';
            for (i = 0; temp_in[i] != ':'; i++)
                temp_out[i] = temp_in[i];
            srh = atoi(temp_out);
            for (j = 0; j <= 8; j++)
                temp_out[j] = ' ';
            for (j = i + 1; j <= 8; j++)
                temp_out[j - (i + 1)] = temp_in[j];
            srm = atoi(temp_out);

            parser.getTokens();
            parser >> temp_in;
            for (j = 0; j <= 8; j++)
                temp_out[j] = ' ';
            for (i = 0; temp_in[i] != ':'; i++)
                temp_out[i] = temp_in[i];
            ssh = atoi(temp_out);
            for (j = 0; j <= 8; j++)
                temp_out[j] = ' ';
            for (j = i + 1; j <= 8; j++)
                temp_out[j - (i + 1)] = temp_in[j];
            ssm = atoi(temp_out);
            while (token.compare("endtime") != 0)
            {
                parser.getTokens();
                parser >> token;
            }
        }
        else if (str == "light")
        { // Ra: ustawianie światła przeniesione do FirstInit
            WriteLog("Scenery light definition");
            vector3 lp;
            parser.getTokens();
            parser >> lp.x;
            parser.getTokens();
            parser >> lp.y;
            parser.getTokens();
            parser >> lp.z;
            lp = Normalize(lp); // kierunek padania
            Global::lightPos[0] = lp.x; // daylight position
            Global::lightPos[1] = lp.y;
            Global::lightPos[2] = lp.z;
            parser.getTokens();
            parser >> Global::ambientDayLight[0]; // kolor wszechobceny
            parser.getTokens();
            parser >> Global::ambientDayLight[1];
            parser.getTokens();
            parser >> Global::ambientDayLight[2];

            parser.getTokens();
            parser >> Global::diffuseDayLight[0]; // kolor padający
            parser.getTokens();
            parser >> Global::diffuseDayLight[1];
            parser.getTokens();
            parser >> Global::diffuseDayLight[2];

            parser.getTokens();
            parser >> Global::specularDayLight[0]; // kolor odbity
            parser.getTokens();
            parser >> Global::specularDayLight[1];
            parser.getTokens();
            parser >> Global::specularDayLight[2];

            do
            {
                parser.getTokens();
                parser >> token;
            } while (token.compare("endlight") != 0);
        }
        else if (str == "camera")
        {
            vector3 xyz, abc;
            xyz = abc = vector3(0, 0, 0); // wartości domyślne, bo nie wszystie muszą być
            int i = -1, into = -1; // do której definicji kamery wstawić
            WriteLog("Scenery camera definition");
            do
            { // opcjonalna siódma liczba określa numer kamery, a kiedyś były tylko 3
                parser.getTokens();
                parser >> token;
                switch (++i)
                { // kiedyś camera miało tylko 3 współrzędne
                case 0:
                    xyz.x = atof(token.c_str());
                    break;
                case 1:
                    xyz.y = atof(token.c_str());
                    break;
                case 2:
                    xyz.z = atof(token.c_str());
                    break;
                case 3:
                    abc.x = atof(token.c_str());
                    break;
                case 4:
                    abc.y = atof(token.c_str());
                    break;
                case 5:
                    abc.z = atof(token.c_str());
                    break;
                case 6:
                    into = atoi(token.c_str()); // takie sobie, bo można wpisać -1
                }
            } while (token.compare("endcamera") != 0);
            if (into < 0)
                into = ++Global::iCameraLast;
            if ((into >= 0) && (into < 10))
            { // przepisanie do odpowiedniego miejsca w tabelce
                Global::pFreeCameraInit[into] = xyz;
                abc.x = DegToRad(abc.x);
                abc.y = DegToRad(abc.y);
                abc.z = DegToRad(abc.z);
                Global::pFreeCameraInitAngle[into] = abc;
                Global::iCameraLast = into; // numer ostatniej
            }
        }
        else if (str == "sky")
        { // youBy - niebo z pliku
            WriteLog("Scenery sky definition");
            parser.getTokens();
            parser >> token;
            string SkyTemp = token;
            if (Global::asSky == "1")
                Global::asSky = SkyTemp;
            do
            { // pożarcie dodatkowych parametrów
                parser.getTokens();
                parser >> token;
            } while (token.compare("endsky") != 0);
            WriteLog(Global::asSky.c_str());
        }
        else if (str == "firstinit")
            FirstInit();
        else if (str == "description")
        {
            do
            {
                parser.getTokens();
                parser >> token;
            } while (token.compare("enddescription") != 0);
        }
        else if (str == "test")
        { // wypisywanie treści po przetworzeniu
            WriteLog("---> Parser test:");
            do
            {
                parser.getTokens();
                parser >> token;
                WriteLog(token.c_str());
            } while (token.compare("endtest") != 0);
            WriteLog("---> End of parser test.");
        }
        else if (str == "config")
        { // możliwość przedefiniowania parametrów w scenerii
            Global::ConfigParse(parser); // parsowanie dodatkowych ustawień
        }
        else if (str != "")
        { // pomijanie od nierozpoznanej komendy do jej zakończenia
            if ((token.length() > 2) && (atof(token.c_str()) == 0.0))
            { // jeśli nie liczba, to spróbować pominąć komendę
                WriteLog("Unrecognized command: " + str);
                str = "end" + str;
                do
                {
                    parser.getTokens();
                    token = "";
                    parser >> token;
                } while ((token != "") && (token.compare(str.c_str()) != 0));
            }
            else // jak liczba to na pewno błąd
                Error("Unrecognized command: " + str);
        }
/*        else if (str == "")
            break;
*/
        // LastNode=NULL;

        token = "";
        parser.getTokens();
        parser >> token;
    }

#ifdef EU07_USE_OLD_TNAMES_CLASS
    sTracks->Sort(TP_TRACK); // finalne sortowanie drzewa torów
    sTracks->Sort(TP_MEMCELL); // finalne sortowanie drzewa komórek pamięci
    sTracks->Sort(TP_MODEL); // finalne sortowanie drzewa modeli
    sTracks->Sort(0); // finalne sortowanie drzewa eventów
#endif
    if (!bInitDone)
        FirstInit(); // jeśli nie było w scenerii
    if (Global::pTerrainCompact)
        TerrainWrite(); // Ra: teraz można zapisać teren w jednym pliku
    Global::iPause &= ~0x10; // koniec pauzy wczytywania
    return true;
}

bool TGround::InitEvents()
{ //łączenie eventów z pozostałymi obiektami
    TGroundNode *tmp, *trk;
    char buff[255];
    int i;
    for (TEvent *Current = RootEvent; Current; Current = Current->evNext2)
    {
        switch (Current->Type)
        {
        case tp_AddValues: // sumowanie wartości
        case tp_UpdateValues: // zmiana wartości
            tmp = FindGroundNode(Current->asNodeName,
                                 TP_MEMCELL); // nazwa komórki powiązanej z eventem
            if (tmp)
            { // McZapkie-100302
                if (Current->iFlags & (conditional_trackoccupied | conditional_trackfree))
                { // jeśli chodzi o zajetosc toru (tor może być inny, niż wpisany w komórce)
                    trk = FindGroundNode(Current->asNodeName,
                                         TP_TRACK); // nazwa toru ta sama, co nazwa komórki
                    if (trk)
                        Current->Params[9].asTrack = trk->pTrack;
                    if (!Current->Params[9].asTrack)
                        ErrorLog("Bad event: track \"" + Current->asNodeName +
                                 "\" does not exists in \"" + Current->asName + "\"");
                }
                Current->Params[4].nGroundNode = tmp;
                Current->Params[5].asMemCell = tmp->MemCell; // komórka do aktualizacji
                if (Current->iFlags & (conditional_memcompare))
                    Current->Params[9].asMemCell = tmp->MemCell; // komórka do badania warunku
                if (!tmp->MemCell->asTrackName
                         .empty()) // tor powiązany z komórką powiązaną z eventem
                { // tu potrzebujemy wskaźnik do komórki w (tmp)
                    trk = FindGroundNode(tmp->MemCell->asTrackName, TP_TRACK);
                    if (trk)
                        Current->Params[6].asTrack = trk->pTrack;
                    else
                        ErrorLog("Bad memcell: track \"" + tmp->MemCell->asTrackName +
                                 "\" not exists in memcell \"" + tmp->asName + "\"");
                }
                else
                    Current->Params[6].asTrack = NULL;
            }
            else
            { // nie ma komórki, to nie będzie działał poprawnie
                Current->Type = tp_Ignored; // deaktywacja
                ErrorLog("Bad event: \"" + Current->asName + "\" cannot find memcell \"" +
                         Current->asNodeName + "\"");
            }
            break;
        case tp_LogValues: // skojarzenie z memcell
            if (Current->asNodeName.empty())
            { // brak skojarzenia daje logowanie wszystkich
                Current->Params[9].asMemCell = NULL;
                break;
            }
        case tp_GetValues:
        case tp_WhoIs:
            tmp = FindGroundNode(Current->asNodeName, TP_MEMCELL);
            if (tmp)
            {
                Current->Params[8].nGroundNode = tmp;
                Current->Params[9].asMemCell = tmp->MemCell;
                if (Current->Type == tp_GetValues) // jeśli odczyt komórki
                    if (tmp->MemCell->IsVelocity()) // a komórka zawiera komendę SetVelocity albo
                        // ShuntVelocity
                        Current->bEnabled = false; // to event nie będzie dodawany do kolejki
            }
            else
            { // nie ma komórki, to nie będzie działał poprawnie
                Current->Type = tp_Ignored; // deaktywacja
                ErrorLog("Bad event: \"" + Current->asName + "\" cannot find memcell \"" +
                         Current->asNodeName + "\"");
            }
            break;
        case tp_CopyValues: // skopiowanie komórki do innej
            tmp = FindGroundNode(Current->asNodeName, TP_MEMCELL); // komórka docelowa
            if (tmp)
            {
                Current->Params[4].nGroundNode = tmp;
                Current->Params[5].asMemCell = tmp->MemCell; // komórka docelowa
                if (!tmp->MemCell->asTrackName
                         .empty()) // tor powiązany z komórką powiązaną z eventem
                { // tu potrzebujemy wskaźnik do komórki w (tmp)
                    trk = FindGroundNode(tmp->MemCell->asTrackName, TP_TRACK);
                    if (trk)
                        Current->Params[6].asTrack = trk->pTrack;
                    else
                        ErrorLog("Bad memcell: track \"" + tmp->MemCell->asTrackName +
                                 "\" not exists in memcell \"" + tmp->asName + "\"");
                }
                else
                    Current->Params[6].asTrack = NULL;
            }
            else
                ErrorLog("Bad copyvalues: event \"" + Current->asName +
                         "\" cannot find memcell \"" + Current->asNodeName + "\"");
            strcpy(
                buff,
                Current->Params[9].asText); // skopiowanie nazwy drugiej komórki do bufora roboczego
            SafeDeleteArray(Current->Params[9].asText); // usunięcie nazwy komórki
            tmp = FindGroundNode(buff, TP_MEMCELL); // komórka źódłowa
            if (tmp)
            {
                Current->Params[8].nGroundNode = tmp;
                Current->Params[9].asMemCell = tmp->MemCell; // komórka źródłowa
            }
            else
                ErrorLog("Bad copyvalues: event \"" + Current->asName +
                         "\" cannot find memcell \"" + buff + "\"");
            break;
        case tp_Animation: // animacja modelu
            tmp = FindGroundNode(Current->asNodeName, TP_MODEL); // egzemplarza modelu do animowania
            if (tmp)
            {
                strcpy(
                    buff,
                    Current->Params[9].asText); // skopiowanie nazwy submodelu do bufora roboczego
                SafeDeleteArray(Current->Params[9].asText); // usunięcie nazwy submodelu
                if (Current->Params[0].asInt == 4)
                    Current->Params[9].asModel = tmp->Model; // model dla całomodelowych animacji
                else
                { // standardowo przypisanie submodelu
                    Current->Params[9].asAnimContainer = tmp->Model->GetContainer(buff); // submodel
                    if (Current->Params[9].asAnimContainer)
                    {
                        Current->Params[9].asAnimContainer->WillBeAnimated(); // oflagowanie
                        // animacji
                        if (!Current->Params[9]
                                 .asAnimContainer->Event()) // nie szukać, gdy znaleziony
                            Current->Params[9].asAnimContainer->EventAssign(
                                FindEvent(Current->asNodeName + "." + buff + ":done"));
                    }
                }
            }
            else
                ErrorLog("Bad animation: event \"" + Current->asName + "\" cannot find model \"" +
                         Current->asNodeName + "\"");
            Current->asNodeName = "";
            break;
        case tp_Lights: // zmiana świeteł modelu
            tmp = FindGroundNode(Current->asNodeName, TP_MODEL);
            if (tmp)
                Current->Params[9].asModel = tmp->Model;
            else
                ErrorLog("Bad lights: event \"" + Current->asName + "\" cannot find model \"" +
                         Current->asNodeName + "\"");
            Current->asNodeName = "";
            break;
        case tp_Visible: // ukrycie albo przywrócenie obiektu
            tmp = FindGroundNode(Current->asNodeName, TP_MODEL); // najpierw model
            if (!tmp)
                tmp = FindGroundNode(Current->asNodeName, TP_TRACTION); // może druty?
            if (!tmp)
                tmp = FindGroundNode(Current->asNodeName, TP_TRACK); // albo tory?
            if (tmp)
                Current->Params[9].nGroundNode = tmp;
            else
                ErrorLog("Bad visibility: event \"" + Current->asName + "\" cannot find model \"" +
                         Current->asNodeName + "\"");
            Current->asNodeName = "";
            break;
        case tp_Switch: // przełożenie zwrotnicy albo zmiana stanu obrotnicy
            tmp = FindGroundNode(Current->asNodeName, TP_TRACK);
            if (tmp)
            { // dowiązanie toru
                if (!tmp->pTrack->iAction) // jeśli nie jest zwrotnicą ani obrotnicą
                    tmp->pTrack->iAction |= 0x100; // to będzie się zmieniał stan uszkodzenia
                Current->Params[9].asTrack = tmp->pTrack;
                if (!Current->Params[0].asInt) // jeśli przełącza do stanu 0
                    if (Current->Params[2].asdouble >=
                        0.0) // jeśli jest zdefiniowany dodatkowy ruch iglic
                        Current->Params[9].asTrack->Switch(
                            0, Current->Params[1].asdouble,
                            Current->Params[2].asdouble); // przesłanie parametrów
            }
            else
                ErrorLog("Bad switch: event \"" + Current->asName + "\" cannot find track \"" +
                         Current->asNodeName + "\"");
            Current->asNodeName = "";
            break;
        case tp_Sound: // odtworzenie dźwięku
            tmp = FindGroundNode(Current->asNodeName, TP_SOUND);
            if (tmp)
                Current->Params[9].tsTextSound = tmp->tsStaticSound;
            else
                ErrorLog("Bad sound: event \"" + Current->asName +
                         "\" cannot find static sound \"" + Current->asNodeName + "\"");
            Current->asNodeName = "";
            break;
        case tp_TrackVel: // ustawienie prędkości na torze
            if (!Current->asNodeName.empty())
            {
                tmp = FindGroundNode(Current->asNodeName, TP_TRACK);
                if (tmp)
                {
                    tmp->pTrack->iAction |=
                        0x200; // flaga zmiany prędkości toru jest istotna dla skanowania
                    Current->Params[9].asTrack = tmp->pTrack;
                }
                else
                    ErrorLog("Bad velocity: event \"" + Current->asName +
                             "\" cannot find track \"" + Current->asNodeName + "\"");
            }
            Current->asNodeName = "";
            break;
        case tp_DynVel: // komunikacja z pojazdem o konkretnej nazwie
            if (Current->asNodeName == "activator")
                Current->Params[9].asDynamic = NULL;
            else
            {
                tmp = FindGroundNode(Current->asNodeName, TP_DYNAMIC);
                if (tmp)
                    Current->Params[9].asDynamic = tmp->DynamicObject;
                else
                    Error("Event \"" + Current->asName + "\" cannot find dynamic \"" +
                          Current->asNodeName + "\"");
            }
            Current->asNodeName = "";
            break;
        case tp_Multiple:
            if (Current->Params[9].asText != NULL)
            { // przepisanie nazwy do bufora
                strcpy(buff, Current->Params[9].asText);
                SafeDeleteArray(Current->Params[9].asText);
                Current->Params[9].asPointer = NULL; // zerowanie wskaźnika, aby wykryć brak obeiktu
            }
            else
                buff[0] = '\0';
            if (Current->iFlags & (conditional_trackoccupied | conditional_trackfree))
            { // jeśli chodzi o zajetosc toru
                tmp = FindGroundNode(buff, TP_TRACK);
                if (tmp)
                    Current->Params[9].asTrack = tmp->pTrack;
                if (!Current->Params[9].asTrack)
                {
                    ErrorLog("Bad event: Track \"" + string(buff) +
                             "\" does not exist in \"" + Current->asName + "\"");
                    Current->iFlags &=
                        ~(conditional_trackoccupied | conditional_trackfree); // zerowanie flag
                }
            }
            else if (Current->iFlags &
                     (conditional_memstring | conditional_memval1 | conditional_memval2))
            { // jeśli chodzi o komorke pamieciową
                tmp = FindGroundNode(buff, TP_MEMCELL);
                if (tmp)
                    Current->Params[9].asMemCell = tmp->MemCell;
                if (!Current->Params[9].asMemCell)
                {
                    ErrorLog("Bad event: MemCell \"" + string(buff) +
                             "\" does not exist in \"" + Current->asName + "\"");
                    Current->iFlags &=
                        ~(conditional_memstring | conditional_memval1 | conditional_memval2);
                }
            }
            for (i = 0; i < 8; i++)
            {
                if (Current->Params[i].asText != NULL)
                {
                    strcpy(buff, Current->Params[i].asText);
                    SafeDeleteArray(Current->Params[i].asText);
                    Current->Params[i].asEvent = FindEvent(buff);
					if( !Current->Params[ i ].asEvent ) { // Ra: tylko w logu informacja o braku
						if( ( Current->Params[ i ].asText == NULL )
						 || ( std::string( Current->Params[ i ].asText ).substr( 0, 5 ) != "none_" ) ) {
							WriteLog( "Event \"" + string( buff ) + "\" does not exist" );
							ErrorLog( "Missed event: " + string( buff ) + " in multiple " + Current->asName );
						}
                        }
                }
            }
            break;
        case tp_Voltage: // zmiana napięcia w zasilaczu (TractionPowerSource)
            if (!Current->asNodeName.empty())
            {
                tmp = FindGroundNode(Current->asNodeName,
                                     TP_TRACTIONPOWERSOURCE); // podłączenie zasilacza
                if (tmp)
                    Current->Params[9].psPower = tmp->psTractionPowerSource;
                else
                    ErrorLog("Bad voltage: event \"" + Current->asName +
                             "\" cannot find power source \"" + Current->asNodeName + "\"");
            }
            Current->asNodeName = "";
            break;
        case tp_Message: // wyświetlenie komunikatu
            break;
        }
        if (Current->fDelay < 0)
            AddToQuery(Current, NULL);
    }
    for (TGroundNode *Current = nRootOfType[TP_MEMCELL]; Current; Current = Current->nNext)
    { // Ra: eventy komórek pamięci, wykonywane po wysłaniu komendy do zatrzymanego pojazdu
        Current->MemCell->AssignEvents(FindEvent(Current->asName + ":sent"));
    }
    return true;
}

void TGround::InitTracks()
{ //łączenie torów ze sobą i z eventami
    TGroundNode *Current, *Model;
    TTrack *tmp; // znaleziony tor
    TTrack *Track;
    int iConnection, state;
    string name;
    // tracks=tracksfar=0;
    for (Current = nRootOfType[TP_TRACK]; Current; Current = Current->nNext)
    {
        Track = Current->pTrack;
        if (Global::iHiddenEvents & 1)
            if (!Current->asName.empty())
            { // jeśli podana jest nazwa torów, można szukać eventów skojarzonych przez nazwę
                if (Track->asEvent0Name.empty())
                    if (FindEvent(Current->asName + ":event0"))
                        Track->asEvent0Name = Current->asName + ":event0";
                if (Track->asEvent1Name.empty())
                    if (FindEvent(Current->asName + ":event1"))
                        Track->asEvent1Name = Current->asName + ":event1";
                if (Track->asEvent2Name.empty())
                    if (FindEvent(Current->asName + ":event2"))
                        Track->asEvent2Name = Current->asName + ":event2";

                if (Track->asEventall0Name.empty())
                    if (FindEvent(Current->asName + ":eventall0"))
                        Track->asEventall0Name = Current->asName + ":eventall0";
                if (Track->asEventall1Name.empty())
                    if (FindEvent(Current->asName + ":eventall1"))
                        Track->asEventall1Name = Current->asName + ":eventall1";
                if (Track->asEventall2Name.empty())
                    if (FindEvent(Current->asName + ":eventall2"))
                        Track->asEventall2Name = Current->asName + ":eventall2";
            }
        Track->AssignEvents(
            Track->asEvent0Name.empty() ? NULL : FindEvent(Track->asEvent0Name),
            Track->asEvent1Name.empty() ? NULL : FindEventScan(Track->asEvent1Name),
            Track->asEvent2Name.empty() ? NULL : FindEventScan(Track->asEvent2Name));
        Track->AssignallEvents(
            Track->asEventall0Name.empty() ? NULL : FindEvent(Track->asEventall0Name),
            Track->asEventall1Name.empty() ? NULL : FindEvent(Track->asEventall1Name),
            Track->asEventall2Name.empty() ? NULL :
                                               FindEvent(Track->asEventall2Name)); // MC-280503
        switch (Track->eType)
        {
        case tt_Table: // obrotnicę też łączymy na starcie z innymi torami
            Model = FindGroundNode(Current->asName, TP_MODEL); // szukamy modelu o tej samej nazwie
            // if (tmp) //mamy model, trzeba zapamiętać wskaźnik do jego animacji
            { // jak coś pójdzie źle, to robimy z tego normalny tor
                // Track->ModelAssign(tmp->Model->GetContainer(NULL)); //wiązanie toru z modelem
                // obrotnicy
                Track->RaAssign(
                    Current, Model ? Model->Model : NULL, FindEvent(Current->asName + ":done"),
                    FindEvent(Current->asName + ":joined")); // wiązanie toru z modelem obrotnicy
                // break; //jednak połączę z sąsiednim, jak ma się wysypywać null track
            }
            if (!Model) // jak nie ma modelu
                break; // to pewnie jest wykolejnica, a ta jest domyślnie zamknięta i wykoleja
        case tt_Normal: // tylko proste są podłączane do rozjazdów, stąd dwa rozjazdy się nie
            // połączą ze sobą
            if (Track->CurrentPrev() == NULL) // tylko jeśli jeszcze nie podłączony
            {
                tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_0(), iConnection, Current);
                switch (iConnection)
                {
                case -1: // Ra: pierwsza koncepcja zawijania samochodów i statków
                    // if ((Track->iCategoryFlag&1)==0) //jeśli nie jest torem szynowym
                    // Track->ConnectPrevPrev(Track,0); //łączenie końca odcinka do samego siebie
                    break;
                case 0:
                    Track->ConnectPrevPrev(tmp, 0);
                    break;
                case 1:
                    Track->ConnectPrevNext(tmp, 1);
                    break;
                case 2:
                    Track->ConnectPrevPrev(tmp, 0); // do Point1 pierwszego
                    tmp->SetConnections(0); // zapamiętanie ustawień w Segmencie
                    break;
                case 3:
                    Track->ConnectPrevNext(tmp, 1); // do Point2 pierwszego
                    tmp->SetConnections(0); // zapamiętanie ustawień w Segmencie
                    break;
                case 4:
                    tmp->Switch(1);
                    Track->ConnectPrevPrev(tmp, 2); // do Point1 drugiego
                    tmp->SetConnections(1); // robi też Switch(0)
                    tmp->Switch(0);
                    break;
                case 5:
                    tmp->Switch(1);
                    Track->ConnectPrevNext(tmp, 3); // do Point2 drugiego
                    tmp->SetConnections(1); // robi też Switch(0)
                    tmp->Switch(0);
                    break;
                }
            }
            if (Track->CurrentNext() == NULL) // tylko jeśli jeszcze nie podłączony
            {
                tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_1(), iConnection, Current);
                switch (iConnection)
                {
                case -1: // Ra: pierwsza koncepcja zawijania samochodów i statków
                    // if ((Track->iCategoryFlag&1)==0) //jeśli nie jest torem szynowym
                    // Track->ConnectNextNext(Track,1); //łączenie końca odcinka do samego siebie
                    break;
                case 0:
                    Track->ConnectNextPrev(tmp, 0);
                    break;
                case 1:
                    Track->ConnectNextNext(tmp, 1);
                    break;
                case 2:
                    Track->ConnectNextPrev(tmp, 0);
                    tmp->SetConnections(0); // zapamiętanie ustawień w Segmencie
                    break;
                case 3:
                    Track->ConnectNextNext(tmp, 1);
                    tmp->SetConnections(0); // zapamiętanie ustawień w Segmencie
                    break;
                case 4:
                    tmp->Switch(1);
                    Track->ConnectNextPrev(tmp, 2);
                    tmp->SetConnections(1); // robi też Switch(0)
                    // tmp->Switch(0);
                    break;
                case 5:
                    tmp->Switch(1);
                    Track->ConnectNextNext(tmp, 3);
                    tmp->SetConnections(1); // robi też Switch(0)
                    // tmp->Switch(0);
                    break;
                }
            }
            break;
        case tt_Switch: // dla rozjazdów szukamy eventów sygnalizacji rozprucia
            Track->AssignForcedEvents(FindEvent(Current->asName + ":forced+"),
                                      FindEvent(Current->asName + ":forced-"));
            break;
        }
        name = Track->IsolatedName(); // pobranie nazwy odcinka izolowanego
        if (!name.empty()) // jeśli została zwrócona nazwa
            Track->IsolatedEventsAssign(FindEvent(name + ":busy"), FindEvent(name + ":free"));
        if (Current->asName.substr(0, 1) ==
            "*") // możliwy portal, jeśli nie podłączony od striny 1
            if (!Track->CurrentPrev() && Track->CurrentNext())
                Track->iCategoryFlag |= 0x100; // ustawienie flagi portalu
    }
    // WriteLog("Total "+AnsiString(tracks)+", far "+AnsiString(tracksfar));
    TIsolated *p = TIsolated::Root();
    while (p)
    { // jeśli się znajdzie, to podać wskaźnik
        Current = FindGroundNode(p->asName, TP_MEMCELL); // czy jest komóka o odpowiedniej nazwie
        if (Current)
            p->pMemCell = Current->MemCell; // przypisanie powiązanej komórki
        else
        { // utworzenie automatycznej komórki
            Current = new TGroundNode(); // to nie musi mieć nazwy, nazwa w wyszukiwarce wystarczy
            // Current->asName=p->asName; //mazwa identyczna, jak nazwa odcinka izolowanego
            Current->MemCell = new TMemCell(NULL); // nowa komórka
#ifdef EU07_USE_OLD_TNAMES_CLASS
            sTracks->Add(TP_MEMCELL, p->asName.c_str(), Current); // dodanie do wyszukiwarki
#else
            m_trackmap.Add( TP_MEMCELL, p->asName, Current );
#endif
            Current->nNext =
                nRootOfType[TP_MEMCELL]; // to nie powinno tutaj być, bo robi się śmietnik
            nRootOfType[TP_MEMCELL] = Current;
            iNumNodes++;
            p->pMemCell = Current->MemCell; // wskaźnik komóki przekazany do odcinka izolowanego
        }
        p = p->Next();
    }
    // for (Current=nRootOfType[TP_TRACK];Current;Current=Current->nNext)
    // if (Current->pTrack->eType==tt_Cross)
    //  Current->pTrack->ConnectionsLog(); //zalogowanie informacji o połączeniach
}

void TGround::InitTraction()
{ //łączenie drutów ze sobą oraz z torami i eventami
    TGroundNode *nCurrent, *nTemp;
    TTraction *tmp; // znalezione przęsło
    TTraction *Traction;
    int iConnection;
    string name;
    for (nCurrent = nRootOfType[TP_TRACTION]; nCurrent; nCurrent = nCurrent->nNext)
    { // podłączenie do zasilacza, żeby można było sumować prąd kilku pojazdów
        // a jednocześnie z jednego miejsca zmieniać napięcie eventem
        // wykonywane najpierw, żeby można było logować podłączenie 2 zasilaczy do jednego drutu
        // izolator zawieszony na przęśle jest ma być osobnym odcinkiem drutu o długości ok. 1m,
        // podłączonym do zasilacza o nazwie "*" (gwiazka); "none" nie będzie odpowiednie
        Traction = nCurrent->hvTraction;
        nTemp = FindGroundNode(Traction->asPowerSupplyName, TP_TRACTIONPOWERSOURCE);
        if (nTemp) // jak zasilacz znaleziony
            Traction->PowerSet(nTemp->psTractionPowerSource); // to podłączyć do przęsła
        else if (Traction->asPowerSupplyName != "*") // gwiazdka dla przęsła z izolatorem
            if (Traction->asPowerSupplyName != "none") // dopuszczamy na razie brak podłączenia?
            { // logowanie błędu i utworzenie zasilacza o domyślnej zawartości
                ErrorLog("Missed TractionPowerSource: " + Traction->asPowerSupplyName);
                nTemp = new TGroundNode();
                nTemp->iType = TP_TRACTIONPOWERSOURCE;
                nTemp->asName = Traction->asPowerSupplyName;
                nTemp->psTractionPowerSource = new TTractionPowerSource(nTemp);
                nTemp->psTractionPowerSource->Init(Traction->NominalVoltage, Traction->MaxCurrent);
                nTemp->nNext = nRootOfType[nTemp->iType]; // ostatni dodany dołączamy na końcu
                // nowego
                nRootOfType[nTemp->iType] = nTemp; // ustawienie nowego na początku listy
                iNumNodes++;
            }
    }
    for (nCurrent = nRootOfType[TP_TRACTION]; nCurrent; nCurrent = nCurrent->nNext)
    {
        Traction = nCurrent->hvTraction;
        if (!Traction->hvNext[0]) // tylko jeśli jeszcze nie podłączony
        {
            tmp = FindTraction(&Traction->pPoint1, iConnection, nCurrent);
            switch (iConnection)
            {
            case 0:
                Traction->Connect(0, tmp, 0);
                break;
            case 1:
                Traction->Connect(0, tmp, 1);
                break;
            }
            if (Traction->hvNext[0]) // jeśli został podłączony
                if (Traction->psSection && tmp->psSection) // tylko przęsło z izolatorem może nie
                    // mieć zasilania, bo ma 2, trzeba
                    // sprawdzać sąsiednie
                    if (Traction->psSection !=
                        tmp->psSection) // połączone odcinki mają różne zasilacze
                    { // to może być albo podłączenie podstacji lub kabiny sekcyjnej do sekcji, albo
                        // błąd
                        if (Traction->psSection->bSection && !tmp->psSection->bSection)
                        { //(tmp->psSection) jest podstacją, a (Traction->psSection) nazwą sekcji
                            tmp->PowerSet(Traction->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else if (!Traction->psSection->bSection && tmp->psSection->bSection)
                        { //(Traction->psSection) jest podstacją, a (tmp->psSection) nazwą sekcji
                            Traction->PowerSet(tmp->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else // jeśli obie to sekcje albo obie podstacje, to będzie błąd
                            ErrorLog("Bad power: at " +
                                     to_string(Traction->pPoint1.x, 2, 6) + " " +
                                     to_string(Traction->pPoint1.y, 2, 6) + " " +
                                     to_string(Traction->pPoint1.z, 2, 6));
                    }
        }
        if (!Traction->hvNext[1]) // tylko jeśli jeszcze nie podłączony
        {
            tmp = FindTraction(&Traction->pPoint2, iConnection, nCurrent);
            switch (iConnection)
            {
            case 0:
                Traction->Connect(1, tmp, 0);
                break;
            case 1:
                Traction->Connect(1, tmp, 1);
                break;
            }
            if (Traction->hvNext[1]) // jeśli został podłączony
                if (Traction->psSection && tmp->psSection) // tylko przęsło z izolatorem może nie
                    // mieć zasilania, bo ma 2, trzeba
                    // sprawdzać sąsiednie
                    if (Traction->psSection != tmp->psSection)
                    { // to może być albo podłączenie podstacji lub kabiny sekcyjnej do sekcji, albo
                        // błąd
                        if (Traction->psSection->bSection && !tmp->psSection->bSection)
                        { //(tmp->psSection) jest podstacją, a (Traction->psSection) nazwą sekcji
                            tmp->PowerSet(Traction->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else if (!Traction->psSection->bSection && tmp->psSection->bSection)
                        { //(Traction->psSection) jest podstacją, a (tmp->psSection) nazwą sekcji
                            Traction->PowerSet(tmp->psSection); // zastąpienie wskazaniem sekcji
                        }
                        else // jeśli obie to sekcje albo obie podstacje, to będzie błąd
                            ErrorLog("Bad power: at " +
                                     to_string(Traction->pPoint2.x, 2, 6) + " " +
                                     to_string(Traction->pPoint2.y, 2, 6) + " " +
                                     to_string(Traction->pPoint2.z, 2, 6));
                    }
        }
    }
    iConnection = 0; // teraz będzie licznikiem końców
    for (nCurrent = nRootOfType[TP_TRACTION]; nCurrent; nCurrent = nCurrent->nNext)
    { // operacje mające na celu wykrywanie bieżni wspólnych i łączenie przęseł naprążania
        if (nCurrent->hvTraction->WhereIs()) // oznakowanie przedostatnich przęseł
        { // poszukiwanie bieżni wspólnej dla przedostatnich przęseł, również w celu połączenia
            // zasilania
            // to się nie sprawdza, bo połączyć się mogą dwa niezasilane odcinki jako najbliższe
            // sobie
            // nCurrent->hvTraction->hvParallel=TractionNearestFind(nCurrent->pCenter,0,nCurrent);
            // //szukanie najbliższego przęsła
            // trzeba by zliczać końce, a potem wpisać je do tablicy, aby sukcesywnie podłączać do
            // zasilaczy
            nCurrent->hvTraction->iTries = 5; // oznaczanie końcowych
            ++iConnection;
        }
        if (nCurrent->hvTraction->fResistance[0] == 0.0)
        {
            nCurrent->hvTraction
                ->ResistanceCalc(); // obliczanie przęseł w segmencie z bezpośrednim zasilaniem
            // ErrorLog("Section "+nCurrent->hvTraction->asPowerSupplyName+" connected"); //jako
            // niby błąd będzie bardziej widoczne
            nCurrent->hvTraction->iTries = 0; // nie potrzeba mu szukać zasilania
        }
        // if (!Traction->hvParallel) //jeszcze utworzyć pętle z bieżni wspólnych
    }
    int zg = 0; // zgodność kierunku przęseł, tymczasowo iterator do tabeli końców
    TGroundNode **nEnds = new TGroundNode *[iConnection]; // końców jest ok. 10 razy mniej niż
    // wszystkich przęseł (Quark: 216)
    for (nCurrent = nRootOfType[TP_TRACTION]; nCurrent; nCurrent = nCurrent->nNext)
    { //łączenie bieżni wspólnych, w tym oznaczanie niepodanych jawnie
        Traction = nCurrent->hvTraction;
        if (!Traction->asParallel.empty()) // będzie wskaźnik na inne przęsło
            if ((Traction->asParallel == "none") ||
                (Traction->asParallel == "*")) // jeśli nieokreślone
                Traction->iLast =
                    2; // jakby przedostatni - niech po prostu szuka (iLast już przeliczone)
            else if (!Traction->hvParallel) // jeśli jeszcze nie został włączony w kółko
            {
                nTemp = FindGroundNode(Traction->asParallel, TP_TRACTION);
                if (nTemp)
                { // o ile zostanie znalezione przęsło o takiej nazwie
                    if (!nTemp->hvTraction
                             ->hvParallel) // jeśli tamten jeszcze nie ma wskaźnika bieżni wspólnej
                        Traction->hvParallel =
                            nTemp->hvTraction; // wpisać siebie i dalej dać mu wskaźnik zwrotny
                    else // a jak ma, to albo dołączyć się do kółeczka
                        Traction->hvParallel =
                            nTemp->hvTraction->hvParallel; // przjąć dotychczasowy wskaźnik od niego
                    nTemp->hvTraction->hvParallel =
                        Traction; // i na koniec ustawienie wskaźnika zwrotnego
                }
                if (!Traction->hvParallel)
                    ErrorLog("Missed overhead: " + Traction->asParallel); // logowanie braku
            }
        if (Traction->iTries > 0) // jeśli zaznaczony do podłączenia
            // if (!nCurrent->hvTraction->psPower[0]||!nCurrent->hvTraction->psPower[1])
            if (zg < iConnection) // zabezpieczenie
                nEnds[zg++] = nCurrent; // wypełnianie tabeli końców w celu szukania im połączeń
    }
    while (zg < iConnection)
        nEnds[zg++] = NULL; // zapełnienie do końca tablicy, jeśli by jakieś końce wypadły
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
};

void TGround::TrackJoin(TGroundNode *Current)
{ // wyszukiwanie sąsiednich torów do podłączenia (wydzielone na użytek obrotnicy)
    TTrack *Track = Current->pTrack;
    TTrack *tmp;
    int iConnection;
    if (!Track->CurrentPrev())
    {
        tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_0(), iConnection,
                        Current); // Current do pominięcia
        switch (iConnection)
        {
        case 0:
            Track->ConnectPrevPrev(tmp, 0);
            break;
        case 1:
            Track->ConnectPrevNext(tmp, 1);
            break;
        }
    }
    if (!Track->CurrentNext())
    {
        tmp = FindTrack(Track->CurrentSegment()->FastGetPoint_1(), iConnection, Current);
        switch (iConnection)
        {
        case 0:
            Track->ConnectNextPrev(tmp, 0);
            break;
        case 1:
            Track->ConnectNextNext(tmp, 1);
            break;
        }
    }
}

// McZapkie-070602: wyzwalacze zdarzen
bool TGround::InitLaunchers()
{
    TGroundNode *Current, *tmp;
    TEventLauncher *EventLauncher;
    int i;
    for (Current = nRootOfType[TP_EVLAUNCH]; Current; Current = Current->nNext)
    {
        EventLauncher = Current->EvLaunch;
        if (EventLauncher->iCheckMask != 0)
            if (EventLauncher->asMemCellName != "none")
            { // jeśli jest powiązana komórka pamięci
                tmp = FindGroundNode(EventLauncher->asMemCellName, TP_MEMCELL);
                if (tmp)
                    EventLauncher->MemCell = tmp->MemCell; // jeśli znaleziona, dopisać
                else
                    MessageBox(0, "Cannot find Memory Cell for Event Launcher", "Error", MB_OK);
            }
            else
                EventLauncher->MemCell = NULL;
        EventLauncher->Event1 = (EventLauncher->asEvent1Name != "none") ?
                                    FindEvent(EventLauncher->asEvent1Name) :
                                    NULL;
        EventLauncher->Event2 = (EventLauncher->asEvent2Name != "none") ?
                                    FindEvent(EventLauncher->asEvent2Name) :
                                    NULL;
    }
    return true;
}

TTrack * TGround::FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude)
{ // wyszukiwanie innego toru kończącego się w (Point)
    TTrack *Track;
    TGroundNode *Current;
    TTrack *tmp;
    iConnection = -1;
    TSubRect *sr;
    // najpierw szukamy w okolicznych segmentach
    int c = GetColFromX(Point.x);
    int r = GetRowFromZ(Point.z);
    if ((sr = FastGetSubRect(c, r)) != NULL) // 75% torów jest w tym samym sektorze
        if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
            return tmp;
    int i, x, y;
    for (i = 1; i < 9;
         ++i) // sektory w kolejności odległości, 4 jest tu wystarczające, 9 na wszelki wypadek
    { // niemal wszystkie podłączone tory znajdują się w sąsiednich 8 sektorach
        x = SectorOrder[i].x;
        y = SectorOrder[i].y;
        if ((sr = FastGetSubRect(c + y, r + x)) != NULL)
            if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
                return tmp;
        if (x)
            if ((sr = FastGetSubRect(c + y, r - x)) != NULL)
                if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
                    return tmp;
        if (y)
            if ((sr = FastGetSubRect(c - y, r + x)) != NULL)
                if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
                    return tmp;
        if ((sr = FastGetSubRect(c - y, r - x)) != NULL)
            if ((tmp = sr->FindTrack(&Point, iConnection, Exclude->pTrack)) != NULL)
                return tmp;
    }
#if 0
 //wyszukiwanie czołgowe (po wszystkich jak leci) - nie ma chyba sensu
 for (Current=nRootOfType[TP_TRACK];Current;Current=Current->Next)
 {
  if ((Current->iType==TP_TRACK) && (Current!=Exclude))
  {
   iConnection=Current->pTrack->TestPoint(&Point);
   if (iConnection>=0) return Current->pTrack;
  }
 }
#endif
    return NULL;
}

TTraction * TGround::FindTraction(vector3 *Point, int &iConnection, TGroundNode *Exclude)
{ // wyszukiwanie innego przęsła kończącego się w (Point)
    TTraction *Traction;
    TGroundNode *Current;
    TTraction *tmp;
    iConnection = -1;
    TSubRect *sr;
    // najpierw szukamy w okolicznych segmentach
    int c = GetColFromX(Point->x);
    int r = GetRowFromZ(Point->z);
    if ((sr = FastGetSubRect(c, r)) != NULL) // większość będzie w tym samym sektorze
        if ((tmp = sr->FindTraction(Point, iConnection, Exclude->hvTraction)) != NULL)
            return tmp;
    int i, x, y;
    for (i = 1; i < 9;
         ++i) // sektory w kolejności odległości, 4 jest tu wystarczające, 9 na wszelki wypadek
    { // wszystkie przęsła powinny zostać znajdować się w sąsiednich 8 sektorach
        x = SectorOrder[i].x;
        y = SectorOrder[i].y;
        if ((sr = FastGetSubRect(c + y, r + x)) != NULL)
            if ((tmp = sr->FindTraction(Point, iConnection, Exclude->hvTraction)) != NULL)
                return tmp;
        if (x & y)
        {
            if ((sr = FastGetSubRect(c + y, r - x)) != NULL)
                if ((tmp = sr->FindTraction(Point, iConnection, Exclude->hvTraction)) != NULL)
                    return tmp;
            if ((sr = FastGetSubRect(c - y, r + x)) != NULL)
                if ((tmp = sr->FindTraction(Point, iConnection, Exclude->hvTraction)) != NULL)
                    return tmp;
        }
        if ((sr = FastGetSubRect(c - y, r - x)) != NULL)
            if ((tmp = sr->FindTraction(Point, iConnection, Exclude->hvTraction)) != NULL)
                return tmp;
    }
    return NULL;
};

TTraction * TGround::TractionNearestFind(vector3 &p, int dir, TGroundNode *n)
{ // wyszukanie najbliższego do (p) przęsła o tej samej nazwie sekcji (ale innego niż podłączone)
    // oraz zasilanego z kierunku (dir)
    TGroundNode *nCurrent, *nBest = NULL;
    int i, j, k, zg;
    double d, dist = 200.0 * 200.0; //[m] odległość graniczna
    // najpierw szukamy w okolicznych segmentach
    int c = GetColFromX(n->pCenter.x);
    int r = GetRowFromZ(n->pCenter.z);
    TSubRect *sr;
    for (i = -1; i <= 1; ++i) // przeglądamy 9 najbliższych sektorów
        for (j = -1; j <= 1; ++j) //
            if ((sr = FastGetSubRect(c + i, r + j)) != NULL) // o ile w ogóle sektor jest
                for (nCurrent = sr->nRenderWires; nCurrent; nCurrent = nCurrent->nNext3)
                    if (nCurrent->iType == TP_TRACTION)
                        if (nCurrent->hvTraction->psSection ==
                            n->hvTraction->psSection) // jeśli ta sama sekcja
                            if (nCurrent != n) // ale nie jest tym samym
                                if (nCurrent->hvTraction !=
                                    n->hvTraction
                                        ->hvNext[0]) // ale nie jest bezpośrednio podłączonym
                                    if (nCurrent->hvTraction != n->hvTraction->hvNext[1])
                                        if (nCurrent->hvTraction->psPower
                                                [k = (DotProduct(
                                                          n->hvTraction->vParametric,
                                                          nCurrent->hvTraction->vParametric) >= 0 ?
                                                          dir ^ 1 :
                                                          dir)]) // ma zasilanie z odpowiedniej
                                            // strony
                                            if (nCurrent->hvTraction->fResistance[k] >=
                                                0.0) //żeby się nie propagowały jakieś ujemne
                                            { // znaleziony kandydat do połączenia
                                                d = SquareMagnitude(
                                                    p -
                                                    nCurrent
                                                        ->pCenter); // kwadrat odległości środków
                                                if (dist > d)
                                                { // zapamiętanie nowego najbliższego
                                                    dist = d; // nowy rekord odległości
                                                    nBest = nCurrent;
                                                    zg = k; // z którego końca brać wskaźnik
                                                    // zasilacza
                                                }
                                            }
    if (nBest) // jak znalezione przęsło z zasilaniem, to podłączenie "równoległe"
    {
        n->hvTraction->ResistanceCalc(dir, nBest->hvTraction->fResistance[zg],
                                      nBest->hvTraction->psPower[zg]);
        // testowo skrzywienie przęsła tak, aby pokazać skąd ma zasilanie
        // if (dir) //1 gdy ciąg dalszy jest od strony Point2
        // n->hvTraction->pPoint3=0.25*(nBest->pCenter+3*(zg?nBest->hvTraction->pPoint4:nBest->hvTraction->pPoint3));
        // else
        // n->hvTraction->pPoint4=0.25*(nBest->pCenter+3*(zg?nBest->hvTraction->pPoint4:nBest->hvTraction->pPoint3));
    }
    return (nBest ? nBest->hvTraction : NULL);
};

bool TGround::AddToQuery(TEvent *Event, TDynamicObject *Node)
{
    if (Event->bEnabled) // jeśli może być dodany do kolejki (nie używany w skanowaniu)
        if (!Event->iQueued) // jeśli nie dodany jeszcze do kolejki
        { // kolejka eventów jest posortowana względem (fStartTime)
            Event->Activator = Node;
            if (Event->Type == tp_AddValues ? (Event->fDelay == 0.0) : false)
            { // eventy AddValues trzeba wykonywać natychmiastowo, inaczej kolejka może zgubić
                // jakieś dodawanie
                // Ra: kopiowanie wykonania tu jest bez sensu, lepiej by było wydzielić funkcję
                // wykonującą eventy i ją wywołać
                if (EventConditon(Event))
                { // teraz mogą być warunki do tych eventów
                    Event->Params[5].asMemCell->UpdateValues(
                        Event->Params[0].asText, Event->Params[1].asdouble,
                        Event->Params[2].asdouble, Event->iFlags);
                    if (Event->Params[6].asTrack)
                    { // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla
                        // wszystkich 'dynamic' na danym torze
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
                        for (int i = 0; i < Event->Params[6].asTrack->iNumDynamics; ++i)
                            Event->Params[5].asMemCell->PutCommand(
                                Event->Params[6].asTrack->Dynamics[i]->Mechanik,
                                &Event->Params[4].nGroundNode->pCenter);
#else
                        for( auto dynamic : Event->Params[ 6 ].asTrack->Dynamics ) {
                            Event->Params[ 5 ].asMemCell->PutCommand(
                                dynamic->Mechanik,
                                &Event->Params[ 4 ].nGroundNode->pCenter );
                        }
#endif
                        //if (DebugModeFlag)
                            WriteLog("EVENT EXECUTED: AddValues & Track command - " +
                                     std::string(Event->Params[0].asText) + " " +
                                     std::to_string(Event->Params[1].asdouble) + " " +
                                     std::to_string(Event->Params[2].asdouble));
                    }
                    //else if (DebugModeFlag)
                        WriteLog("EVENT EXECUTED: AddValues - " +
                                 std::string(Event->Params[0].asText) + " " +
                                 std::to_string(Event->Params[1].asdouble) + " " +
                                 std::to_string(Event->Params[2].asdouble));
                }
                Event =
                    Event
                        ->evJoined; // jeśli jest kolejny o takiej samej nazwie, to idzie do kolejki
            }
            if (Event)
            { // standardowe dodanie do kolejki
                WriteLog("EVENT ADDED TO QUEUE: " + Event->asName +
                         (Node ? (" by " + Node->asName) : string("")));
                Event->fStartTime =
                    fabs(Event->fDelay) + Timer::GetTime(); // czas od uruchomienia scenerii
                if (Event->fRandomDelay > 0.0)
                    Event->fStartTime += Event->fRandomDelay * Random(10000) *
                                         0.0001; // doliczenie losowego czasu opóźnienia
                ++Event->iQueued; // zabezpieczenie przed podwójnym dodaniem do kolejki
                if (QueryRootEvent ? Event->fStartTime >= QueryRootEvent->fStartTime : false)
                    QueryRootEvent->AddToQuery(Event); // dodanie gdzieś w środku
                else
                { // dodanie z przodu: albo nic nie ma, albo ma być wykonany szybciej niż pierwszy
                    Event->evNext = QueryRootEvent;
                    QueryRootEvent = Event;
                }
            }
        }
    return true;
}

bool TGround::EventConditon(TEvent *e)
{ // sprawdzenie spelnienia warunków dla eventu
    if (e->iFlags <= update_only)
        return true; // bezwarunkowo
    if (e->iFlags & conditional_trackoccupied)
        return (!e->Params[9].asTrack->IsEmpty());
    else if (e->iFlags & conditional_trackfree)
        return (e->Params[9].asTrack->IsEmpty());
    else if (e->iFlags & conditional_propability)
    {
        double rprobability = 1.0 * rand() / RAND_MAX;
        WriteLog("Random integer: " + std::to_string(rprobability) + "/" +
                 std::to_string(e->Params[10].asdouble));
        return (e->Params[10].asdouble > rprobability);
    }
    else if (e->iFlags & conditional_memcompare)
    { // porównanie wartości
        if (tmpEvent->Params[9].asMemCell->Compare(e->Params[10].asText, e->Params[11].asdouble,
                                                   e->Params[12].asdouble, e->iFlags))
			{ //logowanie spełnionych warunków
			LogComment = e->Params[9].asMemCell->Text() + string(" ") +
                         to_string(e->Params[9].asMemCell->Value1(), 2, 8) + " " +
                         to_string(tmpEvent->Params[9].asMemCell->Value2(), 2, 8) +
                         " = ";
            if (TestFlag(e->iFlags, conditional_memstring))
                LogComment += string(tmpEvent->Params[10].asText);
            else
                LogComment += "*";
            if (TestFlag(tmpEvent->iFlags, conditional_memval1))
                LogComment += " " + to_string(tmpEvent->Params[11].asdouble, 2, 8);
            else
                LogComment += " *";
            if (TestFlag(tmpEvent->iFlags, conditional_memval2))
                LogComment += " " + to_string(tmpEvent->Params[12].asdouble, 2, 8);
            else
                LogComment += " *";
			WriteLog(LogComment);
            return true;
			}
        //else if (Global::iWriteLogEnabled && DebugModeFlag) //zawsze bo to bardzo istotne w debugowaniu scenariuszy
		else
        { // nie zgadza się, więc sprawdzmy, co
            LogComment = e->Params[9].asMemCell->Text() + string(" ") +
                         to_string(e->Params[9].asMemCell->Value1(), 2, 8) + " " +
                         to_string(tmpEvent->Params[9].asMemCell->Value2(), 2, 8) +
                         " != ";
            if (TestFlag(e->iFlags, conditional_memstring))
                LogComment += (tmpEvent->Params[10].asText);
            else
                LogComment += "*";
            if (TestFlag(tmpEvent->iFlags, conditional_memval1))
                LogComment += " " + to_string(tmpEvent->Params[11].asdouble, 2, 8);
            else
                LogComment += " *";
            if (TestFlag(tmpEvent->iFlags, conditional_memval2))
                LogComment += " " + to_string(tmpEvent->Params[12].asdouble, 2, 8);
            else
                LogComment += " *";
            WriteLog(LogComment);
        }
    }
    return false;
};

bool TGround::CheckQuery()
{ // sprawdzenie kolejki eventów oraz wykonanie tych, którym czas minął
    TLocation loc;
    int i;
    /* //Ra: to w ogóle jakiś chory kod jest; wygląda jak wyszukanie eventu z najlepszym czasem
     Double evtime,evlowesttime; //Ra: co to za typ?
     //evlowesttime=1000000;
     if (QueryRootEvent)
     {
      OldQRE=QueryRootEvent;
      tmpEvent=QueryRootEvent;
     }
     if (QueryRootEvent)
     {
      for (i=0;i<90;++i)
      {
       evtime=((tmpEvent->fStartTime)-(Timer::GetTime())); //pobranie wartości zmiennej
       if (evtime<evlowesttime)
       {
        evlowesttime=evtime;
        tmp2Event=tmpEvent;
       }
       if (tmpEvent->Next)
        tmpEvent=tmpEvent->Next;
       else
        i=100;
      }
      if (OldQRE!=tmp2Event)
      {
       QueryRootEvent->AddToQuery(QueryRootEvent);
       QueryRootEvent=tmp2Event;
      }
     }
    */
    /*
     if (QueryRootEvent)
     {//wypisanie kolejki
      tmpEvent=QueryRootEvent;
      WriteLog("--> Event queue:");
      while (tmpEvent)
      {
       WriteLog(tmpEvent->asName+" "+AnsiString(tmpEvent->fStartTime));
       tmpEvent=tmpEvent->Next;
      }
     }
    */
    while (QueryRootEvent ? QueryRootEvent->fStartTime < Timer::GetTime() : false)
    { // eventy są posortowana wg czasu wykonania
        tmpEvent = QueryRootEvent; // wyjęcie eventu z kolejki
        if (QueryRootEvent->evJoined) // jeśli jest kolejny o takiej samej nazwie
        { // to teraz on będzie następny do wykonania
            QueryRootEvent = QueryRootEvent->evJoined; // następny będzie ten doczepiony
            QueryRootEvent->evNext = tmpEvent->evNext; // pamiętając o następnym z kolejki
            QueryRootEvent->fStartTime =
                tmpEvent->fStartTime; // czas musi być ten sam, bo nie jest aktualizowany
            QueryRootEvent->Activator = tmpEvent->Activator; // pojazd aktywujący
            // w sumie można by go dodać normalnie do kolejki, ale trzeba te połączone posortować wg
            // czasu wykonania
        }
        else // a jak nazwa jest unikalna, to kolejka idzie dalej
            QueryRootEvent = QueryRootEvent->evNext; // NULL w skrajnym przypadku
        if (tmpEvent->bEnabled)
        { // w zasadzie te wyłączone są skanowane i nie powinny się nigdy w kolejce znaleźć
            WriteLog("EVENT LAUNCHED: " + tmpEvent->asName +
                     (tmpEvent->Activator ? string(" by " + tmpEvent->Activator->asName) :
                                            string("")));
            switch (tmpEvent->Type)
            {
            case tp_CopyValues: // skopiowanie wartości z innej komórki
                tmpEvent->Params[5].asMemCell->UpdateValues(
                    tmpEvent->Params[9].asMemCell->Text(),
                    tmpEvent->Params[9].asMemCell->Value1(),
                    tmpEvent->Params[9].asMemCell->Value2(),
                    tmpEvent->iFlags // flagi określają, co ma być skopiowane
                    );
            // break; //żeby się wysłało do torów i nie było potrzeby na AddValues * 0 0
            case tp_AddValues: // różni się jedną flagą od UpdateValues
            case tp_UpdateValues:
                if (EventConditon(tmpEvent))
                { // teraz mogą być warunki do tych eventów
                    if (tmpEvent->Type != tp_CopyValues) // dla CopyValues zrobiło się wcześniej
                        tmpEvent->Params[5].asMemCell->UpdateValues(
                            tmpEvent->Params[0].asText,
                            tmpEvent->Params[1].asdouble,
                            tmpEvent->Params[2].asdouble,
                            tmpEvent->iFlags);
                    if (tmpEvent->Params[6].asTrack)
                    { // McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla
                        // wszystkich 'dynamic' na danym torze
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
                        for (int i = 0; i < tmpEvent->Params[6].asTrack->iNumDynamics; ++i)
                            tmpEvent->Params[5].asMemCell->PutCommand(
                                tmpEvent->Params[6].asTrack->Dynamics[i]->Mechanik,
                                &tmpEvent->Params[4].nGroundNode->pCenter);
#else
                        for( auto dynamic : tmpEvent->Params[ 6 ].asTrack->Dynamics ) {
                            tmpEvent->Params[ 5 ].asMemCell->PutCommand(
                                dynamic->Mechanik,
                                &tmpEvent->Params[ 4 ].nGroundNode->pCenter );
                        }
#endif
                        //if (DebugModeFlag)
                        WriteLog("Type: UpdateValues & Track command - " +
                            tmpEvent->Params[5].asMemCell->Text() + " " +
                            std::to_string( tmpEvent->Params[ 5 ].asMemCell->Value1() ) + " " +
                            std::to_string( tmpEvent->Params[ 5 ].asMemCell->Value2() ) );
                    }
                    else //if (DebugModeFlag) 
                        WriteLog("Type: UpdateValues - " +
                            tmpEvent->Params[5].asMemCell->Text() + " " +
                            std::to_string( tmpEvent->Params[ 5 ].asMemCell->Value1() ) + " " +
                            std::to_string( tmpEvent->Params[ 5 ].asMemCell->Value2() ) );
                }
                break;
            case tp_GetValues:
                if (tmpEvent->Activator)
                {
                    // loc.X= -tmpEvent->Params[8].nGroundNode->pCenter.x;
                    // loc.Y=  tmpEvent->Params[8].nGroundNode->pCenter.z;
                    // loc.Z=  tmpEvent->Params[8].nGroundNode->pCenter.y;
                    if (Global::iMultiplayer) // potwierdzenie wykonania dla serwera (odczyt
                        // semafora już tak nie działa)
                        WyslijEvent(tmpEvent->asName, tmpEvent->Activator->GetName());
                    // tmpEvent->Params[9].asMemCell->PutCommand(tmpEvent->Activator->Mechanik,loc);
                    tmpEvent->Params[9].asMemCell->PutCommand(
                        tmpEvent->Activator->Mechanik, &tmpEvent->Params[8].nGroundNode->pCenter);
                }
                WriteLog("Type: GetValues");
                break;
            case tp_PutValues:
                if (tmpEvent->Activator)
                {
                    loc.X =
                        -tmpEvent->Params[3].asdouble; // zamiana, bo fizyka ma inaczej niż sceneria
                    loc.Y = tmpEvent->Params[5].asdouble;
                    loc.Z = tmpEvent->Params[4].asdouble;
                    if (tmpEvent->Activator->Mechanik) // przekazanie rozkazu do AI
                        tmpEvent->Activator->Mechanik->PutCommand(
                            tmpEvent->Params[0].asText, tmpEvent->Params[1].asdouble,
                            tmpEvent->Params[2].asdouble, loc);
                    else
                    { // przekazanie do pojazdu
                        tmpEvent->Activator->MoverParameters->PutCommand(
                            tmpEvent->Params[0].asText, tmpEvent->Params[1].asdouble,
                            tmpEvent->Params[2].asdouble, loc);
                    }
                }
                WriteLog("Type: PutValues");
                break;
            case tp_Lights:
                if (tmpEvent->Params[9].asModel)
                    for (i = 0; i < iMaxNumLights; i++)
                        if (tmpEvent->Params[i].asdouble >= 0) //-1 zostawia bez zmiany
                            tmpEvent->Params[9].asModel->LightSet(
                                i, tmpEvent->Params[i].asdouble); // teraz też ułamek
                break;
            case tp_Visible:
                if (tmpEvent->Params[9].nGroundNode)
                    tmpEvent->Params[9].nGroundNode->bVisible = (tmpEvent->Params[i].asInt > 0);
                break;
            case tp_Velocity:
                Error("Not implemented yet :(");
                break;
            case tp_Exit:
                MessageBox(0, tmpEvent->asNodeName.c_str(), " THE END ", MB_OK);
                Global::iTextMode = -1; // wyłączenie takie samo jak sekwencja F10 -> Y
                return false;
            case tp_Sound:
                switch (tmpEvent->Params[0].asInt)
                { // trzy możliwe przypadki:
                case 0:
                    tmpEvent->Params[9].tsTextSound->Stop();
                    break;
                case 1:
                    tmpEvent->Params[9].tsTextSound->Play(
                        1, 0, true, tmpEvent->Params[9].tsTextSound->vSoundPosition);
                    break;
                case -1:
                    tmpEvent->Params[9].tsTextSound->Play(
                        1, DSBPLAY_LOOPING, true, tmpEvent->Params[9].tsTextSound->vSoundPosition);
                    break;
                }
                break;
            case tp_Disable:
                Error("Not implemented yet :(");
                break;
            case tp_Animation: // Marcin: dorobic translacje - Ra: dorobiłem ;-)
                if (tmpEvent->Params[0].asInt == 1)
                    tmpEvent->Params[9].asAnimContainer->SetRotateAnim(
                        vector3(tmpEvent->Params[1].asdouble, tmpEvent->Params[2].asdouble,
                                tmpEvent->Params[3].asdouble),
                        tmpEvent->Params[4].asdouble);
                else if (tmpEvent->Params[0].asInt == 2)
                    tmpEvent->Params[9].asAnimContainer->SetTranslateAnim(
                        vector3(tmpEvent->Params[1].asdouble, tmpEvent->Params[2].asdouble,
                                tmpEvent->Params[3].asdouble),
                        tmpEvent->Params[4].asdouble);
                else if (tmpEvent->Params[0].asInt == 4)
                    tmpEvent->Params[9].asModel->AnimationVND(
                        tmpEvent->Params[8].asPointer,
                        tmpEvent->Params[1].asdouble, // tu mogą być dodatkowe parametry, np. od-do
                        tmpEvent->Params[2].asdouble, tmpEvent->Params[3].asdouble,
                        tmpEvent->Params[4].asdouble);
                break;
            case tp_Switch:
                if (tmpEvent->Params[9].asTrack)
                    tmpEvent->Params[9].asTrack->Switch(tmpEvent->Params[0].asInt,
                                                        tmpEvent->Params[1].asdouble,
                                                        tmpEvent->Params[2].asdouble);
                if (Global::iMultiplayer) // dajemy znać do serwera o przełożeniu
                    WyslijEvent(tmpEvent->asName, ""); // wysłanie nazwy eventu przełączajacego
                // Ra: bardziej by się przydała nazwa toru, ale nie ma do niej stąd dostępu
                break;
            case tp_TrackVel:
                if (tmpEvent->Params[9].asTrack)
                { // prędkość na zwrotnicy może być ograniczona z góry we wpisie, większej się nie
                    // ustawi eventem
                    WriteLog("type: TrackVel");
                    // WriteLog("Vel: ",tmpEvent->Params[0].asdouble);
                    tmpEvent->Params[9].asTrack->VelocitySet(tmpEvent->Params[0].asdouble);
                    if (DebugModeFlag) // wyświetlana jest ta faktycznie ustawiona
                        WriteLog("vel: ", tmpEvent->Params[9].asTrack->VelocityGet());
                }
                break;
            case tp_DynVel:
                Error("Event \"DynVel\" is obsolete");
                break;
            case tp_Multiple:
            {
                bCondition = EventConditon(tmpEvent);
                if (bCondition || (tmpEvent->iFlags &
                                   conditional_anyelse)) // warunek spelniony albo było użyte else
                {
                    WriteLog("Multiple passed");
                    for (i = 0; i < 8; ++i)
                    { // dodawane do kolejki w kolejności zapisania
                        if (tmpEvent->Params[i].asEvent)
                            if (bCondition != bool(tmpEvent->iFlags & (conditional_else << i)))
                            {
                                if (tmpEvent->Params[i].asEvent != tmpEvent)
                                    AddToQuery(tmpEvent->Params[i].asEvent,
                                               tmpEvent->Activator); // normalnie dodać
                                else // jeśli ma być rekurencja
                                    if (tmpEvent->fDelay >=
                                        5.0) // to musi mieć sensowny okres powtarzania
                                    if (tmpEvent->iQueued < 2)
                                    { // trzeba zrobić wyjątek, aby event mógł się sam dodać do
                                        // kolejki, raz już jest, ale będzie usunięty
                                        // pętla eventowa może być uruchomiona wiele razy, ale tylko
                                        // pierwsze uruchomienie zadziała
                                        tmpEvent->iQueued =
                                            0; // tymczasowo, aby był ponownie dodany do kolejki
                                        AddToQuery(tmpEvent, tmpEvent->Activator);
                                        tmpEvent->iQueued =
                                            2; // kolejny raz już absolutnie nie dodawać
                                    }
                            }
                    }
                    if (Global::iMultiplayer) // dajemy znać do serwera o wykonaniu
                        if ((tmpEvent->iFlags & conditional_anyelse) ==
                            0) // jednoznaczne tylko, gdy nie było else
                        {
                            if (tmpEvent->Activator)
                                WyslijEvent(tmpEvent->asName, tmpEvent->Activator->GetName());
                            else
                                WyslijEvent(tmpEvent->asName, "");
                        }
                }
            }
            break;
            case tp_WhoIs: // pobranie nazwy pociągu do komórki pamięci
                if (tmpEvent->iFlags & update_load)
                { // jeśli pytanie o ładunek
                    if (tmpEvent->iFlags & update_memadd) // jeśli typ pojazdu
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
                        tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
                            strdup(tmpEvent->Activator->MoverParameters->TypeName.c_str()), // typ pojazdu
                            0, // na razie nic
                            0, // na razie nic
                            tmpEvent->iFlags &
                                (update_memstring | update_memval1 | update_memval2));
#else
                        tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
                            tmpEvent->Activator->MoverParameters->TypeName, // typ pojazdu
                            0, // na razie nic
                            0, // na razie nic
                            tmpEvent->iFlags &
                                (update_memstring | update_memval1 | update_memval2));
#endif
                    else // jeśli parametry ładunku
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
                        tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
                            tmpEvent->Activator->MoverParameters->LoadType != "" ?
                                strdup(tmpEvent->Activator->MoverParameters->LoadType.c_str()) :
                                (char*)"none", // nazwa ładunku
                            tmpEvent->Activator->MoverParameters->Load, // aktualna ilość
                            tmpEvent->Activator->MoverParameters->MaxLoad, // maksymalna ilość
                            tmpEvent->iFlags &
                                (update_memstring | update_memval1 | update_memval2));
#else
                        tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
                            tmpEvent->Activator->MoverParameters->LoadType, // nazwa ładunku
                            tmpEvent->Activator->MoverParameters->Load, // aktualna ilość
                            tmpEvent->Activator->MoverParameters->MaxLoad, // maksymalna ilość
                            tmpEvent->iFlags &
                                (update_memstring | update_memval1 | update_memval2));
#endif
                }
                else if (tmpEvent->iFlags & update_memadd)
                { // jeśli miejsce docelowe pojazdu
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
                    tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
                        strdup(tmpEvent->Activator->asDestination.c_str()), // adres docelowy
                        tmpEvent->Activator->DirectionGet(), // kierunek pojazdu względem czoła
                        // składu (1=zgodny,-1=przeciwny)
                        tmpEvent->Activator->MoverParameters
                            ->Power, // moc pojazdu silnikowego: 0 dla wagonu
                        tmpEvent->iFlags & (update_memstring | update_memval1 | update_memval2));
#else
                    tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
                        tmpEvent->Activator->asDestination, // adres docelowy
                        tmpEvent->Activator->DirectionGet(), // kierunek pojazdu względem czoła składu (1=zgodny,-1=przeciwny)
                        tmpEvent->Activator->MoverParameters ->Power, // moc pojazdu silnikowego: 0 dla wagonu
                        tmpEvent->iFlags & (update_memstring | update_memval1 | update_memval2));
#endif
                }
                else if (tmpEvent->Activator->Mechanik)
                    if (tmpEvent->Activator->Mechanik->Primary())
                    { // tylko jeśli ktoś tam siedzi - nie powinno dotyczyć pasażera!
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
                        tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
							const_cast<char *>(tmpEvent->Activator->Mechanik->TrainName().c_str()),
                            tmpEvent->Activator->Mechanik->StationCount() -
                                tmpEvent->Activator->Mechanik
                                    ->StationIndex(), // ile przystanków do końca
                            tmpEvent->Activator->Mechanik->IsStop() ? 1 :
                                                                      0, // 1, gdy ma tu zatrzymanie
                            tmpEvent->iFlags);
#else
                        tmpEvent->Params[ 9 ].asMemCell->UpdateValues(
							tmpEvent->Activator->Mechanik->TrainName(),
                            tmpEvent->Activator->Mechanik->StationCount() -
                                tmpEvent->Activator->Mechanik
                                    ->StationIndex(), // ile przystanków do końca
                            tmpEvent->Activator->Mechanik->IsStop() ? 1 :
                                                                      0, // 1, gdy ma tu zatrzymanie
                            tmpEvent->iFlags);
#endif
                        WriteLog("Train detected: " + tmpEvent->Activator->Mechanik->TrainName());
                    }
                break;
            case tp_LogValues: // zapisanie zawartości komórki pamięci do logu
                if (tmpEvent->Params[9].asMemCell) // jeśli była podana nazwa komórki
                    WriteLog("Memcell \"" + tmpEvent->asNodeName + "\": " +
                             tmpEvent->Params[9].asMemCell->Text() + " " +
                             std::to_string(tmpEvent->Params[9].asMemCell->Value1()) + " " +
                             std::to_string(tmpEvent->Params[9].asMemCell->Value2()));
                else // lista wszystkich
                    for (TGroundNode *Current = nRootOfType[TP_MEMCELL]; Current;
                         Current = Current->nNext)
                        WriteLog("Memcell \"" + Current->asName + "\": " +
                                 Current->MemCell->Text() + " " + std::to_string(Current->MemCell->Value1()) + " " +
                                 std::to_string(Current->MemCell->Value2()));
                break;
            case tp_Voltage: // zmiana napięcia w zasilaczu (TractionPowerSource)
                if (tmpEvent->Params[9].psPower)
                { // na razie takie chamskie ustawienie napięcia zasilania
                    WriteLog("type: Voltage");
                    tmpEvent->Params[9].psPower->VoltageSet(tmpEvent->Params[0].asdouble);
                }
            case tp_Friction: // zmiana tarcia na scenerii
            { // na razie takie chamskie ustawienie napięcia zasilania
                WriteLog("type: Friction");
                Global::fFriction = (tmpEvent->Params[0].asdouble);
            }
            break;
            case tp_Message: // wyświetlenie komunikatu
                break;
            } // switch (tmpEvent->Type)
        } // if (tmpEvent->bEnabled)
        --tmpEvent->iQueued; // teraz moze być ponownie dodany do kolejki
        /*
          if (QueryRootEvent->eJoined) //jeśli jest kolejny o takiej samej nazwie
          {//to teraz jego dajemy do wykonania
           QueryRootEvent->eJoined->Next=QueryRootEvent->Next; //pamiętając o następnym z kolejki
           QueryRootEvent->eJoined->fStartTime=QueryRootEvent->fStartTime; //czas musi być ten sam,
          bo nie jest aktualizowany
           //QueryRootEvent->fStartTime=0;
           QueryRootEvent=QueryRootEvent->eJoined; //a wykonać ten doczepiony
          }
          else
          {//a jak nazwa jest unikalna, to kolejka idzie dalej
           //QueryRootEvent->fStartTime=0;
           QueryRootEvent=QueryRootEvent->Next; //NULL w skrajnym przypadku
          }
        */
    } // while
    return true;
}

void TGround::OpenGLUpdate(HDC hDC)
{
    SwapBuffers(hDC); // swap buffers (double buffering)
};

void TGround::UpdatePhys(double dt, int iter)
{ // aktualizacja fizyki stałym krokiem: dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeń
    for (TGroundNode *Current = nRootOfType[TP_TRACTIONPOWERSOURCE]; Current;
         Current = Current->nNext)
        Current->psTractionPowerSource->Update(dt * iter); // zerowanie sumy prądów
};

bool TGround::Update(double dt, int iter)
{ // aktualizacja animacji krokiem FPS: dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeń
    if (dt == 0.0)
    { // jeśli załączona jest pauza, to tylko obsłużyć ruch w kabinie trzeba
        return true;
    }
    // Ra: w zasadzie to trzeba by utworzyć oddzielną listę taboru do liczenia fizyki
    //    na którą by się zapisywały wszystkie pojazdy będące w ruchu
    //    pojazdy stojące nie potrzebują aktualizacji, chyba że np. ktoś im zmieni nastawę hamulca
    //    oddzielną listę można by zrobić na pojazdy z napędem, najlepiej posortowaną wg typu napędu
    if (iter > 1) // ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
    { // pierwsza iteracja i wyznaczenie stalych:
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
        { //
            Current->DynamicObject->MoverParameters->ComputeConstans();
            Current->DynamicObject->CoupleDist();
            Current->DynamicObject->UpdateForce(dt, dt, false);
        }
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
            Current->DynamicObject->FastUpdate(dt);
        // pozostale iteracje
        for (int i = 1; i < (iter - 1); ++i) // jeśli iter==5, to wykona się 3 razy
        {
            for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
                Current->DynamicObject->UpdateForce(dt, dt, false);
            for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
                Current->DynamicObject->FastUpdate(dt);
        }
        // ABu 200205: a to robimy tylko raz, bo nie potrzeba więcej
        // Winger 180204 - pantografy
        double dt1 = dt * iter; // całkowity czas
        UpdatePhys(dt1, 1);
        TAnimModel::AnimUpdate(dt1); // wykonanie zakolejkowanych animacji
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
        { // Ra: zmienić warunek na sprawdzanie pantografów w jednej zmiennej: czy pantografy i czy
            // podniesione
            if (Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType ==
                CurrentCollector)
                GetTraction(Current->DynamicObject); // poszukiwanie drutu dla pantografów
            Current->DynamicObject->UpdateForce(dt, dt1, true); //,true);
        }
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
            Current->DynamicObject->Update(dt, dt1); // Ra 2015-01: tylko tu przelicza sieć
        // trakcyjną
    }
    else
    { // jezeli jest tylko jedna iteracja
        UpdatePhys(dt, 1);
        TAnimModel::AnimUpdate(dt); // wykonanie zakolejkowanych animacji
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
        {
            if (Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType ==
                CurrentCollector)
                GetTraction(Current->DynamicObject);
            Current->DynamicObject->MoverParameters->ComputeConstans();
            Current->DynamicObject->CoupleDist();
            Current->DynamicObject->UpdateForce(dt, dt, true); //,true);
        }
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
            Current->DynamicObject->Update(dt, dt); // Ra 2015-01: tylko tu przelicza sieć trakcyjną
    }
    if (bDynamicRemove)
    { // jeśli jest coś do usunięcia z listy, to trzeba na końcu
        for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
            if (!Current->DynamicObject->bEnabled)
            {
                DynamicRemove(Current->DynamicObject); // usunięcie tego i podłączonych
                Current = nRootDynamic; // sprawdzanie listy od początku
            }
        bDynamicRemove = false; // na razie koniec
    }
    return true;
};

// Winger 170204 - szukanie trakcji nad pantografami
bool TGround::GetTraction(TDynamicObject *model)
{ // aktualizacja drutu zasilającego dla każdego pantografu, żeby odczytać napięcie
    // jeśli pojazd się nie porusza, to nie ma sensu przeliczać tego więcej niż raz
    double fRaParam; // parametr równania parametrycznego odcinka drutu
    double fVertical; // odległość w pionie; musi być w zasięgu ruchu "pionowego" pantografu
    double fHorizontal; // odległość w bok; powinna być mniejsza niż pół szerokości pantografu
    vector3 vLeft, vUp, vFront, dwys;
    vector3 pant0;
    vector3 vParam; // współczynniki równania parametrycznego drutu
    vector3 vStyk; // punkt przebicia drutu przez płaszczyznę ruchu pantografu
    vector3 vGdzie; // wektor położenia drutu względem pojazdu
    vFront = model->VectorFront(); // wektor normalny dla płaszczyzny ruchu pantografu
    vUp = model->VectorUp(); // wektor pionu pudła (pochylony od pionu na przechyłce)
    vLeft = model->VectorLeft(); // wektor odległości w bok (odchylony od poziomu na przechyłce)
    dwys = model->GetPosition(); // współrzędne środka pojazdu
    TAnimPant *p; // wskaźnik do obiektu danych pantografu
    for (int k = 0; k < model->iAnimType[ANIM_PANTS]; ++k)
    { // pętla po pantografach
        p = model->pants[k].fParamPants;
        if (k ? model->MoverParameters->PantRearUp : model->MoverParameters->PantFrontUp)
        { // jeśli pantograf podniesiony
            pant0 = dwys + (vLeft * p->vPos.z) + (vUp * p->vPos.y) + (vFront * p->vPos.x);
            if (p->hvPowerWire)
            { // jeżeli znamy drut z poprzedniego przebiegu
                int n = 30; //żeby się nie zapętlił
                while (p->hvPowerWire)
                { // powtarzane aż do znalezienia odpowiedniego odcinka na liście dwukierunkowej
                    // obliczamy wyraz wolny równania płaszczyzny (to miejsce nie jest odpowienie)
                    vParam = p->hvPowerWire->vParametric; // współczynniki równania parametrycznego
                    fRaParam = -DotProduct(pant0, vFront);
                    // podstawiamy równanie parametryczne drutu do równania płaszczyzny pantografu
                    // vFront.x*(t1x+t*vParam.x)+vFront.y*(t1y+t*vParam.y)+vFront.z*(t1z+t*vParam.z)+fRaDist=0;
                    fRaParam = -(DotProduct(p->hvPowerWire->pPoint1, vFront) + fRaParam) /
                               DotProduct(vParam, vFront);
                    if (fRaParam <
                        -0.001) // histereza rzędu 7cm na 70m typowego przęsła daje 1 promil
                        p->hvPowerWire = p->hvPowerWire->hvNext[0];
                    else if (fRaParam > 1.001)
                        p->hvPowerWire = p->hvPowerWire->hvNext[1];
                    else if (p->hvPowerWire->iLast & 3)
                    { // dla ostatniego i przedostatniego przęsła wymuszamy szukanie innego
                        p->hvPowerWire = NULL; // nie to, że nie ma, ale trzeba sprawdzić inne
                        // p->fHorizontal=fHorizontal; //zapamiętanie położenia drutu
                        break;
                    }
                    else if (p->hvPowerWire->hvParallel)
                    { // jeśli przęsło tworzy bieżnię wspólną, to trzeba sprawdzić pozostałe
                        p->hvPowerWire = NULL; // nie to, że nie ma, ale trzeba sprawdzić inne
                        // p->fHorizontal=fHorizontal; //zapamiętanie położenia drutu
                        break; // tymczasowo dla bieżni wspólnych poszukiwanie po całości
                    }
                    else
                    { // jeśli t jest w przedziale, wyznaczyć odległość wzdłuż wektorów vUp i vLeft
                        vStyk = p->hvPowerWire->pPoint1 + fRaParam * vParam; // punkt styku
                        // płaszczyzny z drutem
                        // (dla generatora łuku
                        // el.)
                        vGdzie = vStyk - pant0; // wektor
                        // odległość w pionie musi być w zasięgu ruchu "pionowego" pantografu
                        fVertical = DotProduct(
                            vGdzie, vUp); // musi się mieścić w przedziale ruchu pantografu
                        // odległość w bok powinna być mniejsza niż pół szerokości pantografu
                        fHorizontal = fabs(DotProduct(vGdzie, vLeft)) -
                                      p->fWidth; // to się musi mieścić w przedziale zależnym od
                        // szerokości pantografu
                        // jeśli w pionie albo w bok jest za daleko, to dany drut jest nieużyteczny
                        if (fHorizontal > 0) // 0.635 dla AKP-1 AKP-4E
                        { // drut wyszedł poza zakres roboczy, ale jeszcze jest nabieżnik -
                            // pantograf się unosi bez utraty prądu
                            if (fHorizontal > p->fWidthExtra) // czy wyszedł za nabieżnik
                            {
                                p->hvPowerWire = NULL; // dotychczasowy drut nie liczy się
                                // p->fHorizontal=fHorizontal; //zapamiętanie położenia drutu
                            }
                            else
                            { // problem jest, gdy nowy drut jest wyżej, wtedy pantograf odłącza się
                                // od starego, a na podniesienie do nowego potrzebuje czasu
                                p->PantTraction =
                                    fVertical +
                                    0.15 * fHorizontal / p->fWidthExtra; // na razie liniowo na
                                // nabieżniku, dokładność
                                // poprawi się później
                                // p->fHorizontal=fHorizontal; //zapamiętanie położenia drutu
                            }
                        }
                        else
                        { // po wyselekcjonowaniu drutu, przypisać go do toru, żeby nie trzeba było
                            // szukać
                            // dla 3 końcowych przęseł sprawdzić wszystkie dostępne przęsła
                            // bo mogą być umieszczone równolegle nad torem - połączyć w pierścień
                            // najlepiej, jakby odcinki równoległe były oznaczone we wpisach
                            // WriteLog("Drut: "+AnsiString(fHorizontal)+" "+AnsiString(fVertical));
                            p->PantTraction = fVertical;
                            // p->fHorizontal=fHorizontal; //zapamiętanie położenia drutu
                            break; // koniec pętli, aktualny drut pasuje
                        }
                    }
                    if (--n <= 0) // coś za długo to szukanie trwa
                        p->hvPowerWire = NULL;
                }
            }
            if (!p->hvPowerWire) // else nie, bo mógł zostać wyrzucony
            { // poszukiwanie po okolicznych sektorach
                int c = GetColFromX(dwys.x) + 1;
                int r = GetRowFromZ(dwys.z) + 1;
                TSubRect *tmp;
                TGroundNode *node;
                p->PantTraction = 5.0; // taka za duża wartość
                for (int j = r - 2; j <= r; j++)
                    for (int i = c - 2; i <= c; i++)
                    { // poszukiwanie po najbliższych sektorach niewiele da przy większym
                        // zagęszczeniu
                        tmp = FastGetSubRect(i, j);
                        if (tmp)
                        { // dany sektor może nie mieć nic w środku
                            for (node = tmp->nRenderWires; node;
                                 node = node->nNext3) // następny z grupy
                                if (node->iType ==
                                    TP_TRACTION) // w grupie tej są druty oraz inne linie
                                {
                                    vParam =
                                        node->hvTraction
                                            ->vParametric; // współczynniki równania parametrycznego
                                    fRaParam = -DotProduct(pant0, vFront);
                                    fRaParam = -(DotProduct(node->hvTraction->pPoint1, vFront) +
                                                 fRaParam) /
                                               DotProduct(vParam, vFront);
                                    if ((fRaParam >= -0.001) ? (fRaParam <= 1.001) : false)
                                    { // jeśli tylko jest w przedziale, wyznaczyć odległość wzdłuż
                                        // wektorów vUp i vLeft
                                        vStyk = node->hvTraction->pPoint1 +
                                                fRaParam * vParam; // punkt styku płaszczyzny z
                                        // drutem (dla generatora łuku
                                        // el.)
                                        vGdzie = vStyk - pant0; // wektor
                                        fVertical = DotProduct(
                                            vGdzie,
                                            vUp); // musi się mieścić w przedziale ruchu pantografu
                                        if (fVertical >= 0.0) // jeśli ponad pantografem (bo może
                                            // łapać druty spod wiaduktu)
                                            if (Global::bEnableTraction ?
                                                    fVertical < p->PantWys - 0.15 :
                                                    false) // jeśli drut jest niżej niż 15cm pod
                                            // ślizgiem
                                            { // przełączamy w tryb połamania, o ile jedzie;
                                                // (bEnableTraction) aby dało się jeździć na
                                                // koślawych
                                                // sceneriach
                                                fHorizontal = fabs(DotProduct(vGdzie, vLeft)) -
                                                              p->fWidth; // i do tego jeszcze
                                                // wejdzie pod ślizg
                                                if (fHorizontal <= 0.0) // 0.635 dla AKP-1 AKP-4E
                                                {
                                                    p->PantWys =
                                                        -1.0; // ujemna liczba oznacza połamanie
                                                    p->hvPowerWire = NULL; // bo inaczej się zasila
                                                    // w nieskończoność z
                                                    // połamanego
                                                    // p->fHorizontal=fHorizontal; //zapamiętanie
                                                    // położenia drutu
                                                    if (model->MoverParameters->EnginePowerSource
                                                            .CollectorParameters.CollectorsNo >
                                                        0) // liczba pantografów
                                                        --model->MoverParameters->EnginePowerSource
                                                              .CollectorParameters
                                                              .CollectorsNo; // teraz będzie
                                                    // mniejsza
                                                    if (DebugModeFlag)
                                                        ErrorLog(
                                                            "Pant. break: at " +
                                                            to_string(pant0.x, 2, 7) +
                                                            " " +
                                                            to_string(pant0.y, 2, 7) +
                                                            " " +
                                                            to_string(pant0.z, 2, 7));
                                                }
                                            }
                                            else if (fVertical < p->PantTraction) // ale niżej, niż
                                            // poprzednio
                                            // znaleziony
                                            {
                                                fHorizontal =
                                                    fabs(DotProduct(vGdzie, vLeft)) - p->fWidth;
                                                if (fHorizontal <= 0.0) // 0.635 dla AKP-1 AKP-4E
                                                { // to się musi mieścić w przedziale zaleznym od
                                                    // szerokości pantografu
                                                    p->hvPowerWire =
                                                        node->hvTraction; // jakiś znaleziony
                                                    p->PantTraction =
                                                        fVertical; // zapamiętanie nowej wysokości
                                                    // p->fHorizontal=fHorizontal; //zapamiętanie
                                                    // położenia drutu
                                                }
                                                else if (fHorizontal <
                                                         p->fWidthExtra) // czy zmieścił się w
                                                // zakresie nabieżnika?
                                                { // problem jest, gdy nowy drut jest wyżej, wtedy
                                                    // pantograf odłącza się od starego, a na
                                                    // podniesienie do nowego potrzebuje czasu
                                                    fVertical +=
                                                        0.15 * fHorizontal /
                                                        p->fWidthExtra; // korekta wysokości o
                                                    // nabieżnik - drut nad
                                                    // nabieżnikiem jest
                                                    // geometrycznie jakby nieco
                                                    // wyżej
                                                    if (fVertical <
                                                        p->PantTraction) // gdy po korekcie jest
                                                    // niżej, niż poprzednio
                                                    // znaleziony
                                                    { // gdyby to wystarczyło, to możemy go uznać
                                                        p->hvPowerWire =
                                                            node->hvTraction; // może być
                                                        p->PantTraction =
                                                            fVertical; // na razie liniowo na
                                                        // nabieżniku, dokładność
                                                        // poprawi się później
                                                        // p->fHorizontal=fHorizontal;
                                                        // //zapamiętanie położenia drutu
                                                    }
                                                }
                                            }
                                    } // warunek na parametr drutu <0;1>
                                } // pętla po drutach
                        } // sektor istnieje
                    } // pętla po sektorach
            } // koniec poszukiwania w sektorach
            if (!p->hvPowerWire) // jeśli drut nie znaleziony
                if (!Global::bLiveTraction) // ale można oszukiwać
                    model->pants[k].fParamPants->PantTraction = 1.4; // to dajemy coś tam dla picu
        } // koniec obsługi podniesionego
        else
            p->hvPowerWire = NULL; // pantograf opuszczony
    }
    // if (model->fWahaczeAmp<model->MoverParameters->DistCounter)
    //{//nieużywana normalnie zmienna ogranicza powtórzone logowania
    // model->fWahaczeAmp=model->MoverParameters->DistCounter;
    // ErrorLog(FloatToStrF(1000.0*model->MoverParameters->DistCounter,ffFixed,7,3)+","+FloatToStrF(p->PantTraction,ffFixed,7,3)+","+FloatToStrF(p->fHorizontal,ffFixed,7,3)+","+FloatToStrF(p->PantWys,ffFixed,7,3)+","+AnsiString(p->hvPowerWire?1:0));
    // //
    // if (p->fHorizontal>1.0)
    //{
    // //Global::iPause|=1; //zapauzowanie symulacji
    // Global::fTimeSpeed=1; //spowolnienie czasu do obejrzenia pantografu
    // return true; //łapacz
    //}
    //}
    return true;
};

bool TGround::RenderDL(vector3 pPosition)
{ // renderowanie scenerii z Display List - faza nieprzezroczystych
    glDisable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.45); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    ++TGroundRect::iFrameNumber; // zwięszenie licznika ramek (do usuwniania nadanimacji)
    CameraDirection.x = sin(Global::pCameraRotation); // wektor kierunkowy
    CameraDirection.z = cos(Global::pCameraRotation);
    int tr, tc;
    TGroundNode *node;
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_LIGHTING);
    int n = 2 * iNumSubRects; //(2*==2km) promień wyświetlanej mapy w sektorach
    int c = GetColFromX(pPosition.x);
    int r = GetRowFromZ(pPosition.z);
    TSubRect *tmp;
    for (node = srGlobal.nRenderHidden; node; node = node->nNext3)
        node->RenderHidden(); // rednerowanie globalnych (nie za często?)
    int i, j, k;
    // renderowanie czołgowe dla obiektów aktywnych a niewidocznych
    for (j = r - n; j <= r + n; j++)
        for (i = c - n; i <= c + n; i++)
            if ((tmp = FastGetSubRect(i, j)) != NULL)
            {
                tmp->LoadNodes(); // oznaczanie aktywnych sektorów
                for (node = tmp->nRenderHidden; node; node = node->nNext3)
                    node->RenderHidden();
                tmp->RenderSounds(); // jeszcze dźwięki pojazdów by się przydały, również
                // niewidocznych
            }
    // renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
    iRendered = 0; // ilość renderowanych sektorów
    vector3 direction;
    for (k = 0; k < Global::iSegmentsRendered; ++k) // sektory w kolejności odległości
    { // przerobione na użycie SectorOrder
        i = SectorOrder[k].x; // na starcie oba >=0
        j = SectorOrder[k].y;
        do
        {
            if (j <= 0)
                i = -i; // pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0
            // czwarty: j>=0, i>=0;
            j = -j; // i oraz j musi być zmienione wcześniej, żeby continue działało
            direction = vector3(i, 0, j); // wektor od kamery do danego sektora
            if (LengthSquared3(direction) > 5) // te blisko są zawsze wyświetlane
            {
                direction = SafeNormalize(direction); // normalizacja
                if (CameraDirection.x * direction.x + CameraDirection.z * direction.z < 0.55)
                    continue; // pomijanie sektorów poza kątem patrzenia
            }
            Rects[(i + c) / iNumSubRects][(j + r) / iNumSubRects]
                .RenderDL(); // kwadrat kilometrowy nie zawsze, bo szkoda FPS
            if ((tmp = FastGetSubRect(i + c, j + r)) != NULL)
                if (tmp->iNodeCount) // o ile są jakieś obiekty, bo po co puste sektory przelatywać
                    pRendered[iRendered++] = tmp; // tworzenie listy sektorów do renderowania
        } while ((i < 0) || (j < 0)); // są 4 przypadki, oprócz i=j=0
    }
    for (i = 0; i < iRendered; i++)
        pRendered[i]->RenderDL(); // renderowanie nieprzezroczystych
    return true;
}

bool TGround::RenderAlphaDL(vector3 pPosition)
{ // renderowanie scenerii z Display List - faza przezroczystych
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.04); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    TGroundNode *node;
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    TSubRect *tmp;
    // Ra: renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
    int i;
    for (i = iRendered - 1; i >= 0; --i) // od najdalszych
    { // przezroczyste trójkąty w oddzielnym cyklu przed modelami
        tmp = pRendered[i];
        for (node = tmp->nRenderRectAlpha; node; node = node->nNext3)
            node->RenderAlphaDL(); // przezroczyste modele
    }
    for (i = iRendered - 1; i >= 0; --i) // od najdalszych
    { // renderowanie przezroczystych modeli oraz pojazdów
        pRendered[i]->RenderAlphaDL();
    }
    glDisable(GL_LIGHTING); // linie nie powinny świecić
    for (i = iRendered - 1; i >= 0; --i) // od najdalszych
    { // druty na końcu, żeby się nie robiły białe plamy na tle lasu
        tmp = pRendered[i];
        for (node = tmp->nRenderWires; node; node = node->nNext3)
            node->RenderAlphaDL(); // druty
    }
    return true;
}

bool TGround::RenderVBO(vector3 pPosition)
{ // renderowanie scenerii z VBO - faza nieprzezroczystych
    glDisable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.45); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    ++TGroundRect::iFrameNumber; // zwięszenie licznika ramek
    CameraDirection.x = sin(Global::pCameraRotation); // wektor kierunkowy
    CameraDirection.z = cos(Global::pCameraRotation);
    int tr, tc;
    TGroundNode *node;
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_LIGHTING);
    int n = 2 * iNumSubRects; //(2*==2km) promień wyświetlanej mapy w sektorach
    int c = GetColFromX(pPosition.x);
    int r = GetRowFromZ(pPosition.z);
    TSubRect *tmp;
    for (node = srGlobal.nRenderHidden; node; node = node->nNext3)
        node->RenderHidden(); // rednerowanie globalnych (nie za często?)
    int i, j, k;
    // renderowanie czołgowe dla obiektów aktywnych a niewidocznych
    for (j = r - n; j <= r + n; j++)
        for (i = c - n; i <= c + n; i++)
        {
            if ((tmp = FastGetSubRect(i, j)) != NULL)
            {
                for (node = tmp->nRenderHidden; node; node = node->nNext3)
                    node->RenderHidden();
                tmp->RenderSounds(); // jeszcze dźwięki pojazdów by się przydały, również
                // niewidocznych
            }
        }
    // renderowanie progresywne - zależne od FPS oraz kierunku patrzenia
    iRendered = 0; // ilość renderowanych sektorów
    vector3 direction;
    for (k = 0; k < Global::iSegmentsRendered; ++k) // sektory w kolejności odległości
    { // przerobione na użycie SectorOrder
        i = SectorOrder[k].x; // na starcie oba >=0
        j = SectorOrder[k].y;
        do
        {
            if (j <= 0)
                i = -i; // pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0
            // czwarty: j>=0, i>=0;
            j = -j; // i oraz j musi być zmienione wcześniej, żeby continue działało
            direction = vector3(i, 0, j); // wektor od kamery do danego sektora
            if (LengthSquared3(direction) > 5) // te blisko są zawsze wyświetlane
            {
                direction = SafeNormalize(direction); // normalizacja
                if (CameraDirection.x * direction.x + CameraDirection.z * direction.z < 0.55)
                    continue; // pomijanie sektorów poza kątem patrzenia
            }
            Rects[(i + c) / iNumSubRects][(j + r) / iNumSubRects]
                .RenderVBO(); // kwadrat kilometrowy nie zawsze, bo szkoda FPS
            if ((tmp = FastGetSubRect(i + c, j + r)) != NULL)
                if (tmp->iNodeCount) // jeżeli są jakieś obiekty, bo po co puste sektory przelatywać
                    pRendered[iRendered++] = tmp; // tworzenie listy sektorów do renderowania
        } while ((i < 0) || (j < 0)); // są 4 przypadki, oprócz i=j=0
    }
    // dodać rednerowanie terenu z E3D - jedno VBO jest używane dla całego modelu, chyba że jest ich
    // więcej
    if (Global::pTerrainCompact)
        Global::pTerrainCompact->TerrainRenderVBO(TGroundRect::iFrameNumber);
    for (i = 0; i < iRendered; i++)
    { // renderowanie nieprzezroczystych
        pRendered[i]->RenderVBO();
    }
    return true;
}

bool TGround::RenderAlphaVBO(vector3 pPosition)
{ // renderowanie scenerii z VBO - faza przezroczystych
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.04); // im mniejsza wartość, tym większa ramka, domyślnie 0.1f
    TGroundNode *node;
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    TSubRect *tmp;
    int i;
    for (i = iRendered - 1; i >= 0; --i) // od najdalszych
    { // renderowanie przezroczystych trójkątów sektora
        tmp = pRendered[i];
        tmp->LoadNodes(); // ewentualne tworzenie siatek
        if (tmp->StartVBO())
        {
            for (node = tmp->nRenderRectAlpha; node; node = node->nNext3)
                if (node->iVboPtr >= 0)
                    node->RenderAlphaVBO(); // nieprzezroczyste obiekty terenu
            tmp->EndVBO();
        }
    }
    for (i = iRendered - 1; i >= 0; --i) // od najdalszych
        pRendered[i]->RenderAlphaVBO(); // przezroczyste modeli oraz pojazdy
    glDisable(GL_LIGHTING); // linie nie powinny świecić
    for (i = iRendered - 1; i >= 0; --i) // od najdalszych
    { // druty na końcu, żeby się nie robiły białe plamy na tle lasu
        tmp = pRendered[i];
        if (tmp->StartVBO())
        {
            for (node = tmp->nRenderWires; node; node = node->nNext3)
                node->RenderAlphaVBO(); // przezroczyste modele
            tmp->EndVBO();
        }
    }
    return true;
};

//---------------------------------------------------------------------------
void TGround::Navigate(std::string const &ClassName, UINT Msg, WPARAM wParam, LPARAM lParam)
{ // wysłanie komunikatu do sterującego
    HWND h = FindWindow(ClassName.c_str(), 0); // można by to zapamiętać
    if (h == 0)
        h = FindWindow(0, ClassName.c_str()); // można by to zapamiętać
    SendMessage(h, Msg, wParam, lParam);
};
//--------------------------------
void TGround::WyslijEvent(const std::string &e, const std::string &d)
{ // Ra: jeszcze do wyczyszczenia
    DaneRozkaz r;
    r.iSygn = 'EU07';
    r.iComm = 2; // 2 - event
    int i = e.length(), j = d.length();
    r.cString[0] = char(i);
    strcpy(r.cString + 1, e.c_str()); // zakończony zerem
    r.cString[i + 2] = char(j); // licznik po zerze kończącym
    strcpy(r.cString + 3 + i, d.c_str()); // zakończony zerem
    COPYDATASTRUCT cData;
    cData.dwData = 'EU07'; // sygnatura
    cData.cbData = 12 + i + j; // 8+dwa liczniki i dwa zera kończące
    cData.lpData = &r;
	//m7todo
    //Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + e + " sent" );
};
//---------------------------------------------------------------------------
void TGround::WyslijUszkodzenia(const std::string &t, char fl)
{ // wysłanie informacji w postaci pojedynczego tekstu
	DaneRozkaz r;
	r.iSygn = 'EU07';
	r.iComm = 13; // numer komunikatu
	int i = t.length();
	r.cString[0] = char(fl);
	r.cString[1] = char(i);
	strcpy(r.cString + 2, t.c_str()); // z zerem kończącym
	COPYDATASTRUCT cData;
	cData.dwData = 'EU07'; // sygnatura
	cData.cbData = 11 + i; // 8+licznik i zero kończące
	cData.lpData = &r;
	//m7todo
	//Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + t + " sent");
};
//---------------------------------------------------------------------------
void TGround::WyslijString(const std::string &t, int n)
{ // wysłanie informacji w postaci pojedynczego tekstu
    DaneRozkaz r;
    r.iSygn = 'EU07';
    r.iComm = n; // numer komunikatu
    int i = t.length();
    r.cString[0] = char(i);
    strcpy(r.cString + 1, t.c_str()); // z zerem kończącym
    COPYDATASTRUCT cData;
    cData.dwData = 'EU07'; // sygnatura
    cData.cbData = 10 + i; // 8+licznik i zero kończące
    cData.lpData = &r;
	//m7todo
    //Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + t + " sent");
};
//---------------------------------------------------------------------------
void TGround::WyslijWolny(const std::string &t)
{ // Ra: jeszcze do wyczyszczenia
    WyslijString(t, 4); // tor wolny
};
//--------------------------------
void TGround::WyslijNamiary(TGroundNode *t)
{ // wysłanie informacji o pojeździe - (float), długość ramki będzie zwiększana w miarę potrzeby
    // WriteLog("Wysylam pojazd");
    DaneRozkaz r;
    r.iSygn = 'EU07';
    r.iComm = 7; // 7 - dane pojazdu
    int i = 32, j = t->asName.length();
    r.iPar[0] = i; // ilość danych liczbowych
    r.fPar[1] = Global::fTimeAngleDeg / 360.0; // aktualny czas (1.0=doba)
    r.fPar[2] = t->DynamicObject->MoverParameters->Loc.X; // pozycja X
    r.fPar[3] = t->DynamicObject->MoverParameters->Loc.Y; // pozycja Y
    r.fPar[4] = t->DynamicObject->MoverParameters->Loc.Z; // pozycja Z
    r.fPar[5] = t->DynamicObject->MoverParameters->V; // prędkość ruchu X
    r.fPar[6] = t->DynamicObject->MoverParameters->nrot * M_PI *
                t->DynamicObject->MoverParameters->WheelDiameter; // prędkość obrotowa kóŁ
    r.fPar[7] = 0; // prędkość ruchu Z
    r.fPar[8] = t->DynamicObject->MoverParameters->AccS; // przyspieszenie X
    r.fPar[9] = t->DynamicObject->MoverParameters->AccN; // przyspieszenie Y //na razie nie
    r.fPar[10] = t->DynamicObject->MoverParameters->AccV; // przyspieszenie Z
    r.fPar[11] = t->DynamicObject->MoverParameters->DistCounter; // przejechana odległość w km
    r.fPar[12] = t->DynamicObject->MoverParameters->PipePress; // ciśnienie w PG
    r.fPar[13] = t->DynamicObject->MoverParameters->ScndPipePress; // ciśnienie w PZ
    r.fPar[14] = t->DynamicObject->MoverParameters->BrakePress; // ciśnienie w CH
    r.fPar[15] = t->DynamicObject->MoverParameters->Compressor; // ciśnienie w ZG
    r.fPar[16] = t->DynamicObject->MoverParameters->Itot; // Prąd całkowity
    r.iPar[17] = t->DynamicObject->MoverParameters->MainCtrlPos; // Pozycja NJ
    r.iPar[18] = t->DynamicObject->MoverParameters->ScndCtrlPos; // Pozycja NB
    r.iPar[19] = t->DynamicObject->MoverParameters->MainCtrlActualPos; // Pozycja jezdna
    r.iPar[20] = t->DynamicObject->MoverParameters->ScndCtrlActualPos; // Pozycja bocznikowania
    r.iPar[21] = t->DynamicObject->MoverParameters->ScndCtrlActualPos; // Pozycja bocznikowania
    r.iPar[22] = t->DynamicObject->MoverParameters->ResistorsFlag * 1 +
                 t->DynamicObject->MoverParameters->ConverterFlag * 2 +
                 +t->DynamicObject->MoverParameters->CompressorFlag * 4 +
                 t->DynamicObject->MoverParameters->Mains * 8 +
                 +t->DynamicObject->MoverParameters->DoorLeftOpened * 16 +
                 t->DynamicObject->MoverParameters->DoorRightOpened * 32 +
                 +t->DynamicObject->MoverParameters->FuseFlag * 64 +
                 t->DynamicObject->MoverParameters->DepartureSignal * 128;
    // WriteLog("Zapisalem stare");
    // WriteLog("Mam patykow "+IntToStr(t->DynamicObject->iAnimType[ANIM_PANTS]));
    for (int p = 0; p < 4; p++)
    {
        //   WriteLog("Probuje pant "+IntToStr(p));
        if (p < t->DynamicObject->iAnimType[ANIM_PANTS])
        {
            r.fPar[23 + p] = t->DynamicObject->pants[p].fParamPants->PantWys; // stan pantografów 4
            //     WriteLog("Zapisalem pant "+IntToStr(p));
        }
        else
        {
            r.fPar[23 + p] = -2;
            //     WriteLog("Nie mam pant "+IntToStr(p));
        }
    }
    // WriteLog("Zapisalem pantografy");
    for (int p = 0; p < 3; p++)
        r.fPar[27 + p] =
            t->DynamicObject->MoverParameters->ShowCurrent(p + 1); // amperomierze kolejnych grup
    // WriteLog("zapisalem prady");
    r.iPar[30] = t->DynamicObject->MoverParameters->WarningSignal; // trabienie
    r.fPar[31] = t->DynamicObject->MoverParameters->RunningTraction.TractionVoltage; // napiecie WN
    // WriteLog("Parametry gotowe");
    i <<= 2; // ilość bajtów
    r.cString[i] = char(j); // na końcu nazwa, żeby jakoś zidentyfikować
    strcpy(r.cString + i + 1, t->asName.c_str()); // zakończony zerem
    COPYDATASTRUCT cData;
    cData.dwData = 'EU07'; // sygnatura
    cData.cbData = 10 + i + j; // 8+licznik i zero kończące
    cData.lpData = &r;
    // WriteLog("Ramka gotowa");
	//m7todo
    //Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
    // WriteLog("Ramka poszla!");
	CommLog( Now() + " " + std::to_string(r.iComm) + " " + t->asName + " sent");
};
//
void TGround::WyslijObsadzone()
{   // wysłanie informacji o pojeździe
	DaneRozkaz2 r;
	r.iSygn = 'EU07';
	r.iComm = 12;   // kod 12
	for (int i=0; i<1984; ++i) r.cString[i] = 0;

	int i = 0;
	for (TGroundNode *Current = nRootDynamic; Current; Current = Current->nNext)
		if (Current->DynamicObject->Mechanik)
		{
			strcpy(r.cString + 64 * i, Current->DynamicObject->asName.c_str());
			r.fPar[16 * i + 4] = Current->DynamicObject->GetPosition().x;
			r.fPar[16 * i + 5] = Current->DynamicObject->GetPosition().y;
			r.fPar[16 * i + 6] = Current->DynamicObject->GetPosition().z;
			r.iPar[16 * i + 7] = Current->DynamicObject->Mechanik->GetAction();
			strcpy(r.cString + 64 * i + 32, Current->DynamicObject->GetTrack()->IsolatedName().c_str());
			strcpy(r.cString + 64 * i + 48, Current->DynamicObject->Mechanik->Timetable()->TrainName.c_str());
			i++;
			if (i>30) break;
		}
	while (i <= 30)
	{
		strcpy(r.cString + 64 * i, string("none").c_str());
		r.fPar[16 * i + 4] = 1;
		r.fPar[16 * i + 5] = 2;
		r.fPar[16 * i + 6] = 3;
		r.iPar[16 * i + 7] = 0;
		strcpy(r.cString + 64 * i + 32, string("none").c_str());
		strcpy(r.cString + 64 * i + 48, string("none").c_str());
		i++;
	}

	COPYDATASTRUCT cData;
	cData.dwData = 'EU07';     // sygnatura
	cData.cbData = 8 + 1984; // 8+licznik i zero kończące
	cData.lpData = &r;
	// WriteLog("Ramka gotowa");
	//m7todo
	//Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
	CommLog( Now() + " " + std::to_string(r.iComm) + " obsadzone" + " sent");
}

//--------------------------------
void TGround::WyslijParam(int nr, int fl)
{ // wysłanie parametrów symulacji w ramce (nr) z flagami (fl)
    DaneRozkaz r;
    r.iSygn = 'EU07';
    r.iComm = nr; // zwykle 5
    r.iPar[0] = fl; // flagi istotności kolejnych parametrów
    int i = 0; // domyślnie brak danych
    switch (nr)
    { // można tym przesyłać różne zestawy parametrów
    case 5: // czas i pauza
        r.fPar[1] = Global::fTimeAngleDeg / 360.0; // aktualny czas (1.0=doba)
        r.iPar[2] = Global::iPause; // stan zapauzowania
        i = 8; // dwa parametry po 4 bajty każdy
        break;
    }
    COPYDATASTRUCT cData;
    cData.dwData = 'EU07'; // sygnatura
    cData.cbData = 12 + i; // 12+rozmiar danych
    cData.lpData = &r;
	//m7todo
    //Navigate("TEU07SRK", WM_COPYDATA, (WPARAM)Global::hWnd, (LPARAM)&cData);
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TGround::RadioStop(vector3 pPosition)
{ // zatrzymanie pociągów w okolicy
    TGroundNode *node;
    TSubRect *tmp;
    int c = GetColFromX(pPosition.x);
    int r = GetRowFromZ(pPosition.z);
    int i, j;
    int n = 2 * iNumSubRects; // przeglądanie czołgowe okolicznych torów w kwadracie 4km×4km
    for (j = r - n; j <= r + n; j++)
        for (i = c - n; i <= c + n; i++)
            if ((tmp = FastGetSubRect(i, j)) != NULL)
                for (node = tmp->nRootNode; node != NULL; node = node->nNext2)
                    if (node->iType == TP_TRACK)
                        node->pTrack->RadioStop(); // przekazanie do każdego toru w każdym segmencie
};

TDynamicObject * TGround::DynamicNearest(vector3 pPosition, double distance, bool mech)
{ // wyszukanie pojazdu najbliższego względem (pPosition)
    TGroundNode *node;
    TSubRect *tmp;
    TDynamicObject *dyn = NULL;
    int c = GetColFromX(pPosition.x);
    int r = GetRowFromZ(pPosition.z);
    int i, j, k;
    double sqm = distance * distance, sqd; // maksymalny promien poszukiwań do kwadratu
    for (j = r - 1; j <= r + 1; j++) // plus dwa zewnętrzne sektory, łącznie 9
        for (i = c - 1; i <= c + 1; i++)
            if ((tmp = FastGetSubRect(i, j)) != NULL)
                for (node = tmp->nRootNode; node; node = node->nNext2) // następny z sektora
                    if (node->iType == TP_TRACK) // Ra: przebudować na użycie tabeli torów?
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
                        for( k = 0; k < node->pTrack->iNumDynamics; k++ )
                            if (mech ? (node->pTrack->Dynamics[k]->Mechanik != NULL) :
                                       true) // czy ma mieć obsadę
                                if ((sqd = SquareMagnitude(
                                         node->pTrack->Dynamics[k]->GetPosition() - pPosition)) <
                                    sqm)
                                {
                                    sqm = sqd; // nowa odległość
                                    dyn = node->pTrack->Dynamics[k]; // nowy lider
                                }
#else
                        for( auto dynamic : node->pTrack->Dynamics ) {
                            if( mech ? ( dynamic->Mechanik != nullptr ) : true ) {
                                // czy ma mieć obsadę
                                if( ( sqd = SquareMagnitude( dynamic->GetPosition() - pPosition ) ) < sqm ) {
                                    sqm = sqd; // nowa odległość
                                    dyn = dynamic; // nowy lider
                                }
                            }
                        }
#endif
    return dyn;
};
TDynamicObject * TGround::CouplerNearest(vector3 pPosition, double distance, bool mech)
{ // wyszukanie pojazdu, którego sprzęg jest najbliżej względem (pPosition)
    TGroundNode *node;
    TSubRect *tmp;
    TDynamicObject *dyn = NULL;
    int c = GetColFromX(pPosition.x);
    int r = GetRowFromZ(pPosition.z);
    int i, j, k;
    double sqm = distance * distance, sqd; // maksymalny promien poszukiwań do kwadratu
    for (j = r - 1; j <= r + 1; j++) // plus dwa zewnętrzne sektory, łącznie 9
        for (i = c - 1; i <= c + 1; i++)
            if ((tmp = FastGetSubRect(i, j)) != NULL)
                for (node = tmp->nRootNode; node; node = node->nNext2) // następny z sektora
                    if (node->iType == TP_TRACK) // Ra: przebudować na użycie tabeli torów?
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
                        for( k = 0; k < node->pTrack->iNumDynamics; k++ )
                            if (mech ? (node->pTrack->Dynamics[k]->Mechanik != NULL) :
                                       true) // czy ma mieć obsadę
                            {
                                if ((sqd = SquareMagnitude(
                                         node->pTrack->Dynamics[k]->HeadPosition() - pPosition)) <
                                    sqm)
                                {
                                    sqm = sqd; // nowa odległość
                                    dyn = node->pTrack->Dynamics[k]; // nowy lider
                                }
                                if ((sqd = SquareMagnitude(
                                         node->pTrack->Dynamics[k]->RearPosition() - pPosition)) <
                                    sqm)
                                {
                                    sqm = sqd; // nowa odległość
                                    dyn = node->pTrack->Dynamics[k]; // nowy lider
                                }
                            }
#else
                        for( auto dynamic : node->pTrack->Dynamics ) {
                            if( mech ? ( dynamic->Mechanik != nullptr ) : true ) {
                                // czy ma mieć obsadę
                                if( ( sqd = SquareMagnitude( dynamic->HeadPosition() - pPosition ) ) < sqm ) {
                                    sqm = sqd; // nowa odległość
                                    dyn = dynamic; // nowy lider
                                }
                                if( ( sqd = SquareMagnitude( dynamic->RearPosition() - pPosition ) ) < sqm ) {
                                    sqm = sqd; // nowa odległość
                                    dyn = dynamic; // nowy lider
                                }
                            }
                        }
#endif
    return dyn;
};
//---------------------------------------------------------------------------
void TGround::DynamicRemove(TDynamicObject *dyn)
{ // Ra: usunięcie pojazdów ze scenerii (gdy dojadą na koniec i nie sa potrzebne)
    TDynamicObject *d = dyn->Prev();
    if (d) // jeśli coś jest z przodu
        DynamicRemove(d); // zaczynamy od tego z przodu
    else
    { // jeśli mamy już tego na początku
        TGroundNode **n, *node;
        d = dyn; // od pierwszego
        while (d)
        {
            if (d->MyTrack)
                d->MyTrack->RemoveDynamicObject(d); // usunięcie z toru o ile nie usunięty
            n = &nRootDynamic; // lista pojazdów od początku
            // node=NULL; //nie znalezione
            while (*n ? (*n)->DynamicObject != d : false)
            { // usuwanie z listy pojazdów
                n = &((*n)->nNext); // sprawdzenie kolejnego pojazdu na liście
            }
            if ((*n)->DynamicObject == d)
            { // jeśli znaleziony
                node = (*n); // zapamiętanie węzła, aby go usunąć
                (*n) = node->nNext; // pominięcie na liście
                Global::TrainDelete(d);
                d = d->Next(); // przejście do kolejnego pojazdu, póki jeszcze jest
                delete node; // usuwanie fizyczne z pamięci
            }
            else
                d = NULL; // coś nie tak!
        }
    }
};

//---------------------------------------------------------------------------
void TGround::TerrainRead(std::string const &f){
    // Ra: wczytanie trójkątów terenu z pliku E3D
};

//---------------------------------------------------------------------------
void TGround::TerrainWrite()
{ // Ra: zapisywanie trójkątów terenu do pliku E3D
    if (Global::pTerrainCompact->TerrainLoaded())
        return; // jeśli zostało wczytane, to nie ma co dalej robić
    if (Global::asTerrainModel.empty())
        return;
    // Trójkąty są zapisywane kwadratami kilometrowymi.
    // Kwadrat 500500 jest na środku (od 0.0 do 1000.0 na OX oraz OZ).
    // Ewentualnie w numerowaniu kwadratów uwzględnic wpis //$g.
    // Trójkąty są grupowane w submodele wg tekstury.
    // Triangle_strip oraz triangle_fan są rozpisywane na pojedyncze trójkąty,
    // chyba że dla danej tekstury wychodzi tylko jeden submodel.
    TModel3d *m = new TModel3d(); // wirtualny model roboczy z oddzielnymi submodelami
    TSubModel *sk; // wskaźnik roboczy na submodel kwadratu
    TSubModel *st; // wskaźnik roboczy na submodel tekstury
    // Zliczamy kwadraty z trójkątami, ilość tekstur oraz wierzchołków.
    // Ilość kwadratów i ilość tekstur określi ilość submodeli.
    // int sub=0; //całkowita ilość submodeli
    // int ver=0; //całkowita ilość wierzchołków
    int i, j, k; // indeksy w pętli
    TGroundNode *Current;
    float8 *ver; // trójkąty
    TSubModel::iInstance = 0; // pozycja w tabeli wierzchołków liczona narastająco
    for (i = 0; i < iNumRects; ++i) // pętla po wszystkich kwadratach kilometrowych
        for (j = 0; j < iNumRects; ++j)
            if (Rects[i][j].iNodeCount)
            { // o ile są jakieś trójkąty w środku
                sk = new TSubModel(); // nowy submodel dla kawadratu
                // numer kwadratu XXXZZZ, przy czym X jest ujemne - XXX rośnie na wschód, ZZZ rośnie
                // na północ
                sk->NameSet( std::to_string(1000 * (500 + i - iNumRects / 2) + (500 + j - iNumRects / 2)).c_str() ); // nazwa=numer kwadratu
                m->AddTo(NULL, sk); // dodanie submodelu dla kwadratu
                for (Current = Rects[i][j].nRootNode; Current; Current = Current->nNext2)
                    if (Current->TextureID)
                        switch (Current->iType)
                        { // pętla po trójkątach - zliczanie wierzchołków, dodaje submodel dla
                        // każdej tekstury
                        case GL_TRIANGLES:
                            Current->iVboPtr = sk->TriangleAdd(
                                m, Current->TextureID,
                                Current->iNumVerts); // zwiększenie ilości trójkątów w submodelu
                            m->iNumVerts +=
                                Current->iNumVerts; // zwiększenie całkowitej ilości wierzchołków
                            break;
                        case GL_TRIANGLE_STRIP: // na razie nie, bo trzeba przerabiać na pojedyncze
                            // trójkąty
                            break;
                        case GL_TRIANGLE_FAN: // na razie nie, bo trzeba przerabiać na pojedyncze
                            // trójkąty
                            break;
                        }
                for (Current = Rects[i][j].nRootNode; Current; Current = Current->nNext2)
                    if (Current->TextureID)
                        switch (Current->iType)
                        { // pętla po trójkątach - dopisywanie wierzchołków
                        case GL_TRIANGLES:
                            // ver=sk->TrianglePtr(TTexturesManager::GetName(Current->TextureID).c_str(),Current->iNumVerts);
                            // //wskaźnik na początek
                            ver = sk->TrianglePtr(Current->TextureID, Current->iVboPtr,
                                                  Current->Ambient, Current->Diffuse,
                                                  Current->Specular); // wskaźnik na początek
                            // WriteLog("Zapis "+AnsiString(Current->iNumVerts)+" trójkątów w
                            // ("+AnsiString(i)+","+AnsiString(j)+") od
                            // "+AnsiString(Current->iVboPtr)+" dla
                            // "+AnsiString(Current->TextureID));
                            Current->iVboPtr = -1; // bo to było tymczasowo używane
                            for (k = 0; k < Current->iNumVerts; ++k)
                            { // przepisanie współrzędnych
                                ver[k].Point.x = Current->Vertices[k].Point.x;
                                ver[k].Point.y = Current->Vertices[k].Point.y;
                                ver[k].Point.z = Current->Vertices[k].Point.z;
                                ver[k].Normal.x = Current->Vertices[k].Normal.x;
                                ver[k].Normal.y = Current->Vertices[k].Normal.y;
                                ver[k].Normal.z = Current->Vertices[k].Normal.z;
                                ver[k].tu = Current->Vertices[k].tu;
                                ver[k].tv = Current->Vertices[k].tv;
                            }
                            break;
                        case GL_TRIANGLE_STRIP: // na razie nie, bo trzeba przerabiać na pojedyncze
                            // trójkąty
                            break;
                        case GL_TRIANGLE_FAN: // na razie nie, bo trzeba przerabiać na pojedyncze
                            // trójkąty
                            break;
                        }
            }
    m->SaveToBinFile(strdup(("models\\" + Global::asTerrainModel).c_str()));
};
//---------------------------------------------------------------------------

void TGround::TrackBusyList()
{ // wysłanie informacji o wszystkich zajętych odcinkach
    TGroundNode *Current;
    TTrack *Track;
    for (Current = nRootOfType[TP_TRACK]; Current; Current = Current->nNext)
        if (!Current->asName.empty()) // musi być nazwa
#ifdef EU07_USE_OLD_TTRACK_DYNAMICS_ARRAY
            if( Current->pTrack->iNumDynamics ) // osi to chyba nie ma jak policzyć
#else
            if( false == Current->pTrack->Dynamics.empty() )
#endif
                WyslijString(Current->asName, 8); // zajęty
};
//---------------------------------------------------------------------------

void TGround::IsolatedBusyList()
{ // wysłanie informacji o wszystkich odcinkach izolowanych
    TIsolated *Current;
    for (Current = TIsolated::Root(); Current; Current = Current->Next())
        if (Current->Busy()) // sprawdź zajętość
            WyslijString(Current->asName, 11); // zajęty
        else
            WyslijString(Current->asName, 10); // wolny
    WyslijString("none", 10); // informacja o końcu listy
};
//---------------------------------------------------------------------------

void TGround::IsolatedBusy(const std::string t)
{ // wysłanie informacji o odcinku izolowanym (t)
    // Ra 2014-06: do wyszukania użyć drzewka nazw
    TIsolated *Current;
    for (Current = TIsolated::Root(); Current; Current = Current->Next())
        if (Current->asName == t) // wyszukiwanie odcinka o nazwie (t)
            if (Current->Busy()) // sprawdź zajetość
            {
                WyslijString(Current->asName, 11); // zajęty
                return; // nie sprawdzaj dalszych
            }
    WyslijString(t, 10); // wolny
};
//---------------------------------------------------------------------------

void TGround::Silence(vector3 gdzie)
{ // wyciszenie wszystkiego w sektorach przed przeniesieniem kamery z (gdzie)
    int tr, tc;
    TGroundNode *node;
    int n = 2 * iNumSubRects; //(2*==2km) promień wyświetlanej mapy w sektorach
    int c = GetColFromX(gdzie.x); // sektory wg dotychczasowej pozycji kamery
    int r = GetRowFromZ(gdzie.z);
    TSubRect *tmp;
    int i, j, k;
    // renderowanie czołgowe dla obiektów aktywnych a niewidocznych
    for (j = r - n; j <= r + n; j++)
        for (i = c - n; i <= c + n; i++)
            if ((tmp = FastGetSubRect(i, j)) != NULL)
            { // tylko dźwięki interesują
                for (node = tmp->nRenderHidden; node; node = node->nNext3)
                    node->RenderHidden();
                tmp->RenderSounds(); // dźwięki pojazdów by się przydało wyłączyć
            }
};
//---------------------------------------------------------------------------
