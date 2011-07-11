//---------------------------------------------------------------------------

#ifndef VBOH
#define VBOH
//---------------------------------------------------------------------------
class CVertNormTex
{
public:
 float x;  //X wierzcho�ka
 float y;  //Y wierzcho�ka
 float z;  //Z wierzcho�ka
 float nx; //X wektora normalnego
 float ny; //Y wektora normalnego
 float nz; //Z wektora normalnego
 float u;  //U mapowania
 float v;  //V mapowania
};

class CMesh
{//wsparcie dla VBO
public:
 int m_nVertexCount; // ilo�� wierzcho�k�w
 CVertNormTex *m_pVNT;
 unsigned int m_nVBOVertices; //numer VBO z wierzcho�kami
 __fastcall CMesh();
 __fastcall ~CMesh();
 void __fastcall MakeArray(int n); //tworzenie tablicy z elementami VNT
 void __fastcall BuildVBOs(); //zamiana tablic na VBO
 void __fastcall Clear(); //zwolnienie zasob�w
 bool __fastcall StartVBO();
 void __fastcall EndVBO();
 bool __fastcall StartColorVBO();
};

#endif

