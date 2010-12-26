//---------------------------------------------------------------------------

#ifndef VBOH
#define VBOH
//---------------------------------------------------------------------------
class CVert         // Klasa wierzcho³ka
{
public:
 float x;         // Sk³adowa X
 float y;         // Sk³adowa Y
 float z;         // Sk³adowa Z
 //double x;         // Sk³adowa X
 //double y;         // Sk³adowa Y
 //double z;         // Sk³adowa Z
 __fastcall CVert() {};
 __fastcall CVert(double fX,double fY,double fZ) {x=fX; y=fY; z=fZ;};
};
class CVec         // Klasa wektora normalnego
{
public:
 float x;         // Sk³adowa X
 float y;         // Sk³adowa Y
 float z;         // Sk³adowa Z
};
//typedef CVert CVec;         // Obie nazwy s¹ synonimami
class CTexCoord         // Klasa wspó³rzêdnych tekstur
{
public:
 float u;         // Sk³adowa U
 float v;         // Sk³adowa V
};

class CMesh
{//wsparcie dla VBO
public:
 int m_nVertexCount;         // Iloœæ wierzcho³ków
 // Dane siatki
 CVert *m_pVertices;         // Dane wierzcho³ków
 CVec *m_pNormals;         // Dane wierzcho³ków
 CTexCoord *m_pTexCoords;         // Wspó³rzêdne tekstur
 // Nazwy dla obiektów VBO
 unsigned int m_nVBOVertices;         // Nazwa VBO z wierzcho³kami
 unsigned int m_nVBONormals;
 unsigned int m_nVBOTexCoords;         // Nazwa VBO z koordynatami tekstur
 __fastcall CMesh();
 __fastcall ~CMesh();
 void __fastcall MakeArrays(int n); //tworzenie tablic
 void __fastcall BuildVBOs(); //zamiana tablic na VBO
 void __fastcall Release(); //zwolnienie zasobów
 bool __fastcall StartVBO();
 void __fastcall EndVBO();
};
#endif
