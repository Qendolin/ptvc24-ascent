#version 450 core

layout(location = 0) in vec3 in_direction;

layout(location = 0) out vec3 out_color;

layout(binding = 0) uniform samplerCube u_hdri_tex;

const vec3 FOG_COLOR = vec3(83, 110, 170) / 255.0;

void main() {
	vec3 dir = normalize(in_direction);
	out_color = texture(u_hdri_tex, dir).rgb;
	float fog = max(dot(dir, vec3(0, 1, 0)), 0.0);
	fog = pow(1.0-fog, 5.0);
	out_color = mix(out_color, FOG_COLOR, fog);
}