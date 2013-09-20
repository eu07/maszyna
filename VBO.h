//---------------------------------------------------------------------------

#ifndef VBOH
#define VBOH
//---------------------------------------------------------------------------
class CVertNormTex
{
public:
 float x;  //X wierzcho³ka
 float y;  //Y wierzcho³ka
 float z;  //Z wierzcho³ka
 float nx; //X wektora normalnego
 float ny; //Y wektora normalnego
 float nz; //Z wektora normalnego
 float u;  //U mapowania
 float v;  //V mapowania
};

class CMesh
{//wsparcie dla VBO
public:
 int m_nVertexCount; //liczba wierzcho³ków
 CVertNormTex *m_pVNT;
 unsigned int m_nVBOVertices; //numer VBO z wierzcho³kami
 __fastcall CMesh();
 __fastcall ~CMesh();
 void __fastcall MakeArray(int n); //tworzenie tablicy z elementami VNT
 void __fastcall BuildVBOs(bool del=true); //zamiana tablic na VBO
 void __fastcall Clear(); //zwolnienie zasobów
 bool __fastcall StartVBO();
 void __fastcall EndVBO();
 bool __fastcall StartColorVBO();
};

#endif

