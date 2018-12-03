#include "network/manager.h"
#include "network/tcp.h"

network::manager::manager()
{
	servers.emplace_back(std::make_shared<tcp_server>(io_context));
}

void network::manager::poll()
{
	io_context.poll();
}
