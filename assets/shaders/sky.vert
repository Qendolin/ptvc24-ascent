#version 450 core

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec3 out_direction;

out gl_PerVertex {
	vec4 gl_Position;
};

uniform mat4 u_rotation_projection_mat;

void main() {
	out_direction = in_position;
	gl_Position = u_rotation_projection_mat * vec4(in_position, 1.);
	gl_Position.z = -gl_Position.w;
}