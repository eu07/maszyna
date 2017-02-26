/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef VBOH
#define VBOH
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
  public:
    int m_nVertexCount; // liczba wierzchołków
    CVertNormTex *m_pVNT;
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
