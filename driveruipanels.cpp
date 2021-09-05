/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "driveruipanels.h"

#include "Globals.h"
#include "application.h"
#include "translation.h"
#include "simulation.h"
#include "simulationtime.h"
#include "simulationenvironment.h"
#include "Timer.h"
#include "Event.h"
#include "TractionPower.h"
#include "Camera.h"
#include "mtable.h"
#include "Train.h"
#include "Button.h"
#include "Driver.h"
#include "AnimModel.h"
#include "DynObj.h"
#include "Model3d.h"
#include "renderer.h"
#include "utilities.h"
#include "Logs.h"

void
drivingaid_panel::update() {

    if( false == is_open ) { return; }

    text_lines.clear();

    auto const *train { simulation::Train };
    auto const *controlled { ( train ? train->Dynamic() : nullptr ) };

    if( ( controlled == nullptr )
     || ( controlled->Mechanik == nullptr ) ) { return; }

    auto const *mover = controlled->MoverParameters;
    auto const *driver = controlled->Mechanik;
    auto const *owner = ( controlled->ctOwner != nullptr ? controlled->ctOwner : controlled->Mechanik );

    { // throttle, velocity, speed limits and grade
        std::string expandedtext;
        if( is_expanded ) {
            // grade
            std::string gradetext;
            auto const reverser { ( mover->DirActive > 0 ? 1 : -1 ) };
            auto const grade { controlled->VectorFront().y * 100 * ( controlled->DirectionGet() == reverser ? 1 : -1 ) * reverser };
            if( std::abs( grade ) >= 0.25 ) {
                std::snprintf(
                    m_buffer.data(), m_buffer.size(),
                    locale::strings[ locale::string::driver_aid_grade ].c_str(),
                    grade );
                gradetext = m_buffer.data();
            }
            // next speed limit
            auto const speedlimit { static_cast<int>( owner->VelDesired ) };
            auto nextspeedlimit { speedlimit };
            auto nextspeedlimitdistance { std::numeric_limits<double>::max() };
            if( speedlimit != 0 ) { // if we aren't allowed to move then any next speed limit is irrelevant
                // nie przekraczać rozkladowej
                auto const schedulespeedlimit { (
                    ( ( owner->OrderCurrentGet() & ( Obey_train | Bank ) ) != 0 ) && ( owner->TrainParams.TTVmax > 0.0 ) ? static_cast<int>( owner->TrainParams.TTVmax ) :
                    ( ( owner->OrderCurrentGet() & ( Obey_train | Bank ) ) == 0 ) ? static_cast<int>( owner->fShuntVelocity ) :
                        -1 ) };
                // first take note of any speed change which should occur after passing potential current speed limit
                if( owner->VelLimitLastDist.second > 0 ) {
                    nextspeedlimit = min_speed( schedulespeedlimit, static_cast<int>( owner->VelLimitLastDist.first ) );
                    nextspeedlimitdistance = owner->VelLimitLastDist.second;
                }
                // then take into account speed change ahead, compare it with speed after potentially clearing last limit
                // lower of these two takes priority; otherwise limit lasts at least until potential last limit is cleared
                auto const noactivespeedlimit { owner->VelLimitLastDist.second < 0 };
                auto const speedatproximitydistance { min_speed( schedulespeedlimit, static_cast<int>( owner->VelNext ) ) };
                if( speedatproximitydistance == nextspeedlimit ) {
                    if( noactivespeedlimit ) {
                        nextspeedlimit = speedatproximitydistance;
                        nextspeedlimitdistance = owner->ActualProximityDist;
                    }
                }
                else if( speedatproximitydistance < nextspeedlimit ) {
                    // if the speed limit ahead is more strict than our current limit, it's important enough to report
                    if( speedatproximitydistance < owner->VelDesired ) {
                        nextspeedlimit = speedatproximitydistance;
                        nextspeedlimitdistance = owner->ActualProximityDist;
                    }
                    // otherwise report it only if it's located after our current (lower) limit ends
                    else if( owner->ActualProximityDist > nextspeedlimitdistance ) {
                        nextspeedlimit = speedatproximitydistance;
                        nextspeedlimitdistance = owner->ActualProximityDist;
                    }
                }
                else if( noactivespeedlimit ) { // implicit proximity > last, report only if last limit isn't present
                    nextspeedlimit = speedatproximitydistance;
                    nextspeedlimitdistance = owner->ActualProximityDist;
                }
                // HACK: if our current speed limit extends beyond our scan range don't display potentially misleading information about its length
                if( nextspeedlimitdistance >= EU07_AI_SPEEDLIMITEXTENDSBEYONDSCANRANGE ) {
                    nextspeedlimit = speedlimit;
                }
                // HACK: hide next speed limit if the 'limit' is a vehicle in front of us
                else if( owner->ActualProximityDist == std::abs( owner->TrackObstacle() ) ) {
                    nextspeedlimit = speedlimit;
                }
            }
            std::string nextspeedlimittext;
            if( nextspeedlimit != speedlimit ) {
                std::snprintf(
                    m_buffer.data(), m_buffer.size(),
                    locale::strings[ locale::string::driver_aid_nextlimit ].c_str(),
                    nextspeedlimit,
                    nextspeedlimitdistance * 0.001 );
                nextspeedlimittext = m_buffer.data();
            }
            // current speed and limit
            std::snprintf(
                m_buffer.data(), m_buffer.size(),
                locale::strings[ locale::string::driver_aid_speedlimit ].c_str(),
                static_cast<int>( std::floor( mover->Vel ) ),
                speedlimit,
                nextspeedlimittext.c_str(),
                gradetext.c_str() );
            expandedtext = m_buffer.data();
        }
        // base data and optional bits put together
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::driver_aid_throttle ].c_str(),
            ( mover->EIMCtrlType > 0 ? std::max( 0, static_cast<int>( 100.4 * mover->eimic_real ) ) : driver->Controlling()->MainCtrlPos ),
            ( mover->EIMCtrlType > 0 ? driver->Controlling()->MainCtrlPos : driver->Controlling()->ScndCtrlPos ),
            ( mover->SpeedCtrlUnit.IsActive ? 'T' : mover->DirActive > 0 ? 'D' : mover->DirActive < 0 ? 'R' : 'N' ),
            expandedtext.c_str());

        text_lines.emplace_back( m_buffer.data(), Global.UITextColor );
    }

    { // brakes, air pressure
        std::string expandedtext;
        if( is_expanded ) {
            std::snprintf (
                m_buffer.data(), m_buffer.size(),
                locale::strings[ locale::string::driver_aid_pressures ].c_str(),
                mover->BrakePress * 100,
                mover->PipePress * 100 );
            expandedtext = m_buffer.data();
        }
        auto const basicbraking { mover->fBrakeCtrlPos };
        auto const eimicbraking { std::max( 0.0, -100.0 * mover->eimic_real ) };
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::driver_aid_brakes ].c_str(),
//            ( mover->EIMCtrlType == 0 ? basicbraking : mover->EIMCtrlType == 3 ? ( mover->UniCtrlIntegratedBrakeCtrl ? eimicbraking : basicbraking ) : eimicbraking ),
            ( mover->UniCtrlIntegratedBrakeCtrl ? eimicbraking : basicbraking ),
            mover->LocalBrakePosA * LocalBrakePosNo,
            ( mover->SlippingWheels ? '!' : ' ' ),
            expandedtext.c_str() );

        text_lines.emplace_back( m_buffer.data(), Global.UITextColor );
    }

    { // alerter, hints
        std::string expandedtext;
        if( is_expanded ) {
            auto const stoptime { static_cast<int>( owner->ExchangeTime ) };
            if( stoptime > 0 ) {
                std::snprintf(
                    m_buffer.data(), m_buffer.size(),
                    locale::strings[ locale::string::driver_aid_loadinginprogress ].c_str(),
                    stoptime );
                expandedtext = m_buffer.data();
            }
            else {
                auto const trackobstacledistance { std::abs( owner->TrackObstacle() ) };
                if( trackobstacledistance <= 75.0 ) {
                    std::snprintf(
                        m_buffer.data(), m_buffer.size(),
                        locale::strings[ locale::string::driver_aid_vehicleahead ].c_str(),
                        trackobstacledistance );
                    expandedtext = m_buffer.data();
                }
            }
        }
        std::string textline = (
            ( ( true == TestFlag( mover->SecuritySystem.Status, s_aware ) )
           && ( train != nullptr )
           && ( train->fBlinkTimer > 0 ) ) ?
                locale::strings[ locale::string::driver_aid_alerter ] :
                "          " );
        textline += (
            ( true == TestFlag( mover->SecuritySystem.Status, s_active ) ) ?
                locale::strings[ locale::string::driver_aid_shp ] :
                "     " );

        text_lines.emplace_back( textline + "  " + expandedtext, Global.UITextColor );
    }

    auto const sizex = ( is_expanded ? 735 : 135 );
    size = { sizex, 85 };
}

void
scenario_panel::update() {

    if( false == is_open ) { return; }

    text_lines.clear();

    auto const *train { simulation::Train };
    auto const *controlled { ( train ? train->Dynamic() : nullptr ) };
    auto const &camera { Global.pCamera };
    m_nearest = (
        false == FreeFlyModeFlag ? controlled :
        camera.m_owner != nullptr ? camera.m_owner :
        std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) ); // w trybie latania lokalizujemy wg mapy
    if( m_nearest == nullptr ) { return; }
    auto const *owner { (
        ( ( m_nearest->Mechanik != nullptr ) && ( m_nearest->Mechanik->primary() ) ) ?
            m_nearest->Mechanik :
            m_nearest->ctOwner ) };
    if( owner == nullptr ) { return; }

    std::string textline =
        locale::strings[ locale::string::driver_scenario_currenttask ]
        + "\n " + owner->OrderCurrent();

    text_lines.emplace_back( textline, Global.UITextColor );
}

void
scenario_panel::render() {

    if( false == is_open ) { return; }
    if( true == text_lines.empty() ) { return; }
    if( m_nearest == nullptr ) { return; } // possibly superfluous given the above but, eh

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
    auto const panelname { (
        title.empty() ?
            name :
            title )
        + "###" + name };
    if( true == ImGui::Begin( panelname.c_str(), &is_open, flags ) ) {
        // potential assignment section
        auto const *owner { (
            ( ( m_nearest->Mechanik != nullptr ) && ( m_nearest->Mechanik->primary() ) ) ?
                m_nearest->Mechanik :
                m_nearest->ctOwner ) };
        if( owner != nullptr ) {
            auto const assignmentheader { locale::strings[ locale::string::driver_scenario_assignment ] };
            if( ( false == owner->assignment().empty() )
             && ( true == ImGui::CollapsingHeader( assignmentheader.c_str() ) ) ) {
                ImGui::TextWrapped( "%s", owner->assignment().c_str() );
                ImGui::Separator();
            }
        }
        // current task
        for( auto const &line : text_lines ) {
            ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
        }
        // hints
        if( owner != nullptr ) {
            auto const hintheader{ locale::strings[ locale::string::driver_hint_header ] };
            if( true == ImGui::CollapsingHeader( hintheader.c_str(), ImGuiTreeNodeFlags_DefaultOpen ) ) {
                for( auto const &hint : owner->m_hints ) {
                    auto const isdone { std::get<TController::hintpredicate>( hint )( std::get<float>( hint ) ) };
                    auto const hintcolor{ (
                        isdone ?
                            colors::uitextgreen :
                            Global.UITextColor ) };
                    ImGui::PushStyleColor( ImGuiCol_Text, { hintcolor.r, hintcolor.g, hintcolor.b, hintcolor.a } );
                    ImGui::TextWrapped( locale::strings[ std::get<locale::string>( hint ) ].c_str(), std::get<float>( hint ) );
                    ImGui::PopStyleColor();
                }
            }
        }
    }
    ImGui::End();
}

void
timetable_panel::update() {

    if( false == is_open ) { return; }

    text_lines.clear();
    m_tablelines.clear();

    auto const *train { simulation::Train };
    auto const *controlled { ( train ? train->Dynamic() : nullptr ) };
    auto const &camera { Global.pCamera };
    auto const &time { simulation::Time.data() };

    { // current time
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::driver_timetable_header ].c_str(),
            37, 37,
            locale::strings[ locale::string::driver_timetable_name ].c_str(),
            time.wHour,
            time.wMinute,
            time.wSecond );

        title = m_buffer.data();
    }

    auto *vehicle { (
        false == FreeFlyModeFlag ? controlled :
        camera.m_owner != nullptr ? camera.m_owner :
        std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) ) }; // w trybie latania lokalizujemy wg mapy

    if( vehicle == nullptr ) { return; }
    // if the nearest located vehicle doesn't have a direct driver, try to query its owner
    auto const *owner = (
        ( ( vehicle->Mechanik != nullptr ) && ( vehicle->Mechanik->primary() ) ) ?
            vehicle->Mechanik :
            vehicle->ctOwner );
    if( owner == nullptr ) { return; }

    auto const &table = owner->TrainTimetable();

    // destination
    {
        auto textline = Bezogonkow( owner->Relation(), true );
        if( false == textline.empty() ) {
            textline += " (" + Bezogonkow( owner->TrainName(), true ) + ")";
        }
        text_lines.emplace_back( textline, Global.UITextColor );
    }

    if( false == is_expanded ) {
        // next station
        auto const nextstation = owner->NextStop();
        if( false == nextstation.empty() ) {
            // jeśli jest podana relacja, to dodajemy punkt następnego zatrzymania
            auto textline = " -> " + nextstation;

            text_lines.emplace_back( textline, Global.UITextColor );
        }
    }

    if( is_expanded ) {

        if( owner->is_train() ) {
            // consist data
            auto consistmass { owner->fMass };
            auto consistlength { owner->fLength };
            if( ( false == owner->is_dmu() )
             && ( false == owner->is_emu() ) ) {
				//odejmij lokomotywy czynne, a przynajmniej aktualną
                consistmass -= owner->pVehicle->MoverParameters->TotalMass;
                // subtract potential other half of a two-part vehicle
                auto const *previous { owner->pVehicle->PrevC( coupling::permanent ) };
                if( previous != nullptr ) { consistmass -= previous->MoverParameters->TotalMass; }
                auto const *next { owner->pVehicle->NextC( coupling::permanent ) };
                if( next != nullptr ) { consistmass -= next->MoverParameters->TotalMass; }
			}
            std::snprintf(
                m_buffer.data(), m_buffer.size(),
                locale::strings[ locale::string::driver_timetable_consistdata ].c_str(),
                static_cast<int>( table.LocLoad ),
                static_cast<int>( consistmass / 1000 ),
                static_cast<int>( consistlength ) );

            text_lines.emplace_back( m_buffer.data(), Global.UITextColor );
        }

        if( 0 == table.StationCount ) {
            // only bother if there's stations to list
            text_lines.emplace_back( locale::strings[ locale::string::driver_timetable_notimetable ], Global.UITextColor );
        } 
        else {
			// header
            m_tablelines.emplace_back( u8"┌─────┬────────────────────────────────────┬─────────┬─────┐", Global.UITextColor );

            TMTableLine const *tableline;
            for( int i = table.StationStart; i <= table.StationCount; ++i ) {
                // wyświetlenie pozycji z rozkładu
                tableline = table.TimeTable + i; // linijka rozkładu

                bool vmaxchange { true };
                if( i > table.StationStart ) {
                    auto const *previoustableline { tableline - 1 };
                    if( tableline->vmax == previoustableline->vmax ) {
                        vmaxchange = false;
                    }
                }
                std::string vmax { "   " };
                if( true == vmaxchange ) {
                    vmax += to_string( tableline->vmax, 0 );
                    vmax = vmax.substr( vmax.size() - 3, 3 ); // z wyrównaniem do prawej
                }
                auto const station { (
                    Bezogonkow( tableline->StationName, true )
                    + "                                  " )
                    .substr( 0, 34 ) };
                auto const location { (
                    ( tableline->km > 0.0 ?
                        to_string( tableline->km, 2 ) :
                        "" )
                    + "                                  " )
                    .substr( 0, 34 - tableline->StationWare.size() ) };
                auto const arrival { (
                    tableline->Ah >= 0 ?
                        to_string( int( 100 + tableline->Ah ) ).substr( 1, 2 ) + ":" + to_minutes_str( tableline->Am, true, 3 ) :
                        u8"  │   " ) };
                auto const departure { (
                    tableline->Dh >= 0 ?
                        to_string( int( 100 + tableline->Dh ) ).substr( 1, 2 ) + ":" + to_minutes_str( tableline->Dm, true, 3 ) :
                        u8"  │   " ) };
                auto const candepart { (
                       ( table.StationStart < table.StationIndex )
                    && ( i < table.StationIndex )
                    && ( ( tableline->Ah < 0 ) // pass-through, always valid
                      || ( tableline->is_maintenance ) // maintenance stop, always valid
                      || ( time.wHour * 60 + time.wMinute + time.wSecond * 0.0167 >= tableline->Dh * 60 + tableline->Dm ) ) ) };
                auto const loadchangeinprogress { ( ( static_cast<int>( std::ceil( -1.0 * owner->fStopTime ) ) ) > 0 ) };
                auto const isatpassengerstop { ( true == owner->IsAtPassengerStop ) && ( vehicle->MoverParameters->Vel < 1.0 ) };
                auto const traveltime { (
                    i < 2 ? "   " :
                    tableline->Ah >= 0 ? to_minutes_str( CompareTime( table.TimeTable[ i - 1 ].Dh, table.TimeTable[ i - 1 ].Dm, tableline->Ah, tableline->Am ), false, 3 ) :
                    to_minutes_str( std::max( 0.0, CompareTime( table.TimeTable[ i - 1 ].Dh, table.TimeTable[ i - 1 ].Dm, tableline->Dh, tableline->Dm ) - 0.5 ), false, 3 ) ) };
                auto const linecolor { (
                    ( i != table.StationStart ) ? Global.UITextColor :
                    loadchangeinprogress ? colors::uitextred :
                    candepart ? colors::uitextgreen : // czas minął i odjazd był, to nazwa stacji będzie na zielono
                    isatpassengerstop ? colors::uitextorange :
                    Global.UITextColor ) };
                auto const trackcount{ ( tableline->TrackNo == 1 ? u8" ┃  " : u8" ║  " ) };
                m_tablelines.emplace_back(
                    ( u8"│ " + vmax + u8" │ " + station + trackcount + arrival + u8" │ " + traveltime + u8" │" ),
                    linecolor );
                m_tablelines.emplace_back(
                    ( u8"│     │ " + location + tableline->StationWare + trackcount + departure + u8" │     │" ),
                    linecolor );
                // divider/footer
                if( i < table.StationCount ) {
                    auto const *nexttableline { tableline + 1 };
                    std::string const vmaxnext{ ( tableline->vmax == nexttableline->vmax ? u8"│     ├" : u8"├─────┼" ) };
                    auto const trackcountnext{ ( nexttableline->TrackNo == 1 ? u8"╂" : u8"╫" ) };
                    m_tablelines.emplace_back(
                        vmaxnext + u8"────────────────────────────────────" + trackcountnext + u8"─────────┼─────┤",
                        Global.UITextColor );
                }
                else {
                    m_tablelines.emplace_back(
                        u8"└─────┴────────────────────────────────────┴─────────┴─────┘",
                        Global.UITextColor );
                }
            }
        }
    } // is_expanded
}

void
timetable_panel::render() {

    if( false == is_open ) { return; }
    if( true  == text_lines.empty() ) { return; }

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
    auto const panelname { (
        title.empty() ?
            name :
            title )
        + "###" + name };
    if( true == ImGui::Begin( panelname.c_str(), &is_open, flags ) ) {
        for( auto const &line : text_lines ) {
            ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
        }
        if( is_expanded ) {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 1, 0 ) );
            for( auto const &line : m_tablelines ) {
                ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
            }
            ImGui::PopStyleVar();
        }
    }
    ImGui::End();
}

void
debug_panel::update() {

    if( false == is_open ) { return; }

    // input item bindings
    m_input.train = simulation::Train;
    m_input.controlled = ( m_input.train ? m_input.train->Dynamic() : nullptr );
    m_input.camera = &( Global.pCamera );
    m_input.vehicle = (
        false == FreeFlyModeFlag ? m_input.controlled :
        m_input.camera->m_owner != nullptr ? m_input.camera->m_owner :
        std::get<TDynamicObject *>( simulation::Region->find_vehicle( m_input.camera->Pos, 20, false, false ) ) ); // w trybie latania lokalizujemy wg mapy
    m_input.mover =
        ( m_input.vehicle != nullptr ?
            m_input.vehicle->MoverParameters :
            nullptr );
    m_input.mechanik = (
        m_input.vehicle != nullptr ?
            m_input.vehicle->Mechanik :
            nullptr );

    // header section
    text_lines.clear();

    auto textline = "Version " + Global.asVersion;

    text_lines.emplace_back( textline, Global.UITextColor );

    // sub-sections
    m_vehiclelines.clear();
    m_enginelines.clear();
    m_ailines.clear();
    m_scantablelines.clear();
    m_scenariolines.clear();
    m_eventqueuelines.clear();
    m_powergridlines.clear();
    m_cameralines.clear();
    m_rendererlines.clear();

    update_section_vehicle( m_vehiclelines );
    update_section_engine( m_enginelines );
    update_section_ai( m_ailines );
    update_section_scantable( m_scantablelines );
    update_section_scenario( m_scenariolines );
    update_section_eventqueue( m_eventqueuelines );
    update_section_powergrid( m_powergridlines );
    update_section_camera( m_cameralines );
    update_section_renderer( m_rendererlines );
}

void
debug_panel::render() {

    if( false == is_open ) { return; }

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
    auto const panelname { (
        title.empty() ?
            name :
            title )
        + "###" + name };
    if( true == ImGui::Begin( panelname.c_str(), &is_open, flags ) ) {
        // header section
        for( auto const &line : text_lines ) {
            ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
        }
        // sections
        ImGui::Separator();
        if( true == render_section( "Vehicle", m_vehiclelines ) ) {
            if( ( m_input.mover )
             && ( m_input.mover->DamageFlag != 0 ) ) {
                if( true == ImGui::Button( "Fix Status" ) ) {
                    // TODO: refactor status reset into mover method
                    m_input.mover->DamageFlag = 0;
                    m_input.mover->EngDmgFlag = 0;
                    if( std::abs( m_input.mover->V ) < 0.0001 ) { // don't alter speed of already moving vehicle
                        // HACK: force vehicle position re-calculation
                        m_input.mover->V = 0.0001;
                    }
//                    m_input.mover->DistCounter = 0.0;
                    m_input.mover->WheelFlat = 0.0;
                    m_input.mover->AlarmChainFlag = false;
                    m_input.mover->OffsetTrackH = 0.0;
                    m_input.mover->OffsetTrackV = 0.0;
                    // pantographs
                    for( auto idx = 0; idx < m_input.vehicle->iAnimType[ ANIM_PANTS ]; ++idx ) {
                        auto &pantograph { *( m_input.vehicle->pants[ idx ].fParamPants ) };
                        if( pantograph.PantWys >= 0.0 ) { // skip undamaged instances
                            continue; }
                        ++m_input.mover->EnginePowerSource.CollectorParameters.CollectorsNo;
                        pantograph.fAngleL = pantograph.fAngleL0;
                        pantograph.fAngleU = pantograph.fAngleU0;
                        pantograph.PantWys =
                              pantograph.fLenL1 * std::sin( pantograph.fAngleL )
                            + pantograph.fLenU1 * std::sin( pantograph.fAngleU )
                            + pantograph.fHeight;
                    }
                }
            }
        }
        render_section( "Vehicle Engine", m_enginelines );
        render_section( "Vehicle AI", m_ailines );
        render_section( "Vehicle Scan Table", m_scantablelines );
        render_section_scenario();
        render_section_eventqueue();
        if( true == render_section( "Power Grid", m_powergridlines ) ) {
            // traction state debug
            ImGui::Checkbox( "Debug Traction", &DebugTractionFlag );
        }
        render_section( "Camera", m_cameralines );
        render_section( "Gfx Renderer", m_rendererlines );
        render_section_settings();
        // toggles
        ImGui::Separator();
        ImGui::Checkbox( "Debug Mode", &DebugModeFlag );
    }
    ImGui::End();
}

bool
debug_panel::render_section_scenario() {

    if( false == render_section( "Scenario", m_scenariolines ) ) { return false; }
    if( Application.is_client() ) {
        // NOTE: simulation clients can't adjust scenarion state, this is reserved for the server/standalone instance
        return true;
    }
    // fog slider
    {
        auto fogrange = std::log( Global.fFogEnd );
        if( ImGui::SliderFloat(
            ( to_string( std::exp( fogrange ), 0, 5 ) + " m###fogend" ).c_str(), &fogrange, std::log( 10.0f ), std::log( 25000.0f ), "Fog distance" ) ) {
            command_relay relay;
            relay.post(
                user_command::setweather,
                clamp( std::exp( fogrange ), 10.0f, 25000.0f ),
                Global.Overcast,
                GLFW_PRESS, 0 );
        }
    }
    // cloud cover slider
    {
        if( ImGui::SliderFloat(
            ( to_string( Global.Overcast, 2, 5 ) + " (" + Global.Weather + ")###overcast" ).c_str(), &Global.Overcast, 0.0f, 2.0f, "Cloud cover" ) ) {
            command_relay relay;
            relay.post(
                user_command::setweather,
                Global.fFogEnd,
                clamp( Global.Overcast, 0.0f, 2.0f ),
                GLFW_PRESS, 0 );
        }
    }
    // day of year slider
    {
        if( ImGui::SliderFloat(
            ( to_string( Global.fMoveLight, 0, 5 ) + " (" + Global.Season + ")###movelight" ).c_str(), &Global.fMoveLight, 0.0f, 364.0f, "Day of year" ) ) {
            command_relay relay;
            relay.post(
                user_command::setdatetime,
                clamp( Global.fMoveLight, 0.0f, 365.0f ),
                simulation::Time.data().wHour * 60 + simulation::Time.data().wMinute,
                GLFW_PRESS, 0 );
        }
    }
    // dynamic material update checkbox
    ImGui::Checkbox( "Update Item Materials", &Global.UpdateMaterials );
    if( DebugModeFlag ) {
        if( ImGui::Checkbox( "Force Daylight", &Global.FakeLight ) ) {
            simulation::Environment.on_daylight_change();
        }
    }
    // advanced options, only visible in debug mode
    if( DebugModeFlag ) {
        // time of day slider
        {
            ImGui::PushStyleColor( ImGuiCol_Text, { Global.UITextColor.r, Global.UITextColor.g, Global.UITextColor.b, Global.UITextColor.a } );
            ImGui::TextUnformatted( "CAUTION: time change will affect simulation state" );
            ImGui::PopStyleColor();
            auto time = simulation::Time.data().wHour * 60 + simulation::Time.data().wMinute;
            auto const timestring {
                std::string( to_string( int( 100 + simulation::Time.data().wHour ) ).substr( 1, 2 )
                    + ":"
                    + std::string( to_string( int( 100 + simulation::Time.data().wMinute ) ).substr( 1, 2 ) ) ) };
            if( ImGui::SliderInt( ( timestring + " (" + Global.Period + ")###simulationtime" ).c_str(), &time, 0, 1439, "Time of day" ) ) {
                command_relay relay;
                relay.post(
                    user_command::setdatetime,
                    Global.fMoveLight,
                    clamp( time, 0, 1439 ),
                    GLFW_PRESS, 0 );
            }
        }
        // time acceleration
        {
            auto timerate { ( Global.fTimeSpeed == 60 ? 4 : Global.fTimeSpeed == 20 ? 3 : Global.fTimeSpeed == 5 ? 2 : 1 ) };
            if( ImGui::SliderInt( ( "x " + to_string( Global.fTimeSpeed, 0 ) + "###timeacceleration" ).c_str(), &timerate, 1, 4, "Time acceleration" ) ) {
                Global.fTimeSpeed = ( timerate == 4 ? 60 : timerate == 3 ? 20 : timerate == 2 ? 5 : 1 );
            }
        }
    }

    return true;
}

bool
debug_panel::render_section_eventqueue() {

    if( false == ImGui::CollapsingHeader( "Scenario Event Queue" ) ) { return false; }
    // event queue name filter
    ImGui::PushItemWidth( -1 );
    ImGui::InputTextWithHint( "", "Search event queue", m_eventsearch.data(), m_eventsearch.size() );
    ImGui::PopItemWidth();
    // event queue
    render_section( m_eventqueuelines );
    // event queue activator filter
    ImGui::Checkbox( "By This Vehicle Only", &m_eventqueueactivevehicleonly );

    return true;
}

void
debug_panel::update_section_vehicle( std::vector<text_line> &Output ) {

    if( m_input.vehicle == nullptr ) { return; }
    if( m_input.mover == nullptr ) { return; }

    auto const &vehicle { *m_input.vehicle };
    auto const &mover { *m_input.mover };

    auto const isowned { /* ( vehicle.Mechanik == nullptr ) && */ ( vehicle.ctOwner != nullptr ) && ( vehicle.ctOwner->Vehicle() != m_input.vehicle ) };
    auto const isdieselenginepowered { ( mover.EngineType == TEngineType::DieselElectric ) || ( mover.EngineType == TEngineType::DieselEngine ) };
    auto const isdieselinshuntmode { mover.ShuntMode && mover.EngineType == TEngineType::DieselElectric };

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        locale::strings[ locale::string::debug_vehicle_nameloadstatuscouplers ].c_str(),
        mover.Name.c_str(),
        std::string( isowned ? locale::strings[ locale::string::debug_vehicle_owned ].c_str() + vehicle.ctOwner->OwnerName() : "" ).c_str(),
        mover.LoadAmount,
        mover.LoadType.name.c_str(),
        mover.EngineDescription( 0 ).c_str(),
        // TODO: put wheel flat reporting in the enginedescription()
        std::string( mover.WheelFlat > 0.01 ? " Flat: " + to_string( mover.WheelFlat, 1 ) + " mm" : "" ).c_str(),
        update_vehicle_coupler( end::front ).c_str(),
        update_vehicle_coupler( end::rear ).c_str() );

    Output.emplace_back( std::string{ m_buffer.data() }, Global.UITextColor );

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        locale::strings[ locale::string::debug_vehicle_devicespower ].c_str(),
        // devices
        ( mover.Battery ? 'B' : '.' ),
        ( mover.PantsValve.is_active ? '+' : '.' ),
        ( mover.Pantographs[ end::rear  ].valve.is_active ? 'O' : ( mover.Pantographs[ end::rear  ].valve.is_enabled ? 'o' : '.' ) ),
        ( mover.Pantographs[ end::front ].valve.is_active ? 'P' : ( mover.Pantographs[ end::front ].valve.is_enabled ? 'p' : '.' ) ),
        ( mover.PantPressLockActive ? '!' : ( mover.PantPressSwitchActive ? '*' : '.' ) ),
        ( mover.WaterPump.is_active ? 'W' : ( false == mover.WaterPump.breaker ? '-' : ( mover.WaterPump.is_enabled ? 'w' : '.' ) ) ),
        ( mover.WaterHeater.is_damaged ? '!' : ( mover.WaterHeater.is_active ? 'H' : ( false == mover.WaterHeater.breaker ? '-' : ( mover.WaterHeater.is_enabled ? 'h' : '.' ) ) ) ),
        ( mover.FuelPump.is_active ? 'F' : ( mover.FuelPump.is_enabled ? 'f' : '.' ) ),
        ( mover.OilPump.is_active ? 'O' : ( mover.OilPump.is_enabled ? 'o' : '.' ) ),
        ( mover.Mains ? 'M' : '.' ),
        ( mover.FuseFlag ? '!' : '.' ),
        ( mover.ConverterFlag ? 'X' : ( false == mover.ConverterAllowLocal ? '-' : ( mover.ConverterAllow ?  'x' : '.' ) ) ),
        ( mover.ConvOvldFlag ? '!' : '.' ),
        ( mover.CompressorFlag ? 'C' : ( false == mover.CompressorAllowLocal ? '-' : ( ( mover.CompressorAllow || ( mover.CompressorStart == start_t::automatic && mover.CompressorSpeed > 0.0 ) ) ? 'c' : '.' ) ) ),
        ( mover.CompressorGovernorLock ? '!' : '.' ),
        ( mover.StLinSwitchOff ? '-' : ( mover.ControlPressureSwitch ? '!' : ( mover.StLinFlag ? '+' : '.' ) ) ),
        ( mover.Heating ? 'H' : ( mover.HeatingAllow ? 'h' : '.' ) ),
        ( mover.Hamulec->Releaser() ? '^' : '.' ),
        std::string( m_input.mechanik ? locale::strings[ locale::string::debug_vehicle_radio ] + ( mover.Radio ? std::to_string( m_input.mechanik->iRadioChannel ) : "-" ) : "" ).c_str(),
        std::string( isdieselenginepowered ? locale::strings[ locale::string::debug_vehicle_oilpressure ] + to_string( mover.OilPump.pressure, 2 )  : "" ).c_str(),
        // power transfers
        // 3000v
        mover.Couplers[ end::front ].power_high.voltage,
        mover.Couplers[ end::front ].power_high.current,
        std::string( mover.Couplers[ end::front ].power_high.is_local ? ":" : ":=" ).c_str(),
        mover.EngineVoltage,
        std::string( mover.Couplers[ end::rear ].power_high.is_local ? ":" : "=:" ).c_str(),
        mover.Couplers[ end::rear ].power_high.voltage,
        mover.Couplers[ end::rear ].power_high.current,
        // 110v
        mover.Couplers[ end::front ].power_110v.voltage,
        mover.Couplers[ end::front ].power_110v.current,
        std::string( mover.Couplers[ end::front ].power_110v.is_local ? ":" : ":=" ).c_str(),
        mover.PowerCircuits[ 1 ].first,
        std::string( mover.Couplers[ end::rear ].power_110v.is_local ? ":" : "=:" ).c_str(),
        mover.Couplers[ end::rear ].power_110v.voltage,
        mover.Couplers[ end::rear ].power_110v.current );

    Output.emplace_back( m_buffer.data(), Global.UITextColor );

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        locale::strings[ locale::string::debug_vehicle_controllersenginerevolutions ].c_str(),
        // controllers
        mover.MainCtrlPos,
        mover.MainCtrlActualPos,
        std::string( isdieselinshuntmode ? to_string( mover.AnPos, 2 ) + locale::strings[ locale::string::debug_vehicle_shuntmode ] : std::to_string( mover.ScndCtrlPos ) + "(" + std::to_string( mover.ScndCtrlActualPos ) + ")" ).c_str(),
        // engine
        mover.EnginePower,
        std::abs( mover.TrainType == dt_EZT ? mover.ShowCurrent( 0 ) : mover.Im ),
        // revolutions
        std::abs( mover.enrot ) * 60,
        std::abs( mover.nrot ) * mover.Transmision.Ratio * 60,
        mover.RventRot * 60,
        std::abs( mover.MotorBlowers[end::front].revolutions ),
        std::abs( mover.MotorBlowers[end::rear].revolutions ),
        mover.dizel_heat.rpmw,
        mover.dizel_heat.rpmw2 );

    std::string textline { m_buffer.data() };

    if( isdieselenginepowered ) {
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::debug_vehicle_temperatures ].c_str(),
            mover.dizel_heat.Ts,
            mover.dizel_heat.To,
            mover.dizel_heat.temperatura1,
            ( mover.WaterCircuitsLink ? '-' : '|' ),
            mover.dizel_heat.temperatura2 );
        textline += m_buffer.data();
    }

    Output.emplace_back( textline, Global.UITextColor );

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        locale::strings[ locale::string::debug_vehicle_brakespressures ].c_str(),
        // brakes
        mover.fBrakeCtrlPos,
        mover.BrakeOpModeFlag,
        update_vehicle_brake().c_str(),
        mover.LoadFlag,
        mover.LocalBrakePosA,
        mover.LocalBrakePosAEIM,
        ( mover.ManualBrakePos / static_cast<float>( ManualBrakePosNo ) ),
        ( mover.SpringBrake.Activate ? 1.f : 0.f ),
        // cylinders
        mover.BrakePress,
        mover.LocBrakePress,
        mover.Hamulec->GetBrakeStatus(),
        // pipes
        mover.PipePress,
        mover.BrakeCtrlPos2,
        mover.ScndPipePress,
        mover.CntrlPipePress,
        // tanks
        mover.Hamulec->GetBRP(),
        mover.Compressor,
        mover.Hamulec->GetCRP() );

    textline = m_buffer.data();

    if( mover.EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::debug_vehicle_pantograph ].c_str(),
            mover.PantPress,
            ( mover.bPantKurek3 ? '-' : '|' ) );
        textline += m_buffer.data();
    }

    Output.emplace_back( textline, Global.UITextColor );

    if( tprev != simulation::Time.data().wSecond ) {
        tprev = simulation::Time.data().wSecond;
        Acc = ( mover.Vel - VelPrev ) / 3.6;
        VelPrev = mover.Vel;
    }

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        locale::strings[ locale::string::debug_vehicle_forcesaccelerationvelocityposition ].c_str(),
        // forces
        mover.Ft * 0.001f * ( mover.CabOccupied ? mover.CabOccupied : vehicle.ctOwner ? vehicle.ctOwner->Controlling()->CabOccupied : 1 ) + 0.001f,
        mover.Fb * 0.001f,
        mover.Adhesive( mover.RunningTrack.friction ),
        ( mover.SlippingWheels ? " (!)" : "" ),
        // acceleration
        Acc,
        mover.AccN + 0.001f,
        std::string( std::abs( mover.RunningShape.R ) > 15000.0 ? "~0" : to_string( mover.RunningShape.R, 0 ) ).c_str(),
        // velocity
        vehicle.GetVelocity(),
        mover.DistCounter,
        // position
        vehicle.GetPosition().x,
        vehicle.GetPosition().y,
        vehicle.GetPosition().z );

    Output.emplace_back( m_buffer.data(), Global.UITextColor );

    if( mover.EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::debug_vehicle_poweruse ].c_str(),
            std::abs( mover.EnergyMeter.first ),
            std::abs( mover.EnergyMeter.second ),
            mover.EnergyMeter.first + mover.EnergyMeter.second );
        textline = m_buffer.data();
        if( DebugTractionFlag ) {
            for( int i = 0; i < vehicle.iAnimType[ ANIM_PANTS ]; ++i ) { // pętla po wszystkich pantografach
                auto const *p { vehicle.pants[ i ].fParamPants };
                if( p && p->hvPowerWire ) {
                    auto const *powerwire { p->hvPowerWire };
                    std::snprintf(
                        m_buffer.data(), m_buffer.size(),
                        locale::strings[ locale::string::debug_vehicle_powerwire ].c_str(),
                        i,
                        powerwire->psPower[ 0 ] ? powerwire->psPower[ 0 ]->name() : "none",
                        powerwire->psPower[ 1 ] ? powerwire->psPower[ 1 ]->name() : "none",
                        powerwire->pPoint1[ 0 ],
                        powerwire->pPoint1[ 1 ],
                        powerwire->pPoint1[ 2 ] );
                    textline += m_buffer.data();
                }
            }
        }
        Output.emplace_back( textline, Global.UITextColor );
    }
}

std::string
debug_panel::update_vehicle_coupler( int const Side ) {
    // NOTE: mover and vehicle are guaranteed to be valid by the caller
    auto const &mover { *( m_input.mover ) };

    std::string const controltype{ ( mover.Couplers[ Side ].control_type.empty() ? "[*]" : "[" + mover.Couplers[ Side ].control_type + "]" ) };
    std::string couplerstatus { locale::strings[ locale::string::debug_vehicle_none ] };
    std::string const adapterstatus { ( mover.Couplers[ Side ].adapter_type == TCouplerType::NoCoupler ? "" : "[A]" ) };

    auto const *connected { m_input.vehicle->MoverParameters->Neighbours[ Side ].vehicle };

    if( connected == nullptr ) {
        
        return controltype + " " + couplerstatus + " " + adapterstatus;
    }

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        "%s %s %s[%d] (%.1f m)",
        controltype.c_str(),
        connected->name().c_str(),
        adapterstatus.c_str(),
        mover.Couplers[ Side ].CouplingFlag,
        mover.Neighbours[ Side ].distance );

    return { m_buffer.data() };
}

std::string
debug_panel::update_vehicle_brake() const {
    // NOTE: mover is guaranteed to be valid by the caller
    auto const &mover { *( m_input.mover ) };

    std::string brakedelay;

    std::vector<std::pair<int, std::string>> delays {
        { bdelay_G, "G" },
        { bdelay_P, "P" },
        { bdelay_R, "R" },
        { bdelay_M, "+Mg" } };

    for( auto const &delay : delays ) {
        if( ( mover.BrakeDelayFlag & delay.first ) == delay.first ) {
            brakedelay += delay.second;
        }
    }

    return brakedelay;
}

void
debug_panel::update_section_engine( std::vector<text_line> &Output ) {
    // engine data
    if( m_input.vehicle == nullptr ) { return; }
    if( m_input.mover == nullptr ) { return; }

    auto const &vehicle{ *m_input.vehicle };
    auto const &mover{ *m_input.mover };

    // induction motor data
    if( mover.EngineType == TEngineType::ElectricInductionMotor ) {

        Output.emplace_back( "      eimc:            eimv:            press:", Global.UITextColor );
        for( int i = 0; i <= 20; ++i ) {

            std::string parameters =
                mover.eimc_labels[ i ] + to_string( mover.eimc[ i ], 2, 9 )
                + " | "
                + mover.eimv_labels[ i ] + to_string( mover.eimv[ i ], 2, 9 );

            if( i < 10 ) {
                // NOTE: we pull consist data from the train structure, so show this data only if we're viewing controlled vehicle
                parameters +=
                    ( ( m_input.train != nullptr ) && ( m_input.train->Dynamic() == m_input.vehicle ) ?
                        " | " + TTrain::fPress_labels[ i ] + to_string( m_input.train->fPress[ i ][ 0 ], 2, 9 ) :
                        "" );
            }
            else if( i == 12 ) {
                parameters += "        med:";
            }
            else if( i >= 13 ) {
                parameters += " | " + vehicle.MED_labels[ i - 13 ] + to_string( vehicle.MED[ 0 ][ i - 13 ], 2, 9 );
            }

            Output.emplace_back( parameters, Global.UITextColor );
        }
        Output.emplace_back( "Inverter:\n frequency: " + to_string( mover.InverterFrequency, 2 ), Global.UITextColor );
    }
    // diesel engine data
    if( mover.EngineType == TEngineType::DieselEngine ) {

		double fuel_mean = 0.0;
		if (mover.DistCounter > 0.5) {
			fuel_mean = 100 * mover.dizel_FuelConsumptedTotal / mover.DistCounter;
		}

        std::string parameterstext = "param       value";
        std::vector< std::pair <std::string, double> > const paramvalues {
			{ "  rpm: ", mover.enrot * 60.0 },
			{ "efill: ", mover.dizel_fill },
            { "etorq: ", mover.dizel_Torque },
			{ "power: ", mover.dizel_Power },
			{ "f_act: ", mover.dizel_FuelConsumptionActual },
			{ "f_tot: ", mover.dizel_FuelConsumptedTotal },
			{ "fmean: ", fuel_mean },
            { "creal: ", mover.dizel_engage },
            { "cdesi: ", mover.dizel_engagestate },
            { "cdelt: ", mover.dizel_engagedeltaomega },
            { "gears: ", mover.dizel_automaticgearstatus} };
        for( auto const &parameter : paramvalues ) {
            parameterstext += "\n" + parameter.first + to_string( parameter.second, 2, 9 );
        }
        Output.emplace_back( parameterstext, Global.UITextColor );

        parameterstext = "hydro      value";
        std::vector< std::pair <std::string, double> > const hydrovalues {
            { "hTCnI: ", mover.hydro_TC_nIn },
            { "hTCnO: ", mover.hydro_TC_nOut },
            { "hTCTM: ", mover.hydro_TC_TMRatio },
            { "hTCTI: ", mover.hydro_TC_TorqueIn },
            { "hTCTO: ", mover.hydro_TC_TorqueOut },
            { "hTCfl: ", mover.hydro_TC_Fill },
            { "hRtFl: ", mover.hydro_R_Fill } ,
			{ " hRtn: ", mover.hydro_R_n } ,
			{ "hRtTq: ", mover.hydro_R_Torque }
		
		};
        for( auto const &parameter : hydrovalues ) {
            parameterstext += "\n" + parameter.first + to_string( parameter.second, 2, 9 );
        }
        Output.emplace_back( parameterstext, Global.UITextColor );
    }
}

void
debug_panel::update_section_ai( std::vector<text_line> &Output ) {

    if( m_input.mover == nullptr )    { return; }
    if( m_input.mechanik == nullptr ) { return; }

    auto const &mover{ *m_input.mover };
    auto const &mechanik{ *m_input.mechanik };

    // biezaca komenda dla AI
    auto textline =
        "Current order: [" + std::to_string( mechanik.OrderPos ) + "] "
        + mechanik.Order2Str( mechanik.OrderCurrentGet() );

    if( mechanik.fStopTime < 0.0 ) {
        textline += "\n stop time: " + to_string( std::abs( mechanik.fStopTime ), 1 );
    }

    Output.emplace_back( textline, Global.UITextColor );

    if( ( mechanik.VelNext == 0.0 )
     && ( mechanik.eSignNext ) ) {
        // jeśli ma zapamiętany event semafora, nazwa eventu semafora
        Output.emplace_back( "Current signal: " + Bezogonkow( mechanik.eSignNext->m_name ), Global.UITextColor );
    }

    // distances
    textline =
        "Distances:\n proximity: " + to_string( mechanik.ActualProximityDist, 0 )
        + ", braking: " + to_string( mechanik.fBrakeDist, 0 );
    if( mechanik.VelLimitLastDist.second > 0 ) {
        textline += ", last limit: " + (
            mechanik.VelLimitLastDist.second < EU07_AI_SPEEDLIMITEXTENDSBEYONDSCANRANGE ?
                to_string( mechanik.VelLimitLastDist.second, 0 ) :
                "???" );
    }
    if( mechanik.SwitchClearDist > 0 ) {
        textline += ", switches: " + to_string( mechanik.SwitchClearDist, 0 );
    }

    if( mechanik.Obstacle.distance < 5000 ) {
        textline +=
            "\n obstacle: " + to_string( mechanik.Obstacle.distance, 0 )
            + " (" + mechanik.Obstacle.vehicle->asName + ")";
    }

    Output.emplace_back( textline, Global.UITextColor );

    // velocity factors
    textline =
        "Velocity:\n desired: " + to_string( mechanik.VelDesired, 0 )
        + ", next: " + to_string( mechanik.VelNext, 0 )
        + "\n current: " + to_string( mover.Vel + 0.001f, 2 );

    std::vector< std::pair< double, std::string > > const restrictions{
        { mechanik.VelSignalLast, "signal" },
        { mechanik.VelLimitLast, "limit" },
        { mechanik.VelRoad, "road" },
        { mechanik.VelRestricted, "restricted" },
        { mover.RunningTrack.Velmax, "track" } };

    std::string restrictionstext;
    for( auto const &restriction : restrictions ) {
        if( restriction.first < 0.0 ) { continue; }
        if( false == restrictionstext.empty() ) {
            restrictionstext += ", ";
        }
        restrictionstext +=
            to_string( restriction.first, 0 )
            + " (" + restriction.second + ")";
    }

    if( false == restrictionstext.empty() ) {
        textline += "\n restrictions: " + restrictionstext;
    }

    Output.emplace_back( textline, Global.UITextColor );

    // acceleration
    textline =
        "Acceleration:\n desired: " + to_string( mechanik.AccDesired, 2 )
        + ", corrected: " + to_string( mechanik.AccDesired * mechanik.BrakeAccFactor(), 2 )
        + "\n current: " + to_string( mechanik.AbsAccS + 0.001f, 2 )
        + ", slope: " + to_string( mechanik.fAccGravity + 0.001f, 2 ) + " (" + ( mechanik.fAccGravity > 0.01 ? "\\" : ( mechanik.fAccGravity < -0.01 ? "/" : "-" ) ) + ")"
		+ "\n desired diesel percentage: " + to_string(mechanik.DizelPercentage)
	    + "/" + to_string(mechanik.DizelPercentage_Speed)
		+ "/" + to_string(100.4*mechanik.mvControlling->eimic_real, 0);

    Output.emplace_back( textline, Global.UITextColor );

    // brakes
    textline =
        "Brakes:\n highest pressure: " + to_string( mechanik.fReady, 2 ) + ( mechanik.Ready ? " (all brakes released)" : "" )
        + "\n activation threshold: " + to_string( mechanik.fAccThreshold, 2 )
        + ", delays: " + to_string( mechanik.fBrake_a0[ 0 ], 2 ) + " + " + to_string( mechanik.fBrake_a1[ 0 ], 2 )
        + "\n virtual brake position: " + to_string( mechanik.BrakeCtrlPosition, 2 )
        + "\n test stage: " + to_string( mechanik.DynamicBrakeTest )
        + ", applied at: " + to_string( mechanik.DBT_VelocityBrake, 0 )
        + ", release at: " + to_string( mechanik.DBT_VelocityRelease, 0 )
        + ", done at: " + to_string( mechanik.DBT_VelocityFinish, 0 );

    Output.emplace_back( textline, Global.UITextColor );

    // ai driving flags
    std::vector<std::string> const drivingflagnames {
        "StopCloser", "StopPoint", "Active", "Press", "Connect", "Primary", "Late", "StopHere",
        "StartHorn", "StartHornNow", "StartHornDone", "Oerlikons", "IncSpeed", "TrackEnd", "SwitchFound", "GuardSignal",
        "Visibility", "DoorOpened", "PushPull", "SignalFound", "StopPointFound" /*"SemaphorWasElapsed", "TrainInsideStation", "SpeedLimitFound"*/ };

    textline = "Driving flags:";
    for( int idx = 0, flagbit = 1; idx < drivingflagnames.size(); ++idx, flagbit <<= 1 ) {
        if( mechanik.DrivigFlags() & flagbit ) {
            textline += "\n " + drivingflagnames[ idx ];
        }
    }

    Output.emplace_back( textline, Global.UITextColor );
}

void
debug_panel::update_section_scantable( std::vector<text_line> &Output ) {

    if( m_input.mechanik == nullptr ) { return; }

    Output.emplace_back( "Flags:       Dist:    Vel:  Name:", Global.UITextColor );

    auto const &mechanik{ *m_input.mechanik };

    std::size_t i = 0; std::size_t const speedtablesize = clamp( static_cast<int>( mechanik.TableSize() ) - 1, 0, 30 );
    do {
        auto const scanline = mechanik.TableText( i );
        if( scanline.empty() ) { break; }
        Output.emplace_back( Bezogonkow( scanline ), Global.UITextColor );
        ++i;
    } while( i < speedtablesize );
    if( Output.size() == 1 ) {
        Output.front().data = "(no points of interest)";
    }
}

void
debug_panel::update_section_scenario( std::vector<text_line> &Output ) {

    auto textline =
        "vehicles: " + to_string( Timer::subsystem.sim_dynamics.average(), 2 ) + " msec"
        + " update total: " + to_string( Timer::subsystem.sim_total.average(), 2 ) + " msec";

    Output.emplace_back( textline, Global.UITextColor );
    // current luminance level
    textline = "Light level: " + to_string( Global.fLuminance, 3 ) + ( Global.FakeLight ? "(*)" : "" );
    textline +=
        "\nWind: azimuth "
        + to_string( simulation::Environment.wind_azimuth(), 0 ) // ma być azymut, czyli 0 na północy i rośnie na wschód
        + " "
        + std::string( "N NEE SES SWW NW" )
        .substr( 0 + 2 * std::floor( std::fmod( 8 + ( glm::radians( simulation::Environment.wind_azimuth() ) + 0.5 * M_PI_4 ) / M_PI_4, 8 ) ), 2 )
        + ", " + to_string( glm::length( simulation::Environment.wind() ), 1 ) + " m/s";
    textline += "\nAir temperature: " + to_string( Global.AirTemperature, 1 ) + " deg C";

    Output.emplace_back( textline, Global.UITextColor );
}

void
debug_panel::update_section_eventqueue( std::vector<text_line> &Output ) {

    std::string textline;

    // current event queue
    auto const time { Timer::GetTime() };
    auto const *event { simulation::Events.begin() };
    auto const searchfilter { std::string( m_eventsearch.data() ) };

    Output.emplace_back( "Delay:   Event:", Global.UITextColor );

    while( ( event != nullptr )
        && ( Output.size() < 30 ) ) {

        if( ( false == event->m_ignored )
         && ( false == event->m_passive )
         && ( ( false == m_eventqueueactivevehicleonly )
           || ( event->m_activator == m_input.vehicle ) ) ) {

            auto const label { event->m_name + ( event->m_activator ? " (by: " + event->m_activator->asName + ")" : "" ) };

            if( ( false == searchfilter.empty() )
             && ( false == contains( label, searchfilter ) ) ) {
                event = event->m_next;
                continue;
            }

            auto const delay { "   " + to_string( std::max( 0.0, event->m_launchtime - time ), 1 ) };
            textline =
                delay.substr( delay.length() - 6 )
                + "   "
                + label + ( event->m_sibling ? " (joint event)" : "" );

            Output.emplace_back( textline, Global.UITextColor );
        }
        event = event->m_next;
    }
    if( Output.size() == 1 ) {
        // event queue can be empty either because no event got through active filters, or because it is genuinely empty
        Output.front().data = (
            simulation::Events.begin() == nullptr ?
                "(no queued events)" :
                "(no matching events)" );
    }
}

void
debug_panel::update_section_powergrid( std::vector<text_line> &Output ) {

    auto const lowpowercolor { glm::vec4( 164.0f / 255.0f, 132.0f / 255.0f, 84.0f / 255.0f, 1.f ) };
    auto const nopowercolor { glm::vec4( 164.0f / 255.0f, 84.0f / 255.0f, 84.0f / 255.0f, 1.f ) };

    Output.emplace_back( "Name:               Output: Current: Timeout:", Global.UITextColor );

    std::string textline;

    for( auto const *powerstation : simulation::Powergrid.sequence() ) {

        if( true == powerstation->IsAutogenerated ) { continue; }
        if( true == powerstation->bSection )        { continue; }

        auto const name { (
            powerstation->m_name.empty() ?
                "(unnamed)" :
                powerstation->m_name )
            + "                              " };

        textline =
            name.substr( 0, 20 )
            + " " + to_string( powerstation->OutputVoltage, 0, 5 )
            + " " + to_string( powerstation->TotalCurrent, 1, 8 )
            + " " + to_string( powerstation->FuseTimer, 1, 8 )
            + ( powerstation->FuseCounter == 0 ?
                "" :
                " (x" + to_string( powerstation->FuseCounter ) + ")" );

        Output.emplace_back(
            textline,
            ( ( powerstation->FastFuse || powerstation->SlowFuse ) ? nopowercolor :
              powerstation->OutputVoltage < ( 0.8 * powerstation->NominalVoltage ) ? lowpowercolor :
              Global.UITextColor ) );
    }

    if( Output.size() == 1 ) {
        Output.front().data = "(no power stations)";
    }
}

void
debug_panel::update_section_camera( std::vector<text_line> &Output ) {

    if( m_input.camera == nullptr ) { return; }

    auto const &camera{ *m_input.camera };

    // camera data
    auto textline =
        "Position: ["
        + to_string( camera.Pos.x, 2 ) + ", "
        + to_string( camera.Pos.y, 2 ) + ", "
        + to_string( camera.Pos.z, 2 ) + "]";

    Output.emplace_back( textline, Global.UITextColor );

    textline =
        "Azimuth: "
        + to_string( 180.0 - glm::degrees( camera.Angle.y ), 0 ) // ma być azymut, czyli 0 na północy i rośnie na wschód
        + " "
        + std::string( "S SEE NEN NWW SW" )
        .substr( 0 + 2 * floor( fmod( 8 + ( camera.Angle.y + 0.5 * M_PI_4 ) / M_PI_4, 8 ) ), 2 );

    Output.emplace_back( textline, Global.UITextColor );
}

void
debug_panel::update_section_renderer( std::vector<text_line> &Output ) {

            // gfx renderer data
            auto textline =
                "FoV: " + to_string( Global.FieldOfView / Global.ZoomFactor, 1 )
                + ", Draw range: " + to_string( Global.BaseDrawRange * Global.fDistanceFactor, 0 ) + "m"
//                + "; sectors: " + std::to_string( GfxRenderer->m_drawcount )
//                + ", FPS: " + to_string( Timer::GetFPS(), 2 );
                + ", FPS: " + std::to_string( static_cast<int>(std::round(GfxRenderer->Framerate())) )
                + ( Global.VSync ? " (vsync on)" : "" );
            if( Global.iSlowMotion ) {
                textline += " (slowmotion " + to_string( Global.iSlowMotion ) + ")";
            }

            Output.emplace_back( textline, Global.UITextColor );

            textline =
                std::string( "Rendering mode: " )
                + ( Global.GfxRenderer == "default" ?
                    "Shaders" :
                    ( Global.BasicRenderer ?
                        "Legacy Simple" :
                        "Legacy" ) )
                + ( Global.bUseVBO ?
                    ", VBO" :
                    ", Display Lists" )
                + " ";
            if( false == Global.LastGLError.empty() ) {
                textline +=
                    "Last openGL error: "
                    + Global.LastGLError;
            }

            Output.emplace_back( textline, Global.UITextColor );

            // renderer stats
            Output.emplace_back( GfxRenderer->info_times(), Global.UITextColor );
            Output.emplace_back( GfxRenderer->info_stats(), Global.UITextColor );
}

bool
debug_panel::render_section( std::string const &Header, std::vector<text_line> const &Lines ) {

    if( true == Lines.empty() ) { return false; }
    if( false == ImGui::CollapsingHeader( Header.c_str() ) ) { return false; }

    return render_section( Lines );
}

bool
debug_panel::render_section( std::vector<text_line> const &Lines ) {

    for( auto const &line : Lines ) {
        ImGui::PushStyleColor( ImGuiCol_Text, { line.color.r, line.color.g, line.color.b, line.color.a } );
        ImGui::TextUnformatted( line.data.c_str() );
        ImGui::PopStyleColor();
//        ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
    }
    return true;
}

bool
debug_panel::render_section_settings() {

    if( false == ImGui::CollapsingHeader( "Settings" ) ) { return false; }

    ImGui::PushStyleColor( ImGuiCol_Text, { Global.UITextColor.r, Global.UITextColor.g, Global.UITextColor.b, Global.UITextColor.a } );
    ImGui::TextUnformatted( "Graphics" );
    ImGui::PopStyleColor();
    // reflection fidelity
    ImGui::SliderInt( ( to_string( Global.reflectiontune.fidelity ) + "###reflectionfidelity" ).c_str(), &Global.reflectiontune.fidelity, 0, 2, "Reflection fidelity" );
    ImGui::SliderInt( ( to_string( Global.gfx_shadow_rank_cutoff ) + "###shadowrankcutoff" ).c_str(), &Global.gfx_shadow_rank_cutoff, 1, 3, "Shadow ranks" );
    if( ImGui::SliderFloat( ( to_string( std::abs( Global.gfx_shadow_angle_min ), 2 ) + "###shadowanglecutoff" ).c_str(), &Global.gfx_shadow_angle_min, -1.0, -0.2, "Shadow angle cutoff" ) ) {
        Global.gfx_shadow_angle_min = quantize( Global.gfx_shadow_angle_min, 0.05f );
    };
    if( DebugModeFlag ) {
        // sky sliders
        {
            ImGui::SliderFloat(
                ( to_string( Global.m_skysaturationcorrection, 2, 5 ) + "###skysaturation" ).c_str(), &Global.m_skysaturationcorrection, 0.0f, 3.0f, "Sky saturation" );
        }
        {
            ImGui::SliderFloat(
                ( to_string( Global.m_skyhuecorrection, 2, 5 ) + "###skyhue" ).c_str(), &Global.m_skyhuecorrection, 0.0f, 1.0f, "Sky hue correction" );
        }
    }

    ImGui::PushStyleColor( ImGuiCol_Text, { Global.UITextColor.r, Global.UITextColor.g, Global.UITextColor.b, Global.UITextColor.a } );
    ImGui::TextUnformatted( "Sound" );
    ImGui::PopStyleColor();
    // audio volume sliders
    ImGui::SliderFloat( ( to_string( static_cast<int>( Global.AudioVolume * 100 ) ) + "%###volumemain" ).c_str(), &Global.AudioVolume, 0.0f, 2.0f, "Main audio volume" );
    if( ImGui::SliderFloat( ( to_string( static_cast<int>( Global.VehicleVolume * 100 ) ) + "%###volumevehicle" ).c_str(), &Global.VehicleVolume, 0.0f, 1.0f, "Vehicle sounds" ) ) {
        audio::event_volume_change = true;
    }
    if( ImGui::SliderFloat( ( to_string( static_cast<int>( Global.EnvironmentPositionalVolume * 100 ) ) + "%###volumepositional" ).c_str(), &Global.EnvironmentPositionalVolume, 0.0f, 1.0f, "Positional sounds" ) ) {
        audio::event_volume_change = true;
    }
    if( ImGui::SliderFloat( ( to_string( static_cast<int>( Global.EnvironmentAmbientVolume * 100 ) ) + "%###volumeambient" ).c_str(), &Global.EnvironmentAmbientVolume, 0.0f, 1.0f, "Ambient sounds" ) ) {
        audio::event_volume_change = true;
    }

    return true;
}

void
transcripts_panel::update() {

    if( false == is_open ) { return; }

    text_lines.clear();

    for( auto const &transcript : ui::Transcripts.aLines ) {
        if( Global.fTimeAngleDeg + ( transcript.fShow - Global.fTimeAngleDeg > 180 ? 360 : 0 ) < transcript.fShow ) { continue; }
        text_lines.emplace_back( ExchangeCharInString( transcript.asText, '|', ' ' ), colors::white );
    }
}

void
transcripts_panel::render() {

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
    auto const panelname { (
        title.empty() ?
            name :
            title )
        + "###" + name };
    if( true == ImGui::Begin( panelname.c_str(), &is_open, flags ) ) {
        // header section
        for( auto const &line : text_lines ) {
            ImGui::TextWrapped( "%s", line.data.c_str() );
        }
    }
    ImGui::End();
}
