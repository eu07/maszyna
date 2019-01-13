#pragma once
#include <asio.hpp>
#include <memory>
#include "network/network.h"
#include "command.h"

namespace network
{
    class manager
	{
		asio::io_context io_context;

	public:
		manager();

		std::shared_ptr<network::server> server;
		std::shared_ptr<network::client> client;

		void create_server();
		void connect();
		void poll();

		std::tuple<double, double, command_queue::commands_map> get_next_delta();
		void push_delta(double delta, double sync, command_queue::commands_map commands);
		command_queue::commands_map pop_commands();
		void send_commands(command_queue::commands_map commands);
		void request_train(std::string name);
	};
}
