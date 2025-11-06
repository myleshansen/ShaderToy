#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;

out vec3 v_color;
out vec3 v_position;

void main()
{
    gl_Position = vec4(in_pos, 1.0);
    v_color = in_color;
    v_position = in_pos;
}