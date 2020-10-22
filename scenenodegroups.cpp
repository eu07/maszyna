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

#include "Event.h"
#include "MemCell.h"

#include "AnimModel.h"
#include "widgets/map_objects.h"

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
node_groups::close()
{
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

bool node_groups::assign_cross_switch(map::track_switch& sw, std::string &sw_name, std::string const &id, size_t idx)
{
    sw.action[idx] = simulation::Events.FindEvent(sw_name + ":" + id);
    if (!sw.action[idx])
        sw.action[idx] = simulation::Events.FindEvent(sw_name + id);

    if (!sw.action[idx])
        return false;

    multi_event *multi = dynamic_cast<multi_event*>(sw.action[idx]);

    if (!multi)
        return false;

    auto names = multi->dump_children_names();
    for (auto it = names.begin(); it != names.end(); it++) {
        *it = it->substr(sw_name.size());
        if (it->size() > 4)
            continue;

        int pos_a = it->find_last_of('a');
        int pos_b = it->find_last_of('b');
        int pos_c = it->find_last_of('c');
        int pos_d = it->find_last_of('d');

        int pos_0 = it->find_last_of('0');
        int pos_1 = it->find_last_of('1');

        int pos;

        if (pos_a > pos_b && pos_a > pos_c && pos_a > pos_d)
            pos = 0;
        else if (pos_b > pos_a && pos_b > pos_c && pos_b > pos_d)
            pos = 1;
        else if (pos_c > pos_a && pos_c > pos_b && pos_c > pos_d)
            pos = 2;
        else
            pos = 3;

        sw.preview[idx][pos] = ((pos_0 > pos_1) ? '0' : '1');
    }

    return true;
}

void
node_groups::update_map()
{
	map::Objects.entries.clear();

    std::unordered_map<std::string, std::shared_ptr<map::track_switch>> last_switch_map;

	for (auto const &pair : m_groupmap) {
		auto const &group = pair.second;

		for (basic_node *node : group.nodes) {
			std::string postfix { "_sem_mem" };

			if (typeid(*node) == typeid(TMemCell) && string_ends_with(node->name(), postfix)) {
				std::string sem_name = node->name().substr(0, node->name().length() - postfix.length());

				auto sem_info = std::make_shared<map::semaphore>();
				map::Objects.entries.push_back(sem_info);

				sem_info->location = node->location();
				sem_info->memcell = static_cast<TMemCell*>(node);
				sem_info->name = sem_name;

				for (basic_event *event : group.events) {
					if (string_starts_with(event->name(), sem_name)
					        && event->name().substr(sem_name.length()).find("sem") == std::string::npos) {
						sem_info->events.push_back(event);
					}
				}

				for (basic_node *node : group.nodes)
					if (auto *model = dynamic_cast<TAnimModel*>(node))
						if (string_starts_with(model->name(), sem_name))
							sem_info->models.push_back(model);
			}

            if (Global.map_manualswitchcontrol) {
                if (TTrack *track = dynamic_cast<TTrack*>(node)) {
                    if (track->eType != tt_Switch)
                        continue;

                    basic_event *sw_p = simulation::Events.FindEvent(track->name() + "+");
                    basic_event *sw_m = simulation::Events.FindEvent(track->name() + "-");

                    if (sw_p && sw_m) {
                        auto map_launcher = std::make_shared<map::track_switch>();
                        map::Objects.entries.push_back(map_launcher);

                        map_launcher->location = node->location();
                        map_launcher->name = node->name();
                        map_launcher->action[0] = sw_p;
                        map_launcher->action[1] = sw_m;
                        map_launcher->track[0] = track;
                        map_launcher->preview[0][0] = '0';
                        map_launcher->preview[1][0] = '1';

                        continue;
                    }

                    std::string sw_name = track->name();
                    if (sw_name.size() <= 2)
                        continue;

                    char lastc = sw_name.back();
                    sw_name.pop_back();
                    if (sw_name.back() == '_')
                        sw_name.pop_back();

                    if (!(simulation::Events.FindEvent(sw_name + ":ac") || simulation::Events.FindEvent(sw_name + "ac")))
                        continue;

                    std::shared_ptr<map::track_switch> last_switch;

                    auto it = last_switch_map.find(sw_name);
                    if (it != last_switch_map.end()) {
                        last_switch = it->second;
                    } else {
                        last_switch = std::make_shared<map::track_switch>();
                        last_switch->name = sw_name;
                        last_switch_map.insert(std::make_pair(sw_name, last_switch));
                    }

                    if (lastc < 'a' || lastc > 'd')
                        continue;

                    last_switch->track[lastc - 'a'] = track;
                    for (auto trk : last_switch->track)
                        if (!trk)
                            goto skip_e;

                    if (!assign_cross_switch(*last_switch, sw_name, "ac", 0))
                        skip_e: continue;
                    if (!assign_cross_switch(*last_switch, sw_name, "ad", 1))
                        continue;
                    if (!assign_cross_switch(*last_switch, sw_name, "bc", 2))
                        continue;
                    if (!assign_cross_switch(*last_switch, sw_name, "bd", 3))
                        continue;

                    last_switch->location = (last_switch->track[0]->location() + last_switch->track[1]->location() + last_switch->track[2]->location() + last_switch->track[3]->location()) / 4.0;
                    map::Objects.entries.push_back(last_switch);

                    last_switch_map.erase(sw_name);
                }
            } else {
                if (TEventLauncher *launcher = dynamic_cast<TEventLauncher*>(node)) {
                    if (!launcher || !launcher->Event1 || !launcher->Event2)
                        continue;

                    auto map_launcher = std::make_shared<map::launcher>();
                    map::Objects.entries.push_back(map_launcher);

                    map_launcher->location = node->location();
                    map_launcher->name = node->name();
                    map_launcher->first_event = launcher->Event1;
                    map_launcher->second_event = launcher->Event2;
                    if (map_launcher->name.empty())
                        map_launcher->name = launcher->Event1->name();

                    if (launcher->Event1->name().find_first_of("-+:") != std::string::npos)
                        map_launcher->type = map::launcher::track_switch;
                    else
                        map_launcher->type = map::launcher::level_crossing;
                }
            }
		}
	}
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
node_groups::export_as_text( std::ostream &Output, bool Dirty ) const {
    for( auto const &group : m_groupmap ) {
		bool any = false;
        for( auto *node : group.second.nodes ) {
			if (node->dirty() != Dirty)
				continue;
            // HACK: auto-generated memory cells aren't exported, so we check for this
            // TODO: is_exportable as basic_node method
            if( ( typeid( *node ) == typeid( TMemCell ) )
             && ( false == static_cast<TMemCell *>( node )->is_exportable ) ) {
                continue;
            }

			if (!any)
				Output << "group\n";
			any = true;

            node->export_as_text( Output );
        }
        for( auto *event : group.second.events ) {
			if (Dirty)
				continue;

			if (!any)
				Output << "group\n";
			any = true;

            event->export_as_text( Output );
        }
		if (any)
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
