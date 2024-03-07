#version 450 core

layout(location = 0) in vec3 in_clip_pos;

layout(location = 0) out vec3 out_color;

uniform mat4 u_projection_mat;
uniform mat4 u_view_mat;

void main() {
	// reconstruct world position from quad
	vec4 view = inverse(u_projection_mat) * vec4(in_clip_pos, 1.0);
	vec4 world = mat4(inverse(mat3(u_view_mat))) * view;
	vec4 dir = vec4(normalize(world.xyz), 1.0);
	vec4 up = vec4(0, 1, 0, 0);
	vec4 sun = normalize(vec4(0, 1, -1, 0));
	vec3 gamma = vec3(2.2);
	// don't forget to convert sRGB colors to linear
	vec3 col0 = pow(vec3(40, 40, 43) / 255, gamma);
	vec3 col1 = pow(vec3(72, 72, 85) / 255, gamma);
	vec3 col2 = pow(vec3(232, 248, 255) / 255, gamma);
	vec3 col3 = pow(vec3(199, 231, 245) / 255, gamma);
	vec3 col4 = pow(vec3(56, 163, 209) / 255, gamma);
	float theta = dot(normalize(up.xyz), normalize(dir.xyz));
	vec3 color = mix(col0, col1, smoothstep(-1.0, -0.04, theta));
	color = mix(color, col2, smoothstep(-0.04, 0.0, theta));
	color = mix(color, col3, smoothstep(0.0, 0.12, theta));
	color = mix(color, col4, smoothstep(0.12, 1.0, theta));
	color += smoothstep(0.997, 0.999, dot(dir, sun));
	// convert linear to sRGB for display
	out_color = pow(color, vec3(1.0/gamma));
}