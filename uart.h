#pragma once
#include <libserialport.h>
#include "command.h"
#include "Globals.h"

class uart_input
{
    enum input_type_t
    {
        toggle,
        impulse,
        impulse_r,
        impulse_r_on,
        impulse_r_off
    };

    std::array<std::tuple<size_t, user_command, input_type_t>, 23> input_bits =
    {
        std::make_tuple(1, user_command::linebreakertoggle, impulse_r_off),
        std::make_tuple(2, user_command::linebreakertoggle, impulse_r_on),
        std::make_tuple(3, user_command::motoroverloadrelayreset, impulse),
        std::make_tuple(5, user_command::converteroverloadrelayreset, impulse),
        std::make_tuple(6, user_command::motorconnectorsopen, toggle),
        std::make_tuple(7, user_command::alerteracknowledge, impulse_r),

        std::make_tuple(9, user_command::convertertoggle, toggle),
        std::make_tuple(10, user_command::compressortoggle, toggle),
        std::make_tuple(11, user_command::sandboxactivate, impulse),
        std::make_tuple(12, user_command::heatingtoggle, toggle),

        std::make_tuple(15, user_command::motoroverloadrelaythresholdtoggle, toggle),
        std::make_tuple(16, user_command::pantographtogglefront, toggle),
        std::make_tuple(17, user_command::pantographtogglerear, toggle),
        std::make_tuple(18, user_command::wheelspinbrakeactivate, impulse),
        std::make_tuple(19, user_command::headlightsdimtoggle, toggle),
        std::make_tuple(20, user_command::interiorlightdimtoggle, toggle),
        std::make_tuple(21, user_command::independentbrakebailoff, impulse),
        std::make_tuple(22, user_command::hornhighactivate, impulse),
        std::make_tuple(23, user_command::hornlowactivate, impulse),

        std::make_tuple(24, user_command::batterytoggle, toggle),
        std::make_tuple(25, user_command::headlighttoggleleft, toggle),
        std::make_tuple(26, user_command::headlighttoggleupper, toggle),
        std::make_tuple(27, user_command::headlighttoggleright, toggle)
    };

    sp_port *port = nullptr;
    command_relay relay;
    std::array<uint8_t, 16> old_packet;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
    Global::uart_conf_t conf;
	bool data_pending = false;

public:
    uart_input();
    ~uart_input();
    void poll();
};
