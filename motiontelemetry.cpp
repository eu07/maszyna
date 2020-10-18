#include "stdafx.h"
#include "motiontelemetry.h"
#include "Globals.h"
#include "Logs.h"
#include "Train.h"
#include "Timer.h"
#include "Driver.h"
#include "simulation.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

motiontelemetry::motiontelemetry()
{
	conf = Global.motiontelemetry_conf;

#ifdef _WIN32
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd))
		throw std::runtime_error("failed to init winsock");
#endif

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = (conf.proto == "tcp" ? SOCK_STREAM : SOCK_DGRAM);

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

	if (Global.iPause)
		return;

    const TTrain *t = simulation::Train;
	if (!t)
		return;

	double dt = Timer::GetDeltaTime();

	glm::dvec3 front = t->Dynamic()->VectorFront();
	glm::dvec3 up = t->Dynamic()->VectorUp();
	glm::dvec3 left = t->Dynamic()->VectorLeft();

	glm::dvec3 pos = t->Dynamic()->GetPosition();
	glm::dvec3 vel = (pos - last_pos) / dt;
	glm::dvec3 acc = (vel - last_vel) / dt;
	
	glm::dvec3 local_acc;
	
	if (conf.includegravity)
	{
		glm::dvec3 gravity(0.0, 9.81, 0.0);
		local_acc = glm::dvec3(-glm::dot(gravity, left), glm::dot(gravity, up), glm::dot(gravity, front));
	}

	if (conf.latposbased)
		local_acc.x -= glm::dot(acc, left);
	else
		local_acc.x -= t->Occupied()->AccN;

	local_acc.y += glm::dot(acc, up) + t->Occupied()->AccVert * conf.axlebumpscale;

	if (conf.fwdposbased)
		local_acc.z += glm::dot(acc, front);
	else
		local_acc.z += t->Occupied()->AccSVBased;

	local_acc /= 9.81;

	// roll calculation, maybe too complicated?

	// transform to left-handed
	front.z *= -1;
	up.z *= -1;

	// make sure that vectors are orthonormal
	glm::dvec3 oright = glm::normalize(glm::cross(up, front));
	glm::dvec3 oup = glm::cross(front, oright);

	// right and up vector without roll
	glm::dvec3 right0 = glm::normalize(glm::cross(glm::dvec3(0.0, 1.0, 0.0), front));
	glm::dvec3 up0 = glm::cross(front, right0);

	double cosroll = glm::dot(up0, oup);
	double sinroll;

	if (abs(right0.x) > abs(right0.y) && abs(right0.x) > abs(right0.z))
		sinroll = (up0.x * cosroll - oup.x) / right0.x;
	else if (abs(right0.y) > abs(right0.x) && abs(right0.y) > abs(right0.z))
		sinroll = (up0.y * cosroll - oup.y) / right0.y;
	else
		sinroll = (up0.z * cosroll - oup.z) / right0.z;

	glm::dvec3 rot(asin(-front.y), atan2(front.x, front.z), asin(sinroll));

	double velocity = t->Occupied()->V;
	double yaw_vel = (rot.y - last_yaw) / dt;

	last_yaw = rot.y;
	last_pos = pos;
	last_vel = vel;

    if (t->Occupied()->CabActive < 0)
	{
		velocity *= -1;
		local_acc.x *= -1;
		local_acc.z *= -1;
		yaw_vel *= -1;
		rot *= -1;
	}

	float buffer[12] = { 0 };
	buffer[0] = Timer::GetTime();
	buffer[1] = velocity;
	buffer[2] = local_acc.y;
	buffer[3] = local_acc.z;
	buffer[4] = local_acc.x;
	buffer[5] = glm::degrees(rot.x);
	buffer[6] = glm::degrees(rot.z);
	buffer[7] = glm::degrees(rot.y);
	buffer[8] = yaw_vel;
	buffer[9] = 1.0f;

	if (send(sock, (char*)buffer, sizeof(buffer), 0) == -1)
		WriteLog("motiontelemetry: socket send failed");
}
