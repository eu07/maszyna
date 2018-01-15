#pragma once

#include <string>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#endif

class motiontelemetry
{
public:
	struct conf_t
	{
		bool enable;
		std::string proto;
		std::string address;
		std::string port;
		float updatetime;
	};

private:
	int sock;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
	conf_t conf;

public:
	motiontelemetry();
	~motiontelemetry();
	void update();
};