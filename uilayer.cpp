

#include "stdafx.h"
#include "uilayer.h"
#include "uitranscripts.h"

#include "globals.h"
#include "translation.h"
#include "simulation.h"
#include "mtable.h"
#include "train.h"
#include "dynobj.h"
#include "model3d.h"
#include "renderer.h"
#include "timer.h"
#include "utilities.h"
#include "logs.h"

ui_layer UILayer;

extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window ); //m7todo: potrzebne do directsound
}

ui_layer::~ui_layer() {
/*
// this should be invoked manually, or we risk trying to delete the lists after the context is gone
    if( m_fontbase != -1 )
        ::glDeleteLists( m_fontbase, 96 );
*/
}

bool
ui_layer::init( GLFWwindow *Window ) {

    clear_texts();

    UIHeader = std::make_shared<ui_panel>( 20, 20 ); // header ui panel
    UITable = std::make_shared<ui_panel>( 20, 100 ); // schedule or scan table
    UITranscripts = std::make_shared<ui_panel>( 85, 600 ); // voice transcripts
    // make 4 empty lines for the ui header, to cut down on work down the road
    UIHeader->text_lines.emplace_back( "", Global.UITextColor );
    UIHeader->text_lines.emplace_back( "", Global.UITextColor );
    UIHeader->text_lines.emplace_back( "", Global.UITextColor );
    UIHeader->text_lines.emplace_back( "", Global.UITextColor );
    // bind the panels with ui object. maybe not the best place for this but, eh
    push_back( UIHeader );
    push_back( UITable );
    push_back( UITranscripts );

    m_window = Window;
    HFONT font; // Windows Font ID
    m_fontbase = ::glGenLists(96); // storage for 96 characters
    HDC hDC = ::GetDC( glfwGetWin32Window( m_window ) );
    font = ::CreateFont( -MulDiv( 10, ::GetDeviceCaps( hDC, LOGPIXELSY ), 72 ), // height of font
                        0, // width of font
                        0, // angle of escapement
                        0, // orientation angle
                        (Global.bGlutFont ? FW_MEDIUM : FW_HEAVY), // font weight
                        FALSE, // italic
                        FALSE, // underline
                        FALSE, // strikeout
                        DEFAULT_CHARSET, // character set identifier
                        OUT_DEFAULT_PRECIS, // output precision
                        CLIP_DEFAULT_PRECIS, // clipping precision
                        (Global.bGlutFont ? CLEARTYPE_QUALITY : PROOF_QUALITY), // output quality
                        DEFAULT_PITCH | FF_DONTCARE, // family and pitch
                        "Lucida Console"); // font name
    ::SelectObject(hDC, font); // selects the font we want
    if( TRUE == ::wglUseFontBitmaps( hDC, 32, 96, m_fontbase ) ) {
        // builds 96 characters starting at character 32
        WriteLog( "Display Lists font used" ); //+AnsiString(glGetError())
        WriteLog( "Font init OK" ); //+AnsiString(glGetError())
        Global.DLFont = true;
        return true;
    }
    else {
        ErrorLog( "Font init failed" );
//        return false;
        // NOTE: we report success anyway, given some cards can't produce fonts in this manner
        Global.DLFont = false;
        return true;
    }
}

// potentially processes provided input key. returns: true if key was processed, false otherwise
bool
ui_layer::on_key( int const Key, int const Action ) {
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
ui_layer::update() {

    UITable->text_lines.clear();
    std::string uitextline1, uitextline2, uitextline3, uitextline4;
    UILayer.set_tooltip( "" );

    auto const *train { ( Global.pWorld ? Global.pWorld->train() : nullptr ) };
    auto const *controlled { ( Global.pWorld ? Global.pWorld->controlled() : nullptr ) };
    auto const *camera { Global.pCamera };

    if( ( train != nullptr ) && ( false == FreeFlyModeFlag ) ) {
        if( false == DebugModeFlag ) {
            // in regular mode show control functions, for defined controls
            UILayer.set_tooltip( locale::label_cab_control( train->GetLabel( GfxRenderer.Pick_Control() ) ) );
        }
        else {
            // in debug mode show names of submodels, to help with cab setup and/or debugging
            auto const cabcontrol = GfxRenderer.Pick_Control();
            UILayer.set_tooltip( ( cabcontrol ? cabcontrol->pName : "" ) );
        }
    }
    if( ( true == Global.ControlPicking ) && ( true == FreeFlyModeFlag ) && ( true == DebugModeFlag ) ) {
        auto const scenerynode = GfxRenderer.Pick_Node();
        UILayer.set_tooltip(
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

                auto const &mover = controlled->MoverParameters;
                auto const &driver = controlled->Mechanik;

                uitextline2 = "Throttle: " + to_string( driver->Controlling()->MainCtrlPos, 0, 2 ) + "+" + std::to_string( driver->Controlling()->ScndCtrlPos );
                     if( mover->ActiveDir > 0 ) { uitextline2 += " D"; }
                else if( mover->ActiveDir < 0 ) { uitextline2 += " R"; }
                else                            { uitextline2 += " N"; }

                uitextline3 = "Brakes:" + to_string( mover->fBrakeCtrlPos, 1, 5 ) + "+" + std::to_string( mover->LocalBrakePos ) + ( mover->SlippingWheels ? " !" : "  " );

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
                    uitextline2 +=
                        " Speed: " + std::to_string( static_cast<int>( std::floor( mover->Vel ) ) ) + " km/h"
                        + " (limit: " + std::to_string( static_cast<int>( std::floor( driver->VelDesired ) ) ) + " km/h"
                        + ", next limit: " + std::to_string( static_cast<int>( std::floor( controlled->Mechanik->VelNext ) ) ) + " km/h"
                        + " in " + to_string( controlled->Mechanik->ActualProximityDist * 0.001, 1 ) + " km)";
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
                    std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera->Pos, 20, false, false ) ) :
                    controlled ) }; // w trybie latania lokalizujemy wg mapy

            if( vehicle == nullptr ) { break; }
            // if the nearest located vehicle doesn't have a direct driver, try to query its owner
            auto const owner = (
                ( ( vehicle->Mechanik != nullptr ) && ( vehicle->Mechanik->Primary() ) ) ?
                    vehicle->Mechanik :
                    vehicle->ctOwner );
            if( owner == nullptr ){ break; }

            auto const table = owner->Timetable();
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

            uitextline2 = Bezogonkow( owner->Relation(), true ) + " (" + Bezogonkow( owner->Timetable()->TrainName, true ) + ")";
            auto const nextstation = Bezogonkow( owner->NextStop(), true );
            if( !nextstation.empty() ) {
                // jeśli jest podana relacja, to dodajemy punkt następnego zatrzymania
                uitextline3 = " -> " + nextstation;
            }

            if( Global.iScreenMode[ Global.iTextMode - GLFW_KEY_F1 ] == 1 ) {

                if( 0 == table->StationCount ) {
                    // only bother if there's stations to list
                    UITable->text_lines.emplace_back( "(no timetable)", Global.UITextColor );
                } 
                else {
                    // header
                    UITable->text_lines.emplace_back( "+----------------------------+-------+-------+-----+", Global.UITextColor );

                    TMTableLine *tableline;
                    for( int i = owner->iStationStart; i <= std::min( owner->iStationStart + 15, table->StationCount ); ++i ) {
                        // wyświetlenie pozycji z rozkładu
                        tableline = table->TimeTable + i; // linijka rozkładu

                        std::string station =
                            ( tableline->StationName + "                          " ).substr( 0, 26 );
                        std::string arrival =
                            ( tableline->Ah >= 0 ?
                            to_string( int( 100 + tableline->Ah ) ).substr( 1, 2 ) + ":" + to_string( int( 100 + tableline->Am ) ).substr( 1, 2 ) :
                            "     " );
                        std::string departure =
                            ( tableline->Dh >= 0 ?
                            to_string( int( 100 + tableline->Dh ) ).substr( 1, 2 ) + ":" + to_string( int( 100 + tableline->Dm ) ).substr( 1, 2 ) :
                            "     " );
                        std::string vmax =
                            "   "
                            + to_string( tableline->vmax, 0 );
                        vmax = vmax.substr( vmax.length() - 3, 3 ); // z wyrównaniem do prawej

                        UITable->text_lines.emplace_back(
                            Bezogonkow( "| " + station + " | " + arrival + " | " + departure + " | " + vmax + " | " + tableline->StationWare, true ),
                            ( ( owner->iStationStart < table->StationIndex )
                           && ( i < table->StationIndex )
                           && ( ( time.wHour * 60 + time.wMinute ) >= ( tableline->Dh * 60 + tableline->Dm ) ) ?
                                glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f ) :// czas minął i odjazd był, to nazwa stacji będzie na zielono
                                Global.UITextColor ) );
                        // divider/footer
                        UITable->text_lines.emplace_back( "+----------------------------+-------+-------+-----+", Global.UITextColor );
                    }
                }
            }

            break;
        }

        case( GLFW_KEY_F3 ) : {

            auto *vehicle{
                ( FreeFlyModeFlag ?
                    std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera->Pos, 20, false, false ) ) :
                    controlled ) }; // w trybie latania lokalizujemy wg mapy

            if( vehicle != nullptr ) {
                // jeśli domyślny ekran po pierwszym naciśnięciu
                uitextline1 = "Vehicle name: " + vehicle->MoverParameters->Name;

                if( ( vehicle->Mechanik == nullptr ) && ( vehicle->ctOwner ) ) {
                    // for cars other than leading unit indicate the leader
                    uitextline1 += ", owned by " + vehicle->ctOwner->OwnerName();
                }
                uitextline1 += "; Status: " + vehicle->MoverParameters->EngineDescription( 0 );
                // informacja o sprzęgach
                uitextline1 +=
                    "; C0:" +
                    ( vehicle->PrevConnected ?
                        vehicle->PrevConnected->name() + ":" + to_string( vehicle->MoverParameters->Couplers[ 0 ].CouplingFlag ) + (
                            vehicle->MoverParameters->Couplers[ 0 ].CouplingFlag == 0 ?
                                " (" + to_string( vehicle->MoverParameters->Couplers[ 0 ].CoupleDist, 1 ) + " m)" :
                                "" ) :
                        "none" );
                uitextline1 +=
                    " C1:" +
                    ( vehicle->NextConnected ?
                        vehicle->NextConnected->name() + ":" + to_string( vehicle->MoverParameters->Couplers[ 1 ].CouplingFlag ) + (
                            vehicle->MoverParameters->Couplers[ 1 ].CouplingFlag == 0 ?
                                " (" + to_string( vehicle->MoverParameters->Couplers[ 1 ].CoupleDist, 1 ) + " m)" :
                                "" ) :
                        "none" );

                // equipment flags
                uitextline2  = ( vehicle->MoverParameters->Battery ? "B" : "." );
                uitextline2 += ( vehicle->MoverParameters->Mains ? "M" : "." );
                uitextline2 += ( vehicle->MoverParameters->PantRearUp ? ( vehicle->MoverParameters->PantRearVolt > 0.0 ? "O" : "o" ) : "." );
                uitextline2 += ( vehicle->MoverParameters->PantFrontUp ? ( vehicle->MoverParameters->PantFrontVolt > 0.0 ? "P" : "p" ) : "." );
                uitextline2 += ( vehicle->MoverParameters->PantPressLockActive ? "!" : ( vehicle->MoverParameters->PantPressSwitchActive ? "*" : "." ) );
                uitextline2 += ( vehicle->MoverParameters->WaterPump.is_active ? "W" : ( false == vehicle->MoverParameters->WaterPump.breaker ? "-" : ( vehicle->MoverParameters->WaterPump.is_enabled ? "w" : "." ) ) );
                uitextline2 += ( true == vehicle->MoverParameters->WaterHeater.is_damaged ? "!" : ( vehicle->MoverParameters->WaterHeater.is_active ? "H" : ( false == vehicle->MoverParameters->WaterHeater.breaker ? "-" : ( vehicle->MoverParameters->WaterHeater.is_enabled ? "h" : "." ) ) ) );
                uitextline2 += ( vehicle->MoverParameters->FuelPump.is_active ? "F" : ( vehicle->MoverParameters->FuelPump.is_enabled ? "f" : "." ) );
                uitextline2 += ( vehicle->MoverParameters->OilPump.is_active ? "O" : ( vehicle->MoverParameters->OilPump.is_enabled ? "o" : "." ) );
                uitextline2 += ( false == vehicle->MoverParameters->ConverterAllowLocal ? "-" : ( vehicle->MoverParameters->ConverterAllow ? ( vehicle->MoverParameters->ConverterFlag ? "X" : "x" ) : "." ) );
                uitextline2 += ( vehicle->MoverParameters->ConvOvldFlag ? "!" : "." );
                uitextline2 += ( vehicle->MoverParameters->CompressorFlag ? "C" : ( false == vehicle->MoverParameters->CompressorAllowLocal ? "-" : ( ( vehicle->MoverParameters->CompressorAllow || vehicle->MoverParameters->CompressorStart == start::automatic ) ? "c" : "." ) ) );
                uitextline2 += ( vehicle->MoverParameters->CompressorGovernorLock ? "!" : "." );

                auto const train { Global.pWorld->train() };
                if( ( train != nullptr ) && ( train->Dynamic() == vehicle ) ) {
                    uitextline2 += ( vehicle->MoverParameters->Radio ? " R: " : " r: " ) + std::to_string( train->RadioChannel() );
                }
/*
                uitextline2 +=
                    " AnlgB: " + to_string( tmp->MoverParameters->AnPos, 1 )
                    + "+"
                    + to_string( tmp->MoverParameters->LocalBrakePosA, 1 )
*/
                uitextline2 += " Bdelay: ";
                if( ( vehicle->MoverParameters->BrakeDelayFlag & bdelay_G ) == bdelay_G )
                    uitextline2 += "G";
                if( ( vehicle->MoverParameters->BrakeDelayFlag & bdelay_P ) == bdelay_P )
                    uitextline2 += "P";
                if( ( vehicle->MoverParameters->BrakeDelayFlag & bdelay_R ) == bdelay_R )
                    uitextline2 += "R";
                if( ( vehicle->MoverParameters->BrakeDelayFlag & bdelay_M ) == bdelay_M )
                    uitextline2 += "+Mg";

                uitextline2 += ", Load: " + to_string( vehicle->MoverParameters->Load, 0 ) + " (" + to_string( vehicle->MoverParameters->LoadFlag, 0 ) + ")";

                uitextline2 +=
                    "; Pant: "
                    + to_string( vehicle->MoverParameters->PantPress, 2 )
                    + ( vehicle->MoverParameters->bPantKurek3 ? "-ZG" : "|ZG" );

                uitextline2 +=
                    "; Ft: " + to_string(
                        vehicle->MoverParameters->Ft * 0.001f * (
                            vehicle->MoverParameters->ActiveCab ? vehicle->MoverParameters->ActiveCab :
                            vehicle->ctOwner ? vehicle->ctOwner->Controlling()->ActiveCab :
                            1 ), 1 )
                    + ", Fb: " + to_string( vehicle->MoverParameters->Fb * 0.001f, 1 )
                    + ", Fr: " + to_string( vehicle->MoverParameters->Adhesive( vehicle->MoverParameters->RunningTrack.friction ), 2 )
                    + ( vehicle->MoverParameters->SlippingWheels ? " (!)" : "" );

                if( vehicle->Mechanik ) {
                    uitextline2 += "; Ag: " + to_string( vehicle->Mechanik->fAccGravity, 2 );
                }

                uitextline2 +=
                    "; TC:"
                    + to_string( vehicle->MoverParameters->TotalCurrent, 0 );
                auto const frontcouplerhighvoltage =
                    to_string( vehicle->MoverParameters->Couplers[ side::front ].power_high.voltage, 0 )
                    + "@"
                    + to_string( vehicle->MoverParameters->Couplers[ side::front ].power_high.current, 0 );
                auto const rearcouplerhighvoltage =
                    to_string( vehicle->MoverParameters->Couplers[ side::rear ].power_high.voltage, 0 )
                    + "@"
                    + to_string( vehicle->MoverParameters->Couplers[ side::rear ].power_high.current, 0 );
                uitextline2 += ", HV: ";
                if( vehicle->MoverParameters->Couplers[ side::front ].power_high.local == false ) {
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
                    "TrB: " + to_string( vehicle->MoverParameters->BrakePress, 2 )
                    + ", " + to_hex_str( vehicle->MoverParameters->Hamulec->GetBrakeStatus(), 2 );

                uitextline3 +=
                    "; LcB: " + to_string( vehicle->MoverParameters->LocBrakePress, 2 )
                    + "; hat: " + to_string( vehicle->MoverParameters->BrakeCtrlPos2, 2 )
                    + "; pipes: " + to_string( vehicle->MoverParameters->PipePress, 2 )
                    + "/" + to_string( vehicle->MoverParameters->ScndPipePress, 2 )
                    + "/" + to_string( vehicle->MoverParameters->EqvtPipePress, 2 )
                    + ", MT: " + to_string( vehicle->MoverParameters->CompressedVolume, 3 )
                    + ", BT: " + to_string( vehicle->MoverParameters->Volume, 3 )
                    + ", CtlP: " + to_string( vehicle->MoverParameters->CntrlPipePress, 3 )
                    + ", CtlT: " + to_string( vehicle->MoverParameters->Hamulec->GetCRP(), 3 );

                if( vehicle->MoverParameters->ManualBrakePos > 0 ) {

                    uitextline3 += "; manual brake on";
                }
/*
                if( tmp->MoverParameters->LocalBrakePos > 0 ) {

                    uitextline3 += ", local brake on";
                }
                else {

                    uitextline3 += ", local brake off";
                }
*/
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
                        UITable->text_lines.emplace_back( Bezogonkow( scanline ), Global.UITextColor );
                        ++i;
                    } while( i < speedtablesize ); // TController:iSpeedTableSize TODO: change when the table gets recoded
                }
            }
            else {
                // wyświetlenie współrzędnych w scenerii oraz kąta kamery, gdy nie mamy wskaźnika
                uitextline1 =
                    "Camera position: "
                    + to_string( camera->Pos.x, 2 ) + " "
                    + to_string( camera->Pos.y, 2 ) + " "
                    + to_string( camera->Pos.z, 2 )
                    + ", azimuth: "
                    + to_string( 180.0 - glm::degrees( camera->Yaw ), 0 ) // ma być azymut, czyli 0 na północy i rośnie na wschód
                    + " "
                    + std::string( "S SEE NEN NWW SW" )
                    .substr( 0 + 2 * floor( fmod( 8 + ( camera->Yaw + 0.5 * M_PI_4 ) / M_PI_4, 8 ) ), 2 );
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

            break;
        }

        case( GLFW_KEY_F10 ) : {

            uitextline1 = ( "Press [Y] key to quit / Aby zakonczyc program, przycisnij klawisz [Y]." );

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

                auto *vehicle {
                    ( FreeFlyModeFlag ?
                        std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera->Pos, 20, false, false ) ) :
                        controlled ) }; // w trybie latania lokalizujemy wg mapy
                if( vehicle == nullptr ) {
                    break;
                }
                uitextline1 =
                    "vel: " + to_string( vehicle->GetVelocity(), 2 ) + "/" + to_string( vehicle->MoverParameters->nrot* M_PI * vehicle->MoverParameters->WheelDiameter * 3.6, 2 )
                    + " km/h;" + ( vehicle->MoverParameters->SlippingWheels ? " (!)" : "    " )
                    + " dist: " + to_string( vehicle->MoverParameters->DistCounter, 2 ) + " km"
                    + "; pos: [" + to_string( vehicle->GetPosition().x, 2 ) + ", " + to_string( vehicle->GetPosition().y, 2 ) + ", " + to_string( vehicle->GetPosition().z, 2 ) + "]"
                    + ", PM=" + to_string( vehicle->MoverParameters->WheelFlat, 1 )
                    + " mm; enrot=" + to_string( vehicle->MoverParameters->enrot * 60, 0 )
                    + " tmrot=" + to_string( std::abs( vehicle->MoverParameters->nrot ) * vehicle->MoverParameters->Transmision.Ratio * 60, 0 )
                    + "; ventrot=" + to_string( vehicle->MoverParameters->RventRot * 60, 1 )
                    + "; fanrot=" + to_string( vehicle->MoverParameters->dizel_heat.rpmw, 1 ) + ", " + to_string( vehicle->MoverParameters->dizel_heat.rpmw2, 1 );

                uitextline2 =
                    "HamZ=" + to_string( vehicle->MoverParameters->fBrakeCtrlPos, 2 )
                    + "; HamP=" + std::to_string( vehicle->MoverParameters->LocalBrakePos ) + "/" + to_string( vehicle->MoverParameters->LocalBrakePosA, 2 )
                    + "; NasJ=" + std::to_string( vehicle->MoverParameters->MainCtrlPos ) + "(" + std::to_string( vehicle->MoverParameters->MainCtrlActualPos ) + ")"
                    + ( vehicle->MoverParameters->ShuntMode ?
                        "; NasB=" + to_string( vehicle->MoverParameters->AnPos, 2 ) :
                        "; NasB=" + std::to_string( vehicle->MoverParameters->ScndCtrlPos ) + "(" + std::to_string( vehicle->MoverParameters->ScndCtrlActualPos ) + ")" )
                    + "; I=" +
                    ( vehicle->MoverParameters->TrainType == dt_EZT ?
                        std::to_string( int( vehicle->MoverParameters->ShowCurrent( 0 ) ) ) :
                       std::to_string( int( vehicle->MoverParameters->Im ) ) )
                    + "; U=" + to_string( int( vehicle->MoverParameters->RunningTraction.TractionVoltage + 0.5 ) )
                    + "; R=" +
                    ( std::abs( vehicle->MoverParameters->RunningShape.R ) > 10000.0 ?
                        "~0.0" :
                        to_string( vehicle->MoverParameters->RunningShape.R, 1 ) )
                    + " An=" + to_string( vehicle->MoverParameters->AccN, 2 ); // przyspieszenie poprzeczne

                if( tprev != simulation::Time.data().wSecond ) {
                    tprev = simulation::Time.data().wSecond;
                    Acc = ( vehicle->MoverParameters->Vel - VelPrev ) / 3.6;
                    VelPrev = vehicle->MoverParameters->Vel;
                }
                uitextline2 += "; As=" + to_string( Acc, 2 ); // przyspieszenie wzdłużne
/*
                uitextline2 += " eAngle=" + to_string( std::cos( vehicle->MoverParameters->eAngle ), 2 );
*/
                uitextline2 += "; oilP=" + to_string( vehicle->MoverParameters->OilPump.pressure_present, 3 );
                uitextline2 += " oilT=" + to_string( vehicle->MoverParameters->dizel_heat.To, 2 );
                uitextline2 += "; waterT=" + to_string( vehicle->MoverParameters->dizel_heat.temperatura1, 2 );
                uitextline2 += ( vehicle->MoverParameters->WaterCircuitsLink ? "-" : "|" );
                uitextline2 += to_string( vehicle->MoverParameters->dizel_heat.temperatura2, 2 );
                uitextline2 += "; engineT=" + to_string( vehicle->MoverParameters->dizel_heat.Ts, 2 );

                uitextline3 =
                    "cyl.ham. " + to_string( vehicle->MoverParameters->BrakePress, 2 )
                    + "; prz.gl. " + to_string( vehicle->MoverParameters->PipePress, 2 )
                    + "; zb.gl. " + to_string( vehicle->MoverParameters->CompressedVolume, 2 )
                    // youBy - drugi wezyk
                    + "; p.zas. " + to_string( vehicle->MoverParameters->ScndPipePress, 2 );
 
                // McZapkie: warto wiedziec w jakim stanie sa przelaczniki
                if( vehicle->MoverParameters->ConvOvldFlag )
                    uitextline3 += " C! ";
                else if( vehicle->MoverParameters->FuseFlag )
                    uitextline3 += " F! ";
                else if( !vehicle->MoverParameters->Mains )
                    uitextline3 += " () ";
                else {
                    switch(
                        vehicle->MoverParameters->ActiveDir *
                        ( vehicle->MoverParameters->Imin == vehicle->MoverParameters->IminLo ?
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
                if( vehicle->MoverParameters->RunningTrack.Velmax == -1 ) {
                    uitextline3 += " Vtrack=Vmax";
                }
                else {
                    uitextline3 += " Vtrack " + to_string( vehicle->MoverParameters->RunningTrack.Velmax, 2 );
                }

                if( ( vehicle->MoverParameters->EnginePowerSource.SourceType == CurrentCollector )
                    || ( vehicle->MoverParameters->TrainType == dt_EZT ) ) {
                    uitextline3 +=
                        "; pant. " + to_string( vehicle->MoverParameters->PantPress, 2 )
                        + ( vehicle->MoverParameters->bPantKurek3 ? "=" : "^" ) + "ZG";
                }

                // McZapkie: komenda i jej parametry
                if( vehicle->MoverParameters->CommandIn.Command != ( "" ) ) {
                    uitextline4 =
                        "C:" + vehicle->MoverParameters->CommandIn.Command
                        + " V1=" + to_string( vehicle->MoverParameters->CommandIn.Value1, 0 )
                        + " V2=" + to_string( vehicle->MoverParameters->CommandIn.Value2, 0 );
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
                if( vehicle->MoverParameters->EngineType == ElectricInductionMotor ) {

                    UITable->text_lines.emplace_back( "      eimc:            eimv:            press:", Global.UITextColor );
                    for( int i = 0; i <= 20; ++i ) {

                        std::string parameters =
                            vehicle->MoverParameters->eimc_labels[ i ] + to_string( vehicle->MoverParameters->eimc[ i ], 2, 9 )
                            + " | "
                            + vehicle->MoverParameters->eimv_labels[ i ] + to_string( vehicle->MoverParameters->eimv[ i ], 2, 9 );

                        if( i < 10 ) {
                            parameters += " | " + train->fPress_labels[i] + to_string( train->fPress[ i ][ 0 ], 2, 9 );
                        }
                        else if( i == 12 ) {
                            parameters += "        med:";
                        }
                        else if( i >= 13 ) {
                            parameters += " | " + vehicle->MED_labels[ i - 13 ] + to_string( vehicle->MED[ 0 ][ i - 13 ], 2, 9 );
                        }

                        UITable->text_lines.emplace_back( parameters, Global.UITextColor );
                    }
                }
				if (vehicle->MoverParameters->EngineType == DieselEngine) {
					std::string parameters = "param       value";
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "efill: " + to_string(vehicle->MoverParameters->dizel_fill, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "etorq: " + to_string(vehicle->MoverParameters->dizel_Torque, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "creal: " + to_string(vehicle->MoverParameters->dizel_engage, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "cdesi: " + to_string(vehicle->MoverParameters->dizel_engagestate, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "cdelt: " + to_string(vehicle->MoverParameters->dizel_engagedeltaomega, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "gears: " + to_string(vehicle->MoverParameters->dizel_automaticgearstatus, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hydro      value";
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCnI: " + to_string(vehicle->MoverParameters->hydro_TC_nIn, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCnO: " + to_string(vehicle->MoverParameters->hydro_TC_nOut, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTM: " + to_string(vehicle->MoverParameters->hydro_TC_TMRatio, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTI: " + to_string(vehicle->MoverParameters->hydro_TC_TorqueIn, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCTO: " + to_string(vehicle->MoverParameters->hydro_TC_TorqueOut, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCfl: " + to_string(vehicle->MoverParameters->hydro_TC_Fill, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					parameters = "hTCLR: " + to_string(vehicle->MoverParameters->hydro_TC_LockupRate, 2, 9);
					UITable->text_lines.emplace_back(parameters, Global.UITextColor);
					//parameters = "hTCXX: " + to_string(vehicle->MoverParameters->hydro_TC_nIn, 2, 9);
					//UITable->text_lines.emplace_back(parameters, Global.UITextColor);
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
    auto &headerdata = UIHeader->text_lines;
    headerdata[ 0 ].data = uitextline1;
    headerdata[ 1 ].data = uitextline2;
    headerdata[ 2 ].data = uitextline3;
    headerdata[ 3 ].data = uitextline4;

    // stenogramy dźwięków (ukryć, gdy tabelka skanowania lub rozkład?)
    auto &transcripts = UITranscripts->text_lines;
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

void
ui_layer::render() {

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0, std::max( 1, Global.iWindowWidth ), std::max( 1, Global.iWindowHeight ), 0, -1, 1 );

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );
	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_ALPHA_TEST );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );

    ::glColor4fv( glm::value_ptr( colors::white ) );

    // render code here
    render_background();
    render_texture();

    glDisable( GL_TEXTURE_2D );
    glDisable( GL_TEXTURE_CUBE_MAP );

    render_progress();

    glDisable( GL_BLEND );

    render_panels();
    render_tooltip();

	glPopAttrib();
}

void
ui_layer::set_progress( float const Progress, float const Subtaskprogress ) {

    m_progress = Progress * 0.01f;
    m_subtaskprogress = Subtaskprogress * 0.01f;
}

void
ui_layer::set_background( std::string const &Filename ) {

    if( false == Filename.empty() ) {
        m_background = GfxRenderer.Fetch_Texture( Filename );
    }
    else {
        m_background = null_handle;
    }
    if( m_background != null_handle ) {
        auto const &texture = GfxRenderer.Texture( m_background );
        m_progressbottom = ( texture.width() != texture.height() );
    }
}

void
ui_layer::render_progress() {

	if( (m_progress == 0.0f) && (m_subtaskprogress == 0.0f) ) return;

    glm::vec2 origin, size;
    if( m_progressbottom == true ) {
        origin = glm::vec2{ 0.0f, 768.0f - 20.0f };
        size   = glm::vec2{ 1024.0f, 20.0f };
    }
    else {
        origin = glm::vec2{ 75.0f, 640.0f };
        size   = glm::vec2{ 320.0f, 16.0f };
    }

    quad( glm::vec4( origin.x, origin.y, origin.x + size.x, origin.y + size.y ), glm::vec4(0.0f, 0.0f, 0.0f, 0.25f) );
    // secondary bar
    if( m_subtaskprogress ) {
        quad(
            glm::vec4( origin.x, origin.y, origin.x + size.x * m_subtaskprogress, origin.y + size.y),
            glm::vec4( 8.0f/255.0f, 160.0f/255.0f, 8.0f/255.0f, 0.35f ) );
    }
    // primary bar
	if( m_progress ) {
        quad(
            glm::vec4( origin.x, origin.y, origin.x + size.x * m_progress, origin.y + size.y ),
            glm::vec4( 8.0f / 255.0f, 160.0f / 255.0f, 8.0f / 255.0f, 1.0f ) );
    }

    if( false == m_progresstext.empty() ) {
        float const screenratio = static_cast<float>( Global.iWindowWidth ) / Global.iWindowHeight;
        float const width =
            ( screenratio >= (4.0f/3.0f) ?
                ( 4.0f / 3.0f ) * Global.iWindowHeight :
                Global.iWindowWidth );
        float const heightratio =
            ( screenratio >= ( 4.0f / 3.0f ) ?
                Global.iWindowHeight / 768.f :
                Global.iWindowHeight / 768.f * screenratio / ( 4.0f / 3.0f ) );
        float const height = 768.0f * heightratio;

        ::glColor4f( 216.0f / 255.0f, 216.0f / 255.0f, 216.0f / 255.0f, 1.0f );
        auto const charsize = 9.0f;
        auto const textwidth = m_progresstext.size() * charsize;
        auto const textheight = 12.0f;
        ::glRasterPos2f(
            ( 0.5f * ( Global.iWindowWidth  - width )  + origin.x * heightratio ) + ( ( size.x * heightratio - textwidth ) * 0.5f * heightratio ),
            ( 0.5f * ( Global.iWindowHeight - height ) + origin.y * heightratio ) + ( charsize ) + ( ( size.y * heightratio - textheight ) * 0.5f * heightratio ) );
        print( m_progresstext );
    }
}

void
ui_layer::render_panels() {

    if( m_panels.empty() ) { return; }

    float const width = std::min( 4.f / 3.f, static_cast<float>(Global.iWindowWidth) / std::max( 1, Global.iWindowHeight ) ) * Global.iWindowHeight;
    float const height = Global.iWindowHeight / 768.f;

    for( auto const &panel : m_panels ) {

        int lineidx = 0;
        for( auto const &line : panel->text_lines ) {

            ::glColor4fv( glm::value_ptr( line.color ) );
            ::glRasterPos2f(
                0.5f * ( Global.iWindowWidth - width ) + panel->origin_x * height,
                panel->origin_y * height + 20.f * lineidx );
            print( line.data );
            ++lineidx;
        }
    }
}

void
ui_layer::render_tooltip() {

    if( m_tooltip.empty() ) { return; }

    glm::dvec2 mousepos;
    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );

    ::glColor4fv( glm::value_ptr( colors::white ) );
    ::glRasterPos2f( mousepos.x + 20.0f, mousepos.y + 25.0f );
    print( m_tooltip );
}

void
ui_layer::render_background() {

	if( m_background == 0 ) return;
    // NOTE: we limit/expect the background to come with 4:3 ratio.
    // TODO, TBD: if we expose texture width or ratio from texture object, this limitation could be lifted
    GfxRenderer.Bind_Texture( m_background );
    auto const height { 768.0f };
    auto const &texture = GfxRenderer.Texture( m_background );
    float const width = (
        texture.width() == texture.height() ?
            1024.0f : // legacy mode, square texture displayed as 4:3 image
            texture.width() / ( texture.height() / 768.0f ) );
    quad(
        glm::vec4(
            ( 1024.0f * 0.5f ) - ( width  * 0.5f ),
            (  768.0f * 0.5f ) - ( height * 0.5f ),
            ( 1024.0f * 0.5f ) - ( width  * 0.5f ) + width,
            (  768.0f * 0.5f ) - ( height * 0.5f ) + height ),
        colors::white );
}

void
ui_layer::render_texture() {

    if( m_texture != 0 ) {
        ::glColor4fv( glm::value_ptr( colors::white ) );

        GfxRenderer.Bind_Texture( null_handle );
        ::glBindTexture( GL_TEXTURE_2D, m_texture );

        auto const size = 512.f;
        auto const offset = 64.f;

        glBegin( GL_TRIANGLE_STRIP );

        glMultiTexCoord2f( m_textureunit, 0.f, 1.f ); glVertex2f( offset, Global.iWindowHeight - offset - size );
        glMultiTexCoord2f( m_textureunit, 0.f, 0.f ); glVertex2f( offset, Global.iWindowHeight - offset );
        glMultiTexCoord2f( m_textureunit, 1.f, 1.f ); glVertex2f( offset + size, Global.iWindowHeight - offset - size );
        glMultiTexCoord2f( m_textureunit, 1.f, 0.f ); glVertex2f( offset + size, Global.iWindowHeight - offset );

        glEnd();

        ::glBindTexture( GL_TEXTURE_2D, 0 );
    }
}

void
ui_layer::print( std::string const &Text )
{
    if( (false == Global.DLFont)
     || (true == Text.empty()) )
        return;
    
    ::glPushAttrib( GL_LIST_BIT );

    ::glListBase( m_fontbase - 32 );
    ::glCallLists( Text.size(), GL_UNSIGNED_BYTE, Text.c_str() );

    ::glPopAttrib();
}

void
ui_layer::quad( glm::vec4 const &Coordinates, glm::vec4 const &Color ) {

    float const screenratio = static_cast<float>( Global.iWindowWidth ) / Global.iWindowHeight;
    float const width =
        ( screenratio >= ( 4.f / 3.f ) ?
            ( 4.f / 3.f ) * Global.iWindowHeight :
            Global.iWindowWidth );
    float const heightratio =
        ( screenratio >= ( 4.f / 3.f ) ?
            Global.iWindowHeight / 768.f :
            Global.iWindowHeight / 768.f * screenratio / ( 4.f / 3.f ) );
    float const height = 768.f * heightratio;

    glColor4fv(glm::value_ptr(Color));

    glBegin( GL_TRIANGLE_STRIP );

    glMultiTexCoord2f( m_textureunit, 0.f, 1.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.x * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.y * heightratio );
    glMultiTexCoord2f( m_textureunit, 0.f, 0.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.x * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.w * heightratio );
    glMultiTexCoord2f( m_textureunit, 1.f, 1.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.z * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.y * heightratio );
    glMultiTexCoord2f( m_textureunit, 1.f, 0.f ); glVertex2f( 0.5f * ( Global.iWindowWidth - width ) + Coordinates.z * heightratio, 0.5f * ( Global.iWindowHeight - height ) + Coordinates.w * heightratio );

    glEnd();
}
