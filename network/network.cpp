#include "network/network.h"
#include "network/message.h"
#include "Logs.h"

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);
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
	std::shared_ptr<std::string> buf = std::make_shared<std::string>();
	std::ostringstream stream(*buf.get());
	msg.serialize(stream);
	stream.flush();

	send_data(buf);
}
