/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include"stdafx.h"
#include "Names.h"

//---------------------------------------------------------------------------

/*
Moduł zarządzający plikami oraz wyszukiwaniem obiektów wg nazw.
1. Ma przydzielony z góry (EU07.INI) obszar pamięci (rzędu 16MB).
2. W przypadku przepełnienia dostępnej pamięci wystąpi błąd wczytywania.
3. Obszar ten będzie zużywany na rekordy obiektów oraz ciągi tekstowe z nazwami.
4. Rekordy będą sortowane w ramach typu (tekstury, dźwięki, modele, node, eventy).
5. Pierwszy etap wyszukiwania to 5 bitów z pierwszego bajtu i 3 z drugiego (256).
6. Dla plików istnieje możliwość wczytania ich w innym terminie.
7. Możliwość wczytania plików w oddzielnym watku (np. tekstur).

Obsługiwane pliki:
1. Tekstury, można wczytywać później, rekord przechowuje numer podany przez kartę graficzną.
2. Dźwięki, można wczytać później.
3. Modele, można wczytać później o ile nie mają animacji eventami i nie dotyczą pojazdów.

Obiekty sortowane wg nazw, można dodawać i usuwać komórki scenerii:
4. Tory, drogi, rzeki - wyszukiwanie w celu sprawdzenia zajetości.
5. Eventy - wyszukiwane przy zewnętrznym wywołaniu oraz podczas wczytywania.
6. Pojazdy - wyszukiwane w celu wysyłania komend.
7. Egzemplarze modeli animowanych - wyszukiwanie w celu połączenia z animacjami.

*/

#ifdef EU07_USE_OLD_TNAMES_CLASS

void ItemRecord::TreeAdd(ItemRecord *r, int c)
{ // dodanie rekordu do drzewa - ustalenie w której gałęzi
    // zapisać w (iFlags) ile znaków jest zgodnych z nadrzędnym, żeby nie sprawdzać wszystkich od
    // zera
    if ((cName[c] && r->cName[c]) ? cName[c] == r->cName[c] : false)
        TreeAdd(r, c + 1); // ustawić wg kolejnego znaku, chyba że zero
    else if ((unsigned char)(cName[c]) < (unsigned char)(r->cName[c]))
    { // zero jest najmniejsze - doczepiamy jako (rNext)
        if (!rNext)
            rNext = r;
        else
            rNext->TreeAdd(r, 0); // doczepić do tej gałęzi
    }
    else
    {
        if (!rPrev)
            rPrev = r;
        else
            rPrev->TreeAdd(r, 0); // doczepić do tej gałęzi
    }
};

void ItemRecord::ListGet(ItemRecord *r, int *&n)
{ // rekurencyjne wypełnianie posortowanej listy na podstawie drzewa
    if (rPrev)
        rPrev->ListGet(r, n); // dodanie wszystkich wcześniejszych
    *n++ = this - r; // dodanie swojego indeksu do tabeli
    if (rNext)
        rNext->ListGet(r, n); // dodanie wszystkich późniejszych
};

void * ItemRecord::TreeFind(const char *n)
{ // wyszukanie ciągu (n)
    ItemRecord *r = TreeFindRecord(n);
    return r ? r->pData : NULL;
};

ItemRecord * ItemRecord::TreeFindRecord(const char *n)
{ // wyszukanie ciągu (n)
    ItemRecord *r = this; //żeby nie robić rekurencji
    int i = 0;
    do
    {
        if (!n[i])
            if (!r->cName[i])
                return r; // znaleziony
        if (n[i] == r->cName[i])
            ++i; // porównać kolejny znak
        else if ((unsigned char)(n[i]) < (unsigned char)(r->cName[i]))
        {
            i = 0; // porównywać od nowa
            r = r->rPrev; // wcześniejsza gałąź drzewa
        }
        else
        {
            i = 0; // porównywać od nowa
            r = r->rNext; // późniejsza gałąź drzewa
        }
    } while (r);
    return NULL;
};

TNames::TNames()
{ // tworzenie bufora
    iSize = 16 * 1024 * 1024; // rozmiar bufora w bajtach
    cBuffer = new char[iSize];
    ZeroMemory(cBuffer, iSize); // nie trzymać jakiś starych śmieci
    rRecords = (ItemRecord *)cBuffer;
    cLast = cBuffer + iSize; // bajt za buforem
    iLast = -1;
    ZeroMemory(rTypes, 20 * sizeof(ItemRecord *));
};

TNames::~TNames() {

	delete[] cBuffer;
}

int TNames::Add(int t, const char *n)
{ // dodanie obiektu typu (t) o nazwie (n)
    int len = strlen(n) + 1; // ze znacznikiem końca
    cLast -= len; // rezerwacja miejsca
    memcpy(cLast, n, len); // przekopiowanie tekstu do bufora
    // cLast[len-1]='\0';
    rRecords[++iLast].cName = cLast; // połączenie nazwy z rekordem
    rRecords[iLast].iFlags = t;
    if (!rTypes[t])
        rTypes[t] = rRecords + iLast; // korzeń drzewa, bo nie było wcześniej
    else
        rTypes[t]->TreeAdd(rRecords + iLast, 0); // doczepienie jako gałąź
    // rTypes[t]=Sort(t); //sortowanie uruchamiać ręcznie
    if ((iLast & 0x3F) == 0) // nie za często, bo sortowania zajmą więcej czasu niż wyszukiwania
        Sort(t); // optymalizacja drzewa co jakiś czas
    return iLast;
}
int TNames::Add(int t, const char *n, void *d)
{
    int i = Add(t, n);
    rRecords[iLast].pData = d;
    return i;
};

bool TNames::Update(int t, const char *n, void *d)
{ // dodanie jeśli nie ma, wymiana (d), gdy jest
    ItemRecord *r = FindRecord(t, n); // najpierw sprawdzić, czy już jest
    if (r)
    { // przy zdublowaniu nazwy podmieniać w drzewku na późniejszy
        r->pData = d;
        return true; // duplikat
    }
    // Add(t,n,d); //nazwa unikalna
    return false; // został dodany nowy
};

ItemRecord * TNames::TreeSet(int *n, int d, int u)
{ // rekurencyjne wypełnianie drzewa pozycjami od (d) do (u)
    if (d == u)
    {
        rRecords[n[d]].rPrev = rRecords[n[d]].rNext = NULL;
        return rRecords + n[d]; // tej gałęzi nie ma
    }
    else if (d > u)
        return NULL;
    int p = (u + d) >> 1; // połowa
    rRecords[n[p]].rPrev = TreeSet(n, d, p - 1); // zapisanie wcześniejszych gałęzi
    rRecords[n[p]].rNext = TreeSet(n, p + 1, u); // zapisanie późniejszych gałęzi
    return rRecords + n[p];
};

void TNames::Sort(int t)
{ // przebudowa drzewa typu (t), zwraca wierzchołek drzewa
    if (iLast < 3)
        return; // jak jest mało, to nie ma sensu sortować
    if (rTypes[t]) // jeśli jest jakiś rekord danego typu
    {
        int *r = new int[iLast + 1]; // robocza tablica indeksów - numery posortowanych rekordów
        int *q = r; // wskaźnik roboczy, przekazywany przez referencję
        rTypes[t]->ListGet(rRecords, q); // drzewo jest już posortowane - zamienić je na listę
        rTypes[t] = TreeSet(r, 0, (q - r) - 1);
        delete[] r;
    }
    return;
};

ItemRecord * TNames::FindRecord(const int t, const char *n)
{ // poszukiwanie rekordu w celu np. zmiany wskaźnika
    return rTypes[t] ? rTypes[t]->TreeFindRecord(n) : NULL;
};

#endif
