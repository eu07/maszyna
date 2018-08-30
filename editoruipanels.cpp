/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "editoruipanels.h"

#include "globals.h"
#include "camera.h"
#include "animmodel.h"
#include "track.h"
#include "event.h"
#include "renderer.h"
#include "utilities.h"

void
itemproperties_panel::update( scene::basic_node const *Node ) {

    if( false == is_open ) { return; }

    text_lines.clear();
    std::string textline;

    // scenario inspector
    auto const *node { Node };
    auto const &camera { Global.pCamera };

    if( node == nullptr ) {
        auto const mouseposition { camera.Pos + GfxRenderer.Mouse_Position() };
        textline = "mouse location: [" + to_string( mouseposition.x, 2 ) + ", " + to_string( mouseposition.y, 2 ) + ", " + to_string( mouseposition.z, 2 ) + "]";
        text_lines.emplace_back( textline, Global.UITextColor );
        return;
    }

    textline =
        "node name: " + node->name()
        + "\nlocation: [" + to_string( node->location().x, 2 ) + ", " + to_string( node->location().y, 2 ) + ", " + to_string( node->location().z, 2 ) + "]"
        + " (distance: " + to_string( glm::length( glm::dvec3{ node->location().x, 0.0, node->location().z } -glm::dvec3{ camera.Pos.x, 0.0, camera.Pos.z } ), 1 ) + " m)";
    text_lines.emplace_back( textline, Global.UITextColor );

    // subclass-specific data
    // TBD, TODO: specialized data dump method in each node subclass, or data imports in the panel for provided subclass pointer?
    if( typeid( *node ) == typeid( TAnimModel ) ) {

        auto const *subnode = static_cast<TAnimModel const *>( node );

        textline = "angle: " + to_string( clamp_circular( subnode->vAngle.y, 360.f ), 2 ) + " deg";
        textline += "; lights: ";
        if( subnode->iNumLights > 0 ) {
            textline += '[';
            for( int lightidx = 0; lightidx < subnode->iNumLights; ++lightidx ) {
                textline += to_string( subnode->lsLights[ lightidx ] );
                if( lightidx < subnode->iNumLights - 1 ) {
                    textline += ", ";
                }
            }
            textline += ']';
        }
        else {
            textline += "none";
        }
        text_lines.emplace_back( textline, Global.UITextColor );

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
        text_lines.emplace_back( "mesh: " + modelfile, Global.UITextColor );
        text_lines.emplace_back( "skin: " + texturefile, Global.UITextColor );
    }
    else if( typeid( *node ) == typeid( TTrack ) ) {

        auto const *subnode = static_cast<TTrack const *>( node );
        // basic attributes
        textline =
            "isolated: " + ( ( subnode->pIsolated != nullptr ) ? subnode->pIsolated->asName : "none" )
            + "\nvelocity: " + to_string( subnode->SwitchExtension ? subnode->SwitchExtension->fVelocity : subnode->fVelocity )
            + "\nwidth: " + to_string( subnode->fTrackWidth ) + " m"
            + "\nfriction: " + to_string( subnode->fFriction, 2 )
            + "\nquality: " + to_string( subnode->iQualityFlag );
        text_lines.emplace_back( textline, Global.UITextColor );
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
        textline = "skins:\n " + texturefile + "\n " + texturefile2;
        text_lines.emplace_back( textline, Global.UITextColor );
        // paths
        textline = "paths: ";
        for( auto const &path : subnode->m_paths ) {
            textline +=
                "\n ["
                + to_string( path.points[ segment_data::point::start ].x, 3 ) + ", "
                + to_string( path.points[ segment_data::point::start ].y, 3 ) + ", "
                + to_string( path.points[ segment_data::point::start ].z, 3 ) + "]->"
                + "["
                + to_string( path.points[ segment_data::point::end ].x, 3 ) + ", "
                + to_string( path.points[ segment_data::point::end ].y, 3 ) + ", "
                + to_string( path.points[ segment_data::point::end ].z, 3 ) + "] ";
        }
        text_lines.emplace_back( textline, Global.UITextColor );
        // events
        textline.clear();

        std::vector< std::pair< std::string, TTrack::event_sequence const * > > const eventsequences {
            { "ev0", &subnode->m_events0 }, { "ev0all", &subnode->m_events0all },
            { "ev1", &subnode->m_events1 }, { "ev1all", &subnode->m_events1all },
            { "ev2", &subnode->m_events2 }, { "ev2all", &subnode->m_events2all } };

        for( auto const &eventsequence : eventsequences ) {

            if( eventsequence.second->empty() ) { continue; }

            textline += ( textline.empty() ? "" : "\n" ) + eventsequence.first + ": [";
            for( auto const &event : *( eventsequence.second ) ) {
                if( textline.back() != '[' ) {
                    textline += ", ";
                }
                textline += (
                    event.second ?
                        event.second->asName :
                        event.first + " (missing)" );
            }
            textline += "] ";
        }
        text_lines.emplace_back( textline, Global.UITextColor );
    }
    else if( typeid( *node ) == typeid( TMemCell ) ) {

        auto const *subnode = static_cast<TMemCell const *>( node );

        textline =
            "data: [" + subnode->Text() + "]"
            + " [" + to_string( subnode->Value1(), 2 ) + "]"
            + " [" + to_string( subnode->Value2(), 2 ) + "]";
        text_lines.emplace_back( textline, Global.UITextColor );
        textline = "track: " + ( subnode->asTrackName.empty() ? "none" : subnode->asTrackName );
        text_lines.emplace_back( textline, Global.UITextColor );
    }
}

void
itemproperties_panel::render() {

    if( false == is_open ) { return; }
    if( true == text_lines.empty() ) { return; }

    auto flags =
        ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoCollapse
        | ( size.x > 0 ? ImGuiWindowFlags_NoResize : 0 );

    if( size.x > 0 ) {
        ImGui::SetNextWindowSize( ImVec2( size.x, size.y ) );
    }
    if( size_min.x > 0 ) {
        ImGui::SetNextWindowSizeConstraints( ImVec2( size_min.x, size_min.y ), ImVec2( size_max.x, size_max.y ) );
    }
    if( true == ImGui::Begin( name.c_str(), nullptr, flags ) ) {
        // header section
        for( auto const &line : text_lines ) {
            ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
        }
    }
    ImGui::End();
}
