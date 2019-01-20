#include "network/network.h"
#include "network/message.h"
#include "Logs.h"
#include "sn_utils.h"
#include "Timer.h"
#include "application.h"

// connection

void network::connection::disconnect() {
	WriteLog("net: peer dropped", logtype::net);
	state = DEAD;
}

void network::connection::set_handler(std::function<void (const message &)> handler) {
	message_handler = handler;
}

network::connection::connection(bool client) {
	is_client = client;
	state = AWAITING_HELLO;
}

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);

	if (is_client) {
		client_hello msg;
		msg.version = 1;
		send_message(msg);
	}
}

    /*
	if (msg->type == message::TYPE_MAX)
	{
		disconnect();
		return;
	}

	if (is_client) {
		if (msg->type == message::SERVER_HELLO) {
			auto cmd = std::dynamic_pointer_cast<server_hello>(msg);

			state = ACTIVE;
			Global.random_seed = cmd->seed;
			Global.random_engine.seed(Global.random_seed);

			WriteLog("net: accept received", logtype::net);
		}
		else if (msg->type == message::FRAME_INFO) {
			auto delta = std::dynamic_pointer_cast<frame_info>(msg);
			auto now = std::chrono::high_resolution_clock::now();
			delta_queue.push(std::make_pair(now, delta));
		}

	} else {
		if (msg->type == message::CLIENT_HELLO) {
			server_hello reply;
			reply.seed = Global.random_seed;
			state = ACTIVE;

			send_message(reply);

			WriteLog("net: client accepted", logtype::net);
		}
		else if (msg->type == message::REQUEST_COMMAND) {
			auto cmd = std::dynamic_pointer_cast<request_command>(msg);

			for (auto const &kv : cmd->commands)
				client_commands_queue.emplace(kv);
		}
	}
	*/

// --------------

/*
std::tuple<double, double, command_queue::commands_map> network::connection::get_next_delta()
{
	if (delta_queue.empty()) {
		return std::tuple<double, double,
		        command_queue::commands_map>(0.0, 0.0, command_queue::commands_map());
	}

	///auto now = std::chrono::high_resolution_clock::now();

	//double last_frame = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();
	//last_time = now;
	//accum += last_frame;

	auto entry = delta_queue.front();

	//if (accum > remote_dt) {
	    //accum -= remote_dt;

	    delta_queue.pop();
		return std::make_tuple(entry.second->dt, entry.second->sync, entry.second->commands);
	//}
	//else {
		//return 0.0;
	//}
}

command_queue::commands_map network::connection::pop_commands()
{
	command_queue::commands_map map(client_commands_queue);
	client_commands_queue.clear();
	return map;
}
*/

// server

void network::server::push_delta(double dt, double sync, const command_queue::commands_map &commands)
{
	if (dt == 0.0 && commands.empty())
		return;

	frame_info msg;
	msg.dt = dt;
	msg.sync = sync;
	msg.commands = commands;

	for (auto c : clients)
		if (c->state == connection::ACTIVE)
			c->send_message(msg);
}

command_queue::commands_map network::server::pop_commands()
{
	command_queue::commands_map map(client_commands_queue);
	client_commands_queue.clear();
	return map;
}

void network::server::handle_message(connection &conn, const message &msg)
{
	if (msg.type == message::TYPE_MAX)
	{
		conn.disconnect();
		return;
	}

	if (msg.type == message::CLIENT_HELLO) {
		server_hello reply;
		reply.seed = Global.random_seed;
		conn.state = connection::ACTIVE;

		conn.send_message(reply);

		WriteLog("net: client accepted", logtype::net);
	}
	else if (msg.type == message::REQUEST_COMMAND) {
		auto cmd = dynamic_cast<const request_command&>(msg);

		for (auto const &kv : cmd.commands)
			client_commands_queue.emplace(kv);
	}
}

// ------------

// client

std::tuple<double, double, command_queue::commands_map> network::client::get_next_delta()
{
	if (delta_queue.empty()) {
		return std::tuple<double, double,
		        command_queue::commands_map>(0.0, 0.0, command_queue::commands_map());
	}

	auto entry = delta_queue.front();
	delta_queue.pop();

	return std::make_tuple(entry.second.dt, entry.second.sync, entry.second.commands);
}

void network::client::send_commands(command_queue::commands_map commands)
{
	if (commands.empty())
		return;

	request_command msg;
	msg.commands = commands;

	conn->send_message(msg);
}

void network::client::handle_message(connection &conn, const message &msg)
{
	if (msg.type == message::TYPE_MAX)
	{
		conn.disconnect();
		return;
	}

	if (msg.type == message::SERVER_HELLO) {
		auto cmd = dynamic_cast<const server_hello&>(msg);

		conn.state = connection::ACTIVE;
		Global.random_seed = cmd.seed;
		Global.random_engine.seed(Global.random_seed);

		WriteLog("net: accept received", logtype::net);
	}
	else if (msg.type == message::FRAME_INFO) {
		auto delta = dynamic_cast<const frame_info&>(msg);
		auto now = std::chrono::high_resolution_clock::now();
		delta_queue.push(std::make_pair(now, delta));
	}
}

// --------------
