#include "network/network.h"
#include "network/message.h"
#include "Logs.h"

void network::connection::connected()
{
	WriteLog("net: socket connected", logtype::net);
}

void network::connection::message_received(message msg)
{

}

void network::connection::data_received(uint8_t *buffer, size_t len)
{

}
