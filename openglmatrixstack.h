/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <stack>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL/glew.h"
#ifdef _WINDOWS
#include "GL/wglew.h"
#endif

// encapsulation of the fixed pipeline opengl matrix stack
class opengl_matrices {

// types:
class opengl_stack {

public:
// constructors:
    opengl_stack() { m_stack.emplace(1.0f); }

// methods:
    glm::mat4 const &
        data() const { return m_stack.top(); }
    void
        push_matrix() { m_stack.emplace(m_stack.top()); }
    void
        pop_matrix() {
            m_stack.pop();
            if( m_stack.empty() ) { m_stack.emplace(1.0f); }
            upload(); }
    void
        load_identity() {
            m_stack.top() = glm::mat4( 1.0f );
            upload(); }
    void
        rotate( float const Angle, glm::vec3 const &Axis ) {
            m_stack.top() = glm::rotate( m_stack.top(), Angle, Axis );
            upload(); }
    void
        translate( glm::vec3 const &Translation ) {
            m_stack.top() = glm::translate( m_stack.top(), Translation );
            upload(); }
    void
        multiply( glm::mat4 const &Matrix ) {
            m_stack.top() *= Matrix;
            upload(); }
    void
        perspective( float const Fovy, float const Aspect, float const Znear, float const Zfar ) {
            m_stack.top() *= glm::perspective( Fovy, Aspect, Znear, Zfar );
            upload(); }
    void
        look_at( glm::vec3 const &Eye, glm::vec3 const &Center, glm::vec3 const &Up ) {
            m_stack.top() *= glm::lookAt( Eye, Center, Up );
            upload(); }

private:
// types:
    typedef std::stack<glm::mat4> mat4_stack;

// methods:
    void
        upload() { ::glLoadMatrixf( &m_stack.top()[0][0] ); }

// members:
    mat4_stack m_stack;
};

enum stack_mode { gl_projection = 0, gl_modelview = 1 };
typedef std::vector<opengl_stack> openglstack_array;

public:
// constructors:
    opengl_matrices() {
            m_stacks.emplace_back(); // projection
            m_stacks.emplace_back(); // modelview
        }

// methods:
    void
        mode( GLuint const Mode ) {
            switch( Mode ) {
                case GL_PROJECTION: { m_mode = stack_mode::gl_projection; break; }
                case GL_MODELVIEW:  { m_mode = stack_mode::gl_modelview; break; }
                default:            { break; } }
            glMatrixMode( Mode ); }
    glm::mat4 const &
        data( GLuint const Mode = -1 ) const {
            switch( Mode ) {
                case GL_PROJECTION: { return m_stacks[ stack_mode::gl_projection ].data(); }
                case GL_MODELVIEW:  { return m_stacks[ stack_mode::gl_modelview ].data(); }
                default:            { return m_stacks[ m_mode ].data(); } } }
    float const *
        data_array( GLuint const Mode = -1 ) const {
            return glm::value_ptr( data( Mode ) ); }
    void
        push_matrix() { m_stacks[ m_mode ].push_matrix(); }
    void
        pop_matrix() { m_stacks[ m_mode ].pop_matrix(); }
    void
        load_identity() { m_stacks[ m_mode ].load_identity(); }
    template <typename _Type>
    void
        rotate( _Type const Angle, _Type const X, _Type const Y, _Type const Z ) {
            m_stacks[ m_mode ].rotate(
                static_cast<float>(Angle) * 0.0174532925f, // deg2rad
                glm::vec3(
                    static_cast<float>(X),
                    static_cast<float>(Y),
                    static_cast<float>(Z) ) ); }
    template <typename _Type>
    void
        translate( _Type const X, _Type const Y, _Type const Z ) {
            m_stacks[ m_mode ].translate(
                glm::vec3(
                    static_cast<float>( X ),
                    static_cast<float>( Y ),
                    static_cast<float>( Z ) ) ); }
    template <typename _Type>
    void
        multiply( _Type const *Matrix ) {
            m_stacks[ m_mode ].multiply(
                glm::make_mat4( Matrix ) ); }
    template <typename _Type>
    void
        perspective( _Type const Fovy, _Type const Aspect, _Type const Znear, _Type const Zfar ) {
            m_stacks[ m_mode ].perspective(
                static_cast<float>(Fovy) * 0.0174532925f, // deg2rad
                static_cast<float>( Aspect ),
                static_cast<float>( Znear ),
                static_cast<float>( Zfar ) ); }
    template <typename _Type>
    void
        look_at( _Type const Eyex, _Type const Eyey, _Type const Eyez, _Type const Centerx, _Type const Centery, _Type const Centerz, _Type const Upx, _Type const Upy, _Type const Upz ) {
            m_stacks[ m_mode ].look_at(
                glm::vec3(
                    static_cast<float>( Eyex ),
                    static_cast<float>( Eyey ),
                    static_cast<float>( Eyez ) ),
                glm::vec3(
                    static_cast<float>( Centerx ),
                    static_cast<float>( Centery ),
                    static_cast<float>( Centerz ) ),
                glm::vec3(
                    static_cast<float>( Upx ),
                    static_cast<float>( Upy ),
                    static_cast<float>( Upz ) ) ); }

private:
// members:
    stack_mode m_mode{ stack_mode::gl_projection };
    openglstack_array m_stacks;

};

extern opengl_matrices OpenGLMatrices;

// NOTE: standard opengl calls re-definitions
#define glMatrixMode OpenGLMatrices.mode
#define glPushMatrix OpenGLMatrices.push_matrix
#define glPopMatrix OpenGLMatrices.pop_matrix
#define glLoadIdentity OpenGLMatrices.load_identity
#define glRotated OpenGLMatrices.rotate
#define glRotatef OpenGLMatrices.rotate
#define glTranslated OpenGLMatrices.translate
#define glTranslatef OpenGLMatrices.translate
// NOTE: no scale override as we aren't using it anywhere
#define glMultMatrixd OpenGLMatrices.multiply
#define glMultMatrixf OpenGLMatrices.multiply
#define gluPerspective OpenGLMatrices.perspective
#define gluLookAt OpenGLMatrices.look_at

//---------------------------------------------------------------------------
