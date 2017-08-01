/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "float3d.h"
#include "dumb3d.h"

std::vector<glm::vec4> const ndcfrustumshapepoints = {
    { -1, -1, -1, 1 },{ 1, -1, -1, 1 },{ 1, 1, -1, 1 },{ -1, 1, -1, 1 }, // z-near
    { -1, -1,  1, 1 },{ 1, -1,  1, 1 },{ 1, 1,  1, 1 },{ -1, 1,  1, 1 } }; // z-far

// generic frustum class. used to determine if objects are inside current view area

class cFrustum {

public:
// constructors:

// methods:
	// update the frustum to match current view orientation
    void
        calculate();
	void
        calculate(glm::mat4 const &Projection, glm::mat4 const &Modelview);
	// returns true if specified point is inside of the frustum
    inline
    bool
        point_inside( glm::vec3 const &Point ) const { return point_inside( Point.x, Point.y, Point.z ); }
    inline
    bool
        point_inside( float3 const &Point ) const { return point_inside( Point.x, Point.y, Point.z ); }
    inline
    bool
        point_inside( Math3D::vector3 const &Point ) const { return point_inside( static_cast<float>( Point.x ), static_cast<float>( Point.y ), static_cast<float>( Point.z ) ); }
    bool
        point_inside( float const X, float const Y, float const Z ) const;
	// tests if the sphere is in frustum, returns the distance between origin and sphere centre
    inline
    float
        sphere_inside( glm::vec3 const &Center, float const Radius ) const { return sphere_inside( Center.x, Center.y, Center.z, Radius ); }
    inline
    float
        sphere_inside( float3 const &Center, float const Radius ) const { return sphere_inside( Center.x, Center.y, Center.z, Radius ); }
    inline
    float
        sphere_inside( Math3D::vector3 const &Center, float const Radius ) const { return sphere_inside( static_cast<float>( Center.x ), static_cast<float>( Center.y ), static_cast<float>( Center.z ), Radius ); }
    float
        sphere_inside( float const X, float const Y, float const Z, float const Radius ) const;
	// returns true if specified cube is inside of the frustum. Size = half of the length
    inline
    bool
        cube_inside( glm::vec3 const &Center, float const Size ) const { return cube_inside( Center.x, Center.y, Center.z, Size ); }
    inline
    bool
        cube_inside( float3 const &Center, float const Size ) const { return cube_inside( Center.x, Center.y, Center.z, Size ); }
    inline
    bool
        cube_inside( Math3D::vector3 const &Center, float const Size ) const { return cube_inside( static_cast<float>( Center.x ), static_cast<float>( Center.y ), static_cast<float>( Center.z ), Size ); }
    bool
        cube_inside( float const X, float const Y, float const Z, float const Size ) const;
    // transforms provided set of clip space points to world space
    template <class Iterator_>
    void
        transform_to_world( Iterator_ First, Iterator_ Last ) const {
            std::for_each(
                First, Last,
                [this]( glm::vec4 &point ) {
                    // transform each point using the cached matrix...
                    point = this->m_inversetransformation * point;
                    // ...and scale by transformed w
                    point = glm::vec4{ glm::vec3{ point } / point.w, 1.f }; } ); }
    // debug helper, draws shape of frustum in world space
    void
        draw( glm::vec3 const &Offset ) const;

// members:
    std::vector<glm::vec4> m_frustumpoints; // coordinates of corners for defined frustum, in world space

private:
// types:
    // planes of the frustum
    enum side { side_RIGHT = 0, side_LEFT = 1, side_BOTTOM = 2, side_TOP = 3, side_BACK = 4, side_FRONT = 5 };
    // parameters of the frustum plane: A, B, C define plane normal, D defines distance from origin
    enum plane { plane_A = 0, plane_B = 1, plane_C = 2, plane_D = 3 };

// methods:
	void
        normalize_plane( cFrustum::side const Side );	// normalizes a plane (A side) from the frustum

// members:
	float m_frustum[6][4]; // holds the A B C and D values (normal & distance) for each side of the frustum.
    glm::mat4 m_inversetransformation; // cached inverse transformation matrix
};
