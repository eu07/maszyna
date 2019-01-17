#pragma once
#include <memory>
#include <functional>
#include <optional>
#include <queue>
#include <chrono>
#include "network/message.h"
#include "command.h"

namespace network
{
    class connection
	{
		friend class server;
		friend class client;

	private:
		/*
		std::queue<
		    std::pair<std::chrono::high_resolution_clock::time_point,
		    std::shared_ptr<frame_info>>> delta_queue;

		command_queue::commands_map client_commands_queue;
		bool is_client;

		//std::chrono::high_resolution_clock::time_point last_time;
		//double accum = -1.0;
		*/
		bool is_client;

	protected:
		std::function<void(const message &msg)> message_handler;

	public:
		virtual void connected() = 0;
		virtual void send_message(const message &msg) = 0;

		connection(bool client = false);
		void set_handler(std::function<void(const message &msg)> handler);

		virtual void disconnect() = 0;
		virtual void send_data(std::shared_ptr<std::string> buffer) = 0;

		enum peer_state {
			AWAITING_HELLO,
			CATCHING_UP,
			ACTIVE,
			DEAD
		};
		peer_state state;
	};

	class server
	{
	protected:
		void handle_message(connection &conn, const message &msg);

		std::vector<std::shared_ptr<connection>> clients;
		std::fstream recorder;

		command_queue::commands_map client_commands_queue;

	public:
		server();
		void push_delta(double dt, double sync, const command_queue::commands_map &commands);
		command_queue::commands_map pop_commands();
	};

	class client
	{
	protected:
		void handle_message(connection &conn, const message &msg);
		std::shared_ptr<connection> conn;

		std::queue<
		    std::pair<std::chrono::high_resolution_clock::time_point,
		        frame_info>> delta_queue;

	public:
		std::tuple<double, double, command_queue::commands_map> get_next_delta();
		void send_commands(command_queue::commands_map commands);
	};
}
