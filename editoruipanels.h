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
#include "Classes.h"

class itemproperties_panel : public ui_panel {

public:
    itemproperties_panel( std::string const &Name, bool const Isopen )
        : ui_panel( Name, Isopen )
    {}

    void update( scene::basic_node const *Node );
    void render() override;

private:
// methods
    void update_group();
    bool render_group();

// members
    scene::basic_node const *m_node { nullptr }; // scene node bound to the panel
    scene::group_handle m_grouphandle { null_handle }; // scene group bound to the panel
    std::string m_groupprefix;
    std::vector<text_line> m_grouplines;
};
