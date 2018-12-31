#include "network/manager.h"
#include "network/tcp.h"

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

std::tuple<double, command_queue::commanddatasequence_map> network::manager::get_next_delta()
{
	return client->get_next_delta();
}

void network::manager::push_delta(double delta, command_queue::commanddatasequence_map commands)
{
	server->push_delta(delta, commands);
}
