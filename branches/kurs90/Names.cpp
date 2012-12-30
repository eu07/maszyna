//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Names.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
/*
Modu³ zarz¹dzaj¹cy plikami oraz wyszukiwaniem obiektów wg nazw.
1. Ma przydzielony z góry (EU07.INI) obszar pamiêci (rzêdu 16MB).
2. W przypadku przepe³nienia dostêpnej pamiêci wyst¹pi b³¹d wczytywania.
3. Obszar ten bêdzie zu¿ywany na rekordy obiektów oraz ci¹gi tekstowe z nazwami.
4. Rekordy bêd¹ sortowane w ramach typu (tekstury, dŸwiêki, modele, node, eventy).
5. Pierwszy etap wyszukiwania to 5 bitów z pierwszego bajtu i 3 z drugiego (256).
6. Dla plików istnieje mo¿liwoœæ wczytania ich w innym terminie.
7. Mo¿liwoœæ wczytania plików w oddzielnym watku (np. tekstur).

Obs³ugiwane pliki:
1. Tekstury, mo¿na wczytywaæ póŸniej, rekord przechowuje numer podany przez kartê graficzn¹.
2. DŸwiêki, mo¿na wczytaæ póŸniej.
3. Modele, mo¿na wczytaæ póŸniej o ile nie maj¹ animacji eventami i nie dotycz¹ pojazdów.

Obiekty sortowane wg nazw, mo¿na dodawaæ i usuwaæ komórki scenerii:
4. Tory, drogi, rzeki - wyszukiwanie w celu sprawdzenia zajetoœci.
5. Eventy - wyszukiwane przy zewnêtrznym wywo³aniu oraz podczas wczytywania.
6. Pojazdy - wyszukiwane w celu wysy³ania komend.
7. Egzemplarze modeli animowanych - wyszukiwanie w celu po³¹czenia z animacjami.

*/

void __fastcall ItemRecord::TreeAdd(ItemRecord *r,int c)
{//dodanie rekordu do drzewa - ustalenie w której ga³êzi
 //zapisaæ w (iFlags) ile znaków jest zgodnych z nadrzêdnym, ¿eby nie sprawdzaæ wszystkich od zera
 if ((cName[c]&&r->cName[c])?cName[c]==r->cName[c]:false)
  TreeAdd(r,c+1); //ustawiæ wg kolejnego znaku, chyba ¿e zero
 else
  if ((unsigned char)(cName[c])<(unsigned char)(r->cName[c]))
  {//zero jest najmniejsze - doczepiamy jako (rNext)
   if (!rNext) rNext=r;
   else rNext->TreeAdd(r,0); //doczepiæ do tej ga³êzi
  }
  else
  {
   if (!rPrev) rPrev=r;
   else rPrev->TreeAdd(r,0); //doczepiæ do tej ga³êzi
  }
};

void __fastcall ItemRecord::ListGet(ItemRecord *r,int*&n)
{//rekurencyjne wype³nianie posortowanej listy na podstawie drzewa
 if (rPrev) rPrev->ListGet(r,n); //dodanie wszystkich wczeœniejszych
 *n++=this-r; //dodanie swojego indeksu do tabeli
 if (rNext) rNext->ListGet(r,n); //dodanie wszystkich póŸniejszych
};

void* __fastcall ItemRecord::TreeFind(const char *n)
{//wyszukanie ci¹gu (n)
 ItemRecord *r=TreeFindRecord(n);
 return r?r->pData:NULL;
};

ItemRecord* __fastcall ItemRecord::TreeFindRecord(const char *n)
{//wyszukanie ci¹gu (n)
 ItemRecord *r=this; //¿eby nie robiæ rekurencji
 int i=0;
 do
 {
  if (!n[i]) if (!r->cName[i]) return r; //znaleziony
  if (n[i]==r->cName[i])
   ++i; //porównaæ kolejny znak
  else
   if ((unsigned char)(n[i])<(unsigned char)(r->cName[i]))
   {
    i=0; //porównywaæ od nowa
    r=r->rPrev; //wczeœniejsza ga³¹Ÿ drzewa
   }
   else
   {
    i=0; //porównywaæ od nowa
    r=r->rNext; //póŸniejsza ga³¹Ÿ drzewa
   }
 } while (r);
 return NULL;
};

__fastcall TNames::TNames()
{//tworzenie bufora
 iSize=16*1024*1024; //rozmiar bufora w bajtach
 cBuffer=new char[iSize];
 ZeroMemory(cBuffer,iSize); //nie trzymaæ jakiœ starych œmieci
 rRecords=(ItemRecord*)cBuffer;
 cLast=cBuffer+iSize; //bajt za buforem
 iLast=-1;
 ZeroMemory(rTypes,20*sizeof(ItemRecord*));
};

int __fastcall TNames::Add(int t,const char *n)
{//dodanie obiektu typu (t) o nazwie (n)
 int len=strlen(n)+1; //ze znacznikiem koñca
 cLast-=len; //rezerwacja miejsca
 memcpy(cLast,n,len); //przekopiowanie tekstu do bufora
 //cLast[len-1]='\0';
 rRecords[++iLast].cName=cLast; //po³¹czenie nazwy z rekordem
 rRecords[iLast].iFlags=t;
 if (!rTypes[t])
  rTypes[t]=rRecords+iLast; //korzeñ drzewa, bo nie by³o wczeœniej
 else
  rTypes[t]->TreeAdd(rRecords+iLast,0); //doczepienie jako ga³¹Ÿ
 //rTypes[t]=Sort(t); //sortowanie uruchamiaæ rêcznie
 if ((iLast&0x3F)==0) //nie za czêsto, bo sortowania zajm¹ wiêcej czasu ni¿ wyszukiwania
  Sort(t); //optymalizacja drzewa co jakiœ czas
 return iLast;
}
int __fastcall TNames::Add(int t,const char *n,void *d)
{
 int i=Add(t,n);
 rRecords[iLast].pData=d;
 return i;
};

bool __fastcall TNames::Update(int t,const char *n,void *d)
{//dodanie jeœli nie ma, wymiana (d), gdy jest
 ItemRecord *r=FindRecord(t,n); //najpierw sprawdziæ, czy ju¿ jest
 if (r)
 {//przy zdublowaniu nazwy podmieniaæ w drzewku na póŸniejszy
  r->pData=d;
  return true; //duplikat
 }
 //Add(t,n,d); //nazwa unikalna
 return false; //zosta³ dodany nowy
};

ItemRecord* __fastcall TNames::TreeSet(int *n,int d,int u)
{//rekurencyjne wype³nianie drzewa pozycjami od (d) do (u)
 if (d==u)
 {
  rRecords[n[d]].rPrev=rRecords[n[d]].rNext=NULL;
  return rRecords+n[d]; //tej ga³êzi nie ma
 }
 else if (d>u) return NULL;
 int p=(u+d)>>1; //po³owa
 rRecords[n[p]].rPrev=TreeSet(n,d,p-1); //zapisanie wczeœniejszych ga³êzi
 rRecords[n[p]].rNext=TreeSet(n,p+1,u); //zapisanie póŸniejszych ga³êzi
 return rRecords+n[p];
};

void __fastcall TNames::Sort(int t)
{//przebudowa drzewa typu (t), zwraca wierzcho³ek drzewa
 if (iLast<3) return; //jak jest ma³o, to nie ma sensu sortowaæ
 if (rTypes[t]) //jeœli jest jakiœ rekord danego typu
 {int *r=new int[iLast+1]; //robocza tablica indeksów - numery posortowanych rekordów
  int *q=r; //wskaŸnik roboczy, przekazywany przez referencjê
  rTypes[t]->ListGet(rRecords,q); //drzewo jest ju¿ posortowane - zamieniæ je na listê
  rTypes[t]=TreeSet(r,0,(q-r)-1);
  delete[] r;
 }
 return;
};

ItemRecord* __fastcall TNames::FindRecord(const int t,const char *n)
{//poszukiwanie rekordu w celu np. zmiany wskaŸnika
 return rTypes[t]?rTypes[t]->TreeFindRecord(n):NULL;
};

