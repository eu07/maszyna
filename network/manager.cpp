#include "network/manager.h"
#include "network/tcp.h"

network::manager::manager()
{
}

void network::manager::create_server()
{
	servers.emplace_back(std::make_shared<tcp_server>(io_context));
}

void network::manager::poll()
{
	io_context.poll();
}

void network::manager::connect()
{
	client = std::make_shared<tcp_client>(io_context);
}
