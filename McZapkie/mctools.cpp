/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
MaSzyna EU07 - SPKS
Brakes.
Copyright (C) 2007-2014 Maciej Cierniak
*/
#include "stdafx.h"
#include "mctools.h"
#include "Globals.h"

/*================================================*/

bool DebugModeFlag = false;
bool FreeFlyModeFlag = false;

double Max0R(double x1, double x2)
{
    if (x1 > x2)
        return x1;
    else
        return x2;
}

double Min0R(double x1, double x2)
{
    if (x1 < x2)
        return x1;
    else
        return x2;
}

// shitty replacement for Borland timestamp function
// TODO: replace with something sensible
std::string Now() {

    std::time_t timenow = std::time( nullptr );
    std::tm tm = *std::localtime( &timenow );
    std::stringstream  converter;
    converter << std::put_time( &tm, "%c" );
    return converter.str();
}

bool TestFlag(int Flag, int Value)
{
    if ((Flag & Value) == Value)
        return true;
    else
        return false;
}

bool SetFlag(int &Flag, int Value) {

    if (Value > 0)
    {
        if ((Flag & Value) == 0)
        {
            Flag |= Value;
            return true; // true, gdy było wcześniej 0 i zostało ustawione
        }
    }
    else if (Value < 0)
    {
        Value = abs(Value);
        if ((Flag & Value) == Value)
        {
            Flag &= ~Value; // Value jest ujemne, czyli zerowanie flagi
            return true; // true, gdy było wcześniej 1 i zostało wyzerowane
        }
    }
	return false;
}

bool UnSetFlag(int &Flag, int Value)
{
	Flag &= ~Value;
	return true;
}

inline double Random(double a, double b)
{
    std::uniform_real_distribution<> dis(a, b);
    return dis(Global::random_engine);
}

bool FuzzyLogic(double Test, double Threshold, double Probability)
{
    if ((Test > Threshold) && (!DebugModeFlag))
        return
            (Random() < Probability * Threshold * 1.0 / Test) /*im wiekszy Test tym wieksza szansa*/;
    else
        return false;
}

bool FuzzyLogicAI(double Test, double Threshold, double Probability)
{
    if ((Test > Threshold))
        return
            (Random() < Probability * Threshold * 1.0 / Test) /*im wiekszy Test tym wieksza szansa*/;
    else
        return false;
}

std::string DUE(std::string s) /*Delete Before Equal sign*/
{
    //DUE = Copy(s, Pos("=", s) + 1, length(s));
	return s.substr(s.find("=") + 1, s.length());
}

std::string DWE(std::string s) /*Delete After Equal sign*/
{
    size_t ep = s.find("=");
	if (ep != std::string::npos)
		//DWE = Copy(s, 1, ep - 1);
		return s.substr(0, ep);
    else
        return s;
}

std::string ExchangeCharInString( std::string const &Source, char const &From, char const &To )
{
	std::string replacement; replacement.reserve( Source.size() );
	std::for_each(Source.cbegin(), Source.cend(), [&](char const idx) {
		if( idx != From )    { replacement += idx; }
		else {
			if( To != NULL ) { replacement += To; } }
	} );

	return replacement;
}

std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems)
{ // dzieli tekst na wektor tekstow

    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> Split(const std::string &s, char delim)
{ // dzieli tekst na wektor tekstow
    std::vector<std::string> elems;
    Split(s, delim, elems);
    return elems;
}

std::vector<std::string> Split(const std::string &s)
{ // dzieli tekst na wektor tekstow po białych znakach
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (ss >> item)
	{
		elems.push_back(item);
	}
	return elems;
}

std::string to_string(int _Val)
{
	std::ostringstream o;
	o << _Val;
	return o.str();
};

std::string to_string(unsigned int _Val)
{
	std::ostringstream o;
	o << _Val;
	return o.str();
};

std::string to_string(double _Val)
{
	std::ostringstream o;
	o << _Val;
	return o.str();
};

std::string to_string(int _Val, int precision)
{
	std::ostringstream o;
	o << std::fixed << std::setprecision(precision);
	o << _Val;
	return o.str();
};

std::string to_string(double _Val, int precision)
{
	std::ostringstream o;
	o << std::fixed << std::setprecision(precision);
	o << _Val;
	return o.str();
};

std::string to_string(int _Val, int precision, int width)
{
	std::ostringstream o;
	o.width(width);
	o << std::fixed << std::setprecision(precision);
	o << _Val;
	return o.str();
};

std::string to_string(double const Value, int const Precision, int const Width)
{
	std::ostringstream converter;
	converter << std::setw( Width ) << std::fixed << std::setprecision(Precision) << Value;
	return converter.str();
};

std::string to_hex_str( int const Value, int const Width )
{
	std::ostringstream converter;
	converter << "0x" << std::uppercase << std::setfill( '0' ) << std::setw( Width ) << std::hex << Value;
	return converter.str();
};

int stol_def(const std::string &str, const int &DefaultValue) {

    int result { DefaultValue };
    std::stringstream converter;
    converter << str;
    converter >> result;
    return result;
}

std::string ToLower(std::string const &text)
{
	std::string lowercase( text );
	std::transform(text.begin(), text.end(), lowercase.begin(), ::tolower);
	return lowercase;
}

std::string ToUpper(std::string const &text)
{
	std::string uppercase( text );
	std::transform(text.begin(), text.end(), uppercase.begin(), ::toupper);
	return uppercase;
}

// replaces polish letters with basic ascii
void
win1250_to_ascii( std::string &Input ) {

    std::unordered_map<char, char> charmap{
        { 165, 'A' }, { 198, 'C' }, { 202, 'E' }, { 163, 'L' }, { 209, 'N' }, { 211, 'O' }, { 140, 'S' }, { 143, 'Z' }, { 175, 'Z' },
        { 185, 'a' }, { 230, 'c' }, { 234, 'e' }, { 179, 'l' }, { 241, 'n' }, { 243, 'o' }, { 156, 's' }, { 159, 'z' }, { 191, 'z' }
    };
    std::unordered_map<char, char>::const_iterator lookup;
    for( auto &input : Input ) {
        if( ( lookup = charmap.find( input ) ) != charmap.end() )
            input = lookup->second;
    }
}

template <>
bool
extract_value( bool &Variable, std::string const &Key, std::string const &Input, std::string const &Default ) {

    auto value = extract_value( Key, Input );
    if( false == value.empty() ) {
        // set the specified variable to retrieved value
        Variable = ( ToLower( value ) == "yes" );
        return true; // located the variable
    }
    else {
        // set the variable to provided default value
        if( false == Default.empty() ) {
            Variable = ( ToLower( Default ) == "yes" );
        }
        return false; // couldn't locate the variable in provided input
    }
}

bool FileExists( std::string const &Filename ) {

    std::ifstream file( Filename );
    return( true == file.is_open() );
}
