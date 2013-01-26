//---------------------------------------------------------------------------

#ifndef NamesH
#define NamesH
//---------------------------------------------------------------------------
class ItemRecord
{//rekord opisuj�cy obiekt; raz utworzony nie przemieszcza si�
 //rozmiar rekordu mo�na zmieni� w razie potrzeby
public:
 char *cName; //wska�nik do nazwy umieszczonej w buforze
 int iFlags; //flagi bitowe
 ItemRecord *rPrev,*rNext; //posortowane drzewo (przebudowywane w razie potrzeby)
 union
 {void *pData; //wska�nik do obiektu
  int iData; //albo numer obiektu (tekstury)
  unsigned int uData;
 };
 //typedef
 void __fastcall ListGet(ItemRecord *r,int*&n);
 void __fastcall TreeAdd(ItemRecord *r,int c);
 template <typename TOut> inline TOut* DataGet() {return (TOut*)pData;};
 template <typename TOut> inline void DataSet(TOut *x) {pData=(void*)x;};
 void* __fastcall TreeFind(const char *n);
 ItemRecord* __fastcall TreeFindRecord(const char *n);
};

class TNames
{
public:
 int iSize; //rozmiar bufora
 char *cBuffer; //bufor dla rekord�w (na pocz�tku) i nazw (na ko�cu)
 ItemRecord *rRecords; //rekordy na pocz�tku bufora
 char *cLast; //ostatni u�yty bajt na nazwy
 ItemRecord *rTypes[20]; //ro�ne typy obiekt�w (pocz�tek drzewa)
 int iLast; //ostatnio u�yty rekord
public:
 __fastcall TNames();
 int __fastcall Add(int t,const char *n); //dodanie obiektu typu (t)
 int __fastcall Add(int t,const char *n,void *d); //dodanie obiektu z wska�nikiem
 int __fastcall Add(int t,const char *n,int d); //dodanie obiektu z numerem
 bool __fastcall Update(int t,const char *n,void *d); //dodanie je�li nie ma, wymiana (d), gdy jest
 void __fastcall TreeSet();
 ItemRecord* __fastcall TreeSet(int *n,int d,int u);
 void __fastcall Sort(int t); //przebudowa drzewa typu (t)
 ItemRecord* __fastcall Item(int n); //rekord o numerze (n)
 inline void* Find(const int t,const char *n)
 {return rTypes[t]?rTypes[t]->TreeFind(n):NULL;};
 ItemRecord* __fastcall FindRecord(const int t,const char *n);
 //template <typename TOut> inline TOut* Find(const int t,const char *n)
 //{return (TOut*)(rTypes[t]->TreeFind(n));};
};
#endif

