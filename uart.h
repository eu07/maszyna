#pragma once

#include <libserialport.h>
#include "command.h"

extern const char* uart_baudrates_list[];
extern const size_t uart_baudrates_list_num;

class uart_status {
    public:
        std::string port_name = "";
        std::vector<std::string> available_ports = {};
        int selected_port_index = -1;
        int selected_baud_index = -1;
        int active_port_index = -1;
        int active_baud_index = -1;
        int baud = 0;
        bool enabled = false;
        bool is_connected = false;
        bool is_synced = false;
        unsigned long packets_sent = 0;
        unsigned long packets_received = 0;

        void reset_stats();
};

class uart_input
{
public:
// types
    struct conf_t {
        bool enable = false;
        std::string port;
        int baud;
        float updatetime;

        float mainbrakemin = 0.0f;
        float mainbrakemax = 65535.0f;
        float localbrakemin = 0.0f;
        float localbrakemax = 65535.0f;
        float tankmax = 10.0f;
        float tankuart = 65535.0f;
        float pipemax = 10.0f;
        float pipeuart = 65535.0f;
        float brakemax = 10.0f;
        float brakeuart = 65535.0f;
        float pantographmax = 10.0f;
        float pantographuart = 65535.0f;
        float hvmax = 100000.0f;
        float hvuart = 65535.0f;
        float currentmax = 10000.0f;
        float currentuart = 65535.0f;
        float lvmax = 150.0f;
        float lvuart = 65535.0f;
		float tachoscale = 1.0f;

        bool mainenable = true;
        bool scndenable = true;
        bool trainenable = true;
        bool localenable = true;
        bool radiovolumeenable = false;
        bool radiochannelenable = false;

        bool debug = false;

		bool mainpercentage = false;
    };

// methods
    uart_input();
    ~uart_input();
    bool
        init() { return recall_bindings(); }
    bool
        recall_bindings();
    void
        poll();
    bool
        is_connected();

private:
// types
    enum class input_type_t
    {
        toggle, // two commands, each mapped to one state; press event on state change
        impulse, // one command; press event when set, release when cleared
        value // one command; press event, value of specified byte passed as param1
    };

    using input_pin_t = std::tuple<std::size_t, input_type_t, user_command, user_command>;
    using inputpin_sequence = std::vector<input_pin_t>;

    bool setup_port();
    void find_ports();

// members
    sp_port *port = nullptr;
    inputpin_sequence m_inputbindings;
    command_relay relay;
    std::array<std::uint8_t, 16> old_packet; // TBD, TODO: replace with vector of configurable size?
    std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_setup;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_port_find;
    conf_t conf;
	bool data_pending = false;
    bool error_notified = false;
    std::uint8_t m_trainstatecab { 0 }; // helper, keeps track of last active cab. 0: front cab, 1: rear cab
};

extern uart_status UartStatus;
