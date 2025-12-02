#version 460 core

layout(location = 0) out vec4 fragColor;
in vec2 fragCoord;

uniform float iTime;
uniform vec4 iDate;
uniform vec3 u_color;
uniform vec2 iResolution;

// BEGIN_USER_CODE
vec3 userColor(vec2 uv)
{
    return 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
}

// END_USER_CODE

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec3 col = userColor(uv);
    fragColor = vec4(col, 1.0);
}
