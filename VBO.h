/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "shader.h"

#ifndef VBOH
#define VBOH

#define EU07_USE_OLD_VERTEXBUFFER

//---------------------------------------------------------------------------
class CVertNormTex
{
  public:
    float x = 0.0; // X wierzchołka
    float y = 0.0; // Y wierzchołka
    float z = 0.0; // Z wierzchołka
    float nx = 0.0; // X wektora normalnego
    float ny = 0.0; // Y wektora normalnego
    float nz = 0.0; // Z wektora normalnego
    float u = 0.0; // U mapowania
    float v = 0.0; // V mapowania

	void deserialize(std::istream&);
	void serialize(std::ostream&);
};

class CMesh
{ // wsparcie dla VBO
	GLuint vao;
	gl_program shader;
  public:
    int m_nVertexCount; // liczba wierzchołków
#ifdef EU07_USE_OLD_VERTEXBUFFER
    CVertNormTex *m_pVNT;
#else
    std::vector<CVertNormTex> m_pVNT;
#endif
    unsigned int m_nVBOVertices; // numer VBO z wierzchołkami
    CMesh();
    ~CMesh();
    void MakeArray(int n); // tworzenie tablicy z elementami VNT
    void BuildVBOs(bool del = true); // zamiana tablic na VBO
    void Clear(); // zwolnienie zasobów
    bool StartVBO();
    void EndVBO();
    bool StartColorVBO();
};

#endif
