/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

// deserialize little endian uint64
uint64_t sn_utils::ld_uint64(std::istream &s)
{
	uint8_t buf[8];
	s.read((char*)buf, 8);
	uint64_t v = ((uint64_t)buf[7] << 56) | ((uint64_t)buf[6] << 48) |
	             ((uint64_t)buf[5] << 40) | ((uint64_t)buf[4] << 32) |
	             ((uint64_t)buf[3] << 24) | ((uint64_t)buf[2] << 16) |
	             ((uint64_t)buf[1] << 8) | (uint64_t)buf[0];
	return reinterpret_cast<uint64_t&>(v);
}

// deserialize little endian int64
int64_t sn_utils::ld_int64(std::istream &s)
{
	uint8_t buf[8];
	s.read((char*)buf, 8);
	uint64_t v = ((uint64_t)buf[7] << 56) | ((uint64_t)buf[6] << 48) |
	             ((uint64_t)buf[5] << 40) | ((uint64_t)buf[4] << 32) |
	             ((uint64_t)buf[3] << 24) | ((uint64_t)buf[2] << 16) |
	             ((uint64_t)buf[1] << 8) | (uint64_t)buf[0];
	return reinterpret_cast<int64_t&>(v);
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

bool sn_utils::d_bool(std::istream& s)
{
    return ( ld_uint16( s ) == 1 );
}

glm::dvec3 sn_utils::d_dvec3(std::istream& s)
{
    return {
        ld_float64(s),
        ld_float64(s),
        ld_float64(s) };
}

glm::vec3 sn_utils::d_vec3( std::istream& s )
{
	return {
		ld_float32(s),
		ld_float32(s),
		ld_float32(s) };
}

glm::vec4 sn_utils::d_vec4( std::istream& s )
{
    return {
        ld_float32(s),
        ld_float32(s),
        ld_float32(s),
        ld_float32(s) };
}

uint8_t sn_utils::d_uint8( std::istream& s ) {

	uint8_t buf;
	s.read((char*)&buf, 1);
	return buf;
}


void sn_utils::ls_uint16(std::ostream &s, uint16_t v)
{
	uint8_t buf[2];
	buf[0] = v;
	buf[1] = v >> 8;
	s.write((char*)buf, 2);
}

void sn_utils::ls_uint32(std::ostream &s, uint32_t v)
{
	uint8_t buf[4];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	s.write((char*)buf, 4);
}

void sn_utils::ls_int32(std::ostream &s, int32_t v)
{
	uint8_t buf[4];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	s.write((char*)buf, 4);
}

void sn_utils::ls_uint64(std::ostream &s, uint64_t v)
{
	uint8_t buf[8];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	buf[4] = v >> 32;
	buf[5] = v >> 40;
	buf[6] = v >> 48;
	buf[7] = v >> 56;
	s.write((char*)buf, 8);
}

void sn_utils::ls_int64(std::ostream &s, int64_t v)
{
	uint8_t buf[8];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	buf[4] = v >> 32;
	buf[5] = v >> 40;
	buf[6] = v >> 48;
	buf[7] = v >> 56;
	s.write((char*)buf, 8);
}

void sn_utils::ls_float32(std::ostream &s, float t)
{
	uint32_t v = reinterpret_cast<uint32_t&>(t);
	uint8_t buf[4];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	s.write((char*)buf, 4);
}

void sn_utils::ls_float64(std::ostream &s, double t)
{
	uint64_t v = reinterpret_cast<uint64_t&>(t);
	uint8_t buf[8];
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	buf[4] = v >> 32;
	buf[5] = v >> 40;
	buf[6] = v >> 48;
	buf[7] = v >> 56;
	s.write((char*)buf, 8);
}

void sn_utils::s_uint8(std::ostream &s, uint8_t v)
{
    s.write((char*)&v, 1);
}

void sn_utils::s_str(std::ostream &s, std::string v)
{
	const char* buf = v.c_str();
	s.write(buf, v.size() + 1);
}

void sn_utils::s_bool(std::ostream &s, bool v)
{
    ls_uint16(
        s,
        ( true == v ?
            1 :
            0 ) );
}

void sn_utils::s_dvec3(std::ostream &s, glm::dvec3 const &v)
{
    ls_float64(s, v.x);
    ls_float64(s, v.y);
    ls_float64(s, v.z);
}

void sn_utils::s_vec3(std::ostream &s, glm::vec3 const &v)
{
	ls_float32(s, v.x);
	ls_float32(s, v.y);
	ls_float32(s, v.z);
}

void sn_utils::s_vec4(std::ostream &s, glm::vec4 const &v)
{
    ls_float32(s, v.x);
    ls_float32(s, v.y);
    ls_float32(s, v.z);
    ls_float32(s, v.w);
}
