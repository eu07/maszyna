#version 330 core
  
in vec2 f_coords;

#texture (color_tex, 0, RGB)
uniform sampler2D color_tex;

#texture (velocity_tex, 1, RG)
uniform sampler2D velocity_tex;

#include <common>

void main()
{    
    vec2 texelSize = 1.0 / vec2(textureSize(color_tex, 0));
    vec2 velocity = texture(velocity_tex, f_coords).rg * param[0].r;
    
    float speed = length(velocity / texelSize);
    int nSamples = clamp(int(speed), 1, 64);
    
    vec4 oResult = texture(color_tex, f_coords);
    for (int i = 1; i < nSamples; ++i)
    {
        vec2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5);
        oResult += texture(color_tex, f_coords + offset);
    }
    oResult /= float(nSamples);
    
    gl_FragData[0] = oResult;
}  
