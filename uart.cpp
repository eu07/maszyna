#include "stdafx.h"
#include "uart.h"
#include "Globals.h"
#include "World.h"
#include "Train.h"
#include "Logs.h"

uart_input::uart_input()
{
    conf = Global::uart_conf;

    if (sp_get_port_by_name(conf.port.c_str(), &port) != SP_OK)
        throw std::runtime_error("uart: cannot find specified port");

    if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK)
        throw std::runtime_error("uart: cannot open port");

    if (sp_set_baudrate(port, conf.baud) != SP_OK)
        throw std::runtime_error("uart: cannot set baudrate");

    if (sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE) != SP_OK)
        throw std::runtime_error("uart: cannot set flowcontrol");

    if (sp_flush(port, SP_BUF_BOTH) != SP_OK)
        throw std::runtime_error("uart: cannot flush");

    old_packet.fill(0);
    last_update = std::chrono::high_resolution_clock::now();
}

uart_input::~uart_input()
{
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

    TTrain *t = Global::pWorld->train();

    sp_return ret;

    if (sp_input_waiting(port) >= 16)
    {
        std::array<uint8_t, 16> buffer;
        ret = sp_blocking_read(port, (void*)buffer.data(), buffer.size(), 0);
		if (ret < 0)
            throw std::runtime_error("uart: failed to read from port");
        
		sp_drain(port);
		data_pending = false;

        for (auto entry : input_bits)
        {
            input_type_t type = std::get<2>(entry);

            size_t byte = std::get<0>(entry) / 8;
            size_t bit = std::get<0>(entry) % 8;

            bool state = (bool)(buffer[byte] & (1 << bit));

			bool repeat = (type == impulse_r || 
			               type == impulse_r_off ||
			               type == impulse_r_on);

			bool changed = (state != (bool)(old_packet[byte] & (1 << bit)));

            if (!changed && !(repeat && state))
                continue;

            int action;
            command_data::desired_state_t desired_state;

            if (type == toggle)
            {
                action = GLFW_PRESS;
                desired_state = state ? command_data::ON : command_data::OFF;
            }
            else if (type == impulse_r_on)
            {
                action = state ? (changed ? GLFW_PRESS : GLFW_REPEAT) : GLFW_RELEASE;
                desired_state = command_data::ON;
            }
            else if (type == impulse_r_off)
            {
                action = state ? (changed ? GLFW_PRESS : GLFW_REPEAT) : GLFW_RELEASE;
                desired_state = command_data::OFF;
            }
            else if (type == impulse || type == impulse_r)
            {
                action = state ? (changed ? GLFW_PRESS : GLFW_REPEAT) : GLFW_RELEASE;
                desired_state = command_data::TOGGLE;
            }

            relay.post(std::get<1>(entry), 0, 0, action, 0, desired_state);
        }

        int mainctrl = buffer[6];
        int scndctrl = buffer[7];
        float trainbrake = (float)(((uint16_t)buffer[8] | ((uint16_t)buffer[9] << 8)) - conf.mainbrakemin) / (conf.mainbrakemax - conf.mainbrakemin);
        float localbrake = (float)(((uint16_t)buffer[10] | ((uint16_t)buffer[11] << 8)) - conf.mainbrakemin) / (conf.localbrakemax - conf.localbrakemin);

        t->set_mainctrl(mainctrl);
        t->set_scndctrl(scndctrl);
        t->set_trainbrake(trainbrake);
        t->set_localbrake(localbrake);

        old_packet = buffer;
    }

	if (!data_pending && sp_output_waiting(port) == 0)
	{
	    // TODO: ugly! move it into structure like input_bits

	    uint8_t buzzer = (uint8_t)t->get_alarm();
	    uint8_t tacho = (uint8_t)t->get_tacho();
	    uint16_t tank_press = (uint16_t)std::min(conf.tankuart, t->get_tank_pressure() / conf.tankmax * conf.tankuart);
	    uint16_t pipe_press = (uint16_t)std::min(conf.pipeuart, t->get_pipe_pressure() / conf.pipemax * conf.pipeuart);
	    uint16_t brake_press = (uint16_t)std::min(conf.brakeuart, t->get_brake_pressure() / conf.brakemax * conf.brakeuart);
	    uint16_t hv_voltage = (uint16_t)std::min(conf.hvuart, t->get_hv_voltage() / conf.hvmax * conf.hvuart);
	    auto current = t->get_current();
	    uint16_t current1 = (uint16_t)std::min(conf.currentuart, current[0]) / conf.currentmax * conf.currentuart;
	    uint16_t current2 = (uint16_t)std::min(conf.currentuart, current[1]) / conf.currentmax * conf.currentuart;
	    uint16_t current3 = (uint16_t)std::min(conf.currentuart, current[2]) / conf.currentmax * conf.currentuart;

	    std::array<uint8_t, 31> buffer =
	    {
	        0, 0, //byte 0-1
			tacho, //byte 2
	        0, 0, 0, 0, 0, 0, //byte 3-8
	        SPLIT_INT16(brake_press), //byte 9-10
	        SPLIT_INT16(pipe_press), //byte 11-12
	        SPLIT_INT16(tank_press), //byte 13-14
	        SPLIT_INT16(hv_voltage), //byte 15-16
	        SPLIT_INT16(current1), //byte 17-18
	        SPLIT_INT16(current2), //byte 19-20
	        SPLIT_INT16(current3), //byte 21-22
			0, 0, 0, 0, 0, 0, 0, 0 //byte 23-30
	    };

	    buffer[4] |= t->btLampkaOpory.b() << 1;
	    buffer[4] |= t->btLampkaWysRozr.b() << 2;

	    buffer[6] |= t->btLampkaOgrzewanieSkladu.b() << 0;
	    buffer[6] |= t->btLampkaOpory.b() << 1;
	    buffer[6] |= t->btLampkaPoslizg.b() << 2;
	    buffer[6] |= t->btLampkaCzuwaka.b() << 6;
	    buffer[6] |= t->btLampkaSHP.b() << 7;

	    buffer[7] |= t->btLampkaStyczn.b() << 0;
	    buffer[7] |= t->btLampkaNadmPrzetw.b() << 2;
	    buffer[7] |= t->btLampkaNadmSil.b() << 4;
	    buffer[7] |= t->btLampkaWylSzybki.b() << 5;
	    buffer[7] |= t->btLampkaNadmSpr.b() << 6;

	    buffer[8] |= buzzer << 7;

		sp_flush(port, SP_BUF_INPUT); // flush input buffer in preparation for reply packet

	    ret = sp_blocking_write(port, (void*)buffer.data(), buffer.size(), 0);
	    if (ret != buffer.size())
			throw std::runtime_error("uart: failed to write to port");

		data_pending = true;
	}
}
