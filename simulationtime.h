/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>

#include "winheaders.h"

// wrapper for scenario time
class scenario_time {

public:
    scenario_time() {
        m_time.wHour = 10; m_time.wMinute = 30; }
    void
        init();
    void
        update( double const Deltatime );
    inline
    SYSTEMTIME &
        data() {
            return m_time; }
    inline
    SYSTEMTIME const &
        data() const {
            return m_time; }
    inline
    double
        second() const {
            return ( m_time.wMilliseconds * 0.001 + m_time.wSecond ); }
    inline
    int
        year_day() const {
            return m_yearday; }
    int
        julian_day() const;
    inline
    double
        zone_bias() const {
            return m_timezonebias; }

    /** Returns std::string in format: `"mm:ss"`. */
    operator std::string();
private:
    // converts provided time transition date to regular date
    void
        convert_transition_time( SYSTEMTIME &Time ) const;
    // calculates day and month from given day of year
    void
        daymonth( WORD &Day, WORD &Month, WORD const Year, WORD const Yearday );
    // calculates day of year from given date
    int
        year_day( int Day, int const Month, int const Year ) const;
    // calculates day of week for provided date
    int
        day_of_week( int const Day, int const Month, int const Year ) const;
    // calculates day of month for specified weekday of specified month of the year
    int
        day_of_month( int const Week, int const Weekday, int const Month, int const Year ) const;
    // returns number of days between specified days of week
    int
        weekdays( int const First, int const Second ) const;

    SYSTEMTIME m_time;
    double m_milliseconds{ 0.0 };
    int m_yearday;
    char m_monthdaycounts[ 2 ][ 13 ];
    double m_timezonebias{ 0.0 };
};

namespace simulation {

extern scenario_time Time;

} // simulation

//---------------------------------------------------------------------------
