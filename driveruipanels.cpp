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
#include "translation.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Timer.h"
#include "Event.h"
#include "Camera.h"
#include "mtable.h"
#include "Train.h"
#include "Driver.h"
#include "AnimModel.h"
#include "DynObj.h"
#include "Model3d.h"
#include "renderer.h"
#include "utilities.h"
#include "Logs.h"

#undef snprintf // defined by train.h->pyint.h->python

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

    { // throttle, velocity, speed limits and grade
        std::string expandedtext;
        if( is_expanded ) {
            // grade
            std::string gradetext;
            auto const reverser { ( mover->ActiveDir > 0 ? 1 : -1 ) };
            auto const grade { controlled->VectorFront().y * 100 * ( controlled->DirectionGet() == reverser ? 1 : -1 ) * reverser };
            if( std::abs( grade ) >= 0.25 ) {
                std::snprintf(
                    m_buffer.data(), m_buffer.size(),
                    locale::strings[ locale::string::driver_aid_grade ].c_str(),
                    grade );
                gradetext = m_buffer.data();
            }
            // next speed limit
            auto const speedlimit { static_cast<int>( std::floor( driver->VelDesired ) ) };
            auto const nextspeedlimit { static_cast<int>( std::floor( driver->VelNext ) ) };
            std::string nextspeedlimittext;
            if( nextspeedlimit != speedlimit ) {
                std::snprintf(
                    m_buffer.data(), m_buffer.size(),
                    locale::strings[ locale::string::driver_aid_nextlimit ].c_str(),
                    nextspeedlimit,
                    driver->ActualProximityDist * 0.001 );
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
            driver->Controlling()->MainCtrlPos,
            driver->Controlling()->ScndCtrlPos,
            ( mover->ActiveDir > 0 ? 'D' : mover->ActiveDir < 0 ? 'R' : 'N' ),
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
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::driver_aid_brakes ].c_str(),
            mover->fBrakeCtrlPos,
            mover->LocalBrakePosA * LocalBrakePosNo,
            ( mover->SlippingWheels ? '!' : ' ' ),
            expandedtext.c_str() );

        text_lines.emplace_back( m_buffer.data(), Global.UITextColor );
    }

    { // alerter, hints
        std::string expandedtext;
        if( is_expanded ) {
            auto const stoptime { static_cast<int>( std::ceil( -1.0 * controlled->Mechanik->fStopTime ) ) };
            if( stoptime > 0 ) {
                std::snprintf(
                    m_buffer.data(), m_buffer.size(),
                    locale::strings[ locale::string::driver_aid_loadinginprogress ].c_str(),
                    stoptime );
                expandedtext = m_buffer.data();
            }
            else {
                auto const trackblockdistance{ std::abs( controlled->Mechanik->TrackBlock() ) };
                if( trackblockdistance <= 75.0 ) {
                    std::snprintf(
                        m_buffer.data(), m_buffer.size(),
                        locale::strings[ locale::string::driver_aid_vehicleahead ].c_str(),
                        trackblockdistance );
                    expandedtext = m_buffer.data();
                }
            }
        }
        std::string textline =
            ( true == TestFlag( mover->SecuritySystem.Status, s_aware ) ?
                locale::strings[ locale::string::driver_aid_alerter ] :
                "          " );
        textline +=
            ( true == TestFlag( mover->SecuritySystem.Status, s_active ) ?
                locale::strings[ locale::string::driver_aid_shp ] :
                "     " );

        text_lines.emplace_back( textline + "  " + expandedtext, Global.UITextColor );
    }
}

void
timetable_panel::update() {

    if( false == is_open ) { return; }

    text_lines.clear();

    auto const *train { simulation::Train };
    auto const *controlled { ( train ? train->Dynamic() : nullptr ) };
    auto const &camera { Global.pCamera };
    auto const &time { simulation::Time.data() };

    { // current time
        std::snprintf(
            m_buffer.data(), m_buffer.size(),
            locale::strings[ locale::string::driver_timetable_time ].c_str(),
            time.wHour,
            time.wMinute,
            time.wSecond );

        text_lines.emplace_back( m_buffer.data(), Global.UITextColor );
    }

    auto *vehicle { (
        false == FreeFlyModeFlag ? controlled :
        camera.m_owner != nullptr ? camera.m_owner :
        std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) ) }; // w trybie latania lokalizujemy wg mapy

    if( vehicle == nullptr ) { return; }
    // if the nearest located vehicle doesn't have a direct driver, try to query its owner
    auto const *owner = (
        ( ( vehicle->Mechanik != nullptr ) && ( vehicle->Mechanik->Primary() ) ) ?
            vehicle->Mechanik :
            vehicle->ctOwner );
    if( owner == nullptr ) { return; }

    auto const *table = owner->TrainTimetable();
    if( table == nullptr ) { return; }

    { // destination
        auto textline = Bezogonkow( owner->Relation(), true );
        if( false == textline.empty() ) {
            textline += " (" + Bezogonkow( owner->TrainName(), true ) + ")";
        }

        text_lines.emplace_back( textline, Global.UITextColor );
    }

    { // next station
        auto const nextstation = Bezogonkow( owner->NextStop(), true );
        if( false == nextstation.empty() ) {
            // jeśli jest podana relacja, to dodajemy punkt następnego zatrzymania
            auto textline = " -> " + nextstation;

            text_lines.emplace_back( textline, Global.UITextColor );
        }
    }

    if( is_expanded ) {

        text_lines.emplace_back( "", Global.UITextColor );

        if( 0 == table->StationCount ) {
            // only bother if there's stations to list
            text_lines.emplace_back( locale::strings[ locale::string::driver_timetable_notimetable ], Global.UITextColor );
        } 
        else {
            auto const readycolor { glm::vec4( 84.0f / 255.0f, 164.0f / 255.0f, 132.0f / 255.0f, 1.f ) };
            // header
            text_lines.emplace_back( "+-----+------------------------------------+-------+-----+", Global.UITextColor );

            TMTableLine const *tableline;
            for( int i = owner->iStationStart; i <= table->StationCount; ++i ) {
                // wyświetlenie pozycji z rozkładu
                tableline = table->TimeTable + i; // linijka rozkładu

                std::string vmax =
                    "   "
                    + to_string( tableline->vmax, 0 );
                vmax = vmax.substr( vmax.size() - 3, 3 ); // z wyrównaniem do prawej
                std::string const station = (
                    Bezogonkow( tableline->StationName, true )
                    + "                                  " )
                    .substr( 0, 34 );
                std::string const location = (
                    ( tableline->km > 0.0 ?
                        to_string( tableline->km, 2 ) :
                        "" )
                    + "                                  " )
                    .substr( 0, 34 - tableline->StationWare.size() );
                std::string const arrival = (
                    tableline->Ah >= 0 ?
                        to_string( int( 100 + tableline->Ah ) ).substr( 1, 2 ) + ":" + to_string( int( 100 + tableline->Am ) ).substr( 1, 2 ) :
                        "  |  " );
                std::string const departure = (
                    tableline->Dh >= 0 ?
                        to_string( int( 100 + tableline->Dh ) ).substr( 1, 2 ) + ":" + to_string( int( 100 + tableline->Dm ) ).substr( 1, 2 ) :
                        "  |  " );
                auto const candeparture = (
                       ( owner->iStationStart < table->StationIndex )
                    && ( i < table->StationIndex )
                    && ( ( tableline->Ah < 0 ) // pass-through, always valid
                      || ( time.wHour * 60 + time.wMinute >= tableline->Dh * 60 + tableline->Dm ) ) );
                auto traveltime =
                    "   "
                    + ( i < 2 ? "" :
                        tableline->Ah >= 0 ? to_string( CompareTime( table->TimeTable[ i - 1 ].Dh, table->TimeTable[ i - 1 ].Dm, tableline->Ah, tableline->Am ), 0 ) :
                        to_string( std::max( 0.0, CompareTime( table->TimeTable[ i - 1 ].Dh, table->TimeTable[ i - 1 ].Dm, tableline->Dh, tableline->Dm ) - 0.5 ), 0 ) );
                traveltime = traveltime.substr( traveltime.size() - 3, 3 ); // z wyrównaniem do prawej

                text_lines.emplace_back(
                    ( "| " + vmax + " | " + station + " | " + arrival + " | " + traveltime + " |" ),
                        ( candeparture ?
                        readycolor :// czas minął i odjazd był, to nazwa stacji będzie na zielono
                        Global.UITextColor ) );
                text_lines.emplace_back(
                    ( "|     | " + location + tableline->StationWare + " | " + departure + " |     |" ),
                        ( candeparture ?
                        readycolor :// czas minął i odjazd był, to nazwa stacji będzie na zielono
                        Global.UITextColor ) );
                // divider/footer
                text_lines.emplace_back( "+-----+------------------------------------+-------+-----+", Global.UITextColor );
            }
        }
    } // is_expanded
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
    m_cameralines.clear();
    m_rendererlines.clear();

    update_section_vehicle( m_vehiclelines );
    update_section_engine( m_enginelines );
    update_section_ai( m_ailines );
    update_section_scantable( m_scantablelines );
    update_section_scenario( m_scenariolines );
    update_section_eventqueue( m_eventqueuelines );
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
    if( true == ImGui::Begin( name.c_str(), &is_open, flags ) ) {
        // header section
        for( auto const &line : text_lines ) {
            ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
        }
        // sections
        ImGui::Separator();
        render_section( "Vehicle", m_vehiclelines );
        render_section( "Vehicle Engine", m_enginelines );
        render_section( "Vehicle AI", m_ailines );
        render_section( "Vehicle Scan Table", m_scantablelines );
        render_section( "Scenario", m_scenariolines );
        if( true == render_section( "Scenario Event Queue", m_eventqueuelines ) ) {
            // event queue filter
            ImGui::Checkbox( "By This Vehicle Only", &m_eventqueueactivevehicleonly );
        }
        render_section( "Camera", m_cameralines );
        render_section( "Gfx Renderer", m_rendererlines );
        // toggles
        ImGui::Separator();
        ImGui::Checkbox( "Debug Mode", &DebugModeFlag );
        if( DebugModeFlag )
        {
            ImGui::Indent();
            ImGui::Checkbox(
                    "Draw normal traction",
                    &GfxRenderer.settings.force_normal_traction_render );
            ImGui::Unindent();
        }
    }
    ImGui::End();
}

void
debug_panel::update_section_vehicle( std::vector<text_line> &Output ) {

    if( m_input.vehicle == nullptr ) { return; }
    if( m_input.mover == nullptr ) { return; }

    auto const &vehicle { *m_input.vehicle };
    auto const &mover { *m_input.mover };

    auto const isowned { ( vehicle.Mechanik == nullptr ) && ( vehicle.ctOwner != nullptr ) };
    auto const isplayervehicle { ( m_input.train != nullptr ) && ( m_input.train->Dynamic() == m_input.vehicle ) };
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
        update_vehicle_coupler( side::front ).c_str(),
        update_vehicle_coupler( side::rear ).c_str() );

    Output.emplace_back( std::string{ m_buffer.data() }, Global.UITextColor );

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        locale::strings[ locale::string::debug_vehicle_devicespower ].c_str(),
        // devices
        ( mover.Battery ? 'B' : '.' ),
        ( mover.Mains ? 'M' : '.' ),
        ( mover.FuseFlag ? '!' : '.' ),
        ( mover.PantRearUp ? ( mover.PantRearVolt > 0.0 ? 'O' : 'o' ) : '.' ),
        ( mover.PantFrontUp ? ( mover.PantFrontVolt > 0.0 ? 'P' : 'p' ) : '.' ),
        ( mover.PantPressLockActive ? '!' : ( mover.PantPressSwitchActive ? '*' : '.' ) ),
        ( mover.WaterPump.is_active ? 'W' : ( false == mover.WaterPump.breaker ? '-' : ( mover.WaterPump.is_enabled ? 'w' : '.' ) ) ),
        ( true == mover.WaterHeater.is_damaged ? '!' : ( mover.WaterHeater.is_active ? 'H' : ( false == mover.WaterHeater.breaker ? '-' : ( mover.WaterHeater.is_enabled ? 'h' : '.' ) ) ) ),
        ( mover.FuelPump.is_active ? 'F' : ( mover.FuelPump.is_enabled ? 'f' : '.' ) ),
        ( mover.OilPump.is_active ? 'O' : ( mover.OilPump.is_enabled ? 'o' : '.' ) ),
        ( false == mover.ConverterAllowLocal ? '-' : ( mover.ConverterAllow ? ( mover.ConverterFlag ? 'X' : 'x' ) : '.' ) ),
        ( mover.ConvOvldFlag ? '!' : '.' ),
        ( mover.CompressorFlag ? 'C' : ( false == mover.CompressorAllowLocal ? '-' : ( ( mover.CompressorAllow || mover.CompressorStart == start_t::automatic ) ? 'c' : '.' ) ) ),
        ( mover.CompressorGovernorLock ? '!' : '.' ),
        std::string( isplayervehicle ? locale::strings[ locale::string::debug_vehicle_radio ] + ( mover.Radio ? std::to_string( m_input.train->RadioChannel() ) : "-" ) : "" ).c_str(),
        std::string( isdieselenginepowered ? locale::strings[ locale::string::debug_vehicle_oilpressure ] + to_string( mover.OilPump.pressure, 2 )  : "" ).c_str(),
        // power transfers
        mover.Couplers[ side::front ].power_high.voltage,
        mover.Couplers[ side::front ].power_high.current,
        std::string( mover.Couplers[ side::front ].power_high.local ? "" : "-" ).c_str(),
        std::string( vehicle.DirectionGet() ? ":<<:" : ":>>:" ).c_str(),
        std::string( mover.Couplers[ side::rear ].power_high.local ? "" : "-" ).c_str(),
        mover.Couplers[ side::rear ].power_high.voltage,
        mover.Couplers[ side::rear ].power_high.current );

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
        std::abs( mover.MotorBlowers[side::front].revolutions ),
        std::abs( mover.MotorBlowers[side::rear].revolutions ),
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
        mover.LocalBrakePosA,
        mover.BrakeOpModeFlag,
        update_vehicle_brake().c_str(),
        mover.LoadFlag,
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
        mover.Ft * 0.001f * ( mover.ActiveCab ? mover.ActiveCab : vehicle.ctOwner ? vehicle.ctOwner->Controlling()->ActiveCab : 1 ) + 0.001f,
        mover.Fb * 0.001f,
        mover.Adhesive( mover.RunningTrack.friction ),
        ( mover.SlippingWheels ? " (!)" : "" ),
        // acceleration
        Acc,
        mover.AccN + 0.001f,
        std::string( std::abs( mover.RunningShape.R ) > 10000.0 ? "~0" : to_string( mover.RunningShape.R, 0 ) ).c_str(),
        // velocity
        vehicle.GetVelocity(),
        mover.DistCounter,
        // position
        vehicle.GetPosition().x,
        vehicle.GetPosition().y,
        vehicle.GetPosition().z );

    Output.emplace_back( m_buffer.data(), Global.UITextColor );

}

std::string
debug_panel::update_vehicle_coupler( int const Side ) {
    // NOTE: mover and vehicle are guaranteed to be valid by the caller
    std::string couplerstatus { locale::strings[ locale::string::debug_vehicle_none ] };

    auto const *connected { (
        Side == side::front ?
            m_input.vehicle->PrevConnected :
            m_input.vehicle->NextConnected ) };

    if( connected == nullptr ) { return couplerstatus; }

    auto const &mover { *( m_input.mover ) };

    std::snprintf(
        m_buffer.data(), m_buffer.size(),
        "%s [%d]%s",
        connected->name().c_str(),
        mover.Couplers[ Side ].CouplingFlag,
        std::string( mover.Couplers[ Side ].CouplingFlag == 0 ? " (" + to_string( mover.Couplers[ Side ].CoupleDist, 1 ) + " m)" : "" ).c_str() );

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

    if( m_input.train == nullptr ) { return; }
    if( m_input.vehicle == nullptr ) { return; }
    if( m_input.mover == nullptr ) { return; }

    auto const &train { *m_input.train };
    auto const &vehicle{ *m_input.vehicle };
    auto const &mover{ *m_input.mover };

        // engine data
                // induction motor data
    if( mover.EngineType == TEngineType::ElectricInductionMotor ) {

        Output.emplace_back( "      eimc:            eimv:            press:", Global.UITextColor );
        for( int i = 0; i <= 20; ++i ) {

            std::string parameters =
                mover.eimc_labels[ i ] + to_string( mover.eimc[ i ], 2, 9 )
                + " | "
                + mover.eimv_labels[ i ] + to_string( mover.eimv[ i ], 2, 9 );

            if( i < 10 ) {
                parameters += " | " + train.fPress_labels[ i ] + to_string( train.fPress[ i ][ 0 ], 2, 9 );
            }
            else if( i == 12 ) {
                parameters += "        med:";
            }
            else if( i >= 13 ) {
                parameters += " | " + vehicle.MED_labels[ i - 13 ] + to_string( vehicle.MED[ 0 ][ i - 13 ], 2, 9 );
            }

            Output.emplace_back( parameters, Global.UITextColor );
        }
    }
    if( mover.EngineType == TEngineType::DieselEngine ) {

        std::string parameterstext = "param       value";
        std::vector< std::pair <std::string, double> > const paramvalues {
            { "efill: ", mover.dizel_fill },
            { "etorq: ", mover.dizel_Torque },
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
            { "hTCLR: ", mover.hydro_TC_LockupRate } };
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
    auto textline = "Current order: " + mechanik.OrderCurrent();

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

    Output.emplace_back( textline, Global.UITextColor );

    // velocity factors
    textline =
        "Velocity:\n desired: " + to_string( mechanik.VelDesired, 0 )
        + ", next: " + to_string( mechanik.VelNext, 0 );

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
        + "\n current: " + to_string( mechanik.AbsAccS_pub + 0.001f, 2 )
        + ", slope: " + to_string( mechanik.fAccGravity + 0.001f, 2 ) + " (" + ( mechanik.fAccGravity > 0.01 ? "\\" : ( mechanik.fAccGravity < -0.01 ? "/" : "-" ) ) + ")"
        + "\n brake threshold: " + to_string( mechanik.fAccThreshold, 2 )
        + ", delays: " + to_string( mechanik.fBrake_a0[ 0 ], 2 )
        + "+" + to_string( mechanik.fBrake_a1[ 0 ], 2 );

    Output.emplace_back( textline, Global.UITextColor );

    // brakes
    textline =
        "Brakes:\n consist: " + to_string( mechanik.fReady, 2 ) + " or less";

    Output.emplace_back( textline, Global.UITextColor );

    // ai driving flags
    std::vector<std::string> const drivingflagnames {
        "StopCloser", "StopPoint", "Active", "Press", "Connect", "Primary", "Late", "StopHere",
        "StartHorn", "StartHornNow", "StartHornDone", "Oerlikons", "IncSpeed", "TrackEnd", "SwitchFound", "GuardSignal",
        "Visibility", "DoorOpened", "PushPull", "SemaphorFound", "StopPointFound" /*"SemaphorWasElapsed", "TrainInsideStation", "SpeedLimitFound"*/ };

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
    textline = "Cloud cover: " + to_string( Global.Overcast, 3 );
    textline += "\nLight level: " + to_string( Global.fLuminance, 3 );
    if( Global.FakeLight ) { textline += "(*)"; }
    textline += "\nAir temperature: " + to_string( Global.AirTemperature, 1 ) + " deg C";

    Output.emplace_back( textline, Global.UITextColor );
}

void
debug_panel::update_section_eventqueue( std::vector<text_line> &Output ) {

    std::string textline;

    // current event queue
    auto const time { Timer::GetTime() };
    auto const *event { simulation::Events.begin() };

    Output.emplace_back( "Delay:   Event:", Global.UITextColor );

    while( ( event != nullptr )
        && ( Output.size() < 30 ) ) {

        if( ( false == event->m_ignored )
         && ( false == event->m_passive )
         && ( ( false == m_eventqueueactivevehicleonly )
           || ( event->m_activator == m_input.vehicle ) ) ) {

            auto const delay { "   " + to_string( std::max( 0.0, event->m_launchtime - time ), 1 ) };
            textline = delay.substr( delay.length() - 6 )
                + "   " + event->m_name
                + ( event->m_activator ? " (by: " + event->m_activator->asName + ")" : "" )
                + ( event->m_sibling ? " (joint event)" : "" );

            Output.emplace_back( textline, Global.UITextColor );
        }
        event = event->m_next;
    }
    if( Output.size() == 1 ) {
        Output.front().data = "(no queued events)";
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
                + ", Draw range x " + to_string( Global.fDistanceFactor, 1 )
//                + "; sectors: " + std::to_string( GfxRenderer.m_drawcount )
//                + ", FPS: " + to_string( Timer::GetFPS(), 2 );
                + ", FPS: " + std::to_string( static_cast<int>(std::round(GfxRenderer.Framerate())) );
            if( Global.iSlowMotion ) {
                textline += " (slowmotion " + to_string( Global.iSlowMotion ) + ")";
            }

            Output.emplace_back( textline, Global.UITextColor );

            textline = "";
            if( false == Global.LastGLError.empty() ) {
                textline +=
                    "Last openGL error: "
                    + Global.LastGLError;
            }

            Output.emplace_back( textline, Global.UITextColor );

            // renderer stats
            Output.emplace_back( GfxRenderer.info_times(), Global.UITextColor );
            Output.emplace_back( GfxRenderer.info_stats(), Global.UITextColor );
}

bool
debug_panel::render_section( std::string const &Header, std::vector<text_line> const &Lines ) {

    if( true == Lines.empty() ) { return false; }
    if( false == ImGui::CollapsingHeader( Header.c_str() ) ) { return false; }

    for( auto const &line : Lines ) {
        ImGui::PushStyleColor( ImGuiCol_Text, { line.color.r, line.color.g, line.color.b, line.color.a } );
        ImGui::TextUnformatted( line.data.c_str() );
        ImGui::PopStyleColor();
//        ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
    }
    return true;
}

void
transcripts_panel::update() {

    if( false == is_open ) { return; }

    text_lines.clear();

    for( auto const &transcript : ui::Transcripts.aLines ) {
        if( Global.fTimeAngleDeg >= transcript.fShow ) {
            // NOTE: legacy transcript lines use | as new line mark
            text_lines.emplace_back( ExchangeCharInString( transcript.asText, '|', ' ' ), colors::white );
        }
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
            ImGui::TextWrapped( line.data.c_str() );
        }
    }
    ImGui::End();
}
