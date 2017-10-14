/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <unordered_map>
#include <string>

#ifdef EU07_USE_OLD_GROUNDCODE
template <typename Type_>
class TNames {

public:
// types:

// constructors:
    TNames() = default;

// destructor:

// methods:
    // dodanie obiektu z wskaźnikiem. updates data field if the object already exists. returns true for insertion, false for update
    bool
        Add( int const Type, std::string const &Name, Type_ Data ) {

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
    Type_
        Find( int const Type, std::string const &Name ) {

            auto const &map = find_map( Type );
            auto const lookup = map.find( Name );
            if( lookup != map.end() ) { return lookup->second; }
            else                      { return nullptr; }
    }

private:
// types:
    typedef std::unordered_map<std::string, Type_>              type_map;
    typedef std::unordered_map<int, type_map>                   typemap_map;

// methods:
    // returns database stored with specified type key; creates new database if needed.
    type_map &
        find_map( int const Type ) {
    
            return m_maps.emplace( Type, type_map() ).first->second;
    }

// members:
    typemap_map                          m_maps;             // list of object maps of types specified so far
};
#endif

template <typename Type_>
class basic_table {

public:
// destructor
    ~basic_table() {
        for( auto *item : m_items ) {
            delete item; } }
// methods
    // adds provided item to the collection. returns: true if there's no duplicate with the same name, false otherwise
    bool
        insert( Type_ *Item ) {
            m_items.emplace_back( Item );
            auto const itemname = Item->name();
            if( ( true == itemname.empty() ) || ( itemname == "none" ) ) {
                return true;
            }
            auto const itemhandle { m_items.size() - 1 };
            // add item name to the map
            auto mapping = m_itemmap.emplace( itemname, itemhandle );
            if( true == mapping.second ) {
                return true;
            }
            // cell with this name already exists; update mapping to point to the new one, for backward compatibility
            mapping.first->second = itemhandle;
            return false; }
    // locates item with specified name. returns pointer to the item, or nullptr
    Type_ *
        find( std::string const &Name ) {
            auto lookup = m_itemmap.find( Name );
            return (
                lookup != m_itemmap.end() ?
                    m_items[ lookup->second ] :
                    nullptr ); }

protected:
// types
    using type_sequence = std::deque<Type_ *>;
    using type_map = std::unordered_map<std::string, std::size_t>;
// members
    type_sequence m_items;
    type_map m_itemmap;

public:
    // data access
    typename type_sequence &
        sequence() {
            return m_items; }

};
