#pragma once
/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include <unordered_map>
#include <string>

template <typename _Pointer>
class TNames {

public:
// types:

// constructors:
    TNames() = default;

// destructor:

// methods:
    // dodanie obiektu z wskaźnikiem. updates data field if the object already exists. returns true for insertion, false for update
    bool
        Add( int const Type, std::string const &Name, _Pointer Data ) {

            auto lookup = find_map( Type ).emplace( Name, Data );
            if( lookup.second == false ) {
                // record already exists, update it
                lookup.first->second = Data;
                return false;
            }
            else {
                // new record inserted, bail out
                return true;
            }
    }
    // returns pointer associated with provided label, or nullptr if there's no match
    _Pointer
        Find( int const Type, std::string const &Name ) {

            auto const &map = find_map( Type );
            auto const lookup = map.find( Name );
            if( lookup != map.end() ) { return lookup->second; }
            else                      { return nullptr; }
    }

private:
// types:
    typedef std::unordered_map<std::string, _Pointer>           pointer_map;
    typedef std::unordered_map<int, pointer_map>                pointermap_map;

// methods:
    // returns database stored with specified type key; creates new database if needed.
    pointer_map &
        find_map( int const Type ) {
    
            return m_maps.emplace( Type, pointer_map() ).first->second;
    }

// members:
    pointermap_map                          m_maps;             // list of pointer maps of types specified so far
};

#ifdef EU07_USE_OLD_TNAMES_CLASS
//---------------------------------------------------------------------------
class ItemRecord
{ // rekord opisujący obiekt; raz utworzony nie przemieszcza się
    // rozmiar rekordu można zmienić w razie potrzeby
  public:
    char *cName; // wskaźnik do nazwy umieszczonej w buforze
    int iFlags; // flagi bitowe
    ItemRecord *rPrev, *rNext; // posortowane drzewo (przebudowywane w razie potrzeby)
    union
    {
        void *pData; // wskaźnik do obiektu
        int iData; // albo numer obiektu (tekstury)
        unsigned int uData;
    };
    // typedef
    void ListGet(ItemRecord *r, int *&n);
    void TreeAdd(ItemRecord *r, int c);
    template <typename TOut> inline TOut *DataGet()
    {
        return (TOut *)pData;
    };
    template <typename TOut> inline void DataSet(TOut *x)
    {
        pData = (void *)x;
    };
    void * TreeFind(const char *n);
    ItemRecord * TreeFindRecord(const char *n);
};

class TNames
{
  public:
    int iSize; // rozmiar bufora
    char *cBuffer; // bufor dla rekordów (na początku) i nazw (na końcu)
    ItemRecord *rRecords; // rekordy na początku bufora
    char *cLast; // ostatni użyty bajt na nazwy
    ItemRecord *rTypes[20]; // rożne typy obiektów (początek drzewa)
    int iLast; // ostatnio użyty rekord
  public:
    TNames();
	~TNames();
    int Add(int t, const char *n); // dodanie obiektu typu (t)
    int Add(int t, const char *n, void *d); // dodanie obiektu z wskaźnikiem
    int Add(int t, const char *n, int d); // dodanie obiektu z numerem
    bool Update(int t, const char *n, void *d); // dodanie jeśli nie ma, wymiana (d), gdy jest
    void TreeSet();
    ItemRecord * TreeSet(int *n, int d, int u);
    void Sort(int t); // przebudowa drzewa typu (t)
    ItemRecord * Item(int n); // rekord o numerze (n)
    inline void *Find(const int t, const char *n)
    {
        return rTypes[t] ? rTypes[t]->TreeFind(n) : NULL;
    };
    ItemRecord * FindRecord(const int t, const char *n);
    // template <typename TOut> inline TOut* Find(const int t,const char *n)
    //{return (TOut*)(rTypes[t]->TreeFind(n));};
};
#endif
