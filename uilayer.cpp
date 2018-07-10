#include "stdafx.h"
#include "uilayer.h"
#include "uitranscripts.h"

#include "Globals.h"
#include "translation.h"
#include "simulation.h"
#include "mtable.h"
#include "Train.h"
#include "DynObj.h"
#include "Model3d.h"
#include "renderer.h"
#include "Timer.h"
#include "utilities.h"
#include "Logs.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

extern bool take_screenshot;
ui_layer UILayer;

ui_layer::~ui_layer() {

}

bool
ui_layer::init( GLFWwindow *Window ) {
    m_window = Window;
    log_active = Global.loading_log;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_io = &ImGui::GetIO();
    //imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    imgui_io->Fonts->AddFontFromFileTTF("DejaVuSansMono.ttf", 13.0f);

    ImGui_ImplGlfw_InitForOpenGL(m_window, false);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsClassic();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void ui_layer::cleanup()
{
    ImGui::EndFrame();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool ui_layer::key_callback(int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(m_window, key, scancode, action, mods);
    return imgui_io->WantCaptureKeyboard;
}

bool ui_layer::char_callback(unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(m_window, c);
    return imgui_io->WantCaptureKeyboard;
}

bool ui_layer::scroll_callback(double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(m_window, xoffset, yoffset);
    return imgui_io->WantCaptureMouse;
}

bool ui_layer::mouse_button_callback(int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(m_window, button, action, mods);
    return imgui_io->WantCaptureMouse;
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
        case GLFW_KEY_F11:
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
            m_f1active = !m_f1active;
            return true;
        }

        case GLFW_KEY_F2: {
            // parametry pojazdu
            m_f2active = !m_f2active;
            return true;
        }

        case GLFW_KEY_F3: {
            // timetable
            m_f3active = !m_f3active;
            return true;
        }

        case GLFW_KEY_F8: {
            // renderer debug data
            m_f8active = !m_f8active;
            return true;
        }

        case GLFW_KEY_F9: {
            // wersja
            m_f9active = !m_f9active;
            return true;
        }

        case GLFW_KEY_F10: {
            // quit
            m_f10active = !m_f10active;
            return true;
        }

        case GLFW_KEY_F11: {
            // scenario inspector
            m_f11active = !m_f11active;
            return true;
        }

        case GLFW_KEY_F12: {
            log_active = !log_active;
            return true;
        }

        default: {
            break;
        }
    }

    return false;
}

void
ui_layer::render() {
    // render code here
    Timer::subsystem.gfx_gui.start();

    render_background();

    render_progress();

    render_tooltip();

    //ImGui::ShowDemoWindow();

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

    if (m_f1active)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Vehicle info", &m_f1active, ImGuiWindowFlags_NoResize);

        auto const &time = simulation::Time.data();
        uitextline1 =
            "Time: "
            + to_string( time.wHour ) + ":"
            + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
            + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );
        if( Global.iPause ) {
            uitextline1 += " (paused)";
        }
        ImGui::TextUnformatted(uitextline1.c_str());

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

            static bool detail = false;
            ImGui::Checkbox("More", &detail);
            if (detail) {
                auto const speedlimit { static_cast<int>( std::floor( driver->VelDesired ) ) };
                uitextline2 +=
                    " Speed: " + std::to_string( static_cast<int>( std::floor( mover->Vel ) ) ) + " km/h"
                    + " (limit: " + std::to_string( speedlimit ) + " km/h";
                auto const nextspeedlimit { static_cast<int>( std::floor( controlled->Mechanik->VelNext ) ) };
                if( nextspeedlimit != speedlimit ) {
                    uitextline2 +=
                        ", new limit: " + std::to_string( nextspeedlimit ) + " km/h"
                        + " in " + to_string( controlled->Mechanik->ActualProximityDist * 0.001, 1 ) + " km";
                }
                uitextline2 += ")";
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

            ImGui::TextUnformatted(uitextline2.c_str());
            ImGui::TextUnformatted(uitextline3.c_str());
            ImGui::TextUnformatted(uitextline4.c_str());
        }

        ImGui::End();
    }

    if (m_f2active)
    {
        // timetable
        auto *vehicle {
            ( FreeFlyModeFlag ?
                std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera->Pos, 20, false, false ) ) :
                controlled ) }; // w trybie latania lokalizujemy wg mapy

        if( vehicle == nullptr )
            goto f2_cancel;
        // if the nearest located vehicle doesn't have a direct driver, try to query its owner
        auto const owner = (
            ( ( vehicle->Mechanik != nullptr ) && ( vehicle->Mechanik->Primary() ) ) ?
                vehicle->Mechanik :
                vehicle->ctOwner );
        if( owner == nullptr )
            goto f2_cancel;

        auto const *table = owner->TrainTimetable();
        if( table == nullptr )
            goto f2_cancel;

        auto const &time = simulation::Time.data();
        uitextline1 =
            "Time: "
            + to_string( time.wHour ) + ":"
            + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
            + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );
        if( Global.iPause ) {
            uitextline1 += " (paused)";
        }

        uitextline2 = std::string( owner->Relation() ) + " (" + std::string( owner->TrainName() ) + ")";
        auto const nextstation = std::string( owner->NextStop() );
        uitextline3 = "";
        if( !nextstation.empty() ) {
            // jeśli jest podana relacja, to dodajemy punkt następnego zatrzymania
            uitextline3 = " -> " + nextstation;
        }

        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Timetable", &m_f2active, ImGuiWindowFlags_NoResize);
        ImGui::TextUnformatted(uitextline1.c_str());
        ImGui::TextUnformatted(uitextline2.c_str());
        ImGui::TextUnformatted(uitextline3.c_str());

        if( table->StationCount > 0)
        {
            static bool show = false;
            ImGui::Checkbox("show", &show);
            if (show)
            {
                // header
                ImGui::TextUnformatted("+-----+------------------------------------+-------+-----+");

                TMTableLine const *tableline;
                for( int i = owner->iStationStart; i <= table->StationCount; ++i ) {
                    // wyświetlenie pozycji z rozkładu
                    tableline = table->TimeTable + i; // linijka rozkładu

                    std::string vmax =
                        "   "
                        + to_string( tableline->vmax, 0 );
                    vmax = vmax.substr( vmax.size() - 3, 3 ); // z wyrównaniem do prawej
                    std::string const station = (
                        std::string( tableline->StationName )
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

                    uitextline1 = "| " + vmax + " | " + station + " | " + arrival + " | " + traveltime + " |";
                    uitextline2 = "|     | " + location + tableline->StationWare + " | " + departure + " |     |";

                    if (candeparture)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

                    ImGui::TextUnformatted(uitextline1.c_str());
                    ImGui::TextUnformatted(uitextline2.c_str());

                    if (candeparture)
                        ImGui::PopStyleColor();

                    // divider/footer
                    ImGui::TextUnformatted("+-----+------------------------------------+-------+-----+");
                }
            }
        }

        ImGui::End();
    }

    f2_cancel:;

    if (m_f3active)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Vehicle status", &m_f3active, ImGuiWindowFlags_NoResize);

        uitextline1 = "";
        uitextline2 = "";
        uitextline3 = "";
        uitextline4 = "";

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
                    uitextline4 += " (" + ( vehicle->Mechanik->eSignNext->asName ) + ")";
                }

                // biezaca komenda dla AI
                uitextline4 += ", command: " + vehicle->Mechanik->OrderCurrent();
            }

            static bool scantable = false;
            if (scantable)
            {
                // f2 screen, track scan mode
                if( vehicle->Mechanik)
                {
                    ImGui::SetNextWindowSize(ImVec2(0, 0));
                    ImGui::Begin("Scan table", &scantable, ImGuiWindowFlags_NoResize);
                    std::size_t i = 0; std::size_t const speedtablesize = clamp( static_cast<int>( vehicle->Mechanik->TableSize() ) - 1, 0, 30 );
                    do {
                        std::string scanline = vehicle->Mechanik->TableText( i );
                        if( scanline.empty() ) { break; }
                        ImGui::TextUnformatted(scanline.c_str());
                        ++i;
                    } while( i < speedtablesize ); // TController:iSpeedTableSize TODO: change when the table gets recoded
                    ImGui::End();
                }
            }

            ImGui::TextUnformatted(uitextline1.c_str());
            ImGui::TextUnformatted(uitextline2.c_str());
            ImGui::TextUnformatted(uitextline3.c_str());
            ImGui::TextUnformatted(uitextline4.c_str());
            ImGui::Checkbox("Show scan table", &scantable);
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

            ImGui::TextUnformatted(uitextline1.c_str());
            ImGui::TextUnformatted(uitextline2.c_str());
        }
        ImGui::End();
    }

    if (m_f8active)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Renderer stats", &m_f8active, ImGuiWindowFlags_NoResize);
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
            std::string( "Rendering mode: VBO" );
        if( false == Global.LastGLError.empty() ) {
            uitextline2 +=
                "Last openGL error: "
                + Global.LastGLError;
        }
        // renderer stats
        uitextline3 = GfxRenderer.info_times();
        uitextline4 = GfxRenderer.info_stats();

        ImGui::TextUnformatted(uitextline1.c_str());
        ImGui::TextUnformatted(uitextline2.c_str());
        ImGui::TextUnformatted(uitextline3.c_str());
        ImGui::TextUnformatted(uitextline4.c_str());
        ImGui::End();
    }

    if (m_f9active)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("About", &m_f9active, ImGuiWindowFlags_NoResize);

        // informacja o wersji
        uitextline1 = "MaSzyna " + Global.asVersion; // informacja o wersji
        if( Global.iMultiplayer ) {
            uitextline1 += " (multiplayer mode is active)";
        }
        uitextline3 =
              "vehicles: " + to_string( Timer::subsystem.sim_dynamics.average(), 2 ) + " msec"
            + " update total: " + to_string( Timer::subsystem.sim_total.average(), 2 ) + " msec";

        ImGui::TextUnformatted(uitextline1.c_str());
        ImGui::TextUnformatted(uitextline3.c_str());
        ImGui::End();
    }

    if (m_f10active)
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Quit", &m_f10active, ImGuiWindowFlags_NoResize);
        ImGui::TextUnformatted("Quit simulation?");
        if (ImGui::Button("Yes"))
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        ImGui::SameLine();
        if (ImGui::Button("No"))
            m_f10active = false;
        ImGui::End();
    }

    if (m_f11active)
    {
        // scenario inspector
        auto const time { Timer::GetTime() };
        auto const *event { simulation::Events.begin() };
        if (event == nullptr)
        {
            m_f11active = false;
            goto f11_cancel;
        }

        ImGui::Begin("Inspector", &m_f11active);

        auto eventtableindex{ 0 };
        while( ( event != nullptr )) {

            if( ( false == event->m_ignored )
             && ( true == event->bEnabled ) ) {

                auto const delay { "   " + to_string( std::max( 0.0, event->fStartTime - time ), 1 ) };
                auto const eventline =
                    "Delay: " + delay.substr( delay.length() - 6 )
                    + ", Event: " + event->asName
                    + ( event->Activator ? " (by: " + event->Activator->asName + ")" : "" )
                    + ( event->evJoined ? " (joint event)" : "" );

                ImGui::TextUnformatted(eventline.c_str());
                ++eventtableindex;
            }
            event = event->evNext;
        }

        ImGui::End();
    }

    f11_cancel:;

    auto *vehicle {
        ( FreeFlyModeFlag ?
            std::get<TDynamicObject *>( simulation::Region->find_vehicle( camera->Pos, 20, false, false ) ) :
            controlled ) }; // w trybie latania lokalizujemy wg mapy

    if( DebugModeFlag && vehicle)
    {
        uitextline1 =
            "vel: " + to_string( vehicle->GetVelocity(), 2 ) + "/" + to_string( vehicle->MoverParameters->nrot* M_PI * vehicle->MoverParameters->WheelDiameter * 3.6, 2 )
            + " km/h;" + ( vehicle->MoverParameters->SlippingWheels ? " (!)" : "    " )
            + " dist: " + to_string( vehicle->MoverParameters->DistCounter, 2 ) + " km"
            + "; pos: [" + to_string( vehicle->GetPosition().x, 2 ) + ", " + to_string( vehicle->GetPosition().y, 2 ) + ", " + to_string( vehicle->GetPosition().z, 2 ) + "]"
            + ", PM=" + to_string( vehicle->MoverParameters->WheelFlat, 1 )
            + " mm; enpwr=" + to_string( vehicle->MoverParameters->EnginePower, 1 )
            + "; enrot=" + to_string( vehicle->MoverParameters->enrot * 60, 0 )
            + " tmrot=" + to_string( std::abs( vehicle->MoverParameters->nrot ) * vehicle->MoverParameters->Transmision.Ratio * 60, 0 )
            + "; ventrot=" + to_string( vehicle->MoverParameters->RventRot * 60, 1 )
            + "; fanrot=" + to_string( vehicle->MoverParameters->dizel_heat.rpmw, 1 ) + ", " + to_string( vehicle->MoverParameters->dizel_heat.rpmw2, 1 );

        uitextline2 =
            "HamZ=" + to_string( vehicle->MoverParameters->fBrakeCtrlPos, 2 )
            + "; HamP=" + std::to_string( vehicle->MoverParameters->LocalBrakePos ) + "/" + to_string( vehicle->MoverParameters->LocalBrakePosA, 2 )
            + "; NasJ=" + std::to_string( vehicle->MoverParameters->MainCtrlPos ) + "(" + std::to_string( vehicle->MoverParameters->MainCtrlActualPos ) + ")"
            + ( ( vehicle->MoverParameters->ShuntMode && vehicle->MoverParameters->EngineType == DieselElectric ) ?
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

        uitextline4 = "";

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

        std::list<std::string> table;

        // induction motor data
        if( vehicle->MoverParameters->EngineType == ElectricInductionMotor ) {

            table.emplace_back( "      eimc:            eimv:            press:" );
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

                table.emplace_back( parameters);
            }
        }
        if (vehicle->MoverParameters->EngineType == DieselEngine) {
            std::string parameters = "param       value";
            table.emplace_back(parameters);
            parameters = "efill: " + to_string(vehicle->MoverParameters->dizel_fill, 2, 9);
            table.emplace_back(parameters);
            parameters = "etorq: " + to_string(vehicle->MoverParameters->dizel_Torque, 2, 9);
            table.emplace_back(parameters);
            parameters = "creal: " + to_string(vehicle->MoverParameters->dizel_engage, 2, 9);
            table.emplace_back(parameters);
            parameters = "cdesi: " + to_string(vehicle->MoverParameters->dizel_engagestate, 2, 9);
            table.emplace_back(parameters);
            parameters = "cdelt: " + to_string(vehicle->MoverParameters->dizel_engagedeltaomega, 2, 9);
            table.emplace_back(parameters);
            parameters = "gears: " + to_string(vehicle->MoverParameters->dizel_automaticgearstatus, 2, 9);
            table.emplace_back(parameters);
            parameters = "hydro      value";
            table.emplace_back(parameters);
            parameters = "hTCnI: " + to_string(vehicle->MoverParameters->hydro_TC_nIn, 2, 9);
            table.emplace_back(parameters);
            parameters = "hTCnO: " + to_string(vehicle->MoverParameters->hydro_TC_nOut, 2, 9);
            table.emplace_back(parameters);
            parameters = "hTCTM: " + to_string(vehicle->MoverParameters->hydro_TC_TMRatio, 2, 9);
            table.emplace_back(parameters);
            parameters = "hTCTI: " + to_string(vehicle->MoverParameters->hydro_TC_TorqueIn, 2, 9);
            table.emplace_back(parameters);
            parameters = "hTCTO: " + to_string(vehicle->MoverParameters->hydro_TC_TorqueOut, 2, 9);
            table.emplace_back(parameters);
            parameters = "hTCfl: " + to_string(vehicle->MoverParameters->hydro_TC_Fill, 2, 9);
            table.emplace_back(parameters);
            parameters = "hTCLR: " + to_string(vehicle->MoverParameters->hydro_TC_LockupRate, 2, 9);
            table.emplace_back(parameters);
            //parameters = "hTCXX: " + to_string(vehicle->MoverParameters->hydro_TC_nIn, 2, 9);
            //table.emplace_back(parameters);
        }
        ImGui::Begin("Debug", &DebugModeFlag, ImGuiWindowFlags_NoResize);
        ImGui::TextUnformatted(uitextline1.c_str());
        ImGui::TextUnformatted(uitextline2.c_str());
        ImGui::TextUnformatted(uitextline3.c_str());
        ImGui::TextUnformatted(uitextline4.c_str());
        for (const std::string &str : table)
            ImGui::TextUnformatted(str.c_str());
        ImGui::End();
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

    // stenogramy dźwięków (ukryć, gdy tabelka skanowania lub rozkład?)
    std::vector<std::string> transcripts;
    for( auto const &transcript : ui::Transcripts.aLines ) {

        if( Global.fTimeAngleDeg >= transcript.fShow ) {

            cParser parser( transcript.asText );
            while( true == parser.getTokens( 1, false, "|" ) ) {

                std::string transcriptline; parser >> transcriptline;
                transcripts.emplace_back( transcriptline );
            }
        }
    }

    if (!transcripts.empty())
    {
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("Transcript", nullptr, ImGuiWindowFlags_NoResize);
        for (auto const &s : transcripts)
            ImGui::TextUnformatted(s.c_str());
        ImGui::End();
    }

    if (log_active)
    {
        ImGui::SetNextWindowSize(ImVec2(700, 400));
        if (m_progress != 0.0f)
            ImGui::SetNextWindowPos(ImVec2(50, 150));
        ImGui::Begin("Log", &log_active, ImGuiWindowFlags_NoResize);
        for (const std::string &s : log)
            ImGui::TextUnformatted(s.c_str());
        if (ImGui::GetScrollY() == ImGui::GetScrollMaxY())
            ImGui::SetScrollHere(1.0f);
        ImGui::End();
    }

    glm::dvec2 mousepos;
    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );

    if (((Global.ControlPicking && mousepos.y < 50.0f) || imgui_io->WantCaptureMouse) && m_progress == 0.0f)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Scenery"))
            {
                ImGui::MenuItem("Debug mode", nullptr, &DebugModeFlag);
                ImGui::MenuItem("Quit", "F10", &m_f10active);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("Basic info", "F1", &m_f1active);
                ImGui::MenuItem("Timetable", "F2", &m_f2active);
                ImGui::MenuItem("Vehicle info", "F3", &m_f3active);
                ImGui::MenuItem("Renderer stats", "F8", &m_f8active);
                ImGui::MenuItem("Scenery inspector", "F11", &m_f11active);
                ImGui::MenuItem("Log", "F12", &log_active);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
                static bool log = Global.iWriteLogEnabled & 1;

                ImGui::MenuItem("Logging to log.txt", nullptr, &log);
                if (log)
                    Global.iWriteLogEnabled |= 1;
                else
                    Global.iWriteLogEnabled &= ~1;

                if (ImGui::MenuItem("Screenshot", "PrtScr"))
                    take_screenshot = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::MenuItem("About", "F9", &m_f9active);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    glDebug("uilayer render");

    ImGui::Render();
    Timer::subsystem.gfx_gui.stop();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glDebug("uilayer render done");
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
    }
}

void
ui_layer::render_progress() {
    if( (m_progress == 0.0f) && (m_subtaskprogress == 0.0f) ) return;

    ImGui::SetNextWindowPos(ImVec2(50, 50));
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin("Progress", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    if (!m_progresstext.empty())
        ImGui::ProgressBar(m_progress, ImVec2(300, 0), m_progresstext.c_str());
    else
        ImGui::ProgressBar(m_progress, ImVec2(300, 0));
    ImGui::ProgressBar(m_subtaskprogress, ImVec2(300, 0));
    ImGui::End();
}

void
ui_layer::render_tooltip() {

    if( m_tooltip.empty() ) { return; }

    glm::dvec2 mousepos;
    glfwGetCursorPos( m_window, &mousepos.x, &mousepos.y );

    ImGui::SetNextWindowPos(ImVec2(mousepos.x, mousepos.y));
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin("Tooltip", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::TextUnformatted(m_tooltip.c_str());
    ImGui::End();
}

void
ui_layer::render_background() {

	if( m_background == 0 ) return;

    ImVec2 size = ImGui::GetIO().DisplaySize;
    opengl_texture &tex = GfxRenderer.Texture(m_background);
    tex.create();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("Logo", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Image(reinterpret_cast<void*>(tex.id), size, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}
