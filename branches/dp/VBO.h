//---------------------------------------------------------------------------

#ifndef VBOH
#define VBOH
//---------------------------------------------------------------------------
class CVert         // Klasa wierzcho�ka
{
public:
 float x;         // Sk�adowa X
 float y;         // Sk�adowa Y
 float z;         // Sk�adowa Z
 //double x;         // Sk�adowa X
 //double y;         // Sk�adowa Y
 //double z;         // Sk�adowa Z
 __fastcall CVert() {};
 __fastcall CVert(double fX,double fY,double fZ) {x=fX; y=fY; z=fZ;};
};
class CVec         // Klasa wektora normalnego
{
public:
 float x;         // Sk�adowa X
 float y;         // Sk�adowa Y
 float z;         // Sk�adowa Z
};
//typedef CVert CVec;         // Obie nazwy s� synonimami
class CTexCoord         // Klasa wsp�rz�dnych tekstur
{
public:
 float u;         // Sk�adowa U
 float v;         // Sk�adowa V
};

class CMesh
{//wsparcie dla VBO
public:
 int m_nVertexCount;         // Ilo�� wierzcho�k�w
 // Dane siatki
 CVert *m_pVertices;         // Dane wierzcho�k�w
 CVec *m_pNormals;         // Dane wierzcho�k�w
 CTexCoord *m_pTexCoords;         // Wsp�rz�dne tekstur
 // Nazwy dla obiekt�w VBO
 unsigned int m_nVBOVertices;         // Nazwa VBO z wierzcho�kami
 unsigned int m_nVBONormals;
 unsigned int m_nVBOTexCoords;         // Nazwa VBO z koordynatami tekstur
 __fastcall CMesh();
 __fastcall ~CMesh();
 void __fastcall MakeArrays(int n); //tworzenie tablic
 void __fastcall BuildVBOs(); //zamiana tablic na VBO
 void __fastcall Release(); //zwolnienie zasob�w
 bool __fastcall StartVBO();
 void __fastcall EndVBO();
};
#endif
