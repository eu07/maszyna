/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "stdafx.h"

#include <string>
#include <fstream>
#include <ctime>
#include <vector>
#include <sstream>
#include "dumb3d.h"

/*rozne takie duperele do operacji na stringach w paszczalu, pewnie w delfi sa lepsze*/
/*konwersja zmiennych na stringi, funkcje matematyczne, logiczne, lancuchowe, I/O etc*/

#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define DegToRad(a) ((M_PI / 180.0) * (a)) //(a) w nawiasie, bo może być dodawaniem
#define RadToDeg(r) ((180.0 / M_PI) * (r))

#define szSceneryPath "scenery/"
#define szTexturePath "textures/"
#define szModelPath "models/"
#define szDynamicPath "dynamic/"
#define szSoundPath "sounds/"
#define szDataPath "data/"

#define MAKE_ID4(a,b,c,d) (((std::uint32_t)(d)<<24)|((std::uint32_t)(c)<<16)|((std::uint32_t)(b)<<8)|(std::uint32_t)(a))

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

double CompareTime( double t1h, double t1m, double t2h, double t2m );

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

std::string to_string(int Value);
std::string to_string(unsigned int Value);
std::string to_string(int Value, int precision);
std::string to_string(int Value, int precision, int width);
std::string to_string(double Value);
std::string to_string(double Value, int precision);
std::string to_string(double Value, int precision, int width);
std::string to_hex_str( int const Value, int const width = 4 );

inline std::string to_string(bool Value) {

	return ( Value == true ? "true" : "false" );
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
// TODO: unify with win1250_to_ascii()
std::string Bezogonkow( std::string Input, bool const Underscorestospaces = false );

inline
std::string
extract_value( std::string const &Key, std::string const &Input ) {
    // NOTE, HACK: the leading space allows to uniformly look for " variable=" substring
    std::string const input { " " + Input };
    std::string value;
    auto lookup = input.find( " " + Key + "=" );
    if( lookup != std::string::npos ) {
        value = input.substr( input.find_first_not_of( ' ', lookup + Key.size() + 2 ) );
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

std::pair<std::string, std::string> FileExists( std::vector<std::string> const &Names, std::vector<std::string> const &Extensions );

// returns time of last modification for specified file
std::time_t last_modified( std::string const &Filename );

// potentially erases file extension from provided file name. returns: true if extension was removed, false otherwise
bool
erase_extension( std::string &Filename );

// potentially erase leading slashes from provided file path
void
erase_leading_slashes( std::string &Filename );

// potentially replaces backward slashes in provided file path with unix-compatible forward slashes
void
replace_slashes( std::string &Filename );

// returns potential path part from provided file name
std::string substr_path( std::string const &Filename );

// returns common prefix of two provided strings
std::ptrdiff_t len_common_prefix( std::string const &Left, std::string const &Right );

template <typename Type_>
void SafeDelete( Type_ &Pointer ) {
    delete Pointer;
    Pointer = nullptr;
}

template <typename Type_>
void SafeDeleteArray( Type_ &Pointer ) {
    delete[] Pointer;
    Pointer = nullptr;
}

template <typename Type_>
Type_
clamp( Type_ const Value, Type_ const Min, Type_ const Max ) {

    Type_ value = Value;
    if( value < Min ) { value = Min; }
    if( value > Max ) { value = Max; }
    return value;
}

// keeps the provided value in specified range 0-Range, as if the range was circular buffer
template <typename Type_>
Type_
clamp_circular( Type_ Value, Type_ const Range = static_cast<Type_>(360) ) {

    Value -= Range * (int)( Value / Range ); // clamp the range to 0-360
    if( Value < Type_(0) ) Value += Range;

    return Value;
}

template <typename Type_>
Type_
quantize( Type_ const Value, Type_ const Step ) {

    return ( Step * std::round( Value / Step ) );
}

template <typename Type_>
Type_
min_speed( Type_ const Left, Type_ const Right ) {

    if( Left == Right ) { return Left; }

    return std::min(
        ( Left != -1 ?
            Left :
            std::numeric_limits<Type_>::max() ),
        ( Right != -1 ?
            Right :
            std::numeric_limits<Type_>::max() ) );
}

template <typename Type_>
Type_
interpolate( Type_ const &First, Type_ const &Second, float const Factor ) {

    return static_cast<Type_>( ( First * ( 1.0f - Factor ) ) + ( Second * Factor ) );
}

template <typename Type_>
Type_
interpolate( Type_ const &First, Type_ const &Second, double const Factor ) {

    return static_cast<Type_>( ( First * ( 1.0 - Factor ) ) + ( Second * Factor ) );
}

// tests whether provided points form a degenerate triangle
template <typename VecType_>
bool
degenerate( VecType_ const &Vertex1, VecType_ const &Vertex2, VecType_ const &Vertex3 ) {

    //  degenerate( A, B, C, minarea ) = ( ( B - A ).cross( C - A ) ).lengthSquared() < ( 4.0f * minarea * minarea );
    return ( glm::length2( glm::cross( Vertex2 - Vertex1, Vertex3 - Vertex1 ) ) == 0.0 );
}

// calculates bounding box for provided set of points
template <class Iterator_, class VecType_>
void
bounding_box( VecType_ &Mincorner, VecType_ &Maxcorner, Iterator_ First, Iterator_ Last ) {

    Mincorner = VecType_( std::numeric_limits<typename VecType_::value_type>::max() );
    Maxcorner = VecType_( std::numeric_limits<typename VecType_::value_type>::lowest() );

    std::for_each(
        First, Last,
        [&]( typename Iterator_::value_type &point ) {
            Mincorner = glm::min( Mincorner, VecType_{ point } );
            Maxcorner = glm::max( Maxcorner, VecType_{ point } ); } );
}

// finds point on specified segment closest to specified point in 3d space. returns: point on segment as value in range 0-1 where 0 = start and 1 = end of the segment
template <typename VecType_>
typename VecType_::value_type
nearest_segment_point( VecType_ const &Segmentstart, VecType_ const &Segmentend, VecType_ const &Point ) {

    auto const v = Segmentend - Segmentstart;
    auto const w = Point - Segmentstart;

    auto const c1 = glm::dot( w, v );
    if( c1 <= 0.0 ) {
        return 0.0;
    }
    auto const c2 = glm::dot( v, v );
    if( c2 <= c1 ) {
        return 1.0;
    }
    return c1 / c2;
}

glm::dvec3 LoadPoint( class cParser &Input );

// extracts a group of tokens from provided data stream
std::string
deserialize_random_set( cParser &Input, char const *Break = "\n\r\t ;" );

int count_trailing_zeros(uint32_t val);

namespace threading {

// simple POD pairing of a data item and a mutex
// NOTE: doesn't do any locking itself, it's merely for cleaner argument arrangement and passing
template <typename Type_>
struct lockable {

    Type_ data;
    std::mutex mutex;
};

// basic wrapper simplifying use of std::condition_variable for most typical cases.
// has its own mutex and secondary variable to ignore spurious wakeups
class condition_variable {

public:
// methods
    void
        wait() {
            std::unique_lock<std::mutex> lock( m_mutex );
            m_condition.wait(
                lock,
                [ this ]() {
                    return m_spurious == false; } ); }
    template< class Rep_, class Period_ >
    void
        wait_for( const std::chrono::duration<Rep_, Period_> &Time ) {
            std::unique_lock<std::mutex> lock( m_mutex );
            m_condition.wait_for(
                lock,
                Time,
                [ this ]() {
                    return m_spurious == false; } ); }
    void
        notify_one() {
            spurious( false );
            m_condition.notify_one();
    }
    void
        notify_all() {
            spurious( false );
            m_condition.notify_all();
    }
    void
        spurious( bool const Spurious ) {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_spurious = Spurious; }

private:
// members
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_spurious { true };
};

} // threading

//---------------------------------------------------------------------------
