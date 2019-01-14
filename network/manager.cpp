#include "network/manager.h"
#include "network/tcp.h"
#include "simulation.h"

network::manager::manager()
{
}

void network::manager::create_server()
{
	server = std::make_shared<tcp_server>(io_context);
}

void network::manager::poll()
{
	io_context.poll();
}

void network::manager::connect()
{
	client = std::make_shared<tcp_client>(io_context);
}

std::tuple<double, double, command_queue::commands_map> network::manager::get_next_delta()
{
	return client->get_next_delta();
}

void network::manager::push_delta(double delta, double sync, command_queue::commands_map commands)
{
	server->push_delta(delta, sync, commands);
}

command_queue::commands_map network::manager::pop_commands()
{
	return server->pop_commands();
}

void network::manager::send_commands(command_queue::commands_map commands)
{
	client->send_commands(commands);
}
