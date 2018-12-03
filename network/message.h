#pragma once
#include "network/message.h"

namespace network
{
    struct message
	{
		enum type_e
		{
			CONNECT_REQUEST = 0,
			CONNECT_ACCEPT,
			STEP_INFO,
			VEHICLE_COMMAND,
			TYPE_MAX
		};

		type_e type;

		void serialize(std::ostream &stream);

		static message deserialize(std::istream &stream);
	};
}
