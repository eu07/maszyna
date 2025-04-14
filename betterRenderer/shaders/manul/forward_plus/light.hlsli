#ifndef FORWARD_PLUS_LIGHT_HLSLI
#define FORWARD_PLUS_LIGHT_HLSLI

struct Light {
  float3 m_origin;
  float m_radius;
  float3 m_direction;
  float3 m_color;
  float m_cos_inner;
  float m_cos_outer;
};

struct PackedLight {
  float3 m_origin;
  float m_radius;
  uint4 m_packed_data;
};

Light UnpackLight(in PackedLight packed_light) {
  Light light;
  light.m_origin = packed_light.m_origin;
  light.m_radius = packed_light.m_radius;
  light.m_direction.x = f16tof32(packed_light.m_packed_data[0]);
  light.m_direction.y = f16tof32(packed_light.m_packed_data[0] >> 16);
  light.m_direction.z = f16tof32(packed_light.m_packed_data[1]);
  light.m_cos_inner = f16tof32(packed_light.m_packed_data[1] >> 16);
  light.m_color.x = f16tof32(packed_light.m_packed_data[2]);
  light.m_color.y = f16tof32(packed_light.m_packed_data[2] >> 16);
  light.m_color.z = f16tof32(packed_light.m_packed_data[3]);
  light.m_cos_outer = f16tof32(packed_light.m_packed_data[3] >> 16);
  return light;
}

#endif