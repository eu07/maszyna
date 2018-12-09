#pragma once
#include <asio.hpp>
#include <memory>
#include <functional>
#include <optional>
#include "network/message.h"

namespace network
{
    class connection : public std::enable_shared_from_this<connection>
	{
	private:
		void message_received(message &msg);
		void send_message(message &msg);

	protected:
		virtual void disconnect() = 0;
		virtual void send_data(std::shared_ptr<std::string> buffer) = 0;

		void data_received(std::string &buffer);

	public:
		virtual void connected();
	};

	class server
	{
	protected:
		std::vector<std::shared_ptr<connection>> clients;
	};

	class client
	{
	protected:
		std::shared_ptr<connection> srv;
	};
}
