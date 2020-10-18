#include "stdafx.h"
#include "network/message.h"
#include "sn_utils.h"

void network::client_hello::serialize(std::ostream &stream) const
{
	sn_utils::ls_int32(stream, version);
	sn_utils::ls_uint32(stream, start_packet);
}

void network::client_hello::deserialize(std::istream &stream)
{
	version = sn_utils::ld_int32(stream);
	start_packet = sn_utils::ld_uint32(stream);
}

void network::server_hello::serialize(std::ostream &stream) const
{
	sn_utils::ls_uint32(stream, seed);
	sn_utils::ls_int64(stream, timestamp);
    sn_utils::ls_int64(stream, config);
    sn_utils::s_str(stream, scenario);
}

void network::server_hello::deserialize(std::istream &stream)
{
	seed = sn_utils::ld_uint32(stream);
	timestamp = sn_utils::ld_int64(stream);
    config = sn_utils::ld_int64(stream);
    scenario = sn_utils::d_str(stream);
}

void ::network::request_command::serialize(std::ostream &stream) const
{
	sn_utils::ls_uint32(stream, commands.size());
	for (auto const &kv : commands)
	{
		sn_utils::ls_uint32(stream, kv.first);
		sn_utils::ls_uint32(stream, kv.second.size());
		for (command_data const &data : kv.second)
		{
			sn_utils::ls_uint32(stream, (uint32_t)data.command);
			sn_utils::ls_int32(stream, data.action);
			sn_utils::ls_float64(stream, data.param1);
			sn_utils::ls_float64(stream, data.param2);
			sn_utils::ls_float64(stream, data.time_delta);

			sn_utils::s_bool(stream, data.freefly);
			sn_utils::s_vec3(stream, data.location);

			sn_utils::s_str(stream, data.payload);
		}
	}
}

void network::request_command::deserialize(std::istream &stream)
{
	uint32_t commands_size = sn_utils::ld_uint32(stream);
	for (uint32_t i = 0; i < commands_size; i++)
	{
		uint32_t recipient = sn_utils::ld_uint32(stream);
		uint32_t sequence_size = sn_utils::ld_uint32(stream);

		command_queue::commanddata_sequence sequence;
		for (uint32_t i = 0; i < sequence_size; i++)
		{
			command_data data;
			data.command = (user_command)sn_utils::ld_uint32(stream);
			data.action = sn_utils::ld_int32(stream);
			data.param1 = sn_utils::ld_float64(stream);
			data.param2 = sn_utils::ld_float64(stream);
			data.time_delta = sn_utils::ld_float64(stream);

			data.freefly = sn_utils::d_bool(stream);
			data.location = sn_utils::d_vec3(stream);

			data.payload = sn_utils::d_str(stream);

			sequence.emplace_back(data);
		}

		commands.emplace(recipient, sequence);
	}
}

void network::frame_info::serialize(std::ostream &stream) const
{
	sn_utils::ls_float64(stream, render_dt);
	sn_utils::ls_float64(stream, dt);
	sn_utils::ls_float64(stream, sync);

	request_command::serialize(stream);
}

void network::frame_info::deserialize(std::istream &stream)
{
	render_dt = sn_utils::ld_float64(stream);
	dt = sn_utils::ld_float64(stream);
	sync = sn_utils::ld_float64(stream);

	request_command::deserialize(stream);
}

std::shared_ptr<network::message> network::deserialize_message(std::istream &stream)
{
	message::type_e type = (message::type_e)sn_utils::ld_uint16(stream);

	std::shared_ptr<message> msg;

	if (type == message::CLIENT_HELLO)
		msg = std::make_shared<client_hello>();
	else if (type == message::SERVER_HELLO)
		msg = std::make_shared<server_hello>();
	else if (type == message::FRAME_INFO)
		msg = std::make_shared<frame_info>();
	else if (type == message::REQUEST_COMMAND)
		msg = std::make_shared<request_command>();

	msg->deserialize(stream);

	return msg;
}

void network::serialize_message(const message &msg, std::ostream &stream)
{
	sn_utils::ls_uint16(stream, (uint16_t)msg.type);
	msg.serialize(stream);
}
