#include "network/network.h"
#include "network/message.h"
#include "Logs.h"
#include "sn_utils.h"
#include "Timer.h"
#include "application.h"

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);

	if (!Global.network_conf.is_server) {
		std::shared_ptr<message> hello = std::make_shared<message>(message::CONNECT_REQUEST);
		send_message(hello);
	}
}

void network::connection::message_received(std::shared_ptr<message> &msg)
{
	if (msg->type == message::TYPE_MAX)
	{
		disconnect();
		return;
	}

	if (msg->type == message::CONNECT_REQUEST)
	{
		std::shared_ptr<accept_message> reply = std::make_shared<accept_message>();
		reply->seed = Global.random_seed;

		WriteLog("client accepted", logtype::net);

		send_message(reply);
	}
	else if (msg->type == message::CONNECT_ACCEPT)
	{
		auto cmd = std::dynamic_pointer_cast<accept_message>(msg);

		WriteLog("accept received", logtype::net);
		Global.random_seed = cmd->seed;
		Global.random_engine.seed(Global.random_seed);
	}
	else if (msg->type == message::CLIENT_COMMAND)
	{
		auto cmd = std::dynamic_pointer_cast<command_message>(msg);
		for (auto const &kv : cmd->commands)
			client_commands_queue.emplace(kv);
	}
	else if (msg->type == message::STEP_INFO)
	{
		auto delta = std::dynamic_pointer_cast<delta_message>(msg);
		auto now = std::chrono::high_resolution_clock::now();
		delta_queue.push(std::make_pair(now, delta));
	}
	else if (msg->type == message::REQUEST_SPAWN_TRAIN)
	{
		auto req = std::dynamic_pointer_cast<string_message>(msg);
		Application.request_train(req->name);
	}
	else if (msg->type == message::SPAWN_TRAIN)
	{
		auto req = std::dynamic_pointer_cast<string_message>(msg);
		Application.spawn_train(req->name);
	}
}

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

void network::connection::data_received(std::string &buffer)
{
	std::istringstream body(buffer);
	std::shared_ptr<message> msg = deserialize_message(body);

	message_received(msg);
}

void network::connection::send_message(std::shared_ptr<message> msg)
{
	std::ostringstream stream;
	sn_utils::ls_uint32(stream, 0x37305545);
	sn_utils::ls_uint32(stream, msg->get_size());
	sn_utils::ls_uint16(stream, (uint16_t)msg->type);
	msg->serialize(stream);
	stream.flush();

	std::shared_ptr<std::string> buf = std::make_shared<std::string>(stream.str());
	send_data(buf);
}

network::server::server()
{
	recorder.open("recorder.bin", std::ios::trunc | std::ios::out | std::ios::binary);
}

void network::server::push_delta(double dt, double sync, command_queue::commands_map commands)
{
	std::shared_ptr<delta_message> msg = std::make_shared<delta_message>();
	msg->dt = dt;
	msg->sync = sync;
	msg->commands = commands;

	sn_utils::ls_uint32(recorder, 0x37305545);
	sn_utils::ls_uint32(recorder, msg->get_size());
	sn_utils::ls_uint16(recorder, (uint16_t)msg->type);
	msg->serialize(recorder);
	recorder.flush();

	for (auto c : clients)
		c->send_message(msg);
}

void network::server::notify_train(std::string name)
{
	std::shared_ptr<string_message> msg = std::make_shared<string_message>(message::SPAWN_TRAIN);
	msg->name = name;

	for (auto c : clients)
		c->send_message(msg);
}

command_queue::commands_map network::server::pop_commands()
{
	command_queue::commands_map map;
	for (auto c : clients) {
		command_queue::commands_map cmap = c->pop_commands();
		for (auto const &kv : cmap) {
			auto lookup = map.emplace(kv.first, command_queue::commanddata_sequence());
			for (command_data const &data : kv.second)
				lookup.first->second.emplace_back(data);
		}
	}
	return map;
}

std::tuple<double, double, command_queue::commands_map> network::client::get_next_delta()
{
	return conn->get_next_delta();
}

void network::client::send_commands(command_queue::commands_map commands)
{
	if (commands.empty())
		return;

	std::shared_ptr<command_message> msg = std::make_shared<command_message>();
	msg->commands = commands;

	conn->send_message(msg);
}

void network::client::request_train(std::string name)
{
	std::shared_ptr<string_message> msg = std::make_shared<string_message>(message::REQUEST_SPAWN_TRAIN);
	msg->name = name;

	conn->send_message(msg);
}
