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
#include "scenenodegroups.h"

#include "Globals.h"
#include "application.h"
#include "simulation.h"
#include "MemCell.h"
#include "Camera.h"
#include "AnimModel.h"
#include "renderer.h"

namespace scene {

void
basic_editor::translate( scene::basic_node *Node, glm::dvec3 const &Location, bool const Snaptoground ) {

	auto &initiallocation { Node->location() };

	// fixup NaNs
	if (std::isnan(initiallocation.x))
		initiallocation.x = Location.x;
	if (std::isnan(initiallocation.y))
		initiallocation.y = Location.y;
	if (std::isnan(initiallocation.z))
		initiallocation.z = Location.z;

    auto targetlocation { Location };
    if( false == Snaptoground ) {
        targetlocation.y = initiallocation.y;
    }
    // NOTE: bit of a waste for single nodes, for the sake of less varied code down the road
    auto const translation { targetlocation - initiallocation };

	Node->mark_dirty();
    if( Node->group() <= 1 ) {
        translate_node( Node, Node->location() + translation );
    }
    else {
        // translate entire group
        // TODO: contextual switch between group and item translation
        // TODO: translation of affected/relevant events
        auto &nodegroup { scene::Groups.group( Node->group() ).nodes };
        std::for_each(
            std::begin( nodegroup ), std::end( nodegroup ),
            [&]( auto *node ) {
                translate_node( node, node->location() + translation ); } );
    }
}

void
basic_editor::translate( scene::basic_node *Node, float const Offset ) {

    // NOTE: offset scaling is calculated early so the same multiplier can be applied to potential whole group
    auto location { Node->location() };
    auto const distance { glm::length( location - glm::dvec3{ Global.pCamera.Pos } ) };
    auto const offset { static_cast<float>( Offset * std::max( 1.0, distance * 0.01 ) ) };

    if( Node->group() <= 1 ) {
        translate_node( Node, offset );
    }
    else {
        // translate entire group
        // TODO: contextual switch between group and item translation
        // TODO: translation of affected/relevant events
        auto &nodegroup { scene::Groups.group( Node->group() ).nodes };
        std::for_each(
            std::begin( nodegroup ), std::end( nodegroup ),
            [&]( auto *node ) {
                translate_node( node, offset ); } );
    }
}

void
basic_editor::translate_node( scene::basic_node *Node, glm::dvec3 const &Location ) {

    if( typeid( *Node ) == typeid( TAnimModel ) ) {
        translate_instance( static_cast<TAnimModel *>( Node ), Location );
    }
    else if( typeid( *Node ) == typeid( TMemCell ) ) {
        translate_memorycell( static_cast<TMemCell *>( Node ), Location );
    }
}

void
basic_editor::translate_node( scene::basic_node *Node, float const Offset ) {

    if( typeid( *Node ) == typeid( TAnimModel ) ) {
        translate_instance( static_cast<TAnimModel *>( Node ), Offset );
    }
    else if( typeid( *Node ) == typeid( TMemCell ) ) {
        translate_memorycell( static_cast<TMemCell *>( Node ), Offset );
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

    glm::vec3 rotation { 0, Angle.y, 0 };

    // quantize resulting angle if requested and type of the node allows it
    // TBD, TODO: angle quantization for types other than instanced models
    if( ( Quantization > 0.f )
     && ( typeid( *Node ) == typeid( TAnimModel ) ) ) {

        auto const initialangle { static_cast<TAnimModel *>( Node )->Angles() };
        rotation += initialangle;

        // TBD, TODO: adjustable quantization step
        rotation.y = quantize( rotation.y, Quantization );

        rotation -= initialangle;
    }

    if( Node->group() <= 1 ) {
        rotate_node( Node, rotation );
    }
    else {
        // rotate entire group
        // TODO: contextual switch between group and item rotation
        // TODO: translation of affected/relevant events
        auto const &rotationcenter { Node->location() };
        auto const &nodegroup { scene::Groups.group( Node->group() ).nodes };
        std::for_each(
            std::begin( nodegroup ), std::end( nodegroup ),
            [&]( auto *node ) {
                rotate_node( node, rotation );
                if( node != Node ) {
                    translate_node(
                        node,
                        rotationcenter
                        + glm::rotateY(
                            node->location() - rotationcenter,
                            glm::radians<double>( rotation.y ) ) ); } } );
    }
}

void
basic_editor::rotate_node( scene::basic_node *Node, glm::vec3 const &Angle ) {

    if( typeid( *Node ) == typeid( TAnimModel ) ) {
        rotate_instance( static_cast<TAnimModel *>( Node ), Angle );
    }
}

void
basic_editor::rotate_instance( TAnimModel *Instance, glm::vec3 const &Angle ) {

    auto targetangle { Instance->Angles() + Angle };

    targetangle.x = clamp_circular( targetangle.x, 360.f );
    targetangle.y = clamp_circular( targetangle.y, 360.f );
    targetangle.z = clamp_circular( targetangle.z, 360.f );

    Instance->Angles( targetangle );
}

} // scene

//---------------------------------------------------------------------------
