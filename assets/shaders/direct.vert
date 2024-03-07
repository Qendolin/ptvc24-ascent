#version 430


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_world_position;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_normal;

uniform mat4 u_view_projection_mat;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
	gl_Position = u_view_projection_mat * vec4(in_position, 1);
	out_world_position = in_position;
	out_color = in_color;
	out_normal = in_normal;
}