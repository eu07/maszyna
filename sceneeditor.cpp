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
#include "application.h"
#include "simulation.h"
#include "camera.h"
#include "animmodel.h"
#include "renderer.h"

namespace scene {

void
basic_editor::translate( scene::basic_node *Node, glm::dvec3 const &Location, bool const Snaptoground ) {

    auto *node { Node }; // placeholder for operations on multiple nodes

    auto location { Location };
    if( false == Snaptoground ) {
        location.y = node->location().y;
    }

    if( typeid( *node ) == typeid( TAnimModel ) ) {
        translate_instance( static_cast<TAnimModel *>( node ), location );
    }
    else if( typeid( *node ) == typeid( TMemCell ) ) {
        translate_memorycell( static_cast<TMemCell *>( node ), location );
    }

}

void
basic_editor::translate( scene::basic_node *Node, float const Offset ) {

    // NOTE: offset scaling is calculated early so the same multiplier can be applied to potential whole group
    auto location { Node->location() };
    auto const distance { glm::length( location - glm::dvec3{ Global.pCamera->Pos } ) };
    auto const offset { Offset * std::max( 1.0, distance * 0.01 ) };

    auto *node { Node }; // placeholder for operations on multiple nodes

    if( typeid( *node ) == typeid( TAnimModel ) ) {
        translate_instance( static_cast<TAnimModel *>( node ), offset );
    }
    else if( typeid( *node ) == typeid( TMemCell ) ) {
        translate_memorycell( static_cast<TMemCell *>( node ), offset );
    }
}

void
basic_editor::translate_instance( TAnimModel *Instance, glm::dvec3 const &Location ) {

    simulation::Region->erase( Instance );
    Instance->location( Location );
    simulation::Region->insert( Instance );
}

void
basic_editor::translate_instance( TAnimModel *Instance, float const Offset ) {

    auto location { Instance->location() };
    location.y += Offset;
    Instance->location( location );
}

void
basic_editor::translate_memorycell( TMemCell *Memorycell, glm::dvec3 const &Location ) {

    simulation::Region->erase( Memorycell );
    Memorycell->location( Location );
    simulation::Region->insert( Memorycell );
}

void
basic_editor::translate_memorycell( TMemCell *Memorycell, float const Offset ) {

    auto location { Memorycell->location() };
    location.y += Offset;
    Memorycell->location( location );
}

void
basic_editor::rotate( scene::basic_node *Node, glm::vec3 const &Angle, float const Quantization ) {

    auto *node { Node }; // placeholder for operations on multiple nodes

    if( typeid( *node ) == typeid( TAnimModel ) ) {
        rotate_instance( static_cast<TAnimModel *>( node ), Angle, Quantization );
    }
}

void
basic_editor::rotate_instance( TAnimModel *Instance, glm::vec3 const &Angle, float const Quantization ) {

    // adjust node data
    glm::vec3 angle = glm::dvec3 { Instance->Angles() };
    angle.y = clamp_circular( angle.y + Angle.y, 360.f );
    if( Quantization > 0.f ) {
        // TBD, TODO: adjustable quantization step
        angle.y = quantize( angle.y, Quantization );
    }
    Instance->Angles( angle );
    // update scene
}

} // scene

//---------------------------------------------------------------------------
