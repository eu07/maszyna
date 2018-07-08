#include "object.h"
#include "bindable.h"

#define UBS_PAD(x) uint64_t : x * 4; uint64_t : x * 4;

namespace gl
{
    static GLuint next_binding_point;

    class ubo : public object, public bindable<ubo>
    {
    public:
        ubo(int size, int index);
        ~ubo();

        using bindable::bind;
        static void bind(GLuint i);

        void update(void *data, int offset, int size);
    };

    // layout std140
    // structs must match with GLSL
    // ordered to minimize padding

    _Pragma("pack(push, 1)")

    struct scene_ubs
    {
        glm::mat4 projection;
        glm::mat4 lightview;
        float time;
    };

    static_assert(sizeof(scene_ubs) == 132, "bad size of ubs");

    const size_t MAX_PARAMS = 3;

    struct model_ubs
    {
        glm::mat4 modelview;
        glm::mat3x4 modelviewnormal;
        glm::vec4 param[MAX_PARAMS];

        glm::vec3 velocity;
        float opacity;
        float emission;

        void set_modelview(const glm::mat4 &mv)
        {
            modelview = mv;
            modelviewnormal = glm::mat3(glm::transpose(glm::inverse(mv)));
        }
    };

    static_assert(sizeof(model_ubs) == 156 + 8 * MAX_PARAMS, "bad size of ubs");

    struct light_element_ubs
    {
        enum type_e
        {
            SPOT = 0,
            POINT,
            DIR
        };

        glm::vec3 pos;
        type_e type;

        glm::vec3 dir;
        float in_cutoff;

        glm::vec3 color;
        float out_cutoff;

        float linear;
        float quadratic;

        UBS_PAD(8);
    };

    static_assert(sizeof(light_element_ubs) == 64, "bad size of ubs");

    const size_t MAX_LIGHTS = 16;

    struct light_ubs
    {
        glm::vec3 ambient;
        float fog_density;

        glm::vec3 fog_color;
        uint32_t lights_count;

        light_element_ubs lights[MAX_LIGHTS];
    };

    static_assert(sizeof(light_ubs) == 32 + sizeof(light_element_ubs) * MAX_LIGHTS, "bad size of ubs");

    _Pragma("pack(pop)")
}
