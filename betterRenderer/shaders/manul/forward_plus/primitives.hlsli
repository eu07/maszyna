// Partially stolen from https://www.3dgep.com/forward-plus/

#ifndef FORWARD_PLUS_PRIMITIVES_HLSLI
#define FORWARD_PLUS_PRIMITIVES_HLSLI

/* ---------------------------------------------------------------------------------------------- */
/*                                              Plane                                             */
/* ---------------------------------------------------------------------------------------------- */

struct Plane
{
    float3 N;   // Plane normal.
    float  d;   // Distance to origin.
};

// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes a right-handed (counter-clockwise winding order) 
// coordinate system to determine the direction of the plane normal.
Plane ComputePlane( float3 p0, float3 p1, float3 p2 )
{
    Plane plane;

    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    plane.N = normalize( cross( v0, v2 ) );

    // Compute the distance to the origin using p0.
    plane.d = dot( plane.N, p0 );

    return plane;
}

// Check to see if a point is fully behind (inside the negative halfspace of) a plane.
bool PointInsidePlane( float3 p, Plane plane )
{
    return dot( plane.N, p ) - plane.d < 0;
}

/* ---------------------------------------------------------------------------------------------- */
/*                                             Frustum                                            */
/* ---------------------------------------------------------------------------------------------- */

struct ConeFrustum
{
    float3 m_origin;
    float m_cos_angle;
    float3 m_direction;
    float m_sin_angle;
    float m_cos_angle_sqr;
    float m_inv_sin_angle;
};

ConeFrustum ComputeConeFrustum(in float3 origin, in float3 direction, in float cos_angle) {
  ConeFrustum cone;
  cone.m_origin = origin;
  cone.m_direction = direction;
  cone.m_cos_angle = cos_angle;
  cone.m_cos_angle_sqr = cos_angle * cos_angle;
  cone.m_sin_angle = sqrt(1. - saturate(cone.m_cos_angle_sqr));
  cone.m_inv_sin_angle = 1. / cone.m_sin_angle;
  return cone;
}

// Four planes of a view frustum (in view space).
// The planes are:
//  * Left,
//  * Right,
//  * Top,
//  * Bottom.
// The back and/or front planes can be computed from depth values in the 
// light culling compute shader.
struct Frustum
{
    Plane planes[4];   // left, right, top, bottom frustum planes.
    float3 m_view_ray;
    float m_tan_frustum_angle;
};

/* ---------------------------------------------------------------------------------------------- */
/*                                             Sphere                                             */
/* ---------------------------------------------------------------------------------------------- */

struct Sphere
{
    float3 m_origin;   // Center point.
    float  m_radius;   // Radius.
};

// Check to see if a sphere is fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool SphereInsidePlane( Sphere sphere, Plane plane )
{
    return dot( plane.N, sphere.m_origin ) - plane.d < -sphere.m_radius;
}

// Check to see of a light is partially contained within the frustum.
bool SphereInsideFrustum( Sphere sphere, Frustum frustum, float zNear, float zFar )
{
    bool result = true;

    // First check depth
    // Note: Here, the view vector points in the -Z axis so the 
    // far depth value will be approaching -infinity.
    if ( sphere.m_origin.z - sphere.m_radius > zNear || sphere.m_origin.z + sphere.m_radius < zFar )
    {
        result = false;
    }

    // Then check frustum planes
    for ( int i = 0; i < 4 && result; ++i )
    {
        if ( SphereInsidePlane( sphere, frustum.planes[i] ) )
        {
            result = false;
        }
    }

    return result;
}

/* ---------------------------------------------------------------------------------------------- */
/*                                              Cone                                              */
/* ---------------------------------------------------------------------------------------------- */

struct Cone
{
    float3 T;   // Cone tip.
    float  h;   // Height of the cone.
    float3 d;   // Direction of the cone.
    float  r;   // bottom radius of the cone.
};

// Check to see if a cone if fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool ConeInsidePlane( Cone cone, Plane plane )
{
    // Compute the farthest point on the end of the cone to the positive space of the plane.
    float3 m = cross( cross( plane.N, cone.d ), cone.d );
    float3 Q = cone.T + cone.d * cone.h - m * cone.r;

    // The cone is in the negative halfspace of the plane if both
    // the tip of the cone and the farthest point on the end of the cone to the 
    // positive halfspace of the plane are both inside the negative halfspace 
    // of the plane.
    return PointInsidePlane( cone.T, plane ) && PointInsidePlane( Q, plane );
}

bool ConeInsideFrustum( Cone cone, Frustum frustum, float zNear, float zFar )
{
    bool result = true;

    Plane nearPlane = { float3( 0, 0, -1 ), -zNear };
    Plane farPlane = { float3( 0, 0, 1 ), zFar };

    // First check the near and far clipping planes.
    if ( ConeInsidePlane( cone, nearPlane ) || ConeInsidePlane( cone, farPlane ) )
    {
        result = false;
    }

    // Then check frustum planes
    for ( int i = 0; i < 4 && result; ++i )
    {
        if ( ConeInsidePlane( cone, frustum.planes[i] ) )
        {
            result = false;
        }
    }

    return result;
}

/* ---------------------------------------------------------------------------------------------- */
/*                                              AABB                                              */
/* ---------------------------------------------------------------------------------------------- */

struct Aabb {
    float3 min;
    float3 max;
};

float3 GetAabbCorner( Aabb aabb, float4x4 transform, uint index ) {
  float3 t = float3(
    ( index & 1 ) ? 1. : 0.,
    ( index & 2 ) ? 1. : 0.,
    ( index & 4 ) ? 1. : 0. );
  return mul( 
    transform, 
    float4( ( 1. - t ) * aabb.min + t * aabb.max, 1. ) ).xyz;
}

bool AabbInsidePlane( Aabb aabb, float4x4 transform, Plane plane ) {
  bool result = true;

  for( uint i = 0; i < 8 && result; ++i ) {
    result = PointInsidePlane( GetAabbCorner( aabb, transform, i ), plane );
  }

  return result;
}

bool AabbInsideFrustum( Aabb aabb, float4x4 transform, Frustum frustum, float zNear, float zFar )
{
    bool result = true;

    Plane nearPlane = { float3( 0, 0, -1 ), -zNear };
    Plane farPlane = { float3( 0, 0, 1 ), zFar };

    // First check the near and far clipping planes.
    if ( AabbInsidePlane( aabb, transform, nearPlane ) || AabbInsidePlane( aabb, transform, farPlane ) )
    {
        result = false;
    }

    // Then check frustum planes
    for ( int i = 0; i < 4 && result; ++i )
    {
        if ( AabbInsidePlane( aabb, transform, frustum.planes[i] ) )
        {
            result = false;
        }
    }

    return result;
}

bool DoQueryInfiniteCone(in Sphere sphere, in ConeFrustum cone)
{
  float3 U = cone.m_origin - (sphere.m_radius * cone.m_inv_sin_angle) * cone.m_direction;
  float3 CmU = sphere.m_origin - U;
  float AdCmU = dot(cone.m_direction, CmU);
  if (AdCmU > 0.)
  {
    float sqrLengthCmU = dot(CmU, CmU);
    if (AdCmU * AdCmU >= sqrLengthCmU * cone.m_cos_angle_sqr)
    {
      float3 CmV = sphere.m_origin - cone.m_origin;
      float AdCmV = dot(cone.m_direction, CmV);
      if (AdCmV < -sphere.m_radius)
      {
        return false;
      }

      float rSinAngle = sphere.m_radius * cone.m_sin_angle;
      if (AdCmV >= -rSinAngle)
      {
        return true;
      }

      float sqrLengthCmV = dot(CmV, CmV);
      return sqrLengthCmV <= sphere.m_radius * sphere.m_radius;
    }
  }

  return false;
}

#endif