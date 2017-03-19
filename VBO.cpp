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
#include "World.h"
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

void CVertNormTex::serialize(std::ostream &s)
{
	sn_utils::ls_float32(s, x);
	sn_utils::ls_float32(s, y);
	sn_utils::ls_float32(s, z);

	sn_utils::ls_float32(s, nx);
	sn_utils::ls_float32(s, ny);
	sn_utils::ls_float32(s, nz);

	sn_utils::ls_float32(s, u);
	sn_utils::ls_float32(s, v);
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
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nVBOVertices);
    glBufferData(GL_ARRAY_BUFFER, m_nVertexCount * sizeof(CVertNormTex), m_pVNT, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

    if (del)
        SafeDeleteArray(m_pVNT); // wierzchołki już się nie przydadzą
};

void CMesh::Clear()
{ // niewirtualne zwolnienie zasobów przez sprzątacz albo destruktor
    // inna nazwa, żeby nie mieszało się z funkcją wirtualną sprzątacza
    if (m_nVBOVertices) // jeśli było coś rezerwowane
    {
        glDeleteBuffers(1, &m_nVBOVertices);
		glDeleteVertexArrays(1, &vao);
    }
    m_nVBOVertices = 0;
    m_nVertexCount = -1; // do ponownego zliczenia
    SafeDeleteArray(m_pVNT); // usuwanie tablic, gdy były użyte do Vertex Array
};

extern TWorld World;

bool CMesh::StartVBO()
{ // początek rysowania elementów z VBO
    if (m_nVertexCount <= 0)
        return false; // nie ma nic do rysowania w ten sposób
    if (m_nVBOVertices)
		glBindVertexArray(vao);
	
	glUseProgram(World.shader);
	World.shader.copy_gl_mvp();
    return true; // można rysować z VBO
};

bool CMesh::StartColorVBO()
{ // początek rysowania punktów świecących z VBO
	return false; //m7todo
	/*
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
	*/
};

void CMesh::EndVBO()
{ // koniec użycia VBO
	glBindVertexArray(0);
	glUseProgram(0);
};
