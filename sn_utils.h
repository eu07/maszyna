#include <string>

class sn_utils
{
public:
	static uint16_t ld_uint16(std::istream &s);
	static uint32_t ld_uint32(std::istream &s);
	static int32_t ld_int32(std::istream &s);
	static float ld_float32(std::istream &s);
	static double ld_float64(std::istream &s);
	static std::string d_str(std::istream &s);
};