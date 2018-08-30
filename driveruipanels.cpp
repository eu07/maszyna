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

#include "globals.h"
#include "translation.h"
#include "simulation.h"
#include "simulationtime.h"
#include "event.h"
#include "camera.h"
#include "mtable.h"
#include "train.h"
#include "driver.h"
#include "animmodel.h"
#include "dynobj.h"
#include "model3d.h"
#include "renderer.h"
#include "utilities.h"
#include "logs.h"

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
        auto textline =
            "Throttle: " + to_string( driver->Controlling()->MainCtrlPos, 0, 2 )
            + "+" + std::to_string( driver->Controlling()->ScndCtrlPos )
            + " "
            + ( mover->ActiveDir > 0 ? 'D' :
                mover->ActiveDir < 0 ? 'R' :
                                       'N' );

        if( is_expanded ) {

            auto const speedlimit { static_cast<int>( std::floor( driver->VelDesired ) ) };
            textline +=
                " Speed: " + std::to_string( static_cast<int>( std::floor( mover->Vel ) ) ) + " km/h"
                + " (limit: " + std::to_string( speedlimit ) + " km/h";
            auto const nextspeedlimit { static_cast<int>( std::floor( driver->VelNext ) ) };
            if( nextspeedlimit != speedlimit ) {
                textline +=
                    ", new limit: " + std::to_string( nextspeedlimit ) + " km/h"
                    + " in " + to_string( driver->ActualProximityDist * 0.001, 1 ) + " km";
            }
            textline += ")";
            auto const reverser { ( mover->ActiveDir > 0 ? 1 : -1 ) };
            auto const grade { controlled->VectorFront().y * 100 * ( controlled->DirectionGet() == reverser ? 1 : -1 ) * reverser };
            if( std::abs( grade ) >= 0.25 ) {
                textline += " Grade: " + to_string( grade, 1 ) + "%%";
            }
        }

        text_lines.emplace_back( textline, Global.UITextColor );
    }

    { // brakes, air pressure
        auto textline =
            "Brakes:" + to_string( mover->fBrakeCtrlPos, 1, 5 )
            + "+" + to_string( mover->LocalBrakePosA * LocalBrakePosNo, 0 )
            + ( mover->SlippingWheels ? " !" : "  " );
        if( textline.size() > 16 ) {
            textline.erase( 16 );
        }

        if( is_expanded ) {

            textline +=
                " Pressure: " + to_string( mover->BrakePress * 100.0, 2 ) + " kPa"
                + " (train pipe: " + to_string( mover->PipePress * 100.0, 2 ) + " kPa)";
        }

        text_lines.emplace_back( textline, Global.UITextColor );
    }

    { // alerter, hints
        std::string textline =
            ( true == TestFlag( mover->SecuritySystem.Status, s_aware ) ?
                "!ALERTER! " :
                "          " );
        textline +=
            ( true == TestFlag( mover->SecuritySystem.Status, s_active ) ?
                "!SHP! " :
                "      " );

        if( is_expanded ) {

            auto const stoptime { static_cast<int>( -1.0 * controlled->Mechanik->fStopTime ) };
            if( stoptime > 0 ) {
                textline += " Loading/unloading in progress (" + to_string( stoptime ) + ( stoptime > 1 ? " seconds" : " second" ) + " left)";
            }
            else {
                auto const trackblockdistance { std::abs( controlled->Mechanik->TrackBlock() ) };
                if( trackblockdistance <= 75.0 ) {
                    textline += " Another vehicle ahead (distance: " + to_string( trackblockdistance, 1 ) + " m)";
                }
            }
        }

        text_lines.emplace_back( textline, Global.UITextColor );
    }

    auto const sizex = ( is_expanded ? 660 : 130 );
    size = { sizex, 85 };
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
        auto textline =
            "Time: "
            + to_string( time.wHour ) + ":"
            + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
            + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );

        text_lines.emplace_back( textline, Global.UITextColor );
    }

    auto *vehicle {
        ( FreeFlyModeFlag ?
            std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) :
            controlled ) }; // w trybie latania lokalizujemy wg mapy

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
            text_lines.emplace_back( "(no timetable)", Global.UITextColor );
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
                    && ( ( time.wHour * 60 + time.wMinute ) >= ( tableline->Dh * 60 + tableline->Dm ) ) );
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
    m_input.vehicle =
        ( FreeFlyModeFlag ?
            std::get<TDynamicObject *>( simulation::Region->find_vehicle( m_input.camera->Pos, 20, false, false ) ) :
            m_input.controlled ); // w trybie latania lokalizujemy wg mapy
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
        render_section( "Scenario Event Queue", m_eventqueuelines );
        render_section( "Camera", m_cameralines );
        render_section( "Gfx Renderer", m_rendererlines );
        // toggles
        ImGui::Separator();
        ImGui::Checkbox( "Debug Mode", &DebugModeFlag );
    }
    ImGui::End();
}

void
debug_panel::update_section_vehicle( std::vector<text_line> &Output ) {

    if( m_input.vehicle == nullptr ) { return; }
    if( m_input.mover == nullptr ) { return; }

    auto const &vehicle { *m_input.vehicle };
    auto const &mover { *m_input.mover };

        // f3 data
        auto textline = "Vehicle name: " + mover.Name;

        if( ( vehicle.Mechanik == nullptr ) && ( vehicle.ctOwner ) ) {
            // for cars other than leading unit indicate the leader
            textline += ", owned by " + vehicle.ctOwner->OwnerName();
        }
        textline += "\nStatus: " + mover.EngineDescription( 0 );
        if( mover.WheelFlat > 0.01 ) {
            textline += " Flat: " + to_string( mover.WheelFlat, 1 ) + " mm";
        }

        // informacja o sprzęgach
        textline +=
            "\nC0:" +
            ( vehicle.PrevConnected ?
                vehicle.PrevConnected->name() + ":" + to_string( mover.Couplers[ 0 ].CouplingFlag ) + (
                    mover.Couplers[ 0 ].CouplingFlag == 0 ?
                    " (" + to_string( mover.Couplers[ 0 ].CoupleDist, 1 ) + " m)" :
                    "" ) :
                "none" );
        textline +=
            " C1:" +
            ( vehicle.NextConnected ?
                vehicle.NextConnected->name() + ":" + to_string( mover.Couplers[ 1 ].CouplingFlag ) + (
                    mover.Couplers[ 1 ].CouplingFlag == 0 ?
                    " (" + to_string( mover.Couplers[ 1 ].CoupleDist, 1 ) + " m)" :
                    "" ) :
                "none" );

        Output.emplace_back( textline, Global.UITextColor );

        // equipment flags
        textline = ( mover.Battery ? "B" : "." );
        textline += ( mover.Mains ? "M" : "." );
        textline += ( mover.PantRearUp ? ( mover.PantRearVolt > 0.0 ? "O" : "o" ) : "." );
        textline += ( mover.PantFrontUp ? ( mover.PantFrontVolt > 0.0 ? "P" : "p" ) : "." );
        textline += ( mover.PantPressLockActive ? "!" : ( mover.PantPressSwitchActive ? "*" : "." ) );
        textline += ( mover.WaterPump.is_active ? "W" : ( false == mover.WaterPump.breaker ? "-" : ( mover.WaterPump.is_enabled ? "w" : "." ) ) );
        textline += ( true == mover.WaterHeater.is_damaged ? "!" : ( mover.WaterHeater.is_active ? "H" : ( false == mover.WaterHeater.breaker ? "-" : ( mover.WaterHeater.is_enabled ? "h" : "." ) ) ) );
        textline += ( mover.FuelPump.is_active ? "F" : ( mover.FuelPump.is_enabled ? "f" : "." ) );
        textline += ( mover.OilPump.is_active ? "O" : ( mover.OilPump.is_enabled ? "o" : "." ) );
        textline += ( false == mover.ConverterAllowLocal ? "-" : ( mover.ConverterAllow ? ( mover.ConverterFlag ? "X" : "x" ) : "." ) );
        textline += ( mover.ConvOvldFlag ? "!" : "." );
        textline += ( mover.CompressorFlag ? "C" : ( false == mover.CompressorAllowLocal ? "-" : ( ( mover.CompressorAllow || mover.CompressorStart == start_t::automatic ) ? "c" : "." ) ) );
        textline += ( mover.CompressorGovernorLock ? "!" : "." );

        if( ( m_input.train != nullptr ) && ( m_input.train->Dynamic() == m_input.vehicle ) ) {
            textline += ( mover.Radio ? " R: " : " r: " ) + std::to_string( m_input.train->RadioChannel() );
        }
        textline += " Bdelay: ";
        if( ( mover.BrakeDelayFlag & bdelay_G ) == bdelay_G )
            textline += "G";
        if( ( mover.BrakeDelayFlag & bdelay_P ) == bdelay_P )
            textline += "P";
        if( ( mover.BrakeDelayFlag & bdelay_R ) == bdelay_R )
            textline += "R";
        if( ( mover.BrakeDelayFlag & bdelay_M ) == bdelay_M )
            textline += "+Mg";

        textline += ", Load: " + to_string( mover.Load, 0 ) + " (" + to_string( mover.LoadFlag, 0 ) + ")";

        textline +=
            "\nFt: " + to_string(
                mover.Ft * 0.001f * (
                    mover.ActiveCab ? mover.ActiveCab :
                    vehicle.ctOwner ? vehicle.ctOwner->Controlling()->ActiveCab :
                    1 ), 1 )
            + ", Fb: " + to_string( mover.Fb * 0.001f, 1 )
            + ", Fr: " + to_string( mover.Adhesive( mover.RunningTrack.friction ), 2 )
            + ( mover.SlippingWheels ? " (!)" : "" );

        textline +=
            "\nPant: "
            + to_string( mover.PantPress, 2 )
            + ( mover.bPantKurek3 ? "-ZG" : "|ZG" );
        textline +=
            " TC:"
            + to_string( mover.TotalCurrent, 0 );
        auto const frontcouplerhighvoltage =
            to_string( mover.Couplers[ side::front ].power_high.voltage, 0 )
            + "@"
            + to_string( mover.Couplers[ side::front ].power_high.current, 0 );
        auto const rearcouplerhighvoltage =
            to_string( mover.Couplers[ side::rear ].power_high.voltage, 0 )
            + "@"
            + to_string( mover.Couplers[ side::rear ].power_high.current, 0 );
        textline += ", HV: ";
        if( mover.Couplers[ side::front ].power_high.local == false ) {
            textline +=
                "(" + frontcouplerhighvoltage + ")-"
                + ":F" + ( vehicle.DirectionGet() ? "<<" : ">>" ) + "R:"
                + "-(" + rearcouplerhighvoltage + ")";
        }
        else {
            textline +=
                frontcouplerhighvoltage
                + ":F" + ( vehicle.DirectionGet() ? "<<" : ">>" ) + "R:"
                + rearcouplerhighvoltage;
        }

        Output.emplace_back( textline, Global.UITextColor );

        textline =
            "TrB: " + to_string( mover.BrakePress, 2 )
            + ", " + to_hex_str( mover.Hamulec->GetBrakeStatus(), 2 );

        textline +=
            "; LcB: " + to_string( mover.LocBrakePress, 2 )
            + "; hat: " + to_string( mover.BrakeCtrlPos2, 2 )
            + "\npipes: " + to_string( mover.PipePress, 2 )
            + "/" + to_string( mover.ScndPipePress, 2 )
            + "/" + to_string( mover.EqvtPipePress, 2 )
            + "\nMT: " + to_string( mover.CompressedVolume, 3 )
            + ", BT: " + to_string( mover.Volume, 3 )
            + ", CtlP: " + to_string( mover.CntrlPipePress, 3 )
            + ", CtlT: " + to_string( mover.Hamulec->GetCRP(), 3 );

        if( mover.ManualBrakePos > 0 ) {

            textline += "; manual brake on";
        }

        Output.emplace_back( textline, Global.UITextColor );

        // debug mode f1 data
        textline =
            "vel: " + to_string( vehicle.GetVelocity(), 2 ) + "/" + to_string( mover.nrot* M_PI * mover.WheelDiameter * 3.6, 2 )
            + " km/h;"
            + " dist: " + to_string( mover.DistCounter, 2 ) + " km"
            + "\npos: [" + to_string( vehicle.GetPosition().x, 2 ) + ", " + to_string( vehicle.GetPosition().y, 2 ) + ", " + to_string( vehicle.GetPosition().z, 2 ) + "]"
            + "\nenpwr=" + to_string( mover.EnginePower, 1 )
            + "; enrot=" + to_string( mover.enrot * 60, 0 )
            + " tmrot=" + to_string( std::abs( mover.nrot ) * mover.Transmision.Ratio * 60, 0 )
            + "; ventrot=" + to_string( mover.RventRot * 60, 1 )
            + "; fanrot=" + to_string( mover.dizel_heat.rpmw, 1 ) + ", " + to_string( mover.dizel_heat.rpmw2, 1 );

        Output.emplace_back( textline, Global.UITextColor );

        textline =
            "HamZ=" + to_string( mover.fBrakeCtrlPos, 2 )
            + "; HamP=" + to_string( mover.LocalBrakePosA, 2 )
            + "; NasJ=" + std::to_string( mover.MainCtrlPos ) + "(" + std::to_string( mover.MainCtrlActualPos ) + ")"
            + ( ( mover.ShuntMode && mover.EngineType == TEngineType::DieselElectric ) ?
                "; NasB=" + to_string( mover.AnPos, 2 ) :
                "; NasB=" + std::to_string( mover.ScndCtrlPos ) + "(" + std::to_string( mover.ScndCtrlActualPos ) + ")" )
            + "\nI=" +
            ( mover.TrainType == dt_EZT ?
                std::to_string( int( mover.ShowCurrent( 0 ) ) ) :
                std::to_string( int( mover.Im ) ) )
            + "; U=" + to_string( int( mover.RunningTraction.TractionVoltage + 0.5 ) )
            + "; R=" +
            ( std::abs( mover.RunningShape.R ) > 10000.0 ?
                "~0.0" :
                to_string( mover.RunningShape.R, 1 ) )
            + " An=" + to_string( mover.AccN, 2 ); // przyspieszenie poprzeczne

        if( tprev != simulation::Time.data().wSecond ) {
            tprev = simulation::Time.data().wSecond;
            Acc = ( mover.Vel - VelPrev ) / 3.6;
            VelPrev = mover.Vel;
        }
        textline += "; As=" + to_string( Acc, 2 ); // przyspieszenie wzdłużne
                                                   //                uitextline2 += " eAngle=" + to_string( std::cos( mover.eAngle ), 2 );
        textline += "\noilP=" + to_string( mover.OilPump.pressure_present, 3 );
        textline += " oilT=" + to_string( mover.dizel_heat.To, 2 );
        textline += "; waterT=" + to_string( mover.dizel_heat.temperatura1, 2 );
        textline += ( mover.WaterCircuitsLink ? "-" : "|" );
        textline += to_string( mover.dizel_heat.temperatura2, 2 );
        textline += "; engineT=" + to_string( mover.dizel_heat.Ts, 2 );

        Output.emplace_back( textline, Global.UITextColor );

        textline =
            "cyl.ham. " + to_string( mover.BrakePress, 2 )
            + "; prz.gl. " + to_string( mover.PipePress, 2 )
            + "; zb.gl. " + to_string( mover.CompressedVolume, 2 )
            // youBy - drugi wezyk
            + "; p.zas. " + to_string( mover.ScndPipePress, 2 );

        // McZapkie: warto wiedziec w jakim stanie sa przelaczniki
        if( mover.ConvOvldFlag )
            textline += " C! ";
        else if( mover.FuseFlag )
            textline += " F! ";
        else if( !mover.Mains )
            textline += " () ";
        else {
            switch(
                mover.ActiveDir *
                ( mover.Imin == mover.IminLo ?
                    1 :
                    2 ) ) {
                case  2: { textline += " >> "; break; }
                case  1: { textline += " -> "; break; }
                case  0: { textline += " -- "; break; }
                case -1: { textline += " <- "; break; }
                case -2: { textline += " << "; break; }
            }
        }

        Output.emplace_back( textline, Global.UITextColor );

        // McZapkie: komenda i jej parametry
        if( mover.CommandIn.Command != ( "" ) ) {
            textline =
                "C:" + mover.CommandIn.Command
                + " V1=" + to_string( mover.CommandIn.Value1, 0 )
                + " V2=" + to_string( mover.CommandIn.Value2, 0 );

            Output.emplace_back( textline, Global.UITextColor );
        }
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
                            parameters += " | " + train.fPress_labels[i] + to_string( train.fPress[ i ][ 0 ], 2, 9 );
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
				if ( mover.EngineType == TEngineType::DieselEngine) {
					std::string parameters = "param       value";
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "efill: " + to_string( mover.dizel_fill, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "etorq: " + to_string( mover.dizel_Torque, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "creal: " + to_string( mover.dizel_engage, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "cdesi: " + to_string( mover.dizel_engagestate, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "cdelt: " + to_string( mover.dizel_engagedeltaomega, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "gears: " + to_string( mover.dizel_automaticgearstatus, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hydro      value";
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCnI: " + to_string( mover.hydro_TC_nIn, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCnO: " + to_string( mover.hydro_TC_nOut, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTM: " + to_string( mover.hydro_TC_TMRatio, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTI: " + to_string( mover.hydro_TC_TorqueIn, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTO: " + to_string( mover.hydro_TC_TorqueOut, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCfl: " + to_string( mover.hydro_TC_Fill, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCLR: " + to_string( mover.hydro_TC_LockupRate, 2, 9);
                    Output.emplace_back(parameters, Global.UITextColor);
					//parameters = "hTCXX: " + to_string(mover->hydro_TC_nIn, 2, 9);
					//engine_lines.emplace_back(parameters, Global.UITextColor);
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
        Output.emplace_back( "Current signal: " + Bezogonkow( mechanik.eSignNext->asName ), Global.UITextColor );
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
        + "\n current: " + to_string( mechanik.AbsAccS_pub, 2 )
        + ", slope: " + to_string( mechanik.fAccGravity, 2 ) + " (" + ( mechanik.fAccGravity > 0.01 ? "\\" : ( mechanik.fAccGravity < -0.01 ? "/" : "-" ) ) + ")"
        + "\n brake threshold: " + to_string( mechanik.fAccThreshold, 2 )
        + ", delays: " + to_string( mechanik.fBrake_a0[ 0 ], 2 )
        + "+" + to_string( mechanik.fBrake_a1[ 0 ], 2 );

    Output.emplace_back( textline, Global.UITextColor );

    // ai driving flags
    std::vector<std::string> const drivingflagnames {
        "StopCloser", "StopPoint", "Active", "Press", "Connect", "Primary", "Late", "StopHere",
        "StartHorn", "StartHornNow", "StartHornDone", "Oerlikons", "IncSpeed", "TrackEnd", "SwitchFound", "GuardSignal",
        "Visibility", "DoorOpened", "PushPull", "SemaphorFound", "SemaphorWasElapsed", "TrainInsideStation", "SpeedLimitFound" };

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

    auto const &mechanik{ *m_input.mechanik };

    std::size_t i = 0; std::size_t const speedtablesize = clamp( static_cast<int>( mechanik.TableSize() ) - 1, 0, 30 );
    do {
        auto const scanline = mechanik.TableText( i );
        if( scanline.empty() ) { break; }
        Output.emplace_back( Bezogonkow( scanline ), Global.UITextColor );
        ++i;
    } while( i < speedtablesize );
    if( Output.empty() ) {
        Output.emplace_back( "(no points of interest)", Global.UITextColor );
    }
}

void
debug_panel::update_section_scenario( std::vector<text_line> &Output ) {

    auto textline =
        "vehicles: " + to_string( Timer::subsystem.sim_dynamics.average(), 2 ) + " msec"
        + " update total: " + to_string( Timer::subsystem.sim_total.average(), 2 ) + " msec";

    Output.emplace_back( textline, Global.UITextColor );
    // current luminance level
    textline = "Light level: " + to_string( Global.fLuminance, 3 );
    if( Global.FakeLight ) { textline += "(*)"; }

    Output.emplace_back( textline, Global.UITextColor );
}

void
debug_panel::update_section_eventqueue( std::vector<text_line> &Output ) {

    std::string textline;

            // current event queue
            auto const time { Timer::GetTime() };
            auto const *event { simulation::Events.begin() };
            auto eventtableindex{ 0 };
            while( ( event != nullptr )
                && ( eventtableindex < 30 ) ) {

                if( ( false == event->m_ignored )
                 && ( true == event->bEnabled ) ) {

                    auto const delay { "   " + to_string( std::max( 0.0, event->fStartTime - time ), 1 ) };
                    textline =
                        "Delay: " + delay.substr( delay.length() - 6 )
                        + ", Event: " + event->asName
                        + ( event->Activator ? " (by: " + event->Activator->asName + ")" : "" )
                        + ( event->evJoined ? " (joint event)" : "" );

                    Output.emplace_back( textline, Global.UITextColor );
                    ++eventtableindex;
                }
                event = event->evNext;
            }
            if( Output.empty() ) {
                textline = "(no queued events)";
                Output.emplace_back( textline, Global.UITextColor );
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
        + to_string( 180.0 - glm::degrees( camera.Yaw ), 0 ) // ma być azymut, czyli 0 na północy i rośnie na wschód
        + " "
        + std::string( "S SEE NEN NWW SW" )
        .substr( 0 + 2 * floor( fmod( 8 + ( camera.Yaw + 0.5 * M_PI_4 ) / M_PI_4, 8 ) ), 2 );

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

            textline =
                std::string( "Rendering mode: " )
                + ( Global.bUseVBO ?
                    "VBO" :
                    "Display Lists" )
                + " ";
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

void
debug_panel::render_section( std::string const &Header, std::vector<text_line> const &Lines ) {

    if( Lines.empty() ) { return; }
    if( false == ImGui::CollapsingHeader( Header.c_str() ) ) { return; }

    for( auto const &line : Lines ) {
        ImGui::TextColored( ImVec4( line.color.r, line.color.g, line.color.b, line.color.a ), line.data.c_str() );
    }
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
    if( true == ImGui::Begin( name.c_str(), &is_open, flags ) ) {
        // header section
        for( auto const &line : text_lines ) {
            ImGui::TextWrapped( line.data.c_str() );
        }
    }
    ImGui::End();
}
