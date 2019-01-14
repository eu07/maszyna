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

		command_queue::commands_map client_commands_queue;
		bool is_client;

		//std::chrono::high_resolution_clock::time_point last_time;
		//double accum = -1.0;

	protected:
		virtual void disconnect() = 0;
		virtual void send_data(std::shared_ptr<std::string> buffer) = 0;

		void data_received(std::string &buffer);

	public:
		connection(bool client = false);
		void send_message(std::shared_ptr<message> msg);
		virtual void connected();

		std::tuple<double, double, command_queue::commands_map> get_next_delta();
		command_queue::commands_map pop_commands();
	};

	class server
	{
	protected:
		std::vector<std::shared_ptr<connection>> clients;
		std::fstream recorder;

	public:
		server();
		void push_delta(double dt, double sync, command_queue::commands_map commands);
		command_queue::commands_map pop_commands();
	};

	class client
	{
	protected:
		std::shared_ptr<connection> conn;

	public:
		std::tuple<double, double, command_queue::commands_map> get_next_delta();
		void send_commands(command_queue::commands_map commands);
	};
}
