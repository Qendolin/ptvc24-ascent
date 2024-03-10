#version 430


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_normal; 
layout(location = 1) out vec2 out_uv; 

uniform mat4 u_view_projection_mat;
uniform mat4 u_model_mat;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
	gl_Position = u_view_projection_mat * u_model_mat * vec4(in_position, 1);
	// NOTE: Non uniform scaling not supported!
	out_normal = mat3(u_model_mat) * in_normal;
	out_uv = in_uv;
}