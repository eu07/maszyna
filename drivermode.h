/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "applicationmode.h"

#include "driverkeyboardinput.h"
#include "drivermouseinput.h"
#include "gamepadinput.h"
#include "uart.h"
#include "console.h"
#include "camera.h"
#include "classes.h"

class driver_mode : public application_mode {

public:
// constructors
    driver_mode();

// methods
    // initializes internal data structures of the mode. returns: true on success, false otherwise
    bool
        init() override;
    // mode-specific update of simulation data. returns: false on error, true otherwise
    bool
        update() override;
    // maintenance method, called when the mode is activated
    void
        enter() override;
    // maintenance method, called when the mode is deactivated
    void
        exit() override;
    // input handlers
    void
        on_key( int const Key, int const Scancode, int const Action, int const Mods ) override;
    void
        on_cursor_pos( double const Horizontal, double const Vertical ) override;
    void
        on_mouse_button( int const Button, int const Action, int const Mods ) override;
    void
        on_scroll( double const Xoffset, double const Yoffset ) override;
    void
        on_event_poll() override;

private:
// types
    struct drivermode_input {

        gamepad_input gamepad;
        drivermouse_input mouse;
        glm::dvec2 mouse_pickmodepos; // stores last mouse position in control picking mode
        driverkeyboard_input keyboard;
        Console console;
        std::unique_ptr<uart_input> uart;

        bool init();
        void poll();
    };

// methods
    void update_camera( const double Deltatime );
    // handles vehicle change flag
    void OnKeyDown( int cKey );
    void ChangeDynamic();
    void InOutKey( bool const Near = true );
    void FollowView( bool wycisz = true );
    void DistantView( bool const Near = false );
    void set_picking( bool const Picking );

// members
    drivermode_input m_input;
    std::array<basic_event *, 10> KeyEvents { nullptr }; // eventy wyzwalane z klawiaury
    TCamera Camera;
    TCamera DebugCamera;
    TDynamicObject *pDynamicNearest { nullptr }; // vehicle nearest to the active camera. TODO: move to camera
    double fTime50Hz { 0.0 }; // bufor czasu dla komunikacji z PoKeys
    double const m_primaryupdaterate { 1.0 / 100.0 };
    double const m_secondaryupdaterate { 1.0 / 50.0 };
    double m_primaryupdateaccumulator { m_secondaryupdaterate }; // keeps track of elapsed simulation time, for core fixed step routines
    double m_secondaryupdateaccumulator { m_secondaryupdaterate }; // keeps track of elapsed simulation time, for less important fixed step routines
    int iPause { 0 }; // wykrywanie zmian w zapauzowaniu
};
