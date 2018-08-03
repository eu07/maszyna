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

driver_ui::driver_ui() {

    clear_texts();
/*
    UIHeader = std::make_shared<ui_panel>( 20, 20 ); // header ui panel
    UITable = std::make_shared<ui_panel>( 20, 100 ); // schedule or scan table
    UITranscripts = std::make_shared<ui_panel>( 85, 600 ); // voice transcripts
*/
    // make 4 empty lines for the ui header, to cut down on work down the road
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    UIHeader.text_lines.emplace_back( "", Global.UITextColor );
    // bind the panels with ui object. maybe not the best place for this but, eh
    push_back( &UIHeader );
    push_back( &UITable );
    push_back( &UITranscripts );
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
        case GLFW_KEY_F3:
        case GLFW_KEY_F8:
        case GLFW_KEY_F9:
        case GLFW_KEY_F10:
        case GLFW_KEY_F12: { // ui mode selectors

            if( ( true == Global.ctrlState )
             || ( true == Global.shiftState ) ) {
                // only react to keys without modifiers
                return false;
            }

            if( Action == GLFW_RELEASE ) { return true; } // recognized, but ignored
/*
            EditorModeFlag = ( Key == GLFW_KEY_F11 );
            if( ( true == EditorModeFlag )
             && ( false == Global.ControlPicking ) ) {
                set_cursor( GLFW_CURSOR_NORMAL );
                Global.ControlPicking = true;
            }
*/
        }

        default: { // everything else
            break;
        }
    }

    switch (Key) {
            
        case GLFW_KEY_F1: {
            // basic consist info
            if( Global.iTextMode == Key ) { ++Global.iScreenMode[ Key - GLFW_KEY_F1 ]; }
            if( Global.iScreenMode[ Key - GLFW_KEY_F1 ] > 1 ) {
                // wyłączenie napisów
                Global.iTextMode = 0;
                Global.iScreenMode[ Key - GLFW_KEY_F1 ] = 0;
            }
            else {
                Global.iTextMode = Key;
            }
            return true;
        }

        case GLFW_KEY_F2: {
            // parametry pojazdu
            if( Global.iTextMode == Key ) { ++Global.iScreenMode[ Key - GLFW_KEY_F1 ]; }
            if( Global.iScreenMode[ Key - GLFW_KEY_F1 ] > 1 ) {
                // wyłączenie napisów
                Global.iTextMode = 0;
                Global.iScreenMode[ Key - GLFW_KEY_F1 ] = 0;
            }
            else {
                Global.iTextMode = Key;
            }
            return true;
        }

        case GLFW_KEY_F3: {
            // timetable
            if( Global.iTextMode == Key ) { ++Global.iScreenMode[ Key - GLFW_KEY_F1 ]; }
            if( Global.iScreenMode[ Key - GLFW_KEY_F1 ] > 1 ) {
                // wyłączenie napisów
                Global.iTextMode = 0;
                Global.iScreenMode[ Key - GLFW_KEY_F1 ] = 0;
            }
            else {
                Global.iTextMode = Key;
            }
            return true;
        }

        case GLFW_KEY_F8: {
            // renderer debug data
            Global.iTextMode = Key;
            return true;
        }

        case GLFW_KEY_F9: {
            // wersja
            Global.iTextMode = Key;
            return true;
        }

        case GLFW_KEY_F10: {
            // quit
            if( Global.iTextMode == Key ) {
                Global.iTextMode =
                    ( Global.iPause && ( Key != GLFW_KEY_F1 ) ?
                        GLFW_KEY_F1 :
                        0 ); // wyłączenie napisów, chyba że pauza
            }
            else {
                Global.iTextMode = Key;
            }
            return true;
        }
/*
        case GLFW_KEY_F11: {
            // scenario inspector
            Global.iTextMode = Key;
            return true;
        }
*/
        case GLFW_KEY_F12: {
            // coś tam jeszcze
            Global.iTextMode = Key;
            return true;
        }

        case GLFW_KEY_Y: {
            // potentially quit
            if( Global.iTextMode != GLFW_KEY_F10 ) { return false; } // not in quit mode

            if( Action == GLFW_RELEASE ) { return true; } // recognized, but ignored

            glfwSetWindowShouldClose( m_window, 1 );
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

    UITable.text_lines.clear();
    std::string uitextline1, uitextline2, uitextline3, uitextline4;
    set_tooltip( "" );

    auto const *train { simulation::Train };
    auto const *controlled { ( train ? train->Dynamic() : nullptr ) };
    auto const &camera { Global.pCamera };

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

    switch( Global.iTextMode ) {

        case( GLFW_KEY_F1 ) : {
            // f1, default mode: current time and timetable excerpt
            auto const &time = simulation::Time.data();
            uitextline1 =
                "Time: "
                + to_string( time.wHour ) + ":"
                + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
                + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );
            if( Global.iPause ) {
                uitextline1 += " (paused)";
            }

            if( ( controlled != nullptr ) 
             && ( controlled->Mechanik != nullptr ) ) {

                auto const *mover = controlled->MoverParameters;
                auto const *driver = controlled->Mechanik;

                uitextline2 = "Throttle: " + to_string( driver->Controlling()->MainCtrlPos, 0, 2 ) + "+" + std::to_string( driver->Controlling()->ScndCtrlPos );
                     if( mover->ActiveDir > 0 ) { uitextline2 += " D"; }
                else if( mover->ActiveDir < 0 ) { uitextline2 += " R"; }
                else                            { uitextline2 += " N"; }

                uitextline3 = "Brakes:" + to_string( mover->fBrakeCtrlPos, 1, 5 ) + "+" + to_string( mover->LocalBrakePosA * LocalBrakePosNo, 0 ) + ( mover->SlippingWheels ? " !" : "  " );

                uitextline4 = (
                    true == TestFlag( mover->SecuritySystem.Status, s_aware ) ?
                        "!ALERTER! " :
                        "          " );
                uitextline4 += (
                    true == TestFlag( mover->SecuritySystem.Status, s_active ) ?
                        "!SHP! " :
                        "      " );

                if( Global.iScreenMode[ Global.iTextMode - GLFW_KEY_F1 ] == 1 ) {
                    // detail mode on second key press
                    auto const speedlimit { static_cast<int>( std::floor( driver->VelDesired ) ) };
                    uitextline2 +=
                        " Speed: " + std::to_string( static_cast<int>( std::floor( mover->Vel ) ) ) + " km/h"
                        + " (limit: " + std::to_string( speedlimit ) + " km/h";
                    auto const nextspeedlimit { static_cast<int>( std::floor( driver->VelNext ) ) };
                    if( nextspeedlimit != speedlimit ) {
                        uitextline2 +=
                            ", new limit: " + std::to_string( nextspeedlimit ) + " km/h"
                            + " in " + to_string( driver->ActualProximityDist * 0.001, 1 ) + " km";
                    }
                    uitextline2 += ")";
                    auto const reverser { ( mover->ActiveDir > 0 ? 1 : -1 ) };
                    auto const grade { controlled->VectorFront().y * 100 * ( controlled->DirectionGet() == reverser ? 1 : -1 ) * reverser };
                    if( std::abs( grade ) >= 0.25 ) {
                        uitextline2 += " Grade: " + to_string( grade, 1 ) + "%";
                    }
                    uitextline3 +=
                        " Pressure: " + to_string( mover->BrakePress * 100.0, 2 ) + " kPa"
                        + " (train pipe: " + to_string( mover->PipePress * 100.0, 2 ) + " kPa)";

                    auto const stoptime { static_cast<int>( -1.0 * controlled->Mechanik->fStopTime ) };
                    if( stoptime > 0 ) {
                        uitextline4 += " Loading/unloading in progress (" + to_string( stoptime ) + ( stoptime > 1 ? " seconds" : " second" ) + " left)";
                    }
                    else {
                        auto const trackblockdistance{ std::abs( controlled->Mechanik->TrackBlock() ) };
                        if( trackblockdistance <= 75.0 ) {
                            uitextline4 += " Another vehicle ahead (distance: " + to_string( trackblockdistance, 1 ) + " m)";
                        }
                    }
                }
            }

            break;
        }

        case( GLFW_KEY_F2 ) : {
            // timetable
            auto *vehicle {
                ( FreeFlyModeFlag ?
                    std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) :
                    controlled ) }; // w trybie latania lokalizujemy wg mapy

            if( vehicle == nullptr ) { break; }
            // if the nearest located vehicle doesn't have a direct driver, try to query its owner
            auto const owner = (
                ( ( vehicle->Mechanik != nullptr ) && ( vehicle->Mechanik->Primary() ) ) ?
                    vehicle->Mechanik :
                    vehicle->ctOwner );
            if( owner == nullptr ){ break; }

            auto const *table = owner->TrainTimetable();
            if( table == nullptr ) { break; }

            auto const &time = simulation::Time.data();
            uitextline1 =
                "Time: "
                + to_string( time.wHour ) + ":"
                + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
                + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );
            if( Global.iPause ) {
                uitextline1 += " (paused)";
            }

            uitextline2 = Bezogonkow( owner->Relation(), true ) + " (" + Bezogonkow( owner->TrainName(), true ) + ")";
            auto const nextstation = Bezogonkow( owner->NextStop(), true );
            if( !nextstation.empty() ) {
                // jeśli jest podana relacja, to dodajemy punkt następnego zatrzymania
                uitextline3 = " -> " + nextstation;
            }

            if( Global.iScreenMode[ Global.iTextMode - GLFW_KEY_F1 ] == 1 ) {

                if( 0 == table->StationCount ) {
                    // only bother if there's stations to list
                    UITable.text_lines.emplace_back( "(no timetable)", Global.UITextColor );
                } 
                else {
                    // header
                    UITable.text_lines.emplace_back( "+-----+------------------------------------+-------+-----+", Global.UITextColor );

                    TMTableLine const *tableline;
                    for( int i = owner->iStationStart; i <= std::min( owner->iStationStart + 10, table->StationCount ); ++i ) {
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

                        UITable.text_lines.emplace_back(
                            ( "| " + vmax + " | " + station + " | " + arrival + " | " + traveltime + " |" ),
                             ( candeparture ?
                                glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) :// czas minął i odjazd był, to nazwa stacji będzie na zielono
                                Global.UITextColor ) );
                        UITable.text_lines.emplace_back(
                            ( "|     | " + location + tableline->StationWare + " | " + departure + " |     |" ),
                             ( candeparture ?
                                glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) :// czas minął i odjazd był, to nazwa stacji będzie na zielono
                                Global.UITextColor ) );
                        // divider/footer
                        UITable.text_lines.emplace_back( "+-----+------------------------------------+-------+-----+", Global.UITextColor );
                    }
                    if( owner->iStationStart + 10 < table->StationCount ) {
                        // if we can't display entire timetable, add a scrolling indicator at the bottom
                        UITable.text_lines.emplace_back( "                           ...                            ", Global.UITextColor );
                    }
                }
            }

            break;
        }

        case( GLFW_KEY_F3 ) : {

            auto const *vehicle {
                ( FreeFlyModeFlag ?
                    std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) :
                    controlled ) }; // w trybie latania lokalizujemy wg mapy

            if( vehicle != nullptr ) {
                // jeśli domyślny ekran po pierwszym naciśnięciu
                auto const *mover { vehicle->MoverParameters };

                uitextline1 = "Vehicle name: " + mover->Name;

                if( ( vehicle->Mechanik == nullptr ) && ( vehicle->ctOwner ) ) {
                    // for cars other than leading unit indicate the leader
                    uitextline1 += ", owned by " + vehicle->ctOwner->OwnerName();
                }
                uitextline1 += "; Status: " + mover->EngineDescription( 0 );

                // informacja o sprzęgach
                uitextline1 +=
                    "; C0:" +
                    ( vehicle->PrevConnected ?
                        vehicle->PrevConnected->name() + ":" + to_string( mover->Couplers[ 0 ].CouplingFlag ) + (
                            mover->Couplers[ 0 ].CouplingFlag == 0 ?
                                " (" + to_string( mover->Couplers[ 0 ].CoupleDist, 1 ) + " m)" :
                                "" ) :
                        "none" );
                uitextline1 +=
                    " C1:" +
                    ( vehicle->NextConnected ?
                        vehicle->NextConnected->name() + ":" + to_string( mover->Couplers[ 1 ].CouplingFlag ) + (
                            mover->Couplers[ 1 ].CouplingFlag == 0 ?
                                " (" + to_string( mover->Couplers[ 1 ].CoupleDist, 1 ) + " m)" :
                                "" ) :
                        "none" );

                // equipment flags
                uitextline2  = ( mover->Battery ? "B" : "." );
                uitextline2 += ( mover->Mains ? "M" : "." );
                uitextline2 += ( mover->PantRearUp ? ( mover->PantRearVolt > 0.0 ? "O" : "o" ) : "." );
                uitextline2 += ( mover->PantFrontUp ? ( mover->PantFrontVolt > 0.0 ? "P" : "p" ) : "." );
                uitextline2 += ( mover->PantPressLockActive ? "!" : ( mover->PantPressSwitchActive ? "*" : "." ) );
                uitextline2 += ( mover->WaterPump.is_active ? "W" : ( false == mover->WaterPump.breaker ? "-" : ( mover->WaterPump.is_enabled ? "w" : "." ) ) );
                uitextline2 += ( true == mover->WaterHeater.is_damaged ? "!" : ( mover->WaterHeater.is_active ? "H" : ( false == mover->WaterHeater.breaker ? "-" : ( mover->WaterHeater.is_enabled ? "h" : "." ) ) ) );
                uitextline2 += ( mover->FuelPump.is_active ? "F" : ( mover->FuelPump.is_enabled ? "f" : "." ) );
                uitextline2 += ( mover->OilPump.is_active ? "O" : ( mover->OilPump.is_enabled ? "o" : "." ) );
                uitextline2 += ( false == mover->ConverterAllowLocal ? "-" : ( mover->ConverterAllow ? ( mover->ConverterFlag ? "X" : "x" ) : "." ) );
                uitextline2 += ( mover->ConvOvldFlag ? "!" : "." );
                uitextline2 += ( mover->CompressorFlag ? "C" : ( false == mover->CompressorAllowLocal ? "-" : ( ( mover->CompressorAllow || mover->CompressorStart == start_t::automatic ) ? "c" : "." ) ) );
                uitextline2 += ( mover->CompressorGovernorLock ? "!" : "." );

                auto const *train { simulation::Train };
                if( ( train != nullptr ) && ( train->Dynamic() == vehicle ) ) {
                    uitextline2 += ( mover->Radio ? " R: " : " r: " ) + std::to_string( train->RadioChannel() );
                }
                uitextline2 += " Bdelay: ";
                if( ( mover->BrakeDelayFlag & bdelay_G ) == bdelay_G )
                    uitextline2 += "G";
                if( ( mover->BrakeDelayFlag & bdelay_P ) == bdelay_P )
                    uitextline2 += "P";
                if( ( mover->BrakeDelayFlag & bdelay_R ) == bdelay_R )
                    uitextline2 += "R";
                if( ( mover->BrakeDelayFlag & bdelay_M ) == bdelay_M )
                    uitextline2 += "+Mg";

                uitextline2 += ", Load: " + to_string( mover->Load, 0 ) + " (" + to_string( mover->LoadFlag, 0 ) + ")";

                uitextline2 +=
                    "; Pant: "
                    + to_string( mover->PantPress, 2 )
                    + ( mover->bPantKurek3 ? "-ZG" : "|ZG" );

                uitextline2 +=
                    "; Ft: " + to_string(
                        mover->Ft * 0.001f * (
                            mover->ActiveCab ? mover->ActiveCab :
                            vehicle->ctOwner ? vehicle->ctOwner->Controlling()->ActiveCab :
                            1 ), 1 )
                    + ", Fb: " + to_string( mover->Fb * 0.001f, 1 )
                    + ", Fr: " + to_string( mover->Adhesive( mover->RunningTrack.friction ), 2 )
                    + ( mover->SlippingWheels ? " (!)" : "" );

                if( vehicle->Mechanik ) {
                    uitextline2 += "; Ag: " + to_string( vehicle->Mechanik->fAccGravity, 2 ) + " (" + ( vehicle->Mechanik->fAccGravity > 0.01 ? "\\" : ( vehicle->Mechanik->fAccGravity < -0.01 ? "/" : "-" ) ) + ")";
                }

                uitextline2 +=
                    "; TC:"
                    + to_string( mover->TotalCurrent, 0 );
                auto const frontcouplerhighvoltage =
                    to_string( mover->Couplers[ side::front ].power_high.voltage, 0 )
                    + "@"
                    + to_string( mover->Couplers[ side::front ].power_high.current, 0 );
                auto const rearcouplerhighvoltage =
                    to_string( mover->Couplers[ side::rear ].power_high.voltage, 0 )
                    + "@"
                    + to_string( mover->Couplers[ side::rear ].power_high.current, 0 );
                uitextline2 += ", HV: ";
                if( mover->Couplers[ side::front ].power_high.local == false ) {
                    uitextline2 +=
                            "(" + frontcouplerhighvoltage + ")-"
                        + ":F" + ( vehicle->DirectionGet() ? "<<" : ">>" ) + "R:"
                        + "-(" + rearcouplerhighvoltage + ")";
                }
                else {
                    uitextline2 +=
                            frontcouplerhighvoltage
                        + ":F" + ( vehicle->DirectionGet() ? "<<" : ">>" ) + "R:"
                        + rearcouplerhighvoltage;
                }

                uitextline3 +=
                    "TrB: " + to_string( mover->BrakePress, 2 )
                    + ", " + to_hex_str( mover->Hamulec->GetBrakeStatus(), 2 );

                uitextline3 +=
                    "; LcB: " + to_string( mover->LocBrakePress, 2 )
                    + "; hat: " + to_string( mover->BrakeCtrlPos2, 2 )
                    + "; pipes: " + to_string( mover->PipePress, 2 )
                    + "/" + to_string( mover->ScndPipePress, 2 )
                    + "/" + to_string( mover->EqvtPipePress, 2 )
                    + ", MT: " + to_string( mover->CompressedVolume, 3 )
                    + ", BT: " + to_string( mover->Volume, 3 )
                    + ", CtlP: " + to_string( mover->CntrlPipePress, 3 )
                    + ", CtlT: " + to_string( mover->Hamulec->GetCRP(), 3 );

                if( mover->ManualBrakePos > 0 ) {

                    uitextline3 += "; manual brake on";
                }

                if( vehicle->Mechanik ) {
                    // o ile jest ktoś w środku
                    std::string flags = "cpapcplhhndoiefgvdpseil "; // flagi AI (definicja w Driver.h)
                    for( int i = 0, j = 1; i < 23; ++i, j <<= 1 )
                        if( false == ( vehicle->Mechanik->DrivigFlags() & j ) ) // jak bit ustawiony
                            flags[ i ] = '.';// std::toupper( flags[ i ] ); // ^= 0x20; // to zmiana na wielką literę

                    uitextline4 = flags;

                    uitextline4 +=
                        "Driver: Vd=" + to_string( vehicle->Mechanik->VelDesired, 0 )
                        + " Ad=" + to_string( vehicle->Mechanik->AccDesired, 2 )
                        + " Ah=" + to_string( vehicle->Mechanik->fAccThreshold, 2 )
                        + "@" + to_string( vehicle->Mechanik->fBrake_a0[ 0 ], 2 )
                        + "+" + to_string( vehicle->Mechanik->fBrake_a1[ 0 ], 2 )
                        + " Bd=" + to_string( vehicle->Mechanik->fBrakeDist, 0 )
                        + " Pd=" + to_string( vehicle->Mechanik->ActualProximityDist, 0 )
                        + " Vn=" + to_string( vehicle->Mechanik->VelNext, 0 )
                        + " VSl=" + to_string( vehicle->Mechanik->VelSignalLast, 0 )
                        + " VLl=" + to_string( vehicle->Mechanik->VelLimitLast, 0 )
                        + " VRd=" + to_string( vehicle->Mechanik->VelRoad, 0 )
                        + " VRst=" + to_string( vehicle->Mechanik->VelRestricted, 0 );

                    if( ( vehicle->Mechanik->VelNext == 0.0 )
                     && ( vehicle->Mechanik->eSignNext ) ) {
                        // jeśli ma zapamiętany event semafora, nazwa eventu semafora
                        uitextline4 += " (" + Bezogonkow( vehicle->Mechanik->eSignNext->asName ) + ")";
                    }

                    // biezaca komenda dla AI
                    uitextline4 += ", command: " + vehicle->Mechanik->OrderCurrent();
                }

                if( Global.iScreenMode[ Global.iTextMode - GLFW_KEY_F1 ] == 1 ) {
                    // f2 screen, track scan mode
                    if( vehicle->Mechanik == nullptr ) {
                        //żeby była tabelka, musi być AI
                        break;
                    }

                    std::size_t i = 0; std::size_t const speedtablesize = clamp( static_cast<int>( vehicle->Mechanik->TableSize() ) - 1, 0, 30 );
                    do {
                        std::string scanline = vehicle->Mechanik->TableText( i );
                        if( scanline.empty() ) { break; }
                        UITable.text_lines.emplace_back( Bezogonkow( scanline ), Global.UITextColor );
                        ++i;
                    } while( i < speedtablesize ); // TController:iSpeedTableSize TODO: change when the table gets recoded
                }
            }
            else {
                // wyświetlenie współrzędnych w scenerii oraz kąta kamery, gdy nie mamy wskaźnika
                uitextline1 =
                    "Camera position: "
                    + to_string( camera.Pos.x, 2 ) + " "
                    + to_string( camera.Pos.y, 2 ) + " "
                    + to_string( camera.Pos.z, 2 )
                    + ", azimuth: "
                    + to_string( 180.0 - glm::degrees( camera.Yaw ), 0 ) // ma być azymut, czyli 0 na północy i rośnie na wschód
                    + " "
                    + std::string( "S SEE NEN NWW SW" )
                    .substr( 0 + 2 * floor( fmod( 8 + ( camera.Yaw + 0.5 * M_PI_4 ) / M_PI_4, 8 ) ), 2 );
                // current luminance level
                uitextline2 = "Light level: " + to_string( Global.fLuminance, 3 );
                if( Global.FakeLight ) { uitextline2 += "(*)"; }
            }

            break;
        }

        case( GLFW_KEY_F8 ) : {
            // gfx renderer data
            uitextline1 =
                "FoV: " + to_string( Global.FieldOfView / Global.ZoomFactor, 1 )
                + ", Draw range x " + to_string( Global.fDistanceFactor, 1 )
//                + "; sectors: " + std::to_string( GfxRenderer.m_drawcount )
//                + ", FPS: " + to_string( Timer::GetFPS(), 2 );
                + ", FPS: " + std::to_string( static_cast<int>(std::round(GfxRenderer.Framerate())) );
            if( Global.iSlowMotion ) {
                uitextline1 += " (slowmotion " + to_string( Global.iSlowMotion ) + ")";
            }

            uitextline2 =
                std::string( "Rendering mode: " )
                + ( Global.bUseVBO ?
                    "VBO" :
                    "Display Lists" )
                + " ";
            if( false == Global.LastGLError.empty() ) {
                uitextline2 +=
                    "Last openGL error: "
                    + Global.LastGLError;
            }
            // renderer stats
            uitextline3 = GfxRenderer.info_times();
            uitextline4 = GfxRenderer.info_stats();

            break;
        }

        case( GLFW_KEY_F9 ) : {
            // informacja o wersji
            uitextline1 = "MaSzyna " + Global.asVersion; // informacja o wersji
            if( Global.iMultiplayer ) {
                uitextline1 += " (multiplayer mode is active)";
            }
            uitextline3 =
                  "vehicles: " + to_string( Timer::subsystem.sim_dynamics.average(), 2 ) + " msec"
                + " update total: " + to_string( Timer::subsystem.sim_total.average(), 2 ) + " msec";
            // current event queue
            auto const time { Timer::GetTime() };
            auto const *event { simulation::Events.begin() };
            auto eventtableindex{ 0 };
            while( ( event != nullptr )
                && ( eventtableindex < 30 ) ) {

                if( ( false == event->m_ignored )
                 && ( true == event->bEnabled ) ) {

                    auto const delay { "   " + to_string( std::max( 0.0, event->fStartTime - time ), 1 ) };
                    auto const eventline =
                        "Delay: " + delay.substr( delay.length() - 6 )
                        + ", Event: " + event->asName
                        + ( event->Activator ? " (by: " + event->Activator->asName + ")" : "" )
                        + ( event->evJoined ? " (joint event)" : "" );

                    UITable.text_lines.emplace_back( eventline, Global.UITextColor );
                    ++eventtableindex;
                }
                event = event->evNext;
            }

            break;
        }

        case( GLFW_KEY_F10 ) : {

            uitextline1 = "Press [Y] key to quit / Aby zakonczyc program, przycisnij klawisz [Y].";

            break;
        }

        case( GLFW_KEY_F12 ) : {
            // opcje włączenia i wyłączenia logowania
            uitextline1 = "[0] Debugmode " + std::string( DebugModeFlag ? "(on)" : "(off)" );
            uitextline2 = "[1] log.txt " + std::string( ( Global.iWriteLogEnabled & 1 ) ? "(on)" : "(off)" );
            uitextline3 = "[2] Console " + std::string( ( Global.iWriteLogEnabled & 2 ) ? "(on)" : "(off)" );

            break;
        }

        default: {
            // uncovered cases, nothing to do here...
            // ... unless we're in debug mode
            if( DebugModeFlag ) {

                auto const *vehicle {
                    ( FreeFlyModeFlag ?
                        std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera.Pos, 20, false, false ) ) :
                        controlled ) }; // w trybie latania lokalizujemy wg mapy
                if( vehicle == nullptr ) {
                    break;
                }
                auto const *mover { vehicle->MoverParameters };
                uitextline1 =
                    "vel: " + to_string( vehicle->GetVelocity(), 2 ) + "/" + to_string( mover->nrot* M_PI * mover->WheelDiameter * 3.6, 2 )
                    + " km/h;" + ( mover->SlippingWheels ? " (!)" : "    " )
                    + " dist: " + to_string( mover->DistCounter, 2 ) + " km"
                    + "; pos: [" + to_string( vehicle->GetPosition().x, 2 ) + ", " + to_string( vehicle->GetPosition().y, 2 ) + ", " + to_string( vehicle->GetPosition().z, 2 ) + "]"
                    + ", PM=" + to_string( mover->WheelFlat, 1 )
                    + " mm; enpwr=" + to_string( mover->EnginePower, 1 )
                    + "; enrot=" + to_string( mover->enrot * 60, 0 )
                    + " tmrot=" + to_string( std::abs( mover->nrot ) * mover->Transmision.Ratio * 60, 0 )
                    + "; ventrot=" + to_string( mover->RventRot * 60, 1 )
                    + "; fanrot=" + to_string( mover->dizel_heat.rpmw, 1 ) + ", " + to_string( mover->dizel_heat.rpmw2, 1 );

                uitextline2 =
                    "HamZ=" + to_string( mover->fBrakeCtrlPos, 2 )
                    + "; HamP=" + to_string( mover->LocalBrakePosA, 2 )
                    + "; NasJ=" + std::to_string( mover->MainCtrlPos ) + "(" + std::to_string( mover->MainCtrlActualPos ) + ")"
                    + ( ( mover->ShuntMode && mover->EngineType == TEngineType::DieselElectric ) ?
                        "; NasB=" + to_string( mover->AnPos, 2 ) :
                        "; NasB=" + std::to_string( mover->ScndCtrlPos ) + "(" + std::to_string( mover->ScndCtrlActualPos ) + ")" )
                    + "; I=" +
                    ( mover->TrainType == dt_EZT ?
                        std::to_string( int( mover->ShowCurrent( 0 ) ) ) :
                       std::to_string( int( mover->Im ) ) )
                    + "; U=" + to_string( int( mover->RunningTraction.TractionVoltage + 0.5 ) )
                    + "; R=" +
                    ( std::abs( mover->RunningShape.R ) > 10000.0 ?
                        "~0.0" :
                        to_string( mover->RunningShape.R, 1 ) )
                    + " An=" + to_string( mover->AccN, 2 ); // przyspieszenie poprzeczne

                if( tprev != simulation::Time.data().wSecond ) {
                    tprev = simulation::Time.data().wSecond;
                    Acc = ( mover->Vel - VelPrev ) / 3.6;
                    VelPrev = mover->Vel;
                }
                uitextline2 += "; As=" + to_string( Acc, 2 ); // przyspieszenie wzdłużne
//                uitextline2 += " eAngle=" + to_string( std::cos( mover->eAngle ), 2 );
                uitextline2 += "; oilP=" + to_string( mover->OilPump.pressure_present, 3 );
                uitextline2 += " oilT=" + to_string( mover->dizel_heat.To, 2 );
                uitextline2 += "; waterT=" + to_string( mover->dizel_heat.temperatura1, 2 );
                uitextline2 += ( mover->WaterCircuitsLink ? "-" : "|" );
                uitextline2 += to_string( mover->dizel_heat.temperatura2, 2 );
                uitextline2 += "; engineT=" + to_string( mover->dizel_heat.Ts, 2 );

                uitextline3 =
                    "cyl.ham. " + to_string( mover->BrakePress, 2 )
                    + "; prz.gl. " + to_string( mover->PipePress, 2 )
                    + "; zb.gl. " + to_string( mover->CompressedVolume, 2 )
                    // youBy - drugi wezyk
                    + "; p.zas. " + to_string( mover->ScndPipePress, 2 );
 
                // McZapkie: warto wiedziec w jakim stanie sa przelaczniki
                if( mover->ConvOvldFlag )
                    uitextline3 += " C! ";
                else if( mover->FuseFlag )
                    uitextline3 += " F! ";
                else if( !mover->Mains )
                    uitextline3 += " () ";
                else {
                    switch(
                        mover->ActiveDir *
                        ( mover->Imin == mover->IminLo ?
                            1 :
                            2 ) ) {
                        case  2: { uitextline3 += " >> "; break; }
                        case  1: { uitextline3 += " -> "; break; }
                        case  0: { uitextline3 += " -- "; break; }
                        case -1: { uitextline3 += " <- "; break; }
                        case -2: { uitextline3 += " << "; break; }
                    }
                }
                // McZapkie: predkosc szlakowa
                if( mover->RunningTrack.Velmax == -1 ) {
                    uitextline3 += " Vtrack=Vmax";
                }
                else {
                    uitextline3 += " Vtrack " + to_string( mover->RunningTrack.Velmax, 2 );
                }

                if( ( mover->EnginePowerSource.SourceType == TPowerSource::CurrentCollector )
                    || ( mover->TrainType == dt_EZT ) ) {
                    uitextline3 +=
                        "; pant. " + to_string( mover->PantPress, 2 )
                        + ( mover->bPantKurek3 ? "=" : "^" ) + "ZG";
                }

                // McZapkie: komenda i jej parametry
                if( mover->CommandIn.Command != ( "" ) ) {
                    uitextline4 =
                        "C:" + mover->CommandIn.Command
                        + " V1=" + to_string( mover->CommandIn.Value1, 0 )
                        + " V2=" + to_string( mover->CommandIn.Value2, 0 );
                }
                if( ( vehicle->Mechanik )
                 && ( vehicle->Mechanik->AIControllFlag == AIdriver ) ) {
                    uitextline4 +=
                        "AI: Vd=" + to_string( vehicle->Mechanik->VelDesired, 0 )
                        + " ad=" + to_string(vehicle->Mechanik->AccDesired, 2)
						+ "/" + to_string(vehicle->Mechanik->AccDesired*vehicle->Mechanik->BrakeAccFactor(), 2)
						+ " atrain=" + to_string(vehicle->Mechanik->fBrake_a0[0], 2)
						+ "+" + to_string(vehicle->Mechanik->fBrake_a1[0], 2)
						+ " aS=" + to_string(vehicle->Mechanik->AbsAccS_pub, 2)
                        + " Pd=" + to_string( vehicle->Mechanik->ActualProximityDist, 0 )
                        + " Vn=" + to_string( vehicle->Mechanik->VelNext, 0 );
                }

                // induction motor data
                if( mover->EngineType == TEngineType::ElectricInductionMotor ) {

                    UITable.text_lines.emplace_back( "      eimc:            eimv:            press:", Global.UITextColor );
                    for( int i = 0; i <= 20; ++i ) {

                        std::string parameters =
                            mover->eimc_labels[ i ] + to_string( mover->eimc[ i ], 2, 9 )
                            + " | "
                            + mover->eimv_labels[ i ] + to_string( mover->eimv[ i ], 2, 9 );

                        if( i < 10 ) {
                            parameters += " | " + train->fPress_labels[i] + to_string( train->fPress[ i ][ 0 ], 2, 9 );
                        }
                        else if( i == 12 ) {
                            parameters += "        med:";
                        }
                        else if( i >= 13 ) {
                            parameters += " | " + vehicle->MED_labels[ i - 13 ] + to_string( vehicle->MED[ 0 ][ i - 13 ], 2, 9 );
                        }

                        UITable.text_lines.emplace_back( parameters, Global.UITextColor );
                    }
                }
				if (mover->EngineType == TEngineType::DieselEngine) {
					std::string parameters = "param       value";
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "efill: " + to_string(mover->dizel_fill, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "etorq: " + to_string(mover->dizel_Torque, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "creal: " + to_string(mover->dizel_engage, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "cdesi: " + to_string(mover->dizel_engagestate, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "cdelt: " + to_string(mover->dizel_engagedeltaomega, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "gears: " + to_string(mover->dizel_automaticgearstatus, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hydro      value";
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCnI: " + to_string(mover->hydro_TC_nIn, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCnO: " + to_string(mover->hydro_TC_nOut, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTM: " + to_string(mover->hydro_TC_TMRatio, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTI: " + to_string(mover->hydro_TC_TorqueIn, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTO: " + to_string(mover->hydro_TC_TorqueOut, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCfl: " + to_string(mover->hydro_TC_Fill, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCLR: " + to_string(mover->hydro_TC_LockupRate, 2, 9);
					UITable.text_lines.emplace_back(parameters, Global.UITextColor);
					//parameters = "hTCXX: " + to_string(mover->hydro_TC_nIn, 2, 9);
					//UITable.text_lines.emplace_back(parameters, Global.UITextColor);
				}

            } // if( DebugModeFlag && Controlled )

            break;
        }
    }

#ifdef EU07_USE_OLD_UI_CODE
    if( Controlled && DebugModeFlag && !Global.iTextMode ) {

        uitextline1 +=
            ( "; d_omega " ) + to_string( Controlled->MoverParameters->dizel_engagedeltaomega, 3 );

        if( Controlled->MoverParameters->EngineType == ElectricInductionMotor ) {

            for( int i = 0; i <= 8; i++ ) {
                for( int j = 0; j <= 9; j++ ) {
                    glRasterPos2f( 0.05f + 0.03f * i, 0.16f - 0.01f * j );
                    uitextline4 = to_string( Train->fEIMParams[ i ][ j ], 2 );
                }
            }
        }
    }
#endif

    // update the ui header texts
    auto &headerdata = UIHeader.text_lines;
    headerdata[ 0 ].data = uitextline1;
    headerdata[ 1 ].data = uitextline2;
    headerdata[ 2 ].data = uitextline3;
    headerdata[ 3 ].data = uitextline4;

    // stenogramy dźwięków (ukryć, gdy tabelka skanowania lub rozkład?)
    auto &transcripts = UITranscripts.text_lines;
    transcripts.clear();
    for( auto const &transcript : ui::Transcripts.aLines ) {

        if( Global.fTimeAngleDeg >= transcript.fShow ) {

            cParser parser( transcript.asText );
            while( true == parser.getTokens( 1, false, "|" ) ) {

                std::string transcriptline; parser >> transcriptline;
                transcripts.emplace_back( transcriptline, glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
            }
        }
    }

}
