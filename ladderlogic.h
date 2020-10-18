/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"

namespace plc {

using element_handle = short;

// basic logic element.
class basic_element {
public:
// types
    // rtti
    enum class type_e {
        variable,
        timer,
        counter,
    };
// constructors
    template<typename ...Args_>
    basic_element( basic_element::type_e Type = basic_element::type_e::variable, Args_ ...Args );
// methods
    // data access
    auto input() -> int &;
    auto output() const -> int;
private:
// types
    // cell content variants
    struct variable {
        std::int32_t value;
    };
    struct timer {
        std::int32_t value;
        short time_preset;
        short time_elapsed;
    };
    struct counter {
        std::int32_t value;
        short count_limit;
        short count_value;
    };
// members
    std::variant<variable, timer, counter> data;

    static int blank;
// friends
    friend class basic_controller;
};

class basic_controller {

public:
// methods
    auto load( std::string const &Filename ) -> bool;
    auto update( double const Timestep ) -> int;
    // finds element with specified name, potentially creating new element of specified type initialized with provided arguments. returns: handle to the element
    template<typename ...Args_>
    auto find_or_insert( std::string const &Name, basic_element::type_e Type = basic_element::type_e::variable, Args_ ...Args ) -> element_handle;
    // data access
    auto input( element_handle const Element ) -> int &;
    auto output( element_handle const Element ) const -> int;

private:
//types
    // plc program instruction
    enum class opcode_e : short {
        op_nop,
        op_ld,
        op_ldi,
        op_and,
        op_ani,
        op_anb,
        op_or,
        op_ori,
        op_orb,
        op_out,
        op_set,
        op_rst,
    };
    struct operation {
        opcode_e code;
        short element;
        short parameter1;
        short parameter2;
    };
    // containers
    using element_sequence = std::vector<basic_element>;
    using name_sequence = std::vector<std::string>;
    using operation_sequence = std::vector<operation>;
    using handle_sequence = std::vector<element_handle>;
// methods
    auto deserialize_operation( cParser &Input ) -> bool;
    // adds provided item to the collection. returns: true if there's no duplicate with the same name, false otherwise
    auto insert( std::string const Name, basic_element Element ) -> element_handle;
    // runs one cycle of current program. returns: error code or 0 if there's no error
    auto run() -> int;
    void log_error( std::string const &Error, int const Line = -1 ) const;
    auto guess_element_type_from_name( std::string const &Name ) const->basic_element::type_e;
    inline
    auto inverse( int const Value ) const -> int {
        return ( Value == 0 ? 1 : 0 ); }
    // element access
    inline
    auto element( element_handle const Element ) const -> basic_element const {
        return m_elements[ Element - 1 ]; }
    inline
    auto element( element_handle const Element ) -> basic_element & {
        return m_elements[ Element - 1 ]; }
// members
    static std::map<std::string, basic_controller::opcode_e> const m_operationcodemap;
    element_sequence m_elements; // collection of elements accessed by the plc program
    name_sequence m_elementnames;
    handle_sequence m_timerhandles; // indices of timer elements, timer update optimization helper
    std::string m_programfilename; // cached filename of currently loaded program
    operation_sequence m_program; // current program for the plc
    std::vector<int> m_accumulator; // state accumulator for currently processed program rung
    bool m_popstack { false }; // whether ld(i) operation should pop the accumulator stack or just add onto it
    double m_updateaccumulator { 0.0 }; // 
    double m_updaterate { 0.1 };
};

template<typename ...Args_>
basic_element::basic_element( basic_element::type_e Type, Args_ ...Args )
{
    switch( Type ) {
        case type_e::variable: {
            data = variable{ Args ... };
            break;
        }
        case type_e::timer: {
            data = timer{ Args ... };
            break;
        }
        case type_e::counter: {
            data = counter{ Args ... };
            break;
        }
        default: {
            // TBD: log error if we get here?
            break;
        }
    }
}

template<typename ...Args_>
auto basic_controller::find_or_insert( std::string const &Name, basic_element::type_e Type, Args_ ...Args ) -> element_handle {
    // NOTE: because we expect all lookups to be performed only (once) during controller (code) initialization
    // we're using simple linear container for names, to allow for easy access to both elements and their names with the same handle
    auto index { 1 };
    for( auto const &name : m_elementnames ) {
        if( name == Name ) {
            return index;
        }
        ++index;
    }
    // create and insert a new element if we didn't find existing one
    return insert( Name, basic_element( Type, Args ... ) );
}

} // plc
