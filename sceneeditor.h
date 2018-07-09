/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "scenenode.h"

namespace scene {

class basic_editor {

public:
// methods
    bool
        on_key( int const Key, int const Action );
    bool
        on_mouse_button( int const Button, int const Action );
    bool
        on_mouse_move( double const Mousex, double const Mousey );
    scene::basic_node const *
        node() const {
            return m_node; }
private:
// types
    struct node_snapshot {

        scene::basic_node *node;
        std::string data;

        node_snapshot( scene::basic_node *Node ) :
            node( Node ) {
            if( Node != nullptr ) {
                Node->export_as_text( data ); } };
    };
    friend bool operator==( basic_editor::node_snapshot const &Left, basic_editor::node_snapshot const &Right );
    friend bool operator!=( basic_editor::node_snapshot const &Left, basic_editor::node_snapshot const &Right );
// methods
    bool
        mode_translation() const;
    bool
        mode_translation_vertical() const;
    bool
        mode_snap() const;
    void
        translate( glm::dvec3 const &Location );
    void
        translate( float const Offset );
    void
        translate_instance( TAnimModel *Instance, glm::dvec3 const &Location );
    void
        translate_instance( TAnimModel *Instance, float const Offset );
    void
        translate_memorycell( TMemCell *Memorycell, glm::dvec3 const &Location );
    void
        translate_memorycell( TMemCell *Memorycell, float const Offset );
    void
        rotate( glm::vec3 const &Angle );
    void
        rotate_instance( TAnimModel *Instance, glm::vec3 const &Angle );
// members
    scene::basic_node *m_node; // temporary helper, currently selected scene node
    node_snapshot m_nodesnapshot { nullptr }; // currently selected scene node in its pre-modified state
    glm::dvec2 m_mouseposition { 0.0 };
    bool m_mouseleftbuttondown { false };

};

inline bool operator==( basic_editor::node_snapshot const &Left, basic_editor::node_snapshot const &Right ) { return ( ( Left.node == Right.node ) && ( Left.data == Right.data ) ); }
inline bool operator!=( basic_editor::node_snapshot const &Left, basic_editor::node_snapshot const &Right ) { return ( !( Left == Right ) ); }

extern basic_editor Editor;

} // scene

//---------------------------------------------------------------------------
