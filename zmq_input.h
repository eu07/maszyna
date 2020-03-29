#pragma once

#include <zmq_addon.hpp>
#include "command.h"

class zmq_input
{
    enum class output_fields {
        shp,
        alerter,
        radio_stop,
        motor_resistors,
        line_breaker,
        motor_overload,
        motor_connectors,
        wheelslip,
        converter_overload,
        converter_off,
        compressor_overload,
        ventilator_overload,
        motor_overload_threshold,
        train_heating,
        cab,
        recorder_braking,
        recorder_power,
        alerter_sound,
        coupled_hv_voltage_relays,
        velocity,
        reservoir_pressure,
        pipe_pressure,
        brake_pressure,
        hv_voltage,
        hv_current_1,
        hv_current_2,
        hv_current_3,
        lv_voltage,
        distance,
        radio_channel,
        springbrake_active,
        time_month_of_era,
        time_minute_of_month,
        time_millisecond_of_day
    };

    zmq::context_t ctx;
    std::optional<zmq::socket_t> sock;

    enum class input_type
    {
        none,
        toggle, // two commands, each mapped to one state; press event on state change
        impulse, // one command; press event when set, release when cleared
        value // one command; press event, value of specified byte passed as param1
    };

    struct peer_state {
        float update_interval;
        std::vector<output_fields> sopi_list;
        std::vector<std::tuple<input_type, user_command, user_command, bool>> sipo_list;;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
    };

    std::map<uint32_t, peer_state> peers;

    float unpack_float(const zmq::message_t &);
    zmq::message_t pack_float(float f);
    zmq::message_t pack_field(output_fields f);

    std::unordered_map<std::string, user_command> nametocommandmap;

    static std::unordered_map<std::string, output_fields> output_fields_map;
    command_relay relay;

public:
    zmq_input();
    void poll();
};
