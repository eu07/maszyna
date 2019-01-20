#include "network/network.h"

namespace network::file
{
    class server : public network::server
	{
	private:
		std::fstream stream;

	public:
		server();
	};

	class client : public network::client
	{
	private:
		std::fstream stream;

	public:
		client();
	};
}
