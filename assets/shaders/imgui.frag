#version 430


layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_texture;

void main()
{
	out_color = in_color * texture(u_texture, in_uv.xy);
}