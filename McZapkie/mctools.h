#pragma once

/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*rozne takie duperele do operacji na stringach w paszczalu, pewnie w delfi sa lepsze*/
/*konwersja zmiennych na stringi, funkcje matematyczne, logiczne, lancuchowe, I/O etc*/

#include <string>
#include <fstream>
#include <ctime>
#include <vector>
#include <sstream>

extern bool DebugModeFlag;
extern bool FreeFlyModeFlag;
extern bool EditorModeFlag;
extern bool DebugCameraFlag;

/*funkcje matematyczne*/
double Max0R(double x1, double x2);
double Min0R(double x1, double x2);

inline double Sign(double x)
{
    return x >= 0 ? 1.0 : -1.0;
}

inline long Round(double const f)
{
	return (long)(f + 0.5);
	//return lround(f);
}

double Random(double a, double b);

inline double Random()
{
	return Random(0.0,1.0);
}

inline double Random(double b)
{
	return Random(0.0, b);
}

inline double BorlandTime()
{
    auto timesinceepoch = std::time( nullptr );
    return timesinceepoch / (24.0 * 60 * 60);
/*
    // std alternative
    auto timesinceepoch = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>( timesinceepoch ).count() / (24.0 * 60 * 60);
*/
}

std::string Now();

/*funkcje logiczne*/
inline bool TestFlag( int const Flag, int const Value ) { return ( ( Flag & Value ) == Value ); }
bool SetFlag( int &Flag,  int const Value);
bool ClearFlag(int &Flag, int const Value);

bool FuzzyLogic(double Test, double Threshold, double Probability);
/*jesli Test>Threshold to losowanie*/
bool FuzzyLogicAI(double Test, double Threshold, double Probability);
/*to samo ale zawsze niezaleznie od DebugFlag*/

/*operacje na stringach*/
std::string DUE(std::string s);  /*Delete Until Equal sign*/
std::string DWE(std::string s);  /*Delete While Equal sign*/
std::string ExchangeCharInString( std::string const &Source, char const From, char const To ); // zamienia jeden znak na drugi
std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> Split(const std::string &s, char delim);
//std::vector<std::string> Split(const std::string &s);

std::string to_string(int _Val);
std::string to_string(unsigned int _Val);
std::string to_string(int _Val, int precision);
std::string to_string(int _Val, int precision, int width);
std::string to_string(double _Val);
std::string to_string(double _Val, int precision);
std::string to_string(double _Val, int precision, int width);
std::string to_hex_str( int const _Val, int const width = 4 );

inline std::string to_string(bool _Val) {

	return _Val == true ? "true" : "false";
}

template <typename Type_, glm::precision Precision_ = glm::defaultp>
std::string to_string( glm::tvec3<Type_, Precision_> const &Value ) {
    return to_string( Value.x, 2 ) + ", " + to_string( Value.y, 2 ) + ", " + to_string( Value.z, 2 );
}

template <typename Type_, glm::precision Precision_ = glm::defaultp>
std::string to_string( glm::tvec4<Type_, Precision_> const &Value, int const Width = 2 ) {
    return to_string( Value.x, Width ) + ", " + to_string( Value.y, Width ) + ", " + to_string( Value.z, Width ) + ", " + to_string( Value.w, Width );
}

int stol_def(const std::string & str, const int & DefaultValue);

std::string ToLower(std::string const &text);
std::string ToUpper(std::string const &text);

// replaces polish letters with basic ascii
void win1250_to_ascii( std::string &Input );

inline
std::string
extract_value( std::string const &Key, std::string const &Input ) {

    std::string value;
    auto lookup = Input.find( Key + "=" );
    if( lookup != std::string::npos ) {
        value = Input.substr( Input.find_first_not_of( ' ', lookup + Key.size() + 1 ) );
        lookup = value.find( ' ' );
        if( lookup != std::string::npos ) {
            // trim everything past the value
            value.erase( lookup );
        }
    }
    return value;
}

template <typename Type_>
bool
extract_value( Type_ &Variable, std::string const &Key, std::string const &Input, std::string const &Default ) {

    auto value = extract_value( Key, Input );
    if( false == value.empty() ) {
        // set the specified variable to retrieved value
        std::stringstream converter;
        converter << value;
        converter >> Variable;
        return true; // located the variable
    }
    else {
        // set the variable to provided default value
        if( false == Default.empty() ) {
            std::stringstream converter;
            converter << Default;
            converter >> Variable;
        }
        return false; // couldn't locate the variable in provided input
    }
}

template <>
bool
extract_value( bool &Variable, std::string const &Key, std::string const &Input, std::string const &Default );

bool FileExists( std::string const &Filename );

// returns time of last modification for specified file
time_t last_modified( std::string const &Filename );
