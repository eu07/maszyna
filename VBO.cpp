/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "VBO.h"
#include "GL/glew.h"
#include "usefull.h"
#include "sn_utils.h"
//---------------------------------------------------------------------------

void CVertNormTex::deserialize(std::istream &s)
{
	x = sn_utils::ld_float32(s);
	y = sn_utils::ld_float32(s);
	z = sn_utils::ld_float32(s);

	nx = sn_utils::ld_float32(s);
	ny = sn_utils::ld_float32(s);
	nz = sn_utils::ld_float32(s);

	u = sn_utils::ld_float32(s);
	v = sn_utils::ld_float32(s);
}

CMesh::CMesh()
{ // utworzenie pustego obiektu
    m_pVNT = nullptr;
    m_nVertexCount = -1;
    m_nVBOVertices = 0; // nie zarezerwowane
};

CMesh::~CMesh()
{ // usuwanie obiektu
    Clear(); // zwolnienie zasobów
};

void CMesh::MakeArray(int n)
{ // tworzenie tablic
    m_nVertexCount = n;
    m_pVNT = new CVertNormTex[m_nVertexCount]; // przydzielenie pamięci dla tablicy
};

void CMesh::BuildVBOs(bool del)
{ // tworzenie VBO i kasowanie już niepotrzebnych tablic
    // pobierz numer VBO oraz ustaw go jako aktywny
    glGenBuffers(1, &m_nVBOVertices); // pobierz numer
    glBindBuffer(GL_ARRAY_BUFFER, m_nVBOVertices); // ustaw bufor jako aktualny
    glBufferData(GL_ARRAY_BUFFER, m_nVertexCount * sizeof(CVertNormTex), m_pVNT, GL_STATIC_DRAW);
    // WriteLog("Assigned VBO number "+AnsiString(m_nVBOVertices)+", vertices:
    // "+AnsiString(m_nVertexCount));
    if (del)
        SafeDeleteArray(m_pVNT); // wierzchołki już się nie przydadzą
};

void CMesh::Clear()
{ // niewirtualne zwolnienie zasobów przez sprzątacz albo destruktor
    // inna nazwa, żeby nie mieszało się z funkcją wirtualną sprzątacza
    if (m_nVBOVertices) // jeśli było coś rezerwowane
    {
        glDeleteBuffers(1, &m_nVBOVertices); // Free The Memory
        // WriteLog("Released VBO number "+AnsiString(m_nVBOVertices));
    }
    m_nVBOVertices = 0;
    m_nVertexCount = -1; // do ponownego zliczenia
    SafeDeleteArray(m_pVNT); // usuwanie tablic, gdy były użyte do Vertex Array
};

bool CMesh::StartVBO()
{ // początek rysowania elementów z VBO
    if (m_nVertexCount <= 0)
        return false; // nie ma nic do rysowania w ten sposób
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if (m_nVBOVertices)
    {
        glBindBuffer(GL_ARRAY_BUFFER_ARB, m_nVBOVertices);
        glVertexPointer( 3, GL_FLOAT, sizeof(CVertNormTex), static_cast<char *>(nullptr) ); // pozycje
		glNormalPointer( GL_FLOAT, sizeof( CVertNormTex ), static_cast<char *>( nullptr ) + 12 ); // normalne
		glTexCoordPointer( 2, GL_FLOAT, sizeof( CVertNormTex ), static_cast<char *>( nullptr ) + 24 ); // wierzchołki
    }
    return true; // można rysować z VBO
};

bool CMesh::StartColorVBO()
{ // początek rysowania punktów świecących z VBO
    if (m_nVertexCount <= 0)
        return false; // nie ma nic do rysowania w ten sposób
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    if (m_nVBOVertices)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_nVBOVertices);
		glVertexPointer( 3, GL_FLOAT, sizeof( CVertNormTex ), static_cast<char *>( nullptr ) ); // pozycje
        // glColorPointer(3,GL_UNSIGNED_BYTE,sizeof(CVertNormTex),((char*)NULL)+12); //kolory
		glColorPointer( 3, GL_FLOAT, sizeof( CVertNormTex ), static_cast<char *>( nullptr ) + 12 ); // kolory
    }
    return true; // można rysować z VBO
};

void CMesh::EndVBO()
{ // koniec użycia VBO
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    // glBindBuffer(GL_ARRAY_BUFFER,0); //takie coś psuje, mimo iż polecali użyć
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Ra: to na przyszłość
};
