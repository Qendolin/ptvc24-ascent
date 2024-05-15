#version 430


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 4) in mat4 in_model_mat;

uniform mat4 u_view_mat;
uniform mat4 u_projection_mat;
uniform float u_size_bias;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = u_projection_mat * u_view_mat * in_model_mat * vec4(in_position - in_normal * u_size_bias, 1);
}