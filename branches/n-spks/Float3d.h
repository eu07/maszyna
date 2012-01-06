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
 void InitialRotate()
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


#endif
