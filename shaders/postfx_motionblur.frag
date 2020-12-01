in vec2 f_coords;
layout(location = 0) out vec4 out_color;

#texture (color_tex, 0, RGB)
uniform sampler2D color_tex;

#texture (velocity_tex, 1, RG)
uniform sampler2D velocity_tex;

#texture (depth_tex, 2, R)
uniform sampler2D depth_tex;

#include <common>

void main()
{
    float ori_depth = texture(depth_tex, f_coords).x;

    vec2 texelSize = 1.0 / vec2(textureSize(color_tex, 0));
    vec2 velocity = texture(velocity_tex, f_coords).xy * param[0].x;
    
    float speed = length(velocity / texelSize);
    int nSamples = clamp(int(speed), 1, 32);
    
    float depth_diff = 0.001 * 8.0;
    vec4 result = vec4(0.0);

    result += vec4(texture(color_tex, f_coords).rgb, 1.0);

    for (int i = 1; i < nSamples; ++i)
    {
        vec2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5);

        vec2 coord = f_coords + offset;
        float depth = texture(depth_tex, coord).x;

        if (abs(depth - ori_depth) < depth_diff)
            result += vec4(texture(color_tex, coord).rgb, 1.0);
    }

    out_color = vec4((result.rgb / result.w), 1.0);
}
