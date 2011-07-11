//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Model3d.h"
#include "Usefull.h"
#include "Texture.h"
#include "logs.h"
#include "Globals.h"
#include "Timer.h"
#include "mtable.hpp"

double fSquareDist=0;
int TSubModel::iInstance; //numer renderowanego egzemplarza obiektu

__fastcall TSubModel::TSubModel()
{
 FirstInit();
 eType=smt_Unknown;
 Vertices=NULL;
 iNumVerts=-1; //do sprawdzenia
 iVboPtr=-1;
 uiDisplayList=0;
 fLight=-1.0; //�wietcenie wy��czone
};

void __fastcall TSubModel::FirstInit()
{
 //Index=-1;
 v_RotateAxis=vector3(0,0,0);
 v_TransVector=vector3(0,0,0);
 f_Angle=0;
 b_Anim=at_None;
 b_aAnim=at_None;
 Visible=false;
 Matrix.Identity();
 Next=NULL;
 Child=NULL;
 TextureID=0;
 TexAlpha=false;
 iFlags=0;
 //TexHash=false;
 //Hits=NULL;
 //CollisionPts=NULL;
 //CollisionPtsCount=0;
 Transparency=0;
 bWire=false;
 fWireSize=0;
 fNearAttenStart=40;
 fNearAttenEnd=80;
 bUseNearAtten=false;
 iFarAttenDecay=0;
 fFarDecayRadius=100;
 fcosFalloffAngle=0.5;
 fcosHotspotAngle=0.3;
 fCosViewAngle=0;
 fSquareMaxDist=10000;
 fSquareMinDist=0;
};

__fastcall TSubModel::~TSubModel()
{
 if (uiDisplayList) glDeleteLists(uiDisplayList,1);
 //SafeDeleteArray(Indices);
 SafeDelete(Next);
 SafeDelete(Child);
 delete[] Vertices;
};

int __fastcall TSubModel::SeekFaceNormal(DWORD *Masks,int f,DWORD dwMask,vector3 *pt,GLVERTEX *Vertices)
{//szukanie punktu stycznego do (pt), zwraca numer wierzcho�ka, a nie tr�jk�ta
 int iNumFaces=iNumVerts/3; //bo maska powierzchni jest jedna na tr�jk�t
 GLVERTEX *p; //roboczy wska�nik
 for (int i=f;i<iNumFaces;++i) //p�tla po tr�jk�tach, od tr�jk�ta (f)
  if (Masks[i]&dwMask) //je�li wsp�lna maska powierzchni
  {p=Vertices+3*i;
   if (p->Point==*pt) return 3*i;
   if ((++p)->Point==*pt) return 3*i+1;
   if ((++p)->Point==*pt) return 3*i+2;
  }
 return -1; //nie znaleziono stycznego wierzcho�ka
}

float emm1[]={1,1,1,0};
float emm2[]={0,0,0,1};

inline double readIntAsDouble(cParser& parser,int base=255)
{
 int value;
 parser.getToken(value);
 return double(value)/base;
};

template <typename ColorT>
inline void readColor(cParser& parser,ColorT* color)
{
 parser.ignoreToken();
 color[0]=readIntAsDouble(parser);
 color[1]=readIntAsDouble(parser);
 color[2]=readIntAsDouble(parser);
};

inline void readMatrix(cParser& parser,matrix4x4& matrix)
{//Ra: wczytanie transforma
 for (int x=0;x<=3;x++) //wiersze
  for (int y=0;y<=3;y++) //kolumny
   parser.getToken(matrix(x)[y]);
};

int __fastcall TSubModel::Load(cParser& parser,TModel3d *Model,int Pos)
{//Ra: VBO tworzone na poziomie modelu, a nie submodeli
 iNumVerts=0;
 iVboPtr=Pos; //pozycja w VBO
 //TMaterialColorf Ambient,Diffuse,Specular;
 f4Ambient[0]=f4Ambient[1]=f4Ambient[2]=f4Ambient[3]=1.0; //{1,1,1,1};
 f4Diffuse[0]=f4Diffuse[1]=f4Diffuse[2]=f4Diffuse[3]=1.0; //{1,1,1,1};
 f4Specular[0]=f4Specular[1]=f4Specular[2]=0.0; f4Specular[3]=1.0; //{0,0,0,1};
 //GLuint TextureID;
 //char *extName;
 if (!parser.expectToken("type:"))
  Error("Model type parse failure!");
 {
  std::string type;
  parser.getToken(type);
  if (type=="mesh")
   eType=smt_Mesh; //submodel - tr�jkaty
  else if (type=="point")
   eType=smt_Point; //co to niby jest?
  else if (type=="freespotlight")
   eType=smt_FreeSpotLight; //�wiate�ko
  else if (type=="text")
   eType=smt_Text; //wy�wietlacz tekstowy (generator napis�w)
  else if (type=="stars")
   eType=smt_Stars; //wiele punkt�w �wietlnych
 };
 parser.ignoreToken();
 parser.getToken(Name); //ze zmian� na ma�e!
 if (parser.expectToken("anim:")) //Ra: ta informacja by si� przyda�a!
 {//rodzaj animacji
  std::string type;
  parser.getToken(type);
  if (type!="false")
  {iFlags|=0x4000; //jak animacja, to trzeba przechowywa� macierz zawsze
   if      (type=="seconds_jump")  b_Anim=b_aAnim=at_SecondsJump; //sekundy z przeskokiem
   else if (type=="minutes_jump")  b_Anim=b_aAnim=at_MinutesJump; //minuty z przeskokiem
   else if (type=="hours_jump")    b_Anim=b_aAnim=at_HoursJump; //godziny z przeskokiem
   else if (type=="hours24_jump")  b_Anim=b_aAnim=at_Hours24Jump; //godziny z przeskokiem
   else if (type=="seconds")       b_Anim=b_aAnim=at_Seconds; //minuty p�ynnie
   else if (type=="minutes")       b_Anim=b_aAnim=at_Minutes; //minuty p�ynnie
   else if (type=="hours")         b_Anim=b_aAnim=at_Hours; //godziny p�ynnie
   else if (type=="hours24")       b_Anim=b_aAnim=at_Hours24; //godziny p�ynnie
   else if (type=="billboard")     b_Anim=b_aAnim=at_Billboard; //obr�t w pionie do kamery
   else if (type=="wind")          b_Anim=b_aAnim=at_Wind; //ruch pod wp�ywem wiatru
   else if (type=="sky")           b_Anim=b_aAnim=at_Sky; //aniamacja nieba
   else b_Anim=b_aAnim=at_Undefined; //nieznana forma animacji
  }
 }
 if (eType==smt_Mesh) readColor(parser,f4Ambient); //ignoruje token przed
 readColor(parser,f4Diffuse);
 if (eType==smt_Mesh) readColor(parser,f4Specular);
 parser.ignoreTokens(1); //zignorowanie nazwy "SelfIllum:"
 {
  std::string light;
  parser.getToken(light);
  if (light=="true")
   fLight=2.0; //zawsze �wieci
  else if (light=="false")
   fLight=-1.0; //zawsze ciemy
  else
   fLight=atof(light.c_str());
 };
 if (eType==smt_FreeSpotLight)
 {
  if (!parser.expectToken("nearattenstart:"))
   Error("Model light parse failure!");
  parser.getToken(fNearAttenStart);
  parser.ignoreToken();
  parser.getToken(fNearAttenEnd);
  parser.ignoreToken();
  bUseNearAtten=parser.expectToken("true");
  parser.ignoreToken();
  parser.getToken(iFarAttenDecay);
  parser.ignoreToken();
  parser.getToken(fFarDecayRadius);
  parser.ignoreToken();
  parser.getToken(fcosFalloffAngle);
  fcosFalloffAngle=cos(fcosFalloffAngle * M_PI / 180);
  parser.ignoreToken();
  parser.getToken(fcosHotspotAngle);
  fcosHotspotAngle=cos(fcosHotspotAngle * M_PI / 180);
  iNumVerts=1;
  iFlags|=2; //rysowane w cyklu nieprzezroczystych
 }
 else if (eType==smt_Mesh)
 {
  parser.ignoreToken();
  bWire=parser.expectToken("true");
  parser.ignoreToken();
  parser.getToken(fWireSize);
  parser.ignoreToken();
  Transparency=readIntAsDouble(parser,100.0f);
  if (!parser.expectToken("map:"))
   Error("Model map parse failure!");
  std::string texture;
  parser.getToken(texture);
  if (texture=="none")
  {//rysowanie podanym kolorem
   TextureID=0;
   iFlags|=2; //rysowane w cyklu nieprzezroczystych
  }
  else if (texture.find("replacableskin")!=texture.npos)
  {// McZapkie-060702: zmienialne skory modelu
   TextureID=-1;
   iFlags|=1; //zmienna tekstura
  }
  else
  {//jesli tylko nazwa pliku to dawac biezaca sciezke do tekstur
   if (texture.find_first_of("/\\")==texture.npos)
    texture.insert(0,Global::asCurrentTexturePath.c_str());
   TextureID=TTexturesManager::GetTextureID(texture);
   TexAlpha=TTexturesManager::GetAlpha(TextureID);
   iFlags|=TexAlpha?4:2; //2-nieprzezroczysta, 4-przezroczysta
  };
 }
 else iFlags|=2;
 parser.ignoreToken();
 parser.getToken(fSquareMaxDist);
 fSquareMaxDist*=fSquareMaxDist;
 parser.ignoreToken();
 parser.getToken(fSquareMinDist);
 fSquareMinDist*=fSquareMinDist;
 parser.ignoreToken();
 readMatrix(parser,Matrix); //wczytanie transform
 //if ((iFlags&0x4000)==0) //o ile nie ma animacji
 for (int i=0;i<16;++i)
  if (Matrix.readArray()[i]!=((i%5)?0.0:1.0)) //jedynki tylko na 0, 5, 10 i 15
  {
   iFlags|=0x8000; //transform niejedynkowy - trzeba przechowa�
   break;
  }
 int iNumFaces; //ilo�� tr�jk�t�w
 DWORD *sg; //maski przynale�no�ci tr�jk�t�w do powierzchni
 if (eType==smt_Mesh)
 {//wczytywanie wierzcho�k�w
  parser.ignoreToken();
  parser.getToken(iNumVerts);
  if (iNumVerts%3)
  {
   iNumVerts=0;
   Error("Mesh error, iNumVertices%3!=0");
   return 0;
  }
  Vertices=new GLVERTEX[iNumVerts];
  iNumFaces=iNumVerts/3;
  sg=new DWORD[iNumFaces]; //maski powierzchni
  for (int i=0;i<iNumVerts;i++)
  {//Ra: z konwersj� na uk�ad scenerii - b�dzie wydajniejsze wy�wietlanie
   if (i%3==0)
    parser.getToken(sg[i/3]); //maska powierzchni tr�jk�ta
   parser.getToken(Vertices[i].Point.x);
   parser.getToken(Vertices[i].Point.y);
   parser.getToken(Vertices[i].Point.z);
   //Vertices[i].Normal=vector3(0,0,0); //b�dzie liczony potem
   parser.getToken(Vertices[i].tu);
   parser.getToken(Vertices[i].tv);
   if (i%3==2) //je�eli wczytano 3 punkty
   {if (Vertices[i  ].Point==Vertices[i-1].Point ||
        Vertices[i-1].Point==Vertices[i-2].Point ||
        Vertices[i-2].Point==Vertices[i  ].Point)
    {//je�eli punkty si� nak�adaj� na siebie
     --iNumFaces; //o jeden tr�jk�t mniej
     iNumVerts-=3; //czyli o 3 wierzcho�ki
     i-=3; //wczytanie kolejnego w to miejsce
     WriteLog(AnsiString("Degenerated triangle ignored in: \"")+Name.c_str()+"\"");
    }
    if (((Vertices[i  ].Point-Vertices[i-1].Point).Length()>2000.0) ||
        ((Vertices[i-1].Point-Vertices[i-2].Point).Length()>2000.0) ||
        ((Vertices[i-2].Point-Vertices[i  ].Point).Length()>2000.0))
    {//je�eli s� dalej ni� 2km od siebie
     --iNumFaces; //o jeden tr�jk�t mniej
     iNumVerts-=3; //czyli o 3 wierzcho�ki
     i-=3; //wczytanie kolejnego w to miejsce
     WriteLog(AnsiString("Too large triangle ignored in: \"")+Name.c_str()+"\"");
    }
   }
  }
  int i; //indeks dla tr�jk�t�w
  vector3 *n=new vector3[iNumFaces]; //tablica wektor�w normalnych dla tr�jk�t�w
  for (i=0;i<iNumFaces;i++) //p�tla po tr�jk�tach - b�dzie szybciej, jak wst�pnie przeliczymy normalne tr�jk�t�w
   n[i]=SafeNormalize(CrossProduct(Vertices[i*3].Point-Vertices[i*3+1].Point,Vertices[i*3].Point-Vertices[i*3+2].Point));
  int v; //indeks dla wierzcho�k�w
  int *wsp=new int[iNumVerts]; //z kt�rego wierzcho�ka kopiowa� wektor normalny
  for (v=0;v<iNumVerts;v++)
   wsp[v]=-1; //wektory normalne nie s� policzone
  int f; //numer tr�jk�ta stycznego
  vector3 norm; //roboczy wektor normalny
  for (v=0;v<iNumVerts;v++)
  {//p�tla po wierzcho�kach tr�jk�t�w
   if (wsp[v]>=0) //je�li ju� by� liczony wektor normalny z u�yciem tego wierzcho�ka
    Vertices[v].Normal=Vertices[wsp[v]].Normal; //to wystarczy skopiowa� policzony wcze�niej
   else
   {//inaczej musimy dopiero policzy�
    i=v/3; //numer tr�jk�ta
    norm=vector3(0,0,0); //liczenie zaczynamy od zera
    f=v; //zaczynamy dodawanie wektor�w normalnych od w�asnego
    while (f>=0)
    {//sumowanie z wektorem normalnym s�siada (w��cznie ze sob�)
     wsp[f]=v; //informacja, �e w tym wierzcho�ku jest ju� policzony wektor normalny
     norm+=n[f/3];
     f=SeekFaceNormal(sg,f/3+1,sg[i],&Vertices[v].Point,Vertices); //i szukanie od kolejnego tr�jk�ta
    }
    Vertices[v].Normal=SafeNormalize(norm); //przepisanie do wierzcho�ka tr�jk�ta
   }
  }
  delete[] wsp;
  delete[] n;
  delete[] sg;
 }
 else if (eType==smt_Stars)
 {//punkty �wiec�ce dook�lnie - sk�adnia jak dla smt_Mesh
  parser.ignoreToken();
  parser.getToken(iNumVerts);
  Vertices=new GLVERTEX[iNumVerts];
  int i,j;
  for (i=0;i<iNumVerts;i++)
  {
   if (i%3==0)
    parser.ignoreToken(); //maska powierzchni tr�jk�ta
   parser.getToken(Vertices[i].Point.x);
   parser.getToken(Vertices[i].Point.y);
   parser.getToken(Vertices[i].Point.z);
   parser.getToken(j); //zakodowany kolor
   parser.ignoreToken();
   Vertices[i].Normal.x=((j    )&0xFF)/255.0; //R
   Vertices[i].Normal.y=((j>> 8)&0xFF)/255.0; //G
   Vertices[i].Normal.z=((j>>16)&0xFF)/255.0; //B
  }
 }
 if (!Global::bUseVBO)
 {//Ra: przy VBO to si� nie przyda
  iFlags|=0x4000; //wy��czenie przeliczania wierzcho�k�w, bo nie s� zachowane
  if (eType==smt_Mesh)
  {
#ifdef USE_VERTEX_ARRAYS
   // ShaXbee-121209: przekazywanie wierzcholkow hurtem
   glVertexPointer(3,GL_DOUBLE,sizeof(GLVERTEX),&Vertices[0].Point.x);
   glNormalPointer(GL_DOUBLE,sizeof(GLVERTEX),&Vertices[0].Normal.x);
   glTexCoordPointer(2,GL_FLOAT,sizeof(GLVERTEX),&Vertices[0].tu);
#endif
   uiDisplayList=glGenLists(1);
   glNewList(uiDisplayList,GL_COMPILE);
   glColor3fv(f4Diffuse);   //McZapkie-240702: zamiast ub
#ifdef USE_VERTEX_ARRAYS
   glDrawArrays(GL_TRIANGLES,0,iNumVerts);
#else
   glBegin(bWire?GL_LINES:GL_TRIANGLES);
   for (int i=0;i<iNumVerts;i++)
   {
    glNormal3dv(&Vertices[i].Normal.x);
    glTexCoord2f(Vertices[i].tu,Vertices[i].tv);
    glVertex3dv(&Vertices[i].Point.x);
   };
   glEnd();
#endif
   glEndList();
  }
  else if (eType==smt_FreeSpotLight)
  {
   uiDisplayList=glGenLists(1);
   glNewList(uiDisplayList,GL_COMPILE);
   glBindTexture(GL_TEXTURE_2D,0);
//     if (eType==smt_FreeSpotLight)
//      {
//       if (iFarAttenDecay==0)
//         glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);
//      }
//      else
//TODO: poprawic zeby dzialalo
   //glColor3f(f4Diffuse[0],f4Diffuse[1],f4Diffuse[2]);
   glColorMaterial(GL_FRONT,GL_EMISSION);
   glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
   glBegin(GL_POINTS);
    glVertex3f(0,0,0);
   glEnd();
   glEnable(GL_LIGHTING);
   glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
   glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
   glEndList();
  }
  else if (eType==smt_Stars)
  {//punkty �wiec�ce dook�lnie
   uiDisplayList=glGenLists(1);
   glNewList(uiDisplayList,GL_COMPILE);
   glBindTexture(GL_TEXTURE_2D,0); //tekstury nie ma
   glColorMaterial(GL_FRONT,GL_EMISSION);
   glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
   glBegin(GL_POINTS);
   for (int i=0;i<iNumVerts;i++)
   {
    glColor3d(Vertices[i].Normal.x,Vertices[i].Normal.y,Vertices[i].Normal.z);
    glVertex3dv(&Vertices[i].Point.x);
   };
   glEnd();
   glEnable(GL_LIGHTING);
   glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
   glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
   glEndList();
  }
  SafeDeleteArray(Vertices); //przy VBO musz� zosta� do za�adowania ca�ego modelu
 }
 Visible=true; //si� potem wy��czy w razie potrzeby
 return iNumVerts; //do okre�lenia wielko�ci VBO
};

void __fastcall TSubModel::InitialRotate(bool doit)
{//konwersja uk�adu wsp�rz�dnych na zgodny ze sceneri�
 if (Next) Next->InitialRotate(doit);
 if (iFlags&0xC000) //je�li jest animacja albo niejednostkowy
 {//niejednostkowy transform jest mno�ony i wystarczy zabawy
  if (doit)
  {//obr�t lewostronny
   matrix4x4 *mat,tmp;
   mat=GetMatrix(); //transform submodelu
   tmp.Identity();
   tmp.Rotation(M_PI/2,vector3(1,0,0)); //obr�t wzgl�dem osi OX o 90�
   (*mat)=tmp*(*mat);
   tmp.Identity();
   tmp.Rotation(M_PI,vector3(0,0,1)); //obr�t wzgl�dem osi OZ o 90�
   (*mat)=tmp*(*mat);
  }
  if (Child)
   Child->InitialRotate(false); //potomnych nie obracamy ju�, tylko przegl�damy
  else
   if ((iFlags&0xC000)==0x8000) //o ile nie ma animacji
   {//jak nie ma potomnych, mo�na wymno�y� przez transform i wyjedynkowa� go
    matrix4x4 *mat=GetMatrix(); //transform submodelu
    if (Vertices)
     for (int i=0;i<iNumVerts;++i)
      Vertices[i].Point=(*mat)*Vertices[i].Point;
    mat->Identity(); //jedynkowanie transformu po przeliczeniu wierzcho�k�w
    iFlags&=~0x8000; //transform jedynkowy
   }
 }
 else //jak jest jednostkowy i nie ma animacji
  if (doit)
  {//je�li jest jednostkowy transform, to przeliczamy wierzcho�ki, a mno�enie podajemy dalej
   double t;
   if (Vertices)
    for (int i=0;i<iNumVerts;++i)
    {
     Vertices[i].Point.x=-Vertices[i].Point.x; //zmiana znaku X
     t=Vertices[i].Point.y; //zamiana Y i Z
     Vertices[i].Point.y=Vertices[i].Point.z;
     Vertices[i].Point.z=t;
    }
   if (Child) Child->InitialRotate(doit); //potomne ewentualnie obr�cimy
  }
};

void __fastcall TSubModel::AddChild(TSubModel *SubModel)
{//dodanie submodelu potemnego (uzale�nionego)
 if (Child==NULL)
  Child=SubModel;
 else
  Child->AddNext(SubModel);
};

void __fastcall TSubModel::AddNext(TSubModel *SubModel)
{//dodanie submodelu kolejnego (wsp�lny przodek)
 if (Next==NULL)
  Next=SubModel;
 else
  Next->AddNext(SubModel);
};

int __fastcall TSubModel::Flags()
{//analiza koniecznych zmian pomi�dzy submodelami
 //samo pomijanie glBindTexture() nie poprawi wydajno�ci
 //ale mo�na sprawdzi�, czy mo�na w og�le pomin�� kod do tekstur (sprawdzanie replaceskin)
 int i;
 if (Child)
 {//Child jest renderowany po danym submodelu
  if (Child->TextureID) //o ile ma tekstur�
   if (Child->TextureID!=TextureID) //i jest ona inna ni� rodzica
    Child->iFlags|=0x80; //to trzeba sprawdza�, jak z teksturami jest
  i=Child->Flags();
  iFlags|=0x00FF0000&((i<<16)|(i)|(i>>8)); //potomny, rodze�stwo i dzieci
 }
 if (Next)
 {//Next jest renderowany przed danym submodelem
  i=Next->Flags();
  iFlags|=0xFF000000&((i<<24)|(i<<8)|(i)); //nast�pny, kolejne i ich dzieci
  if (TextureID) //o ile dany ma tekstur�
   if ((TextureID!=Next->TextureID)||(i&0x00800000)) //a ma inn� albo dzieci zmieniaj�
    iFlags|=0x80; //to dany submodel musi sobie j� ustawia�
  //tekstury nie ustawiamy tylko wtedy, gdy jest taka sama jak Next i jego dzieci nie zmieniaj�
 }
 return iFlags;
};

void __fastcall TSubModel::SetRotate(vector3 vNewRotateAxis,double fNewAngle)
{//obr�cenie submodelu wg podanej osi (np. wskaz�wki w kabinie)
 v_RotateAxis=vNewRotateAxis;
 f_Angle=fNewAngle;
 if (fNewAngle!=0.0)
 {b_Anim=at_Rotate;
  b_aAnim=at_Rotate;
 }
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetRotateXYZ(vector3 vNewAngles)
{//obr�cenie submodelu o podane k�ty wok� osi lokalnego uk�adu
 v_Angles=vNewAngles;
 b_Anim=at_RotateXYZ;
 b_aAnim=at_RotateXYZ;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetTranslate(vector3 vNewTransVector)
{//przesuni�cie submodelu (np. w kabinie)
 v_TransVector=vNewTransVector;
 b_Anim=at_Translate;
 b_aAnim=at_Translate;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

struct ToLower
{
 char operator()(char input) { return tolower(input); }
};

TSubModel* __fastcall TSubModel::GetFromName(std::string search)
{

    TSubModel* result;

    std::transform(search.begin(),search.end(),search.begin(),ToLower());

    if (search==Name)
        return this;

    if (Next)
    {
        result=Next->GetFromName(search);
        if (result)
            return result;
    };

    if (Child)
    {
        result=Child->GetFromName(search);
        if (result)
            return result;
    };

    return NULL;

};

WORD hbIndices[18]={3,0,1,5,4,2,1,0,4,1,5,3,2,3,5,2,4,0};

void __fastcall TSubModel::RaAnimation(TAnimType a)
{//wykonanie animacji niezale�nie od renderowania
 switch (a)
 {//korekcja po�o�enia, je�li submodel jest animowany
  case at_Translate: //Ra: by�o "true"
   if (iAnimOwner!=iInstance) break; //cudza animacja
   glTranslatef(v_TransVector.x,v_TransVector.y,v_TransVector.z);
   break;
  case at_Rotate: //Ra: by�o "true"
   if (iAnimOwner!=iInstance) break; //cudza animacja
   glRotatef(f_Angle,v_RotateAxis.x,v_RotateAxis.y,v_RotateAxis.z);
   break;
  case at_RotateXYZ:
   if (iAnimOwner!=iInstance) break; //cudza animacja
   glTranslatef(v_TransVector.x,v_TransVector.y,v_TransVector.z);
   glRotatef(v_Angles.x,1.0,0.0,0.0);
   glRotatef(v_Angles.y,0.0,1.0,0.0);
   glRotatef(v_Angles.z,0.0,0.0,1.0);
   break;
  case at_SecondsJump: //sekundy z przeskokiem
   glRotatef(floor(GlobalTime->mr)*6.0,0.0,1.0,0.0);
   break;
  case at_MinutesJump: //minuty z przeskokiem
   glRotatef(GlobalTime->mm*6.0,0.0,1.0,0.0);
   break;
  case at_HoursJump: //godziny skokowo 12h/360�
   glRotatef(GlobalTime->hh*30.0*0.5,0.0,1.0,0.0);
   break;
  case at_Hours24Jump: //godziny skokowo 24h/360�
   glRotatef(GlobalTime->hh*15.0*0.25,0.0,1.0,0.0);
   break;
  case at_Seconds: //sekundy p�ynnie
   glRotatef(GlobalTime->mr*6.0,0.0,1.0,0.0);
   break;
  case at_Minutes: //minuty p�ynnie
   glRotatef(GlobalTime->mm*6.0+GlobalTime->mr*0.1,0.0,1.0,0.0);
   break;
  case at_Hours: //godziny p�ynnie 12h/360�
   //glRotatef(GlobalTime->hh*30.0+GlobalTime->mm*0.5+GlobalTime->mr/120.0,0.0,1.0,0.0);
   glRotatef(0.5*Global::fTimeAngleDeg,0.0,1.0,0.0);
   break;
  case at_Hours24: //godziny p�ynnie 24h/360�
   //glRotatef(GlobalTime->hh*15.0+GlobalTime->mm*0.25+GlobalTime->mr/240.0,0.0,1.0,0.0);
   glRotatef(Global::fTimeAngleDeg,0.0,1.0,0.0);
   break;
  case at_Billboard: //obr�t w pionie do kamery
   {matrix4x4 mat; //potrzebujemy wsp�rz�dne przesuni�cia �rodka uk�adu wsp�rz�dnych submodelu
    glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray()); //pobranie aktualnej matrycy
    vector3 gdzie=vector3(mat[3][0],mat[3][1],mat[3][2]); //pocz�tek uk�adu wsp�rz�dnych submodelu wzgl�dem kamery
    glLoadIdentity(); //macierz jedynkowa
    glTranslated(gdzie.x,gdzie.y,gdzie.z); //pocz�tek uk�adu zostaje bez zmian
    glRotated(atan2(gdzie.x,gdzie.z)*180.0/M_PI,0.0,1.0,0.0); //jedynie obracamy w pionie o k�t
   }
   break;
  case at_Wind: //ruch pod wp�ywem wiatru (wiatr b�dziemy liczy� potem...)
   glRotated(1.5*sin(M_PI*GlobalTime->mr/6.0),0.0,1.0,0.0);
   break;
  case at_Sky: //animacja nieba
   glRotated(Global::fLatitudeDeg,1.0,0.0,0.0); //ustawienie osi OY na p�noc
   //glRotatef(Global::fTimeAngleDeg,0.0,1.0,0.0); //obr�t dobowy osi OX
   glRotated(-fmod(Global::fSunSpeed*Global::fTimeAngleDeg,360.0),0.0,1.0,0.0); //obr�t dobowy osi OX
   break;
 }
};

void __fastcall TSubModel::RaRender(GLuint ReplacableSkinId,bool bAlpha)
{//g��wna procedura renderowania
 if (Next)
  if (bAlpha?(iFlags&0x02000000):(iFlags&0x03000000))
   Next->RaRender(ReplacableSkinId,bAlpha); //dalsze rekurencyjnie
 if (Visible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix();
   //if (iFlags&0x8000)
    glMultMatrixd(Matrix.getArray());
   if (b_Anim) RaAnimation(b_Anim);
  }
  if ((TextureID==-1)) // && (ReplacableSkinId!=0))
  {//zmienialne skory
   glBindTexture(GL_TEXTURE_2D,ReplacableSkinId);
   //if (ReplacableSkinId>0)
   TexAlpha=bAlpha; //zmiana tylko w przypadku wymienej tekstury
  }
  else
   glBindTexture(GL_TEXTURE_2D,TextureID);
  if (eType==smt_Mesh)
  {
   glColor3fv(f4Diffuse);   //McZapkie-240702: zamiast ub
   if (!TexAlpha || !Global::bRenderAlpha)  //rysuj gdy nieprzezroczyste lub # albo gdy zablokowane alpha
   {
    //glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,f4Diffuse); //to samo, co glColor
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
   }
  }
  else if (eType==smt_FreeSpotLight)
  {
   matrix4x4 mat;
   glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray());
   //k�t mi�dzy kierunkiem �wiat�a a wsp�rz�dnymi kamery
   vector3 gdzie=mat*vector3(0,0,0); //pozycja punktu �wiec�cego wzgl�dem kamery
   fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-gdzie),Normalize(gdzie));
   //(by�o mi�dzy kierunkiem �wiat�a a k�tem kamery)
   //fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-mat*vector3(0,0,0)),vector3(0,0,1));
   if (fCosViewAngle>fcosFalloffAngle)  //kat wiekszy niz max stozek swiatla
   {
    double Distdimm=1.0;
    if (fCosViewAngle<fcosHotspotAngle) //zmniejszona jasno�� mi�dzy Hotspot a Falloff
     if (fcosFalloffAngle<fcosHotspotAngle)
      Distdimm=1.0-(fcosHotspotAngle-fCosViewAngle)/(fcosHotspotAngle-fcosFalloffAngle);

/*  TODO: poprawic to zeby dzialalo

2- Inverse (Applies inverse decay. The formula is luminance=R0/R, where R0 is
 the radial source of the light if no attenuation is used, or the Near End
 value of the light if Attenuation is used. R is the radial distance of the
  illuminated surface from R0.)

3- Inverse Square (Applies inverse-square decay. The formula for this is (R0/R)^2.
 This is actually the "real-world" decay of light, but you might find it too dim
 in the world of computer graphics.)

<light>.DecayRadius -- The distance over which the decay occurs.

             if (iFarAttenDecay>0)
              switch (iFarAttenDecay)
              {
               case 1:
                   Distdimm=fFarDecayRadius/(1+sqrt(fSquareDist));  //dorobic od kata
               break;
               case 2:
                   Distdimm=fFarDecayRadius/(1+fSquareDist);  //dorobic od kata
               break;
              }
             if (Distdimm>1)
              Distdimm=1;

*/
    //glColor3f(f4Diffuse[0],f4Diffuse[1],f4Diffuse[2]);
    //glColorMaterial(GL_FRONT,GL_EMISSION);
    float color[4]={f4Diffuse[0]*Distdimm,f4Diffuse[1]*Distdimm,f4Diffuse[2]*Distdimm,0};
    //glColor3f(f4Diffuse[0]*Distdimm,f4Diffuse[1]*Distdimm,f4Diffuse[2]*Distdimm);
    //glColorMaterial(GL_FRONT,GL_EMISSION);
    glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
    glColor3fv(color); //inaczej s� bia�e
    glMaterialfv(GL_FRONT,GL_EMISSION,color);
    glDrawArrays(GL_POINTS,iVboPtr,iNumVerts);  //narysuj wierzcho�ek z VBO
    glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    glEnable(GL_LIGHTING);
    //glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE); //co ma ustawia� glColor
   }
  }
  else if (eType==smt_Stars)
  {
   //glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
   if (Global::fLuminance<fLight)
   {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
    glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie punkty z VBO
    //glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
   }
   //else
   // glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie punkty
   //glEnable(GL_LIGHTING);
  }
/*Ra: tu co� jest bez sensu...
    else
    {
     glBindTexture(GL_TEXTURE_2D, 0);
//        if (eType==smt_FreeSpotLight)
//         {
//          if (iFarAttenDecay==0)
//            glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);
//         }
//         else
//TODO: poprawic zeby dzialalo
     glColor3f(f4Diffuse[0],f4Diffuse[1],f4Diffuse[2]);
     glColorMaterial(GL_FRONT,GL_EMISSION);
     glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
     //glBegin(GL_POINTS);
     glDrawArrays(GL_POINTS,iVboPtr,iNumVerts);  //narysuj wierzcho�ek z VBO
     //       glVertex3f(0,0,0);
     //glEnd();
     glEnable(GL_LIGHTING);
     glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
     //glEndList();
    }
*/
  if (Child!=NULL)
   if (bAlpha?(iFlags&0x00020000):(iFlags&0x00030000))
    Child->RaRender(ReplacableSkinId,bAlpha);
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_Anim<at_SecondsJump)
  b_Anim=at_None; //wy��czenie animacji dla kolejnego u�ycia submodelu
};       //Render

void __fastcall TSubModel::RaRenderAlpha(GLuint ReplacableSkinId,bool bAlpha)
{
 if (Next)
  if (bAlpha?(iFlags&0x05000000):(iFlags&0x04000000))
   Next->RaRenderAlpha(ReplacableSkinId,bAlpha);
 if (Visible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix(); //zapami�tanie matrycy
   //if (iFlags&0x8000)
    glMultMatrixd(Matrix.getArray());
   if (b_aAnim) RaAnimation(b_aAnim);
  }
  glColor3fv(f4Diffuse);
  //zmienialne skory
  if (eType==smt_Mesh)
  {
   if ((TextureID==-1)) // && (ReplacableSkinId!=0))
   {
    glBindTexture(GL_TEXTURE_2D,ReplacableSkinId);
    TexAlpha=bAlpha; //zmiana tylko w przypadku wymienej tekstury
   }
   else
    glBindTexture(GL_TEXTURE_2D,TextureID);
   if (TexAlpha && Global::bRenderAlpha)  //mozna rysowac bo przezroczyste i nie ma #
   {
   //glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,f4Diffuse);
   if (Global::fLuminance<fLight)
   {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
    glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
    glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
   }
   else
    glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
  }
  }
  else if (eType==smt_FreeSpotLight)
  {
//        if (CosViewAngle>0)  //dorobic od kata
//        {
            //return; //nie mo�e by� return - trzeba skasowa� animacj�
//        }
  // dorobic aureol�!
  }
  if (Child)
   if (bAlpha?(iFlags&0x00050000):(iFlags&0x00040000))
    Child->RaRenderAlpha(ReplacableSkinId,bAlpha);
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_aAnim<at_SecondsJump)
  b_aAnim=at_None; //wy��czenie animacji dla kolejnego u�ycia submodelu
}; //RenderAlpha

void __fastcall TSubModel::Render(GLuint ReplacableSkinId,bool bAlpha)
{//g��wna procedura renderowania
 if (Next)
  if (bAlpha?(iFlags&0x02000000):(iFlags&0x03000000))
   Next->Render(ReplacableSkinId,bAlpha); //dalsze rekurencyjnie
 if (Visible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix();
   //if (iFlags&0x8000)
    glMultMatrixd(Matrix.getArray());
   if (b_Anim) RaAnimation(b_Anim);
  }
  //zmienialne skory
  if (eType==smt_FreeSpotLight)
  {
   matrix4x4 mat;
   glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray());
   //k�t mi�dzy kierunkiem �wiat�a a wsp�rz�dnymi kamery
   vector3 gdzie=mat*vector3(0,0,0); //pozycja wzgl�dna punktu �wiec�cego
   fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-gdzie),Normalize(gdzie));
   //(by�o mi�dzy kierunkiem �wiat�a a k�tem kamery)
   //fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-mat*vector3(0,0,0)),vector3(0,0,1));
   if (fCosViewAngle>fcosFalloffAngle)  //kat wiekszy niz max stozek swiatla
   {
    double Distdimm=1.0;
    if (fCosViewAngle<fcosHotspotAngle) //zmniejszona jasno�� mi�dzy Hotspot a Falloff
     if (fcosFalloffAngle<fcosHotspotAngle)
      Distdimm=1.0-(fcosHotspotAngle-fCosViewAngle)/(fcosHotspotAngle-fcosFalloffAngle);
    glColor3f(f4Diffuse[0]*Distdimm,f4Diffuse[1]*Distdimm,f4Diffuse[2]*Distdimm);
/*  TODO: poprawic to zeby dzialalo
              if (iFarAttenDecay>0)
               switch (iFarAttenDecay)
               {
                case 1:
                    Distdimm=fFarDecayRadius/(1+sqrt(fSquareDist));  //dorobic od kata
                break;
                case 2:
                    Distdimm=fFarDecayRadius/(1+fSquareDist);  //dorobic od kata
                break;
               }
              if (Distdimm>1)
               Distdimm=1;
              glColor3f(Diffuse[0]*Distdimm,Diffuse[1]*Distdimm,Diffuse[2]*Distdimm);
*/
   //           glPopMatrix();
   //        return;
    glCallList(uiDisplayList); //wy�wietlenie warunkowe
   }
  }
  else if (eType==smt_Mesh)
  {if ((TextureID==-1)) // && (ReplacableSkinId!=0))
   {
    glBindTexture(GL_TEXTURE_2D,ReplacableSkinId);
    //if (ReplacableSkinId>0)
    TexAlpha=bAlpha; //zmiana tylko w przypadku wymienej tekstury
   }
   else
    glBindTexture(GL_TEXTURE_2D,TextureID);
   if (!TexAlpha || !Global::bRenderAlpha)  //rysuj gdy nieprzezroczyste lub # albo gdy zablokowane alpha
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glCallList(uiDisplayList); //tylko dla siatki
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glCallList(uiDisplayList); //tylko dla siatki
  }
  else if (eType==smt_Stars)
  {
   //glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
   if (Global::fLuminance<fLight)
   {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
    glCallList(uiDisplayList); //narysuj naraz wszystkie punkty z DL
    glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
   }
   //else
   // glDrawArrays(GL_TRIANGLES,iVboPtr,iNumVerts);  //narysuj naraz wszystkie punkty
   //glEnable(GL_LIGHTING);
  }
  if (Child!=NULL)
   if (bAlpha?(iFlags&0x00020000):(iFlags&0x00030000))
    Child->Render(ReplacableSkinId,bAlpha);
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_Anim<at_SecondsJump)
  b_Anim=at_None; //wy��czenie animacji dla kolejnego u�ycia subm
}; //Render

void __fastcall TSubModel::RenderAlpha(GLuint ReplacableSkinId,bool bAlpha)
{
 if (Next!=NULL)
  if (bAlpha?(iFlags&0x05000000):(iFlags&0x04000000))
   Next->RenderAlpha(ReplacableSkinId,bAlpha);
 if (Visible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix();
   //if (iFlags&0x8000)
    glMultMatrixd(Matrix.getArray());
   if (b_aAnim) RaAnimation(b_aAnim);
  }
  if (eType==smt_FreeSpotLight)
  {
//        if (CosViewAngle>0)  //dorobic od kata
//        {
//            return;
//        }
     // dorobic aureole!
  }
  else
  {
   //zmienialne skory
   if ((TextureID==-1)) // && (ReplacableSkinId!=0))
    {
     glBindTexture(GL_TEXTURE_2D,ReplacableSkinId);
     if (ReplacableSkinId>0)
       TexAlpha=bAlpha;  //zmiana tylko w przypadku wymienej tekstury
    }
   else
    glBindTexture(GL_TEXTURE_2D,TextureID);
   //jak przezroczyste s� wy��czone, to tu w og�le nie wchodzi
   if (TexAlpha && Global::bRenderAlpha)  //mozna rysowac bo przezroczyste i nie ma #
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glCallList(uiDisplayList); //tylko dla siatki
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glCallList(uiDisplayList); //tylko dla siatki
  }
  if (Child!=NULL)
   if (bAlpha?(iFlags&0x00050000):(iFlags&0x00040000))
    Child->RenderAlpha(ReplacableSkinId,bAlpha);
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_aAnim<at_SecondsJump)
  b_aAnim=at_None; //wy��czenie animacji dla kolejnego u�ycia submodelu
}; //RenderAlpha


matrix4x4* __fastcall TSubModel::GetTransform()
{
 return &Matrix;
};


//---------------------------------------------------------------------------

void  __fastcall TSubModel::RaArrayFill(CVertNormTex *Vert)
{//wype�nianie tablic VBO
 if (Next) Next->RaArrayFill(Vert);
 if (Child) Child->RaArrayFill(Vert);
 if (eType==smt_Mesh)
  for (int i=0;i<iNumVerts;++i)
  {Vert[iVboPtr+i].x =Vertices[i].Point.x;
   Vert[iVboPtr+i].y =Vertices[i].Point.y;
   Vert[iVboPtr+i].z =Vertices[i].Point.z;
   Vert[iVboPtr+i].nx=Vertices[i].Normal.x;
   Vert[iVboPtr+i].ny=Vertices[i].Normal.y;
   Vert[iVboPtr+i].nz=Vertices[i].Normal.z;
   Vert[iVboPtr+i].u =Vertices[i].tu;
   Vert[iVboPtr+i].v =Vertices[i].tv;
  }
 else if (eType==smt_FreeSpotLight)
  Vert[iVboPtr].x=Vert[iVboPtr].y=Vert[iVboPtr].z=0.0;
};
//---------------------------------------------------------------------------

__fastcall TModel3d::TModel3d()
{
//    Root=NULL;
//    Materials=NULL;
//    MaterialsCount=0;
 Root=NULL;
 //SubModelsCount=0;
 iFlags=0;
//    ReplacableSkinID=0;
};
/*
__fastcall TModel3d::TModel3d(char *FileName)
{
//    Root=NULL;
//    Materials=NULL;
//    MaterialsCount=0;
 Root=NULL;
 SubModelsCount=0;
 iFlags=0;
 LoadFromFile(FileName);
};
*/
__fastcall TModel3d::~TModel3d()
{
 SafeDelete(Root);
 //SafeDeleteArray(Materials);
};

bool __fastcall TModel3d::AddTo(const char *Name,TSubModel *SubModel)
{
    TSubModel *tmp=GetFromName(Name);
    if (tmp!=NULL)
    {
        tmp->AddChild(SubModel);
        return true;
    }
    else
    {
        if (Root!=NULL)
            Root->AddNext(SubModel);
        else
            Root=SubModel;

        return true;
    }
};

TSubModel* __fastcall TModel3d::GetFromName(const char *sName)
{
    return Root ? Root->GetFromName(sName) : NULL;
};

/*
TMaterial* __fastcall TModel3d::GetMaterialFromName(char *sName)
{
    AnsiString tmp=AnsiString(sName).Trim();
    for (int i=0; i<MaterialsCount; i++)
        if (strcmp(sName,Materials[i].Name.c_str())==0)
//        if (Trim()==Materials[i].Name.tmp)
            return Materials+i;
    return Materials;
}
*/

void __fastcall TModel3d::LoadFromTextFile(char *FileName,bool dynamic)
{
 WriteLog("Loading - text model: "+AnsiString(FileName));
 cParser parser(FileName,cParser::buffer_FILE); //Ra: tu powinno by� "models\\"...
 TSubModel *SubModel;
 std::string token;
 parser.getToken(token);
 iNumVerts=0;
 while (token!="" || parser.eof())
 {
  std::string parent;
  parser.getToken(parent);
  if (parent=="") break;
  SubModel=new TSubModel();
  iNumVerts+=SubModel->Load(parser,this,iNumVerts);
  if (!AddTo(parent.c_str(),SubModel)) delete SubModel;
  //SubModelsCount++;
  parser.getToken(token);
 }
 if (Root)
 {
  if (!Global::bUseVBO) //dla DL wierzcho�ki s� kompilowane przy wczytywaniu
   Root->WillBeAnimated(); //i nie da si� ich przeliczy�
  if (!dynamic) //dynamic zrobi to sam dopiero po przeanalizowaniu animacji submodeli
   Init();
 }
}

void __fastcall TModel3d::Init()
{//obr�cenie pocz�tkowe uk�adu wsp�rz�dnych, dla pojazd�w wykonywane po analizie animacji
 if (iFlags&0x8000) return; //zosta� ju� obr�cony
 if (Root)
 {Root->InitialRotate(true); //konwersja uk�adu wsp�rz�dnych
  if (iNumVerts)
  {
#ifdef USE_VBO
   if (Global::bUseVBO)
   {//tworzenie tymczasowej tablicy z wierzcho�kami ca�ego modelu
    MakeArray(iNumVerts); //tworzenie tablic dla VBO
    Root->RaArrayFill(m_pVNT); //wype�nianie tablicy
    BuildVBOs(); //tworzenie VBO i usuwanie tablicy z pami�ci
   }
#endif
   iFlags=Root->Flags()|0x8000; //flagi ca�ego modelu
   //if (Root->TextureID) //o ile ma tekstur�
   // Root->iFlags|=0x80; //konieczno�� ustawienia tekstury
  }
 }
};

void __fastcall TModel3d::SaveToFile(char *FileName)
{
    Error("Not implemented yet :(");
};

void __fastcall TModel3d::BreakHierarhy()
{
    Error("Not implemented yet :(");
};

/*
void __fastcall TModel3d::Render(vector3 pPosition,double fAngle,GLuint ReplacableSkinId,bool bAlpha)
{
//    glColor3f(1.0f,1.0f,1.0f);
//    glColor3f(0.0f,0.0f,0.0f);
 glPushMatrix();

 glTranslated(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
/*
 matrix4x4 Identity;
 Identity.Identity();

    matrix4x4 CurrentMatrix;
    glGetdoublev(GL_MODELVIEW_MATRIX,CurrentMatrix.getArray());
    vector3 pos=vector3(0,0,0);
    pos=CurrentMatrix*pos;
    fSquareDist=SquareMagnitude(pos);
  * /
    fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition());

#ifdef _DEBUG
    if (Root)
        Root->Render(ReplacableSkinId,bAlpha);
#else
    Root->Render(ReplacableSkinId,bAlpha);
#endif
    glPopMatrix();
};
*/

void __fastcall TModel3d::Render(double fSquareDistance,GLuint ReplacableSkinId,bool bAlpha)
{
 if (bAlpha?(iFlags&0x02020002):(iFlags&0x03030003))
 {fSquareDist=fSquareDistance;
  Root->Render(ReplacableSkinId,bAlpha);
 }
};

/*
void __fastcall TModel3d::RenderAlpha(vector3 pPosition,double fAngle,GLuint ReplacableSkinId,bool bAlpha)
{
    glPushMatrix();
    glTranslated(pPosition.x,pPosition.y,pPosition.z);
    if (fAngle!=0)
        glRotatef(fAngle,0,1,0);
    fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition());
#ifdef _DEBUG
    if (Root)
        Root->RenderAlpha(ReplacableSkinId,bAlpha);
#else
    Root->RenderAlpha(ReplacableSkinId,bAlpha);
#endif
    glPopMatrix();
};
*/

void __fastcall TModel3d::RenderAlpha(double fSquareDistance,GLuint ReplacableSkinId,bool bAlpha)
{
 if (bAlpha?(iFlags&0x05050005):(iFlags&0x04040004))
 {
  fSquareDist=fSquareDistance;
  Root->RenderAlpha(ReplacableSkinId,bAlpha);
 }
};

void __fastcall TModel3d::RaRender(vector3 pPosition,double fAngle,GLuint ReplacableSkinId,bool bAlpha)
{
//    glColor3f(1.0f,1.0f,1.0f);
//    glColor3f(0.0f,0.0f,0.0f);
 glPushMatrix(); //zapami�tanie matrycy przekszta�cenia
 glTranslated(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
/*
 matrix4x4 Identity;
 Identity.Identity();

 matrix4x4 CurrentMatrix;
 glGetdoublev(GL_MODELVIEW_MATRIX,CurrentMatrix.getArray());
 vector3 pos=vector3(0,0,0);
 pos=CurrentMatrix*pos;
 fSquareDist=SquareMagnitude(pos);
*/
 fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition());
 if (StartVBO())
 {Root->RaRender(ReplacableSkinId,bAlpha);
  EndVBO();
 }
 glPopMatrix(); //przywr�cenie ustawie� przekszta�cenia
};

void __fastcall TModel3d::RaRender(double fSquareDistance,GLuint ReplacableSkinId,bool bAlpha)
{//renderowanie specjalne, np. kabiny
 if (bAlpha?(iFlags&0x02020002):(iFlags&0x03030003))
 {fSquareDist=fSquareDistance;
  if (StartVBO())
  {Root->RaRender(ReplacableSkinId,bAlpha);
   EndVBO();
  }
 }
};

void __fastcall TModel3d::RaRenderAlpha(double fSquareDistance,GLuint ReplacableSkinId,bool bAlpha)
{//renderowanie specjalne, np. kabiny
 if (bAlpha?(iFlags&0x05050005):(iFlags&0x04040004))
 {fSquareDist=fSquareDistance;
  if (StartVBO())
  {Root->RaRenderAlpha(ReplacableSkinId,bAlpha);
   EndVBO();
  }
 }
};

void __fastcall TModel3d::RaRenderAlpha(vector3 pPosition,double fAngle,GLuint ReplacableSkinId,bool bAlpha)
{
 glPushMatrix();
 glTranslated(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
 fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition());
 if (StartVBO())
 {Root->RaRenderAlpha(ReplacableSkinId,bAlpha);
  EndVBO();
 }
 glPopMatrix();
};


//-----------------------------------------------------------------------------
//2011-03-16 cztery nowe funkcje renderowania z mo�liwo�ci� pochylania obiekt�w
//-----------------------------------------------------------------------------

void __fastcall TModel3d::Render(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId,bool bAlpha)
{//nieprzezroczyste, Display List
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotatef(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotatef(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotatef(vAngle->z,0.0,0.0,1.0);
 fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition());
 Root->Render(ReplacableSkinId,bAlpha);
 glPopMatrix();
};
void __fastcall TModel3d::RenderAlpha(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId,bool bAlpha)
{//przezroczyste, Display List
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotatef(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotatef(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotatef(vAngle->z,0.0,0.0,1.0);
 fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition());
 Root->RenderAlpha(ReplacableSkinId,bAlpha);
 glPopMatrix();
};
void __fastcall TModel3d::RaRender(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId,bool bAlpha)
{//nieprzezroczyste, VBO
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotatef(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotatef(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotatef(vAngle->z,0.0,0.0,1.0);
 fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition());
 if (StartVBO())
 {Root->RaRender(ReplacableSkinId,bAlpha);
  EndVBO();
 }
 glPopMatrix();
};
void __fastcall TModel3d::RaRenderAlpha(vector3* vPosition,vector3* vAngle,GLuint ReplacableSkinId,bool bAlpha)
{//przezroczyste, VBO
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotatef(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotatef(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotatef(vAngle->z,0.0,0.0,1.0);
 fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition());
 if (StartVBO())
 {Root->RaRenderAlpha(ReplacableSkinId,bAlpha);
  EndVBO();
 }
 glPopMatrix();
};

//---------------------------------------------------------------------------
#pragma package(smart_init)
