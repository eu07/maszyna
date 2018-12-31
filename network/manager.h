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
		std::shared_ptr<network::server> server;
		std::shared_ptr<network::client> client;

	public:
		manager();

		void create_server();
		void connect();
		void poll();

		std::tuple<double, command_queue::commanddatasequence_map> get_next_delta();
		void push_delta(double delta, command_queue::commanddatasequence_map commands);
	};
}
