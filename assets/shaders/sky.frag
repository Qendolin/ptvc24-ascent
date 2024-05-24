#version 450 core

layout(location = 0) in vec3 in_direction;

layout(location = 0) out vec3 out_color;

layout(binding = 0) uniform samplerCube u_hdri_tex;

void main() {
	out_color = texture(u_hdri_tex, normalize(in_direction)).rgb;
}