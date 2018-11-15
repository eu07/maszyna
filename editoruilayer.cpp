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
}

// updates state of UI elements
void
editor_ui::update() {

    set_tooltip( "" );

    if( ( true == Global.ControlPicking )
     && ( true == DebugModeFlag ) ) {

        auto const scenerynode = GfxRenderer.Pick_Node();
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
