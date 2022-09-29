#include "stdafx.h"
#include "uart.h"

#include "Globals.h"
#include "simulation.h"
#include "Train.h"
#include "parser.h"
#include "Logs.h"
#include "simulationtime.h"
#include "application.h"

const char* uart_baudrates_list[] = {
    "300",
    "1200",
    "2400",
    "4800",
    "9600",
    "19200",
    "38400",
    "57600",
    "74880",
    "115200",
    "230400",
    "250000",
    "500000",
    "1000000",
    "2000000"
};

const size_t uart_baudrates_list_num = (
        sizeof(uart_baudrates_list)/sizeof(uart_baudrates_list[0])
    );

void uart_status::reset_stats() {
    packets_sent = 0;
    packets_received = 0;
}

uart_status UartStatus;

uart_input::uart_input()
{
    conf = Global.uart_conf;
    uart_status *status = &UartStatus;

    status->enabled = conf.enable;
    status->port_name = conf.port;
    status->baud = conf.baud;

    const std::string baudStr = std::to_string(status->baud);

    for(int i=0;i<uart_baudrates_list_num;i++) {
        if(baudStr == uart_baudrates_list[i]) {
            status->selected_baud_index = i;
            status->active_port_index = i;
            break;
        }
    }

    old_packet.fill(0);
    last_update = std::chrono::high_resolution_clock::now();
    last_setup = std::chrono::high_resolution_clock::now();

    find_ports();
}

void uart_input::find_ports() {
    uart_status *status = &UartStatus;

    struct sp_port **ports;
    if (sp_list_ports(&ports) == SP_OK) {
        status->available_ports.clear();
        status->active_port_index = -1;
        status->selected_port_index = -1;
        for (int i=0; ports[i]; i++) {
            std::string newport = std::string(sp_get_port_name(ports[i]));
            status->available_ports.emplace_back(newport);
            if(newport == status->port_name) {
                status->active_port_index = i;
                status->selected_port_index = i;
            }
        }
        if(status->selected_port_index > status->available_ports.size()) {
            status->selected_port_index = -1;
        }
        sp_free_port_list(ports);
    } else {
        WriteLog("uart: cannot read serial ports list");
    }
    last_port_find = std::chrono::high_resolution_clock::now();
}

bool uart_input::setup_port()
{
    uart_status *status = &UartStatus;

    if(!port) {
        find_ports();
    }
    if (port) {
      sp_close(port);
      sp_free_port(port);
      port = nullptr;
    }

    last_setup = std::chrono::high_resolution_clock::now();

    if (sp_get_port_by_name(status->port_name.c_str(), &port) != SP_OK) {
        if(!error_notified) {
            status->is_connected = false;
            ErrorLog("uart: cannot find specified port '"+conf.port+"'");
            find_ports();
        }
        error_notified = true;
        return false;
    }

    if (sp_open(port, (sp_mode)(SP_MODE_READ | SP_MODE_WRITE)) != SP_OK) {
        if(!error_notified) {
            status->is_connected = false;
            ErrorLog("uart: cannot open port '"+status->port_name+"'");
            find_ports();
        }
        error_notified = true;
        port = nullptr;
        return false;
    }

	sp_port_config *config;

    if (sp_new_config(&config) != SP_OK ||
		sp_set_config_baudrate(config, status->baud) != SP_OK ||
		sp_set_config_flowcontrol(config, SP_FLOWCONTROL_NONE) != SP_OK ||
		sp_set_config_bits(config, 8) != SP_OK ||
		sp_set_config_stopbits(config, 1) != SP_OK ||
		sp_set_config_parity(config, SP_PARITY_NONE) != SP_OK ||
		sp_set_config(port, config) != SP_OK) {
        if(!error_notified) {
            status->is_connected = false;
            ErrorLog("uart: cannot set config");
        }
        error_notified = true;
        port = nullptr;
        find_ports();
        return false;
    }

	sp_free_config(config);

    if (sp_flush(port, SP_BUF_BOTH) != SP_OK) {
        if(!error_notified) {
            status->is_connected = false;
            ErrorLog("uart: cannot flush");
        }
        error_notified = true;
        port = nullptr;
        find_ports();
        return false;
    }

    if(error_notified || ! status->is_connected) {
        error_notified = false;
        ErrorLog("uart: connected to '"+status->port_name+"'");
        status->reset_stats();
        status->is_connected = true;
        data_pending = false;
    }

    return true;
}

uart_input::~uart_input()
{
  if (!port)
    return;

	std::array<std::uint8_t, 52> buffer = { 0 };
	buffer[0] = 0xEF;
	buffer[1] = 0xEF;
	buffer[2] = 0xEF;
	buffer[3] = 0xEF;
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
    while( true == bindingparser.getTokens( 1, true, "\n\r" ) ) {

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
                auto const commandcount { ( bindingtype == input_type_t::toggle ? 2 : 1 ) };
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

#define SPLIT_INT16(x) (uint8_t)(x), (uint8_t)((x) >> 8)

void uart_input::poll()
{
    uart_status *status = &UartStatus;
    auto now = std::chrono::high_resolution_clock::now();

    /* handle baud change */
    if(status->active_baud_index != status->selected_baud_index) {
        status->baud = std::stoul(uart_baudrates_list[status->selected_baud_index]);
        status->active_baud_index = status->selected_baud_index;
        status->reset_stats();
        status->is_connected = false;
        setup_port();
    }

    /* handle port change */
    if(status->available_ports.size() > 0 && status->selected_port_index >= 0 && status->active_port_index != status->selected_port_index) {
        status->port_name = status->available_ports[status->selected_port_index];
        status->active_port_index = status->selected_port_index;
        status->reset_stats();
        status->is_connected = false;
        setup_port();
    }

    if (
        (!port && std::chrono::duration<float>(now - last_port_find).count() > 1.0)
        || (port && std::chrono::duration<float>(now - last_port_find).count() > 5.0)
    ) {
        find_ports();
    }

    if(!status->enabled) {
        if(port) {
          sp_close(port);
          sp_free_port(port);
          port = nullptr;
        }
        status->is_connected = false;
        return;
    }

    if (std::chrono::duration<float>(now - last_update).count() < conf.updatetime)
        return;
    last_update = now;


    /* if connection error occured, slow down reconnection tries */
    if (!port && error_notified && std::chrono::duration<float>(now - last_setup).count() < 1.0) {
        return;
    }


    if (!port) {
      setup_port();
      return;
    }

    auto const *t =simulation::Train;
	if (!t)
		return;

    sp_return ret;

    if ((ret = sp_input_waiting(port)) >= 20)
    {
        std::array<uint8_t, 20> tmp_buffer; // TBD, TODO: replace with vector of configurable size?
        ret = sp_blocking_read(port, (void*)tmp_buffer.data(), tmp_buffer.size(), 0);

        if (ret < 0) {
          setup_port();
          return;
        }

		bool sync;
		if (tmp_buffer[0] != 0xEF || tmp_buffer[1] != 0xEF || tmp_buffer[2] != 0xEF || tmp_buffer[3] != 0xEF) {
            UartStatus.is_synced = false;
			if (conf.debug)
				WriteLog("uart: bad sync");
			sync = false;
		}
		else {
            UartStatus.is_synced = true;
			if (conf.debug)
				WriteLog("uart: sync ok");
			sync = true;
		}

		if (!sync) {
			int sync_cnt = 0;
			int sync_fail = 0;
			unsigned char sc = 0;
			while ((ret = sp_blocking_read(port, &sc, 1, 10)) >= 0) {
				if (conf.debug)
					WriteLog("uart: read byte: " + std::to_string((int)sc));
				if (sc == 0xEF)
					sync_cnt++;
				else {
					sync_cnt = 0;
					sync_fail++;
				}
				if (sync_cnt == 4) {
					if (conf.debug)
						WriteLog("uart: synced, skipping one packet..");
					ret = sp_blocking_read(port, (void*)tmp_buffer.data(), 16, 500);
					  if (ret < 0) {
              setup_port();
              return;
          }
					data_pending = false;
					break;
				}
				if (sync_fail >= 60) {
					if (conf.debug)
						WriteLog("uart: tired of trying");
					break;
				}
			}
			if (ret < 0) {
        setup_port();
      }
			return;
		}

		std::array<uint8_t, 16> buffer;
		memmove(&buffer[0], &tmp_buffer[4], 16);
        UartStatus.packets_received++;

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
            bool const changed { ( ( ( old_packet[ byte ] & ( 1 << bit ) ) != 0 ) != state ) };

            if( false == changed ) { continue; }

            auto const type { std::get<input_type_t>( entry ) };
            auto const action { (
                type != input_type_t::impulse ?
                    GLFW_PRESS :
                    ( state ?
                        GLFW_PRESS :
                        GLFW_RELEASE ) ) };

            auto const command { (
                type != input_type_t::toggle ?
                    std::get<2>( entry ) :
                    ( state ?
                        std::get<2>( entry ) :
                        std::get<3>( entry ) ) ) };

            // TODO: pass correct entity id once the missing systems are in place
			relay.post( command, 0, 0, action, 0 );
        }

        if( true == conf.mainenable ) {
            // master controller
			if (false == conf.mainpercentage) {
				//old method for direct positions
				relay.post(
					user_command::mastercontrollerset,
					buffer[6],
					0,
					GLFW_PRESS,
					// TODO: pass correct entity id once the missing systems are in place
					0);
			}
			else {
				auto desiredpercent{ buffer[6] * 0.01 };
				auto desiredposition{ desiredpercent > 0.01 ? 1 + ((simulation::Train->Occupied()->MainCtrlPosNo - 1) * desiredpercent) : buffer[6] };
				relay.post(
					user_command::mastercontrollerset,
					desiredposition,
					0,
					GLFW_PRESS,
					// TODO: pass correct entity id once the missing systems are in place
					0);
				simulation::Train->Occupied()->eimic_analog = desiredpercent;
			}
        }
        if( true == conf.scndenable ) {
            // second controller
            relay.post(
                user_command::secondcontrollerset,
                static_cast<int8_t>(buffer[ 7 ]),
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
                position,
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
                position,
                0,
                GLFW_PRESS,
                // TODO: pass correct entity id once the missing systems are in place
			    0 );
        }
        if( true == conf.radiochannelenable ) {
            relay.post(
                user_command::radiochannelset,
                static_cast<int8_t>(buffer[12] & 0xF),
                0,
                GLFW_PRESS,
                0
            );
        }
        if( true == conf.radiovolumeenable ) {
            int8_t requested_volume = static_cast<int8_t>((buffer[12] & 0xF0) >> 4);
            relay.post(
                user_command::radiovolumeset,
                requested_volume == 0xF ? 1.0 : requested_volume * (1.0 / 15.0),
                0,
                GLFW_PRESS,
                0
            );
        }

        old_packet = buffer;
    }

  if (ret < 0) {
    setup_port();
    return;
  }

	if (!data_pending && sp_output_waiting(port) == 0)
	{
	    // TODO: ugly! move it into structure like input_bits
        auto const trainstate = t->get_state();

		SYSTEMTIME time = simulation::Time.data();
		uint16_t tacho = Global.iPause ? 0 : (trainstate.velocity * conf.tachoscale);
	    uint16_t tank_press = (uint16_t)std::min(conf.tankuart, trainstate.reservoir_pressure * 0.1f / conf.tankmax * conf.tankuart);
	    uint16_t pipe_press = (uint16_t)std::min(conf.pipeuart, trainstate.pipe_pressure * 0.1f / conf.pipemax * conf.pipeuart);
	    uint16_t brake_press = (uint16_t)std::min(conf.brakeuart, trainstate.brake_pressure * 0.1f / conf.brakemax * conf.brakeuart);
        uint16_t pantograph_press = (uint16_t)std::min(conf.pantographuart, trainstate.pantograph_pressure * 0.1f / conf.pantographmax * conf.pantographuart );
        uint16_t hv_voltage = (uint16_t)std::min(conf.hvuart, trainstate.hv_voltage / conf.hvmax * conf.hvuart);
	    uint16_t current1 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[0] / conf.currentmax * conf.currentuart);
	    uint16_t current2 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[1] / conf.currentmax * conf.currentuart);
	    uint16_t current3 = (uint16_t)std::min(conf.currentuart, trainstate.hv_current[2] / conf.currentmax * conf.currentuart);
	    uint32_t odometer = trainstate.distance * 10000.0;
        uint16_t lv_voltage = (uint16_t)std::min( conf.lvuart, trainstate.lv_voltage / conf.lvmax * conf.lvuart );
        if( trainstate.cab > 0 ) {
            // NOTE: moving from a cab to engine room doesn't change cab indicator
            m_trainstatecab = trainstate.cab - 1;
        }

	    std::array<uint8_t, 52> buffer {
			//preamble
			0xEF, 0xEF, 0xEF, 0xEF,
			//byte 0-1 (counting without preamble)
			SPLIT_INT16(tacho),
            //byte 2
			(uint8_t)(
                trainstate.epbrake_enabled << 0
              | trainstate.ventilator_overload << 1
              | trainstate.motor_overload_threshold << 2
			  | trainstate.emergencybrake << 3
			  | trainstate.lockpipe << 4
			  | trainstate.dir_forward << 5
			  | trainstate.dir_backward << 6),
            //byte 3
			(uint8_t)(
                trainstate.coupled_hv_voltage_relays << 0
			  | trainstate.doorleftallowed << 1
			  | trainstate.doorleftopened << 2
			  | trainstate.doorrightallowed << 3
			  | trainstate.doorrightopened << 4
			  | trainstate.doorstepallowed << 5
			  | trainstate.battery << 6
			  | trainstate.radiomessageindicator << 7),
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
              | trainstate.ground_relay << 3
              | trainstate.motor_overload << 4
              | trainstate.line_breaker << 5
              | trainstate.compressor_overload << 6),
            //byte 6
			(uint8_t)(
                m_trainstatecab << 2
              | trainstate.recorder_braking << 3
              | trainstate.recorder_power << 4
			  | trainstate.radio_stop << 5
			  | trainstate.springbrake_active << 6
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
			//byte 21-22
			SPLIT_INT16((time.wYear - 1) * 12 + time.wMonth - 1),
			//byte 23-24
			SPLIT_INT16((time.wDay - 1) * 1440 + time.wHour * 60 + time.wMinute),
			//byte 25-26
			SPLIT_INT16(time.wSecond * 1000 + time.wMilliseconds),
			//byte 27-30
			SPLIT_INT16((uint16_t)odometer), SPLIT_INT16((uint16_t)(odometer >> 16)),
			//byte 31-32
			SPLIT_INT16(lv_voltage),
			//byte 33
			(uint8_t)trainstate.radio_channel,
            //byte 34-35
            SPLIT_INT16(pantograph_press),
			//byte 36-47
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	    };

		if (conf.debug)
		{
			char buf[buffer.size() * 3 + 1];
            buf[ buffer.size() * 3 ] = 0;
			size_t pos = 0;
			for (uint8_t b : buffer)
				pos += sprintf(&buf[pos], "%02X ", b);
			WriteLog("uart: tx: " + std::string(buf));
		}

	    ret = sp_blocking_write(port, (void*)buffer.data(), buffer.size(), 0);
			if (ret < 0) {
        setup_port();
        return;
      }
        UartStatus.packets_sent++;

		data_pending = true;
	}
}

bool uart_input::is_connected() {
    return (port != nullptr);
}
