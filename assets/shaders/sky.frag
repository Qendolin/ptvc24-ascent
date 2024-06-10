#version 450 core

layout(location = 0) in vec3 in_direction;

layout(location = 0) out vec3 out_color;

layout(binding = 0) uniform samplerCube u_hdri_tex;

void main() {
	vec3 dir = normalize(in_direction);
	out_color = texture(u_hdri_tex, dir).rgb;
}