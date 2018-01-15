#include "motiontelemetry.h"
#include "Globals.h"
#include "Logs.h"
#include "Train.h"
#include "Timer.h"
#include "Driver.h"
#include <ws2tcpip.h>

motiontelemetry::motiontelemetry()
{
	conf = Global::motiontelemetry_conf;

#ifdef _WIN32
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd))
		throw std::runtime_error("failed to init winsock");
#endif

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));

	protoent *pe = getprotobyname(conf.proto.c_str());
	if (!pe)
		throw std::runtime_error("motiontelemetry: unknown protocol");
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(conf.address.c_str(), conf.port.c_str(), &hints, &res))
		throw std::runtime_error("motiontelemetry: getaddrinfo failed");

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock == -1)
		throw std::runtime_error("motiontelemetry: failed to create socket");

	if (connect(sock, res->ai_addr, res->ai_addrlen) == -1)
		throw std::runtime_error("motiontelemetry: failed to connect socket");

	WriteLog("motiontelemetry: socket connected");
}

motiontelemetry::~motiontelemetry()
{
	shutdown(sock, 2);
#ifdef _WIN32
	WSACleanup();
#endif
}

void motiontelemetry::update()
{
	auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration<float>(now - last_update).count() < conf.updatetime)
		return;
	last_update = now;

	TTrain *t = Global::pWorld->train();
	if (!t)
		return;

	float forward = t->Occupied()->AccSVBased;
	float lateral = t->Occupied()->AccN;
	float vertical = t->Occupied()->AccVert;

	WriteLog("forward: " + std::to_string(forward) + ", lateral: " + std::to_string(lateral) + ", vertical: " + std::to_string(vertical));
}