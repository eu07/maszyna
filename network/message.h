#pragma once
#include "network/message.h"
#include "command.h"
#include <queue>

namespace network
{
    struct message
	{
		enum type_e
		{
			CONNECT_REQUEST = 0,
			CONNECT_ACCEPT,
			STEP_INFO,
			CLIENT_COMMAND,
			TYPE_MAX
		};

		type_e type;

		message(type_e t) : type(t) {}
		virtual void serialize(std::ostream &stream);
		virtual void deserialize(std::istream &stream);
		virtual size_t get_size() { return 2; }
	};

	struct accept_message : public message
	{
		accept_message() : message(CONNECT_ACCEPT) {}

		uint32_t seed;

		virtual void serialize(std::ostream &stream) override;
		virtual void deserialize(std::istream &stream) override;
		virtual size_t get_size() override;
	};

	struct command_message : public message
	{
		command_message() : message(CLIENT_COMMAND) {}
		command_message(type_e type) : message(type) {}

		command_queue::commands_map commands;

		virtual void serialize(std::ostream &stream) override;
		virtual void deserialize(std::istream &stream) override;
		virtual size_t get_size() override;
	};

	struct delta_message : public command_message
	{
		delta_message() : command_message(STEP_INFO) {}

		double dt;
		double sync;

		virtual void serialize(std::ostream &stream) override;
		virtual void deserialize(std::istream &stream) override;
		virtual size_t get_size() override;
	};

	struct string_message : public message
	{
		string_message(type_e type) : message(type) {}

		std::string name;

		virtual void serialize(std::ostream &stream) override;
		virtual void deserialize(std::istream &stream) override;
		virtual size_t get_size() override;
	};

	std::shared_ptr<message> deserialize_message(std::istream &stream);
}
