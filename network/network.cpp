#include "network/network.h"
#include "network/message.h"
#include "Logs.h"
#include "sn_utils.h"
#include "Timer.h"

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);

	std::shared_ptr<message> hello = std::make_shared<message>(message::CONNECT_REQUEST);
	send_message(hello);
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
		std::shared_ptr<message> reply = std::make_shared<message>(message::CONNECT_ACCEPT);

		WriteLog("client accepted", logtype::net);

		send_message(reply);
	}
	else if (msg->type == message::CONNECT_ACCEPT)
	{
		WriteLog("accept received", logtype::net);
	}
	else if (msg->type == message::STEP_INFO)
	{
		auto delta = std::dynamic_pointer_cast<delta_message>(msg);
		auto now = std::chrono::high_resolution_clock::now();
		delta_queue.push(std::make_pair(now, delta));
	}
}

std::tuple<double, command_queue::commanddatasequence_map> network::connection::get_next_delta()
{
	if (delta_queue.empty()) {
		return std::tuple<double,
		        command_queue::commanddatasequence_map>(0.0, command_queue::commanddatasequence_map());
	}

	///auto now = std::chrono::high_resolution_clock::now();

	//double last_frame = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();
	//last_time = now;
	//accum += last_frame;

	auto entry = delta_queue.front();

	//if (accum > remote_dt) {
	    //accum -= remote_dt;

	    delta_queue.pop();
		return std::make_tuple(entry.second->dt, entry.second->commands);
	//}
	//else {
		//return 0.0;
	//}
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

void network::server::push_delta(double dt, command_queue::commanddatasequence_map commands)
{
	std::shared_ptr<delta_message> msg = std::make_shared<delta_message>();
	msg->dt = dt;
	msg->commands = commands;

	for (auto c : clients)
		c->send_message(msg);
}

std::tuple<double, command_queue::commanddatasequence_map> network::client::get_next_delta()
{
	return conn->get_next_delta();
}
