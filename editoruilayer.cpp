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

#include "globals.h"
#include "camera.h"
#include "animmodel.h"
#include "track.h"
#include "event.h"
#include "renderer.h"
#include "utilities.h"
#include "logs.h"

editor_ui::editor_ui() {

    clear_texts();
/*
    UIHeader = std::make_shared<ui_panel>( 20, 20 ); // header ui panel
*/
    // make 4 empty lines for the ui header, to cut down on work down the road
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    // bind the panels with ui object. maybe not the best place for this but, eh
    push_back( &UIHeader );
}

// potentially processes provided input key. returns: true if key was processed, false otherwise
bool
editor_ui::on_key( int const Key, int const Action ) {

    return false;
}

// updates state of UI elements
void
editor_ui::update() {

    std::string uitextline1, uitextline2, uitextline3, uitextline4;
    set_tooltip( "" );

    auto const &camera { Global.pCamera };

    if( ( true == Global.ControlPicking )
     && ( true == DebugModeFlag ) ) {

        auto const scenerynode = GfxRenderer.Pick_Node();
        set_tooltip(
            ( scenerynode ?
                scenerynode->name() :
                "" ) );
    }

    // scenario inspector
    auto const *node { m_node };

    if( node == nullptr ) {
        auto const mouseposition { camera.Pos + GfxRenderer.Mouse_Position() };
        uitextline1 = "mouse location: [" + to_string( mouseposition.x, 2 ) + ", " + to_string( mouseposition.y, 2 ) + ", " + to_string( mouseposition.z, 2 ) + "]";
        goto update;
    }

    uitextline1 =
        "node name: " + node->name()
        + "; location: [" + to_string( node->location().x, 2 ) + ", " + to_string( node->location().y, 2 ) + ", " + to_string( node->location().z, 2 ) + "]"
        + " (distance: " + to_string( glm::length( glm::dvec3{ node->location().x, 0.0, node->location().z } -glm::dvec3{ camera.Pos.x, 0.0, camera.Pos.z } ), 1 ) + " m)";
    // subclass-specific data
    // TBD, TODO: specialized data dump method in each node subclass, or data imports in the panel for provided subclass pointer?
    if( typeid( *node ) == typeid( TAnimModel ) ) {

        auto const *subnode = static_cast<TAnimModel const *>( node );

        uitextline2 = "angle: " + to_string( clamp_circular( subnode->vAngle.y, 360.f ), 2 ) + " deg";
        uitextline2 += "; lights: ";
        if( subnode->iNumLights > 0 ) {
            uitextline2 += '[';
            for( int lightidx = 0; lightidx < subnode->iNumLights; ++lightidx ) {
                uitextline2 += to_string( subnode->lsLights[ lightidx ] );
                if( lightidx < subnode->iNumLights - 1 ) {
                    uitextline2 += ", ";
                }
            }
            uitextline2 += ']';
        }
        else {
            uitextline2 += "none";
        }
            // 3d shape
        auto modelfile { (
            ( subnode->pModel != nullptr ) ?
                subnode->pModel->NameGet() :
                "none" ) };
        if( modelfile.find( szModelPath ) == 0 ) {
            // don't include 'models/' in the path
            modelfile.erase( 0, std::string{ szModelPath }.size() );
        }
        // texture
        auto texturefile { (
            ( subnode->Material()->replacable_skins[ 1 ] != null_handle ) ?
                GfxRenderer.Material( subnode->Material()->replacable_skins[ 1 ] ).name :
                "none" ) };
        if( texturefile.find( szTexturePath ) == 0 ) {
            // don't include 'textures/' in the path
            texturefile.erase( 0, std::string{ szTexturePath }.size() );
        }
        uitextline3 = "mesh: " + modelfile;
        uitextline4 = "skin: " + texturefile;
    }
    else if( typeid( *node ) == typeid( TTrack ) ) {

        auto const *subnode = static_cast<TTrack const *>( node );
        // basic attributes
        uitextline2 =
            "isolated: " + ( ( subnode->pIsolated != nullptr ) ? subnode->pIsolated->asName : "none" )
            + "; velocity: " + to_string( subnode->SwitchExtension ? subnode->SwitchExtension->fVelocity : subnode->fVelocity )
            + "; width: " + to_string( subnode->fTrackWidth ) + " m"
            + "; friction: " + to_string( subnode->fFriction, 2 )
            + "; quality: " + to_string( subnode->iQualityFlag );
        // textures
        auto texturefile { (
            ( subnode->m_material1 != null_handle ) ?
                GfxRenderer.Material( subnode->m_material1 ).name :
                "none" ) };
        if( texturefile.find( szTexturePath ) == 0 ) {
            texturefile.erase( 0, std::string{ szTexturePath }.size() );
        }
        auto texturefile2{ (
            ( subnode->m_material2 != null_handle ) ?
                GfxRenderer.Material( subnode->m_material2 ).name :
                "none" ) };
        if( texturefile2.find( szTexturePath ) == 0 ) {
            texturefile2.erase( 0, std::string{ szTexturePath }.size() );
        }
        uitextline2 += "; skins: [" + texturefile + ", " + texturefile2 + "]";
        // paths
        uitextline3 = "paths: ";
        for( auto const &path : subnode->m_paths ) {
            uitextline3 +=
                "["
                + to_string( path.points[ segment_data::point::start ].x, 3 ) + ", "
                + to_string( path.points[ segment_data::point::start ].y, 3 ) + ", "
                + to_string( path.points[ segment_data::point::start ].z, 3 ) + "]->"
                + "["
                + to_string( path.points[ segment_data::point::end ].x, 3 ) + ", "
                + to_string( path.points[ segment_data::point::end ].y, 3 ) + ", "
                + to_string( path.points[ segment_data::point::end ].z, 3 ) + "] ";
        }
        // events
        std::vector< std::pair< std::string, TTrack::event_sequence const * > > const eventsequences {
            { "ev0", &subnode->m_events0 }, { "ev0all", &subnode->m_events0all },
            { "ev1", &subnode->m_events1 }, { "ev1all", &subnode->m_events1all },
            { "ev2", &subnode->m_events2 }, { "ev2all", &subnode->m_events2all } };

        for( auto const &eventsequence : eventsequences ) {

            if( eventsequence.second->empty() ) { continue; }

            uitextline4 += eventsequence.first + ": [";
            for( auto const &event : *( eventsequence.second ) ) {
                if( uitextline4.back() != '[' ) {
                    uitextline4 += ", ";
                }
                if( event.second ) {
                    uitextline4 += event.second->asName;
                }
            }
            uitextline4 += "] ";
        }

    }
    else if( typeid( *node ) == typeid( TMemCell ) ) {

        auto const *subnode = static_cast<TMemCell const *>( node );

        uitextline2 =
            "data: [" + subnode->Text() + "]"
            + " [" + to_string( subnode->Value1(), 2 ) + "]"
            + " [" + to_string( subnode->Value2(), 2 ) + "]";
        uitextline3 = "track: " + ( subnode->asTrackName.empty() ? "none" : subnode->asTrackName );
    }

update:
    // update the ui header texts
    auto &headerdata = UIHeader.text_lines;
    headerdata[ 0 ].data = uitextline1;
    headerdata[ 1 ].data = uitextline2;
    headerdata[ 2 ].data = uitextline3;
    headerdata[ 3 ].data = uitextline4;
}

void
editor_ui::set_node( scene::basic_node * Node ) {

    m_node = Node;
}
