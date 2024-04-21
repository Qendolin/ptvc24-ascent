#version 430


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in mat4 in_model_mat;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_uv; 
layout(location = 2) out mat3 out_tbn; // uses location 2-4 because of mat3

uniform mat4 u_view_projection_mat;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
	gl_Position = u_view_projection_mat * in_model_mat * vec4(in_position, 1);
	
	out_position = (in_model_mat * vec4(in_position, 1.0)).xyz;
	out_uv = in_uv;
	// NOTE: Non uniform scaling not supported!
	mat3 normal_matrix = mat3(in_model_mat);
	vec3 T = normalize(normal_matrix * in_tangent.xyz);
	vec3 N = normalize(normal_matrix * in_normal);
	vec3 bitangent = cross(in_normal, in_tangent.xyz) * in_tangent.w;
	vec3 B = normalize(normal_matrix * bitangent);
	out_tbn = mat3(T, B, N);
}