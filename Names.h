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

template <typename _Type>
class TNames {

public:
// types:

// constructors:
    TNames() = default;

// destructor:

// methods:
    // dodanie obiektu z wskaźnikiem. updates data field if the object already exists. returns true for insertion, false for update
    bool
        Add( int const Type, std::string const &Name, _Type Data ) {

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
    _Type
        Find( int const Type, std::string const &Name ) {

            auto const &map = find_map( Type );
            auto const lookup = map.find( Name );
            if( lookup != map.end() ) { return lookup->second; }
            else                      { return nullptr; }
    }

private:
// types:
    typedef std::unordered_map<std::string, _Type>              type_map;
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
