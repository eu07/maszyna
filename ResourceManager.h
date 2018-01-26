/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

int const null_handle = 0;

enum class resource_state {
    none,
    loading,
    good,
    failed
};

using resource_timestamp = std::chrono::steady_clock::time_point;

// takes containers providing access to specific element through operator[]
// with elements of std::pair<resource *, resource_timestamp>
// the element should provide method release() freeing resources owned by the element
template <class Container_>
class garbage_collector {

public:
// constructor:
    garbage_collector( Container_ &Container, unsigned int const Secondstolive, std::size_t const Sweepsize, std::string const Resourcename = "resource" ) :
        m_unusedresourcetimetolive{ std::chrono::seconds{ Secondstolive } },
        m_unusedresourcesweepsize{ Sweepsize },
        m_resourcename{ Resourcename },
        m_container{ Container }
    {}

// methods:
    // performs resource sweep. returns: number of released resources
    int
        sweep() {
            m_resourcetimestamp = std::chrono::steady_clock::now();
            // garbage collection sweep is limited to a number of records per call, to reduce impact on framerate
            auto const sweeplastindex =
                std::min(
                    m_resourcesweepindex + m_unusedresourcesweepsize,
                    m_container.size() );
            auto const blanktimestamp { std::chrono::steady_clock::time_point() };
            int releasecount{ 0 };
            for( auto resourceindex = m_resourcesweepindex; resourceindex < sweeplastindex; ++resourceindex ) {
                if( ( m_container[ resourceindex ].second != blanktimestamp )
                 && ( m_resourcetimestamp - m_container[ resourceindex ].second > m_unusedresourcetimetolive ) ) {

                    m_container[ resourceindex ].first->release();
                    m_container[ resourceindex ].second = blanktimestamp;
                    ++releasecount;
                }
            }
/*
            if( releasecount ) {
                WriteLog( "Resource garbage sweep released " + std::to_string( releasecount ) + " " + ( releasecount == 1 ? m_resourcename : m_resourcename + "s" ) );
            }
*/
            m_resourcesweepindex = (
                m_resourcesweepindex + m_unusedresourcesweepsize >= m_container.size() ?
                    0 : // if the next sweep chunk is beyond actual data, so start anew
                    m_resourcesweepindex + m_unusedresourcesweepsize );
    
            return releasecount; }

    std::chrono::steady_clock::time_point
        timestamp() const {
            return m_resourcetimestamp; }

private:
// members:
    std::chrono::nanoseconds const m_unusedresourcetimetolive;
    typename Container_::size_type const m_unusedresourcesweepsize;
    std::string const m_resourcename;
    Container_ &m_container;
    typename Container_::size_type m_resourcesweepindex { 0 };
    std::chrono::steady_clock::time_point m_resourcetimestamp { std::chrono::steady_clock::now() };
};
