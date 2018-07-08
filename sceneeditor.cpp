/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "sceneeditor.h"

#include "globals.h"
#include "simulation.h"
#include "renderer.h"

namespace scene {

basic_editor Editor;

bool
basic_editor::on_mouse_button( int const Button, int const Action ) {

    if( false == EditorModeFlag )        { return false; }
    // TBD: automatically activate and enforce picking mode and/or freefly mode when editor is active?
    if( false == FreeFlyModeFlag )       { return false; }
    if( false == Global.ControlPicking ) { return false; }

    if( Button == GLFW_MOUSE_BUTTON_LEFT ) {

        if( Action == GLFW_PRESS ) {

            m_node = GfxRenderer.Update_Pick_Node();
            m_nodesnapshot = { m_node };
            if( m_node ) {
                glfwSetInputMode( Global.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
            }
        }
        else {
            // left button release
            // TODO: record the current undo step on the undo stack
            m_nodesnapshot = { m_node };
            if( m_node ) {
                glfwSetInputMode( Global.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
            }
        }

        m_mouseleftbuttondown = ( Action == GLFW_PRESS );

        return ( m_node != nullptr );
    }

    return false;
}

bool
basic_editor::on_mouse_move( double const Mousex, double const Mousey ) {

    auto const mousemove { glm::dvec2{ Mousex, Mousey } - m_mouseposition };
    m_mouseposition = { Mousex, Mousey };

    if( false == EditorModeFlag ) { return false; }
    if( false == m_mouseleftbuttondown ) { return false; }
    if( m_node == nullptr ) { return false; }

    if( mode_translation() ) {
        // move selected node
        if( mode_translation_vertical() ) {
            auto const translation { mousemove.y * -0.01f };
            translate( translation );
        }
        else {
            auto const mouseworldposition{ Global.pCamera->Pos + GfxRenderer.Mouse_Position() };
            translate( mouseworldposition );
        }
    }
    else {
        // rotate selected node
        auto const rotation { glm::vec3 { mousemove.y, mousemove.x, 0 } * 0.25f };
        rotate( rotation );
    }

    return true;
}

void
basic_editor::translate( glm::dvec3 const &Location ) {

    auto *node { m_node }; // placeholder for operations on multiple nodes

    auto location { Location };

    if( typeid( *node ) == typeid( TAnimModel ) ) {
        // TBD, TODO: don't modify y coordinate if snap-to-ground mode is active?
//        location.y = node->location().y;
        translate_instance( static_cast<TAnimModel *>( node ), location );
    }
}

void
basic_editor::translate( float const Offset ) {

    // NOTE: offset scaling is calculated early so the same multiplier can be applied to potential whole group
    auto location { m_node->location() };
    auto const distance { glm::length( location - glm::dvec3{ Global.pCamera->Pos } ) };
    auto const offset { Offset * std::max( 1.0, distance * 0.01 ) };

    auto *node { m_node }; // placeholder for operations on multiple nodes

    if( typeid( *node ) == typeid( TAnimModel ) ) {
        translate_instance( static_cast<TAnimModel *>( node ), offset );
    }
}

void
basic_editor::translate_instance( TAnimModel *Instance, glm::dvec3 const &Location ) {

    simulation::Region->erase_instance( Instance );
    Instance->location( Location );
    simulation::Region->insert_instance( Instance, scene::scratch_data() );
}

void
basic_editor::translate_instance( TAnimModel *Instance, float const Offset ) {

    auto location { Instance->location() };
    location.y += Offset;
    Instance->location( location );
}

void
basic_editor::rotate( glm::vec3 const &Angle ) {

    auto *node { m_node }; // placeholder for operations on multiple nodes

    if( typeid( *node ) == typeid( TAnimModel ) ) {
        rotate_instance( static_cast<TAnimModel *>( node ), Angle );
    }
}

void
basic_editor::rotate_instance( TAnimModel *Instance, glm::vec3 const &Angle ) {

    // adjust node data
    glm::vec3 angle = glm::dvec3 { Instance->Angles() };
    angle.y = clamp_circular( angle.y + Angle.y, 360.f );
    Instance->Angles( angle );
    // update scene
}

bool
basic_editor::mode_translation() const {

    return ( false == Global.ctrlState );
}

bool
basic_editor::mode_translation_vertical() const {

    return ( true == Global.shiftState );
}

} // scene

//---------------------------------------------------------------------------
