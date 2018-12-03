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
#include "editormouseinput.h"
#include "editorkeyboardinput.h"
#include "Camera.h"
#include "sceneeditor.h"
#include "scenenode.h"

class editor_mode : public application_mode {

public:
// constructors
    editor_mode();
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
        on_scroll( double const Xoffset, double const Yoffset ) override { ; }
    void
        on_event_poll() override;

private:
// types
    struct editormode_input {

        editormouse_input mouse;
        editorkeyboard_input keyboard;

        bool init();
        void poll();
    };

    struct state_backup {

        TCamera camera;
        bool freefly;
        bool picking;
    };
// methods
    void
        update_camera( double const Deltatime );
    bool
        mode_translation() const;
    bool
        mode_translation_vertical() const;
    bool
        mode_rotation() const;
    bool
        mode_snap() const;
// members
    state_backup m_statebackup; // helper, cached variables to be restored on mode exit
    editormode_input m_input;
    TCamera Camera;
    double fTime50Hz { 0.0 }; // bufor czasu dla komunikacji z PoKeys
    scene::basic_editor m_editor;
    scene::basic_node *m_node; // currently selected scene node
    bool m_takesnapshot { true }; // helper, hints whether snapshot of selected node(s) should be taken before modification
    bool m_dragging = false;
};
