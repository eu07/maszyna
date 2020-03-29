#include "stdafx.h"
#include "zmq_input.h"
#include "Globals.h"
#include "Logs.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Train.h"

zmq_input::zmq_input()
{
    sock.emplace(ctx, zmq::socket_type::router);
    sock->bind(Global.zmq_address);

    // build helper translation tables
    std::size_t commandid = 0;
    for( auto const &description : simulation::Commands_descriptions ) {
        nametocommandmap.emplace(
            description.name,
            static_cast<user_command>( commandid ) );
        ++commandid;
    }
}

float zmq_input::unpack_float(const zmq::message_t &msg) {
    if (msg.size() < 4)
        return 0.0f;

    uint8_t *buf = (uint8_t*)msg.data();
    uint32_t v = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    return reinterpret_cast<float&>(v);
}

zmq::message_t zmq_input::pack_float(float f) {
    uint32_t v = reinterpret_cast<uint32_t&>(f);
    uint8_t buf[4];
    buf[3] = v;
    buf[2] = v >> 8;
    buf[1] = v >> 16;
    buf[0] = v >> 24;
    return zmq::message_t(buf, 4);
}

void zmq_input::poll()
{
    zmq::multipart_t multipart;
    bool ok;
    while ((ok = multipart.recv(*sock, ZMQ_DONTWAIT))) {
        if (multipart.size() < 3)
            continue;

        if (multipart[0].size() != 5)
            continue;

        uint8_t* buf = (uint8_t*)multipart[0].data();
        uint32_t peer_id = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];

        auto peer_it = peers.find(peer_id);
        if (peer_it == peers.end()) {
            peer_it = peers.emplace(peer_id, peer_state()).first;
            peer_it->second.update_interval = 60.0f;
            peer_it->second.last_update = std::chrono::high_resolution_clock::now();
        }

        if (multipart[1] == zmq::message_t("REG_SOPI", 8)) {
            if (multipart.size() < 4)
                continue;

            peer_it->second.update_interval = unpack_float(multipart[2]);
            peer_it->second.sopi_list.clear();

            for (size_t i = 3; i < multipart.size(); i++) {
                std::string chan_name((char*)multipart[i].data(), multipart[i].size());

                auto chan_it = output_fields_map.find(chan_name);
                if (chan_it != output_fields_map.end())
                    peer_it->second.sopi_list.push_back(chan_it->second);
            }
        }
        if (multipart[1] == zmq::message_t("REG_SIPO", 8)) {
            peer_it->second.sipo_list.clear();

            for (size_t i = 2; i < multipart.size(); i++) {
                std::string chan_name((char*)multipart[i].data(), multipart[i].size());

                std::istringstream stream(chan_name);
                std::string id1, id2;
                std::getline(stream, id1, '/');
                std::getline(stream, id2, '/');

                input_type type = input_type::none;
                user_command cmd1 = user_command::none, cmd2 = user_command::none;

                auto cmd1_it = nametocommandmap.find(id1);
                if (cmd1_it != nametocommandmap.end())
                    cmd1 = cmd1_it->second;

                if (id2 == "" || id2 == "v")
                    type = input_type::value;
                else if (id2 == "i")
                    type = input_type::impulse;
                else {
                    type =  input_type::toggle;
                    auto cmd2_it = nametocommandmap.find(id2);
                    if (cmd2_it != nametocommandmap.end())
                        cmd2 = cmd2_it->second;
                }

                if (cmd1 == user_command::none)
                    type = input_type::none;

                peer_it->second.sipo_list.push_back(std::make_tuple(type, cmd1, cmd2, false));
            }
        }
        if (multipart[1] == zmq::message_t("SIPO_DATA", 9)) {
            if (peer_it->second.sipo_list.size() != multipart.size() - 2) {
                zmq::multipart_t msg;

                zmq::message_t addr;
                addr.copy(multipart[0]);

                msg.add(std::move(addr));
                msg.addstr("UNCF_SIPO");
                msg.send(*sock, ZMQ_DONTWAIT);

                continue;
            }

            size_t i = 1;
            for (auto &entry : peer_it->second.sipo_list) {
                i++;

                float value = unpack_float(multipart[i]);
                input_type type = std::get<0>(entry);

                if (type == input_type::value) {
                    relay.post(std::get<1>(entry), value, 0, GLFW_PRESS, 0);
                    continue;
                }
                else if (type == input_type::none)
                    continue;

                bool state = (value > 0.5f);
                bool changed = (state != std::get<3>(entry));

                if (!changed)
                    continue;

                auto const action { (
                    type != input_type::impulse ?
                        GLFW_PRESS :
                        ( state ?
                            GLFW_PRESS :
                            GLFW_RELEASE ) ) };

                auto const command { (
                    type != input_type::toggle ?
                        std::get<1>( entry ) :
                        ( state ?
                            std::get<1>( entry ) :
                            std::get<2>( entry ) ) ) };

                std::get<3>(entry) = state;

                relay.post(command, 0, 0, action, 0);
            }
        }
    }

    auto now = std::chrono::high_resolution_clock::now();

    for (auto peer = peers.begin(); peer != peers.end();) {
        if (std::chrono::duration<float>(now - peer->second.last_update).count() < peer->second.update_interval) {
            ++peer;
            continue;
        }

        peer->second.last_update = now;

        if (peer->second.sopi_list.empty()) {
            ++peer;
            continue;
        }

        uint8_t peerbuf[5] = { 0, (uint8_t)(peer->first >> 24), (uint8_t)(peer->first >> 16), (uint8_t)(peer->first >> 8), (uint8_t)(peer->first) };
        zmq::multipart_t msg;

        msg.addmem(peerbuf, sizeof(peerbuf));
        msg.addstr("SOPI_DATA");

        for (output_fields field : peer->second.sopi_list)
            msg.add(pack_field(field));

        if (!msg.send(*sock, ZMQ_DONTWAIT))
            peer = peers.erase(peer);
        else
            ++peer;
    }
}

std::unordered_map<std::string, zmq_input::output_fields> zmq_input::output_fields_map = {
    { "shp", output_fields::shp },
    { "alerter", output_fields::alerter },
    { "radio_stop", output_fields::radio_stop },
    { "motor_resistors", output_fields::motor_resistors },
    { "line_breaker", output_fields::line_breaker },
    { "motor_overload", output_fields::motor_overload },
    { "motor_connectors", output_fields::motor_connectors },
    { "wheelslip", output_fields::wheelslip },
    { "converter_overload", output_fields::converter_overload },
    { "converter_off", output_fields::converter_off },
    { "compressor_overload", output_fields::compressor_overload },
    { "ventilator_overload", output_fields::ventilator_overload },
    { "motor_overload_threshold", output_fields::motor_overload_threshold },
    { "train_heating", output_fields::train_heating },
    { "cab", output_fields::cab },
    { "recorder_braking", output_fields::recorder_braking },
    { "recorder_power", output_fields::recorder_power },
    { "alerter_sound",output_fields:: alerter_sound },
    { "coupled_hv_voltage_relays", output_fields::coupled_hv_voltage_relays },
    { "velocity", output_fields::velocity },
    { "reservoir_pressure", output_fields::reservoir_pressure },
    { "pipe_pressure", output_fields::pipe_pressure },
    { "brake_pressure", output_fields::brake_pressure },
    { "hv_voltage", output_fields::hv_voltage },
    { "hv_current_1", output_fields::hv_current_1 },
    { "hv_current_2", output_fields::hv_current_2 },
    { "hv_current_3", output_fields::hv_current_3 },
    { "lv_voltage", output_fields::lv_voltage },
    { "distance", output_fields::distance },
    { "radio_channel", output_fields::radio_channel },
    { "springbrake_active", output_fields::springbrake_active },
    { "time_month_of_era", output_fields::time_month_of_era },
    { "time_minute_of_month", output_fields::time_minute_of_month },
    { "time_millisecond_of_day", output_fields::time_millisecond_of_day }
};

zmq::message_t zmq_input::pack_field(zmq_input::output_fields f) {
    const SYSTEMTIME time = simulation::Time.data();

    if (f == output_fields::time_month_of_era)
        return pack_float((time.wYear - 1) * 12 + time.wMonth - 1);
    if (f == output_fields::time_minute_of_month)
        return pack_float((time.wDay - 1) * 1440 + time.wHour * 60 + time.wMinute);
    if (f == output_fields::time_millisecond_of_day)
        return pack_float(time.wSecond * 1000 + time.wMilliseconds);

    TTrain *train = simulation::Train;
    if (!train)
        return pack_float(0.0f);
    const TTrain::state_t state = train->get_state();

    if (f == output_fields::shp)
        return pack_float(state.shp);
    if (f == output_fields::alerter)
        return pack_float(state.alerter);
    if (f == output_fields::radio_stop)
        return pack_float(state.radio_stop);
    if (f == output_fields::motor_resistors)
        return pack_float(state.motor_resistors);
    if (f == output_fields::line_breaker)
        return pack_float(state.line_breaker);
    if (f == output_fields::motor_overload)
        return pack_float(state.motor_overload);
    if (f == output_fields::motor_connectors)
        return pack_float(state.motor_connectors);
    if (f == output_fields::wheelslip)
        return pack_float(state.wheelslip);
    if (f == output_fields::converter_overload)
        return pack_float(state.converter_overload);
    if (f == output_fields::converter_off)
        return pack_float(state.converter_off);
    if (f == output_fields::compressor_overload)
        return pack_float(state.compressor_overload);
    if (f == output_fields::ventilator_overload)
        return pack_float(state.ventilator_overload);
    if (f == output_fields::motor_overload_threshold)
        return pack_float(state.motor_overload_threshold);
    if (f == output_fields::train_heating)
        return pack_float(state.train_heating);
    if (f == output_fields::cab)
        return pack_float(state.cab);
    if (f == output_fields::recorder_braking)
        return pack_float(state.recorder_braking);
    if (f == output_fields::recorder_power)
        return pack_float(state.recorder_power);
    if (f == output_fields::alerter_sound)
        return pack_float(state.alerter_sound);
    if (f == output_fields::coupled_hv_voltage_relays)
        return pack_float(state.coupled_hv_voltage_relays);
    if (f == output_fields::velocity)
        return pack_float(state.velocity);
    if (f == output_fields::reservoir_pressure)
        return pack_float(state.reservoir_pressure);
    if (f == output_fields::pipe_pressure)
        return pack_float(state.pipe_pressure);
    if (f == output_fields::brake_pressure)
        return pack_float(state.brake_pressure);
    if (f == output_fields::hv_voltage)
        return pack_float(state.hv_voltage);
    if (f == output_fields::hv_current_1)
        return pack_float(state.hv_current[0]);
    if (f == output_fields::hv_current_2)
        return pack_float(state.hv_current[1]);
    if (f == output_fields::hv_current_3)
        return pack_float(state.hv_current[2]);
    if (f == output_fields::lv_voltage)
        return pack_float(state.lv_voltage);
    if (f == output_fields::distance)
        return pack_float(state.distance);
    if (f == output_fields::radio_channel)
        return pack_float(state.radio_channel);
    if (f == output_fields::springbrake_active)
        return pack_float(state.springbrake_active);

    return pack_float(0.0f);
}
