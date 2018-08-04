/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "uilayer.h"

// component implementing specific mode of application behaviour
// base interface
class application_mode {

public:
// destructor
    virtual
        ~application_mode() = default;
// methods;
    // initializes internal data structures of the mode. returns: true on success, false otherwise
    virtual
    bool
        init() = 0;
    // mode-specific update of simulation data. returns: false on error, true otherwise
    virtual
    bool
        update() = 0;
    // draws node-specific user interface
    inline
    void
        render_ui() {
            if( m_userinterface != nullptr ) {
                m_userinterface->render(); } }
    inline
    void
        set_progress( float const Progress = 0.f, float const Subtaskprogress = 0.f ) {
            if( m_userinterface != nullptr ) {
                m_userinterface->set_progress( Progress, Subtaskprogress ); } }
    // maintenance method, called when the mode is activated
    virtual
    void
        enter() = 0;
    // maintenance method, called when the mode is deactivated
    virtual
    void
        exit() = 0;
    // input handlers
    virtual
    void
        on_key( int const Key, int const Scancode, int const Action, int const Mods ) = 0;
    virtual
    void
        on_cursor_pos( double const X, double const Y ) = 0;
    virtual
    void
        on_mouse_button( int const Button, int const Action, int const Mods ) = 0;
    virtual
    void
        on_scroll( double const Xoffset, double const Yoffset ) = 0;
    virtual
    void
        on_event_poll() = 0;

protected:
// members
    std::shared_ptr<ui_layer> m_userinterface;
};
