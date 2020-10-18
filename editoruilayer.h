/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "uilayer.h"
#include "editoruipanels.h"

namespace scene {

class basic_node;

}

class editor_ui : public ui_layer {

public:
// constructors
    editor_ui();
// methods
    // updates state of UI elements
    void
        update() override;
    void
        set_node( scene::basic_node * Node );
	void
	    add_node_template(const std::string &desc);
	const std::string *
	    get_active_node_template();
	nodebank_panel::edit_mode
	    mode();

private:
// members
    itemproperties_panel m_itempropertiespanel { "Node Properties", true };
	nodebank_panel m_nodebankpanel{ "Node Bank", true };
    scene::basic_node * m_node { nullptr }; // currently bound scene node, if any
};
