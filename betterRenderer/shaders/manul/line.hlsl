#include "color_transform.hlsli"

struct VertexInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
};

struct VertexOutput {
  float3 m_Position : Position;
};

struct PixelInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float4 m_PositionSV : SV_Position;
  float4 m_PositionCS : PositionCS;
  float4 m_HistoryPositionCS : HistoryPositionCS;
};

struct PixelOutput {
  float4 m_Color : SV_Target0;
  float4 m_Emission : SV_Target1;
  float4 m_Params : SV_Target2;
  float4 m_Normal : SV_Target3;
  float2 m_Motion : SV_Target4;
};

cbuffer VertexConstants : register(b0) {
  float4x4 g_JitteredProjection;
  float4x4 g_Projection;
  float4x4 g_ProjectionHistory;
};

struct LinePushConstants {
  float4x3 m_ModelView;
  float4x3 m_ModelViewHistory;
  float3 m_Color;
  float m_LineWeight;
  float m_Metalness;
  float m_Roughness;
};

#ifdef SPIRV
[[vk::push_constant]] ConstantBuffer<LinePushConstants> g_DrawConstants;
#else
cbuffer g_Const : register(b1) { LinePushConstants g_DrawConstants; }
#endif

VertexOutput vtx_main(in VertexInput vs_in) {
  VertexOutput result;
  result.m_Position = vs_in.m_Position;
  return result;
}

#define NUM_LINE_VERTS 4

static const float3 g_VertexNormals[NUM_LINE_VERTS] = {
  float3(-1., 0., 0.), 
  float3(0., 1., 0.), 
  float3(1., 0., 0.), 
  float3(0., -1., 0.) };

PixelInput ComputePoint(in float3 position, in float3 normal);

[maxvertexcount(NUM_LINE_VERTS * 6)]
void geo_main(line VertexOutput gs_in[2], inout TriangleStream<PixelInput> gs_out) {
  float3 normal = normalize(gs_in[1].m_Position - gs_in[0].m_Position);
  float3 up = normal.z < 0.99984769515 ? float3(0., 0., 1.) : float3(0., 1., 0.);
  float3 tangent = normalize(cross(normal, up));
  up = cross(tangent, normal);

  float3x3 offset_matrix = float3x3(tangent, up, normal);
  for(int i = 0; i < NUM_LINE_VERTS; ++i) {

    float3 normal = mul(g_VertexNormals[i], offset_matrix);
    float3 next_normal = mul(g_VertexNormals[(i + 1) % NUM_LINE_VERTS], offset_matrix);

    float3 offset = normal * .5 * g_DrawConstants.m_LineWeight;
    float3 next_offset = next_normal * .5 * g_DrawConstants.m_LineWeight;

    gs_out.Append(ComputePoint(gs_in[0].m_Position + offset, normal));
    gs_out.Append(ComputePoint(gs_in[0].m_Position + next_offset, next_normal));
    gs_out.Append(ComputePoint(gs_in[1].m_Position + offset, normal));
    gs_out.Append(ComputePoint(gs_in[1].m_Position + next_offset, next_normal));
    gs_out.RestartStrip();
  }
};

PixelOutput pix_main(in PixelInput ps_in) {
  PixelOutput result;
  result.m_Color.rgb = REC709_to_XYZ(g_DrawConstants.m_Color);
  result.m_Color.a = 1.;
  result.m_Emission = 0.;
  result.m_Params = float4(g_DrawConstants.m_Metalness, g_DrawConstants.m_Roughness, 1., .5);
  result.m_Normal.rgb = ps_in.m_Normal * .5 + .5;
  result.m_Normal.a = 1.;
  result.m_Motion = (ps_in.m_HistoryPositionCS.xy / ps_in.m_HistoryPositionCS.w) - (ps_in.m_PositionCS.xy / ps_in.m_PositionCS.w);
  result.m_Motion = result.m_Motion * float2(.5, -.5);
  return result;
}

PixelInput ComputePoint(in float3 position, in float3 normal) {
  PixelInput result;
  result.m_Position = mul(float4(position, 1.), g_DrawConstants.m_ModelView).xyz;
  result.m_Normal = mul(float4(normal, 0.), g_DrawConstants.m_ModelView).xyz;
  result.m_PositionSV = mul(g_JitteredProjection, float4(result.m_Position, 1.));
  result.m_PositionCS = mul(g_Projection, float4(result.m_Position, 1.));
  float3 history_position = mul(float4(position, 1.), g_DrawConstants.m_ModelViewHistory);
  result.m_HistoryPositionCS = mul(g_ProjectionHistory, float4(history_position, 1.));
  return result;
}
