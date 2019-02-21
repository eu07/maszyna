#pragma once
#include <memory>
#include "network/network.h"
#include "command.h"

namespace network
{
    class server_manager
	{
	private:
		std::vector<std::shared_ptr<server>> servers;
		std::shared_ptr<std::fstream> backbuffer;

	public:
		server_manager();

		void push_delta(double render_dt, double dt, double sync, const command_queue::commands_map &commands);
		command_queue::commands_map pop_commands();
		void create_server(const std::string &backend, const std::string &conf);
	};

    class manager
	{
	public:
		manager();

		std::optional<server_manager> servers;
		std::shared_ptr<network::client> client;

		void create_server(const std::string &backend, const std::string &conf);
		void connect(const std::string &backend, const std::string &conf);
		void update();
	};
}
