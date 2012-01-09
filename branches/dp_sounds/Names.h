//---------------------------------------------------------------------------

#ifndef NamesH
#define NamesH
//---------------------------------------------------------------------------
class ItemRecord
{//rekord opisuj¹cy obiekt; raz utworzony nie przemieszcza siê
public:
 char* cName; //wskaŸnik do nazwy umieszczonej w buforze
 int iFlags; //flagi bitowe
 ItemRecord *rPrev,*rNext; //posortowane drzewo (przebudowywane)
 union
 {void *pData; //wskaŸnik do obiektu
  int iData; //albo numer obiektu (tekstury)
 };
 //typedef
};

class Names
{
 int iSize; //rozmiar bufora
 char *cBuffer; //bufor dla rekordów (na pocz¹tku) i nazw (na koñcu)
 ItemRecord *rRecords; //rekordy na pocz¹tku bufora
 char *cLast; //ostatni u¿yty bajt na nazwy
 ItemRecord *rTypes[20]; //ro¿ne typy obiektów (pocz¹tek drzewa)
 int iLast; //ostatnio u¿yty rekord
public:
 __fastcall Names();
 int __fastcall Add(int t,const char *n); //dodanie obiektu typu (t)
 int __fastcall Add(int t,const char *n,void *d); //dodanie obiektu z wskaŸnikiem
 int __fastcall Add(int t,const char *n,int d); //dodanie obiektu z numerem
 ItemRecord* __fastcall Sort(int t); //przebudowa drzewa typu (t)
 ItemRecord* __fastcall Item(int n); //rekord o numerze (n)
};
#endif

