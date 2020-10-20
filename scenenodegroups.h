/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "scenenode.h"
#include "widgets/map_objects.h"

namespace scene {

struct basic_group {
// members
    std::vector<scene::basic_node *> nodes;
    std::vector<basic_event *> events;
};

// holds lists of grouped scene nodes
class node_groups {
    // NOTE: during scenario deserialization encountering *.inc file causes creation of a new group on the group stack
    // this allows all nodes listed in this *.inc file to be grouped and potentially modified together by the editor.
public:
// constructors
    node_groups() = default;
// methods
    // requests creation of a new node group. returns: handle to the group
    group_handle
        create();
    // indicates creation of current group ended. returns: handle to the parent group or null_handle if group stack is empty
    group_handle
        close();
	// update minimap objects
	void
	    update_map();
    // returns current active group, or null_handle if group stack is empty
    group_handle
        handle() const;
    // places provided node in specified group
    void
        insert( scene::group_handle const Group, scene::basic_node *Node );
    // places provided event in specified group
    void
        insert( scene::group_handle const Group, basic_event *Event );
    // grants direct access to specified group
    scene::basic_group &
        group( scene::group_handle const Group ) {
            return m_groupmap[ Group ]; }
    // sends basic content of the class in legacy (text) format to provided stream
    void
        export_as_text( std::ostream &Output, bool const Dirty ) const;

private:
// types
    using group_map = std::unordered_map<scene::group_handle, scene::basic_group>;
// methods
    // removes specified group from the group list and group information from the group's nodes
    void
        erase( group_map::const_iterator Group );
    // creates handle for a new group
    group_handle
        create_handle();
    bool
        assign_cross_switch(map::track_switch&sw, std::string &sw_name, const std::string &id, size_t idx);
// members
    group_map m_groupmap; // map of established node groups
    std::stack<scene::group_handle> m_activegroup; // helper, group to be assigned to newly created nodes
};

extern node_groups Groups;

} // scene

//---------------------------------------------------------------------------
