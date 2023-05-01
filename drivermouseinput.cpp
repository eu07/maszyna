/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "drivermouseinput.h"

#include "Globals.h"
#include "application.h"
#include "utilities.h"
#include "Globals.h"
#include "Timer.h"
#include "simulation.h"
#include "Train.h"
#include "AnimModel.h"
#include "renderer.h"
#include "uilayer.h"
#include "Logs.h"

auto const EU07_CONTROLLER_MOUSESLIDERSIZE{ 0.6 };

void
mouse_slider::bind( user_command const &Command ) {

    m_command = Command;

    auto const *train { simulation::Train };
    TMoverParameters const *vehicle { nullptr };
    switch( m_command ) {
        case user_command::jointcontrollerset:
        case user_command::mastercontrollerset:
        case user_command::secondcontrollerset: {
            vehicle = ( train ? train->Controlled() : nullptr );
            break;
        }
        case user_command::trainbrakeset:
        case user_command::independentbrakeset: {
            vehicle = ( train ? train->Occupied() : nullptr );
            break;
        }
        default: {
            break;
        }
    }
    if( vehicle == nullptr ) { return; }

    // calculate initial value and accepted range
    switch( m_command ) {
        case user_command::jointcontrollerset: {
            // NOTE: special case, merges two separate controls
            auto const *occupied { train ? train->Occupied() : nullptr };
            if( occupied == nullptr ) { return; }

            auto const powerrange { static_cast<double>(
                vehicle->CoupledCtrl ?
                    vehicle->MainCtrlPosNo + vehicle->ScndCtrlPosNo :
                    vehicle->MainCtrlPosNo ) };
            // for simplicity upper half of the range controls power, lower controls brakes
            auto const brakerangemultiplier { powerrange / LocalBrakePosNo };

            m_valuerange = 1.0;
            m_value =
                0.5
                + 0.5 * ( vehicle->CoupledCtrl ?
                        vehicle->MainCtrlPos + vehicle->ScndCtrlPos :
                        vehicle->MainCtrlPos ) / powerrange
                - 0.5 * occupied->LocalBrakePosA;
            m_analogue = true;
            m_invertrange = false;
            break;
        }
        case user_command::mastercontrollerset: {
            m_valuerange = (
                vehicle->CoupledCtrl ?
                    vehicle->MainCtrlPosNo + vehicle->ScndCtrlPosNo :
                    vehicle->MainCtrlPosNo );
            m_value = (
                vehicle->CoupledCtrl ?
                    vehicle->MainCtrlPos + vehicle->ScndCtrlPos :
                    vehicle->MainCtrlPos );
            m_analogue = false;
            m_invertrange = false;
            break;
        }
        case user_command::secondcontrollerset: {
            m_valuerange = vehicle->ScndCtrlPosNo;
            m_value = vehicle->ScndCtrlPos;
            m_analogue = false;
            m_invertrange = true;
            break;
        }
        case user_command::trainbrakeset: {
            m_valuerange = 1.0;
            m_value = ( vehicle->fBrakeCtrlPos - vehicle->Handle->GetPos( bh_MIN ) ) / ( vehicle->Handle->GetPos( bh_MAX ) - vehicle->Handle->GetPos( bh_MIN ) );
            m_analogue = true;
            m_invertrange = true;
            break;
        }
        case user_command::independentbrakeset: {
            m_valuerange = 1.0;
            m_value = vehicle->LocalBrakePosA;
            m_analogue = true;
            m_invertrange = true;
            break;
        }
        default: {
            m_valuerange = 1;
            break;
        }
    }
    // hide the cursor and place it in accordance with current slider value
    m_cursorposition = glm::dvec2(Global.cursor_pos);
    Application.set_cursor( GLFW_CURSOR_DISABLED );

    auto const controlsize { Global.window_size.y * EU07_CONTROLLER_MOUSESLIDERSIZE };
    auto const controledge { Global.window_size.y * 0.5 + controlsize * 0.5 };
    auto const stepsize { controlsize / m_valuerange };

    if( m_invertrange ) {
        m_value = ( m_analogue ? 1.0 : m_valuerange ) - m_value;
    }

    Application.set_cursor_pos(
        Global.window_size.y,
        ( m_analogue ?
            controledge - m_value * controlsize :
            controledge - m_value * stepsize - 0.5 * stepsize ) );
}

void
mouse_slider::release() {

    m_command = user_command::none;
    Application.set_cursor_pos( m_cursorposition.x, m_cursorposition.y );
    Application.set_cursor( GLFW_CURSOR_NORMAL );
}

void
mouse_slider::on_move( double const Mousex, double const Mousey ) {

    auto const controlsize { Global.window_size.y * EU07_CONTROLLER_MOUSESLIDERSIZE };
    auto const controledge { Global.window_size.y * 0.5 + controlsize * 0.5 };
    auto const stepsize { controlsize / m_valuerange };

    auto mousey = clamp( Mousey, controledge - controlsize, controledge );
    m_value = (
        m_analogue ?
            ( controledge - mousey ) / controlsize :
            std::floor( ( controledge - mousey ) / stepsize ) );
    if( m_invertrange ) {
        m_value = ( m_analogue ? 1.0 : m_valuerange ) - m_value; }
}



bool
drivermouse_input::init() {

#ifdef _WIN32
    DWORD systemkeyboardspeed;
    ::SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, &systemkeyboardspeed, 0 );
    m_updaterate = interpolate( 0.5, 0.04, systemkeyboardspeed / 31.0 );
    DWORD systemkeyboarddelay;
    ::SystemParametersInfo( SPI_GETKEYBOARDDELAY, 0, &systemkeyboarddelay, 0 );
    m_updatedelay = interpolate( 0.25, 1.0, systemkeyboarddelay / 3.0 );
#endif

    default_bindings();
    recall_bindings();

    return true;
}

bool
drivermouse_input::recall_bindings() {

    cParser bindingparser( "eu07_input-mouse.ini", cParser::buffer_FILE );
    if( false == bindingparser.ok() ) {
        return false;
    }

    // build helper translation tables
    std::unordered_map<std::string, user_command> nametocommandmap;
    std::size_t commandid = 0;
    for( auto const &description : simulation::Commands_descriptions ) {
        nametocommandmap.emplace(
            description.name,
            static_cast<user_command>( commandid ) );
        ++commandid;
    }

    // NOTE: to simplify things we expect one entry per line, and whole entry in one line
    while( true == bindingparser.getTokens( 1, true, "\n\r" ) ) {

        std::string bindingentry;
        bindingparser >> bindingentry;
        cParser entryparser( bindingentry );

        if( true == entryparser.getTokens( 1, true, "\n\r\t " ) ) {

            std::string bindingpoint {};
            entryparser >> bindingpoint;

            std::vector< std::reference_wrapper<user_command> > bindingtargets;

            if( bindingpoint == "wheel" ) {
                bindingtargets.emplace_back( std::ref( m_wheelbindings.up ) );
                bindingtargets.emplace_back( std::ref( m_wheelbindings.down ) );
            }
            // TODO: binding targets for mouse buttons

            for( auto &bindingtarget : bindingtargets ) {
                // grab command(s) associated with the input pin
                auto const bindingcommandname{ entryparser.getToken<std::string>() };
                if( true == bindingcommandname.empty() ) {
                    // no tokens left, may as well complain then call it a day
                    WriteLog( "Mouse binding for " + bindingpoint + " didn't specify associated command(s)" );
                    break;
                }
                auto const commandlookup = nametocommandmap.find( bindingcommandname );
                if( commandlookup == nametocommandmap.end() ) {
                    WriteLog( "Mouse binding for " + bindingpoint + " specified unknown command, \"" + bindingcommandname + "\"" );
                }
                else {
                    bindingtarget.get() = commandlookup->second;
                }
            }
        }
    }

    return true;
}

void
drivermouse_input::move( double Mousex, double Mousey ) {

    if( false == Global.ControlPicking ) {
        // default control mode
        m_relay.post(
            user_command::viewturn,
            Mousex,
            Mousey,
            GLFW_PRESS,
		    0 );
    }
    else {
        // control picking mode
        if( m_slider.command() != user_command::none ) {
            m_slider.on_move( Mousex, Mousey );
            m_relay.post(
                m_slider.command(),
                m_slider.value(),
                0,
                GLFW_PRESS,
			    0 );
        }

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
            viewoffset.x,
            viewoffset.y,
            GLFW_PRESS,
		    0 );
        m_cursorposition = cursorposition;
    }
}

void
drivermouse_input::scroll( double const Xoffset, double const Yoffset ) {

    if( Global.ctrlState || m_buttons[1] == GLFW_PRESS ) {
        // ctrl + scroll wheel or holding right mouse button + scroll wheel adjusts fov
		Global.FieldOfView = clamp( static_cast<float>( Global.FieldOfView - Yoffset * 20.0 / Timer::subsystem.mainloop_total.average() ), 15.0f, 75.0f );
    }
    else {
        // scroll adjusts master controller
        // TODO: allow configurable scroll commands
        auto command {
            adjust_command(
                ( Yoffset > 0.0 ) ?
                    m_wheelbindings.up :
                    m_wheelbindings.down ) };

        m_relay.post(
            command,
            0,
            0,
            GLFW_PRESS,
            // TODO: pass correct entity id once the missing systems are in place
            0 );
    }
}

void
drivermouse_input::button( int const Button, int const Action ) {

    // store key state
    if( Button >= 0 ) {
        m_buttons[ Button ] = Action;
    }

    if( false == Global.ControlPicking ) { return; }

    if( true == FreeFlyModeFlag ) {
        // freefly mode
        // left mouse button launches on_click event associated with to the node
        if( Button == GLFW_MOUSE_BUTTON_LEFT ) {
            if( Action == GLFW_PRESS ) {
                GfxRenderer->Pick_Node_Callback(
                    [this](scene::basic_node *node) {
                        if( ( node == nullptr )
                         || ( typeid( *node ) != typeid( TAnimModel ) ) )
                            return;
                        simulation::Region->on_click( static_cast<TAnimModel const *>( node ) ); } );
            }
        }
        // right button controls panning
        if( Button == GLFW_MOUSE_BUTTON_RIGHT ) {
            m_pickmodepanning = ( Action == GLFW_PRESS );
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
                m_pickwaiting = false;
                if( Button == GLFW_MOUSE_BUTTON_LEFT ) {
                    if( m_slider.command() != user_command::none ) {
                        m_relay.post( m_slider.command(), 0, 0, Action, 0 );
                        m_slider.release();
                    }
                }
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
            m_pickwaiting = true;
            GfxRenderer->Pick_Control_Callback(
                [this, Button, Action, &mousecommand](TSubModel const *control, const glm::vec2 pos) {

                    bool pickwaiting = m_pickwaiting;
                    m_pickwaiting = false;

                    // click on python screen
                    if (Button == GLFW_MOUSE_BUTTON_LEFT
                            && control && control->screen_touch_list) {

                        control->screen_touch_list->emplace_back(pos);
                    }

                    auto const controlbindings { bindings( simulation::Train->GetLabel( control ) ) };
                    // if the recognized element under the cursor has a command associated with the pressed button, notify the recipient
                    mousecommand = (
                        Button == GLFW_MOUSE_BUTTON_LEFT ?
                            controlbindings.first :
                            controlbindings.second
                        );

                    if( mousecommand == user_command::none ) {
                        // if we don't have any recognized element under the cursor and the right button was pressed, enter view panning mode
                        if( Button == GLFW_MOUSE_BUTTON_RIGHT ) {
                            m_pickmodepanning = true;
                        }
                        return;
                    }
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
						    m_varyingpollrateorigin = m_cursorposition;
							m_varyingpollrate = true;
							break;
					    }
					    case user_command::jointcontrollerset:
					    case user_command::mastercontrollerset:
					    case user_command::secondcontrollerset:
					    case user_command::trainbrakeset:
					    case user_command::independentbrakeset: {
						    m_slider.bind( mousecommand );
							mousecommand = user_command::none;
							return;
					    }
					    default: {
						    break;
					    }
					}
					// NOTE: basic keyboard controls don't have any parameters
					// NOTE: as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
					// TODO: pass correct entity id once the missing systems are in place
					m_relay.post( mousecommand, 0, 0, Action, 0 );
					if (!pickwaiting) // already depressed
						m_relay.post( mousecommand, 0, 0, GLFW_RELEASE, 0 );
					m_updateaccumulator = -0.25; // prevent potential command repeat right after issuing one
				} );
        }
    }
}

int
drivermouse_input::button( int const Button ) const {

    return m_buttons[ Button ];
}

void
drivermouse_input::poll() {

    m_updateaccumulator += Timer::GetDeltaRenderTime();

    auto updaterate { m_updaterate };
    if( m_varyingpollrate ) {
        updaterate /= std::max( 0.15, 2.0 * glm::length( m_cursorposition - m_varyingpollrateorigin ) / std::max( 1, Global.window_size.y ) );
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

user_command
drivermouse_input::command() const {

    return (
        m_slider.command() != user_command::none ? m_slider.command() :
        m_mousecommandleft != user_command::none ? m_mousecommandleft :
        m_mousecommandright );
}

// returns pair of bindings associated with specified cab control
std::pair<user_command, user_command>
drivermouse_input::bindings( std::string const &Control ) const {

    auto const lookup{ m_buttonbindings.find( Control ) };

    if( lookup != m_buttonbindings.end() )
        return { lookup->second.left, lookup->second.right };
    else {
        return { user_command::none, user_command::none };
    }
}

void
drivermouse_input::default_bindings() {

    m_buttonbindings = {
        { "jointctrl:", {
            user_command::jointcontrollerset,
            user_command::none } },
        { "mainctrl:", {
            user_command::mastercontrollerset,
            user_command::none } },
        { "scndctrl:", {
            user_command::secondcontrollerset,
            user_command::none } },
        { "shuntmodepower:", {
            user_command::secondcontrollerincrease,
            user_command::secondcontrollerdecrease } },
        { "tempomat_sw:", {
            user_command::tempomattoggle,
            user_command::none } },
        { "tempomatoff_sw:", {
            user_command::tempomattoggle,
            user_command::none } },
        { "dirkey:", {
            user_command::reverserincrease,
            user_command::reverserdecrease } },
        { "dirforward_bt:", {
            user_command::reverserforward,
            user_command::none } },
        { "dirneutral_bt:", {
            user_command::reverserneutral,
            user_command::none } },
        { "dirbackward_bt:", {
            user_command::reverserbackward,
            user_command::none } },
        { "brakectrl:", {
            user_command::trainbrakeset,
            user_command::none } },
        { "localbrake:", {
            user_command::independentbrakeset,
            user_command::none } },
        { "manualbrake:", {
            user_command::manualbrakeincrease,
            user_command::manualbrakedecrease } },
        { "alarmchain:", {
            user_command::alarmchaintoggle,
            user_command::none } },
        { "alarmchainon:", {
            user_command::alarmchainenable,
            user_command::none} },
        { "alarmchainoff:", {
            user_command::alarmchainenable,
            user_command::none} },
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
        { "brakeopmode_sw:", {
            user_command::trainbrakeoperationmodeincrease,
            user_command::trainbrakeoperationmodedecrease } },
        { "maxcurrent_sw:", {
            user_command::motoroverloadrelaythresholdtoggle,
            user_command::none } },
        { "waterpumpbreaker_sw:", {
            user_command::waterpumpbreakertoggle,
            user_command::none } },
        { "waterpump_sw:", {
            user_command::waterpumptoggle,
            user_command::none } },
        { "waterheaterbreaker_sw:", {
            user_command::waterheaterbreakertoggle,
            user_command::none } },
        { "waterheater_sw:", {
            user_command::waterheatertoggle,
            user_command::none } },
        { "fuelpump_sw:", {
            user_command::fuelpumptoggle,
            user_command::none } },
        { "oilpump_sw:", {
            user_command::oilpumptoggle,
            user_command::none } },
        { "motorblowersfront_sw:", {
            user_command::motorblowerstogglefront,
            user_command::none } },
        { "motorblowersrear_sw:", {
            user_command::motorblowerstogglerear,
            user_command::none } },
        { "motorblowersalloff_sw:", {
            user_command::motorblowersdisableall,
            user_command::none } },
        { "coolingfans_sw:", {
            user_command::coolingfanstoggle,
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
        { "shp_reset_bt:", {
            user_command::cabsignalacknowledge,
            user_command::none } },
        { "releaser_bt:", {
            user_command::independentbrakebailoff,
            user_command::none } },
		{ "springbraketoggle_bt:",{
			user_command::springbraketoggle,
			user_command::none } },
		{ "springbrakeon_bt:",{
			user_command::springbrakeenable,
			user_command::none } },
		{ "springbrakeoff_bt:",{
			user_command::springbrakedisable,
			user_command::none } },
		{ "universalbrake1_bt:",{
			user_command::universalbrakebutton1,
			user_command::none } },
		{ "universalbrake2_bt:",{
			user_command::universalbrakebutton2,
			user_command::none } },
		{ "universalbrake3_bt:",{
			user_command::universalbrakebutton3,
			user_command::none } },
		{ "epbrake_bt:",{
			user_command::epbrakecontroltoggle,
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
        { "whistle_bt:", {
            user_command::whistleactivate,
            user_command::none } },
        { "fuse_bt:", {
            user_command::motoroverloadrelayreset,
            user_command::none } },
        { "converterfuse_bt:", {
            user_command::converteroverloadrelayreset,
            user_command::none } },
        { "relayreset1_bt:", {
            user_command::universalrelayreset1,
            user_command::none } },
        { "relayreset2_bt:", {
            user_command::universalrelayreset2,
            user_command::none } },
        { "relayreset3_bt:", {
            user_command::universalrelayreset3,
            user_command::none } },
        { "stlinoff_bt:", {
            user_command::motorconnectorsopen,
            user_command::none } },
        { "doorleftpermit_sw:", {
            user_command::doorpermitleft,
            user_command::none } },
        { "doorrightpermit_sw:", {
            user_command::doorpermitright,
            user_command::none } },
        { "doorpermitpreset_sw:", {
            user_command::doorpermitpresetactivatenext,
            user_command::doorpermitpresetactivateprevious } },
        { "door_left_sw:", {
            user_command::doortoggleleft,
            user_command::none } },
        { "door_right_sw:", {
            user_command::doortoggleright,
            user_command::none } },
        { "doorlefton_sw:", {
            user_command::dooropenleft,
            user_command::none } },
        { "doorrighton_sw:", {
            user_command::dooropenright,
            user_command::none } },
        { "doorleftoff_sw:", {
            user_command::doorcloseleft,
            user_command::none } },
        { "doorrightoff_sw:", {
            user_command::doorcloseright,
            user_command::none } },
        { "doorallon_sw:", {
            user_command::dooropenall,
            user_command::none } },
        { "dooralloff_sw:", {
            user_command::doorcloseall,
            user_command::none } },
        { "doorstep_sw:", {
            user_command::doorsteptoggle,
            user_command::none } },
        { "doormode_sw:", {
            user_command::doormodetoggle,
            user_command::none } },
		{ "mirrors_sw:", {
			user_command::mirrorstoggle,
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
		{ "compressorlist_sw:", {
			user_command::compressorpresetactivatenext,
			user_command::compressorpresetactivateprevious } },
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
        { "radioon_sw:", {
            user_command::radioenable,
            user_command::none } },
        { "radiooff_sw:", {
            user_command::radiodisable,
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
        { "radiostopon_sw:", {
            user_command::radiostopenable,
            user_command::none } },
        { "radiostopoff_sw:", {
            user_command::radiostopdisable,
            user_command::none } },
        { "radiotest_sw:", {
            user_command::radiostoptest,
            user_command::none } },
        { "radiocall3_sw:", {
            user_command::radiocall3send,
            user_command::none } },
		{ "radiovolume_sw:",{
			user_command::radiovolumeincrease,
			user_command::radiovolumedecrease } },
		{ "radiovolumeprev_sw:",{
			user_command::radiovolumedecrease,
			user_command::none } },
		{ "radiovolumenext_sw:",{
			user_command::radiovolumeincrease,
			user_command::none } },
        { "pantfront_sw:", {
            user_command::pantographtogglefront,
            user_command::none } },
        { "pantrear_sw:", {
            user_command::pantographtogglerear,
            user_command::none } },
        { "pantfrontoff_sw:", {
            user_command::pantographlowerfront,
            user_command::none } },
        { "pantrearoff_sw:", {
            user_command::pantographlowerrear,
            user_command::none } },
        { "pantalloff_sw:", {
            user_command::pantographlowerall,
            user_command::none } },
        { "pantselected_sw:", {
            user_command::pantographtoggleselected,
            user_command::none } }, // TBD: bind lowerselected in case of toggle switch
        { "pantselectedoff_sw:", {
            user_command::pantographlowerselected,
            user_command::none } },
        { "pantselect_sw:", {
            user_command::pantographselectnext,
            user_command::pantographselectprevious } },
        { "pantvalves_sw:", {
            user_command::pantographvalvesupdate,
            user_command::pantographvalvesoff } },
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
        { "distancecounter_sw:", {
            user_command::distancecounteractivate,
            user_command::none } },
        { "instrumentlight_sw:", {
            user_command::instrumentlighttoggle,
            user_command::none } },
        { "dashboardlight_sw:", {
            user_command::dashboardlighttoggle,
            user_command::none } },
        { "dashboardlighton_sw:", {
            user_command::dashboardlightenable,
            user_command::none } },
        { "dashboardlightoff_sw:", {
            user_command::dashboardlightdisable,
            user_command::none } },
        { "timetablelight_sw:", {
            user_command::timetablelighttoggle,
            user_command::none } },
        { "timetablelighton_sw:", {
            user_command::timetablelightenable,
            user_command::none } },
        { "timetablelightoff_sw:", {
            user_command::timetablelightdisable,
            user_command::none } },
        { "cablight_sw:", {
            user_command::interiorlighttoggle,
            user_command::none } },
        { "cablightdim_sw:", {
            user_command::interiorlightdimtoggle,
            user_command::none } },
        { "compartmentlights_sw:", {
            user_command::compartmentlightstoggle,
            user_command::none } },
        { "compartmentlightson_sw:", {
            user_command::compartmentlightsenable,
            user_command::none } },
        { "compartmentlightsoff_sw:", {
            user_command::compartmentlightsdisable,
            user_command::none } },
        { "battery_sw:", {
            user_command::batterytoggle,
            user_command::none } },
        { "batteryon_sw:", {
            user_command::batteryenable,
            user_command::none } },
        { "batteryoff_sw:", {
            user_command::batterydisable,
            user_command::none } },
		{ "cabactivation_sw:", {
			user_command::cabactivationtoggle,
			user_command::none } },
        { "couplingdisconnect_sw:",{
			user_command::occupiedcarcouplingdisconnect,
			user_command::none } },
		{ "couplingdisconnectback_sw:",{
			user_command::occupiedcarcouplingdisconnectback,
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
            user_command::none } },
		{ "speedinc_bt:",{
			user_command::speedcontrolincrease,
			user_command::none } },
		{ "speeddec_bt:",{
			user_command::speedcontroldecrease,
			user_command::none } },
		{ "speedctrlpowerinc_bt:",{
			user_command::speedcontrolpowerincrease,
			user_command::none } },
		{ "speedctrlpowerdec_bt:",{
			user_command::speedcontrolpowerdecrease,
			user_command::none } },
		{ "speedbutton0:",{
			user_command::speedcontrolbutton0,
			user_command::none } },
		{ "speedbutton1:",{
			user_command::speedcontrolbutton1,
			user_command::none } },
		{ "speedbutton2:",{
			user_command::speedcontrolbutton2,
			user_command::none } },
		{ "speedbutton3:",{
			user_command::speedcontrolbutton3,
			user_command::none } },
		{ "speedbutton4:",{
			user_command::speedcontrolbutton4,
			user_command::none } },
		{ "speedbutton5:",{
			user_command::speedcontrolbutton5,
			user_command::none } },
		{ "speedbutton6:",{
			user_command::speedcontrolbutton6,
			user_command::none } },
		{ "speedbutton7:",{
			user_command::speedcontrolbutton7,
			user_command::none } },
		{ "speedbutton8:",{
			user_command::speedcontrolbutton8,
			user_command::none } },
		{ "speedbutton9:",{
			user_command::speedcontrolbutton9,
			user_command::none } },
		{ "inverterenable1_bt:",{
			user_command::inverterenable1,
			user_command::none } },
		{ "inverterenable2_bt:",{
			user_command::inverterenable2,
			user_command::none } },
		{ "inverterenable3_bt:",{
			user_command::inverterenable3,
			user_command::none } },
		{ "inverterenable4_bt:",{
			user_command::inverterenable4,
			user_command::none } },
		{ "inverterenable5_bt:",{
			user_command::inverterenable5,
			user_command::none } },
		{ "inverterenable6_bt:",{
			user_command::inverterenable6,
			user_command::none } },
		{ "inverterenable7_bt:",{
			user_command::inverterenable7,
			user_command::none } },
		{ "inverterenable8_bt:",{
			user_command::inverterenable8,
			user_command::none } },
		{ "inverterenable9_bt:",{
			user_command::inverterenable9,
			user_command::none } },
		{ "inverterenable10_bt:",{
			user_command::inverterenable10,
			user_command::none } },
		{ "inverterenable11_bt:",{
			user_command::inverterenable11,
			user_command::none } },
		{ "inverterenable12_bt:",{
			user_command::inverterenable12,
			user_command::none } },
		{ "inverterdisable1_bt:",{
			user_command::inverterdisable1,
			user_command::none } },
		{ "inverterdisable2_bt:",{
			user_command::inverterdisable2,
			user_command::none } },
		{ "inverterdisable3_bt:",{
			user_command::inverterdisable3,
			user_command::none } },
		{ "inverterdisable4_bt:",{
			user_command::inverterdisable4,
			user_command::none } },
		{ "inverterdisable5_bt:",{
			user_command::inverterdisable5,
			user_command::none } },
		{ "inverterdisable6_bt:",{
			user_command::inverterdisable6,
			user_command::none } },
		{ "inverterdisable7_bt:",{
			user_command::inverterdisable7,
			user_command::none } },
		{ "inverterdisable8_bt:",{
			user_command::inverterdisable8,
			user_command::none } },
		{ "inverterdisable9_bt:",{
			user_command::inverterdisable9,
			user_command::none } },
		{ "inverterdisable10_bt:",{
			user_command::inverterdisable10,
			user_command::none } },
		{ "inverterdisable11_bt:",{
			user_command::inverterdisable11,
			user_command::none } },
		{ "inverterdisable12_bt:",{
			user_command::inverterdisable12,
			user_command::none } },
		{ "invertertoggle1_bt:",{
			user_command::invertertoggle1,
			user_command::none } },
		{ "invertertoggle2_bt:",{
			user_command::invertertoggle2,
			user_command::none } },
		{ "invertertoggle3_bt:",{
			user_command::invertertoggle3,
			user_command::none } },
		{ "invertertoggle4_bt:",{
			user_command::invertertoggle4,
			user_command::none } },
		{ "invertertoggle5_bt:",{
			user_command::invertertoggle5,
			user_command::none } },
		{ "invertertoggle6_bt:",{
			user_command::invertertoggle6,
			user_command::none } },
		{ "invertertoggle7_bt:",{
			user_command::invertertoggle7,
			user_command::none } },
		{ "invertertoggle8_bt:",{
			user_command::invertertoggle8,
			user_command::none } },
		{ "invertertoggle9_bt:",{
			user_command::invertertoggle9,
			user_command::none } },
		{ "invertertoggle10_bt:",{
			user_command::invertertoggle10,
			user_command::none } },
		{ "invertertoggle11_bt:",{
			user_command::invertertoggle11,
			user_command::none } },
		{ "invertertoggle12_bt:",{
			user_command::invertertoggle12,
			user_command::none } },

    };
}

user_command
drivermouse_input::adjust_command( user_command Command ) {

    if( ( true == Global.shiftState )
     && ( Command != user_command::none ) ) {
        switch( Command ) {
            case user_command::mastercontrollerincrease: { Command = user_command::mastercontrollerincreasefast; break; }
            case user_command::mastercontrollerdecrease: { Command = user_command::mastercontrollerdecreasefast; break; }
            case user_command::secondcontrollerincrease: { Command = user_command::secondcontrollerincreasefast; break; }
            case user_command::secondcontrollerdecrease: { Command = user_command::secondcontrollerdecreasefast; break; }
            case user_command::independentbrakeincrease: { Command = user_command::independentbrakeincreasefast; break; }
            case user_command::independentbrakedecrease: { Command = user_command::independentbrakedecreasefast; break; }
            default: { break; }
        }
    }

    return Command;
}

//---------------------------------------------------------------------------
