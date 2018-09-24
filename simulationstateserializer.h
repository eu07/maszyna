/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "parser.h"
#include "scene.h"

namespace simulation {

class state_serializer {

public:
// methods
    // restores simulation data from specified file. returns: true on success, false otherwise
    bool
        deserialize( std::string const &Scenariofile );
    // stores class data in specified file, in legacy (text) format
    void
        export_as_text( std::string const &Scenariofile ) const;

    struct scratch_data
    {
        struct binary_data
        {
            bool terrain{ false };
        } binary;

        struct location_data
        {
            std::stack<glm::dvec3> offset;
            glm::vec3 rotation;
        } location;

        struct trainset_data
        {
            std::string name;
            std::string track;
            float offset { 0.f };
            float velocity { 0.f };
            std::vector<TDynamicObject *> vehicles;
            std::vector<int> couplings;
            TDynamicObject * driver { nullptr };
            bool is_open { false };
        } trainset;

        bool initialized { false };
    };

private:
// methods
    // restores class data from provided stream
    void deserialize( cParser &Input );
    void deserialize_area( cParser &Input );
    void deserialize_atmo( cParser &Input );
    void deserialize_camera( cParser &Input );
    void deserialize_config( cParser &Input );
    void deserialize_description( cParser &Input );
    void deserialize_event( cParser &Input );
    void deserialize_lua( cParser &Input );
    void deserialize_firstinit( cParser &Input );
    void deserialize_group( cParser &Input );
    void deserialize_endgroup( cParser &Input );
    void deserialize_light( cParser &Input );
    void deserialize_node( cParser &Input );
    void deserialize_origin( cParser &Input );
    void deserialize_endorigin( cParser &Input );
    void deserialize_rotate( cParser &Input );
    void deserialize_sky( cParser &Input );
    void deserialize_test( cParser &Input );
    void deserialize_time( cParser &Input );
    void deserialize_trainset( cParser &Input );
    void deserialize_endtrainset( cParser &Input );
    TTrack * deserialize_path( cParser &Input, scene::node_data const &Nodedata );
    TTraction * deserialize_traction( cParser &Input, scene::node_data const &Nodedata );
    TTractionPowerSource * deserialize_tractionpowersource( cParser &Input, scene::node_data const &Nodedata );
    TMemCell * deserialize_memorycell( cParser &Input, scene::node_data const &Nodedata );
    TEventLauncher * deserialize_eventlauncher( cParser &Input, scene::node_data const &Nodedata );
    TAnimModel * deserialize_model( cParser &Input, scene::node_data const &Nodedata );
    TDynamicObject * deserialize_dynamic( cParser &Input, scene::node_data const &Nodedata );
    sound_source * deserialize_sound( cParser &Input, scene::node_data const &Nodedata );
    // skips content of stream until specified token
    void skip_until( cParser &Input, std::string const &Token );
    // transforms provided location by specifed rotation and offset
    glm::dvec3 transform( glm::dvec3 Location );

    scratch_data scratchpad;
};

}
