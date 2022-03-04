#pragma once

#include "object.h"
#include "bindable.h"
#include "buffer.h"

#define UBS_PAD(x) uint8_t PAD[x]

namespace gl
{
    class ubo : public buffer
    {
        int index;

    public:
        ubo(int size, int index, GLenum hint = GL_DYNAMIC_DRAW);

        void bind_uniform();

        void update(const uint8_t *data, int offset, int size);
        template <typename T> void update(const T &data, size_t offset = 0)
        {
            update(reinterpret_cast<const uint8_t*>(&data), offset, sizeof(data));
        }
    };

    // layout std140
    // structs must match with GLSL
    // ordered to minimize padding

#pragma pack(push, 1)

    const size_t MAX_TEXTURES = 8;
    const size_t ENVMAP_SIZE = 1024;
    const size_t MAX_CASCADES = 3;
    const size_t HELPER_TEXTURES = 4;
    const size_t SHADOW_TEX = MAX_TEXTURES + 0;
    const size_t ENV_TEX = MAX_TEXTURES + 1;
    const size_t HEADLIGHT_TEX = MAX_TEXTURES + 2;

    struct scene_ubs
    {
        glm::mat4 projection;
        glm::mat4 inv_view;
        glm::mat4 lightview[MAX_CASCADES];
        glm::vec3 cascade_end;
        float time;
    };

    static_assert(sizeof(scene_ubs) == 336, "bad size of ubs");

    const size_t MAX_PARAMS = 3;

    struct model_ubs
    {
        glm::mat4 modelview;
        glm::mat3x4 modelviewnormal;
        glm::vec4 param[MAX_PARAMS];

        glm::mat4 future;
        float opacity;
        float emission;
        float fog_density;
        float alpha_mult;
        float shadow_tone;
        UBS_PAD(12);

        void set_modelview(const glm::mat4 &mv)
        {
            modelview = mv;
            modelviewnormal = glm::mat3x4(glm::mat3(glm::transpose(glm::inverse(mv))));
        }
    };

    static_assert(sizeof(model_ubs) == 208 + 16 * MAX_PARAMS, "bad size of ubs");

    struct light_element_ubs
    {
        enum type_e
        {
            SPOT = 0,
            POINT,
            DIR,
            HEADLIGHTS
        };

        glm::vec3 pos;
        type_e type;

        glm::vec3 dir;
        float in_cutoff;

        glm::vec3 color;
        float out_cutoff;

        float linear;
        float quadratic;

		float intensity;
		float ambient;

        glm::mat4 headlight_projection;
        glm::vec4 headlight_weights;
    };

    static_assert(sizeof(light_element_ubs) == 144, "bad size of ubs");

    const size_t MAX_LIGHTS = 8;

    struct light_ubs
    {
        glm::vec3 ambient;
		UBS_PAD(4);

        glm::vec3 fog_color;
        uint32_t lights_count;

        light_element_ubs lights[MAX_LIGHTS];
    };

    static_assert(sizeof(light_ubs) == 32 + sizeof(light_element_ubs) * MAX_LIGHTS, "bad size of ubs");

#pragma pack(pop)
}
