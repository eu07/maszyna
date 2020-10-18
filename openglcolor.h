/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Globals.h"

// encapsulation of the fixed pipeline opengl color
class opengl_color {

public:
// constructors:
    opengl_color() = default;

// methods:
    inline
    void
        color3( glm::vec3 const &Color ) {
            return color4( glm::vec4{ Color, 1.f } ); }
    inline
    void
        color3( float const Red, float const Green, float const Blue ) {
            return color3( glm::vec3 { Red, Green, Blue } ); }
    inline
    void
        color3( float const *Value ) {
            return color3( glm::make_vec3( Value ) ); }
    inline
    void
        color4( glm::vec4 const &Color ) {
            if( ( Color != m_color ) || ( false == Global.bUseVBO ) ) {
                m_color = Color;
                ::glColor4fv( glm::value_ptr( m_color ) ); } }
    inline
    void
        color4( float const Red, float const Green, float const Blue, float const Alpha ) {
            return color4( glm::vec4{ Red, Green, Blue, Alpha } ); }
    inline
    void
        color4( float const *Value ) {
            return color4( glm::make_vec4( Value ) );
    }
    inline
    glm::vec4 const &
        data() const {
            return m_color; }
    inline
    float const *
        data_array() const {
            return glm::value_ptr( m_color ); }

private:
// members:
    glm::vec4 m_color { -1 };
};

extern opengl_color OpenGLColor;

// NOTE: standard opengl calls re-definitions
#undef glColor3f
#undef glColor3fv
#undef glColor4f
#undef glColor4fv

#define glColor3f OpenGLColor.color3
#define glColor3fv OpenGLColor.color3
#define glColor4f OpenGLColor.color4
#define glColor4fv OpenGLColor.color4

//---------------------------------------------------------------------------

