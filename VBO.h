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

#endif
