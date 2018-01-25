#pragma once

struct opengl_light {

    GLuint id{ (GLuint)-1 };
    glm::vec3 direction;
    glm::vec4
        position { 0.f, 0.f, 0.f, 1.f }, // 4th parameter specifies directional(0) or omni-directional(1) light source
        ambient { 0.f, 0.f, 0.f, 1.f },
        diffuse { 1.f, 1.f, 1.f, 1.f },
        specular { 1.f, 1.f, 1.f, 1.f };

    inline
    void apply_intensity( float const Factor = 1.0f ) {

        if( Factor == 1.0 ) {

            glLightfv( id, GL_AMBIENT, glm::value_ptr(ambient) );
            glLightfv( id, GL_DIFFUSE, glm::value_ptr(diffuse) );
            glLightfv( id, GL_SPECULAR, glm::value_ptr(specular) );
        }
        else {
            // temporary light scaling mechanics (ultimately this work will be left to the shaders
            glm::vec4 scaledambient( ambient.r * Factor, ambient.g * Factor, ambient.b * Factor, ambient.a );
            glm::vec4 scaleddiffuse( diffuse.r * Factor, diffuse.g * Factor, diffuse.b * Factor, diffuse.a );
            glm::vec4 scaledspecular( specular.r * Factor, specular.g * Factor, specular.b * Factor, specular.a );
            glLightfv( id, GL_AMBIENT, glm::value_ptr(scaledambient) );
            glLightfv( id, GL_DIFFUSE, glm::value_ptr(scaleddiffuse) );
            glLightfv( id, GL_SPECULAR, glm::value_ptr(scaledspecular) );
        }
    }
    inline
    void apply_angle() {

        glLightfv( id, GL_POSITION, glm::value_ptr(position) );
        if( position.w == 1.f ) {
            glLightfv( id, GL_SPOT_DIRECTION, glm::value_ptr(direction) );
        }
    }
    inline
    void set_position( glm::vec3 const &Position ) {

        position = glm::vec4( Position, position.w );
    }
};

