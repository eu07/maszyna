#pragma once
#include <asio.hpp>
#include <memory>
#include "network/network.h"

namespace network
{
    class manager
	{
		asio::io_context io_context;
		std::vector<std::shared_ptr<server>> servers;
		std::shared_ptr<network::client> client;

	public:
		manager();

		void create_server();
		void connect();
		void poll();
	};
}
