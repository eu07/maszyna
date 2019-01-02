#include "network/manager.h"
#include "network/tcp.h"
#include "simulation.h"

network::manager::manager()
{
}

void network::manager::create_server()
{
	server = std::make_shared<tcp_server>(io_context);
}

void network::manager::poll()
{
	io_context.poll();
}

void network::manager::connect()
{
	client = std::make_shared<tcp_client>(io_context);
}

std::tuple<double, command_queue::commands_map> network::manager::get_next_delta()
{
	return client->get_next_delta();
}

void network::manager::push_delta(double delta, command_queue::commands_map commands)
{
	server->push_delta(delta, commands);
}

command_queue::commands_map network::manager::pop_commands()
{
	return server->pop_commands();
}

void network::manager::send_commands(command_queue::commands_map commands)
{
	client->send_commands(commands);
}

void network::manager::request_train(std::string name)
{
	if (server) {
		TTrain *train = simulation::Trains.find(name);
		if (train)
			return;

		TDynamicObject *dynobj = simulation::Vehicles.find(name);
		if (!dynobj)
			return;

		train = new TTrain();
		if (train->Init(dynobj)) {
			simulation::Trains.insert(train, name);
			server->notify_train(name);
		}
		else {
			delete train;
			train = nullptr;
		}
	}
	if (client) {
		client->request_train(name);
	}
}
