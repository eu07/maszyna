/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "ladderlogic.h"

#include "parser.h"
#include "utilities.h"
#include "Logs.h"

namespace plc {

int basic_element::blank = -1;

auto
basic_element::input() -> int & {

    switch( (basic_element::type_e)data.index() ) {
        case basic_element::type_e::variable: {
            return std::get<variable>(data).value;
        }
        case basic_element::type_e::timer: {
            return std::get<timer>(data).value;
        }
        case basic_element::type_e::counter: {
            return std::get<counter>(data).value;
        }
    }

    return blank; // not reachable
}

auto
basic_element::output() const -> int {

    switch( (basic_element::type_e)data.index() ) {
        case basic_element::type_e::variable: {
            return std::get<variable>(data).value;
        }
        case basic_element::type_e::timer: {
            return ( std::get<timer>(data).time_elapsed >= std::get<timer>(data).time_preset ? std::get<timer>(data).value : 0 );
        }
        case basic_element::type_e::counter: {
            return ( std::get<counter>(data).count_value >= std::get<counter>(data).count_limit ? 1 : 0 );
        }
    }

    return -1; // not reachable
}

auto
basic_controller::input( element_handle const Element ) -> int & {

    return m_elements[ Element - 1 ].input();
}

auto
basic_controller::output( element_handle const Element ) const -> int {

    return m_elements[ Element - 1 ].output();
}

auto
basic_controller::load( std::string const &Filename ) -> bool {

    m_program.clear();
    m_updateaccumulator = 0.0;

    m_programfilename = Filename;
    cParser input( m_programfilename, cParser::buffer_FILE );
    bool result { false };
    while( true == deserialize_operation( input ) ) {
        result = true; // once would suffice but, eh
    }

    return result;
}

auto
basic_controller::update( double const Timestep ) -> int {

    if( false == m_timerhandles.empty() ) {
        // update timers
        m_updateaccumulator += Timestep;
        auto const updatecount = std::floor( m_updateaccumulator / m_updaterate );
        if( updatecount > 0 ) {
            auto const updateamount = static_cast<short>( updatecount * 1000 * m_updaterate );
            for( auto const timerhandle : m_timerhandles ) {
                auto &timer{ element( timerhandle ) };
                auto &timerdata{ std::get<basic_element::timer>(timer.data) };
                if( timer.input() > 0 ) {
                    timerdata.time_elapsed =
                        std::min<short>(
                            timerdata.time_preset,
                            timerdata.time_elapsed + updateamount );
                }
                else {
                    timerdata.time_elapsed = 0;
                }
            }
            m_updateaccumulator -= m_updaterate * updatecount;
        }
    }

    return run();
}

std::map<std::string, basic_controller::opcode_e> const basic_controller::m_operationcodemap = {
    { "ld", opcode_e::op_ld }, { "ldi", opcode_e::op_ldi },
    { "and", opcode_e::op_and }, { "ani", opcode_e::op_ani }, { "anb", opcode_e::op_anb },
    { "or", opcode_e::op_or }, { "ori", opcode_e::op_ori }, { "orb", opcode_e::op_orb },
    { "out", opcode_e::op_out }, { "set", opcode_e::op_set }, { "rst", opcode_e::op_rst },
    { "end", opcode_e::op_nop }
};

auto
basic_controller::deserialize_operation( cParser &Input ) -> bool {

    auto operationdata{ Input.getToken<std::string>( true, "\n\r" ) };
    if( true == operationdata.empty() ) { return false; }

    operation operation = { opcode_e::op_nop, 0, 0, 0 };

    cParser operationparser( operationdata, cParser::buffer_TEXT );
    // HACK: operation potentially contains 1-2 parameters so we try to grab the whole set
    operationparser.getTokens( 3, "\t " );
    std::string
        operationname,
        operationelement,
        operationparameter;
    operationparser
        >> operationname
        >> operationelement
        >> operationparameter;

    auto const lookup { m_operationcodemap.find( operationname ) };
    operation.code = (
        lookup != m_operationcodemap.end() ?
            lookup->second :
            opcode_e::op_nop );
    if( lookup == m_operationcodemap.end() ) {
        log_error( "contains unknown command \"" + operationname + "\"", Input.Line() - 1 );
    }

    if( operation.code == opcode_e::op_nop ) { return true; }

    if( false == operationelement.empty() ) {
        operation.element =
            find_or_insert(
                operationelement,
                guess_element_type_from_name( operationelement ) );
    }

    if( false == operationparameter.empty() ) {
        auto const parameter{ split_string_and_number( operationparameter ) };
        operation.parameter1 = static_cast<short>( parameter.second );
    }

    m_program.emplace_back( operation );

    return true;
}

auto
basic_controller::insert( std::string const Name, basic_element Element ) -> element_handle {

    m_elements.push_back( Element );
    m_elementnames.push_back( Name );

    auto const elementhandle{ static_cast<short>( m_elements.size() ) };

    // for timers make note of the element in the timer list
    if( (basic_element::type_e)Element.data.index() == basic_element::type_e::timer ) {
        m_timerhandles.push_back( elementhandle );
    }

    return elementhandle;
}

// runs one cycle of current program
auto
basic_controller::run() -> int {
    
    m_accumulator.clear();
    m_popstack = false;
    auto programline { 1 };

    for( auto const &operation : m_program ) {
        // TBD: replace switch with function table for better readability/maintenance?
        switch( operation.code ) {

            case opcode_e::op_ld: {
                if( m_popstack ) {
                    if( false == m_accumulator.empty() ) {
                        m_accumulator.pop_back();
                    }
                    m_popstack = false;
                }
                m_accumulator.emplace_back( output( operation.element ) );
                break;
            }

            case opcode_e::op_ldi: {
                if( m_popstack ) {
                    if( false == m_accumulator.empty() ) {
                        m_accumulator.pop_back();
                    }
                    m_popstack = false;
                }
                m_accumulator.emplace_back( inverse( output( operation.element ) ) );
                break;
            }

            case opcode_e::op_and: {
                if( m_accumulator.empty() ) {
                    log_error( "attempted AND with empty accumulator", programline );
                    break;
                }
                m_accumulator.back() &= output( operation.element );
                break;
            }

            case opcode_e::op_ani: {
                if( m_accumulator.empty() ) {
                    log_error( "attempted ANI with empty accumulator", programline );
                    break;
                }
                m_accumulator.back() &= inverse( output( operation.element ) );
                break;
            }

            case opcode_e::op_anb: {
                if( m_accumulator.size() < 2 ) {
                    log_error( "attempted ANB with empty stack", programline );
                    break;
                }
                auto const operand { m_accumulator.back() };
                m_accumulator.pop_back();
                m_accumulator.back() &= operand;
                break;
            }

            case opcode_e::op_or: {
                if( m_accumulator.empty() ) {
                    log_error( "attempted OR with empty accumulator", programline );
                    break;
                }
                m_accumulator.back() |= output( operation.element );
                break;
            }

            case opcode_e::op_ori : {
                if( m_accumulator.empty() ) {
                    log_error( "attempted ORI with empty accumulator", programline );
                    break;
                }
                m_accumulator.back() |= inverse( output( operation.element ) );
                break;
            }

            case opcode_e::op_orb: {
                if( m_accumulator.size() < 2 ) {
                    log_error( "attempted ORB with empty stack", programline );
                    break;
                }
                auto const operand{ m_accumulator.back() };
                m_accumulator.pop_back();
                m_accumulator.back() |= operand;
                break;
            }

            case opcode_e::op_out: {
                if( m_accumulator.empty() ) {
                    log_error( "attempted OUT with empty accumulator", programline );
                    break;
                }
                auto &target { element( operation.element ) };
                auto const initialstate { target.input() };
                target.input() = m_accumulator.back();
                // additional operations for advanced element types
                switch( (basic_element::type_e)target.data.index() ) {
                    case basic_element::type_e::timer: {
                        std::get<basic_element::timer>(target.data).time_preset = operation.parameter1;
                        break;
                    }
                    case basic_element::type_e::counter: {
                        std::get<basic_element::counter>(target.data).count_limit = operation.parameter1;
                        // increase counter value on input activation
                        if( ( initialstate == 0 ) && ( target.input() != 0 ) ) {
/*
                            // TBD: use overflow-prone version instead of safe one?
                            target.data.counter.count_value += 1;
*/
                            std::get<basic_element::counter>(target.data).count_value =
                                std::min<short>(
                                    std::get<basic_element::counter>(target.data).count_limit,
                                    std::get<basic_element::counter>(target.data).count_value + 1 );
                        }
                        break;
                    }
                }
                // accumulator was published at least once, next ld(i) operation will start a new rung
                m_popstack = true;
                break;
            }

            case opcode_e::op_set: {
                if( m_accumulator.empty() ) {
                    log_error( "attempted SET with empty accumulator", programline );
                    break;
                }
                if( m_accumulator.back() == 0 ) {
                    break;
                }
                auto &target { element( operation.element ) };
                auto const initialstate { target.input() };
                target.input() = m_accumulator.back();
                // additional operations for advanced element types
                switch( (basic_element::type_e)target.data.index() ) {
                    case basic_element::type_e::counter: {
                        // NOTE: siemens counter behavior
                        // TODO: check whether this is true for mitsubishi
                        std::get<basic_element::counter>(target.data).count_limit = std::get<basic_element::counter>(target.data).count_value;
/*
                        if( ( initialstate == 0 ) && ( target.input() != 0 ) ) {
                            target.data.counter.count_value =
                                std::min<short>(
                                    target.data.counter.count_limit,
                                    target.data.counter.count_value + 1 );
                        }
*/
                        break;
                    }
                }
                // accumulator was published at least once, next ld(i) operation will start a new rung
                m_popstack = true;
                break;
            }

            case opcode_e::op_rst: {
                if( m_accumulator.empty() ) {
                    log_error( "attempted RST with empty accumulator", programline );
                    break;
                }
                if( m_accumulator.back() == 0 ) {
                    break;
                }
                auto &target{ element( operation.element ) };
                target.input() = 0;
                // additional operations for advanced element types
                switch( (basic_element::type_e)target.data.index() ) {
                    case basic_element::type_e::counter: {
                        std::get<basic_element::counter>(target.data).count_value = 0;
                        break;
                    }
                }
                // accumulator was published at least once, next ld(i) operation will start a new rung
                m_popstack = true;
                break;
            }
        }
        ++programline;
    }

    return 0;
}

void
basic_controller::log_error( std::string const &Error, int const Line ) const {

    ErrorLog(
        "Bad plc program: \"" + m_programfilename + "\" "
        + Error
        + ( Line > 0 ?
            " (line " + to_string( Line ) + ")" :
            "" ) );
}

auto
basic_controller::guess_element_type_from_name( std::string const &Name ) const -> basic_element::type_e {

    auto const name { split_string_and_number( Name ) };

    if( ( name.first == "t" ) || ( name.first == "ton" ) || ( name.first.find( "timer." ) == 0 ) ) {
        return basic_element::type_e::timer;
    }
    if( ( name.first == "c" ) || ( name.first.find( "counter." ) == 0 ) ) {
        return basic_element::type_e::counter;
    }

    return basic_element::type_e::variable;
}

} // plc
