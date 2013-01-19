//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

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
//---------------------------------------------------------------------------
#pragma package(smart_init)

double TSubModel::fSquareDist=0;
int TSubModel::iInstance; //numer renderowanego egzemplarza obiektu
GLuint *TSubModel::ReplacableSkinId=NULL;
int TSubModel::iAlpha=0x30300030; //maska do testowania flag tekstur wymiennych
TModel3d* TSubModel::pRoot; //Ra: tymczasowo wska�nik na model widoczny z submodelu
AnsiString* TSubModel::pasText;
//przyk�ady dla TSubModel::iAlpha:
// 0x30300030 - wszystkie bez kana�u alfa
// 0x31310031 - tekstura -1 u�ywana w danym cyklu, pozosta�e nie
// 0x32320032 - tekstura -2 u�ywana w danym cyklu, pozosta�e nie
// 0x34340034 - tekstura -3 u�ywana w danym cyklu, pozosta�e nie
// 0x38380038 - tekstura -4 u�ywana w danym cyklu, pozosta�e nie
// 0x3F3F003F - wszystkie wymienne tekstury u�ywane w danym cyklu
//Ale w TModel3d oker�la przezroczysto�� tekstur wymiennych!

int TSubModelInfo::iTotalTransforms=0; //ilo�� transform�w
int TSubModelInfo::iTotalNames=0; //d�ugo�� obszaru nazw
int TSubModelInfo::iTotalTextures=0; //d�ugo�� obszaru tekstur
int TSubModelInfo::iCurrent=0; //aktualny obiekt
TSubModelInfo* TSubModelInfo::pTable=NULL; //tabele obiekt�w pomocniczych


char* TStringPack::String(int n)
{//zwraca wska�nik do �a�cucha o podanym numerze
 if (index?n<(index[1]>>2)-2:false)
  return data+8+index[n+2]; //indeks upraszcza kwesti� wyszukiwania
 //jak nie ma indeksu, to trzeba szuka�
 int max=*((int*)(data+4)); //d�ugo�� obszaru �a�cuch�w
 char* ptr=data+8; //pocz�ek obszaru �a�cuch�w
 for (int i=0;i<n;++i)
 {//wyszukiwanie �a�cuch�w nie jest zbyt optymalne, ale nie musi by�
  while (*ptr) ++ptr; //wyszukiwanie zera
  ++ptr; //pomini�cie zera
  if (ptr>data+max) return NULL; //zbyt wysoki numer
 }
 return ptr;
};

__fastcall TSubModel::TSubModel()
{
 ZeroMemory(this,sizeof(TSubModel)); //istotne przy zapisywaniu wersji binarnej
 FirstInit();
};

void __fastcall TSubModel::FirstInit()
{
 eType=TP_ROTATOR;
 Vertices=NULL;
 uiDisplayList=0;
 iNumVerts=-1; //do sprawdzenia
 iVboPtr=-1;
 fLight=-1.0; //�wietcenie wy��czone
 v_RotateAxis=float3(0,0,0);
 v_TransVector=float3(0,0,0);
 f_Angle=0;
 b_Anim=at_None;
 b_aAnim=at_None;
 fVisible=0.0; //zawsze widoczne
 iVisible=1;
 fMatrix=NULL; //to samo co iMatrix=0;
 Next=NULL;
 Child=NULL;
 TextureID=0;
 //TexAlpha=false;
 iFlags=0x0200; //bit 9=1: submodel zosta� utworzony a nie ustawiony na wczytany plik
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
 fCosFalloffAngle=0.5; //120�?
 fCosHotspotAngle=0.3; //145�?
 fCosViewAngle=0;
 fSquareMaxDist=10000*10000; //10km
 fSquareMinDist=0;
 iName=-1; //brak nazwy
 iTexture=0; //brak tekstury
 //asName="";
 //asTexture="";
 pName=pTexture=NULL;
 f4Ambient[0]=f4Ambient[1]=f4Ambient[2]=f4Ambient[3]=1.0; //{1,1,1,1};
 f4Diffuse[0]=f4Diffuse[1]=f4Diffuse[2]=f4Diffuse[3]=1.0; //{1,1,1,1};
 f4Specular[0]=f4Specular[1]=f4Specular[2]=0.0; f4Specular[3]=1.0; //{0,0,0,1};
 f4Emision[0]=f4Emision[1]=f4Emision[2]=f4Emision[3]=1.0;
};

__fastcall TSubModel::~TSubModel()
{
 if (uiDisplayList) glDeleteLists(uiDisplayList,1);
 if (iFlags&0x0200)
 {//wczytany z pliku tekstowego musi sam posprz�ta�
  //SafeDeleteArray(Indices);
  SafeDelete(Next);
  SafeDelete(Child);
  delete fMatrix; //w�asny transform trzeba usun�� (zawsze jeden)
  delete[] Vertices;
  delete[] pTexture;
  delete[] pName;
 }
/*
 else
 {//wczytano z pliku binarnego (nie jest w�a�cicielem tablic)
 }
*/
};

void __fastcall TSubModel::TextureNameSet(const char *n)
{//ustawienie nazwy submodelu, o ile nie jest wczytany z E3D
 if (iFlags&0x0200)
 {//tylko je�eli submodel zosta utworzony przez new
  delete[] pTexture; //usuni�cie poprzedniej
  int i=strlen(n);
  if (i)
  {//utworzenie nowej
   pTexture=new char[i+1];
   strcpy(pTexture,n);
  }
  else
   pTexture=NULL;
 }
};

void __fastcall TSubModel::NameSet(const char *n)
{//ustawienie nazwy submodelu, o ile nie jest wczytany z E3D
 if (iFlags&0x0200)
 {//tylko je�eli submodel zosta utworzony przez new
  delete[] pName; //usuni�cie poprzedniej
  int i=strlen(n);
  if (i)
  {//utworzenie nowej
   pName=new char[i+1];
   strcpy(pName,n);
  }
  else
   pName=NULL;
 }
};

//int __fastcall TSubModel::SeekFaceNormal(DWORD *Masks, int f,DWORD dwMask,vector3 *pt,GLVERTEX *Vertices)
int __fastcall TSubModel::SeekFaceNormal(DWORD *Masks,int f,DWORD dwMask,float3 *pt,float8 *Vertices)
{//szukanie punktu stycznego do (pt), zwraca numer wierzcho�ka, a nie tr�jk�ta
 int iNumFaces=iNumVerts/3; //bo maska powierzchni jest jedna na tr�jk�t
 //GLVERTEX *p; //roboczy wska�nik
 float8 *p; //roboczy wska�nik
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

inline void readColor(cParser& parser,int &color)
{
 int r,g,b;
 parser.ignoreToken();
 parser.getToken(r);
 parser.getToken(g);
 parser.getToken(b);
 color=r+(g<<8)+(b<<16);
};
/*
inline void readMatrix(cParser& parser,matrix4x4& matrix)
{//Ra: wczytanie transforma
 for (int x=0;x<=3;x++) //wiersze
  for (int y=0;y<=3;y++) //kolumny
   parser.getToken(matrix(x)[y]);
};
*/
inline void readMatrix(cParser& parser,float4x4& matrix)
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
 //GLuint TextureID;
 //char *extName;
 if (!parser.expectToken("type:"))
  Error("Model type parse failure!");
 {
  std::string type;
  parser.getToken(type);
  if (type=="mesh")
   eType=GL_TRIANGLES; //submodel - tr�jkaty
  else if (type=="point")
   eType=GL_POINTS; //co to niby jest?
  else if (type=="freespotlight")
   eType=TP_FREESPOTLIGHT; //�wiate�ko
  else if (type=="text")
   eType=TP_TEXT; //wy�wietlacz tekstowy (generator napis�w)
  else if (type=="stars")
   eType=TP_STARS; //wiele punkt�w �wietlnych
 };
 parser.ignoreToken();
 std::string token;
 //parser.getToken(token1); //ze zmian� na ma�e!
 parser.getTokens(1,false); //nazwa submodelu bez zmieny na ma�e
 parser >> token;
 NameSet(token.c_str());

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
   else if (type=="ik")            b_Anim=b_aAnim=at_IK; //IK: zadaj�cy
   else if (type=="ik11")          b_Anim=b_aAnim=at_IK11; //IK: kierunkowany
   else if (type=="ik21")          b_Anim=b_aAnim=at_IK21; //IK: kierunkowany
   else if (type=="ik22")          b_Anim=b_aAnim=at_IK22; //IK: kierunkowany
   else b_Anim=b_aAnim=at_Undefined; //nieznana forma animacji
  }
 }
 if (eType<TP_ROTATOR) readColor(parser,f4Ambient); //ignoruje token przed
 readColor(parser,f4Diffuse);
 if (eType<TP_ROTATOR) readColor(parser,f4Specular);
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
 if (eType==TP_FREESPOTLIGHT)
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
  parser.getToken(fCosFalloffAngle); //k�t liczony dla �rednicy, a nie promienia
  fCosFalloffAngle=cos(DegToRad(0.5*fCosFalloffAngle));
  parser.ignoreToken();
  parser.getToken(fCosHotspotAngle); //k�t liczony dla �rednicy, a nie promienia
  fCosHotspotAngle=cos(DegToRad(0.5*fCosHotspotAngle));
  iNumVerts=1;
  iFlags|=0x4010; //rysowane w cyklu nieprzezroczystych, macierz musi zosta� bez zmiany
 }
 else if (eType<TP_ROTATOR)
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
   iFlags|=0x10; //rysowane w cyklu nieprzezroczystych
  }
  else if (texture.find("replacableskin")!=texture.npos)
  {// McZapkie-060702: zmienialne skory modelu
   TextureID=-1;
   iFlags|=(Transparency==0.0)?0x10:1; //zmienna tekstura 1
  }
  else if (texture=="-1")
  {
   TextureID=-1;
   iFlags|=(Transparency==0.0)?0x10:1; //zmienna tekstura 1
  }
  else if (texture=="-2")
  {
   TextureID=-2;
   iFlags|=(Transparency==0.0)?0x10:2; //zmienna tekstura 2
  }
  else if (texture=="-3")
  {
   TextureID=-3;
   iFlags|=(Transparency==0.0)?0x10:4; //zmienna tekstura 3
  }
  else if (texture=="-4")
  {
   TextureID=-4;
   iFlags|=(Transparency==0.0)?0x10:8; //zmienna tekstura 4
  }
  else
  {//jesli tylko nazwa pliku to dawac biezaca sciezke do tekstur
   //asTexture=AnsiString(texture.c_str()); //zapami�tanie nazwy tekstury
   TextureNameSet(texture.c_str());
   if (texture.find_first_of("/\\")==texture.npos)
    texture.insert(0,Global::asCurrentTexturePath.c_str());
   TextureID=TTexturesManager::GetTextureID(texture);
   //TexAlpha=TTexturesManager::GetAlpha(TextureID);
   //iFlags|=TexAlpha?0x20:0x10; //0x10-nieprzezroczysta, 0x20-przezroczysta
   iFlags|=TTexturesManager::GetAlpha(TextureID)?0x20:0x10; //0x10-nieprzezroczysta, 0x20-przezroczysta
  };
 }
 else iFlags|=0x10;
 parser.ignoreToken();
 parser.getToken(fSquareMaxDist);
 if (fSquareMaxDist>=0.0)
  fSquareMaxDist*=fSquareMaxDist;
 else
  fSquareMaxDist=10000*10000; //10km
 parser.ignoreToken();
 parser.getToken(fSquareMinDist);
 fSquareMinDist*=fSquareMinDist;
 parser.ignoreToken();
 fMatrix=new float4x4();
 readMatrix(parser,*fMatrix); //wczytanie transform
 if (!fMatrix->IdentityIs())
  iFlags|=0x8000; //transform niejedynkowy - trzeba go przechowa�
 int iNumFaces; //ilo�� tr�jk�t�w
 DWORD *sg; //maski przynale�no�ci tr�jk�t�w do powierzchni
 if (eType<TP_ROTATOR)
 {//wczytywanie wierzcho�k�w
  parser.ignoreToken();
  parser.getToken(iNumVerts);
  if (iNumVerts%3)
  {
   iNumVerts=0;
   Error("Mesh error, (iNumVertices="+AnsiString(iNumVerts)+")%3!=0");
   return 0;
  }
  //Vertices=new GLVERTEX[iNumVerts];
  if (iNumVerts)
  {Vertices=new float8[iNumVerts];
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
      //WriteLog(AnsiString("Degenerated triangle ignored in: \"")+asName+"\"");
      WriteLog(AnsiString("Degenerated triangle ignored in: \"")+AnsiString(pName)+"\"");
     }
     if (i>0) //pierwszy tr�jk�t mo�e by� zdegenerowany i zostanie usuni�ty
      if (((Vertices[i  ].Point-Vertices[i-1].Point).Length()>2000.0) ||
          ((Vertices[i-1].Point-Vertices[i-2].Point).Length()>2000.0) ||
          ((Vertices[i-2].Point-Vertices[i  ].Point).Length()>2000.0))
      {//je�eli s� dalej ni� 2km od siebie
       --iNumFaces; //o jeden tr�jk�t mniej
       iNumVerts-=3; //czyli o 3 wierzcho�ki
       i-=3; //wczytanie kolejnego w to miejsce
       WriteLog(AnsiString("Too large triangle ignored in: \"")+AnsiString(pName)+"\"");
      }
    }
   }
   int i; //indeks dla tr�jk�t�w
   float3 *n=new float3[iNumFaces]; //tablica wektor�w normalnych dla tr�jk�t�w
   for (i=0;i<iNumFaces;i++) //p�tla po tr�jk�tach - b�dzie szybciej, jak wst�pnie przeliczymy normalne tr�jk�t�w
    n[i]=SafeNormalize(CrossProduct(Vertices[i*3].Point-Vertices[i*3+1].Point,Vertices[i*3].Point-Vertices[i*3+2].Point));
   int v; //indeks dla wierzcho�k�w
   int *wsp=new int[iNumVerts]; //z kt�rego wierzcho�ka kopiowa� wektor normalny
   for (v=0;v<iNumVerts;v++)
    wsp[v]=-1; //wektory normalne nie s� policzone
   int f; //numer tr�jk�ta stycznego
   float3 norm; //roboczy wektor normalny
   for (v=0;v<iNumVerts;v++)
   {//p�tla po wierzcho�kach tr�jk�t�w
    if (wsp[v]>=0) //je�li ju� by� liczony wektor normalny z u�yciem tego wierzcho�ka
     Vertices[v].Normal=Vertices[wsp[v]].Normal; //to wystarczy skopiowa� policzony wcze�niej
    else
    {//inaczej musimy dopiero policzy�
     i=v/3; //numer tr�jk�ta
     norm=float3(0,0,0); //liczenie zaczynamy od zera
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
  else //gdy brak wierzcho�k�w
  {eType=TP_ROTATOR; //submodel pomocniczy, ma tylko macierz przekszta�cenia
   iVboPtr=iNumVerts=0; //dla formalno�ci
  }
 }
 else if (eType==TP_STARS)
 {//punkty �wiec�ce dook�lnie - sk�adnia jak dla smt_Mesh
  parser.ignoreToken();
  parser.getToken(iNumVerts);
  //Vertices=new GLVERTEX[iNumVerts];
  Vertices=new float8[iNumVerts];
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
 //Visible=true; //si� potem wy��czy w razie potrzeby
 //iFlags|=0x0200; //wczytano z pliku tekstowego (jest w�a�cicielem tablic)
 if (iNumVerts<1) iFlags&=~0x3F; //cykl renderowania uzale�niony od potomnych
 return iNumVerts; //do okre�lenia wielko�ci VBO
};

int __fastcall TSubModel::TriangleAdd(TModel3d *m,int tex,int tri)
{//dodanie tr�jk�t�w do submodelu, u�ywane przy tworzeniu E3D terenu
 TSubModel *s=this;
 while (s?(s->TextureID!=tex):false)
 {//szukanie submodelu o danej teksturze
  if (s==this)
   s=Child;
  else
   s=s->Next;
 }
 if (!s)
 {if (TextureID<=0)
   s=this; //u�ycie g��wnego
  else
  {//dodanie nowego submodelu do listy potomnych
   s=new TSubModel();
   m->AddTo(this,s);
  }
  //s->asTexture=AnsiString(TTexturesManager::GetName(tex).c_str());
  s->TextureNameSet(TTexturesManager::GetName(tex).c_str());
  s->TextureID=tex;
  s->eType=GL_TRIANGLES;
  //iAnimOwner=0; //roboczy wska�nik na wierzcho�ek
 }
 if (s->iNumVerts<0)
  s->iNumVerts=tri; //bo na pocz�tku jest -1, czyli �e nie wiadomo
 else
  s->iNumVerts+=tri; //aktualizacja ilo�ci wierzcho�k�w
 return s->iNumVerts-tri; //zwraca pozycj� tych tr�jk�t�w w submodelu
};

float8* __fastcall TSubModel::TrianglePtr(int tex,int pos,int *la,int *ld,int*ls)
{//zwraca wska�nik do wype�nienia tabeli wierzcho�k�w, u�ywane przy tworzeniu E3D terenu
 TSubModel *s=this;
 while (s?s->TextureID!=tex:false)
 {//szukanie submodelu o danej teksturze
  if (s==this)
   s=Child;
  else
   s=s->Next;
 }
 if (!s)
  return NULL; //co� nie tak posz�o
 //WriteLog("Zapis "+t+" od "+pos);
 if (!s->Vertices)
 {//utworznie tabeli tr�jk�t�w
  s->Vertices=new float8[s->iNumVerts];
  //iVboPtr=pos; //pozycja submodelu w tabeli wierzcho�k�w
  //pos+=iNumVerts; //rezerwacja miejsca w tabeli
  s->iVboPtr=iInstance; //pozycja submodelu w tabeli wierzcho�k�w
  iInstance+=s->iNumVerts; //pozycja dla nast�pnego
 }
 s->ColorsSet(la,ld,ls); //ustawienie kolor�w �wiate�
 return s->Vertices+pos; //wska�nik na wolne miejsce w tabeli wierzcho�k�w
};

void __fastcall TSubModel::DisplayLists()
{//utworznie po jednej skompilowanej li�cie dla ka�dego submodelu
 if (Global::bUseVBO) return; //Ra: przy VBO to si� nie przyda
 //iFlags|=0x4000; //wy��czenie przeliczania wierzcho�k�w, bo nie s� zachowane
 if (eType<TP_ROTATOR)
 {
  if (iNumVerts>0)
  {
   uiDisplayList=glGenLists(1);
   glNewList(uiDisplayList,GL_COMPILE);
   glColor3fv(f4Diffuse);   //McZapkie-240702: zamiast ub
#ifdef USE_VERTEX_ARRAYS
  // ShaXbee-121209: przekazywanie wierzcholkow hurtem
   glVertexPointer(3,GL_DOUBLE,sizeof(GLVERTEX),&Vertices[0].Point.x);
   glNormalPointer(GL_DOUBLE,sizeof(GLVERTEX),&Vertices[0].Normal.x);
   glTexCoordPointer(2,GL_FLOAT,sizeof(GLVERTEX),&Vertices[0].tu);
   glDrawArrays(eType,0,iNumVerts);
#else
   glBegin(eType);
   for (int i=0;i<iNumVerts;i++)
   {
/*
    glNormal3dv(&Vertices[i].Normal.x);
    glTexCoord2f(Vertices[i].tu,Vertices[i].tv);
    glVertex3dv(&Vertices[i].Point.x);
*/
    glNormal3fv(&Vertices[i].Normal.x);
    glTexCoord2f(Vertices[i].tu,Vertices[i].tv);
    glVertex3fv(&Vertices[i].Point.x);
   };
   glEnd();
#endif
   glEndList();
  }
 }
 else if (eType==TP_FREESPOTLIGHT)
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
 else if (eType==TP_STARS)
 {//punkty �wiec�ce dook�lnie
  uiDisplayList=glGenLists(1);
  glNewList(uiDisplayList,GL_COMPILE);
  glBindTexture(GL_TEXTURE_2D,0); //tekstury nie ma
  glColorMaterial(GL_FRONT,GL_EMISSION);
  glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
  glBegin(GL_POINTS);
  for (int i=0;i<iNumVerts;i++)
  {
   glColor3f(Vertices[i].Normal.x,Vertices[i].Normal.y,Vertices[i].Normal.z);
   //glVertex3dv(&Vertices[i].Point.x);
   glVertex3fv(&Vertices[i].Point.x);
  };
  glEnd();
  glEnable(GL_LIGHTING);
  glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
  glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
  glEndList();
 }
 //SafeDeleteArray(Vertices); //przy VBO musz� zosta� do za�adowania ca�ego modelu
 if (Child) Child->DisplayLists();
 if (Next) Next->DisplayLists();
};

void __fastcall TSubModel::InitialRotate(bool doit)
{//konwersja uk�adu wsp�rz�dnych na zgodny ze sceneri�
 if (iFlags&0xC000) //je�li jest animacja albo niejednostkowy transform
 {//niejednostkowy transform jest mno�ony i wystarczy zabawy
  if (doit)
  {//obr�t lewostronny
   if (!fMatrix) //macierzy mo�e nie by� w dodanym "bananie"
   {fMatrix=new float4x4(); //tworzy macierz o przypadkowej zawarto�ci
    fMatrix->Identity(); //a zaczynamy obracanie od jednostkowej
   }
   iFlags|=0x8000; //po obr�ceniu b�dzie raczej niejedynkowy matrix
   fMatrix->InitialRotate(); //zmiana znaku X oraz zamiana Y i Z
   if (fMatrix->IdentityIs()) iFlags&=~0x8000; //jednak jednostkowa po obr�ceniu
  }
  if (Child)
   Child->InitialRotate(false); //potomnych nie obracamy ju�, tylko ewentualnie optymalizujemy
  else
   if (Global::iConvertModels&2) //optymalizacja jest opcjonalna
    if ((iFlags&0xC000)==0x8000) //o ile nie ma animacji
    {//jak nie ma potomnych, mo�na wymno�y� przez transform i wyjedynkowa� go
     float4x4 *mat=GetMatrix(); //transform submodelu
     if (Vertices)
     {for (int i=0;i<iNumVerts;++i)
       Vertices[i].Point=(*mat)*Vertices[i].Point;
      (*mat)(3)[0]=(*mat)(3)[1]=(*mat)(3)[2]=0.0; //zerujemy przesuni�cie przed obraniem normalnych
      for (int i=0;i<iNumVerts;++i)
       Vertices[i].Normal=SafeNormalize((*mat)*Vertices[i].Normal);
     }
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
     //wektory normalne r�wnie� trzeba przekszta�ci�, bo si� �le o�wietlaj�
     Vertices[i].Normal.x=-Vertices[i].Normal.x; //zmiana znaku X
     t=Vertices[i].Normal.y; //zamiana Y i Z
     Vertices[i].Normal.y=Vertices[i].Normal.z;
     Vertices[i].Normal.z=t;
    }
   if (Child) Child->InitialRotate(doit); //potomne ewentualnie obr�cimy
  }
 if (Next) Next->InitialRotate(doit);
};

void __fastcall TSubModel::ChildAdd(TSubModel *SubModel)
{//dodanie submodelu potemnego (uzale�nionego)
 //Ra: zmiana kolejno�ci, �eby kolejne m�c renderowa� po aktualnym (by�o przed)
 if (SubModel) SubModel->NextAdd(Child); //Ra: zmiana kolejno�ci renderowania
 Child=SubModel;
};

void __fastcall TSubModel::NextAdd(TSubModel *SubModel)
{//dodanie submodelu kolejnego (wsp�lny przodek)
 if (Next)
  Next->NextAdd(SubModel);
 else
  Next=SubModel;
};

int __fastcall TSubModel::FlagsCheck()
{//analiza koniecznych zmian pomi�dzy submodelami
 //samo pomijanie glBindTexture() nie poprawi wydajno�ci
 //ale mo�na sprawdzi�, czy mo�na w og�le pomin�� kod do tekstur (sprawdzanie replaceskin)
 int i;
 if (Child)
 {//Child jest renderowany po danym submodelu
  if (Child->TextureID) //o ile ma tekstur�
   if (Child->TextureID!=TextureID) //i jest ona inna ni� rodzica
    Child->iFlags|=0x80; //to trzeba sprawdza�, jak z teksturami jest
  i=Child->FlagsCheck();
  iFlags|=0x00FF0000&((i<<16)|(i)|(i>>8)); //potomny, rodze�stwo i dzieci
  if (eType==TP_TEXT)
  {//wy��czenie renderowania Next dla znak�w wy�wietlacza tekstowego
   TSubModel *p=Child;
   while (p)
   {p->iFlags&=0xC0FFFFFF;
    p=p->Next;
   }
  }
 }
 if (Next)
 {//Next jest renderowany po danym submodelu (kolejno�� odwr�cona po wczytaniu T3D)
  if (TextureID) //o ile dany ma tekstur�
   if ((TextureID!=Next->TextureID)||(i&0x00800000)) //a ma inn� albo dzieci zmieniaj�
    iFlags|=0x80; //to dany submodel musi sobie j� ustawia�
  i=Next->FlagsCheck();
  iFlags|=0xFF000000&((i<<24)|(i<<8)|(i)); //nast�pny, kolejne i ich dzieci
  //tekstury nie ustawiamy tylko wtedy, gdy jest taka sama jak Next i jego dzieci nie zmieniaj�
 }
 return iFlags;
};

void __fastcall TSubModel::SetRotate(float3 vNewRotateAxis,float fNewAngle)
{//obr�cenie submodelu wg podanej osi (np. wskaz�wki w kabinie)
 v_RotateAxis=vNewRotateAxis;
 f_Angle=fNewAngle;
 if (fNewAngle!=0.0)
 {b_Anim=at_Rotate;
  b_aAnim=at_Rotate;
 }
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetRotateXYZ(float3 vNewAngles)
{//obr�cenie submodelu o podane k�ty wok� osi lokalnego uk�adu
 v_Angles=vNewAngles;
 b_Anim=at_RotateXYZ;
 b_aAnim=at_RotateXYZ;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetRotateXYZ(vector3 vNewAngles)
{//obr�cenie submodelu o podane k�ty wok� osi lokalnego uk�adu
 v_Angles.x=vNewAngles.x;
 v_Angles.y=vNewAngles.y;
 v_Angles.z=vNewAngles.z;
 b_Anim=at_RotateXYZ;
 b_aAnim=at_RotateXYZ;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetTranslate(float3 vNewTransVector)
{//przesuni�cie submodelu (np. w kabinie)
 v_TransVector=vNewTransVector;
 b_Anim=at_Translate;
 b_aAnim=at_Translate;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetTranslate(vector3 vNewTransVector)
{//przesuni�cie submodelu (np. w kabinie)
 v_TransVector.x=vNewTransVector.x;
 v_TransVector.y=vNewTransVector.y;
 v_TransVector.z=vNewTransVector.z;
 b_Anim=at_Translate;
 b_aAnim=at_Translate;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

void __fastcall TSubModel::SetRotateIK1(float3 vNewAngles)
{//obr�cenie submodelu o podane k�ty wok� osi lokalnego uk�adu
 v_Angles=vNewAngles;
 iAnimOwner=iInstance; //zapami�tanie czyja jest animacja
}

struct ToLower
{
 char operator()(char input) { return tolower(input); }
};

TSubModel* __fastcall TSubModel::GetFromName(AnsiString search,bool i)
{
 return GetFromName(search.c_str(),i);
};

TSubModel* __fastcall TSubModel::GetFromName(char *search,bool i)
{
 TSubModel* result;
 //std::transform(search.begin(),search.end(),search.begin(),ToLower());
 //search=search.LowerCase();
 //AnsiString name=AnsiString();
 if (pName&&search)
  if ((i?stricmp(pName,search):strcmp(pName,search))==0)
   return this;
  else
   if (pName==search)
    return this; //oba NULL
 if (Next)
 {
  result=Next->GetFromName(search);
  if (result) return result;
 }
 if (Child)
 {
  result=Child->GetFromName(search);
  if (result) return result;
 }
 return NULL;
};

//WORD hbIndices[18]={3,0,1,5,4,2,1,0,4,1,5,3,2,3,5,2,4,0};

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
   glRotatef(2.0*Global::fTimeAngleDeg,0.0,1.0,0.0);
   break;
  case at_Hours24: //godziny p�ynnie 24h/360�
   //glRotatef(GlobalTime->hh*15.0+GlobalTime->mm*0.25+GlobalTime->mr/240.0,0.0,1.0,0.0);
   glRotatef(Global::fTimeAngleDeg,0.0,1.0,0.0);
   break;
  case at_Billboard: //obr�t w pionie do kamery
   {matrix4x4 mat; //potrzebujemy wsp�rz�dne przesuni�cia �rodka uk�adu wsp�rz�dnych submodelu
    glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray()); //pobranie aktualnej matrycy
    float3 gdzie=float3(mat[3][0],mat[3][1],mat[3][2]); //pocz�tek uk�adu wsp�rz�dnych submodelu wzgl�dem kamery
    glLoadIdentity(); //macierz jedynkowa
    glTranslatef(gdzie.x,gdzie.y,gdzie.z); //pocz�tek uk�adu zostaje bez zmian
    glRotated(atan2(gdzie.x,gdzie.z)*180.0/M_PI,0.0,1.0,0.0); //jedynie obracamy w pionie o k�t
   }
   break;
  case at_Wind: //ruch pod wp�ywem wiatru (wiatr b�dziemy liczy� potem...)
   glRotated(1.5*sin(M_PI*GlobalTime->mr/6.0),0.0,1.0,0.0);
   break;
  case at_Sky: //animacja nieba
   glRotated(Global::fLatitudeDeg,1.0,0.0,0.0); //ustawienie osi OY na p�noc
   //glRotatef(Global::fTimeAngleDeg,0.0,1.0,0.0); //obr�t dobowy osi OX
   glRotated(-fmod(Global::fTimeAngleDeg,360.0),0.0,1.0,0.0); //obr�t dobowy osi OX
   break;
  case at_IK11: //ostatni element animacji szkieletowej (podudzie, stopa)
   glRotatef(v_Angles.z,0.0,1.0,0.0); //obr�t wzgl�dem osi pionowej (azymut)
   glRotatef(v_Angles.x,1.0,0.0,0.0); //obr�t wzgl�dem poziomu (deklinacja)
   break;
 }
 if (mAnimMatrix) //mo�na by to da� np. do at_Translate
 {glMultMatrixf(mAnimMatrix->readArray());
  mAnimMatrix=NULL; //jak animator b�dzie potrzebowa�, to ustawi ponownie
 }
};

void __fastcall TSubModel::RenderDL()
{//g��wna procedura renderowania przez DL
 if (iVisible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix();
   if (fMatrix)
    glMultMatrixf(fMatrix->readArray());
   if (b_Anim) RaAnimation(b_Anim);
  }
  if (eType<TP_ROTATOR)
  {//renderowanie obiekt�w OpenGL
   if (iAlpha&iFlags&0x1F)  //rysuj gdy element nieprzezroczysty
   {if (TextureID<0) // && (ReplacableSkinId!=0))
    {//zmienialne sk�ry
     glBindTexture(GL_TEXTURE_2D,ReplacableSkinId[-TextureID]);
     //TexAlpha=!(iAlpha&1); //zmiana tylko w przypadku wymienej tekstury
    }
    else
     glBindTexture(GL_TEXTURE_2D,TextureID); //r�wnie� 0
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glCallList(uiDisplayList); //tylko dla siatki
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glCallList(uiDisplayList); //tylko dla siatki
   }
  }
  else if (eType==TP_FREESPOTLIGHT)
  {
   matrix4x4 mat; //macierz opisuje uk�ad renderowania wzgl�dem kamery
   glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray());
   //k�t mi�dzy kierunkiem �wiat�a a wsp�rz�dnymi kamery
   vector3 gdzie=mat*vector3(0,0,0); //pozycja wzgl�dna punktu �wiec�cego
   fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-gdzie),Normalize(gdzie));
   if (fCosViewAngle>fCosFalloffAngle)  //k�t wi�kszy ni� maksymalny sto�ek swiat�a
   {
    double Distdimm=1.0;
    if (fCosViewAngle<fCosHotspotAngle) //zmniejszona jasno�� mi�dzy Hotspot a Falloff
     if (fCosFalloffAngle<fCosHotspotAngle)
      Distdimm=1.0-(fCosHotspotAngle-fCosViewAngle)/(fCosHotspotAngle-fCosFalloffAngle);
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
  else if (eType==TP_STARS)
  {
   //glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
   if (Global::fLuminance<fLight)
   {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
    glCallList(uiDisplayList); //narysuj naraz wszystkie punkty z DL
    glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
   }
  }
  if (Child!=NULL)
    if (iAlpha&iFlags&0x001F0000)
     Child->RenderDL();
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_Anim<at_SecondsJump)
  b_Anim=at_None; //wy��czenie animacji dla kolejnego u�ycia subm
 if (Next)
  if (iAlpha&iFlags&0x1F000000)
   Next->RenderDL(); //dalsze rekurencyjnie
}; //Render

void __fastcall TSubModel::RenderAlphaDL()
{//renderowanie przezroczystych przez DL
 if (iVisible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix();
   if (fMatrix)
    glMultMatrixf(fMatrix->readArray());
   if (b_aAnim) RaAnimation(b_aAnim);
  }
  if (eType<TP_ROTATOR)
  {//renderowanie obiekt�w OpenGL
   if (iAlpha&iFlags&0x2F)  //rysuj gdy element przezroczysty
   {if (TextureID<0) // && (ReplacableSkinId!=0))
    {//zmienialne sk�ry
     glBindTexture(GL_TEXTURE_2D,ReplacableSkinId[-TextureID]);
     //TexAlpha=iAlpha&1; //zmiana tylko w przypadku wymienej tekstury
    }
    else
     glBindTexture(GL_TEXTURE_2D,TextureID); //r�wnie� 0
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glCallList(uiDisplayList); //tylko dla siatki
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glCallList(uiDisplayList); //tylko dla siatki
   }
  }
  else if (eType==TP_FREESPOTLIGHT)
  {
   // dorobi� aureol�!
  }
  if (Child!=NULL)
   if (eType==TP_TEXT)
   {//tekst renderujemy w specjalny spos�b, zamiast submodeli z �a�cucha Child
    int i,j=pasText->Length();
    TSubModel *p;
    char c;
    for (i=1;i<=j;++i) //Ra: szukanie submodeli jest bez sensu, trzeba zrobi� tabelk� wska�nik�w
    {
     c=(*pasText)[i]; //znak do wy�wietlenia
     p=Child;
     while (p?c!=(*p->pName):false) p=p->Next; //szukanie znaku
     if (p)
     {
      p->RenderAlphaDL();
      if (p->fMatrix) glMultMatrixf(p->fMatrix->readArray()); //przesuwanie widoku
     }
    }
   }
   else
    if (iAlpha&iFlags&0x002F0000)
     Child->RenderAlphaDL();
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_aAnim<at_SecondsJump)
  b_aAnim=at_None; //wy��czenie animacji dla kolejnego u�ycia submodelu
 if (Next!=NULL)
  if (iAlpha&iFlags&0x2F000000)
   Next->RenderAlphaDL();
}; //RenderAlpha

void __fastcall TSubModel::RenderVBO()
{//g��wna procedura renderowania przez VBO
 if (iVisible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix();
   if (fMatrix)
    glMultMatrixf(fMatrix->readArray());
   if (b_Anim) RaAnimation(b_Anim);
  }
  if (eType<TP_ROTATOR)
  {//renderowanie obiekt�w OpenGL
   if (iAlpha&iFlags&0x1F)  //rysuj gdy element nieprzezroczysty
   {if (TextureID<0) // && (ReplacableSkinId!=0))
    {//zmienialne sk�ry
     glBindTexture(GL_TEXTURE_2D,ReplacableSkinId[-TextureID]);
     //TexAlpha=!(iAlpha&1); //zmiana tylko w przypadku wymienej tekstury
    }
    else
     glBindTexture(GL_TEXTURE_2D,TextureID); //r�wnie� 0
    glColor3fv(f4Diffuse);   //McZapkie-240702: zamiast ub
    //glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,f4Diffuse); //to samo, co glColor
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glDrawArrays(eType,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glDrawArrays(eType,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
   }
  }
  else if (eType==TP_FREESPOTLIGHT)
  {
   matrix4x4 mat;
   glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray());
   //k�t mi�dzy kierunkiem �wiat�a a wsp�rz�dnymi kamery
   vector3 gdzie=mat*vector3(0,0,0); //pozycja punktu �wiec�cego wzgl�dem kamery
   fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-gdzie),Normalize(gdzie));
   if (fCosViewAngle>fCosFalloffAngle)  //kat wiekszy niz max stozek swiatla
   {
    double Distdimm=1.0;
    if (fCosViewAngle<fCosHotspotAngle) //zmniejszona jasno�� mi�dzy Hotspot a Falloff
     if (fCosFalloffAngle<fCosHotspotAngle)
      Distdimm=1.0-(fCosHotspotAngle-fCosViewAngle)/(fCosHotspotAngle-fCosFalloffAngle);

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
  else if (eType==TP_STARS)
  {
   //glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
   if (Global::fLuminance<fLight)
   {//Ra: pewnie mo�na by to zrobi� lepiej, bez powtarzania StartVBO()
    pRoot->EndVBO(); //Ra: to te� nie jest zbyt �adne
    if (pRoot->StartColorVBO())
    {//wy�wietlanie kolorowych punkt�w zamiast tr�jk�t�w
     glBindTexture(GL_TEXTURE_2D,0); //tekstury nie ma
     glColorMaterial(GL_FRONT,GL_EMISSION);
     glDisable(GL_LIGHTING);  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
     //glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glDrawArrays(GL_POINTS,iVboPtr,iNumVerts);  //narysuj naraz wszystkie punkty z VBO
     glEnable(GL_LIGHTING);
     glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
     //glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
     pRoot->EndVBO();
     pRoot->StartVBO();
    }
   }
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
   if (iAlpha&iFlags&0x001F0000)
    Child->RenderVBO();
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_Anim<at_SecondsJump)
  b_Anim=at_None; //wy��czenie animacji dla kolejnego u�ycia submodelu
 if (Next)
  if (iAlpha&iFlags&0x1F000000)
   Next->RenderVBO(); //dalsze rekurencyjnie
}; //RaRender

void __fastcall TSubModel::RenderAlphaVBO()
{//renderowanie przezroczystych przez VBO
 if (iVisible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
 {
  if (iFlags&0xC000)
  {glPushMatrix(); //zapami�tanie matrycy
   if (fMatrix)
    glMultMatrixf(fMatrix->readArray());
   if (b_aAnim) RaAnimation(b_aAnim);
  }
  glColor3fv(f4Diffuse);
  if (eType<TP_ROTATOR)
  {//renderowanie obiekt�w OpenGL
   if (iAlpha&iFlags&0x2F)  //rysuj gdy element przezroczysty
   {
    if (TextureID<0) // && (ReplacableSkinId!=0))
    {//zmienialne skory
     glBindTexture(GL_TEXTURE_2D,ReplacableSkinId[-TextureID]);
     //TexAlpha=iAlpha&1; //zmiana tylko w przypadku wymienej tekstury
    }
    else
     glBindTexture(GL_TEXTURE_2D,TextureID); //r�wnie� 0
    if (Global::fLuminance<fLight)
    {glMaterialfv(GL_FRONT,GL_EMISSION,f4Diffuse);  //zeby swiecilo na kolorowo
     glDrawArrays(eType,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
     glMaterialfv(GL_FRONT,GL_EMISSION,emm2);
    }
    else
     glDrawArrays(eType,iVboPtr,iNumVerts);  //narysuj naraz wszystkie tr�jk�ty z VBO
   }
  }
  else if (eType==TP_FREESPOTLIGHT)
  {
   // dorobi� aureol�!
  }
  if (Child)
   if (iAlpha&iFlags&0x002F0000)
    Child->RenderAlphaVBO();
  if (iFlags&0xC000)
   glPopMatrix();
 }
 if (b_aAnim<at_SecondsJump)
  b_aAnim=at_None; //wy��czenie animacji dla kolejnego u�ycia submodelu
 if (Next)
  if (iAlpha&iFlags&0x2F000000)
   Next->RenderAlphaVBO();
}; //RaRenderAlpha

//---------------------------------------------------------------------------

void  __fastcall TSubModel::RaArrayFill(CVertNormTex *Vert)
{//wype�nianie tablic VBO
 if (Child) Child->RaArrayFill(Vert);
 if ((eType<TP_ROTATOR)||(eType==TP_STARS))
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
 else if (eType==TP_FREESPOTLIGHT)
  Vert[iVboPtr].x=Vert[iVboPtr].y=Vert[iVboPtr].z=0.0;
 if (Next) Next->RaArrayFill(Vert);
};

void __fastcall TSubModel::Info()
{//zapisanie informacji o submodelu do obiektu pomocniczego
 TSubModelInfo *info=TSubModelInfo::pTable+TSubModelInfo::iCurrent;
 info->pSubModel=this;
 if (fMatrix&&(iFlags&0x8000)) //ma matryc� i jest ona niejednostkowa
  info->iTransform=info->iTotalTransforms++;
 if (TextureID>0)
 {//je�li ma tekstur� niewymienn�
  for (int i=0;i<info->iCurrent;++i)
   if (TextureID==info->pTable[i].pSubModel->TextureID) //por�wnanie z wcze�niejszym
   {info->iTexture=info->pTable[i].iTexture; //taki jaki ju� by�
    break; //koniec sprawdzania
   }
  if (info->iTexture<0) //je�li nie znaleziono we wcze�niejszych
  {info->iTexture=++info->iTotalTextures; //przydzielenie numeru tekstury w pliku (od 1)
   AnsiString t=AnsiString(pTexture);
   if (t.SubString(t.Length()-3,4)==".tga")
    t.Delete(t.Length()-3,4);
   else if (t.SubString(t.Length()-3,4)==".dds")
    t.Delete(t.Length()-3,4);
   if (t!=AnsiString(pTexture))
   {//je�li si� zmieni�o
    //pName=new char[token.length()+1]; //nie ma sensu skraca� tabeli
    strcpy(pTexture,t.c_str());
   }
   info->iTextureLen=t.Length()+1; //przygotowanie do zapisania, z zerem na ko�cu
  }
 }
 else info->iTexture=TextureID; //nie ma albo wymienna
 //if (asName.Length())
 if (pName)
 {info->iName=info->iTotalNames++; //przydzielenie numeru nazwy w pliku (od 0)
  info->iNameLen=strlen(pName)+1; //z zerem na ko�cu
 }
 ++info->iCurrent; //przej�cie do kolejnego obiektu pomocniczego
 if (Child)
 {info->iChild=info->iCurrent;
  Child->Info();
 }
 if (Next)
 {info->iNext=info->iCurrent;
  Next->Info();
 }
};

void __fastcall TSubModel::InfoSet(TSubModelInfo *info)
{//ustawienie danych wg obiektu pomocniczego do zapisania w pliku
 int ile=(char*)&uiDisplayList-(char*)&eType; //ilo�� bajt�w pomi�dzy tymi zmiennymi 
 ZeroMemory(this,sizeof(TSubModel)); //zerowaie ca�o�ci
 CopyMemory(this,info->pSubModel,ile); //skopiowanie pami�ci 1:1
 iTexture=info->iTexture;//numer nazwy tekstury, a nie numer w OpenGL
 TextureID=info->iTexture;//numer tekstury w OpenGL
 iName=info->iName; //numer nazwy w obszarze nazw
 iMatrix=info->iTransform; //numer macierzy
 Next=(TSubModel*)info->iNext; //numer nast�pnego
 Child=(TSubModel*)info->iChild; //numer potomnego
 iFlags&=~0x200; //nie jest wczytany z tekstowego
 //asTexture=asName="";
 pTexture=pName=NULL;
};

void __fastcall TSubModel::BinInit(TSubModel *s,float4x4 *m,float8 *v,TStringPack *t,TStringPack *n)
{//ustawienie wska�nik�w w submodelu
 Child=((int)Child>0)?s+(int)Child:NULL; //zerowy nie mo�e by� potomnym
 Next=((int)Next>0)?s+(int)Next:NULL; //zerowy nie mo�e by� nast�pnym
 fMatrix=((iMatrix>=0)&&m)?m+iMatrix:NULL;
 //if (n&&(iName>=0)) asName=AnsiString(n->String(iName)); else asName="";
 if (n&&(iName>=0)) pName=n->String(iName); else pName=NULL;
 if (iTexture>0)
 {//obs�uga sta�ej tekstury
  //TextureID=TTexturesManager::GetTextureID(t->String(TextureID));
  //asTexture=AnsiString(t->String(iTexture));
  pTexture=t->String(iTexture);
  AnsiString t=AnsiString(pTexture);
  if (t.LastDelimiter("/\\")==0)
   t.Insert(Global::asCurrentTexturePath,1);
  TextureID=TTexturesManager::GetTextureID(t.c_str());
  //TexAlpha=TTexturesManager::GetAlpha(TextureID); //zmienna robocza
  //ustawienie cyklu przezroczyste/nieprzezroczyste zale�nie od w�asno�ci sta�ej tekstury
  iFlags=(iFlags&~0x30)|(TTexturesManager::GetAlpha(TextureID)?0x20:0x10); //0x10-nieprzezroczysta, 0x20-przezroczysta
 }
 b_aAnim=b_Anim; //skopiowanie animacji do drugiego cyklu
 iFlags&=~0x0200; //wczytano z pliku binarnego (nie jest w�a�cicielem tablic)
 Vertices=v+iVboPtr;
 iVisible=1; //tymczasowo u�ywane
 //if (!iNumVerts) eType=-1; //tymczasowo zmiana typu, �eby si� nie renderowa�o na si��
};

void __fastcall TSubModel::ColorsSet(int *a,int *d,int*s)
{//ustawienie kolor�w dla modelu terenu
 int i;
 if (a) for (i=0;i<4;++i) f4Ambient[i]=a[i]/255.0;
 if (d) for (i=0;i<4;++i) f4Diffuse[i]=d[i]/255.0;
 if (s) for (i=0;i<4;++i) f4Specular[i]=s[i]/255.0;
};
//---------------------------------------------------------------------------

__fastcall TModel3d::TModel3d()
{
 //Materials=NULL;
 //MaterialsCount=0;
 Root=NULL;
 iFlags=0;
 iSubModelsCount=0;
 iModel=NULL; //tylko jak wczytany model binarny
 iNumVerts=0; //nie ma jeszcze wierzcho�k�w
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
 //SafeDeleteArray(Materials);
 if (iFlags&0x0200)
 {//wczytany z pliku tekstowego, submodele sprz�taj� same
  SafeDelete(Root); //submodele si� usun� rekurencyjnie
 }
 else
 {//wczytano z pliku binarnego (jest w�a�cicielem tablic)
  m_pVNT=NULL; //nie usuwa� tego, bo wskazuje na iModel
  Root=NULL;
  delete[] iModel; //usuwamy ca�y wczytany plik i to wystarczy
 }
 //p�niej si� jeszcze usuwa obiekt z kt�rego dziedziczymy tabel� VBO
};

void __fastcall TModel3d::AddToNamed(const char *Name,TSubModel *SubModel)
{
 AddTo(Name?GetFromName(Name):NULL,SubModel); //szukanie nadrz�dnego
};

void __fastcall TModel3d::AddTo(TSubModel *tmp,TSubModel *SubModel)
{//jedyny poprawny spos�b dodawania submodeli, inaczej mog� zgin�� przy zapisie E3D
 if (tmp)
 {//je�li znaleziony, pod��czamy mu jako potomny
  tmp->ChildAdd(SubModel);
 }
 else
 {//je�li nie znaleziony, podczepiamy do �a�cucha g��wnego
  SubModel->NextAdd(Root); //Ra: zmiana kolejno�ci renderowania wymusza zmian� tu
  Root=SubModel;
 }
 ++iSubModelsCount; //teraz jest o 1 submodel wi�cej
 iFlags|=0x0200; //submodele s� oddzielne
};

TSubModel* __fastcall TModel3d::GetFromName(const char *sName)
{//wyszukanie submodelu po nazwie
 if (!sName) return Root; //potrzebne do terenu z E3D
 if (iFlags&0x0200) //wczytany z pliku tekstowego, wyszukiwanie rekurencyjne
  return Root?Root->GetFromName(sName):NULL;
 else //wczytano z pliku binarnego, mo�na wyszuka� iteracyjnie
 {
  //for (int i=0;i<iSubModelsCount;++i)
  return Root?Root->GetFromName(sName):NULL;
 }
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

bool __fastcall TModel3d::LoadFromFile(char *FileName,bool dynamic)
{//wczytanie modelu z pliku
 AnsiString name=AnsiString(FileName).LowerCase();
 int i=name.LastDelimiter(".");
 if (i)
  if (name.SubString(i,name.Length()-i+1)==".t3d")
   name.Delete(i,4);
 asBinary=name+".e3d";
 if (FileExists(asBinary))
 {LoadFromBinFile(asBinary.c_str());
  asBinary=""; //wy��czenie zapisu
  Init();
 }
 else
 {if (FileExists(name+".t3d"))
  {LoadFromTextFile(FileName,dynamic); //wczytanie tekstowego
   if (!dynamic) //pojazdy dopiero po ustawieniu animacji
    Init(); //generowanie siatek i zapis E3D
  }
 }
 return Root?(iSubModelsCount>0):false; //brak pliku albo problem z wczytaniem
};

void __fastcall TModel3d::LoadFromBinFile(char *FileName)
{//wczytanie modelu z pliku binarnego
 WriteLog("Loading - binary model: "+AnsiString(FileName));
 int i=0,j,k,ch,size;
 TFileStream *fs=new TFileStream(AnsiString(FileName),fmOpenRead);
 size=fs->Size>>2;
 iModel=new int[size]; //ten wska�nik musi by� w modelu, aby zwolni� pami��
 fs->Read(iModel,fs->Size); //wczytanie pliku
 delete fs;
 float4x4 *m=NULL; //transformy
 //zestaw kromek:
 while ((i<<2)<size) //w pliku mo�e by� kilka modeli
 {ch=iModel[i]; //nazwa kromki
  j=i+(iModel[i+1]>>2); //pocz�tek nast�pnej kromki
  if (ch=='E3D0') //g��wna: 'E3D0',len,pod-kromki
  {//tylko t� kromk� znamy, mo�e kiedy� jeszcze DOF si� zrobi
   i+=2;
   while (i<j)
   {//przetwarzanie kromek wewn�trznych
    ch=iModel[i]; //nazwa kromki
    k=(iModel[i+1]>>2); //d�ugo�� aktualnej kromki
    switch (ch)
    {case 'MDL0': //zmienne modelu: 'E3D0',len,(informacje o modelu)
      break;
     case 'VNT0': //wierzcho�ki: 'VNT0',len,(32 bajty na wierzcho�ek)
      iNumVerts=(k-2)>>3;
      m_nVertexCount=iNumVerts;
      m_pVNT=(CVertNormTex*)(iModel+i+2);
      break;
     case 'SUB0': //submodele: 'SUB0',len,(256 bajt�w na submodel)
      iSubModelsCount=(k-2)/64;
      Root=(TSubModel*)(iModel+i+2); //numery na wska�niki przetworzymy p�niej
      break;
     case 'SUB1': //submodele: 'SUB1',len,(320 bajt�w na submodel)
      iSubModelsCount=(k-2)/80;
      Root=(TSubModel*)(iModel+i+2); //numery na wska�niki przetworzymy p�niej
      for (ch=1;ch<iSubModelsCount;++ch) //trzeba przesun�� bli�ej, bo 256 wystarczy
       MoveMemory(((char*)Root)+256*ch,((char*)Root)+320*ch,256);
      break;
     case 'TRA0': //transformy: 'TRA0',len,(64 bajty na transform)
      m=(float4x4*)(iModel+i+2); //tabela transform�w
      break;
     case 'TRA1': //transformy: 'TRA1',len,(128 bajt�w na transform)
      m=(float4x4*)(iModel+i+2); //tabela transform�w
      for (ch=0;ch<((k-2)>>1);++ch)
       *(((float*)m)+ch)=*(((double*)m)+ch); //przepisanie double do float
      break;
     case 'IDX1': //indeksy 1B: 'IDX2',len,(po bajcie na numer wierzcho�ka)
      break;
     case 'IDX2': //indeksy 2B: 'IDX2',len,(po 2 bajty na numer wierzcho�ka)
      break;
     case 'IDX4': //indeksy 4B: 'IDX4',len,(po 4 bajty na numer wierzcho�ka)
      break;
     case 'TEX0': //tekstury: 'TEX0',len,(�a�cuchy zako�czone zerem - pliki tekstur)
      Textures.Init((char*)(iModel+i)); //��cznie z nag��wkiem
      break;
     case 'TIX0': //indeks nazw tekstur
      Textures.InitIndex((int*)(iModel+i)); //��cznie z nag��wkiem
      break;
     case 'NAM0': //nazwy: 'NAM0',len,(�a�cuchy zako�czone zerem - nazwy submodeli)
      Names.Init((char*)(iModel+i)); //��cznie z nag��wkiem
      break;
     case 'NIX0': //indeks nazw submodeli
      Names.InitIndex((int*)(iModel+i)); //��cznie z nag��wkiem
      break;
    }
    i+=k; //przej�cie do kolejnej kromki
   }
  }
  i=j;
 }
 for (i=0;i<iSubModelsCount;++i)
  Root[i].BinInit(Root,m,(float8*)m_pVNT,&Textures,&Names); //aktualizacja wska�nik�w w submodelach
 iFlags&=~0x0200;
 return;
};

void __fastcall TModel3d::LoadFromTextFile(char *FileName,bool dynamic)
{//wczytanie submodelu z pliku tekstowego
 WriteLog("Loading - text model: "+AnsiString(FileName));
 iFlags|=0x0200; //wczytano z pliku tekstowego (w�a�cicielami tablic s� submodle)
 cParser parser(FileName,cParser::buffer_FILE); //Ra: tu powinno by� "models\\"...
 TSubModel *SubModel;
 std::string token;
 parser.getToken(token);
 iNumVerts=0; //w konstruktorze to jest
 while (token!="" || parser.eof())
 {
  std::string parent;
  //parser.getToken(parent);
  parser.getTokens(1,false); //nazwa submodelu nadrz�dnego bez zmieny na ma�e
  parser >> parent;
  if (parent=="") break;
  SubModel=new TSubModel();
  iNumVerts+=SubModel->Load(parser,this,iNumVerts);
  AddToNamed(parent.c_str(),SubModel);
  //iSubModelsCount++;
  parser.getToken(token);
 }
 //Ra: od wersji 334 przechylany jest ca�y model, a nie tylko pierwszy submodel
 //ale bujanie kabiny nadal u�ywa banan�w :( od 393 przywr�cone, ale z dodatkowym warunkiem
 if (Global::iConvertModels&4)
 {//automatyczne banany czasem psu�y przechylanie kabin...
  if (dynamic&&Root)
  {if (Root->NextGet()) //je�li ma jakiekolwiek kolejne
   {//dynamic musi mie� "banana", bo tylko pierwszy obiekt jest animowany, a nast�pne nie
    SubModel=new TSubModel(); //utworzenie pustego
    SubModel->ChildAdd(Root);
    Root=SubModel;
    ++iSubModelsCount;
   }
   Root->WillBeAnimated(); //bo z tym jest du�o problem�w
  }
 }
}

void __fastcall TModel3d::Init()
{//obr�cenie pocz�tkowe uk�adu wsp�rz�dnych, dla pojazd�w wykonywane po analizie animacji
 if (iFlags&0x8000) return; //operacje zosta�y ju� wykonane
 if (Root)
 {if (iFlags&0x0200) //je�li wczytano z pliku tekstowego
  {//jest jaki� dziwny b��d, �e obkr�cany ma by� tylko ostatni submodel g��wnego �a�cucha
   //TSubModel *p=Root;
   //do
   //{p->InitialRotate(true); //ostatniemu nale�y si� konwersja uk�adu wsp�rz�dnych
   // p=p->NextGet();
   //}
   //while (p->NextGet())
   //Root->InitialRotate(false); //a poprzednim tylko optymalizacja
   Root->InitialRotate(true); //argumet okre�la, czy wykona� pierwotny obr�t
  }
  iFlags|=Root->FlagsCheck()|0x8000; //flagi ca�ego modelu
  if (!asBinary.IsEmpty()) //je�li jest podana nazwa
  {if (Global::iConvertModels) //i w��czony zapis
    SaveToBinFile(asBinary.c_str()); //utworzy tablic� (m_pVNT)
   asBinary=""; //zablokowanie powt�rnego zapisu
  }
  if (iNumVerts)
  {
   if (Global::bUseVBO)
   {if (!m_pVNT) //je�li nie ma jeszcze tablicy (wczytano z pliku tekstowego)
    {//tworzenie tymczasowej tablicy z wierzcho�kami ca�ego modelu
     MakeArray(iNumVerts); //tworzenie tablic dla VBO
     Root->RaArrayFill(m_pVNT); //wype�nianie tablicy
     BuildVBOs(); //tworzenie VBO i usuwanie tablicy z pami�ci
    }
    else
     BuildVBOs(false); //tworzenie VBO bez usuwania tablicy z pami�ci
   }
   else
   {//przygotowanie skompilowanych siatek dla DisplayLists
    Root->DisplayLists(); //tworzenie skompilowanej listy dla submodelu
   }
   //if (Root->TextureID) //o ile ma tekstur�
   // Root->iFlags|=0x80; //konieczno�� ustawienia tekstury
  }
 }
};

void __fastcall TModel3d::SaveToBinFile(char *FileName)
{//zapis modelu binarnego
 WriteLog("Saving E3D binary model.");
 int i,zero=0;
 TSubModelInfo *info=new TSubModelInfo[iSubModelsCount];
 info->Reset();
 Root->Info(); //zebranie informacji o submodelach
 int len; //��czna d�ugo�� pliku
 int sub; //ilo�� submodeli (w bajtach)
 int tra; //wielko�� obszaru transform�w
 int vnt; //wielko�� obszaru wierzcho�k�w
 int tex=0; //wielko�� obszaru nazw tekstur
 int nam=0; //wielko�� obszaru nazw submodeli
 sub=8+sizeof(TSubModel)*iSubModelsCount;
 tra=info->iTotalTransforms?8+64*info->iTotalTransforms:0;
 vnt=8+32*iNumVerts;
 for (i=0;i<iSubModelsCount;++i)
 {tex+=info[i].iTextureLen;
  nam+=info[i].iNameLen;
 }
 if (tex) tex+=9; //8 na nag��wek i jeden ci�g pusty (tylko znacznik ko�ca)
 if (nam) nam+=8;
 len=8+sub+tra+vnt+tex+((-tex)&3)+nam+((-nam)&3);
 TSubModel *roboczy=new TSubModel(); //bufor u�ywany do zapisywania
 //AnsiString *asN=&roboczy->asName,*asT=&roboczy->asTexture;
 //roboczy->FirstInit(); //�eby delete nie usuwa�o czego nie powinno
 TFileStream *fs=new TFileStream(AnsiString(FileName),fmCreate);
 fs->Write("E3D0",4); //kromka g��wna
 fs->Write(&len,4);
 {fs->Write("SUB0",4); //dane submodeli
  fs->Write(&sub,4);
  for (i=0;i<iSubModelsCount;++i)
  {roboczy->InfoSet(info+i);
   fs->Write(roboczy,sizeof(TSubModel)); //zapis jednego submodelu
  }
 }
 if (tra)
 {//zapis transform�w
  fs->Write("TRA0",4); //transformy
  fs->Write(&tra,4);
  for (i=0;i<iSubModelsCount;++i)
   if (info[i].iTransform>=0)
    fs->Write(info[i].pSubModel->GetMatrix(),16*4);
 }
 {//zapis wierzcho�k�w
  MakeArray(iNumVerts); //tworzenie tablic dla VBO
  Root->RaArrayFill(m_pVNT); //wype�nianie tablicy
  fs->Write("VNT0",4); //wierzcho�ki
  fs->Write(&vnt,4);
  fs->Write(m_pVNT,32*iNumVerts);
 }
 if (tex) //mo�e by� jeden submodel ze zmienn� tekstur� i nazwy nie b�dzie
 {//zapis nazw tekstur
  fs->Write("TEX0",4); //nazwy tekstur
  i=(tex+3)&~3; //zaokr�glenie w g�r�
  fs->Write(&i,4);
  fs->Write(&zero,1); //ci�g o numerze zero nie jest u�ywany, ma tylko znacznik ko�ca
  for (i=0;i<iSubModelsCount;++i)
   if (info[i].iTextureLen)
    fs->Write(info[i].pSubModel->pTexture,info[i].iTextureLen);
  if ((-tex)&3) fs->Write(&zero,((-tex)&3)); //wyr�wnanie do wielokrotno�ci 4 bajt�w
 }
 if (nam) //mo�e by� jeden anonimowy submodel w modelu
 {//zapis nazw submodeli
  fs->Write("NAM0",4); //nazwy submodeli
  i=(nam+3)&~3; //zaokr�glenie w g�r�
  fs->Write(&i,4);
  for (i=0;i<iSubModelsCount;++i)
   if (info[i].iNameLen)
    fs->Write(info[i].pSubModel->pName,info[i].iNameLen);
  if ((-nam)&3) fs->Write(&zero,((-nam)&3)); //wyr�wnanie do wielokrotno�ci 4 bajt�w
 }
 delete fs;
 //roboczy->FirstInit(); //�eby delete nie usuwa�o czego nie powinno
 //roboczy->iFlags=0; //�eby delete nie usuwa�o czego nie powinno
 //roboczy->asName)=asN;
 //&roboczy->asTexture=asT;
 delete roboczy;
 delete[] info;
};

void __fastcall TModel3d::BreakHierarhy()
{
 Error("Not implemented yet :(");
};

/*
void __fastcall TModel3d::Render(vector3 pPosition,double fAngle,GLuint ReplacableSkinId,int iAlpha)
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
        Root->Render(ReplacableSkinId,iAlpha);
#else
    Root->Render(ReplacableSkinId,iAlpha);
#endif
    glPopMatrix();
};
*/

void __fastcall TModel3d::Render(double fSquareDistance,GLuint *ReplacableSkinId,int iAlpha)
{
 iAlpha^=0x0F0F000F; //odwr�cenie flag tekstur, aby wy�apa� nieprzezroczyste
 if (iAlpha&iFlags&0x1F1F001F) //czy w og�le jest co robi� w tym cyklu?
 {TSubModel::fSquareDist=fSquareDistance; //zmienna globalna!
  Root->ReplacableSet(ReplacableSkinId,iAlpha);
  Root->RenderDL();
 }
};

void __fastcall TModel3d::RenderAlpha(double fSquareDistance,GLuint *ReplacableSkinId,int iAlpha)
{
 if (iAlpha&iFlags&0x2F2F002F)
 {
  TSubModel::fSquareDist=fSquareDistance; //zmienna globalna!
  Root->ReplacableSet(ReplacableSkinId,iAlpha);
  Root->RenderAlphaDL();
 }
};

/*
void __fastcall TModel3d::RaRender(vector3 pPosition,double fAngle,GLuint *ReplacableSkinId,int iAlpha)
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
/*
 fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {//odwr�cenie flag, aby wy�apa� nieprzezroczyste
  Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
  Root->RaRender();
  EndVBO();
 }
 glPopMatrix(); //przywr�cenie ustawie� przekszta�cenia
};
*/

void __fastcall TModel3d::RaRender(double fSquareDistance,GLuint *ReplacableSkinId,int iAlpha)
{//renderowanie specjalne, np. kabiny
 iAlpha^=0x0F0F000F; //odwr�cenie flag tekstur, aby wy�apa� nieprzezroczyste
 if (iAlpha&iFlags&0x1F1F001F) //czy w og�le jest co robi� w tym cyklu?
 {TSubModel::fSquareDist=fSquareDistance; //zmienna globalna!
  if (StartVBO())
  {//odwr�cenie flag, aby wy�apa� nieprzezroczyste
   Root->ReplacableSet(ReplacableSkinId,iAlpha);
   Root->pRoot=this;
   Root->RenderVBO();
   EndVBO();
  }
 }
};

void __fastcall TModel3d::RaRenderAlpha(double fSquareDistance,GLuint *ReplacableSkinId,int iAlpha)
{//renderowanie specjalne, np. kabiny
 if (iAlpha&iFlags&0x2F2F002F) //czy w og�le jest co robi� w tym cyklu?
 {TSubModel::fSquareDist=fSquareDistance; //zmienna globalna!
  if (StartVBO())
  {Root->ReplacableSet(ReplacableSkinId,iAlpha);
   Root->RenderAlphaVBO();
   EndVBO();
  }
 }
};

/*
void __fastcall TModel3d::RaRenderAlpha(vector3 pPosition,double fAngle,GLuint *ReplacableSkinId,int iAlpha)
{
 glPushMatrix();
 glTranslatef(pPosition.x,pPosition.y,pPosition.z);
 if (fAngle!=0)
  glRotatef(fAngle,0,1,0);
 fSquareDist=SquareMagnitude(pPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {Root->ReplacableSet(ReplacableSkinId,iAlpha);
  Root->RaRenderAlpha();
  EndVBO();
 }
 glPopMatrix();
};
*/

//-----------------------------------------------------------------------------
//2011-03-16 cztery nowe funkcje renderowania z mo�liwo�ci� pochylania obiekt�w
//-----------------------------------------------------------------------------

void __fastcall TModel3d::Render(vector3 *vPosition,vector3 *vAngle,GLuint *ReplacableSkinId,int iAlpha)
{//nieprzezroczyste, Display List
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
 TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition()); //zmienna globalna!
 //odwr�cenie flag, aby wy�apa� nieprzezroczyste
 Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
 Root->RenderDL();
 glPopMatrix();
};
void __fastcall TModel3d::RenderAlpha(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId,int iAlpha)
{//przezroczyste, Display List
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
 TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition()); //zmienna globalna!
 Root->ReplacableSet(ReplacableSkinId,iAlpha);
 Root->RenderAlphaDL();
 glPopMatrix();
};
void __fastcall TModel3d::RaRender(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId,int iAlpha)
{//nieprzezroczyste, VBO
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
 TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {//odwr�cenie flag, aby wy�apa� nieprzezroczyste
  Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
  Root->RenderVBO();
  EndVBO();
 }
 glPopMatrix();
};
void __fastcall TModel3d::RaRenderAlpha(vector3* vPosition,vector3* vAngle,GLuint *ReplacableSkinId,int iAlpha)
{//przezroczyste, VBO
 glPushMatrix();
 glTranslated(vPosition->x,vPosition->y,vPosition->z);
 if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
 if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
 if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
 TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {Root->ReplacableSet(ReplacableSkinId,iAlpha);
  Root->RenderAlphaVBO();
  EndVBO();
 }
 glPopMatrix();
};

//-----------------------------------------------------------------------------
//2012-02 funkcje do tworzenia terenu z E3D
//-----------------------------------------------------------------------------

int __fastcall TModel3d::TerrainCount()
{//zliczanie kwadrat�w kilometrowych (g��wna linia po Next) do tworznia tablicy
 int i=0;
 TSubModel *r=Root;
 while (r)
 {r=r->NextGet();
  ++i;
 }
 return i;
};
TSubModel* __fastcall TModel3d::TerrainSquare(int n)
{//pobieranie wska�nika do submodelu (n)
 int i=0;
 TSubModel *r=Root;
 while (i<n)
 {r=r->NextGet();
  ++i;
 }
 r->UnFlagNext(); //blokowanie wy�wietlania po Next g��wnej listy
 return r;
};
void __fastcall TModel3d::TerrainRenderVBO(int n)
{//renderowanie terenu z VBO
 glPushMatrix();
 //glTranslated(vPosition->x,vPosition->y,vPosition->z);
 //if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
 //if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
 //if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
 //TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition()); //zmienna globalna!
 if (StartVBO())
 {//odwr�cenie flag, aby wy�apa� nieprzezroczyste
  //Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
  TSubModel *r=Root;
  while (r)
  {
   if (r->iVisible==n) //tylko je�li ma by� widoczny w danej ramce (problem dla 0==false)
    r->RenderVBO(); //sub kolejne (Next) si� nie wyrenderuj�
   r=r->NextGet();
  }
  EndVBO();
 }
 glPopMatrix();
};

