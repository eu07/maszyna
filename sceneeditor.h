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

// TODO: move the snapshot to history stack
struct node_snapshot {

    scene::basic_node *node;
    std::string data;

    node_snapshot( scene::basic_node *Node ) :
        node( Node ) {
        if( Node != nullptr ) {
            Node->export_as_text( data ); } };
};

inline bool operator==( node_snapshot const &Left, node_snapshot const &Right ) { return ( ( Left.node == Right.node ) && ( Left.data == Right.data ) ); }
inline bool operator!=( node_snapshot const &Left, node_snapshot const &Right ) { return ( !( Left == Right ) ); }

class basic_editor {

public:
// methods
    void
        translate( scene::basic_node *Node, glm::dvec3 const &Location, bool const Snaptoground );
    void
        translate( scene::basic_node *Node, float const Offset );
    void
        rotate( scene::basic_node *Node, glm::vec3 const &Angle, float const Quantization );

private:
// methods
    void
        translate_node( scene::basic_node *Node, glm::dvec3 const &Location );
    void
        translate_node( scene::basic_node *Node, float const Offset );
    void
        translate_instance( TAnimModel *Instance, glm::dvec3 const &Location );
    void
        translate_instance( TAnimModel *Instance, float const Offset );
    void
        translate_memorycell( TMemCell *Memorycell, glm::dvec3 const &Location );
    void
        translate_memorycell( TMemCell *Memorycell, float const Offset );
    void
        rotate_node( scene::basic_node *Node, glm::vec3 const &Angle );
    void
        rotate_instance( TAnimModel *Instance, glm::vec3 const &Angle );
};

} // scene

//---------------------------------------------------------------------------
