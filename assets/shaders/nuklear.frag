#version 450 core

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D u_texture;
uniform int u_use_texture;

void main() {
	vec4 color = vec4(1.0);
	if(u_use_texture == 1) {
		color = texture(u_texture, in_uv);
	}
	out_color = in_color * color;
}