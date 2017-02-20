#include "stdafx.h"
#include "sn_utils.h"

// sanity checks
static_assert(std::numeric_limits<double>::is_iec559, "IEEE754 required");
static_assert(sizeof(float) == 4, "Float must be 4 bytes");
static_assert(sizeof(double) == 8, "Double must be 8 bytes");
static_assert(-1 == ~0, "Two's complement required");

// deserialize little endian uint16
uint16_t sn_utils::ld_uint16(std::istream &s)
{
	uint8_t buf[2];
	s.read((char*)buf, 2);
	uint16_t v = (buf[1] << 8) | buf[0];
	return reinterpret_cast<uint16_t&>(v);
}

// deserialize little endian uint32
uint32_t sn_utils::ld_uint32(std::istream &s)
{
	uint8_t buf[4];
	s.read((char*)buf, 4);
	uint32_t v = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return reinterpret_cast<uint32_t&>(v);
}

// deserialize little endian int32
int32_t sn_utils::ld_int32(std::istream &s)
{
	uint8_t buf[4];
	s.read((char*)buf, 4);
	uint32_t v = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return reinterpret_cast<int32_t&>(v);
}

// deserialize little endian ieee754 float32
float sn_utils::ld_float32(std::istream &s)
{
	uint8_t buf[4];
	s.read((char*)buf, 4);
	uint32_t v = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return reinterpret_cast<float&>(v);
}

// deserialize little endian ieee754 float64
double sn_utils::ld_float64(std::istream &s)
{
	uint8_t buf[8];
	s.read((char*)buf, 8);
	uint64_t v = ((uint64_t)buf[7] << 56) | ((uint64_t)buf[6] << 48) |
		         ((uint64_t)buf[5] << 40) | ((uint64_t)buf[4] << 32) |
	             ((uint64_t)buf[3] << 24) | ((uint64_t)buf[2] << 16) |
		         ((uint64_t)buf[1] << 8) | (uint64_t)buf[0];
	return reinterpret_cast<double&>(v);
}

// deserialize null-terminated string
std::string sn_utils::d_str(std::istream &s)
{
	std::string r;
	r.reserve(32);
	char buf[1];
	while (true)
	{
		s.read(buf, 1);
		if (buf[0] == 0)
			break;
		r.push_back(buf[0]);
	}
	return r;
}