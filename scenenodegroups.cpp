/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "scenenodegroups.h"

#include "event.h"
#include "memcell.h"

namespace scene {

node_groups Groups;

// requests creation of a new node group. returns: handle to the group
scene::group_handle
node_groups::create() {

    m_activegroup.push( create_handle() );

    return handle();
}

// indicates creation of current group ended. returns: handle to the parent group or null_handle if group stack is empty
scene::group_handle
node_groups::close() {

    if( false == m_activegroup.empty() ) {

        auto const closinggroup { m_activegroup.top() };
        m_activegroup.pop();
        // if the completed group holds only one item and there's no chance more items will be added, disband it
        if( ( true == m_activegroup.empty() )
         || ( m_activegroup.top() != closinggroup ) ) {

            auto lookup { m_groupmap.find( closinggroup ) };
            if( ( lookup != m_groupmap.end() )
             && ( ( lookup->second.nodes.size() + lookup->second.events.size() ) <= 1 ) ) {

                erase( lookup );
            }
        }
    }
    return handle();
}

// returns current active group, or null_handle if group stack is empty
group_handle
node_groups::handle() const {

    return (
        m_activegroup.empty() ?
            null_handle :
            m_activegroup.top() );
}

// places provided node in specified group
void
node_groups::insert( scene::group_handle const Group, scene::basic_node *Node ) {

    // TBD, TODO: automatically unregister the node from its current group?
    Node->group( Group );

    if( Group == null_handle ) { return; }

    auto &nodesequence { m_groupmap[ Group ].nodes };
    if( std::find( std::begin( nodesequence ), std::end( nodesequence ), Node ) == std::end( nodesequence ) ) {
        // don't add the same node twice
        nodesequence.emplace_back( Node );
    }
}

// places provided event in specified group
void
node_groups::insert( scene::group_handle const Group, basic_event *Event ) {

    // TBD, TODO: automatically unregister the event from its current group?
    Event->group( Group );

    if( Group == null_handle ) { return; }

    auto &eventsequence { m_groupmap[ Group ].events };
    if( std::find( std::begin( eventsequence ), std::end( eventsequence ), Event ) == std::end( eventsequence ) ) {
        // don't add the same node twice
        eventsequence.emplace_back( Event );
    }
}

// sends basic content of the class in legacy (text) format to provided stream
void
node_groups::export_as_text( std::ostream &Output ) const {

    for( auto const &group : m_groupmap ) {

        Output << "group\n";
        for( auto *node : group.second.nodes ) {
            // HACK: auto-generated memory cells aren't exported, so we check for this
            // TODO: is_exportable as basic_node method
            if( ( typeid( *node ) == typeid( TMemCell ) )
             && ( false == static_cast<TMemCell *>( node )->is_exportable ) ) {
                continue;
            }
            node->export_as_text( Output );
        }
        for( auto *event : group.second.events ) {
            event->export_as_text( Output );
        }
        Output << "endgroup\n";
    }
}

// removes specified group from the group list and group information from the group's nodes
void
node_groups::erase( group_map::const_iterator Group ) {

    for( auto *node : Group->second.nodes ) {
        node->group( null_handle );
    }
    for( auto *event : Group->second.events ) {
        event->group( null_handle );
    }
    m_groupmap.erase( Group );
}

// creates handle for a new group
group_handle
node_groups::create_handle() {
    // NOTE: for simplification nested group structure are flattened
    return(
        m_activegroup.empty() ?
            m_groupmap.size() + 1 : // new group isn't created until node registration
            m_activegroup.top() );
}

} // scene

//---------------------------------------------------------------------------
