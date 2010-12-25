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

#endif
