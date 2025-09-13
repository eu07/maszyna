
StructuredBuffer<float4x3> g_InTransforms : register(t0);
RWStructuredBuffer<float4x3> g_OutCulledTransforms : register(u0);

struct CommandBuffer {
  uint g_IndexCountPerInstance;
  uint g_InstanceCount;
  uint g_StartIndexLocation;
  int  g_BaseVertexLocation;
  uint g_StartInstanceLocation;
};

RWStructuredBuffer<CommandBuffer> g_CommandBuffer : register(u1);

cbuffer ViewData : register(b0) {
  float4 g_FrustumPlanes[6];
  float4x4 g_Projection;
}

cbuffer PushConstants : register(b1) {
  float3 g_Origin;
  float g_InstanceRadius;
  float g_MinRadius;
  float g_MaxRadius;
  uint g_NumInstances;
}

[numthreads(64, 1, 1)]
void main(uint3 DispatchId : SV_DispatchThreadID) {
  if(DispatchId.x >= g_NumInstances) return;
  float4x3 instance = g_InTransforms[DispatchId.x];
  //float3 position = mul(float4(0., 0., 0., 1.), instance) + g_Origin;
  //for(int i = 0; i < 6; ++i) {
  //  if(dot(float4(position, 1.), g_FrustumPlanes[i]) < -g_InstanceRadius) return;
  //}
  //float4 ndc = mul(g_Projection, float4(0., g_InstanceRadius, -length(position), 1.));
  //float radius = ndc.y / ndc.w;
  //if(radius > g_MinRadius && radius <= g_MaxRadius) {
    uint index;
    InterlockedAdd(g_CommandBuffer[0].g_InstanceCount, 1, index);
    g_OutCulledTransforms[index] = instance;
  //}
}
