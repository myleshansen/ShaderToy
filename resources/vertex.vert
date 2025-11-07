#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;

out vec3 v_color;
out vec2 fragCoord;

void main()
{
    gl_Position = vec4(in_pos, 1.0);
    fragCoord = in_pos.xy;
}