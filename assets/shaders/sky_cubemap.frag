#version 450 core

layout(location = 0) in vec3 in_clip_pos;

layout(location = 0) out vec3 out_color;

layout(binding = 0) uniform samplerCube u_hdri_tex;

uniform mat4 u_projection_mat;
uniform mat4 u_view_mat;

// FIXME: This shader is slow. Better to draw an actual cube to render the cube map.
void main() {
	// reconstruct world position from quad
	vec4 view = inverse(u_projection_mat) * vec4(in_clip_pos, 1.0);
	vec4 world = mat4(inverse(mat3(u_view_mat))) * view;
	vec3 dir = normalize(world.xyz);

	out_color = texture(u_hdri_tex, dir).rgb;
}