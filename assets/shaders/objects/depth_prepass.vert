#version 430

layout(location = 0) in vec3 in_position;
layout(location = 4) in mat4 in_model_mat;

uniform mat4 u_view_mat;
uniform mat4 u_projection_mat;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 position_ws = in_model_mat * vec4(in_position, 1.0);
	vec4 position_vs = u_view_mat * position_ws;
	vec4 position_cs = u_projection_mat * position_vs;
	gl_Position = position_cs;
}