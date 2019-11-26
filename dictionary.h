/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

// collection of keyword-value pairs
// NOTE: since our python dictionary operates on a few types, most of the class was hardcoded for simplicity
struct dictionary_source {
// types
    template <typename Type_>
    using keyvaluepair_sequence = std::vector<std::pair<std::string, Type_>>;
// members
    keyvaluepair_sequence<double> floats;
    keyvaluepair_sequence<int> integers;
    keyvaluepair_sequence<bool> bools;
    keyvaluepair_sequence<std::string> strings;
    keyvaluepair_sequence<std::vector<glm::vec2>> vec2_lists;
// constructors
    dictionary_source() = default;
    dictionary_source( std::string const &Input );
// methods
    inline void insert( std::string const &Key, double const Value )      { floats.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, int const Value )         { integers.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, bool const Value )        { bools.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, std::string const Value ) { strings.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, std::vector<glm::vec2> const Value ) { vec2_lists.emplace_back( Key, Value ); }
};
