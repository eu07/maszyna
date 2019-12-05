/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "frustum.h"
#include "scene.h"

// simple camera object. paired with 'virtual camera' in the scene
class opengl_camera {

public:
// constructors
    opengl_camera() = default;
// methods:
    inline
    void
        update_frustum() { update_frustum( m_projection, m_modelview ); }
    inline
    void
        update_frustum(glm::mat4 frustumtest_proj) {
            update_frustum(frustumtest_proj, m_modelview); }
    void
        update_frustum( glm::mat4 const &Projection, glm::mat4 const &Modelview );
    bool
        visible( scene::bounding_area const &Area ) const;
    bool
        visible( TDynamicObject const *Dynamic ) const;
    inline
    glm::dvec3 const &
        position() const { return m_position; }
    inline
    glm::dvec3 &
        position() { return m_position; }
    inline
    glm::mat4 const &
        projection() const { return m_projection; }
    inline
    glm::mat4 &
        projection() { return m_projection; }
    inline
    glm::mat4 const &
        modelview() const { return m_modelview; }
    inline
    glm::mat4 &
        modelview() { return m_modelview; }
    inline
    std::vector<glm::vec4> &
        frustum_points() { return m_frustumpoints; }
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

private:
// members:
    cFrustum m_frustum;
    std::vector<glm::vec4> m_frustumpoints; // visualization helper; corners of defined frustum, in world space
    glm::dvec3 m_position;
    glm::mat4 m_projection;
    glm::mat4 m_modelview;
    glm::mat4 m_inversetransformation; // cached transformation to world space
};

//---------------------------------------------------------------------------
