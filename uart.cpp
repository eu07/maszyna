#include "stdafx.h"
#include "uart.h"

#include "Globals.h"
#include "World.h"
#include "Train.h"
#include "Logs.h"

uart_input::uart_input()
{
    conf = Global.uart_conf;

    if (sp_get_port_by_name(conf.port.c_str(), &port) != SP_OK)
        throw std::runtime_error("uart: cannot find specified port");

    if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK)
        throw std::runtime_error("uart: cannot open port");

	sp_port_config *config;

    if (sp_new_config(&config) != SP_OK ||
		sp_set_config_baudrate(config, conf.baud) != SP_OK ||
		sp_set_config_flowcontrol(config, SP_FLOWCONTROL_NONE) != SP_OK ||
		sp_set_config_bits(config, 8) != SP_OK ||
		sp_set_config_stopbits(config, 1) != SP_OK ||
		sp_set_config_parity(config, SP_PARITY_NONE) != SP_OK ||
		sp_set_config(port, config) != SP_OK)
        throw std::runtime_error("uart: cannot set config");

	sp_free_config(config);

    if (sp_flush(port, SP_BUF_BOTH) != SP_OK)
        throw std::runtime_error("uart: cannot flush");

    old_packet.fill(0);
    last_update = std::chrono::high_resolution_clock::now();
}

uart_input::~uart_input()
{
	std::array<uint8_t, 31> buffer = { 0 };
	sp_blocking_write(port, (void*)buffer.data(), buffer.size(), 0);
	sp_drain(port);

    sp_close(port);
    sp_free_port(port);
}

#define SPLIT_INT16(x) (uint8_t)x, (uint8_t)(x >> 8)

void uart_input::poll()
{
    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration<float>(now - last_update).count() < conf.updatetime)
        return;
    last_update = now;

    auto *t = Global.pWorld->train();
	if (!t)
		return;

    sp_return ret;

    if ((ret = sp_input_waiting(port)) >= 16)
    {
        std::array<uint8_t, 16> buffer;
        ret = sp_blocking_read(port, (void*)buffer.data(), buffer.size(), 0);
		if (ret < 0)
            throw std::runtime_error("uart: failed to read from port");

		if (conf.debug)
		{
			char buf[buffer.size() * 3 + 1];
			size_t pos = 0;
			for (uint8_t b : buffer)
				pos += sprintf(&buf[pos], "%02X ", b);
			WriteLog("uart: rx: " + std::string(buf));
		}

		data_pending = false;

        for (auto entry : input_bits)
        {
            input_type_t type = std::get<2>(entry);

            size_t byte = std::get<0>(entry) / 8;
            size_t bit = std::get<0>(entry) % 8;

            bool state = ( (buffer[byte] & (1 << bit)) != 0 );

			bool repeat = (type == impulse_r || 
			               type == impulse_r_off ||
			               type == impulse_r_on);

            bool changed = ( (buffer[ byte ] & (1 << bit)) != (old_packet[ byte ] & (1 << bit)) );

            if (!changed && !(repeat && state))
                continue;

            int action;
            command_hint desired_state;

            if (type == toggle)
            {
                action = GLFW_PRESS;
                desired_state = ( state ? command_hint::on : command_hint::off );
            }
            else if (type == impulse_r_on)
            {
                action = ( state ? (changed ? GLFW_PRESS : GLFW_REPEAT) : GLFW_RELEASE );
                desired_state = command_hint::on;
            }
            else if (type == impulse_r_off)
            {
                action = ( state ? (changed ? GLFW_PRESS : GLFW_REPEAT) : GLFW_RELEASE );
                desired_state = command_hint::off;
            }
            else if (type == impulse || type == impulse_r)
            {
                action = ( state ? (changed ? GLFW_PRESS : GLFW_REPEAT) : GLFW_RELEASE );
                desired_state = command_hint::none;
            }

            relay.post( std::get<1>(entry), 0, 0, action, desired_state, 0 );
        }

        if( true == conf.mainenable ) {
            // master controller
            relay.post(
                user_command::mastercontrollerset,
                buffer[ 6 ],
                0,
                GLFW_PRESS,
                command_hint::none,
                // TODO: pass correct entity id once the missing systems are in place
                0 );
        }
        if( true == conf.scndenable ) {
            // second controller
            relay.post(
                user_command::secondcontrollerset,
                buffer[ 7 ],
                0,
                GLFW_PRESS,
                command_hint::none,
                // TODO: pass correct entity id once the missing systems are in place
                0 );
        }
        if( true == conf.trainenable ) {
            // train brake
            double const position { (float)( ( (uint16_t)buffer[ 8 ] | ( (uint16_t)buffer[ 9 ] << 8 ) ) - conf.mainbrakemin ) / ( conf.mainbrakemax - conf.mainbrakemin ) };
            relay.post(
                user_command::trainbrakeset,
                reinterpret_cast<std::uint64_t const &>( position ),
                0,
                GLFW_PRESS,
                command_hint::none,
                // TODO: pass correct entity id once the missing systems are in place
                0 );
        }
        if( true == conf.localenable ) {
            // independent brake
            double const position { (float)( ( (uint16_t)buffer[ 10 ] | ( (uint16_t)buffer[ 11 ] << 8 ) ) - conf.localbrakemin ) / ( conf.localbrakemax - conf.localbrakemin ) };
            relay.post(
                user_command::independentbrakeset,
                reinterpret_cast<std::uint64_t const &>( position ),
                0,
                GLFW_PRESS,
                command_hint::none,
                // TODO: pass correct entity id once the missing systems are in place
                0 );
        }

        old_packet = buffer;
    }

	if (!data_pending && sp_output_waiting(port) == 0)
	{
	    // TODO: ugly! move it into structure like input_bits
        auto const trainstate = t->get_state();

	    uint8_t tacho = Global.iPause ? 0 : trainstate.velocity;
	    uint16_t tank_press = (uint16_t)std::min(conf.tankuart, trainstate.reservoir_pressure * 0.1f / conf.tankmax * conf.tankuart);
	    uint16_t pipe_press = (uint16_t)std::min(conf.pipeuart, trainstate.pipe_pressure * 0.1f / conf.pipemax * conf.pipeuart);
	    uint16_t brake_press = (uint16_t)std::min(conf.brakeuart, trainstate.brake_pressure * 0.1f / conf.brakemax * conf.brakeuart);
	    uint16_t hv_voltage = (uint16_t)std::min(conf.hvuart, trainstate.hv_voltage / conf.hvmax * conf.hvuart);
	    uint16_t current1 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[0]) / conf.currentmax * conf.currentuart;
	    uint16_t current2 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[1]) / conf.currentmax * conf.currentuart;
	    uint16_t current3 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[2]) / conf.currentmax * conf.currentuart;

	    std::array<uint8_t, 31> buffer {
            //byte 0
			tacho,
            //byte 1
	        0,
            //byte 2
			(uint8_t)(
                trainstate.motor_resistors << 1
              | trainstate.motor_overload_threshold << 2),
            //byte 3
			0,
            //byte 4
			(uint8_t)(
                trainstate.train_heating << 0
              | trainstate.motor_resistors << 1
              | trainstate.wheelslip << 2
              | trainstate.alerter << 6
              | trainstate.shp << 7),
            //byte 5
			(uint8_t)(
                trainstate.motor_connectors << 0
              | trainstate.converter_overload << 2
              | trainstate.motor_overload << 4
              | trainstate.line_breaker << 5
              | trainstate.compressor_overload << 6),
            //byte 6
			(uint8_t)( trainstate.alerter_sound << 7),
	        SPLIT_INT16(brake_press), //byte 7-8
	        SPLIT_INT16(pipe_press), //byte 9-10
	        SPLIT_INT16(tank_press), //byte 11-12
	        SPLIT_INT16(hv_voltage), //byte 13-14
	        SPLIT_INT16(current1), //byte 15-16
	        SPLIT_INT16(current2), //byte 17-18
	        SPLIT_INT16(current3), //byte 19-20
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0 //byte 21-30
	    };

		if (conf.debug)
		{
			char buf[buffer.size() * 3 + 1];
			size_t pos = 0;
			for (uint8_t b : buffer)
				pos += sprintf(&buf[pos], "%02X ", b);
			WriteLog("uart: tx: " + std::string(buf));
		}

	    ret = sp_blocking_write(port, (void*)buffer.data(), buffer.size(), 0);
	    if (ret != buffer.size())
			throw std::runtime_error("uart: failed to write to port");

		data_pending = true;
	}
}
