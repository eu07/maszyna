// Shamelessly stolen from https://www.3dgep.com/forward-plus/

#include "light.hlsli"
#include "primitives.hlsli"

#define FORWARD_PLUS_TILE_SIZE 8
#define FORWARD_PLUS_LIGHTS_PER_TILE 64

struct ComputeShaderInput
{
  uint3 groupID           : SV_GroupID;
  uint3 groupThreadID     : SV_GroupThreadID;
  uint3 dispatchThreadID  : SV_DispatchThreadID;
  uint  groupIndex        : SV_GroupIndex;
};

/* ---------------------------------------------------------------------------------------------- */
/*                                        Constant Buffers                                        */
/* ---------------------------------------------------------------------------------------------- */

// Global variables
cbuffer DispatchParams : register(b4)
{
  // Number of groups dispatched. (This parameter is not available as an HLSL system value!)
  uint3   numThreadGroups;

  // Total number of threads dispatched. (Also not available as an HLSL system value!)
  // Note: This value may be less than the actual number of threads executed 
  // if the screen size is not evenly divisible by the block size.
  uint3   numThreads;
};

// Parameters required to convert screen space coordinates to view space.
cbuffer ScreenToViewParams : register( b3 )
{
    float4x4 InverseProjection;
    float4x4 WorldToViewSpace;
    float2 ScreenDimensions;
    uint NumLights;
}

/* ---------------------------------------------------------------------------------------------- */
/*                                        Shader Resources                                        */
/* ---------------------------------------------------------------------------------------------- */

// The depth from the screen space texture.
Texture2D<float> DepthTextureVS : register(t3);

StructuredBuffer<PackedLight> Lights : register(t8);

// Precomputed frustums for the grid.
StructuredBuffer<Frustum> in_Frustums : register(t9);

/* ---------------------------------------------------------------------------------------------- */
/*                                     Unordered Access views                                     */
/* ---------------------------------------------------------------------------------------------- */

// View space frustums for the grid cells.
RWStructuredBuffer<Frustum> out_Frustums: register(u0);

// Global counter for current index into the light index list.
// "o_" prefix indicates light lists for opaque geometry while 
// "t_" prefix indicates light lists for transparent geometry.
globallycoherent RWStructuredBuffer<uint> o_LightIndexCounter: register(u1);
globallycoherent RWStructuredBuffer<uint> t_LightIndexCounter: register(u2);

// Light index lists and light grids.
globallycoherent RWStructuredBuffer<uint> o_LightIndexList : register(u3);
globallycoherent RWStructuredBuffer<uint> t_LightIndexList : register(u4);
globallycoherent RWTexture2D<uint2> o_LightGrid : register(u5);
globallycoherent RWTexture2D<uint2> t_LightGrid : register(u6);

/* ---------------------------------------------------------------------------------------------- */
/*                                      Group shared globals                                      */
/* ---------------------------------------------------------------------------------------------- */

groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared Frustum GroupFrustum;

// Opaque geometry light lists.
groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[FORWARD_PLUS_LIGHTS_PER_TILE];

// Transparent geometry light lists.
groupshared uint t_LightCount;
groupshared uint t_LightIndexStartOffset;
groupshared uint t_LightList[FORWARD_PLUS_LIGHTS_PER_TILE];

/* ---------------------------------------------------------------------------------------------- */
/*                                        Utility functions                                       */
/* ---------------------------------------------------------------------------------------------- */

// Add the light to the visible light list for opaque geometry.
void o_AppendLight(uint lightIndex);

// Add the light to the visible light list for transparent geometry.
void t_AppendLight(uint lightIndex);

// Convert clip space coordinates to view space
float4 ClipToView(float4 clip);

// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen);

/* ---------------------------------------------------------------------------------------------- */
/*                                       Frustum computation                                      */
/* ---------------------------------------------------------------------------------------------- */

[numthreads(FORWARD_PLUS_TILE_SIZE, FORWARD_PLUS_TILE_SIZE, 1)]
void CS_ComputeFrustums(in ComputeShaderInput input) {
  // View space eye position is always at the origin.
  const float3 eyePos = (float3)0.;

  // Compute the 4 corner points on the far clipping plane to use as the 
  // frustum vertices.
  float4 screenSpace[5];
  // Top left point
  screenSpace[0] = float4(input.dispatchThreadID.xy * FORWARD_PLUS_TILE_SIZE, -1.0f, 1.0f);
  // Top right point
  screenSpace[1] = float4(float2(input.dispatchThreadID.x + 1, input.dispatchThreadID.y) * FORWARD_PLUS_TILE_SIZE, -1.0f, 1.0f);
  // Bottom left point
  screenSpace[2] = float4(float2(input.dispatchThreadID.x, input.dispatchThreadID.y + 1) * FORWARD_PLUS_TILE_SIZE, -1.0f, 1.0f);
  // Bottom right point
  screenSpace[3] = float4(float2(input.dispatchThreadID.x + 1, input.dispatchThreadID.y + 1) * FORWARD_PLUS_TILE_SIZE, -1.0f, 1.0f);
  // View ray
  screenSpace[4] = float4(float2(input.dispatchThreadID.x + .5, input.dispatchThreadID.y + .5) * FORWARD_PLUS_TILE_SIZE, -1.0f, 1.0f);

  float3 viewSpace[5];
  // Now convert the screen space points to view space
  for (int i = 0; i < 5; i++)
  {
    viewSpace[i] = -ScreenToView(screenSpace[i]).xyz;
  }

  // Now build the frustum planes from the view space points
  Frustum frustum;

  // Left plane
  frustum.planes[0] = ComputePlane(eyePos, viewSpace[2], viewSpace[0]);
  // Right plane
  frustum.planes[1] = ComputePlane(eyePos, viewSpace[1], viewSpace[3]);
  // Top plane
  frustum.planes[2] = ComputePlane(eyePos, viewSpace[0], viewSpace[1]);
  // Bottom plane
  frustum.planes[3] = ComputePlane(eyePos, viewSpace[3], viewSpace[2]);

  frustum.m_view_ray = normalize(viewSpace[4]);
  float cos_frustum_angle = dot(normalize(viewSpace[0]), normalize(viewSpace[4]));
  float sin_frustum_angle = sqrt(1. - saturate(cos_frustum_angle * cos_frustum_angle));
  frustum.m_tan_frustum_angle = sin_frustum_angle / cos_frustum_angle;
  //frustum.cone = ComputeConeFrustum((float3)0., normalize(viewSpace[4]), abs());

  // Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
  if (input.dispatchThreadID.x < numThreads.x && input.dispatchThreadID.y < numThreads.y)
  {
    uint index = input.dispatchThreadID.x + (input.dispatchThreadID.y * numThreads.x);
    out_Frustums[index] = frustum;
  }
}

/* ---------------------------------------------------------------------------------------------- */
/*                                          Light culling                                         */
/* ---------------------------------------------------------------------------------------------- */

float sdSphere( float3 pos, float s )
{
    return length(pos)-s;
}

float sdSolidAngle(float3 pos, float2 c, float ra)
{
    float2 p = float2( length(pos.xz), pos.y );
    //float2 p = float2( length(pos.xz), pos.y );
    float l = length(p) - ra;
	  float m = length(p - c*clamp(dot(p,c),0.0,ra) );
    return max(l,m*sign(c.y*p.x-c.x*p.y));
}

float sdSolidAngleOutside(float3 pos, float2 c, float ra)
{
    float2 p = float2( length(pos.xz), pos.y );
    //float2 p = float2( length(pos.xz), pos.y );
    float l = length(p) - ra;
	  float m = length(p - c*clamp(dot(p,c),0.0,ra) );
    return max(l,-m*sign(c.y*p.x-c.x*p.y));
}


[numthreads(FORWARD_PLUS_TILE_SIZE, FORWARD_PLUS_TILE_SIZE, 1)]
void CS_CullLights(in ComputeShaderInput input) {
  // Calculate min & max depth in threadgroup / tile.
  int2 texCoord = input.dispatchThreadID.xy;
  float fDepth = DepthTextureVS[texCoord].r;

  uint uDepth = asuint(fDepth);

  if (!input.groupIndex) // Avoid contention by other threads in the group.
  {
    uMinDepth = 0xffffffff;
    uMaxDepth = 0;
    o_LightCount = 0;
    t_LightCount = 0;
    GroupFrustum = in_Frustums[input.groupID.x + (input.groupID.y * numThreadGroups.x)];
  }

  GroupMemoryBarrierWithGroupSync();

  InterlockedMin(uMinDepth, uDepth);
  InterlockedMax(uMaxDepth, uDepth);

  GroupMemoryBarrierWithGroupSync();

  float fMinDepth = asfloat(uMaxDepth);
  float fMaxDepth = asfloat(uMinDepth);

  // Convert depth values to view space.
  float minDepthVS = ClipToView(float4(0, 0, fMinDepth, 1)).z;
  float maxDepthVS = ClipToView(float4(0, 0, fMaxDepth, 1)).z;
  float nearClipVS = ClipToView(float4(0, 0, 1, 1)).z;

  // Clipping plane for minimum depth value 
  // (used for testing lights within the bounds of opaque geometry).
  Plane minPlane = { float3(0, 0, -1), -minDepthVS };
  Plane farPlane = { float3(0, 0, 1), maxDepthVS };
  Plane nearPlane = { float3(0, 0, -1), -nearClipVS };

  for (uint i = input.groupIndex; i < NumLights; i += FORWARD_PLUS_TILE_SIZE * FORWARD_PLUS_TILE_SIZE) {
    Light light = UnpackLight(Lights[i]);
    float3 lightPosVS = mul(WorldToViewSpace, float4(light.m_origin, 1.)).xyz;
    Sphere sphere = { lightPosVS, light.m_radius };
    float tan_angle = GroupFrustum.m_tan_frustum_angle;
    float3 rd = GroupFrustum.m_view_ray;
    float dist = nearClipVS / rd.z;
    float maxDist = maxDepthVS / rd.z;
    float minDistOpaque = minDepthVS / rd.z;
    bool intersection = false;
    bool is_opaque = false;
    
    if(light.m_cos_outer < -1.5) {
      while(dist < maxDist) {
        float3 ro = dist * rd;
        float distance = sdSphere(ro - lightPosVS, light.m_radius);
        if(distance <= dist * tan_angle) {
          intersection = true;
          is_opaque = dist > minDistOpaque;
          break;
        }
        dist += distance;
      }

      if(!is_opaque) {
        dist = minDistOpaque - 1.e-2;
        while(dist < maxDist) {
          float3 ro = dist * rd;
          float distance = sdSphere(ro - lightPosVS, light.m_radius);
          if(distance <= dist * tan_angle) {
            is_opaque = true;
            break;
          }
          dist += distance;
        }
      }
    } else {
      float3 lightDirVS = mul((float3x3)WorldToViewSpace, light.m_direction);
      float3 tang = select(lightDirVS.z < .999, float3(0., 0., 1.), float3(0., 1., 0.));
      tang = normalize(tang - lightDirVS * dot(tang, lightDirVS));
      float3 bitang = cross(lightDirVS, tang);

      float3x3 transform = {tang, lightDirVS, bitang};

      float2 angle = {sqrt(1. - light.m_cos_outer * light.m_cos_outer), light.m_cos_outer};
      while(dist < maxDist) {
        float3 ro = dist * rd;
        float distance = sdSolidAngle(mul(transform, ro - lightPosVS), angle, light.m_radius);
        if(distance <= dist * tan_angle) {
          intersection = true;
          is_opaque = dist > minDistOpaque;
          break;
        }
        dist += distance;
      }

      if(!is_opaque) {
        dist = minDistOpaque - 1.e-2;
        while(dist < maxDist) {
          float3 ro = dist * rd;
          float distance = sdSolidAngle(mul(transform, ro - lightPosVS), angle, light.m_radius);
          if(distance <= dist * tan_angle) {
            is_opaque = true;
            break;
          }
          dist += distance;
        }
      }

    }


    if(intersection) {
      t_AppendLight(i);

      if (is_opaque)
      {
        // Add light to light list for opaque geometry.
        o_AppendLight(i);
      }
    }

    //continue;
//
//
    //if(!DoQueryInfiniteCone(sphere, GroupFrustum.cone)) continue;
    //if (dot(light.m_color, (float3)1.)) {
    //  float cosOuterCone = light.m_cos_outer;
//
    //  if (cosOuterCone == 0. || true) { // Point light
    //    if(SphereInsidePlane(sphere, nearPlane) || SphereInsidePlane(sphere, farPlane))
    //      continue;
//
    //    //if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
    //    //{
    //      // Add light to light list for transparent geometry.
    //      t_AppendLight(i);
//
    //      if (!SphereInsidePlane(sphere, minPlane))
    //      {
    //        // Add light to light list for opaque geometry.
    //        o_AppendLight(i);
    //      }
    //    //}
    //  }
    //  else { // Spot light
    //    float sinOuterCone = sqrt(saturate(1. - cosOuterCone * cosOuterCone));
    //    float tanOuterCone = sinOuterCone / cosOuterCone;
    //    float3 lightDirVS = mul((float3x3)WorldToViewSpace, light.m_direction);
    //    float coneRadius =  tanOuterCone * light.m_radius;
    //    Cone cone = { lightPosVS, light.m_radius, lightDirVS, coneRadius };
    //    //if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS) &&
    //    //    ConeInsideFrustum(cone, GroupFrustum, nearClipVS, maxDepthVS))
    //    //{
    //      // Add light to light list for transparent geometry.
    //      t_AppendLight(i);
//
    //      //if (!SphereInsidePlane(sphere, minPlane)
    //      //    && !ConeInsidePlane(cone, minPlane))
    //      //{
    //        // Add light to light list for opaque geometry.
    //        o_AppendLight(i);
    //      //}
    //    //}
    //  }
    //}
  }

  // Wait till all threads in group have caught up.
  GroupMemoryBarrierWithGroupSync();    
  
  // Update global memory with visible light buffer.
  // First update the light grid (only thread 0 in group needs to do this)
  if (!input.groupIndex)
  {
    // Update light grid for opaque geometry.
    InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
    o_LightGrid[input.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);

    // Update light grid for transparent geometry.
    InterlockedAdd(t_LightIndexCounter[0], t_LightCount, t_LightIndexStartOffset);
    t_LightGrid[input.groupID.xy] = uint2(t_LightIndexStartOffset, t_LightCount);
  }

  GroupMemoryBarrierWithGroupSync();    
  
  // Now update the light index list (all threads).
  // For opaque geometry.
  for (uint i = input.groupIndex; i < o_LightCount; i += FORWARD_PLUS_TILE_SIZE * FORWARD_PLUS_TILE_SIZE)
  {
    o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
  }

  // For transparent geometry.
  for (uint i = input.groupIndex; i < t_LightCount; i += FORWARD_PLUS_TILE_SIZE * FORWARD_PLUS_TILE_SIZE)
  {
    t_LightIndexList[t_LightIndexStartOffset + i] = t_LightList[i];
  }
}

/* ---------------------------------------------------------------------------------------------- */
/*                                    Function implementations                                    */
/* ---------------------------------------------------------------------------------------------- */

// Add the light to the visible light list for opaque geometry.
void o_AppendLight(uint lightIndex)
{
  uint index; // Index into the visible lights array.
  InterlockedAdd(o_LightCount, 1, index);
  if (index < FORWARD_PLUS_LIGHTS_PER_TILE)
  {
    o_LightList[index] = lightIndex;
  }
}

// Add the light to the visible light list for transparent geometry.
void t_AppendLight(uint lightIndex)
{
  uint index; // Index into the visible lights array.
  InterlockedAdd(t_LightCount, 1, index);
  if (index < FORWARD_PLUS_LIGHTS_PER_TILE)
  {
    t_LightList[index] = lightIndex;
  }
}

// Convert clip space coordinates to view space
float4 ClipToView( float4 clip )
{
    // View space position.
    float4 view = mul( InverseProjection, clip );
    // Perspective projection.
    view = view / view.w;

    return view;
}

// Convert screen space coordinates to view space.
float4 ScreenToView( float4 screen )
{
    // Convert to normalized texture coordinates
    float2 texCoord = screen.xy / ScreenDimensions;

    // Convert to clip space
    float4 clip = float4( float2( texCoord.x, 1.0f - texCoord.y ) * 2.0f - 1.0f, screen.z, screen.w );

    return ClipToView( clip );
}