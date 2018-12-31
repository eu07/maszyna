#pragma once
#include <asio.hpp>
#include <memory>
#include <functional>
#include <optional>
#include <queue>
#include <chrono>
#include "network/message.h"
#include "command.h"

namespace network
{
    class connection : public std::enable_shared_from_this<connection>
	{
	private:
		void message_received(std::shared_ptr<message> &msg);
		std::queue<
		    std::pair<std::chrono::high_resolution_clock::time_point,
		    std::shared_ptr<delta_message>>> delta_queue;

		//std::chrono::high_resolution_clock::time_point last_time;
		//double accum = -1.0;

	protected:
		virtual void disconnect() = 0;
		virtual void send_data(std::shared_ptr<std::string> buffer) = 0;

		void data_received(std::string &buffer);

	public:
		void send_message(std::shared_ptr<message> msg);
		virtual void connected();

		std::tuple<double, command_queue::commanddatasequence_map> get_next_delta();
	};

	class server
	{
	protected:
		std::vector<std::shared_ptr<connection>> clients;

	public:
		void push_delta(double dt, command_queue::commanddatasequence_map commands);
	};

	class client
	{
	protected:
		std::shared_ptr<connection> conn;

	public:
		std::tuple<double, command_queue::commanddatasequence_map> get_next_delta();
	};
}
