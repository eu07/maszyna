#include "network/network.h"
#include "network/message.h"
#include "Logs.h"
#include "sn_utils.h"

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);

	message hello;
	hello.type = message::CONNECT_REQUEST;
	send_message(hello);
}

void network::connection::message_received(message &msg)
{
	if (msg.type == message::TYPE_MAX)
	{
		disconnect();
		return;
	}

	if (msg.type == message::CONNECT_REQUEST)
	{
		message reply({message::CONNECT_ACCEPT});

		WriteLog("client accepted", logtype::net);

		std::string reply_buf;
		std::ostringstream stream(reply_buf);
		reply.serialize(stream);
	}
}

void network::connection::data_received(std::string &buffer)
{
	std::istringstream body(buffer);
	message msg = message::deserialize(body);

	message_received(msg);
}

void network::connection::send_message(message &msg)
{
	std::ostringstream stream;
	sn_utils::ls_uint32(stream, 0x37305545);
	sn_utils::ls_uint32(stream, 2);
	msg.serialize(stream);
	stream.flush();

	std::shared_ptr<std::string> buf = std::make_shared<std::string>(stream.str());
	send_data(buf);
}
