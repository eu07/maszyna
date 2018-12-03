#include "network/message.h"
#include "sn_utils.h"

void network::message::serialize(std::ostream &stream)
{
	sn_utils::ls_uint16(stream, (uint16_t)type);
}

network::message network::message::deserialize(std::istream &stream)
{
	type_e type = (type_e)sn_utils::ld_uint16(stream);
	if ((int)type > (int)message::TYPE_MAX)
	{
		type = message::TYPE_MAX;
	}

	return message({type});
}
