//---------------------------------------------------------------------------

#ifndef NamesH
#define NamesH
//---------------------------------------------------------------------------
class ItemRecord
{//rekord opisuj�cy obiekt; raz utworzony nie przemieszcza si�
public:
 char* cName; //wska�nik do nazwy umieszczonej w buforze
 int iFlags; //flagi bitowe
 ItemRecord *rPrev,*rNext; //posortowane drzewo (przebudowywane)
 union
 {void *pData; //wska�nik do obiektu
  int iData; //albo numer obiektu (tekstury)
 };
 //typedef
};

class Names
{
 int iSize; //rozmiar bufora
 char *cBuffer; //bufor dla rekord�w (na pocz�tku) i nazw (na ko�cu)
 ItemRecord *rRecords; //rekordy na pocz�tku bufora
 char *cLast; //ostatni u�yty bajt na nazwy
 ItemRecord *rTypes[20]; //ro�ne typy obiekt�w (pocz�tek drzewa)
 int iLast; //ostatnio u�yty rekord
public:
 __fastcall Names();
 int __fastcall Add(int t,const char *n); //dodanie obiektu typu (t)
 int __fastcall Add(int t,const char *n,void *d); //dodanie obiektu z wska�nikiem
 int __fastcall Add(int t,const char *n,int d); //dodanie obiektu z numerem
 ItemRecord* __fastcall Sort(int t); //przebudowa drzewa typu (t)
 ItemRecord* __fastcall Item(int n); //rekord o numerze (n)
};
#endif

