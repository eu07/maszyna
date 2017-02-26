#include <string>

class sn_utils
{
public:
	static uint16_t ld_uint16(std::istream&);
	static uint32_t ld_uint32(std::istream&);
	static int32_t ld_int32(std::istream&);
	static float ld_float32(std::istream&);
	static double ld_float64(std::istream&);
	static std::string d_str(std::istream&);

	static void ls_uint16(std::ostream&, uint16_t);
	static void ls_uint32(std::ostream&, uint32_t);
	static void ls_int32(std::ostream&, int32_t);
	static void ls_float32(std::ostream&, float);
	static void ls_float64(std::ostream&, double);
	static void s_str(std::ostream&, std::string);
};