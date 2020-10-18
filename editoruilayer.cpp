/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "editoruilayer.h"

#include "Globals.h"
#include "scenenode.h"
#include "renderer.h"

editor_ui::editor_ui() {

    clear_panels();
    // bind the panels with ui object. maybe not the best place for this but, eh

    add_external_panel( &m_itempropertiespanel );
    add_external_panel( &m_nodebankpanel );
}

// updates state of UI elements
void
editor_ui::update() {

    set_tooltip( "" );

    if( ( true == Global.ControlPicking )
     && ( true == DebugModeFlag ) ) {

        auto const scenerynode = GfxRenderer->Pick_Node();
        set_tooltip(
            ( scenerynode ?
                scenerynode->name() :
                "" ) );
    }

    ui_layer::update();
    m_itempropertiespanel.update( m_node );
}

void
editor_ui::set_node( scene::basic_node * Node ) {

    m_node = Node;
}

void
editor_ui::add_node_template(const std::string &desc) {
	m_nodebankpanel.add_template(desc);
}

std::string const *
editor_ui::get_active_node_template() {
	return m_nodebankpanel.get_active_template();
}

nodebank_panel::edit_mode
editor_ui::mode() {
	return m_nodebankpanel.mode;
}
