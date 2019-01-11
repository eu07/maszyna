#include "network/message.h"
#include "sn_utils.h"

void network::message::serialize(std::ostream &stream)
{

}

void network::message::deserialize(std::istream &stream)
{

}

void network::accept_message::serialize(std::ostream &stream)
{
	sn_utils::ls_uint32(stream, seed);
}

void network::accept_message::deserialize(std::istream &stream)
{
	seed = sn_utils::ld_uint32(stream);
}

size_t network::accept_message::get_size()
{
	return message::get_size() + 4;
}

void::network::command_message::serialize(std::ostream &stream)
{
	sn_utils::ls_uint32(stream, commands.size());
	for (auto const &kv : commands) {
		sn_utils::ls_uint32(stream, kv.first);
		sn_utils::ls_uint32(stream, kv.second.size());
		for (command_data const &data : kv.second) {
			sn_utils::ls_uint32(stream, (uint32_t)data.command);
			sn_utils::ls_int32(stream, data.action);
			sn_utils::ls_float64(stream, data.param1);
			sn_utils::ls_float64(stream, data.param2);
			sn_utils::ls_float64(stream, data.time_delta);
		}
	}
}

void network::command_message::deserialize(std::istream &stream)
{
	uint32_t commands_size = sn_utils::ld_uint32(stream);
	for (uint32_t i = 0; i < commands_size; i++) {
		uint32_t recipient = sn_utils::ld_uint32(stream);
		uint32_t sequence_size = sn_utils::ld_uint32(stream);

		command_queue::commanddata_sequence sequence;
		for (uint32_t i = 0; i < sequence_size; i++) {
			command_data data;
			data.command = (user_command)sn_utils::ld_uint32(stream);
			data.action = sn_utils::ld_int32(stream);
			data.param1 = sn_utils::ld_float64(stream);
			data.param2 = sn_utils::ld_float64(stream);
			data.time_delta = sn_utils::ld_float64(stream);

			sequence.emplace_back(data);
		}

		commands.emplace(recipient, sequence);
	}
}

size_t network::command_message::get_size()
{
	size_t cmd_size = 4;

	for (auto const &kv : commands) {
		cmd_size += 8;
		for (command_data const &data : kv.second) {
			cmd_size += 8 + 3 * 8;
		}
	}

	return message::get_size() + cmd_size;
}

size_t network::delta_message::get_size()
{
	return command_message::get_size() + 16;
}

void network::delta_message::serialize(std::ostream &stream)
{
	sn_utils::ls_float64(stream, dt);
	sn_utils::ls_float64(stream, sync);

	command_message::serialize(stream);
}

void network::delta_message::deserialize(std::istream &stream)
{
	dt = sn_utils::ld_float64(stream);
	sync = sn_utils::ld_float64(stream);

	command_message::deserialize(stream);
}

void network::string_message::serialize(std::ostream &stream)
{
	sn_utils::s_str(stream, name);
}

void network::string_message::deserialize(std::istream &stream)
{
	name = sn_utils::d_str(stream);
}

size_t network::string_message::get_size()
{
	return message::get_size() + name.size() + 1;
}

std::shared_ptr<network::message> network::deserialize_message(std::istream &stream)
{
	message::type_e type = (message::type_e)sn_utils::ld_uint16(stream);

	std::shared_ptr<message> msg;
	if (type == message::CONNECT_ACCEPT) {
		auto m = std::make_shared<accept_message>();
		m->type = type;
		m->deserialize(stream);
		msg = m;
	}
	else if (type == message::STEP_INFO) {
		auto m = std::make_shared<delta_message>();
		m->type = type;
		m->deserialize(stream);
		msg = m;
	}
	else if (type == message::CLIENT_COMMAND) {
		auto m = std::make_shared<command_message>();
		m->type = type;
		m->deserialize(stream);
		msg = m;
	}
	else if (type == message::REQUEST_SPAWN_TRAIN || type == message::SPAWN_TRAIN) {
		auto m = std::make_shared<string_message>(type);
		m->type = type;
		m->deserialize(stream);
		msg = m;
	}
	else {
		msg = std::make_shared<message>(type);
	}

	return msg;
}
