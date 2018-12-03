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
		void message_received(message msg);

	protected:
		virtual void disconnect() = 0;
		virtual void send_data(uint8_t *buffer, size_t len) = 0;
		void data_received(uint8_t *buffer, size_t len);

	public:
		virtual void connected();
	};

	class server
	{
	protected:
		std::vector<std::shared_ptr<connection>> clients;
	};
}
