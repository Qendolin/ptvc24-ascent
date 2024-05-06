#version 430 core

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_tint;

layout(binding = 0) uniform sampler2D u_sprite;

layout(location = 0) out vec4 out_color;

void main() {

    vec4 color = texture(u_sprite, in_uv);
    if(color.a < 0.5)
        discard;
    out_color.a = 1.0;
    out_color.rgb = color.rgb * in_tint.rgb * (1.0 + in_tint.a);
}