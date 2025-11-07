#version 460 core

layout(location = 0) out vec4 fragColor;

in vec2 fragCoord;

uniform float iTime;
uniform vec3 u_color;
uniform vec2 iResolution;

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

    // Output to screen
    fragColor = vec4(col,1.0);
}