#include "network/manager.h"
#include "simulation.h"
#include "network/backend/asio.h"

command_queue::commands_map network::server_manager::pop_commands()
{
	command_queue::commands_map map;

	for (auto srv : servers)
		add_to_dequemap(map, srv->pop_commands());

	return map;
}

void network::server_manager::push_delta(double dt, double sync, const command_queue::commands_map &commands)
{
	for (auto srv : servers)
		srv->push_delta(dt, sync, commands);
}

void network::server_manager::create_server(asio::io_context &ctx, const std::string &host, uint32_t port)
{
	servers.emplace_back(std::make_shared<tcp::server>(ctx, host, port));
}

network::manager::manager()
{
}

void network::manager::poll()
{
	io_context.poll();
}

void network::manager::create_server(const std::string &host, uint32_t port)
{
	servers.emplace();
	servers->create_server(io_context, host, port);
}

void network::manager::connect(const std::string &host, uint32_t port)
{
	client = std::make_shared<tcp::client>(io_context, host, port);
}
