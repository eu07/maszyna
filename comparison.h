/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

enum class comparison_operator {
    equal, // ==
    not_equal, // !=
    lt, // <
    gt, // >
    lt_eq, // <=
    gt_eq // >=
};

enum class comparison_pass {
    all, // &&
    any, // ||
    none // !
};

template <typename Type_>
bool compare( Type_ const Left, Type_ const Right, comparison_operator const Operator ) {
    switch( Operator ) {
        case comparison_operator::equal:     { return Left == Right; }
        case comparison_operator::not_equal: { return Left != Right; }
        case comparison_operator::lt:     { return Left < Right; }
        case comparison_operator::gt:     { return Left > Right; }
        case comparison_operator::lt_eq:  { return Left <= Right; }
        case comparison_operator::gt_eq:  { return Left >= Right; }
        default:                          { return false; }
    }
}

inline
std::string
to_string( comparison_operator const Operator ) {
    switch( Operator ) {
        case comparison_operator::equal:     { return "=="; }
        case comparison_operator::not_equal: { return "!="; }
        case comparison_operator::lt:     { return "<"; }
        case comparison_operator::gt:     { return ">"; }
        case comparison_operator::lt_eq:  { return "<="; }
        case comparison_operator::gt_eq:  { return ">"; }
        default:                          { return "??"; }
    }
}

inline
comparison_pass
comparison_pass_from_string( std::string const Input ) {
         if( Input == "all" ) { return comparison_pass::all; }
    else if( Input == "any" ) { return comparison_pass::any; }
    else if( Input == "none" ) { return comparison_pass::none; }

    return comparison_pass::all; // legacy default
}

inline
comparison_operator
comparison_operator_from_string( std::string const Input ) {
         if( Input == "==" ) { return comparison_operator::equal; }
    else if( Input == "!=" ) { return comparison_operator::not_equal; }
    else if( Input == "<" )  { return comparison_operator::lt; }
    else if( Input == ">" )  { return comparison_operator::gt; }
    else if( Input == "<=" ) { return comparison_operator::lt_eq; }
    else if( Input == ">=" ) { return comparison_operator::gt_eq; }

    return comparison_operator::equal; // legacy default
}

//---------------------------------------------------------------------------

