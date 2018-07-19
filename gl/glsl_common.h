#include "ubo.h"

namespace gl
{
    const std::string glsl_common =
    "const uint MAX_LIGHTS = " + std::to_string(MAX_LIGHTS) + "U;\n" +
    "const uint MAX_PARAMS = " + std::to_string(MAX_PARAMS) + "U;\n" +
    "const uint ENVMAP_SIZE = " + std::to_string(ENVMAP_SIZE) + "U;\n" +
    R"STRING(
    const uint LIGHT_SPOT = 0U;
    const uint LIGHT_POINT = 1U;
    const uint LIGHT_DIR = 2U;

    struct light_s
    {
            vec3 pos;
            uint type;

            vec3 dir;
            float in_cutoff;

            vec3 color;
            float out_cutoff;

            float linear;
            float quadratic;
    };

    layout(std140) uniform light_ubo
    {
            vec3 ambient;
            float fog_density;

            vec3 fog_color;
            uint lights_count;

            light_s lights[MAX_LIGHTS];
    };

    layout (std140) uniform model_ubo
    {
            mat4 modelview;
            mat3 modelviewnormal;
            vec4 param[MAX_PARAMS];

            vec3 velocity;
            float opacity;
            float emission;
    };

    layout (std140) uniform scene_ubo
    {
        mat4 projection;
        mat4 lightview;
        float time;
    };

    )STRING";
}
