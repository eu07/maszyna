#pragma pack_matrix(column_major)

struct VertexInput {
  float3 m_Position : Position;
  float4 m_PositionCS : PositionCS;
  float4 m_HistoryPositionCS : HistoryPositionCS;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
  float4 m_PositionSV : SV_Position;
};

struct VertexOutput {
  float3 m_Position : Position;
  float4 m_PositionCS : PositionCS;
  float4 m_HistoryPositionCS : HistoryPositionCS;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
  float4 m_PositionSV : SV_Position;
  uint m_ViewportIndex : SV_RenderTargetArrayIndex;
};

cbuffer CubeDrawConstants : register(b0) {
  float4x4 g_FaceProjections[6];
};

[maxvertexcount(18)]
void main(triangle in VertexInput gs_in[3], inout TriangleStream<VertexOutput> gs_out) {
  for(uint vp = 0; vp < 6; ++vp) {
    for(uint vertex = 0; vertex < 3; ++vertex) {
      VertexOutput output;
      VertexInput input = gs_in[2 - vertex];
      output.m_Position = input.m_Position;
      output.m_PositionCS = mul(g_FaceProjections[vp], float4(output.m_Position * float3(-1., 1., 1.), 1.));
      output.m_HistoryPositionCS = output.m_PositionCS;
      output.m_PositionSV = output.m_PositionCS;
      output.m_Normal = input.m_Normal;
      output.m_TexCoord = input.m_TexCoord;
      output.m_Tangent = input.m_Tangent;
      output.m_ViewportIndex = vp;
      gs_out.Append(output);
    }
    gs_out.RestartStrip();
  }
}
