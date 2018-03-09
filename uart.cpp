#include "stdafx.h"
#include "uart.h"

#include "Globals.h"
#include "World.h"
#include "Train.h"
#include "parser.h"
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
	std::array<std::uint8_t, 31> buffer = { 0 };
	sp_blocking_write(port, (void*)buffer.data(), buffer.size(), 0);
	sp_drain(port);

    sp_close(port);
    sp_free_port(port);
}

bool
uart_input::recall_bindings() {

    m_inputbindings.clear();

    cParser bindingparser( "eu07_input-uart.ini", cParser::buffer_FILE );
    if( false == bindingparser.ok() ) {
        return false;
    }

    // build helper translation tables
    std::unordered_map<std::string, user_command> nametocommandmap;
    std::size_t commandid = 0;
    for( auto const &description : simulation::Commands_descriptions ) {
        nametocommandmap.emplace(
            description.name,
            static_cast<user_command>( commandid ) );
        ++commandid;
    }
    std::unordered_map<std::string, input_type_t> nametotypemap {
        { "impulse", input_type_t::impulse },
        { "toggle", input_type_t::toggle },
        { "value", input_type_t::value } };

    // NOTE: to simplify things we expect one entry per line, and whole entry in one line
    while( true == bindingparser.getTokens( 1, true, "\n" ) ) {

        std::string bindingentry;
        bindingparser >> bindingentry;
        cParser entryparser( bindingentry );

        if( true == entryparser.getTokens( 2, true, "\n\r\t " ) ) {

            std::size_t bindingpin {};
            std::string bindingtypename {};
            entryparser
                >> bindingpin
                >> bindingtypename;

            auto const typelookup = nametotypemap.find( bindingtypename );
            if( typelookup == nametotypemap.end() ) {

                WriteLog( "Uart binding for input pin " + std::to_string( bindingpin ) + " specified unknown control type, \"" + bindingtypename + "\"" );
            }
            else {

                auto const bindingtype { typelookup->second };
                std::array<user_command, 2> bindingcommands { user_command::none, user_command::none };
                auto const commandcount { ( bindingtype == toggle ? 2 : 1 ) };
                for( int commandidx = 0; commandidx < commandcount; ++commandidx ) {
                    // grab command(s) associated with the input pin
                    auto const bindingcommandname { entryparser.getToken<std::string>() };
                    if( true == bindingcommandname.empty() ) {
                        // no tokens left, may as well complain then call it a day
                        WriteLog( "Uart binding for input pin " + std::to_string( bindingpin ) + " didn't specify associated command(s)" );
                        break;
                    }
                    auto const commandlookup = nametocommandmap.find( bindingcommandname );
                    if( commandlookup == nametocommandmap.end() ) {
                        WriteLog( "Uart binding for input pin " + std::to_string( bindingpin ) + " specified unknown command, \"" + bindingcommandname + "\"" );
                    }
                    else {
                        bindingcommands[ commandidx ] = commandlookup->second;
                    }
                }
                // push the binding on the list
                m_inputbindings.emplace_back( bindingpin, bindingtype, bindingcommands[ 0 ], bindingcommands[ 1 ] );
            }
        }
    }

    return true;
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
        std::array<uint8_t, 16> buffer; // TBD, TODO: replace with vector of configurable size?
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

        for (auto const &entry : m_inputbindings) {

            auto const byte { std::get<std::size_t>( entry ) / 8 };
            auto const bit { std::get<std::size_t>( entry ) % 8 };

            bool const state { ( ( buffer[ byte ] & ( 1 << bit ) ) != 0 ) };
            bool const changed { ( ( old_packet[ byte ] & ( 1 << bit ) ) != state ) };

            if( false == changed ) { continue; }

            auto const type { std::get<input_type_t>( entry ) };
            auto const action { (
                type != impulse ?
                    GLFW_PRESS :
                    ( state ?
                        GLFW_PRESS :
                        GLFW_RELEASE ) ) };

            auto const command { (
                type != toggle ?
                    std::get<2>( entry ) :
                    ( state ?
                        std::get<2>( entry ) :
                        std::get<3>( entry ) ) ) };

            // TODO: pass correct entity id once the missing systems are in place
            relay.post( command, 0, 0, action, 0 );
        }

        if( true == conf.mainenable ) {
            // master controller
            relay.post(
                user_command::mastercontrollerset,
                buffer[ 6 ],
                0,
                GLFW_PRESS,
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
	    uint16_t current1 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[0] / conf.currentmax * conf.currentuart);
	    uint16_t current2 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[1] / conf.currentmax * conf.currentuart);
	    uint16_t current3 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[2] / conf.currentmax * conf.currentuart);

	    std::array<uint8_t, 31> buffer {
            //byte 0
			tacho,
            //byte 1
	        0,
            //byte 2
			(uint8_t)(
                trainstate.ventilator_overload << 1
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
			(uint8_t)(
                trainstate.recorder_braking << 3
              | trainstate.recorder_power << 4
              | trainstate.alerter_sound << 7),
            //byte 7-8
	        SPLIT_INT16(brake_press),
            //byte 9-10
	        SPLIT_INT16(pipe_press),
            //byte 11-12
	        SPLIT_INT16(tank_press),
            //byte 13-14
	        SPLIT_INT16(hv_voltage),
            //byte 15-16
	        SPLIT_INT16(current1),
            //byte 17-18
	        SPLIT_INT16(current2),
            //byte 19-20
	        SPLIT_INT16(current3),
            //byte 21-30
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
