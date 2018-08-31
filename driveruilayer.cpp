/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "driveruilayer.h"

#include "globals.h"
#include "translation.h"
#include "simulation.h"
#include "train.h"
#include "animmodel.h"
#include "renderer.h"

driver_ui::driver_ui() {

    clear_panels();
    // bind the panels with ui object. maybe not the best place for this but, eh
    push_back( &m_aidpanel );
    push_back( &m_timetablepanel );
    push_back( &m_debugpanel );
    push_back( &m_transcriptspanel );

    m_timetablepanel.size_min = { 435, 110 };
    m_timetablepanel.size_max = { 435, Global.iWindowHeight * 0.95 };

    m_transcriptspanel.size_min = { 435, 85 };
    m_transcriptspanel.size_max = { Global.iWindowWidth * 0.95, Global.iWindowHeight * 0.95 };
}

// potentially processes provided input key. returns: true if key was processed, false otherwise
bool
driver_ui::on_key( int const Key, int const Action ) {
    // TODO: pass the input first through an active ui element if there's any
    // if the ui element shows no interest or we don't have one, try to interpret the input yourself:
    // shared conditions
    switch( Key ) {

        case GLFW_KEY_F1:
        case GLFW_KEY_F2:
        case GLFW_KEY_F10:
        case GLFW_KEY_F12: { // ui mode selectors

            if( ( true == Global.ctrlState )
             || ( true == Global.shiftState ) ) {
                // only react to keys without modifiers
                return false;
            }

            if( Action != GLFW_PRESS ) { return true; } // recognized, but ignored
        }

        default: { // everything else
            break;
        }
    }

    switch (Key) {
            
        case GLFW_KEY_F1: {
            // basic consist info
            auto state = (
                ( m_aidpanel.is_open == false ) ? 0 :
                ( m_aidpanel.is_expanded == false ) ? 1 :
                2 );
            state = clamp_circular( ++state, 3 );
            
            m_aidpanel.is_open = ( state > 0 );
            m_aidpanel.is_expanded = ( state > 1 );

            return true;
        }

        case GLFW_KEY_F2: {
            // timetable
            auto state = (
                ( m_timetablepanel.is_open == false ) ? 0 :
                ( m_timetablepanel.is_expanded == false ) ? 1 :
                2 );
            state = clamp_circular( ++state, 3 );
            
            m_timetablepanel.is_open = ( state > 0 );
            m_timetablepanel.is_expanded = ( state > 1 );

            return true;
        }

        case GLFW_KEY_F12: {
            // debug panel
            m_debugpanel.is_open = !m_debugpanel.is_open;
            return true;
        }

        default: {
            break;
        }
    }

    return false;
}

// updates state of UI elements
void
driver_ui::update() {

    set_tooltip( "" );

    auto const *train { simulation::Train };

    if( ( train != nullptr ) && ( false == FreeFlyModeFlag ) ) {
        if( false == DebugModeFlag ) {
            // in regular mode show control functions, for defined controls
            set_tooltip( locale::label_cab_control( train->GetLabel( GfxRenderer.Pick_Control() ) ) );
        }
        else {
            // in debug mode show names of submodels, to help with cab setup and/or debugging
            auto const cabcontrol = GfxRenderer.Pick_Control();
            set_tooltip( ( cabcontrol ? cabcontrol->pName : "" ) );
        }
    }
    if( ( true == Global.ControlPicking ) && ( true == FreeFlyModeFlag ) && ( true == DebugModeFlag ) ) {
        auto const scenerynode = GfxRenderer.Pick_Node();
        set_tooltip(
            ( scenerynode ?
                scenerynode->name() :
                "" ) );
    }

    ui_layer::update();
}

// render() subclass details
void
driver_ui::render_() {

    auto const pausemask { 1 | 2 };
    if( ( Global.iPause & pausemask ) != 0 ) {
        // pause/quit modal
        auto const popupheader{ "Simulation Paused" };
        ImGui::OpenPopup( popupheader );
        if( ImGui::BeginPopupModal( popupheader, nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
            if( ImGui::Button( "Resume", ImVec2( 120, 0 ) ) ) {
                ImGui::CloseCurrentPopup();
                Global.iPause &= ~pausemask;
            }
            if( ImGui::Button( "Quit", ImVec2( 120, 0 ) ) ) {
                ImGui::CloseCurrentPopup();
                glfwSetWindowShouldClose( m_window, 1 );
            }
            ImGui::EndPopup();
        }
    }
}
