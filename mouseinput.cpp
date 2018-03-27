/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "mouseinput.h"
#include "Globals.h"
#include "Timer.h"
#include "World.h"
#include "Train.h"
#include "utilities.h"
#include "renderer.h"

extern TWorld World;

bool
mouse_input::init() {

#ifdef _WIN32
    DWORD systemkeyboardspeed;
    ::SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, &systemkeyboardspeed, 0 );
    m_updaterate = interpolate( 0.5, 0.04, systemkeyboardspeed / 31.0 );
    DWORD systemkeyboarddelay;
    ::SystemParametersInfo( SPI_GETKEYBOARDDELAY, 0, &systemkeyboarddelay, 0 );
    m_updatedelay = interpolate( 0.25, 1.0, systemkeyboarddelay / 3.0 );
#endif
    return true;
}

void
mouse_input::move( double Mousex, double Mousey ) {

    if( false == Global.ControlPicking ) {
        // default control mode
        m_relay.post(
            user_command::viewturn,
            reinterpret_cast<std::uint64_t const &>( Mousex ),
            reinterpret_cast<std::uint64_t const &>( Mousey ),
            GLFW_PRESS,
            // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
            // TODO: pass correct entity id once the missing systems are in place
            0 );
    }
    else {
        // control picking mode
        if( false == m_pickmodepanning ) {
            // even if the view panning isn't active we capture the cursor position in case it does get activated
            m_cursorposition.x = Mousex;
            m_cursorposition.y = Mousey;
            return;
        }
        glm::dvec2 cursorposition { Mousex, Mousey };
        auto const viewoffset = cursorposition - m_cursorposition;
        m_relay.post(
            user_command::viewturn,
            reinterpret_cast<std::uint64_t const &>( viewoffset.x ),
            reinterpret_cast<std::uint64_t const &>( viewoffset.y ),
            GLFW_PRESS,
            // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
            // TODO: pass correct entity id once the missing systems are in place
            0 );
        m_cursorposition = cursorposition;
    }
}

void
mouse_input::button( int const Button, int const Action ) {

    if( false == Global.ControlPicking ) { return; }

    if( true == FreeFlyModeFlag ) {
        // world editor controls
        // currently we only handle view panning in this mode
        if( Action == GLFW_RELEASE ) {
            // if it's the right mouse button that got released we were potentially in view panning mode; stop it
            if( Button == GLFW_MOUSE_BUTTON_RIGHT ) {
                m_pickmodepanning = false;
            }
        }
        else {
            // button press
            if( Button == GLFW_MOUSE_BUTTON_RIGHT ) {
                // the right button activates mouse panning mode
                m_pickmodepanning = true;
            }
        }
    }
    else {
        // cab controls mode
        user_command &mousecommand = (
            Button == GLFW_MOUSE_BUTTON_LEFT ?
                m_mousecommandleft :
                m_mousecommandright
            );

        if( Action == GLFW_RELEASE ) {
            if( mousecommand != user_command::none ) {
                // NOTE: basic keyboard controls don't have any parameters
                // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
                // TODO: pass correct entity id once the missing systems are in place
                m_relay.post( mousecommand, 0, 0, Action, 0 );
                mousecommand = user_command::none;
            }
            else {
                // if it's the right mouse button that got released and we had no command active, we were potentially in view panning mode; stop it
                if( Button == GLFW_MOUSE_BUTTON_RIGHT ) {
                    m_pickmodepanning = false;
                }
            }
            // if we were in varying command repeat rate, we can stop that now. done without any conditions, to catch some unforeseen edge cases
            m_varyingpollrate = false;
        }
        else {
            // if not release then it's press
            auto train = World.train();
            if( train != nullptr ) {
                auto lookup = m_mousecommands.find( train->GetLabel( GfxRenderer.Update_Pick_Control() ) );
                if( lookup != m_mousecommands.end() ) {
                    mousecommand = (
                        Button == GLFW_MOUSE_BUTTON_LEFT ?
                            lookup->second.left :
                            lookup->second.right
                        );
                    if( mousecommand != user_command::none ) {
                        // check manually for commands which have 'fast' variants launched with shift modifier
                        if( Global.shiftState ) {
                            switch( mousecommand ) {
                                case user_command::mastercontrollerincrease: { mousecommand = user_command::mastercontrollerincreasefast; break; }
                                case user_command::mastercontrollerdecrease: { mousecommand = user_command::mastercontrollerdecreasefast; break; }
                                case user_command::secondcontrollerincrease: { mousecommand = user_command::secondcontrollerincreasefast; break; }
                                case user_command::secondcontrollerdecrease: { mousecommand = user_command::secondcontrollerdecreasefast; break; }
                                case user_command::independentbrakeincrease: { mousecommand = user_command::independentbrakeincreasefast; break; }
                                case user_command::independentbrakedecrease: { mousecommand = user_command::independentbrakedecreasefast; break; }
                                default: { break; }
                            }
                        }
                        // NOTE: basic keyboard controls don't have any parameters
                        // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
                        // TODO: pass correct entity id once the missing systems are in place
                        m_relay.post( mousecommand, 0, 0, Action, 0 );
                        m_updateaccumulator = -0.25; // prevent potential command repeat right after issuing one

                        switch( mousecommand ) {
                            case user_command::mastercontrollerincrease:
                            case user_command::mastercontrollerdecrease:
                            case user_command::secondcontrollerincrease:
                            case user_command::secondcontrollerdecrease:
                            case user_command::trainbrakeincrease:
                            case user_command::trainbrakedecrease:
                            case user_command::independentbrakeincrease:
                            case user_command::independentbrakedecrease: {
                                // these commands trigger varying repeat rate mode,
                                // which scales the rate based on the distance of the cursor from its point when the command was first issued
                                m_commandstartcursor = m_cursorposition;
                                m_varyingpollrate = true;
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                }
                else {
                    // if we don't have any recognized element under the cursor and the right button was pressed, enter view panning mode
                    if( Button == GLFW_MOUSE_BUTTON_RIGHT ) {
                        m_pickmodepanning = true;
                    }
                }
            }
        }
    }
}

void
mouse_input::poll() {

    m_updateaccumulator += Timer::GetDeltaRenderTime();

    auto updaterate { m_updaterate };
    if( m_varyingpollrate ) {
        updaterate /= std::max( 0.15, 2.0 * glm::length( m_cursorposition - m_commandstartcursor ) / std::max( 1, Global.iWindowHeight ) );
    }

    while( m_updateaccumulator > updaterate ) {

        if( m_mousecommandleft != user_command::none ) {
            // NOTE: basic keyboard controls don't have any parameters
            // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
            // TODO: pass correct entity id once the missing systems are in place
            m_relay.post( m_mousecommandleft, 0, 0, GLFW_REPEAT, 0 );
        }
        if( m_mousecommandright != user_command::none ) {
            // NOTE: basic keyboard controls don't have any parameters
            // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
            // TODO: pass correct entity id once the missing systems are in place
            m_relay.post( m_mousecommandright, 0, 0, GLFW_REPEAT, 0 );
        }
        m_updateaccumulator -= updaterate;
    }
}

void
mouse_input::default_bindings() {

    m_mousecommands = {
        { "mainctrl:", {
            user_command::mastercontrollerincrease,
            user_command::mastercontrollerdecrease } },
        { "scndctrl:", {
            user_command::secondcontrollerincrease,
            user_command::secondcontrollerdecrease } },
        { "dirkey:", {
            user_command::reverserincrease,
            user_command::reverserdecrease } },
        { "brakectrl:", {
            user_command::trainbrakeincrease,
            user_command::trainbrakedecrease } },
        { "localbrake:", {
            user_command::independentbrakeincrease,
            user_command::independentbrakedecrease } },
        { "manualbrake:", {
            user_command::manualbrakeincrease,
            user_command::manualbrakedecrease } },
        { "alarmchain:", {
            user_command::alarmchaintoggle,
            user_command::none } },
        { "brakeprofile_sw:", {
            user_command::brakeactingspeedincrease,
            user_command::brakeactingspeeddecrease } },
        // TODO: dedicated methods for braking speed switches
        { "brakeprofileg_sw:", {
            user_command::brakeactingspeedsetcargo,
            user_command::brakeactingspeedsetpassenger } },
        { "brakeprofiler_sw:", {
            user_command::brakeactingspeedsetrapid,
            user_command::brakeactingspeedsetpassenger } },
        { "maxcurrent_sw:", {
            user_command::motoroverloadrelaythresholdtoggle,
            user_command::none } },
        { "fuelpump_sw:", {
            user_command::fuelpumptoggle,
            user_command::none } },
        { "main_off_bt:", {
            user_command::linebreakeropen,
            user_command::none } },
        { "main_on_bt:",{
            user_command::linebreakerclose,
            user_command::none } },
        { "security_reset_bt:", {
            user_command::alerteracknowledge,
            user_command::none } },
        { "releaser_bt:", {
            user_command::independentbrakebailoff,
            user_command::none } },
        { "sand_bt:", {
            user_command::sandboxactivate,
            user_command::none } },
        { "antislip_bt:", {
            user_command::wheelspinbrakeactivate,
            user_command::none } },
        { "horn_bt:", {
            user_command::hornhighactivate,
            user_command::hornlowactivate } },
        { "hornlow_bt:", {
            user_command::hornlowactivate,
            user_command::none } },
        { "hornhigh_bt:", {
            user_command::hornhighactivate,
            user_command::none } },
        { "fuse_bt:", {
            user_command::motoroverloadrelayreset,
            user_command::none } },
        { "converterfuse_bt:", {
            user_command::converteroverloadrelayreset,
            user_command::none } },
        { "stlinoff_bt:", {
            user_command::motorconnectorsopen,
            user_command::none } },
        { "door_left_sw:", {
            user_command::doortoggleleft,
            user_command::none } },
        { "door_right_sw:", {
            user_command::doortoggleright,
            user_command::none } },
        { "departure_signal_bt:", {
            user_command::departureannounce,
            user_command::none } },
        { "upperlight_sw:", {
            user_command::headlighttoggleupper,
            user_command::none } },
        { "leftlight_sw:", {
            user_command::headlighttoggleleft,
            user_command::none } },
        { "rightlight_sw:", {
            user_command::headlighttoggleright,
            user_command::none } },
        { "dimheadlights_sw:", {
            user_command::headlightsdimtoggle,
            user_command::none } },
        { "leftend_sw:", {
            user_command::redmarkertoggleleft,
            user_command::none } },
        { "rightend_sw:", {
            user_command::redmarkertoggleright,
            user_command::none } },
        { "lights_sw:", {
            user_command::lightspresetactivatenext,
            user_command::lightspresetactivateprevious } },
        { "rearupperlight_sw:", {
            user_command::headlighttogglerearupper,
            user_command::none } },
        { "rearleftlight_sw:", {
            user_command::headlighttogglerearleft,
            user_command::none } },
        { "rearrightlight_sw:", {
            user_command::headlighttogglerearright,
            user_command::none } },
        { "rearleftend_sw:", {
            user_command::redmarkertogglerearleft,
            user_command::none } },
        { "rearrightend_sw:", {
            user_command::redmarkertogglerearright,
            user_command::none } },
        { "compressor_sw:", {
            user_command::compressortoggle,
            user_command::none } },
        { "compressorlocal_sw:", {
            user_command::compressortogglelocal,
            user_command::none } },
        { "converter_sw:", {
            user_command::convertertoggle,
            user_command::none } },
        { "converterlocal_sw:", {
            user_command::convertertogglelocal,
            user_command::none } },
        { "converteroff_sw:", {
            user_command::convertertoggle,
            user_command::none } }, // TODO: dedicated converter shutdown command
        { "main_sw:", {
            user_command::linebreakertoggle,
            user_command::none } },
        { "radio_sw:", {
            user_command::radiotoggle,
            user_command::none } },
        { "radiochannel_sw:", {
            user_command::radiochannelincrease,
            user_command::radiochanneldecrease } },
        { "radiochannelprev_sw:", {
            user_command::radiochanneldecrease,
            user_command::none } },
        { "radiochannelnext_sw:", {
            user_command::radiochannelincrease,
            user_command::none } },
        { "radiostop_sw:", {
            user_command::radiostopsend,
            user_command::none } },
        { "radiotest_sw:", {
            user_command::radiostoptest,
            user_command::none } },
        { "pantfront_sw:", {
            user_command::pantographtogglefront,
            user_command::none } },
        { "pantrear_sw:", {
            user_command::pantographtogglerear,
            user_command::none } },
        { "pantfrontoff_sw:", {
            user_command::pantographtogglefront,
            user_command::none } }, // TODO: dedicated lower pantograph commands
        { "pantrearoff_sw:", {
            user_command::pantographtogglerear,
            user_command::none } }, // TODO: dedicated lower pantograph commands
        { "pantalloff_sw:", {
            user_command::pantographlowerall,
            user_command::none } },
        { "pantselected_sw:", {
            user_command::none,
            user_command::none } }, // TODO: selected pantograph(s) operation command
        { "pantselectedoff_sw:", {
            user_command::none,
            user_command::none } }, // TODO: lower selected pantograp(s) command
        { "pantcompressor_sw:", {
            user_command::pantographcompressoractivate,
            user_command::none } },
        { "pantcompressorvalve_sw:", {
            user_command::pantographcompressorvalvetoggle,
            user_command::none } },
        { "trainheating_sw:", {
            user_command::heatingtoggle,
            user_command::none } },
        { "signalling_sw:", {
            user_command::mubrakingindicatortoggle,
            user_command::none } },
        { "door_signalling_sw:", {
            user_command::doorlocktoggle,
            user_command::none } },
        { "nextcurrent_sw:", {
            user_command::mucurrentindicatorothersourceactivate,
            user_command::none } },
        { "instrumentlight_sw:", {
            user_command::instrumentlighttoggle,
            user_command::none } },
        { "cablight_sw:", {
            user_command::interiorlighttoggle,
            user_command::none } },
        { "cablightdim_sw:", {
            user_command::interiorlightdimtoggle,
            user_command::none } },
        { "battery_sw:", {
            user_command::batterytoggle,
            user_command::none } },
        { "universal0:", {
            user_command::generictoggle0,
            user_command::none } },
        { "universal1:", {
            user_command::generictoggle1,
            user_command::none } },
        { "universal2:", {
            user_command::generictoggle2,
            user_command::none } },
        { "universal3:", {
            user_command::generictoggle3,
            user_command::none } },
        { "universal4:", {
            user_command::generictoggle4,
            user_command::none } },
        { "universal5:", {
            user_command::generictoggle5,
            user_command::none } },
        { "universal6:", {
            user_command::generictoggle6,
            user_command::none } },
        { "universal7:", {
            user_command::generictoggle7,
            user_command::none } },
        { "universal8:", {
            user_command::generictoggle8,
            user_command::none } },
        { "universal9:", {
            user_command::generictoggle9,
            user_command::none } }
    };
}

//---------------------------------------------------------------------------
