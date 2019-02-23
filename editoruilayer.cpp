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
#include "renderer.h"

editor_ui::editor_ui() {

    clear_panels();
    // bind the panels with ui object. maybe not the best place for this but, eh
    push_back( &m_itempropertiespanel );
	push_back( &m_nodebankpanel );
}

// potentially processes provided input key. returns: true if key was processed, false otherwise
bool
editor_ui::on_key( int const Key, int const Action ) {
    if (ui_layer::on_key(Key, Action))
        return true;

    return false;
}

// updates state of UI elements
void
editor_ui::update() {

    set_tooltip( "" );

    if( ( true == Global.ControlPicking )
     && ( true == DebugModeFlag ) ) {

        auto const scenerynode = GfxRenderer.get_picked_node();
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


nodebank_panel::edit_mode
editor_ui::mode() {
	return m_nodebankpanel.mode;
}

void
editor_ui::add_node_template(const std::string &desc) {
	m_nodebankpanel.add_template(desc);
}

const std::string *editor_ui::get_active_node_template() {
	return m_nodebankpanel.get_active_template();
}

void editor_ui::render_menu_contents() {
	ui_layer::render_menu_contents();

	if (ImGui::BeginMenu(locale::strings[locale::string::ui_mode_windows].c_str()))
	{
		ImGui::MenuItem(m_nodebankpanel.title.c_str(), nullptr, &m_nodebankpanel.is_open);
		ImGui::EndMenu();
	}
}
