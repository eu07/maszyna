//---------------------------------------------------------------------------

#ifndef float3dH
#define float3dH
#include <math.h>
//---------------------------------------------------------------------------

class float3
{//wapó³rzêdne wiercho³ka 3D o pojedynczej precyzji
public:
 float x,y,z;
 float3(void) {};
 __fastcall float3(float a,float b,float c) {x=a;y=b;z=c;};
 double inline __fastcall Length() const;
};

inline bool operator==(const float3& v1,const float3& v2)
{return (v1.x==v2.x&&v1.y==v2.y&&v1.z==v2.z);
};
inline float3& operator+=(float3& v1,const float3& v2)
{v1.x+=v2.x; v1.y+=v2.y; v1.z+=v2.z;
 return v1;
};
inline float3 operator-(const float3& v)
{return float3(-v.x,-v.y,-v.z);
};
inline float3 operator-(const float3 &v1,const float3 &v2)
{return float3(v1.x-v2.x,v1.y-v2.y,v1.z-v2.z);
};
inline float3 operator+(const float3 &v1,const float3 &v2)
{return float3(v1.x+v2.x,v1.y+v2.y,v1.z+v2.z);
};
double inline __fastcall float3::Length() const
{return sqrt(x*x+y*y+z*z);
};
inline float3 operator/(const float3& v, double k)
{return float3(v.x/k,v.y/k,v.z/k);
};
inline float3 SafeNormalize(const float3 &v)
{//bezpieczna normalizacja (wektor d³ugoœci 1.0)
 double l=v.Length();
 float3 retVal;
 if (l==0)
  retVal.x=retVal.y=retVal.z=0;
 else
  retVal=v/l;
 return retVal;
};
inline float3 CrossProduct(const float3& v1,const float3& v2)
{return float3(v1.y*v2.z-v1.z*v2.y,v2.x*v1.z-v2.z*v1.x,v1.x*v2.y-v1.y*v2.x);
}

class float4
{//kwaternion obrotu
public:
 float x,y,z,w;
 __fastcall float4() {x=y=z=0.f;w=1.f;};
 __fastcall float4(float a,float b,float c,float d) {x=a;y=b;z=c;w=d;};
 double inline float4::LengthSquared() const
 {return x*x+y*y+z*z+w*w;};
 double inline float4::Length() const
 {return sqrt(x*x+y*y+z*z+w*w);};
};
inline float4 operator*(const float4 &q1,const float4 &q2)
{//mno¿enie to prawie jak mno¿enie macierzy
 return float4
  (q1.w*q2.x+q1.x*q2.w+q1.y*q2.z-q1.z*q2.y,
   q1.w*q2.y+q1.y*q2.w+q1.z*q2.x-q1.x*q2.z,
   q1.w*q2.z+q1.z*q2.w+q1.x*q2.y-q1.y*q2.x,
   q1.w*q2.w-q1.x*q2.x-q1.y*q2.y-q1.z*q2.z
  );
}
inline float4 operator-(const float4& q)
{//sprzê¿ony; odwrotny tylko dla znormalizowanych!
 return float4(-q.x,-q.y,-q.z,q.w);
};
inline float4 operator-(const float4 &q1,const float4 &q2)
{//z odejmowaniem nie ma lekko
 return (-q1)*q2; //inwersja tylko dla znormalizowanych!
};
inline float4 operator+(const float4 &v1,const float4 &v2)
{return float4(v1.x+v2.x,v1.y+v2.y,v1.z+v2.z,v1.w+v2.w);};
inline float4 operator/(const float4& v, double k)
{return float4(v.x/k,v.y/k,v.z/k,v.w/k);};
inline float4 Normalize(const float4 &v)
{//bezpieczna normalizacja (wektor d³ugoœci 1.0)
 double l=v.LengthSquared();
 if (l==1.0)
  return v;
 if (l==0.0)
  return float4(); //wektor zerowy, w=1
 else
  return v/sqrt(l); //pierwiastek liczony tylko jeœli trzeba wykonaæ dzielenia
};
float Dot(const float4 &q1,const float4 &q2)
{//iloczyn skalarny
 return q1.x*q2.x+q1.y*q2.y+q1.z*q2.z+q1.w*q2.w;
}
inline float4& operator*=(float4& v1,double d)
{//mno¿enie przez skalar, jaki ma sens?
 v1.x*=d; v1.y*=d; v1.z*=d; v1.w*=d;
 return v1;
};
inline float4 Slerp(const float4 &q0,const float4 &q1,float t)
//void Slerp(QUATERNION *Out, const QUATERNION &q0, const QUATERNION &q1, float t)
{//interpolacja sweryczna
 float cosOmega=Dot(q0,q1);
 float4 new_q1(q1);
 if (cosOmega<0.0f)
 {//je¿eli s¹ niezgodne kierunki, jeden z nich trzeba zanegowaæ
  new_q1.x=-new_q1.x;
  new_q1.y=-new_q1.y;
  new_q1.z=-new_q1.z;
  new_q1.w=-new_q1.w;
  cosOmega=-cosOmega;
 }
 double k0,k1;
 if (cosOmega>0.9999f)
 {//jeœli jesteœmy z (t) na maksimum kosinusa, to tam prawie liniowo jest
  k0=1.0f-t;
  k1=t;
 }
 else
 {//a w ogólnym przypadku trzeba liczyæ na trygonometriê
  double sinOmega=sqrt(1.0f-cosOmega*cosOmega); //sinus z jedynki tryg.
  double omega=atan2(sinOmega,cosOmega); //wyznaczenie k¹ta
  double oneOverSinOmega=1.0f/sinOmega; //odwrotnoœæ sinusa, bo sinus w mianowniku
  k0=sin((1.0f-t)*omega)*oneOverSinOmega;
  k1=sin(t*omega)*oneOverSinOmega;
 }
 return float4
  (q0.x*k0+new_q1.x*k1,
   q0.y*k0+new_q1.y*k1,
   q0.z*k0+new_q1.z*k1,
   q0.w*k0+new_q1.w*k1
  );
}


struct float8
{//wiercho³ek 3D z wektorem normalnym i mapowaniem, pojedyncza precyzja
public:
 float3 Point;
 float3 Normal;
 float tu,tv;
};

class float4x4
{//macierz transformacji pojedynczej precyzji
 float e[16];
public:
 float4x4(void) {};
 float4x4(float f[16]) {for (int i=0;i<16;++i) e[i]=f[i];};
 float* __fastcall operator() (int i) { return &e[i<<2]; }
 const float* __fastcall readArray(void) { return e; }
 void __fastcall Identity()
 {for (int i=0;i<16;++i)
   e[i]=0;
  e[0]=e[5]=e[10]=e[15]=1.0f;
 }
 const float* operator[](int i) const {return &e[i<<2];};
 void __fastcall InitialRotate()
 {//taka specjalna rotacja, nie ma co ci¹gaæ trygonometrii
  float f;
  for (int i=0;i<16;i+=4)
  {e[i]=-e[i]; //zmiana znaku X
   f=e[i+1]; e[i+1]=e[i+2]; e[i+2]=f; //zamiana Y i Z
  }
 };
 inline float4x4& Rotation(double angle,float3 axis);
 inline bool IdentityIs()
 {//sprawdzenie jednostkowoœci
  for (int i=0;i<16;++i)
   if (e[i]!=((i%5)?0.0:1.0)) //jedynki tylko na 0, 5, 10 i 15
    return false;
  return true;
 }
 void __fastcall Quaternion(float4 *q);
 inline float3* TranslationGet()
 {return (float3*)(e+12);}
};

inline float3 operator*(const float4x4& m,const float3& v)
{//mno¿enie wektora przez macierz
 return float3(
  v.x*m[0][0]+v.y*m[1][0]+v.z*m[2][0]+m[3][0],
  v.x*m[0][1]+v.y*m[1][1]+v.z*m[2][1]+m[3][1],
  v.x*m[0][2]+v.y*m[1][2]+v.z*m[2][2]+m[3][2]
  );
}

inline float4x4& float4x4::Rotation(double angle,float3 axis)
{
 double c=cos(angle);
 double s=sin(angle);
 // One minus c (short name for legibility of formulai)
 double omc=(1-c);
 if (axis.Length()!=1.0f) axis=SafeNormalize(axis);
 double x = axis.x;
 double y = axis.y;
 double z = axis.z;
 double xs = x * s;
 double ys = y * s;
 double zs = z * s;
 double xyomc = x * y * omc;
 double xzomc = x * z * omc;
 double yzomc = y * z * omc;
 e[0] =x*x*omc+c;
 e[1] =xyomc+zs;
 e[2] =xzomc-ys;
 e[3] =0;
 e[4] =xyomc-zs;
 e[5] =y*y*omc+c;
 e[6] =yzomc+xs;
 e[7] =0;
 e[8] =xzomc+ys;
 e[9] =yzomc-xs;
 e[10]=z*z*omc+c;
 e[11]=0;
 e[12]=0;
 e[13]=0;
 e[14]=0;
 e[15]=1;
 return *this;
};

inline float4x4 operator*(const float4x4& m1, const float4x4& m2)
{//iloczyn macierzy
 float4x4 retVal;
 for (int x=0;x<4;++x)
  for (int y=0;y<4;++y)
  {
   retVal(x)[y]=0;
    for (int i=0;i<4;++i)
     retVal(x)[y]+=m1[i][y]*m2[x][i];
  }
 return retVal;
};

// From code in Graphics Gems; p. 766
inline float Det2x2(float a,float b,float c,float d)
{//obliczenie wyznacznika macierzy 2×2
 return a*d-b*c;
};

inline float Det3x3(float a1,float a2,float a3,
                    float b1,float b2,float b3,
                    float c1,float c2,float c3)
{//obliczenie wyznacznika macierzy 3×3
 return
  +a1*Det2x2(b2,b3,c2,c3)
  -b1*Det2x2(a2,a3,c2,c3)
  +c1*Det2x2(a2,a3,b2,b3);
};

float Det(const float4x4 &m)
{//obliczenie wyznacznika macierzy 4×4
 float a1=m[0][0],a2=m[1][0],a3=m[2][0],a4=m[3][0];
 float b1=m[0][1],b2=m[1][1],b3=m[2][1],b4=m[3][1];
 float c1=m[0][2],c2=m[1][2],c3=m[2][2],c4=m[3][2];
 float d1=m[0][3],d2=m[1][3],d3=m[2][3],d4=m[3][3];
 return
  +a1*Det3x3(b2,b3,b4,c2,c3,c4,d2,d3,d4)
  -b1*Det3x3(a2,a3,a4,c2,c3,c4,d2,d3,d4)
  +c1*Det3x3(a2,a3,a4,b2,b3,b4,d2,d3,d4)
  -d1*Det3x3(a2,a3,a4,b2,b3,b4,c2,c3,c4);
};

#endif
