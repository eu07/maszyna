#include "stdafx.h"
#include "network/network.h"
#include "network/message.h"
#include "Logs.h"
#include "sn_utils.h"
#include "Timer.h"
#include "application.h"
#include "Globals.h"

std::uint32_t const EU07_NETWORK_VERSION = 2;

namespace network {

backend_list_t& backend_list() {
    // HACK: static initialization order fiasco fix
    // NOTE: potential static deinitialization order fiasco
    static backend_list_t backend_list;
    return backend_list;
}

}
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
		msg.version = EU07_NETWORK_VERSION;
		msg.start_packet = packet_counter;
		send_message(msg);
	}
}

void network::connection::catch_up()
{
	backbuffer->seekg(backbuffer_pos);

	std::vector<std::shared_ptr<message>> messages;

	for (int i = 0; i < CATCHUP_PACKETS; i++) {
		if (backbuffer->peek() == EOF) {
			send_messages(messages);
			state = ACTIVE;
			backbuffer->seekg(0, std::ios_base::end);
			return;
		}

		auto msg = deserialize_message(*backbuffer.get());

		if (packet_counter) {
			packet_counter--;
			i--; // TODO: it would be better to skip frames in chunks
			continue;
		}

		messages.push_back(msg);
	}

	backbuffer_pos = backbuffer->tellg();
	backbuffer->seekg(0, std::ios_base::end);

	send_messages(messages);
}

void network::connection::send_complete(std::shared_ptr<std::string> buf)
{
	if (!is_client && state == CATCHING_UP) {
		catch_up();
	}
}

// --------------

// server
network::server::server(std::shared_ptr<std::istream> buf) : backbuffer(buf)
{

}

void network::server::push_delta(const frame_info &msg)
{
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

		if (cmd.version != EU07_NETWORK_VERSION // wrong version
		        || !Global.ready_to_load) { // not ready yet
			conn->disconnect();
			return;
		}

		server_hello reply;
		reply.seed = Global.random_seed;
		reply.timestamp = Global.starting_timestamp;
        reply.config = 0; // TODO: pass bitfield with state of relevant setting switches
        reply.scenario = Global.SceneryFile;
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

void network::client::update()
{
	if (conn && conn->state == connection::DEAD) {
		conn.reset();
	}

	if (!conn) {
		if (!reconnect_delay) {
			connect();
			reconnect_delay = RECONNECT_DELAY_FRAMES;
		}
		reconnect_delay--;
	}
}

// client
std::tuple<double, double, command_queue::commands_map> network::client::get_next_delta(int counter)
{
	auto now = std::chrono::high_resolution_clock::now();
	if (counter == 1) {
		frame_time = now - last_frame;
		last_frame = now;
	}

	if (delta_queue.empty()) {
		// buffer underflow
		return std::tuple<double, double,
		        command_queue::commands_map>(0.0, 0.0, command_queue::commands_map());
	}


	float size = delta_queue.size() - consume_counter;
	auto entry = delta_queue.front();
	float mult = entry.render_dt / std::chrono::duration_cast<std::chrono::duration<float>>(frame_time).count();

	if (counter == 1 && size < MAX_BUFFER_SIZE * 2.0f) {
		last_target = last_target * TARGET_MIX +
		        (std::min(TARGET_MIN + jitteriness * JITTERINESS_MULTIPIER, MAX_BUFFER_SIZE)) * (1.0f - TARGET_MIX);
		float diff = size - last_target;
		jitteriness = std::max(jitteriness * JITTERINESS_MIX, std::abs(diff));

		float speed = 1.0f + diff * CONSUME_MULTIPIER;

		consume_counter += speed;
	}

	float last_rcv_diff = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_rcv).count();

	if (size > MAX_BUFFER_SIZE || consume_counter > mult || last_rcv_diff > 1.0f) {
		if (consume_counter > mult) {
			consume_counter = std::clamp(consume_counter - mult, -MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
		}

		delta_queue.pop();

		return std::make_tuple(entry.dt, entry.sync, entry.commands);
	} else {
		// nothing to push
		return std::tuple<double, double,
		        command_queue::commands_map>(0.0, 0.0, command_queue::commands_map());
	}
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

		if (!Global.ready_to_load) {
			Global.random_seed = cmd.seed;
			Global.random_engine.seed(Global.random_seed);
			Global.starting_timestamp = cmd.timestamp;
            // TODO: configure simulation settings according to received cmd.config
            Global.SceneryFile = cmd.scenario;
			Global.ready_to_load = true;
		} else if (Global.random_seed != cmd.seed) {
			ErrorLog("net: seed mismatch", logtype::net);
			conn->disconnect();
			return;
		}

		WriteLog("net: accept received", logtype::net);
	}

	if (conn->state != connection::ACTIVE)
		return;

	if (msg.type == message::FRAME_INFO) {
		resume_frame_counter++;

		auto delta = dynamic_cast<const frame_info&>(msg);
		delta_queue.push(delta);
		last_rcv = std::chrono::high_resolution_clock::now();
	}
}

// --------------
