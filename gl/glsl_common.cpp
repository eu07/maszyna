#include "stdafx.h"
#include "glsl_common.h"

std::string gl::glsl_common;

void gl::glsl_common_setup()
{
    glsl_common =
    "#define SHADOWMAP_ENABLED " + std::to_string((int)Global.gfx_shadowmap_enabled) + "\n" +
    "#define ENVMAP_ENABLED " + std::to_string((int)Global.gfx_envmap_enabled) + "\n" +
    "#define MOTIONBLUR_ENABLED " + std::to_string((int)Global.gfx_postfx_motionblur_enabled) + "\n" +
    "#define POSTFX_ENABLED " + std::to_string((int)!Global.gfx_skippipeline) + "\n" +
    "#define EXTRAEFFECTS_ENABLED " + std::to_string((int)Global.gfx_extraeffects) + "\n" +
    "#define MAX_LIGHTS " + std::to_string(MAX_LIGHTS) + "U\n" +
    "#define MAX_CASCADES " + std::to_string(MAX_CASCADES) + "U\n" +
    "#define MAX_PARAMS " + std::to_string(MAX_PARAMS) + "U\n" +
    R"STRING(
    const uint LIGHT_SPOT = 0U;
    const uint LIGHT_POINT = 1U;
    const uint LIGHT_DIR = 2U;
    const uint LIGHT_HEADLIGHTS = 3U;

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

	        float intensity;
	        float ambient;

            mat4 headlight_projection;
            vec4 headlight_weights;
    };

    layout(std140) uniform light_ubo
    {
            vec3 ambient;

            vec3 fog_color;
            uint lights_count;

            light_s lights[MAX_LIGHTS];
    };

    layout (std140) uniform model_ubo
    {
            mat4 modelview;
            mat3 modelviewnormal;
            vec4 param[MAX_PARAMS];

            mat4 future;
            float opacity;
            float emission;
            float fog_density;
            float alpha_mult;
            float shadow_tone;
    };

    layout (std140) uniform scene_ubo
    {
        mat4 projection;
        mat4 inv_view;
        mat4 lightview[MAX_CASCADES];
        vec3 cascade_end;
        float time;
    };

    )STRING";
}
