/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "utilities.h"

// geometry vertex with double precision position
struct world_vertex {

// members
    glm::dvec3 position;
    glm::vec3 normal;
    glm::vec2 texture;

// overloads
    // operator+
    template <typename Scalar_>
    world_vertex &
        operator+=( Scalar_ const &Right ) {
            position += Right;
            normal += Right;
            texture += Right;
            return *this; }
    template <typename Scalar_>
    friend
    world_vertex
        operator+( world_vertex Left, Scalar_ const &Right ) {
            Left += Right;
            return Left; }
    // operator*
    template <typename Scalar_>
    world_vertex &
        operator*=( Scalar_ const &Right ) {
            position *= Right;
            normal *= Right;
            texture *= Right;
            return *this; }
    template <typename Type_>
    friend
    world_vertex
        operator*( world_vertex Left, Type_ const &Right ) {
            Left *= Right;
            return Left; }
// methods
    void serialize( std::ostream& ) const;
    void deserialize( std::istream& );
    // wyliczenie współrzędnych i mapowania punktu na środku odcinka v1<->v2
    void
        set_half( world_vertex const &Vertex1, world_vertex const &Vertex2 ) {
            *this =
                interpolate(
                    Vertex1,
                    Vertex2,
                    0.5 ); }
    // wyliczenie współrzędnych i mapowania punktu na odcinku v1<->v2
    void
        set_from_x( world_vertex const &Vertex1, world_vertex const &Vertex2, double const X ) {
            *this =
                interpolate(
                    Vertex1,
                    Vertex2,
                    ( X - Vertex1.position.x ) / ( Vertex2.position.x - Vertex1.position.x ) ); }
    // wyliczenie współrzędnych i mapowania punktu na odcinku v1<->v2
    void
        set_from_z( world_vertex const &Vertex1, world_vertex const &Vertex2, double const Z ) {
            *this =
                interpolate(
                    Vertex1,
                    Vertex2,
                    ( Z - Vertex1.position.z ) / ( Vertex2.position.z - Vertex1.position.z ) ); }
};

template <>
world_vertex &
world_vertex::operator+=( world_vertex const &Right );

template <>
world_vertex &
world_vertex::operator*=( world_vertex const &Right );

//---------------------------------------------------------------------------
