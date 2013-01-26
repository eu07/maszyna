//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Names.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
/*
Modu� zarz�dzaj�cy plikami oraz wyszukiwaniem obiekt�w wg nazw.
1. Ma przydzielony z g�ry (EU07.INI) obszar pami�ci (rz�du 16MB).
2. W przypadku przepe�nienia dost�pnej pami�ci wyst�pi b��d wczytywania.
3. Obszar ten b�dzie zu�ywany na rekordy obiekt�w oraz ci�gi tekstowe z nazwami.
4. Rekordy b�d� sortowane w ramach typu (tekstury, d�wi�ki, modele, node, eventy).
5. Pierwszy etap wyszukiwania to 5 bit�w z pierwszego bajtu i 3 z drugiego (256).
6. Dla plik�w istnieje mo�liwo�� wczytania ich w innym terminie.
7. Mo�liwo�� wczytania plik�w w oddzielnym watku (np. tekstur).

Obs�ugiwane pliki:
1. Tekstury, mo�na wczytywa� p�niej, rekord przechowuje numer podany przez kart� graficzn�.
2. D�wi�ki, mo�na wczyta� p�niej.
3. Modele, mo�na wczyta� p�niej o ile nie maj� animacji eventami i nie dotycz� pojazd�w.

Obiekty sortowane wg nazw, mo�na dodawa� i usuwa� kom�rki scenerii:
4. Tory, drogi, rzeki - wyszukiwanie w celu sprawdzenia zajeto�ci.
5. Eventy - wyszukiwane przy zewn�trznym wywo�aniu oraz podczas wczytywania.
6. Pojazdy - wyszukiwane w celu wysy�ania komend.
7. Egzemplarze modeli animowanych - wyszukiwanie w celu po��czenia z animacjami.

*/

void __fastcall ItemRecord::TreeAdd(ItemRecord *r,int c)
{//dodanie rekordu do drzewa - ustalenie w kt�rej ga��zi
 //zapisa� w (iFlags) ile znak�w jest zgodnych z nadrz�dnym, �eby nie sprawdza� wszystkich od zera
 if ((cName[c]&&r->cName[c])?cName[c]==r->cName[c]:false)
  TreeAdd(r,c+1); //ustawi� wg kolejnego znaku, chyba �e zero
 else
  if ((unsigned char)(cName[c])<(unsigned char)(r->cName[c]))
  {//zero jest najmniejsze - doczepiamy jako (rNext)
   if (!rNext) rNext=r;
   else rNext->TreeAdd(r,0); //doczepi� do tej ga��zi
  }
  else
  {
   if (!rPrev) rPrev=r;
   else rPrev->TreeAdd(r,0); //doczepi� do tej ga��zi
  }
};

void __fastcall ItemRecord::ListGet(ItemRecord *r,int*&n)
{//rekurencyjne wype�nianie posortowanej listy na podstawie drzewa
 if (rPrev) rPrev->ListGet(r,n); //dodanie wszystkich wcze�niejszych
 *n++=this-r; //dodanie swojego indeksu do tabeli
 if (rNext) rNext->ListGet(r,n); //dodanie wszystkich p�niejszych
};

void* __fastcall ItemRecord::TreeFind(const char *n)
{//wyszukanie ci�gu (n)
 ItemRecord *r=TreeFindRecord(n);
 return r?r->pData:NULL;
};

ItemRecord* __fastcall ItemRecord::TreeFindRecord(const char *n)
{//wyszukanie ci�gu (n)
 ItemRecord *r=this; //�eby nie robi� rekurencji
 int i=0;
 do
 {
  if (!n[i]) if (!r->cName[i]) return r; //znaleziony
  if (n[i]==r->cName[i])
   ++i; //por�wna� kolejny znak
  else
   if ((unsigned char)(n[i])<(unsigned char)(r->cName[i]))
   {
    i=0; //por�wnywa� od nowa
    r=r->rPrev; //wcze�niejsza ga��� drzewa
   }
   else
   {
    i=0; //por�wnywa� od nowa
    r=r->rNext; //p�niejsza ga��� drzewa
   }
 } while (r);
 return NULL;
};

__fastcall TNames::TNames()
{//tworzenie bufora
 iSize=16*1024*1024; //rozmiar bufora w bajtach
 cBuffer=new char[iSize];
 ZeroMemory(cBuffer,iSize); //nie trzyma� jaki� starych �mieci
 rRecords=(ItemRecord*)cBuffer;
 cLast=cBuffer+iSize; //bajt za buforem
 iLast=-1;
 ZeroMemory(rTypes,20*sizeof(ItemRecord*));
};

int __fastcall TNames::Add(int t,const char *n)
{//dodanie obiektu typu (t) o nazwie (n)
 int len=strlen(n)+1; //ze znacznikiem ko�ca
 cLast-=len; //rezerwacja miejsca
 memcpy(cLast,n,len); //przekopiowanie tekstu do bufora
 //cLast[len-1]='\0';
 rRecords[++iLast].cName=cLast; //po��czenie nazwy z rekordem
 rRecords[iLast].iFlags=t;
 if (!rTypes[t])
  rTypes[t]=rRecords+iLast; //korze� drzewa, bo nie by�o wcze�niej
 else
  rTypes[t]->TreeAdd(rRecords+iLast,0); //doczepienie jako ga���
 //rTypes[t]=Sort(t); //sortowanie uruchamia� r�cznie
 if ((iLast&0x3F)==0) //nie za cz�sto, bo sortowania zajm� wi�cej czasu ni� wyszukiwania
  Sort(t); //optymalizacja drzewa co jaki� czas
 return iLast;
}
int __fastcall TNames::Add(int t,const char *n,void *d)
{
 int i=Add(t,n);
 rRecords[iLast].pData=d;
 return i;
};

bool __fastcall TNames::Update(int t,const char *n,void *d)
{//dodanie je�li nie ma, wymiana (d), gdy jest
 ItemRecord *r=FindRecord(t,n); //najpierw sprawdzi�, czy ju� jest
 if (r)
 {//przy zdublowaniu nazwy podmienia� w drzewku na p�niejszy
  r->pData=d;
  return true; //duplikat
 }
 //Add(t,n,d); //nazwa unikalna
 return false; //zosta� dodany nowy
};

ItemRecord* __fastcall TNames::TreeSet(int *n,int d,int u)
{//rekurencyjne wype�nianie drzewa pozycjami od (d) do (u)
 if (d==u)
 {
  rRecords[n[d]].rPrev=rRecords[n[d]].rNext=NULL;
  return rRecords+n[d]; //tej ga��zi nie ma
 }
 else if (d>u) return NULL;
 int p=(u+d)>>1; //po�owa
 rRecords[n[p]].rPrev=TreeSet(n,d,p-1); //zapisanie wcze�niejszych ga��zi
 rRecords[n[p]].rNext=TreeSet(n,p+1,u); //zapisanie p�niejszych ga��zi
 return rRecords+n[p];
};

void __fastcall TNames::Sort(int t)
{//przebudowa drzewa typu (t), zwraca wierzcho�ek drzewa
 if (iLast<3) return; //jak jest ma�o, to nie ma sensu sortowa�
 if (rTypes[t]) //je�li jest jaki� rekord danego typu
 {int *r=new int[iLast+1]; //robocza tablica indeks�w - numery posortowanych rekord�w
  int *q=r; //wska�nik roboczy, przekazywany przez referencj�
  rTypes[t]->ListGet(rRecords,q); //drzewo jest ju� posortowane - zamieni� je na list�
  rTypes[t]=TreeSet(r,0,(q-r)-1);
  delete[] r;
 }
 return;
};

ItemRecord* __fastcall TNames::FindRecord(const int t,const char *n)
{//poszukiwanie rekordu w celu np. zmiany wska�nika
 return rTypes[t]?rTypes[t]->TreeFindRecord(n):NULL;
};

