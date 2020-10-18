/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <unordered_map>
#include <string>

template <typename Type_>
class basic_table {

public:
// destructor
    ~basic_table() {
        for( auto *item : m_items ) {
			if (item)
				delete item; } }
// methods
    // adds provided item to the collection. returns: true if there's no duplicate with the same name, false otherwise
    bool
        insert( Type_ *Item, std::string itemname ) {
            m_items.emplace_back( Item );
            if( ( true == itemname.empty() ) || ( itemname == "none" ) ) {
                return true;
            }
            auto const itemhandle { m_items.size() - 1 };
            // add item name to the map
            auto mapping = m_itemmap.emplace( itemname, itemhandle );
            if( true == mapping.second ) {
                return true;
            }
            // item with this name already exists; update mapping to point to the new one, for backward compatibility
            mapping.first->second = itemhandle;
            return false; }
	bool insert (Type_ *Item)
	{
		return insert(Item, Item->name());
	}
	void purge (std::string const &Name)
	{
		auto lookup = m_itemmap.find( Name );
		if (lookup == m_itemmap.end())
			return;
		delete m_items[lookup->second];

		detach(Name);
	}
	void detach (std::string const &Name)
	{
		auto lookup = m_itemmap.find( Name );
		if (lookup == m_itemmap.end())
			return;

		m_items[lookup->second] = nullptr;
		// TBD, TODO: remove from m_items?

		m_itemmap.erase(lookup);
	}
	uint32_t find_id( std::string const &Name) const {
		auto lookup = m_itemmap.find( Name );
		return (
		    lookup != m_itemmap.end() ?
		        lookup->second :
		        -1 );
	}
	void purge (Type_ *Item)
	{
		for (auto it = m_items.begin(); it != m_items.end(); it++) {
			if (*it == Item) {
				delete *it;
				*it = nullptr;
				return;
			}
		}
	}
    // locates item with specified name. returns pointer to the item, or nullptr
    Type_ *
        find( std::string const &Name ) const {
            auto lookup = m_itemmap.find( Name );
            return (
                lookup != m_itemmap.end() ?
                    m_items[ lookup->second ] :
                    nullptr ); }

protected:
// types
    using type_sequence = std::deque<Type_ *>;
    using index_map = std::unordered_map<std::string, std::size_t>;
// members
    type_sequence m_items;
    index_map m_itemmap;

public:
    // data access
    type_sequence &
        sequence() {
            return m_items; }
    type_sequence const &
        sequence() const {
            return m_items; }

};
