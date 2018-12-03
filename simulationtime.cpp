/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "simulationtime.h"

#include "Globals.h"
#include "utilities.h"

namespace simulation {

scenario_time Time;

} // simulation

void
scenario_time::init() {
    char monthdaycounts[ 2 ][ 13 ] = {
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };
    ::memcpy( m_monthdaycounts, monthdaycounts, sizeof( monthdaycounts ) );

    // potentially adjust scenario clock
    auto const requestedtime { clamp_circular<int>( m_time.wHour * 60 + m_time.wMinute + Global.ScenarioTimeOffset * 60, 24 * 60 ) };
    auto const requestedhour { ( requestedtime / 60 ) % 24 };
    auto const requestedminute { requestedtime % 60 };
    // cache requested elements, if any

#ifdef __linux__
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm *tms = localtime(&ts.tv_sec);
	m_time.wYear = tms->tm_year;
	m_time.wMonth = tms->tm_mon;
	m_time.wDayOfWeek = tms->tm_wday;
	m_time.wDay = tms->tm_mday;
	m_time.wHour = tms->tm_hour;
	m_time.wMinute = tms->tm_min;
	m_time.wSecond = tms->tm_sec;
	m_time.wMilliseconds = ts.tv_nsec / 1000000;

/*
    time_t local = mktime(localtime(&ts.tv_sec));
    time_t utc = mktime(gmtime(&ts.tv_sec));
    m_timezonebias = (double)(local - utc) / 3600.0f;
*/
#elif _WIN32
    ::GetLocalTime( &m_time );
#endif

    if( Global.fMoveLight > 0.0 ) {
        // day and month of the year can be overriden by scenario setup
        daymonth( m_time.wDay, m_time.wMonth, m_time.wYear, static_cast<WORD>( Global.fMoveLight ) );
    }

    if( requestedhour != -1 ) { m_time.wHour = static_cast<WORD>( clamp( requestedhour, 0, 23 ) ); }
    if( requestedminute != -1 ) { m_time.wMinute = static_cast<WORD>( clamp( requestedminute, 0, 59 ) ); }
    // if the time is taken from the local clock leave the seconds intact, otherwise set them to zero
    if( ( requestedhour != -1 )
     || ( requestedminute != 1 ) ) {
        m_time.wSecond = 0;
    }

    m_yearday = year_day( m_time.wDay, m_time.wMonth, m_time.wYear );

    // calculate time zone bias
    // retrieve relevant time zone info from system registry (or fall back on supplied default)
    // TODO: select timezone matching defined geographic location and/or country
    struct registry_time_zone_info {
        long Bias;
        long StandardBias;
        long DaylightBias;
        SYSTEMTIME StandardDate;
        SYSTEMTIME DaylightDate;
    } timezoneinfo = { -60, 0, -60, { 0, 10, 0, 5, 3, 0, 0, 0 }, { 0, 3, 0, 5, 2, 0, 0, 0 } };

    convert_transition_time( timezoneinfo.StandardDate );
    convert_transition_time( timezoneinfo.DaylightDate );

    auto zonebias { timezoneinfo.Bias };
    if( m_yearday < year_day( timezoneinfo.DaylightDate.wDay, timezoneinfo.DaylightDate.wMonth, m_time.wYear ) ) {
        zonebias += timezoneinfo.StandardBias;
    }
    else if( m_yearday < year_day( timezoneinfo.StandardDate.wDay, timezoneinfo.StandardDate.wMonth, m_time.wYear ) ) {
        zonebias += timezoneinfo.DaylightBias;
    }
    else {
        zonebias += timezoneinfo.StandardBias;
    }

    m_timezonebias = ( zonebias / 60.0 );
}

void
scenario_time::update( double const Deltatime ) {

    m_milliseconds += ( 1000.0 * Deltatime );
    while( m_milliseconds >= 1000.0 ) {

        ++m_time.wSecond;
        m_milliseconds -= 1000.0;
    }
    m_time.wMilliseconds = std::floor( m_milliseconds );
    while( m_time.wSecond >= 60 ) {

        ++m_time.wMinute;
        m_time.wSecond -= 60;
    }
    while( m_time.wMinute >= 60 ) {

        ++m_time.wHour;
        m_time.wMinute -= 60;
    }
    while( m_time.wHour >= 24 ) {

        ++m_time.wDay;
        ++m_time.wDayOfWeek;
        if( m_time.wDayOfWeek >= 7 ) {
            m_time.wDayOfWeek -= 7;
        }
        m_time.wHour -= 24;
    }
    int leap { ( m_time.wYear % 4 == 0 ) && ( m_time.wYear % 100 != 0 ) || ( m_time.wYear % 400 == 0 ) };
    while( m_time.wDay > m_monthdaycounts[ leap ][ m_time.wMonth ] ) {

        m_time.wDay -= m_monthdaycounts[ leap ][ m_time.wMonth ];
        ++m_time.wMonth;
        // unlikely but we might've entered a new year
        if( m_time.wMonth > 12 ) {

            ++m_time.wYear;
            leap = ( m_time.wYear % 4 == 0 ) && ( m_time.wYear % 100 != 0 ) || ( m_time.wYear % 400 == 0 );
            m_time.wMonth -= 12;
        }
    }
}

int
scenario_time::year_day( int Day, const int Month, const int Year ) const {

    char const daytab[ 2 ][ 13 ] = {
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    };

    int const leap { ( Year % 4 == 0 ) && ( Year % 100 != 0 ) || ( Year % 400 == 0 ) };
    for( int i = 1; i < Month; ++i )
        Day += daytab[ leap ][ i ];

    return Day;
}

void
scenario_time::daymonth( WORD &Day, WORD &Month, WORD const Year, WORD const Yearday ) {

    WORD daytab[ 2 ][ 13 ] = {
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };

    int const leap { ( Year % 4 == 0 ) && ( Year % 100 != 0 ) || ( Year % 400 == 0 ) };
    WORD idx = 1;
    while( ( idx < 13 ) && ( Yearday >= daytab[ leap ][ idx ] ) ) {

        ++idx;
    }
    Month = idx;
    Day = Yearday - daytab[ leap ][ idx - 1 ];
}

int
scenario_time::julian_day() const {

    int yy = ( m_time.wYear < 0 ? m_time.wYear + 1 : m_time.wYear ) - std::floor( ( 12 - m_time.wMonth ) / 10.f );
    int mm = m_time.wMonth + 9;
    if( mm >= 12 ) { mm -= 12; }

    int K1 = std::floor( 365.25 * ( yy + 4712 ) );
    int K2 = std::floor( 30.6 * mm + 0.5 );

    // for dates in Julian calendar
    int JD = K1 + K2 + m_time.wDay + 59;
    // for dates in Gregorian calendar; 2299160 is October 15th, 1582
    const int gregorianswitchday = 2299160;
    if( JD > gregorianswitchday ) {

        int K3 = std::floor( std::floor( ( yy * 0.01 ) + 49 ) * 0.75 ) - 38;
        JD -= K3;
    }

    return JD;
}

// calculates day of week for provided date
int
scenario_time::day_of_week( int const Day, int const Month, int const Year ) const {

	// using Zeller's congruence, http://en.wikipedia.org/wiki/Zeller%27s_congruence
	int const q = Day;
	int const m = Month > 2 ? Month : Month + 12;
	int const y = Month > 2 ? Year : Year - 1;

	int const h = ( q + ( 26 * ( m + 1 ) / 10 ) + y + ( y / 4 ) + 6 * ( y / 100 ) + ( y / 400 ) ) % 7;
	
/*	return ( (h + 5) % 7 ) + 1; // iso week standard, with monday = 1
*/	return ( (h + 6) % 7 ) + 1; // sunday = 1 numbering method, used in north america, japan 
}

// calculates day of month for specified weekday of specified month of the year
int
scenario_time::day_of_month( int const Week, int const Weekday, int const Month, int const Year ) const {

	int day = 0;
	int dayoffset = weekdays( day_of_week( 1, Month, Year ), Weekday );

    day = ( Week - 1 ) * 7 + 1 + dayoffset;

    if( Week == 5 ) {
        // 5th week potentially indicates last week in the month, not necessarily actual 5th
        char const daytab[ 2 ][ 13 ] = {
            { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
            { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
        };
        int const leap { ( Year % 4 == 0 ) && ( Year % 100 != 0 ) || ( Year % 400 == 0 ) };

        while( day > daytab[ leap ][ Month ] ) {
            day -= 7;
        }
    }

	return day;
}

// returns number of days between specified days of week
int
scenario_time::weekdays( int const First, int const Second ) const {

    if( Second >= First ) { return Second - First; }
    else { return 7 - First + Second; }
}

// helper, converts provided time transition date to regular date
void
scenario_time::convert_transition_time( SYSTEMTIME &Time ) const {

    // NOTE: windows uses 0-6 range for days of week numbering, our methods use 1-7
    Time.wDay = day_of_month( Time.wDay, Time.wDayOfWeek + 1, Time.wMonth, m_time.wYear );
}
