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

network::connection::connection(bool client, size_t counter) {
	packet_counter = counter;
	is_client = client;
	state = AWAITING_HELLO;
}

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);

	if (is_client) {
		client_hello msg;
		msg.version = 1;
		msg.start_packet = packet_counter;
		send_message(msg);
	}
}

void network::connection::catch_up()
{
	backbuffer->seekg(backbuffer_pos);

	std::vector<std::shared_ptr<message>> messages;

	for (size_t i = 0; i < CATCHUP_PACKETS; i++) {
		if (backbuffer->peek() == EOF) {
			send_messages(messages);
			backbuffer->seekg(0, std::ios_base::end);
			state = ACTIVE;
			return;
		}

		if (packet_counter) {
			packet_counter--;
			continue;
		}

		messages.push_back(deserialize_message(*backbuffer.get()));
	}

	send_messages(messages);

	backbuffer_pos = backbuffer->tellg();

	backbuffer->seekg(0, std::ios_base::end);
}

void network::connection::send_complete(std::shared_ptr<std::string> buf)
{
	if (!is_client && state == CATCHING_UP) {
		catch_up();
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

network::server::server(std::shared_ptr<std::istream> buf) : backbuffer(buf)
{

}

void network::server::push_delta(double dt, double sync, const command_queue::commands_map &commands)
{
	frame_info msg;
	msg.dt = dt;
	msg.sync = sync;
	msg.commands = commands;

	for (auto it = clients.begin(); it != clients.end(); ) {
		if ((*it)->state == connection::DEAD) {
			it = clients.erase(it);
			continue;
		}

		if ((*it)->state == connection::ACTIVE)
			(*it)->send_message(msg);

		it++;
	}
}

command_queue::commands_map network::server::pop_commands()
{
	command_queue::commands_map map(client_commands_queue);
	client_commands_queue.clear();
	return map;
}

void network::server::handle_message(std::shared_ptr<connection> conn, const message &msg)
{
	if (msg.type == message::TYPE_MAX)
	{
		conn->disconnect();
		return;
	}

	if (msg.type == message::CLIENT_HELLO) {
		auto cmd = dynamic_cast<const client_hello&>(msg);

		server_hello reply;
		reply.seed = Global.random_seed;
		conn->state = connection::CATCHING_UP;
		conn->backbuffer = backbuffer;
		conn->backbuffer_pos = 0;
		conn->packet_counter = cmd.start_packet;

		conn->send_message(reply);

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
int zzz = 10;
std::tuple<double, double, command_queue::commands_map> network::client::get_next_delta()
{
	/*
	if (conn && conn->state == connection::DEAD) {
		conn = nullptr;
	}
	*/

	if (!conn) {
		zzz--;
		if (zzz < 0)
			connect();
	}

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
	if (!conn || conn->state == connection::DEAD || commands.empty())
		return;
	// eh, maybe queue lost messages

	request_command msg;
	msg.commands = commands;

	conn->send_message(msg);
}

void network::client::handle_message(std::shared_ptr<connection> conn, const message &msg)
{
	if (msg.type >= message::TYPE_MAX)
	{
		conn->disconnect();
		return;
	}

	if (msg.type == message::SERVER_HELLO) {
		auto cmd = dynamic_cast<const server_hello&>(msg);
		conn->state = connection::ACTIVE;

		Global.random_seed = cmd.seed;
		Global.random_engine.seed(Global.random_seed);

		WriteLog("net: accept received", logtype::net);
	}
	else if (msg.type == message::FRAME_INFO) {
		delta_count++;

		auto delta = dynamic_cast<const frame_info&>(msg);
		auto now = std::chrono::high_resolution_clock::now();
		delta_queue.push(std::make_pair(now, delta));
	}
}

// --------------
